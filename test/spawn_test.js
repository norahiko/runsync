"use strict";

var runsync = require("../index.js");
var chai = require("chai");
var fs = require("fs");

var assert = chai.assert;
var equal = assert.strictEqual;
var deepEqual = assert.deepEqual;

suite("spawn:", function() {
    test("echo", function() {
        var res = runsync.spawn("echo", ["hello", "world"]);

        equal(res.file, "echo");
        assert.deepEqual(res.args, ["echo", "hello", "world"]);
        equal(typeof res.pid, "number");
        equal(res.status, 0);
        equal(res.signal, null);
        equal(res.error, undefined);

        equal(res.output.length, 3);
        equal(res.stdout.constructor, Buffer);
        equal(res.stdout.toString(), "hello world\n");
        equal(res.stderr.constructor, Buffer);
        equal(res.stderr.toString(), "");
    });

    test("echo encoding: utf8", function () {
        var res = runsync.spawn("echo", ["hello"], { encoding: "utf8" });
        equal(res.stdout, "hello\n");
        equal(res.stderr, "");
        deepEqual(res.output, [null, "hello\n", ""]);
    });

    test("not exists", function() {
        var res = runsync.spawn("not_exists_exe");

        equal(res.status, 127);
        equal(res.signal, null);
        equal(res.stdout, null);
        equal(res.stderr, null);

        equal(res.error.constructor, Error);
        equal(res.error.code, "ENOENT");
    });

    test("sleep timeout", function() {
        var res = runsync.spawn("sleep", ["1"], { timeout: 20 });
        equal(res.signal, "SIGTERM");
        equal(res.error.code, "ETIMEDOUT");
    });

    test("spawn uppercase.js", function() {
        var input = "hello, world";
        var path = "test/spawn_script/uppercase.js";
        var res = runsync.spawn("node", [path], { input: input, encoding: "utf8" });
        equal(res.stdout, input.toUpperCase());
    });

    test("spawn delay.js", function() {
        var path = "test/spawn_script/delay.js";
        var res = runsync.spawn("node", [path], { timeout: 3000, encoding: "utf8"});
        equal(res.stdout, "I'm late!\n");
    });

    test("redirect", function() {
        var outfile = "test/redirect_output.txt";
        var errfile = "test/redirect_error.txt";
        var out = fs.openSync(outfile, "w");
        var err = fs.openSync(errfile, "w");
        var args = ["-e", "console.log('stdout'); console.error('stderr');"];
        runsync.spawn("node", args, { stdio: ["pipe", out, err] });

        fs.closeSync(out);
        fs.closeSync(err);
        equal(fs.readFileSync(outfile).toString(), "stdout\n");
        equal(fs.readFileSync(errfile).toString(), "stderr\n");
    });

});
