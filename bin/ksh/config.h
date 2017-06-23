/*	$NetBSD: config.h,v 1.37 2017/06/23 00:29:42 kamil Exp $	*/

/* config.h.  Generated automatically by configure.  */
/* config.h.in.  Generated automatically from configure.in by autoheader.  */
/*
 * This file, acconfig.h, which is a part of pdksh (the public domain ksh),
 * is placed in the public domain.  It comes with no licence, warranty
 * or guarantee of any kind (i.e., at your own risk).
 */

#ifndef CONFIG_H
#define CONFIG_H

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef gid_t */

/* Define if your struct stat has st_rdev.  */
#define HAVE_ST_RDEV 1

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#define HAVE_SYS_WAIT_H 1

/* Define if on MINIX.  */
/* #undef _MINIX */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef mode_t */

/* Define to `long' if <sys/types.h> doesn't define.  */
/* #undef off_t */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef pid_t */

/* Define if the system does not provide POSIX.1 features except
   with this defined.  */
/* #undef _POSIX_1_SOURCE */

/* Define if you need to in order for stat and other things to work.  */
/* #undef _POSIX_SOURCE */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define if the `S_IS*' macros in <sys/stat.h> do not work properly.  */
/* #undef STAT_MACROS_BROKEN */

/* Define if `sys_siglist' is declared by <signal.h>.  */
#define SYS_SIGLIST_DECLARED 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef uid_t */

/* Define if your kernel doesn't handle scripts starting with #! */
/* #undef SHARPBANG */

/* Define as the return value of signal handlers (0 or ).  */
#define RETSIGVAL 

/* Define if you have posix signal routines (sigaction(), et. al.) */
#define POSIX_SIGNALS 1

/* Define to use the fake posix signal routines (sigact.[ch]) */
/* #undef USE_FAKE_SIGACT */

/* Define if signals don't interrupt read() */
/* #undef SIGNALS_DONT_INTERRUPT */

/* Define if you have bsd versions of the setpgrp() and getpgrp() routines */
/* #undef BSD_PGRP */

/* Define if you have POSIX versions of the setpgid() and getpgrp() routines */
#define POSIX_PGRP 1

/* Define if you have sysV versions of the setpgrp() and getpgrp() routines */
/* #undef SYSV_PGRP */

/* Define if you don't have setpgrp(), setpgid() or getpgrp() routines */
/* #undef NO_PGRP */

/* Define if C compiler groks function prototypes */
#define HAVE_PROTOTYPES 1

/* Define if C compiler groks __attribute__((...)) (const, noreturn, format) */
#define HAVE_GCC_FUNC_ATTR 1

/* Define to 32-bit signed integer type if <sys/types.h> doesn't define */
/* #undef clock_t */

/* Define to the type of struct rlimit fields if the rlim_t type is missing */
/* #undef rlim_t */

/* Define if time() is declared in <time.h> */
#define TIME_DECLARED 1

/* Define to `unsigned' if <signal.h> doesn't define */
/* #undef sigset_t */

/* Define if sys_errlist[] and sys_nerr are in the C library */
#define HAVE_SYS_ERRLIST 1

/* Define if sys_errlist[] and sys_nerr are defined in <errno.h> */
#define SYS_ERRLIST_DECLARED 1

/* Define if sys_siglist[] is in the C library */
#define HAVE_SYS_SIGLIST 1

/* Define if you have a sane <termios.h> header file */
#define HAVE_TERMIOS_H 1

/* Define if you have a lstat() function in your C library */
#define HAVE_LSTAT 1

/* Define if you have a sane <termio.h> header file */
/* #undef HAVE_TERMIO_H */

/* Define if you don't have times() or if it always returns 0 */
/* #undef TIMES_BROKEN */

/* Define if opendir() will open non-directory files */
/* #undef OPENDIR_DOES_NONDIR */

/* Define if the pgrp of setpgrp() can't be the pid of a zombie process */
/* #undef NEED_PGRP_SYNC */

/* Define if you have a POSIX.1 compatible <sys/wait.h> */
#define POSIX_SYS_WAIT 1

/* Define if your OS maps references to /dev/fd/n to file descriptor n */
#define HAVE_DEV_FD 1

/* Default PATH */
#ifdef RESCUEDIR
#define DEFAULT_PATH RESCUEDIR ":/bin:/usr/bin:/sbin:/usr/sbin"
#else
#define DEFAULT_PATH "/bin:/usr/bin:/sbin:/usr/sbin"
#endif

/* Include ksh features? */
#define KSH 1

/* Include emacs editing? */
#define EMACS 1

/* Include vi editing? */
#define VI 1

/* Include job control? */
#define JOBS 1

/* Include brace-expansion? */
#define BRACE_EXPAND 1

/* Include any history? */
#define HISTORY 1

/* Include complex history? */
/* #undef COMPLEX_HISTORY */

/* Strict POSIX behaviour? */
#define POSIXLY_CORRECT 1

/* Specify default $ENV? */
#define DEFAULT_ENV	"$HOME/.kshrc"

/* Include shl(1) support? */
/* #undef SWTCH */

/* Include game-of-life? */
/* #undef SILLY */

/* Define if you have the _setjmp function.  */
#define HAVE__SETJMP

/* Define if you have the confstr function.  */
#define HAVE_CONFSTR 1

/* Define if you have the flock function.  */
#define HAVE_FLOCK 1

/* Define if you have the getcwd function.  */
#define HAVE_GETCWD 1

/* Define if you have the getgroups function.  */
#define HAVE_GETGROUPS

/* Define if you have the getpagesize function.  */
#define HAVE_GETPAGESIZE 1

/* Define if you have the getrusage function.  */
#define HAVE_GETRUSAGE

/* Define if you have the getwd function.  */
#define HAVE_GETWD 1

/* Define if you have the killpg function.  */
#define HAVE_KILLPG 1

/* Define if you have the nice function.  */
#define HAVE_NICE 1

/* Define if you have the setrlimit function.  */
#define HAVE_SETRLIMIT 1

/* Define if you have the sigsetjmp function.  */
#define HAVE_SIGSETJMP 1

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the sysconf function.  */
#define HAVE_SYSCONF 1

/* Define if you have the tcsetpgrp function.  */
#define HAVE_TCSETPGRP 1

/* Define if you have the ulimit function.  */
#define HAVE_ULIMIT

/* Define if you have the valloc function.  */
#define HAVE_VALLOC 1

/* Define if you have the wait3 function.  */
#define HAVE_WAIT3 1

/* Define if you have the waitpid function.  */
#define HAVE_WAITPID 1

/* Define if you have the <dirent.h> header file.  */
#define HAVE_DIRENT_H 1

/* Define if you have the <ndir.h> header file.  */
/* #undef HAVE_NDIR_H */

/* Define if you have the <paths.h> header file.  */
#define HAVE_PATHS_H 1

/* Define if you have the <sys/dir.h> header file.  */
#define HAVE_SYS_DIR_H

/* Define if you have the <sys/ndir.h> header file.  */
/* #undef HAVE_SYS_NDIR_H */

/* Define if you have the <sys/param.h> header file.  */
#define HAVE_SYS_PARAM_H 1

/* Define if you have the <sys/resource.h> header file.  */
#define HAVE_SYS_RESOURCE_H 1

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <sys/wait.h> header file.  */
#define HAVE_SYS_WAIT_H 1

/* Define if you have the <ulimit.h> header file.  */
#define HAVE_ULIMIT_H

/* Define if you have the <values.h> header file.  */
/* #undef HAVE_VALUES_H */

/* Need to use a separate file to keep the configure script from commenting
 * out the undefs....
 */
#include "conf-end.h"

#endif /* CONFIG_H */
