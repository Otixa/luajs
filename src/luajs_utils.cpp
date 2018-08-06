//
// Created by Lukas Kollmer on 12.04.17.
//

#include "luajs_utils.h"
#include "uuid/sole.h"


const char *RandomUUID() {
    sole::uuid id = sole::uuid4();

    // char *u = (char *) malloc(37);
    // strcpy(u, uuidStr);
    return id.str().c_str();
}

const char *ValueToChar(v8::Isolate *isolate, v8::Local<v8::Value> val){
    if(!val->IsString()){
        isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Argument Must Be A String")));
        return NULL;
    }

    v8::String::Utf8Value val_string(val);
    char * val_char_ptr = (char *) malloc(val_string.length() + 1);
    strcpy(val_char_ptr, *val_string);
    return val_char_ptr;
}

v8::Local<v8::Value> ValueFromLuaObject(v8::Isolate *isolate, lua_State *L, int index) {
    int type = lua_type(L, index);

    switch (type) {
        case LUA_TNUMBER: {
            double value = lua_tonumber(L, index);
            return v8::Local<v8::Number>::New(isolate, v8::Number::New(isolate, value));
        }
        case LUA_TBOOLEAN: {
            bool value = static_cast<bool>(lua_toboolean(L, index));
            return v8::Local<v8::Boolean>::New(isolate, v8::Boolean::New(isolate, value));
        }
        case LUA_TSTRING: {
            const char *value = lua_tostring(L, index);
            return v8::Local<v8::String>::New(isolate, v8::String::NewFromUtf8(isolate, value));
        }
        case LUA_TTABLE: {
            v8::Local<v8::Object> obj = v8::Object::New(isolate);

            lua_pushnil(L);
            while (lua_next(L, -2) != 0) {
                v8::Local<v8::Value> key = ValueFromLuaObject(isolate, L, -2);
                v8::Local<v8::Value> value = ValueFromLuaObject(isolate, L, -2);
                obj->Set(key, value);
                lua_pop(L, 1);
            }
            return obj;
        }
        default: {
            return v8::Local<v8::Primitive>::New(isolate, v8::Undefined(isolate));
        }
    }
}

void PushValueToLua(v8::Isolate *isolate, v8::Local<v8::Value> value, lua_State *L) {
    if (value->IsBoolean()) {
        lua_pushboolean(L, static_cast<int>(value->ToBoolean()->Value()));
    } else if (value->IsNumber()) {
        lua_pushnumber(L, value->ToNumber(isolate)->Value());
    } else if (value->IsString()) {
        lua_pushstring(L, ValueToChar(isolate, value));
    } else if (value->IsNull()) {
        lua_pushnil(L);
    } else if (value->IsObject()) {
        v8::Local<v8::Object> obj = value->ToObject();
        v8::Local<v8::Array> keys = obj->GetPropertyNames();

        lua_createtable(L, 0, keys->Length());

        for (int i = 0; i < keys->Length(); ++i) {
            v8::Local<v8::Value> key = keys->Get(i);
            v8::Local<v8::Value> value = obj->Get(key);
            PushValueToLua(isolate, key, L);
            PushValueToLua(isolate, value, L);
            lua_settable(L, -3);
        }
    } else { // Unsupported type
        lua_pushnil(L);
    }
}
