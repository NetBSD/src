/* $NetBSD: osf1.h,v 1.2 1999/04/26 01:24:26 cgd Exp $ */

/*
 * Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _COMPAT_OSF1_OSF1_H_
#define _COMPAT_OSF1_OSF1_H_

/*
 * Collected OSF/1 definitions and structures, sorted by OSF/1 header.
 * Error numbers (errno.h) aren't here, since they're likely to change
 * (additions) more often.
 *
 * This file is up to date as of Digital UNIX V4.0.
 */

#include <sys/types.h>
#include <compat/osf1/osf1_errno.h>

/* type definitions used by structures */

typedef int32_t		osf1_dev_t;
typedef u_int32_t	osf1_ino_t;
typedef u_int32_t	osf1_mode_t;
typedef u_int16_t	osf1_nlink_t;
typedef u_int32_t	osf1_uid_t;
typedef u_int32_t	osf1_gid_t;
typedef u_int64_t	osf1_off_t;
typedef int32_t		osf1_time_t;
typedef int32_t		osf1_int;
typedef u_int32_t	osf1_uint_t;
typedef u_int64_t	osf1_sigset_t;
typedef u_int64_t	osf1_size_t;
typedef void		*osf1_void_ptr;	/* XXX hard to fix size */
typedef void		*osf1_fcn_ptr;	/* XXX hard to fix size, bogus */


/* fcntl.h */

/* fcntl ops */
#define OSF1_F_DUPFD		0
#define OSF1_F_GETFD		1	/* uses flags, see below */
#define OSF1_F_SETFD		2	/* uses flags, see below */
#define OSF1_F_GETFL		3	/* uses flags, see below */
#define OSF1_F_SETFL		4	/* uses flags, see below */
#define OSF1_F_GETOWN		5
#define OSF1_F_SETOWN		6
#define OSF1_F_GETLK		7	/* uses osf1_flock, see below */
#define OSF1_F_SETLK		8	/* uses osf1_flock, see below */
#define OSF1_F_SETLKW		9	/* uses osf1_flock, see below */
#define OSF1_F_RGETLK		10	/* [lock mgr op] */
#define OSF1_F_RSETLK		11	/* [lock mgr op] */
#define OSF1_F_CNVT		12	/* [lock mgr op] */
#define OSF1_F_RSETLKW		13	/* [lock mgr op] */
#define OSF1_F_PURGEFS		14	/* [lock mgr op] */
#define OSF1_F_PURGENFS		15	/* [DECsafe op] */

/* fcntl GETFD/SETFD flags */
#define OSF1_FD_CLOEXEC		1

/* fcntl GETFL/SETFL flags */
/* XXX */

/* struct osf1_flock, for GETLK/SETLK/SETLKW */
/* XXX */

/* open flags */
#define OSF1_O_RDONLY		0x00000000
#define OSF1_O_WRONLY		0x00000001
#define OSF1_O_RDWR		0x00000002
#define OSF1_O_ACCMODE		0x00000003	/* mask of RD and WR bits */
#define OSF1_O_NONBLOCK		0x00000004
#define OSF1_O_APPEND		0x00000008
/* no				0x00000010 */
#define OSF1_O_DEFER		0x00000020
/* no				0x00000040 */
/* no				0x00000080 */
/* no				0x00000100 */
#define OSF1_O_CREAT		0x00000200
#define OSF1_O_TRUNC		0x00000400
#define OSF1_O_EXCL		0x00000800
#define OSF1_O_NOCTTY		0x00001000
#define OSF1_O_SYNC		0x00004000
#define OSF1_O_NDELAY		0x00008000
#define OSF1_O_DRD		0x00008000	/* == O_NDELAY, DON'T USE */
/* no				0x00010000 */
/* no				0x00020000 */
/* no				0x00040000 */
#define OSF1_O_DSYNC		0x00080000
#define OSF1_O_RSYNC		0x00100000
/* no				0x00200000+ */


/* ioctl.h */

#define OSF1_IOCPARM_MASK	0x1fff
#define OSF1_IOCPARM_LEN(x)	(((x) >> 16) & OSF1_IOCPARM_MASK)
#define OSF1_IOCGROUP(x)	(((x) >> 8) & 0xff)
#define OSF1_IOCCMD(x)          ((x) & 0xff)

#define OSF1_IOCPARM_MAX	8192
#define OSF1_IOC_VOID		0x20000000
#define OSF1_IOC_OUT		0x40000000
#define OSF1_IOC_IN		0x80000000
#define OSF1_IOC_INOUT		(OSF1_IOC_IN|OSF1_IOC_OUT)
#define OSF1_IOC_DIRMASK	0xe0000000


/* mman.h */

/* protection mask */
#define OSF1_PROT_NONE		0		/* pseudo-flag */
#define	OSF1_PROT_READ		0x0001
#define	OSF1_PROT_WRITE		0x0002
#define	OSF1_PROT_EXEC		0x0004

/* mmap flags */
#define OSF1_MAP_SHARED		0x0001
#define OSF1_MAP_PRIVATE	0x0002

#define OSF1_MAP_FILE		0		/* pseudo-flag */
#define OSF1_MAP_ANON		0x0010
#define OSF1_MAP_TYPE		0x00f0

#define OSF1_MAP_FIXED		0x0100
#define OSF1_MAP_VARIABLE	0		/* pseudo-flag */

#define OSF1_MAP_HASSEMAPHORE	0x0200
#define OSF1_MAP_INHERIT	0x0400
#define OSF1_MAP_UNALIGNED	0x0800


/* mount.h */

#if 0
osf1_mount.c:struct osf1_statfs {
osf1_mount.c:struct osf1_ufs_args {
osf1_mount.c:struct osf1_cdfs_args {
osf1_mount.c:struct osf1_mfs_args {
osf1_mount.c:struct osf1_nfs_args {
#endif


/* reboot.h */

/* reboot flags */
#define OSF1_RB_AUTOBOOT	0		/* pseudo-flag */

#define OSF1_RB_ASKNAME		0x0001
#define OSF1_RB_SINGLE		0x0002
#define OSF1_RB_NOSYNC		0x0004
#define OSF1_RB_KDB		0x0004		/* == RB_NOSYNC; boot only? */
#define OSF1_RB_HALT		0x0008
#define OSF1_RB_INITNAME	0x0010
#define OSF1_RB_DFLTROOT	0x0020
#define OSF1_RB_ALTBOOT		0x0040
#define OSF1_RB_UNIPROC		0x0080
#define OSF1_RB_PARAM		0x0100
#define OSF1_RB_DUMP		0x0200


/* signal.h */

struct osf1_sigaction {
	osf1_fcn_ptr	sa_handler;
	osf1_sigset_t	sa_mask;
	osf1_int	sa_flags;
	osf1_int	sa_signo;
};

/* actually from sysmisc.h */
struct osf1_sigaltstack {
	osf1_void_ptr	ss_sp;
	osf1_int	ss_flags;
	osf1_size_t	ss_size;
};

/* sigaction flags */
#define OSF1_SA_ONSTACK		0x00000001
#define OSF1_SA_RESTART		0x00000002
#define OSF1_SA_NOCLDSTOP	0x00000004
#define OSF1_SA_NODEFER		0x00000008
#define OSF1_SA_RESETHAND	0x00000010
#define OSF1_SA_NOCLDWAIT	0x00000020
#define OSF1_SA_SIGINFO		0x00000040

/* sigaltstack flags */
#define OSF1_SS_ONSTACK		0x00000001
#define OSF1_SS_DISABLE		0x00000002
#define OSF1_SS_NOMASK		0x00000004
#define OSF1_SS_UCONTEXT	0x00000008


/* socket.h */

/* max message iov len */
#define	OSF1_MSG_MAXIOVLEN	16

/* send/recv-family message flags */
#define OSF1_MSG_OOB		0x0001
#define OSF1_MSG_PEEK		0x0002
#define OSF1_MSG_DONTROUTE	0x0004
#define OSF1_MSG_EOR		0x0008
#define OSF1_MSG_TRUNC		0x0010
#define OSF1_MSG_CTRUNC		0x0020
#define OSF1_MSG_WAITALL	0x0040


/* stat.h */

struct osf1_stat {
	osf1_dev_t	st_dev;
	osf1_ino_t	st_ino;
	osf1_mode_t	st_mode;
	osf1_nlink_t	st_nlink;
	osf1_uid_t	st_uid;
	osf1_gid_t	st_gid;
	osf1_dev_t	st_rdev;
	osf1_off_t	st_size;
	osf1_time_t	st_atime_sec;
	osf1_int	st_spare1;
	osf1_time_t	st_mtime_sec;
	osf1_int	st_spare2;
	osf1_time_t	st_ctime_sec;
	osf1_int	st_spare3;
	osf1_uint_t	st_blksize;
	osf1_int	st_blocks;
	osf1_uint_t	st_flags;
	osf1_uint_t	st_gen;
};


/* uio.h */

/*
 * The X/Open version of this uses size_t iov_len, but we can't count on
 * the not-in-int bits being zero.  (The non-X/Open version uses int.)
 */
struct osf1_iovec {
	osf1_void_ptr	iov_base;
	osf1_int	iov_len;
};

#endif /* _COMPAT_OSF1_OSF1_H_ */
