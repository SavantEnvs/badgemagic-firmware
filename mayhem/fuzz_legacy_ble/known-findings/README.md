# Known upstream findings (fuzz_legacy_ble)

- `legacy_ble_rx-heap-overflow.bin`: `legacy_ble_rx` (`src/legacyctrl.c:112`) reallocs the image buffer to
  `64 + 11*n` bytes (header-supplied bitmap count) but copies a full 16-byte packet per call, so any
  `data_len` that is not a multiple of 16, or `n == 0`, overflows the heap buffer.
- After a completed transfer `data` is NULL but the packet counter is never reset, so a following packet
  without a header computes `NULL + c*16` (UBSan "applying non-zero offset to null pointer", same line).

Input format: 3-byte session preamble (byte 0 bit 0 = PIN gate closed, bytes 1..2 = PIN), then the 16-byte writes;
the reproducer uses preamble `00 00 00` (gate open).

Replay: `mayhem/build/fuzz_legacy_ble-standalone mayhem/fuzz_legacy_ble/known-findings/legacy_ble_rx-heap-overflow.bin`
