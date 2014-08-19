# Linker script for Alpha VMS systems.
# Tristan Gingold <gingold@adacore.com>.

PAGESIZE=0x10000

cat <<EOF
OUTPUT_FORMAT("${OUTPUT_FORMAT}")
${LIB_SEARCH_DIRS}

SECTIONS
{
  ${RELOCATING+. = ${PAGESIZE};}

  /* RW initialized data.  */
  \$DATA\$ ALIGN (${PAGESIZE}) : {
    *(\$DATA\$)
  }
  /* RW data unmodified (zero-initialized).  */
  \$BSS\$ ALIGN (${PAGESIZE}) : {
    *(\$BSS\$)
  }
  /* RO, executable code.  */
  \$CODE\$ ALIGN (${PAGESIZE}) : {
    *(\$CODE\$ *\$CODE*)
  }
  /* RO initialized data.  */
  \$LITERAL\$ ALIGN (${PAGESIZE}) : {
    *(\$LINK\$)
    *(\$LITERAL\$)
    *(\$READONLY\$)
    *(\$READONLY_ADDR\$)
    *(eh_frame)
    *(jcr)
    *(ctors)
    *(dtors)
    *(gcc_except_table)

    /* LIB$INITIALIZE stuff.  */
    *(LIB\$INITIALIZDZ)	/* Start marker.  */
    *(LIB\$INITIALIZD_)	/* Hi priority.  */
    *(LIB\$INITIALIZE)	/* User.  */
    *(LIB\$INITIALIZE$)	/* End marker.  */
  }

  \$DWARF\$ ALIGN (${PAGESIZE}) : {
    \$dwarf2.debug_pubtypes = .;
    *(debug_pubtypes)
    \$dwarf2.debug_ranges = .;
    *(debug_ranges)

    \$dwarf2.debug_abbrev = .;
    *(debug_abbrev)
    \$dwarf2.debug_aranges = .;
    *(debug_aranges)
    \$dwarf2.debug_frame = .;
    *(debug_frame)
    \$dwarf2.debug_info = .;
    *(debug_info)
    \$dwarf2.debug_line = .;
    *(debug_line)
    \$dwarf2.debug_loc = .;
    *(debug_loc)
    \$dwarf2.debug_macinfo = .;
    *(debug_macinfo)
    \$dwarf2.debug_macro = .;
    *(debug_macro)
    \$dwarf2.debug_pubnames = .;
    *(debug_pubnames)
    \$dwarf2.debug_str = .;
    *(debug_str)
    \$dwarf2.debug_zzzzzz = .;
  }

  \$DST\$ 0 : {
    *(\$DST\$)
  }
}
EOF
