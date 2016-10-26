# Copyright (C) 2014-2016 Free Software Foundation, Inc.
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

cat <<EOF
/* Copyright (C) 2014-2016 Free Software Foundation, Inc.

   Copying and distribution of this script, with or without modification,
   are permitted in any medium without royalty provided the copyright
   notice and this notice are preserved.  */

OUTPUT_FORMAT("${OUTPUT_FORMAT}","${OUTPUT_FORMAT}","${OUTPUT_FORMAT}")
OUTPUT_ARCH(${ARCH})

__TEXT_REGION_LENGTH__ = DEFINED(__TEXT_REGION_LENGTH__) ? __TEXT_REGION_LENGTH__ : $TEXT_LENGTH;
__DATA_REGION_LENGTH__ = DEFINED(__DATA_REGION_LENGTH__) ? __DATA_REGION_LENGTH__ : $DATA_LENGTH;
__FUSE_REGION_LENGTH__ = DEFINED(__FUSE_REGION_LENGTH__) ? __FUSE_REGION_LENGTH__ : 2;
__LOCK_REGION_LENGTH__ = DEFINED(__LOCK_REGION_LENGTH__) ? __LOCK_REGION_LENGTH__ : 2;
__SIGNATURE_REGION_LENGTH__ = DEFINED(__SIGNATURE_REGION_LENGTH__) ? __SIGNATURE_REGION_LENGTH__ : 4;

MEMORY
{
  text   (rx)   : ORIGIN = $TEXT_ORIGIN, LENGTH = __TEXT_REGION_LENGTH__
  data   (rw!x) : ORIGIN = $DATA_ORIGIN, LENGTH = __DATA_REGION_LENGTH__

  /* Provide offsets for config, lock and signature to match
     production file format. Ignore offsets in datasheet.  */

  config    (rw!x) : ORIGIN = 0x820000, LENGTH = __FUSE_REGION_LENGTH__
  lock      (rw!x) : ORIGIN = 0x830000, LENGTH = __LOCK_REGION_LENGTH__
  signature (rw!x) : ORIGIN = 0x840000, LENGTH = __SIGNATURE_REGION_LENGTH__
}

SECTIONS
{
  /* Read-only sections, merged into text segment: */
  ${TEXT_DYNAMIC+${DYNAMIC}}
  .hash        ${RELOCATING-0} : { *(.hash)		}
  .dynsym      ${RELOCATING-0} : { *(.dynsym)		}
  .dynstr      ${RELOCATING-0} : { *(.dynstr)		}
  .gnu.version ${RELOCATING-0} : { *(.gnu.version)	}
  .gnu.version_d ${RELOCATING-0} : { *(.gnu.version_d)	}
  .gnu.version_r ${RELOCATING-0} : { *(.gnu.version_r)	}

  .rel.init    ${RELOCATING-0} : { *(.rel.init)	}
  .rela.init   ${RELOCATING-0} : { *(.rela.init)	}
  .rel.text    ${RELOCATING-0} :
    {
      *(.rel.text)
      ${RELOCATING+*(.rel.text.*)}
      ${RELOCATING+*(.rel.gnu.linkonce.t*)}
    }
  .rela.text   ${RELOCATING-0} :
    {
      *(.rela.text)
      ${RELOCATING+*(.rela.text.*)}
      ${RELOCATING+*(.rela.gnu.linkonce.t*)}
    }
  .rel.fini    ${RELOCATING-0} : { *(.rel.fini)	}
  .rela.fini   ${RELOCATING-0} : { *(.rela.fini)	}
  .rel.rodata  ${RELOCATING-0} :
    {
      *(.rel.rodata)
      ${RELOCATING+*(.rel.rodata.*)}
      ${RELOCATING+*(.rel.gnu.linkonce.r*)}
    }
  .rela.rodata ${RELOCATING-0} :
    {
      *(.rela.rodata)
      ${RELOCATING+*(.rela.rodata.*)}
      ${RELOCATING+*(.rela.gnu.linkonce.r*)}
    }
  .rel.data    ${RELOCATING-0} :
    {
      *(.rel.data)
      ${RELOCATING+*(.rel.data.*)}
      ${RELOCATING+*(.rel.gnu.linkonce.d*)}
    }
  .rela.data   ${RELOCATING-0} :
    {
      *(.rela.data)
      ${RELOCATING+*(.rela.data.*)}
      ${RELOCATING+*(.rela.gnu.linkonce.d*)}
    }
  .rel.ctors   ${RELOCATING-0} : { *(.rel.ctors)	}
  .rela.ctors  ${RELOCATING-0} : { *(.rela.ctors)	}
  .rel.dtors   ${RELOCATING-0} : { *(.rel.dtors)	}
  .rela.dtors  ${RELOCATING-0} : { *(.rela.dtors)	}
  .rel.got     ${RELOCATING-0} : { *(.rel.got)		}
  .rela.got    ${RELOCATING-0} : { *(.rela.got)	}
  .rel.bss     ${RELOCATING-0} : { *(.rel.bss)		}
  .rela.bss    ${RELOCATING-0} : { *(.rela.bss)	}
  .rel.plt     ${RELOCATING-0} : { *(.rel.plt)		}
  .rela.plt    ${RELOCATING-0} : { *(.rela.plt)	}

  /* Internal text space or external memory.  */
  .text ${RELOCATING-0} : 
  {
    *(.vectors)
    KEEP(*(.vectors))

    /* For data that needs to reside in the lower 64k of progmem.  */
    ${RELOCATING+ *(.progmem.gcc*)}

    /* PR 13812: Placing the trampolines here gives a better chance
       that they will be in range of the code that uses them.  */
    ${RELOCATING+. = ALIGN(2);}
    ${CONSTRUCTING+ __trampolines_start = . ; }
    /* The jump trampolines for the 16-bit limited relocs will reside here.  */
    *(.trampolines)
    ${RELOCATING+ *(.trampolines*)}
    ${CONSTRUCTING+ __trampolines_end = . ; }

    /* avr-libc expects these data to reside in lower 64K. */
    ${RELOCATING+ *libprintf_flt.a:*(.progmem.data)}
    ${RELOCATING+ *libc.a:*(.progmem.data)}

    ${RELOCATING+ *(.progmem*)}

    ${RELOCATING+. = ALIGN(2);}

    /* For future tablejump instruction arrays for 3 byte pc devices.
       We don't relax jump/call instructions within these sections.  */
    *(.jumptables) 
    ${RELOCATING+ *(.jumptables*)}

    /* For code that needs to reside in the lower 128k progmem.  */
    *(.lowtext)
    ${RELOCATING+ *(.lowtext*)}

    ${CONSTRUCTING+ __ctors_start = . ; }
    ${CONSTRUCTING+ *(.ctors) }
    ${CONSTRUCTING+ __ctors_end = . ; }
    ${CONSTRUCTING+ __dtors_start = . ; }
    ${CONSTRUCTING+ *(.dtors) }
    ${CONSTRUCTING+ __dtors_end = . ; }
    KEEP(SORT(*)(.ctors))
    KEEP(SORT(*)(.dtors))

    /* From this point on, we don't bother about wether the insns are
       below or above the 16 bits boundary.  */
    *(.init0)  /* Start here after reset.  */
    KEEP (*(.init0))
    *(.init1)
    KEEP (*(.init1))
    *(.init2)  /* Clear __zero_reg__, set up stack pointer.  */
    KEEP (*(.init2))
    *(.init3)
    KEEP (*(.init3))
    *(.init4)  /* Initialize data and BSS.  */
    KEEP (*(.init4))
    *(.init5)
    KEEP (*(.init5))
    *(.init6)  /* C++ constructors.  */
    KEEP (*(.init6))
    *(.init7)
    KEEP (*(.init7))
    *(.init8)
    KEEP (*(.init8))
    *(.init9)  /* Call main().  */
    KEEP (*(.init9))
    *(.text)
    ${RELOCATING+. = ALIGN(2);}
    ${RELOCATING+ *(.text.*)}
    ${RELOCATING+. = ALIGN(2);}
    *(.fini9)  /* _exit() starts here.  */
    KEEP (*(.fini9))
    *(.fini8)
    KEEP (*(.fini8))
    *(.fini7)
    KEEP (*(.fini7))
    *(.fini6)  /* C++ destructors.  */
    KEEP (*(.fini6))
    *(.fini5)
    KEEP (*(.fini5))
    *(.fini4)
    KEEP (*(.fini4))
    *(.fini3)
    KEEP (*(.fini3))
    *(.fini2)
    KEEP (*(.fini2))
    *(.fini1)
    KEEP (*(.fini1))
    *(.fini0)  /* Infinite loop after program termination.  */
    KEEP (*(.fini0))
    ${RELOCATING+ _etext = . ; }
  } ${RELOCATING+ > text}

  .data        ${RELOCATING-0} :
  {
    ${RELOCATING+ PROVIDE (__data_start = .) ; }
    *(.data)
    ${RELOCATING+ *(.data*)}
    *(.rodata)  /* We need to include .rodata here if gcc is used */
    ${RELOCATING+ *(.rodata*)} /* with -fdata-sections.  */
    *(.gnu.linkonce.d*)
    ${RELOCATING+. = ALIGN(2);}
    ${RELOCATING+ _edata = . ; }
    ${RELOCATING+ PROVIDE (__data_end = .) ; }
  } ${RELOCATING+ > data ${RELOCATING+AT> text}}

  .bss ${RELOCATING+ ADDR(.data) + SIZEOF (.data)} ${RELOCATING-0} :${RELOCATING+ AT (ADDR (.bss))}
  {
    ${RELOCATING+ PROVIDE (__bss_start = .) ; }
    *(.bss)
    ${RELOCATING+ *(.bss*)}
    *(COMMON)
    ${RELOCATING+ PROVIDE (__bss_end = .) ; }
  } ${RELOCATING+ > data}

  ${RELOCATING+ __data_load_start = LOADADDR(.data); }
  ${RELOCATING+ __data_load_end = __data_load_start + SIZEOF(.data); }

  /* Global data not cleared after reset.  */
  .noinit ${RELOCATING+ ADDR(.bss) + SIZEOF (.bss)} ${RELOCATING-0} : ${RELOCATING+ AT (ADDR (.noinit))}
  {
    ${RELOCATING+ PROVIDE (__noinit_start = .) ; }
    *(.noinit*)
    ${RELOCATING+ PROVIDE (__noinit_end = .) ; }
    ${RELOCATING+ _end = . ;  }
    ${RELOCATING+ PROVIDE (__heap_start = .) ; }
  } ${RELOCATING+ > data}

  .lock ${RELOCATING-0}:
  {
    KEEP(*(.lock*))
  } ${RELOCATING+ > lock}

  .signature ${RELOCATING-0}:
  {
    KEEP(*(.signature*))
  } ${RELOCATING+ > signature}

  .config ${RELOCATING-0}:
  {
    KEEP(*(.config*))
  } ${RELOCATING+ > config}

  /* Stabs debugging sections.  */
  .stab 0 : { *(.stab) }
  .stabstr 0 : { *(.stabstr) }
  .stab.excl 0 : { *(.stab.excl) }
  .stab.exclstr 0 : { *(.stab.exclstr) }
  .stab.index 0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment 0 : { *(.comment) }
  .note.gnu.build-id : { *(.note.gnu.build-id) }
EOF

. $srcdir/scripttempl/DWARF.sc

cat <<EOF
}
EOF
