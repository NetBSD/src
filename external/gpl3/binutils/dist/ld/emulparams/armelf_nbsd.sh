. ${srcdir}/emulparams/armelf.sh
MAXPAGESIZE="CONSTANT (MAXPAGESIZE)"
TEXT_START_ADDR=0x00010000
TARGET2_TYPE=got-rel
GENERATE_PIE_SCRIPT=yes

unset DATA_START_SYMBOLS
unset STACK_ADDR
unset EMBEDDED

case "$target" in
  aarch64*-*-netbsd* | arm*-*-netbsdelf*-*eabi*)
    case "$EMULATION_NAME" in
    armelf*_nbsd)
      LIB_PATH='=/usr/lib/oabi'
      ;;
    esac
    ;;
esac
