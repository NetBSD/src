# Linker script for MCore PE.
#
# Copyright (C) 2014-2020 Free Software Foundation, Inc.
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

if test -z "${RELOCATEABLE_OUTPUT_FORMAT}"; then
  RELOCATEABLE_OUTPUT_FORMAT=${OUTPUT_FORMAT}
fi

# We can't easily and portably get an unquoted $ in a shell
# substitution, so we do this instead.
# Sorting of the .foo$* sections is required by the definition of
# grouped sections in PE.
# Sorting of the file names in R_IDATA is required by the
# current implementation of dlltool (this could probably be changed to
# use grouped sections instead).
if test "${RELOCATING}"; then
  R_TEXT='*(SORT(.text$*))'
  R_DATA='*(SORT(.data$*))'
  R_RDATA='*(SORT(.rdata$*))'
  R_IDATA='
    SORT(*)(.idata$2)
    SORT(*)(.idata$3)
    /* These zeroes mark the end of the import list.  */
    LONG (0); LONG (0); LONG (0); LONG (0); LONG (0);
    SORT(*)(.idata$4)
    SORT(*)(.idata$5)
    SORT(*)(.idata$6)
    SORT(*)(.idata$7)'
  R_CRT='*(SORT(.CRT$*))'
  R_RSRC='*(SORT(.rsrc$*))'
else
  R_TEXT=
  R_DATA=
  R_RDATA=
  R_IDATA=
  R_CRT=
  R_RSRC=
fi

if test "$RELOCATING"; then
  # Can't use ${RELOCATING+blah "blah" blah} for this,
  # because bash 2.x will lose the doublequotes.
  cat <<EOF
OUTPUT_FORMAT("${OUTPUT_FORMAT}", "${BIG_OUTPUT_FORMAT}",
		           "${LITTLE_OUTPUT_FORMAT}")
EOF
fi

cat <<EOF
/* Copyright (C) 2014-2020 Free Software Foundation, Inc.

   Copying and distribution of this script, with or without modification,
   are permitted in any medium without royalty provided the copyright
   notice and this notice are preserved.  */

${LIB_SEARCH_DIRS}

${RELOCATING+ENTRY (_mainCRTStartup)}

SECTIONS
{
  .text ${RELOCATING+ __image_base__ + __section_alignment__ } :
  {
    ${RELOCATING+ KEEP (*(SORT_NONE(.init)))}
    *(.text)
    ${R_TEXT}
    ${RELOCATING+ *(.text.*)}
    *(.glue_7t)
    *(.glue_7)
    ${CONSTRUCTING+ ___CTOR_LIST__ = .; __CTOR_LIST__ = . ;
			LONG (-1); *(.ctors); *(.ctor); LONG (0); }
    ${CONSTRUCTING+ ___DTOR_LIST__ = .; __DTOR_LIST__ = . ;
			LONG (-1); *(.dtors); *(.dtor);  LONG (0); }
    ${RELOCATING+ KEEP (*(SORT_NONE(.fini)))}
    ${RELOCATING+/* ??? Why is .gcc_exc here?  */}
    ${RELOCATING+ *(.gcc_exc)}
    ${RELOCATING+ etext = .;}
    *(.gcc_except_table)
  }

  /* The Cygwin32 library uses a section to avoid copying certain data
     on fork.  This used to be named ".data$nocopy".  The linker used
     to include this between __data_start__ and __data_end__, but that
     breaks building the cygwin32 dll.  Instead, we name the section
     ".data_cygwin_nocopy" and explicitly include it after __data_end__. */

  .data ${RELOCATING+BLOCK(__section_alignment__)} :
  {
    ${RELOCATING+__data_start__ = . ;}
    *(.data)
    *(.data2)
    ${R_DATA}
    ${RELOCATING+__data_end__ = . ;}
    ${RELOCATING+*(.data_cygwin_nocopy)}
  }

  .bss ${RELOCATING+BLOCK(__section_alignment__)} :
  {
    ${RELOCATING+__bss_start__ = . ;}
    *(.bss)
    *(COMMON)
    ${RELOCATING+__bss_end__ = . ;}
  }

  .rdata ${RELOCATING+BLOCK(__section_alignment__)} :
  {
    *(.rdata)
    ${R_RDATA}
    *(.eh_frame)
  }

  .edata ${RELOCATING+BLOCK(__section_alignment__)} :
  {
    *(.edata)
  }

  /DISCARD/ :
  {
    *(.debug\$S)
    *(.debug\$T)
    *(.debug\$F)
    *(.drectve)
  }

  .idata ${RELOCATING+BLOCK(__section_alignment__)} :
  {
    /* This cannot currently be handled with grouped sections.
	See pe.em:sort_sections.  */
    ${R_IDATA}
  }
  .CRT ${RELOCATING+BLOCK(__section_alignment__)} :
  {
    ${R_CRT}
  }

  .endjunk ${RELOCATING+BLOCK(__section_alignment__)} :
  {
    /* end is deprecated, don't use it */
    ${RELOCATING+ end = .;}
    ${RELOCATING+ _end = .;}
    ${RELOCATING+ __end__ = .;}
  }

  .reloc ${RELOCATING+BLOCK(__section_alignment__)} :
  {
    *(.reloc)
  }

  .rsrc ${RELOCATING+BLOCK(__section_alignment__)} :
  {
    *(.rsrc)
    ${R_RSRC}
  }

  .stab ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    [ .stab ]
  }

  .stabstr ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    [ .stabstr ]
  }

  .stack 0x80000 :
  {
    _stack = .;
    *(.stack)
  }
}
EOF
