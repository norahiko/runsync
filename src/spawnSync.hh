#ifndef SPAWN_SYNC_POLYFILL_H
#define SPAWN_SYNC_POLYFILL_H

#include <node.h>

#define TypeError(msg) ThrowException(Exception::TypeError(String::New(msg)));
#define Symbol(str) v8::String::NewSymbol(str)

typedef const v8::Arguments&  Args;
typedef v8::Local<v8::Value> JsValue;
typedef v8::Local<v8::String> JsString;
typedef v8::Local<v8::Array>  JsArray;
typedef v8::Local<v8::Object> JsObject;
#endif
