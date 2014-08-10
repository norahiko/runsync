"use strict";

var runsync = require("../index.js");
var chai = require("chai");

var assert = chai.assert;
var equal = assert.strictEqual;

suite("execFile:", function() {
    test("success", function() {
        var output = runsync.execFile("echo", ["hello"], { encoding: "utf8" });
        equal(output, "hello\n");
    });

    test("fail", function() {
        assert.throws(function () {
            runsync.execFile("not_exists_exe", ["arg", "arg"]);
        }, "Command failed");
    });
});
