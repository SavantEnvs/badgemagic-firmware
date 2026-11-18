# Known upstream findings (fuzz_ng_parse)

Reproducers are kept here, not in `testsuite/`, so they are not replayed as seeds.

- `streaming_setting-oob-read.bin`: `ng_parse` dispatches a zero-length payload to `streaming_setting`
  (`src/main.c:385`), which reads `params[0]` with `len == 0` (ASan heap-buffer-overflow read).
  The same missing `len` check shows up in `cfg_ble_alwayon` (`src/ngctrl.c:69`), `cfg_reset_rx`
  (`src/ngctrl.c:25`) and `cfg_led_brightness` (`src/ngctrl.c:156`).

Replay: `mayhem/build/fuzz_ng_parse-standalone mayhem/fuzz_ng_parse/known-findings/streaming_setting-oob-read.bin`
