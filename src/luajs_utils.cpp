//
// Created by Lukas Kollmer on 12.04.17.
//

#include "luajs_utils.h"


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