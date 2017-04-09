OUTPUT_ARCH(arm)
ENTRY(_start)

SECTIONS {
    . = 0x80000000;
    
	.text : {
			* (.text)
	}
	. = ALIGN(4);
	.data : {
			* (.data)
	}
	. = ALIGN(4);
    __bss_start = .;
    .bss ALIGN(4)  : { *(.bss)  *(COMMON) }
    __bss_end = .;
}
 
