/*
 This script is used when Daisy boots from QSPI flash. It must be initialized by bootloader in advance.
*/

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
/* _estack = 0x20020000; */   /* end of RAM */
/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x000;      /* required amount of heap  */
_Min_Stack_Size = 0x2000; /* required amount of stack */

/* Specify the memory areas */
MEMORY
{
ITCMRAM (rx)      : ORIGIN = 0x00000000, LENGTH = 64K
DTCMRAM (rw)      : ORIGIN = 0x20000000, LENGTH = 64K /* First half of DTCM is used as FW RAM */
DTCMHEAP (rw)     : ORIGIN = 0x20010000, LENGTH = 64K /* Second half of DTCM is used as fast heap for patches */
/* Second half of DTCMRAM could be fast heap? */
PATCHRAM (rwx)    : ORIGIN = 0x24000000, LENGTH = 512K
RAM (rwx)         : ORIGIN = 0x30000000, LENGTH = 256K /* Firmware code, copied by bootloader */
/* CARDBUF (rwx)     : ORIGIN = 0x30030000, LENGTH = 64K *//* Buffer for card streaming */
BUFFERS (rwx)     : ORIGIN = 0x30040000, LENGTH = 32K /* DMA buffer */
RAM_D3 (rwx)      : ORIGIN = 0x38000000, LENGTH = 64K /* Fast LOG table? */
EXTRAM (rwx)      : ORIGIN = 0xc0000000, LENGTH = 32M
FIRMWARE (r)      : ORIGIN = 0x08000000, LENGTH = 512K  /* Flash - firmware storage part. It would be used as bootrom and storage for builtin LUTs */
STORAGE (rx)      : ORIGIN = 0x08080000, LENGTH = 1536K  /* Flash - patch storage part. That's one hell of a lot of patches */
}
/* Device flash is 2M, split it between FW and patch/settings storage
*/


SECTIONS
{
	.firmware_header :
	{
		. = ALIGN(4);
		KEEP(*(.firmware_header))
	} > FIRMWARE

	/*
	 * ITCM memory used for faster code execution. Should be used by IRQ handlers and other time-critical
	 * tasks. Some frequently used data can also go here if necessary to improve cache performance.
	 */
	_siitcm = LOADADDR(.isr_vector);
	.isr_vector :
	{
		. = ALIGN(4);
		_sitcm = .;
    	PROVIDE ( _ISR_VECTOR = . );
		KEEP(*(.isr_vector))
	} > ITCMRAM AT > FIRMWARE

	.itcm : 
	{
		. = ALIGN(4);
		*(.itcm)
		*(.itcm*)
		*/Owl.o (.text .text*)
		*/Codec.o (.text .text*)
		*/usbd*.o (.text .text*)
		*/uart*.o (.text .text*)
		*/ServiceCalls.o (.text .text*)
		*/ProgramManager.o (.text .text*)
		*/Midi*.o (.text .text*)
		*/stm32h7xx_it.o (.text .text*)
		_eitcm = .;
	} > ITCMRAM AT > FIRMWARE

	/*
	 * Code (text and most constant data) will be copied to RAM in advance by bootloader
	 */

	_sicode = LOADADDR(.text);
	.text :
	{
		. = ALIGN(4);
		_scode = .;

		*(.text)
		*(.text*)
		*(.glue_7)
		*(.glue_7t)
	    *(.eh_frame)
		
		KEEP(*(.init))
		KEEP(*(.fini))
		. = ALIGN(4);

	} > RAM AT > FIRMWARE

  	.rodata :
  	{
    	. = ALIGN(4);
     	*(.rodata)         /* .rodata sections (constants, strings, etc.) */
    	*(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    	. = ALIGN(4);
  	} > RAM AT > FIRMWARE
 
 	.ARM.extab : {
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > RAM AT > FIRMWARE

  	.ARM : {
    	__exidx_start = .;
    	*(.ARM.exidx*)
    	__exidx_end = .;
  	} > RAM AT > FIRMWARE

	.preinit_array :
	{
		PROVIDE_HIDDEN(__preinit_array_start = .);
		KEEP(*(.preinit_array*))
		PROVIDE_HIDDEN(__preinit_array_end = .);
	} > RAM AT > FIRMWARE

	.init_array :
	{
		PROVIDE_HIDDEN(__init_array_start = .);
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array*))
		PROVIDE_HIDDEN(__init_array_end = .);
	} > RAM AT > FIRMWARE

	.fini_array :
	{
		PROVIDE_HIDDEN(__fini_array_start = .);
		KEEP(*(.fini_array*))
		KEEP(*(SORT(.fini_array.*)))
		PROVIDE_HIDDEN(__fini_array_end = .);
		_ecode = .;
	} > RAM AT > FIRMWARE

	/*
	 * Tables section won't be relocated from flash
	 */

	.tables :
	{
		. = ALIGN(4);
		*(.tables)
		*(.tables*)
	} > FIRMWARE


	/*
	 * Data, BSS and stack will be stored in first half of DTCM section
	 */

	_sidata = LOADADDR(.data);
	.data :
	{
		. = ALIGN(4);
		_sdata = .;

		PROVIDE(__data_start = _sdata);
		*(.data)
		*(.data*)
		. = ALIGN(4);
		_edata = .;

		PROVIDE(__data_end = _edata);
 	} > DTCMRAM AT > FIRMWARE

	.bss (NOLOAD):
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
	} > DTCMRAM


	/* User_heap_stack section, used to check that there is enough RAM left */
  	._user_heap_stack :
  	{
    	. = ALIGN(4);
	    PROVIDE ( _end = . );
		PROVIDE(end = .); 
    	. = . + _Min_Heap_Size;
	    . = . + _Min_Stack_Size;
    	. = ALIGN(4);
  	} > DTCMRAM
  	_estack = ORIGIN(DTCMRAM) + LENGTH(DTCMRAM); /* end of firmware RAM */


	.dtcmheap (NOLOAD):
	{
		. = ALIGN(4);
		_DTCMHEAP_BEGIN = .;

		PROVIDE(__bss_start__ = _sbss);
		*(.dtcmheap)
		*(.dtcmheap*)

	} > DTCMHEAP
	_DTCMHEAP_SIZE = LENGTH(DTCMHEAP);
	_DTCMHEAP_END = ORIGIN(DTCMHEAP) + LENGTH(DTCMHEAP);

  	/* External memory, vars must be located here explicitly */
  	/* Example: int foo __attribute__ ((section (".extdata"))); */
  	.extdata (NOLOAD) :
  	{
	    /* *(.extdata) */
    	/* . = ALIGN(8); */
    	PROVIDE ( _EXTRAM = . );
    } >EXTRAM
    _EXTRAM_END = ORIGIN(EXTRAM) + LENGTH(EXTRAM);
	_EXTRAM_SIZE = LENGTH(EXTRAM);

  	.patchram (NOLOAD) :
  	{
    	PROVIDE (_PATCHRAM = .);
  	} >PATCHRAM
  	_PATCHRAM_END = ORIGIN(PATCHRAM) + LENGTH(PATCHRAM);
  	_PATCHRAM_SIZE = LENGTH(PATCHRAM);	


	.firmware_image (NOLOAD) :
	{
		. = ALIGN(4);
	    PROVIDE (_FIRMWARE_STORAGE_BEGIN = .);
  	} > FIRMWARE
  	_FIRMWARE_STORAGE_END = ORIGIN(FIRMWARE) + LENGTH(FIRMWARE);
    _FIRMWARE_STORAGE_SIZE = LENGTH(FIRMWARE);

	.patch_storage (NOLOAD) :
	{
		. = ALIGN(4);
	    PROVIDE (_FLASH_STORAGE_BEGIN = .);
  	} >STORAGE
  	_FLASH_STORAGE_END = ORIGIN(STORAGE) + LENGTH(STORAGE);
    _FLASH_STORAGE_SIZE = LENGTH(STORAGE);

	/* Section for DMA buffers with caching disabled by MPU */
	/* _sinocache = LOADADDR(.nocache); */
	.nocache (NOLOAD) :
	{
		. = ALIGN(4);
		PROVIDE ( _BUFFERS_BEGIN = .);
		_snocache = .;
		*(.nocache)
		*(.nocache*)		
		_enocache = .;
	} > BUFFERS
	_BUFFERS_END = ORIGIN(BUFFERS) + LENGTH(BUFFERS);
	_BUFFERS_SIZE = LENGTH(BUFFERS);

	/*
	 * Fast log table. Note that this ram is slower than others, maybe we should use D2 if we
	 * can make some space there.
	 */
	/*
	_silogtable = LOADADDR(.log_table);
	.log_table : 
	{
		. = ALIGN(4);
		_slogtable = .;
		*(.log_table)
		*(.log_table*)		
		_elogtable = .;
		PROVIDE ( _LOG_BEGIN = .);
	} > RAM_D3 AT > FIRMWARE
	*/

    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }

	.ARM.attributes 0 : { *(.ARM.attributes) }
	
}
