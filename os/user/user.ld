OUTPUT_ARCH(riscv)
ENTRY(main)
BASE_ADDRESS = 0x0;

SECTIONS {
    . = BASE_ADDRESS;
    .text : { 
        *(.text.main)
        *(.text.entry)
        *(.text .text.*)
    }

    .bss : { 
        *(.bss .bss.*)
        *(.sbss .sbss.*)
    }

    .data : { 
        *(.data .rodata) 
        *(.sdata .sdata.*)
    }
}