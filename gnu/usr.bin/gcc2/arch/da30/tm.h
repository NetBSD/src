/*	$Id: tm.h,v 1.1 1993/11/23 06:09:42 paulus Exp $ */

#include <machine/ansi.h>
#include "da30/m68k.h"

/* See m68k.h.  7 means 68020 with 68881.  */

#define TARGET_DEFAULT 7

/* Define __HAVE_68881__ in preprocessor, unless -msoft-float is specified.
   This will control the use of inline 68881 insns in certain macros.  */

#define CPP_SPEC "%{!msoft-float:-D__HAVE_68881__ -D__HAVE_FPU__} %{posix:-D_POSIX_SOURCE}"

/* Names to predefine in the preprocessor for this target machine.  */

#define CPP_PREDEFINES "-Dmc68000 -Dmc68020 -Dunix -D__BSD_NET2__ -D__NetBSD__ -Dda30"

/* Specify -k to assembler for PIC code generation. */

#define ASM_SPEC "%{fpic:-k} %{fPIC:-k}"

/* Support -static, -symbolic and -shared options (at least minimally).
   Also use -dp when doing dynamic linking.  Don't include a startup
   file when linking a shared library. */

#define LINK_SPEC	\
  "%{static:-Bstatic} %{shared:-Bshareable} %{symbolic:-Bsymbolic} \
   %{!static:%{!shared:-dp}}"

#define STARTFILE_SPEC  \
  "%{!shared:%{pg:gcrt0.o%s}%{!pg:%{p:mcrt0.o%s}%{!p:crt0.o%s}}}"

/* No more libg.a; no libraries if making shared object */

#define LIB_SPEC "%{!shared:%{!p:%{!pg:-lc}}%{p:-lc_p}%{pg:-lc_p}}"

/* Make gcc agree with <machine/ansi.h> */

#undef WCHAR_TYPE
#define WCHAR_TYPE	"short unsigned int"
#define WCHAR_UNSIGNED	1
#undef WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE	16

/* NetBSD does have atexit.  */

#define HAVE_ATEXIT

/* Every structure or union's size must be a multiple of 2 bytes.  */

#define STRUCTURE_SIZE_BOUNDARY 16

/* This is BSD, so it wants DBX format.  */

#define DBX_DEBUGGING_INFO

/* Do not break .stabs pseudos into continuations.  */

#define DBX_CONTIN_LENGTH 0

/* This is the char to use for continuation (in case we need to turn
   continuation back on).  */

#define DBX_CONTIN_CHAR '?'

/* Don't use the `xsfoo;' construct in DBX output; this system
   doesn't support it.  */

#define DBX_NO_XREFS

/* Don't default to pcc-struct-return, because gcc is the only compiler, and
   we want to retain compatibility with older gcc versions.  */
#define DEFAULT_PCC_STRUCT_RETURN 0

/*
 * Some imports from svr4.h in support of shared libraries.
 * Currently, we need the DECLARE_OBJECT_SIZE stuff.
 */

#define SIZE_ASM_OP	".size"

#define ASM_DECLARE_OBJECT_NAME(FILE, NAME, DECL)			\
  do {									\
    if (!flag_inhibit_size_directive)					\
      {									\
	fprintf (FILE, "\t%s ", SIZE_ASM_OP);				\
	assemble_name (FILE, NAME);					\
	fprintf (FILE, ",%d\n",  int_size_in_bytes (TREE_TYPE (decl)));	\
      }									\
    ASM_OUTPUT_LABEL(FILE, NAME);					\
  } while (0)
