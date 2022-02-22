/*
 This script is used when Daisy boots from QSPI flash. It must be initialized by bootloader in advance.
*/

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
/* _estack = 0x20020000; */   /* end of RAM */
/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x000;      /* required amount of heap  */
_Min_Stack_Size = 0x800; /* required amount of stack */

/* Specify the memory areas */
MEMORY
{
ITCMRAM (rx)      : ORIGIN = 0x00000000, LENGTH = 32K /* We split ITCM between 3 sections */
ITCMHEAP (rw)     : ORIGIN = 0x00008000, LENGTH = 28K
PROGRAMSTACK(rw)  : ORIGIN = 0x0000f000, LENGTH = 4K
DTCMRAM (rw)      : ORIGIN = 0x20000000, LENGTH = 48K /* First 48kb of DTCM is used as FW RAM */
PATCHRAM (rw)     : ORIGIN = 0x2000c000, LENGTH = 80K /* Remaining 80kb of DTCM is used as patch RAM */
PLUSRAM (rwx)     : ORIGIN = 0x24000000, LENGTH = 510K
SD_BUF  (rwx)     : ORIGIN = 0x2407f800, LENGTH = 2K
RAM (rwx)         : ORIGIN = 0x30000000, LENGTH = 256K /* Firmware code, copied by bootloader */
BUFFERS (rwx)     : ORIGIN = 0x30040000, LENGTH = 32K /* DMA buffer */
RAM_D3 (rwx)      : ORIGIN = 0x38000000, LENGTH = 64K /* UNUSED */
EXTRAM (rwx)      : ORIGIN = 0xc0000000, LENGTH = 64M
BOOTLOADER (rx)   : ORIGIN = 0x08000000, LENGTH = 128K /* Bootloader on flash */
/* BOOTROM (rx)      : ORIGIN = 0x90000000, LENGTH = 512K */ /* QSPI flash - firmware part. This is used by bootloader only. */
FIRMWARE (r)      : ORIGIN = 0x90000000, LENGTH = 512K  /* QSPI flash - firmware storage part. It would be used as bootrom and storage for builtin LUTs */
STORAGE (rx)      : ORIGIN = 0x90080000, LENGTH = 7680K  /* QSPI flash - patch storage part. That's one hell of a lot of patches */
}
/* Internal flash is 128k and only used for bootloader
 * QSPI flash is 8M, split it between FW and patch/settings storage
 */

/*
 *	NOTE: we can write QSPI FLASH firmware, because we're running entirely from memory
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
    	PROVIDE ( _ISR_VECTOR_END = . );
	} > ITCMRAM AT > FIRMWARE
	_ISR_VECTOR_SIZE = _ISR_VECTOR_END - _ISR_VECTOR;

	.itcm : 
	{
		. = ALIGN(4);
		*(.itcm)
		*(.itcm*)
		*/Owl.o (.text .text*)
		*/Codec.o (.text .text*)
		*/Daisy.o (.text .text*)
		*/usbd*.o (.text .text*)
		*/uart*.o (.text .text*)
		*/ServiceCalls.o (.text .text*)
		*/ProgramManager.o (.text .text*)
		*/ScreenBuffer.o (.text .text*)
		*/Graphics.o (.text .text*)
		*/ssd1309.o (.text .text*)
		*/stm32h7xx_it.o (.text .text*)
		*/stm32h7xx_hal_spi.o (.text .text*)
		_eitcm = .;
	} > ITCMRAM AT > FIRMWARE

	.itcmheap (NOLOAD):
	{
		. = ALIGN(4);
		PROVIDE(_ITCMHEAP = .);
		*(.itcmheap)
		*(.itcmheap*)
	} > ITCMHEAP
	_ITCMHEAP_SIZE = LENGTH(ITCMHEAP);

	.programstack (NOLOAD):
	{
		. = ALIGN(4);
		*(.programstack)
		*(.programstack*)
	} > PROGRAMSTACK

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

	.plusram (NOLOAD):
	{
		. = ALIGN(4);
		PROVIDE(_PLUSRAM = .);

	} > PLUSRAM
	_PLUSRAM_SIZE = LENGTH(PLUSRAM);
	_PLUSRAM_END = ORIGIN(PLUSRAM) + LENGTH(PLUSRAM);

	.sd_buf (NOLOAD):
	{
		. = ALIGN(4);
		PROVIDE(_SD_BUF = .);

		*(.sd_buf)
		*(.sd_buf*)

	} > SD_BUF
	_SD_BUF_SIZE = LENGTH(SD_BUF);
	_SD_BUF_END = ORIGIN(SD_BUF) + LENGTH(SD_BUF);

  	/* Bootloader will be used for finding bootloader token */
  	.bootloader (NOLOAD) :
  	{
	    PROVIDE (_BOOTLOADER = .);
  	} >BOOTLOADER
  	_BOOTLOADER_END = ORIGIN(BOOTLOADER) + LENGTH(BOOTLOADER);	
	_BOOTLOADER_TOKEN = _ISR_VECTOR_END;

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

/* This would be used if we'll want to write FW ROM without bootloader */	

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

/*
	.heap (NOLOAD) :
	{
		. = ALIGN(4);
		PROVIDE(__heap_start__ = .);
		KEEP(*(.heap))
		. = ALIGN(4);
		PROVIDE(__heap_end__ = .);
	} > SRAM
*/
/*
	.reserved_for_stack (NOLOAD) :
	{
		. = ALIGN(4);
		PROVIDE(__reserved_for_stack_start__ = .);
		KEEP(*(.reserved_for_stack))
		. = ALIGN(4);
		PROVIDE(__reserved_for_stack_end__ = .);
	} > SRAM
*/
    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }

	.ARM.attributes 0 : { *(.ARM.attributes) }
	
}
