/* NetBSD/sparc64 ELF configuration */

/*
 * Pull in generic SPARC64 ELF configuration, and then clean up
 * afterwards
 */

/* Name the target CPU.  This must be before <sparc/sparc.h>. */
#ifndef TARGET_CPU_DEFAULT
#define TARGET_CPU_DEFAULT    TARGET_CPU_ultrasparc
#endif

#include <sparc/sp64-elf.h>

#undef CPP_PREDEFINES
#define CPP_PREDEFINES "-D__sparc__ -D__sparc64__ -D__arch64__ -D__sparc_v9__ -D__NetBSD__ -D__ELF__"

#undef LINK_SPEC
#define LINK_SPEC \
 "-m elf64_sparc \
  %{assert*} %{R*} \
  %{shared:-shared} \
  %{!shared: \
    -dc -dp \
    %{!nostdlib:%{!r*:%{!e*:-e __start}}} \
    %{!static: \
      %{rdynamic:-export-dynamic} \
      %{!dynamic-linker:-dynamic-linker /usr/libexec/ld.elf_so}} \
    %{static:-static}}"

/* Name the port. */
#undef TARGET_NAME
#define TARGET_NAME     "sparc64-netbsd"

#include <sparc/netbsd-elf-common.h>
