/* config.h.  Generated automatically by configure.  */
/* config.in.  Generated automatically from configure.in by autoheader.  */

/* Whether malloc must be declared even if <stdlib.h> is included.  */
/* #undef NEED_DECLARATION_MALLOC */

/* Whether realloc must be declared even if <stdlib.h> is included.  */
/* #undef NEED_DECLARATION_REALLOC */

/* Whether free must be declared even if <stdlib.h> is included.  */
/* #undef NEED_DECLARATION_FREE */

/* Whether strerror must be declared even if <string.h> is included.  */
/* #undef NEED_DECLARATION_STRERROR */

/* Define if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
/* #undef _ALL_SOURCE */
#endif

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if the `long double' type works.  */
#define HAVE_LONG_DOUBLE 1

/* Define if you have a working `mmap' system call.  */
#define HAVE_MMAP 1

/* Define if on MINIX.  */
/* #undef _MINIX */

/* Define if the system does not provide POSIX.1 features except
   with this defined.  */
/* #undef _POSIX_1_SOURCE */

/* Define if you need to in order for stat and other things to work.  */
/* #undef _POSIX_SOURCE */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define if the `S_IS*' macros in <sys/stat.h> do not work properly.  */
/* #undef STAT_MACROS_BROKEN */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if fpregset_t type is available. */
/* #undef HAVE_FPREGSET_T */

/* Define if gregset_t type is available. */
/* #undef HAVE_GREGSET_T */

/* Define if the `long long' type works.  */
#define CC_HAS_LONG_LONG 1

/* Define if the "ll" format works to print long long ints. */
#define PRINTF_HAS_LONG_LONG 1

/* Define if the "%Lg" format works to print long doubles. */
#define PRINTF_HAS_LONG_DOUBLE 1

/* Define if the "%Lg" format works to scan long doubles. */
#define SCANF_HAS_LONG_DOUBLE 1

/* Define if using Solaris thread debugging.  */
/* #undef HAVE_THREAD_DB_LIB */

/* Define on a GNU/Linux system to work around problems in sys/procfs.h.  */
/* #undef START_INFERIOR_TRAPS_EXPECTED */
/* #undef sys_quotactl */

/* Define if you have HPUX threads */
/* #undef HAVE_HPUX_THREAD_SUPPORT */

/* Define if you want to use the memory mapped malloc package (mmalloc). */
/* #undef USE_MMALLOC */

/* Define if the runtime uses a routine from mmalloc before gdb has a chance
   to initialize mmalloc, and we want to force checking to be used anyway.
   This may cause spurious memory corruption messages if the runtime tries
   to explicitly deallocate that memory when gdb calls exit. */
/* #undef MMCHECK_FORCE */

/* Define if you have the getpagesize function.  */
#define HAVE_GETPAGESIZE 1

/* Define if you have the poll function.  */
#define HAVE_POLL 1

/* Define if you have the sbrk function.  */
#define HAVE_SBRK 1

/* Define if you have the select function.  */
#define HAVE_SELECT 1

/* Define if you have the setpgid function.  */
#define HAVE_SETPGID 1

/* Define if you have the sigaction function.  */
#define HAVE_SIGACTION 1

/* Define if you have the <endian.h> header file.  */
/* #undef HAVE_ENDIAN_H */

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <link.h> header file.  */
#define HAVE_LINK_H 1

/* Define if you have the <memory.h> header file.  */
#define HAVE_MEMORY_H 1

/* Define if you have the <objlist.h> header file.  */
/* #undef HAVE_OBJLIST_H */

/* Define if you have the <sgtty.h> header file.  */
#define HAVE_SGTTY_H 1

/* Define if you have the <stddef.h> header file.  */
#define HAVE_STDDEF_H 1

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <strings.h> header file.  */
#define HAVE_STRINGS_H 1

/* Define if you have the <sys/procfs.h> header file.  */
/* #undef HAVE_SYS_PROCFS_H */

/* Define if you have the <termio.h> header file.  */
/* #undef HAVE_TERMIO_H */

/* Define if you have the <termios.h> header file.  */
#define HAVE_TERMIOS_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the dl library (-ldl).  */
/* #undef HAVE_LIBDL */

/* Define if you have the m library (-lm).  */
#define HAVE_LIBM 1
