"use strict";

var constants = require("constants");
var assert = require("assert");
var fs = require("fs");
var pathModule = require("path");
var util = require("util");

var isWindows = process.platform === "win32";
var shell = isWindows ? "cmd.exe" : "/bin/sh";
var polyfill;
var errorCodeMap;

try {
    polyfill = require("./build/Release/polyfill.node");
} catch(_) {
    polyfill = require("./build/Debug/polyfill.node");
}

errorCodeMap = Object.create(null);
for(var code in constants) {
    if(code[0] === "E") {   // code == Error code
        errorCodeMap[constants[code]] = code;
    }
}


/**
 * exports
 */

exports.polyfill = polyfill;

exports.spawn = function spawn(file, args, options) {
    if(Array.isArray(args) === false) {
        options = args;
        args = [];
    }

    var proc = new ProcessSync(file, args, options || {});
    return proc.run();
};

exports.exec = function exec(command, options) {
    var result = exports.popen(command, options);
    if(result.stderr) {
        process.stderr.write(result.stderr.toString());
    }
    if(result.error || result.status !== 0) {
        throw new Error("Command failed: `" + command + "`\n" + result.stderr.toString());
    }
    return result.stdout;
};

exports.popen = function popen(command, options) {
    var args = buildCommandArgs(command);
    var result = exports.spawn(shell, args, options);
    result.command = result.args[2];
    return result;
};

exports.shell = function shell(command, options) {
    options = util._extend({}, options);
    options.stdio = "inherit";
    var result = exports.popen(command, options);

    if(result.error || result.status !== 0) {
        throw new Error("Command failed: `" + command + "`\n" + result.stderr.toString());
    }
};

var tmpdir = require("os").tmpdir();

exports.stdinFile = pathModule.join(tmpdir, "runsync_stdin_" + process.pid);

exports.stdoutFile = pathModule.join(tmpdir, "runsync_stdout_" + process.pid);

exports.stderrFile = pathModule.join(tmpdir, "runsync_stderr_" + process.pid);


/**
 * ProcessSync class
 */

function ProcessSync(file, args, options) {
    this.file = file;
    this.args = args.slice(0);
    this.args.unshift(this.file);
    this.options = util._extend({}, options);
    this.normalizeOptions();

}

ProcessSync.prototype.run = function() {
    var result = polyfill.spawnSync(this.file, this.args, this.options);
    this.readStdioFiles(result);
    this.setErrorObject(result);
    return result;
};

ProcessSync.prototype.normalizeOptions = function () {
    var envPairs = [];
    for(var name in this.options.env) {
        envPairs.push(name + "=" + this.options.env[name]);
    }
    this.options.envPairs = envPairs;
    this.initStdioPipe();
};

ProcessSync.prototype.initStdioPipe = function() {
    var opt = this.options;

    if(opt.stdio === undefined) {
        opt.stdio = "pipe";
    }
    if(opt.stdio === "pipe") {
        opt.stdio = ["pipe", "pipe", "pipe"];
    } else if(opt.stdio === "inherit") {
        opt.stdio = ["inherit", "inherit", "inherit"];
    } else if(opt.stdio === "ignore") {
        opt.stdio = ["ignore", "ignore", "ignore"];
    }

    assert(opt.stdio instanceof Array);

    if(opt.stdio[0] === "pipe") {
        fs.writeFileSync(exports.stdinFile, opt.input || "");
        opt.stdio[0] = fs.openSync(exports.stdinFile, "r");
    }
    if(opt.stdio[1] === "pipe") {
        opt.stdio[1] = fs.openSync(exports.stdoutFile, "w");
    }
    if(opt.stdio[2] === "pipe") {
        opt.stdio[2] = fs.openSync(exports.stderrFile, "w");
    }
};

ProcessSync.prototype.readStdioFiles = function(res) {
    var opt = this.options;
    var encoding = opt.encoding === "buffer" ? null : opt.encoding;
    res.stdout = null;
    res.stderr = null;

    if(typeof opt.stdio[0] === "number") {
        fs.closeSync(opt.stdio[0]);
    }
    if(typeof opt.stdio[1] === "number") {
        fs.closeSync(opt.stdio[1]);
        res.stdout = fs.readFileSync(exports.stdoutFile, encoding);
    }
    if(typeof opt.stdio[2] === "number") {
        fs.closeSync(opt.stdio[2]);
        res.stderr = fs.readFileSync(exports.stderrFile, encoding);
    }

    res.output = [null, res.stdout, res.stderr];

};

ProcessSync.prototype.setErrorObject = function(res) {
    if(res._hasTimedOut) {
        delete res._hasTimedOut;
        res.error = createSpawnError("", constants.ETIMEDOUT);
    }

    if(res.error !== undefined || res.status !== 127) {
        return;
    }

    var stderr = res.stderr.toString();
    var match = stderr.match(/^errno: (\d+)\n/);
    if(match) {
        var message = stderr.slice(match[0].length, -1);
        res.error = createSpawnError(message, parseInt(match[1]));
        res.stdout = null;
        res.stderr = null;
    }
};

function buildCommandArgs(command) {
    if (isWindows) {
        return ["/s", "/c", "\"" + command + "\""];
    } else {
        return ["-c", command];
    }
}

function createSpawnError(msg, errno) {
    var code = errorCodeMap[errno] || "";
    var message = "spawnSync " + code;
    if(msg) {
        message += " " + msg;
    }
    var error = new Error(message);
    error.errno = errno;
    error.code = code;
    error.syscall = "spawnSync";
    return error;
}
