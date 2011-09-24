SCRIPT_NAME=elf
ELFSIZE=64
OUTPUT_FORMAT="elf64-l1om"
NO_REL_RELOCS=yes
TEXT_START_ADDR=0x400000
MAXPAGESIZE="CONSTANT (MAXPAGESIZE)"
COMMONPAGESIZE="CONSTANT (COMMONPAGESIZE)"
ARCH="l1om"
MACHINE=
COMPILE_IN=yes
NOP=0x90909090
TEMPLATE_NAME=elf32
GENERATE_SHLIB_SCRIPT=yes
GENERATE_PIE_SCRIPT=yes
NO_SMALL_DATA=yes
LARGE_SECTIONS=yes
SEPARATE_GOTPLT=24

if [ "x${host}" = "x${target}" ]; then
  case " $EMULATION_LIBPATH " in
    *" ${EMULATION_NAME} "*)
      NATIVE=yes
  esac
fi

# Linux modifies the default library search path to first include
# a 64-bit specific directory.
case "$target" in
  l1om*-linux*)
    case "$EMULATION_NAME" in
      *l1om*) LIBPATH_SUFFIX=64 ;;
    esac
    ;;
esac
SCRIPT_NAME=elf
ELFSIZE=64
OUTPUT_FORMAT="elf64-l1om"
NO_REL_RELOCS=yes
TEXT_START_ADDR=0x400000
MAXPAGESIZE="CONSTANT (MAXPAGESIZE)"
COMMONPAGESIZE="CONSTANT (COMMONPAGESIZE)"
ARCH="l1om"
MACHINE=
COMPILE_IN=yes
NOP=0x90909090
TEMPLATE_NAME=elf32
GENERATE_SHLIB_SCRIPT=yes
GENERATE_PIE_SCRIPT=yes
NO_SMALL_DATA=yes
LARGE_SECTIONS=yes
SEPARATE_GOTPLT=24

if [ "x${host}" = "x${target}" ]; then
  case " $EMULATION_LIBPATH " in
    *" ${EMULATION_NAME} "*)
      NATIVE=yes
  esac
fi

# Linux modifies the default library search path to first include
# a 64-bit specific directory.
case "$target" in
  l1om*-linux*)
    case "$EMULATION_NAME" in
      *l1om*) LIBPATH_SUFFIX=64 ;;
    esac
    ;;
esac
