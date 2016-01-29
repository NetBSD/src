. ${srcdir}/emulparams/plt_unwind.sh
. ${srcdir}/emulparams/extern_protected_data.sh
. ${srcdir}/emulparams/call_nop.sh
SCRIPT_NAME=elf_chaos
OUTPUT_FORMAT="elf32-i386"
TEXT_START_ADDR=0x40000000
MAXPAGESIZE="CONSTANT (MAXPAGESIZE)"
ARCH=i386
MACHINE=
NOP=0x90909090
TEMPLATE_NAME=elf32
GENERATE_SHLIB_SCRIPT=yes
NO_SMALL_DATA=yes
