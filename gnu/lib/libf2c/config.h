/*	$NetBSD: config.h,v 1.1 1998/04/03 21:04:34 tv Exp $	*/

#ifndef __CONFIG_H__
#define __CONFIG_H__

/*
 * Manually generated from config.h.in for use in a native libf2c compile.
 */

#ifdef NETBSD_NATIVE

/* Define to empty if the keyword does not work.  */
/*#undef const*/

/* Define if your struct stat has st_blksize.  */
#define HAVE_ST_BLKSIZE 1

/* Define if your struct stat has st_blocks.  */
#define HAVE_ST_BLOCKS 1

/* Define if your struct stat has st_rdev.  */
#define HAVE_ST_RDEV 1

/* Define to `int' if <sys/types.h> doesn't define.  */
/*#undef mode_t*/

/* Define to `int' if <sys/types.h> doesn't define.  */
/*#undef pid_t*/

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/*#undef size_t*/

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define if your <sys/time.h> declares struct tm.  */
#define TM_IN_SYS_TIME 1

/* Define as the path of the `chmod' program. */
/*#undef CHMOD_PATH*/

/* Define if you have the clock function.  */
#define HAVE_CLOCK 1

/* Define if you have the getcwd function.  */
#define HAVE_GETCWD 1

/* Define if you have the gethostname function.  */
#define HAVE_GETHOSTNAME 1

/* Define if you have the getrusage function.  */
#define HAVE_GETRUSAGE 1

/* Define if you have the getwd function.  */
#define HAVE_GETWD 1

/* Define if you have the lstat function.  */
#define HAVE_LSTAT 1

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the symlink function.  */
#define HAVE_SYMLINK 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/*
 * Tell f2cext.c to build all extension calls into one object file.
 */

#define Labort 
#define Lderf 
#define Lderfc 
#define Lef1asc 
#define Lef1cmc 
#define Lerf 
#define Lerfc 
#define Lexit 
#define Lgetarg 
#define Lgetenv 
#define Liargc 
#define Lsignal 
#define Lsystem 
#define Lflush 
#define Lftell 
#define Lfseek 
#define Laccess 
#define Lbesj0 
#define Lbesj1 
#define Lbesjn 
#define Lbesy0 
#define Lbesy1 
#define Lbesyn 
#define Lchdir 
#define Lchmod 
#define Lctime 
#define Ldate 
#define Ldbesj0 
#define Ldbesj1 
#define Ldbesjn 
#define Ldbesy0 
#define Ldbesy1 
#define Ldbesyn 
#define Ldtime 
#define Letime 
#define Lfdate 
#define Lfgetc 
#define Lfget 
#define Lflush1 
#define Lfnum 
#define Lfputc 
#define Lfput 
#define Lfstat 
#define Lgerror 
#define Lgetcwd 
#define Lgetgid 
#define Lgetlog 
#define Lgetpid 
#define Lgetuid 
#define Lgmtime 
#define Lhostnm 
#define Lidate 
#define Lierrno 
#define Lirand 
#define Lisatty 
#define Litime 
#define Lkill 
#define Llink 
#define Llnblnk 
#define Llstat 
#define Lltime 
#define Lmclock 
#define Lperror 
#define Lrand 
#define Lrename 
#define Lsecnds 
#define Lsecond 
#define Lsleep 
#define Lsrand 
#define Lstat 
#define Lsymlnk 
#define Lsclock 
#define Ltime 
#define Lttynam 
#define Lumask 
#define Lunlink 
#define Lvxtidt 
#define Lvxttim 
#define Lalarm

#endif /* NETBSD_NATIVE */
#endif /* !__CONFIG_H__ */
