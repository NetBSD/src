source_sh ${srcdir}/emulparams/elf64_sparc.sh
source_sh ${srcdir}/emulparams/solaris2.sh
TEXT_START_ADDR=0x100000000
EXTRA_EM_FILE=solaris2
OUTPUT_FORMAT="elf64-sparc-sol2"
LIBPATH_SUFFIX=/sparcv9
LIBPATH_SKIP_NONNATIVE=yes
ELF_INTERPRETER_NAME=\"/usr/lib/sparcv9/ld.so.1\"
