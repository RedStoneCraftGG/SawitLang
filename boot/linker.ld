/* Linker script untuk kernel Sawit/S++ (Multiboot 2, i386). */
ENTRY(_start)

SECTIONS
{
    /* Image dimuat di 1M. Header Multiboot HARUS menjadi section pertama
       agar mendarat di offset file 0x1000 (QEMU hanya memindai 8KB awal).
       _start mengikuti header (~24 byte), jadi entry sedikit di atas 0x100000. */
    . = 1M;

    .multiboot : ALIGN(4)
    {
        *(.multiboot)
    }

    .text :
    {
        *(.text)
        *(.text.*)
    }

    .rodata : { *(.rodata*) }
    .data   : { *(.data*) }

    .bss :
    {
        *(.bss*)
        *(COMMON)
    }

    /DISCARD/ :
    {
        *(.eh_frame)
        *(.note.GNU-stack)
        *(.comment)
    }
}
