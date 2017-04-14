const assert = require('assert');

const luajs = require('./index.js')

describe('LuaState', function() {
  it('Can\'t have the same name multiple times', function() {
    assert.throws(() => {
      let lua1 = new luajs.LuaState('lua1');
      let lua2 = new luajs.LuaState('lua1');
    });
  });

  it('should return the correct result for the lua script', function() {
    let lua = new luajs.LuaState();
    let result = lua.doStringSync('return 5 * 5;');
    assert.equal(result, 25);
  });

  it('should throw if lua encounters an error', function() {
    let lua = new luajs.LuaState();
    assert.throws(() => {
      lua.doStringSync('retrn 1;');
    });
  });

  it('should call the promise w/ the correct return value', function() {
    let lua = new luajs.LuaState();
    lua.doString('return 5 * 5;').then(result => {
      assert.equal(result, 25);
    }).catch(error => {
      assert(false, "Shouldn't reach here");
    })
  });
})