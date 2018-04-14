cat <<EOF
OUTPUT_FORMAT("${OUTPUT_FORMAT}","${OUTPUT_FORMAT}","${OUTPUT_FORMAT}")
OUTPUT_ARCH(${ARCH})

MEMORY
{
  imem   (x)   : ORIGIN = $TEXT_ORIGIN, LENGTH = $TEXT_LENGTH
  dmem   (rw!x) : ORIGIN = $DATA_ORIGIN, LENGTH = $DATA_LENGTH
}

__HEAP_SIZE = DEFINED(__HEAP_SIZE) ? __HEAP_SIZE : 32;
__STACK_SIZE = DEFINED(__STACK_SIZE) ? __STACK_SIZE : 512;

${RELOCATING+ PROVIDE (_stack_top = ORIGIN(dmem) + LENGTH(dmem)) ; }

${RELOCATING+ENTRY (_start)}

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

  .rel.init    ${RELOCATING-0} : { *(.rel.init)		}
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
  .rel.fini    ${RELOCATING-0} : { *(.rel.fini)		}
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
  .rela.got    ${RELOCATING-0} : { *(.rela.got)		}
  .rel.bss     ${RELOCATING-0} : { *(.rel.bss)		}
  .rela.bss    ${RELOCATING-0} : { *(.rela.bss)		}
  .rel.plt     ${RELOCATING-0} : { *(.rel.plt)		}
  .rela.plt    ${RELOCATING-0} : { *(.rela.plt)		}

  /* Internal text space.  */
  .text ${RELOCATING-0} :
  {
    ${RELOCATING+ _text_start = . ; }

    ${RELOCATING+. = ALIGN(4);}

    ${RELOCATING+*(.init0)  /* Start here after reset.  */}
    ${RELOCATING+KEEP (*(.init0))}

    ${RELOCATING+. = ALIGN(4);}
    *(.text)
    ${RELOCATING+. = ALIGN(4);}
    ${RELOCATING+*(.text.*)}
    ${RELOCATING+. = ALIGN(4);}
    ${RELOCATING+*(.gnu.linkonce.t*)}
    ${RELOCATING+. = ALIGN(4);}

    ${RELOCATING+ _text_end = . ; }
  } ${RELOCATING+ > imem}

  .data        ${RELOCATING-0} :
  {
    /* Optional variable that user is prepared to have NULL address.  */
    ${RELOCATING+ *(.data.atzero*)}

    /* CRT is prepared for constructor/destructor table to have
       a "valid" NULL address.  */
    ${CONSTRUCTING+ _ctors_start = . ; }
    ${CONSTRUCTING+ KEEP (*(SORT_BY_INIT_PRIORITY(.ctors.*)))}
    ${CONSTRUCTING+ KEEP (*(.ctors))}
    ${CONSTRUCTING+ _ctors_end = . ; }
    ${CONSTRUCTING+ _dtors_start = . ; }
    ${CONSTRUCTING+ KEEP (*(SORT_BY_INIT_PRIORITY(.dtors.*)))}
    ${CONSTRUCTING+ KEEP (*(.dtors))}
    ${CONSTRUCTING+ _dtors_end = . ; }

    /* DATA memory starts at address 0.  So to avoid placing a valid static
       variable at the invalid NULL address, we introduce the .data.atzero
       section.  If CRT can make some use of it - great.  Otherwise skip a
       word.  In all cases .data/.bss sections must start at non-zero.  */
    . += (. == 0 ? 4 : 0);

    ${RELOCATING+ PROVIDE (_data_start = .) ; }
    *(.data)
    ${RELOCATING+ *(.data*)}
    ${RELOCATING+ *(.rodata)  /* We need to include .rodata here if gcc is used.  */}
    ${RELOCATING+ *(.rodata.*) /* with -fdata-sections.  */}
    ${RELOCATING+*(.gnu.linkonce.d*)}
    ${RELOCATING+*(.gnu.linkonce.r*)}
    ${RELOCATING+. = ALIGN(4);}
    ${RELOCATING+ PROVIDE (_data_end = .) ; }
  } ${RELOCATING+ > dmem }

  .resource_table ${RELOCATING-0} :
  {
    *(.resource_table)
    KEEP (*(.resource_table))
  }  > dmem

  .bss ${RELOCATING-0} :
  {
    ${RELOCATING+ PROVIDE (_bss_start = .) ; }
    *(.bss)
    ${RELOCATING+ *(.bss.*)}
    ${RELOCATING+*(.gnu.linkonce.b*)}
    *(COMMON)
    ${RELOCATING+ PROVIDE (_bss_end = .) ; }
  } ${RELOCATING+ > dmem}

  /* Global data not cleared after reset.  */
  .noinit ${RELOCATING-0}:
  {
    ${RELOCATING+ PROVIDE (_noinit_start = .) ; }
    *(.noinit)
    ${RELOCATING+ PROVIDE (_noinit_end = .) ; }
    ${RELOCATING+ PROVIDE (_heap_start = .) ; }
    ${RELOCATING+ . += __HEAP_SIZE ; }
    /* Stack is not here really.  It will be put at the end of DMEM.
       But we take into account its size here, in order to allow
       for MEMORY overflow checking during link time.  */
    ${RELOCATING+ . += __STACK_SIZE ; }
  } ${RELOCATING+ > dmem}

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
