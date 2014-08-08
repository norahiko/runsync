#include "spawnSync.hh"

#ifdef _MSC_VER
# include "win/runner.hh"
#else
# include "unix/runner.hh"
#endif

using namespace v8;

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
    Local<Object> result = runner.Run();
    NanReturnValue(result);
}


void init(Handle<Object> exports) {
    NODE_SET_METHOD(exports, "spawnSync", SpawnSync);
}


NODE_MODULE(polyfill, init)
