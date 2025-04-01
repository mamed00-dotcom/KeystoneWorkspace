ENTRY(_start)

SECTIONS {
  .text : { *(.text*) }
  .data : { *(.data*) }
  .bss  : { *(.bss*) *(COMMON) }

  /* optional: a small heap if you do malloc 
  .malloc (NOLOAD) : ALIGN(8) {
    __malloc_start = .;
    . = . + 0x10000;
    __malloc_end = .;
  }
  */

  . = ALIGN(0x1000);
  _end = .;
}

