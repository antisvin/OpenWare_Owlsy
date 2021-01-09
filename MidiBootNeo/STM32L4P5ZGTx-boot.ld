/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
/*_estack = 0x20050000;*/    /* end of RAM */
/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x000;      /* required amount of heap  */
_Min_Stack_Size = 0x200; /* required amount of stack */

/* Specify the memory areas */
MEMORY
{
RAM (rwx)        : ORIGIN = 0x20000000, LENGTH = 48K
SYSRAM (rwx)     : ORIGIN = 0x2000c000, LENGTH = 16K /* Used only to set magic word for bootloader in the end - we could change the address to increas firmware size a bit */
PATCHRAM (rwx)   : ORIGIN = 0x2000c000, LENGTH = 256K /* Total RAM is 320kb */
FLASH (rx)       : ORIGIN = 0x08000000, LENGTH = 32K
APPLICATION (rx) : ORIGIN = 0x08008000, LENGTH = 256K
STORAGE (rw)     : ORIGIN = 0x08048000, LENGTH = 736K
}

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >FLASH

  /* The program code and other data goes into FLASH */
  .text :
  {
    . = ALIGN(4);
		_stext = .;
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
  } >FLASH

  /* Constant data goes into FLASH */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } >FLASH

  .ARM.extab   : 
  { 
  . = ALIGN(4);
  *(.ARM.extab* .gnu.linkonce.armextab.*)
  . = ALIGN(4);
  } >FLASH
  .ARM : {
	. = ALIGN(4);
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
	. = ALIGN(4);
  } >FLASH
	.ARM.attributes :
	{
		*(.ARM.attributes)
	} > FLASH

  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH
  
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH
  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

  /* used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data : 
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >RAM AT> FLASH

  
  /* Uninitialized data section */
  . = ALIGN(4);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss secion */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM

  /* User_heap_stack section, used to check that there is enough RAM left */
  ._user_heap_stack :
  {
    . = ALIGN(4);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(4);
  } >RAM
  	_estack = ORIGIN(RAM)+LENGTH(RAM); /* end of firmware RAM */

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

   .patchram (NOLOAD) :
    {
        . = ALIGN(4);
    PROVIDE (_PATCHRAM = .);
    } >PATCHRAM


  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}


