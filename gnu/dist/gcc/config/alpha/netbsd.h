/* Get generic alpha definitions. */

#include <alpha/alpha.h>

/* Get generic alpha ELF definitions. */

#include <alpha/elf.h>

/* Get generic NetBSD definitions. */

#define NETBSD_ELF
#include <netbsd.h>

/* Names to predefine in the preprocessor for this target machine.
   XXX NetBSD, by convention, shouldn't do __alpha, but lots of applications
   expect it because that's what OSF/1 does. */

#undef CPP_PREDEFINES
#define CPP_PREDEFINES "-D__alpha__ -D__alpha -D__NetBSD__ -D__ELF__ -D__KPRINTF_ATTRIBUTE__ -Asystem(unix) -Asystem(NetBSD) -Acpu(alpha) -Amachine(alpha)"

/* Make gcc agree with <machine/ansi.h> */

#undef WCHAR_TYPE
#define WCHAR_TYPE "int"

#undef WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE 32

/* This is BSD, so it wants DBX format.  */
#define DBX_DEBUGGING_INFO

/* This is the char to use for continuation (in case we need to turn
   continuation back on).  */

#define DBX_CONTIN_CHAR '?'

/* Name the port */
#define	TARGET_NAME	"alpha-netbsd"

/* XXX Override this back to the default; <alpha/elf.h> mucks with it
   for Linux/Alpha. */

#undef TARGET_VERSION
#define	TARGET_VERSION	fprintf (stderr, " (%s)", TARGET_NAME)

/* Output assembler code to FILE to increment profiler label # LABELNO
   for profiling a function entry.  Under NetBSD/Alpha, the assembler does
   nothing special with -pg. */

#undef FUNCTION_PROFILER
#define FUNCTION_PROFILER(FILE, LABELNO)                        \
fputs ("\tjsr $28,_mcount\n", (FILE)); /* at */

/* Show that we need a GP when profiling.  */
#define TARGET_PROFILING_NEEDS_GP

#define bsd4_4
#undef HAS_INIT_SECTION

#undef PREFERRED_DEBUGGING_TYPE
#define PREFERRED_DEBUGGING_TYPE DBX_DEBUG

/* Provide an ASM_SPEC appropriate for a NetBSD/alpha target.  This differs
   from the generic NetBSD ASM_SPEC in that no special handling of PIC is
   necessary on the Alpha. */

#undef ASM_SPEC
#define	ASM_SPEC " %|"

/* Provide a LINK_SPEC appropriate for a NetBSD/alpha ELF target.  Only
   the linker emulation and -O options are Alpha-specific.  The rest are
   common to all ELF targets, except for the name of the start function. */

#undef LINK_SPEC
#define	LINK_SPEC \
 "-m elf64alpha \
  %{O*:-O3} %{!O*:-O1} \
  %{assert*} \
  %{shared:-shared} \
  %{!shared: \
    -dc -dp \
    %{!nostdlib:%{!r*:%{!e*:-e __start}}} \
    %{!static:
      %{rdynamic:-export-dynamic} \
      %{!dynamic-linker:-dynamic-linker /usr/libexec/ld.elf_so}} \
    %{static:-static}}"
