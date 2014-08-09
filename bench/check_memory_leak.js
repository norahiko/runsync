var runsync = require("../index.js");

setInterval(function() {
    for(var i = 0; i < 500; i++) {
        runsync.spawn("echo", ["hello"]);
    }
    console.log(process.memoryUsage());
}, 400);


