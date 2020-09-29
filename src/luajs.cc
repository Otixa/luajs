#include <node.h>
#include "luastate.h"

extern "C" {
#include "lua/lua.h"
}

namespace luajs {

    using v8::FunctionCallbackInfo;
    using v8::Isolate;
    using v8::Local;
    using v8::Object;
    using v8::String;
    using v8::Value;
    using v8::MaybeLocal;


    void DefineConstants(Local<Object> exports) {
        NODE_DEFINE_CONSTANT(exports, LUA_ERRSYNTAX);

        /*
        NODE_DEFINE_CONSTANT(exports, LUA_VERSION_MAJOR);
        NODE_DEFINE_CONSTANT(exports, LUA_VERSION_MINOR);
        NODE_DEFINE_CONSTANT(exports, LUA_VERSION_NUM);
        NODE_DEFINE_CONSTANT(exports, LUA_VERSION_RELEASE);
        NODE_DEFINE_CONSTANT(exports, LUA_VERSION);
        NODE_DEFINE_CONSTANT(exports, LUA_RELEASE);
        NODE_DEFINE_CONSTANT(exports, LUA_COPYRIGHT);
        NODE_DEFINE_CONSTANT(exports, LUA_AUTHORS);
         */

        NODE_DEFINE_CONSTANT(exports, LUA_OK);
        NODE_DEFINE_CONSTANT(exports, LUA_YIELD);
        NODE_DEFINE_CONSTANT(exports, LUA_ERRRUN);
        NODE_DEFINE_CONSTANT(exports, LUA_ERRSYNTAX);
        NODE_DEFINE_CONSTANT(exports, LUA_ERRMEM);
        NODE_DEFINE_CONSTANT(exports, LUA_ERRGCMM);
        NODE_DEFINE_CONSTANT(exports, LUA_ERRERR);

        NODE_DEFINE_CONSTANT(exports, LUA_TNONE);
        NODE_DEFINE_CONSTANT(exports, LUA_TNIL);
        NODE_DEFINE_CONSTANT(exports, LUA_TBOOLEAN);
        NODE_DEFINE_CONSTANT(exports, LUA_TLIGHTUSERDATA);
        NODE_DEFINE_CONSTANT(exports, LUA_TNUMBER);
        NODE_DEFINE_CONSTANT(exports, LUA_TSTRING);
        NODE_DEFINE_CONSTANT(exports, LUA_TTABLE);
        NODE_DEFINE_CONSTANT(exports, LUA_TFUNCTION);
        NODE_DEFINE_CONSTANT(exports, LUA_TUSERDATA);
        NODE_DEFINE_CONSTANT(exports, LUA_TTHREAD);


        NODE_DEFINE_CONSTANT(exports, LUA_NUMTAGS);

        NODE_DEFINE_CONSTANT(exports, LUA_MINSTACK);

        NODE_DEFINE_CONSTANT(exports, LUA_OPADD);
        NODE_DEFINE_CONSTANT(exports, LUA_OPSUB);
        NODE_DEFINE_CONSTANT(exports, LUA_OPMUL);
        NODE_DEFINE_CONSTANT(exports, LUA_OPDIV);
        NODE_DEFINE_CONSTANT(exports, LUA_OPMOD);
        NODE_DEFINE_CONSTANT(exports, LUA_OPPOW);
        NODE_DEFINE_CONSTANT(exports, LUA_OPUNM);
        NODE_DEFINE_CONSTANT(exports, LUA_OPEQ);
        NODE_DEFINE_CONSTANT(exports, LUA_OPLT);
        NODE_DEFINE_CONSTANT(exports, LUA_OPLE);

        NODE_DEFINE_CONSTANT(exports, LUA_GCSTOP);
        NODE_DEFINE_CONSTANT(exports, LUA_GCRESTART);
        NODE_DEFINE_CONSTANT(exports, LUA_GCCOLLECT);
        NODE_DEFINE_CONSTANT(exports, LUA_GCCOUNT);
        NODE_DEFINE_CONSTANT(exports, LUA_GCCOUNTB);
        NODE_DEFINE_CONSTANT(exports, LUA_GCSTEP);
        NODE_DEFINE_CONSTANT(exports, LUA_GCSETPAUSE);
        NODE_DEFINE_CONSTANT(exports, LUA_GCSETSTEPMUL);
        //NODE_DEFINE_CONSTANT(exports, LUA_GCSETMAJORINC);
        NODE_DEFINE_CONSTANT(exports, LUA_GCISRUNNING);
        //NODE_DEFINE_CONSTANT(exports, LUA_GCGEN);
        //NODE_DEFINE_CONSTANT(exports, LUA_GCINC);

        NODE_DEFINE_CONSTANT(exports, LUA_HOOKCALL);
        NODE_DEFINE_CONSTANT(exports, LUA_HOOKRET);
        NODE_DEFINE_CONSTANT(exports, LUA_HOOKLINE);
        NODE_DEFINE_CONSTANT(exports, LUA_HOOKCOUNT);
        NODE_DEFINE_CONSTANT(exports, LUA_HOOKTAILCALL);
    }

    static void LuaVersion(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, LUA_VERSION, v8::NewStringType::kNormal).ToLocalChecked());
    }

    void Initialize(Local<Object> exports) {
        NODE_SET_METHOD(exports, "luaVersion", LuaVersion);
        LuaState::Init(exports);
        DefineConstants(exports);
    }

    NODE_MODULE(luajs, Initialize)
}