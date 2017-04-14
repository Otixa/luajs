const binding = require('bindings')('luajs.node');

module.exports = {
    version: binding.luaVersion(),
    LuaState: binding.LuaState
};

Object.keys(binding).forEach(function(k) {
  if (k.match(/^L/)) module.exports[k] = binding[k];
});
