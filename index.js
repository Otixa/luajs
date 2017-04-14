const binding = require('bindings')('luajs.node');

module.exports = {
    version: binding.luaVersion(),
    LuaState: binding.LuaState
};

Object.keys(binding).forEach(function(k) {
  if (k.match(/^LUA_/)) module.exports[k] = binding[k];
});