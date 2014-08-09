"use strict";

var runsync = require("../index.js");
var chai = require("chai");

var assert = chai.assert;
var equal = assert.strictEqual;
var deepEqual = assert.deepEqual;

suite("options:", function() {
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

    test("env", function() {
        var res = runsync.popen("echo $env_value", { env: { env_value: "hello" }, encoding: "utf8" });
        equal(res.stdout, "hello\n");
    });

    test("encoding: buffer", function() {
        var res = runsync.spawn("echo", ["hello"], { encoding: "buffer" });
        assert(res.stdout instanceof Buffer);
        equal(res.stdout.toString(), "hello\n");
    });

    test("ignore output", function() {
        var res = runsync.popen("echo hello", { stdio: [null] });
        deepEqual(res.output, [null, null, null]);
    });

    test("invalid pipe", function() {
        assert.throws(function () {
            runsync.spawn("echo", ["hello"], { stdio: ["foo", "bar", "baz"] });
        });
    });
});
