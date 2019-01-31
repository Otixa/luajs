//
// Created by Lukas Kollmer on 12.04.17.
//

#include <map>
#include <set>
#include <string>
#include <algorithm>
#include "luastate.h"
#include "luajs_utils.h"
#include <nan.h>
#include <iostream>
#include "asprintf.h"

using namespace v8;

using ResolverPersistent = Nan::Persistent<v8::Promise::Resolver>;
using FunctionPersistent = Nan::Persistent<v8::Function, v8::NonCopyablePersistentTraits<v8::Function>>;

std::set<std::string> luaStateNames;
std::map<std::string, FunctionPersistent> functions;

static bool NameExists(std::string name)
{
  return luaStateNames.find(name) != luaStateNames.end();
}

#define CHECK_LUA_STATE_IS_OPEN(isolate, obj)                                                  \
  if (obj->isClosed_)                                                                          \
  {                                                                                            \
    char *errorMsg;                                                                            \
    asprintf(&errorMsg, "Error: LuaState %s is closed\n", obj->name_);                         \
    isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(isolate, errorMsg))); \
    free(errorMsg);                                                                            \
    return;                                                                                    \
  }

#define lua_do(func)                                                              \
  void async_##func(uv_work_t *req)                                               \
  {                                                                               \
    async_lua_worker *worker = static_cast<async_lua_worker *>(req->data);        \
    if (func(worker->state->GetLuaState(), (char *)worker->data))                 \
    {                                                                             \
      worker->error = true;                                                       \
      sprintf(worker->msg, "%s", lua_tostring(worker->state->GetLuaState(), -1)); \
    }                                                                             \
  }

namespace luajs
{

  struct async_lua_worker
  {
    Isolate *isolate;
    ResolverPersistent *persistent;
    void *data;
    bool error;
    char msg[1000];
    luajs::LuaState *state;
    int successRetval;
    bool returnFromStack = true;
  };

  void async_after(uv_work_t *req, int status)
  {
    async_lua_worker *worker = static_cast<async_lua_worker *>(req->data);
    HandleScope scope(worker->isolate);

    auto resolver = Nan::New(*worker->persistent);

    if (worker->error)
    {
      resolver->Reject(Nan::New(worker->msg).ToLocalChecked());
    }
    else
    {
      if (worker->returnFromStack)
      {
        Local<Value> retval = ValueFromLuaObject(worker->isolate, worker->state->GetLuaState(), -1);
        resolver->Resolve(retval);
      }
      else
      {
        resolver->Resolve(Nan::New(worker->successRetval));
      }
    }

    worker->persistent->Reset();
    delete worker->persistent;
  }

  lua_do(luaL_dostring)
    lua_do(luaL_dofile)

    using node::ObjectWrap;

  Persistent<Function> LuaState::constructor;

  LuaState* LuaState::instance = 0;
  LuaState* LuaState::getCurrentInstance() {

    return LuaState::instance;
  }
  void LuaState::setCurrentInstance(LuaState* instance) {
    LuaState::instance = instance;
  }

  LuaState::LuaState(lua_State *state, const char *name) : lua_(state), name_(name), isClosed_(false)
  {
    luaStateNames.insert(std::string(name));
  }

  LuaState::~LuaState() {}

  void LuaState::Init(Local<Object> exports)
  {

    Isolate *isolate = exports->GetIsolate();

    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFromUtf8(isolate, "LuaState"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    NODE_SET_PROTOTYPE_METHOD(tpl, "close", Close);
    NODE_SET_PROTOTYPE_METHOD(tpl, "reset", Reset);

    NODE_SET_PROTOTYPE_METHOD(tpl, "doString", DoString);
    NODE_SET_PROTOTYPE_METHOD(tpl, "doFile", DoFile);

    NODE_SET_PROTOTYPE_METHOD(tpl, "toValue", ToValue);

    NODE_SET_PROTOTYPE_METHOD(tpl, "doStringSync", DoStringSync);
    NODE_SET_PROTOTYPE_METHOD(tpl, "doFileSync", DoFileSync);
    NODE_SET_PROTOTYPE_METHOD(tpl, "getGlobal", GetGlobal);
    NODE_SET_PROTOTYPE_METHOD(tpl, "setGlobal", SetGlobal);
    NODE_SET_PROTOTYPE_METHOD(tpl, "registerFunction", RegisterFunction);
    NODE_SET_PROTOTYPE_METHOD(tpl, "getStatus", GetStatus);

    //NODE_SET_PROTOTYPE_METHOD(tpl, "loadString", LoadString);
    //NODE_SET_PROTOTYPE_METHOD(tpl, "loadStringSync", LoadStringSync);

    constructor.Reset(isolate, tpl->GetFunction());
    exports->Set(String::NewFromUtf8(isolate, "LuaState"), tpl->GetFunction());
  }

  void LuaState::New(const FunctionCallbackInfo<Value> &args)
  {
    Isolate *isolate = args.GetIsolate();
    HandleScope scope(isolate);

    char *name;

    if (!args[0]->IsString())
    {
      do
      {
        name = (char *)RandomUUID();
      } while (NameExists(std::string(name)));
    }
    else
    {
      name = (char *)ValueToChar(isolate, args[0]);
    }

    if (NameExists(std::string(name)))
    {
      char *excMessage;
      asprintf(&excMessage, "Error: LuaState with name '%s' already exists", name);
      isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(isolate, excMessage)));
      free(excMessage);
      return;
    }

    if (args.IsConstructCall())
    {
      LuaState *obj = new LuaState(luaL_newstate(), name);
      luaL_openlibs(obj->lua_);

      obj->Wrap(args.This());
      args.GetReturnValue().Set(args.This());
    }
  }

  void LuaState::Reset(const v8::FunctionCallbackInfo<v8::Value> &args)
  {
    Isolate *isolate = args.GetIsolate();
    HandleScope scope(isolate);
    LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

    CHECK_LUA_STATE_IS_OPEN(isolate, obj);

    obj->Close(args);
    obj->lua_ = luaL_newstate();
    obj->isClosed_ = false;
  }

  void LuaState::Close(const v8::FunctionCallbackInfo<v8::Value> &args)
  {
    Isolate *isolate = args.GetIsolate();
    HandleScope scope(isolate);
    LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

    CHECK_LUA_STATE_IS_OPEN(isolate, obj);

    lua_close(obj->lua_);
    obj->lua_ = NULL;
    obj->isClosed_ = true;
  }

  Local<Array> GetDebug(Isolate *isolate, lua_State *L)
  {
    Local<Array> out = Array::New(isolate);
    lua_Debug info;
    int level = 0;
    while (lua_getstack(L, level, &info))
    {
      lua_getinfo(L, "nSl", &info);
      Local<String> str = String::Concat(
        String::Concat(Integer::New(isolate, level)->ToString(), String::NewFromUtf8(isolate, "| ")),
        String::Concat(String::NewFromUtf8(isolate, info.short_src), String::NewFromUtf8(isolate, "|ln: ")));
      /*sprintf(s, "  [%d] %s:%d -- %s [%s]\n",
          level, info.short_src, info.currentline,
          (info.name ? info.name : "<unknown>"), info.what);*/
      ++level;
    }
    return out;
  }

  void LuaState::ToValue(const FunctionCallbackInfo<Value> &args)
  {
    Isolate *isolate = args.GetIsolate();
    HandleScope scope(isolate);
    if (args.Length() != 1)
    {
      isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "LuaState#toValue takes exactly one argument")));
    }

    int offset = args[0]->Int32Value();

    LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());
    if (lua_gettop(obj->lua_))
    {
      args.GetReturnValue().Set(ValueFromLuaObject(isolate, obj->lua_, -offset));
      return;
    }
    isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "LuaState#toValue stack is empty")));
  }

  int LuaState::CallFunction(lua_State* L) {
    int n = lua_gettop(L);
    char *func_name = (char *)lua_tostring(L, lua_upvalueindex(1));

      
      const unsigned argc = n;
      Local<Value>* argv = new Local<Value>[argc];
      Isolate* isolate = Nan::GetCurrentContext()->Global()->GetIsolate();
      LuaState* self = LuaState::getCurrentInstance();

      int i;
      for (i = 1; i <= n; ++i) {
        argv[i - 1] = ValueFromLuaObject(isolate, L, i);
      }
      Local<Value> ret_val = Nan::Undefined();

      std::map<const char *, Nan::Persistent<v8::Function> >::iterator iter;
      for (iter = self->functions.begin(); iter != self->functions.end(); iter++) {
        if (strcmp(iter->first, func_name) == 0) {
          v8::Local<v8::Function> func = Nan::New(iter->second);
          ret_val = Nan::MakeCallback(Nan::GetCurrentContext()->Global(), func, argc, argv);
          break;
        }
      }

      PushValueToLua(Nan::GetCurrentContext()->Global()->GetIsolate(), ret_val, L);
    return 1;
  }

  void LuaState::RegisterFunction(const FunctionCallbackInfo<Value> &args) {
    Isolate *isolate = args.GetIsolate();
    HandleScope scope(isolate);

    if (args.Length() < 1) {
      Nan::ThrowTypeError("LuaState.RegisterFunction Must Have 2 Arguments");
      return;
    }

    if (!args[0]->IsString()) {
      Nan::ThrowTypeError("LuaState.RegisterFunction Argument 1 Must Be A String");
      return;
    }

    if (!args[1]->IsFunction()) {
      Nan::ThrowTypeError("LuaState.RegisterFunction Argument 2 Must Be A Function");
      return;
    }

    LuaState* obj = ObjectWrap::Unwrap<LuaState>(args.This());
    CHECK_LUA_STATE_IS_OPEN(isolate, obj);
    lua_State* L = obj->lua_;

    const char* func_name = ValueToChar(isolate, args[0]);
    Persistent<Function> pfunc;
    Nan::Persistent<v8::Function> func(Local<v8::Function>::Cast(args[1]));
    obj->functions[func_name].Reset(func);

    lua_pushstring(L, func_name);
    lua_pushcclosure(L, CallFunction, 1);
    lua_setglobal(L, func_name);
    args.GetReturnValue().Set(Undefined(isolate));
  }

  static int traceback(lua_State *L)
  {
    if (!lua_isstring(L, 1)) /* 'message' not a string? */
      return 1;              /* keep it intact */
    lua_getglobal(L, "debug");
    if (!lua_istable(L, -1))
    {
      lua_pop(L, 1);
      return 1;
    }
    lua_getfield(L, -1, "traceback");
    if (!lua_isfunction(L, -1))
    {
      lua_pop(L, 2);
      return 1;
    }
    lua_pushvalue(L, 1);
    lua_pushinteger(L, 2);
    lua_call(L, 2, 1);
    return 1;
  }

  void LuaState::DoStringSync(const FunctionCallbackInfo<Value> &args)
  {
    Isolate *isolate = args.GetIsolate();
    HandleScope scope(isolate);

    if (args.Length() > 2)
    {
      isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "LuaState#doStringSync too many arguments")));
    }

    const char *code = ValueToChar(isolate, args[0]);
    const char *name;
    if (!args[1]->IsUndefined())
    {
      name = ValueToChar(isolate, args[1]);
    }
    else
    {
      name = code;
    }
    LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());
    LuaState::setCurrentInstance(obj);
    CHECK_LUA_STATE_IS_OPEN(isolate, obj);
    lua_pushcfunction(obj->lua_, traceback);
    if ((luaL_loadbuffer(obj->lua_, code, strlen(code), name) || lua_pcall(obj->lua_, 0, LUA_MULTRET, -2)))
    {
      Local<Object> retn = Object::New(isolate);
      Local<Value> luaStackTrace = ValueFromLuaObject(isolate, obj->lua_, -1);
      retn->Set(
        String::NewFromUtf8(isolate, "Stack"),
        luaStackTrace);
      retn->Set(
        String::NewFromUtf8(isolate, "Debug"),
        Integer::New(isolate, lua_gettop(obj->lua_)));
      isolate->ThrowException(retn);
      LuaState::setCurrentInstance(0);
      return;
    }
    else
    {
      if (lua_gettop(obj->lua_))
      {
        args.GetReturnValue().Set(ValueFromLuaObject(isolate, obj->lua_, -1));
      }
    }
    LuaState::setCurrentInstance(0);
  }

  void LuaState::DoFileSync(const FunctionCallbackInfo<Value> &args)
  {
    Isolate *isolate = args.GetIsolate();
    HandleScope scope(isolate);

    if (args.Length() != 1)
    {
      isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "LuaState#DoFileSync takes exactly one argument")));
      return;
    }

    const char *file = ValueToChar(isolate, args[0]);

    LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());
    LuaState::setCurrentInstance(obj);
    CHECK_LUA_STATE_IS_OPEN(isolate, obj);

    if (luaL_dofile(obj->lua_, file))
    {
      const char *luaErrorMsg = lua_tostring(obj->lua_, -1);
      isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, luaErrorMsg)));
      LuaState::setCurrentInstance(0);
      return;
    }
    else
    {
      if (lua_gettop(obj->lua_))
      {
        args.GetReturnValue().Set(ValueFromLuaObject(isolate, obj->lua_, -1));
        LuaState::setCurrentInstance(0);
      }
      return;
    }
  }

  void LuaState::DoString(const FunctionCallbackInfo<Value> &args)
  {
    Isolate *isolate = args.GetIsolate();
    HandleScope scope(isolate);

    LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

    CHECK_LUA_STATE_IS_OPEN(isolate, obj);

    auto fillWorker = [](const FunctionCallbackInfo<Value> &args, async_lua_worker **worker) {
      (*worker)->data = (void *)ValueToChar(args.GetIsolate(), args[0]);
    };

    auto promise = LuaState::CreatePromise(args, fillWorker, async_luaL_dostring, async_after);

    args.GetReturnValue().Set(promise);
  }

  void LuaState::DoFile(const FunctionCallbackInfo<Value> &args)
  {
    Isolate *isolate = args.GetIsolate();
    HandleScope scope(isolate);

    LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

    CHECK_LUA_STATE_IS_OPEN(isolate, obj);

    auto fillWorker = [](const FunctionCallbackInfo<Value> &args, async_lua_worker **worker) {
      (*worker)->data = (void *)ValueToChar(args.GetIsolate(), args[0]);
    };

    auto promise = LuaState::CreatePromise(args, fillWorker, async_luaL_dofile, async_after);

    args.GetReturnValue().Set(promise);
  }

  void LuaState::GetGlobal(const FunctionCallbackInfo<Value> &args)
  {
    Isolate *isolate = args.GetIsolate();
    HandleScope scope(isolate);

    if (!args[0]->IsString())
    {
      isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "LuaState#getGlobal takes exactly one string")));
      return;
    }
    const char *globalName = ValueToChar(isolate, args[0]);

    LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

    CHECK_LUA_STATE_IS_OPEN(isolate, obj);

    lua_getglobal(obj->lua_, globalName);
    args.GetReturnValue().Set(ValueFromLuaObject(isolate, obj->lua_, -1));
  }

  void LuaState::SetGlobal(const FunctionCallbackInfo<Value> &args)
  {
    Isolate *isolate = args.GetIsolate();
    HandleScope scope(isolate);

    if (args.Length() != 2)
    {
      isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "LuaState#setGlobal takes exactly two arguments")));
      return;
    }

    LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

    CHECK_LUA_STATE_IS_OPEN(isolate, obj);

    const char *name = ValueToChar(isolate, args[0]);

    Local<Value> value = args[1];
    PushValueToLua(isolate, value, obj->lua_);

    lua_setglobal(obj->lua_, name);
  }

  void LuaState::GetStatus(const FunctionCallbackInfo<Value> &args)
  {
    Isolate *isolate = args.GetIsolate();
    HandleScope scope(isolate);

    LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

    CHECK_LUA_STATE_IS_OPEN(isolate, obj);

    int status = lua_status(obj->GetLuaState());

    args.GetReturnValue().Set(Number::New(isolate, status));
  }

  // Currently not exported to Node
  void LuaState::LoadStringSync(const FunctionCallbackInfo<Value> &args)
  {
    Isolate *isolate = args.GetIsolate();
    HandleScope scope(isolate);

    if (args.Length() != 1 && !args[0]->IsString())
    {
      isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "LuaState#loadString takes one string argument")));
      return;
    }

    const char *code = ValueToChar(isolate, args[0]);

    LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

    CHECK_LUA_STATE_IS_OPEN(isolate, obj);

    int success = luaL_loadstring(obj->GetLuaState(), code);

    args.GetReturnValue().Set(Number::New(isolate, success));
  }

  // Currently not exported to Node
  void LuaState::LoadString(const FunctionCallbackInfo<Value> &args)
  {
    Isolate *isolate = args.GetIsolate();
    HandleScope scope(isolate);

    LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

    CHECK_LUA_STATE_IS_OPEN(isolate, obj);

    if (args.Length() != 1 && !args[0]->IsString())
    {
      isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "LuaState#loadString takes one string argument")));
      return;
    }

    auto fillWorker = [](const FunctionCallbackInfo<Value> &args, async_lua_worker **worker) {
      Isolate *isolate = args.GetIsolate();
      HandleScope scope(isolate);
      (*worker)->data = (void *)ValueToChar(isolate, args[0]);
      (*worker)->returnFromStack = false;
    };

    auto work = [](uv_work_t *req) {
      async_lua_worker *worker = static_cast<async_lua_worker *>(req->data);
      worker->successRetval = luaL_loadstring(worker->state->GetLuaState(), (char *)worker->data);
    };

    auto promise = LuaState::CreatePromise(args, fillWorker, work, async_after);
    args.GetReturnValue().Set(promise);
  }

  //
  // Helper functions
  //

  template <typename Functor>
  Local<Promise> LuaState::CreatePromise(
    const FunctionCallbackInfo<Value> &args,
    Functor fillWorker,
    const uv_work_cb work_cb,
    const uv_after_work_cb after_work_cb)
  {
    Isolate *isolate = args.GetIsolate();
    HandleScope scope(isolate);

    luajs::LuaState *obj = node::ObjectWrap::Unwrap<luajs::LuaState>(args.This());

    auto resolver = v8::Promise::Resolver::New(args.GetIsolate());
    auto promise = resolver->GetPromise();
    auto persistent = new ResolverPersistent(resolver);

    async_lua_worker *reqData = new async_lua_worker();

    reqData->isolate = isolate;
    fillWorker(args, &reqData);
    reqData->persistent = persistent;

    reqData->state = obj;
    obj->Ref();

    uv_work_t *req = new uv_work_t;
    req->data = reqData;

    uv_queue_work(uv_default_loop(), req, work_cb, after_work_cb);

    return promise;
  }

} // namespace luajs
