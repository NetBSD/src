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

MEMORY
{
  text   (rx)   : ORIGIN = $ROM_START,  LENGTH = $ROM_SIZE
  data   (rwx)  : ORIGIN = $RAM_START, 	LENGTH = $RAM_SIZE
  vectors (rw)  : ORIGIN = 0xffe0,      LENGTH = 0x20
}

SECTIONS
{
  /* Read-only sections, merged into text segment.  */
  ${TEXT_DYNAMIC+${DYNAMIC}}
  .hash        ${RELOCATING-0} : { *(.hash)             }
  .dynsym      ${RELOCATING-0} : { *(.dynsym)           }
  .dynstr      ${RELOCATING-0} : { *(.dynstr)           }
  .gnu.version ${RELOCATING-0} : { *(.gnu.version)      }
  .gnu.version_d ${RELOCATING-0} : { *(.gnu.version_d)  }
  .gnu.version_r ${RELOCATING-0} : { *(.gnu.version_r)  }

  .rel.init    ${RELOCATING-0} : { *(.rel.init) }
  .rela.init   ${RELOCATING-0} : { *(.rela.init)        }
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
  .rel.fini    ${RELOCATING-0} : { *(.rel.fini) }
  .rela.fini   ${RELOCATING-0} : { *(.rela.fini)        }
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
  .rel.ctors   ${RELOCATING-0} : { *(.rel.ctors)        }
  .rela.ctors  ${RELOCATING-0} : { *(.rela.ctors)       }
  .rel.dtors   ${RELOCATING-0} : { *(.rel.dtors)        }
  .rela.dtors  ${RELOCATING-0} : { *(.rela.dtors)       }
  .rel.got     ${RELOCATING-0} : { *(.rel.got)          }
  .rela.got    ${RELOCATING-0} : { *(.rela.got)         }
  .rel.bss     ${RELOCATING-0} : { *(.rel.bss)          }
  .rela.bss    ${RELOCATING-0} : { *(.rela.bss)         }
  .rel.plt     ${RELOCATING-0} : { *(.rel.plt)          }
  .rela.plt    ${RELOCATING-0} : { *(.rela.plt)         }

  /* Internal text space.  */
  .text :
  {
    ${RELOCATING+. = ALIGN(2);}
    *(SORT_NONE(.init))
    *(SORT_NONE(.init0))  /* Start here after reset.  */
    *(SORT_NONE(.init1))
    *(SORT_NONE(.init2))
    *(SORT_NONE(.init3))
    *(SORT_NONE(.init4))
    *(SORT_NONE(.init5))
    *(SORT_NONE(.init6)) /* C++ constructors.  */
    *(SORT_NONE(.init7))
    *(SORT_NONE(.init8))
    *(SORT_NONE(.init9))  /* Call main().  */

    ${CONSTRUCTING+ __ctors_start = . ; }
    ${CONSTRUCTING+ *(.ctors) }
    ${CONSTRUCTING+ __ctors_end = . ; }
    ${CONSTRUCTING+ __dtors_start = . ; }
    ${CONSTRUCTING+ *(.dtors) }
    ${CONSTRUCTING+ __dtors_end = . ; }

    ${RELOCATING+. = ALIGN(2);}
    *(.text)
    ${RELOCATING+. = ALIGN(2);}
    *(.text.*)
    ${RELOCATING+. = ALIGN(2);}
    *(.text:*)

    ${RELOCATING+. = ALIGN(2);}
    *(SORT_NONE(.fini9))
    *(SORT_NONE(.fini8))
    *(SORT_NONE(.fini7))
    *(SORT_NONE(.fini6))  /* C++ destructors.  */
    *(SORT_NONE(.fini5))
    *(SORT_NONE(.fini4))
    *(SORT_NONE(.fini3))
    *(SORT_NONE(.fini2))
    *(SORT_NONE(.fini1))
    *(SORT_NONE(.fini0))  /* Infinite loop after program termination.  */
    *(SORT_NONE(.fini))

    ${RELOCATING+ _etext = . ; }
  } ${RELOCATING+ > text}

  .rodata :
  {
    *(.rodata .rodata.* .gnu.linkonce.r.*)
    *(.const)
    *(.const:*)
  } ${RELOCATING+ > text}

  .data ${RELOCATING-0} :
  {  
    ${RELOCATING+ PROVIDE (__data_start = .) ; }
    ${RELOCATING+. = ALIGN(2);}
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d*)
    ${RELOCATING+. = ALIGN(2);}
    ${RELOCATING+ _edata = . ; }
  } ${RELOCATING+ > data ${RELOCATING+AT> text}}
  
  __romdatastart = LOADADDR(.data);
  __romdatacopysize = SIZEOF(.data);
  
  .bss ${RELOCATING+ SIZEOF(.data) + ADDR(.data)} :
  {
    ${RELOCATING+. = ALIGN(2);}
    ${RELOCATING+ PROVIDE (__bss_start = .) ; }
    ${RELOCATING+ PROVIDE (__bssstart = .); }
    *(.bss)
    *(COMMON)
    ${RELOCATING+ PROVIDE (__bss_end = .) ; }
  } ${RELOCATING+ > data}
  ${RELOCATING+ PROVIDE (__bsssize = SIZEOF(.bss)); }

  .noinit ${RELOCATING+ SIZEOF(.bss) + ADDR(.bss)} :
  {
    ${RELOCATING+ PROVIDE (__noinit_start = .) ; }
    *(.noinit)
    *(COMMON)
    ${RELOCATING+ PROVIDE (__noinit_end = .) ; }
  } ${RELOCATING+ > data}

  .persistent ${RELOCATING+ SIZEOF(.noinit) + ADDR(.noinit)} :
  {
    ${RELOCATING+ PROVIDE (__persistent_start = .) ; }
    *(.persistent)
    ${RELOCATING+ PROVIDE (__persistent_end = .) ; }
  } ${RELOCATING+ > data}

  ${RELOCATING+ _end = . ;}

  .vectors ${RELOCATING-0}:
  {
    ${RELOCATING+ PROVIDE (__vectors_start = .) ; }
    *(.vectors*)
    ${RELOCATING+ _vectors_end = . ; }
  } ${RELOCATING+ > vectors}

  .MSP430.attributes 0 :
  {
    KEEP (*(.MSP430.attributes))
    KEEP (*(.gnu.attributes))
    KEEP (*(__TI_build_attributes))
  }

  /* Stabs debugging sections.  */
  .stab 0 : { *(.stab) } 
  .stabstr 0 : { *(.stabstr) }
  .stab.excl 0 : { *(.stab.excl) }
  .stab.exclstr 0 : { *(.stab.exclstr) }
  .stab.index 0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment 0 : { *(.comment) }
 
EOF

. $srcdir/scripttempl/DWARF.sc

cat <<EOF
  PROVIDE (__stack = ${STACK}) ;
  PROVIDE (__data_start_rom = _etext) ;
  PROVIDE (__data_end_rom   = _etext + SIZEOF (.data)) ;
  PROVIDE (__noinit_start_rom = _etext + SIZEOF (.data)) ;
  PROVIDE (__noinit_end_rom = _etext + SIZEOF (.data) + SIZEOF (.noinit)) ;
}
EOF
