var runsync = require("../index.js");

setInterval(function() {
    for(var i = 0; i < 500; i++) {
        var res = runsync.spawn("echo", ["hello"]);
        if(res.error) {
            throw res.error;
        }
    }
    console.log(process.memoryUsage());
}, 400);


