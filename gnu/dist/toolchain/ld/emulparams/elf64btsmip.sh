# If you change this file, please also look at files which source this one:
# elf64ltsmip.sh

. ${srcdir}/emulparams/elf32bmipn32.sh
OUTPUT_FORMAT="elf64-tradbigmips"
BIG_OUTPUT_FORMAT="elf64-tradbigmips"
LITTLE_OUTPUT_FORMAT="elf64-tradlittlemips"
ELFSIZE=64

GENERATE_SHLIB_SCRIPT=yes
DATA_ADDR=0x0400000000
NONPAGED_TEXT_START_ADDR=0x10000000
SHLIB_TEXT_START_ADDR=0
TEXT_DYNAMIC=

unset EXECUTABLE_SYMBOLS
unset WRITABLE_RODATA

# Magic sections.
INITIAL_READONLY_SECTIONS='.MIPS.options : { *(.MIPS.options) }'
OTHER_TEXT_SECTIONS='*(.mips16.fn.*) *(.mips16.call.*)'
OTHER_SECTIONS='
  .gptab.sdata : { *(.gptab.data) *(.gptab.sdata) }
  .gptab.sbss : { *(.gptab.bss) *(.gptab.sbss) }
'
