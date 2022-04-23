/*	$NetBSD: netbsd32.h,v 1.140 2022/04/23 17:46:23 reinoud Exp $	*/

/*
 * Copyright (c) 1998, 2001, 2008, 2015 Matthew R. Green
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

#include <sys/param.h> /* precautionary upon removal from ucred.h */
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/syscallargs.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/ucontext.h>
#include <sys/ucred.h>
#include <sys/module_hook.h>

#include <compat/sys/ucontext.h>
#include <compat/sys/mount.h>
#include <compat/sys/signal.h>
#include <compat/sys/siginfo.h>
#include <compat/common/compat_util.h>

#include <nfs/rpcv2.h>

/*
 * first define the basic types we need, and any applicable limits.
 */

typedef int32_t netbsd32_long;
typedef uint32_t netbsd32_u_long;
typedef int64_t netbsd32_quad;

typedef uint32_t netbsd32_clock_t;
typedef uint32_t netbsd32_size_t;
typedef int32_t netbsd32_ssize_t;
typedef int32_t netbsd32_clockid_t;
typedef int32_t netbsd32_key_t;
typedef int32_t netbsd32_intptr_t;
typedef uint32_t netbsd32_uintptr_t;

/* Note: 32-bit sparc defines ssize_t as long but still has same size as int. */
#define	NETBSD32_SSIZE_MAX	INT32_MAX

/* netbsd32_[u]int64 are machine dependent and defined below */

/*
 * machine dependant section; must define:
 *	NETBSD32_POINTER_TYPE
 *		- 32-bit pointer type, normally uint32_t but can be int32_t
 *		  for platforms which rely on sign-extension of pointers
 *		  such as SH-5.
 *                eg:	#define NETBSD32_POINTER_TYPE uint32_t
 *	netbsd32_pointer_t
 *		- a typedef'd struct with the above as an "i32" member.
 *                eg:	typedef struct {
 *				NETBSD32_POINTER_TYPE i32;
 *			} netbsd32_pointer_t;
 *	NETBSD32PTR64(p32)
 *		- Translate a 32-bit pointer into something valid in a
 *		  64-bit context.
 *	struct netbsd32_sigcontext
 *		- 32bit compatibility sigcontext structure for this arch.
 *	netbsd32_sigcontextp_t
 *		- type of pointer to above, normally uint32_t
 *	void netbsd32_setregs(struct proc *p, struct exec_package *pack,
 *	    unsigned long stack);
 *	int netbsd32_sigreturn(struct proc *p, void *v,
 *	    register_t *retval);
 *	void netbsd32_sendsig(sig_t catcher, int sig, int mask, u_long code);
 *	char netbsd32_esigcode[], netbsd32_sigcode[]
 *		- the above are abvious
 *
 * pull in the netbsd32 machine dependent header, that may help with the
 * above, or it may be provided via the MD layer itself.
 */
#include <machine/netbsd32_machdep.h>

/*
 * Conversion functions for the rest of the compat32 code:
 *
 * NETBSD32PTR64()	Convert user-supplied 32bit pointer to 'void *'
 * NETBSD32PTR32()	Assign a 'void *' to a 32bit pointer variable
 * NETBSD32PTR32PLUS()	Add an integer to a 32bit pointer
 *
 * Under rare circumstances the following get used:
 *
 * NETBSD32PTR32I()	Convert 'void *' to the 32bit pointer base type.
 * NETBSD32IPTR64()	Convert 32bit pointer base type to 'void *'
 */
#define	NETBSD32PTR64(p32)		NETBSD32IPTR64((p32).i32)
#define	NETBSD32PTR32(p32, p64)		((p32).i32 = NETBSD32PTR32I(p64))
#define	NETBSD32PTR32PLUS(p32, incr)	netbsd32_ptr32_incr(&p32, incr)
#define	NETBSD32PTR32I(p32)		netbsd32_ptr32i(p32)
#define	NETBSD32IPTR64(p32)		netbsd32_iptr64(p32)

static __inline NETBSD32_POINTER_TYPE
netbsd32_ptr32i(const void *p64)
{
	uintptr_t u64 = (uintptr_t)p64;
	KASSERTMSG(u64 == (uintptr_t)(NETBSD32_POINTER_TYPE)u64,
	    "u64 %jx != %jx", (uintmax_t)u64,
	   (uintmax_t)(NETBSD32_POINTER_TYPE)u64);
	return u64;
}

static __inline void *
netbsd32_iptr64(NETBSD32_POINTER_TYPE p32)
{
	return (void *)(intptr_t)p32;
}

static __inline netbsd32_pointer_t
netbsd32_ptr32_incr(netbsd32_pointer_t *p32, uint32_t incr)
{
	netbsd32_pointer_t n32 = *p32;

	n32.i32 += incr;
	KASSERT(NETBSD32PTR64(n32) > NETBSD32PTR64(*p32));
	return *p32 = n32;
}

/* Nothing should be using the raw type, so kill it */
#undef NETBSD32_POINTER_TYPE

/*
 * 64 bit integers only have 4-byte alignment on some 32 bit ports,
 * but always have 8-byte alignment on 64 bit systems.
 * NETBSD32_INT64_ALIGN may be __attribute__((__aligned__(4)))
 */
typedef int64_t netbsd32_int64 NETBSD32_INT64_ALIGN;
typedef uint64_t netbsd32_uint64 NETBSD32_INT64_ALIGN;
#undef NETBSD32_INT64_ALIGN

/* Type used in siginfo, avoids circular dependencies between headers. */
CTASSERT(sizeof(netbsd32_uint64) == sizeof(netbsd32_siginfo_uint64));
CTASSERT(__alignof__(netbsd32_uint64) == __alignof__(netbsd32_siginfo_uint64));

/*
 * all pointers are netbsd32_pointer_t (defined in <machine/netbsd32_machdep.h>)
 */

typedef netbsd32_pointer_t netbsd32_voidp;
typedef netbsd32_pointer_t netbsd32_u_shortp;
typedef netbsd32_pointer_t netbsd32_charp;
typedef netbsd32_pointer_t netbsd32_u_charp;
typedef netbsd32_pointer_t netbsd32_charpp;
typedef netbsd32_pointer_t netbsd32_size_tp;
typedef netbsd32_pointer_t netbsd32_intp;
typedef netbsd32_pointer_t netbsd32_uintp;
typedef netbsd32_pointer_t netbsd32_longp;
typedef netbsd32_pointer_t netbsd32_caddrp;
typedef netbsd32_pointer_t netbsd32_caddr;
typedef netbsd32_pointer_t netbsd32_gid_tp;
typedef netbsd32_pointer_t netbsd32_fsid_tp_t;
typedef netbsd32_pointer_t netbsd32_lwpidp;
typedef netbsd32_pointer_t netbsd32_ucontextp;
typedef netbsd32_pointer_t netbsd32_caddr_t;
typedef netbsd32_pointer_t netbsd32_lwpctlp;
typedef netbsd32_pointer_t netbsd32_pid_tp;
typedef netbsd32_pointer_t netbsd32_psetidp_t;
typedef netbsd32_pointer_t netbsd32_aclp_t;

/*
 * now, the compatibility structures and their fake pointer types.
 */

/* from <sys/types.h> */
typedef netbsd32_pointer_t netbsd32_fd_setp_t;
typedef netbsd32_intptr_t netbsd32_semid_t;
typedef netbsd32_pointer_t netbsd32_semidp_t;
typedef netbsd32_uint64 netbsd32_dev_t;
typedef netbsd32_int64 netbsd32_off_t;
typedef netbsd32_uint64 netbsd32_ino_t;
typedef netbsd32_int64 netbsd32_blkcnt_t;

/* from <sys/spawn.h> */
typedef netbsd32_pointer_t netbsd32_posix_spawn_file_actionsp;
typedef netbsd32_pointer_t netbsd32_posix_spawnattrp;
typedef netbsd32_pointer_t netbsd32_posix_spawn_file_actions_entryp;

/* from <sys/uio.h> */
typedef netbsd32_pointer_t netbsd32_iovecp_t;
struct netbsd32_iovec {
	netbsd32_voidp	 iov_base;	/* Base address. */
	netbsd32_size_t	 iov_len;	/* Length. */
};

/* from <sys/time.h> */
typedef int32_t netbsd32_timer_t;
typedef	int32_t netbsd32_time50_t;
typedef	netbsd32_int64 netbsd32_time_t;
typedef netbsd32_pointer_t netbsd32_timerp_t;
typedef netbsd32_pointer_t netbsd32_clockidp_t;

typedef netbsd32_pointer_t netbsd32_timespec50p_t;
struct netbsd32_timespec50 {
	netbsd32_time50_t tv_sec;			/* seconds */
	netbsd32_long	tv_nsec;	/* and nanoseconds */
};

typedef netbsd32_pointer_t netbsd32_timespecp_t;
struct netbsd32_timespec {
	netbsd32_time_t tv_sec;			/* seconds */
	netbsd32_long	tv_nsec;	/* and nanoseconds */
};

typedef netbsd32_pointer_t netbsd32_timeval50p_t;
struct netbsd32_timeval50 {
	netbsd32_time50_t	tv_sec;		/* seconds */
	netbsd32_long		tv_usec;	/* and microseconds */
};

typedef netbsd32_pointer_t netbsd32_timevalp_t;
struct netbsd32_timeval {
	netbsd32_time_t	tv_sec;		/* seconds */
	suseconds_t	tv_usec;	/* and microseconds */
};

typedef netbsd32_pointer_t netbsd32_timezonep_t;
struct netbsd32_timezone {
	int	tz_minuteswest;	/* minutes west of Greenwich */
	int	tz_dsttime;	/* type of dst correction */
};

typedef netbsd32_pointer_t netbsd32_itimerval50p_t;
struct	netbsd32_itimerval50 {
	struct	netbsd32_timeval50 it_interval;	/* timer interval */
	struct	netbsd32_timeval50 it_value;	/* current value */
};

typedef netbsd32_pointer_t netbsd32_itimervalp_t;
struct	netbsd32_itimerval {
	struct	netbsd32_timeval it_interval;	/* timer interval */
	struct	netbsd32_timeval it_value;	/* current value */
};

typedef netbsd32_pointer_t netbsd32_itimerspec50p_t;
struct netbsd32_itimerspec50 {
	struct netbsd32_timespec50 it_interval;
	struct netbsd32_timespec50 it_value;
};

typedef netbsd32_pointer_t netbsd32_itimerspecp_t;
struct netbsd32_itimerspec {
	struct netbsd32_timespec it_interval;
	struct netbsd32_timespec it_value;
};

/* from <sys/mount.h> */
typedef netbsd32_pointer_t netbsd32_fidp_t;

typedef netbsd32_pointer_t netbsd32_fhandlep_t;
typedef netbsd32_pointer_t netbsd32_compat_30_fhandlep_t;

typedef netbsd32_pointer_t netbsd32_statfsp_t;
struct netbsd32_statfs {
	short	f_type;			/* type of file system */
	unsigned short	f_flags;	/* copy of mount flags */
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

struct netbsd32_export_args30 {
	int	ex_flags;		/* export related flags */
	uid_t	ex_root;		/* mapping for root uid */
	struct	uucred ex_anon;		/* mapping for anonymous user */
	netbsd32_pointer_t ex_addr;	/* net address to which exported */
	int	ex_addrlen;		/* and the net address length */
	netbsd32_pointer_t ex_mask;	/* mask of valid bits in saddr */
	int	ex_masklen;		/* and the smask length */
	netbsd32_charp ex_indexfile;	/* index file for WebNFS URLs */
};

/* from <sys/poll.h> */
typedef netbsd32_pointer_t netbsd32_pollfdp_t;

/* from <sys/ptrace.h> */
typedef netbsd32_pointer_t netbsd32_ptrace_io_descp_t;
struct netbsd32_ptrace_io_desc {
	int	piod_op;		/* I/O operation */
	netbsd32_voidp piod_offs;	/* child offset */
	netbsd32_voidp piod_addr;	/* parent offset */
	netbsd32_size_t piod_len;	/* request length (in) /
					   actual count (out) */
};

struct netbsd32_ptrace_siginfo {
	siginfo32_t	psi_siginfo;	/* signal information structure */
	lwpid_t		psi_lwpid;	/* destination LWP of the signal
					 * value 0 means the whole process
					 * (route signal to all LWPs) */
};

#define PL32_LNAMELEN 20

struct netbsd32_ptrace_lwpstatus {
	lwpid_t		pl_lwpid;
	sigset_t	pl_sigpend;
	sigset_t	pl_sigmask;
	char		pl_name[PL32_LNAMELEN];
	netbsd32_voidp	pl_private;
};

/* from <sys/quotactl.h> */
typedef netbsd32_pointer_t netbsd32_quotactlargsp_t;
struct netbsd32_quotactlargs {
	unsigned qc_op;
	union {
		struct {
			netbsd32_pointer_t qc_info;
		} stat;
		struct {
			int qc_idtype;
			netbsd32_pointer_t qc_info;
		} idtypestat;
		struct {
			int qc_objtype;
			netbsd32_pointer_t qc_info;
		} objtypestat;
		struct {
			netbsd32_pointer_t qc_key;
			netbsd32_pointer_t qc_val;
		} get;
		struct {
			netbsd32_pointer_t qc_key;
			netbsd32_pointer_t qc_val;
		} put;
		struct {
			netbsd32_pointer_t qc_key;
		} del;
		struct {
			netbsd32_pointer_t qc_cursor;
		} cursoropen;
		struct {
			netbsd32_pointer_t qc_cursor;
		} cursorclose;
		struct {
			netbsd32_pointer_t qc_cursor;
			int qc_idtype;
		} cursorskipidtype;
		struct {
			netbsd32_pointer_t qc_cursor;
			netbsd32_pointer_t qc_keys;
			netbsd32_pointer_t qc_vals;
			unsigned qc_maxnum;
			netbsd32_pointer_t qc_ret;
		} cursorget;
		struct {
			netbsd32_pointer_t qc_cursor;
			netbsd32_pointer_t qc_ret;
		} cursoratend;
		struct {
			netbsd32_pointer_t qc_cursor;
		} cursorrewind;
		struct {
			int qc_idtype;
			netbsd32_pointer_t qc_quotafile;
		} quotaon;
		struct {
			int qc_idtype;
		} quotaoff;
	} u;
};

/* from <sys/resource.h> */
typedef netbsd32_pointer_t netbsd32_rusage50p_t;
struct	netbsd32_rusage50 {
	struct netbsd32_timeval50 ru_utime;/* user time used */
	struct netbsd32_timeval50 ru_stime;/* system time used */
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

typedef netbsd32_pointer_t netbsd32_rusagep_t;
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

typedef netbsd32_pointer_t netbsd32_wrusagep_t;
struct netbsd32_wrusage {
	struct netbsd32_rusage	wru_self;
	struct netbsd32_rusage	wru_children;
};

typedef netbsd32_pointer_t netbsd32_orlimitp_t;

typedef netbsd32_pointer_t netbsd32_rlimitp_t;

struct netbsd32_loadavg {
	fixpt_t	ldavg[3];
	netbsd32_long	fscale;
};

/* from <sys/swap.h> */
struct netbsd32_swapent {
	netbsd32_dev_t	se_dev;		/* device id */
	int	se_flags;		/* flags */
	int	se_nblks;		/* total blocks */
	int	se_inuse;		/* blocks in use */
	int	se_priority;		/* priority of this device */
	char	se_path[PATH_MAX+1];	/* path	name */
};

/* from <sys/ipc.h> */
typedef netbsd32_pointer_t netbsd32_ipc_permp_t;
struct netbsd32_ipc_perm {
	uid_t		cuid;	/* creator user id */
	gid_t		cgid;	/* creator group id */
	uid_t		uid;	/* user id */
	gid_t		gid;	/* group id */
	mode_t		mode;	/* r/w permission */
	unsigned short	_seq;	/* sequence # (to generate unique msg/sem/shm id) */
	netbsd32_key_t	_key;	/* user specified msg/sem/shm key */
};
struct netbsd32_ipc_perm14 {
	unsigned short	cuid;	/* creator user id */
	unsigned short	cgid;	/* creator group id */
	unsigned short	uid;	/* user id */
	unsigned short	gid;	/* group id */
	unsigned short	mode;	/* r/w permission */
	unsigned short	seq;	/* sequence # (to generate unique msg/sem/shm id) */
	netbsd32_key_t	key;	/* user specified msg/sem/shm key */
};

/* from <sys/msg.h> */
typedef netbsd32_pointer_t netbsd32_msgp_t;
struct netbsd32_msg {
	netbsd32_msgp_t msg_next;	/* next msg in the chain */
	netbsd32_long	msg_type;	/* type of this message */
    					/* >0 -> type of this message */
    					/* 0 -> free header */
	unsigned short	msg_ts;		/* size of this message */
	short	msg_spot;		/* location of start of msg in buffer */
};

typedef uint32_t netbsd32_msgqnum_t;
typedef netbsd32_size_t netbsd32_msglen_t;

typedef netbsd32_pointer_t netbsd32_msqid_dsp_t;
struct netbsd32_msqid_ds {
	struct netbsd32_ipc_perm msg_perm;	/* operation permission strucure */
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
	netbsd32_msglen_t _msg_cbytes;	/* # of bytes currently in queue */
};
typedef netbsd32_pointer_t netbsd32_msqid_ds50p_t;
struct netbsd32_msqid_ds50 {
	struct netbsd32_ipc_perm msg_perm;	/* operation permission strucure */
	netbsd32_msgqnum_t	msg_qnum;	/* number of messages in the queue */
	netbsd32_msglen_t	msg_qbytes;	/* max # of bytes in the queue */
	pid_t		msg_lspid;	/* process ID of last msgsend() */
	pid_t		msg_lrpid;	/* process ID of last msgrcv() */
	int32_t		msg_stime;	/* time of last msgsend() */
	int32_t		msg_rtime;	/* time of last msgrcv() */
	int32_t		msg_ctime;	/* time of last change */

	/*
	 * These members are private and used only in the internal
	 * implementation of this interface.
	 */
	netbsd32_msgp_t _msg_first;	/* first message in the queue */
	netbsd32_msgp_t	_msg_last;	/* last message in the queue */
	netbsd32_msglen_t _msg_cbytes;	/* # of bytes currently in queue */
};

typedef netbsd32_pointer_t netbsd32_msqid_ds14p_t;
struct netbsd32_msqid_ds14 {
	struct	netbsd32_ipc_perm14 msg_perm;	/* msg queue permission bits */
	netbsd32_msgp_t	msg_first;	/* first message in the queue */
	netbsd32_msgp_t	msg_last;	/* last message in the queue */
	netbsd32_u_long	msg_cbytes;	/* number of bytes in use on the queue */
	netbsd32_u_long	msg_qnum;	/* number of msgs in the queue */
	netbsd32_u_long	msg_qbytes;	/* max # of bytes on the queue */
	pid_t msg_lspid;		/* pid of last msgsnd() */
	pid_t msg_lrpid;		/* pid of last msgrcv() */
	int32_t		msg_stime;	/* time of last msgsnd() */
	netbsd32_long	msg_pad1;
	int32_t		msg_rtime;	/* time of last msgrcv() */
	netbsd32_long	msg_pad2;
	int32_t		msg_ctime;	/* time of last msgctl() */
	netbsd32_long	msg_pad3;
	netbsd32_long	msg_pad4[4];
};

/* from <sys/sem.h> */
typedef netbsd32_pointer_t netbsd32_semp_t;

typedef netbsd32_pointer_t netbsd32_semid_dsp_t;
struct netbsd32_semid_ds {
	struct netbsd32_ipc_perm	sem_perm;/* operation permission struct */
	unsigned short	sem_nsems;	/* number of sems in set */
	netbsd32_time_t	sem_otime;	/* last operation time */
	netbsd32_time_t	sem_ctime;	/* last change time */

	/*
	 * These members are private and used only in the internal
	 * implementation of this interface.
	 */
	netbsd32_semp_t	_sem_base;	/* pointer to first semaphore in set */
};

typedef netbsd32_pointer_t netbsd32_semid_ds50p_t;
struct netbsd32_semid_ds50 {
	struct netbsd32_ipc_perm	sem_perm;/* operation permission struct */
	unsigned short	sem_nsems;	/* number of sems in set */
	int32_t		sem_otime;	/* last operation time */
	int32_t		sem_ctime;	/* last change time */

	/*
	 * These members are private and used only in the internal
	 * implementation of this interface.
	 */
	netbsd32_semp_t	_sem_base;	/* pointer to first semaphore in set */
};

typedef netbsd32_pointer_t netbsd32_semid_ds14p_t;
struct netbsd32_semid_ds14 {
	struct netbsd32_ipc_perm14	sem_perm;/* operation permission struct */
	netbsd32_semp_t	sem_base;	/* pointer to first semaphore in set */
	unsigned short	sem_nsems;	/* number of sems in set */
	netbsd32_time_t	sem_otime;	/* last operation time */
	netbsd32_long	sem_pad1;	/* SVABI/386 says I need this here */
	netbsd32_time_t	sem_ctime;	/* last change time */
					/* Times measured in secs since */
					/* 00:00:00 GMT, Jan. 1, 1970 */
	int32_t		sem_pad2;	/* SVABI/386 says I need this here */
	int32_t		sem_pad3[4];	/* SVABI/386 says I need this here */
};

typedef uint32_t netbsd32_semunu_t;
typedef netbsd32_pointer_t netbsd32_semunp_t;
union netbsd32_semun {
	int	val;			/* value for SETVAL */
	netbsd32_semid_dsp_t buf;	/* buffer for IPC_STAT & IPC_SET */
	netbsd32_u_shortp array;	/* array for GETALL & SETALL */
};

typedef netbsd32_pointer_t netbsd32_semun50p_t;
union netbsd32_semun50 {
	int	val;			/* value for SETVAL */
	netbsd32_semid_ds50p_t buf;	/* buffer for IPC_STAT & IPC_SET */
	netbsd32_u_shortp array;	/* array for GETALL & SETALL */
};

typedef netbsd32_pointer_t netbsd32_sembufp_t;
struct netbsd32_sembuf {
	unsigned short	sem_num;	/* semaphore # */
	short		sem_op;		/* semaphore operation */
	short		sem_flg;	/* operation flags */
};

/* from <sys/shm.h> */
typedef netbsd32_pointer_t netbsd32_shmid_dsp_t;
struct netbsd32_shmid_ds {
	struct netbsd32_ipc_perm shm_perm; /* operation permission structure */
	netbsd32_size_t	shm_segsz;	/* size of segment in bytes */
	pid_t		shm_lpid;	/* process ID of last shm op */
	pid_t		shm_cpid;	/* process ID of creator */
	shmatt_t	shm_nattch;	/* number of current attaches */
	netbsd32_time_t	shm_atime;	/* time of last shmat() */
	netbsd32_time_t	shm_dtime;	/* time of last shmdt() */
	netbsd32_time_t	shm_ctime;	/* time of last change by shmctl() */
	netbsd32_voidp	_shm_internal;	/* sysv stupidity */
};

typedef netbsd32_pointer_t netbsd32_shmid_ds50p_t;
struct netbsd32_shmid_ds50 {
	struct netbsd32_ipc_perm shm_perm; /* operation permission structure */
	netbsd32_size_t	shm_segsz;	/* size of segment in bytes */
	pid_t		shm_lpid;	/* process ID of last shm op */
	pid_t		shm_cpid;	/* process ID of creator */
	shmatt_t	shm_nattch;	/* number of current attaches */
	int32_t		shm_atime;	/* time of last shmat() */
	int32_t		shm_dtime;	/* time of last shmdt() */
	int32_t		shm_ctime;	/* time of last change by shmctl() */
	netbsd32_voidp	_shm_internal;	/* sysv stupidity */
};

typedef netbsd32_pointer_t netbsd32_shmid_ds14p_t;
struct netbsd32_shmid_ds14 {
	struct netbsd32_ipc_perm14 shm_perm; /* operation permission structure */
	int		shm_segsz;	/* size of segment in bytes */
	pid_t		shm_lpid;	/* process ID of last shm op */
	pid_t		shm_cpid;	/* process ID of creator */
	short		shm_nattch;	/* number of current attaches */
	int32_t		shm_atime;	/* time of last shmat() */
	int32_t		shm_dtime;	/* time of last shmdt() */
	int32_t		shm_ctime;	/* time of last change by shmctl() */
	netbsd32_voidp	_shm_internal;	/* sysv stupidity */
};

/* from <sys/signal.h> */
typedef netbsd32_pointer_t netbsd32_sigsetp_t;
typedef netbsd32_pointer_t netbsd32_sigactionp_t;
struct	netbsd32_sigaction13 {
	netbsd32_voidp netbsd32_sa_handler;	/* signal handler */
	sigset13_t netbsd32_sa_mask;		/* signal mask to apply */
	int	netbsd32_sa_flags;		/* see signal options below */
};

struct	netbsd32_sigaction {
	netbsd32_voidp netbsd32_sa_handler;	/* signal handler */
	sigset_t netbsd32_sa_mask;		/* signal mask to apply */
	int	netbsd32_sa_flags;		/* see signal options below */
};

typedef netbsd32_pointer_t netbsd32_sigaltstack13p_t;
struct netbsd32_sigaltstack13 {
	netbsd32_charp	ss_sp;		/* signal stack base */
	int	ss_size;		/* signal stack length */
	int	ss_flags;		/* SS_DISABLE and/or SS_ONSTACK */
};

typedef netbsd32_pointer_t netbsd32_sigaltstackp_t;
struct netbsd32_sigaltstack {
	netbsd32_voidp	ss_sp;		/* signal stack base */
	netbsd32_size_t	ss_size;	/* signal stack length */
	int	ss_flags;		/* SS_DISABLE and/or SS_ONSTACK */
};

typedef netbsd32_pointer_t netbsd32_sigstackp_t;
struct	netbsd32_sigstack {
	netbsd32_voidp	ss_sp;		/* signal stack pointer */
	int	ss_onstack;		/* current status */
};

typedef netbsd32_pointer_t netbsd32_sigvecp_t;
struct	netbsd32_sigvec {
	netbsd32_voidp sv_handler;	/* signal handler */
	int	sv_mask;		/* signal mask to apply */
	int	sv_flags;		/* see signal options below */
};

typedef netbsd32_pointer_t netbsd32_siginfop_t;

union netbsd32_sigval {
	int	sival_int;
	netbsd32_voidp	sival_ptr;
};

typedef netbsd32_pointer_t netbsd32_sigeventp_t;
struct netbsd32_sigevent {
	int	sigev_notify;
	int	sigev_signo;
	union netbsd32_sigval	sigev_value;
	netbsd32_voidp	sigev_notify_function;
	netbsd32_voidp	sigev_notify_attributes;
};

/* from <sys/sigtypes.h> */
typedef netbsd32_pointer_t netbsd32_stackp_t;

/* from <sys/socket.h> */
typedef netbsd32_pointer_t netbsd32_sockaddrp_t;
typedef netbsd32_pointer_t netbsd32_osockaddrp_t;
typedef netbsd32_pointer_t netbsd32_socklenp_t;

typedef netbsd32_pointer_t netbsd32_msghdrp_t;
struct netbsd32_msghdr {
	netbsd32_caddr_t msg_name;		/* optional address */
	unsigned int	msg_namelen;		/* size of address */
	netbsd32_iovecp_t msg_iov;		/* scatter/gather array */
	unsigned int	msg_iovlen;		/* # elements in msg_iov */
	netbsd32_caddr_t msg_control;		/* ancillary data, see below */
	unsigned int	msg_controllen;		/* ancillary data buffer len */
	int	msg_flags;		/* flags on received message */
};

typedef netbsd32_pointer_t netbsd32_omsghdrp_t;
struct netbsd32_omsghdr {
	netbsd32_caddr_t msg_name;		/* optional address */
	int		 msg_namelen;		/* size of address */
	netbsd32_iovecp_t msg_iov;		/* scatter/gather array */
	int		 msg_iovlen;		/* # elements in msg_iov */
	netbsd32_caddr_t msg_accrights;		/* access rights sent/recvd */
	u_int		 msg_accrightslen;
};

typedef netbsd32_pointer_t netbsd32_mmsghdrp_t;
struct netbsd32_mmsghdr {
	struct netbsd32_msghdr msg_hdr;
	unsigned int msg_len;
};

/* from <sys/stat.h> */
typedef netbsd32_pointer_t netbsd32_stat12p_t;
struct netbsd32_stat12 {		/* NetBSD-1.2 stat struct */
	uint32_t	st_dev;		/* inode's device */
	uint32_t	st_ino;		/* inode's number */
	uint16_t	st_mode;	/* inode protection mode */
	uint16_t	st_nlink;	/* number of hard links */
	uid_t		st_uid;		/* user ID of the file's owner */
	gid_t		st_gid;		/* group ID of the file's group */
	uint32_t	st_rdev;	/* device type */
	struct netbsd32_timespec50 st_atimespec;/* time of last access */
	struct netbsd32_timespec50 st_mtimespec;/* time of last data modification */
	struct netbsd32_timespec50 st_ctimespec;/* time of last file status change */
	netbsd32_int64	st_size;	/* file size, in bytes */
	netbsd32_int64	st_blocks;	/* blocks allocated for file */
	uint32_t	st_blksize;	/* optimal blocksize for I/O */
	uint32_t	st_flags;	/* user defined flags for file */
	uint32_t	st_gen;		/* file generation number */
	int32_t		st_lspare;
	netbsd32_int64	st_qspare[2];
};

typedef netbsd32_pointer_t netbsd32_stat43p_t;
struct netbsd32_stat43 {		/* BSD-4.3 stat struct */
	uint16_t  st_dev;		/* inode's device */
	uint32_t  st_ino;		/* inode's number */
	uint16_t  st_mode;		/* inode protection mode */
	uint16_t  st_nlink;		/* number of hard links */
	uint16_t  st_uid;		/* user ID of the file's owner */
	uint16_t  st_gid;		/* group ID of the file's group */
	uint16_t  st_rdev;		/* device type */
	int32_t	  st_size;		/* file size, in bytes */
	struct netbsd32_timespec50 st_atimespec;/* time of last access */
	struct netbsd32_timespec50 st_mtimespec;/* time of last data modification */
	struct netbsd32_timespec50 st_ctimespec;/* time of last file status change */
	int32_t	  st_blksize;		/* optimal blocksize for I/O */
	int32_t	  st_blocks;		/* blocks allocated for file */
	uint32_t  st_flags;		/* user defined flags for file */
	uint32_t  st_gen;		/* file generation number */
};
typedef netbsd32_pointer_t netbsd32_stat13p_t;
struct netbsd32_stat13 {
	uint32_t  st_dev;		/* inode's device */
	uint32_t  st_ino;		/* inode's number */
	mode_t	  st_mode;		/* inode protection mode */
	nlink_t	  st_nlink;		/* number of hard links */
	uid_t	  st_uid;		/* user ID of the file's owner */
	gid_t	  st_gid;		/* group ID of the file's group */
	uint32_t  st_rdev;		/* device type */
	struct netbsd32_timespec50 st_atimespec;/* time of last access */
	struct netbsd32_timespec50 st_mtimespec;/* time of last data modification */
	struct netbsd32_timespec50 st_ctimespec;/* time of last file status change */
	netbsd32_int64	  st_size;		/* file size, in bytes */
	netbsd32_uint64  st_blocks;		/* blocks allocated for file */
	blksize_t st_blksize;		/* optimal blocksize for I/O */
	uint32_t  st_flags;		/* user defined flags for file */
	uint32_t  st_gen;		/* file generation number */
	uint32_t  st_spare;		/* file generation number */
	struct	  netbsd32_timespec50 st_birthtimespec;
	uint32_t  st_spare2;
};

typedef netbsd32_pointer_t netbsd32_stat50p_t;
struct netbsd32_stat50 {
	uint32_t	st_dev;		/* inode's device */
	mode_t		st_mode;	/* inode protection mode */
	netbsd32_uint64	st_ino;		/* inode's number */
	nlink_t		st_nlink;	/* number of hard links */
	uid_t		st_uid;		/* user ID of the file's owner */
	gid_t		st_gid;		/* group ID of the file's group */
	uint32_t	st_rdev;	/* device type */
	struct netbsd32_timespec50 st_atimespec;/* time of last access */
	struct netbsd32_timespec50 st_mtimespec;/* time of last data modification */
	struct netbsd32_timespec50 st_ctimespec;/* time of last file status change */
	struct netbsd32_timespec50 st_birthtimespec; /* time of creation */
	netbsd32_int64	st_size;	/* file size, in bytes */
	netbsd32_uint64 st_blocks;	/* blocks allocated for file */
	blksize_t	st_blksize;	/* optimal blocksize for I/O */
	uint32_t	st_flags;	/* user defined flags for file */
	uint32_t	st_gen;		/* file generation number */
	uint32_t	st_spare[2];
};

typedef netbsd32_pointer_t netbsd32_statp_t;
struct netbsd32_stat {
	netbsd32_dev_t	st_dev;		/* inode's device */
	mode_t		st_mode;	/* inode protection mode */
	netbsd32_uint64	st_ino;		/* inode's number */
	nlink_t		st_nlink;	/* number of hard links */
	uid_t		st_uid;		/* user ID of the file's owner */
	gid_t		st_gid;		/* group ID of the file's group */
	netbsd32_dev_t	st_rdev;	/* device type */
	struct netbsd32_timespec st_atimespec;/* time of last access */
	struct netbsd32_timespec st_mtimespec;/* time of last data modification */
	struct netbsd32_timespec st_ctimespec;/* time of last file status change */
	struct netbsd32_timespec st_birthtimespec; /* time of creation */
	netbsd32_int64	st_size;	/* file size, in bytes */
	netbsd32_uint64 st_blocks;	/* blocks allocated for file */
	blksize_t	st_blksize;	/* optimal blocksize for I/O */
	uint32_t	st_flags;	/* user defined flags for file */
	uint32_t	st_gen;		/* file generation number */
	uint32_t	st_spare[2];
};

/* from <sys/statvfs.h> */
typedef netbsd32_pointer_t netbsd32_statvfs90p_t;
struct netbsd32_statvfs90 {
	netbsd32_u_long	f_flag;		/* copy of mount exported flags */
	netbsd32_u_long	f_bsize;	/* system block size */
	netbsd32_u_long	f_frsize;	/* system fragment size */
	netbsd32_u_long	f_iosize;	/* optimal file system block size */
	netbsd32_uint64	f_blocks;	/* number of blocks in file system */
	netbsd32_uint64	f_bfree;	/* free blocks avail in file system */
	netbsd32_uint64	f_bavail;	/* free blocks avail to non-root */
	netbsd32_uint64	f_bresvd;	/* blocks reserved for root */
	netbsd32_uint64	f_files;	/* total file nodes in file system */
	netbsd32_uint64	f_ffree;	/* free file nodes in file system */
	netbsd32_uint64	f_favail;	/* free file nodes avail to non-root */
	netbsd32_uint64	f_fresvd;	/* file nodes reserved for root */
	netbsd32_uint64	f_syncreads;	/* count of sync reads since mount */
	netbsd32_uint64	f_syncwrites;	/* count of sync writes since mount */
	netbsd32_uint64	f_asyncreads;	/* count of async reads since mount */
	netbsd32_uint64	f_asyncwrites;	/* count of async writes since mount */
	fsid_t		f_fsidx;	/* NetBSD compatible fsid */
	netbsd32_u_long	f_fsid;		/* Posix compatible fsid */
	netbsd32_u_long	f_namemax;	/* maximum filename length */
	uid_t		f_owner;	/* user that mounted the file system */
	uint32_t	f_spare[4];	/* spare space */
	char	f_fstypename[_VFS_NAMELEN]; /* fs type name */
	char	f_mntonname[_VFS_MNAMELEN];  /* directory on which mounted */
	char	f_mntfromname[_VFS_MNAMELEN];  /* mounted file system */
};

typedef netbsd32_pointer_t netbsd32_statvfsp_t;
struct netbsd32_statvfs {
	netbsd32_u_long	f_flag;		/* copy of mount exported flags */
	netbsd32_u_long	f_bsize;	/* system block size */
	netbsd32_u_long	f_frsize;	/* system fragment size */
	netbsd32_u_long	f_iosize;	/* optimal file system block size */
	netbsd32_uint64	f_blocks;	/* number of blocks in file system */
	netbsd32_uint64	f_bfree;	/* free blocks avail in file system */
	netbsd32_uint64	f_bavail;	/* free blocks avail to non-root */
	netbsd32_uint64	f_bresvd;	/* blocks reserved for root */
	netbsd32_uint64	f_files;	/* total file nodes in file system */
	netbsd32_uint64	f_ffree;	/* free file nodes in file system */
	netbsd32_uint64	f_favail;	/* free file nodes avail to non-root */
	netbsd32_uint64	f_fresvd;	/* file nodes reserved for root */
	netbsd32_uint64	f_syncreads;	/* count of sync reads since mount */
	netbsd32_uint64	f_syncwrites;	/* count of sync writes since mount */
	netbsd32_uint64	f_asyncreads;	/* count of async reads since mount */
	netbsd32_uint64	f_asyncwrites;	/* count of async writes since mount */
	fsid_t		f_fsidx;	/* NetBSD compatible fsid */
	netbsd32_u_long	f_fsid;		/* Posix compatible fsid */
	netbsd32_u_long	f_namemax;	/* maximum filename length */
	uid_t		f_owner;	/* user that mounted the file system */
	netbsd32_uint64	f_spare[4];	/* spare space */
	char	f_fstypename[_VFS_NAMELEN]; /* fs type name */
	char	f_mntonname[_VFS_MNAMELEN];  /* directory on which mounted */
	char	f_mntfromname[_VFS_MNAMELEN];  /* mounted file system */
	char	f_mntfromlabel[_VFS_MNAMELEN];  /* disk label name if available */
};

/* from <sys/timex.h> */
typedef netbsd32_pointer_t netbsd32_ntptimevalp_t;
typedef netbsd32_pointer_t netbsd32_ntptimeval30p_t;
typedef netbsd32_pointer_t netbsd32_ntptimeval50p_t;

struct netbsd32_ntptimeval30 {
	struct netbsd32_timeval50 time;	/* current time (ro) */
	netbsd32_long maxerror;	/* maximum error (us) (ro) */
	netbsd32_long esterror;	/* estimated error (us) (ro) */
};
struct netbsd32_ntptimeval50 {
	struct netbsd32_timespec50 time;	/* current time (ro) */
	netbsd32_long maxerror;	/* maximum error (us) (ro) */
	netbsd32_long esterror;	/* estimated error (us) (ro) */
	netbsd32_long tai;	/* TAI offset */
	int time_state;		/* time status */
};

struct netbsd32_ntptimeval {
	struct netbsd32_timespec time;	/* current time (ro) */
	netbsd32_long maxerror;	/* maximum error (us) (ro) */
	netbsd32_long esterror;	/* estimated error (us) (ro) */
	netbsd32_long tai;	/* TAI offset */
	int time_state;		/* time status */
};
typedef netbsd32_pointer_t netbsd32_timexp_t;
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

/* <prop/plistref.h> */
struct netbsd32_plistref {
	netbsd32_pointer_t pref_plist;
	netbsd32_size_t pref_len;
};

/* <nv.h> */
typedef struct {
	netbsd32_pointer_t buf;
	netbsd32_size_t    len;
	int                flags;
} netbsd32_nvlist_ref_t;

/* from <ufs/lfs/lfs.h> */
typedef netbsd32_pointer_t netbsd32_block_infop_t;  /* XXX broken */

/* from <sys/utsname.h> */
typedef netbsd32_pointer_t netbsd32_utsnamep_t;

/* from <compat/common/kern_info_09.c> */
typedef netbsd32_pointer_t netbsd32_outsnamep_t;

/* from <arch/sparc{,64}/include/vuid_event.h> */
typedef struct firm_event32 {
	unsigned short	id;		/* key or MS_* or LOC_[XY]_DELTA */
	unsigned short	pad;		/* unused, at least by X11 */
	int	value;		/* VKEY_{UP,DOWN} or locator delta */
	struct netbsd32_timeval time;
} Firm_event32;

/* from <sys/uuid.h> */
typedef netbsd32_pointer_t netbsd32_uuidp_t;

/* from <sys/event.h> */
typedef netbsd32_pointer_t netbsd32_keventp_t;

struct netbsd32_kevent {
	netbsd32_uintptr_t	ident;
	uint32_t		filter;
	uint32_t		flags;
	uint32_t		fflags;
	netbsd32_int64		data;
	netbsd32_pointer_t	udata;
};

/* from <sys/sched.h> */
typedef netbsd32_pointer_t netbsd32_sched_paramp_t;
typedef netbsd32_pointer_t netbsd32_cpusetp_t;

/* from <fs/tmpfs/tmpfs_args.h> */
struct netbsd32_tmpfs_args {
        int                     ta_version;

        /* Size counters. */
        netbsd32_ino_t          ta_nodes_max;
        netbsd32_off_t          ta_size_max;

        /* Root node attributes. */
        uid_t                   ta_root_uid;
        gid_t                   ta_root_gid;
        mode_t                  ta_root_mode;
};

/* from <fs/cd9660/cd9660_mount.h> */
struct netbsd32_iso_args {
	netbsd32_charp fspec;
	struct netbsd32_export_args30 _pad1;
	int	flags;
};

/* from <ufs/ufs/ufs_mount.h> */
struct netbsd32_ufs_args {
	netbsd32_charp		fspec;
};

struct netbsd32_mfs_args {
	netbsd32_charp		fspec;
	struct netbsd32_export_args30	_pad1;
	netbsd32_voidp		base;
	netbsd32_u_long		size;
};

/* from <nfs/nfs.h> */
struct netbsd32_nfsd_args {
	int		sock;
	netbsd32_voidp	name;
	int		namelen;
};

typedef netbsd32_pointer_t netbsd32_nfsdp;
struct netbsd32_nfsd_srvargs {
	netbsd32_nfsdp	nsd_nfsd;
	uid_t		nsd_uid;
	uint32_t	nsd_haddr;
	struct uucred	nsd_cr;
	int		nsd_authlen;
	netbsd32_u_charp nsd_authstr;
	int		nsd_verflen;
	netbsd32_u_charp nsd_verfstr;
	struct netbsd32_timeval	nsd_timestamp;
	uint32_t	nsd_ttl;
	NFSKERBKEY_T	nsd_key;
};

typedef netbsd32_pointer_t netbsd32_export_argsp;
struct netbsd32_export_args {
	int		ex_flags;
	uid_t		ex_root;
	struct uucred	ex_anon;
	netbsd32_sockaddrp_t ex_addr;
	int		ex_addrlen;
	netbsd32_sockaddrp_t ex_mask;
	int		ex_masklen;
	netbsd32_charp	ex_indexfile;
};

struct netbsd32_mountd_exports_list {
	const netbsd32_charp	mel_path;
	netbsd32_size_t		mel_nexports;
	netbsd32_export_argsp	mel_exports;
};

/* from <nfs/nfsmount,h> */
struct netbsd32_nfs_args {
	int32_t		version;	/* args structure version number */
	netbsd32_sockaddrp_t addr;	/* file server address */
	int32_t		addrlen;	/* length of address */
	int32_t		sotype;		/* Socket type */
	int32_t		proto;		/* and Protocol */
	netbsd32_u_charp fh;		/* File handle to be mounted */
	int32_t		fhsize;		/* Size, in bytes, of fh */
	int32_t		flags;		/* flags */
	int32_t		wsize;		/* write size in bytes */
	int32_t		rsize;		/* read size in bytes */
	int32_t		readdirsize;	/* readdir size in bytes */
	int32_t		timeo;		/* initial timeout in .1 secs */
	int32_t		retrans;	/* times to retry send */
	int32_t		maxgrouplist;	/* Max. size of group list */
	int32_t		readahead;	/* # of blocks to readahead */
	int32_t		leaseterm;	/* Ignored; Term (sec) of lease */
	int32_t		deadthresh;	/* Retrans threshold */
	netbsd32_charp	hostname;	/* server's name */
};

/* from <msdosfs/msdosfsmount.h> */
struct netbsd32_msdosfs_args {
	netbsd32_charp	fspec;		/* blocks special holding the fs to mount */
	struct	netbsd32_export_args30 _pad1; /* compat with old userland tools */
	uid_t	uid;		/* uid that owns msdosfs files */
	gid_t	gid;		/* gid that owns msdosfs files */
	mode_t  mask;		/* mask to be applied for msdosfs perms */
	int	flags;		/* see below */

	/* Following items added after versioning support */
	int	version;	/* version of the struct */
	mode_t  dirmask;	/* v2: mask to be applied for msdosfs perms */
	int	gmtoff;		/* v3: offset from UTC in seconds */
};

/* from <udf/udf_mount.h> */
struct netbsd32_udf_args {
	uint32_t	 version;	/* version of this structure         */
	netbsd32_charp	 fspec;		/* mount specifier                   */
	int32_t		 sessionnr;	/* session specifier, rel of abs     */
	uint32_t	 udfmflags;	/* mount options                     */
	int32_t		 gmtoff;	/* offset from UTC in seconds        */

	uid_t		 anon_uid;	/* mapping of anonymous files uid    */
	gid_t		 anon_gid;	/* mapping of anonymous files gid    */
	uid_t		 nobody_uid;	/* nobody:nobody will map to -1:-1   */
	gid_t		 nobody_gid;	/* nobody:nobody will map to -1:-1   */

	uint32_t	 sector_size;	/* for mounting dumps/files          */

	/* extendable */
	uint8_t	 reserved[32];
};

/* from <miscfs/genfs/layer.h> */
struct netbsd32_layer_args {
	netbsd32_charp target;		/* Target of loopback  */
	struct netbsd32_export_args30 _pad1; /* compat with old userland tools */
};

/* from <miscfs/nullfs/null.h> */
struct netbsd32_null_args {
	struct	netbsd32_layer_args	la;	/* generic layerfs args */
};

struct netbsd32_posix_spawn_file_actions_entry {
	enum { FAE32_OPEN, FAE32_DUP2, FAE32_CLOSE,
	    FAE32_CHDIR, FAE32_FCHDIR } fae_action;

	int fae_fildes;
	union {
		struct {
			netbsd32_charp path;
			int oflag;
			mode_t mode;
		} open;
		struct {
			int newfildes;
		} dup2;
		struct {
			netbsd32_charp path;
		} chdir;
	} fae_data;
};

struct netbsd32_posix_spawn_file_actions {
	unsigned int size;	/* size of fae array */
	unsigned int len;	/* how many slots are used */
	netbsd32_posix_spawn_file_actions_entryp fae;
};

struct netbsd32_modctl_load {
	netbsd32_charp ml_filename;
	int ml_flags;
	netbsd32_charp ml_props;
	netbsd32_size_t ml_propslen;
};

struct netbsd32_mq_attr {
	netbsd32_long	mq_flags;
	netbsd32_long	mq_maxmsg;
	netbsd32_long	mq_msgsize;
	netbsd32_long	mq_curmsgs;
};
typedef netbsd32_pointer_t netbsd32_mq_attrp_t;

#if 0
int	netbsd32_kevent(struct lwp *, void *, register_t *);
#endif

/*
 * here are some macros to convert between netbsd32 and native 64 bit types.
 * note that they do *NOT* act like good macros and put ()'s around all
 * arguments cuz this _breaks_ SCARG().
 */
#define NETBSD32TO64(s32uap, uap, name) \
	    SCARG(uap, name) = SCARG(s32uap, name)
#define NETBSD32TOP(s32uap, uap, name, type) \
	    SCARG(uap, name) = SCARG_P32(s32uap, name)
#define NETBSD32TOX(s32uap, uap, name, type) \
	    SCARG(uap, name) = (type)SCARG(s32uap, name)
#define NETBSD32TOX64(s32uap, uap, name, type) \
	    SCARG(uap, name) = (type)(long)SCARG(s32uap, name)

/* and some standard versions */
#define	NETBSD32TO64_UAP(name)		NETBSD32TO64(uap, &ua, name)
#define	NETBSD32TOP_UAP(name, type)	NETBSD32TOP(uap, &ua, name, type)
#define	NETBSD32TOX_UAP(name, type)	NETBSD32TOX(uap, &ua, name, type)
#define	NETBSD32TOX64_UAP(name, type)	NETBSD32TOX64(uap, &ua, name, type)

#define	SCARG_P32(uap, name) NETBSD32PTR64(SCARG(uap, name))

struct coredump_iostate;
int	coredump_netbsd32(struct lwp *, struct coredump_iostate *);
int	real_coredump_netbsd32(struct lwp *, struct coredump_iostate *);

/*
 * random other stuff
 */

vaddr_t netbsd32_vm_default_addr(struct proc *, vaddr_t, vsize_t, int);
void netbsd32_adjust_limits(struct proc *);

void	netbsd32_si_to_si32(siginfo32_t *, const siginfo_t *);
void	netbsd32_si32_to_si(siginfo_t *, const siginfo32_t *);

void	netbsd32_ksi32_to_ksi(struct _ksiginfo *si, const struct __ksiginfo32 *si32);

void	netbsd32_read_lwpstatus(struct lwp *, struct netbsd32_ptrace_lwpstatus *);

#ifdef KTRACE
void netbsd32_ktrpsig(int, sig_t, const sigset_t *, const ksiginfo_t *);
#else
#define netbsd32_ktrpsig NULL
#endif


void	startlwp32(void *);
struct compat_50_netbsd32___semctl14_args;
int	do_netbsd32___semctl14(struct lwp *, const struct compat_50_netbsd32___semctl14_args *, register_t *, void *);

struct iovec *netbsd32_get_iov(struct netbsd32_iovec *, int, struct iovec *,
	    int);

#ifdef SYSCTL_SETUP_PROTO
SYSCTL_SETUP_PROTO(netbsd32_sysctl_emul_setup);
#endif /* SYSCTL_SETUP_PROTO */

#ifdef __HAVE_STRUCT_SIGCONTEXT
MODULE_HOOK(netbsd32_sendsig_sigcontext_16_hook, void,
    (const struct ksiginfo *, const sigset_t *));
#endif

extern struct sysent netbsd32_sysent[];
extern const uint32_t netbsd32_sysent_nomodbits[]; 
#ifdef SYSCALL_DEBUG 
extern const char * const netbsd32_syscallnames[];
#endif

extern struct sysctlnode netbsd32_sysctl_root;

struct netbsd32_modctl_args;
MODULE_HOOK(compat32_80_modctl_hook, int,
    (struct lwp *, const struct netbsd32_modctl_args *, register_t *));

/*
 * Finally, declare emul_netbsd32 as this is needed in lots of
 * places when calling syscall_{,dis}establish()
 */

extern struct emul emul_netbsd32;
extern char netbsd32_sigcode[], netbsd32_esigcode[];

/*
 * Prototypes for MD initialization routines
 */
void netbsd32_machdep_md_init(void);
void netbsd32_machdep_md_fini(void);
void netbsd32_machdep_md_13_init(void);
void netbsd32_machdep_md_13_fini(void);
void netbsd32_machdep_md_16_init(void);
void netbsd32_machdep_md_16_fini(void);

#endif /* _COMPAT_NETBSD32_NETBSD32_H_ */
