const binding = require('bindings')('luajs.node');

module.exports = {
    version: binding.luaVersion(),
    LuaState: binding.LuaState
};

console.log(`Lua Version: ${module.exports.version}`);


let lua = new module.exports.LuaState();

let retval = lua.doStringSync("return 5 * 10");

console.log(retval);
