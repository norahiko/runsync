process.stdin.on("readable", function () {
    var data = process.stdin.read();
    if(data !== null) {
        process.stdout.write(data.toString().toUpperCase());
    }
});
