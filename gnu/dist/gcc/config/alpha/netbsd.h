/* XXX */

#include "alpha/alpha.h"

#undef	WCHAR_TYPE
#define	WCHAR_TYPE "int"

#undef	WCHAR_TYPE_SIZE
#define	WCHAR_TYPE_SIZE 32

/* NetBSD-specific things: */

#undef CPP_PREDEFINES
#define CPP_PREDEFINES "-D__NetBSD__ -D__alpha__ -D__alpha"

/* Look for the include files in the system-defined places.  */

#undef GPLUSPLUS_INCLUDE_DIR
#define GPLUSPLUS_INCLUDE_DIR "/usr/include/g++"

#undef GCC_INCLUDE_DIR
#define GCC_INCLUDE_DIR "/usr/include"

#undef INCLUDE_DEFAULTS
#define INCLUDE_DEFAULTS                \
  {                                     \
    { GPLUSPLUS_INCLUDE_DIR, 1, 1 },    \
    { GCC_INCLUDE_DIR, 0, 0 },          \
    { 0, 0, 0 }                         \
  }


/* Under NetBSD, the normal location of the `ld' and `as' programs is the
   /usr/bin directory.  */

#undef MD_EXEC_PREFIX
#define MD_EXEC_PREFIX "/usr/bin/"

/* Under NetBSD, the normal location of the various *crt*.o files is the
   /usr/lib directory.  */

#undef MD_STARTFILE_PREFIX
#define MD_STARTFILE_PREFIX "/usr/lib/"


/* Provide a CPP_SPEC appropriate for NetBSD.  Current we just deal with
   the GCC option `-posix'.  */

#undef CPP_SPEC
#define CPP_SPEC "%{posix:-D_POSIX_SOURCE}"

/* Provide an ASM_SPEC appropriate for NetBSD. */

#undef ASM_SPEC
#define ASM_SPEC " %|"

#undef ASM_FINAL_SPEC

/* Provide a LIB_SPEC appropriate for NetBSD.  Just select the appropriate
   libc, depending on whether we're doing profiling.  */

#undef LIB_SPEC
#define LIB_SPEC "%{!p:%{!pg:-lc}}%{p:-lc_p}%{pg:-lc_p}"

/* Provide a LINK_SPEC appropriate for NetBSD.  Here we provide support
   for the special GCC options -static, -assert, and -nostdlib.  */

#undef LINK_SPEC
#define LINK_SPEC \
  "%{!nostdlib:%{!r*:%{!e*:-e __start}}} -dc -dp %{static:-Bstatic} %{assert*}"

/* Output assembler code to FILE to increment profiler label # LABELNO
   for profiling a function entry.  Under NetBSD/Alpha, the assembler does
   nothing special with -pg. */

#undef FUNCTION_PROFILER
#define FUNCTION_PROFILER(FILE, LABELNO)			\
	fputs ("\tjsr $28,_mcount\n", (FILE)); /* at */

/* Show that we need a GP when profiling.  */
#define TARGET_PROFILING_NEEDS_GP

#define bsd4_4
#undef HAS_INIT_SECTION

#undef PREFERRED_DEBUGGING_TYPE
#define PREFERRED_DEBUGGING_TYPE DBX_DEBUG
