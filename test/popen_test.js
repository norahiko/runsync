"use strict";

var runsync = require("../index.js");
var chai = require("chai");
var fs = require("fs");

var assert = chai.assert;
var equal = assert.strictEqual;

suite("popen:", function() {
    test("echo stdout and stderr", function() {
        var res = runsync.popen("echo hello && echo world 1>&2", { encoding: "utf8" });

        equal(typeof res.file, "string");
        equal(res.args.constructor, Array);
        equal(typeof res.pid, "number");
        equal(res.status, 0);
        equal(res.signal, null);
        equal(res.error, undefined);

        equal(res.output.length, 3);
        equal(res.stdout, "hello\n");
        equal(res.stderr, "world\n");
    });

    test("read stdin", function() {
        var res = runsync.popen("cat", { input: "hello", encoding: "utf8" });
        equal(res.stdout, "hello");
    });

    test("redirect output", function() {
        var res = runsync.popen("echo hello > test/output.txt");
        equal(res.stdout.length, 0);
        equal(fs.readFileSync("test/output.txt").toString(), "hello\n");
    });

    test("redirect input", function() {
        fs.writeFileSync("test/input.txt", "hello");
        var res = runsync.popen("cat < test/input.txt");
        equal(res.stdout.toString(), "hello");
    });

    test("exit 123", function() {
        var res = runsync.popen("echo fail && exit 123", { encoding: "utf8" });
        equal(res.stdout, "fail\n");
        equal(res.status, 123);
        equal(res.error, undefined);
    });

    test("not exists", function() {
        var res = runsync.popen("not_exists_exe");
        equal(res.status, 127);
        equal(res.error, undefined);
    });

    test("node stdout and stderr", function() {
        var script = [
            "console.log('hello');",
            "console.error('world');",
            "throw new Error('node error')",
        ].join("");
        var command = "node -e \"" + script + "\"";

        var res = runsync.popen(command, { encoding: "utf8" });
        assert.notEqual(res.status, 0);
    });

    test("timeout", function () {
        var res = runsync.popen("sleep 3", { timeout: 30 });
        equal(res.signal, "SIGTERM");
        assert.notEqual(res.status, 0);

        equal(res.error.constructor, Error);
        equal(res.error.code, "ETIMEDOUT");
    });

    test("echo $PWD", function() {
        var res = runsync.popen("echo $PWD", { cwd: "/", encoding: "utf8" });
        equal(res.stdout, "/\n");
    });

    test("invalid cwd path", function() {
        var res = runsync.popen("echo $PWD", { cwd: "/not_exits_dir" });
        assert.notEqual(res.status, 0);
        equal(res.stdout, null);
        equal(res.stderr, null);
        equal(res.error.constructor, Error);
        equal(res.error.code, "ENOENT");
    });

    test("additional pipe", function() {
        var options = {
            stdio: ["pipe", "pipe",  "pipe",  "pipe",  "pipe"],
            encoding: "utf8",
        };
        var res = runsync.popen("echo hello 1>&3 && echo world 1>&4", options);
        equal(res.output[3], "hello\n");
        equal(res.output[4], "world\n");
    });
});
