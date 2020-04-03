. ${srcdir}/emulparams/armelf_linux.sh

OUTPUT_FORMAT="elf32-littlearm-fdpic"
BIG_OUTPUT_FORMAT="elf32-bigarm-fdpic"
LITTLE_OUTPUT_FORMAT="elf32-littlearm-fdpic"

# Use the ARM ABI-compliant exception-handling sections.
OTHER_READONLY_SECTIONS="
  .ARM.extab ${RELOCATING-0} : { *(.ARM.extab${RELOCATING+* .gnu.linkonce.armextab.*}) }
  ${RELOCATING+ PROVIDE_HIDDEN (__exidx_start = .); }
  .ARM.exidx ${RELOCATING-0} : { *(.ARM.exidx${RELOCATING+* .gnu.linkonce.armexidx.*}) }
  ${RELOCATING+ PROVIDE_HIDDEN (__exidx_end = .); }
  .rofixup : {
	${RELOCATING+__ROFIXUP_LIST__ = .;}
	*(.rofixup)
	${RELOCATING+__ROFIXUP_END__ = .;}
}"
