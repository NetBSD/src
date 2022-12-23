# If you change this file, please also look at files which source this one:
# elf32_sparc_vxworks.sh

SCRIPT_NAME=elf
OUTPUT_FORMAT="elf32-sparc"
NO_REL_RELOCS=yes
TEXT_START_ADDR=0x10000
MAXPAGESIZE="CONSTANT (MAXPAGESIZE)"
COMMONPAGESIZE="CONSTANT (COMMONPAGESIZE)"
ALIGNMENT=8
ARCH=sparc
MACHINE=
TEMPLATE_NAME=elf
DATA_PLT=
GENERATE_SHLIB_SCRIPT=yes
#ELFSIZE=32
GENERATE_PIE_SCRIPT=yes
NOP=0x01000000
NO_SMALL_DATA=yes

case "$target" in
  sparc64-*-netbsd*)
    case "$EMULATION_NAME" in
      *32*)
	LIB_PATH='=/usr/lib/sparc'
	;;
    esac
    ;;
esac
