/*	$NetBSD: config.h,v 1.4 1999/02/02 20:00:39 tv Exp $	*/

/****************/
/* bfd config.h */
/****************/

/* Name of package.  */
#define PACKAGE "bfd"

/* Version of package.  */
#define VERSION "2.9.1"

/* Whether strstr must be declared even if <string.h> is included.  */
/* #undef NEED_DECLARATION_STRSTR */

/* Whether malloc must be declared even if <stdlib.h> is included.  */
/* #undef NEED_DECLARATION_MALLOC */

/* Whether realloc must be declared even if <stdlib.h> is included.  */
/* #undef NEED_DECLARATION_REALLOC */

/* Whether free must be declared even if <stdlib.h> is included.  */
/* #undef NEED_DECLARATION_FREE */

/* Whether getenv must be declared even if <stdlib.h> is included.  */
/* #undef NEED_DECLARATION_GETENV */

/* Define if you have a working `mmap' system call.  */
#define HAVE_MMAP 1

/* Define if you need to in order for stat and other things to work.  */
/* #undef _POSIX_SOURCE */

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Do we need to use the b modifier when opening binary files?  */
/* #undef USE_BINARY_FOPEN */

/* Name of host specific header file to include in trad-core.c.  */
/* #undef TRAD_HEADER */

/* Define only if <sys/procfs.h> is available *and* it defines prstatus_t.  */
/* #undef HAVE_SYS_PROCFS_H */

/* Do we really want to use mmap if it's available?  */
#define USE_MMAP 1

/* Define if you have the fcntl function.  */
#define HAVE_FCNTL 1

/* Define if you have the fdopen function.  */
#define HAVE_FDOPEN 1

/* Define if you have the getpagesize function.  */
#define HAVE_GETPAGESIZE 1

/* Define if you have the madvise function.  */
#define HAVE_MADVISE 1

/* Define if you have the mprotect function.  */
#define HAVE_MPROTECT 1

/* Define if you have the setitimer function.  */
#define HAVE_SETITIMER 1

/* Define if you have the sysconf function.  */
#define HAVE_SYSCONF 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <stddef.h> header file.  */
#define HAVE_STDDEF_H 1

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <strings.h> header file.  */
#define HAVE_STRINGS_H 1

/* Define if you have the <sys/file.h> header file.  */
#define HAVE_SYS_FILE_H 1

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <time.h> header file.  */
#define HAVE_TIME_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/**********************/
/* libiberty config.h */
/**********************/

/* Define if using alloca.c.  */
/* #undef C_ALLOCA */

/* Define to one of _getb67, GETB67, getb67 for Cray-2 and Cray-YMP systems.
   This function is required for alloca.c support on those systems.  */
/* #undef CRAY_STACKSEG_END */

/* Define if you have alloca, as a function or macro.  */
#define HAVE_ALLOCA 1

/* Define if you have <alloca.h> and it should be used (not on Ultrix).  */
/* #undef HAVE_ALLOCA_H */

/* Define if you have <vfork.h>.  */
/* #undef HAVE_VFORK_H */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef pid_t */

/* Define if you need to in order for stat and other things to work.  */
/* #undef _POSIX_SOURCE */

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
 STACK_DIRECTION > 0 => grows toward higher addresses
 STACK_DIRECTION < 0 => grows toward lower addresses
 STACK_DIRECTION = 0 => direction of growth unknown
 */
/* #undef STACK_DIRECTION */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define vfork as fork if vfork does not work.  */
/* #undef vfork */

/* Define if you have the sys_errlist variable.  */
#define HAVE_SYS_ERRLIST 1

/* Define if you have the sys_nerr variable.  */
#define HAVE_SYS_NERR 1

/* Define if you have the sys_siglist variable.  */
#define HAVE_SYS_SIGLIST 1

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the asprintf function.  */
/* #undef HAVE_ASPRINTF */

/* Define if you have the atexit function.  */
#define HAVE_ATEXIT 1

/* Define if you have the basename function.  */
#define HAVE_BASENAME 1

/* Define if you have the bcmp function.  */
#define HAVE_BCMP 1

/* Define if you have the bcopy function.  */
#define HAVE_BCOPY 1

/* Define if you have the bzero function.  */
#define HAVE_BZERO 1

/* Define if you have the clock function.  */
#define HAVE_CLOCK 1

/* Define if you have the getcwd function.  */
#define HAVE_GETCWD 1

/* Define if you have the getpagesize function.  */
#define HAVE_GETPAGESIZE 1

/* Define if you have the getrusage function.  */
#define HAVE_GETRUSAGE 1

/* Define if you have the index function.  */
#define HAVE_INDEX 1

/* Define if you have the insque function.  */
/* #undef HAVE_INSQUE */

/* Define if you have the memchr function.  */
#define HAVE_MEMCHR 1

/* Define if you have the memcmp function.  */
#define HAVE_MEMCMP 1

/* Define if you have the memcpy function.  */
#define HAVE_MEMCPY 1

/* Define if you have the memmove function.  */
#define HAVE_MEMMOVE 1

/* Define if you have the memset function.  */
#define HAVE_MEMSET 1

/* Define if you have the on_exit function.  */
/* #undef HAVE_ON_EXIT */

/* Define if you have the psignal function.  */
#define HAVE_PSIGNAL 1

/* Define if you have the random function.  */
#define HAVE_RANDOM 1

/* Define if you have the rename function.  */
#define HAVE_RENAME 1

/* Define if you have the rindex function.  */
#define HAVE_RINDEX 1

/* Define if you have the sigsetmask function.  */
#define HAVE_SIGSETMASK 1

/* Define if you have the strcasecmp function.  */
#define HAVE_STRCASECMP 1

/* Define if you have the strchr function.  */
#define HAVE_STRCHR 1

/* Define if you have the strdup function.  */
#define HAVE_STRDUP 1

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the strncasecmp function.  */
#define HAVE_STRNCASECMP 1

/* Define if you have the strrchr function.  */
#define HAVE_STRRCHR 1

/* Define if you have the strsignal function.  */
#define HAVE_STRSIGNAL 1

/* Define if you have the strstr function.  */
#define HAVE_STRSTR 1

/* Define if you have the strtod function.  */
#define HAVE_STRTOD 1

/* Define if you have the strtol function.  */
#define HAVE_STRTOL 1

/* Define if you have the strtoul function.  */
#define HAVE_STRTOUL 1

/* Define if you have the sysconf function.  */
#define HAVE_SYSCONF 1

/* Define if you have the times function.  */
#define HAVE_TIMES 1

/* Define if you have the tmpnam function.  */
#define HAVE_TMPNAM 1

/* Define if you have the vasprintf function.  */
/* #undef HAVE_VASPRINTF */

/* Define if you have the vfprintf function.  */
#define HAVE_VFPRINTF 1

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF 1

/* Define if you have the vsprintf function.  */
#define HAVE_VSPRINTF 1

/* Define if you have the waitpid function.  */
#define HAVE_WAITPID 1

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <sys/file.h> header file.  */
#define HAVE_SYS_FILE_H 1

/* Define if you have the <sys/param.h> header file.  */
#define HAVE_SYS_PARAM_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1
