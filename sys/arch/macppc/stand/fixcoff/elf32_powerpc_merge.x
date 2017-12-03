/*	$NetBSD: elf32_powerpc_merge.x,v 1.2.150.1 2017/12/03 11:36:25 jdolecek Exp $ */
OUTPUT_ARCH(powerpc)
SECTIONS
{
  /* Read-only sections, merged into text segment: */
  . = + SIZEOF_HEADERS;
  .interp : { *(.interp) }
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
  .rel.bss       : { *(.rel.bss)		}
  .rela.bss      : { *(.rela.bss)		}
  .rel.plt       : { *(.rel.plt)		}
  .rela.plt      : { *(.rela.plt)		}
  .init          : { *(.init)	} =0
  .plt : { *(.plt) }
  .text      :
  {
    *(.text)
    *(.rodata)
    *(.rodata.*)
    *(.rodata1)
    *(.got1)
    *(.eh_frame_hdr)
    *(.eh_frame)
  }
  .fini      : { *(.fini)    } =0
  .ctors     : { *(.ctors)   }
  .dtors     : { *(.dtors)   }
  _etext = .;
  PROVIDE (etext = .);
  .pad       : { LONG(0) }
  /* Read-write section, merged into data segment: */
  . = (. + 0x0FFF) & 0xFFFFF000;
  .data    :
  {
    *(.data)
    *(.data1)
    *(.sdata)
    *(.sdata2)
    *(.got.plt) *(.got)
    *(.dynamic)
    CONSTRUCTORS
  }
  . = ALIGN(4);
  _edata  =  .;
  PROVIDE (edata = .);
  __bss_start = .;
  .bss       :
  {
   *(.sbss) *(.scommon)
   *(.dynbss)
   *(.bss)
   *(COMMON)
  }
  _end = . ;
  PROVIDE (end = .);
}

