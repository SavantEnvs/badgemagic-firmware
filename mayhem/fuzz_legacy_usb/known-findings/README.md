# Known upstream findings (fuzz_legacy_usb)

- `transfer-then-extra-packet.bin`: a complete 7764-byte session (header declaring one 700-column bitmap, so
  the transfer is longer than 4 KB) followed by one more 64-byte transfer. `legacy_usb_rx`
  (`src/legacyctrl.c:164`) never resets `rx_len`/`data_len`/`data` after completion (`data` was freed), so the
  next transfer is copied to `data + rx_len` of the freed buffer (ASan heap-buffer-overflow / use-after-free).
- By reading: the copy is not bounded by the declared length, and `buf[0..7]` is read regardless of `len`.

Replay: `mayhem/build/fuzz_legacy_usb mayhem/fuzz_legacy_usb/known-findings/transfer-then-extra-packet.bin`
