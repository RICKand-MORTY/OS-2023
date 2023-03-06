SECTIONS
{
	. = 0x80200000,

	.text.boot : { *(.text.boot) }
	. = ALIGN(8);
	_text = .;
	.text : { *(.text) }
	_etext = .;
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	. = ALIGN(0x8);
	bss_begin = .;
	.bss : { *(.bss*) } 
	bss_end = .;
}

