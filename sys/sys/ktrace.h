/*	$NetBSD: ktrace.h,v 1.46.2.1 2007/05/07 10:56:13 yamt Exp $	*/

/*
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ktrace.h	8.2 (Berkeley) 2/19/95
 */

#ifndef _SYS_KTRACE_H_
#define _SYS_KTRACE_H_

#include <sys/mutex.h>

/*
 * operations to ktrace system call  (KTROP(op))
 */
#define KTROP_SET		0	/* set trace points */
#define KTROP_CLEAR		1	/* clear trace points */
#define KTROP_CLEARFILE		2	/* stop all tracing to file */
#define	KTROP_MASK		0x3
#define	KTROP(o)		((o)&KTROP_MASK) /* macro to extract operation */
/*
 * flags (ORed in with operation)
 */
#define KTRFLAG_DESCEND		4	/* perform op on all children too */

/*
 * ktrace record header
 */
struct ktr_header {
	int	ktr_len;		/* length of record minus length of old header */
#if BYTE_ORDER == LITTLE_ENDIAN
	short	ktr_type;		/* trace record type */
	short	ktr_version;		/* trace record version */
#else
	short	ktr_version;		/* trace record version */
	short	ktr_type;		/* trace record type */
#endif
	pid_t	ktr_pid;		/* process id */
	char	ktr_comm[MAXCOMLEN+1];	/* command name */
	union {
		struct timeval _tv;	/* v0 timestamp */
		struct timespec _ts;	/* v1 timespec */
	} _ktr_time;
	union {
		const void *_buf;	/* v0 unused */
		lwpid_t _lid;		/* v1 lwp id */
	} _ktr_id;
};

#define ktr_lid	_ktr_id._lid
#define ktr_time _ktr_time._ts
#define ktr_tv _ktr_time._tv
#define ktr_ts _ktr_time._ts
#define ktr_unused _ktr_id._buf

#define	KTR_SHIMLEN	offsetof(struct ktr_header, ktr_pid)

/*
 * Test for kernel trace point
 */
#define KTRPOINT(p, type)	\
	(((p)->p_traceflag & (1<<(type))) != 0)

/*
 * ktrace record types
 */

/*
 * KTR_SYSCALL - system call record
 */
#define KTR_SYSCALL	1
struct ktr_syscall {
	int	ktr_code;		/* syscall number */
	int	ktr_argsize;		/* size of arguments */
	/*
	 * followed by ktr_argsize/sizeof(register_t) "register_t"s
	 */
};

/*
 * KTR_SYSRET - return from system call record
 */
#define KTR_SYSRET	2
struct ktr_sysret {
	short	ktr_code;
	short	ktr_eosys;		/* XXX unused */
	int	ktr_error;
	register_t ktr_retval;
	register_t ktr_retval_1;
};

/*
 * KTR_NAMEI - namei record
 */
#define KTR_NAMEI	3
	/* record contains pathname */

/*
 * KTR_GENIO - trace generic process i/o
 */
#define KTR_GENIO	4
struct ktr_genio {
	int	ktr_fd;
	enum	uio_rw ktr_rw;
	/*
	 * followed by data successfully read/written
	 */
};

/*
 * KTR_PSIG - trace processed signal
 */
#define	KTR_PSIG	5
struct ktr_psig {
	int	signo;
	sig_t	action;
	sigset_t mask;
	int	code;
	/*
	 * followed by optional siginfo_t
	 */
};

/*
 * KTR_CSW - trace context switches
 */
#define KTR_CSW		6
struct ktr_csw {
	int	out;	/* 1 if switch out, 0 if switch in */
	int	user;	/* 1 if usermode (ivcsw), 0 if kernel (vcsw) */
};

/*
 * KTR_EMUL - emulation change
 */
#define KTR_EMUL	7
	/* record contains emulation name */

/*
 * KTR_USER - user record
 */
#define	KTR_USER	8
#define KTR_USER_MAXIDLEN	20
#define KTR_USER_MAXLEN		2048	/* maximum length of passed data */
struct ktr_user {
	char 	ktr_id[KTR_USER_MAXIDLEN];	/* string id of caller */
	/*
	 * Followed by ktr_len - sizeof(struct ktr_user) of user data.
	 */
};

/*
 * KTR_MMSG - Mach message
 */
#define KTR_MMSG		9
struct ktr_mmsg {
	/*
	 * This is a Mach message header
	 */
	int	ktr_bits;
	int	ktr_size;
	int	ktr_remote_port;
	int	ktr_local_port;
	int	ktr_reserved;
	int	ktr_id;
	/*
	 * Followed by ktr_size - sizeof(mach_msg_header_t) of message payload
	 */
};

/*
 * KTR_EXEC_ARG, KTR_EXEC_ENV - Arguments and environment from exec
 */
#define KTR_EXEC_ARG		10
#define KTR_EXEC_ENV		11
	/* record contains arg/env string */

/*
 * KTR_MOOL - Mach Out Of Line data
 */
#define KTR_MOOL		12
struct ktr_mool {
	const void 	*uaddr;	/* User address */
	size_t		size;	/* Data len */
	/* Followed by size bytes of data */
};

/*
 * KTR_SAUPCALL - scheduler activated upcall.
 *
 * The structure is no longer used, but retained for compatibility.
 */
#define	KTR_SAUPCALL	13
struct ktr_saupcall {
	int ktr_type;
	int ktr_nevent;
	int ktr_nint;
	void *ktr_sas;
	void *ktr_ap;
	/*
	 * followed by nevent sa_t's from sas[]
	 */
};

/*
 * KTR_MIB - MIB name and data
 */
#define KTR_MIB		14
	/* Record contains MIB name */


/*
 * kernel trace points (in p_traceflag)
 */
#define KTRFAC_MASK	0x00ffffff
#define KTRFAC_SYSCALL	(1<<KTR_SYSCALL)
#define KTRFAC_SYSRET	(1<<KTR_SYSRET)
#define KTRFAC_NAMEI	(1<<KTR_NAMEI)
#define KTRFAC_GENIO	(1<<KTR_GENIO)
#define	KTRFAC_PSIG	(1<<KTR_PSIG)
#define KTRFAC_CSW	(1<<KTR_CSW)
#define KTRFAC_EMUL	(1<<KTR_EMUL)
#define	KTRFAC_USER	(1<<KTR_USER)
#define KTRFAC_MMSG	(1<<KTR_MMSG)
#define KTRFAC_EXEC_ARG	(1<<KTR_EXEC_ARG)
#define KTRFAC_EXEC_ENV	(1<<KTR_EXEC_ENV)
#define KTRFAC_MOOL	(1<<KTR_MOOL)
#define	KTRFAC_MIB	(1<<KTR_MIB)
/*
 * trace flags (also in p_traceflags)
 */
#define KTRFAC_ROOT	0x80000000	/* root set this trace */
#define KTRFAC_INHERIT	0x40000000	/* pass trace flags to children */
#define KTRFAC_TRC_EMUL	0x10000000	/* ktrace KTR_EMUL before next trace */
#define	KTRFAC_VER_MASK	0x0f000000	/* record version mask */
#define	KTRFAC_VER_SHIFT	24	/* record version shift */

#define	KTRFAC_VERSION(tf)	(((tf) & KTRFAC_VER_MASK) >> KTRFAC_VER_SHIFT)

#define	KTRFACv0	(0 << KTRFAC_VER_SHIFT)
#define	KTRFACv1	(1 << KTRFAC_VER_SHIFT)

#ifndef	_KERNEL

#include <sys/cdefs.h>

__BEGIN_DECLS
int	ktrace(const char *, int, int, pid_t);
int	fktrace(int, int, int, pid_t);
int	utrace(const char *, void *, size_t);
__END_DECLS

#else

void ktrinit(void);
void ktrcsw(struct lwp *, int, int);
void ktremul(struct lwp *);
void ktrgenio(struct lwp *, int, enum uio_rw, struct iovec *, int, int);
void ktrnamei(struct lwp *, const char *, size_t);
void ktrnamei2(struct lwp *, const char *, size_t, const char *, size_t);
void ktrpsig(struct lwp *, int, sig_t, const sigset_t *, const ksiginfo_t *);
void ktrsyscall(struct lwp *, register_t, register_t,
    const struct sysent *, register_t []);
void ktrsysret(struct lwp *, register_t, int, register_t *);
int ktruser(struct lwp *, const char *, void *, size_t, int);
void ktrmmsg(struct lwp *, const void *, size_t);
void ktrkmem(struct lwp *, int, const void *, size_t);
void ktrmib(struct lwp *, const int *, u_int);
void ktrmool(struct lwp *, const void *, size_t, const void *);

void ktrderef(struct proc *);
void ktradref(struct proc *);

extern kmutex_t ktrace_mutex;

#endif	/* !_KERNEL */

#endif /* _SYS_KTRACE_H_ */
