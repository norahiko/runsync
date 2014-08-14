#include "runsync.hh"
#include "runner.hh"


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

    runsync::SpawnRunner runner(file, spawn_args, options);
    runner.Run();
    Local<Object> result = runner.BuildResultObject();
    NanReturnValue(result);
}


void init(Handle<Object> exports) {
    NODE_SET_METHOD(exports, "spawnSync", SpawnSync);
}


NODE_MODULE(runsync, init)
