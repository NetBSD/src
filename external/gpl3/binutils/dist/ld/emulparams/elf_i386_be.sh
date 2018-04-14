. ${srcdir}/emulparams/extern_protected_data.sh
. ${srcdir}/emulparams/dynamic_undefined_weak.sh
. ${srcdir}/emulparams/call_nop.sh
SCRIPT_NAME=elf
OUTPUT_FORMAT="elf32-i386"
NO_RELA_RELOCS=yes
TEXT_START_ADDR=0x80000000
MAXPAGESIZE="CONSTANT (MAXPAGESIZE)"
ARCH=i386
MACHINE=
TEMPLATE_NAME=elf32
GENERATE_SHLIB_SCRIPT=yes
NO_SMALL_DATA=yes
