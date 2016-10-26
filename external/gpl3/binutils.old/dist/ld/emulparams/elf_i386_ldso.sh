. ${srcdir}/emulparams/plt_unwind.sh
. ${srcdir}/emulparams/extern_protected_data.sh
. ${srcdir}/emulparams/call_nop.sh
SCRIPT_NAME=elf
OUTPUT_FORMAT="elf32-i386"
NO_RELA_RELOCS=yes
TEXT_START_ADDR=0x08048000
MAXPAGESIZE="CONSTANT (MAXPAGESIZE)"
ARCH=i386
MACHINE=
TEMPLATE_NAME=elf32
GENERATE_SHLIB_SCRIPT=yes
ELF_INTERPRETER_NAME=\"/usr/lib/ld.so.1\"
NO_SMALL_DATA=yes
