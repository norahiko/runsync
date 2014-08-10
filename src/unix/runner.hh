#ifndef RUNSYNC_RUNNER_H_
#define RUNSYNC_RUNNER_H_

#include "../runsync.hh"

namespace runsync {

class SpawnRunner {
    public:
        SpawnRunner(const Local<String>&, const Local<Array>&, const Local<Object>&);
        Local<Object> Run();

    private:
        Local<Object> BuildResultObject(int, pid_t);
        char** BuildArgs();
        int RunChild();
        int RunParent(pid_t);
        int SetEnvironment();
        int PipeStdio();
        int ChangeDirectory();
        int SetUID();
        void SendErrno(const char*);

        Local<String> file_;
        Local<Array> args_;
        Local<Object> options_;

        int err_pipe_[2];
        char* exec_file_;
        int64_t timeout_; // milliseconds
        bool has_timedout_;
};

} // namespace runsync
#endif
