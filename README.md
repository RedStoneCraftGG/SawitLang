# SawitLang (Sawit/S++)

![Sawit](https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white)
![S++](https://img.shields.io/badge/c%2B%2B-%2300599C.svg?style=for-the-badge&logo=cplusplus&logoColor=white)

SawitLang adalah transpiler (source-to-source compiler) yang terinspirasi dari [BrainrotLang](https://github.com/Brainrotlang/brainrot), namun dengan cikal bakal khas Indonesia di tahun 2026 (lebih tepatnya pada saat masa pemerintahan Hokage Ke-8).

SawitLang dibuat sebagai **MURNI** candaan sekaligus satir halus kepada pemerintah +62 yang tidak bisa menangani **masalah** dengan benar dan hanya mengutamakan kepentingan utama program pemerintah yang **belum tentu bermanfaat** (boro-boro 17+8=25, 10+6 aja 17).

## Asal Usul Proyek

Terlintas di kepala aja saat bengong di kantor wkwkwkwk

## Kenapa Harus Sawit?

Sawit (Elaeis guineensis Jacq) adalah tanaman rumput-rumputan dan batangnya tidak bisa berkecambah layaknya pohon pada umumnya (Monokotil) serta memiliki akar serabut. 
Buahnya bisa di ekspor ke Tel-Avi-... Eh, maksudnya ke luar negeri ataupun diolah menjadi minyak goreng, bahan bakar non-fosil (bio-solar), dan tentu bisa dijual sebagai penghasilan utama.

Masalahnya datang ketika hutan lebat ditebangi secara masal, kemudian diganti oleh bibit sawit. Sehingga tanah yang menopangnya menjadi rawan sekali terkena longsor, tidak bisa menyerap air dengan benar, dan bahkan menyebabkan banjir bandang 3 provinsi sekaligus.
Puncaknya ketika hutan di pulau RiverEx yang pada dasarnya adalah tanah Histosol / Organosol di tebang sekaligus di-purge oleh tembakan tank M3A1 Satan. Baik milik perusahaan, negara, maupun pribadi, Hutan-hutan yang ada di pulau RiverEx lenyap dan menghasilkan kabut/asap yang menyakitkan paru-paru, bahkan hingga ke negeri lain.
Akibatnya, kota atau daerah yang berada di tengah pulau, tiba tiba teleport ke planet mars dan orang orang yang gagal beradaptasi seketika pindah ke isekai. Di tahun ini (per 29 Oktober 2026, semenjak repositori ini dibuat) ada setidaknya 306.309 orang terkena kode redeem: ISPA2026. dan diantaranya adalah bocil-bocil sebanyak 152.194 orang.

Dan itulah kenapa saya memilih *Sawit* sebagai subjek utamanya.

> Catatan: gunakan akal dan logika diluar otak, karena butuh pemahaman absurd agar bisa memahami ini. Saya sengaja menulis seperti ini karena KOMDIK sedang gencar-gencarnya melakukan pembatasan terhadap konten yang membahas [[Content Has Been Blocked by Ipo-Chan🌴🔥]]

## Apa Motivasinya Membuat Proyek Ini?

Ngabisin token AI. Sayang bentar lagi Expired.

> Update: Dapat bansos lagi 🤑

## Apakah Akan Ada Update Atau Digunakan Secara Serius Kedepannya?

Sudah ada updatenya. Dan juga ada proyek contoh yang benar-benar menggunakan SawitLang ini seperti [Professional Procrastination Management System](https://github.com/flessan/tugasnyawit), dan [WowokOS-Kernel](https://github.com/RedStoneCraftGG/WowokOS-Kernel).

## Ringkasan

| Aspek | Keterangan |
|---|---|
| Implementasi utama | `main.go` → binary `sawit.exe` (Windows) / `sawit` (Linux) |
| Alternatif | `sawit_transpiler.py`, mandiri, aset runtime/boot tertanam |
| Runtime hosted | `sawit_runtime.h` (butuh libc) |
| Runtime kernel | `sawit_kernel.h` (freestanding, tanpa libc) |
| Contoh utama | `examples/wowokos-kernel/` — repositori kernel mandiri |
| Status | v1.1, sudah dipakai untuk membangun kernel Multiboot yang benar-benar boot |

Aset boot (`boot/start.S`, `boot/linker.ld`) dan kedua runtime ditanam ke dalam
binary Go maupun skrip Python. Artinya binary/skrip bisa dipakai dari folder
mana pun tanpa menyalin file pendukung.

## Build transpiler

```powershell
# Windows
go build -trimpath -ldflags="-s -w" -o sawit.exe .

# Linux / WSL
CGO_ENABLED=0 GOOS=linux GOARCH=amd64 go build -trimpath -ldflags="-s -w" -o sawit .
```

Atau pakai release zip `sawitlang-v1.1.zip` yang berisi binary siap pakai,
port Python, contoh, dan `SHA256SUMS`.

## CLI

```
sawit <perintah> [opsi] <file>
```

| Perintah | Fungsi |
|---|---|
| `transpile` | Cetak hasil translasi ke stdout, atau tulis ke `-o` |
| `build` | Transpile + kompilasi ke executable |
| `run` | Build, jalankan, lalu hapus executable sementara |
| `init` | Buat proyek baru berisi `src/`, runtime, boot, dan `AMDAL.txt` |
| `audit` | Periksa struktur `AMDAL.txt` |

| Opsi | Berlaku untuk | Fungsi |
|---|---|---|
| `-o <path>` | `transpile`, `build` | File keluaran |
| `--compiler <nama>` | `build`, `run` | Paksa GCC/Clang tertentu, mis. `i686-elf-gcc` |
| `--keep-generated` | `build` | Simpan file C/C++ hasil transpile |
| `--freestanding` | `transpile`, `build` | Pakai `sawit_kernel.h`, tanpa libc |
| `--multiboot` | `transpile`, `build` | ELF32 + header Multiboot 1+2 (implisit freestanding) |
| `--force` | `init` | Timpa proyek yang sudah ada |
| `--fix` | `audit` | Tulis ulang `AMDAL.txt` dan simpan cadangan `.bak` |
| `--` | `run` | Pemisah argumen yang diteruskan ke program |

Contoh:

```powershell
./sawit.exe transpile examples/hello.sawit
./sawit.exe build examples/hello.sawit -o hello.exe
./sawit.exe run examples/hello.sawit
./sawit.exe init proyek-baru
./sawit.exe audit AMDAL.txt --fix
```

Port Python memakai perintah dan opsi yang sama:

```powershell
python sawit_transpiler.py transpile examples/hello.sawit
python sawit_transpiler.py build examples/hello.sawit -o hello.exe
```

Translasi berbasis token, sehingga keyword di dalam komentar dan string tidak
ikut berubah. Mode `run` tidak bisa dipakai bersama `--freestanding` atau
`--multiboot`.

## Kamus MVP

Semua kata kunci bersifat case-sensitive. Translasi hanya menyentuh token;
teks di dalam komentar dan string tidak diubah.

### Tipe data

| Sawit | Padanan | Keterangan |
|---|---|---|
| `tandan` | `int` | bilangan bulat |
| `pertalite` | `float` | presisi tunggal |
| `pertamax` | `double` | presisi ganda |
| `buah` | `char` | karakter |
| `pelepah` | `std::string` | butuh C++ / `imporBibit <string>` |
| `panen` / `rodok` | `bool` | boolean (`rodok` alias) |
| `hutan` | `void` | tanpa nilai balik |

### Kontrol alur

| Sawit | Padanan |
|---|---|
| `kalauPanen` / `kalauTidak` | `if` / `else` |
| `rapat` / `kasus` | `switch` / `case` |
| `muter` / `selama` | `for` / `while` |
| `pidato` … `janji (kondisi)` | `do` … `while (kondisi)` |
| `pulang` / `lanjutPidato` | `break` / `continue` |
| `lanjutOmonOmon` | `continue` (alias) |
| `setor` | `return` |
| `relokasi` | `goto` (butuh label) |

### Kelas, struct, dan namespace

| Sawit | Padanan |
|---|---|
| `perkebunan` / `kopdes` | `class` / `struct` |
| `jalanProduksi` / `lahanPribadi` / `lahanPejabat` | `public` / `private` / `protected` |
| `wilayahKebun` | `namespace` |
| `panenUtama` | `main` |
| `bakarHutan` / `gusur` | `new` / `delete` (butuh `.sawitpp`) |

Constructor/destructor sengaja tidak diberi alias (`tanam`/`tebang` sudah
tidak dipakai). Tulis seperti C++ biasa.

### Preprocessor, qualifier, dan konstanta

| Sawit | Padanan |
|---|---|
| `imporBibit` | `#include` |
| `RUU` / `PerPres` | `#define` (`PerPres` alias) |
| `keping8` … `keping64` | `uint8_t` … `uint64_t` |
| `alamat` | `uintptr_t` |
| `takTanda` / `tetap` / `menguap` | `unsigned` / `const` / `volatile` |
| `statis` / `luar` / `segera` | `static` / `extern` / `inline` |
| `kosong` / `seberapa` | `NULL` / `sizeof` |
| `rumpun` / `serikat` / `sebutan` | `enum` / `union` / `typedef` |
| `sesuai` / `hoaks` | `true` / `false` |
| `cakramRAM` / `cakramATA` | `SAWIT_DEV_RAM` / `SAWIT_DEV_ATA` |

### Contoh cepat (hosted)

```sawit
imporBibit <stdio.h>

tandan panenUtama() {
    tandan dana = MBG(1000);
    pajak(dana, 11);
    tandan cairan = cair(dana);
    kalauPanen (cairan > 0) {
        buzzer("Cair: %d\n", cairan);
    } kalauTidak {
        demoMasyarakat("Saldo tidak cukup");
    }
    setor 0;
}
```

## Runtime & helper

### Hosted (libc)

| Sawit | Fungsi C | Keterangan |
|---|---|---|
| `buzzer(...)` | `sawit_buzzer` | Cetak ke stdout (mirip `printf`) |
| `bungkam(stream)` / `bukaSuara(stream)` | `sawit_bungkam` / `sawit_buka_suara` | Matikan/nyalakan output tanpa menutup stream |
| `demoMasyarakat(...)` | `sawit_demo_masyarakat` | Log stderr berprefix `[DEMO MASYARAKAT]` |
| `pajak(nilai, persen)` | `sawit_pajak` | `nilai -= nilai * persen / 100` |
| `MBG(n)` / `cair(n)` | `sawit_mbg` / `sawit_cair` | Ledger anggaran |
| `beli(byte)` / `jual(ptr)` | `sawit_beli` / `sawit_jual` | Allocator ter-lacak; `jual` aman untuk pointer asing/double-free |
| `rampas(&dst, &src)` / `sita(...)` | `sawit_rampas` | Pindahkan kepemilikan blok, `src` jadi `kosong` |
| `nyawit(fn)` | `sawit_nyawit` | Panggil fungsi lewat pointer |
| `bersihkanLayar()` | `sawit_bersihkan_layar` | Bersihkan layar |
| `sadap(...)` / `sadapBaris(buf, n)` | `sawit_sadap` / `sawit_sadap_baris` | Baca stdin / baris (newline dibuang) |
| `sadapKarakter()` / `sadapSisa()` | `sawit_sadap_karakter` / `sawit_sadap_sisa` | Baca karakter / buang sisa input |
| `tutupTelinga()` / `bukaTelinga()` | `sawit_tutup_telinga` / `sawit_buka_telinga` | Kontrol stdin |
| `samaKata` / `panjangKata` / `terjemahAngka` / `salinKata` | `sawit_sama_kata` dll. | Utilitas string |

Di mode kernel, `beli`/`jual` memakai heap free-list 64 KB: `jual()` benar-benar
mengembalikan dan menggabungkan blok, `ukuranBlok(ptr)` melaporkan ukuran payload
yang valid. `rampas()`/`sita()` tetap berfungsi tanpa copy.

### Kernel: mesin & input

| Sawit | Fungsi C | Keterangan |
|---|---|---|
| `semprot(port, val)` / `hisap(port)` | `sawit_outb` / `sawit_inb` | Port I/O x86 |
| `kunciInterupsi()` / `bukaInterupsi()` / `hentiMesin()` | `sawit_cli` / `sawit_sti` / `sawit_hlt` | Kontrol CPU |
| `tulisLayar(s)` / `tulisWarna(attr, s)` | `sawit_tulis_layar` / `sawit_tulis_warna` | VGA text mode `0xB8000` |
| `tulisSerial(s)` | `sawit_serial_write` | COM1, 9600 8N1 |
| `pasangMesin(hz)` | `sawit_pasang_mesin` | GDT + IDT + PIC + PIT + serial + `sti` |
| `tunda(tick)` / `bacaTick()` | `sawit_tunda` / `sawit_tick` | Delay berbasis PIT |
| `gentar(pesan)` | `sawit_panic` | Tulis serial + layar, lalu halt |
| `infoMultiboot()` / `infoRAM()` | `sawit_mb_ok` / `sawit_mb_ram_kb` | Info boot GRUB |
| `infoPetaMemori(&basis, &ukuran)` | `sawit_mb_largest_usable` | Blok RAM usable terbesar |
| `sadapScancode()` | `sawit_sadap_scancode` | Scancode mentah dari buffer IRQ1 |
| `nyalakanUlang()` | `sawit_nyalakan_ulang` | Reset 8042 |

### Kernel: memori & proses

| Sawit | Fungsi C | Keterangan |
|---|---|---|
| `pasangHalaman()` / `halamanAktif()` | `sawit_paging_init` / `sawit_paging_ok` | Paging identitas 4 MB |
| `frameTotal()` / `frameBebas()` | `sawit_pmm_total` / `sawit_pmm_free` | Physical memory manager bitmap |
| `mintaFrame()` / `kembalikanFrame(a)` | `sawit_pmm_alloc_frame` / `sawit_pmm_free_frame` | Alokasi frame 4 KB |
| `prosesBaru(fn, arg)` | `sawit_process_create` | Task kooperatif ring 0, return PID |
| `prosesYield()` / `prosesKeluar()` | `sawit_yield` / `sawit_process_exit` | Jadwal / hentikan task |
| `prosesId()` / `prosesJumlah()` | `sawit_process_current_id` / `sawit_process_count` | Info task |
| `panggilSistem(n, a, b, c)` | `sawit_syscall` | `int 0x80`, lihat tabel syscall |

### Kernel: penyimpanan & layanan

| Sawit | Fungsi C | Keterangan |
|---|---|---|
| `pasangCakram(dev)` | `sawit_ata_init` | Probe ATA PIO LBA28 (polling, `nIEN`) |
| `adaCakram(dev)` / `sektorCakram(dev)` | `sawit_blk_ok` / `sawit_blk_sectors` | Status & kapasitas block device |
| `bacaSektor` / `tulisSektor` | `sawit_blk_read` / `sawit_blk_write` | Baca/tulis 1 sektor 512 byte |
| `formatCakram` / `pasangBerkas` / `terpasangBerkas` | `sawit_fs_format` / `sawit_fs_mount` / `sawit_fs_mounted` | SawitFS |
| `blokTotal()` / `blokBebas()` | `sawit_fs_total_blocks` / `sawit_fs_free_blocks` | Statistik blok |
| `buatBerkas` / `bukaBerkas` / `hapusBerkas` / `ukuranBerkas` | `sawit_fs_*` | Operasi berkas |
| `bacaBerkas(fd, buf, n)` / `tulisBerkas(fd, buf, n)` | `sawit_fs_read` / `sawit_fs_write` | Maks. 512 byte per berkas |
| `daftarBerkas()` | `sawit_fs_list` | Cetak daftar berkas |
| `layananBaca/Tulis/Buka/Daftar` | `sawit_sys_read/write/open/list` | FS lewat syscall |

`cakramRAM` = RAM disk 32 KB; `cakramATA` = ATA primary master. SawitFS:
superblock sektor 0, bitmap sektor 1, 16 inode sektor 2, data mulai sektor 3.
Ini filesystem belajar, bukan FAT/ext.

### Kernel: userland

| Sawit | Fungsi C | Keterangan |
|---|---|---|
| `prosesUserBaru()` | `sawit_user_spawn` | Spawn payload ring 3, return PID atau `-1` |
| `prosesUserJumlah()` | `sawit_user_count` | Jumlah proses ring 3 aktif |
| `prosesRing()` | `sawit_current_ring` | CPL saat ini (`0` kernel, `3` user) |

Nomor syscall `int 0x80`: `0` yield, `1` getpid, `2` write, `3` exit,
`4` fs_read, `5` fs_write, `6` fs_open, `7` fs_list. Gate bernomor 128
ber-DPL 3 sehingga bisa dipanggil dari ring 3.

### Padanan nama file build system

| Sawit | Upstream |
|---|---|
| `AMDAL.txt` | `CMakeLists.txt` |
| `SuratIzinTanam` | `Makefile` |
| `ProposalKebun` | `meson.build` |
| `ProyekStrategisNasional` | `BUILD.bazel` |
| `IzinTambang.txt` | `conanfile.txt` |
| `antekModal.json` | `vcpkg.json` |

`sawit init` menghasilkan proyek berisi `src/main.sawit`, runtime, boot assets,
binary transpiler lokal, `AMDAL.txt`, dan `CMakeLists.txt` shim.

## Mode kernel & WowokOS

Ada dua mode build untuk kernel:

- `--freestanding` — memakai `sawit_kernel.h`, tanpa libc. Hasilnya masih binary
  host (PE/ELF), **belum bisa di-boot**, dan tidak bisa di-`run`.
- `--multiboot` — menambahkan header Multiboot 1+2, `_start`, dan linker script.
  Hasilnya ELF32 yang dimuat sekitar `0x100000`, bisa di-boot langsung oleh QEMU
  lewat `-kernel` atau oleh GRUB lewat ISO.

```bash
# WSL / Linux
python3 sawit_transpiler.py build wowokos-kernel/src/main.sawitpp \
  --multiboot -o build/kernel.elf

qemu-system-i386 -kernel build/kernel.elf -m 128 -no-reboot -no-shutdown
```

Build Multiboot wajib dari WSL/Linux. TDM-GCC di Windows menghasilkan PE/COFF,
bukan ELF32; transpiler akan menolak `--multiboot` dengan pesan yang jelas bila
dipanggil langsung dari Windows tanpa cross-toolchain.

Kernel sudah diuji boot sungguhan di **QEMU** (`-kernel` dan ISO GRUB) maupun
**VMware** (ISO GRUB). Silakan pakai sub-proyek mandiri untuk pengembangan:

```bash
cd wowokos-kernel                 # proyek terpisah
make -f SuratIzinTanam check
make -f SuratIzinTanam            # build/kernel.elf
make -f SuratIzinTanam run        # qemu -kernel (jalur utama)
make -f SuratIzinTanam iso        # build/wowokos.iso
make -f SuratIzinTanam run-iso
```

Lihat [WowokOS-Kernel/README.md](https://github.com/RedStoneCraftGG/WowokOS-Kernel/blob/main/README.md) untuk daftar lengkap perintah shell,
prasyarat QEMU/GRUB, dan panduan troubleshooting panic.

Fitur kernel yang sudah berfungsi: GDT/IDT/PIC/PIT, keyboard IRQ1 dengan
scancode→ASCII, serial COM1, heap free-list, paging identitas + PMM, task
kooperatif ring 0, syscall `int 0x80`, RAM disk + ATA PIO, SawitFS, dan userland
ring 3 dengan validasi pointer.

Belum ada: isolasi alamat per proses (belum ada page directory sendiri),
scheduler preemptive, FAT/ext, ELF loader, DMA, dan jaringan.

## Test

```bash
go test ./...                     # unit test (Go)
bash tests/e2e.sh                 # regression + paritas Go/Python
bash tests/multiboot.sh           # build + smoke test boot, butuh WSL/Linux
```
