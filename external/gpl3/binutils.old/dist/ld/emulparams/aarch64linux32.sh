ARCH="aarch64:ilp32"
MACHINE=
NOP=0

SCRIPT_NAME=elf
ELFSIZE=32
OUTPUT_FORMAT="elf32-littleaarch64"
BIG_OUTPUT_FORMAT="elf32-bigaarch64"
LITTLE_OUTPUT_FORMAT="elf32-littleaarch64"
NO_REL_RELOCS=yes

TEMPLATE_NAME=elf32
EXTRA_EM_FILE=aarch64elf

GENERATE_SHLIB_SCRIPT=yes
GENERATE_PIE_SCRIPT=yes

MAXPAGESIZE="CONSTANT (MAXPAGESIZE)"
COMMONPAGESIZE="CONSTANT (COMMONPAGESIZE)"
SEPARATE_GOTPLT=12
IREL_IN_PLT=

TEXT_START_ADDR=0x400000

DATA_START_SYMBOLS='PROVIDE (__data_start = .);';

# AArch64 does not support .s* sections.
NO_SMALL_DATA=yes

OTHER_BSS_SYMBOLS='__bss_start__ = .;'
OTHER_BSS_END_SYMBOLS='_bss_end__ = . ; __bss_end__ = . ;'
OTHER_END_SYMBOLS='__end__ = . ;'

OTHER_SECTIONS='.note.gnu.arm.ident 0 : { KEEP (*(.note.gnu.arm.ident)) }'
ATTRS_SECTIONS='.ARM.attributes 0 : { KEEP (*(.ARM.attributes)) KEEP (*(.gnu.attributes)) }'
# Ensure each PLT entry is aligned to a cache line.
PLT=".plt          ${RELOCATING-0} : ALIGN(16) { *(.plt)${IREL_IN_PLT+ *(.iplt)} }"

# Linux modifies the default library search path to first include
# a 32-bit specific directory.
case "$target" in
  aarch64*-linux*)
    case "$EMULATION_NAME" in
      aarch64linux*) LIBPATH_SUFFIX=ilp32 ;;
    esac
    ;;
esac

ELF_INTERPRETER_NAME=\"/lib/ld-linux-aarch64_ilp32.so.1\"
