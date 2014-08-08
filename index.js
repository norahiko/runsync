"use strict";

var constants = require("constants");
var assert = require("assert");
var fs = require("fs");
var pathModule = require("path");
var util = require("util");

var isWindows = process.platform === "win32";
var shell = isWindows ? "cmd.exe" : "/bin/sh";
var tmpdir = require("os").tmpdir();
var binding = require("./build/Release/runsync.node");
var errorCodeMap = Object.create(null);

for(var code in constants) {
    if(code[0] === "E" && ! (constants[code] in errorCodeMap)) {
        errorCodeMap[constants[code]] = code;
    }
}


/**
 * exports
 */

exports.binding = binding;

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
        var msg = "Command failed: `" + command + "`";
        if(result.stderr) {
            msg += "\n" + result.stderr.toString();
        }
        throw new Error(msg);
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
    var result = binding.spawnSync(this.file, this.args, this.options);
    this.readOutput(result);
    this.setSpawnError(result);
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
        } else if(i === 0) {
            fs.unlinkSync(pipe);
            return null;
        } else {
            var out = fs.readFileSync(pipe);
            fs.unlinkSync(pipe);
            if(encoding) {
                out = out.toString(encoding);
            }
            return out;
        }
    });
    res.stdout = res.output[1];
    res.stderr = res.output[2];
};

SyncProcess.prototype.setSpawnError = function(res) {
    if(res._hasTimedOut) {
        delete res._hasTimedOut;
        res.error = createSpawnError(constants.ETIMEDOUT, "");
    }

    if(res._errmsg) {
        var msg = res._errmsg;
        delete res._errmsg;
        var match = /^(\d+) (.*)/.exec(msg);
        res.error = createSpawnError(Number(match[1]), match[2]);
    }

    if(res.error) {
        res.output = null;
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

function createSpawnError(errno, msg) {
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
