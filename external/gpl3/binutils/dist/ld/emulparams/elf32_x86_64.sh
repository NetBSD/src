. ${srcdir}/emulparams/plt_unwind.sh
SCRIPT_NAME=elf
ELFSIZE=32
OUTPUT_FORMAT="elf32-x86-64"
NO_REL_RELOCS=yes
TEXT_START_ADDR=0x400000
MAXPAGESIZE="CONSTANT (MAXPAGESIZE)"
COMMONPAGESIZE="CONSTANT (COMMONPAGESIZE)"
ARCH="i386:x64-32"
MACHINE=
TEMPLATE_NAME=elf32
GENERATE_SHLIB_SCRIPT=yes
GENERATE_PIE_SCRIPT=yes
NO_SMALL_DATA=yes
LARGE_SECTIONS=yes
SEPARATE_GOTPLT="SIZEOF (.got.plt) >= 24 ? 24 : 0"
IREL_IN_PLT=

if [ "x${host}" = "x${target}" ]; then
  case " $EMULATION_LIBPATH " in
    *" ${EMULATION_NAME} "*)
      NATIVE=yes
  esac
fi

# Linux modifies the default library search path to first include
# a 32-bit specific directory.
case "$target" in
  x86_64*-linux*|i[3-7]86-*-linux-*)
    case "$EMULATION_NAME" in
      *32*) LIBPATH_SUFFIX=x32 ;;
      *64*) LIBPATH_SUFFIX=64 ;;
    esac
    ;;
esac
