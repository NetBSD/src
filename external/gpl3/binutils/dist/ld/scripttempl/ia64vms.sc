# Linker script for Itanium VMS systems.
# Tristan Gingold <gingold@adacore.com>.

PAGESIZE=0x10000
BLOCKSIZE=0x200

cat <<EOF
OUTPUT_FORMAT("${OUTPUT_FORMAT}")
${LIB_SEARCH_DIRS}
ENTRY(__entry)

SECTIONS
{
  /* RW segment.  */
  ${RELOCATING+. = ${PAGESIZE};}

  \$DATA\$ ALIGN (${BLOCKSIZE}) : {
    *(\$DATA\$ .data .data.*)
    *(\$BSS\$ .bss)
  }

  /* Code segment.  Note: name must be \$CODE\$ */
  ${RELOCATING+. = ALIGN (${PAGESIZE});}

  \$CODE\$ ALIGN (${BLOCKSIZE}) : {
    *(\$CODE\$ .text)
  }
  .plt ALIGN (8) : {
    *(.plt)
  }

  /* RO segment.  */
  ${RELOCATING+. = ALIGN (${PAGESIZE});}

  /* RO initialized data.  */
  \$LITERAL\$ ALIGN (${BLOCKSIZE}) : {
    *(\$LITERAL\$)
    *(\$READONLY\$ .rodata)
    *(.jcr)
    *(.ctors)
    *(.dtors)
    *(.opd)
    *(.gcc_except_table)

    /* LIB$INITIALIZE stuff.  */
    *(LIB\$INITIALIZDZ)	/* Start marker.  */
    *(LIB\$INITIALIZD_)	/* Hi priority.  */
    *(LIB\$INITIALIZE)	/* User.  */
    *(LIB\$INITIALIZE$)	/* End marker.  */
  }

  /* Short segment.  */
  ${RELOCATING+. = ALIGN (${PAGESIZE});}

  .srodata : {
    *(.srodata)
  }
  .got ALIGN (8) : {
    *(.got)
  }
  .IA_64.pltoff ALIGN (16) : {
    *(.IA_64.pltoff)
  }
  \$TFR\$ ALIGN (16) : {
    /* Tranfer vector.  */
    __entry = .;
    *(.transfer)
  }

  ${RELOCATING+. = ALIGN (${PAGESIZE});}

  \$RW_SHORT\$ ALIGN (${BLOCKSIZE}) : {
    *(.sdata .sdata.*)
    *(.sbss)
  }

  ${RELOCATING+. = ALIGN (${PAGESIZE});}

  .IA_64.unwind ALIGN (${BLOCKSIZE}) : {
    *(.IA_64.unwind .IA_64.unwind.*)
  }

  .IA_64.unwind_info ALIGN (8) : {
    *(.IA_64.unwind_info .IA_64.unwind_info.*)
  }

  ${RELOCATING+. = ALIGN (${PAGESIZE});}

  .dynamic /* \$DYNAMIC\$ */ ALIGN (${BLOCKSIZE}) : {
    *(.dynamic)
    *(.vmsdynstr)
    *(.fixups)
  }

  ${RELOCATING+. = ALIGN (${PAGESIZE});}

  .dynstr : { *(.dynstr) }

  .dynsym       ${RELOCATING-0} : { *(.dynsym) }
  .rela.got : { *(.rela.got) }
  .got.plt : { *(.got.plt) }
  .gnu.version_d : { *(.gnu.version_d) }
  .gnu.version : { *(.gnu.version) }
  .gnu.version_r : { *(.gnu.version_r) }
  .rela.IA_64.pltoff : { *(.rela.IA_64.pltoff) }

  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info${RELOCATING+ .gnu.linkonce.wi.*}) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  .trace_info     0 : { *(.trace_info) }
  .trace_abbrev   0 : { *(.trace_abbrev) }
  .trace_aranges  0 : { *(.trace_aranges) }

  /* DWARF 3 */
  .debug_pubtypes 0 : { *(.debug_pubtypes) }
  .debug_ranges   0 : { *(.debug_ranges) }

  /* DWARF Extension.  */
  .debug_macro    0 : { *(.debug_macro) } 
  
  .note : { *(.vms.note) }

  /DISCARD/ : { *(.note) }
}
EOF
