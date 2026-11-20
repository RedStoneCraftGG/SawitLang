#ifndef SAWIT_KERNEL_H
#define SAWIT_KERNEL_H

/* Runtime freestanding Sawit/S++ — TANPA libc.
 * Diaktifkan oleh --freestanding. Ditujukan untuk kernel x86 32-bit
 * dengan output VGA text mode dan driver keyboard port 0x60.
 * Helper bertema tetap bernama sawit_* agar hasil transpile identik.
 */

/* Tidak menyertakan <stdarg.h>/<stddef.h> sengaja: pada MinGW keduanya
 * menarik corecrt.h yang mendefinisikan uintptr_t dan bertabrakan dengan
 * typedef freestanding di bawah. Kernel sejati tidak punya libc sama sekali. */
#if defined(__GNUC__) || defined(__clang__)
typedef __builtin_va_list sawit_va_list;
#define sawit_va_start(ap, last) __builtin_va_start(ap, last)
#define sawit_va_arg(ap, type)   __builtin_va_arg(ap, type)
#define sawit_va_end(ap)         __builtin_va_end(ap)
#else
typedef void *sawit_va_list;
#define sawit_va_start(ap, last) ((void)(last), (void)(ap))
#define sawit_va_arg(ap, type)   ((type)0)
#define sawit_va_end(ap)         ((void)(ap))
#endif

#define NULL ((void *)0)

/* Tipe lebar tetap tanpa <stdint.h> (header itu tidak freestanding di semua toolchain). */
#ifdef __UINT8_TYPE__
typedef __UINT8_TYPE__ uint8_t;
#else
typedef unsigned char uint8_t;
#endif
#ifdef __INT8_TYPE__
typedef __INT8_TYPE__ int8_t;
#else
typedef signed char int8_t;
#endif
#ifdef __UINT16_TYPE__
typedef __UINT16_TYPE__ uint16_t;
#else
typedef unsigned short uint16_t;
#endif
#ifdef __INT16_TYPE__
typedef __INT16_TYPE__ int16_t;
#else
typedef signed short int16_t;
#endif
#ifdef __UINT32_TYPE__
typedef __UINT32_TYPE__ uint32_t;
#else
typedef unsigned int uint32_t;
#endif
#ifdef __INT32_TYPE__
typedef __INT32_TYPE__ int32_t;
#else
typedef signed int int32_t;
#endif
#ifdef __UINT64_TYPE__
typedef __UINT64_TYPE__ uint64_t;
#else
typedef unsigned long long uint64_t;
#endif
#ifdef __INT64_TYPE__
typedef __INT64_TYPE__ int64_t;
#else
typedef signed long long int64_t;
#endif
#ifdef __UINTPTR_TYPE__
typedef __UINTPTR_TYPE__ uintptr_t;
#else
typedef unsigned long uintptr_t;
#endif

/* ---------- VGA text mode 0xB8000 ---------- */
#define SAWIT_VGA_BASE 0xB8000UL
#define SAWIT_VGA_COLS 80
#define SAWIT_VGA_ROWS 25
#define SAWIT_VGA_ATTR 0x07

static volatile unsigned short *sawit_vga =
    (volatile unsigned short *)SAWIT_VGA_BASE;
static unsigned int sawit_vga_pos = 0;

static inline void sawit_kernel_putc(unsigned char c) {
    if (c == '\n') {
        sawit_vga_pos = (sawit_vga_pos / SAWIT_VGA_COLS + 1) * SAWIT_VGA_COLS;
        return;
    }
    if (c == '\r') {
        sawit_vga_pos -= sawit_vga_pos % SAWIT_VGA_COLS;
        return;
    }
    if (sawit_vga_pos >= SAWIT_VGA_COLS * SAWIT_VGA_ROWS) {
        unsigned int i;
        for (i = 0; i < (SAWIT_VGA_ROWS - 1) * SAWIT_VGA_COLS; ++i)
            sawit_vga[i] = sawit_vga[i + SAWIT_VGA_COLS];
        sawit_vga_pos = (SAWIT_VGA_ROWS - 1) * SAWIT_VGA_COLS;
        for (i = 0; i < SAWIT_VGA_COLS; ++i)
            sawit_vga[sawit_vga_pos + i] = (SAWIT_VGA_ATTR << 8) | ' ';
    }
    sawit_vga[sawit_vga_pos++] = (unsigned short)((SAWIT_VGA_ATTR << 8) | c);
}

static inline void sawit_tulis_layar(const char *text) {
    if (!text) return;
    while (*text) sawit_kernel_putc((unsigned char)*text++);
}

static inline void sawit_bersihkan_layar(void) {
    unsigned int i;
    for (i = 0; i < SAWIT_VGA_COLS * SAWIT_VGA_ROWS; ++i)
        sawit_vga[i] = (SAWIT_VGA_ATTR << 8) | ' ';
    sawit_vga_pos = 0;
}

/* Pakai 32-bit: pembagian 64-bit butuh __udivdi3/__umoddi3 dari libgcc,
 * yang tidak tersedia pada link -nostdlib. Semua nilai printf kernel
 * (int/unsigned/pointer) muat di 32 bit pada target i386. */
static inline void sawit_kernel_print_uint(unsigned int value,
                                           unsigned int base, int upper) {
    char digits[24];
    int used = 0;
    if (value == 0) {
        sawit_kernel_putc('0');
        return;
    }
    while (value) {
        unsigned int digit = (unsigned int)(value % base);
        digits[used++] = (char)(digit < 10
            ? '0' + digit
            : (upper ? 'A' : 'a') + digit - 10);
        value /= base;
    }
    while (used) sawit_kernel_putc((unsigned char)digits[--used]);
}

static inline void sawit_kernel_buzzer_va(const char *format, sawit_va_list args) {
    const char *p = format;
    if (!p) return;
    while (*p) {
        if (*p != '%') {
            sawit_kernel_putc((unsigned char)*p++);
            continue;
        }
        ++p;
        if (!*p) break;
        switch (*p) {
        case 's': {
            const char *text = sawit_va_arg(args, const char *);
            sawit_tulis_layar(text ? text : "(null)");
            break;
        }
        case 'c':
            sawit_kernel_putc((unsigned char)sawit_va_arg(args, int));
            break;
        case 'd':
        case 'i': {
            int value = sawit_va_arg(args, int);
            if (value < 0) {
                sawit_kernel_putc('-');
                /* Hindari negasi INT_MIN yang overflow. */
                sawit_kernel_print_uint((unsigned int)(-(value + 1)) + 1u, 10, 0);
            } else {
                sawit_kernel_print_uint((unsigned int)value, 10, 0);
            }
            break;
        }
        case 'u':
            sawit_kernel_print_uint(sawit_va_arg(args, unsigned int), 10, 0);
            break;
        case 'x':
            sawit_kernel_print_uint(sawit_va_arg(args, unsigned int), 16, 0);
            break;
        case 'X':
            sawit_kernel_print_uint(sawit_va_arg(args, unsigned int), 16, 1);
            break;
        case 'p':
            sawit_kernel_putc('0');
            sawit_kernel_putc('x');
            sawit_kernel_print_uint((unsigned int)(uintptr_t)sawit_va_arg(args, void *), 16, 0);
            break;
        case '%':
            sawit_kernel_putc('%');
            break;
        default:
            sawit_kernel_putc('%');
            sawit_kernel_putc((unsigned char)*p);
        }
        ++p;
    }
}

/* Mode senyap memakai stream semu: stdout=1, stderr=2. */
typedef int SawitStream;
#define stdout 1
#define stderr 2
static int sawit_stdout_bungkam = 0;
static int sawit_stderr_bungkam = 0;
static int sawit_stdin_bungkam = 0;

static inline void sawit_bungkam(SawitStream stream) {
    if (stream == stdout) sawit_stdout_bungkam = 1;
    if (stream == stderr) sawit_stderr_bungkam = 1;
}
static inline void sawit_buka_suara(SawitStream stream) {
    if (stream == stdout) sawit_stdout_bungkam = 0;
    if (stream == stderr) sawit_stderr_bungkam = 0;
}
static inline void sawit_tutup_telinga(void) { sawit_stdin_bungkam = 1; }
static inline void sawit_buka_telinga(void)  { sawit_stdin_bungkam = 0; }

static inline void sawit_buzzer(const char *format, ...) {
    if (sawit_stdout_bungkam) return;
    sawit_va_list args;
    sawit_va_start(args, format);
    sawit_kernel_buzzer_va(format, args);
    sawit_va_end(args);
}

static inline void sawit_demo_masyarakat(const char *format, ...) {
    if (sawit_stderr_bungkam) return;
    sawit_tulis_layar("[DEMO MASYARAKAT] ");
    sawit_va_list args;
    sawit_va_start(args, format);
    sawit_kernel_buzzer_va(format, args);
    sawit_va_end(args);
    sawit_kernel_putc('\n');
}

/* ---------- String & konversi (tanpa string.h) ---------- */
static inline int sawit_sama_kata(const char *a, const char *b) {
    if (!a || !b) return a == b;
    while (*a && *b && *a == *b) { ++a; ++b; }
    return *a == *b;
}
static inline unsigned long sawit_panjang_kata(const char *text) {
    unsigned long n = 0;
    if (!text) return 0;
    while (text[n]) ++n;
    return n;
}
static inline void sawit_salin_kata(char *dst, const char *src) {
    if (!dst || !src) return;
    while ((*dst++ = *src++)) { }
}
static inline int sawit_terjemah_angka(const char *text) {
    int value = 0, sign = 1;
    if (!text) return 0;
    while (*text == ' ' || *text == '\t') ++text;
    if (*text == '-') { sign = -1; ++text; }
    else if (*text == '+') ++text;
    while (*text >= '0' && *text <= '9') value = value * 10 + (*text++ - '0');
    return value * sign;
}

/* ---------- Input ---------- */
/* Formatted scanf belum tersedia di kernel: kembalikan -1 bila dipanggil. */
static inline int sawit_sadap(const char *format, ...) {
    (void)format;
    return sawit_stdin_bungkam ? -1 : -1;
}
/* Baris kosong sampai driver keyboard buffer-driven ditambahkan. */
static inline char *sawit_sadap_baris(char *buffer, unsigned long size) {
    if (!buffer || size == 0) return (char *)0;
    buffer[0] = '\0';
    return buffer;
}
static inline void sawit_sadap_sisa(void) { }
static inline int sawit_sadap_karakter(void);

/* ---------- Port I/O & kontrol CPU (x86, GCC/Clang) ---------- */
#if defined(__GNUC__) || defined(__clang__)
static inline uint8_t sawit_inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("in %%dx, %%al" : "=a"(value) : "d"(port));
    return value;
}
static inline void sawit_outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("out %%al, %%dx" : : "a"(value), "d"(port));
}
static inline void sawit_cli(void) { __asm__ volatile ("cli"); }
static inline void sawit_sti(void) { __asm__ volatile ("sti"); }
static inline void sawit_hlt(void) { __asm__ volatile ("hlt"); }
#else
static inline uint8_t sawit_inb(uint16_t port) { (void)port; return 0; }
static inline void sawit_outb(uint16_t port, uint8_t value) { (void)port; (void)value; }
static inline void sawit_cli(void) { }
static inline void sawit_sti(void) { }
static inline void sawit_hlt(void) { }
#endif

/* Sadap mentah: baca port 0x60, mengembalikan scancode (bukan ASCII). */
static inline int sawit_sadap_karakter(void) {
    if (sawit_stdin_bungkam) return -1;
    if (sawit_inb(0x64) & 1u) return (int)sawit_inb(0x60);
    return -1;
}

/* ---------- Ledger & alokator mandiri ---------- */
#define sawit_pajak(value, percent) ((value) -= ((value) * (percent)) / 100)
static int sawit_anggaran = 0;
static inline int sawit_mbg(int amount) {
    if (amount > 0) sawit_anggaran += amount;
    return amount;
}
static inline int sawit_cair(int amount) {
    if (amount <= 0 || amount > sawit_anggaran) return 0;
    sawit_anggaran -= amount;
    return amount;
}

#define SAWIT_KERNEL_HEAP_SIZE 65536
static unsigned char sawit_kernel_heap[SAWIT_KERNEL_HEAP_SIZE];
static uintptr_t sawit_kernel_heap_pos = 0;

static inline void *sawit_beli(unsigned long bytes) {
    uintptr_t aligned;
    if (bytes == 0) bytes = 1;
    aligned = (sawit_kernel_heap_pos + 7u) & ~((uintptr_t)7u);
    if (aligned + (uintptr_t)bytes > SAWIT_KERNEL_HEAP_SIZE) return (void *)0;
    sawit_kernel_heap_pos = aligned + (uintptr_t)bytes;
    return (void *)(sawit_kernel_heap + aligned);
}
/* Bump allocator belum bisa mengembalikan blok; jual sengaja no-op. */
static inline void sawit_jual(void *pointer) { (void)pointer; }

/* rampas(): transfer pointer antar variabel, sumber dikosongkan. */
static inline void sawit_rampas(void **dst, void **src) {
    if (!dst || !src || dst == src) return;
    void *pointer = *src;
    *dst = pointer;
    *src = NULL;
}

#define sawit_nyawit(function) ((function)())

/* g++ menyisipkan panggilan __main() untuk constructor global. Tanpa CRT,
 * simbol itu harus disediakan sendiri supaya tautan -nostdlib berhasil. */
#if defined(__cplusplus)
extern "C" void __main(void) { }
#endif

#endif /* SAWIT_KERNEL_H */
