# runsync 

Polyfill of spawnSync and execSync for Node-0.10.x ***(Unix only yet)***

[![Build Status](https://travis-ci.org/norahiko/runsync.svg?branch=master)](https://travis-ci.org/norahiko/runsync)
[![Coverage Status](https://coveralls.io/repos/norahiko/runsync/badge.png?branch=master)](https://coveralls.io/r/norahiko/runsync?branch=master)

[![NPM](https://nodei.co/npm/runsync.png)](https://nodei.co/npm/runsync/)


## Instllation
Requires [node-gyp] (https://github.com/TooTallNate/node-gyp)

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


## API

### `runsync.spawn(executable, [args], [options])`
* Polyfill of [child_process.spawnSync](http://nodejs.org/dist/v0.11.13/docs/api/child_process.html#child_process_child_process_spawnsync_command_args_options).

```js
var res = runsync.spawn("node", ["-e", "console.log('Hello, World!')"], { encoding: "utf8" });
console.log(res.stdout) // => 'Hello, World!\n'
```

-----

### `runsync.exec(command, [options])`
* Polyfill of [child_process.execSync](http://nodejs.org/dist/v0.11.13/docs/api/child_process.html#child_process_child_process_execsync_command_options).

```js
var output = runsync.exec("sleep 3 && echo Hello!", { timeout: 1000 });
// => throw Exception because of timeout
```

-----

### `runsync.execFile(command, [options])`
* Polyfill of [child_process.execFileSync](http://nodejs.org/dist/v0.11.13/docs/api/child_process.html#child_process_child_process_execfilesync_command_args_options).

```js
var html = runsync.execFile("curl", ["--silent", "-X", "GET", "http://example.com"]);
console.log(html.toString()); // => '<!doctype html>\n<html>\n<head>\n ...'
```

-----

### `runsync.popen(command, [options])`
* This is similar to `runsync.exec`, but it returns **spawn object** likes `runsync.spawn`.
* This method will not throw Exceptions even if command fails.

```js
var result = runsync.popen("echo `cat` && echo strerr 1>&2", { input: "stdin", encoding: "utf8" });
console.log(result.stdout) // => "stdin\n"
console.log(result.stderr) // => "stderr\n"
```

-----

### `runsync.shell(command, [options])`
* This is similar to `runsync.exec`, but always set **'inherit'** to **options.stdio**.
* Returns Nothing(undefined).
* This method will throw Exceptions if command fails.

```js
try {
  runsync.shell("mocha --reporter nyan");
  // 31  -_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_,------,
  // 1   -_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_|   /\_/\ 
  // 0   -_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-^|__( x .x) 
  //     -_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-  ""  "" 
  //  31 passing (468ms)
  //  1 failing

} catch(err) {
  console.log(err.message);
  // => 'Command failed: `mocha -u tdd --reporter nyan`'
}
```
