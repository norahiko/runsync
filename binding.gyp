{
  "targets": [
    {
      "target_name": "runsync",
      "sources": ["src/runsync.cc"],
      "include_dirs": ["<!(node -e \"require('nan')\")"]
    },
  ]
}
