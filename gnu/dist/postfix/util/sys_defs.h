#ifndef _SYS_DEFS_H_INCLUDED_
#define _SYS_DEFS_H_INCLUDED_

/*++
/* NAME
/*	sys_defs 3h
/* SUMMARY
/*	portability header
/* SYNOPSIS
/*	#include <sys_defs.h>
/* DESCRIPTION
/* .nf

 /*
  * Specific platforms. Major release numbers differ for a good reason. So be
  * a good girl, plan for the future, and at least include the major release
  * number in the system type (for example, SUNOS5 or FREEBSD2). The system
  * type is determined by the makedefs shell script in the top-level
  * directory. Adding support for a new system type means updating the
  * makedefs script, and adding a section below for the new system.
  */
#if defined(FREEBSD2) || defined(FREEBSD3) || defined(FREEBSD4) \
    || defined(BSDI2) || defined(BSDI3) || defined(BSDI4) \
    || defined(OPENBSD2) || defined(NETBSD1) || defined(RHAPSODY5)
#define SUPPORTED
#include <sys/types.h>
#include <sys/param.h>
#define USE_PATHS_H
#define USE_FLOCK_LOCK
#define HAS_SUN_LEN
#define HAS_FSYNC
#define HAS_DB
#define HAS_SA_LEN
#define DEF_DB_TYPE	"hash"
#ifdef __NetBSD__
#if (defined(__NetBSD_Version__) && __NetBSD_Version__ >= 104250000)
/* sendmail 8.10 */
#define ALIAS_DB_MAP	"hash:/etc/mail/aliases"
#if (defined(__NetBSD_Version__) && __NetBSD_Version__ >= 104000000)
/* NetBSD 1.4 and later has basename(3) */
#define HAVE_BASENAME
#endif
#else
#define ALIAS_DB_MAP	"hash:/etc/aliases"
#endif
#else
#define ALIAS_DB_MAP	"hash:/etc/aliases"
#endif
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin"
#define USE_STATFS
#define STATFS_IN_SYS_MOUNT_H
#define HAS_POSIX_REGEXP
#endif

#if defined(OPENBSD2)
#define HAS_ISSETUGID
#endif

#if defined(NETBSD1)
#define USE_DOT_LOCK
#endif

#if defined(RHAPSODY5)
#define NORETURN	void
#define HAS_NETINFO
#endif

#ifdef ULTRIX4
#define SUPPORTED
/* Ultrix by default has only 64 descriptors per process */
#ifndef FD_SETSIZE
#define FD_SETSIZE	96
#endif
#include <sys/types.h>
#define UNSAFE_CTYPE			/* XXX verify */
#define _PATH_MAILDIR	"/var/spool/mail"
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_DEFPATH	"/bin:/usr/bin:/usr/ucb"
#define _PATH_STDPATH	"/bin:/usr/bin:/usr/etc:/usr/ucb"
#define USE_FLOCK_LOCK
#define USE_DOT_LOCK
#define HAS_FSYNC
/* might be set by makedef */
#ifdef HAS_DB
#define DEF_DB_TYPE	"hash"
#define ALIAS_DB_MAP	"hash:/etc/aliases"
#else
#define HAS_DBM
#define	DEF_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	"dbm:/etc/aliases"
#endif
extern int optind;
extern char *optarg;
extern int opterr;
extern int h_errno;

#define MISSING_STRFTIME_E
#define HAS_NIS
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define ROOT_PATH	"/bin:/usr/bin:/etc:/usr/etc:/usr/ucb"
#define USE_STATFS
#define USE_STRUCT_FS_DATA
#define STATFS_IN_SYS_MOUNT_H
/* Ultrix misses just S_ISSOCK, the others are there */
#define S_ISSOCK(mode)	(((mode) & (S_IFMT)) == (S_IFSOCK))
#define DUP2_DUPS_CLOSE_ON_EXEC
#define MISSING_USLEEP
#define NO_HERRNO
#endif

#ifdef OSF1
#define SUPPORTED
#include <sys/types.h>
#define MISSING_SETENV
#define USE_PATHS_H
#define _PATH_DEFPATH "/usr/bin:/usr/ucb"
#define USE_FLOCK_LOCK
#define USE_DOT_LOCK
#define HAS_FSYNC
#define HAVE_BASENAME
#define HAS_DBM
#define DEF_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	"dbm:/var/adm/sendmail/aliases"
extern int optind;			/* XXX use <getopt.h> */
extern char *optarg;			/* XXX use <getopt.h> */
extern int opterr;			/* XXX use <getopt.h> */

#define HAS_NIS
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin:/usr/ucb"
#define USE_STATFS
#define STATFS_IN_SYS_MOUNT_H
#define HAS_POSIX_REGEXP
#endif

#ifdef SUNOS4
#define SUPPORTED
#include <sys/types.h>
#define UNSAFE_CTYPE
#define fpos_t	long
#define MISSING_SETENV
#define MISSING_STRERROR
#define _PATH_MAILDIR	"/var/spool/mail"
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_DEFPATH	"/usr/bin:/usr/ucb"
#define _PATH_STDPATH	"/usr/bin:/usr/etc:/usr/ucb"
#define USE_FLOCK_LOCK
#define USE_DOT_LOCK
#define HAS_FSYNC
#define HAS_DBM
#define DEF_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	"dbm:/etc/aliases"
extern int optind;
extern char *optarg;
extern int opterr;

#define HAS_NIS
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define ROOT_PATH	"/bin:/usr/bin:/etc:/usr/etc:/usr/ucb"
#define USE_STATFS
#define STATFS_IN_SYS_VFS_H
#define memmove(d,s,l)	bcopy(s,d,l)
#define NO_HERRNO
#endif

#ifdef SUNOS5
#define SUPPORTED
#define _SVID_GETTOD			/* Solaris 2.5, XSH4.2 versus SVID */
#include <sys/types.h>
#define MISSING_SETENV
#define _PATH_MAILDIR	"/var/mail"
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_DEFPATH	"/usr/bin:/usr/ucb"
#define _PATH_STDPATH	"/usr/bin:/usr/sbin:/usr/ucb"
#define USE_FCNTL_LOCK
#define USE_DOT_LOCK
#define HAS_FSYNC
#define HAS_DBM
#define DEF_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	"dbm:/etc/mail/aliases"
#define HAS_NIS
#define USE_SYS_SOCKIO_H		/* Solaris 2.5, changed sys/ioctl.h */
#define GETTIMEOFDAY(t)	gettimeofday(t)
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin:/usr/ucb"
#define FIONREAD_IN_SYS_FILIO_H
#define USE_STATVFS
#define STATVFS_IN_SYS_STATVFS_H
#define STREAM_CONNECTIONS		/* avoid UNIX-domain sockets */
#define LOCAL_LISTEN	stream_listen
#define LOCAL_ACCEPT	stream_accept
#define LOCAL_CONNECT	stream_connect
#define LOCAL_TRIGGER	stream_trigger
#define HAS_VOLATILE_LOCKS
#endif

#ifdef UW7				/* UnixWare 7 */
#define SUPPORTED
#include <sys/types.h>
#define _PATH_MAILDIR	"/var/mail"
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_DEFPATH	"/usr/bin:/usr/ucb"
#define _PATH_STDPATH	"/usr/bin:/usr/sbin:/usr/ucb"
#define MISSING_SETENV
#define USE_FCNTL_LOCK
#define USE_DOT_LOCK
#define HAS_FSYNC
#define HAS_DBM
#define DEF_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	"dbm:/etc/mail/aliases"
#define HAS_NIS
#define USE_SYS_SOCKIO_H
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin:/usr/ucb"
#define FIONREAD_IN_SYS_FILIO_H
#define DBM_NO_TRAILING_NULL
#define USE_STATVFS
#define STATVFS_IN_SYS_STATVFS_H
#define UNIX_DOMAIN_CONNECT_BLOCKS_FOR_ACCEPT
#endif

#ifdef UW21				/* UnixWare 2.1.x */
#define SUPPORTED
#include <sys/types.h>
#define _PATH_MAILDIR   "/var/mail"
#define _PATH_BSHELL    "/bin/sh"
#define _PATH_DEFPATH   "/usr/bin:/usr/ucb"
#define _PATH_STDPATH   "/usr/bin:/usr/sbin:/usr/ucb"
#define MISSING_SETENV
#define USE_FCNTL_LOCK
#define USE_DOT_LOCK
#define HAS_FSYNC
#define HAS_DBM
#define DEF_DB_TYPE     "dbm"
#define ALIAS_DB_MAP    "dbm:/etc/mail/aliases"
/* Uncomment the following line if you have NIS package installed
#define HAS_NIS */
#define USE_SYS_SOCKIO_H
#define GETTIMEOFDAY(t) gettimeofday(t,NULL)
#define ROOT_PATH       "/bin:/usr/bin:/sbin:/usr/sbin:/usr/ucb"
#define FIONREAD_IN_SYS_FILIO_H
#define DBM_NO_TRAILING_NULL
#define USE_STATVFS
#define STATVFS_IN_SYS_STATVFS_H
#define UNIX_DOMAIN_CONNECT_BLOCKS_FOR_ACCEPT
#endif

#ifdef AIX4
#define SUPPORTED
#include <sys/types.h>
#define MISSING_SETENV
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_MAILDIR   "/var/spool/mail"	/* paths.h lies */
#define _PATH_DEFPATH	"/usr/bin:/usr/ucb"
#define _PATH_STDPATH	"/usr/bin:/usr/sbin:/usr/ucb"
#define USE_FCNTL_LOCK
#define USE_DOT_LOCK
#define USE_SYS_SELECT_H
#define HAS_FSYNC
#define HAS_DBM
#define DEF_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	"dbm:/etc/aliases"
#define HAS_NIS
#define HAS_SA_LEN
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define RESOLVE_H_NEEDS_STDIO_H
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin:/usr/ucb"
#define SOCKADDR_SIZE	size_t
#define SOCKOPT_SIZE	size_t
#define USE_STATVFS
#define STATVFS_IN_SYS_STATVFS_H
#define STRCASECMP_IN_STRINGS_H
extern time_t time(time_t *);
extern int seteuid(uid_t);
extern int setegid(gid_t);
extern int initgroups(const char *, int);

#endif

#ifdef AIX3
#define SUPPORTED
#include <sys/types.h>
#define MISSING_SETENV
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_MAILDIR   "/var/spool/mail"	/* paths.h lies */
#define _PATH_DEFPATH	"/usr/bin:/usr/ucb"
#define _PATH_STDPATH	"/usr/bin:/usr/sbin:/usr/ucb"
#define USE_FCNTL_LOCK
#define USE_DOT_LOCK
#define USE_SYS_SELECT_H
#define HAS_FSYNC
#define HAS_DBM
#define DEF_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	"dbm:/etc/aliases"
#define HAS_NIS
#define HAS_SA_LEN
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define RESOLVE_H_NEEDS_STDIO_H
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin:/usr/ucb"
#define SOCKADDR_SIZE	size_t
#define SOCKOPT_SIZE	size_t
#define USE_STATFS
#define STATFS_IN_SYS_STATFS_H
#define STRCASECMP_IN_STRINGS_H
extern time_t time(time_t *);
extern int seteuid(uid_t);
extern int setegid(gid_t);
extern int initgroups(const char *, int);

#endif

#if defined(IRIX5) || defined(IRIX6)
#define SUPPORTED
#include <sys/types.h>
#define MISSING_SETENV
#define _PATH_MAILDIR	"/var/mail"
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_DEFPATH	"/usr/bin:/usr/bsd"
#define _PATH_STDPATH	"/usr/bin:/usr/sbin:/usr/bsd"
#define USE_FCNTL_LOCK
#define USE_DOT_LOCK
#define HAS_FSYNC
#define HAS_DBM
#define DEF_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	"dbm:/etc/aliases"
#define HAS_NIS
#define USE_SYS_SOCKIO_H		/* XXX check */
#define GETTIMEOFDAY(t)	gettimeofday(t)
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin:/usr/bsd"
#define FIONREAD_IN_SYS_FILIO_H		/* XXX check */
#define DBM_NO_TRAILING_NULL		/* XXX check */
#define USE_STATVFS
#define STATVFS_IN_SYS_STATVFS_H
#endif

#if defined(IRIX5)
#define MISSING_USLEEP
#endif

#ifdef LINUX2
#define SUPPORTED
#include <sys/types.h>
#define USE_PATHS_H
#define USE_FLOCK_LOCK
#define USE_DOT_LOCK
#define HAS_FSYNC
#define HAS_DB
#define DEF_DB_TYPE	"hash"
#define ALIAS_DB_MAP	"hash:/etc/aliases"
#define HAS_NIS
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin"
#define FIONREAD_IN_TERMIOS_H
#define USE_STATFS
#define STATFS_IN_SYS_VFS_H
#define UNIX_DOMAIN_CONNECT_BLOCKS_FOR_ACCEPT
#define PREPEND_PLUS_TO_OPTSTRING
#define HAS_POSIX_REGEXP
#endif

 /*
  * HPUX11 was copied from HPUX10, but can perhaps be trimmed down a bit.
  */
#ifdef HPUX11
#define SUPPORTED
#define USE_SIG_RETURN
#include <sys/types.h>
#define HAS_DBM
#define USE_FCNTL_LOCK
#define HAS_FSYNC
#define DEF_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	"dbm:/etc/mail/aliases"
#define ROOT_PATH	"/usr/bin:/sbin:/usr/sbin"
#define MISSING_SETENV
#define HAS_NIS
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_MAILDIR	"/var/mail"
#define _PATH_DEFPATH	"/usr/bin"
#define _PATH_STDPATH	"/usr/bin:/sbin:/usr/sbin"
#define MISSING_SETEUID
#define HAVE_SETRESUID
#define MISSING_SETEGID
#define HAVE_SETRESGID
extern int h_errno;			/* <netdb.h> imports too much stuff */

#define USE_STATFS
#define STATFS_IN_SYS_VFS_H
#define HAS_POSIX_REGEXP
#endif

#ifdef HPUX10
#define SUPPORTED
#define USE_SIG_RETURN
#include <sys/types.h>
#define HAS_DBM
#define USE_FCNTL_LOCK
#define HAS_FSYNC
#define DEF_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	"dbm:/etc/mail/aliases"
#define ROOT_PATH	"/usr/bin:/sbin:/usr/sbin"
#define MISSING_SETENV
#define HAS_NIS
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_MAILDIR	"/var/mail"
#define _PATH_DEFPATH	"/usr/bin"
#define _PATH_STDPATH	"/usr/bin:/sbin:/usr/sbin"
#define MISSING_SETEUID
#define HAVE_SETRESUID
#define MISSING_SETEGID
#define HAVE_SETRESGID
extern int h_errno;			/* <netdb.h> imports too much stuff */

#define USE_STATFS
#define STATFS_IN_SYS_VFS_H
#define HAS_POSIX_REGEXP
#endif

#ifdef HPUX9
#define SUPPORTED
#define USE_SIG_RETURN
#include <sys/types.h>
#define HAS_DBM
#define USE_FCNTL_LOCK
#define HAS_FSYNC
#define HAS_NIS
#define MISSING_SETENV
#define MISSING_RLIMIT_FSIZE
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define DEF_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	"dbm:/usr/lib/aliases"
#define ROOT_PATH	"/bin:/usr/bin:/etc"
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_MAILDIR	"/usr/mail"
#define _PATH_DEFPATH	"/bin:/usr/bin"
#define _PATH_STDPATH	"/bin:/usr/bin:/etc"
#define MISSING_SETEUID
#define HAVE_SETRESUID
#define MISSING_SETEGID
#define HAVE_SETRESGID
extern int h_errno;

#define USE_ULIMIT			/* no setrlimit() */
#define USE_STATFS
#define STATFS_IN_SYS_VFS_H
#define HAS_POSIX_REGEXP
#endif

 /*
  * NEXTSTEP3, without -lposix, because its naming service is broken.
  */
#ifdef NEXTSTEP3
#define SUPPORTED
#include <sys/types.h>
#define HAS_DBM
#define USE_FLOCK_LOCK
#define USE_STATFS
#define HAVE_SYS_DIR_H
#define STATFS_IN_SYS_VFS_H
#define HAS_FSYNC
#define HAS_NIS
#define HAS_NETINFO
#define MISSING_SETENV_PUTENV
#define MISSING_MKFIFO
#define MISSING_SIGSET_T
#define MISSING_SIGACTION
#define MISSING_STD_FILENOS
#define MISSING_SETSID
#define MISSING_WAITPID
#define MISSING_UTIMBUF
#define HAS_WAIT4
#define WAIT_STATUS_T union wait
#define NORMAL_EXIT_STATUS(x) (WIFEXITED(x) && !WEXITSTATUS (x))
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define _PATH_MAILDIR	"/usr/spool/mail"
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_DEFPATH	"/bin:/usr/bin:/usr/ucb"
#define _PATH_STDPATH	"/bin:/usr/bin:/usr/ucb"
#define ROOT_PATH	"/bin:/usr/bin:/usr/etc:/usr/ucb"
#define DEF_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	"netinfo:/aliases"
#include <libc.h>
#define MISSING_POSIX_S_IS
#define MISSING_POSIX_S_MODES
/* It's amazing what is all missing...	*/
#define isascii(c)	((unsigned)(c)<=0177)
extern int opterr;

#define MISSING_PID_T
#define MISSING_STRFTIME_E
#define FD_CLOEXEC	1
#define O_NONBLOCK	O_NDELAY
#define WEXITSTATUS(x)	((x).w_retcode)
#define WTERMSIG(x)	((x).w_termsig)
#endif

 /*
  * OPENSTEP does not have posix (some fix...)
  */
#ifdef OPENSTEP4
#define SUPPORTED
#include <sys/types.h>
#define HAS_DBM
#define USE_FLOCK_LOCK
#define USE_STATFS
#define HAVE_SYS_DIR_H
#define STATFS_IN_SYS_VFS_H
#define HAS_FSYNC
#define HAS_NIS
#define HAS_NETINFO
#define MISSING_SETENV_PUTENV
#define MISSING_MKFIFO
#define MISSING_SIGSET_T
#define MISSING_SIGACTION
#define MISSING_STD_FILENOS
#define MISSING_SETSID
#define MISSING_WAITPID
#define MISSING_UTIMBUF
#define HAS_WAIT4
#define WAIT_STATUS_T union wait
#define NORMAL_EXIT_STATUS(x) (WIFEXITED(x) && !WEXITSTATUS (x))
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define _PATH_MAILDIR	"/usr/spool/mail"
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_DEFPATH	"/bin:/usr/bin:/usr/ucb"
#define _PATH_STDPATH	"/bin:/usr/bin:/usr/ucb"
#define ROOT_PATH	"/bin:/usr/bin:/usr/etc:/usr/ucb"
#define DEF_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	"netinfo:/aliases"
#include <libc.h>
#define MISSING_POSIX_S_IS
#define MISSING_POSIX_S_MODES
/* It's amazing what is all missing...	*/
#define isascii(c)	((unsigned)(c)<=0177)
extern int opterr;

#define MISSING_PID_T
#define MISSING_STRFTIME_E
#define FD_CLOEXEC	1
#define O_NONBLOCK	O_NDELAY
#define WEXITSTATUS(x)	((x).w_retcode)
#define WTERMSIG(x)	((x).w_termsig)
#define NORETURN			/* the native compiler */
#endif

#ifdef ReliantUnix543
#define SUPPORTED
#include <sys/types.h>
#define MISSING_SETENV
#define _PATH_DEFPATH	"/usr/bin:/usr/ucb"
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_MAILDIR	"/var/spool/mail"
#define USE_FCNTL_LOCK
#define USE_DOT_LOCK
#define HAS_FSYNC
#define FIONREAD_IN_SYS_FILIO_H
#define USE_SYS_SOCKIO_H
#define HAS_DBM
#define DEF_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	"dbm:/var/adm/sendmail/aliases"
extern int optind;			/* XXX use <getopt.h> */
extern char *optarg;			/* XXX use <getopt.h> */
extern int opterr;			/* XXX use <getopt.h> */

#define HAS_NIS
#define GETTIMEOFDAY(t) gettimeofday(t)
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin:/usr/ucb"
#define USE_STATVFS
#define STATVFS_IN_SYS_STATVFS_H
#define MISSING_USLEEP
#endif

#ifdef DCOSX1				/* Siemens Pyramid */
#define SUPPORTED
#include <sys/types.h>
#define _PATH_MAILDIR	"/var/mail"
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_DEFPATH	"/usr/bin:/usr/ucb"
#define _PATH_STDPATH	"/usr/bin:/usr/sbin:/usr/ucb"
#define MISSING_SETENV
#define USE_FCNTL_LOCK
#define USE_DOT_LOCK
#define HAS_FSYNC
#define DEF_DB_TYPE	"hash"
#define ALIAS_DB_MAP	"hash:/etc/aliases"
/* Uncomment the following line if you have NIS package installed */
/* #define HAS_NIS */
#define USE_SYS_SOCKIO_H
#define GETTIMEOFDAY(t) gettimeofday(t,NULL)
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin:/usr/ucb"
#define FIONREAD_IN_SYS_FILIO_H
#define DBM_NO_TRAILING_NULL
#define USE_STATVFS
#define STATVFS_IN_SYS_STATVFS_H
#define UNIX_DOMAIN_CONNECT_BLOCKS_FOR_ACCEPT
#ifndef S_ISSOCK
#define S_ISSOCK(mode)	((mode&0xF000) == 0xC000)
#endif
#endif

#ifdef SCO5
#define SUPPORTED
#include <sys/types.h>
#include <sys/socket.h>
extern int h_errno;
#define _PATH_MAILDIR	"/usr/spool/mail"
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_DEFPATH	"/bin:/usr/bin"
#define USE_PATHS_H
#define USE_FCNTL_LOCK
#define USE_DOT_LOCK
#define HAS_FSYNC
#define HAS_DBM
#define DEF_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	"dbm:/etc/mail/aliases"
#define DBM_NO_TRAILING_NULL
#define HAS_NIS
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define ROOT_PATH	"/bin:/etc:/usr/bin:/tcb/bin"
#define USE_STATVFS
#define STATVFS_IN_SYS_STATVFS_H
#define UNIX_DOMAIN_CONNECT_BLOCKS_FOR_ACCEPT
#define MISSING_SETENV
/* SCO5 misses just S_ISSOCK, the others are there
 * Use C_ISSOCK definition from cpio.h.
 */
#include <cpio.h>
#define S_ISSOCK(mode) (((mode) & (S_IFMT)) == (C_ISSOCK))
#endif

 /*
  * We're not going to try to guess like configure does.
  */
#ifndef SUPPORTED
#error "unsupported platform"
#endif

#ifdef DUP2_DUPS_CLOSE_ON_EXEC
/* dup2_pass_on_exec() can be found in util/sys_compat.c */
extern int dup2_pass_on_exec(int oldd, int newd);

#define DUP2 dup2_pass_on_exec
#else
#define DUP2 dup2
#endif

#ifdef PREPEND_PLUS_TO_OPTSTRING
#define GETOPT(argc, argv, str)	getopt((argc), (argv), "+" str)
#else
#define GETOPT(argc, argv, str) getopt((argc), (argv), (str))
#endif
#define OPTIND  (optind > 0 ? optind : 1)

#if defined(USE_FCNTL_LOCK) && defined(USE_FLOCK_LOCK)
#error "define USE_FCNTL_LOCK or USE_FLOCK_LOCK, not both"
#endif

#if !defined(USE_FCNTL_LOCK) && !defined(USE_FLOCK_LOCK)
#error "define USE_FCNTL_LOCK or USE_FLOCK_LOCK"
#endif

#if defined(USE_STATFS) && defined(USE_STATVFS)
#error "define USE_STATFS or USE_STATVFS, not both"
#endif

#if !defined(USE_STATFS) && !defined(USE_STATVFS)
#error "define USE_STATFS or USE_STATVFS"
#endif

 /*
  * Defaults for normal systems.
  */
#ifndef SOCKADDR_SIZE
#define SOCKADDR_SIZE	int
#endif

#ifndef SOCKOPT_SIZE
#define SOCKOPT_SIZE	int
#endif

#ifndef LOCAL_LISTEN
#define LOCAL_LISTEN	unix_listen
#define LOCAL_ACCEPT	unix_accept
#define LOCAL_CONNECT	unix_connect
#define LOCAL_TRIGGER	unix_trigger
#endif

#if !defined (HAVE_SYS_NDIR_H) && !defined (HAVE_SYS_DIR_H) \
	&& !defined (HAVE_NDIR_H)
#define HAVE_DIRENT_H
#endif

#ifndef WAIT_STATUS_T
typedef int WAIT_STATUS_T;

#define NORMAL_EXIT_STATUS(status)	((status) == 0)
#endif

 /*
  * Turn on the compatibility stuff.
  */
#ifdef MISSING_UTIMBUF
struct utimbuf {
    time_t  actime;
    time_t  modtime;
};

#endif

#ifdef MISSING_STRERROR
extern const char *strerror(int);

#endif

#if defined (MISSING_SETENV) || defined (MISSING_SETENV_PUTENV)
extern int setenv(const char *, const char *, int);

#endif

#ifdef MISSING_SETEUID
extern int seteuid(uid_t euid);

#endif

#ifdef MISSING_SETEGID
extern int setegid(gid_t egid);

#endif

#ifdef MISSING_MKFIFO
extern int mkfifo(char *, int);

#endif

#ifdef MISSING_WAITPID
extern int waitpid(int, WAIT_STATUS_T *status, int options);

#endif

#ifdef MISSING_SETSID
extern int setsid(void);

#endif

#ifdef MISSING_STD_FILENOS
#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2
#endif

#ifdef MISSING_PID_T
typedef int pid_t;

#endif

#ifdef MISSING_POSIX_S_IS
#define S_ISBLK(mode)	(((mode) & (_S_IFMT)) == (_S_IFBLK))
#define S_ISCHR(mode)	(((mode) & (_S_IFMT)) == (_S_IFCHR))
#define S_ISDIR(mode)	(((mode) & (_S_IFMT)) == (_S_IFDIR))
#define S_ISSOCK(mode)	(((mode) & (_S_IFMT)) == (_S_IFSOCK))
#define S_ISFIFO(mode)	(((mode) & (_S_IFMT)) == (_S_IFIFO))
#define S_ISREG(mode)	(((mode) & (_S_IFMT)) == (_S_IFREG))
#endif

#ifdef MISSING_POSIX_S_MODES
#define S_IRUSR	_S_IRUSR
#define S_IRGRP	0000040
#define S_IROTH	0000004
#define S_IWUSR	_S_IWUSR
#define S_IWGRP	0000020
#define S_IWOTH	0000002
#define S_IXUSR	_S_IXUSR
#define S_IXGRP	0000010
#define S_IXOTH	0000001
#define	S_IRWXU	(S_IRUSR | S_IWUSR | S_IXUSR)
#endif

 /*
  * Need to specify what functions never return, so that the compiler can
  * warn for missing initializations and other trouble. However, OPENSTEP4
  * gcc 2.7.x cannot handle this so we define this only if NORETURN isn't
  * already defined above.
  */
#ifndef NORETURN
#if __GNUC__ == 2 && __GNUC_MINOR__ >= 5 || __GNUC__ >= 3
#define NORETURN	void __attribute__((__noreturn__))
#endif
#endif

#ifndef NORETURN
#define NORETURN	void
#endif

 /*
  * Making the ctype.h macros not more expensive than necessary. On some
  * systems, ctype.h misbehaves badly with signed characters.
  */
#define _UCHAR_(c)	((unsigned char)(c))
#ifdef UNSAFE_CTYPE
#define ISASCII(c)	isascii(_UCHAR_(c))
#define ISALNUM(c)	(ISASCII(c) && isalnum(c))
#define ISALPHA(c)	(ISASCII(c) && isalpha(c))
#define ISCNTRL(c)	(ISASCII(c) && iscntrl(c))
#define ISDIGIT(c)	(ISASCII(c) && isdigit(c))
#define ISGRAPH(c)	(ISASCII(c) && isgraph(c))
#define ISLOWER(c)	(ISASCII(c) && islower(c))
#define ISPRINT(c)	(ISASCII(c) && isprint(c))
#define ISPUNCT(c)	(ISASCII(c) && ispunct(c))
#define ISSPACE(c)	(ISASCII(c) && isspace(c))
#define ISUPPER(c)	(ISASCII(c) && isupper(c))
#define TOLOWER(c)	(ISUPPER(c) ? tolower(c) : (c))
#define TOUPPER(c)	(ISLOWER(c) ? toupper(c) : (c))
#else
#define ISASCII(c)	isascii(_UCHAR_(c))
#define ISALNUM(c)	isalnum(_UCHAR_(c))
#define ISALPHA(c)	isalpha(_UCHAR_(c))
#define ISCNTRL(c)	iscntrl(_UCHAR_(c))
#define ISDIGIT(c)	isdigit(_UCHAR_(c))
#define ISGRAPH(c)	isgraph(_UCHAR_(c))
#define ISLOWER(c)	islower(_UCHAR_(c))
#define ISPRINT(c)	isprint(_UCHAR_(c))
#define ISPUNCT(c)	ispunct(_UCHAR_(c))
#define ISSPACE(c)	isspace(_UCHAR_(c))
#define ISUPPER(c)	isupper(_UCHAR_(c))
#define TOLOWER(c)	tolower(_UCHAR_(c))
#define TOUPPER(c)	toupper(_UCHAR_(c))
#endif

 /*
  * Scaffolding. I don't want to lose messages while the program is under
  * development.
  */
extern int REMOVE(const char *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

#endif
