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
    *(\$CODE\$)
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

  \$DST\$ 0 : {
    *(\$DST\$)
  }
}
EOF
