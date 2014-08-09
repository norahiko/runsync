"use strict";

var runsync = require("../index.js");
var chai = require("chai");

var assert = chai.assert;
var equal = assert.strictEqual;

suite("shell:", function() {
    test("exit 0", function() {
        var res;
        assert.doesNotThrow(function() {
            res = runsync.shell("exit 0");
        });
        equal(res, undefined);
    });

    test("exit 1", function() {
        assert.throws(function () {
            runsync.shell("exit 1");
        }, "Command failed");
    });
});
