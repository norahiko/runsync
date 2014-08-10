var runsync = require("../../index.js");
runsync.popen("echo grandchild process", { stdio: "inherit" });
