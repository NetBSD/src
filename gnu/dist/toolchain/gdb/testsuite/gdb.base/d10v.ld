OUTPUT_FORMAT("elf32-d10v", "elf32-d10v",
	      "elf32-d10v")
OUTPUT_ARCH(d10v)
ENTRY(_start)
 SEARCH_DIR(/usr/cygnus/d10v-961230/H-sparc-sun-sunos4.1//lib);
/* Do we need any of these for elf?
   __DYNAMIC = 0;    */
SECTIONS
{
  /* Overlay sections: */
  .ovly0  0x01010000 : AT (0x12010000) { foo.o(.text)  }
  .ovly1  0x01010000 : AT (0x12011000) { bar.o(.text)  }
  .ovly2  0x01011000 : AT (0x12012000) { baz.o(.text)  }
  .ovly3  0x01011000 : AT (0x12013000) { grbx.o(.text) }
  .data00 0x00001000 : AT (0x12014000) { foo.o(.data)  }
  .data01 0x00001000 : AT (0x12015000) { bar.o(.data)  }
  .data02 0x00002000 : AT (0x12016000) { baz.o(.data)  }
  .data03 0x00002000 : AT (0x12017000) { grbx.o(.data) }
  /* Read-only sections, merged into data segment: */
  . = 0x00000004;
  .interp     : { *(.interp) 	}
  .hash          : { *(.hash)		}
  .dynsym        : { *(.dynsym)		}
  .dynstr        : { *(.dynstr)		}
  .rel.text      : { *(.rel.text)		}
  .rela.text     : { *(.rela.text) 	}
  .rel.data      : { *(.rel.data)		}
  .rela.data     : { *(.rela.data) 	}
  .rel.rodata    : { *(.rel.rodata) 	}
  .rela.rodata   : { *(.rela.rodata) 	}
  .rel.got       : { *(.rel.got)		}
  .rela.got      : { *(.rela.got)		}
  .rel.ctors     : { *(.rel.ctors)	}
  .rela.ctors    : { *(.rela.ctors)	}
  .rel.dtors     : { *(.rel.dtors)	}
  .rela.dtors    : { *(.rela.dtors)	}
  .rel.init      : { *(.rel.init)	}
  .rela.init     : { *(.rela.init)	}
  .rel.fini      : { *(.rel.fini)	}
  .rela.fini     : { *(.rela.fini)	}
  .rel.bss       : { *(.rel.bss)		}
  .rela.bss      : { *(.rela.bss)		}
  .rel.plt       : { *(.rel.plt)		}
  .rela.plt      : { *(.rela.plt)		}
  .plt      : { *(.plt)	}
  .rodata    : { *(.rodata) *(.gnu.linkonce.r*) }
  .rodata1   : { *(.rodata1) }
  /* Adjust the address for the data segment.  */
  . = ALIGN(4);
  .data    :
  {
    *(.data)
    *(.gnu.linkonce.d*)
    _ovly_table = .; 
	LONG(ABSOLUTE(ADDR(.ovly0)));
	LONG(SIZEOF(.ovly0));
	LONG(LOADADDR(.ovly0));
	LONG(0);
	LONG(ABSOLUTE(ADDR(.ovly1)));
	LONG(SIZEOF(.ovly1));
	LONG(LOADADDR(.ovly1));
	LONG(0);
	LONG(ABSOLUTE(ADDR(.ovly2)));
	LONG(SIZEOF(.ovly2));
	LONG(LOADADDR(.ovly2));
	LONG(0);
	LONG(ABSOLUTE(ADDR(.ovly3)));
	LONG(SIZEOF(.ovly3));
	LONG(LOADADDR(.ovly3));
	LONG(0);
	LONG(ABSOLUTE(ADDR(.data00)));
	LONG(SIZEOF(.data00));
	LONG(LOADADDR(.data00));
	LONG(0);
	LONG(ABSOLUTE(ADDR(.data01)));
	LONG(SIZEOF(.data01));
	LONG(LOADADDR(.data01));
	LONG(0);
	LONG(ABSOLUTE(ADDR(.data02)));
	LONG(SIZEOF(.data02));
	LONG(LOADADDR(.data02));
	LONG(0);
	LONG(ABSOLUTE(ADDR(.data03)));
	LONG(SIZEOF(.data03));
	LONG(LOADADDR(.data03));
	LONG(0);
    _novlys = .;
	LONG((_novlys - _ovly_table) / 16);
    CONSTRUCTORS
  }
  .data1   : { *(.data1) }
  .ctors         :
  {
    *(.ctors)
  }
  .dtors         :
  {
    *(.dtors)
  }
  .got           : { *(.got.plt) *(.got) }
  .dynamic       : { *(.dynamic) }
  /* We want the small data sections together, so single-instruction offsets
     can access them all, and initialized data all before uninitialized, so
     we can shorten the on-disk segment size.  */
  .sdata     : { *(.sdata) }
  _edata  =  .;
  PROVIDE (edata = .);
  __bss_start = .;
  .sbss      : { *(.sbss) *(.scommon) }
  .bss       :
  {
   *(.dynbss)
   *(.bss)
   *(COMMON)
  }
  _end = . ;
  PROVIDE (end = .);
  /* Stabs debugging sections.  */
  .stab 0 : { *(.stab) }
  .stabstr 0 : { *(.stabstr) }
  .stab.excl 0 : { *(.stab.excl) }
  .stab.exclstr 0 : { *(.stab.exclstr) }
  .stab.index 0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment 0 : { *(.comment) }
  /* DWARF debug sections.
     Symbols in the .debug DWARF section are relative to the beginning of the
     section so we begin .debug at 0.  It's not clear yet what needs to happen
     for the others.   */
  .debug          0 : { *(.debug) }
  .debug_info     0 : { *(.debug_info) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  .line           0 : { *(.line) }
  /* These must appear regardless of  .  */
  /* Hmmm, there's got to be a better way.  This sets the stack to the
     top of the simulator memory (i.e. top of 64K data space). */
  .stack 0x00007FFE : { _stack = .; *(.stack) }
  .text    0x1000000 :
  {
    *(.init)
    *(.fini)
    *(.text)
    /* .gnu.warning sections are handled specially by elf32.em.  */
    *(.gnu.warning)
    *(.gnu.linkonce.t*)
  } =0
  _etext = .;
  PROVIDE (etext = .);
}
