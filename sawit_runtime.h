#ifndef SAWIT_RUNTIME_H
#define SAWIT_RUNTIME_H

/* Dua mode:
 *  - hosted (default): memakai libc, cocok untuk aplikasi & shell Sawit.
 *  - freestanding (#define SAWIT_FREESTANDING): memakai sawit_kernel.h,
 *    tanpa libc, untuk kernel yang di-build dengan --freestanding.
 */
#ifdef SAWIT_FREESTANDING
#include "sawit_kernel.h"
#else

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

static int sawit_stdout_bungkam = 0;
static int sawit_stderr_bungkam = 0;
static int sawit_stdin_bungkam = 0;

static inline void sawit_bungkam(FILE *stream) {
    if (stream == stdout) sawit_stdout_bungkam = 1;
    if (stream == stderr) sawit_stderr_bungkam = 1;
}
static inline void sawit_buka_suara(FILE *stream) {
    if (stream == stdout) sawit_stdout_bungkam = 0;
    if (stream == stderr) sawit_stderr_bungkam = 0;
}
static inline void sawit_tutup_telinga(void) { sawit_stdin_bungkam = 1; }
static inline void sawit_buka_telinga(void)  { sawit_stdin_bungkam = 0; }

static inline void sawit_buzzer(const char *format, ...) {
    if (sawit_stdout_bungkam) return;
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
    fflush(stdout);
}

static inline void sawit_demo_masyarakat(const char *format, ...) {
    if (sawit_stderr_bungkam) return;
    va_list args;
    va_start(args, format);
    fputs("[DEMO MASYARAKAT] ", stderr);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);
    fflush(stderr);
}

static inline void sawit_bersihkan_layar(void) {
#ifdef _WIN32
    (void)system("cls");
#else
    fputs("\033[2J\033[H", stdout);
    fflush(stdout);
#endif
}

/* ---------- Input: sadap() menyadap stdin ---------- */
static inline int sawit_sadap(const char *format, ...) {
    if (sawit_stdin_bungkam) return -1;
    va_list args;
    va_start(args, format);
    int result = vscanf(format, args);
    va_end(args);
    return result;
}

/* sadapBaris: fgets + buang \n/\r di akhir. return 0 (NULL) bila EOF. */
static inline char *sawit_sadap_baris(char *buffer, size_t size) {
    if (sawit_stdin_bungkam || !buffer || size == 0) return NULL;
    if (!fgets(buffer, (int)size, stdin)) return NULL;
    size_t n = strlen(buffer);
    while (n && (buffer[n - 1] == '\n' || buffer[n - 1] == '\r')) buffer[--n] = '\0';
    return buffer;
}

static inline int sawit_sadap_karakter(void) {
    if (sawit_stdin_bungkam) return -1;
    return getchar();
}

/* Buang sisa baris setelah sadap() supaya newline tidak mengganggu input berikut. */
static inline void sawit_sadap_sisa(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

/* ---------- String & konversi ---------- */
static inline int sawit_sama_kata(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}
static inline size_t sawit_panjang_kata(const char *text) { return strlen(text); }
static inline void sawit_salin_kata(char *dst, const char *src) { (void)strcpy(dst, src); }
static inline int sawit_terjemah_angka(const char *text) { return atoi(text); }

#define sawit_pajak(value, percent) \
    ((value) -= ((value) * (percent)) / 100)

/* MBG credits funds; cair disburses only what is currently available. */
static long long sawit_anggaran = 0;

static inline long long sawit_mbg(long long amount) {
    if (amount > 0) sawit_anggaran += amount;
    return amount;
}
static inline long long sawit_cair(long long amount) {
    if (amount <= 0 || amount > sawit_anggaran) return 0;
    sawit_anggaran -= amount;
    return amount;
}

/* Tracked heap allocations: beli allocates, jual releases and unregisters. */
typedef struct SawitAllocation {
    void *pointer;
    size_t bytes;
    struct SawitAllocation *next;
} SawitAllocation;

static SawitAllocation *sawit_allocations = NULL;

static inline void *sawit_beli(size_t bytes) {
    if (bytes == 0) bytes = 1;
    void *pointer = malloc(bytes);
    if (!pointer) return NULL;
    SawitAllocation *record = (SawitAllocation *)malloc(sizeof(SawitAllocation));
    if (!record) {
        free(pointer);
        return NULL;
    }
    record->pointer = pointer;
    record->bytes = bytes;
    record->next = sawit_allocations;
    sawit_allocations = record;
    return pointer;
}

static inline void sawit_jual(void *pointer) {
    if (!pointer) return;
    SawitAllocation **cursor = &sawit_allocations;
    while (*cursor) {
        if ((*cursor)->pointer == pointer) {
            SawitAllocation *record = *cursor;
            *cursor = record->next;
            free(record);
            free(pointer);
            return;
        }
        cursor = &(*cursor)->next;
    }
    /* Pointer bukan hasil beli(): abaikan. Ini mencegah double-free
       (jual dua kali pada pointer yang sama) dan mencegah free() pada
       pointer yang tidak dimiliki allocator (mis. variabel stack). */
}

/* rampas(): pindahkan kepemilikan pointer dari src ke dst, lalu kosongkan src.
 * Catatan alokasi tidak perlu diubah — ia sudah terikat pada nilai pointer.
 * Setelah ini jual(src) jadi no-op dan jual(dst) membebaskan tepat sekali. */
static inline void sawit_rampas(void **dst, void **src) {
    if (!dst || !src || dst == src) return;
    void *pointer = *src;
    *dst = pointer;
    *src = NULL;
}

#define sawit_nyawit(function) ((function)())

#endif /* SAWIT_FREESTANDING */
#endif /* SAWIT_RUNTIME_H */
