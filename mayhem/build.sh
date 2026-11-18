set -euo pipefail

# clang rejects SOURCE_DATE_EPOCH='' (empty) — it must be unset or a valid integer.
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

# Build knobs come from the ENVIRONMENT (overridable), with sane defaults — no if-statements,
# just parameter-expansion fallbacks. Default sanitizers are the base's ASan+UBSan (halting); override
# per build via the Dockerfile's `--build-arg SANITIZER_FLAGS="..."`.
# NB: SANITIZER_FLAGS uses `=` (no colon) on purpose — `=` only fills when the var is UNSET, so an
# explicit EMPTY value (`--build-arg SANITIZER_FLAGS=`) is honored and builds with NO sanitizers
# (useful when you want the program's natural crash / full backtrace, not an ASan report). The other
# knobs use `:=` (default on empty too). MAYHEM_JOBS sets build parallelism (falls back to nproc).
: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
# DEBUG_FLAGS carries DWARF debug info INDEPENDENTLY of the sanitizer off-switch (so an empty
# SANITIZER_FLAGS still yields DWARF symbols). DWARF MUST be < 4 (Mayhem triage can't read >=4); clang-19's
# plain `-g` emits DWARF-5, so `-gdwarf-3` is explicit. Apply $DEBUG_FLAGS to the fuzz/harness/standalone
# builds (NOT the test/oracle build). Rust/Go carry the same intent via their language flags below.
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${RUST_DEBUG_FLAGS:=-C debuginfo=2 -C force-frame-pointers=yes -C llvm-args=-gdwarf-3}"
: "${GO_DEBUG_FLAGS:=-gcflags=all=-N -l}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
# COVERAGE_FLAGS: empty by default → no effect on the normal oracle build. Set it via the Dockerfile's
# `--build-arg COVERAGE_FLAGS="-fprofile-instr-generate -fcoverage-mapping"` to instrument the TEST
# build for source-coverage measurement (how much of the project the test suite actually exercises —
# a quality signal for the oracle; complements the anti-reward-hack sabotage check). APPEND it to the
# test build's compile+link flags in step 3 (NOT the fuzz build); after `test.sh` runs, merge with
# `llvm-profdata` and report with `llvm-cov`. Empty value is honored (`=`, not `:=`).
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS RUST_DEBUG_FLAGS GO_DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

# The firmware targets a WCH CH582 RISC-V MCU. It is built for the host by compiling upstream src/*.c
# unmodified against mayhem/shim (fake vendor headers + hardware stubs); only hardware is stubbed.
# -fno-sanitize=function is the one relaxed UBSan check: upstream's command tables (cmd_lut, ble_setting)
# are called through ABI-identical but differently-typed function pointers. ASan and the rest of UBSan halt.
SHIM_FLAGS="-include mayhem/shim/hw.h -Imayhem/shim -Isrc -DHW_KEY_COUNT=4 -DTHIS_SLOT_A -DVERSION=\"x\" -DVERSION_ABBR=\"x\" -DCLK_OSC32K=2 -w -Wno-incompatible-function-pointer-types -Wno-incompatible-pointer-types -Wno-int-conversion"
SOURCES="mayhem/shim/main_host.c mayhem/shim/shim.c mayhem/shim/peripheral_host.c src/ngctrl.c src/legacyctrl.c src/data.c src/bmlist.c src/config.c src/xbm.c src/font.c src/font3x5.c src/resource.c src/animation.c src/util.c src/util/crc.c src/button.c src/auxbtn.c src/game.c src/flappy.c src/pong.c src/debug.c src/power.c"
FUZZ_FLAGS="$SANITIZER_FLAGS $DEBUG_FLAGS -fno-sanitize=function"
B=mayhem/build
rm -rf "$B"; mkdir -p "$B/o" "$B/to"

# 1) Sanitized project objects (SanCov on all library code).
for f in $SOURCES; do
  $CC $SHIM_FLAGS $FUZZ_FLAGS -fsanitize=fuzzer-no-link -c "$f" -o "$B/o/$(basename "$f").o"
done
$CXX $FUZZ_FLAGS -c mayhem/lsan_off.cc -o "$B/lsan_off.o"
$CC $FUZZ_FLAGS -x c -c "$STANDALONE_FUZZ_MAIN" -o "$B/standalone_main.o"

# 2) libFuzzer targets and their standalone reproducers.
for h in fuzz_ng_parse fuzz_legacy_ble fuzz_flash_image fuzz_ota_iap; do
  $CC $SHIM_FLAGS $FUZZ_FLAGS -fsanitize=fuzzer-no-link -c "mayhem/harness/$h.c" -o "$B/$h.o"
  $CXX $FUZZ_FLAGS $LIB_FUZZING_ENGINE "$B/$h.o" "$B"/o/*.o "$B/lsan_off.o" -o "$SRC/$B/$h"
  $CXX $FUZZ_FLAGS "$B/standalone_main.o" "$B/$h.o" "$B"/o/*.o "$B/lsan_off.o" -o "$SRC/$B/$h-standalone"
done

# 3) File-input target (the USB path keeps un-resettable static state, so one process per input).
$CC $SHIM_FLAGS $FUZZ_FLAGS -fsanitize=fuzzer-no-link -c mayhem/harness/fuzz_legacy_usb.c -o "$B/fuzz_legacy_usb.o"
$CXX $FUZZ_FLAGS "$B/fuzz_legacy_usb.o" "$B"/o/*.o "$B/lsan_off.o" -o "$SRC/$B/fuzz_legacy_usb"

# 4) Oracle: the same firmware sources, normal flags, no sanitizers.
for f in $SOURCES; do
  $CC $SHIM_FLAGS -O1 $COVERAGE_FLAGS -c "$f" -o "$B/to/$(basename "$f").o"
done
$CC $SHIM_FLAGS -O1 $COVERAGE_FLAGS mayhem/harness/selftest.c "$B"/to/*.o -o "$SRC/$B/badgemagic-selftest"
