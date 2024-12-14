.data :
{
    __data_start = .;
    __data_load_start = LOADADDR(.data);
    *(.data .data.* .gnu.linkonce.d.*)
    SORT(CONSTRUCTORS)
    . = ALIGN(4);
    __data_end = .;
}

.bss :
{
    __bss_start = .;
    *(.dynbss)
    *(.bss .bss.* .gnu.linkonce.b.*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end = .;
}

.rodata :
{
    __rodata_start = .;
    *(.rodata .rodata.* .gnu.linkonce.r.*)
    *(.rodata1)
    . = ALIGN(4);
    __rodata_end = .;
}
