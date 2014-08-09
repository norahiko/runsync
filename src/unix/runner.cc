#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <iostream>
#include "runner.hh"

namespace runsync {

const int TIMEOUT_INTERVAL = 1000 * 20; // microseconds

double tv_to_seconds(struct timeval* tv) {
    static double usec = 1.0 / 1000 / 1000;
    return tv->tv_sec + tv->tv_usec * usec;
}

SpawnRunner::SpawnRunner(const Local<String>& file, const Local<Array>& args, const Local<Object>& options)
        : file_(file),
          args_(args),
          options_(options),
          timeout_(-1),
          has_timedout_(false) {
    Local<Value> timeout_opt = options->Get(NanNew<String>("timeout"));
    if(timeout_opt->IsNumber()) {
        timeout_ = timeout_opt->IntegerValue();
    }
}

Local<Object> SpawnRunner::Run() {
    int err = pipe(err_pipe_);
    assert(err == 0);

    pid_t pid = fork();
    if(pid == 0) {
        close(err_pipe_[0]);
        RunChild();
        _exit(127);
    } else {
        close(err_pipe_[1]);
        int stat = RunParent(pid);
        return BuildResultObject(stat, pid);
    }
    return BuildResultObject(0, pid);
}

int SpawnRunner::RunChild() {
    if(PipeStdio()) { return 1; }
    if(SetEnvironment()) { return 1; }
    if(ChangeDirectory()) { return 1; }
    String::Utf8Value file(file_);
    char** args = BuildArgs();
    execvp(*file, args);
    SendErrno(*file);
    return 1;
}

int SpawnRunner::RunParent(pid_t pid) {
    int stat;
    if(0 <= timeout_) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        double timeout = timeout_ * 0.001;
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

Local<Object> SpawnRunner::BuildResultObject(int stat, pid_t pid) {
    Local<Object> result = NanNew<Object>();
    int status = 0;
    bool pipe_closed = false;

    if(WIFEXITED(stat)) {
        status = WEXITSTATUS(stat);
        result->Set(NanNew<String>("signal"), NanNull());
    } else if(WIFSIGNALED(stat)) {
        int sig = WTERMSIG(stat);
        close(err_pipe_[0]);
        pipe_closed = true;
        result->Set(NanNew<String>("signal"), NanNew<String>(node::signo_string(sig)));
        status = 128 + sig;
    }

    result->Set(NanNew<String>("status"), NanNew<Number>(status));
    result->Set(NanNew<String>("pid"), NanNew<Number>(pid));
    result->Set(NanNew<String>("file"), file_);
    result->Set(NanNew<String>("args"), args_);
    if(has_timedout_) {
        result->Set(NanNew<String>("_hasTimedOut"), NanNew<Boolean>(has_timedout_));
    }

    if(pipe_closed == false) {
        char errmsg[1024];
        int read_size = read(err_pipe_[0], errmsg, 1024);
        if(0 < read_size) {
            result->Set(NanNew<String>("_errmsg"), NanNew<String>(errmsg));
        }
        close(err_pipe_[0]);
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
    Local<Array> stdio = options_->Get(NanNew<String>("stdio")).As<Array>();
    uint32_t len = stdio->Length();
    static const char* devfiles[3] = {"/dev/stdin", "/dev/stdout", "/dev/stderr"};

    for(uint32_t fileno = 0; fileno < len; fileno++) {
        Local<Value> pipe = stdio->Get(NanNew<Number>(fileno));
        int fd;
        if(pipe->IsNumber()) {
            fd = pipe->IntegerValue();

        } else {
            String::Utf8Value pipe_value(pipe);
            int mode = fileno == 0 ? O_RDONLY : O_WRONLY;
            if(strcmp(*pipe_value, "inherit") == 0) {
                if(fileno < 3) {
                    fd = open(devfiles[fileno], mode);
                } else {
                    fd = fileno;
                }
            } else if(strcmp(*pipe_value, "ignore") == 0) {
                fd = open("/dev/null", mode);
            } else {
                fd = open(*pipe_value, mode);
            }

            if(fd == -1) {
                SendErrno(*pipe_value);
                return 1;
            }
        }
        if(dup2(fd, fileno) == -1) {
            SendErrno("dup");
            return 1;
        }
    }
    return 0;
}

int SpawnRunner::SetEnvironment() {
    Local<Array> envPairs = options_->Get(NanNew<String>("envPairs")).As<Array>();
    int len = envPairs->Length();
    for(int i = 0; i < len; i++) {
        String::Utf8Value pair_value(envPairs->Get(i));
        int len = pair_value.length();

        char *pair = new char[len + 1];
        strncpy(pair, *pair_value, len);
        putenv(pair);
    }
    return 0;
}

int SpawnRunner::ChangeDirectory() {
    Local<Value> cwd = options_->Get(NanNew<String>("cwd"));
    if(cwd->IsString()) {
        String::Utf8Value cwd_value(cwd);
        int err = chdir(*cwd_value);
        if(err) {
            SendErrno(*cwd_value);
            return err;
        }
    }
    return 0;
}

void SpawnRunner::SendErrno(const char* msg) {
    char str[1024];
    snprintf(str, 1022, "%d %s", errno, msg);
    ssize_t size = write(err_pipe_[1], str, strlen(str) + 1);
    assert(0 < size);

}

} // namespace runsync
