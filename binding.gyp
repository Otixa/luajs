{
  "targets": [
    {
      "target_name": "luajs",
      "variables": {
        "lua_include": "<!(find /usr/include /usr/local/include -name lua.h | sed s/lua.h//)"
      },
      "sources": [
        "src/luajs.cc",
        "src/luastate.cpp",
        "src/luajs_utils.cpp"
      ],
      "include_dirs": [
        "<@(lua_include)",
        "<!(node -e \"require('nan')\")",
      ],
      "libraries": [
        "<!(pkg-config --libs-only-l --silence-errors lua || pkg-config --libs-only-l --silence-errors lua5.1 || echo '')",
        "-ldl"
	  ]
    }
  ]
}