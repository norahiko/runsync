runsync
=======
Polyfill of spawnSync and execSync for Node-0.10.x

## Instllation
**Requires [node-gyp] (https://github.com/TooTallNate/node-gyp)**
```sh
$ npm install runsync
```

## Usage
```js
var runsync = require("runsync");
var msg = runsync.exec("echo Hello, world!");
console.log(msg); // => Hello, world!
```
