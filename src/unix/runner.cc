#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include "runner.hh"
#include <iostream>

using namespace v8;
using namespace std;

const double usec = 1.0 / 1000 / 1000;

double tv_to_seconds(struct timeval* tv) {
    return tv->tv_sec + tv->tv_usec * usec;
}

SpawnRunner::SpawnRunner(JsString& file, JsArray& args, JsObject& options)
        : file_(file),
          args_(args),
          options_(options),

          timeout_(0),
          status_(0),
          child_pid_(-1),
          has_timedout_(false) {
    JsValue timeout_opt = options->Get(Symbol("timeout"));
    if(timeout_opt->IsNumber()) {
        timeout_ = static_cast<int64_t>(timeout_opt->IntegerValue());
    }
}


JsObject SpawnRunner::Run() {
    pid_t pid = fork();
    if(pid == 0) {
        RunChild();
        _exit(127);
    } else {
        int stat = RunParent(pid);
        return BuildResultObject(stat);
    }
    return BuildResultObject(0);
}

int SpawnRunner::RunChild() {
    if(PipeStdio()) { return 1; }
    if(SetEnvironment()) { return 1; }
    if(ChangeDirectory()) { return 1; }
    String::Utf8Value file(file_);
    char** args = BuildArgs();
    execvp(*file, args);
    fprintf(stderr, "errno: %d\n", errno);
    perror(*file);
    return 1;
}

int SpawnRunner::RunParent(pid_t pid) {
    child_pid_ = pid;
    int stat;
    if(0 < timeout_) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        double timeout = timeout_ / 1000.0;
        double start = tv_to_seconds(&tv);

        while(waitpid(pid, &stat, WNOHANG) == 0) {
            usleep(TIMEOUT_INTERVAL);
            gettimeofday(&tv, NULL);
            if(timeout < tv_to_seconds(&tv) - start) {
                kill(pid, SIGTERM);
                has_timedout_ = true;
            }
        }
    } else {
        waitpid(pid, &stat, 0);
    }
    return stat;
}

JsObject SpawnRunner::BuildResultObject(int stat) {
    Local<Object> result = Object::New();

    if(WIFEXITED(stat)) {
        status_ = WEXITSTATUS(stat);
        result->Set(Symbol("signal"), Null());
    } else if(WIFSIGNALED(stat)) {
        int sig = WTERMSIG(stat);
        JsString signame = String::New(node::signo_string(sig));
        result->Set(Symbol("signal"), signame);
        status_ = 128 + sig;
    }

    result->Set(Symbol("status"), Number::New(status_));
    result->Set(Symbol("pid"), Number::New(child_pid_));
    result->Set(Symbol("file"), file_);
    result->Set(Symbol("args"), args_);
    if(has_timedout_) {
        result->Set(Symbol("_hasTimedOut"), Boolean::New(has_timedout_));
    }
    return result;
}

char** SpawnRunner::BuildArgs() {
    int len = args_->Length();
    char** args = new char*[len + 1];

    for(int i = 0; i < len; i++) {
        String::Utf8Value value(args_->Get(i));
        char* str = new char[value.length() + 1];
        strcpy(str, *value);
        args[i] = str;
    }

    // add sentinel
    args[len] = NULL;
    return args;
}

int SpawnRunner::PipeStdio() {
    JsArray stdio = options_->Get(Symbol("stdio")).As<Array>();

    const char* files[3] = {"/dev/stdin", "/dev/stdout", "/dev/stderr"};
    const char* names[3] = {"stdin pipe", "stdout pipe", "stderr pipe"};
    int modes[3] = {O_RDONLY, O_WRONLY, O_WRONLY};

    for(int i = 0; i < 3; i++) {
        JsValue pipe = stdio->Get(Number::New(i));
        int fd;

        if(pipe->IsNumber()) {
            fd = pipe->IntegerValue();
        } else {
            fd = open(files[i], modes[i]);
            if(fd == -1) {
                fprintf(stderr, "errno: %d\n", errno);
                perror(files[i]);
                return 1;
            }
       }
       if(dup2(fd, i) == -1) {
           fprintf(stderr, "errno: %d\n", errno);
           perror(names[i]);
           return 1;
       }
    }
    return 0;
}

int SpawnRunner::SetEnvironment() {
    JsArray envPairs = options_->Get(Symbol("envPairs")).As<Array>();
    int len = envPairs->Length();
    for(int i = 0; i < len; i++) {
        String::Utf8Value value(envPairs->Get(i));
        putenv(*value);
    }

    return 0;
}

int SpawnRunner::ChangeDirectory() {
    JsValue cwd = options_->Get(Symbol("cwd"));
    if(cwd->IsString()) {
        String::Utf8Value cwd_value(cwd);
        int err = chdir(*cwd_value);
        if(err) {
            fprintf(stderr, "errno: %d\n", errno);
            perror(*cwd_value);
            return err;
        }
    }
    return 0;
}
