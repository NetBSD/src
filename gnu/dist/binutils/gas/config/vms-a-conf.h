/* vms-alpha-conf.h.  Generated manually from conf.in,
   and used by config-gas-alpha.com when constructing config.h.  */

/* Define if using alloca.c.  */
#ifdef __GNUC__
#undef C_ALLOCA
#else
#define C_ALLOCA
#endif

/* Define to one of _getb67, GETB67, getb67 for Cray-2 and Cray-YMP systems.
   This function is required for alloca.c support on those systems.  */
#undef CRAY_STACKSEG_END

/* Define if you have <alloca.h> and it should be used (not on Ultrix).  */
#undef HAVE_ALLOCA_H

/* Define as __inline if that's what the C compiler calls it.  */
#ifdef __GNUC__
#undef inline
#else
#define inline
#endif

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown
 */
#define STACK_DIRECTION (-1)

/* Should gas use high-level BFD interfaces?  */
#define BFD_ASSEMBLER

/* Some assert/preprocessor combinations are incapable of handling
   certain kinds of constructs in the argument of assert.  For example,
   quoted strings (if requoting isn't done right) or newlines.  */
#ifdef __GNUC__
#undef BROKEN_ASSERT
#else
#define BROKEN_ASSERT
#endif

/* If we aren't doing cross-assembling, some operations can be optimized,
   since byte orders and value sizes don't need to be adjusted.  */
#undef CROSS_COMPILE

/* Some gas code wants to know these parameters.  */
#define TARGET_ALIAS	"alpha-vms"
#define TARGET_CPU	"alpha"
#define TARGET_CANONICAL	"alpha-dec-vms"
#define TARGET_OS	"openVMS/Alpha"
#define TARGET_VENDOR	"dec"

/* Sometimes the system header files don't declare malloc and realloc.  */
#undef NEED_DECLARATION_MALLOC

/* Sometimes the system header files don't declare free.  */
#undef NEED_DECLARATION_FREE

/* Sometimes errno.h doesn't declare errno itself.  */
#undef NEED_DECLARATION_ERRNO

#undef MANY_SEGMENTS

/* Needed only for sparc configuration */
#undef sparcv9

/* Define if you have the remove function.  */
#define HAVE_REMOVE

/* Define if you have the unlink function.  */
#undef HAVE_UNLINK

/* Define if you have the <errno.h> header file.  */
#define HAVE_ERRNO_H

/* Define if you have the <memory.h> header file.  */
#undef HAVE_MEMORY_H

/* Define if you have the <stdarg.h> header file.  */
#define HAVE_STDARG_H

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H

/* Define if you have the <strings.h> header file.  */
#undef HAVE_STRINGS_H

/* Define if you have the <sys/types.h> header file.  */
#ifdef __GNUC__
#define HAVE_SYS_TYPES_H
#else
#undef HAVE_SYS_TYPES_H
#endif

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H	/* config-gas.com will make one if necessary */

/* Define if you have the <varargs.h> header file.  */
#undef HAVE_VARARGS_H

/* VMS-specific:  we need to set up EXIT_xxx here because the default
   values in as.h are inappropriate for VMS, but we also want to prevent
   as.h's inclusion of <stdlib.h> from triggering redefinition warnings.
   <stdlib.h> guards itself against multiple inclusion, so including it
   here turns as.h's later #include into a no-op.  (We can't simply use
   #ifndef HAVE_STDLIB_H here, because the <stdlib.h> in several older
   gcc-vms distributions neglects to define these two required macros.)  */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if __DECC
#undef EXIT_SUCCESS
#undef EXIT_FAILURE
#define EXIT_SUCCESS 1			/* SS$_NORMAL, STS$K_SUCCESS */
#define EXIT_FAILURE 0x10000002		/* (STS$K_ERROR | STS$M_INHIB_MSG) */
#endif

#include <unixlib.h>
#if __DECC
extern int strcasecmp ();
extern int strncasecmp ();
#endif
