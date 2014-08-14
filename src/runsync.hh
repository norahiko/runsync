#ifndef RUNSYNC_H_
#define RUNSYNC_H_
#include <node.h>
#include <nan.h>

using v8::Array;
using v8::Boolean;
using v8::Handle;
using v8::Local;
using v8::Null;
using v8::Number;
using v8::Object;
using v8::String;
using v8::Value;


namespace runsync {


class SpawnRunner {
    public:
        SpawnRunner(const Local<String>&, const Local<Array>&, const Local<Object>&);
        void Run();
        Local<Object> BuildResultObject();

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

        Local<String> file_;
        Local<Array> args_;
        Local<Object> options_;
        int pid_;
        int status_;
        int err_pipe_[2];
        char* exec_file_;
        int64_t timeout_; // milliseconds
        bool has_timedout_;
        int killSignal_;
};

} // namespace runsync
#endif
