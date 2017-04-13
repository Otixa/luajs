//
// Created by Lukas Kollmer on 12.04.17.
//

#include <map>
#include <set>
#include <string>
#include "luastate.h"
#include "luajs_utils.h"

using namespace v8;

std::set<std::string> luaStateNames;

static bool NameExists(std::string name) {
    return luaStateNames.find(name) != luaStateNames.end();
}

namespace luajs {

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


        NODE_SET_PROTOTYPE_METHOD(tpl, "doStringSync", DoStringSync);
        NODE_SET_PROTOTYPE_METHOD(tpl, "doFileSync", DoFileSync);
        NODE_SET_PROTOTYPE_METHOD(tpl, "getGlobal", GetGlobal);
        NODE_SET_PROTOTYPE_METHOD(tpl, "setGlobal", SetGlobal);

        constructor.Reset(isolate, tpl->GetFunction());
        exports->Set(String::NewFromUtf8(isolate, "LuaState"), tpl->GetFunction());

    }

    void LuaState::New(const FunctionCallbackInfo<Value>& args) {
        Isolate *isolate = args.GetIsolate();

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
        printf("%s\n", __PRETTY_FUNCTION__);
        LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

        obj->Close(args);
        obj->lua_ = luaL_newstate();
        obj->isClosed_ = false;
    }

    void LuaState::Close(const v8::FunctionCallbackInfo<v8::Value>& args) {
        printf("%s\n", __PRETTY_FUNCTION__);
        LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

        lua_close(obj->lua_);
        obj->lua_ = NULL;
        obj->isClosed_ = true;
    }


    void LuaState::DoStringSync(const FunctionCallbackInfo<Value>& args) {
        Isolate *isolate = args.GetIsolate();

        if (args.Length() != 1) {
            isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, "LuaState#DoStringSync takes exactly one argument")));
        }

        const char *code = ValueToChar(isolate, args[0]);

        LuaState *obj = ObjectWrap::Unwrap<LuaState>(args.This());

        if (obj->isClosed_) {
            char *errorMsg;
            asprintf(&errorMsg, "Error: LuaState %s is closed\n", obj->name_);
            isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(isolate, errorMsg)));
            free(errorMsg);
            return;
        }

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