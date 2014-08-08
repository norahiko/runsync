"use strict";

var constants = require("constants");
var assert = require("assert");
var fs = require("fs");
var pathModule = require("path");
var util = require("util");

var isWindows = process.platform === "win32";
var shell = isWindows ? "cmd.exe" : "/bin/sh";
var tmpdir = require("os").tmpdir();
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

    var proc = new SyncProcess(file, args, options);
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


/**
 * SyncProcess class
 */

function SyncProcess(file, args, options) {
    this.file = file;
    this.args = args.slice(0);
    this.args.unshift(this.file);
    this.options = options ? util._extend({}, options) : {};
    this.normalizeOptions(this.options);
    this.initStdioPipe(this.options);
}

SyncProcess.prototype.run = function() {
    var result = polyfill.spawnSync(this.file, this.args, this.options);
    this.readOutput(result);
    this.setErrorObject(result);
    return result;
};

SyncProcess.prototype.normalizeOptions = function (opts) {
    var envPairs = [];
    for(var name in opts.env) {
        envPairs.push(name + "=" + opts.env[name]);
    }
    opts.envPairs = envPairs;

    if(opts.encoding === "buffer") {
        opts.encoding = null;
    }
    opts.input = opts.input || "";
    opts.stdio = opts.stdio || "pipe";
};

SyncProcess.prototype.initStdioPipe = function(opts) {
    if(opts.stdio === "pipe" || opts.stdio === "inherit" || opts.stdio === "ignore") {
        opts.stdio = [opts.stdio, opts.stdio, opts.stdio];
    }
    assert(opts.stdio instanceof Array, "runsync: options.stdio");
    while(opts.stdio.length < 3) {
        opts.stdio.push(null);
    }

    var input = opts.input;
    opts.stdio.forEach(function(pipe, i) {
        if(pipe === "pipe") {
            var filename = getTempfileName();
            fs.writeFileSync(filename, i === 0 ? input : "");
            opts.stdio[i] = filename;
        } else if(pipe === null || pipe === undefined) {
            opts.stdio[i] = "ignore";
        } else if(typeof pipe !== "number" && pipe !== "inherit" && pipe !== "ignore") {
            throw new Error("runsync: Invalid stdio option '" + pipe + "'");
        }
    });
};

SyncProcess.prototype.readOutput = function(res) {
    var encoding = this.options.encoding;
    res.output = this.options.stdio.map(function(pipe, i) {
        if(typeof pipe === "number" || pipe === "inherit" || pipe === "ignore") {
            return null;
        } else {
            var out = fs.readFileSync(pipe);
            if(encoding) {
                out = out.toString(encoding);
            }
            return out;
        }
    });
    res.stdout = res.output[1];
    res.stderr = res.output[2];
};

SyncProcess.prototype.setErrorObject = function(res) {
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

function getTempfileName() {
    var name = "runsync_" + Date.now() + "_" + Math.random().toString().slice(2);
    return pathModule.join(tmpdir, name);
}

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
