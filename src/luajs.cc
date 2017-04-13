#include <node.h>
#include "luastate.h"

namespace luajs {

  using v8::FunctionCallbackInfo;
  using v8::Isolate;
  using v8::Local;
  using v8::Object;
  using v8::String;
  using v8::Value;

  static void LuaVersion(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, LUA_VERSION));
  }

  void Initialize(Local<Object> exports) {
      NODE_SET_METHOD(exports, "luaVersion", LuaVersion);
      LuaState::Init(exports);
  }

  NODE_MODULE(luajs, Initialize)
}