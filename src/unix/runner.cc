#include <errno.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include "runner.hh"

namespace runsync {

const int TIMEOUT_INTERVAL = 1000 * 20; // microseconds

const char* tty_device[] = {"/dev/stdin", "/dev/stdout", "/dev/stderr"};

const double MILLI_TO_MICRO = 0.001;

double tv_to_seconds(struct timeval* tv) {
    static double usec = 1.0 / 1000 / 1000;
    return tv->tv_sec + tv->tv_usec * usec;
}


/**
 * SpawnRunner class
 */

SpawnRunner::SpawnRunner(const Local<String>& file, const Local<Array>& args, const Local<Object>& options)
        : file_(file),
          args_(args),
          options_(options),
          pid_(-1),
          status_(-1),
          timeout_(-1),
          has_timedout_(false) {
    Local<Value> timeout_opt = options->Get(NanNew<String>("timeout"));
    if(timeout_opt->IsNumber()) {
        timeout_ = timeout_opt->IntegerValue();
    }
    killSignal_ = options->Get(NanNew<String>("killSignal"))->Uint32Value();
}

void SpawnRunner::Run() {
    if(pipe(err_pipe_) != 0) {
        return;
    }

    pid_t pid = fork();

    if(pid == -1) {
        SendErrno("fork");
        close(err_pipe_[1]);
    } else if(0 < pid) {
        close(err_pipe_[1]);
        pid_ = pid;
        status_ = RunParent(pid);
    } else {
        close(err_pipe_[0]);
        RunChild();
        _exit(127);
    }
}

int SpawnRunner::RunParent(pid_t pid) {
    int status;
    if(0 <= timeout_) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        double timeout = timeout_ * MILLI_TO_MICRO;
        double start = tv_to_seconds(&tv);

        while(waitpid(pid, &status, WNOHANG) == 0) {
            usleep(TIMEOUT_INTERVAL);
            gettimeofday(&tv, NULL);
            if(timeout < tv_to_seconds(&tv) - start) {
                kill(pid, killSignal_);
                has_timedout_ = true;
            }
        }
    } else {
        waitpid(pid, &status, 0);
    }
    return status;
}

int SpawnRunner::RunChild() {
    if(PipeStdio() ||
       SetEnvironment() ||
       ChangeDirectory() ||
       SetUID() ||
       SetGID()) {
        return 1;
    }
    String::Utf8Value file_value(file_);
    char** args = BuildArgs();
    execvp(*file_value, args);
    SendErrno(*file_value);
    return 1;
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

void SpawnRunner::SendErrno(const char* msg) {
    char str[1024];
    snprintf(str, 1022, "%d %s", errno, msg);
    ssize_t size = write(err_pipe_[1], str, strlen(str) + 1);
    assert(0 < size);
}

int SpawnRunner::PipeStdio() {
    Local<Array> stdio = options_->Get(NanNew<String>("stdio")).As<Array>();
    int len = static_cast<int>(stdio->Length());

    for(int fileno = 0; fileno < len; fileno++) {
        Local<Value> pipe = stdio->Get(NanNew<Number>(fileno));
        int fd;
        int mode = fileno == 0 ? O_RDONLY : O_WRONLY;

        if(pipe->IsNumber()) {
            fd = pipe->Uint32Value();
            if(fd < 3) {
                fd = open(tty_device[fd], mode);
            }
        } else {
            String::Utf8Value pipe_value(pipe);
            if(strcmp(*pipe_value, "ignore") == 0) {
                fd = open("/dev/null", mode);
            } else {
                fd = open(*pipe_value, mode);
            }

            if(fd == -1) {
                SendErrno(*pipe_value);
                return 1;
            }
        }

        if(fd != fileno && dup2(fd, fileno) == -1) {
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

int SpawnRunner::SetUID() {
    Local<Value> uid = options_->Get(NanNew<String>("uid"));
    if(uid->IsUndefined() || uid->IsNull()) {
        return 0;
    } else if(uid->IsNumber() == false) {
        errno = EPERM;
        SendErrno("setuid");
        return 1;
    } else if(setuid(uid->Uint32Value()) == -1) {
        SendErrno("setuid");
        return 1;
    }
    return 0;
}

int SpawnRunner::SetGID() {
    Local<Value> gid = options_->Get(NanNew<String>("gid"));
    if(gid->IsUndefined() || gid->IsNull()) {
        return 0;
    } else if(gid->IsNumber() == false) {
        errno = EPERM;
        SendErrno("setgid");
        return 1;
    } else if(setgid(gid->Uint32Value()) == -1) {
        SendErrno("setgid");
        return 1;
    }
    return 0;
}

Local<Object> SpawnRunner::BuildResultObject() {
    Local<Object> result = NanNew<Object>();
    int exit_status = 0;
    bool pipe_closed = false;

    if(WIFEXITED(status_)) {
        exit_status = WEXITSTATUS(status_);
        result->Set(NanNew<String>("signal"), NanNull());
    } else if(WIFSIGNALED(status_)) {
        int sig = WTERMSIG(status_);
        close(err_pipe_[0]);
        pipe_closed = true;
        result->Set(NanNew<String>("signal"), NanNew<String>(node::signo_string(sig)));
        exit_status = 128 + sig;
    }

    result->Set(NanNew<String>("status"), NanNew<Number>(exit_status));
    result->Set(NanNew<String>("pid"), NanNew<Number>(pid_));
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

} // namespace runsync
