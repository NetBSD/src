# Copyright (C) 2014-2018 Free Software Foundation, Inc.
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

cat << EOF
/* Copyright (C) 2014-2018 Free Software Foundation, Inc.

   Copying and distribution of this script, with or without modification,
   are permitted in any medium without royalty provided the copyright
   notice and this notice are preserved.  */

OUTPUT_FORMAT("elf32-v850-rh850", "elf32-v850-rh850",
	      "elf32-v850-rh850")
OUTPUT_ARCH(v850:rh850)
${RELOCATING+ENTRY(_start)}
SEARCH_DIR(.);
${RELOCATING+EXTERN(__ctbp __ep __gp)};
SECTIONS
{
  /* This saves a little space in the ELF file, since the zda starts
     at a higher location that the ELF headers take up.  */

  .zdata ${ZDATA_START_ADDR} :
  {
	*(.zdata)
	*(.zdata23)
	*(.zbss)
	*(.zbss23)
	*(reszdata)
	*(.zcommon)
  }

  /* This is the read only part of the zero data area.
     Having it as a separate section prevents its
     attributes from being inherited by the zdata
     section.  Specifically it prevents the zdata
     section from being marked READONLY.  */

  .rozdata ${ROZDATA_START_ADDR} :
  {
	*(.rozdata)
	*(romzdata)
	*(romzbss)
	*(.zconst)
	*(.zconst23)
  }

  /* Read-only sections, merged into text segment.  */
  . = ${TEXT_START_ADDR};
  .interp	: { *(.interp) }
  .hash		: { *(.hash) }
  .dynsym	: { *(.dynsym) }
  .dynstr	: { *(.dynstr) }
  .rel.text	: { *(.rel.text) }
  .rela.text	: { *(.rela.text) }
  .rel.data	: { *(.rel.data) }
  .rela.data	: { *(.rela.data) }
  .rel.rodata	: { *(.rel.rodata) }
  .rela.rodata	: { *(.rela.rodata) }
  .rel.gcc_except_table : { *(.rel.gcc_except_table) }
  .rela.gcc_except_table : { *(.rela.gcc_except_table) }
  .rel.got	: { *(.rel.got) }
  .rela.got	: { *(.rela.got) }
  .rel.ctors	: { *(.rel.ctors) }
  .rela.ctors	: { *(.rela.ctors) }
  .rel.dtors	: { *(.rel.dtors) }
  .rela.dtors	: { *(.rela.dtors) }
  .rel.init	: { *(.rel.init) }
  .rela.init	: { *(.rela.init) }
  .rel.fini	: { *(.rel.fini) }
  .rela.fini	: { *(.rela.fini) }
  .rel.bss	: { *(.rel.bss) }
  .rela.bss	: { *(.rela.bss) }
  .rel.plt	: { *(.rel.plt) }
  .rela.plt	: { *(.rela.plt) }
  .init		: { KEEP (*(.init)) } =0
  .plt		: { *(.plt) }

  .text		:
  {
    *(.text)
    ${RELOCATING+*(.text.*)}

    /* .gnu.warning sections are handled specially by elf32.em.  */
    *(.gnu.warning)
    *(.gnu.linkonce.t*)
  } =0

  ${RELOCATING+_etext = .;}
  ${RELOCATING+PROVIDE (etext = .);}

   /* This is special code area at the end of the normal text section.
      It contains a small lookup table at the start followed by the
      code pointed to by entries in the lookup table.  */

  .call_table_data ${CALL_TABLE_START_ADDR} :
  {
    ${RELOCATING+PROVIDE(__ctbp = .);}
    *(.call_table_data)
  } = 0xff   /* Fill gaps with 0xff.  */

  .call_table_text :
  {
    *(.call_table_text)
  }

  .fini		: { KEEP (*(.fini)) } =0
  .rodata	:
  {
	*(.rodata)
	${RELOCATING+*(.rodata.*)}
	*(.gnu.linkonce.r*)
	*(.const)
  }
  .rodata1	: { *(.rodata1) }

  .data		:
  {
    *(.data)
    ${RELOCATING+*(.data.*)}
    *(.gnu.linkonce.d*)
    CONSTRUCTORS
  }
  .data1	: { *(.data1) }
  .ctors	:
  {
    ${CONSTRUCTING+___ctors = .;}
    KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*crtend(.ctors))
    ${CONSTRUCTING+___ctors_end = .;}
  }
  .dtors	:
  {
    ${CONSTRUCTING+___dtors = .;}
    KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*crtend.o(.dtors))
    ${CONSTRUCTING+___dtors_end = .;}
  }
  .jcr		:
  {
    KEEP (*(.jcr))
  }

  .gcc_except_table : { *(.gcc_except_table) }

  .got		: { *(.got.plt) *(.got) }
  .dynamic	: { *(.dynamic) }

  .tdata ${TDATA_START_ADDR} :
  {
	${RELOCATING+PROVIDE (__ep = .);}
	*(.edata)
	*(.edata23)
	*(.tbyte)
	*(.tcommon_byte)
	*(.tdata)
	*(.tdata*)
	*(.ebss)
	*(.ebss23)
	*(.tbss)
	*(.tbss*)
	*(.tcommon)
  }

  /* We want the small data sections together, so single-instruction offsets
     can access them all, and initialized data all before uninitialized, so
     we can shorten the on-disk segment size.  */

  .sdata ${SDATA_START_ADDR} :
  {
	${RELOCATING+PROVIDE (__gp = . + 0x8000);}
	*(.sdata)
	*(.sdata23)
   }

  /* See comment about .rozdata. */
  .rosdata ${ROSDATA_START_ADDR} :
  {
	*(.rosdata)
	*(.sconst)
	*(.sconst23)
  }

  /* We place the .sbss data section AFTER the .rosdata section, so that
     it can directly precede the .bss section.  This allows runtime startup
     code to initialise all the zero-data sections by simply taking the
     value of '_edata' and zeroing until it reaches '_end'.  */

  .sbss :
  {
	${RELOCATING+__sbss_start = .;}
	*(.sbss)
	*(.sbss23)
	*(.scommon)
  }

  ${RELOCATING+_edata  = DEFINED (__sbss_start) ? __sbss_start : . ;}
  ${RELOCATING+PROVIDE (edata = _edata);}

  .bss       :
  {
	${RELOCATING+__bss_start = DEFINED (__sbss_start) ? __sbss_start : . ;}
	${RELOCATING+__real_bss_start = . ;}
	*(.dynbss)
	*(.bss)
	*(COMMON)
  }

  ${RELOCATING+_end = . ;}
  ${RELOCATING+PROVIDE (end = .);}
  ${RELOCATING+PROVIDE (_heap_start = .);}

  .note.renesas 0 : { KEEP(*(.note.renesas)) }

  /* Stabs debugging sections.  */
  .stab 0		: { *(.stab) }
  .stabstr 0		: { *(.stabstr) }
  .stab.excl 0		: { *(.stab.excl) }
  .stab.exclstr 0	: { *(.stab.exclstr) }
  .stab.index 0		: { *(.stab.index) }
  .stab.indexstr 0	: { *(.stab.indexstr) }
  .comment 0		: { *(.comment) }

EOF

. $srcdir/scripttempl/DWARF.sc

cat <<EOF
  /* User stack.  */
  .stack 0x200000	:
  {
	${RELOCATING+__stack = .;}
	*(.stack)
  }
}
EOF
