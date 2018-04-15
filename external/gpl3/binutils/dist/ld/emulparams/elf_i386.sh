. ${srcdir}/emulparams/plt_unwind.sh
. ${srcdir}/emulparams/extern_protected_data.sh
. ${srcdir}/emulparams/dynamic_undefined_weak.sh
. ${srcdir}/emulparams/call_nop.sh
. ${srcdir}/emulparams/cet.sh
SCRIPT_NAME=elf
OUTPUT_FORMAT="elf32-i386"
NO_RELA_RELOCS=yes
TEXT_START_ADDR=0x08048000
MAXPAGESIZE="CONSTANT (MAXPAGESIZE)"
COMMONPAGESIZE="CONSTANT (COMMONPAGESIZE)"
ARCH=i386
MACHINE=
TEMPLATE_NAME=elf32
GENERATE_SHLIB_SCRIPT=yes
#ELFSIZE=32
GENERATE_PIE_SCRIPT=yes
NO_SMALL_DATA=yes
SEPARATE_GOTPLT="SIZEOF (.got.plt) >= 12 ? 12 : 0"
IREL_IN_PLT=

case "$target" in
  x86_64-*-netbsd*)
    case "$EMULATION_NAME" in
      *i386*)
       LIB_PATH='=/usr/lib/i386'
       ;;
    esac
    ;;
esac

# These sections are placed right after .plt section.
OTHER_PLT_SECTIONS="
.plt.got      ${RELOCATING-0} : { *(.plt.got) }
.plt.sec      ${RELOCATING-0} : { *(.plt.sec) }
"

# Linux modify the default library search path to first include
# a 32-bit specific directory.
case "$target" in
  x86_64*-linux* | i[3-7]86*-linux*)
    case "$EMULATION_NAME" in
      *i386*)
	LIBPATH_SUFFIX=32
	LIBPATH_SUFFIX_SKIP=64
	;;
    esac
    ;;
esac
