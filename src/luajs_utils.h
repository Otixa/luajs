//
// Created by Lukas Kollmer on 12.04.17.
//

#ifndef LUAJS_LUAJS_UTILS_H
#define LUAJS_LUAJS_UTILS_H

#include <node.h>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
};


const char *ValueToChar(v8::Isolate *isolate, v8::Local<v8::Value> val);

v8::Local<v8::Value> ValueFromLuaObject(v8::Isolate *isolate, lua_State *L, int index);

void PushValueToLua(v8::Isolate *isolate, v8::Local<v8::Value> value, lua_State *L);

#endif //LUAJS_LUAJS_UTILS_H
