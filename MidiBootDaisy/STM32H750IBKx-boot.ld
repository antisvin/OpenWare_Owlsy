/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
/* _estack = 0x20020000;*/    /* end of RAM */
/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x000;      /* required amount of heap  */
_Min_Stack_Size = 0x200; /* required amount of stack */

/* Specify the memory areas */
MEMORY
{
/* ITCMRAM (xrw)     : ORIGIN = 0x00000000, LENGTH = 64K */
/* DTCMRAM (xrw)     : ORIGIN = 0x20000000, LENGTH = 128K */
RAM (rwx)         : ORIGIN = 0x24000000, LENGTH = 512K
EXTRAM (rwx)      : ORIGIN = 0xc0000000, LENGTH = 64M
FLASH (r)         : ORIGIN = 0x08000000, LENGTH = 128K  /* Bootloader on flash - we only need 32k or so, but don't have other use for it*/
APPLICATION (rx)  : ORIGIN = 0x90000000, LENGTH = 512K  /* QSPI flash - firmware part */
STORAGE (rx)      : ORIGIN = 0x90080000, LENGTH = 7680K  /* QSPI flash - patch storage part. That's one hell of a lot of patches */
}
/* Device flash is 128k and only used for bootloader
 QSPI flash is 8M, split it between FW and patch/settings storage
*/

SECTIONS
{
	.isr_vector :
	{
		. = ALIGN(4);
    	PROVIDE ( _ISR_VECTOR = . );
		KEEP(*(.isr_vector))
		. = ALIGN(4);
	} > FLASH

  	.bootloader_token :
  	{
    	. = ALIGN(4);
    	KEEP(*(.bootloader_token));
  	} > FLASH	

	.text :
	{
		. = ALIGN(4);
		_stext = .;

		*(.text)
		*(.text*)
		*(.rodata)
		*(.rodata*)
		*(.glue_7)
		*(.glue_7t)
		KEEP(*(.init))
		KEEP(*(.fini))
		. = ALIGN(4);
		_etext = .;

	} > FLASH

	/* Constant data goes into FLASH */
  	.rodata :
  	{
    	. = ALIGN(4);
    	*(.rodata)         /* .rodata sections (constants, strings, etc.) */
    	*(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    	. = ALIGN(4);
  	} >FLASH

    .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } >FLASH

	.ARM.attributes :
	{
		*(.ARM.attributes)
	} > FLASH

	.preinit_array :
	{
		PROVIDE(__preinit_array_start = .);
		KEEP(*(.preinit_array*))
		PROVIDE(__preinit_array_end = .);
	} > FLASH

	.init_array :
	{
		PROVIDE(__init_array_start = .);
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array*))
		PROVIDE(__init_array_end = .);
	} > FLASH

	.fini_array :
	{
		PROVIDE(__fini_array_start = .);
		KEEP(*(.fini_array*))
		KEEP(*(SORT(.fini_array.*)))
		PROVIDE(__fini_array_end = .);
	} > FLASH

	.data :
	{
		. = ALIGN(4);
		_sdata = .;

		PROVIDE(__data_start__ = _sdata);
		*(.data)
		*(.data*)
		. = ALIGN(4);
		_edata = .;

		PROVIDE(__data_end__ = _edata);
	} > RAM AT >FLASH

	_sidata = LOADADDR(.data);

	.bss :
	{
		. = ALIGN(4);
		_sbss = .;

		PROVIDE(__bss_start__ = _sbss);
		*(.bss)
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;

		PROVIDE(__bss_end__ = _ebss);
	} > RAM


	/* User_heap_stack section, used to check that there is enough RAM left */
  	._user_heap_stack :
  	{
    	. = ALIGN(4);
	    PROVIDE ( _end = . );
    	. = . + _Min_Heap_Size;
	    . = . + _Min_Stack_Size;
    	. = ALIGN(4);
  	} > RAM
  	_estack = ORIGIN(RAM)+LENGTH(RAM); /* end of firmware RAM */


	PROVIDE(end = .); 
/*
	.dtcmram_bss (NOLOAD) :
	{
		. = ALIGN(4);
		_sdtcmram_bss = .;

		PROVIDE(__dtcmram_bss_start__ = _sdtcmram_bss);
		*(.dtcmram_bss)
		*(.dtcmram_bss*)
		. = ALIGN(4);
		_edtcmram_bss = .;

		PROVIDE(__dtcmram_bss_end__ = _edtcmram_bss);
	} > DTCMRAM

	.sram1_bss (NOLOAD) :
	{
		. = ALIGN(4);
		_ssram1_bss = .;

		PROVIDE(__sram1_bss_start__ = _sram1_bss);
		*(.sram1_bss)
		*(.sram1_bss*)
		. = ALIGN(4);
		_esram1_bss = .;

		PROVIDE(__sram1_bss_end__ = _esram1_bss);
	} > RAM_D2
*/
  	/* External memory, vars must be located here explicitly */
  	/* Example: int foo __attribute__ ((section (".extdata"))); */
  	.extdata (NOLOAD) :
  	{
	    /* *(.extdata) */
    	/* . = ALIGN(8); */
    	PROVIDE ( _EXTRAM = . );
    } >EXTRAM
    _EXTRAM_END = ORIGIN(EXTRAM) + LENGTH(EXTRAM);

	/*
	.sdram_text :
	{
		. = ALIGN(4);
		_ssdram_text = .;

		PROVIDE(__sdram_text_start = _ssdram_text);
		*(.sdram_text)
		*(.sdram_text*)
		. = ALIGN(4);
		_esdram_text = .;

		PROVIDE(__sdram_text_end = _esdram_text);
	} > SDRAM AT >FLASH
	_sisdram_text = LOADADDR(.sdram_text);

	.sdram_bss (NOLOAD) :
	{
		. = ALIGN(4);
		_ssdram_bss = .;

		PROVIDE(__sdram_bss_start = _ssdram_bss);
		*(.sdram_bss)
		*(.sdram_bss*)
		. = ALIGN(4);
		_esdram_bss = .;

		PROVIDE(__sdram_bss_end = _esdram_bss);
	} > SDRAM
*/
/*
    .qspiflash_text :
	{
		. = ALIGN(4);
		_sqspiflash_text = .;

		PROVIDE(__qspiflash_text_start = _sqspiflash_text);
		*(.qspiflash_text)
		*(.qspiflash_text*)
		. = ALIGN(4);
		_eqspiflash_text = .;

		PROVIDE(__qspiflash_text_end = _eqspiflash_text);
	} > QSPIFLASH

	.qspiflash_data :
	{
		. = ALIGN(4);
		_sqspiflash_data = .;

		PROVIDE(__qspiflash_data_start = _sqspiflash_data);
		*(.qspiflash_data)
		*(.qspiflash_data*)
		. = ALIGN(4);
		_eqspiflash_data = .;

		PROVIDE(__qspiflash_data_end = _eqspiflash_data);
	} > QSPIFLASH

	.qspiflash_bss (NOLOAD) :
	{
		. = ALIGN(4);
		_sqspiflash_bss = .;

		PROVIDE(__qspiflash_bss_start = _sqspiflash_bss);
		*(.qspiflash_bss)
		*(.qspiflash_bss*)
		. = ALIGN(4);
		_eqspiflash_bss = .;

		PROVIDE(__qspiflash_bss_end = _eqspiflash_bss);
	} > QSPIFLASH
*/
/*
  	.patchram (NOLOAD) :
	{
    	PROVIDE (_PATCHRAM = .);
  	} >PATCHRAM
  	_PATCHRAM_END = ORIGIN(PATCHRAM) + LENGTH(PATCHRAM);
*/
/*
    .bootrom_storage (NOLOAD) :
    {
        PROVIDE (_BOOTROM_STORAGE_BEGIN = .);
  	} >BOOTROM
    _BOOTROM_STORAGE_END = ORIGIN(BOOTROM) + LENGTH(BOOTROM);
    _BOOTROM_STORAGE_SIZE = LENGTH(BOOTROM);
*/
    .firmware_storage (NOLOAD) :
    {
    . = ALIGN(4);
        PROVIDE (_FIRMWARE_STORAGE_BEGIN = .);
    } >APPLICATION
    _FIRMWARE_STORAGE_END = ORIGIN(APPLICATION) + LENGTH(APPLICATION);
    _FIRMWARE_STORAGE_SIZE = LENGTH(APPLICATION);

   .patch_storage (NOLOAD) :
    {
	. = ALIGN(4);
    PROVIDE (_PATCH_STORAGE_BEGIN = .);
    } >STORAGE
    _PATCH_STORAGE_END = ORIGIN(STORAGE) + LENGTH(STORAGE);
    _PATCH_STORAGE_SIZE = LENGTH(STORAGE);

    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
