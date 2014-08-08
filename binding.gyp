{
  "conditions": [
    ["OS=='win'", {
      "targets": [
        {
          "target_name": "runsync",
          "sources": [ "src/runsync.cc"],
        }, {
          "target_name": "runsync_main",
          "type": "executable",
          "sources": ["src/win/runsync_main.cc"],
          "include_dirs": ["node_modules/nan"],
        }
      ],
    }, {
      "targets": [
        {
          "target_name": "runsync",
          "sources": [ "src/runsync.cc", "src/unix/runner.cc"],
          "include_dirs": ["node_modules/nan"],
        },
      ]
    }],
  ]
}
