#include "runner.hh"


SpawnRunner::SpawnRunner(JsString& file, JsArray& args, JsObject& options)
        : file_(file),
          args_(args),
          options_(options) {
}

