SCRIPT_NAME=aout
OUTPUT_FORMAT="a.out-sunos-big"
BIG_OUTPUT_FORMAT="a.out-sunos-big"
LITTLE_OUTPUT_FORMAT="a.out-sparc-little"
TEXT_START_ADDR=0x2020
case ${LD_FLAG} in
    n|N)	TEXT_START_ADDR=0x2000 ;;
esac
TARGET_PAGE_SIZE=0x2000
ARCH=sparc
