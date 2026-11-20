#!/usr/bin/env bash
# Build + verifikasi kernel Sawit/S++ Multiboot 2 di WSL.
# Jalankan dari WSL:  bash tests/multiboot.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

mkdir -p build

# Pilih implementasi: SAWIT_BIN, atau Go binary, atau port Python.
if [ -n "${SAWIT_BIN:-}" ]; then
  CMD="$SAWIT_BIN"
elif command -v go >/dev/null 2>&1; then
  go build -o sawit .
  CMD="$ROOT/sawit"
else
  CMD="python3 $ROOT/sawit_transpiler.py"
fi

ELF="$ROOT/build/kernel.elf"
echo "=== 1. Build kernel multiboot ==="
$CMD build examples/wowokos-kernel/src/main.sawitpp --multiboot -o "$ELF"
[ -f "$ELF" ] || { echo "GAGAL: $ELF tidak dibuat"; exit 1; }

echo "=== 2. Verifikasi ELF32 + header Multiboot 2 ==="
readelf -h "$ELF" | grep -q 'Class:.*ELF32' || { echo "GAGAL: bukan ELF32"; exit 1; }
ENTRY_HEX=$(readelf -h "$ELF" | awk '/Entry point address/{print $NF}' | sed 's/^0x//')
ENTRY_DEC=$((16#$ENTRY_HEX))
# _start mengikuti header Multiboot (~24 byte), jadi entry sedikit di atas 1M.
if [ "$ENTRY_DEC" -lt $((0x100000)) ] || [ "$ENTRY_DEC" -ge $((0x200000)) ]; then
  echo "GAGAL: entry 0x$ENTRY_HEX di luar region 1MB"
  exit 1
fi
echo "Entry 0x$ENTRY_HEX OK (region 1MB)"
# Cek langsung dari byte ELF (urutan byte little-endian maupun terbalik),
# lebih andal daripada bergantung format tampilan readelf/objdump.
MAGIC_AT=$(python3 - "$ELF" <<'PY' || true
import sys
data = open(sys.argv[1], "rb").read()
for pattern in (b"\xd6\x50\x52\xe8", b"\xe8\x52\x50\xd6"):
    offset = data.find(pattern)
    if offset >= 0:
        print(hex(offset))
        break
PY
)
if [ -z "$MAGIC_AT" ]; then
  echo "GAGAL: magic Multiboot 2 tidak ketemu di ELF"
  echo "--- section header ---"
  readelf -S "$ELF" | head -40
  echo "--- dump .text (awal) ---"
  readelf -x .text "$ELF" 2>&1 | head -8
  exit 1
fi
echo "Magic Multiboot 2 ditemukan di offset $MAGIC_AT"
MAGIC_DEC=$((16#${MAGIC_AT#0x}))
if [ "$MAGIC_DEC" -ge 8192 ]; then
  echo "GAGAL: magic di offset $MAGIC_AT — di luar jendela 8KB QEMU"
  exit 1
fi
echo "ELF32 / entry 1MB / magic <8KB: OK"

echo "=== 3. Siapkan media boot ==="
ISO="$ROOT/build/wowokos.iso"
if sh "$ROOT/examples/wowokos-kernel/buatIso.sh" "$ROOT/build" "$ISO" "$ELF"; then
  echo "ISO GRUB siap: $ISO"
else
  echo "Pembuat ISO tidak tersedia — fallback ke -kernel (bisa kena limit QEMU)."
  BOOT_ARGS=(-kernel "$ELF")
fi

echo "=== 4. Smoke test QEMU (3 detik, tanpa display) ==="
QEMU_BIN=$(command -v qemu-system-i386 || command -v qemu-system-x86_64)
echo "QEMU: $QEMU_BIN"

try_boot() {
  local label="$1"; shift
  set +e
  timeout 3 "$QEMU_BIN" "$@" -m 128 -no-reboot -display none \
    >"$ROOT/build/qemu.log" 2>&1
  local rc=$?
  set -e
  if [ "$rc" -eq 124 ] || [ "$rc" -eq 0 ]; then
    echo "BOOT_SMOKE_OK via $label (rc=$rc — kernel berjalan sampai timeout)"
    BOOT_MODE=("$@")
    return 0
  fi
  echo "Gagal via $label (rc=$rc): $(head -2 "$ROOT/build/qemu.log" | tr '\n' ' ')"
  return 1
}

BOOT_MODE=()
if ! try_boot "-kernel (Multiboot 1)" -kernel "$ELF"; then
  if [ -f "$ISO" ]; then
    if ! try_boot "ISO GRUB" -boot d -cdrom "$ISO"; then
      echo "BOOT_SMOKE_GAGAL (semua jalur):"
      cat "$ROOT/build/qemu.log"
      echo "--- ISO info ---"
      file "$ISO" || true
      xorriso -indev "$ISO" -report_el_torito plain 2>&1 | head -20 || true
      echo "--- program header ELF ---"
      readelf -l "$ELF" | head -40 || true
      exit 1
    fi
  else
    echo "BOOT_SMOKE_GAGAL: -kernel gagal dan ISO tidak tersedia"
    cat "$ROOT/build/qemu.log"
    exit 1
  fi
fi

echo ""
echo "Untuk melihat layar VGA secara interaktif:"
echo "  $QEMU_BIN ${BOOT_MODE[*]} -m 128 -no-reboot -no-shutdown"
echo "Harus tampil: splash SAWIT lalu '=== WowokOS Kernel ==='"
if [ -f "$ISO" ]; then
  echo "ISO juga bisa dibooting di VMware: $ISO"
else
  echo "Untuk ISO VMware: sudo apt install -y grub-common xorriso mtools"
fi
