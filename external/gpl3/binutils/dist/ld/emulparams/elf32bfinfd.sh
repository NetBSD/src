. ${srcdir}/emulparams/bfin.sh
unset STACK_ADDR
OUTPUT_FORMAT="elf32-bfinfdpic"
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
# 0xff700000, 0xff800000, 0xff900000 and 0xffa00000 are also used in
# Dynamic linker and linux kernel. They need to be keep synchronized.
OTHER_SECTIONS="
  .l1.data 0xff700000	:
  {
    *(.l1.data)
  }
  .l1.data.A 0xff800000	:
  {
    *(.l1.data.A)
  }
  .l1.data.B 0xff900000	:
  {
    *(.l1.data.B)
  }
  .l1.text  0xffa00000	:
  {
    *(.l1.text)
  }
"
