var child_process = require("child_process");

if(process.platform === "win32") {
    if(child_process.spawnSync === undefined) {
        throw new Error("Can not install on Windows Node 0.10");
    }
} else {
    child_process.spawn("node-gyp", ["rebuild"], {
        stdio: "inherit",
    }).on("eixt", function(code, signal) {
        if(code || signal) {
            throw new Error("runsync: Build failed");
        }
    });
}
