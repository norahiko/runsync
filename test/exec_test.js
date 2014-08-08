"use strict";

var runsync = require("../index.js");
var chai = require("chai");

var assert = chai.assert;
var equal = assert.strictEqual;

suite("exec:", function() {
    var expected;
    var err = process.stderr.write;

    setup(function () {
        process.stderr.write = function(str) {
            expected = str;
        };
    });

    teardown(function () {
        process.stderr.write = err;
    });


    test("exec ok", function() {
        var res = runsync.exec("echo ok");
        equal(res.toString(), "ok\n");
    });

    test("exec fail", function() {
        assert.throws(function () {
            runsync.exec("echo error 1>&2 && exit 1");
        }, "Command failed");

        equal(expected, "error\n");
    });

    test("throw Error", function() {
        assert.throws(function () {
            runsync.exec("node -e \"throw new Error('exec error');\"");
        }, "Command failed");

        assert(/exec error/.test(expected));
    });
});
