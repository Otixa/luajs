# luajs

NodeJS module to expose Lua bindings to JavaScript.

## Installation

```
npm install luajs
```

## Usage

#### Creating a LuaState:

```js
const luajs = require('luajs');

let lua = new luajs.LuaState();
```
If you want to have multiple `LuaState` instances simultaneously, you need to pass a name to each state:  
`new luajs.LuaState('name');`.

#### Evaluating Code:

Using Promises:

```js
lua.doString('return 5/2;').then(result => {
  console.log(`Result: ${result}`);
}).catch(error => {
  // Handle error
});
```

#### Evaluating Lua Files:
```js
lua.doFile('./script.lua').then(result => {
  console.log(`Result: ${result}`);
}).catch(error => {
  // Handle error
});
```


#### Setting/Getting Globals:
```js
lua.setGlobal('name', 'Lukas');
let me = lua.getGlobal('name');
```

#### Using the syncronous API:

`luajs.LuaState#doString` and `luajs.LuaState#doFile` also have a syncronous API:

```js
let result1 = lua.doStringSync('return 5/2;');
let result2 = lua.doFileSync('./script.lua');
```
(This will throw if lua encounters an error running the code)

#### Other stuff

You should always close a `LuaState`instance once you're done using it:
```js
// Close state
lua.close();

// Reset state (this is equal to closing and re-initializing)
lua.reset();
```
