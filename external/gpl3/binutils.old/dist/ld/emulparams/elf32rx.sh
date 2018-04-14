MACHINE=
SCRIPT_NAME=elf
OUTPUT_FORMAT="elf32-rx-le"
# See also `include/elf/rx.h'
TEXT_START_ADDR=0x10000000
ARCH=rx
ENTRY=_start
EMBEDDED=yes
TEMPLATE_NAME=elf32
EXTRA_EM_FILE=rxelf
# EXTRA_EM_FILE=needrelax
ELFSIZE=32
MAXPAGESIZE=256
# This is like setting STACK_ADDR to 0xbffffffc, except that the setting can
# be overridden, e.g. --defsym _stack=0x0f00, and that we put an extra
# sentinal value at the bottom.
# N.B. We can't use PROVIDE to set the default value in a symbol because
# the address is needed to place the .stack section, which in turn is needed
# to hold the sentinel value(s).
test -z "$CREATE_SHLIB" && OTHER_SECTIONS="  .stack        ${RELOCATING-0}${RELOCATING+(DEFINED(__stack) ? __stack : 0xbffffffc)} :
  {
    ${RELOCATING+__stack = .;}
    *(.stack)
    LONG(0xdeaddead)
  }"
# We do not need .stack for shared library.
test -n "$CREATE_SHLIB" && OTHER_SECTIONS=""

OTHER_TEXT_SECTIONS='*(P)'
OTHER_READONLY_SECTIONS='C_1 : { *(C_1) } C_2 : { *(C_2) } C : { *(C) } W_1 : { *(W_1) } W_2 : { *(W_2) } W : { *(W) }'
OTHER_READWRITE_SECTIONS='D_1 : { *(D_1) } D_2 : { *(D_2) } D : { *(D) }'
OTHER_BSS_SECTIONS='B_1 : { *(B_1) } B_2 : { *(B_2) } B : { *(B) }'
