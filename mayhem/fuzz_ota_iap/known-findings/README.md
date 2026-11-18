# Known upstream findings (fuzz_ota_iap)

Reproducers are kept here, not in `testsuite/`, so they are not replayed as seeds.

- `prom-len-oob-read.bin`: a `CMD_IAP_PROM` write whose own `len` field is 244 (`src/ble/peripheral.c:406`).
  `Rec_OTA_IAP_DataDeal` passes it straight to `FLASH_ROM_WRITE(addr, iap_rec_data.program.buf, len)`, but
  `program.buf` is only `IAP_LEN - 4` = 243 bytes of the 247-byte `iap_rec_data`, so the flash write reads
  past the union (ASan global-buffer-overflow read). `CMD_IAP_VERIFY` (`peripheral.c:469`) does the same
  with `verify.len` for `FLASH_ROM_VERIFY`. The `len` field is attacker-controlled and unchecked.

Replay: `mayhem/build/fuzz_ota_iap-standalone mayhem/fuzz_ota_iap/known-findings/prom-len-oob-read.bin`
