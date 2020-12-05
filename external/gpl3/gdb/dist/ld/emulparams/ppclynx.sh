source_sh ${srcdir}/emulparams/elf32ppc.sh

TEXT_BASE=0x00002000
DYN_TEXT_BASE=0x00400000
TEXT_START_ADDR="(DEFINED(_DYNAMIC) ? ${DYN_TEXT_BASE} : ${TEXT_BASE})"
case ${LD_FLAG} in
    n|N)	TEXT_START_ADDR=0x1000 ;;
esac
ELF_INTERPRETER_NAME=\"/usr/lib/ld.so.1\"

# Leave room of SIZEOF_HEADERS before text.
EMBEDDED=
