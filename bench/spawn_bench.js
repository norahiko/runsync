"use strict";
var runsync = require("../index.js");

var count = 1000;
var start = Date.now();
for(var i = 0; i < count; i++) {
    runsync.spawn("echo", ["hello"]);
}

var total = Date.now() - start;
console.log("spawned %d times", count);
console.log("total  : %d", total);
console.log("average: %d", total / count);
