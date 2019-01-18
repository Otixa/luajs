//
// Created by Lukas Kollmer on 12.04.17.
//

#include <map>
#include <set>
#include <string>
#include "luastate.h"
#include "luajs_utils.h"
#include <nan.h>
#include <iostream>
#include "asprintf.h"

using namespace v8;

using ResolverPersistent = Nan::Persistent<v8::Promise::Resolver>;
using FunctionPersistent = Nan::Persistent<v8::Function, v8::NonCopyablePersistentTraits<v8::Function>>;

std::set<std::string> luaStateNames;
std::map<std::string, FunctionPersistent > functions;

static bool NameExists(std::string name) {
    return luaStateNames.find(name) != luaStateNames.end();
}

#define CHECK_LUA_STATE_IS_OPEN(isolate, obj) \
    if (obj->isClosed_) { \
        char *errorMsg; \
        asprintf(&errorMsg, "Error: LuaState %s is closed\n", obj->name_); \
        isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(isolate, errorMsg))); \
        free(errorMsg); \
        return; \
    }\


#define lua_do(func)                                                                    \
    void async_ ## func (uv_work_t *req) {                                              \
        async_lua_worker *worker = static_cast<async_lua_worker*>(req->data);           \
        if (func (worker->state->GetLuaState(), (char *)worker->data)) {                \
            worker->error = true;                                                       \
            sprintf(worker->msg, "%s", lua_tostring(worker->state->GetLuaState(), -1)); \
        }                                                                               \
    }                                                                                   \


namespace luajs {

    struct async_lua_worker {
        Isolate* isolate;
        ResolverPersistent* persistent;
        void* data;
        bool error;
        char msg[1000];
        luajs::LuaState *state;
        int successRetval;
        bool returnFromStack = true;
    };

    void async_after(uv_work_t *req, int status) {
        async_lua_worker *worker = static_cast<async_lua_worker *>(req->data);
        HandleScope scope(worker->isolate);

        auto resolver = Nan::New(*worker->persistent);

        if (worker->error) {
            resolver->Reject(Nan::New(worker->msg).ToLocalChecked());
        } else {
            if (worker->returnFromStack) {
                Local<Value> retval = ValueFromLuaObject(worker->isolate, worker->state->GetLuaState(), -1);
                resolver->Resolve(retval);
            } else {
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


    LuaState::LuaState(lua_State *state, const char *name) : lua_(state), name_(name), isClosed_(false) {
        luaStateNames.insert(std::string(name));
    }

    LuaState::~LuaState() {}


    void LuaState::Init(Local<Object> exports) {

        Isolate *isolate = exports->GetIsolate();

        Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
        tpl->SetClassName(String::NewFromUtf8(isolate, "LuaState"));
        tpl->InstanceTemplate()->SetInternalFieldCount(1);

        NODE_SET_PROTOTYPE_METHOD(tpl, "close", Close);
        NODE_SET_PROTOTYPE_METHOD(tpl, "reset", Reset);

        NODE_SET_PROTOTYPE_METHOD(tpl, "doString", DoString);
        NODE_SET_PROTOTYPE_METHOD(tpl, "doFile", DoFile);

        NODE_SET_PROTOTYPE_METHOD(tpl, "doStringSync", DoStringSync);
        NODE_SET_PROTOTYPE_METHOD(tpl, "doFileSync", DoFileSync);
        NODE_SET_PROTOTYPE_METHOD(tpl, "getGlobal", GetGlobal);
        NODE_SET_PROTOTYPE_METHOD(tpl, "setGlobal", SetGlobal);

        NODE_SET_PROTOTYPE_METHOD(tpl, "getStatus", GetStatus);

        //NODE_SET_PROTOTYPE_METHOD(tpl, "loadString", LoadString);
        //NODE_SET_PROTOTYPE_METHOD(tpl, "loadStringSync", LoadStringSync);

        constructor.Reset(isolate, tpl->GetFunction());
        exports->Set(String::NewFromUtf8(isolate, "LuaState"), tpl->GetFunction());

    }

    void LuaState::New(const FunctionCallbackInfo<Value>& args) {
        Isolate *isolate = args.GetIsolate();
        HandleScope scope(isolate);

        char *name;

        if (!args[0]->IsString()) {
            do {
                name = (char *)RandomUUID();
            } while (NameExists(std::string(name)));
        } else {
            name = (char *)ValueToChar(isolate, args[0]);
        }

        if (NameExists(std::string(name))) {
            char *excMessage;
            asprintf(&excMessage, "Error: LuaState with name '%s' already exists", name);
            isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(isolate, excMessage)));
            free(excMessage);
            return;
        }

        if (args.IsConstructCall()) {
            LuaState *obj = new LuaState(luaL_newstate(), name);
            luaL_openlibs(obj->lua_);

            obj->Wrap(args.This());
            args.GetReturnValue().Set(args.This());

        }
    }

    void LuaState::Reset(const v8::FunctionCallbackInfo<v8::Value>& args) {
        Isolate *isolate = args.GetIsolate();
        HandleScope scope(isolate);
        LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

        CHECK_LUA_STATE_IS_OPEN(isolate, obj);

        obj->Close(args);
        obj->lua_ = luaL_newstate();
        obj->isClosed_ = false;
    }

    void LuaState::Close(const v8::FunctionCallbackInfo<v8::Value>& args) {
        Isolate *isolate = args.GetIsolate();
        HandleScope scope(isolate);
        LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

        CHECK_LUA_STATE_IS_OPEN(isolate, obj);

        lua_close(obj->lua_);
        obj->lua_ = NULL;
        obj->isClosed_ = true;
    }

    void LuaState::DoStringSync(const FunctionCallbackInfo<Value>& args) {
        Isolate *isolate = args.GetIsolate();
        HandleScope scope(isolate);

        if (args.Length() != 1) {
            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "LuaState#doStringSync takes exactly one argument")));
        }

        const char *code = ValueToChar(isolate, args[0]);

        LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

        CHECK_LUA_STATE_IS_OPEN(isolate, obj);

        if (luaL_dostring(obj->lua_, code)) {
            const char *luaErrorMsg = lua_tostring(obj->lua_, -1);
			luaL_traceback(obj->lua_, obj->lua_, NULL, 1);
			const char *luaStackTrace = lua_tostring(obj->lua_, -1);
			v8::Local<v8::Object> retn = v8::Object::New(isolate);
			retn->Set(
				String::NewFromUtf8(isolate, "error"),
				String::NewFromUtf8(isolate, luaErrorMsg)
			);
			retn->Set(
				String::NewFromUtf8(isolate, "stack"),
				String::NewFromUtf8(isolate, luaStackTrace)
			);
            isolate->ThrowException(Exception::Error(retn));
            return;
        } else {
            if (lua_gettop(obj->lua_)) {
                args.GetReturnValue().Set(ValueFromLuaObject(isolate, obj->lua_, -1));
            }
            return;
        }
    }

    void LuaState::DoFileSync(const FunctionCallbackInfo<Value>& args) {
        Isolate *isolate = args.GetIsolate();
        HandleScope scope(isolate);

        if (args.Length() != 1) {
            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "LuaState#DoFileSync takes exactly one argument")));
            return;
        }

        const char *file = ValueToChar(isolate, args[0]);

        LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

        CHECK_LUA_STATE_IS_OPEN(isolate, obj);

        if (luaL_dofile(obj->lua_, file)) {
            const char *luaErrorMsg = lua_tostring(obj->lua_, -1);
            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, luaErrorMsg)));
            return;
        } else {
            if (lua_gettop(obj->lua_)) {
                args.GetReturnValue().Set(ValueFromLuaObject(isolate, obj->lua_, -1));
            }
            return;
        }
    }

    void LuaState::DoString(const FunctionCallbackInfo<Value>& args) {
        Isolate *isolate = args.GetIsolate();
        HandleScope scope(isolate);

        LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

        CHECK_LUA_STATE_IS_OPEN(isolate, obj);

        auto fillWorker = [] (const FunctionCallbackInfo<Value>& args, async_lua_worker **worker)  {
            (*worker)->data = (void*)ValueToChar(args.GetIsolate(), args[0]);
        };

        auto promise = LuaState::CreatePromise(args, fillWorker, async_luaL_dostring, async_after);

        args.GetReturnValue().Set(promise);
    }

    void LuaState::DoFile(const FunctionCallbackInfo<Value>& args) {
        Isolate *isolate = args.GetIsolate();
        HandleScope scope(isolate);

        LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

        CHECK_LUA_STATE_IS_OPEN(isolate, obj);

        auto fillWorker = [] (const FunctionCallbackInfo<Value>& args, async_lua_worker **worker)  {
            (*worker)->data = (void*)ValueToChar(args.GetIsolate(), args[0]);
        };

        auto promise = LuaState::CreatePromise(args, fillWorker, async_luaL_dofile, async_after);

        args.GetReturnValue().Set(promise);
    }

    void LuaState::GetGlobal(const FunctionCallbackInfo<Value>& args) {
        Isolate *isolate = args.GetIsolate();
        HandleScope scope(isolate);

        if (!args[0]->IsString()) {
            isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "LuaState#getGlobal tales exactly one string")));
            return;
        }
        const char *globalName = ValueToChar(isolate, args[0]);

        LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

        CHECK_LUA_STATE_IS_OPEN(isolate, obj);

        lua_getglobal(obj->lua_, globalName);
        args.GetReturnValue().Set(ValueFromLuaObject(isolate, obj->lua_, -1));
    }

    void LuaState::SetGlobal(const FunctionCallbackInfo<Value>& args) {
        Isolate *isolate = args.GetIsolate();
        HandleScope scope(isolate);

        if (args.Length() != 2) {
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

    void LuaState::GetStatus(const FunctionCallbackInfo<Value>& args) {
        Isolate *isolate = args.GetIsolate();
        HandleScope scope(isolate);

        LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

        CHECK_LUA_STATE_IS_OPEN(isolate, obj);

        int status = lua_status(obj->GetLuaState());

        args.GetReturnValue().Set(Number::New(isolate, status));
    }

    // Currently not exported to Node
    void LuaState::LoadStringSync(const FunctionCallbackInfo<Value>& args) {
        Isolate *isolate = args.GetIsolate();
        HandleScope scope(isolate);

        if (args.Length() != 1 && !args[0]->IsString()) {
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
    void LuaState::LoadString(const FunctionCallbackInfo<Value>& args) {
        Isolate *isolate = args.GetIsolate();
        HandleScope scope(isolate);

        LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

        CHECK_LUA_STATE_IS_OPEN(isolate, obj);

        if (args.Length() != 1 && !args[0]->IsString()) {
            isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "LuaState#loadString takes one string argument")));
            return;
        }

        auto fillWorker = [] (const FunctionCallbackInfo<Value>& args, async_lua_worker **worker)  {
            Isolate *isolate = args.GetIsolate();
            HandleScope scope(isolate);
            (*worker)->data = (void *)ValueToChar(isolate, args[0]);
            (*worker)->returnFromStack = false;
        };

        auto work = [] (uv_work_t *req) {
            async_lua_worker *worker = static_cast<async_lua_worker*>(req->data);
            worker->successRetval = luaL_loadstring(worker->state->GetLuaState(), (char *)worker->data);
        };

        auto promise = LuaState::CreatePromise(args, fillWorker, work, async_after);
        args.GetReturnValue().Set(promise);
    }


    //
    // Helper functions
    //

    template<typename Functor>
    Local<Promise> LuaState::CreatePromise(
            const FunctionCallbackInfo<Value>& args,
            Functor fillWorker,
            const uv_work_cb work_cb,
            const uv_after_work_cb after_work_cb) {
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

}
