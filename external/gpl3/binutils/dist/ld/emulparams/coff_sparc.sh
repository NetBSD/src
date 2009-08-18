SCRIPT_NAME=sparccoff
OUTPUT_FORMAT="coff-sparc"
# following are dubious (borrowed from sparc lynx)
TARGET_PAGE_SIZE=0x1000
TEXT_START_ADDR=0
case ${LD_FLAG} in
    n|N)	TEXT_START_ADDR=0x1000 ;;
esac
ARCH=sparc
