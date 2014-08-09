runsync 
=======
[![Build Status](https://travis-ci.org/norahiko/runsync.svg?branch=master)](https://travis-ci.org/norahiko/runsync)
[![Coverage Status](https://coveralls.io/repos/norahiko/runsync/badge.png?branch=master)](https://coveralls.io/r/norahiko/runsync?branch=master)

Polyfill of spawnSync and execSync for Node-0.10.x ***(Unix only yet)***

## Instllation
**Requires [node-gyp] (https://github.com/TooTallNate/node-gyp)**
```sh
$ npm install runsync
```

## Usage
```js
var runsync = require("runsync");
var result = runsync.spawn("echo", ["Hello", "World!"], { encoding: "utf8" });
console.log(result.stdout); // => Hello world!\n

runsync.exec("sleep 1");

result = runsync.popen("echo Error message 1>&2", { encoding: "utf8" });
console.log(result.stderr); // => Error message\n
```
