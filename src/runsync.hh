#ifndef RUNSYNC_H_
#define RUNSYNC_H_
#include <node.h>
#include <nan.h>


class SpawnRunner {
    public:
        SpawnRunner(
            const v8::Local<v8::String>&,
            const v8::Local<v8::Array>&,
            const v8::Local<v8::Object>&
        );
        void Run();
        v8::Local<v8::Object> BuildResultObject();

    private:
        int RunParent();
        int RunChild();
        char** BuildArgs();
        void SendErrno(const char*);
        int PipeStdio();
        int SetEnvironment();
        int ChangeDirectory();
        int SetUID();
        int SetGID();

        v8::Local<v8::String> file_;
        v8::Local<v8::Array> args_;
        v8::Local<v8::Object> options_;
        int pid_;
        int status_;
        int err_pipe_[2];
        char* exec_file_;
        int64_t timeout_; // milliseconds
        bool has_timedout_;
        int killSignal_;
};

#endif
