source_sh ${srcdir}/emulparams/elf_x86_64.sh
source_sh ${srcdir}/emulparams/solaris2.sh
EXTRA_EM_FILE="solaris2-x86-64"
OUTPUT_FORMAT="elf64-x86-64-sol2"
LIBPATH_SUFFIX=/amd64
LIBPATH_SKIP_NONNATIVE=yes
ELF_INTERPRETER_NAME=\"/usr/lib/amd64/ld.so.1\"
