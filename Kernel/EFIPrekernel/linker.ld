ENTRY(start)

SECTIONS
{
    . = 0x1000;

    start_of_prekernel_image = .;

    .text ALIGN(4K) :
    {
        *(.text*)
    }

    .rodata ALIGN(4K) :
    {
        *(.rodata* .srodata*)
    }

    .data ALIGN(4K) :
    {
        *(.data* .sdata*)
    }

    .bss ALIGN(4K) (NOLOAD) :
    {
        start_of_bss = .;
        *(COMMON)
        *(.bss* .sbss*)
        end_of_bss = .;
    }

    .reloc :
    {
        *(.reloc)
    }

    end_of_prekernel_image = .;
}

/* Needed by aarch64's boot.S. */
size_of_bss_divided_by_8 = (end_of_bss - start_of_bss + 7) / 8;
