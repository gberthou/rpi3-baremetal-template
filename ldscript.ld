MEMORY
{
	KERNEL  : ORIGIN = 0x8000, LENGTH = 0x4000000
}

SECTIONS
{
    .text : ALIGN(4)
    {
        KEEP(*(.text.boot))
        . = ALIGN(4);
        *(.text .text.*)
    } > KERNEL


	.rodata : ALIGN(4)
    {
		*(.rodata .rodata.*)
    } > KERNEL

	.data : ALIGN(4)
    {
		*(.data .data.*)
	} > KERNEL

	.bss : ALIGN(4)
    {
        __bss_begin = .;
		*(.bss .bss.*)
        . = ALIGN(4);
        __bss_end = .;
	} > KERNEL
}

