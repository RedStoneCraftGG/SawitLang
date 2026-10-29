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

## Apakah Akan Ada Update Atau Digunakan Secara Serius Kedepannya?

Gak. Proyek ini sifatnya candaan bahasa pemrograman. Tidak ada hal yang harus diseriusin. bahkan tidak ada orang yanng benar benar mendedikasikan 100% proyeknya menggunakan bahasa Sawit sebagai bahasa utamanya. Kalau ada, saya ga habis pikir lagi 💀

Paling sedikit sentuhan terakhir agar bisa jadi Joke Programing Language yang setidaknya bisa dipakai...

# Implementasi & Cara Pemakaian

```powershell
go build -o sawit.exe .
./sawit.exe transpile examples/hello.sawit
./sawit.exe build examples/hello.sawit -o hello.exe
./sawit.exe run examples/hello.sawit
./sawit.exe init proyek-baru
./sawit.exe audit AMDAL.txt
```

Tambahkan folder hasil build ke `PATH` agar dapat dipanggil langsung dari Command
Prompt sebagai `sawit`.

---

Perintah `build` mendeteksi GCC/Clang secara otomatis. Compiler juga dapat dipilih
secara eksplisit dengan `--compiler gcc` atau `--compiler clang`.

Mode `run` membuat executable sementara, menjalankannya, lalu menghapusnya. Jadi
pengguna cukup memberikan satu file `.sawit` tanpa mengelola hasil build manual.

Versi awal menggunakan translasi berbasis token sehingga keyword di dalam komentar
dan string tidak ikut berubah. Runtime helper berada di `sawit_runtime.h` dan otomatis
disertakan oleh hasil transpiler.

Helper yang tersedia: `buzzer`, `bungkam`, `bukaSuara`, `demoMasyarakat`, `pajak`,
`MBG`, `nyawit`, `cair`, `beli`, `jual`, dan `bersihkanLayar`.

`MBG` mengisi ledger anggaran, `cair` mencairkan saldo yang tersedia, sedangkan
`beli` dan `jual` memakai allocator yang melacak pointer. `jual` mengabaikan
pointer yang tidak dilacak, sehingga double-free dan `jual` pada variabel stack
aman (no-op). `bungkam` dan `bukaSuara` mengontrol output stdout/stderr tanpa
menutup stream.

# Kamus MVP

Ringkasan seluruh kata kunci dan fungsi yang dikenali transpiler. Kolom "Padanan"
adalah hasil translasi ke C/C++. Semua kata kunci bersifat case-sensitive, dan
translasi hanya menyentuh token — teks di dalam komentar dan string tidak diubah.

### Tipe data

| Sawit | Padanan | Keterangan |
|---|---|---|
| `tandan` | `int` | bilangan bulat |
| `pertalite` | `float` | pecahan presisi tunggal |
| `pertamax` | `double` | pecahan presisi ganda |
| `buah` | `char` | karakter |
| `pelepah` | `std::string` | string (butuh C++ / `imporBibit <string>`) |
| `panen` | `bool` | boolean |
| `rodok` | `bool` | alias boolean |
| `hutan` | `void` | tanpa nilai balik |

### Kontrol alur

| Sawit | Padanan | Keterangan |
|---|---|---|
| `kalauPanen` | `if` | |
| `kalauTidak` | `else` | |
| `rapat` | `switch` | |
| `kasus` | `case` | |
| `muter` | `for` | |
| `selama` | `while` | |
| `pidato` | `do` | pembuka `do-while` |
| `janji` | `while` | penutup `do-while` |
| `pulang` | `break` | |
| `lanjutPidato` | `continue` | |
| `lanjutOmonOmon` | `continue` | alias |
| `setor` | `return` | |
| `relokasi` | `goto` | butuh label tujuan |

`do-while` ditulis sebagai pasangan `pidato` … `janji`:

```sawit
pidato {
    // dijalankan minimal sekali
} janji (kondisi);
```

### Kelas, struct, namespace, dan akses

| Sawit | Padanan |
|---|---|
| `perkebunan` | `class` |
| `kopdes` | `struct` |
| `jalanProduksi` | `public` |
| `lahanPribadi` | `private` |
| `lahanPejabat` | `protected` |
| `wilayahKebun` | `namespace` |
| `panenUtama` | `main` |

### Preprocessor, memori, dan boolean

| Sawit | Padanan | Keterangan |
|---|---|---|
| `imporBibit` | `#include` | |
| `RUU` | `#define` | |
| `PerPres` | `#define` | alias |
| `bakarHutan` | `new` | butuh C++ (`.sawitpp`) |
| `gusur` | `delete` | butuh C++ (`.sawitpp`) |
| `sesuai` | `true` | |
| `hoaks` | `false` | |

### Fungsi runtime

| Sawit | Padanan | Keterangan |
|---|---|---|
| `buzzer(...)` | `sawit_buzzer` | cetak ke stdout (mirip `printf`) |
| `bungkam(stream)` | `sawit_bungkam` | bisukan stdout/stderr |
| `bukaSuara(stream)` | `sawit_buka_suara` | nyalakan kembali |
| `demoMasyarakat(...)` | `sawit_demo_masyarakat` | log ke stderr berprefix `[DEMO MASYARAKAT]` |
| `pajak(nilai, persen)` | `sawit_pajak` | makro: `nilai -= nilai * persen / 100` |
| `MBG(jumlah)` | `sawit_mbg` | tambah ke ledger anggaran |
| `cair(jumlah)` | `sawit_cair` | cairkan dari ledger (0 bila melebihi saldo) |
| `beli(byte)` | `sawit_beli` | alokasi heap ter-lacak |
| `jual(ptr)` | `sawit_jual` | bebaskan; no-op bila pointer tak ter-lacak |
| `nyawit(fn)` | `sawit_nyawit` | panggil fungsi lewat pointer: `fn()` |
| `bersihkanLayar()` | `sawit_bersihkan_layar` | bersihkan layar (`cls`/`clear`) |

### Contoh cepat

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

Padanan nama file build system (`AMDAL.txt` dan kawan-kawan) ada di tabel setelah ini.

Setiap proyek baru dimulai dengan `sawit init`. Generator membuat `src/main.sawit`,
runtime lokal, transpiler lokal, dan `AMDAL.txt` sebagai padanan CMakeLists.txt.

Gunakan `sawit audit AMDAL.txt` untuk memeriksa struktur AMDAL. Tambahkan `--fix`
untuk menulis ulang struktur standar dan menyimpan cadangan sebagai `AMDAL.txt.bak`.

| Sawit | Padanan upstream |
|---|---|
| `AMDAL.txt` | `CMakeLists.txt` |
| `SuratIzinTanam` | `Makefile` |
| `ProposalKebun` | `meson.build` |
| `ProyekStrategisNasional` | `BUILD.bazel` |
| `IzinTambang.txt` | `conanfile.txt` |
| `antekModal.json` | `vcpkg.json` |

Contoh aplikasi yang lebih besar tersedia di
`examples/wowokos.sawitpp`, sebuah shell interaktif bergaya MS-DOS dengan boot
sequence, parser perintah, direktori virtual, alokasi memori (beli/jual/new/delete),
ledger anggaran (MBG/cair/pajak), menu switch/case (`rapat`/`kasus`), login
do-while + goto (`pidato`/`janji`/`relokasi`), uptime float/double
(`pertalite`/`pertamax`), class `Kebun` (`perkebunan`/`jalanProduksi`/...),
mode senyap (`bungkam`/`bukaSuara`), dan shutdown. File ini sengaja mencobakan
**seluruh** keyword dan fungsi runtime yang didokumentasikan sebagai showcase bahasa.

Jalankan dengan:

```powershell
./sawit.exe run examples/wowokos.sawitpp
```
