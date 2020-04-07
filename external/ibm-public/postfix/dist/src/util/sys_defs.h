/*	$NetBSD: sys_defs.h,v 1.11 2017/02/20 05:43:12 rin Exp $	*/

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

 /*
  * 4.4BSD and close derivatives.
  */
#if defined(FREEBSD2) || defined(FREEBSD3) || defined(FREEBSD4) \
    || defined(FREEBSD5) || defined(FREEBSD6) || defined(FREEBSD7) \
    || defined(FREEBSD8) || defined(FREEBSD9) || defined(FREEBSD10) \
    || defined(BSDI2) || defined(BSDI3) || defined(BSDI4) \
    || defined(OPENBSD2) || defined(OPENBSD3) || defined(OPENBSD4) \
    || defined(OPENBSD5) || defined(OPENBSD6) \
    || defined(NETBSD1) || defined(NETBSD2) || defined(NETBSD3) \
    || defined(NETBSD4) || defined(NETBSD5) || defined(NETBSD6) \
    || defined(NETBSD7) \
    || defined(EKKOBSD1) || defined(DRAGONFLY)
#define SUPPORTED
#include <sys/types.h>
#include <sys/param.h>
#define UINT32_TYPE	unsigned int
#define UINT16_TYPE	unsigned short
#define USE_PATHS_H
#define HAS_FLOCK_LOCK
#define HAS_FCNTL_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FLOCK
#define DEF_MAILBOX_LOCK "flock, dotlock"
#define HAS_SUN_LEN
#define HAS_FSYNC
#define HAS_DB
#define HAS_SA_LEN
#define NATIVE_DB_TYPE	"hash"
#if (defined(__NetBSD_Version__) && __NetBSD_Version__ >= 104250000)
#define ALIAS_DB_MAP   DEF_DB_TYPE ":/etc/mail/aliases"	/* sendmail 8.10 */
#endif
#if (defined(OpenBSD) && OpenBSD >= 200006)
#define ALIAS_DB_MAP   DEF_DB_TYPE ":/etc/mail/aliases"	/* OpenBSD 2.7 */
#endif
#ifndef ALIAS_DB_MAP
#define ALIAS_DB_MAP	DEF_DB_TYPE ":/etc/aliases"
#endif
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin"
#if (defined(__NetBSD_Version__) && __NetBSD_Version__ > 299000900)
#define USE_STATVFS
#define STATVFS_IN_SYS_STATVFS_H
#else
#define USE_STATFS
#define STATFS_IN_SYS_MOUNT_H
#endif
#define HAS_POSIX_REGEXP
#define HAS_ST_GEN			/* struct stat contains inode
					 * generation number */
#define NATIVE_SENDMAIL_PATH "/usr/sbin/sendmail"
#define NATIVE_MAILQ_PATH "/usr/bin/mailq"
#define NATIVE_NEWALIAS_PATH "/usr/bin/newaliases"
#define NATIVE_COMMAND_DIR "/usr/sbin"
#define NATIVE_DAEMON_DIR "/usr/libexec/postfix"
#define HAS_DLOPEN
#endif

#ifdef FREEBSD2
#define getsid(p) getpgrp()
#ifndef CMSG_SPACE
#define CMSG_SPACE(len) (CMSG_ALIGN(sizeof(struct cmsghdr)) + CMSG_ALIGN(len))
#endif
#ifndef CMSG_LEN
#define CMSG_LEN(len) (CMSG_ALIGN(sizeof(struct cmsghdr)) + (len))
#endif
#ifndef CMSG_ALIGN
#define CMSG_ALIGN(n) ALIGN(n)
#endif
#endif					/* FREEBSD2 */

#ifdef BSDI4
/* #define HAS_IPV6 find out interface lookup method */
#endif

/* __FreeBSD_version version is major+minor */

#if __FreeBSD_version >= 220000
#define PREFERRED_RAND_SOURCE	"dev:/dev/urandom"	/* introduced 2.1.5 */
#endif

#if __FreeBSD_version >= 300000
#define HAS_ISSETUGID
#define HAS_FUTIMES
#endif

#if __FreeBSD_version >= 400000
#define SOCKADDR_SIZE	socklen_t
#define SOCKOPT_SIZE	socklen_t
#endif

#if __FreeBSD_version >= 420000
#define HAS_DUPLEX_PIPE			/* 4.1 breaks with kqueue(2) */
#endif

#if (__FreeBSD_version >= 702104 && __FreeBSD_version <= 800000) \
    || __FreeBSD_version >= 800100
#define HAS_CLOSEFROM
#endif

/* OpenBSD version is year+month */

#if OpenBSD >= 199805			/* XXX */
#define HAS_FUTIMES			/* XXX maybe earlier */
#endif

#if (defined(OpenBSD) && OpenBSD >= 199608)
#define PREFERRED_RAND_SOURCE	"dev:/dev/arandom"	/* XXX earlier */
#endif

#if OpenBSD >= 200000			/* XXX */
#define HAS_ISSETUGID
#endif

#if OpenBSD >= 200200			/* XXX */
#define SOCKADDR_SIZE	socklen_t
#define SOCKOPT_SIZE	socklen_t
#endif

#if OpenBSD >= 200405			/* 3.5 */
#define HAS_CLOSEFROM
#endif

/* __NetBSD_Version__ is major+minor */

#if __NetBSD_Version__ >= 103000000	/* XXX maybe earlier */
#undef DEF_MAILBOX_LOCK
#define DEF_MAILBOX_LOCK "flock, dotlock"
#define PREFERRED_RAND_SOURCE	"dev:/dev/urandom"	/* XXX maybe earlier */
#endif

#if __NetBSD_Version__ >= 105000000
#define HAS_ISSETUGID			/* XXX maybe earlier */
#endif

#if __NetBSD_Version__ >= 106000000	/* XXX maybe earlier */
#define SOCKADDR_SIZE	socklen_t
#define SOCKOPT_SIZE	socklen_t
#endif

#if __NetBSD_Version__ >= 299000900	/* 2.99.9 */
#define HAS_CLOSEFROM
#endif

#if (defined(__NetBSD_Version__) && __NetBSD_Version__ >= 102000000)
#define HAS_FUTIMES
#endif

#if defined(__DragonFly__)
#define HAS_DEV_URANDOM
#define HAS_ISSETUGID
#define HAS_FUTIMES
#define SOCKADDR_SIZE	socklen_t
#define SOCKOPT_SIZE	socklen_t
#define HAS_DUPLEX_PIPE
#endif

#if (defined(__NetBSD_Version__) && __NetBSD_Version__ >= 105000000) \
    || (defined(__FreeBSD__) && __FreeBSD__ >= 4) \
    || (defined(OpenBSD) && OpenBSD >= 200003) \
    || defined(__DragonFly__) \
    || defined(USAGI_LIBINET6)
#ifndef NO_IPV6
#define HAS_IPV6
#endif
#if !defined(NO_IPV6) || defined(__NetBSD__)
#define HAVE_GETIFADDRS
#endif
#endif

#if (defined(__FreeBSD_version) && __FreeBSD_version >= 300000) \
    || (defined(__NetBSD_Version__) && __NetBSD_Version__ >= 103000000) \
    || (defined(OpenBSD) && OpenBSD >= 199700)	/* OpenBSD 2.0?? */ \
    || defined(__DragonFly__)
#define USE_SYSV_POLL
#endif

#ifndef NO_KQUEUE
#if (defined(__FreeBSD_version) && __FreeBSD_version >= 410000) \
    || (defined(__NetBSD_Version__) && __NetBSD_Version__ >= 200000000) \
    || (defined(OpenBSD) && OpenBSD >= 200105)	/* OpenBSD 2.9 */ \
    || defined(__DragonFly__)
#define EVENTS_STYLE	EVENTS_STYLE_KQUEUE
#endif
#endif

#ifndef NO_POSIX_GETPW_R
#if (defined(__FreeBSD_version) && __FreeBSD_version >= 510000) \
    || (defined(__NetBSD_Version__) && __NetBSD_Version__ >= 300000000) \
    || (defined(OpenBSD) && OpenBSD >= 200811)	/* OpenBSD 4.4 */
#define HAVE_POSIX_GETPW_R
#endif
#endif

 /*
  * UNIX on MAC.
  */
#if defined(RHAPSODY5) || defined(MACOSX)
#define SUPPORTED
#include <sys/types.h>
#define UINT32_TYPE	unsigned int
#define UINT16_TYPE	unsigned short
#define USE_PATHS_H
#define HAS_FLOCK_LOCK
#define HAS_FCNTL_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FLOCK
#define DEF_MAILBOX_LOCK "flock, dotlock"
#define HAS_SUN_LEN
#define HAS_FSYNC
#define HAS_DB
#define HAS_SA_LEN
#define NATIVE_DB_TYPE	"hash"
#define ALIAS_DB_MAP	DEF_DB_TYPE ":/etc/aliases"
#define GETTIMEOFDAY(t) gettimeofday(t,(struct timezone *) 0)
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin"
#define USE_STATFS
#define STATFS_IN_SYS_MOUNT_H
#define HAS_POSIX_REGEXP
#ifndef NO_NETINFO
#define HAS_NETINFO
#endif
#ifndef NO_IPV6
#define HAS_IPV6
#define HAVE_GETIFADDRS
#endif
#define HAS_FUTIMES			/* XXX Guessing */
#define NATIVE_SENDMAIL_PATH "/usr/sbin/sendmail"
#define NATIVE_MAILQ_PATH "/usr/bin/mailq"
#define NATIVE_NEWALIAS_PATH "/usr/bin/newaliases"
#define NATIVE_COMMAND_DIR "/usr/sbin"
#define NATIVE_DAEMON_DIR "/usr/libexec/postfix"
#define SOCKADDR_SIZE	socklen_t
#define SOCKOPT_SIZE	socklen_t
#ifndef NO_KQUEUE
#define EVENTS_STYLE	EVENTS_STYLE_KQUEUE
#define USE_SYSV_POLL_THEN_SELECT
#endif
#define USE_MAX_FILES_PER_PROC
#ifndef NO_POSIX_GETPW_R
#define HAVE_POSIX_GETPW_R
#endif
#define HAS_DLOPEN
#define PREFERRED_RAND_SOURCE	"dev:/dev/urandom"
#endif

 /*
  * Ultrix 4.x, a sort of 4.[1-2] BSD system with System V.2 compatibility
  * and POSIX.
  */
#ifdef ULTRIX4
#define SUPPORTED
#define UINT32_TYPE	unsigned int
#define UINT16_TYPE	unsigned short
/* Ultrix by default has only 64 descriptors per process */
#ifndef FD_SETSIZE
#define FD_SETSIZE	96
#endif
#include <sys/types.h>
#define _PATH_MAILDIR	"/var/spool/mail"
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_DEFPATH	"/bin:/usr/bin:/usr/ucb"
#define _PATH_STDPATH	"/bin:/usr/bin:/usr/etc:/usr/ucb"
#define HAS_FLOCK_LOCK
#define HAS_FCNTL_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FLOCK
#define DEF_MAILBOX_LOCK "flock, dotlock"
#define HAS_FSYNC
/* might be set by makedef */
#ifdef HAS_DB
#define NATIVE_DB_TYPE	"hash"
#define ALIAS_DB_MAP	DEF_DB_TYPE ":/etc/aliases"
#else
#define HAS_DBM
#define	NATIVE_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	DEF_DB_TYPE ":/etc/aliases"
#endif
extern int optind;
extern char *optarg;
extern int opterr;
extern int h_errno;

#define MISSING_STRFTIME_E
#ifndef NO_NIS
#define HAS_NIS
#endif
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
#define NATIVE_SENDMAIL_PATH "/usr/lib/sendmail"
#define NATIVE_COMMAND_DIR "/usr/etc"
#define NATIVE_DAEMON_DIR "/usr/libexec/postfix"
#endif

 /*
  * OSF, then Digital UNIX, then Compaq. A BSD-flavored hybrid.
  */
#ifdef OSF1
#define SUPPORTED
#include <sys/types.h>
#define UINT32_TYPE	unsigned int
#define UINT16_TYPE	unsigned short
#define MISSING_SETENV
#define USE_PATHS_H
#define _PATH_DEFPATH "/usr/bin:/usr/ucb"
#define HAS_FLOCK_LOCK
#define HAS_FCNTL_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FLOCK
#define DEF_MAILBOX_LOCK "flock, dotlock"
#define HAS_FSYNC
#define HAVE_BASENAME
#define HAS_DBM
#define NATIVE_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	DEF_DB_TYPE ":/var/adm/sendmail/aliases"
extern int optind;			/* XXX use <getopt.h> */
extern char *optarg;			/* XXX use <getopt.h> */
extern int opterr;			/* XXX use <getopt.h> */

#ifndef NO_NIS
#define HAS_NIS
#endif
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin:/usr/ucb"
#define USE_STATFS
#define STATFS_IN_SYS_MOUNT_H
#define HAS_POSIX_REGEXP
#define BROKEN_WRITE_SELECT_ON_NON_BLOCKING_PIPE
#define NO_MSGHDR_MSG_CONTROL
#ifndef NO_IPV6
#define HAS_IPV6
#endif

#endif

 /*
  * SunOS 4.x, a mostly 4.[2-3] BSD system with System V.2 compatibility and
  * POSIX support.
  */
#ifdef SUNOS4
#define SUPPORTED
#include <sys/types.h>
#include <memory.h>
#define UINT32_TYPE	unsigned int
#define UINT16_TYPE	unsigned short
#define UNSAFE_CTYPE
#define fpos_t	long
#define MISSING_SETENV
#define MISSING_STRERROR
#define MISSING_STRTOUL
#define _PATH_MAILDIR	"/var/spool/mail"
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_DEFPATH	"/usr/bin:/usr/ucb"
#define _PATH_STDPATH	"/usr/bin:/usr/etc:/usr/ucb"
#define HAS_FLOCK_LOCK
#define HAS_FCNTL_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FLOCK
#define DEF_MAILBOX_LOCK "flock, dotlock"
#define HAS_FSYNC
#define HAS_DBM
#define NATIVE_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	DEF_DB_TYPE ":/etc/aliases"
extern int optind;
extern char *optarg;
extern int opterr;

#ifndef NO_NIS
#define HAS_NIS
#endif
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define ROOT_PATH	"/bin:/usr/bin:/etc:/usr/etc:/usr/ucb"
#define USE_STATFS
#define STATFS_IN_SYS_VFS_H
#define memmove(d,s,l)	bcopy(s,d,l)
#define NO_HERRNO
#define NATIVE_SENDMAIL_PATH "/usr/lib/sendmail"
#define NATIVE_MAILQ_PATH "/usr/ucb/mailq"
#define NATIVE_NEWALIAS_PATH "/usr/ucb/newaliases"
#define NATIVE_COMMAND_DIR "/usr/etc"
#define NATIVE_DAEMON_DIR "/usr/libexec/postfix"
#define STRCASECMP_IN_STRINGS_H
#define OCTAL_TO_UNSIGNED(res, str) sscanf((str), "%o", &(res))
#define size_t	unsigned
#define ssize_t	int
#define getsid	getpgrp
#endif

 /*
  * SunOS 5.x, mostly System V Release 4.
  */
#ifdef SUNOS5
#define SUPPORTED
#define _SVID_GETTOD			/* Solaris 2.5, XSH4.2 versus SVID */
#include <sys/types.h>
#define UINT32_TYPE	unsigned int
#define UINT16_TYPE	unsigned short
#define MISSING_SETENV
#define _PATH_MAILDIR	"/var/mail"
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_DEFPATH	"/usr/bin:/usr/ucb"
#define _PATH_STDPATH	"/usr/bin:/usr/sbin:/usr/ucb"
#define HAS_FCNTL_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FCNTL
#define DEF_MAILBOX_LOCK "fcntl, dotlock"
#define HAS_FSYNC
#define HAS_DBM
#define NATIVE_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	DEF_DB_TYPE ":/etc/mail/aliases"
#ifndef NO_NIS
#define HAS_NIS
#ifndef NO_NISPLUS
#define HAS_NISPLUS
#endif					/* NO_NISPLUS */
#endif
#define USE_SYS_SOCKIO_H		/* Solaris 2.5, changed sys/ioctl.h */
#define GETTIMEOFDAY(t)	gettimeofday(t)
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin:/usr/ucb"
#define FIONREAD_IN_SYS_FILIO_H
#define USE_STATVFS
#define STATVFS_IN_SYS_STATVFS_H
#define INT_MAX_IN_LIMITS_H
#ifdef STREAM_CONNECTIONS		/* avoid UNIX-domain sockets */
#define LOCAL_LISTEN	stream_listen
#define LOCAL_ACCEPT	stream_accept
#define LOCAL_CONNECT	stream_connect
#define LOCAL_TRIGGER	stream_trigger
#define LOCAL_SEND_FD	stream_send_fd
#define LOCAL_RECV_FD	stream_recv_fd
#endif
#define HAS_VOLATILE_LOCKS
#define BROKEN_READ_SELECT_ON_TCP_SOCKET
#define CANT_WRITE_BEFORE_SENDING_FD
#ifndef NO_POSIX_REGEXP
#define HAS_POSIX_REGEXP
#endif
#ifndef NO_IPV6
#define HAS_IPV6
#define HAS_SIOCGLIF
#endif
#ifndef NO_CLOSEFROM
#define HAS_CLOSEFROM
#endif
#ifndef NO_DEV_URANDOM
#define PREFERRED_RAND_SOURCE	"dev:/dev/urandom"
#endif
#ifndef NO_FUTIMESAT
#define HAS_FUTIMESAT
#endif
#define USE_SYSV_POLL
#ifndef NO_DEVPOLL
#define EVENTS_STYLE	EVENTS_STYLE_DEVPOLL
#endif
#ifndef NO_POSIX_GETPW_R
#define HAVE_POSIX_GETPW_R
#define GETPW_R_NEEDS_POSIX_PTHREAD_SEMANTICS
#endif

/*
 * Allow build environment to override paths.
 */
#define NATIVE_SENDMAIL_PATH "/usr/lib/sendmail"
#define NATIVE_MAILQ_PATH "/usr/bin/mailq"
#define NATIVE_NEWALIAS_PATH "/usr/bin/newaliases"
#define NATIVE_COMMAND_DIR "/usr/sbin"
#define NATIVE_DAEMON_DIR "/usr/libexec/postfix"

#define HAS_DLOPEN
#endif

 /*
  * UnixWare, System Release 4.
  */
#ifdef UW7				/* UnixWare 7 */
#define SUPPORTED
#include <sys/types.h>
#define UINT32_TYPE	unsigned int
#define UINT16_TYPE	unsigned short
#define _PATH_MAILDIR	"/var/mail"
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_DEFPATH	"/usr/bin:/usr/ucb"
#define _PATH_STDPATH	"/usr/bin:/usr/sbin:/usr/ucb"
#define MISSING_SETENV
#define HAS_FCNTL_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FCNTL
#define DEF_MAILBOX_LOCK "fcntl, dotlock"
#define HAS_FSYNC
#define HAS_DBM
#define NATIVE_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	DEF_DB_TYPE ":/etc/mail/aliases"
#ifndef NO_NIS
#define HAS_NIS
#endif
#define USE_SYS_SOCKIO_H
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin:/usr/ucb"
#define FIONREAD_IN_SYS_FILIO_H
#define DBM_NO_TRAILING_NULL
#define USE_STATVFS
#define STATVFS_IN_SYS_STATVFS_H
#define STRCASECMP_IN_STRINGS_H
#define SET_H_ERRNO(err) (set_h_errno(err))
#endif

#ifdef UW21				/* UnixWare 2.1.x */
#define SUPPORTED
#include <sys/types.h>
#define UINT32_TYPE	unsigned int
#define UINT16_TYPE	unsigned short
#define _PATH_MAILDIR   "/var/mail"
#define _PATH_BSHELL    "/bin/sh"
#define _PATH_DEFPATH   "/usr/bin:/usr/ucb"
#define _PATH_STDPATH   "/usr/bin:/usr/sbin:/usr/ucb"
#define MISSING_SETENV
#define HAS_FCNTL_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FCNTL
#define DEF_MAILBOX_LOCK "fcntl, dotlock"
#define HAS_FSYNC
#define HAS_DBM
#define NATIVE_DB_TYPE     "dbm"
#define ALIAS_DB_MAP    DEF_DB_TYPE ":/etc/mail/aliases"
#ifndef NO_NIS
#define HAS_NIS */
#endif
#define USE_SYS_SOCKIO_H
#define GETTIMEOFDAY(t) gettimeofday(t,NULL)
#define ROOT_PATH       "/bin:/usr/bin:/sbin:/usr/sbin:/usr/ucb"
#define FIONREAD_IN_SYS_FILIO_H
#define DBM_NO_TRAILING_NULL
#define USE_STATVFS
#define STATVFS_IN_SYS_STATVFS_H
#endif

 /*
  * AIX: a SYSV-flavored hybrid. NB: fcntl() and flock() access the same
  * underlying locking primitives.
  */
#if defined(AIX5) || defined(AIX6)
#define SUPPORTED
#include <sys/types.h>
#define UINT32_TYPE	unsigned int
#define UINT16_TYPE	unsigned short
#define MISSING_SETENV
#define USE_PATHS_H
#ifndef _PATH_BSHELL
#define _PATH_BSHELL	"/bin/sh"
#endif
#ifndef _PATH_MAILDIR
#define _PATH_MAILDIR   "/var/spool/mail"	/* paths.h lies */
#endif
#ifndef _PATH_DEFPATH
#define _PATH_DEFPATH	"/usr/bin:/usr/ucb"
#endif
#ifndef _PATH_STDPATH
#define _PATH_STDPATH	"/usr/bin:/usr/sbin:/usr/ucb"
#endif
#define HAS_FCNTL_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FCNTL
#define DEF_MAILBOX_LOCK "fcntl, dotlock"
#define USE_SYS_SELECT_H
#define HAS_FSYNC
#define HAS_DBM
#define NATIVE_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	DEF_DB_TYPE ":/etc/aliases"
#ifndef NO_NIS
#define HAS_NIS
#endif
#define HAS_SA_LEN
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin:/usr/ucb"
#define SOCKADDR_SIZE	socklen_t
#define SOCKOPT_SIZE	socklen_t
#define USE_STATVFS
#define STATVFS_IN_SYS_STATVFS_H
#define NATIVE_SENDMAIL_PATH "/usr/sbin/sendmail"
#define NATIVE_MAILQ_PATH "/usr/sbin/mailq"
#define NATIVE_NEWALIAS_PATH "/usr/sbin/newaliases"
#define NATIVE_COMMAND_DIR "/usr/sbin"
#define NATIVE_DAEMON_DIR "/usr/libexec/postfix"

 /*
  * XXX Need CMSG_SPACE() and CMSG_LEN() but don't want to drag in everything
  * that comes with _LINUX_SOURCE_COMPAT.
  */
#include <sys/socket.h>
#ifndef CMSG_SPACE
#define CMSG_SPACE(len) (_CMSG_ALIGN(sizeof(struct cmsghdr)) + _CMSG_ALIGN(len))
#endif
#ifndef CMSG_LEN
#define CMSG_LEN(len) (_CMSG_ALIGN(sizeof(struct cmsghdr)) + (len))
#endif
#ifndef NO_IPV6
#define HAS_IPV6
#endif
#define BROKEN_AI_PASSIVE_NULL_HOST
#define BROKEN_AI_NULL_SERVICE
#define USE_SYSV_POLL
#define MYMALLOC_FUZZ	1
#endif

#ifdef AIX4
#define SUPPORTED
#include <sys/types.h>
#define UINT32_TYPE	unsigned int
#define UINT16_TYPE	unsigned short
#define MISSING_SETENV
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_MAILDIR   "/var/spool/mail"	/* paths.h lies */
#define _PATH_DEFPATH	"/usr/bin:/usr/ucb"
#define _PATH_STDPATH	"/usr/bin:/usr/sbin:/usr/ucb"
#define HAS_FCNTL_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FCNTL
#define DEF_MAILBOX_LOCK "fcntl, dotlock"
#define USE_SYS_SELECT_H
#define HAS_FSYNC
#define HAS_DBM
#define NATIVE_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	DEF_DB_TYPE ":/etc/aliases"
#ifndef NO_NIS
#define HAS_NIS
#endif
#define HAS_SA_LEN
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define RESOLVE_H_NEEDS_STDIO_H
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin:/usr/ucb"
#define SOCKADDR_SIZE	size_t
#define SOCKOPT_SIZE	size_t
#define USE_STATVFS
#define STATVFS_IN_SYS_STATVFS_H
#define STRCASECMP_IN_STRINGS_H
#if 0
extern time_t time(time_t *);
extern int seteuid(uid_t);
extern int setegid(gid_t);
extern int initgroups(const char *, int);

#endif
#define NATIVE_SENDMAIL_PATH "/usr/lib/sendmail"
#define NATIVE_MAILQ_PATH "/usr/sbin/mailq"
#define NATIVE_NEWALIAS_PATH "/usr/sbin/newaliases"
#define NATIVE_COMMAND_DIR "/usr/sbin"
#define NATIVE_DAEMON_DIR "/usr/libexec/postfix"

#define CANT_USE_SEND_RECV_MSG
#endif

#ifdef AIX3
#define SUPPORTED
#include <sys/types.h>
#define UINT32_TYPE	unsigned int
#define UINT16_TYPE	unsigned short
#define MISSING_SETENV
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_MAILDIR   "/var/spool/mail"	/* paths.h lies */
#define _PATH_DEFPATH	"/usr/bin:/usr/ucb"
#define _PATH_STDPATH	"/usr/bin:/usr/sbin:/usr/ucb"
#define HAS_FCNTL_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FCNTL
#define DEF_MAILBOX_LOCK "fcntl, dotlock"
#define USE_SYS_SELECT_H
#define HAS_FSYNC
#define HAS_DBM
#define NATIVE_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	DEF_DB_TYPE ":/etc/aliases"
#ifndef NO_NIS
#define HAS_NIS
#endif
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

#define NATIVE_SENDMAIL_PATH "/usr/lib/sendmail"

#define CANT_USE_SEND_RECV_MSG
#endif

 /*
  * IRIX, a mix of System V Releases.
  */
#if defined(IRIX5) || defined(IRIX6)
#define SUPPORTED
#include <sys/types.h>
#define UINT32_TYPE	unsigned int
#define UINT16_TYPE	unsigned short
#define MISSING_SETENV
#define _PATH_MAILDIR	"/var/mail"
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_DEFPATH	"/usr/bin:/usr/bsd"
#define _PATH_STDPATH	"/usr/bin:/usr/sbin:/usr/bsd"
#define HAS_FCNTL_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FCNTL
#define DEF_MAILBOX_LOCK "fcntl, dotlock"
#define HAS_FSYNC
#define HAS_DBM
#define NATIVE_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	DEF_DB_TYPE ":/etc/aliases"
#ifndef NO_NIS
#define HAS_NIS
#endif
#define USE_SYS_SOCKIO_H		/* XXX check */
#define GETTIMEOFDAY(t)	gettimeofday(t)
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin:/usr/bsd"
#define FIONREAD_IN_SYS_FILIO_H		/* XXX check */
#define DBM_NO_TRAILING_NULL		/* XXX check */
#define USE_STATVFS
#define STATVFS_IN_SYS_STATVFS_H
#define BROKEN_WRITE_SELECT_ON_NON_BLOCKING_PIPE
#define CANT_USE_SEND_RECV_MSG
#endif

#if defined(IRIX5)
#define MISSING_USLEEP
#endif

#if defined(IRIX6)
#ifndef NO_IPV6
#define HAS_IPV6
#endif
#define HAS_POSIX_REGEXP
#define PIPES_CANT_FIONREAD
#endif

 /*
  * LINUX.
  */
#if defined(LINUX2) || defined(LINUX3) || defined(LINUX4)
#define SUPPORTED
#include <sys/types.h>
#define UINT32_TYPE	unsigned int
#define UINT16_TYPE	unsigned short
#include <features.h>
#define USE_PATHS_H
#define HAS_FLOCK_LOCK
#define HAS_FCNTL_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FLOCK
#define DEF_MAILBOX_LOCK "fcntl, dotlock"	/* RedHat >= 4.x */
#define HAS_FSYNC
#define HAS_DB
#define NATIVE_DB_TYPE	"hash"
#define ALIAS_DB_MAP	DEF_DB_TYPE ":/etc/aliases"
#ifndef NO_NIS
#define HAS_NIS
#endif
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin"
#define FIONREAD_IN_TERMIOS_H
#define USE_STATFS
#define STATFS_IN_SYS_VFS_H
#define PREPEND_PLUS_TO_OPTSTRING
#define HAS_POSIX_REGEXP
#define HAS_DLOPEN
#define NATIVE_SENDMAIL_PATH "/usr/sbin/sendmail"
#define NATIVE_MAILQ_PATH "/usr/bin/mailq"
#define NATIVE_NEWALIAS_PATH "/usr/bin/newaliases"
#define NATIVE_COMMAND_DIR "/usr/sbin"
#define NATIVE_DAEMON_DIR "/usr/libexec/postfix"
#ifdef __GLIBC_PREREQ
#define HAVE_GLIBC_API_VERSION_SUPPORT(maj, min) __GLIBC_PREREQ(maj, min)
#else
#define HAVE_GLIBC_API_VERSION_SUPPORT(maj, min) \
    ((__GLIBC__ << 16) + __GLIBC_MINOR__ >= ((maj) << 16) + (min))
#endif
#if HAVE_GLIBC_API_VERSION_SUPPORT(2, 1)
#define SOCKADDR_SIZE	socklen_t
#define SOCKOPT_SIZE	socklen_t
#endif
#ifndef NO_IPV6
#define HAS_IPV6
#if HAVE_GLIBC_API_VERSION_SUPPORT(2, 4)
/* Really 2.3.3 or later, but there's no __GLIBC_MICRO version macro. */
#define HAVE_GETIFADDRS
#else
#define HAS_PROCNET_IFINET6
#define _PATH_PROCNET_IFINET6 "/proc/net/if_inet6"
#endif
#endif
#include <linux/version.h>
#if !defined(KERNEL_VERSION)
#define KERNEL_VERSION(a,b,c) (LINUX_VERSION_CODE + 1)
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,2,0)) \
	|| (__GLIBC__ < 2)
#define CANT_USE_SEND_RECV_MSG
#define DEF_SMTP_CACHE_DEMAND	0
#else
#define CANT_WRITE_BEFORE_SENDING_FD
#endif
#define PREFERRED_RAND_SOURCE	"dev:/dev/urandom"	/* introduced in 1.1 */
#ifndef NO_EPOLL
#define EVENTS_STYLE	EVENTS_STYLE_EPOLL	/* introduced in 2.5 */
#endif
#define USE_SYSV_POLL
#ifndef NO_POSIX_GETPW_R
#if (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 1) \
	|| (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 1) \
	|| (defined(_BSD_SOURCE) && _BSD_SOURCE >= 1) \
	|| (defined(_SVID_SOURCE) && _SVID_SOURCE >= 1) \
	|| (defined(_POSIX_SOURCE) && _POSIX_SOURCE >= 1)
#define HAVE_POSIX_GETPW_R
#endif
#endif

#endif

#ifdef LINUX1
#define SUPPORTED
#include <sys/types.h>
#define UINT32_TYPE	unsigned int
#define UINT16_TYPE	unsigned short
#define USE_PATHS_H
#define HAS_FLOCK_LOCK
#define HAS_FCNTL_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FLOCK
#define DEF_MAILBOX_LOCK "dotlock"	/* verified RedHat 3.03 */
#define HAS_FSYNC
#define HAS_DB
#define NATIVE_DB_TYPE	"hash"
#define ALIAS_DB_MAP	DEF_DB_TYPE ":/etc/aliases"
#ifndef NO_NIS
#define HAS_NIS
#endif
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin"
#define FIONREAD_IN_TERMIOS_H		/* maybe unnecessary */
#define USE_STATFS
#define STATFS_IN_SYS_VFS_H
#define PREPEND_PLUS_TO_OPTSTRING
#define HAS_POSIX_REGEXP
#define NATIVE_SENDMAIL_PATH "/usr/sbin/sendmail"
#define NATIVE_MAILQ_PATH "/usr/bin/mailq"
#define NATIVE_NEWALIAS_PATH "/usr/bin/newaliases"
#define NATIVE_COMMAND_DIR "/usr/sbin"
#define NATIVE_DAEMON_DIR "/usr/libexec/postfix"
#define CANT_USE_SEND_RECV_MSG
#define DEF_SMTP_CACHE_DEMAND	0
#endif

 /*
  * GNU.
  */
#ifdef GNU0
#define SUPPORTED
#include <sys/types.h>
#include <features.h>
#define USE_PATHS_H
#define HAS_FCNTL_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FCNTL
#define DEF_MAILBOX_LOCK "fcntl, dotlock"	/* RedHat >= 4.x */
#define HAS_FSYNC
#define HAS_DB
#define NATIVE_DB_TYPE	"hash"
#define ALIAS_DB_MAP	DEF_DB_TYPE ":/etc/aliases"
#ifndef NO_NIS
#define HAS_NIS
#endif
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin"
#define FIONREAD_IN_TERMIOS_H
#define USE_STATFS
#define STATFS_IN_SYS_VFS_H
#define UNIX_DOMAIN_CONNECT_BLOCKS_FOR_ACCEPT
#define PREPEND_PLUS_TO_OPTSTRING
#define HAS_POSIX_REGEXP
#define HAS_DLOPEN
#define NATIVE_SENDMAIL_PATH "/usr/sbin/sendmail"
#define NATIVE_MAILQ_PATH "/usr/bin/mailq"
#define NATIVE_NEWALIAS_PATH "/usr/bin/newaliases"
#define NATIVE_COMMAND_DIR "/usr/sbin"
#ifdef DEBIAN
#define NATIVE_DAEMON_DIR	"/usr/lib/postfix"
#ifndef DEF_MANPAGE_DIR
#define DEF_MANPAGE_DIR		"/usr/share/man"
#endif
#ifndef DEF_SAMPLE_DIR
#define DEF_SAMPLE_DIR		"/usr/share/doc/postfix/examples"
#endif
#ifndef DEF_README_DIR
#define DEF_README_DIR		"/usr/share/doc/postfix"
#endif
#else
#define NATIVE_DAEMON_DIR "/usr/libexec/postfix"
#endif
#define SOCKADDR_SIZE	socklen_t
#define SOCKOPT_SIZE	socklen_t
#ifdef __FreeBSD_kernel__
#define HAS_DUPLEX_PIPE
#define HAS_ISSETUGID
#endif
#ifndef NO_IPV6
#define HAS_IPV6
#ifdef __FreeBSD_kernel__
#define HAVE_GETIFADDRS
#else
#define HAS_PROCNET_IFINET6
#define _PATH_PROCNET_IFINET6 "/proc/net/if_inet6"
#endif
#endif
#define CANT_USE_SEND_RECV_MSG
#define DEF_SMTP_CACHE_DEMAND	0
#define PREFERRED_RAND_SOURCE	"dev:/dev/urandom"
#endif

 /*
  * HPUX11 was copied from HPUX10, but can perhaps be trimmed down a bit.
  */
#ifdef HPUX11
#define SUPPORTED
#define USE_SIG_RETURN
#include <sys/types.h>
#define UINT32_TYPE	unsigned int
#define UINT16_TYPE	unsigned short
#define HAS_DBM
#define HAS_FCNTL_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FCNTL
#define DEF_MAILBOX_LOCK "fcntl, dotlock"
#define HAS_FSYNC
#define NATIVE_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	DEF_DB_TYPE ":/etc/mail/aliases"
#define ROOT_PATH	"/usr/bin:/sbin:/usr/sbin"
#define MISSING_SETENV
#ifndef NO_NIS
#define HAS_NIS
#endif
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
#define HAS_DLOPEN
#define NATIVE_SENDMAIL_PATH "/usr/sbin/sendmail"
#define NATIVE_MAILQ_PATH "/usr/bin/mailq"
#define NATIVE_NEWALIAS_PATH "/usr/bin/newaliases"
#define NATIVE_COMMAND_DIR "/usr/sbin"
#define NATIVE_DAEMON_DIR "/usr/libexec/postfix"
#endif

#ifdef HPUX10
#define SUPPORTED
#define USE_SIG_RETURN
#include <sys/types.h>
#define UINT32_TYPE	unsigned int
#define UINT16_TYPE	unsigned short
#define HAS_DBM
#define HAS_FCNTL_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FCNTL
#define DEF_MAILBOX_LOCK "fcntl, dotlock"
#define HAS_FSYNC
#define NATIVE_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	DEF_DB_TYPE ":/etc/mail/aliases"
#define ROOT_PATH	"/usr/bin:/sbin:/usr/sbin"
#define MISSING_SETENV
#ifndef NO_NIS
#define HAS_NIS
#endif
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
#define HAS_SHL_LOAD
#define NATIVE_SENDMAIL_PATH "/usr/sbin/sendmail"
#define NATIVE_MAILQ_PATH "/usr/bin/mailq"
#define NATIVE_NEWALIAS_PATH "/usr/bin/newaliases"
#define NATIVE_COMMAND_DIR "/usr/sbin"
#define NATIVE_DAEMON_DIR "/usr/libexec/postfix"
#endif

#ifdef HPUX9
#define SUPPORTED
#define USE_SIG_RETURN
#include <sys/types.h>
#define UINT32_TYPE	unsigned int
#define UINT16_TYPE	unsigned short
#define HAS_DBM
#define HAS_FCNTL_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FCNTL
#define DEF_MAILBOX_LOCK "fcntl, dotlock"
#define HAS_FSYNC
#ifndef NO_NIS
#define HAS_NIS
#endif
#define MISSING_SETENV
#define MISSING_RLIMIT_FSIZE
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define NATIVE_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	DEF_DB_TYPE ":/usr/lib/aliases"
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
#define HAS_SHL_LOAD
#define NATIVE_SENDMAIL_PATH "/usr/bin/sendmail"
#define NATIVE_MAILQ_PATH "/usr/bin/mailq"
#define NATIVE_NEWALIAS_PATH "/usr/bin/newaliases"
#define NATIVE_DAEMON_DIR "/usr/libexec/postfix"
#endif

 /*
  * NEXTSTEP3, without -lposix, because its naming service is broken.
  */
#ifdef NEXTSTEP3
#define SUPPORTED
#include <sys/types.h>
#define UINT32_TYPE	unsigned int
#define UINT16_TYPE	unsigned short
#define HAS_DBM
#define HAS_FLOCK_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FLOCK
#define DEF_MAILBOX_LOCK "flock, dotlock"
#define USE_STATFS
#define HAVE_SYS_DIR_H
#define STATFS_IN_SYS_VFS_H
#define HAS_FSYNC
#ifndef NO_NIS
#define HAS_NIS
#endif
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
#define NATIVE_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	"netinfo:/aliases"
#include <libc.h>
#define MISSING_POSIX_S_IS
#define MISSING_POSIX_S_MODES
/* It's amazing what is all missing...	*/
#define isascii(c)	((unsigned)(c)<=0177)
extern int opterr;
typedef unsigned short mode_t;

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
#define UINT32_TYPE	unsigned int
#define UINT16_TYPE	unsigned short
#define HAS_DBM
#define HAS_FLOCK_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FLOCK
#define DEF_MAILBOX_LOCK "flock, dotlock"
#define USE_STATFS
#define HAVE_SYS_DIR_H
#define STATFS_IN_SYS_VFS_H
#define HAS_FSYNC
#ifndef NO_NIS
#define HAS_NIS
#endif
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
#define NATIVE_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	"netinfo:/aliases"
#include <libc.h>
#define MISSING_POSIX_S_IS
#define MISSING_POSIX_S_MODES
/* It's amazing what is all missing...	*/
#define isascii(c)	((unsigned)(c)<=0177)
extern int opterr;
typedef unsigned short mode_t;

#define MISSING_PID_T
#define MISSING_STRFTIME_E
#define FD_CLOEXEC	1
#define O_NONBLOCK	O_NDELAY
#define WEXITSTATUS(x)	((x).w_retcode)
#define WTERMSIG(x)	((x).w_termsig)
#endif

#ifdef ReliantUnix543
#define SUPPORTED
#include <sys/types.h>
#define UINT32_TYPE	unsigned int
#define UINT16_TYPE	unsigned short
#define MISSING_SETENV
#define _PATH_DEFPATH	"/usr/bin:/usr/ucb"
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_MAILDIR	"/var/spool/mail"
#define HAS_FCNTL_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FCNTL
#define DEF_MAILBOX_LOCK "fcntl, dotlock"
#define HAS_FSYNC
#define FIONREAD_IN_SYS_FILIO_H
#define USE_SYS_SOCKIO_H
#define HAS_DBM
#define NATIVE_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	DEF_DB_TYPE ":/var/adm/sendmail/aliases"
extern int optind;			/* XXX use <getopt.h> */
extern char *optarg;			/* XXX use <getopt.h> */
extern int opterr;			/* XXX use <getopt.h> */

#ifndef NO_NIS
#define HAS_NIS
#endif
#define GETTIMEOFDAY(t) gettimeofday(t)
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin:/usr/ucb"
#define USE_STATVFS
#define STATVFS_IN_SYS_STATVFS_H
#define MISSING_USLEEP
#endif

#ifdef DCOSX1				/* Siemens Pyramid */
#define SUPPORTED
#include <sys/types.h>
#define UINT32_TYPE	unsigned int
#define UINT16_TYPE	unsigned short
#define _PATH_MAILDIR	"/var/mail"
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_DEFPATH	"/usr/bin:/usr/ucb"
#define _PATH_STDPATH	"/usr/bin:/usr/sbin:/usr/ucb"
#define MISSING_SETENV
#define HAS_FCNTL_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FCNTL
#define DEF_MAILBOX_LOCK "fcntl, dotlock"
#define HAS_FSYNC
#define NATIVE_DB_TYPE	"hash"
#define ALIAS_DB_MAP	DEF_DB_TYPE ":/etc/aliases"
/* Uncomment the following line if you have NIS package installed */
/* #define HAS_NIS */
#define USE_SYS_SOCKIO_H
#define GETTIMEOFDAY(t) gettimeofday(t,NULL)
#define ROOT_PATH	"/bin:/usr/bin:/sbin:/usr/sbin:/usr/ucb"
#define FIONREAD_IN_SYS_FILIO_H
#define DBM_NO_TRAILING_NULL
#define USE_STATVFS
#define STATVFS_IN_SYS_STATVFS_H
#ifndef S_ISSOCK
#define S_ISSOCK(mode)	((mode&0xF000) == 0xC000)
#endif
#endif

#ifdef SCO5
#define SUPPORTED
#include <sys/types.h>
#include <sys/socket.h>
extern int h_errno;

#define UINT32_TYPE	unsigned int
#define UINT16_TYPE	unsigned short
#define _PATH_MAILDIR	"/usr/spool/mail"
#define _PATH_BSHELL	"/bin/sh"
#define _PATH_DEFPATH	"/bin:/usr/bin"
#define USE_PATHS_H
#define HAS_FCNTL_LOCK
#define INTERNAL_LOCK	MYFLOCK_STYLE_FCNTL
#define DEF_MAILBOX_LOCK "fcntl, dotlock"
#define HAS_FSYNC
#define HAS_DBM
#define NATIVE_DB_TYPE	"dbm"
#define ALIAS_DB_MAP	DEF_DB_TYPE ":/etc/mail/aliases"
#define DBM_NO_TRAILING_NULL
#ifndef NO_NIS
#define HAS_NIS
#endif
#define GETTIMEOFDAY(t)	gettimeofday(t,(struct timezone *) 0)
#define ROOT_PATH	"/bin:/etc:/usr/bin:/tcb/bin"
#define USE_STATVFS
#define STATVFS_IN_SYS_STATVFS_H
#define MISSING_SETENV
#define STRCASECMP_IN_STRINGS_H
/* SCO5 misses just S_ISSOCK, the others are there
 * Use C_ISSOCK definition from cpio.h.
 */
#include <cpio.h>
#define S_ISSOCK(mode)	(((mode) & (S_IFMT)) == (C_ISSOCK))
#define CANT_USE_SEND_RECV_MSG
#define DEF_SMTP_CACHE_DEMAND	0
#endif

 /*
  * We're not going to try to guess like configure does.
  */
#ifndef SUPPORTED
#error "unsupported platform"
#endif

 /*
  * Allow command line flags to override native settings
  */
#ifndef DEF_COMMAND_DIR
#ifdef NATIVE_COMMAND_DIR
#define DEF_COMMAND_DIR NATIVE_COMMAND_DIR
#endif
#endif

#ifndef DEF_DAEMON_DIR
#ifdef NATIVE_DAEMON_DIR
#define DEF_DAEMON_DIR NATIVE_DAEMON_DIR
#endif
#endif

#ifndef DEF_SENDMAIL_PATH
#ifdef NATIVE_SENDMAIL_PATH
#define DEF_SENDMAIL_PATH NATIVE_SENDMAIL_PATH
#endif
#endif

#ifndef DEF_MAILQ_PATH
#ifdef NATIVE_MAILQ_PATH
#define DEF_MAILQ_PATH NATIVE_MAILQ_PATH
#endif
#endif

#ifndef DEF_NEWALIAS_PATH
#ifdef NATIVE_NEWALIAS_PATH
#define DEF_NEWALIAS_PATH NATIVE_NEWALIAS_PATH
#endif
#endif

#ifndef DEF_DB_TYPE
#define DEF_DB_TYPE	NATIVE_DB_TYPE
#endif

#define CAST_ANY_PTR_TO_INT(cptr)	((int) (long) (cptr))
#define CAST_INT_TO_VOID_PTR(ival)	((void *) (long) (ival))

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

 /*
  * Check for required but missing definitions.
  */
#if !defined(HAS_FCNTL_LOCK) && !defined(HAS_FLOCK_LOCK)
#error "define HAS_FCNTL_LOCK and/or HAS_FLOCK_LOCK"
#endif

#if !defined(DEF_MAILBOX_LOCK)
#error "define DEF_MAILBOX_LOCK"
#endif

#if !defined(INTERNAL_LOCK)
#error "define INTERNAL_LOCK"
#endif

#if defined(USE_STATFS) && defined(USE_STATVFS)
#error "define USE_STATFS or USE_STATVFS, not both"
#endif

#if !defined(USE_STATFS) && !defined(USE_STATVFS)
#error "define USE_STATFS or USE_STATVFS"
#endif

 /*
  * Defaults for systems that pre-date IPv6 support.
  */
#ifndef __NetBSD__
#ifndef HAS_IPV6
#define EMULATE_IPV4_ADDRINFO
#define MISSING_INET_PTON
#define MISSING_INET_NTOP
extern const char *inet_ntop(int, const void *, char *, size_t);
extern int inet_pton(int, const char *, void *);

#endif
#endif

 /*
  * Workaround: after a watchdog alarm signal, wake up from select/poll/etc.
  * by writing to a pipe. Solaris needs this, and HP-UX apparently, too. The
  * run-time cost is negligible so we just turn it on for all systems. As a
  * side benefit, making this code system-independent will simplify the
  * detection of bit-rot problems.
  */
#ifndef NO_WATCHDOG_PIPE
#define USE_WATCHDOG_PIPE
#endif

 /*
  * If we don't have defined a preferred random device above, but the system
  * has /dev/urandom, then we use that.
  */
#if !defined(PREFERRED_RAND_SOURCE) && defined(HAS_DEV_URANDOM)
#define PREFERRED_RAND_SOURCE	"dev:/dev/urandom"
#endif

 /*
  * Defaults for systems without kqueue, /dev/poll or epoll support.
  * master/multi-server.c and *qmgr/qmgr_transport.c depend on this.
  */
#if !defined(EVENTS_STYLE)
#define EVENTS_STYLE	EVENTS_STYLE_SELECT
#endif

#define EVENTS_STYLE_SELECT	1	/* Traditional BSD select */
#define EVENTS_STYLE_KQUEUE	2	/* FreeBSD kqueue */
#define EVENTS_STYLE_DEVPOLL	3	/* Solaris /dev/poll */
#define EVENTS_STYLE_EPOLL	4	/* Linux epoll */

 /*
  * We use poll() for read/write time limit enforcement on modern systems. We
  * use select() on historical systems without poll() support. And on systems
  * where poll() is not implemented for some file handle types, we try to use
  * select() as a fall-back solution (MacOS X needs this).
  */
#if !defined(USE_SYSV_POLL) && !defined(USE_SYSV_POLL_THEN_SELECT)
#define USE_BSD_SELECT
#endif

 /*
  * The Postfix 2.9 post-install workaround assumes that the inet_protocols
  * default value is "ipv4" when Postfix is compiled without IPv6 support.
  */
#ifndef DEF_INET_PROTOCOLS
#ifdef HAS_IPV6
#define DEF_INET_PROTOCOLS	INET_PROTO_NAME_ALL
#else
#define DEF_INET_PROTOCOLS	INET_PROTO_NAME_IPV4
#endif
#endif

 /*
  * Defaults for systems that pre-date POSIX socklen_t.
  */
#ifndef SOCKADDR_SIZE
#define SOCKADDR_SIZE	int
#endif

#ifndef SOCKOPT_SIZE
#define SOCKOPT_SIZE	int
#endif

 /*
  * Defaults for normal systems.
  */
#ifndef LOCAL_LISTEN
#define LOCAL_LISTEN	unix_listen
#define LOCAL_ACCEPT	unix_accept
#define LOCAL_CONNECT	unix_connect
#define LOCAL_TRIGGER	unix_trigger
#define LOCAL_SEND_FD	unix_send_fd
#define LOCAL_RECV_FD	unix_recv_fd
#endif

#if !defined (HAVE_SYS_NDIR_H) && !defined (HAVE_SYS_DIR_H) \
	&& !defined (HAVE_NDIR_H)
#define HAVE_DIRENT_H
#endif

#ifndef WAIT_STATUS_T
typedef int WAIT_STATUS_T;

#define NORMAL_EXIT_STATUS(status)	((status) == 0)
#endif

#ifdef NO_POSIX_GETPW_R
#undef HAVE_POSIX_GETPW_R
#endif

#ifdef NO_DB
#undef HAS_DB
#endif

#ifndef OCTAL_TO_UNSIGNED
#define OCTAL_TO_UNSIGNED(res, str)	((res) = strtoul((str), (char **) 0, 8))
#endif

 /*
  * Avoid useless type mis-matches when using sizeof in an integer context.
  */
#define INT_SIZEOF(foo)	((int) sizeof(foo))

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

#ifndef HAS_CLOSEFROM
extern int closefrom(int);

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
#define S_ISLNK(mode)	(((mode) & (_S_IFMT)) == (_S_IFLNK))
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
  * Memory alignment of memory allocator results. By default we align for
  * doubles.
  */
#ifndef ALIGN_TYPE
#if defined(__hpux) && defined(__ia64)
#define ALIGN_TYPE	__float80
#elif defined(__ia64__)
#define ALIGN_TYPE	long double
#else
#define ALIGN_TYPE	double
#endif
#endif

 /*
  * Clang-style attribute tests.
  * 
  * XXX Without the unconditional test below, gcc 4.6 will barf on ``elif
  * defined(__clang__) && __has_attribute(__whatever__)'' with error message
  * ``missing binary operator before token "("''.
  */
#ifndef __has_attribute
#define __has_attribute(x) 0
#endif					/* __has_attribute */

 /*
  * Need to specify what functions never return, so that the compiler can
  * warn for missing initializations and other trouble. However, OPENSTEP4
  * gcc 2.7.x cannot handle this so we define this only if NORETURN isn't
  * already defined above.
  * 
  * Data point: gcc 2.7.2 has __attribute__ (Wietse Venema) but gcc 2.6.3 does
  * not (Clive Jones). So we'll set the threshold at 2.7.
  */
#ifndef NORETURN
#if (__GNUC__ == 2 && __GNUC_MINOR__ >= 7) || __GNUC__ >= 3
#define NORETURN	void __attribute__((__noreturn__))
#elif defined(__clang__) && __has_attribute(__noreturn__)
#define NORETURN	void __attribute__((__noreturn__))
#else
#define NORETURN	void
#endif
#endif					/* NORETURN */

 /*
  * Turn on format string argument checking. This is more accurate than
  * printfck, but it misses #ifdef-ed code. XXX I am just guessing at what
  * gcc versions support this. In order to turn this off for some platforms,
  * specify #define PRINTFLIKE and #define SCANFLIKE in the system-dependent
  * sections above.
  */
#ifndef PRINTFLIKE
#if (__GNUC__ == 2 && __GNUC_MINOR__ >= 7) || __GNUC__ >= 3
#define PRINTFLIKE(x,y) __attribute__ ((format (printf, (x), (y))))
#elif defined(__clang__) && __has_attribute(__format__)
#define PRINTFLIKE(x,y)	__attribute__ ((__format__ (__printf__, (x), (y))))
#else
#define PRINTFLIKE(x,y)
#endif
#endif					/* PRINTFLIKE */

#ifndef SCANFLIKE
#if (__GNUC__ == 2 && __GNUC_MINOR__ >= 7) || __GNUC__ >= 3
#define SCANFLIKE(x,y) __attribute__ ((format (scanf, (x), (y))))
#elif defined(__clang__) && __has_attribute(__format__)
#define SCANFLIKE(x,y) __attribute__ ((__format__ (__scanf__, (x), (y))))
#else
#define SCANFLIKE(x,y)
#endif
#endif					/* SCANFLIKE */

 /*
  * Some gcc implementations don't grok these attributes with pointer to
  * function. Again, wild guess of what is supported. To override, specify
  * #define PRINTFPTRLIKE  in the system-dependent sections above.
  */
#ifndef PRINTFPTRLIKE
#if (__GNUC__ >= 3)			/* XXX Rough estimate */
#define PRINTFPTRLIKE(x,y) PRINTFLIKE(x,y)
#elif defined(__clang__) && __has_attribute(__format__)
#define PRINTFPTRLIKE(x,y) __attribute__ ((__format__ (__printf__, (x), (y))))
#else
#define PRINTFPTRLIKE(x,y)
#endif
#endif

 /*
  * Compiler optimization hint. This makes sense only for code in a
  * performance-critical loop.
  */
#ifndef EXPECTED
#if defined(__GNUC__) && (__GNUC__ > 2)
#define EXPECTED(x)	__builtin_expect(!!(x), 1)
#define UNEXPECTED(x)	__builtin_expect(!!(x), 0)
#else
#define EXPECTED(x)	(x)
#define UNEXPECTED(x)	(x)
#endif
#endif

 /*
  * Warn about ignored function result values that must never be ignored.
  * Typically, this is for error results from "read" functions that normally
  * write to output parameters (for example, stat- or scanf-like functions)
  * or from functions that have other useful side effects (for example,
  * fseek- or rename-like functions).
  * 
  * DO NOT use this for functions that write to a stream; it is entirely
  * legitimate to detect write errors with fflush() or fclose() only. On the
  * other hand most (but not all) functions that read from a stream must
  * never ignore result values.
  * 
  * XXX Prepending "(void)" won't shut up GCC. Clang behaves as expected.
  */
#if ((__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || __GNUC__ > 3)
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#elif defined(__clang__) && __has_attribute(warn_unused_result)
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define WARN_UNUSED_RESULT
#endif

 /*
  * ISO C says that the "volatile" qualifier protects against optimizations
  * that cause longjmp() to clobber local variables.
  */
#ifndef NOCLOBBER
#define NOCLOBBER volatile
#endif

 /*
  * Bit banging!! There is no official constant that defines the INT_MAX
  * equivalent of the off_t type. Wietse came up with the following macro
  * that works as long as off_t is some two's complement number.
  * 
  * Note, however, that C99 permits signed integer representations other than
  * two's complement.
  */
#include <limits.h>
#define __MAXINT__(T) ((T) (((((T) 1) << ((sizeof(T) * CHAR_BIT) - 1)) ^ ((T) -1))))
#ifndef OFF_T_MAX
#define OFF_T_MAX __MAXINT__(off_t)
#endif

 /*
  * Setting globals like h_errno can be problematic when Postfix is linked
  * with multi-threaded libraries.
  */
#ifndef SET_H_ERRNO
#define SET_H_ERRNO(err) (h_errno = (err))
#endif

 /*
  * Don't mix socket message send/receive calls with socket stream read/write
  * calls. The fact that you can get away with it only on some stacks implies
  * that there is no long-term guarantee.
  */
#ifndef CAN_WRITE_BEFORE_SENDING_FD
#define CANT_WRITE_BEFORE_SENDING_FD
#endif

 /*
  * FreeBSD sendmsg(2) says that after sending a file descriptor, the sender
  * must not immediately close the descriptor, otherwise it may close the
  * descriptor before it is actually sent.
  */
#ifndef DONT_WAIT_AFTER_SENDING_FD
#define MUST_READ_AFTER_SENDING_FD
#endif

 /*
  * Hope for the best.
  */
#ifndef UINT32_TYPE
#define	UINT32_TYPE uint32_t
#define UINT16_TYPE uint16_t
#endif
#define UINT32_SIZE	4
#define UINT16_SIZE	2

 /*
  * For the sake of clarity.
  */
#ifndef HAVE_CONST_CHAR_STAR
typedef const char *CONST_CHAR_STAR;

#endif

 /*
  * Safety. On some systems, ctype.h misbehaves with non-ASCII or negative
  * characters. More importantly, Postfix uses the ISXXX() macros to ensure
  * protocol compliance, so we have to rule out non-ASCII characters.
  * 
  * XXX The (unsigned char) casts in isalnum() etc arguments are unnecessary
  * because the ISASCII() guard already ensures that the values are
  * non-negative; the casts are done anyway to shut up chatty compilers.
  */
#define ISASCII(c)	isascii(_UCHAR_(c))
#define _UCHAR_(c)	((unsigned char)(c))
#define ISALNUM(c)	(ISASCII(c) && isalnum((unsigned char)(c)))
#define ISALPHA(c)	(ISASCII(c) && isalpha((unsigned char)(c)))
#define ISCNTRL(c)	(ISASCII(c) && iscntrl((unsigned char)(c)))
#define ISDIGIT(c)	(ISASCII(c) && isdigit((unsigned char)(c)))
#define ISGRAPH(c)	(ISASCII(c) && isgraph((unsigned char)(c)))
#define ISLOWER(c)	(ISASCII(c) && islower((unsigned char)(c)))
#define ISPRINT(c)	(ISASCII(c) && isprint((unsigned char)(c)))
#define ISPUNCT(c)	(ISASCII(c) && ispunct((unsigned char)(c)))
#define ISSPACE(c)	(ISASCII(c) && isspace((unsigned char)(c)))
#define ISUPPER(c)	(ISASCII(c) && isupper((unsigned char)(c)))
#define TOLOWER(c)	(ISUPPER(c) ? tolower((unsigned char)(c)) : (c))
#define TOUPPER(c)	(ISLOWER(c) ? toupper((unsigned char)(c)) : (c))

 /*
  * Character sets for parsing.
  */
#define CHARS_COMMA_SP	", \t\r\n"	/* list separator */
#define CHARS_SPACE	" \t\r\n"	/* word separator */
#define CHARS_BRACE	"{}"		/* grouping */

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
