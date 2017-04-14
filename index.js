const binding = require('bindings')('luajs.node');

module.exports = {
    version: binding.luaVersion(),
    LuaState: binding.LuaState
};
