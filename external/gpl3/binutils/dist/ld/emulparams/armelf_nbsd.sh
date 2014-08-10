. ${srcdir}/emulparams/armelf.sh
MAXPAGESIZE="CONSTANT (MAXPAGESIZE)"
TEXT_START_ADDR=0x00010000
TARGET2_TYPE=got-rel

unset DATA_START_SYMBOLS
unset STACK_ADDR
unset EMBEDDED

case "$target" in
  arm*-*-netbsdelf*-*eabi*)
    LIB_PATH='=/usr/lib/oabi'
    ;;
esac
