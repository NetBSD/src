/* NetBSD/sparc ELF configuration */

/*
 * Pull in generic SPARC ELF configuration, and then clean up
 * afterwards
 */
#include <sparc/elf.h>

/* Name the target CPU. */
#ifndef TARGET_CPU_DEFAULT
#define TARGET_CPU_DEFAULT	TARGET_CPU_sparc
#endif

#undef MULDI3_LIBCALL
#undef DIVDI3_LIBCALL
#undef UDIVDI3_LIBCALL
#undef MODDI3_LIBCALL
#undef UMODDI3_LIBCALL
#undef INIT_SUBTARGET_OPTABS  
#define INIT_SUBTARGET_OPTABS  

#undef CPP_PREDEFINES
#define CPP_PREDEFINES "\
-D__sparc__ -D__sparc -D__NetBSD__ -D__ELF__ \
-Asystem(unix) -Asystem(NetBSD) -Acpu(sparc) -Amachine(sparc)"

#include <sparc/netbsd-elf-common.h>

#undef LINK_SPEC
#define LINK_SPEC \
 "-m elf32_sparc \
  %{assert*} %{R*} \
  %{shared:-shared} \
  %{!shared: \
    -dy -dc -dp \
    %{!nostdlib:%{!r*:%{!e*:-e __start}}} \
    %{!static: \
      %{rdynamic:-export-dynamic} \
      %{!dynamic-linker:-dynamic-linker /usr/libexec/ld.elf_so}} \
    %{static:-static}}"

/* Name the port. */
#undef TARGET_NAME
#define TARGET_NAME     "sparc-netbsdelf"
