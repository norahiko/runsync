{
  "conditions": [
    ["OS=='win'", {
      "targets": [
        {
          "target_name": "polyfill",
          "sources": [ "src/spawnSync.cc"],
        }, {
          "target_name": "spawn_main",
          "type": "executable",
          "sources": ["src/win/spawn_main.cc"],
        }
      ],
    }, {
      "targets": [
        {
          "target_name": "polyfill",
            "sources": [ "src/spawnSync.cc", "src/unix/runner.cc"],
        },
      ]
    }],
  ]
}
