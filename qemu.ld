OUTPUT_ARCH(riscv)
ENTRY(k_entry)

BASE_ADDRESS = 0xFFFFFFFF80200000;

SECTIONS
{
    . = BASE_ADDRESS;
    _start = .;

    .text : {
        stext = .;
        *(.text.entry)
        *(.text .text.*)
        . = ALIGN(4K);
        etext = .;
    }

    .rodata : {
        srodata = .;
        *(.rodata .rodata.*)
        . = ALIGN(4K);
        erodata = .;
    }

    .data : {
        sdata = .;
        *(.data .data.*)
        edata = .;
    }

    .bss : {
        sbss = .;
        *(.bss .bss.*)
        ebss = .;
    }

    _end = .;
}