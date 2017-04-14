//
// Created by Lukas Kollmer on 12.04.17.
//

#ifndef LUAJS_LUASTATE_H
#define LUAJS_LUASTATE_H


// NodeJS headers
#include <node.h>
#include <node_object_wrap.h>
#include <v8.h>
#include <uv.h>

// Lua Headers
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
};

namespace luajs {
    class LuaState : public node::ObjectWrap {
    public:
        LuaState(lua_State *state, const char *name);
        static void Init(v8::Local<v8::Object> exports);

        static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
        static void Close(const v8::FunctionCallbackInfo<v8::Value>& args);
        static void Reset(const v8::FunctionCallbackInfo<v8::Value>& args);

        static void DoString(const v8::FunctionCallbackInfo<v8::Value>& args);
        static void DoStringSync(const v8::FunctionCallbackInfo<v8::Value>& args);
        static void DoFile(const v8::FunctionCallbackInfo<v8::Value>& args);
        static void DoFileSync(const v8::FunctionCallbackInfo<v8::Value>& args);
        static void GetGlobal(const v8::FunctionCallbackInfo<v8::Value>& args);
        static void SetGlobal(const v8::FunctionCallbackInfo<v8::Value>& args);

        static void GetStatus(const v8::FunctionCallbackInfo<v8::Value>& args);

        lua_State* GetLuaState() { return lua_; }
        const char* GetName() { return name_; }
        bool IsClosed() { return isClosed_; }

    private:
        ~LuaState();

        lua_State *lua_;
        const char *name_;
        bool isClosed_;
        v8::Isolate* isolate_;

        v8::Isolate* GetIsolate() { return  isolate_; }
        void SetIsolate(v8::Isolate* isolate) { this->isolate_ = isolate; }

        static v8::Persistent<v8::Function> constructor;

        static v8::Local<v8::Promise> CreateLuaCodeEvaluationPromise(const v8::FunctionCallbackInfo<v8::Value>& args, uv_work_cb cb);

    };
}


#endif //LUAJS_LUASTATE_H
