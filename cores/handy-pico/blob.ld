/* Experimental flat image. Uses existing armSNES overlay, not yet wired into LK. */
ENTRY(handy_blob_entry)
SECTIONS
{
    . = 0x4F000000;
    .header : {
        LONG(0x31584e4c);
        LONG(2);
        LONG(handy_blob_entry - 0x4F000000);
        LONG(__load_end - 0x4F000000);
        LONG(__span_end - 0x4F000000);
    }
    .text : { *(.text .text.* .gnu.linkonce.t.*) }
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r.*) }
    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) }
    .ARM.exidx : { __exidx_start = .; *(.ARM.exidx* .gnu.linkonce.armexidx.*) __exidx_end = .; }
    .preinit_array : {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    }
    .init_array : {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN(__init_array_end = .);
    }
    .fini_array : {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN(__fini_array_end = .);
    }
    .data : {
        *(.data .data.* .gnu.linkonce.d.*)
        *(.got .got.*)
        . = ALIGN(16);
        BYTE(0)
        . = ALIGN(16);
    }
    __load_end = .;
    .bss (NOLOAD) : {
        *(.bss .bss.* .gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(16);
    }
    __span_end = .;
    end = .; _end = .;
    /DISCARD/ : { *(.comment) *(.ARM.attributes) *(.note*) }
    ASSERT(__span_end <= 0x50000000, "Handy blob exceeds overlay slot")
}
