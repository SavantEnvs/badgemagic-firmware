# Known upstream findings (fuzz_flash_image)

Reproducers are kept here, not in `testsuite/`, so they are not replayed as seeds.

- `animation-index-oob.bin`: a stored bitmap's mode byte selects `animations[LEGACY_GET_ANIMATION(bm->modes)]`
  (`src/main.c:238`); the selector is 0..15 but the table has 9 entries, so an out-of-range value reads
  past the table and calls through it (UBSan halts).
- `empty-bmlist-null-deref.bin`: a valid `wang` header whose bitmap sizes are all zero makes `load_bmlist`
  (`src/main.c:190`) drop the only bitmap and leave the list empty; the next animation step then
  dereferences `bmlist_current() == NULL` (`src/main.c:238`, UBSan null member access).

Replay: `mayhem/build/fuzz_flash_image-standalone mayhem/fuzz_flash_image/known-findings/<file>.bin`
