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

    test("additional pipe", function() {
        var options = {
            stdio: ["pipe", "pipe",  "pipe",  "pipe",  "pipe"],
            encoding: "utf8",
        };
        var res = runsync.popen("echo hello 1>&3 && echo world 1>&4", options);
        equal(res.output[3], "hello\n");
        equal(res.output[4], "world\n");
    });

    test("nest inherit", function() {
        var res = runsync.spawn("node", ["test/spawn_script/nest_inherit.js"], { encoding: "utf8" });
        equal(res.stdout, "grandchild process\n");
    });

    test("set uid", function() {
        if(process.getuid) {
            var uid = process.getuid();
            var res = runsync.popen("id -u", { uid: uid, encoding: "utf8" });
            equal(res.stdout, uid + "\n");
            equal(res.error, undefined);
        }
    });

    test("set invalid uid", function() {
        if(process.getuid) {
            var res = runsync.popen("id -u", { uid: 0 });
            assert.instanceOf(res.error, Error);
            equal(res.error.code, "EPERM");
        }
    });

    test("set gid", function() {
        if(process.getgid) {
            var gid = process.getgid();
            var res = runsync.popen("id -u", { gid: gid, encoding: "utf8" });
            equal(res.stdout, gid + "\n");
            equal(res.error, undefined);
        }
    });

    test("set invalid gid", function() {
        if(process.getgid) {
            var res = runsync.popen("id -u", { gid: 0 });
            assert.instanceOf(res.error, Error);
            equal(res.error.code, "EPERM");
        }
    });

    test("killSignal", function() {
        var res = runsync.popen("sleep 10", { killSignal: "SIGKILL", timeout: 20 });
        equal(res.signal, "SIGKILL");
    });
});
