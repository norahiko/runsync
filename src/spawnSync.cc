#include "spawnSync.hh"

#ifdef _MSC_VER
# include "win/runner.hh"
#else
# include "unix/runner.hh"
#endif

using namespace v8;

Handle<Value> SpawnSync(Args arguments) {
    HandleScope scope;

    if(arguments.Length() != 3 ||
       arguments[0]->IsString() == false ||
       arguments[1]->IsArray() == false ||
       arguments[2]->IsObject() == false) {
        return TypeError("Invalid arguments");
    }

    JsString file = arguments[0].As<String>();
    JsArray args = arguments[1].As<Array>();
    JsObject options = arguments[2].As<Object>();

    SpawnRunner runner(file, args, options);
    JsObject result = runner.Run();
    return scope.Close(result);
}


void init(Handle<Object> exports) {
    NODE_SET_METHOD(exports, "spawnSync", SpawnSync);
}


NODE_MODULE(polyfill, init)
