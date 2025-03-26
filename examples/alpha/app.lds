SECTIONS {
  .text : {
    *(.text*)
  }

  .data : {
    *(.data*)
  }

  .bss : {
    *(.bss*)
    *(COMMON)
  }

  /* Heap section for malloc */
  .malloc (NOLOAD) : ALIGN(8) {
    __malloc_start = .;
    . = . + 0x10000;  /* Reserve 64 KB heap */
    __malloc_end = .;
  }

  /* Stack space (optional, but good to align and reserve if needed) */
  . = ALIGN(0x1000);
  _end = .;
}

