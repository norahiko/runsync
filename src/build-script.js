var child_process = require("child_process");
if(child_process.spawnSync === undefined) {
    child_process.execFile("node-gyp", ["rebuild"]);
}
