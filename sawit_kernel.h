#ifndef SAWIT_KERNEL_H
#define SAWIT_KERNEL_H

/* Runtime freestanding Sawit/S++ — TANPA libc.
 * Ditujukan untuk kernel x86 32-bit: VGA text mode, port I/O, GDT/IDT,
 * PIC/PIT, serial COM1, keyboard IRQ1, dan heap free-list.
 * Helper bertema tetap bernama sawit_* agar hasil transpile identik.
 */

/* Tidak menyertakan <stdarg.h>/<stddef.h> sengaja: pada MinGW keduanya
 * menarik corecrt.h yang menabrak typedef freestanding di bawah. */
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

/* Tipe lebar tetap tanpa <stdint.h>. */
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

/* Nilai ini diisi start.S dari EAX/EBX saat GRUB memanggil _start.
 * Weak membuat freestanding build tanpa start.S tetap bisa ditaut. */
#ifdef __cplusplus
extern "C" {
#endif
unsigned int sawit_mb_magic __attribute__((weak)) = 0;
unsigned int sawit_mb_info  __attribute__((weak)) = 0;
#ifdef __cplusplus
}
#endif

/* Dipakai oleh cursor VGA sebelum definisi port I/O di bawah. */
static inline uint8_t sawit_inb(uint16_t port);
static inline void sawit_outb(uint16_t port, uint8_t value);

/* ---------- VGA text mode 0xB8000 ---------- */
#define SAWIT_VGA_BASE 0xB8000UL
#define SAWIT_VGA_COLS 80
#define SAWIT_VGA_ROWS 25
#define SAWIT_VGA_ATTR 0x07

static volatile unsigned short *sawit_vga =
    (volatile unsigned short *)SAWIT_VGA_BASE;
static unsigned int sawit_vga_pos = 0;

static inline void sawit_vga_cursor_update(void) {
    unsigned int pos = sawit_vga_pos;
    if (pos >= SAWIT_VGA_COLS * SAWIT_VGA_ROWS)
        pos = SAWIT_VGA_COLS * SAWIT_VGA_ROWS - 1;
    sawit_outb(0x3D4, 0x0E);
    sawit_outb(0x3D5, (uint8_t)((pos >> 8) & 0xFFu));
    sawit_outb(0x3D4, 0x0F);
    sawit_outb(0x3D5, (uint8_t)(pos & 0xFFu));
}

static inline void sawit_vga_scroll(void) {
    unsigned int i;
    for (i = 0; i < (SAWIT_VGA_ROWS - 1) * SAWIT_VGA_COLS; ++i)
        sawit_vga[i] = sawit_vga[i + SAWIT_VGA_COLS];
    sawit_vga_pos = (SAWIT_VGA_ROWS - 1) * SAWIT_VGA_COLS;
    for (i = 0; i < SAWIT_VGA_COLS; ++i)
        sawit_vga[sawit_vga_pos + i] = (SAWIT_VGA_ATTR << 8) | ' ';
    sawit_vga_cursor_update();
}

static inline void sawit_vga_putc_attr(unsigned char attr, unsigned char c) {
    if (c == '\b') {
        if (sawit_vga_pos > 0) {
            --sawit_vga_pos;
            sawit_vga[sawit_vga_pos] = (unsigned short)((SAWIT_VGA_ATTR << 8) | ' ');
        }
        sawit_vga_cursor_update();
        return;
    }
    if (c == '\n') {
        sawit_vga_pos = (sawit_vga_pos / SAWIT_VGA_COLS + 1) * SAWIT_VGA_COLS;
        if (sawit_vga_pos >= SAWIT_VGA_COLS * SAWIT_VGA_ROWS) sawit_vga_scroll();
        sawit_vga_cursor_update();
        return;
    }
    if (c == '\r') {
        sawit_vga_pos -= sawit_vga_pos % SAWIT_VGA_COLS;
        sawit_vga_cursor_update();
        return;
    }
    if (sawit_vga_pos >= SAWIT_VGA_COLS * SAWIT_VGA_ROWS) sawit_vga_scroll();
    sawit_vga[sawit_vga_pos++] = (unsigned short)((attr << 8) | c);
    sawit_vga_cursor_update();
}

static inline void sawit_kernel_putc(unsigned char c) {
    sawit_vga_putc_attr(SAWIT_VGA_ATTR, c);
}

static inline void sawit_tulis_layar(const char *text) {
    if (!text) return;
    while (*text) sawit_kernel_putc((unsigned char)*text++);
}

static inline void sawit_tulis_warna(unsigned char attr, const char *text) {
    while (text && *text) sawit_vga_putc_attr(attr, (unsigned char)*text++);
}

static inline void sawit_bersihkan_layar(void) {
    unsigned int i;
    for (i = 0; i < SAWIT_VGA_COLS * SAWIT_VGA_ROWS; ++i)
        sawit_vga[i] = (SAWIT_VGA_ATTR << 8) | ' ';
    sawit_vga_pos = 0;
    sawit_vga_cursor_update();
}

/* 32-bit saja: pembagian 64-bit butuh __udivdi3 yang tidak ada di -nostdlib. */
static inline void sawit_kernel_print_uint(unsigned int value,
                                           unsigned int base, int upper) {
    char digits[24];
    int used = 0;
    if (value == 0) {
        sawit_kernel_putc('0');
        return;
    }
    while (value) {
        unsigned int digit = value % base;
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
static inline uint16_t sawit_inw(uint16_t port) {
    uint16_t value;
    __asm__ volatile ("in %%dx, %%ax" : "=a"(value) : "d"(port));
    return value;
}
static inline void sawit_outw(uint16_t port, uint16_t value) {
    __asm__ volatile ("out %%ax, %%dx" : : "a"(value), "d"(port));
}
static inline void sawit_cli(void) { __asm__ volatile ("cli"); }
static inline void sawit_sti(void) { __asm__ volatile ("sti"); }
static inline void sawit_hlt(void) { __asm__ volatile ("hlt"); }
#else
static inline uint8_t sawit_inb(uint16_t port) { (void)port; return 0; }
static inline void sawit_outb(uint16_t port, uint8_t value) { (void)port; (void)value; }
static inline uint16_t sawit_inw(uint16_t port) { (void)port; return 0; }
static inline void sawit_outw(uint16_t port, uint16_t value) { (void)port; (void)value; }
static inline void sawit_cli(void) { }
static inline void sawit_sti(void) { }
static inline void sawit_hlt(void) { }
#endif

/* ---------- Serial COM1: log debug untuk QEMU -serial ---------- */
#define SAWIT_COM1 0x3F8u
static inline void sawit_serial_init(void) {
    sawit_outb(SAWIT_COM1 + 1, 0x00);
    sawit_outb(SAWIT_COM1 + 3, 0x80);
    sawit_outb(SAWIT_COM1 + 0, 0x0C);
    sawit_outb(SAWIT_COM1 + 1, 0x00);
    sawit_outb(SAWIT_COM1 + 3, 0x03);
    sawit_outb(SAWIT_COM1 + 2, 0xC7);
    sawit_outb(SAWIT_COM1 + 4, 0x0B);
}
static inline void sawit_serial_putc(unsigned char c) {
    unsigned int guard = 0;
    while (!(sawit_inb(SAWIT_COM1 + 5) & 0x20u) && guard++ < 100000u) { }
    sawit_outb(SAWIT_COM1, c);
}
static inline void sawit_serial_write(const char *text) {
    while (text && *text) sawit_serial_putc((unsigned char)*text++);
}

/* ---------- Panic ---------- */
static inline void sawit_panic(const char *message) {
    sawit_cli();
    sawit_serial_write("[PANIC] ");
    sawit_serial_write(message ? message : "unknown");
    sawit_serial_write("\n");
    sawit_tulis_layar("[PANIC] ");
    sawit_tulis_layar(message ? message : "unknown");
    sawit_tulis_layar("\n");
    for (;;) sawit_hlt();
}

/* ---------- PIC, PIT, GDT, IDT ---------- */
static inline void sawit_pic_remap(void) {
    sawit_outb(0x20, 0x11); sawit_outb(0x21, 0x20);
    sawit_outb(0x21, 0x04); sawit_outb(0x21, 0x01); sawit_outb(0x21, 0xFF);
    sawit_outb(0xA0, 0x11); sawit_outb(0xA1, 0x28);
    sawit_outb(0xA1, 0x02); sawit_outb(0xA1, 0x01); sawit_outb(0xA1, 0xFF);
    sawit_outb(0x21, 0xFC);
}
static inline void sawit_pic_eoi(unsigned int irq) {
    if (irq >= 8u) sawit_outb(0xA0, 0x20);
    sawit_outb(0x20, 0x20);
}
static inline void sawit_pit_init(unsigned int hz) {
    unsigned int divisor = hz ? (1193180u / hz) : 1193180u;
    sawit_outb(0x43, 0x36);
    sawit_outb(0x40, (uint8_t)(divisor & 0xFFu));
    sawit_outb(0x40, (uint8_t)((divisor >> 8) & 0xFFu));
}

#if defined(__GNUC__) || defined(__clang__)
struct sawit_gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct sawit_idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t offset_high;
} __attribute__((packed));

struct sawit_ptr16 {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct sawit_tss {
    uint32_t prev;
    uint32_t esp0, ss0, esp1, ss1, esp2, ss2;
    uint32_t cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs, ldt;
    uint16_t trap, iomap;
} __attribute__((packed));

static struct sawit_gdt_entry sawit_gdt[6];
static struct sawit_idt_entry sawit_idt[256];
static struct sawit_ptr16 sawit_gdtr;
static struct sawit_ptr16 sawit_idtr;
static volatile unsigned int sawit_tick_counter = 0;
static struct sawit_tss sawit_tss;

static void sawit_gdt_set(int index, uint32_t base, uint32_t limit,
                          uint8_t access, uint8_t granularity) {
    sawit_gdt[index].limit_low = (uint16_t)(limit & 0xFFFFu);
    sawit_gdt[index].base_low = (uint16_t)(base & 0xFFFFu);
    sawit_gdt[index].base_mid = (uint8_t)((base >> 16) & 0xFFu);
    sawit_gdt[index].access = access;
    sawit_gdt[index].granularity = (uint8_t)(((limit >> 16) & 0x0Fu) | (granularity & 0xF0u));
    sawit_gdt[index].base_high = (uint8_t)((base >> 24) & 0xFFu);
}

static void sawit_gdt_install(void) {
    uint32_t tss_base = (uint32_t)(uintptr_t)&sawit_tss;
    uint32_t tss_limit = (uint32_t)sizeof(sawit_tss) - 1u;
    sawit_gdt_set(0, 0, 0, 0, 0);
    sawit_gdt_set(1, 0, 0xFFFFFu, 0x9A, 0xCF); /* kernel code */
    sawit_gdt_set(2, 0, 0xFFFFFu, 0x92, 0xCF); /* kernel data */
    sawit_gdt_set(3, 0, 0xFFFFFu, 0xFA, 0xCF); /* user code (siap ring3) */
    sawit_gdt_set(4, 0, 0xFFFFFu, 0xF2, 0xCF); /* user data */
    sawit_gdt_set(5, tss_base, tss_limit, 0x89, 0x40); /* TSS */
    sawit_tss.ss0 = 0x10;
    sawit_tss.esp0 = 0;
    sawit_gdtr.limit = (uint16_t)(sizeof(sawit_gdt) - 1);
    sawit_gdtr.base = (uint32_t)(uintptr_t)&sawit_gdt[0];
#if defined(__ELF__)
    __asm__ volatile ("lgdt %0" : : "m"(sawit_gdtr));
    __asm__ volatile (
        "ljmp $0x08, $1f\n"
        "1:\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "mov $0x28, %%ax\n"
        "ltr %%ax\n"
        : : : "eax");
#else
    /* COFF/PE tidak mendukung ljmp; jalur ini hanya untuk compile-check Windows. */
    sawit_serial_write("[warn] GDT loader dilewati pada target non-ELF\n");
#endif
}

static void sawit_idt_set(unsigned int index, uint32_t offset) {
    sawit_idt[index].offset_low = (uint16_t)(offset & 0xFFFFu);
    sawit_idt[index].selector = 0x08;
    sawit_idt[index].zero = 0;
    sawit_idt[index].type_attr = 0x8E; /* present, DPL=0, interrupt gate */
    sawit_idt[index].offset_high = (uint16_t)(offset >> 16);
}

/* Gate syscall bisa dipanggil dari ring 3 (DPL=3). */
static void sawit_idt_set_user(unsigned int index, uint32_t offset) {
    sawit_idt_set(index, offset);
    sawit_idt[index].type_attr = 0xEE; /* present, DPL=3, interrupt gate */
}

static void __attribute__((interrupt)) sawit_isr_exception(void *frame) {
    (void)frame;
    sawit_panic("exception CPU");
}
static void __attribute__((interrupt)) sawit_isr_pagefault(void *frame) {
    (void)frame;
    sawit_panic("page fault");
}
static void __attribute__((interrupt)) sawit_isr_gpf(void *frame) {
    (void)frame;
    sawit_panic("general protection fault");
}
static void __attribute__((interrupt)) sawit_isr_default(void *frame) {
    (void)frame;
    sawit_panic("interupsi tak tertangani");
}

static void __attribute__((interrupt)) sawit_isr_timer(void *frame) {
    (void)frame;
    ++sawit_tick_counter;
    sawit_pic_eoi(0);
}

static void sawit_keyboard_push(unsigned char sc);
static void sawit_process_init(void);
static void sawit_yield(void);
static unsigned int sawit_process_current_id(void);
static void sawit_process_exit(void);

/* Storage forward declarations (dipakai syscall & demo). */
static int sawit_fs_read(int fd, unsigned char *buf, unsigned int len);
static int sawit_fs_write(int fd, const unsigned char *buf, unsigned int len);
static int sawit_fs_open(const char *name);
static void sawit_fs_list(void);

/* Userland forward declarations (dipakai dispatcher syscall). */
static int sawit_user_ptr_ok(uint32_t addr, uint32_t len);

struct sawit_syscall_frame {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

/* Dispatcher dipanggil dari stub asm dengan frame register lengkap.
   extern "C" mencegah name-mangling saat header dipakai dari .sawitpp (C++). */
#ifdef __cplusplus
extern "C" {
#endif
static void __attribute__((used)) sawit_syscall_dispatch(struct sawit_syscall_frame *f) {
    if (!f) return;
    switch (f->eax) {
    case 0: /* yield */
        sawit_yield();
        f->eax = 0;
        break;
    case 1: /* getpid */
        f->eax = sawit_process_current_id();
        break;
    case 2: /* write(fd, buf, len) — buffer harus milik user */
        if (f->ecx && f->edx && f->edx <= 4096u &&
            sawit_user_ptr_ok(f->ecx, f->edx)) {
            const char *text = (const char *)(uintptr_t)f->ecx;
            unsigned int i;
            for (i = 0; i < f->edx && text[i]; ++i)
                sawit_kernel_putc((unsigned char)text[i]);
            f->eax = i;
        } else f->eax = 0;
        break;
    case 3: /* exit */
        sawit_process_exit();
        break;
    case 4: /* fs_read(fd, buf, len) */
        f->eax = sawit_fs_read((int)f->ebx,
            (unsigned char *)(uintptr_t)f->ecx, f->edx);
        break;
    case 5: /* fs_write(fd, buf, len) */
        f->eax = sawit_fs_write((int)f->ebx,
            (const unsigned char *)(uintptr_t)f->ecx, f->edx);
        break;
    case 6: /* fs_open(name) */
        f->eax = sawit_fs_open((const char *)(uintptr_t)f->ebx);
        break;
    case 7: /* fs_list */
        sawit_fs_list();
        f->eax = 0;
        break;
    default:
        f->eax = (uint32_t)-1;
        break;
    }
}
#ifdef __cplusplus
} /* extern "C" */
#endif

#if defined(__ELF__) && defined(__i386__)
/* Stub ring-0 untuk int 0x80.
 *
 * Atribut `interrupt` GCC TIDAK menaruh frame register lengkap: argumen
 * `void *raw` hanya menunjuk ke EIP yang didorong CPU. Akibatnya pembacaan
 * f->eax (offset 44) mengambil sampah dari stack pemanggil dan dispatch
 * bisa masuk ke case 0/3 (yield/exit) secara acak -> context switch merusak
 * frame interrupt -> "exception CPU".
 *
 * Stub ini menaruh sendiri int_no/err_code + pusha + segment, sehingga
 * tata letaknya persis struct sawit_syscall_frame:
 *   gs fs es ds | edi esi ebp esp ebx edx ecx eax | int_no err_code | eip cs eflags
 * Nilai balik ditulis lewat f->eax lalu popa mengembalikannya ke EAX.
 * Gate 0x8E adalah interrupt gate (DPL=0), jadi IF otomatis mati saat masuk
 * dan dikembalikan oleh iret. */
__attribute__((naked)) static void sawit_isr_syscall(void) {
    __asm__ volatile (
        "pushl $0\n\t"              /* err_code */
        "pushl $128\n\t"            /* int_no */
        "pusha\n\t"                 /* eax ecx edx ebx esp ebp esi edi */
        "pushl %%ds\n\t"
        "pushl %%es\n\t"
        "pushl %%fs\n\t"
        "pushl %%gs\n\t"
        "movl $0x10, %%eax\n\t"     /* selector data kernel */
        "movl %%eax, %%ds\n\t"
        "movl %%eax, %%es\n\t"
        "movl %%esp, %%eax\n\t"
        "pushl %%eax\n\t"           /* argumen: pointer ke frame */
        "call sawit_syscall_dispatch\n\t"
        "addl $4, %%esp\n\t"
        "popl %%gs\n\t"
        "popl %%fs\n\t"
        "popl %%es\n\t"
        "popl %%ds\n\t"
        "popa\n\t"
        "addl $8, %%esp\n\t"        /* buang int_no + err_code */
        "iret\n\t"
        : : : "memory");
}
#else
/* Compile-check Windows (COFF): tidak pernah dijalankan. */
static void __attribute__((interrupt)) sawit_isr_syscall(void *raw) {
    sawit_syscall_dispatch((struct sawit_syscall_frame *)raw);
}
#endif

static void __attribute__((interrupt)) sawit_isr_keyboard(void *frame) {
    (void)frame;
    if (sawit_inb(0x64) & 1u) sawit_keyboard_push(sawit_inb(0x60));
    sawit_pic_eoi(1);
}

static void sawit_idt_install(void) {
    unsigned int i;
    for (i = 0; i < 256u; ++i)
        sawit_idt_set(i, (uint32_t)(uintptr_t)&sawit_isr_default);
    for (i = 0; i < 32u; ++i)
        sawit_idt_set(i, (uint32_t)(uintptr_t)&sawit_isr_exception);
    sawit_idt_set(13, (uint32_t)(uintptr_t)&sawit_isr_gpf);   /* GPF */
    sawit_idt_set(14, (uint32_t)(uintptr_t)&sawit_isr_pagefault);
    sawit_idt_set(32, (uint32_t)(uintptr_t)&sawit_isr_timer);
    sawit_idt_set(33, (uint32_t)(uintptr_t)&sawit_isr_keyboard);
    sawit_idt_set_user(128, (uint32_t)(uintptr_t)&sawit_isr_syscall);
    sawit_idtr.limit = (uint16_t)(sizeof(sawit_idt) - 1);
    sawit_idtr.base = (uint32_t)(uintptr_t)&sawit_idt[0];
#if defined(__ELF__)
    __asm__ volatile ("lidt %0" : : "m"(sawit_idtr));
#endif
}

static inline unsigned int sawit_tick(void) { return sawit_tick_counter; }
static inline void sawit_tunda(unsigned int ticks) {
    unsigned int target = sawit_tick_counter + ticks;
    while (sawit_tick_counter < target) sawit_hlt();
}
static inline void sawit_pasang_mesin(unsigned int hz) {
    sawit_gdt_install();
    sawit_idt_install();
    sawit_serial_init();
    sawit_pit_init(hz);
    sawit_pic_remap();
    sawit_process_init();
    sawit_sti();
}
#else
static inline unsigned int sawit_tick(void) { return 0; }
static inline void sawit_tunda(unsigned int ticks) { (void)ticks; }
static inline void sawit_pasang_mesin(unsigned int hz) { (void)hz; sawit_serial_init(); }
#endif

/* ---------- Keyboard: scancode set 1 -> ASCII + ring buffer ---------- */
#define SAWIT_KB_BUFFER 256u
static char sawit_kb_buffer[SAWIT_KB_BUFFER];
static unsigned char sawit_sc_buffer[64u];
static unsigned int sawit_kb_head = 0;
static unsigned int sawit_kb_tail = 0;
static unsigned int sawit_sc_head = 0;
static unsigned int sawit_sc_tail = 0;
static unsigned char sawit_kb_shift = 0;
static unsigned char sawit_kb_caps = 0;

static char sawit_sc_ascii[128];
static char sawit_sc_shift[128];
static int sawit_sc_tables_ready = 0;

static void sawit_sc_tables_init(void) {
    if (sawit_sc_tables_ready) return;
    sawit_sc_ascii[0x02]='1'; sawit_sc_ascii[0x03]='2'; sawit_sc_ascii[0x04]='3';
    sawit_sc_ascii[0x05]='4'; sawit_sc_ascii[0x06]='5'; sawit_sc_ascii[0x07]='6';
    sawit_sc_ascii[0x08]='7'; sawit_sc_ascii[0x09]='8'; sawit_sc_ascii[0x0A]='9';
    sawit_sc_ascii[0x0B]='0'; sawit_sc_ascii[0x0C]='-'; sawit_sc_ascii[0x0D]='=';
    sawit_sc_ascii[0x0E]='\b'; sawit_sc_ascii[0x0F]='\t';
    sawit_sc_ascii[0x10]='q'; sawit_sc_ascii[0x11]='w'; sawit_sc_ascii[0x12]='e';
    sawit_sc_ascii[0x13]='r'; sawit_sc_ascii[0x14]='t'; sawit_sc_ascii[0x15]='y';
    sawit_sc_ascii[0x16]='u'; sawit_sc_ascii[0x17]='i'; sawit_sc_ascii[0x18]='o';
    sawit_sc_ascii[0x19]='p'; sawit_sc_ascii[0x1A]='['; sawit_sc_ascii[0x1B]=']';
    sawit_sc_ascii[0x1C]='\n'; sawit_sc_ascii[0x1E]='a'; sawit_sc_ascii[0x1F]='s';
    sawit_sc_ascii[0x20]='d'; sawit_sc_ascii[0x21]='f'; sawit_sc_ascii[0x22]='g';
    sawit_sc_ascii[0x23]='h'; sawit_sc_ascii[0x24]='j'; sawit_sc_ascii[0x25]='k';
    sawit_sc_ascii[0x26]='l'; sawit_sc_ascii[0x27]=';'; sawit_sc_ascii[0x28]='\'';
    sawit_sc_ascii[0x29]='`'; sawit_sc_ascii[0x2B]='\\'; sawit_sc_ascii[0x2C]='z';
    sawit_sc_ascii[0x2D]='x'; sawit_sc_ascii[0x2E]='c'; sawit_sc_ascii[0x2F]='v';
    sawit_sc_ascii[0x30]='b'; sawit_sc_ascii[0x31]='n'; sawit_sc_ascii[0x32]='m';
    sawit_sc_ascii[0x33]=','; sawit_sc_ascii[0x34]='.'; sawit_sc_ascii[0x35]='/';
    sawit_sc_ascii[0x37]='*'; sawit_sc_ascii[0x39]=' ';
    sawit_sc_shift[0x02]='!'; sawit_sc_shift[0x03]='@'; sawit_sc_shift[0x04]='#';
    sawit_sc_shift[0x05]='$'; sawit_sc_shift[0x06]='%'; sawit_sc_shift[0x07]='^';
    sawit_sc_shift[0x08]='&'; sawit_sc_shift[0x09]='*'; sawit_sc_shift[0x0A]='(';
    sawit_sc_shift[0x0B]=')'; sawit_sc_shift[0x0C]='_'; sawit_sc_shift[0x0D]='+';
    sawit_sc_shift[0x1A]='{'; sawit_sc_shift[0x1B]='}'; sawit_sc_shift[0x27]=':';
    sawit_sc_shift[0x28]='"'; sawit_sc_shift[0x29]='~'; sawit_sc_shift[0x2B]='|';
    sawit_sc_shift[0x33]='<'; sawit_sc_shift[0x34]='>'; sawit_sc_shift[0x35]='?';
    sawit_sc_tables_ready = 1;
}

static inline int sawit_scancode_to_char(unsigned char sc) {
    unsigned char ch;
    sawit_sc_tables_init();
    if (sc >= 128u) return -1;
    ch = (unsigned char)sawit_sc_ascii[sc];
    if (!ch) return -1;
    if (ch >= 'a' && ch <= 'z' && (sawit_kb_shift ^ sawit_kb_caps))
        return (int)(ch - 32);
    if (sawit_kb_shift && sawit_sc_shift[sc]) return (int)sawit_sc_shift[sc];
    return (int)ch;
}

static void sawit_keyboard_push(unsigned char sc) {
    unsigned int next;
    int ch;
    next = (sawit_sc_head + 1u) % sizeof(sawit_sc_buffer);
    if (next != sawit_sc_tail) {
        sawit_sc_buffer[sawit_sc_head] = sc;
        sawit_sc_head = next;
    }
    if (sc == 0x2Au || sc == 0x36u) { sawit_kb_shift = 1; return; }
    if (sc == 0xAAu || sc == 0xB6u) { sawit_kb_shift = 0; return; }
    if (sc == 0x3Au) { sawit_kb_caps = (unsigned char)!sawit_kb_caps; return; }
    if (sc & 0x80u) return;
    ch = sawit_scancode_to_char(sc);
    if (ch < 0) return;
    next = (sawit_kb_head + 1u) % SAWIT_KB_BUFFER;
    if (next == sawit_kb_tail) return;
    sawit_kb_buffer[sawit_kb_head] = (char)ch;
    sawit_kb_head = next;
}

static inline int sawit_sadap_scancode(void) {
    if (sawit_stdin_bungkam) return -1;
    if (sawit_sc_tail != sawit_sc_head) {
        unsigned char sc = sawit_sc_buffer[sawit_sc_tail];
        sawit_sc_tail = (sawit_sc_tail + 1u) % sizeof(sawit_sc_buffer);
        return (int)sc;
    }
    if (sawit_inb(0x64) & 1u) return (int)sawit_inb(0x60);
    return -1;
}

static inline int sawit_sadap_karakter(void) {
    if (sawit_stdin_bungkam) return -1;
    if (sawit_kb_tail != sawit_kb_head) {
        int ch = (int)sawit_kb_buffer[sawit_kb_tail];
        sawit_kb_tail = (sawit_kb_tail + 1u) % SAWIT_KB_BUFFER;
        return ch;
    }
    if (sawit_inb(0x64) & 1u) return sawit_scancode_to_char(sawit_inb(0x60));
    return -1;
}

static inline char *sawit_sadap_baris(char *buffer, unsigned long size) {
    unsigned long i = 0;
    if (!buffer || size == 0) return (char *)0;
    for (;;) {
        int ch;
        while ((ch = sawit_sadap_karakter()) < 0) sawit_hlt();
        if (ch == '\n' || ch == '\r') break;
        if (ch == '\b') {
            if (i > 0) {
                --i;
                sawit_kernel_putc('\b');
            }
            continue;
        }
        /* Buffer penuh: tetap konsumsi sampai Enter supaya sisa ketikan tidak
           bocor ke prompt berikutnya, tapi jangan ditulis ke layar. */
        if (ch >= 32 && i < size - 1u) {
            buffer[i++] = (char)ch;
            sawit_kernel_putc((unsigned char)ch);
        }
    }
    buffer[i] = '\0';
    sawit_kernel_putc('\n');
    return buffer;
}

static inline void sawit_sadap_sisa(void) {
    sawit_kb_tail = sawit_kb_head;
    sawit_sc_tail = sawit_sc_head;
}

static inline int sawit_sadap(const char *format, ...) {
    char line[256];
    const char *f;
    const char *p;
    sawit_va_list args;
    int count = 0;
    if (!format || sawit_stdin_bungkam) return -1;
    if (!sawit_sadap_baris(line, sizeof(line))) return -1;
    sawit_va_start(args, format);
    f = format;
    p = line;
    while (*f) {
        if (*f != '%') { ++f; continue; }
        ++f;
        if (!*f) break;
        switch (*f++) {
        case 's': {
            char *out = sawit_va_arg(args, char *);
            while (*p == ' ' || *p == '\t') ++p;
            if (out) {
                while (*p && *p != ' ' && *p != '\t') *out++ = *p++;
                *out = '\0';
            } else {
                while (*p && *p != ' ' && *p != '\t') ++p;
            }
            ++count;
            break;
        }
        case 'd':
        case 'i': {
            int *out = sawit_va_arg(args, int *);
            while (*p == ' ' || *p == '\t') ++p;
            if (out) *out = sawit_terjemah_angka(p);
            while (*p && *p != ' ' && *p != '\t') ++p;
            ++count;
            break;
        }
        case 'c': {
            char *out = sawit_va_arg(args, char *);
            if (out) *out = *p ? *p++ : '\0';
            ++count;
            break;
        }
        case '%':
            break;
        default:
            break;
        }
    }
    sawit_va_end(args);
    return count;
}

/* ---------- Multiboot info (dari EAX/EBX lewat start.S) ---------- */
#define SAWIT_MB_MAGIC      0x2BADB002u
#define SAWIT_MB_FLAG_MEM   0x1u
#define SAWIT_MB_FLAG_MMAP  0x40u

static inline int sawit_mb_ok(void) {
    return sawit_mb_magic == SAWIT_MB_MAGIC && sawit_mb_info != 0u;
}
static inline unsigned int sawit_mb_flags(void) {
    return sawit_mb_ok() ? *(const unsigned int *)(uintptr_t)sawit_mb_info : 0u;
}
static inline unsigned int sawit_mb_ram_kb(void) {
    if (!sawit_mb_ok() || !(sawit_mb_flags() & SAWIT_MB_FLAG_MEM)) return 0u;
    return *(const unsigned int *)(uintptr_t)(sawit_mb_info + 8u);
}
static inline unsigned int sawit_mb_largest_usable(unsigned int *base,
                                                   unsigned int *size) {
    unsigned int addr, end, best = 0u, best_base = 0u, count = 0u;
    if (base) *base = 0u;
    if (size) *size = 0u;
    if (!sawit_mb_ok() || !(sawit_mb_flags() & SAWIT_MB_FLAG_MMAP)) return 0u;
    addr = *(const unsigned int *)(uintptr_t)(sawit_mb_info + 48u);
    end = addr + *(const unsigned int *)(uintptr_t)(sawit_mb_info + 44u);
    while (addr + 20u <= end) {
        const unsigned char *e = (const unsigned char *)(uintptr_t)addr;
        unsigned int esize = *(const unsigned int *)(const void *)e;
        unsigned int lo = *(const unsigned int *)(const void *)(e + 4);
        unsigned int hi = *(const unsigned int *)(const void *)(e + 8);
        unsigned int len = *(const unsigned int *)(const void *)(e + 12);
        unsigned int type = *(const unsigned int *)(const void *)(e + 20);
        if (esize < 20u) break;
        if (type == 1u && hi == 0u && len > 0u) {
            ++count;
            if (len > best) { best = len; best_base = lo; }
        }
        addr += esize;
    }
    if (base) *base = best_base;
    if (size) *size = best;
    return count;
}

/* ---------- Physical memory manager + paging 4 KB ---------- */
#define SAWIT_PAGE_SIZE    4096u
#define SAWIT_MAX_FRAMES   32768u  /* 128 MB */

static unsigned char sawit_frame_map[SAWIT_MAX_FRAMES / 8u];
static unsigned int sawit_frame_total = 0;
static unsigned int sawit_frame_used = 0;
static int sawit_pmm_ready = 0;

static void sawit_pmm_init(void) {
    unsigned int i, f, total;
    unsigned int addr, end;
    if (sawit_pmm_ready) return;
    total = 256u;
    if (sawit_mb_ok() && (sawit_mb_flags() & SAWIT_MB_FLAG_MEM))
        total += sawit_mb_ram_kb() / 4u;
    if (total > SAWIT_MAX_FRAMES) total = SAWIT_MAX_FRAMES;
    sawit_frame_total = total;
    /* Start conservatively: everything used; only free confirmed-usable RAM. */
    for (i = 0; i < sizeof(sawit_frame_map); ++i) sawit_frame_map[i] = 0xFFu;
    sawit_frame_used = total;
    /* 4 MB pertama dipakai kernel, stack, heap, page table, dan bitmap. */
    for (f = 0; f < 1024u && f < total; ++f) {
        sawit_frame_map[f >> 3] |= (unsigned char)(1u << (f & 7u));
        ++sawit_frame_used;
    }
    /* Fallback dari mem_upper: anggap RAM di atas 4 MB bisa dipakai. Ini
       memberi hasil wajar walau memory map GRUB hanya berisi region bawah 1 MB. */
    for (f = 1024u; f < total; ++f) {
        sawit_frame_map[f >> 3] &= (unsigned char)~(1u << (f & 7u));
        --sawit_frame_used;
    }
    /* Kalau memory map tersedia, pakai untuk menyempurnakan: tandai region
       reserved (type != 1) di atas 4 MB sebagai terpakai, dan pastikan region
       usable dibebaskan. */
    if (sawit_mb_ok() && (sawit_mb_flags() & SAWIT_MB_FLAG_MMAP)) {
        addr = *(const unsigned int *)(uintptr_t)(sawit_mb_info + 48u);
        end = addr + *(const unsigned int *)(uintptr_t)(sawit_mb_info + 44u);
        while (addr + 20u <= end) {
            const unsigned char *e = (const unsigned char *)(uintptr_t)addr;
            unsigned int esize = *(const unsigned int *)(const void *)e;
            unsigned int lo = *(const unsigned int *)(const void *)(e + 4);
            unsigned int hi = *(const unsigned int *)(const void *)(e + 8);
            unsigned int len = *(const unsigned int *)(const void *)(e + 12);
            unsigned int type = *(const unsigned int *)(const void *)(e + 20);
            unsigned int start, finish;
            unsigned char mask;
            if (esize < 20u) break;
            if (hi != 0u || len < SAWIT_PAGE_SIZE) { addr += esize; continue; }
            start = (lo + SAWIT_PAGE_SIZE - 1u) / SAWIT_PAGE_SIZE;
            finish = (lo + len) / SAWIT_PAGE_SIZE;
            if (start < 1024u) start = 1024u;
            if (finish > total) finish = total;
            for (f = start; f < finish; ++f) {
                mask = (unsigned char)(1u << (f & 7u));
                if (type == 1u) {
                    if (sawit_frame_map[f >> 3] & mask) {
                        sawit_frame_map[f >> 3] &= (unsigned char)~mask;
                        --sawit_frame_used;
                    }
                } else {
                    if (!(sawit_frame_map[f >> 3] & mask)) {
                        sawit_frame_map[f >> 3] |= mask;
                        ++sawit_frame_used;
                    }
                }
            }
            addr += esize;
        }
    }
    sawit_pmm_ready = 1;
}

static unsigned int sawit_pmm_alloc_frame(void) {
    unsigned int byte, bit;
    sawit_pmm_init();
    for (byte = 0; byte < sizeof(sawit_frame_map); ++byte) {
        if (sawit_frame_map[byte] == 0xFFu) continue;
        for (bit = 0; bit < 8u; ++bit) {
            unsigned int f = byte * 8u + bit;
            unsigned char mask = (unsigned char)(1u << bit);
            if (f < sawit_frame_total && !(sawit_frame_map[byte] & mask)) {
                sawit_frame_map[byte] |= mask;
                ++sawit_frame_used;
                return f * SAWIT_PAGE_SIZE;
            }
        }
    }
    return 0;
}

static void sawit_pmm_free_frame(unsigned int physical) {
    unsigned int f = physical / SAWIT_PAGE_SIZE;
    unsigned char mask;
    if (!physical || f >= sawit_frame_total) return;
    mask = (unsigned char)(1u << (f & 7u));
    if (sawit_frame_map[f >> 3] & mask) {
        sawit_frame_map[f >> 3] &= (unsigned char)~mask;
        if (sawit_frame_used) --sawit_frame_used;
    }
}

static inline unsigned int sawit_pmm_total(void) { sawit_pmm_init(); return sawit_frame_total; }
static inline unsigned int sawit_pmm_free(void) {
    sawit_pmm_init();
    return sawit_frame_total - sawit_frame_used;
}

static uint32_t sawit_page_dir[1024] __attribute__((aligned(4096)));
static uint32_t sawit_page_table_low[1024] __attribute__((aligned(4096)));
static int sawit_paging_ready = 0;

static void sawit_paging_init(void) {
    unsigned int i;
    if (sawit_paging_ready) return;
    sawit_pmm_init();
    for (i = 0; i < 1024u; ++i)
        sawit_page_table_low[i] = (i * SAWIT_PAGE_SIZE) | 3u;
    for (i = 0; i < 1024u; ++i) sawit_page_dir[i] = 0;
    /* PDE juga butuh bit U/S: tanpa ini, akses CPL 3 selalu page fault
       walau PTE-nya sudah bertanda user. PTE tetap 3 (supervisor) kecuali
       yang dibuka sawit_page_allow_user(). */
    sawit_page_dir[0] = (uint32_t)(uintptr_t)&sawit_page_table_low | 7u;
#if defined(__ELF__)
    __asm__ volatile ("mov %0, %%cr3" : : "r"(&sawit_page_dir[0]) : "memory");
    __asm__ volatile (
        "mov %%cr0, %%eax\n"
        "or $0x80000000, %%eax\n"
        "mov %%eax, %%cr0\n"
        : : : "eax", "memory");
#endif
    sawit_paging_ready = 1;
    sawit_serial_write("[paging] identity 4MB aktif\n");
}

static inline int sawit_paging_ok(void) { return sawit_paging_ready; }

/* Tandai satu halaman image kernel sebagai bisa diakses ring 3 (bit U/S).
   Payload user sengaja ditaruh di image kernel supaya tidak butuh linker
   script khusus; hanya halaman yang dipilih yang dibuka untuk user. */
static void sawit_page_allow_user(uint32_t addr) {
    unsigned int pti;
    if (!sawit_paging_ready || addr >= 0x400000u) return;
    pti = (unsigned int)((addr >> 12) & 0x3FFu);
    sawit_page_table_low[pti] |= 4u; /* U/S = 1 */
#if defined(__ELF__) && defined(__i386__)
    __asm__ volatile ("invlpg (%0)" : : "r"((void *)(uintptr_t)addr) : "memory");
#endif
}

/* Syscall menolak buffer yang tidak berada di halaman bertanda user. */
static int sawit_user_ptr_ok(uint32_t addr, uint32_t len) {
    uint32_t cur, end;
    if (!sawit_paging_ready || !addr || !len) return 0;
    end = addr + len - 1u;
    if (end < addr || end >= 0x400000u) return 0;
    cur = addr;
    for (;;) {
        unsigned int pti = (unsigned int)((cur >> 12) & 0x3FFu);
        uint32_t pte = sawit_page_table_low[pti];
        if (!(pte & 1u) || !(pte & 4u)) return 0;
        if (cur >= end) break;
        cur = (cur & ~0xFFFu) + 0x1000u;
    }
    return 1;
}

/* ---------- Kontrol mesin tambahan ---------- */
static inline void sawit_nyalakan_ulang(void) {
    sawit_cli();
    sawit_outb(0x64, 0xFE);
    for (;;) sawit_hlt();
}

/* ---------- Ledger & heap free-list mandiri ---------- */
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

#define SAWIT_KERNEL_HEAP_SIZE 65536u
#define SAWIT_HEAP_MAGIC 0x5A17B10Cu

typedef struct sawit_block_header {
    unsigned int magic;
    unsigned int size;
    unsigned int is_free;
    struct sawit_block_header *next;
} sawit_block_header;

static unsigned char sawit_kernel_heap[SAWIT_KERNEL_HEAP_SIZE];
static sawit_block_header *sawit_heap_free = 0;
static int sawit_heap_ready = 0;

static inline unsigned int sawit_align8(unsigned int value) {
    return (value + 7u) & ~7u;
}

static void sawit_heap_init(void) {
    if (sawit_heap_ready) return;
    sawit_heap_free = (sawit_block_header *)(void *)sawit_kernel_heap;
    sawit_heap_free->magic = SAWIT_HEAP_MAGIC;
    sawit_heap_free->size = SAWIT_KERNEL_HEAP_SIZE - (unsigned int)sizeof(sawit_block_header);
    sawit_heap_free->is_free = 1u;
    sawit_heap_free->next = 0;
    sawit_heap_ready = 1;
}

static void *sawit_beli(unsigned long bytes) {
    unsigned int want;
    sawit_block_header **link;
    sawit_heap_init();
    want = sawit_align8(bytes ? (unsigned int)bytes : 1u);
    link = &sawit_heap_free;
    while (*link) {
        sawit_block_header *b = *link;
        unsigned char *raw;
        if (b->magic != SAWIT_HEAP_MAGIC) sawit_panic("heap corrupt");
        if (b->is_free && b->size >= want) {
            if (b->size >= want + (unsigned int)sizeof(sawit_block_header) + 8u) {
                sawit_block_header *rest = (sawit_block_header *)(void *)
                    ((unsigned char *)b + sizeof(sawit_block_header) + want);
                rest->magic = SAWIT_HEAP_MAGIC;
                rest->size = b->size - want - (unsigned int)sizeof(sawit_block_header);
                rest->is_free = 1u;
                rest->next = b->next;
                b->size = want;
                b->next = rest;
            }
            *link = b->next;
            b->is_free = 0u;
            b->next = 0;
            raw = (unsigned char *)b + sizeof(sawit_block_header);
            return (void *)raw;
        }
        link = &b->next;
    }
    return (void *)0;
}

static void sawit_jual(void *pointer) {
    unsigned char *raw;
    sawit_block_header *b;
    sawit_block_header *prev = 0;
    sawit_block_header **link;
    if (!pointer) return;
    sawit_heap_init();
    raw = (unsigned char *)pointer - sizeof(sawit_block_header);
    b = (sawit_block_header *)(void *)raw;
    if (b->magic != SAWIT_HEAP_MAGIC || b->is_free) return;
    b->is_free = 1u;
    link = &sawit_heap_free;
    while (*link && (unsigned char *)*link < raw) {
        prev = *link;
        link = &(*link)->next;
    }
    b->next = *link;
    *link = b;
    if (b->next &&
        (unsigned char *)b + sizeof(sawit_block_header) + b->size == (unsigned char *)b->next &&
        b->next->is_free) {
        sawit_block_header *next = b->next;
        b->size += (unsigned int)sizeof(sawit_block_header) + next->size;
        b->next = next->next;
    }
    if (prev && prev->is_free &&
        (unsigned char *)prev + sizeof(sawit_block_header) + prev->size == raw) {
        prev->size += (unsigned int)sizeof(sawit_block_header) + b->size;
        prev->next = b->next;
    }
}

static inline unsigned int sawit_blok_ukuran(void *pointer) {
    sawit_block_header *b;
    if (!pointer) return 0u;
    b = (sawit_block_header *)(void *)
        ((unsigned char *)pointer - sizeof(sawit_block_header));
    return (b->magic == SAWIT_HEAP_MAGIC && !b->is_free) ? b->size : 0u;
}

/* ---------- Proses ringan (kernel thread, cooperative) ---------- */
#define SAWIT_MAX_PROCESSES 8u
#define SAWIT_PROC_STACK    16384u
#define SAWIT_PROC_FREE     0u
#define SAWIT_PROC_READY    1u
#define SAWIT_PROC_RUNNING  2u
#define SAWIT_PROC_EXITED   3u

typedef void (*sawit_process_entry)(void *);

struct sawit_process {
    unsigned int id;
    unsigned int state;
    uint32_t esp;
    uint32_t stack_top;
    uint32_t user_stack;   /* ESP ring 3 (0 untuk task kernel) */
    unsigned int ring;     /* 0 atau 3 */
    sawit_process_entry entry;
    void *arg;
    unsigned int ticks;
};

static struct sawit_process sawit_procs[SAWIT_MAX_PROCESSES];
static unsigned char sawit_proc_stacks[SAWIT_MAX_PROCESSES][SAWIT_PROC_STACK]
    __attribute__((aligned(16)));
static unsigned int sawit_proc_current = 0;
static int sawit_proc_ready = 0;

static void sawit_task_trampoline(void) {
    struct sawit_process *p = &sawit_procs[sawit_proc_current];
    if (p->entry) p->entry(p->arg);
    sawit_process_exit();
}

#if defined(__ELF__) && defined(__i386__)
/* Naked context switch: no compiler prologue/epilogue, so the fake frame
   layout is fully controlled by us. On entry the stack is [ret][old_esp][new_esp];
   after pusha (8 dwords) old_esp is at +36 and new_esp at +40. A task's saved
   esp points at the EDI slot, so `popa; ret` restores registers and jumps to
   sawit_task_trampoline (placed there by sawit_process_create). */
__attribute__((naked)) static void sawit_context_switch(uint32_t *old_esp, uint32_t new_esp) {
    (void)old_esp;
    (void)new_esp;
    __asm__ volatile (
        "pusha\n\t"
        "cli\n\t"
        "movl 36(%%esp), %%edx\n\t"
        "movl %%esp, (%%edx)\n\t"
        "movl 40(%%esp), %%edx\n\t"
        "movl %%edx, %%esp\n\t"
        "movl $0x10, %%eax\n\t"      /* segmen data kernel untuk task ring 0 */
        "movl %%eax, %%ds\n\t"
        "movl %%eax, %%es\n\t"
        "movl %%eax, %%fs\n\t"
        "movl %%eax, %%gs\n\t"
        "popa\n\t"                   /* popa memulihkan EAX, aman dipakai di atas */
        "sti\n\t"
        "ret\n\t"
        : : : "memory");
}

/* Sama seperti di atas, tapi masuk ke ring 3 lewat iret. Frame di stack task:
   [eax ecx edx ebx dummy-esp ebp esi edi][eip cs eflags esp ss].
   popa memulihkan register, lalu iret mengambil CSPL baru dari frame. */
__attribute__((naked)) static void sawit_context_switch_user(uint32_t *old_esp, uint32_t new_esp) {
    (void)old_esp;
    (void)new_esp;
    __asm__ volatile (
        "pusha\n\t"
        "cli\n\t"
        "movl 36(%%esp), %%edx\n\t"
        "movl %%esp, (%%edx)\n\t"
        "movl 40(%%esp), %%edx\n\t"
        "movl %%edx, %%esp\n\t"
        "movl $0x23, %%eax\n\t"      /* segmen data user, RPL 3 */
        "movl %%eax, %%ds\n\t"
        "movl %%eax, %%es\n\t"
        "movl %%eax, %%fs\n\t"
        "movl %%eax, %%gs\n\t"
        "popa\n\t"
        "iret\n\t"
        : : : "memory");
}
#else
static void sawit_context_switch(uint32_t *old_esp, uint32_t new_esp) {
    (void)old_esp; (void)new_esp;
}
static void sawit_context_switch_user(uint32_t *old_esp, uint32_t new_esp) {
    (void)old_esp; (void)new_esp;
}
#endif

static void sawit_process_init(void) {
    if (sawit_proc_ready) return;
    sawit_procs[0].id = 1;
    sawit_procs[0].state = SAWIT_PROC_RUNNING;
    sawit_procs[0].entry = 0;
    sawit_procs[0].arg = 0;
    sawit_procs[0].ticks = 0;
    sawit_procs[0].ring = 0u;
    sawit_procs[0].user_stack = 0u;
    sawit_proc_current = 0;
    sawit_proc_ready = 1;
    sawit_tss.esp0 = 0;
}

static int sawit_process_slot(void) {
    unsigned int i;
    sawit_process_init();
    for (i = 1; i < SAWIT_MAX_PROCESSES; ++i)
        if (sawit_procs[i].state == SAWIT_PROC_FREE ||
            sawit_procs[i].state == SAWIT_PROC_EXITED)
            return (int)i;
    return -1;
}

static int sawit_process_create(sawit_process_entry entry, void *arg) {
    int index;
    uint32_t top;
    uint32_t *sp;
    sawit_process_init();
    index = sawit_process_slot();
    if (index < 0) return -1;
    top = (uint32_t)(uintptr_t)
        (sawit_proc_stacks[index] + SAWIT_PROC_STACK);
    sp = (uint32_t *)(uintptr_t)top;
    sawit_procs[index].id = (unsigned int)index + 1u;
    sawit_procs[index].state = SAWIT_PROC_READY;
    sawit_procs[index].entry = entry;
    sawit_procs[index].arg = arg;
    sawit_procs[index].ticks = 0;
    sawit_procs[index].stack_top = top;
    sawit_procs[index].user_stack = 0u;
    sawit_procs[index].ring = 0u;
    *--sp = (uint32_t)(uintptr_t)&sawit_task_trampoline;
    *--sp = 0u; /* eax */
    *--sp = 0u; /* ecx */
    *--sp = 0u; /* edx */
    *--sp = 0u; /* ebx */
    *--sp = (uint32_t)(uintptr_t)sp; /* esp saat pusha */
    *--sp = 0u; /* ebp */
    *--sp = 0u; /* esi */
    *--sp = 0u; /* edi */
    sawit_procs[index].esp = (uint32_t)(uintptr_t)sp;
    return (int)sawit_procs[index].id;
}

/* ---------- Userland (ring 3) ----------
 * Payload, pesan, dan stack user sengaja ditempatkan di image kernel dan
 * halamannya dibuka lewat bit U/S. Hanya tiga halaman ini yang bisa diakses
 * ring 3; sisanya tetap supervisor. Ini bukan isolasi penuh (belum ada page
 * directory per proses), tapi cukup untuk membuktikan transisi CPL nyata. */

static char sawit_user_message[4096] __attribute__((aligned(4096))) =
    "Halo dari ring 3!\n";
static unsigned char sawit_user_stack[4096] __attribute__((aligned(4096)));

/* EAX = alamat buffer pesan. Tulis via syscall 2, lalu keluar via syscall 3. */
__attribute__((noinline, aligned(4096))) static void sawit_user_payload(void) {
#if defined(__ELF__) && defined(__i386__)
    __asm__ volatile (
        "movl %%eax, %%ecx\n\t"     /* ECX = buffer pesan */
        "movl $18, %%edx\n\t"       /* panjang "Halo dari ring 3!\n" */
        "movl $1, %%ebx\n\t"        /* fd 1 = layar */
        "movl $2, %%eax\n\t"        /* syscall write */
        "int $0x80\n\t"
        "movl $3, %%eax\n\t"        /* syscall exit */
        "int $0x80\n\t"
        "1: hlt\n\t"
        "jmp 1b\n\t"
        : : : "eax", "ebx", "ecx", "edx", "memory");
#endif
}

typedef void (*sawit_user_entry)(void);

static int sawit_user_create(sawit_user_entry entry, void *arg) {
    int index;
    uint32_t top, ustack;
    uint32_t *sp;
    sawit_process_init();
    index = sawit_process_slot();
    if (index < 0) return -1;
    top = (uint32_t)(uintptr_t)
        (sawit_proc_stacks[index] + SAWIT_PROC_STACK);
    sp = (uint32_t *)(uintptr_t)top;
    ustack = ((uint32_t)(uintptr_t)(sawit_user_stack +
              sizeof(sawit_user_stack) - 16u)) & ~0xFu;

    /* Frame iret: SS, ESP, EFLAGS, CS, EIP (didorong terbalik). */
    *--sp = 0x23u;                       /* SS = user data, RPL 3 */
    *--sp = ustack;                      /* ESP user */
    *--sp = 0x202u;                      /* EFLAGS: IF = 1 */
    *--sp = 0x1Bu;                       /* CS = user code, RPL 3 */
    *--sp = (uint32_t)(uintptr_t)entry;  /* EIP */

    /* Frame popa: eax ecx edx ebx (esp) ebp esi edi. */
    *--sp = 0u; /* edi */
    *--sp = 0u; /* esi */
    *--sp = 0u; /* ebp */
    *--sp = 0u; /* esp (diabaikan popa) */
    *--sp = 0u; /* ebx */
    *--sp = 0u; /* edx */
    *--sp = 0u; /* ecx */
    *--sp = (uint32_t)(uintptr_t)arg;    /* eax = arg */

    sawit_procs[index].id = (unsigned int)index + 1u;
    sawit_procs[index].state = SAWIT_PROC_READY;
    sawit_procs[index].entry = 0; /* user masuk lewat frame iret, bukan trampoline */
    sawit_procs[index].arg = arg;
    sawit_procs[index].ticks = 0;
    sawit_procs[index].stack_top = top;
    sawit_procs[index].user_stack = ustack;
    sawit_procs[index].ring = 3u;
    sawit_procs[index].esp = (uint32_t)(uintptr_t)sp;
    return (int)sawit_procs[index].id;
}

static int sawit_user_spawn(void) {
    uint32_t code = (uint32_t)(uintptr_t)&sawit_user_payload;
    uint32_t data = (uint32_t)(uintptr_t)&sawit_user_message[0];
    uint32_t stack = (uint32_t)(uintptr_t)&sawit_user_stack[0];
    sawit_paging_init();
    sawit_page_allow_user(code);
    sawit_page_allow_user(data);
    sawit_page_allow_user(stack);
    sawit_serial_write("[user] payload ring3 disiapkan\n");
    return sawit_user_create(sawit_user_payload, (void *)(uintptr_t)data);
}

static unsigned int sawit_user_count(void) {
    unsigned int i, n = 0;
    sawit_process_init();
    for (i = 0; i < SAWIT_MAX_PROCESSES; ++i)
        if ((sawit_procs[i].state == SAWIT_PROC_READY ||
             sawit_procs[i].state == SAWIT_PROC_RUNNING) &&
            sawit_procs[i].ring == 3u) ++n;
    return n;
}

static unsigned int sawit_current_ring(void) {
#if defined(__ELF__) && defined(__i386__)
    uint32_t cs;
    __asm__ volatile ("mov %%cs, %0" : "=r"(cs));
    return cs & 3u;
#else
    return 0u;
#endif
}

static void sawit_yield(void) {
    unsigned int i, next;
    struct sawit_process *old;
    sawit_process_init();
    old = &sawit_procs[sawit_proc_current];
    if (old->state == SAWIT_PROC_RUNNING) old->state = SAWIT_PROC_READY;
    for (i = 1; i <= SAWIT_MAX_PROCESSES; ++i) {
        next = (sawit_proc_current + i) % SAWIT_MAX_PROCESSES;
        if (sawit_procs[next].state == SAWIT_PROC_READY) {
            ++sawit_procs[next].ticks;
            sawit_procs[next].state = SAWIT_PROC_RUNNING;
            sawit_proc_current = next;
            sawit_tss.esp0 = sawit_procs[next].stack_top;
            if (sawit_procs[next].ring == 3u)
                sawit_context_switch_user(&old->esp, sawit_procs[next].esp);
            else
                sawit_context_switch(&old->esp, sawit_procs[next].esp);
            return;
        }
    }
    old->state = SAWIT_PROC_RUNNING;
}

static void sawit_process_exit(void) {
    sawit_procs[sawit_proc_current].state = SAWIT_PROC_EXITED;
    sawit_yield();
    for (;;) sawit_hlt();
}

static unsigned int sawit_process_current_id(void) {
    sawit_process_init();
    return sawit_procs[sawit_proc_current].id;
}

static unsigned int sawit_process_count(void) {
    unsigned int i, count = 0;
    sawit_process_init();
    for (i = 0; i < SAWIT_MAX_PROCESSES; ++i)
        if (sawit_procs[i].state == SAWIT_PROC_READY ||
            sawit_procs[i].state == SAWIT_PROC_RUNNING) ++count;
    return count;
}

static int sawit_syscall(unsigned int number, uint32_t a, uint32_t b, uint32_t c) {
    int result = -1;
#if defined(__ELF__) && defined(__i386__)
    __asm__ volatile ("int $0x80"
        : "=a"(result)
        : "a"(number), "b"(a), "c"(b), "d"(c)
        : "memory");
#else
    (void)number; (void)a; (void)b; (void)c;
#endif
    return result;
}

/* ---------- Penyimpanan: block device + filesystem sederhana ---------- */
#define SAWIT_SECTOR_SIZE        512u
#define SAWIT_RAMDISK_SECTORS    64u   /* 32 KB RAM disk */
#define SAWIT_DEV_RAM            0u
#define SAWIT_DEV_ATA            1u

static unsigned char sawit_ramdisk[SAWIT_RAMDISK_SECTORS * SAWIT_SECTOR_SIZE]
    __attribute__((aligned(512)));
static int sawit_ata_ready = 0;
static unsigned int sawit_ata_sectors = 0;

static inline void sawit_copy_bytes(unsigned char *dst, const unsigned char *src,
                                    unsigned int count) {
    while (count--) *dst++ = *src++;
}

/* ATA PIO: primary master, LBA28, polling. IRQ dimatikan (nIEN) supaya tidak
   memicu interupsi tak tertangani. */
static inline void sawit_ata_delay(void) {
    (void)sawit_inb(0x1F7); (void)sawit_inb(0x1F7);
    (void)sawit_inb(0x1F7); (void)sawit_inb(0x1F7);
}
static int sawit_ata_wait_bsy(unsigned int limit) {
    while (limit--) if (!(sawit_inb(0x1F7) & 0x80u)) return 0;
    return -1;
}
static int sawit_ata_wait_drq(unsigned int limit) {
    while (limit--) {
        unsigned char st = sawit_inb(0x1F7);
        if (st & 0x01u) return -1;                 /* ERR */
        if (st & 0x20u) return -1;                 /* DF  */
        if (st & 0x08u) return 0;                  /* DRQ */
        if (st & 0x80u) return -1;                 /* BSY */
    }
    return -1;
}
static int sawit_ata_init(void) {
    unsigned int i, lo = 0u, hi = 0u;
    if (sawit_ata_ready) return 0;
    sawit_outb(0x3F6, 0x02u);                      /* nIEN: matikan IRQ ATA */
    sawit_outb(0x1F6, 0xA0u);                      /* master, CHS mode */
    sawit_ata_delay();
    if (sawit_inb(0x1F7) == 0xFFu) return -1;      /* tidak ada drive */
    if (sawit_ata_wait_bsy(1000000u)) return -1;
    sawit_outb(0x1F7, 0xECu);                      /* IDENTIFY */
    if (sawit_ata_wait_drq(1000000u)) return -1;
    for (i = 0; i < 256u; ++i) {
        uint16_t w = sawit_inw(0x1F0);
        if (i == 60u) lo = w;
        else if (i == 61u) hi = w;
    }
    sawit_ata_sectors = lo | ((unsigned int)hi << 16);
    if (!sawit_ata_sectors) sawit_ata_sectors = 1u;
    sawit_ata_ready = 1;
    return 0;
}
static int sawit_ata_read(unsigned int lba, unsigned char *buf) {
    unsigned int i;
    uint16_t w;
    if (!sawit_ata_ready || !buf) return -1;
    if (sawit_ata_wait_bsy(1000000u)) return -1;
    sawit_outb(0x1F6, (uint8_t)(0xE0u | ((lba >> 24) & 0x0Fu)));
    sawit_outb(0x1F2, 0x01u);
    sawit_outb(0x1F3, (uint8_t)(lba & 0xFFu));
    sawit_outb(0x1F4, (uint8_t)((lba >> 8) & 0xFFu));
    sawit_outb(0x1F5, (uint8_t)((lba >> 16) & 0xFFu));
    sawit_outb(0x1F7, 0x20u);                      /* READ SECTORS */
    if (sawit_ata_wait_drq(1000000u)) return -1;
    for (i = 0; i < SAWIT_SECTOR_SIZE / 2u; ++i) {
        w = sawit_inw(0x1F0);
        buf[i * 2u] = (unsigned char)(w & 0xFFu);
        buf[i * 2u + 1u] = (unsigned char)(w >> 8);
    }
    return 0;
}
static int sawit_ata_write(unsigned int lba, const unsigned char *buf) {
    unsigned int i;
    if (!sawit_ata_ready || !buf) return -1;
    if (sawit_ata_wait_bsy(1000000u)) return -1;
    sawit_outb(0x1F6, (uint8_t)(0xE0u | ((lba >> 24) & 0x0Fu)));
    sawit_outb(0x1F2, 0x01u);
    sawit_outb(0x1F3, (uint8_t)(lba & 0xFFu));
    sawit_outb(0x1F4, (uint8_t)((lba >> 8) & 0xFFu));
    sawit_outb(0x1F5, (uint8_t)((lba >> 16) & 0xFFu));
    sawit_outb(0x1F7, 0x30u);                      /* WRITE SECTORS */
    if (sawit_ata_wait_drq(1000000u)) return -1;
    for (i = 0; i < SAWIT_SECTOR_SIZE / 2u; ++i) {
        uint16_t w = (uint16_t)buf[i * 2u] |
                     ((uint16_t)buf[i * 2u + 1u] << 8);
        sawit_outw(0x1F0, w);
    }
    return sawit_ata_wait_bsy(1000000u);
}

/* API block device: 0 = RAM disk, 1 = ATA primary master. */
static int sawit_blk_ok(unsigned int dev) {
    if (dev == SAWIT_DEV_RAM) return 1;
    if (dev == SAWIT_DEV_ATA) { sawit_ata_init(); return sawit_ata_ready; }
    return 0;
}
static unsigned int sawit_blk_sectors(unsigned int dev) {
    if (dev == SAWIT_DEV_RAM) return SAWIT_RAMDISK_SECTORS;
    if (dev == SAWIT_DEV_ATA) { sawit_ata_init(); return sawit_ata_sectors; }
    return 0u;
}
static int sawit_blk_read(unsigned int dev, unsigned int lba, unsigned char *buf) {
    unsigned int i;
    if (!buf) return -1;
    if (dev == SAWIT_DEV_RAM) {
        if (lba >= SAWIT_RAMDISK_SECTORS) return -1;
        sawit_copy_bytes(buf, sawit_ramdisk + lba * SAWIT_SECTOR_SIZE,
                         SAWIT_SECTOR_SIZE);
        return 0;
    }
    if (dev == SAWIT_DEV_ATA) return sawit_ata_read(lba, buf);
    return -1;
}
static int sawit_blk_write(unsigned int dev, unsigned int lba,
                           const unsigned char *buf) {
    unsigned int i;
    if (!buf) return -1;
    if (dev == SAWIT_DEV_RAM) {
        if (lba >= SAWIT_RAMDISK_SECTORS) return -1;
        sawit_copy_bytes(sawit_ramdisk + lba * SAWIT_SECTOR_SIZE, buf,
                         SAWIT_SECTOR_SIZE);
        return 0;
    }
    if (dev == SAWIT_DEV_ATA) return sawit_ata_write(lba, buf);
    return -1;
}

/* SawitFS: superblock sektor 0, bitmap sektor 1, inode sektor 2, data sektor 3+.
   Satu berkas = satu blok 512 byte (cukup untuk demo shell). */
#define SAWIT_FS_MAGIC        0x53465753u  /* "SWFS" */
#define SAWIT_FS_VERSION      1u
#define SAWIT_FS_MAX_FILES    16u
#define SAWIT_FS_NAME_LEN     12u
#define SAWIT_FS_SECTOR_SUPER 0u
#define SAWIT_FS_SECTOR_MAP   1u
#define SAWIT_FS_SECTOR_INODE 2u
#define SAWIT_FS_DATA_START   3u

struct sawit_fs_inode {
    char name[SAWIT_FS_NAME_LEN];
    uint32_t size;
    uint32_t block;
    uint32_t flags;                              /* 0 = kosong, 1 = terpakai */
} __attribute__((packed));

static unsigned char sawit_fs_scratch[SAWIT_SECTOR_SIZE]
    __attribute__((aligned(4)));
static unsigned int sawit_fs_dev = SAWIT_DEV_RAM;
static int sawit_fs_ready = 0;
static unsigned int sawit_fs_blocks = 0u;

static void sawit_fs_setname(char *dst, const char *src) {
    unsigned int i = 0u;
    if (!dst) return;
    if (!src) { dst[0] = '\0'; return; }
    while (i < SAWIT_FS_NAME_LEN - 1u && src[i]) { dst[i] = src[i]; ++i; }
    dst[i] = '\0';
}
static void sawit_fs_inode_load(unsigned int index, struct sawit_fs_inode *out) {
    unsigned char *raw;
    if (!out) return;
    out->name[0] = '\0'; out->size = 0u; out->block = 0u; out->flags = 0u;
    if (index >= SAWIT_FS_MAX_FILES) return;
    if (sawit_blk_read(sawit_fs_dev, SAWIT_FS_SECTOR_INODE, sawit_fs_scratch))
        return;
    raw = sawit_fs_scratch + index * sizeof(struct sawit_fs_inode);
    sawit_copy_bytes((unsigned char *)(void *)out, raw,
                     (unsigned int)sizeof(struct sawit_fs_inode));
}
static void sawit_fs_inode_store(unsigned int index,
                                 const struct sawit_fs_inode *in) {
    unsigned char *raw;
    if (!in || index >= SAWIT_FS_MAX_FILES) return;
    if (sawit_blk_read(sawit_fs_dev, SAWIT_FS_SECTOR_INODE, sawit_fs_scratch))
        return;
    raw = sawit_fs_scratch + index * sizeof(struct sawit_fs_inode);
    sawit_copy_bytes(raw, (const unsigned char *)(const void *)in,
                     (unsigned int)sizeof(struct sawit_fs_inode));
    (void)sawit_blk_write(sawit_fs_dev, SAWIT_FS_SECTOR_INODE, sawit_fs_scratch);
}
static int sawit_fs_find(const char *name, unsigned int *index,
                         struct sawit_fs_inode *out) {
    unsigned int i;
    struct sawit_fs_inode inode;
    if (!sawit_fs_ready || !name) return -1;
    for (i = 0; i < SAWIT_FS_MAX_FILES; ++i) {
        sawit_fs_inode_load(i, &inode);
        if (inode.flags && sawit_sama_kata(inode.name, name)) {
            if (index) *index = i;
            if (out) *out = inode;
            return (int)i;
        }
    }
    return -1;
}
static int sawit_fs_alloc_block(void) {
    unsigned int bit;
    if (!sawit_fs_ready) return -1;
    if (sawit_blk_read(sawit_fs_dev, SAWIT_FS_SECTOR_MAP, sawit_fs_scratch))
        return -1;
    for (bit = 0u; bit < sawit_fs_blocks; ++bit) {
        unsigned char mask = (unsigned char)(1u << (bit & 7u));
        if (!(sawit_fs_scratch[bit >> 3] & mask)) {
            sawit_fs_scratch[bit >> 3] |= mask;
            if (sawit_blk_write(sawit_fs_dev, SAWIT_FS_SECTOR_MAP,
                                sawit_fs_scratch))
                return -1;
            return SAWIT_FS_DATA_START + (int)bit;
        }
    }
    return -1;
}
static void sawit_fs_free_block(unsigned int block) {
    unsigned int bit;
    if (!sawit_fs_ready || block < SAWIT_FS_DATA_START) return;
    bit = block - SAWIT_FS_DATA_START;
    if (bit >= sawit_fs_blocks) return;
    if (sawit_blk_read(sawit_fs_dev, SAWIT_FS_SECTOR_MAP, sawit_fs_scratch))
        return;
    sawit_fs_scratch[bit >> 3] &= (unsigned char)~(1u << (bit & 7u));
    (void)sawit_blk_write(sawit_fs_dev, SAWIT_FS_SECTOR_MAP, sawit_fs_scratch);
}

static int sawit_fs_format(unsigned int dev) {
    unsigned int sectors, blocks, sector, byte;
    if (!sawit_blk_ok(dev)) return -1;
    sectors = sawit_blk_sectors(dev);
    if (sectors <= SAWIT_FS_DATA_START) return -1;
    blocks = sectors - SAWIT_FS_DATA_START;
    if (blocks > (SAWIT_SECTOR_SIZE * 8u)) blocks = SAWIT_SECTOR_SIZE * 8u;
    for (sector = 0u; sector < 3u; ++sector) {
        for (byte = 0u; byte < SAWIT_SECTOR_SIZE; ++byte)
            sawit_fs_scratch[byte] = 0u;
        if (sawit_blk_write(dev, sector, sawit_fs_scratch)) return -1;
    }
    for (byte = 0u; byte < SAWIT_SECTOR_SIZE; ++byte) sawit_fs_scratch[byte] = 0u;
    *(uint32_t *)(void *)sawit_fs_scratch = SAWIT_FS_MAGIC;
    *(uint32_t *)(void *)(sawit_fs_scratch + 4) = SAWIT_FS_VERSION;
    *(uint32_t *)(void *)(sawit_fs_scratch + 8) = sectors;
    *(uint32_t *)(void *)(sawit_fs_scratch + 12) = blocks;
    if (sawit_blk_write(dev, SAWIT_FS_SECTOR_SUPER, sawit_fs_scratch)) return -1;
    sawit_fs_dev = dev;
    sawit_fs_blocks = blocks;
    sawit_fs_ready = 1;
    return 0;
}
static int sawit_fs_mount(unsigned int dev) {
    uint32_t magic, version, blocks;
    if (!sawit_blk_ok(dev)) return -1;
    if (sawit_blk_read(dev, SAWIT_FS_SECTOR_SUPER, sawit_fs_scratch)) return -1;
    magic = *(uint32_t *)(void *)sawit_fs_scratch;
    version = *(uint32_t *)(void *)(sawit_fs_scratch + 4);
    blocks = *(uint32_t *)(void *)(sawit_fs_scratch + 12);
    if (magic != SAWIT_FS_MAGIC || version != SAWIT_FS_VERSION || !blocks)
        return -1;
    sawit_fs_dev = dev;
    sawit_fs_blocks = blocks;
    sawit_fs_ready = 1;
    return 0;
}
static inline int sawit_fs_mounted(void) { return sawit_fs_ready; }
static inline unsigned int sawit_fs_total_blocks(void) { return sawit_fs_blocks; }
static unsigned int sawit_fs_free_blocks(void) {
    unsigned int bit, free = 0u;
    if (!sawit_fs_ready) return 0u;
    if (sawit_blk_read(sawit_fs_dev, SAWIT_FS_SECTOR_MAP, sawit_fs_scratch))
        return 0u;
    for (bit = 0u; bit < sawit_fs_blocks; ++bit)
        if (!(sawit_fs_scratch[bit >> 3] & (1u << (bit & 7u)))) ++free;
    return free;
}
static int sawit_fs_create(const char *name) {
    struct sawit_fs_inode inode;
    unsigned int i, block;
    if (!sawit_fs_ready || !name || !name[0]) return -1;
    if (sawit_fs_find(name, 0, 0) >= 0) return -1;
    for (i = 0; i < SAWIT_FS_MAX_FILES; ++i) {
        sawit_fs_inode_load(i, &inode);
        if (!inode.flags) break;
    }
    if (i >= SAWIT_FS_MAX_FILES) return -1;
    block = (unsigned int)sawit_fs_alloc_block();
    if (block < SAWIT_FS_DATA_START) return -1;
    sawit_fs_setname(inode.name, name);
    inode.size = 0u;
    inode.block = block;
    inode.flags = 1u;
    sawit_fs_inode_store(i, &inode);
    for (block = 0u; block < SAWIT_SECTOR_SIZE; ++block)
        sawit_fs_scratch[block] = 0u;
    (void)sawit_blk_write(sawit_fs_dev, inode.block, sawit_fs_scratch);
    return (int)i;
}
static int sawit_fs_open(const char *name) { return sawit_fs_find(name, 0, 0); }
static int sawit_fs_read(int fd, unsigned char *buf, unsigned int len) {
    struct sawit_fs_inode inode;
    unsigned int count;
    if (fd < 0 || !buf || !sawit_fs_ready) return -1;
    sawit_fs_inode_load((unsigned int)fd, &inode);
    if (!inode.flags) return -1;
    count = len;
    if (count > inode.size) count = inode.size;
    if (count > SAWIT_SECTOR_SIZE) count = SAWIT_SECTOR_SIZE;
    if (sawit_blk_read(sawit_fs_dev, inode.block, sawit_fs_scratch)) return -1;
    sawit_copy_bytes(buf, sawit_fs_scratch, count);
    return (int)count;
}
static int sawit_fs_write(int fd, const unsigned char *buf, unsigned int len) {
    struct sawit_fs_inode inode;
    unsigned int i, count;
    if (fd < 0 || !buf || !sawit_fs_ready) return -1;
    sawit_fs_inode_load((unsigned int)fd, &inode);
    if (!inode.flags) return -1;
    count = len;
    if (count > SAWIT_SECTOR_SIZE) count = SAWIT_SECTOR_SIZE;
    for (i = 0; i < SAWIT_SECTOR_SIZE; ++i) sawit_fs_scratch[i] = 0u;
    sawit_copy_bytes(sawit_fs_scratch, buf, count);
    if (sawit_blk_write(sawit_fs_dev, inode.block, sawit_fs_scratch)) return -1;
    inode.size = count;
    sawit_fs_inode_store((unsigned int)fd, &inode);
    return (int)count;
}
static int sawit_fs_remove(const char *name) {
    struct sawit_fs_inode inode;
    unsigned int index;
    if (sawit_fs_find(name, &index, &inode) < 0) return -1;
    sawit_fs_free_block(inode.block);
    inode.name[0] = '\0'; inode.flags = 0u; inode.size = 0u; inode.block = 0u;
    sawit_fs_inode_store(index, &inode);
    return 0;
}
static int sawit_fs_size(const char *name) {
    struct sawit_fs_inode inode;
    if (sawit_fs_find(name, 0, &inode) < 0) return -1;
    return (int)inode.size;
}
static void sawit_fs_list(void) {
    struct sawit_fs_inode inode;
    unsigned int i;
    if (!sawit_fs_ready) return;
    for (i = 0; i < SAWIT_FS_MAX_FILES; ++i) {
        sawit_fs_inode_load(i, &inode);
        if (inode.flags)
            sawit_buzzer("%s  %d byte\n", inode.name, (int)inode.size);
    }
}

/* Layanan syscall filesystem: dipanggil lewat panggilSistem(). */
static inline int sawit_sys_read(int fd, unsigned char *buf, unsigned int len) {
    return (int)sawit_syscall(4u, (uint32_t)(uintptr_t)fd,
                              (uint32_t)(uintptr_t)buf, (uint32_t)(uintptr_t)len);
}
static inline int sawit_sys_write(int fd, const unsigned char *buf, unsigned int len) {
    return (int)sawit_syscall(5u, (uint32_t)(uintptr_t)fd,
                              (uint32_t)(uintptr_t)buf, (uint32_t)(uintptr_t)len);
}
static inline int sawit_sys_open(const char *name) {
    return (int)sawit_syscall(6u, (uint32_t)(uintptr_t)name, 0u, 0u);
}
static inline void sawit_sys_list(void) { (void)sawit_syscall(7u, 0u, 0u, 0u); }

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
