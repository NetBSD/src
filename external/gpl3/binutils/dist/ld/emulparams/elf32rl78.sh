MACHINE=
SCRIPT_NAME=elf
OUTPUT_FORMAT="elf32-rl78"
# See also `include/elf/rl78.h'
TEXT_START_ADDR=0x00000
ARCH=rl78
ENTRY=_start
EMBEDDED=yes
TEMPLATE_NAME=elf32
ELFSIZE=32
EXTRA_EM_FILE=needrelax
MAXPAGESIZE=256
# This is like setting STACK_ADDR to 0xffedc, except that the setting can
# be overridden, e.g. --defsym _stack=0x0f00, and that we put an extra
# sentinal value at the bottom.
# N.B. We can't use PROVIDE to set the default value in a symbol because
# the address is needed to place the .stack section, which in turn is needed
# to hold the sentinel value(s).
test -z "$CREATE_SHLIB" && OTHER_SECTIONS="  .stack        ${RELOCATING-0}${RELOCATING+(DEFINED(__stack) ? __stack : 0xffedc)} :
  {
    ${RELOCATING+__stack = .;}
    *(.stack)
    LONG(0xdead)
  }"
# We do not need .stack for shared library.
test -n "$CREATE_SHLIB" && OTHER_SECTIONS=""
