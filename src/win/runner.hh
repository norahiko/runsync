#ifndef WIN_SPAWN_RUNNER
#define WIN_SPAWN_RUNNER
#include "../spawnSync.hh"


class SpawnRunner {
    public:
        SpawnRunner(JsString&, JsArray&, JsObject&);
        JsObject Run();

    private:
        JsString file_;
        JsArray args_;
        JsObject options_;


};


#endif
