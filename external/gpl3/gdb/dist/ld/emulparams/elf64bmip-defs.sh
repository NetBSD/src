source_sh ${srcdir}/emulparams/elf32bmipn32-defs.sh
COMMONPAGESIZE="CONSTANT (COMMONPAGESIZE)"
INITIAL_READONLY_SECTIONS="
  .MIPS.abiflags      ${RELOCATING-0} : { *(.MIPS.abiflags) }
  .MIPS.xhash      ${RELOCATING-0} : { *(.MIPS.xhash) }
  .MIPS.options : { *(.MIPS.options) }
"
