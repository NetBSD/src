/*	$NetBSD: sparc32.h,v 1.2 1998/09/07 01:38:03 eeh Exp $	*/

/*
 * Copyright (c) 1998 Matthew R. Green
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _COMPAT_SPARC32_SPARC32_H_
#define _COMPAT_SPARC32_SPARC32_H_

/*
 * SPARC 32-bit compatibility module.
 */

#include <sys/mount.h>

/*
 * first, define all the types we need.
 */

typedef int32_t sparc32_long;
typedef u_int32_t sparc32_u_long;

typedef u_int32_t sparc32_clock_t;
typedef u_int32_t sparc32_size_t;
typedef int32_t sparc32_ssize_t;
typedef int32_t sparc32_clockid_t;
typedef int32_t sparc32_caddr_t;
typedef int32_t sparc32_key_t;

/* all pointers are int32_t */

typedef int32_t sparc32_voidp;
typedef int32_t sparc32_u_shortp;
typedef int32_t sparc32_charp;
typedef int32_t sparc32_u_charp;
typedef int32_t sparc32_charpp;
typedef int32_t sparc32_size_tp;
typedef int32_t sparc32_intp;
typedef int32_t sparc32_longp;
typedef int32_t sparc32_caddrp;
typedef int32_t sparc32_caddr;
typedef int32_t sparc32_gid_tp;
typedef int32_t sparc32_fsid_tp_t;

/*
 * now, the compatibility structures and their fake pointer types.
 */

/* from <sys/types.h> */
typedef int32_t sparc32_fd_setp_t;

/* from <sys/uio.h> */
typedef int32_t sparc32_iovecp_t;
struct sparc32_iovec {
	sparc32_voidp	iov_base;	/* Base address. */
	size_t	 iov_len;	/* Length. */
};

/* from <sys/time.h> */
typedef int32_t sparc32_timer_t;

typedef int32_t sparc32_timespecp_t;
struct sparc32_timespec {
	time_t	tv_sec;			/* seconds */
	sparc32_long	tv_nsec;	/* and nanoseconds */
};

typedef int32_t sparc32_timevalp_t;
struct sparc32_timeval {
	sparc32_long	tv_sec;		/* seconds */
	sparc32_long	tv_usec;	/* and microseconds */
};

typedef int32_t sparc32_timezonep_t;
struct sparc32_timezone {
	int	tz_minuteswest;	/* minutes west of Greenwich */
	int	tz_dsttime;	/* type of dst correction */
};

typedef int32_t sparc32_itimervalp_t;
struct	sparc32_itimerval {
	struct	sparc32_timeval it_interval;	/* timer interval */
	struct	sparc32_timeval it_value;	/* current value */
};

/* from <sys/mount.h> */
typedef int32_t sparc32_fidp_t;

typedef int32_t sparc32_fhandlep_t;

typedef int32_t sparc32_statfsp_t;
struct sparc32_statfs {
	short	f_type;			/* type of file system */
	u_short	f_flags;		/* copy of mount flags */
	sparc32_long	f_bsize;	/* fundamental file system block size */
	sparc32_long	f_iosize;	/* optimal transfer block size */
	sparc32_long	f_blocks;	/* total data blocks in file system */
	sparc32_long	f_bfree;	/* free blocks in fs */
	sparc32_long	f_bavail;	/* free blocks avail to non-superuser */
	sparc32_long	f_files;	/* total file nodes in file system */
	sparc32_long	f_ffree;	/* free file nodes in fs */
	fsid_t	f_fsid;			/* file system id */
	uid_t	f_owner;		/* user that mounted the file system */
	sparc32_long	f_spare[4];	/* spare for later */
	char	f_fstypename[MFSNAMELEN]; /* fs type name */
	char	f_mntonname[MNAMELEN];	  /* directory on which mounted */
	char	f_mntfromname[MNAMELEN];  /* mounted file system */
};

/* from <sys/poll.h> */
typedef int32_t sparc32_pollfdp_t;

/* from <sys/resource.h> */
typedef int32_t sparc32_rusagep_t;
struct	sparc32_rusage {
	struct sparc32_timeval ru_utime;/* user time used */
	struct sparc32_timeval ru_stime;/* system time used */
	sparc32_long	ru_maxrss;	/* max resident set size */
	sparc32_long	ru_ixrss;	/* integral shared memory size */
	sparc32_long	ru_idrss;	/* integral unshared data " */
	sparc32_long	ru_isrss;	/* integral unshared stack " */
	sparc32_long	ru_minflt;	/* page reclaims */
	sparc32_long	ru_majflt;	/* page faults */
	sparc32_long	ru_nswap;	/* swaps */
	sparc32_long	ru_inblock;	/* block input operations */
	sparc32_long	ru_oublock;	/* block output operations */
	sparc32_long	ru_msgsnd;	/* messages sent */
	sparc32_long	ru_msgrcv;	/* messages received */
	sparc32_long	ru_nsignals;	/* signals received */
	sparc32_long	ru_nvcsw;	/* voluntary context switches */
	sparc32_long	ru_nivcsw;	/* involuntary " */
};

typedef int32_t sparc32_orlimitp_t;

typedef int32_t sparc32_rlimitp_t;

/* from <sys/ipc.h> */
typedef int32_t sparc32_ipc_permp_t;
struct sparc32_ipc_perm {
	ushort	cuid;		/* creator user id */
	ushort	cgid;		/* creator group id */
	ushort	uid;		/* user id */
	ushort	gid;		/* group id */
	ushort	mode;		/* r/w permission */
	ushort	seq;		/* sequence # (to generate unique msg/sem/shm id) */
	sparc32_key_t	key;	/* user specified msg/sem/shm key */
};

/* from <sys/msg.h> */
typedef int32_t sparc32_msgp_t;
struct sparc32_msg {
	sparc32_msgp_t msg_next;	/* next msg in the chain */
	sparc32_long	msg_type;	/* type of this message */
    				/* >0 -> type of this message */
    				/* 0 -> free header */
	u_short	msg_ts;		/* size of this message */
	short	msg_spot;	/* location of start of msg in buffer */
};

typedef int32_t sparc32_msqid_dsp_t;
struct sparc32_msqid_ds {
	struct	sparc32_ipc_perm msg_perm;	/* msg queue permission bits */
	sparc32_msgp_t	msg_first;	/* first message in the queue */
	sparc32_msgp_t	msg_last;	/* last message in the queue */
	sparc32_u_long	msg_cbytes;	/* number of bytes in use on the queue */
	sparc32_u_long	msg_qnum;	/* number of msgs in the queue */
	sparc32_u_long	msg_qbytes;	/* max # of bytes on the queue */
	pid_t msg_lspid;		/* pid of last msgsnd() */
	pid_t msg_lrpid;		/* pid of last msgrcv() */
	time_t	msg_stime;		/* time of last msgsnd() */
	sparc32_long	msg_pad1;
	time_t	msg_rtime;		/* time of last msgrcv() */
	sparc32_long	msg_pad2;
	time_t	msg_ctime;		/* time of last msgctl() */
	sparc32_long	msg_pad3;
	sparc32_long	msg_pad4[4];
};

/* from <sys/sem.h> */
typedef int32_t sparc32_semp_t;

typedef int32_t sparc32_semid_dsp_t;
struct sparc32_semid_ds {
	struct sparc32_ipc_perm	sem_perm;/* operation permission struct */
	sparc32_semp_t	sem_base;	/* pointer to first semaphore in set */
	unsigned short	sem_nsems;	/* number of sems in set */
	time_t	sem_otime;		/* last operation time */
	sparc32_long	sem_pad1;	/* SVABI/386 says I need this here */
	time_t	sem_ctime;		/* last change time */
					/* Times measured in secs since */
					/* 00:00:00 GMT, Jan. 1, 1970 */
	sparc32_long	sem_pad2;	/* SVABI/386 says I need this here */
	sparc32_long	sem_pad3[4];	/* SVABI/386 says I need this here */
};

typedef int32_t sparc32_semunu_t;
union sparc32_semun {
	int	val;		/* value for SETVAL */
	sparc32_semid_dsp_t buf; /* buffer for IPC_STAT & IPC_SET */
	sparc32_u_shortp array;	/* array for GETALL & SETALL */
};

typedef int32_t sparc32_sembufp_t;
struct sparc32_sembuf {
	unsigned short	sem_num;	/* semaphore # */
	short		sem_op;		/* semaphore operation */
	short		sem_flg;	/* operation flags */
};

/* from <sys/shm.h> */
typedef int32_t sparc32_shmid_dsp_t;
struct sparc32_shmid_ds {
	struct sparc32_ipc_perm	shm_perm; /* operation permission structure */
	int		shm_segsz;	/* size of segment in bytes */
	pid_t		shm_lpid;	/* process ID of last shm op */
	pid_t		shm_cpid;	/* process ID of creator */
	short		shm_nattch;	/* number of current attaches */
	time_t		shm_atime;	/* time of last shmat() */
	time_t		shm_dtime;	/* time of last shmdt() */
	time_t		shm_ctime;	/* time of last change by shmctl() */
	sparc32_voidp	shm_internal;	/* sysv stupidity */
};

/* from <sys/signal.h> */
typedef int32_t sparc32_sigactionp_t;
struct	sparc32_sigaction {
	sparc32_voidp sa_handler;	/* signal handler */
	sigset_t sa_mask;		/* signal mask to apply */
	int	sa_flags;		/* see signal options below */
};

typedef int32_t sparc32_sigaltstack13p_t;
struct sparc32_sigaltstack13 {
	sparc32_charp	ss_sp;			/* signal stack base */
	int	ss_size;		/* signal stack length */
	int	ss_flags;		/* SS_DISABLE and/or SS_ONSTACK */
};

typedef int32_t sparc32_sigaltstackp_t;
struct sparc32_sigaltstack {
	sparc32_voidp	ss_sp;			/* signal stack base */
	sparc32_size_t	ss_size;		/* signal stack length */
	int	ss_flags;		/* SS_DISABLE and/or SS_ONSTACK */
};

typedef int32_t sparc32_sigstackp_t;
struct	sparc32_sigstack {
	sparc32_voidp	ss_sp;			/* signal stack pointer */
	int	ss_onstack;		/* current status */
};

typedef int32_t sparc32_sigvecp_t;
struct	sparc32_sigvec {
	sparc32_voidp sv_handler;	/* signal handler */
	int	sv_mask;		/* signal mask to apply */
	int	sv_flags;		/* see signal options below */
};

/* from <sys/socket.h> */
typedef int32_t sparc32_sockaddrp_t;
typedef int32_t sparc32_osockaddrp_t;

typedef int32_t sparc32_msghdrp_t;
struct sparc32_msghdr {
	sparc32_caddr_t	msg_name;		/* optional address */
	u_int	msg_namelen;		/* size of address */
	sparc32_iovecp_t msg_iov;		/* scatter/gather array */
	u_int	msg_iovlen;		/* # elements in msg_iov */
	sparc32_caddr_t	msg_control;		/* ancillary data, see below */
	u_int	msg_controllen;		/* ancillary data buffer len */
	int	msg_flags;		/* flags on received message */
};

typedef int32_t sparc32_omsghdrp_t;
struct sparc32_omsghdr {
	sparc32_caddr_t	msg_name;		/* optional address */
	int	msg_namelen;		/* size of address */
	sparc32_iovecp_t msg_iov;		/* scatter/gather array */
	int	msg_iovlen;		/* # elements in msg_iov */
	sparc32_caddr_t	msg_accrights;		/* access rights sent/received */
	int	msg_accrightslen;
};

/* from <sys/stat.h> */
typedef int32_t sparc32_stat12p_t;
struct sparc32_stat12 {			/* NetBSD-1.2 stat struct */
	dev_t	  st_dev;		/* inode's device */
	ino_t	  st_ino;		/* inode's number */
	u_int16_t st_mode;		/* inode protection mode */
	u_int16_t st_nlink;		/* number of hard links */
	uid_t	  st_uid;		/* user ID of the file's owner */
	gid_t	  st_gid;		/* group ID of the file's group */
	dev_t	  st_rdev;		/* device type */
	struct	  sparc32_timespec st_atimespec;/* time of last access */
	struct	  sparc32_timespec st_mtimespec;/* time of last data modification */
	struct	  sparc32_timespec st_ctimespec;/* time of last file status change */
	off_t	  st_size;		/* file size, in bytes */
	int64_t	  st_blocks;		/* blocks allocated for file */
	u_int32_t st_blksize;		/* optimal blocksize for I/O */
	u_int32_t st_flags;		/* user defined flags for file */
	u_int32_t st_gen;		/* file generation number */
	int32_t	  st_lspare;
	int64_t	  st_qspare[2];
};

typedef int32_t sparc32_stat43p_t;
struct sparc32_stat43 {			/* BSD-4.3 stat struct */
	u_int16_t st_dev;		/* inode's device */
	ino_t	  st_ino;		/* inode's number */
	u_int16_t st_mode;		/* inode protection mode */
	u_int16_t st_nlink;		/* number of hard links */
	u_int16_t st_uid;		/* user ID of the file's owner */
	u_int16_t st_gid;		/* group ID of the file's group */
	u_int16_t st_rdev;		/* device type */
	int32_t	  st_size;		/* file size, in bytes */
	struct	  sparc32_timespec st_atimespec;/* time of last access */
	struct	  sparc32_timespec st_mtimespec;/* time of last data modification */
	struct	  sparc32_timespec st_ctimespec;/* time of last file status change */
	int32_t	  st_blksize;		/* optimal blocksize for I/O */
	int32_t	  st_blocks;		/* blocks allocated for file */
	u_int32_t st_flags;		/* user defined flags for file */
	u_int32_t st_gen;		/* file generation number */
};
typedef int32_t sparc32_statp_t;
struct sparc32_stat {
	dev_t	  st_dev;		/* inode's device */
	ino_t	  st_ino;		/* inode's number */
	mode_t	  st_mode;		/* inode protection mode */
	nlink_t	  st_nlink;		/* number of hard links */
	uid_t	  st_uid;		/* user ID of the file's owner */
	gid_t	  st_gid;		/* group ID of the file's group */
	dev_t	  st_rdev;		/* device type */
	struct	  timespec st_atimespec;/* time of last access */
	struct	  timespec st_mtimespec;/* time of last data modification */
	struct	  timespec st_ctimespec;/* time of last file status change */
	off_t	  st_size;		/* file size, in bytes */
	blkcnt_t  st_blocks;		/* blocks allocated for file */
	blksize_t st_blksize;		/* optimal blocksize for I/O */
	u_int32_t st_flags;		/* user defined flags for file */
	u_int32_t st_gen;		/* file generation number */
	int64_t	  st_qspare[2];
};

/* from <sys/timex.h> */
typedef int32_t sparc32_ntptimevalp_t;
struct sparc32_ntptimeval {
	struct sparc32_timeval time;	/* current time (ro) */
	sparc32_long maxerror;	/* maximum error (us) (ro) */
	sparc32_long esterror;	/* estimated error (us) (ro) */
};

typedef int32_t sparc32_timexp_t;
struct sparc32_timex {
	unsigned int modes;	/* clock mode bits (wo) */
	sparc32_long offset;	/* time offset (us) (rw) */
	sparc32_long freq;	/* frequency offset (scaled ppm) (rw) */
	sparc32_long maxerror;	/* maximum error (us) (rw) */
	sparc32_long esterror;	/* estimated error (us) (rw) */
	int status;		/* clock status bits (rw) */
	sparc32_long constant;	/* pll time constant (rw) */
	sparc32_long precision;	/* clock precision (us) (ro) */
	sparc32_long tolerance;	/* clock frequency tolerance (scaled
				 * ppm) (ro) */
	/*
	 * The following read-only structure members are implemented
	 * only if the PPS signal discipline is configured in the
	 * kernel.
	 */
	sparc32_long ppsfreq;	/* pps frequency (scaled ppm) (ro) */
	sparc32_long jitter;	/* pps jitter (us) (ro) */
	int shift;		/* interval duration (s) (shift) (ro) */
	sparc32_long stabil;	/* pps stability (scaled ppm) (ro) */
	sparc32_long jitcnt;	/* jitter limit exceeded (ro) */
	sparc32_long calcnt;	/* calibration intervals (ro) */
	sparc32_long errcnt;	/* calibration errors (ro) */
	sparc32_long stbcnt;	/* stability limit exceeded (ro) */
};

/* from <ufs/lfs/lfs.h> */
typedef int32_t sparc32_block_infop_t;  /* XXX broken */

/* from <sys/utsname.h> */
typedef int32_t sparc32_utsnamep_t;

/* from <compat/common/kern_info_09.c> */
typedef int32_t sparc32_outsnamep_t;

/* from <arch/sparc/include/signal.h> */
typedef int32_t sparc32_sigcontextp_t;
/* XXX how can this work? */
struct sparc32_sigcontext {
	int	sc_onstack;		/* sigstack state to restore */
	int	sc_mask;		/* signal mask to restore */
	/* begin machine dependent portion */
	int	sc_sp;			/* %sp to restore */
	int	sc_pc;			/* pc to restore */
	int	sc_npc;			/* npc to restore */
	int	sc_psr;			/* psr to restore */
	int	sc_g1;			/* %g1 to restore */
	int	sc_o0;			/* %o0 to restore */
};

/*
 * here are some macros to convert between sparc32 and sparc64 types.
 * note that they do *NOT* act like good macros and put ()'s around all
 * arguments cuz this _breaks_ SCARG().
 */
#define SPARC32TO64(s32uap, uap, name) \
	    SCARG(uap, name) = SCARG(s32uap, name)
#define SPARC32TOP(s32uap, uap, name, type) \
	    SCARG(uap, name) = (type *)(u_long)(u_int)SCARG(s32uap, name)
#define SPARC32TOX(s32uap, uap, name, type) \
	    SCARG(uap, name) = (type)SCARG(s32uap, name)
#define SPARC32TOX64(s32uap, uap, name, type) \
	    SCARG(uap, name) = (type)(u_long)SCARG(s32uap, name)

/* and some standard versions */
#define	SPARC32TO64_UAP(name)		SPARC32TO64(uap, &ua, name);
#define	SPARC32TOP_UAP(name, type)	SPARC32TOP(uap, &ua, name, type);
#define	SPARC32TOX_UAP(name, type)	SPARC32TOX(uap, &ua, name, type);
#define	SPARC32TOX64_UAP(name, type)	SPARC32TOX64(uap, &ua, name, type);

/*
 * random other stuff
 */
#include <compat/common/compat_util.h>
 
extern const char sparc32_emul_path[];
  
#define SPARC32_CHECK_ALT_EXIST(p, sgp, path) \
    emul_find(p, sgp, sparc32_emul_path, (char *)path, (char **)&path, 0)

#endif /* _COMPAT_SPARC32_SPARC32_H_ */
