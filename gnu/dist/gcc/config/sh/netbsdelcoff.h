#include <sh/sh.h>

#undef CPP_SPEC
#define CPP_SPEC "%{posix:-D_POSIX_SOURCE} \
%{!mb:-D__LITTLE_ENDIAN__} \
%{m1:-D__sh1__} \
%{m2:-D__sh2__} \
%{m3:-D__sh3__} \
%{m3e:-D__SH3E__} \
%{!m1:%{!m2:%{!m3:%{!m3e:-D__sh1__}}}}"

#ifdef NETBSD_NATIVE

/* Look for the include files in the system-defined places.  */

#undef GPLUSPLUS_INCLUDE_DIR
#define GPLUSPLUS_INCLUDE_DIR "/usr/include/g++"

#undef GCC_INCLUDE_DIR
#define GCC_INCLUDE_DIR "/usr/include"

#undef INCLUDE_DEFAULTS
#define INCLUDE_DEFAULTS			\
  {						\
    { GPLUSPLUS_INCLUDE_DIR, "G++", 1, 1 },	\
    { GCC_INCLUDE_DIR, "GCC", 0, 0 },		\
    { 0, 0, 0, 0 }				\
  }

/* Under NetBSD, the normal location of the compiler back ends is the
   /usr/libexec directory.  */

#undef MD_EXEC_PREFIX
#define MD_EXEC_PREFIX			"/usr/libexec/"

/* Under NetBSD, the normal location of the various *crt*.o files is the
   /usr/lib directory.  */

#undef MD_STARTFILE_PREFIX
#define MD_STARTFILE_PREFIX		"/usr/lib/"

#endif

#undef CPP_PREDEFINES
#define CPP_PREDEFINES "-D__sh__ -D__sh3__ -D__NetBSD__ -Asystem(unix) -Asystem(NetBSD) -Acpu(sh) -Amachine(sh)"

/* XXX -fpic -fPIC? (msaitoh) */
#undef ASM_SPEC
#define ASM_SPEC  "%{!mb:-little} %{mrelax:-relax}"

#undef LIB_SPEC
#define LIB_SPEC "%{!p:%{!pg:-lc}}%{p:-lc_p}%{pg:-lc_p}"

#undef LINK_SPEC
#define LINK_SPEC \
  "%{!mb:-m shlunx} %{mrelax:-relax} %{!nostdlib:%{!r*:%{!e*:-e start}}} -dc -dp %{R*} %{static:-Bstatic} %{assert*}"

/* set default to little endian */

#undef TARGET_DEFAULT
#define TARGET_DEFAULT LITTLE_ENDIAN_BIT

/* We have atexit(3).  */

#define HAVE_ATEXIT

/* Implicit library calls should use memcpy, not bcopy, etc.  */

#define TARGET_MEM_FUNCTIONS

/* Handle #pragma weak and #pragma pack.  */

#define HANDLE_SYSV_PRAGMA


/* Make gcc agree with <machine/ansi.h> */

#undef SIZE_TYPE
#define SIZE_TYPE "unsigned int"

#undef PTRDIFF_TYPE
#define PTRDIFF_TYPE "int"

#undef WCHAR_TYPE
#define WCHAR_TYPE "int"

#undef WCHAR_UNSIGNED
#define WCHAR_UNSIGNED 0

#undef WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE 32

/* Until they use ELF or something that handles dwarf2 unwinds
   and initialization stuff better.  */
#undef DWARF2_UNWIND_INFO

/* XXX shouldn't use "1f"-style labels */
#undef FUNCTION_PROFILER
#define FUNCTION_PROFILER(STREAM,LABELNO)		\
{							\
	fprintf((STREAM), "\tmov.l\t11f,r1\n");		\
	fprintf((STREAM), "\tmova\t12f,r0\n");		\
	fprintf((STREAM), "\tjmp\t@r1\n");		\
	fprintf((STREAM), "\tnop\n");			\
	fprintf((STREAM), "\t.align\t2\n");		\
	fprintf((STREAM), "11:\t.long\t__mcount\n");	\
	fprintf((STREAM), "12:\n");			\
}
