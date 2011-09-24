SCRIPT_NAME=elf
TEMPLATE_NAME=elf32
OUTPUT_FORMAT="elf32-tic6x-le"
# This address is an arbitrary value expected to be suitable for
# semihosting simulator use, but not on hardware where it is expected
# to be overridden.
TEXT_START_ADDR=0x8000
MAXPAGESIZE="CONSTANT (MAXPAGESIZE)"
ARCH=tic6x
EXECUTABLE_SYMBOLS="EXTERN (__c6xabi_DSBT_BASE);"
SDATA_START_SYMBOLS="PROVIDE_HIDDEN (__c6xabi_DSBT_BASE = .);"
# ".bss" is near (small) BSS, ".far" is far (normal) BSS, ".const" is
# far read-only data, ".rodata" is near read-only data.  ".neardata"
# is near (small) data, ".fardata" is (along with .data) far data.
RODATA_NAME="const"
SDATA_NAME="neardata"
SBSS_NAME="bss"
BSS_NAME="far"
OTHER_SDATA_SECTIONS=".rodata ${RELOCATING-0} : { *(.rodata${RELOCATING+ .rodata.*}) }"
OTHER_READONLY_RELOC_SECTIONS="
  .rel.rodata   ${RELOCATING-0} : { *(.rel.rodata${RELOCATING+ .rel.rodata.*}) }
  .rela.rodata  ${RELOCATING-0} : { *(.rela.rodata${RELOCATING+ .rela.rodata.*}) }"
OTHER_READWRITE_SECTIONS=".fardata ${RELOCATING-0} : { *(.fardata${RELOCATING+ .fardata.*}) }"
OTHER_READWRITE_RELOC_SECTIONS="
  .rel.fardata     ${RELOCATING-0} : { *(.rel.fardata${RELOCATING+ .rel.fardata.*}) }
  .rela.fardata    ${RELOCATING-0} : { *(.rela.fardata${RELOCATING+ .rela.fardata.*}) }"
OTHER_BSS_SECTIONS="
  .heap : 
  { 
    . = ALIGN(4); 
    _HEAP_START = .; 
    . += 0x2000000; 
    _HEAP_MAX = .; 
  }
  .stack :
  {
    . +=  0x100000;
    _STACK_START = .;
  }"
ATTRS_SECTIONS='.c6xabi.attributes 0 : { KEEP (*(.c6xabi.attributes)) KEEP (*(.gnu.attributes)) }'
