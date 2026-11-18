#!/usr/bin/env bash
# Runs the pre-built firmware known-answer suite (mayhem/harness/selftest.c) and reports CTRF.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

runner="$SRC/mayhem/build/badgemagic-selftest"
if [ ! -x "$runner" ]; then
  echo "missing $runner (mayhem/build.sh must build it)" >&2
  emit_ctrf "badgemagic-selftest" 0 1 0
  exit 1
fi

out=$("$runner" 2>&1); rc=$?
printf '%s\n' "$out" | grep -v '^PASS' || true
summary=$(printf '%s\n' "$out" | sed -n 's/^SUMMARY: \([0-9][0-9]*\) tests run; \([0-9][0-9]*\) failed$/\1 \2/p' | tail -n1)
if [ -z "$summary" ]; then
  echo "no SUMMARY line from $runner (rc=$rc)" >&2
  emit_ctrf "badgemagic-selftest" 0 1 0
  exit 1
fi
set -- $summary
total=$1; failed=$2
passed=$(( total - failed ))
[ "$rc" -eq 0 ] || [ "$failed" -gt 0 ] || failed=1
emit_ctrf "badgemagic-selftest" "$passed" "$failed" 0
