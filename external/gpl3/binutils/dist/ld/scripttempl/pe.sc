# Linker script for PE.
#
# Copyright (C) 2014-2016 Free Software Foundation, Inc.
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
  if test "x$LD_FLAG" = "xauto_import" ; then
    R_DATA='*(SORT(.data$*))
            *(.rdata)
	    *(SORT(.rdata$*))'
    R_RDATA=''
  else
    R_DATA='*(SORT(.data$*))'
    R_RDATA='*(.rdata)
             *(SORT(.rdata$*))'
  fi
  R_IDATA234='
    KEEP (SORT(*)(.idata$2))
    KEEP (SORT(*)(.idata$3))
    /* These zeroes mark the end of the import list.  */
    LONG (0); LONG (0); LONG (0); LONG (0); LONG (0);
    KEEP (SORT(*)(.idata$4))'
  R_IDATA5='KEEP (SORT(*)(.idata$5))'
  R_IDATA67='
    KEEP (SORT(*)(.idata$6))
    KEEP (SORT(*)(.idata$7))'
  R_CRT_XC='KEEP (*(SORT(.CRT$XC*)))  /* C initialization */'
  R_CRT_XI='KEEP (*(SORT(.CRT$XI*)))  /* C++ initialization */'
  R_CRT_XL='KEEP (*(SORT(.CRT$XL*)))  /* TLS callbacks */'
  R_CRT_XP='KEEP (*(SORT(.CRT$XP*)))  /* Pre-termination */'
  R_CRT_XT='KEEP (*(SORT(.CRT$XT*)))  /* Termination */'
  R_TLS='
    KEEP (*(.tls$AAA))
    KEEP (*(.tls))
    KEEP (*(.tls$))
    KEEP (*(SORT(.tls$*)))
    KEEP (*(.tls$ZZZ))'
  R_RSRC='
    KEEP (*(.rsrc))
    KEEP (*(.rsrc$*))'
else
  R_TEXT=
  R_DATA=
  R_RDATA='*(.rdata)'
  R_IDATA234=
  R_IDATA5=
  R_IDATA67=
  R_CRT=
  R_RSRC='*(.rsrc)'
fi

cat <<EOF
/* Copyright (C) 2014-2016 Free Software Foundation, Inc.

   Copying and distribution of this script, with or without modification,
   are permitted in any medium without royalty provided the copyright
   notice and this notice are preserved.  */

${RELOCATING+OUTPUT_FORMAT(${OUTPUT_FORMAT})}
${RELOCATING-OUTPUT_FORMAT(${RELOCATEABLE_OUTPUT_FORMAT})}
${OUTPUT_ARCH+OUTPUT_ARCH(${OUTPUT_ARCH})}

${LIB_SEARCH_DIRS}

SECTIONS
{
  ${RELOCATING+/* Make the virtual address and file offset synced if the alignment is}
  ${RELOCATING+   lower than the target page size. */}
  ${RELOCATING+. = SIZEOF_HEADERS;}
  ${RELOCATING+. = ALIGN(__section_alignment__);}
  .text ${RELOCATING+ __image_base__ + ( __section_alignment__ < ${TARGET_PAGE_SIZE} ? . : __section_alignment__ )} :
  {
    ${RELOCATING+ KEEP(*(.init))}
    *(.text)
    ${R_TEXT}
    ${RELOCATING+ *(.text.*)}
    ${RELOCATING+ *(.gnu.linkonce.t.*)}
    *(.glue_7t)
    *(.glue_7)
    ${CONSTRUCTING+ ___CTOR_LIST__ = .; __CTOR_LIST__ = . ;
			LONG (-1);*(.ctors); *(.ctor); *(SORT(.ctors.*));  LONG (0); }
    ${CONSTRUCTING+ ___DTOR_LIST__ = .; __DTOR_LIST__ = . ;
			LONG (-1); *(.dtors); *(.dtor); *(SORT(.dtors.*));  LONG (0); }
    ${RELOCATING+ *(.fini)}
    /* ??? Why is .gcc_exc here?  */
    ${RELOCATING+ *(.gcc_exc)}
    ${RELOCATING+PROVIDE (etext = .);}
    ${RELOCATING+PROVIDE (_etext = .);}
    ${RELOCATING+ *(.gcc_except_table)}
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
    KEEP(*(.jcr))
    ${RELOCATING+__data_end__ = . ;}
    ${RELOCATING+*(.data_cygwin_nocopy)}
  }

  .rdata ${RELOCATING+BLOCK(__section_alignment__)} :
  {
    ${R_RDATA}
    ${RELOCATING+__rt_psrelocs_start = .;}
    KEEP(*(.rdata_runtime_pseudo_reloc))
    ${RELOCATING+__rt_psrelocs_end = .;}
  }
  ${RELOCATING+__rt_psrelocs_size = __rt_psrelocs_end - __rt_psrelocs_start;}
  ${RELOCATING+___RUNTIME_PSEUDO_RELOC_LIST_END__ = .;}
  ${RELOCATING+__RUNTIME_PSEUDO_RELOC_LIST_END__ = .;}
  ${RELOCATING+___RUNTIME_PSEUDO_RELOC_LIST__ = . - __rt_psrelocs_size;}
  ${RELOCATING+__RUNTIME_PSEUDO_RELOC_LIST__ = . - __rt_psrelocs_size;}

  .eh_frame ${RELOCATING+BLOCK(__section_alignment__)} :
  {
    KEEP(*(.eh_frame*))
  }

  .pdata ${RELOCATING+BLOCK(__section_alignment__)} :
  {
    KEEP(*(.pdata))
  }

  .bss ${RELOCATING+BLOCK(__section_alignment__)} :
  {
    ${RELOCATING+__bss_start__ = . ;}
    *(.bss)
    *(COMMON)
    ${RELOCATING+__bss_end__ = . ;}
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
    ${RELOCATING+ *(.note.GNU-stack)}
    ${RELOCATING+ *(.gnu.lto_*)}
  }

  .idata ${RELOCATING+BLOCK(__section_alignment__)} :
  {
    /* This cannot currently be handled with grouped sections.
	See pe.em:sort_sections.  */
    ${R_IDATA234}
    ${RELOCATING+__IAT_start__ = .;}
    ${R_IDATA5}
    ${RELOCATING+__IAT_end__ = .;}
    ${R_IDATA67}
  }
  .CRT ${RELOCATING+BLOCK(__section_alignment__)} :
  { 					
    ${RELOCATING+___crt_xc_start__ = . ;}
    ${R_CRT_XC}
    ${RELOCATING+___crt_xc_end__ = . ;}
    ${RELOCATING+___crt_xi_start__ = . ;}
    ${R_CRT_XI}
    ${RELOCATING+___crt_xi_end__ = . ;}
    ${RELOCATING+___crt_xl_start__ = . ;}
    ${R_CRT_XL}
    /* ___crt_xl_end__ is defined in the TLS Directory support code */
    ${RELOCATING+___crt_xp_start__ = . ;}
    ${R_CRT_XP}
    ${RELOCATING+___crt_xp_end__ = . ;}
    ${RELOCATING+___crt_xt_start__ = . ;}
    ${R_CRT_XT}
    ${RELOCATING+___crt_xt_end__ = . ;}
  }

  /* Windows TLS expects .tls\$AAA to be at the start and .tls\$ZZZ to be
     at the end of section.  This is important because _tls_start MUST
     be at the beginning of the section to enable SECREL32 relocations with TLS
     data.  */
  .tls ${RELOCATING+BLOCK(__section_alignment__)} :
  { 					
    ${RELOCATING+___tls_start__ = . ;}
    ${R_TLS}
    ${RELOCATING+___tls_end__ = . ;}
  }

  .endjunk ${RELOCATING+BLOCK(__section_alignment__)} :
  {
    /* end is deprecated, don't use it */
    ${RELOCATING+PROVIDE (end = .);}
    ${RELOCATING+PROVIDE ( _end = .);}
    ${RELOCATING+ __end__ = .;}
  }

  .rsrc ${RELOCATING+BLOCK(__section_alignment__)} : SUBALIGN(4)
  {
    ${R_RSRC}
  }

  .reloc ${RELOCATING+BLOCK(__section_alignment__)} :
  {
    *(.reloc)
  }

  .stab ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.stab)
  }

  .stabstr ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.stabstr)
  }

  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section.  Unlike other targets that fake this by putting the
     section VMA at 0, the PE format will not allow it.  */

  /* DWARF 1.1 and DWARF 2.  */
  .debug_aranges ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.debug_aranges)
  }
  .zdebug_aranges ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.zdebug_aranges)
  }

  .debug_pubnames ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.debug_pubnames)
  }
  .zdebug_pubnames ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.zdebug_pubnames)
  }

  .debug_pubtypes ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.debug_pubtypes)
  }
  .zdebug_pubtypes ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.zdebug_pubtypes)
  }

  /* DWARF 2.  */
  .debug_info ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.debug_info${RELOCATING+ .gnu.linkonce.wi.*})
  }
  .zdebug_info ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.zdebug_info${RELOCATING+ .zdebug.gnu.linkonce.wi.*})
  }

  .debug_abbrev ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.debug_abbrev)
  }
  .zdebug_abbrev ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.zdebug_abbrev)
  }

  .debug_line ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.debug_line)
  }
  .zdebug_line ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.zdebug_line)
  }

  .debug_frame ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.debug_frame*)
  }
  .zdebug_frame ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.zdebug_frame*)
  }

  .debug_str ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.debug_str)
  }
  .zdebug_str ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.zdebug_str)
  }

  .debug_loc ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.debug_loc)
  }
  .zdebug_loc ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.zdebug_loc)
  }

  .debug_macinfo ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.debug_macinfo)
  }
  .zdebug_macinfo ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.zdebug_macinfo)
  }

  /* SGI/MIPS DWARF 2 extensions.  */
  .debug_weaknames ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.debug_weaknames)
  }
  .zdebug_weaknames ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.zdebug_weaknames)
  }

  .debug_funcnames ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.debug_funcnames)
  }
  .zdebug_funcnames ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.zdebug_funcnames)
  }

  .debug_typenames ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.debug_typenames)
  }
  .zdebug_typenames ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.zdebug_typenames)
  }

  .debug_varnames ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.debug_varnames)
  }
  .zdebug_varnames ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.zdebug_varnames)
  }

  .debug_macro ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.debug_macro)
  }
  .zdebug_macro ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.zdebug_macro)
  }

  /* DWARF 3.  */
  .debug_ranges ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.debug_ranges)
  }
  .zdebug_ranges ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.zdebug_ranges)
  }

  /* DWARF 4.  */
  .debug_types ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.debug_types${RELOCATING+ .gnu.linkonce.wt.*})
  }
  .zdebug_types ${RELOCATING+BLOCK(__section_alignment__)} ${RELOCATING+(NOLOAD)} :
  {
    *(.zdebug_types${RELOCATING+ .gnu.linkonce.wt.*})
  }
}
EOF
