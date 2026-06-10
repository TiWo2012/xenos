#!/bin/sh
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Building ==="
cmake -B build -G Ninja 2>&1
cmake --build build 2>&1

echo ""
echo "=== Running E2E tests in QEMU ==="
OUTFILE=build/qemu_test_output.txt
# nographic implies serial stdio
timeout 30 qemu-system-x86_64 \
  -cdrom build/os.iso \
  -nographic \
  -no-reboot \
  < /dev/null 2>&1 | tee "$OUTFILE" || true

echo ""
echo "=== Checking results ==="
if grep -q "ALL TESTS PASSED" "$OUTFILE"; then
  echo "========================================"
  echo "  *** E2E TESTS PASSED ***"
  echo "========================================"
  exit 0
else
  echo "========================================"
  echo "  *** E2E TESTS FAILED ***"
  echo "========================================"
  echo ""
  echo "Last 30 lines of output:"
  tail -30 "$OUTFILE"
  echo ""
  exit 1
fi
