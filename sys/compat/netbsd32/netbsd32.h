/*	$NetBSD: netbsd32.h,v 1.10 1999/12/30 15:40:45 eeh Exp $	*/

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

#ifndef _COMPAT_NETBSD32_NETBSD32_H_
#define _COMPAT_NETBSD32_NETBSD32_H_
/* We need to change the size of register_t */
#ifdef syscallargs
#undef syscallargs
#endif
/*
 * NetBSD 32-bit compatibility module.
 */

#include <sys/mount.h>

/*
 * first, define all the types we need.
 */

typedef int32_t netbsd32_long;
typedef u_int32_t netbsd32_u_long;

typedef u_int32_t netbsd32_clock_t;
typedef u_int32_t netbsd32_size_t;
typedef int32_t netbsd32_ssize_t;
typedef int32_t netbsd32_clockid_t;
typedef u_int32_t netbsd32_caddr_t;
typedef int32_t netbsd32_key_t;
typedef int32_t netbsd32_intptr_t;
typedef u_int32_t netbsd32_uintptr_t;

/* all pointers are u_int32_t */

typedef u_int32_t netbsd32_voidp;
typedef u_int32_t netbsd32_u_shortp;
typedef u_int32_t netbsd32_charp;
typedef u_int32_t netbsd32_u_charp;
typedef u_int32_t netbsd32_charpp;
typedef u_int32_t netbsd32_size_tp;
typedef u_int32_t netbsd32_intp;
typedef u_int32_t netbsd32_longp;
typedef u_int32_t netbsd32_caddrp;
typedef u_int32_t netbsd32_caddr;
typedef u_int32_t netbsd32_gid_tp;
typedef u_int32_t netbsd32_fsid_tp_t;

/*
 * now, the compatibility structures and their fake pointer types.
 */

/* from <sys/types.h> */
typedef u_int32_t netbsd32_fd_setp_t;

/* from <sys/uio.h> */
typedef u_int32_t netbsd32_iovecp_t;
struct netbsd32_iovec {
	netbsd32_voidp	iov_base;	/* Base address. */
	netbsd32_size_t	 iov_len;	/* Length. */
};

/* from <sys/time.h> */
typedef int32_t netbsd32_timer_t;
typedef	int32_t netbsd32_time_t;

typedef u_int32_t netbsd32_timespecp_t;
struct netbsd32_timespec {
	netbsd32_time_t	tv_sec;			/* seconds */
	netbsd32_long	tv_nsec;	/* and nanoseconds */
};

typedef u_int32_t netbsd32_timevalp_t;
struct netbsd32_timeval {
	netbsd32_long	tv_sec;		/* seconds */
	netbsd32_long	tv_usec;	/* and microseconds */
};

typedef u_int32_t netbsd32_timezonep_t;
struct netbsd32_timezone {
	int	tz_minuteswest;	/* minutes west of Greenwich */
	int	tz_dsttime;	/* type of dst correction */
};

typedef u_int32_t netbsd32_itimervalp_t;
struct	netbsd32_itimerval {
	struct	netbsd32_timeval it_interval;	/* timer interval */
	struct	netbsd32_timeval it_value;	/* current value */
};

/* from <sys/mount.h> */
typedef u_int32_t netbsd32_fidp_t;

typedef u_int32_t netbsd32_fhandlep_t;

typedef u_int32_t netbsd32_statfsp_t;
struct netbsd32_statfs {
	short	f_type;			/* type of file system */
	u_short	f_flags;		/* copy of mount flags */
	netbsd32_long	f_bsize;	/* fundamental file system block size */
	netbsd32_long	f_iosize;	/* optimal transfer block size */
	netbsd32_long	f_blocks;	/* total data blocks in file system */
	netbsd32_long	f_bfree;	/* free blocks in fs */
	netbsd32_long	f_bavail;	/* free blocks avail to non-superuser */
	netbsd32_long	f_files;	/* total file nodes in file system */
	netbsd32_long	f_ffree;	/* free file nodes in fs */
	fsid_t	f_fsid;			/* file system id */
	uid_t	f_owner;		/* user that mounted the file system */
	netbsd32_long	f_spare[4];	/* spare for later */
	char	f_fstypename[MFSNAMELEN]; /* fs type name */
	char	f_mntonname[MNAMELEN];	  /* directory on which mounted */
	char	f_mntfromname[MNAMELEN];  /* mounted file system */
};

/* from <sys/poll.h> */
typedef u_int32_t netbsd32_pollfdp_t;

/* from <sys/resource.h> */
typedef u_int32_t netbsd32_rusagep_t;
struct	netbsd32_rusage {
	struct netbsd32_timeval ru_utime;/* user time used */
	struct netbsd32_timeval ru_stime;/* system time used */
	netbsd32_long	ru_maxrss;	/* max resident set size */
	netbsd32_long	ru_ixrss;	/* integral shared memory size */
	netbsd32_long	ru_idrss;	/* integral unshared data " */
	netbsd32_long	ru_isrss;	/* integral unshared stack " */
	netbsd32_long	ru_minflt;	/* page reclaims */
	netbsd32_long	ru_majflt;	/* page faults */
	netbsd32_long	ru_nswap;	/* swaps */
	netbsd32_long	ru_inblock;	/* block input operations */
	netbsd32_long	ru_oublock;	/* block output operations */
	netbsd32_long	ru_msgsnd;	/* messages sent */
	netbsd32_long	ru_msgrcv;	/* messages received */
	netbsd32_long	ru_nsignals;	/* signals received */
	netbsd32_long	ru_nvcsw;	/* voluntary context switches */
	netbsd32_long	ru_nivcsw;	/* involuntary " */
};

typedef u_int32_t netbsd32_orlimitp_t;

typedef u_int32_t netbsd32_rlimitp_t;

/* from <sys/ipc.h> */
typedef u_int32_t netbsd32_ipc_permp_t;
struct netbsd32_ipc_perm {
	ushort	cuid;		/* creator user id */
	ushort	cgid;		/* creator group id */
	ushort	uid;		/* user id */
	ushort	gid;		/* group id */
	ushort	mode;		/* r/w permission */
	ushort	_seq;		/* sequence # (to generate unique msg/sem/shm id) */
	netbsd32_key_t	_key;	/* user specified msg/sem/shm key */
};
struct netbsd32_ipc_perm14 {
	ushort	cuid;		/* creator user id */
	ushort	cgid;		/* creator group id */
	ushort	uid;		/* user id */
	ushort	gid;		/* group id */
	ushort	mode;		/* r/w permission */
	ushort	seq;		/* sequence # (to generate unique msg/sem/shm id) */
	netbsd32_key_t	key;	/* user specified msg/sem/shm key */
};

/* from <sys/msg.h> */
typedef u_int32_t netbsd32_msgp_t;
struct netbsd32_msg {
	netbsd32_msgp_t msg_next;	/* next msg in the chain */
	netbsd32_long	msg_type;	/* type of this message */
    				/* >0 -> type of this message */
    				/* 0 -> free header */
	u_short	msg_ts;		/* size of this message */
	short	msg_spot;	/* location of start of msg in buffer */
};

typedef u_int32_t netbsd32_msqid_dsp_t;
typedef u_int32_t netbsd32_msgqnum_t;
typedef netbsd32_size_t netbsd32_msglen_t;

struct netbsd32_msqid_ds {
	struct netbsd32_ipc_perm	msg_perm;	/* operation permission strucure */
	netbsd32_msgqnum_t	msg_qnum;	/* number of messages in the queue */
	netbsd32_msglen_t	msg_qbytes;	/* max # of bytes in the queue */
	pid_t		msg_lspid;	/* process ID of last msgsend() */
	pid_t		msg_lrpid;	/* process ID of last msgrcv() */
	netbsd32_time_t		msg_stime;	/* time of last msgsend() */
	netbsd32_time_t		msg_rtime;	/* time of last msgrcv() */
	netbsd32_time_t		msg_ctime;	/* time of last change */

	/*
	 * These members are private and used only in the internal
	 * implementation of this interface.
	 */
	netbsd32_msgp_t _msg_first;	/* first message in the queue */
	netbsd32_msgp_t	_msg_last;	/* last message in the queue */
	netbsd32_msglen_t	_msg_cbytes;	/* # of bytes currently in queue */
};
struct netbsd32_msqid_ds14 {
	struct	netbsd32_ipc_perm14 msg_perm;	/* msg queue permission bits */
	netbsd32_msgp_t	msg_first;	/* first message in the queue */
	netbsd32_msgp_t	msg_last;	/* last message in the queue */
	netbsd32_u_long	msg_cbytes;	/* number of bytes in use on the queue */
	netbsd32_u_long	msg_qnum;	/* number of msgs in the queue */
	netbsd32_u_long	msg_qbytes;	/* max # of bytes on the queue */
	pid_t msg_lspid;		/* pid of last msgsnd() */
	pid_t msg_lrpid;		/* pid of last msgrcv() */
	netbsd32_time_t	msg_stime;		/* time of last msgsnd() */
	netbsd32_long	msg_pad1;
	netbsd32_time_t	msg_rtime;		/* time of last msgrcv() */
	netbsd32_long	msg_pad2;
	netbsd32_time_t	msg_ctime;		/* time of last msgctl() */
	netbsd32_long	msg_pad3;
	netbsd32_long	msg_pad4[4];
};

/* from <sys/sem.h> */
typedef u_int32_t netbsd32_semp_t;

typedef u_int32_t netbsd32_semid_dsp_t;
struct netbsd32_semid_ds {
	struct netbsd32_ipc_perm	sem_perm;/* operation permission struct */
	unsigned short	sem_nsems;	/* number of sems in set */
	netbsd32_time_t		sem_otime;	/* last operation time */
	netbsd32_time_t		sem_ctime;	/* last change time */

	/*
	 * These members are private and used only in the internal
	 * implementation of this interface.
	 */
	netbsd32_semp_t	_sem_base;	/* pointer to first semaphore in set */
};

struct netbsd32_semid_ds14 {
	struct netbsd32_ipc_perm	sem_perm;/* operation permission struct */
	netbsd32_semp_t	sem_base;	/* pointer to first semaphore in set */
	unsigned short	sem_nsems;	/* number of sems in set */
	netbsd32_time_t	sem_otime;		/* last operation time */
	netbsd32_long	sem_pad1;	/* SVABI/386 says I need this here */
	netbsd32_time_t	sem_ctime;		/* last change time */
					/* Times measured in secs since */
					/* 00:00:00 GMT, Jan. 1, 1970 */
	netbsd32_long	sem_pad2;	/* SVABI/386 says I need this here */
	netbsd32_long	sem_pad3[4];	/* SVABI/386 says I need this here */
};

typedef u_int32_t netbsd32_semunu_t;
union netbsd32_semun {
	int	val;		/* value for SETVAL */
	netbsd32_semid_dsp_t buf; /* buffer for IPC_STAT & IPC_SET */
	netbsd32_u_shortp array;	/* array for GETALL & SETALL */
};

typedef u_int32_t netbsd32_sembufp_t;
struct netbsd32_sembuf {
	unsigned short	sem_num;	/* semaphore # */
	short		sem_op;		/* semaphore operation */
	short		sem_flg;	/* operation flags */
};

/* from <sys/shm.h> */
typedef u_int32_t netbsd32_shmid_dsp_t;
struct netbsd32_shmid_ds {
	struct netbsd32_ipc_perm	shm_perm; /* operation permission structure */
	int		shm_segsz;	/* size of segment in bytes */
	pid_t		shm_lpid;	/* process ID of last shm op */
	pid_t		shm_cpid;	/* process ID of creator */
	short		shm_nattch;	/* number of current attaches */
	netbsd32_time_t		shm_atime;	/* time of last shmat() */
	netbsd32_time_t		shm_dtime;	/* time of last shmdt() */
	netbsd32_time_t		shm_ctime;	/* time of last change by shmctl() */
	netbsd32_voidp	_shm_internal;	/* sysv stupidity */
};

/* from <sys/signal.h> */
typedef u_int32_t netbsd32_sigsetp_t;
typedef u_int32_t netbsd32_sigactionp_t;
struct	netbsd32_sigaction {
	netbsd32_voidp sa_handler;	/* signal handler */
	sigset_t sa_mask;		/* signal mask to apply */
	int	sa_flags;		/* see signal options below */
};

typedef u_int32_t netbsd32_sigaltstack13p_t;
struct netbsd32_sigaltstack13 {
	netbsd32_charp	ss_sp;			/* signal stack base */
	int	ss_size;		/* signal stack length */
	int	ss_flags;		/* SS_DISABLE and/or SS_ONSTACK */
};

typedef u_int32_t netbsd32_sigaltstackp_t;
struct netbsd32_sigaltstack {
	netbsd32_voidp	ss_sp;			/* signal stack base */
	netbsd32_size_t	ss_size;		/* signal stack length */
	int	ss_flags;		/* SS_DISABLE and/or SS_ONSTACK */
};

typedef u_int32_t netbsd32_sigstackp_t;
struct	netbsd32_sigstack {
	netbsd32_voidp	ss_sp;			/* signal stack pointer */
	int	ss_onstack;		/* current status */
};

typedef u_int32_t netbsd32_sigvecp_t;
struct	netbsd32_sigvec {
	netbsd32_voidp sv_handler;	/* signal handler */
	int	sv_mask;		/* signal mask to apply */
	int	sv_flags;		/* see signal options below */
};

/* from <sys/socket.h> */
typedef u_int32_t netbsd32_sockaddrp_t;
typedef u_int32_t netbsd32_osockaddrp_t;

typedef u_int32_t netbsd32_msghdrp_t;
struct netbsd32_msghdr {
	netbsd32_caddr_t	msg_name;		/* optional address */
	u_int	msg_namelen;		/* size of address */
	netbsd32_iovecp_t msg_iov;		/* scatter/gather array */
	u_int	msg_iovlen;		/* # elements in msg_iov */
	netbsd32_caddr_t	msg_control;		/* ancillary data, see below */
	u_int	msg_controllen;		/* ancillary data buffer len */
	int	msg_flags;		/* flags on received message */
};

typedef u_int32_t netbsd32_omsghdrp_t;
struct netbsd32_omsghdr {
	netbsd32_caddr_t	msg_name;		/* optional address */
	int	msg_namelen;		/* size of address */
	netbsd32_iovecp_t msg_iov;		/* scatter/gather array */
	int	msg_iovlen;		/* # elements in msg_iov */
	netbsd32_caddr_t	msg_accrights;		/* access rights sent/received */
	int	msg_accrightslen;
};

/* from <sys/stat.h> */
typedef u_int32_t netbsd32_stat12p_t;
struct netbsd32_stat12 {			/* NetBSD-1.2 stat struct */
	dev_t	  st_dev;		/* inode's device */
	ino_t	  st_ino;		/* inode's number */
	u_int16_t st_mode;		/* inode protection mode */
	u_int16_t st_nlink;		/* number of hard links */
	uid_t	  st_uid;		/* user ID of the file's owner */
	gid_t	  st_gid;		/* group ID of the file's group */
	dev_t	  st_rdev;		/* device type */
	struct	  netbsd32_timespec st_atimespec;/* time of last access */
	struct	  netbsd32_timespec st_mtimespec;/* time of last data modification */
	struct	  netbsd32_timespec st_ctimespec;/* time of last file status change */
	off_t	  st_size;		/* file size, in bytes */
	int64_t	  st_blocks;		/* blocks allocated for file */
	u_int32_t st_blksize;		/* optimal blocksize for I/O */
	u_int32_t st_flags;		/* user defined flags for file */
	u_int32_t st_gen;		/* file generation number */
	int32_t	  st_lspare;
	int64_t	  st_qspare[2];
};

typedef u_int32_t netbsd32_stat43p_t;
struct netbsd32_stat43 {			/* BSD-4.3 stat struct */
	u_int16_t st_dev;		/* inode's device */
	ino_t	  st_ino;		/* inode's number */
	u_int16_t st_mode;		/* inode protection mode */
	u_int16_t st_nlink;		/* number of hard links */
	u_int16_t st_uid;		/* user ID of the file's owner */
	u_int16_t st_gid;		/* group ID of the file's group */
	u_int16_t st_rdev;		/* device type */
	int32_t	  st_size;		/* file size, in bytes */
	struct	  netbsd32_timespec st_atimespec;/* time of last access */
	struct	  netbsd32_timespec st_mtimespec;/* time of last data modification */
	struct	  netbsd32_timespec st_ctimespec;/* time of last file status change */
	int32_t	  st_blksize;		/* optimal blocksize for I/O */
	int32_t	  st_blocks;		/* blocks allocated for file */
	u_int32_t st_flags;		/* user defined flags for file */
	u_int32_t st_gen;		/* file generation number */
};
typedef u_int32_t netbsd32_statp_t;
struct netbsd32_stat {
	dev_t	  st_dev;		/* inode's device */
	ino_t	  st_ino;		/* inode's number */
	mode_t	  st_mode;		/* inode protection mode */
	nlink_t	  st_nlink;		/* number of hard links */
	uid_t	  st_uid;		/* user ID of the file's owner */
	gid_t	  st_gid;		/* group ID of the file's group */
	dev_t	  st_rdev;		/* device type */
	struct	  netbsd32_timespec st_atimespec;/* time of last access */
	struct	  netbsd32_timespec st_mtimespec;/* time of last data modification */
	struct	  netbsd32_timespec st_ctimespec;/* time of last file status change */
	off_t	  st_size;		/* file size, in bytes */
	blkcnt_t  st_blocks;		/* blocks allocated for file */
	blksize_t st_blksize;		/* optimal blocksize for I/O */
	u_int32_t st_flags;		/* user defined flags for file */
	u_int32_t st_gen;		/* file generation number */
	int64_t	  st_qspare[2];
};

/* from <sys/timex.h> */
typedef u_int32_t netbsd32_ntptimevalp_t;
struct netbsd32_ntptimeval {
	struct netbsd32_timeval time;	/* current time (ro) */
	netbsd32_long maxerror;	/* maximum error (us) (ro) */
	netbsd32_long esterror;	/* estimated error (us) (ro) */
};

typedef u_int32_t netbsd32_timexp_t;
struct netbsd32_timex {
	unsigned int modes;	/* clock mode bits (wo) */
	netbsd32_long offset;	/* time offset (us) (rw) */
	netbsd32_long freq;	/* frequency offset (scaled ppm) (rw) */
	netbsd32_long maxerror;	/* maximum error (us) (rw) */
	netbsd32_long esterror;	/* estimated error (us) (rw) */
	int status;		/* clock status bits (rw) */
	netbsd32_long constant;	/* pll time constant (rw) */
	netbsd32_long precision;	/* clock precision (us) (ro) */
	netbsd32_long tolerance;	/* clock frequency tolerance (scaled
				 * ppm) (ro) */
	/*
	 * The following read-only structure members are implemented
	 * only if the PPS signal discipline is configured in the
	 * kernel.
	 */
	netbsd32_long ppsfreq;	/* pps frequency (scaled ppm) (ro) */
	netbsd32_long jitter;	/* pps jitter (us) (ro) */
	int shift;		/* interval duration (s) (shift) (ro) */
	netbsd32_long stabil;	/* pps stability (scaled ppm) (ro) */
	netbsd32_long jitcnt;	/* jitter limit exceeded (ro) */
	netbsd32_long calcnt;	/* calibration intervals (ro) */
	netbsd32_long errcnt;	/* calibration errors (ro) */
	netbsd32_long stbcnt;	/* stability limit exceeded (ro) */
};

/* from <ufs/lfs/lfs.h> */
typedef u_int32_t netbsd32_block_infop_t;  /* XXX broken */

/* from <sys/utsname.h> */
typedef u_int32_t netbsd32_utsnamep_t;

/* from <compat/common/kern_info_09.c> */
typedef u_int32_t netbsd32_outsnamep_t;

/*
 * machine depedant section; must define:
 *	struct netbsd32_sigcontext
 *		- 32bit compatibility sigcontext structure for this arch.
 *	netbsd32_sigcontextp_t
 *		- type of pointer to above, normally u_int32_t
 *	void netbsd32_setregs(struct proc *p, struct exec_package *pack,
 *	    u_long stack);
 *	int netbsd32_sigreturn(struct proc *p, void *v,
 *	    register_t *retval);
 *	void netbsd32_sendsig(sig_t catcher, int sig, int mask, u_long code);
 *	char netbsd32_esigcode[], netbsd32_sigcode[]
 *		- the above are abvious
 *
 * pull in the netbsd32 machine dependant header, that may help with the
 * above, or it may be provided via the MD layer itself.
 */
#include <machine/netbsd32_machdep.h>

/*
 * here are some macros to convert between netbsd32 and sparc64 types.
 * note that they do *NOT* act like good macros and put ()'s around all
 * arguments cuz this _breaks_ SCARG().
 */
#define NETBSD32TO64(s32uap, uap, name) \
	    SCARG(uap, name) = SCARG(s32uap, name)
#define NETBSD32TOP(s32uap, uap, name, type) \
	    SCARG(uap, name) = (type *)(u_long)(u_int)SCARG(s32uap, name)
#define NETBSD32TOX(s32uap, uap, name, type) \
	    SCARG(uap, name) = (type)SCARG(s32uap, name)
#define NETBSD32TOX64(s32uap, uap, name, type) \
	    SCARG(uap, name) = (type)(u_long)SCARG(s32uap, name)

/* and some standard versions */
#define	NETBSD32TO64_UAP(name)		NETBSD32TO64(uap, &ua, name);
#define	NETBSD32TOP_UAP(name, type)	NETBSD32TOP(uap, &ua, name, type);
#define	NETBSD32TOX_UAP(name, type)	NETBSD32TOX(uap, &ua, name, type);
#define	NETBSD32TOX64_UAP(name, type)	NETBSD32TOX64(uap, &ua, name, type);

/*
 * random other stuff
 */
#include <compat/common/compat_util.h>
 
extern const char netbsd32_emul_path[];
  
#define NETBSD32_CHECK_ALT_EXIST(p, sgp, path) \
    emul_find(p, sgp, netbsd32_emul_path, (const char *)path, \
	(const char **)&path, 0)

#endif /* _COMPAT_NETBSD32_NETBSD32_H_ */
