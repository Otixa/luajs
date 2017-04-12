//
// Created by Lukas Kollmer on 12.04.17.
//

#ifndef LUAJS_LUASTATE_H
#define LUAJS_LUASTATE_H


// NodeJS headers
#include <node.h>
#include <node_object_wrap.h>
#include <v8.h>

// Lua Headers
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
};

namespace luajs {
    class LuaState : public node::ObjectWrap {
    public:
        LuaState();
        static void Init(v8::Local<v8::Object> exports);

        static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
        static void DoStringSync(const v8::FunctionCallbackInfo<v8::Value>& args);

    private:
        ~LuaState();

        lua_State *lua_;

        static v8::Persistent<v8::Function> constructor;

    };
}


#endif //LUAJS_LUASTATE_H
