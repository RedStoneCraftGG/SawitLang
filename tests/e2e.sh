#!/usr/bin/env bash
# End-to-end + parity test suite for Sawit/S++.
set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT" || exit 1
GO="./sawit.exe"
PY="python sawit_transpiler.py"
TMP="/tmp/sawit-e2e"
rm -rf "$TMP"; mkdir -p "$TMP"
PASS=0; FAIL=0
ok(){ PASS=$((PASS+1)); printf "  PASS  %s\n" "$1"; }
no(){ FAIL=$((FAIL+1)); printf "  FAIL  %s\n" "$1"; }
chk(){ if eval "$2" >/dev/null 2>&1; then ok "$1"; else no "$1"; fi; }

echo "=== 0. Build implementasi utama (Go) ==="
go build -o sawit.exe . 2>&1 && ok "go build" || no "go build"

echo ""
echo "=== 1. Paritas transpile Go vs Python (byte-identik) ==="
for f in examples/*.sawit examples/*.sawitpp examples/wowokos-kernel/src/*.sawitpp; do
  $GO transpile "$f" > "$TMP/go.out" 2>/dev/null
  $PY transpile "$f" > "$TMP/py.out" 2>/dev/null
  if diff -q "$TMP/go.out" "$TMP/py.out" >/dev/null 2>&1; then ok "parity $f"; else no "parity $f"; fi
done

echo ""
echo "=== 2. Contoh: build + run (Go) ==="
out=$($GO run examples/hello.sawit 2>&1); echo "$out" | grep -q "Hidup Jokowi!" && ok "hello run" || no "hello run"
out=$($GO run examples/resource.sawit 2>&1); echo "$out" | grep -q "Dana: 500, cair: 200" && ok "resource run" || no "resource run"
out=$($GO run examples/runtime.sawit 2>&1)
echo "$out" | grep -q "Nyawit!" && ok "runtime nyawit" || no "runtime nyawit"
echo "$out" | grep -q "Dana cair: 890" && ok "runtime ledger" || no "runtime ledger"
echo "$out" | grep -q "Pesan ini dibungkam" && no "runtime mute (leaked)" || ok "runtime mute"

echo ""
echo "=== 3. Semantik runtime (sintetis .sawitpp) ==="
cat > "$TMP/rt.sawitpp" <<'EOF'
imporBibit <cstdio>
hutan tolong() { buzzer("NYAWIT-OK\n"); }
tandan panenUtama() {
    tandan dana = MBG(500);
    tandan c1 = cair(200);
    tandan c2 = cair(1000);
    tandan t = 1000;
    pajak(t, 11);
    buzzer("dana=%d c1=%d c2=%d pajak=%d\n", dana, c1, c2, t);
    hutan *p = beli(16);
    buzzer("beli=%s\n", p != 0 ? "ok" : "null");
    jual(p);
    bungkam(stdout);
    buzzer("HARUSNYA-TERSEMBUNYI\n");
    bukaSuara(stdout);
    nyawit(tolong);
    demoMasyarakat("DEMO-OK");
    setor 0;
}
EOF
out=$($GO run "$TMP/rt.sawitpp" 2>&1)
echo "$out" | grep -q "dana=500 c1=200 c2=0 pajak=890" && ok "MBG/cair/pajak" || no "MBG/cair/pajak"
echo "$out" | grep -q "beli=ok" && ok "beli" || no "beli"
echo "$out" | grep -q "HARUSNYA-TERSEMBUNYI" && no "bungkam (leaked)" || ok "bungkam"
echo "$out" | grep -q "NYAWIT-OK" && ok "nyawit" || no "nyawit"
echo "$out" | grep -q "DEMO MASYARAKAT\] DEMO-OK" && ok "demoMasyarakat" || no "demoMasyarakat"

echo ""
echo "=== 3b. Anti double-free / pointer tak ter-lacak ==="
cat > "$TMP/free.sawit" <<'EOF'
imporBibit <stdio.h>
tandan panenUtama() {
    hutan *p = beli(16);
    jual(p);
    jual(p);
    buzzer("DOUBLE-JUAL-OK\n");
    tandan x = 5;
    jual(&x);
    buzzer("UNTRACKED-JUAL-OK\n");
    setor 0;
}
EOF
out=$($GO run "$TMP/free.sawit" 2>&1); rc=$?
[ $rc -eq 0 ] && echo "$out" | grep -q "DOUBLE-JUAL-OK" && echo "$out" | grep -q "UNTRACKED-JUAL-OK" && ok "double/untracked jual safe (Go)" || no "double/untracked jual safe (Go) rc=$rc"
out=$($PY run "$TMP/free.sawit" 2>&1); rc=$?
[ $rc -eq 0 ] && echo "$out" | grep -q "UNTRACKED-JUAL-OK" && ok "double/untracked jual safe (Py)" || no "double/untracked jual safe (Py) rc=$rc"

echo ""
echo "=== 3c. Input sadap + helper string (hosted) ==="
cat > "$TMP/input.sawitpp" <<'EOF'
imporBibit <cstdio>
tandan panenUtama() {
    buah baris[64];
    buah salinan[64];
    tandan angka = 0;
    sadapBaris(baris, 64);
    buzzer("dapat=%s panjang=%d sama=%d angka=%d\n", baris, (tandan)panjangKata(baris), samaKata(baris, "sawit"), terjemahAngka(baris));
    sadap("%d", &angka);
    sadapSisa();
    buzzer("angka=%d\n", angka);
    salinKata(salinan, baris);
    buzzer("salinan=%s\n", salinan);
    tutupTelinga();
    buzzer("telinga-tutup=%d\n", sadapBaris(baris, 64) == kosong);
    bukaTelinga();
    setor 0;
}
EOF
out=$(printf 'sawit\n42 sisa\n' | $GO run "$TMP/input.sawitpp" 2>&1)
echo "$out" | grep -q "dapat=sawit panjang=5 sama=1 angka=0" && ok "sadapBaris + string helpers" || no "sadapBaris + string helpers: $out"
echo "$out" | grep -q "angka=42" && echo "$out" | grep -q "salinan=sawit" && ok "sadap + sadapSisa + salinKata" || no "sadap + sadapSisa + salinKata: $out"
echo "$out" | grep -q "telinga-tutup=1" && ok "tutupTelinga" || no "tutupTelinga: $out"

echo ""
echo "=== 3d. Mode freestanding (kernel Sawit) ==="
$GO transpile examples/wowokos-kernel/src/main.sawitpp --freestanding > "$TMP/k-go.c" 2>/dev/null
$PY transpile examples/wowokos-kernel/src/main.sawitpp --freestanding > "$TMP/k-py.c" 2>/dev/null
if diff -q "$TMP/k-go.c" "$TMP/k-py.c" >/dev/null 2>&1; then ok "freestanding parity Go/Py"; else no "freestanding parity Go/Py"; fi
grep -q '#define SAWIT_FREESTANDING' "$TMP/k-go.c" && ok "freestanding preamble" || no "freestanding preamble"
grep -q 'sawit_outb' "$TMP/k-go.c" && ok "port I/O mapping" || no "port I/O mapping"
$GO build examples/wowokos-kernel/src/main.sawitpp --freestanding -o "$TMP/kernel-go.exe" >/dev/null 2>&1 && ok "go freestanding build" || no "go freestanding build"
$PY build examples/wowokos-kernel/src/main.sawitpp --freestanding -o "$TMP/kernel-py.exe" >/dev/null 2>&1 && ok "py freestanding build" || no "py freestanding build"
$GO run examples/wowokos-kernel/src/main.sawitpp --freestanding >/dev/null 2>&1 && no "run --freestanding harus ditolak" || ok "run --freestanding ditolak"

echo ""
echo "=== 3e. rampas/sita: transfer kepemilikan pointer ==="
cat > "$TMP/rampas.sawitpp" <<'EOF'
imporBibit <cstdio>
tandan panenUtama() {
    hutan *p = beli(32);
    hutan *q = kosong;
    rampas(&q, &p);
    buzzer("p=%s q=%s\n", p == kosong ? "kosong" : "ada", q != kosong ? "ada" : "kosong");
    jual(p);
    jual(q);
    jual(q);
    hutan *a = beli(16);
    hutan *b = kosong;
    sita(&b, &a);
    buzzer("sita: a=%s b=%s\n", a == kosong ? "kosong" : "ada", b != kosong ? "ada" : "kosong");
    jual(a);
    jual(b);
    rampas(&a, &a);
    buzzer("self-rampas aman\n");
    setor 0;
}
EOF
$GO transpile "$TMP/rampas.sawitpp" > "$TMP/r-go.c" 2>/dev/null
$PY transpile "$TMP/rampas.sawitpp" > "$TMP/r-py.c" 2>/dev/null
diff -q "$TMP/r-go.c" "$TMP/r-py.c" >/dev/null 2>&1 && ok "rampas parity Go/Py" || no "rampas parity Go/Py"
out=$($GO run "$TMP/rampas.sawitpp" 2>&1); rc=$?
[ $rc -eq 0 ] && echo "$out" | grep -q "p=kosong q=ada" && echo "$out" | grep -q "self-rampas aman" && ok "rampas transfer (Go)" || no "rampas transfer (Go) rc=$rc: $out"
out=$($PY run "$TMP/rampas.sawitpp" 2>&1); rc=$?
[ $rc -eq 0 ] && echo "$out" | grep -q "sita: a=kosong b=ada" && ok "sita alias (Py)" || no "sita alias (Py) rc=$rc: $out"

echo ""
echo "=== 3f. Mode --multiboot: transpile, aset boot, paritas ==="
$GO transpile examples/wowokos-kernel/src/main.sawitpp --multiboot > "$TMP/m-go.c" 2>/dev/null
$PY transpile examples/wowokos-kernel/src/main.sawitpp --multiboot > "$TMP/m-py.c" 2>/dev/null
diff -q "$TMP/m-go.c" "$TMP/m-py.c" >/dev/null 2>&1 && ok "multiboot parity Go/Py" || no "multiboot parity Go/Py"
grep -q '#define SAWIT_FREESTANDING' "$TMP/m-go.c" && ok "multiboot implies freestanding" || no "multiboot implies freestanding"
grep -q '0xE85250D6' boot/start.S && grep -q '_start:' boot/start.S && ok "start.S Multiboot 2" || no "start.S Multiboot 2"
grep -q 'ENTRY(_start)' boot/linker.ld && grep -q '1M' boot/linker.ld && ok "linker.ld entry 1MB" || no "linker.ld entry 1MB"
$GO run examples/wowokos-kernel/src/main.sawitpp --multiboot >/dev/null 2>&1 && no "run --multiboot harus ditolak" || ok "run --multiboot ditolak"

echo ""
echo "=== 4. Argumen program (--) Go vs Python ==="
cat > "$TMP/arg.sawitpp" <<'EOF'
imporBibit <cstdio>
tandan panenUtama(tandan argc, buah **argv) {
    muter (tandan i = 0; i < argc; i = i + 1) { buzzer("ARG[%d]=%s\n", i, argv[i]); }
    setor 0;
}
EOF
out=$($GO run "$TMP/arg.sawitpp" -- satu dua 2>&1)
echo "$out" | grep -q "ARG\[1\]=satu" && echo "$out" | grep -q "ARG\[2\]=dua" && ok "Go run -- args" || no "Go run -- args"
out=$($PY run "$TMP/arg.sawitpp" -- satu dua 2>&1)
echo "$out" | grep -q "ARG\[1\]=satu" && echo "$out" | grep -q "ARG\[2\]=dua" && ok "Py run -- args" || no "Py run -- args"

echo ""
echo "=== 5. Preservasi komentar/string/identifier ==="
cat > "$TMP/tok.sawit" <<'EOF'
// tandan panenUtama buzzer
tandan panenUtama() {
    buzzer("tandan panenUtama buzzer\n");
    tandan tandananku = 1;
    setor 0;
}
EOF
$GO transpile "$TMP/tok.sawit" > "$TMP/tok.c"
grep -q '// tandan panenUtama buzzer' "$TMP/tok.c" && ok "comment preserved" || no "comment preserved"
grep -q '"tandan panenUtama buzzer' "$TMP/tok.c" && ok "string preserved" || no "string preserved"
grep -q 'int tandananku' "$TMP/tok.c" && ok "identifier not split" || no "identifier not split"
grep -q 'int main()' "$TMP/tok.c" && ok "keyword replaced" || no "keyword replaced"

echo ""
echo "=== 6. Input rusak tidak boleh panic (Go) ==="
printf '%s' 'buzzer("abc\' > "$TMP/bad.sawit"
$GO transpile "$TMP/bad.sawit" >/dev/null 2>"$TMP/bad.err"; rc=$?
if [ $rc -eq 0 ] && ! grep -qi panic "$TMP/bad.err"; then ok "unterminated string no panic"; else no "unterminated string panic (rc=$rc)"; fi
printf '/* unclosed\n' > "$TMP/bad2.sawit"
$GO transpile "$TMP/bad2.sawit" >/dev/null 2>"$TMP/bad2.err"; rc=$?
if [ $rc -eq 0 ] && ! grep -qi panic "$TMP/bad2.err"; then ok "unclosed comment no panic"; else no "unclosed comment panic (rc=$rc)"; fi

echo ""
echo "=== 7. Pemilihan compiler (.sawit=gcc, .sawitpp=g++) ==="
cat > "$TMP/cls.sawit" <<'EOF'
perkebunan K { jalanProduksi: tandan x; };
tandan panenUtama() { setor 0; }
EOF
cp "$TMP/cls.sawit" "$TMP/cls.sawitpp"
$GO build "$TMP/cls.sawit" -o "$TMP/cls1.exe" >/dev/null 2>&1 && no "sawit(C) should reject class" || ok "sawit(C) uses gcc (class rejected)"
$GO build "$TMP/cls.sawitpp" -o "$TMP/cls2.exe" >/dev/null 2>&1 && ok "sawitpp(C++) uses g++ (class ok)" || no "sawitpp(C++) build"

echo ""
echo "=== 8. Flag -o / flag-sebelum-source / keep-generated ==="
$GO transpile examples/hello.sawit -o "$TMP/o1.c" >/dev/null 2>&1
grep -q 'int main()' "$TMP/o1.c" 2>/dev/null && ok "transpile -o" || no "transpile -o"
$GO transpile -o "$TMP/o2.c" examples/hello.sawit >/dev/null 2>&1
grep -q 'int main()' "$TMP/o2.c" 2>/dev/null && ok "flag-before-source" || no "flag-before-source"
before=$(ls -d examples/.sawit-build-* 2>/dev/null | wc -l)
$GO build examples/hello.sawit -o "$TMP/k.exe" --keep-generated >/dev/null 2>&1
after=$(ls -d examples/.sawit-build-* 2>/dev/null | wc -l)
[ "$after" -gt "$before" ] && ok "--keep-generated leaves dir" || no "--keep-generated leaves dir"
rm -rf examples/.sawit-build-* 2>/dev/null

echo ""
echo "=== 9. init / audit (Go & Python) ==="
rm -rf "$TMP/proj"
$GO init "$TMP/proj" >/dev/null 2>&1 && [ -f "$TMP/proj/src/main.sawit" ] && ok "init creates project" || no "init creates project"
[ -f "$TMP/proj/sawit_kernel.h" ] && ok "init creates kernel header" || no "init creates kernel header"
[ -f "$TMP/proj/boot/start.S" ] && [ -f "$TMP/proj/boot/linker.ld" ] && ok "init creates boot assets" || no "init creates boot assets"
$GO init "$TMP/proj" >/dev/null 2>&1 && no "init refuses existing" || ok "init refuses existing"
$GO init "$TMP/proj" --force >/dev/null 2>&1 && ok "init --force" || no "init --force"
( cd "$TMP/proj" && "$ROOT/sawit.exe" run src/main.sawit > "$TMP/proj.out" 2>&1 )
grep -q "Halo dari proyek Sawit!" "$TMP/proj.out" && ok "generated project runs" || no "generated project runs"
grep -Fq '${SAWIT_EXECUTABLE}' "$TMP/proj/AMDAL.txt" && ok "AMDAL DEPENDS cross-platform" || no "AMDAL DEPENDS cross-platform"
grep -q 'DEPENDS .*sawit\.exe' "$TMP/proj/AMDAL.txt" && no "AMDAL DEPENDS hardcodes sawit.exe" || ok "AMDAL DEPENDS no hardcoded sawit.exe"
if command -v cmake >/dev/null 2>&1 && command -v mingw32-make >/dev/null 2>&1; then
  rm -rf "$TMP/proj/build"
  if ( cd "$TMP/proj" && cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc >/dev/null 2>&1 && cmake --build build >/dev/null 2>&1 ); then
    if [ -f "$TMP/proj/build/panen.exe" ]; then cout=$("$TMP/proj/build/panen.exe" 2>&1); else cout=$("$TMP/proj/build/panen" 2>&1); fi
    echo "$cout" | grep -q "Halo dari proyek Sawit!" && ok "cmake build via AMDAL" || no "cmake build via AMDAL"
  else
    no "cmake build via AMDAL"
  fi
fi
( cd "$TMP/proj" && "$ROOT/sawit.exe" audit AMDAL.txt > "$TMP/audit1.out" 2>&1 )
grep -q "struktur valid" "$TMP/audit1.out" && ok "go audit valid" || no "go audit valid"
printf 'garbage\n' > "$TMP/proj/bad.txt"
( cd "$TMP/proj" && "$ROOT/sawit.exe" audit bad.txt >/dev/null 2>&1 ) && no "go audit invalid should fail" || ok "go audit invalid fails"
( cd "$TMP/proj" && "$ROOT/sawit.exe" audit bad.txt --fix >/dev/null 2>&1 ) && [ -f "$TMP/proj/bad.txt.bak" ] && ok "go audit --fix + bak" || no "go audit --fix + bak"
printf 'garbage\n' > "$TMP/proj/bad2.txt"
( cd "$TMP/proj" && python "$ROOT/sawit_transpiler.py" audit bad2.txt >/dev/null 2>&1 ) && no "py audit invalid should fail" || ok "py audit invalid fails"
( cd "$TMP/proj" && python "$ROOT/sawit_transpiler.py" audit bad2.txt --fix >/dev/null 2>&1 ) && [ -f "$TMP/proj/bad2.txt.bak" ] && ok "py audit --fix + bak" || no "py audit --fix + bak"

echo ""
echo "=== 10. CLI error handling ==="
$GO >/dev/null 2>&1; [ $? -eq 2 ] && ok "no args -> exit 2" || no "no args -> exit 2"
$GO bogus >/dev/null 2>&1; [ $? -eq 1 ] && ok "unknown cmd -> exit 1" || no "unknown cmd -> exit 1"
$GO transpile >/dev/null 2>&1; [ $? -eq 1 ] && ok "missing source -> exit 1" || no "missing source -> exit 1"

echo ""
echo "=== 11. Python legacy shorthand + init ==="
$PY "$TMP/tok.sawit" > "$TMP/legacy.c" 2>&1
grep -q 'int main()' "$TMP/legacy.c" && ok "py legacy shorthand" || no "py legacy shorthand"
rm -rf "$TMP/pyproj"
$PY init "$TMP/pyproj" >/dev/null 2>&1 && [ -f "$TMP/pyproj/src/main.sawit" ] && ok "py init" || no "py init"

echo ""
echo "=== 12. Python mandiri + guard --multiboot di Windows ==="
rm -rf "$TMP/standalone"; mkdir -p "$TMP/standalone"
cp sawit_transpiler.py examples/hello.sawit "$TMP/standalone/"
( cd "$TMP/standalone" && python sawit_transpiler.py build hello.sawit -o hello.exe >/dev/null 2>&1 )
[ -x "$TMP/standalone/hello.exe" ] || [ -f "$TMP/standalone/hello.exe" ] && \
  ok "py script mandiri (tanpa header/boot)" || no "py script mandiri"
[ -z "$(find "$TMP/standalone" -maxdepth 1 -name '.sawit-build-*' -print -quit 2>/dev/null)" ] && \
  ok "py build membersihkan tmp" || no "py build meninggalkan tmp"
[ -f examples/wowokos-kernel/tools/sawit ] && [ -f examples/wowokos-kernel/tools/sawit.sha256 ] && \
  ok "bundled transpiler + checksum" || no "bundled transpiler + checksum"
grep -q './tools/sawit' examples/wowokos-kernel/deteksiSawit.sh && \
  ok "detector memprioritaskan binary bawaan" || no "detector tidak memakai binary bawaan"
[ ! -d examples/wowokos-kernel/boot ] && \
  ok "repo kernel tanpa folder boot" || no "repo kernel masih punya folder boot"
grep -a -q 'ENTRY(_start)' examples/wowokos-kernel/tools/sawit && \
  grep -a -q '0xE85250D6' examples/wowokos-kernel/tools/sawit && \
  ok "start.S/linker.ld tertanam di binary" || no "aset boot tidak tertanam di binary"
grep -Fxq '/build/' examples/wowokos-kernel/.gitignore && \
  ok "build/ kernel diabaikan git" || no "build/ kernel belum diabaikan"
grep -q 'tulisLayar(R"(' examples/wowokos-kernel/src/main.sawitpp && \
  grep -q '=== WowokOS Kernel ===' examples/wowokos-kernel/src/main.sawitpp && \
  ok "splash ringkas di source kernel" || no "splash ringkas tidak ada"
if grep -q '100% READY\|Tekan tombol\|WOWOKOS ::' examples/wowokos-kernel/src/main.sawitpp; then \
  no "splash masih punya teks tambahan"; else ok "splash tanpa teks tambahan"; fi
grep -Fq 'run: $(ELF)' examples/wowokos-kernel/SuratIzinTanam && \
  ok "run target memakai -kernel" || no "run target bukan -kernel"
[ -f examples/wowokos-kernel/buatIso.sh ] && \
  grep -q 'cdboot.img' examples/wowokos-kernel/buatIso.sh && \
  grep -q 'grub-mkrescue -o' examples/wowokos-kernel/buatIso.sh && \
  ok "ISO memakai cdboot.img sesuai manual GRUB" || no "ISO masih pakai boot_hybrid.img"
[ -f examples/wowokos-kernel/buatIso.sh ] && \
  grep -q -- '-c "$ISODIR/boot/grub/grub.cfg"' examples/wowokos-kernel/buatIso.sh && \
  ok "grub.cfg ditanam di core image" || no "grub.cfg belum ditanam"
[ -f examples/wowokos-kernel/buatIso.sh ] && \
  grep -q 'multiboot2 /boot/kernel.elf' examples/wowokos-kernel/buatIso.sh && \
  grep -q 'paksa cd0' examples/wowokos-kernel/buatIso.sh && \
  ok "GRUB menu entries MB1/MB2/cd0" || no "GRUB menu entries belum lengkap"
[ -f examples/wowokos-kernel/buatIso.sh ] && \
  grep -q 'biosdisk iso9660 normal search multiboot' examples/wowokos-kernel/buatIso.sh && \
  ok "ISO menyertakan modul GRUB normal+multiboot" || no "modul GRUB belum lengkap"
[ -f examples/wowokos-kernel/buatIso.sh ] && \
  ok "pembuat ISO El Torito disertakan" || no "pembuat ISO tidak ada"
case "$(uname -s 2>/dev/null)" in
  MINGW*|MSYS*|CYGWIN*)
    msg=$(cd "$TMP/standalone" && python sawit_transpiler.py build hello.sawit --multiboot -o kernel.elf 2>&1)
    echo "$msg" | grep -q "WSL/Linux" && ok "py guard multiboot Windows" || no "py guard multiboot Windows: $msg"
    msg=$($GO build examples/wowokos-kernel/src/main.sawitpp --multiboot -o "$TMP/g-k.elf" 2>&1)
    echo "$msg" | grep -q "WSL/Linux" && ok "go guard multiboot Windows" || no "go guard multiboot Windows: $msg"
    ;;
  *)
    echo "  SKIP  guard Windows (berjalan di $(uname -s))"
    ;;
esac

echo ""
echo "================================"
echo "RESULT: PASS=$PASS FAIL=$FAIL"
[ "$FAIL" -eq 0 ] || exit 1
