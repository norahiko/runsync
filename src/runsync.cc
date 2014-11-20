#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include "runsync.hh"

using v8::Array;
using v8::Boolean;
using v8::Handle;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::String;
using v8::Value;

const int TIMEOUT_INTERVAL = 1000 * 20; // microseconds

int64_t MICRO_SCALE = 1000 * 1000;

int64_t tv_to_microseconds(struct timeval* tv) {
    return tv->tv_sec * MICRO_SCALE + tv->tv_usec;
}


/**
 * SpawnRunner class
 */

SpawnRunner::SpawnRunner(const Local<String>& file, const Local<Array>& args, const Local<Object>& options)
        : file_(file),
          args_(args),
          options_(options),
          pid_(0),
          status_(-1),
          timeout_(-1),
          has_timedout_(false) {
    timeout_ = options->Get(NanNew<String>("timeout"))->IntegerValue();
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
        status_ = RunParent();
    } else {
        close(err_pipe_[0]);
        RunChild();
        _exit(127);
    }
}

int SpawnRunner::RunParent() {
    int status;
    if(0 <= timeout_) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        int64_t timeout = timeout_ * 1000;
        int64_t start = tv_to_microseconds(&tv);

        while(waitpid(pid_, &status, WNOHANG) == 0) {
            usleep(TIMEOUT_INTERVAL);
            gettimeofday(&tv, NULL);
            if(timeout < tv_to_microseconds(&tv) - start) {
                kill(pid_, killSignal_);
                has_timedout_ = true;
            }
        }
    } else {
        while (waitpid(pid_, &status, 0) == -1 && errno == EINTR);
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

    for(int child_fd = 0; child_fd < len; child_fd++) {
        Local<Value> io = stdio->Get(NanNew<Number>(child_fd));
        int fd;
        int mode = child_fd == 0 ? O_RDONLY : O_WRONLY;

        if(io->IsNumber()) {
            // inherit
            fd = io->Uint32Value();
            if(fd < 3) {
                ioctl(fd, FIONCLEX);
                if(child_fd <= 2) {
                    int blocK_mode = 0;
                    ioctl(fd, FIONBIO, &blocK_mode);
                }
            }
        } else {
            String::Utf8Value pipe_value(io);
            fd = open(*pipe_value, mode);

            if(fd == -1) {
                SendErrno(*pipe_value);
                return 1;
            }
        }

        if(fd != child_fd) {
            if(dup2(fd, child_fd) == -1) {
                SendErrno("dup");
                return 1;
            }
            close(fd);
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


/**
 * exports method
 */

NAN_METHOD(SpawnSync) {
    NanScope();

    if(args.Length() != 3 ||
       args[0]->IsString() == false ||
       args[1]->IsArray() == false ||
       args[2]->IsObject() == false) {
        return NanThrowTypeError("Invalid arguments");
    }

    Local<String> file = args[0].As<String>();
    Local<Array> spawn_args = args[1].As<Array>();
    Local<Object> options = args[2].As<Object>();

    SpawnRunner runner(file, spawn_args, options);
    runner.Run();
    Local<Object> result = runner.BuildResultObject();
    NanReturnValue(result);
}

void init(Handle<Object> exports) {
    NODE_SET_METHOD(exports, "spawnSync", SpawnSync);
}

NODE_MODULE(runsync, init)
