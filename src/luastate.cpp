//
// Created by Lukas Kollmer on 12.04.17.
//

#include "luastate.h"
#include "luajs_utils.h"


namespace luajs {
    using v8::Context;
    using v8::Function;
    using v8::FunctionCallbackInfo;
    using v8::FunctionTemplate;
    using v8::Isolate;
    using v8::HandleScope;
    using v8::Exception;
    using v8::Local;
    using v8::Number;
    using v8::Object;
    using v8::Persistent;
    using v8::String;
    using v8::Value;

    using node::ObjectWrap;


    Persistent<Function> LuaState::constructor;


    LuaState::LuaState() {}
    LuaState::~LuaState() {}


    void LuaState::Init(Local<Object> exports) {

        Isolate *isolate = exports->GetIsolate();

        Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
        tpl->SetClassName(String::NewFromUtf8(isolate, "LuaState"));
        tpl->InstanceTemplate()->SetInternalFieldCount(1);

        NODE_SET_PROTOTYPE_METHOD(tpl, "doStringSync", DoStringSync);
        NODE_SET_PROTOTYPE_METHOD(tpl, "doFileSync", DoFileSync);
        NODE_SET_PROTOTYPE_METHOD(tpl, "getGlobal", GetGlobal);
        NODE_SET_PROTOTYPE_METHOD(tpl, "setGlobal", SetGlobal);

        constructor.Reset(isolate, tpl->GetFunction());
        exports->Set(String::NewFromUtf8(isolate, "LuaState"), tpl->GetFunction());

    }

    void LuaState::New(const FunctionCallbackInfo<Value>& args) {
        Isolate *isolate = args.GetIsolate();

        if (args.IsConstructCall()) {
            LuaState *obj = new LuaState();

            obj->lua_ = luaL_newstate();
            luaL_openlibs(obj->lua_);

            obj->Wrap(args.This());

            args.GetReturnValue().Set(args.This());
        }
    }


    void LuaState::DoStringSync(const FunctionCallbackInfo<Value>& args) {
        Isolate *isolate = args.GetIsolate();

        if (args.Length() != 1) {
            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "LuaState#DoStringSync takes exactly one argument")));
        }

        const char *code = ValueToChar(isolate, args[0]);

        printf("Will evaluate '%s'\n", code);

        LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

        if (luaL_dostring(obj->lua_, code)) {
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

    void LuaState::DoFileSync(const FunctionCallbackInfo<Value>& args) {
        Isolate *isolate = args.GetIsolate();

        if (args.Length() != 1) {
            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "LuaState#DoFileSync takes exactly one argument")));
            return;
        }

        const char *file = ValueToChar(isolate, args[0]);

        LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());
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

    void LuaState::GetGlobal(const FunctionCallbackInfo<Value>& args) {
        Isolate *isolate = args.GetIsolate();

        if (!args[0]->IsString()) {
            isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "LuaState#getGlobal tales exactly one string")));
            return;
        }
        const char *globalName = ValueToChar(isolate, args[0]);

        LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

        lua_getglobal(obj->lua_, globalName);
        args.GetReturnValue().Set(ValueFromLuaObject(isolate, obj->lua_, -1));
    }

    void LuaState::SetGlobal(const FunctionCallbackInfo<Value>& args) {
        Isolate *isolate = args.GetIsolate();

        if (args.Length() != 2) {
            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "LuaState#setGlobal takes exactly two arguments")));
            return;
        }

        LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

        const char *name = ValueToChar(isolate, args[0]);

        Local<Value> value = args[1];
        PushValueToLua(isolate, value, obj->lua_);

        lua_setglobal(obj->lua_, name);

    }

}