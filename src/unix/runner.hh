#ifndef UNIX_SPAWN_RUNNER
#define UNIX_SPAWN_RUNNER

#include "../spawnSync.hh"

class SpawnRunner {
    public:
        SpawnRunner(JsString&, JsArray&, JsObject&);
        JsObject Run();

    private:
        JsString file_;
        JsArray args_;
        JsObject options_;

        char* exec_file_;
        JsObject env_;
        int64_t timeout_; // milliseconds

        int status_;
        pid_t child_pid_;
        bool has_timedout_;


        JsObject BuildResultObject(int);
        char** BuildArgs();
        int RunChild();
        int RunParent(pid_t);
        int SetEnvironment();
        int PipeStdio();
        int ChangeDirectory();
};
#endif
