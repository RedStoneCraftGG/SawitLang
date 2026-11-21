/* Multiboot 2 header + entry point untuk kernel Sawit/S++.
 * Dirakit oleh gcc -m32 (GAS), bukan NASM. NASM dipakai nanti untuk boot sector. */
.extern main

.section .multiboot, "a"
.align 4
/* Multiboot 1: dikenali QEMU -kernel dan GRUB "multiboot".
   flags=0 → tidak butuh field tambahan; entry diambil dari ELF. */
.long 0x1BADB002
.long 0
.long -(0x1BADB002)

/* Multiboot 2: dipakai GRUB "multiboot2" bila diinginkan. */
.align 4
multiboot_header_start:
    .long 0xE85250D6                                  /* magic Multiboot 2 */
    .long 0                                           /* arsitektur i386 */
    .long multiboot_header_end - multiboot_header_start /* panjang header */
    .long -(0xE85250D6 + (multiboot_header_end - multiboot_header_start)) /* checksum */
    /* tag akhir */
    .short 0
    .short 0
    .long 8
multiboot_header_end:

.section .text
.global _start
.type _start, @function
_start:
    mov $stack_top, %esp
    /* GRUB mengirim magic di EAX dan pointer info di EBX. Simpan dulu
       sebelum caller apa pun mengubah register ini. */
    mov %eax, sawit_mb_magic
    mov %ebx, sawit_mb_info
    call main
1:
    cli
    hlt
    jmp 1b
.size _start, . - _start

.section .bss
.align 16
stack_bottom:
    .skip 16384
stack_top:

.align 4
.global sawit_mb_magic
sawit_mb_magic:
    .skip 4
.global sawit_mb_info
sawit_mb_info:
    .skip 4
