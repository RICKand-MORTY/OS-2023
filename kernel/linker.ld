SECTIONS
{
	. = 0x80200000,

	_text_boot = .;
	.text.boot : { *(.text.boot) }
	_etext_boot = .;

	. = ALIGN(8);
	_text = .;
	.text : { *(.text) }
	. = ALIGN(4096);
	_etext = .;
	
	_rodata = .;
	.rodata : { *(.rodata) }
	_erodata = .;

	_data = .;
	.data : { *(.data) }
	. = ALIGN(4096);
       _pgd_page = .;
       . += 4096;
	_edata = .;
	
	. = ALIGN(0x8);
	_bss_begin = .;
	.bss : { *(.bss*) } 
	_bss_end = .;
}

