. ${srcdir}/emulparams/arc-endianness.sh
SCRIPT_NAME=arclinux
if [ "x${ARC_ENDIAN}" = "xbig" ]; then
  OUTPUT_FORMAT="elf32-bigarc"
else
  OUTPUT_FORMAT="elf32-littlearc"
fi
LITTLE_OUTPUT_FORMAT="elf32-littlearc"
BIG_OUTPUT_FORMAT="elf32-bigarc"
TEXT_START_ADDR=0x10000
MAXPAGESIZE=0x2000
COMMONPAGESIZE=0x2000
NONPAGED_TEXT_START_ADDR=0x10000
ARCH=arc
MACHINE=
ENTRY=__start
TEMPLATE_NAME=elf32
EXTRA_EM_FILE=arclinux
GENERATE_SHLIB_SCRIPT=yes
SDATA_START_SYMBOLS='__SDATA_BEGIN__ = .;'
OTHER_READONLY_SECTIONS="
  .__arc_profile_desc ${RELOCATING-0} : { *(.__arc_profile_desc) }
  .__arc_profile_forward ${RELOCATING-0} : { *(.__arc_profile_forward) }
"
OTHER_BSS_SECTIONS="
  .__arc_profile_counters ${RELOCATING-0} : { *(.__arc_profile_counters) }
"
