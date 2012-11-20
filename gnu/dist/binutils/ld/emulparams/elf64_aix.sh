# See genscripts.sh and ../scripttempl/elf.sc for the meaning of these.
SCRIPT_NAME=elf
ELFSIZE=64
TEMPLATE_NAME=elf32
OUTPUT_FORMAT="elf64-ia64-aix-little"
ARCH=ia64
MACHINE=
MAXPAGESIZE=0x10000
TEXT_START_ADDR="0x10000000"
DATA_ADDR="ALIGN (0x10000000) + (. & (${MAXPAGESIZE} - 1))"
GENERATE_SHLIB_SCRIPT=yes
NOP=0x00300000010070000002000001000400  # a bundle full of nops
OTHER_GOT_SECTIONS="
  .IA_64.pltoff ${RELOCATING-0} : { *(.IA_64.pltoff) }"
OTHER_PLT_RELOC_SECTIONS="
  .rela.IA_64.pltoff ${RELOCATING-0} : { *(.rela.IA_64.pltoff) }"
OTHER_READONLY_SECTIONS="
  .opd          ${RELOCATING-0} : { *(.opd) }
  .IA_64.unwind_info ${RELOCATING-0} : { *(.IA_64.unwind_info*${RELOCATING+ .gnu.linkonce.ia64unwi.*}) }
  .IA_64.unwind ${RELOCATING-0} : { *(.IA_64.unwind*${RELOCATING+ .gnu.linkonce.ia64unw.*}) }"
LIB_PATH="=/usr/lib/ia64l64:=/usr/lib:=/usr/local/lib"
