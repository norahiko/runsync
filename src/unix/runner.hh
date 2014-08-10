#ifndef RUNSYNC_RUNNER_H_
#define RUNSYNC_RUNNER_H_

#include "../runsync.hh"

namespace runsync {

class SpawnRunner {
    public:
        SpawnRunner(const Local<String>&, const Local<Array>&, const Local<Object>&);
        void Run();
        Local<Object> BuildResultObject();

    private:
        int RunParent(pid_t);
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
};

} // namespace runsync
#endif
