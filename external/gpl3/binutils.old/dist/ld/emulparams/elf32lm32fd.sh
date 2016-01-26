. ${srcdir}/emulparams/elf32lm32.sh
unset STACK_ADDR
OUTPUT_FORMAT="elf32-lm32fdpic"
MAXPAGESIZE="CONSTANT (MAXPAGESIZE)"
TEMPLATE_NAME=elf32
GENERATE_SHLIB_SCRIPT=yes
GENERATE_PIE_SCRIPT=yes
EMBEDDED= # This gets us program headers mapped as part of the text segment.
OTHER_GOT_SYMBOLS=
OTHER_READONLY_SECTIONS="
  .rofixup        : {
    ${RELOCATING+__ROFIXUP_LIST__ = .;}
    *(.rofixup)
    ${RELOCATING+__ROFIXUP_END__ = .;}
  }
"
