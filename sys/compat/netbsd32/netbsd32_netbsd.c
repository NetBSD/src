/*	$NetBSD: netbsd32_netbsd.c,v 1.19.2.4 2001/01/05 17:35:28 bouyer Exp $	*/

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

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_ddb.h"
#include "opt_ktrace.h"
#include "opt_ntp.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_43.h"
#include "opt_sysv.h"

#include "fs_lfs.h"
#include "fs_nfs.h"
#endif

/*
 * Though COMPAT_OLDSOCK is needed only for COMPAT_43, SunOS, Linux,
 * HP-UX, FreeBSD, Ultrix, OSF1, we define it unconditionally so that
 * this would be LKM-safe.
 */
#define COMPAT_OLDSOCK /* used by <sys/socket.h> */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#define msg __msg /* Don't ask me! */
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <sys/signalvar.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/ktrace.h>
#include <sys/trace.h>
#include <sys/resourcevar.h>
#include <sys/pool.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/namei.h>

#include <uvm/uvm_extern.h>

#include <sys/syscallargs.h>
#include <sys/proc.h>
#include <sys/acct.h>
#include <sys/exec.h>
#define	__SYSCTL_PRIVATE
#include <sys/sysctl.h>

#include <net/if.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <machine/frame.h>

#if defined(DDB)
#include <ddb/ddbvar.h>
#endif

/* this is provided by kern/kern_exec.c */
extern int exec_maxhdrsz;
extern struct lock exec_lock;

static __inline void netbsd32_from_timeval __P((struct timeval *, struct netbsd32_timeval *));
static __inline void netbsd32_to_timeval __P((struct netbsd32_timeval *, struct timeval *));
static __inline void netbsd32_from_itimerval __P((struct itimerval *, struct netbsd32_itimerval *));
static __inline void netbsd32_to_itimerval __P((struct netbsd32_itimerval *, struct itimerval *));
static __inline void netbsd32_to_timespec __P((struct netbsd32_timespec *, struct timespec *));
static __inline void netbsd32_from_timespec __P((struct timespec *, struct netbsd32_timespec *));
static __inline void netbsd32_from_rusage __P((struct rusage *, struct netbsd32_rusage *));
static __inline void netbsd32_to_rusage __P((struct netbsd32_rusage *, struct rusage *));
static __inline int netbsd32_to_iovecin __P((struct netbsd32_iovec *, struct iovec *, int));
static __inline void netbsd32_to_msghdr __P((struct netbsd32_msghdr *, struct msghdr *));
static __inline void netbsd32_from_msghdr __P((struct netbsd32_msghdr *, struct msghdr *));
static __inline void netbsd32_from_statfs __P((struct statfs *, struct netbsd32_statfs *));
static __inline void netbsd32_from_timex __P((struct timex *, struct netbsd32_timex *));
static __inline void netbsd32_to_timex __P((struct netbsd32_timex *, struct timex *));
static __inline void netbsd32_from___stat13 __P((struct stat *, struct netbsd32_stat *));
static __inline void netbsd32_to_ipc_perm __P((struct netbsd32_ipc_perm *, struct ipc_perm *));
static __inline void netbsd32_from_ipc_perm __P((struct ipc_perm *, struct netbsd32_ipc_perm *));
static __inline void netbsd32_to_msg __P((struct netbsd32_msg *, struct msg *));
static __inline void netbsd32_from_msg __P((struct msg *, struct netbsd32_msg *));
static __inline void netbsd32_to_msqid_ds __P((struct netbsd32_msqid_ds *, struct msqid_ds *));
static __inline void netbsd32_from_msqid_ds __P((struct msqid_ds *, struct netbsd32_msqid_ds *));
static __inline void netbsd32_to_shmid_ds __P((struct netbsd32_shmid_ds *, struct shmid_ds *));
static __inline void netbsd32_from_shmid_ds __P((struct shmid_ds *, struct netbsd32_shmid_ds *));
static __inline void netbsd32_to_semid_ds __P((struct  netbsd32_semid_ds *, struct  semid_ds *));
static __inline void netbsd32_from_semid_ds __P((struct  semid_ds *, struct  netbsd32_semid_ds *));


static int recvit32 __P((struct proc *, int, struct netbsd32_msghdr *, struct iovec *, caddr_t, 
			 register_t *));
static int dofilereadv32 __P((struct proc *, int, struct file *, struct netbsd32_iovec *, 
			      int, off_t *, int, register_t *));
static int dofilewritev32 __P((struct proc *, int, struct file *, struct netbsd32_iovec *, 
			       int,  off_t *, int, register_t *));
static int change_utimes32 __P((struct vnode *, struct timeval *, struct proc *));

extern char netbsd32_sigcode[], netbsd32_esigcode[];
extern struct sysent netbsd32_sysent[];
#ifdef SYSCALL_DEBUG
extern const char * const netbsd32_syscallnames[];
#endif
#ifdef __HAVE_SYSCALL_INTERN
void syscall_intern __P((struct proc *));
#else
void syscall __P((void));
#endif

const struct emul emul_netbsd32 = {
	"netbsd32",
	"/emul/netbsd32",
#ifndef __HAVE_MINIMAL_EMUL
	0,
	NULL,
	netbsd32_SYS_syscall,
	netbsd32_SYS_MAXSYSCALL,
#endif
	netbsd32_sysent,
#ifdef SYSCALL_DEBUG
	netbsd32_syscallnames,
#else
	NULL,
#endif
	netbsd32_sendsig,
	netbsd32_sigcode,
	netbsd32_esigcode,
	NULL,
	NULL,
	NULL,
#ifdef __HAVE_SYSCALL_INTERN
	syscall_intern,
#else
	syscall,
#endif
};

/* converters for structures that we need */
static __inline void
netbsd32_from_timeval(tv, tv32)
	struct timeval *tv;
	struct netbsd32_timeval *tv32;
{

	tv32->tv_sec = (netbsd32_long)tv->tv_sec;
	tv32->tv_usec = (netbsd32_long)tv->tv_usec;
}

static __inline void
netbsd32_to_timeval(tv32, tv)
	struct netbsd32_timeval *tv32;
	struct timeval *tv;
{

	tv->tv_sec = (long)tv32->tv_sec;
	tv->tv_usec = (long)tv32->tv_usec;
}

static __inline void
netbsd32_from_itimerval(itv, itv32)
	struct itimerval *itv;
	struct netbsd32_itimerval *itv32;
{

	netbsd32_from_timeval(&itv->it_interval, 
			     &itv32->it_interval);
	netbsd32_from_timeval(&itv->it_value, 
			     &itv32->it_value);
}

static __inline void
netbsd32_to_itimerval(itv32, itv)
	struct netbsd32_itimerval *itv32;
	struct itimerval *itv;
{

	netbsd32_to_timeval(&itv32->it_interval, &itv->it_interval);
	netbsd32_to_timeval(&itv32->it_value, &itv->it_value);
}

static __inline void
netbsd32_to_timespec(s32p, p)
	struct netbsd32_timespec *s32p;
	struct timespec *p;
{

	p->tv_sec = (time_t)s32p->tv_sec;
	p->tv_nsec = (long)s32p->tv_nsec;
}

static __inline void
netbsd32_from_timespec(p, s32p)
	struct timespec *p;
	struct netbsd32_timespec *s32p;
{

	s32p->tv_sec = (netbsd32_time_t)p->tv_sec;
	s32p->tv_nsec = (netbsd32_long)p->tv_nsec;
}

static __inline void
netbsd32_from_rusage(rup, ru32p)
	struct rusage *rup;
	struct netbsd32_rusage *ru32p;
{

	netbsd32_from_timeval(&rup->ru_utime, &ru32p->ru_utime);
	netbsd32_from_timeval(&rup->ru_stime, &ru32p->ru_stime);
#define C(var)	ru32p->var = (netbsd32_long)rup->var
	C(ru_maxrss);
	C(ru_ixrss);
	C(ru_idrss);
	C(ru_isrss);
	C(ru_minflt);
	C(ru_majflt);
	C(ru_nswap);
	C(ru_inblock);
	C(ru_oublock);
	C(ru_msgsnd);
	C(ru_msgrcv);
	C(ru_nsignals);
	C(ru_nvcsw);
	C(ru_nivcsw);
#undef C
}

static __inline void
netbsd32_to_rusage(ru32p, rup)
	struct netbsd32_rusage *ru32p;
	struct rusage *rup;
{

	netbsd32_to_timeval(&ru32p->ru_utime, &rup->ru_utime);
	netbsd32_to_timeval(&ru32p->ru_stime, &rup->ru_stime);
#define C(var)	rup->var = (long)ru32p->var
	C(ru_maxrss);
	C(ru_ixrss);
	C(ru_idrss);
	C(ru_isrss);
	C(ru_minflt);
	C(ru_majflt);
	C(ru_nswap);
	C(ru_inblock);
	C(ru_oublock);
	C(ru_msgsnd);
	C(ru_msgrcv);
	C(ru_nsignals);
	C(ru_nvcsw);
	C(ru_nivcsw);
#undef C
}

static __inline int
netbsd32_to_iovecin(iov32p, iovp, len)
	struct netbsd32_iovec *iov32p;
	struct iovec *iovp;
	int len;
{
	int i, error=0;
	u_int32_t iov_base;
	u_int32_t iov_len;
	/* 
	 * We could allocate an iov32p, do a copyin, and translate
	 * each field and then free it all up, or we could copyin
	 * each field separately.  I'm doing the latter to reduce
	 * the number of MALLOC()s.
	 */
	for (i = 0; i < len; i++, iovp++, iov32p++) {
		if ((error = copyin((caddr_t)&iov32p->iov_base, &iov_base, sizeof(iov_base))))
		    return (error);
		if ((error = copyin((caddr_t)&iov32p->iov_len, &iov_len, sizeof(iov_len))))
		    return (error);
		iovp->iov_base = (void *)(u_long)iov_base;
		iovp->iov_len = (size_t)iov_len;
	}
	return error;
}

/* msg_iov must be done separately */
static __inline void
netbsd32_to_msghdr(mhp32, mhp)
	struct netbsd32_msghdr *mhp32;
	struct msghdr *mhp;
{

	mhp->msg_name = (caddr_t)(u_long)mhp32->msg_name;
	mhp->msg_namelen = mhp32->msg_namelen;
	mhp->msg_iovlen = (size_t)mhp32->msg_iovlen;
	mhp->msg_control = (caddr_t)(u_long)mhp32->msg_control;
	mhp->msg_controllen = mhp32->msg_controllen;
	mhp->msg_flags = mhp32->msg_flags;
}

/* msg_iov must be done separately */
static __inline void
netbsd32_from_msghdr(mhp32, mhp)
	struct netbsd32_msghdr *mhp32;
	struct msghdr *mhp;
{

	mhp32->msg_name = mhp32->msg_name;
	mhp32->msg_namelen = mhp32->msg_namelen;
	mhp32->msg_iovlen = mhp32->msg_iovlen;
	mhp32->msg_control = mhp32->msg_control;
	mhp32->msg_controllen = mhp->msg_controllen;
	mhp32->msg_flags = mhp->msg_flags;
}

static __inline void
netbsd32_from_statfs(sbp, sb32p)
	struct statfs *sbp;
	struct netbsd32_statfs *sb32p;
{
	sb32p->f_type = sbp->f_type;
	sb32p->f_flags = sbp->f_flags;
	sb32p->f_bsize = (netbsd32_long)sbp->f_bsize;
	sb32p->f_iosize = (netbsd32_long)sbp->f_iosize;
	sb32p->f_blocks = (netbsd32_long)sbp->f_blocks;
	sb32p->f_bfree = (netbsd32_long)sbp->f_bfree;
	sb32p->f_bavail = (netbsd32_long)sbp->f_bavail;
	sb32p->f_files = (netbsd32_long)sbp->f_files;
	sb32p->f_ffree = (netbsd32_long)sbp->f_ffree;
	sb32p->f_fsid = sbp->f_fsid;
	sb32p->f_owner = sbp->f_owner;
	sb32p->f_spare[0] = 0;
	sb32p->f_spare[1] = 0;
	sb32p->f_spare[2] = 0;
	sb32p->f_spare[3] = 0;
#if 1
	/* May as well do the whole batch in one go */
	memcpy(sb32p->f_fstypename, sbp->f_fstypename, MFSNAMELEN+MNAMELEN+MNAMELEN);
#else
	/* If we want to be careful */
	memcpy(sb32p->f_fstypename, sbp->f_fstypename, MFSNAMELEN);
	memcpy(sb32p->f_mntonname, sbp->f_mntonname, MNAMELEN);
	memcpy(sb32p->f_mntfromname, sbp->f_mntfromname, MNAMELEN);
#endif
}

static __inline void
netbsd32_from_timex(txp, tx32p)
	struct timex *txp;
	struct netbsd32_timex *tx32p;
{

	tx32p->modes = txp->modes;
	tx32p->offset = (netbsd32_long)txp->offset;
	tx32p->freq = (netbsd32_long)txp->freq;
	tx32p->maxerror = (netbsd32_long)txp->maxerror;
	tx32p->esterror = (netbsd32_long)txp->esterror;
	tx32p->status = txp->status;
	tx32p->constant = (netbsd32_long)txp->constant;
	tx32p->precision = (netbsd32_long)txp->precision;
	tx32p->tolerance = (netbsd32_long)txp->tolerance;
	tx32p->ppsfreq = (netbsd32_long)txp->ppsfreq;
	tx32p->jitter = (netbsd32_long)txp->jitter;
	tx32p->shift = txp->shift;
	tx32p->stabil = (netbsd32_long)txp->stabil;
	tx32p->jitcnt = (netbsd32_long)txp->jitcnt;
	tx32p->calcnt = (netbsd32_long)txp->calcnt;
	tx32p->errcnt = (netbsd32_long)txp->errcnt;
	tx32p->stbcnt = (netbsd32_long)txp->stbcnt;
}

static __inline void
netbsd32_to_timex(tx32p, txp)
	struct netbsd32_timex *tx32p;
	struct timex *txp;
{

	txp->modes = tx32p->modes;
	txp->offset = (long)tx32p->offset;
	txp->freq = (long)tx32p->freq;
	txp->maxerror = (long)tx32p->maxerror;
	txp->esterror = (long)tx32p->esterror;
	txp->status = tx32p->status;
	txp->constant = (long)tx32p->constant;
	txp->precision = (long)tx32p->precision;
	txp->tolerance = (long)tx32p->tolerance;
	txp->ppsfreq = (long)tx32p->ppsfreq;
	txp->jitter = (long)tx32p->jitter;
	txp->shift = tx32p->shift;
	txp->stabil = (long)tx32p->stabil;
	txp->jitcnt = (long)tx32p->jitcnt;
	txp->calcnt = (long)tx32p->calcnt;
	txp->errcnt = (long)tx32p->errcnt;
	txp->stbcnt = (long)tx32p->stbcnt;
}

static __inline void
netbsd32_from___stat13(sbp, sb32p)
	struct stat *sbp;
	struct netbsd32_stat *sb32p;
{
	sb32p->st_dev = sbp->st_dev;
	sb32p->st_ino = sbp->st_ino;
	sb32p->st_mode = sbp->st_mode;
	sb32p->st_nlink = sbp->st_nlink;
	sb32p->st_uid = sbp->st_uid;
	sb32p->st_gid = sbp->st_gid;
	sb32p->st_rdev = sbp->st_rdev;
	if (sbp->st_size < (quad_t)1 << 32)
		sb32p->st_size = sbp->st_size;
	else
		sb32p->st_size = -2;
	sb32p->st_atimespec.tv_sec = (netbsd32_time_t)sbp->st_atimespec.tv_sec;
	sb32p->st_atimespec.tv_nsec = (netbsd32_long)sbp->st_atimespec.tv_nsec;
	sb32p->st_mtimespec.tv_sec = (netbsd32_time_t)sbp->st_mtimespec.tv_sec;
	sb32p->st_mtimespec.tv_nsec = (netbsd32_long)sbp->st_mtimespec.tv_nsec;
	sb32p->st_ctimespec.tv_sec = (netbsd32_time_t)sbp->st_ctimespec.tv_sec;
	sb32p->st_ctimespec.tv_nsec = (netbsd32_long)sbp->st_ctimespec.tv_nsec;
	sb32p->st_blksize = sbp->st_blksize;
	sb32p->st_blocks = sbp->st_blocks;
	sb32p->st_flags = sbp->st_flags;
	sb32p->st_gen = sbp->st_gen;
}

static __inline void
netbsd32_to_ipc_perm(ip32p, ipp)
	struct netbsd32_ipc_perm *ip32p;
	struct ipc_perm *ipp;
{

	ipp->cuid = ip32p->cuid;
	ipp->cgid = ip32p->cgid;
	ipp->uid = ip32p->uid;
	ipp->gid = ip32p->gid;
	ipp->mode = ip32p->mode;
	ipp->_seq = ip32p->_seq;
	ipp->_key = (key_t)ip32p->_key;
}

static __inline void
netbsd32_from_ipc_perm(ipp, ip32p)
	struct ipc_perm *ipp;
	struct netbsd32_ipc_perm *ip32p;
{

	ip32p->cuid = ipp->cuid;
	ip32p->cgid = ipp->cgid;
	ip32p->uid = ipp->uid;
	ip32p->gid = ipp->gid;
	ip32p->mode = ipp->mode;
	ip32p->_seq = ipp->_seq;
	ip32p->_key = (netbsd32_key_t)ipp->_key;
}

static __inline void
netbsd32_to_msg(m32p, mp)
	struct netbsd32_msg *m32p;
	struct msg *mp;
{

	mp->msg_next = (struct msg *)(u_long)m32p->msg_next;
	mp->msg_type = (long)m32p->msg_type;
	mp->msg_ts = m32p->msg_ts;
	mp->msg_spot = m32p->msg_spot;
}

static __inline void
netbsd32_from_msg(mp, m32p)
	struct msg *mp;
	struct netbsd32_msg *m32p;
{

	m32p->msg_next = (netbsd32_msgp_t)(u_long)mp->msg_next;
	m32p->msg_type = (netbsd32_long)mp->msg_type;
	m32p->msg_ts = mp->msg_ts;
	m32p->msg_spot = mp->msg_spot;
}

static __inline void
netbsd32_to_msqid_ds(ds32p, dsp)
	struct netbsd32_msqid_ds *ds32p;
	struct msqid_ds *dsp;
{

	netbsd32_to_ipc_perm(&ds32p->msg_perm, &dsp->msg_perm);
	netbsd32_to_msg((struct netbsd32_msg *)(u_long)ds32p->_msg_first, dsp->_msg_first);
	netbsd32_to_msg((struct netbsd32_msg *)(u_long)ds32p->_msg_last, dsp->_msg_last);
	dsp->_msg_cbytes = (u_long)ds32p->_msg_cbytes;
	dsp->msg_qnum = (u_long)ds32p->msg_qnum;
	dsp->msg_qbytes = (u_long)ds32p->msg_qbytes;
	dsp->msg_lspid = ds32p->msg_lspid;
	dsp->msg_lrpid = ds32p->msg_lrpid;
	dsp->msg_rtime = (time_t)ds32p->msg_rtime;
	dsp->msg_stime = (time_t)ds32p->msg_stime;
	dsp->msg_ctime = (time_t)ds32p->msg_ctime;
}

static __inline void
netbsd32_from_msqid_ds(dsp, ds32p)
	struct msqid_ds *dsp;
	struct netbsd32_msqid_ds *ds32p;
{

	netbsd32_from_ipc_perm(&dsp->msg_perm, &ds32p->msg_perm);
	netbsd32_from_msg(dsp->_msg_first, (struct netbsd32_msg *)(u_long)ds32p->_msg_first);
	netbsd32_from_msg(dsp->_msg_last, (struct netbsd32_msg *)(u_long)ds32p->_msg_last);
	ds32p->_msg_cbytes = (netbsd32_u_long)dsp->_msg_cbytes;
	ds32p->msg_qnum = (netbsd32_u_long)dsp->msg_qnum;
	ds32p->msg_qbytes = (netbsd32_u_long)dsp->msg_qbytes;
	ds32p->msg_lspid = dsp->msg_lspid;
	ds32p->msg_lrpid = dsp->msg_lrpid;
	ds32p->msg_rtime = dsp->msg_rtime;
	ds32p->msg_stime = dsp->msg_stime;
	ds32p->msg_ctime = dsp->msg_ctime;
}

static __inline void
netbsd32_to_shmid_ds(ds32p, dsp)
	struct netbsd32_shmid_ds *ds32p;
	struct shmid_ds *dsp;
{

	netbsd32_to_ipc_perm(&ds32p->shm_perm, &dsp->shm_perm);
	dsp->shm_segsz = ds32p->shm_segsz;
	dsp->shm_lpid = ds32p->shm_lpid;
	dsp->shm_cpid = ds32p->shm_cpid;
	dsp->shm_nattch = ds32p->shm_nattch;
	dsp->shm_atime = (long)ds32p->shm_atime;
	dsp->shm_dtime = (long)ds32p->shm_dtime;
	dsp->shm_ctime = (long)ds32p->shm_ctime;
	dsp->_shm_internal = (void *)(u_long)ds32p->_shm_internal;
}

static __inline void
netbsd32_from_shmid_ds(dsp, ds32p)
	struct shmid_ds *dsp;
	struct netbsd32_shmid_ds *ds32p;
{

	netbsd32_from_ipc_perm(&dsp->shm_perm, &ds32p->shm_perm);
	ds32p->shm_segsz = dsp->shm_segsz;
	ds32p->shm_lpid = dsp->shm_lpid;
	ds32p->shm_cpid = dsp->shm_cpid;
	ds32p->shm_nattch = dsp->shm_nattch;
	ds32p->shm_atime = (netbsd32_long)dsp->shm_atime;
	ds32p->shm_dtime = (netbsd32_long)dsp->shm_dtime;
	ds32p->shm_ctime = (netbsd32_long)dsp->shm_ctime;
	ds32p->_shm_internal = (netbsd32_voidp)(u_long)dsp->_shm_internal;
}

static __inline void
netbsd32_to_semid_ds(s32dsp, dsp)
	struct  netbsd32_semid_ds *s32dsp;
	struct  semid_ds *dsp;
{

	netbsd32_from_ipc_perm(&dsp->sem_perm, &s32dsp->sem_perm);
	dsp->_sem_base = (struct __sem *)(u_long)s32dsp->_sem_base;
	dsp->sem_nsems = s32dsp->sem_nsems;
	dsp->sem_otime = s32dsp->sem_otime;
	dsp->sem_ctime = s32dsp->sem_ctime;
}

static __inline void
netbsd32_from_semid_ds(dsp, s32dsp)
	struct  semid_ds *dsp;
	struct  netbsd32_semid_ds *s32dsp;
{

	netbsd32_to_ipc_perm(&s32dsp->sem_perm, &dsp->sem_perm);
	s32dsp->_sem_base = (netbsd32_semp_t)(u_long)dsp->_sem_base;
	s32dsp->sem_nsems = dsp->sem_nsems;
	s32dsp->sem_otime = dsp->sem_otime;
	s32dsp->sem_ctime = dsp->sem_ctime;
}

/*
 * below are all the standard NetBSD system calls, in the 32bit
 * environment, with the necessary conversions to 64bit before
 * calling the real syscall, unless we need to inline the whole
 * syscall here, sigh.
 */

int
netbsd32_exit(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_exit_args /* {
		syscallarg(int) rval;
	} */ *uap = v;
	struct sys_exit_args ua;

	NETBSD32TO64_UAP(rval);
	return sys_exit(p, &ua, retval);
}

int
netbsd32_read(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_read_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_voidp) buf;
		syscallarg(netbsd32_size_t) nbyte;
	} */ *uap = v;
	struct sys_read_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(buf, void *);
	NETBSD32TOX_UAP(nbyte, size_t);
	return sys_read(p, &ua, retval);
}

int
netbsd32_write(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_write_args /* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_voidp) buf;
		syscallarg(netbsd32_size_t) nbyte;
	} */ *uap = v;
	struct sys_write_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(buf, void *);
	NETBSD32TOX_UAP(nbyte, size_t);
	return sys_write(p, &ua, retval);
}

int
netbsd32_close(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_close_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct sys_close_args ua;

	NETBSD32TO64_UAP(fd);
	return sys_close(p, &ua, retval);
}

int
netbsd32_open(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_open_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) flags;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct sys_open_args ua;
	caddr_t sg;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);
	NETBSD32TO64_UAP(mode);
	sg = stackgap_init(p->p_emul);
	CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));

	return (sys_open(p, &ua, retval));
}

int
netbsd32_wait4(q, v, retval)
	struct proc *q;
	void *v;
	register_t *retval;
{
	struct netbsd32_wait4_args /* {
		syscallarg(int) pid;
		syscallarg(netbsd32_intp) status;
		syscallarg(int) options;
		syscallarg(netbsd32_rusagep_t) rusage;
	} */ *uap = v;
	struct netbsd32_rusage ru32;
	int nfound;
	struct proc *p, *t;
	int status, error;

	if (SCARG(uap, pid) == 0)
		SCARG(uap, pid) = -q->p_pgid;
	if (SCARG(uap, options) &~ (WUNTRACED|WNOHANG))
		return (EINVAL);

loop:
	nfound = 0;
	for (p = q->p_children.lh_first; p != 0; p = p->p_sibling.le_next) {
		if (SCARG(uap, pid) != WAIT_ANY &&
		    p->p_pid != SCARG(uap, pid) &&
		    p->p_pgid != -SCARG(uap, pid))
			continue;
		nfound++;
		if (p->p_stat == SZOMB) {
			retval[0] = p->p_pid;

			if (SCARG(uap, status)) {
				status = p->p_xstat;	/* convert to int */
				error = copyout((caddr_t)&status,
						(caddr_t)(u_long)SCARG(uap, status),
						sizeof(status));
				if (error)
					return (error);
			}
			if (SCARG(uap, rusage)) {
				netbsd32_from_rusage(p->p_ru, &ru32);
				if ((error = copyout((caddr_t)&ru32,
						     (caddr_t)(u_long)SCARG(uap, rusage),
						     sizeof(struct netbsd32_rusage))))
					return (error);
			}
			/*
			 * If we got the child via ptrace(2) or procfs, and
			 * the parent is different (meaning the process was
			 * attached, rather than run as a child), then we need
			 * to give it back to the old parent, and send the
			 * parent a SIGCHLD.  The rest of the cleanup will be
			 * done when the old parent waits on the child.
			 */
			if ((p->p_flag & P_TRACED) &&
			    p->p_oppid != p->p_pptr->p_pid) {
				t = pfind(p->p_oppid);
				proc_reparent(p, t ? t : initproc);
				p->p_oppid = 0;
				p->p_flag &= ~(P_TRACED|P_WAITED|P_FSTRACE);
				psignal(p->p_pptr, SIGCHLD);
				wakeup((caddr_t)p->p_pptr);
				return (0);
			}
			p->p_xstat = 0;
			ruadd(&q->p_stats->p_cru, p->p_ru);
			pool_put(&rusage_pool, p->p_ru);

			/*
			 * Finally finished with old proc entry.
			 * Unlink it from its process group and free it.
			 */
			leavepgrp(p);

			LIST_REMOVE(p, p_list);	/* off zombproc */

			LIST_REMOVE(p, p_sibling);

			/*
			 * Decrement the count of procs running with this uid.
			 */
			(void)chgproccnt(p->p_cred->p_ruid, -1);

			/*
			 * Free up credentials.
			 */
			if (--p->p_cred->p_refcnt == 0) {
				crfree(p->p_cred->pc_ucred);
				pool_put(&pcred_pool, p->p_cred);
			}

			/*
			 * Release reference to text vnode
			 */
			if (p->p_textvp)
				vrele(p->p_textvp);

			pool_put(&proc_pool, p);
			nprocs--;
			return (0);
		}
		if (p->p_stat == SSTOP && (p->p_flag & P_WAITED) == 0 &&
		    (p->p_flag & P_TRACED || SCARG(uap, options) & WUNTRACED)) {
			p->p_flag |= P_WAITED;
			retval[0] = p->p_pid;

			if (SCARG(uap, status)) {
				status = W_STOPCODE(p->p_xstat);
				error = copyout((caddr_t)&status,
				    (caddr_t)(u_long)SCARG(uap, status),
				    sizeof(status));
			} else
				error = 0;
			return (error);
		}
	}
	if (nfound == 0)
		return (ECHILD);
	if (SCARG(uap, options) & WNOHANG) {
		retval[0] = 0;
		return (0);
	}
	if ((error = tsleep((caddr_t)q, PWAIT | PCATCH, "wait", 0)) != 0)
		return (error);
	goto loop;
}

int
netbsd32_link(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_link_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_charp) link;
	} */ *uap = v;
	struct sys_link_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(link, const char);
	return (sys_link(p, &ua, retval));
}

int
netbsd32_unlink(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_unlink_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_unlink_args ua;

	NETBSD32TOP_UAP(path, const char);

	return (sys_unlink(p, &ua, retval));
}

int
netbsd32_chdir(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_chdir_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_chdir_args ua;

	NETBSD32TOP_UAP(path, const char);

	return (sys_chdir(p, &ua, retval));
}

int
netbsd32_fchdir(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_fchdir_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct sys_fchdir_args ua;

	NETBSD32TO64_UAP(fd);

	return (sys_fchdir(p, &ua, retval));
}

int
netbsd32_mknod(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_mknod_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(mode_t) mode;
		syscallarg(dev_t) dev;
	} */ *uap = v;
	struct sys_mknod_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(dev);
	NETBSD32TO64_UAP(mode);

	return (sys_mknod(p, &ua, retval));
}

int
netbsd32_chmod(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_chmod_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct sys_chmod_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(mode);

	return (sys_chmod(p, &ua, retval));
}

int
netbsd32_chown(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_chown_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct sys_chown_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(uid);
	NETBSD32TO64_UAP(gid);

	return (sys_chown(p, &ua, retval));
}

int
netbsd32_break(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_break_args /* {
		syscallarg(netbsd32_charp) nsize;
	} */ *uap = v;
	struct sys_obreak_args ua;

	SCARG(&ua, nsize) = (char *)(u_long)SCARG(uap, nsize);
	NETBSD32TOP_UAP(nsize, char);
	return (sys_obreak(p, &ua, retval));
}

int
netbsd32_getfsstat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_getfsstat_args /* {
		syscallarg(netbsd32_statfsp_t) buf;
		syscallarg(netbsd32_long) bufsize;
		syscallarg(int) flags;
	} */ *uap = v;
	struct mount *mp, *nmp;
	struct statfs *sp;
	struct netbsd32_statfs sb32;
	caddr_t sfsp;
	long count, maxcount, error;

	maxcount = SCARG(uap, bufsize) / sizeof(struct netbsd32_statfs);
	sfsp = (caddr_t)(u_long)SCARG(uap, buf);
	simple_lock(&mountlist_slock);
	count = 0;
	for (mp = mountlist.cqh_first; mp != (void *)&mountlist; mp = nmp) {
		if (vfs_busy(mp, LK_NOWAIT, &mountlist_slock)) {
			nmp = mp->mnt_list.cqe_next;
			continue;
		}
		if (sfsp && count < maxcount) {
			sp = &mp->mnt_stat;
			/*
			 * If MNT_NOWAIT or MNT_LAZY is specified, do not
			 * refresh the fsstat cache. MNT_WAIT or MNT_LAXY
			 * overrides MNT_NOWAIT.
			 */
			if (SCARG(uap, flags) != MNT_NOWAIT &&
			    SCARG(uap, flags) != MNT_LAZY &&
			    (SCARG(uap, flags) == MNT_WAIT ||
			     SCARG(uap, flags) == 0) &&
			    (error = VFS_STATFS(mp, sp, p)) != 0) {
				simple_lock(&mountlist_slock);
				nmp = mp->mnt_list.cqe_next;
				vfs_unbusy(mp);
				continue;
			}
			sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
			sp->f_oflags = sp->f_flags & 0xffff;
			netbsd32_from_statfs(sp, &sb32);
			error = copyout(&sb32, sfsp, sizeof(sb32));
			if (error) {
				vfs_unbusy(mp);
				return (error);
			}
			sfsp += sizeof(sb32);
		}
		count++;
		simple_lock(&mountlist_slock);
		nmp = mp->mnt_list.cqe_next;
		vfs_unbusy(mp);
	}
	simple_unlock(&mountlist_slock);
	if (sfsp && count > maxcount)
		*retval = maxcount;
	else
		*retval = count;
	return (0);
}

int
netbsd32_mount(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_mount_args /* {
		syscallarg(const netbsd32_charp) type;
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) flags;
		syscallarg(netbsd32_voidp) data;
	} */ *uap = v;
	struct sys_mount_args ua;

	NETBSD32TOP_UAP(type, const char);
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);
	NETBSD32TOP_UAP(data, void);
	return (sys_mount(p, &ua, retval));
}

int
netbsd32_unmount(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_unmount_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys_unmount_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);
	return (sys_unmount(p, &ua, retval));
}

int
netbsd32_setuid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_setuid_args /* {
		syscallarg(uid_t) uid;
	} */ *uap = v;
	struct sys_setuid_args ua;

	NETBSD32TO64_UAP(uid);
	return (sys_setuid(p, &ua, retval));
}

int
netbsd32_ptrace(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_ptrace_args /* {
		syscallarg(int) req;
		syscallarg(pid_t) pid;
		syscallarg(netbsd32_caddr_t) addr;
		syscallarg(int) data;
	} */ *uap = v;
	struct sys_ptrace_args ua;

	NETBSD32TO64_UAP(req);
	NETBSD32TO64_UAP(pid);
	NETBSD32TOX64_UAP(addr, caddr_t);
	NETBSD32TO64_UAP(data);
	return (sys_ptrace(p, &ua, retval));
}

int
netbsd32_recvmsg(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_recvmsg_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_msghdrp_t) msg;
		syscallarg(int) flags;
	} */ *uap = v;
	struct netbsd32_msghdr msg;
	struct iovec aiov[UIO_SMALLIOV], *uiov, *iov;
	int error;

	error = copyin((caddr_t)(u_long)SCARG(uap, msg), (caddr_t)&msg,
		       sizeof(msg));
		/* netbsd32_msghdr needs the iov pre-allocated */
	if (error)
		return (error);
	if ((u_int)msg.msg_iovlen > UIO_SMALLIOV) {
		if ((u_int)msg.msg_iovlen > IOV_MAX)
			return (EMSGSIZE);
		MALLOC(iov, struct iovec *,
		       sizeof(struct iovec) * (u_int)msg.msg_iovlen, M_IOV,
		       M_WAITOK);
	} else if ((u_int)msg.msg_iovlen > 0)
		iov = aiov;
	else
		return (EMSGSIZE);
#ifdef COMPAT_OLDSOCK
	msg.msg_flags = SCARG(uap, flags) &~ MSG_COMPAT;
#else
	msg.msg_flags = SCARG(uap, flags);
#endif
	uiov = (struct iovec *)(u_long)msg.msg_iov;
	error = netbsd32_to_iovecin((struct netbsd32_iovec *)uiov, 
				   iov, msg.msg_iovlen);
	if (error)
		goto done;
	if ((error = recvit32(p, SCARG(uap, s), &msg, iov, (caddr_t)0, retval)) == 0) {
		error = copyout((caddr_t)&msg, (caddr_t)(u_long)SCARG(uap, msg),
		    sizeof(msg));
	}
done:
	if (iov != aiov)
		FREE(iov, M_IOV);
	return (error);
}

int
recvit32(p, s, mp, iov, namelenp, retsize)
	struct proc *p;
	int s;
	struct netbsd32_msghdr *mp;
	struct iovec *iov;
	caddr_t namelenp;
	register_t *retsize;
{
	struct file *fp;
	struct uio auio;
	int i;
	int len, error;
	struct mbuf *from = 0, *control = 0;
	struct socket *so;
#ifdef KTRACE
	struct iovec *ktriov = NULL;
#endif
	
	/* getsock() will use the descriptor for us */
	if ((error = getsock(p->p_fd, s, &fp)) != 0)
		return (error);
	auio.uio_iov = (struct iovec *)(u_long)mp->msg_iov;
	auio.uio_iovcnt = mp->msg_iovlen;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_rw = UIO_READ;
	auio.uio_procp = p;
	auio.uio_offset = 0;			/* XXX */
	auio.uio_resid = 0;
	for (i = 0; i < mp->msg_iovlen; i++, iov++) {
#if 0
		/* cannot happen iov_len is unsigned */
		if (iov->iov_len < 0) {
			error = EINVAL;
			goto out1;
		}
#endif
		/*
		 * Reads return ssize_t because -1 is returned on error.
		 * Therefore we must restrict the length to SSIZE_MAX to
		 * avoid garbage return values.
		 */
		auio.uio_resid += iov->iov_len;
		if (iov->iov_len > SSIZE_MAX || auio.uio_resid > SSIZE_MAX) {
			error = EINVAL;
			goto out1;
		}
	}
#ifdef KTRACE
	if (KTRPOINT(p, KTR_GENIO)) {
		int iovlen = auio.uio_iovcnt * sizeof(struct iovec);

		MALLOC(ktriov, struct iovec *, iovlen, M_TEMP, M_WAITOK);
		memcpy((caddr_t)ktriov, (caddr_t)auio.uio_iov, iovlen);
	}
#endif
	len = auio.uio_resid;
	so = (struct socket *)fp->f_data;
	error = (*so->so_receive)(so, &from, &auio, NULL,
			  mp->msg_control ? &control : NULL, &mp->msg_flags);
	if (error) {
		if (auio.uio_resid != len && (error == ERESTART ||
		    error == EINTR || error == EWOULDBLOCK))
			error = 0;
	}
#ifdef KTRACE
	if (ktriov != NULL) {
		if (error == 0)
			ktrgenio(p, s, UIO_READ, ktriov,
			    len - auio.uio_resid, error);
		FREE(ktriov, M_TEMP);
	}
#endif
	if (error)
		goto out;
	*retsize = len - auio.uio_resid;
	if (mp->msg_name) {
		len = mp->msg_namelen;
		if (len <= 0 || from == 0)
			len = 0;
		else {
#ifdef COMPAT_OLDSOCK
			if (mp->msg_flags & MSG_COMPAT)
				mtod(from, struct osockaddr *)->sa_family =
				    mtod(from, struct sockaddr *)->sa_family;
#endif
			if (len > from->m_len)
				len = from->m_len;
			/* else if len < from->m_len ??? */
			error = copyout(mtod(from, caddr_t),
					(caddr_t)(u_long)mp->msg_name, (unsigned)len);
			if (error)
				goto out;
		}
		mp->msg_namelen = len;
		if (namelenp &&
		    (error = copyout((caddr_t)&len, namelenp, sizeof(int)))) {
#ifdef COMPAT_OLDSOCK
			if (mp->msg_flags & MSG_COMPAT)
				error = 0;	/* old recvfrom didn't check */
			else
#endif
			goto out;
		}
	}
	if (mp->msg_control) {
#ifdef COMPAT_OLDSOCK
		/*
		 * We assume that old recvmsg calls won't receive access
		 * rights and other control info, esp. as control info
		 * is always optional and those options didn't exist in 4.3.
		 * If we receive rights, trim the cmsghdr; anything else
		 * is tossed.
		 */
		if (control && mp->msg_flags & MSG_COMPAT) {
			if (mtod(control, struct cmsghdr *)->cmsg_level !=
			    SOL_SOCKET ||
			    mtod(control, struct cmsghdr *)->cmsg_type !=
			    SCM_RIGHTS) {
				mp->msg_controllen = 0;
				goto out;
			}
			control->m_len -= sizeof(struct cmsghdr);
			control->m_data += sizeof(struct cmsghdr);
		}
#endif
		len = mp->msg_controllen;
		if (len <= 0 || control == 0)
			len = 0;
		else {
			struct mbuf *m = control;
			caddr_t p = (caddr_t)(u_long)mp->msg_control;

			do {
				i = m->m_len;
				if (len < i) {
					mp->msg_flags |= MSG_CTRUNC;
					i = len;
				}
				error = copyout(mtod(m, caddr_t), p,
				    (unsigned)i);
				if (m->m_next)
					i = ALIGN(i);
				p += i;
				len -= i;
				if (error != 0 || len <= 0)
					break;
			} while ((m = m->m_next) != NULL);
			len = p - (caddr_t)(u_long)mp->msg_control;
		}
		mp->msg_controllen = len;
	}
 out:
	if (from)
		m_freem(from);
	if (control)
		m_freem(control);
 out1:
	FILE_UNUSE(fp, p);
	return (error);
}


int
netbsd32_sendmsg(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_sendmsg_args /* {
		syscallarg(int) s;
		syscallarg(const netbsd32_msghdrp_t) msg;
		syscallarg(int) flags;
	} */ *uap = v;
	struct msghdr msg;
	struct netbsd32_msghdr msg32;
	struct iovec aiov[UIO_SMALLIOV], *iov;
	int error;

	error = copyin((caddr_t)(u_long)SCARG(uap, msg), 
		       (caddr_t)&msg32, sizeof(msg32));
	if (error)
		return (error);
	netbsd32_to_msghdr(&msg32, &msg);
	if ((u_int)msg.msg_iovlen > UIO_SMALLIOV) {
		if ((u_int)msg.msg_iovlen > IOV_MAX)
			return (EMSGSIZE);
		MALLOC(iov, struct iovec *,
		       sizeof(struct iovec) * (u_int)msg.msg_iovlen, M_IOV,
		       M_WAITOK);
	} else if ((u_int)msg.msg_iovlen > 0)
		iov = aiov;
	else
		return (EMSGSIZE);
	error = netbsd32_to_iovecin((struct netbsd32_iovec *)msg.msg_iov, 
				   iov, msg.msg_iovlen);
	if (error)
		goto done;
	msg.msg_iov = iov;
#ifdef COMPAT_OLDSOCK
	msg.msg_flags = 0;
#endif
	/* Luckily we can use this directly */
	error = sendit(p, SCARG(uap, s), &msg, SCARG(uap, flags), retval);
done:
	if (iov != aiov)
		FREE(iov, M_IOV);
	return (error);
}

int
netbsd32_recvfrom(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_recvfrom_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_voidp) buf;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) flags;
		syscallarg(netbsd32_sockaddrp_t) from;
		syscallarg(netbsd32_intp) fromlenaddr;
	} */ *uap = v;
	struct netbsd32_msghdr msg;
	struct iovec aiov;
	int error;

	if (SCARG(uap, fromlenaddr)) {
		error = copyin((caddr_t)(u_long)SCARG(uap, fromlenaddr),
			       (caddr_t)&msg.msg_namelen,
			       sizeof(msg.msg_namelen));
		if (error)
			return (error);
	} else
		msg.msg_namelen = 0;
	msg.msg_name = SCARG(uap, from);
	msg.msg_iov = NULL; /* We can't store a real pointer here */
	msg.msg_iovlen = 1;
	aiov.iov_base = (caddr_t)(u_long)SCARG(uap, buf);
	aiov.iov_len = (u_long)SCARG(uap, len);
	msg.msg_control = 0;
	msg.msg_flags = SCARG(uap, flags);
	return (recvit32(p, SCARG(uap, s), &msg, &aiov,
		       (caddr_t)(u_long)SCARG(uap, fromlenaddr), retval));
}

int
netbsd32_sendto(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_sendto_args /* {
		syscallarg(int) s;
		syscallarg(const netbsd32_voidp) buf;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) flags;
		syscallarg(const netbsd32_sockaddrp_t) to;
		syscallarg(int) tolen;
	} */ *uap = v;
	struct msghdr msg;
	struct iovec aiov;

	msg.msg_name = (caddr_t)(u_long)SCARG(uap, to);		/* XXX kills const */
	msg.msg_namelen = SCARG(uap, tolen);
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	msg.msg_control = 0;
#ifdef COMPAT_OLDSOCK
	msg.msg_flags = 0;
#endif
	aiov.iov_base = (char *)(u_long)SCARG(uap, buf);	/* XXX kills const */
	aiov.iov_len = SCARG(uap, len);
	return (sendit(p, SCARG(uap, s), &msg, SCARG(uap, flags), retval));
}

int
netbsd32_accept(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_accept_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_sockaddrp_t) name;
		syscallarg(netbsd32_intp) anamelen;
	} */ *uap = v;
	struct sys_accept_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(name, struct sockaddr);
	NETBSD32TOP_UAP(anamelen, int);
	return (sys_accept(p, &ua, retval));
}

int
netbsd32_getpeername(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_getpeername_args /* {
		syscallarg(int) fdes;
		syscallarg(netbsd32_sockaddrp_t) asa;
		syscallarg(netbsd32_intp) alen;
	} */ *uap = v;
	struct sys_getpeername_args ua;

	NETBSD32TO64_UAP(fdes);
	NETBSD32TOP_UAP(asa, struct sockaddr);
	NETBSD32TOP_UAP(alen, int);
/* NB: do the protocol specific sockaddrs need to be converted? */
	return (sys_getpeername(p, &ua, retval));
}

int
netbsd32_getsockname(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_getsockname_args /* {
		syscallarg(int) fdes;
		syscallarg(netbsd32_sockaddrp_t) asa;
		syscallarg(netbsd32_intp) alen;
	} */ *uap = v;
	struct sys_getsockname_args ua;

	NETBSD32TO64_UAP(fdes);
	NETBSD32TOP_UAP(asa, struct sockaddr);
	NETBSD32TOP_UAP(alen, int);
	return (sys_getsockname(p, &ua, retval));
}

int
netbsd32_access(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_access_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys_access_args ua;
	caddr_t sg;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);
	sg = stackgap_init(p->p_emul);
	CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));

	return (sys_access(p, &ua, retval));
}

int
netbsd32_chflags(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_chflags_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_u_long) flags;
	} */ *uap = v;
	struct sys_chflags_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);

	return (sys_chflags(p, &ua, retval));
}

int
netbsd32_fchflags(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_fchflags_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_u_long) flags;
	} */ *uap = v;
	struct sys_fchflags_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(flags);

	return (sys_fchflags(p, &ua, retval));
}

int
netbsd32_kill(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_kill_args /* {
		syscallarg(int) pid;
		syscallarg(int) signum;
	} */ *uap = v;
	struct sys_kill_args ua;

	NETBSD32TO64_UAP(pid);
	NETBSD32TO64_UAP(signum);

	return (sys_kill(p, &ua, retval));
}

int
netbsd32_dup(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_dup_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct sys_dup_args ua;

	NETBSD32TO64_UAP(fd);

	return (sys_dup(p, &ua, retval));
}

int
netbsd32_profil(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_profil_args /* {
		syscallarg(netbsd32_caddr_t) samples;
		syscallarg(netbsd32_size_t) size;
		syscallarg(netbsd32_u_long) offset;
		syscallarg(u_int) scale;
	} */ *uap = v;
	struct sys_profil_args ua;

	NETBSD32TOX64_UAP(samples, caddr_t);
	NETBSD32TOX_UAP(size, size_t);
	NETBSD32TOX_UAP(offset, u_long);
	NETBSD32TO64_UAP(scale);
	return (sys_profil(p, &ua, retval));
}

#ifdef KTRACE
int
netbsd32_ktrace(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_ktrace_args /* {
		syscallarg(const netbsd32_charp) fname;
		syscallarg(int) ops;
		syscallarg(int) facs;
		syscallarg(int) pid;
	} */ *uap = v;
	struct sys_ktrace_args ua;

	NETBSD32TOP_UAP(fname, const char);
	NETBSD32TO64_UAP(ops);
	NETBSD32TO64_UAP(facs);
	NETBSD32TO64_UAP(pid);
	return (sys_ktrace(p, &ua, retval));
}
#endif /* KTRACE */

int
netbsd32_sigaction(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_sigaction_args /* {
		syscallarg(int) signum;
		syscallarg(const netbsd32_sigactionp_t) nsa;
		syscallarg(netbsd32_sigactionp_t) osa;
	} */ *uap = v;
	struct sigaction nsa, osa;
	struct netbsd32_sigaction *sa32p, sa32;
	int error;

	if (SCARG(uap, nsa)) {
		sa32p = (struct netbsd32_sigaction *)(u_long)SCARG(uap, nsa);
		if (copyin(sa32p, &sa32, sizeof(sa32)))
			return EFAULT;
		nsa.sa_handler = (void *)(u_long)sa32.sa_handler;
		nsa.sa_mask = sa32.sa_mask;
		nsa.sa_flags = sa32.sa_flags;
	}
	error = sigaction1(p, SCARG(uap, signum), 
			   SCARG(uap, nsa) ? &nsa : 0, 
			   SCARG(uap, osa) ? &osa : 0);
 
	if (error)
		return (error);

	if (SCARG(uap, osa)) {
		sa32.sa_handler = (netbsd32_sigactionp_t)(u_long)osa.sa_handler;
		sa32.sa_mask = osa.sa_mask;
		sa32.sa_flags = osa.sa_flags;
		sa32p = (struct netbsd32_sigaction *)(u_long)SCARG(uap, osa);
		if (copyout(&sa32, sa32p, sizeof(sa32)))
			return EFAULT;
	}

	return (0);
}

int
netbsd32___getlogin(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32___getlogin_args /* {
		syscallarg(netbsd32_charp) namebuf;
		syscallarg(u_int) namelen;
	} */ *uap = v;
	struct sys___getlogin_args ua;

	NETBSD32TOP_UAP(namebuf, char);
	NETBSD32TO64_UAP(namelen);
	return (sys___getlogin(p, &ua, retval));
}

int
netbsd32_setlogin(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_setlogin_args /* {
		syscallarg(const netbsd32_charp) namebuf;
	} */ *uap = v;
	struct sys_setlogin_args ua;

	NETBSD32TOP_UAP(namebuf, char);
	return (sys_setlogin(p, &ua, retval));
}

int
netbsd32_acct(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_acct_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_acct_args ua;

	NETBSD32TOP_UAP(path, const char);
	return (sys_acct(p, &ua, retval));
}

int
netbsd32_revoke(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_revoke_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_revoke_args ua;
	caddr_t sg;

	NETBSD32TOP_UAP(path, const char);
	sg = stackgap_init(p->p_emul);
	CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));

	return (sys_revoke(p, &ua, retval));
}

int
netbsd32_symlink(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_symlink_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_charp) link;
	} */ *uap = v;
	struct sys_symlink_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(link, const char);

	return (sys_symlink(p, &ua, retval));
}

int
netbsd32_readlink(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_readlink_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_charp) buf;
		syscallarg(netbsd32_size_t) count;
	} */ *uap = v;
	struct sys_readlink_args ua;
	caddr_t sg;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(buf, char);
	NETBSD32TOX_UAP(count, size_t);
	sg = stackgap_init(p->p_emul);
	CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));

	return (sys_readlink(p, &ua, retval));
}

/* 
 * Need to completly reimplement this syscall due to argument copying.
 */
/* ARGSUSED */
int
netbsd32_execve(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_execve_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_charpp) argp;
		syscallarg(netbsd32_charpp) envp;
	} */ *uap = v;
	struct sys_execve_args ua;
	caddr_t sg;
	/* Function args */
	int error, i;
	struct exec_package pack;
	struct nameidata nid;
	struct vattr attr;
	struct ucred *cred = p->p_ucred;
	char *argp;
	netbsd32_charp const *cpp;
	char *dp;
	netbsd32_charp sp;
	long argc, envc;
	size_t len;
	char *stack;
	struct ps_strings arginfo;
	struct vmspace *vm;
	char **tmpfap;
	int szsigcode;
	struct exec_vmcmd *base_vcp = NULL;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(argp, char *);
	NETBSD32TOP_UAP(envp, char *);
	sg = stackgap_init(p->p_emul);
	CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));

	/* init the namei data to point the file user's program name */
	/* XXX cgd 960926: why do this here?  most will be clobbered. */
	NDINIT(&nid, LOOKUP, NOFOLLOW, UIO_USERSPACE, SCARG(&ua, path), p);

	/*
	 * initialize the fields of the exec package.
	 */
	pack.ep_name = SCARG(&ua, path);
	pack.ep_hdr = malloc(exec_maxhdrsz, M_EXEC, M_WAITOK);
	pack.ep_hdrlen = exec_maxhdrsz;
	pack.ep_hdrvalid = 0;
	pack.ep_ndp = &nid;
	pack.ep_emul_arg = NULL;
	pack.ep_vmcmds.evs_cnt = 0;
	pack.ep_vmcmds.evs_used = 0;
	pack.ep_vap = &attr;
	pack.ep_flags = 0;

	lockmgr(&exec_lock, LK_SHARED, NULL);

	/* see if we can run it. */
	if ((error = check_exec(p, &pack)) != 0)
		goto freehdr;

	/* XXX -- THE FOLLOWING SECTION NEEDS MAJOR CLEANUP */

	/* allocate an argument buffer */
	argp = (char *) uvm_km_valloc_wait(exec_map, NCARGS);
#ifdef DIAGNOSTIC
	if (argp == (vaddr_t) 0)
		panic("execve: argp == NULL");
#endif
	dp = argp;
	argc = 0;

	/* copy the fake args list, if there's one, freeing it as we go */
	if (pack.ep_flags & EXEC_HASARGL) {
		tmpfap = pack.ep_fa;
		while (*tmpfap != NULL) {
			char *cp;

			cp = *tmpfap;
			while (*cp)
				*dp++ = *cp++;
			dp++;

			FREE(*tmpfap, M_EXEC);
			tmpfap++; argc++;
		}
		FREE(pack.ep_fa, M_EXEC);
		pack.ep_flags &= ~EXEC_HASARGL;
	}

	/* Now get argv & environment */
	if (!(cpp = (netbsd32_charp *)SCARG(&ua, argp))) {
		error = EINVAL;
		goto bad;
	}

	if (pack.ep_flags & EXEC_SKIPARG)
		cpp++;

	while (1) {
		len = argp + ARG_MAX - dp;
		if ((error = copyin(cpp, &sp, sizeof(sp))) != 0)
			goto bad;
		if (!sp)
			break;
		if ((error = copyinstr((char *)(u_long)sp, dp, 
				       len, &len)) != 0) {
			if (error == ENAMETOOLONG)
				error = E2BIG;
			goto bad;
		}
		dp += len;
		cpp++;
		argc++;
	}

	envc = 0;
	/* environment need not be there */
	if ((cpp = (netbsd32_charp *)SCARG(&ua, envp)) != NULL ) {
		while (1) {
			len = argp + ARG_MAX - dp;
			if ((error = copyin(cpp, &sp, sizeof(sp))) != 0)
				goto bad;
			if (!sp)
				break;
			if ((error = copyinstr((char *)(u_long)sp, 
					       dp, len, &len)) != 0) {
				if (error == ENAMETOOLONG)
					error = E2BIG;
				goto bad;
			}
			dp += len;
			cpp++;
			envc++;
		}
	}

	dp = (char *) ALIGN(dp);

	szsigcode = pack.ep_es->es_emul->e_esigcode -
	    pack.ep_es->es_emul->e_sigcode;

	/* Now check if args & environ fit into new stack */
	if (pack.ep_flags & EXEC_32)
		len = ((argc + envc + 2 + pack.ep_es->es_arglen) *
		    sizeof(int) + sizeof(int) + dp + STACKGAPLEN +
		    szsigcode + sizeof(struct ps_strings)) - argp;
	else
		len = ((argc + envc + 2 + pack.ep_es->es_arglen) *
		    sizeof(char *) + sizeof(int) + dp + STACKGAPLEN +
		    szsigcode + sizeof(struct ps_strings)) - argp;

	len = ALIGN(len);	/* make the stack "safely" aligned */

	if (len > pack.ep_ssize) { /* in effect, compare to initial limit */
		error = ENOMEM;
		goto bad;
	}

	/* adjust "active stack depth" for process VSZ */
	pack.ep_ssize = len;	/* maybe should go elsewhere, but... */

	/*
	 * Do whatever is necessary to prepare the address space
	 * for remapping.  Note that this might replace the current
	 * vmspace with another!
	 */
	uvmspace_exec(p);

	/* Now map address space */
	vm = p->p_vmspace;
	vm->vm_taddr = (char *) pack.ep_taddr;
	vm->vm_tsize = btoc(pack.ep_tsize);
	vm->vm_daddr = (char *) pack.ep_daddr;
	vm->vm_dsize = btoc(pack.ep_dsize);
	vm->vm_ssize = btoc(pack.ep_ssize);
	vm->vm_maxsaddr = (char *) pack.ep_maxsaddr;
	vm->vm_minsaddr = (char *) pack.ep_minsaddr;

	/* create the new process's VM space by running the vmcmds */
#ifdef DIAGNOSTIC
	if (pack.ep_vmcmds.evs_used == 0)
		panic("execve: no vmcmds");
#endif
	for (i = 0; i < pack.ep_vmcmds.evs_used && !error; i++) {
		struct exec_vmcmd *vcp;

		vcp = &pack.ep_vmcmds.evs_cmds[i];
		if (vcp->ev_flags & VMCMD_RELATIVE) {
#ifdef DIAGNOSTIC
			if (base_vcp == NULL)
				panic("execve: relative vmcmd with no base");
			if (vcp->ev_flags & VMCMD_BASE)
				panic("execve: illegal base & relative vmcmd");
#endif
			vcp->ev_addr += base_vcp->ev_addr;
		}
		error = (*vcp->ev_proc)(p, vcp);
#ifdef DEBUG
		if (error) {
			if (i > 0)
				printf("vmcmd[%d] = %#lx/%#lx @ %#lx\n", i-1,
				       vcp[-1].ev_addr, vcp[-1].ev_len,
				       vcp[-1].ev_offset);
			printf("vmcmd[%d] = %#lx/%#lx @ %#lx\n", i,
			       vcp->ev_addr, vcp->ev_len, vcp->ev_offset);
		}
#endif
		if (vcp->ev_flags & VMCMD_BASE)
			base_vcp = vcp;
	}

	/* free the vmspace-creation commands, and release their references */
	kill_vmcmds(&pack.ep_vmcmds);

	/* if an error happened, deallocate and punt */
	if (error) {
#ifdef DEBUG
		printf("execve: vmcmd %i failed: %d\n", i-1, error);
#endif
		goto exec_abort;
	}

	/* remember information about the process */
	arginfo.ps_nargvstr = argc;
	arginfo.ps_nenvstr = envc;

	stack = (char *) (vm->vm_minsaddr - len);
	/* Now copy argc, args & environ to new stack */
	if (!(*pack.ep_es->es_copyargs)(&pack, &arginfo, stack, argp)) {
#ifdef DEBUG
		printf("execve: copyargs failed\n");
#endif
		goto exec_abort;
	}

	/* fill process ps_strings info */
	p->p_psstr = (struct ps_strings *)(stack - sizeof(struct ps_strings));
	p->p_psargv = offsetof(struct ps_strings, ps_argvstr);
	p->p_psnargv = offsetof(struct ps_strings, ps_nargvstr);
	p->p_psenv = offsetof(struct ps_strings, ps_envstr);
	p->p_psnenv = offsetof(struct ps_strings, ps_nenvstr);

	/* copy out the process's ps_strings structure */
	if (copyout(&arginfo, (char *)p->p_psstr, sizeof(arginfo))) {
#ifdef DEBUG
		printf("execve: ps_strings copyout failed\n");
#endif
		goto exec_abort;
	}

	/* copy out the process's signal trapoline code */
	if (szsigcode) {
		if (copyout((char *)pack.ep_es->es_emul->e_sigcode,
		    p->p_sigctx.ps_sigcode = (char *)p->p_psstr - szsigcode,
		    szsigcode)) {
#ifdef DEBUG
			printf("execve: sig trampoline copyout failed\n");
#endif
			goto exec_abort;
		}
#ifdef PMAP_NEED_PROCWR
		/* This is code. Let the pmap do what is needed. */
		pmap_procwr(p, (vaddr_t)p->p_sigacts->ps_sigcode, szsigcode);
#endif
	}

	stopprofclock(p);	/* stop profiling */
	fdcloseexec(p);		/* handle close on exec */
	execsigs(p);		/* reset catched signals */
	p->p_ctxlink = NULL;	/* reset ucontext link */

	/* set command name & other accounting info */
	len = min(nid.ni_cnd.cn_namelen, MAXCOMLEN);
	memcpy(p->p_comm, nid.ni_cnd.cn_nameptr, len);
	p->p_comm[len] = 0;
	p->p_acflag &= ~AFORK;

	/* record proc's vnode, for use by procfs and others */
        if (p->p_textvp)
                vrele(p->p_textvp);
	VREF(pack.ep_vp);
	p->p_textvp = pack.ep_vp;

	p->p_flag |= P_EXEC;
	if (p->p_flag & P_PPWAIT) {
		p->p_flag &= ~P_PPWAIT;
		wakeup((caddr_t) p->p_pptr);
	}

	/*
	 * deal with set[ug]id.
	 * MNT_NOSUID and P_TRACED have already been used to disable s[ug]id.
	 */
	if (((attr.va_mode & S_ISUID) != 0 && p->p_ucred->cr_uid != attr.va_uid)
	 || ((attr.va_mode & S_ISGID) != 0 && p->p_ucred->cr_gid != attr.va_gid)){
		p->p_ucred = crcopy(cred);
#ifdef KTRACE
		/*
		 * If process is being ktraced, turn off - unless
		 * root set it.
		 */
		if (p->p_tracep && !(p->p_traceflag & KTRFAC_ROOT))
			ktrderef(p);
#endif
		if (attr.va_mode & S_ISUID)
			p->p_ucred->cr_uid = attr.va_uid;
		if (attr.va_mode & S_ISGID)
			p->p_ucred->cr_gid = attr.va_gid;
		p_sugid(p);
	} else
		p->p_flag &= ~P_SUGID;
	p->p_cred->p_svuid = p->p_ucred->cr_uid;
	p->p_cred->p_svgid = p->p_ucred->cr_gid;

	doexechooks(p);

	uvm_km_free_wakeup(exec_map, (vaddr_t) argp, NCARGS);

	PNBUF_PUT(nid.ni_cnd.cn_pnbuf);
	vn_lock(pack.ep_vp, LK_EXCLUSIVE | LK_RETRY);
	VOP_CLOSE(pack.ep_vp, FREAD, cred, p);
	vput(pack.ep_vp);

	/* setup new registers and do misc. setup. */
	(*pack.ep_es->es_setregs)(p, &pack, (u_long) stack);

	if (p->p_flag & P_TRACED)
		psignal(p, SIGTRAP);

	free(pack.ep_hdr, M_EXEC);

	/*
	 * Call emulation specific exec hook. This can setup setup per-process
	 * p->p_emuldata or do any other per-process stuff an emulation needs.
	 *
	 * If we are executing process of different emulation than the
	 * original forked process, call e_proc_exit() of the old emulation
	 * first, then e_proc_exec() of new emulation. If the emulation is
	 * same, the exec hook code should deallocate any old emulation
	 * resources held previously by this process.
	 */
	if (p->p_emul && p->p_emul->e_proc_exit
	    && p->p_emul != pack.ep_es->es_emul)
		(*p->p_emul->e_proc_exit)(p);

	/*
	 * Call exec hook. Emulation code may NOT store reference to anything
	 * from &pack.
	 */
        if (pack.ep_es->es_emul->e_proc_exec)
                (*pack.ep_es->es_emul->e_proc_exec)(p, &pack);

	/* update p_emul, the old value is no longer needed */
	p->p_emul = pack.ep_es->es_emul;

#ifdef KTRACE
	if (KTRPOINT(p, KTR_EMUL))
		ktremul(p);
#endif

	lockmgr(&exec_lock, LK_RELEASE, NULL);

	return (EJUSTRETURN);

bad:
	/* free the vmspace-creation commands, and release their references */
	kill_vmcmds(&pack.ep_vmcmds);
	/* kill any opened file descriptor, if necessary */
	if (pack.ep_flags & EXEC_HASFD) {
		pack.ep_flags &= ~EXEC_HASFD;
		(void) fdrelease(p, pack.ep_fd);
	}
	/* close and put the exec'd file */
	vn_lock(pack.ep_vp, LK_EXCLUSIVE | LK_RETRY);
	VOP_CLOSE(pack.ep_vp, FREAD, cred, p);
	vput(pack.ep_vp);
	PNBUF_PUT(nid.ni_cnd.cn_pnbuf);
	uvm_km_free_wakeup(exec_map, (vaddr_t) argp, NCARGS);

freehdr:
	lockmgr(&exec_lock, LK_RELEASE, NULL);

	free(pack.ep_hdr, M_EXEC);
	return error;

exec_abort:
	lockmgr(&exec_lock, LK_RELEASE, NULL);

	/*
	 * the old process doesn't exist anymore.  exit gracefully.
	 * get rid of the (new) address space we have created, if any, get rid
	 * of our namei data and vnode, and exit noting failure
	 */
	uvm_deallocate(&vm->vm_map, VM_MIN_ADDRESS,
		VM_MAXUSER_ADDRESS - VM_MIN_ADDRESS);
	if (pack.ep_emul_arg)
		FREE(pack.ep_emul_arg, M_TEMP);
	PNBUF_PUT(nid.ni_cnd.cn_pnbuf);
	vn_lock(pack.ep_vp, LK_EXCLUSIVE | LK_RETRY);
	VOP_CLOSE(pack.ep_vp, FREAD, cred, p);
	vput(pack.ep_vp);
	uvm_km_free_wakeup(exec_map, (vaddr_t) argp, NCARGS);
	free(pack.ep_hdr, M_EXEC);
	exit1(p, W_EXITCODE(0, SIGABRT));
	exit1(p, -1);

	/* NOTREACHED */
	return 0;
}

int
netbsd32_umask(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_umask_args /* {
		syscallarg(mode_t) newmask;
	} */ *uap = v;
	struct sys_umask_args ua;

	NETBSD32TO64_UAP(newmask);
	return (sys_umask(p, &ua, retval));
}

int
netbsd32_chroot(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_chroot_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_chroot_args ua;

	NETBSD32TOP_UAP(path, const char);
	return (sys_chroot(p, &ua, retval));
}

int
netbsd32_sbrk(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_sbrk_args /* {
		syscallarg(int) incr;
	} */ *uap = v;
	struct sys_sbrk_args ua;

	NETBSD32TO64_UAP(incr);
	return (sys_sbrk(p, &ua, retval));
}

int
netbsd32_sstk(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_sstk_args /* {
		syscallarg(int) incr;
	} */ *uap = v;
	struct sys_sstk_args ua;

	NETBSD32TO64_UAP(incr);
	return (sys_sstk(p, &ua, retval));
}

int
netbsd32_munmap(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_munmap_args /* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
	} */ *uap = v;
	struct sys_munmap_args ua;

	NETBSD32TOP_UAP(addr, void);
	NETBSD32TOX_UAP(len, size_t);
	return (sys_munmap(p, &ua, retval));
}

int
netbsd32_mprotect(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_mprotect_args /* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) prot;
	} */ *uap = v;
	struct sys_mprotect_args ua;

	NETBSD32TOP_UAP(addr, void);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TO64_UAP(prot);
	return (sys_mprotect(p, &ua, retval));
}

int
netbsd32_madvise(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_madvise_args /* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) behav;
	} */ *uap = v;
	struct sys_madvise_args ua;

	NETBSD32TOP_UAP(addr, void);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TO64_UAP(behav);
	return (sys_madvise(p, &ua, retval));
}

int
netbsd32_mincore(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_mincore_args /* {
		syscallarg(netbsd32_caddr_t) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(netbsd32_charp) vec;
	} */ *uap = v;
	struct sys_mincore_args ua;

	NETBSD32TOX64_UAP(addr, caddr_t);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TOP_UAP(vec, char);
	return (sys_mincore(p, &ua, retval));
}

int
netbsd32_getgroups(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_getgroups_args /* {
		syscallarg(int) gidsetsize;
		syscallarg(netbsd32_gid_tp) gidset;
	} */ *uap = v;
	struct pcred *pc = p->p_cred;
	int ngrp;
	int error;

	ngrp = SCARG(uap, gidsetsize);
	if (ngrp == 0) {
		*retval = pc->pc_ucred->cr_ngroups;
		return (0);
	}
	if (ngrp < pc->pc_ucred->cr_ngroups)
		return (EINVAL);
	ngrp = pc->pc_ucred->cr_ngroups;
	/* Should convert gid_t to netbsd32_gid_t, but they're the same */
	error = copyout((caddr_t)pc->pc_ucred->cr_groups,
			(caddr_t)(u_long)SCARG(uap, gidset), 
			ngrp * sizeof(gid_t));
	if (error)
		return (error);
	*retval = ngrp;
	return (0);
}

int
netbsd32_setgroups(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_setgroups_args /* {
		syscallarg(int) gidsetsize;
		syscallarg(const netbsd32_gid_tp) gidset;
	} */ *uap = v;
	struct sys_setgroups_args ua;

	NETBSD32TO64_UAP(gidsetsize);
	NETBSD32TOP_UAP(gidset, gid_t);
	return (sys_setgroups(p, &ua, retval));
}

int
netbsd32_setpgid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_setpgid_args /* {
		syscallarg(int) pid;
		syscallarg(int) pgid;
	} */ *uap = v;
	struct sys_setpgid_args ua;

	NETBSD32TO64_UAP(pid);
	NETBSD32TO64_UAP(pgid);
	return (sys_setpgid(p, &ua, retval));
}

int
netbsd32_setitimer(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_setitimer_args /* {
		syscallarg(int) which;
		syscallarg(const netbsd32_itimervalp_t) itv;
		syscallarg(netbsd32_itimervalp_t) oitv;
	} */ *uap = v;
	struct netbsd32_itimerval s32it, *itvp;
	int which = SCARG(uap, which);
	struct netbsd32_getitimer_args getargs;
	struct itimerval aitv;
	int s, error;

	if ((u_int)which > ITIMER_PROF)
		return (EINVAL);
	itvp = (struct netbsd32_itimerval *)(u_long)SCARG(uap, itv);
	if (itvp && (error = copyin(itvp, &s32it, sizeof(s32it))))
		return (error);
	netbsd32_to_itimerval(&s32it, &aitv);
	if (SCARG(uap, oitv) != NULL) {
		SCARG(&getargs, which) = which;
		SCARG(&getargs, itv) = SCARG(uap, oitv);
		if ((error = netbsd32_getitimer(p, &getargs, retval)) != 0)
			return (error);
	}
	if (itvp == 0)
		return (0);
	if (itimerfix(&aitv.it_value) || itimerfix(&aitv.it_interval))
		return (EINVAL);
	s = splclock();
	if (which == ITIMER_REAL) {
		callout_stop(&p->p_realit_ch);
		if (timerisset(&aitv.it_value)) {
			/*
			 * Don't need to check hzto() return value, here.
			 * callout_reset() does it for us.
			 */
			timeradd(&aitv.it_value, &time, &aitv.it_value);
			callout_reset(&p->p_realit_ch, hzto(&aitv.it_value),
			    realitexpire, p);
		}
		p->p_realtimer = aitv;
	} else
		p->p_stats->p_timer[which] = aitv;
	splx(s);
	return (0);
}

int
netbsd32_getitimer(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_getitimer_args /* {
		syscallarg(int) which;
		syscallarg(netbsd32_itimervalp_t) itv;
	} */ *uap = v;
	int which = SCARG(uap, which);
	struct netbsd32_itimerval s32it;
	struct itimerval aitv;
	int s;

	if ((u_int)which > ITIMER_PROF)
		return (EINVAL);
	s = splclock();
	if (which == ITIMER_REAL) {
		/*
		 * Convert from absolute to relative time in .it_value
		 * part of real time timer.  If time for real time timer
		 * has passed return 0, else return difference between
		 * current time and time for the timer to go off.
		 */
		aitv = p->p_realtimer;
		if (timerisset(&aitv.it_value)) {
			if (timercmp(&aitv.it_value, &time, <))
				timerclear(&aitv.it_value);
			else
				timersub(&aitv.it_value, &time, &aitv.it_value);
		}
	} else
		aitv = p->p_stats->p_timer[which];
	splx(s);
	netbsd32_from_itimerval(&aitv, &s32it);
	return (copyout(&s32it, (caddr_t)(u_long)SCARG(uap, itv), sizeof(s32it)));
}

int
netbsd32_fcntl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_fcntl_args /* {
		syscallarg(int) fd;
		syscallarg(int) cmd;
		syscallarg(netbsd32_voidp) arg;
	} */ *uap = v;
	struct sys_fcntl_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(cmd);
	NETBSD32TOP_UAP(arg, void);
	/* XXXX we can do this 'cause flock doesn't change */
	return (sys_fcntl(p, &ua, retval));
}

int
netbsd32_dup2(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_dup2_args /* {
		syscallarg(int) from;
		syscallarg(int) to;
	} */ *uap = v;
	struct sys_dup2_args ua;

	NETBSD32TO64_UAP(from);
	NETBSD32TO64_UAP(to);
	return (sys_dup2(p, &ua, retval));
}

int
netbsd32_select(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_select_args /* {
		syscallarg(int) nd;
		syscallarg(netbsd32_fd_setp_t) in;
		syscallarg(netbsd32_fd_setp_t) ou;
		syscallarg(netbsd32_fd_setp_t) ex;
		syscallarg(netbsd32_timevalp_t) tv;
	} */ *uap = v;
/* This one must be done in-line 'cause of the timeval */
	struct netbsd32_timeval tv32;
	caddr_t bits;
	char smallbits[howmany(FD_SETSIZE, NFDBITS) * sizeof(fd_mask) * 6];
	struct timeval atv;
	int s, ncoll, error = 0, timo;
	size_t ni;
	extern int	selwait, nselcoll;
	extern int selscan __P((struct proc *, fd_mask *, fd_mask *, int, register_t *));

	if (SCARG(uap, nd) < 0)
		return (EINVAL);
	if (SCARG(uap, nd) > p->p_fd->fd_nfiles) {
		/* forgiving; slightly wrong */
		SCARG(uap, nd) = p->p_fd->fd_nfiles;
	}
	ni = howmany(SCARG(uap, nd), NFDBITS) * sizeof(fd_mask);
	if (ni * 6 > sizeof(smallbits))
		bits = malloc(ni * 6, M_TEMP, M_WAITOK);
	else
		bits = smallbits;

#define	getbits(name, x) \
	if (SCARG(uap, name)) { \
		error = copyin((caddr_t)(u_long)SCARG(uap, name), bits + ni * x, ni); \
		if (error) \
			goto done; \
	} else \
		memset(bits + ni * x, 0, ni);
	getbits(in, 0);
	getbits(ou, 1);
	getbits(ex, 2);
#undef	getbits

	if (SCARG(uap, tv)) {
		error = copyin((caddr_t)(u_long)SCARG(uap, tv), (caddr_t)&tv32,
			sizeof(tv32));
		if (error)
			goto done;
		netbsd32_to_timeval(&tv32, &atv);
		if (itimerfix(&atv)) {
			error = EINVAL;
			goto done;
		}
		s = splclock();
		timeradd(&atv, &time, &atv);
		splx(s);
	} else
		timo = 0;
retry:
	ncoll = nselcoll;
	p->p_flag |= P_SELECT;
	error = selscan(p, (fd_mask *)(bits + ni * 0),
			   (fd_mask *)(bits + ni * 3), SCARG(uap, nd), retval);
	if (error || *retval)
		goto done;
	if (SCARG(uap, tv)) {
		/*
		 * We have to recalculate the timeout on every retry.
		 */
		timo = hzto(&atv);
		if (timo <= 0)
			goto done;
	}
	s = splhigh();
	if ((p->p_flag & P_SELECT) == 0 || nselcoll != ncoll) {
		splx(s);
		goto retry;
	}
	p->p_flag &= ~P_SELECT;
	error = tsleep((caddr_t)&selwait, PSOCK | PCATCH, "select", timo);
	splx(s);
	if (error == 0)
		goto retry;
done:
	p->p_flag &= ~P_SELECT;
	/* select is not restarted after signals... */
	if (error == ERESTART)
		error = EINTR;
	if (error == EWOULDBLOCK)
		error = 0;
	if (error == 0) {
#define	putbits(name, x) \
		if (SCARG(uap, name)) { \
			error = copyout(bits + ni * x, (caddr_t)(u_long)SCARG(uap, name), ni); \
			if (error) \
				goto out; \
		}
		putbits(in, 3);
		putbits(ou, 4);
		putbits(ex, 5);
#undef putbits
	}
out:
	if (ni * 6 > sizeof(smallbits))
		free(bits, M_TEMP);
	return (error);
}

int
netbsd32_fsync(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_fsync_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct sys_fsync_args ua;

	NETBSD32TO64_UAP(fd);
	return (sys_fsync(p, &ua, retval));
}

int
netbsd32_setpriority(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_setpriority_args /* {
		syscallarg(int) which;
		syscallarg(int) who;
		syscallarg(int) prio;
	} */ *uap = v;
	struct sys_setpriority_args ua;

	NETBSD32TO64_UAP(which);
	NETBSD32TO64_UAP(who);
	NETBSD32TO64_UAP(prio);
	return (sys_setpriority(p, &ua, retval));
}

int
netbsd32_socket(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_socket_args /* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
	} */ *uap = v;
	struct sys_socket_args ua;

	NETBSD32TO64_UAP(domain);
	NETBSD32TO64_UAP(type);
	NETBSD32TO64_UAP(protocol);
	return (sys_socket(p, &ua, retval));
}

int
netbsd32_connect(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_connect_args /* {
		syscallarg(int) s;
		syscallarg(const netbsd32_sockaddrp_t) name;
		syscallarg(int) namelen;
	} */ *uap = v;
	struct sys_connect_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(name, struct sockaddr);
	NETBSD32TO64_UAP(namelen);
	return (sys_connect(p, &ua, retval));
}

int
netbsd32_getpriority(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_getpriority_args /* {
		syscallarg(int) which;
		syscallarg(int) who;
	} */ *uap = v;
	struct sys_getpriority_args ua;

	NETBSD32TO64_UAP(which);
	NETBSD32TO64_UAP(who);
	return (sys_getpriority(p, &ua, retval));
}

int
netbsd32_bind(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_bind_args /* {
		syscallarg(int) s;
		syscallarg(const netbsd32_sockaddrp_t) name;
		syscallarg(int) namelen;
	} */ *uap = v;
	struct sys_bind_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(name, struct sockaddr);
	NETBSD32TO64_UAP(namelen);
	return (sys_bind(p, &ua, retval));
}

int
netbsd32_setsockopt(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_setsockopt_args /* {
		syscallarg(int) s;
		syscallarg(int) level;
		syscallarg(int) name;
		syscallarg(const netbsd32_voidp) val;
		syscallarg(int) valsize;
	} */ *uap = v;
	struct sys_setsockopt_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TO64_UAP(level);
	NETBSD32TO64_UAP(name);
	NETBSD32TOP_UAP(val, void);
	NETBSD32TO64_UAP(valsize);
	/* may be more efficient to do this inline. */
	return (sys_setsockopt(p, &ua, retval));
}

int
netbsd32_listen(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_listen_args /* {
		syscallarg(int) s;
		syscallarg(int) backlog;
	} */ *uap = v;
	struct sys_listen_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TO64_UAP(backlog);
	return (sys_listen(p, &ua, retval));
}

int
netbsd32_gettimeofday(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_gettimeofday_args /* {
		syscallarg(netbsd32_timevalp_t) tp;
		syscallarg(netbsd32_timezonep_t) tzp;
	} */ *uap = v;
	struct timeval atv;
	struct netbsd32_timeval tv32;
	int error = 0;
	struct netbsd32_timezone tzfake;

	if (SCARG(uap, tp)) {
		microtime(&atv);
		netbsd32_from_timeval(&atv, &tv32);
		error = copyout(&tv32, (caddr_t)(u_long)SCARG(uap, tp), sizeof(tv32));
		if (error)
			return (error);
	}
	if (SCARG(uap, tzp)) {
		/*
		 * NetBSD has no kernel notion of time zone, so we just
		 * fake up a timezone struct and return it if demanded.
		 */
		tzfake.tz_minuteswest = 0;
		tzfake.tz_dsttime = 0;
		error = copyout(&tzfake, (caddr_t)(u_long)SCARG(uap, tzp), sizeof(tzfake));
	}
	return (error);
}

#if 0
static int settime32 __P((struct timeval *));
/* This function is used by clock_settime and settimeofday */
static int
settime32(tv)
	struct timeval *tv;
{
	struct timeval delta;
	int s;

	/* WHAT DO WE DO ABOUT PENDING REAL-TIME TIMEOUTS??? */
	s = splclock();
	timersub(tv, &time, &delta);
	if ((delta.tv_sec < 0 || delta.tv_usec < 0) && securelevel > 1)
		return (EPERM);
#ifdef notyet
	if ((delta.tv_sec < 86400) && securelevel > 0)
		return (EPERM);
#endif
	time = *tv;
	(void) spllowersoftclock();
	timeradd(&boottime, &delta, &boottime);
	timeradd(&runtime, &delta, &runtime);
#	if defined(NFS) || defined(NFSSERVER)
	{
		extern void	nqnfs_lease_updatetime __P((int));

		nqnfs_lease_updatetime(delta.tv_sec);
	}
#	endif
	splx(s);
	resettodr();
	return (0);
}
#endif

int
netbsd32_settimeofday(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_settimeofday_args /* {
		syscallarg(const netbsd32_timevalp_t) tv;
		syscallarg(const netbsd32_timezonep_t) tzp;
	} */ *uap = v;
	struct netbsd32_timeval atv32;
	struct timeval atv;
	struct netbsd32_timezone atz;
	int error;

	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
		return (error);
	/* Verify all parameters before changing time. */
	if (SCARG(uap, tv) && (error = copyin((caddr_t)(u_long)SCARG(uap, tv),
	    &atv32, sizeof(atv32))))
		return (error);
	netbsd32_to_timeval(&atv32, &atv);
	/* XXX since we don't use tz, probably no point in doing copyin. */
	if (SCARG(uap, tzp) && (error = copyin((caddr_t)(u_long)SCARG(uap, tzp),
	    &atz, sizeof(atz))))
		return (error);
	if (SCARG(uap, tv))
		if ((error = settime(&atv)))
			return (error);
	/*
	 * NetBSD has no kernel notion of time zone, and only an
	 * obsolete program would try to set it, so we log a warning.
	 */
	if (SCARG(uap, tzp))
		printf("pid %d attempted to set the "
		    "(obsolete) kernel time zone\n", p->p_pid); 
	return (0);
}

int
netbsd32_fchown(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_fchown_args /* {
		syscallarg(int) fd;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct sys_fchown_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(uid);
	NETBSD32TO64_UAP(gid);
	return (sys_fchown(p, &ua, retval));
}

int
netbsd32_fchmod(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_fchmod_args /* {
		syscallarg(int) fd;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct sys_fchmod_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(mode);
	return (sys_fchmod(p, &ua, retval));
}

int
netbsd32_setreuid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_setreuid_args /* {
		syscallarg(uid_t) ruid;
		syscallarg(uid_t) euid;
	} */ *uap = v;
	struct sys_setreuid_args ua;

	NETBSD32TO64_UAP(ruid);
	NETBSD32TO64_UAP(euid);
	return (sys_setreuid(p, &ua, retval));
}

int
netbsd32_setregid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_setregid_args /* {
		syscallarg(gid_t) rgid;
		syscallarg(gid_t) egid;
	} */ *uap = v;
	struct sys_setregid_args ua;

	NETBSD32TO64_UAP(rgid);
	NETBSD32TO64_UAP(egid);
	return (sys_setregid(p, &ua, retval));
}

int
netbsd32_getrusage(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_getrusage_args /* {
		syscallarg(int) who;
		syscallarg(netbsd32_rusagep_t) rusage;
	} */ *uap = v;
	struct rusage *rup;
	struct netbsd32_rusage ru;

	switch (SCARG(uap, who)) {

	case RUSAGE_SELF:
		rup = &p->p_stats->p_ru;
		calcru(p, &rup->ru_utime, &rup->ru_stime, NULL);
		break;

	case RUSAGE_CHILDREN:
		rup = &p->p_stats->p_cru;
		break;

	default:
		return (EINVAL);
	}
	netbsd32_from_rusage(rup, &ru);
	return (copyout(&ru, (caddr_t)(u_long)SCARG(uap, rusage), sizeof(ru)));
}

int
netbsd32_getsockopt(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_getsockopt_args /* {
		syscallarg(int) s;
		syscallarg(int) level;
		syscallarg(int) name;
		syscallarg(netbsd32_voidp) val;
		syscallarg(netbsd32_intp) avalsize;
	} */ *uap = v;
	struct sys_getsockopt_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TO64_UAP(level);
	NETBSD32TO64_UAP(name);
	NETBSD32TOP_UAP(val, void);
	NETBSD32TOP_UAP(avalsize, int);
	return (sys_getsockopt(p, &ua, retval));
}

int
netbsd32_readv(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_readv_args /* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_iovecp_t) iovp;
		syscallarg(int) iovcnt;
	} */ *uap = v;
	int fd = SCARG(uap, fd);
	struct file *fp;
	struct filedesc *fdp = p->p_fd;

	if ((u_int)fd >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[fd]) == NULL ||
	    (fp->f_flag & FREAD) == 0)
		return (EBADF);

	return (dofilereadv32(p, fd, fp, (struct netbsd32_iovec *)(u_long)SCARG(uap, iovp), 
			      SCARG(uap, iovcnt), &fp->f_offset, FOF_UPDATE_OFFSET, retval));
}

/* Damn thing copies in the iovec! */
int
dofilereadv32(p, fd, fp, iovp, iovcnt, offset, flags, retval)
	struct proc *p;
	int fd;
	struct file *fp;
	struct netbsd32_iovec *iovp;
	int iovcnt;
	off_t *offset;
	int flags;
	register_t *retval;
{
	struct uio auio;
	struct iovec *iov;
	struct iovec *needfree;
	struct iovec aiov[UIO_SMALLIOV];
	long i, cnt, error = 0;
	u_int iovlen;
#ifdef KTRACE
	struct iovec *ktriov = NULL;
#endif

	/* note: can't use iovlen until iovcnt is validated */
	iovlen = iovcnt * sizeof(struct iovec);
	if ((u_int)iovcnt > UIO_SMALLIOV) {
		if ((u_int)iovcnt > IOV_MAX)
			return (EINVAL);
		MALLOC(iov, struct iovec *, iovlen, M_IOV, M_WAITOK);
		needfree = iov;
	} else if ((u_int)iovcnt > 0) {
		iov = aiov;
		needfree = NULL;
	} else
		return (EINVAL);

	auio.uio_iov = iov;
	auio.uio_iovcnt = iovcnt;
	auio.uio_rw = UIO_READ;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_procp = p;
	error = netbsd32_to_iovecin(iovp, iov, iovcnt);
	if (error)
		goto done;
	auio.uio_resid = 0;
	for (i = 0; i < iovcnt; i++) {
		auio.uio_resid += iov->iov_len;
		/*
		 * Reads return ssize_t because -1 is returned on error.
		 * Therefore we must restrict the length to SSIZE_MAX to
		 * avoid garbage return values.
		 */
		if (iov->iov_len > SSIZE_MAX || auio.uio_resid > SSIZE_MAX) {
			error = EINVAL;
			goto done;
		}
		iov++;
	}
#ifdef KTRACE
	/*
	 * if tracing, save a copy of iovec
	 */
	if (KTRPOINT(p, KTR_GENIO))  {
		MALLOC(ktriov, struct iovec *, iovlen, M_TEMP, M_WAITOK);
		memcpy((caddr_t)ktriov, (caddr_t)auio.uio_iov, iovlen);
	}
#endif
	cnt = auio.uio_resid;
	error = (*fp->f_ops->fo_read)(fp, offset, &auio, fp->f_cred, flags);
	if (error)
		if (auio.uio_resid != cnt && (error == ERESTART ||
		    error == EINTR || error == EWOULDBLOCK))
			error = 0;
	cnt -= auio.uio_resid;
#ifdef KTRACE
	if (KTRPOINT(p, KTR_GENIO))
		if (error == 0) {
			ktrgenio(p, fd, UIO_READ, ktriov, cnt,
			    error);
		FREE(ktriov, M_TEMP);
	}
#endif
	*retval = cnt;
done:
	if (needfree)
		FREE(needfree, M_IOV);
	return (error);
}


int
netbsd32_writev(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_writev_args /* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_iovecp_t) iovp;
		syscallarg(int) iovcnt;
	} */ *uap = v;
	int fd = SCARG(uap, fd);
	struct file *fp;
	struct filedesc *fdp = p->p_fd;

	if ((u_int)fd >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[fd]) == NULL ||
	    (fp->f_flag & FWRITE) == 0)
		return (EBADF);

	return (dofilewritev32(p, fd, fp, (struct netbsd32_iovec *)(u_long)SCARG(uap, iovp),
			       SCARG(uap, iovcnt), &fp->f_offset, FOF_UPDATE_OFFSET, retval));
}

int
dofilewritev32(p, fd, fp, iovp, iovcnt, offset, flags, retval)
	struct proc *p;
	int fd;
	struct file *fp;
	struct netbsd32_iovec *iovp;
	int iovcnt;
	off_t *offset;
	int flags;
	register_t *retval;
{
	struct uio auio;
	struct iovec *iov;
	struct iovec *needfree;
	struct iovec aiov[UIO_SMALLIOV];
	long i, cnt, error = 0;
	u_int iovlen;
#ifdef KTRACE
	struct iovec *ktriov = NULL;
#endif

	/* note: can't use iovlen until iovcnt is validated */
	iovlen = iovcnt * sizeof(struct iovec);
	if ((u_int)iovcnt > UIO_SMALLIOV) {
		if ((u_int)iovcnt > IOV_MAX)
			return (EINVAL);
		MALLOC(iov, struct iovec *, iovlen, M_IOV, M_WAITOK);
		needfree = iov;
	} else if ((u_int)iovcnt > 0) {
		iov = aiov;
		needfree = NULL;
	} else
		return (EINVAL);

	auio.uio_iov = iov;
	auio.uio_iovcnt = iovcnt;
	auio.uio_rw = UIO_WRITE;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_procp = p;
	error = netbsd32_to_iovecin(iovp, iov, iovcnt);
	if (error)
		goto done;
	auio.uio_resid = 0;
	for (i = 0; i < iovcnt; i++) {
		auio.uio_resid += iov->iov_len;
		/*
		 * Writes return ssize_t because -1 is returned on error.
		 * Therefore we must restrict the length to SSIZE_MAX to
		 * avoid garbage return values.
		 */
		if (iov->iov_len > SSIZE_MAX || auio.uio_resid > SSIZE_MAX) {
			error = EINVAL;
			goto done;
		}
		iov++;
	}
#ifdef KTRACE
	/*
	 * if tracing, save a copy of iovec
	 */
	if (KTRPOINT(p, KTR_GENIO))  {
		MALLOC(ktriov, struct iovec *, iovlen, M_TEMP, M_WAITOK);
		memcpy((caddr_t)ktriov, (caddr_t)auio.uio_iov, iovlen);
	}
#endif
	cnt = auio.uio_resid;
	error = (*fp->f_ops->fo_write)(fp, offset, &auio, fp->f_cred, flags);
	if (error) {
		if (auio.uio_resid != cnt && (error == ERESTART ||
		    error == EINTR || error == EWOULDBLOCK))
			error = 0;
		if (error == EPIPE)
			psignal(p, SIGPIPE);
	}
	cnt -= auio.uio_resid;
#ifdef KTRACE
	if (KTRPOINT(p, KTR_GENIO))
		if (error == 0) {
			ktrgenio(p, fd, UIO_WRITE, ktriov, cnt,
			    error);
		FREE(ktriov, M_TEMP);
	}
#endif
	*retval = cnt;
done:
	if (needfree)
		FREE(needfree, M_IOV);
	return (error);
}


int
netbsd32_rename(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_rename_args /* {
		syscallarg(const netbsd32_charp) from;
		syscallarg(const netbsd32_charp) to;
	} */ *uap = v;
	struct sys_rename_args ua;

	NETBSD32TOP_UAP(from, const char);
	NETBSD32TOP_UAP(to, const char)

	return (sys_rename(p, &ua, retval));
}

int
netbsd32_flock(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_flock_args /* {
		syscallarg(int) fd;
		syscallarg(int) how;
	} */ *uap = v;
	struct sys_flock_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(how)

	return (sys_flock(p, &ua, retval));
}

int
netbsd32_mkfifo(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_mkfifo_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct sys_mkfifo_args ua;

	NETBSD32TOP_UAP(path, const char)
	NETBSD32TO64_UAP(mode);
	return (sys_mkfifo(p, &ua, retval));
}

int
netbsd32_shutdown(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_shutdown_args /* {
		syscallarg(int) s;
		syscallarg(int) how;
	} */ *uap = v;
	struct sys_shutdown_args ua;

	NETBSD32TO64_UAP(s)
	NETBSD32TO64_UAP(how);
	return (sys_shutdown(p, &ua, retval));
}

int
netbsd32_socketpair(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_socketpair_args /* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
		syscallarg(netbsd32_intp) rsv;
	} */ *uap = v;
	struct sys_socketpair_args ua;

	NETBSD32TO64_UAP(domain);
	NETBSD32TO64_UAP(type);
	NETBSD32TO64_UAP(protocol);
	NETBSD32TOP_UAP(rsv, int);
	/* Since we're just copying out two `int's we can do this */
	return (sys_socketpair(p, &ua, retval));
}

int
netbsd32_mkdir(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_mkdir_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct sys_mkdir_args ua;

	NETBSD32TOP_UAP(path, const char)
	NETBSD32TO64_UAP(mode);
	return (sys_mkdir(p, &ua, retval));
}

int
netbsd32_rmdir(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_rmdir_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_rmdir_args ua;

	NETBSD32TOP_UAP(path, const char);
	return (sys_rmdir(p, &ua, retval));
}

int
netbsd32_utimes(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_utimes_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_timevalp_t) tptr;
	} */ *uap = v;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, (char *)(u_long)SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);

	error = change_utimes32(nd.ni_vp, (struct timeval *)(u_long)SCARG(uap, tptr), p);

	vrele(nd.ni_vp);
	return (error);
}

/*
 * Common routine to set access and modification times given a vnode.
 */
static int
change_utimes32(vp, tptr, p)
	struct vnode *vp;
	struct timeval *tptr;
	struct proc *p;
{
	struct netbsd32_timeval tv32[2];
	struct timeval tv[2];
	struct vattr vattr;
	int error;

	VATTR_NULL(&vattr);
	if (tptr == NULL) {
		microtime(&tv[0]);
		tv[1] = tv[0];
		vattr.va_vaflags |= VA_UTIMES_NULL;
	} else {
		error = copyin(tptr, tv, sizeof(tv));
		if (error)
			return (error);
	}
	netbsd32_to_timeval(&tv32[0], &tv[0]);
	netbsd32_to_timeval(&tv32[1], &tv[1]);
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	vattr.va_atime.tv_sec = tv[0].tv_sec;
	vattr.va_atime.tv_nsec = tv[0].tv_usec * 1000;
	vattr.va_mtime.tv_sec = tv[1].tv_sec;
	vattr.va_mtime.tv_nsec = tv[1].tv_usec * 1000;
	error = VOP_SETATTR(vp, &vattr, p->p_ucred, p);
	VOP_UNLOCK(vp, 0);
	return (error);
}

int
netbsd32_adjtime(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_adjtime_args /* {
		syscallarg(const netbsd32_timevalp_t) delta;
		syscallarg(netbsd32_timevalp_t) olddelta;
	} */ *uap = v;
	struct netbsd32_timeval atv;
	int32_t ndelta, ntickdelta, odelta;
	int s, error;
	extern long bigadj, timedelta;
	extern int tickdelta;

	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
		return (error);

	error = copyin((caddr_t)(u_long)SCARG(uap, delta), &atv, sizeof(struct timeval));
	if (error)
		return (error);
	/*
	 * Compute the total correction and the rate at which to apply it.
	 * Round the adjustment down to a whole multiple of the per-tick
	 * delta, so that after some number of incremental changes in
	 * hardclock(), tickdelta will become zero, lest the correction
	 * overshoot and start taking us away from the desired final time.
	 */
	ndelta = atv.tv_sec * 1000000 + atv.tv_usec;
	if (ndelta > bigadj)
		ntickdelta = 10 * tickadj;
	else
		ntickdelta = tickadj;
	if (ndelta % ntickdelta)
		ndelta = ndelta / ntickdelta * ntickdelta;

	/*
	 * To make hardclock()'s job easier, make the per-tick delta negative
	 * if we want time to run slower; then hardclock can simply compute
	 * tick + tickdelta, and subtract tickdelta from timedelta.
	 */
	if (ndelta < 0)
		ntickdelta = -ntickdelta;
	s = splclock();
	odelta = timedelta;
	timedelta = ndelta;
	tickdelta = ntickdelta;
	splx(s);

	if (SCARG(uap, olddelta)) {
		atv.tv_sec = odelta / 1000000;
		atv.tv_usec = odelta % 1000000;
		(void) copyout(&atv, (caddr_t)(u_long)SCARG(uap, olddelta),
		    sizeof(struct timeval));
	}
	return (0);
}

int
netbsd32_quotactl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_quotactl_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) cmd;
		syscallarg(int) uid;
		syscallarg(netbsd32_caddr_t) arg;
	} */ *uap = v;
	struct sys_quotactl_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(cmd);
	NETBSD32TO64_UAP(uid);
	NETBSD32TOX64_UAP(arg, caddr_t);
	return (sys_quotactl(p, &ua, retval));
}

#if defined(NFS) || defined(NFSSERVER)
int
netbsd32_nfssvc(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32_nfssvc_args /* {
		syscallarg(int) flag;
		syscallarg(netbsd32_voidp) argp;
	} */ *uap = v;
	struct sys_nfssvc_args ua;

	NETBSD32TO64_UAP(flag);
	NETBSD32TOP_UAP(argp, void);
	return (sys_nfssvc(p, &ua, retval));
#else
	/* Why would we want to support a 32-bit nfsd? */
	return (ENOSYS);
#endif
}
#endif

int
netbsd32_statfs(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_statfs_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_statfsp_t) buf;
	} */ *uap = v;
	struct mount *mp;
	struct statfs *sp;
	struct netbsd32_statfs s32;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, (char *)(u_long)SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	mp = nd.ni_vp->v_mount;
	sp = &mp->mnt_stat;
	vrele(nd.ni_vp);
	if ((error = VFS_STATFS(mp, sp, p)) != 0)
		return (error);
	sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
	netbsd32_from_statfs(sp, &s32);
	return (copyout(&s32, (caddr_t)(u_long)SCARG(uap, buf), sizeof(s32)));
}

int
netbsd32_fstatfs(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_fstatfs_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_statfsp_t) buf;
	} */ *uap = v;
	struct file *fp;
	struct mount *mp;
	struct statfs *sp;
	struct netbsd32_statfs s32;
	int error;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	mp = ((struct vnode *)fp->f_data)->v_mount;
	sp = &mp->mnt_stat;
	if ((error = VFS_STATFS(mp, sp, p)) != 0)
		goto out;
	sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
	netbsd32_from_statfs(sp, &s32);
	error = copyout(&s32, (caddr_t)(u_long)SCARG(uap, buf), sizeof(s32));
 out:
	FILE_UNUSE(fp, p);
	return (error);
}

#if defined(NFS) || defined(NFSSERVER)
int
netbsd32_getfh(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_getfh_args /* {
		syscallarg(const netbsd32_charp) fname;
		syscallarg(netbsd32_fhandlep_t) fhp;
	} */ *uap = v;
	struct sys_getfh_args ua;

	NETBSD32TOP_UAP(fname, const char);
	NETBSD32TOP_UAP(fhp, struct fhandle);
	/* Lucky for us a fhandlep_t doesn't change sizes */
	return (sys_getfh(p, &ua, retval));
}
#endif

int
netbsd32_sysarch(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_sysarch_args /* {
		syscallarg(int) op;
		syscallarg(netbsd32_voidp) parms;
	} */ *uap = v;

	switch (SCARG(uap, op)) {
	default:
		printf("(sparc64) netbsd32_sysarch(%d)\n", SCARG(uap, op));
		return EINVAL;
	}
}

int
netbsd32_pread(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_pread_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_voidp) buf;
		syscallarg(netbsd32_size_t) nbyte;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
	} */ *uap = v;
	struct sys_pread_args ua;
	ssize_t rt;
	int error;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(buf, void);
	NETBSD32TOX_UAP(nbyte, size_t);
	NETBSD32TO64_UAP(pad);
	NETBSD32TO64_UAP(offset);
	error = sys_pread(p, &ua, (register_t *)&rt);
	*retval = rt;
	return (error);
}

int
netbsd32_pwrite(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_pwrite_args /* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_voidp) buf;
		syscallarg(netbsd32_size_t) nbyte;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
	} */ *uap = v;
	struct sys_pwrite_args ua;
	ssize_t rt;
	int error;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(buf, void);
	NETBSD32TOX_UAP(nbyte, size_t);
	NETBSD32TO64_UAP(pad);
	NETBSD32TO64_UAP(offset);
	error = sys_pwrite(p, &ua, (register_t *)&rt);
	*retval = rt;
	return (error);
}

#ifdef NTP
int
netbsd32_ntp_gettime(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_ntp_gettime_args /* {
		syscallarg(netbsd32_ntptimevalp_t) ntvp;
	} */ *uap = v;
	struct netbsd32_ntptimeval ntv32;
	struct timeval atv;
	struct ntptimeval ntv;
	int error = 0;
	int s;

	/* The following are NTP variables */
	extern long time_maxerror;
	extern long time_esterror;
	extern int time_status;
	extern int time_state;	/* clock state */
	extern int time_status;	/* clock status bits */

	if (SCARG(uap, ntvp)) {
		s = splclock();
#ifdef EXT_CLOCK
		/*
		 * The microtime() external clock routine returns a
		 * status code. If less than zero, we declare an error
		 * in the clock status word and return the kernel
		 * (software) time variable. While there are other
		 * places that call microtime(), this is the only place
		 * that matters from an application point of view.
		 */
		if (microtime(&atv) < 0) {
			time_status |= STA_CLOCKERR;
			ntv.time = time;
		} else
			time_status &= ~STA_CLOCKERR;
#else /* EXT_CLOCK */
		microtime(&atv);
#endif /* EXT_CLOCK */
		ntv.time = atv;
		ntv.maxerror = time_maxerror;
		ntv.esterror = time_esterror;
		(void) splx(s);

		netbsd32_from_timeval(&ntv.time, &ntv32.time);
		ntv32.maxerror = (netbsd32_long)ntv.maxerror;
		ntv32.esterror = (netbsd32_long)ntv.esterror;
		error = copyout((caddr_t)&ntv32, (caddr_t)(u_long)SCARG(uap, ntvp),
		    sizeof(ntv32));
	}
	if (!error) {

		/*
		 * Status word error decode. If any of these conditions
		 * occur, an error is returned, instead of the status
		 * word. Most applications will care only about the fact
		 * the system clock may not be trusted, not about the
		 * details.
		 *
		 * Hardware or software error
		 */
		if ((time_status & (STA_UNSYNC | STA_CLOCKERR)) ||

		/*
		 * PPS signal lost when either time or frequency
		 * synchronization requested
		 */
		    (time_status & (STA_PPSFREQ | STA_PPSTIME) &&
		    !(time_status & STA_PPSSIGNAL)) ||

		/*
		 * PPS jitter exceeded when time synchronization
		 * requested
		 */
		    (time_status & STA_PPSTIME &&
		    time_status & STA_PPSJITTER) ||

		/*
		 * PPS wander exceeded or calibration error when
		 * frequency synchronization requested
		 */
		    (time_status & STA_PPSFREQ &&
		    time_status & (STA_PPSWANDER | STA_PPSERROR)))
			*retval = TIME_ERROR;
		else
			*retval = time_state;
	}
	return(error);
}

int
netbsd32_ntp_adjtime(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_ntp_adjtime_args /* {
		syscallarg(netbsd32_timexp_t) tp;
	} */ *uap = v;
	struct netbsd32_timex ntv32;
	struct timex ntv;
	int error = 0;
	int modes;
	int s;
	extern long time_freq;		/* frequency offset (scaled ppm) */
	extern long time_maxerror;
	extern long time_esterror;
	extern int time_state;	/* clock state */
	extern int time_status;	/* clock status bits */
	extern long time_constant;		/* pll time constant */
	extern long time_offset;		/* time offset (us) */
	extern long time_tolerance;	/* frequency tolerance (scaled ppm) */
	extern long time_precision;	/* clock precision (us) */

	if ((error = copyin((caddr_t)(u_long)SCARG(uap, tp), (caddr_t)&ntv32,
			sizeof(ntv32))))
		return (error);
	netbsd32_to_timex(&ntv32, &ntv);

	/*
	 * Update selected clock variables - only the superuser can
	 * change anything. Note that there is no error checking here on
	 * the assumption the superuser should know what it is doing.
	 */
	modes = ntv.modes;
	if (modes != 0 && (error = suser(p->p_ucred, &p->p_acflag)))
		return (error);

	s = splclock();
	if (modes & MOD_FREQUENCY)
#ifdef PPS_SYNC
		time_freq = ntv.freq - pps_freq;
#else /* PPS_SYNC */
		time_freq = ntv.freq;
#endif /* PPS_SYNC */
	if (modes & MOD_MAXERROR)
		time_maxerror = ntv.maxerror;
	if (modes & MOD_ESTERROR)
		time_esterror = ntv.esterror;
	if (modes & MOD_STATUS) {
		time_status &= STA_RONLY;
		time_status |= ntv.status & ~STA_RONLY;
	}
	if (modes & MOD_TIMECONST)
		time_constant = ntv.constant;
	if (modes & MOD_OFFSET)
		hardupdate(ntv.offset);

	/*
	 * Retrieve all clock variables
	 */
	if (time_offset < 0)
		ntv.offset = -(-time_offset >> SHIFT_UPDATE);
	else
		ntv.offset = time_offset >> SHIFT_UPDATE;
#ifdef PPS_SYNC
	ntv.freq = time_freq + pps_freq;
#else /* PPS_SYNC */
	ntv.freq = time_freq;
#endif /* PPS_SYNC */
	ntv.maxerror = time_maxerror;
	ntv.esterror = time_esterror;
	ntv.status = time_status;
	ntv.constant = time_constant;
	ntv.precision = time_precision;
	ntv.tolerance = time_tolerance;
#ifdef PPS_SYNC
	ntv.shift = pps_shift;
	ntv.ppsfreq = pps_freq;
	ntv.jitter = pps_jitter >> PPS_AVG;
	ntv.stabil = pps_stabil;
	ntv.calcnt = pps_calcnt;
	ntv.errcnt = pps_errcnt;
	ntv.jitcnt = pps_jitcnt;
	ntv.stbcnt = pps_stbcnt;
#endif /* PPS_SYNC */
	(void)splx(s);

	netbsd32_from_timex(&ntv, &ntv32);
	error = copyout((caddr_t)&ntv32, (caddr_t)(u_long)SCARG(uap, tp),
	    sizeof(ntv32));
	if (!error) {

		/*
		 * Status word error decode. See comments in
		 * ntp_gettime() routine.
		 */
		if ((time_status & (STA_UNSYNC | STA_CLOCKERR)) ||
		    (time_status & (STA_PPSFREQ | STA_PPSTIME) &&
		    !(time_status & STA_PPSSIGNAL)) ||
		    (time_status & STA_PPSTIME &&
		    time_status & STA_PPSJITTER) ||
		    (time_status & STA_PPSFREQ &&
		    time_status & (STA_PPSWANDER | STA_PPSERROR)))
			*retval = TIME_ERROR;
		else
			*retval = time_state;
	}
	return error;
}
#else
int
netbsd32_ntp_gettime(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	return(ENOSYS);
}

int
netbsd32_ntp_adjtime(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	return (ENOSYS);
}
#endif

int
netbsd32_setgid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_setgid_args /* {
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct sys_setgid_args ua;

	NETBSD32TO64_UAP(gid);
	return (sys_setgid(p, v, retval));
}

int
netbsd32_setegid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_setegid_args /* {
		syscallarg(gid_t) egid;
	} */ *uap = v;
	struct sys_setegid_args ua;

	NETBSD32TO64_UAP(egid);
	return (sys_setegid(p, v, retval));
}

int
netbsd32_seteuid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_seteuid_args /* {
		syscallarg(gid_t) euid;
	} */ *uap = v;
	struct sys_seteuid_args ua;

	NETBSD32TO64_UAP(euid);
	return (sys_seteuid(p, v, retval));
}

#ifdef LFS
int
netbsd32_sys_lfs_bmapv(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32_lfs_bmapv_args /* {
		syscallarg(netbsd32_fsid_tp_t) fsidp;
		syscallarg(netbsd32_block_infop_t) blkiov;
		syscallarg(int) blkcnt;
	} */ *uap = v;
	struct sys_lfs_bmapv_args ua;

	NETBSD32TOP_UAP(fdidp, struct fsid);
	NETBSD32TO64_UAP(blkcnt);
	/* XXX finish me */
#else

	return (ENOSYS);	/* XXX */
#endif
}

int
netbsd32_sys_lfs_markv(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32_lfs_markv_args /* {
		syscallarg(netbsd32_fsid_tp_t) fsidp;
		syscallarg(netbsd32_block_infop_t) blkiov;
		syscallarg(int) blkcnt;
	} */ *uap = v;
#endif

	return (ENOSYS);	/* XXX */
}

int
netbsd32_sys_lfs_segclean(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32_lfs_segclean_args /* {
		syscallarg(netbsd32_fsid_tp_t) fsidp;
		syscallarg(netbsd32_u_long) segment;
	} */ *uap = v;
#endif

	return (ENOSYS);	/* XXX */
}

int
netbsd32_sys_lfs_segwait(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32_lfs_segwait_args /* {
		syscallarg(netbsd32_fsid_tp_t) fsidp;
		syscallarg(netbsd32_timevalp_t) tv;
	} */ *uap = v;
#endif

	return (ENOSYS);	/* XXX */
}
#endif

int
netbsd32_pathconf(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_pathconf_args /* {
		syscallarg(int) fd;
		syscallarg(int) name;
	} */ *uap = v;
	struct sys_pathconf_args ua;
	long rt;
	int error;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(name);
	error = sys_pathconf(p, &ua, (register_t *)&rt);
	*retval = rt;
	return (error);
}

int
netbsd32_fpathconf(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_fpathconf_args /* {
		syscallarg(int) fd;
		syscallarg(int) name;
	} */ *uap = v;
	struct sys_fpathconf_args ua;
	long rt;
	int error;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(name);
	error = sys_fpathconf(p, &ua, (register_t *)&rt);
	*retval = rt;
	return (error);
}

int
netbsd32_getrlimit(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_getrlimit_args /* {
		syscallarg(int) which;
		syscallarg(netbsd32_rlimitp_t) rlp;
	} */ *uap = v;
	int which = SCARG(uap, which);

	if ((u_int)which >= RLIM_NLIMITS)
		return (EINVAL);
	return (copyout(&p->p_rlimit[which], (caddr_t)(u_long)SCARG(uap, rlp),
	    sizeof(struct rlimit)));
}

int
netbsd32_setrlimit(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_setrlimit_args /* {
		syscallarg(int) which;
		syscallarg(const netbsd32_rlimitp_t) rlp;
	} */ *uap = v;
		int which = SCARG(uap, which);
	struct rlimit alim;
	int error;

	error = copyin((caddr_t)(u_long)SCARG(uap, rlp), &alim, sizeof(struct rlimit));
	if (error)
		return (error);
	return (dosetrlimit(p, p->p_cred, which, &alim));
}

int
netbsd32_mmap(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_mmap_args /* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) prot;
		syscallarg(int) flags;
		syscallarg(int) fd;
		syscallarg(netbsd32_long) pad;
		syscallarg(off_t) pos;
	} */ *uap = v;
	struct sys_mmap_args ua;
	void *rt;
	int error;

	NETBSD32TOP_UAP(addr, void);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TO64_UAP(prot);
	NETBSD32TO64_UAP(flags);
	NETBSD32TO64_UAP(fd);
	NETBSD32TOX_UAP(pad, long);
	NETBSD32TOX_UAP(pos, off_t);
	error = sys_mmap(p, &ua, (register_t *)&rt);
	if ((long)rt > (long)UINT_MAX)
		printf("netbsd32_mmap: retval out of range: %p", rt);
	*retval = (netbsd32_voidp)(u_long)rt;
	return (error);
}

int
netbsd32_lseek(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_lseek_args /* {
		syscallarg(int) fd;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
		syscallarg(int) whence;
	} */ *uap = v;
	struct sys_lseek_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(pad);
	NETBSD32TO64_UAP(offset);
	NETBSD32TO64_UAP(whence);
	return (sys_lseek(p, &ua, retval));
}

int
netbsd32_truncate(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_truncate_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) pad;
		syscallarg(off_t) length;
	} */ *uap = v;
	struct sys_truncate_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(pad);
	NETBSD32TO64_UAP(length);
	return (sys_truncate(p, &ua, retval));
}

int
netbsd32_ftruncate(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_ftruncate_args /* {
		syscallarg(int) fd;
		syscallarg(int) pad;
		syscallarg(off_t) length;
	} */ *uap = v;
	struct sys_ftruncate_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(pad);
	NETBSD32TO64_UAP(length);
	return (sys_ftruncate(p, &ua, retval));
}

int
netbsd32___sysctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32___sysctl_args /* {
		syscallarg(netbsd32_intp) name;
		syscallarg(u_int) namelen;
		syscallarg(netbsd32_voidp) old;
		syscallarg(netbsd32_size_tp) oldlenp;
		syscallarg(netbsd32_voidp) new;
		syscallarg(netbsd32_size_t) newlen;
	} */ *uap = v;
	int error;
	netbsd32_size_t savelen = 0;
	size_t oldlen = 0;
	sysctlfn *fn;
	int name[CTL_MAXNAME];

/*
 * Some of these sysctl functions do their own copyin/copyout.
 * We need to disable or emulate the ones that need their
 * arguments converted.
 */

	if (SCARG(uap, new) != NULL &&
	    (error = suser(p->p_ucred, &p->p_acflag)))
		return (error);
	/*
	 * all top-level sysctl names are non-terminal
	 */
	if (SCARG(uap, namelen) > CTL_MAXNAME || SCARG(uap, namelen) < 2)
		return (EINVAL);
	error = copyin((caddr_t)(u_long)SCARG(uap, name), &name,
		       SCARG(uap, namelen) * sizeof(int));
	if (error)
		return (error);

	switch (name[0]) {
	case CTL_KERN:
		fn = kern_sysctl;
		break;
	case CTL_HW:
		fn = hw_sysctl;
		break;
	case CTL_VM:
		fn = uvm_sysctl;
		break;
	case CTL_NET:
		fn = net_sysctl;
		break;
	case CTL_VFS:
		fn = vfs_sysctl;
		break;
	case CTL_MACHDEP:
		fn = cpu_sysctl;
		break;
#ifdef DEBUG
	case CTL_DEBUG:
		fn = debug_sysctl;
		break;
#endif
#ifdef DDB
	case CTL_DDB:
		fn = ddb_sysctl;
		break;
#endif
	case CTL_PROC:
		fn = proc_sysctl;
		break;
	default:
		return (EOPNOTSUPP);
	}

	/*
	 * XXX Hey, we wire `old', but what about `new'?
	 */

	if (SCARG(uap, oldlenp) &&
	    (error = copyin((caddr_t)(u_long)SCARG(uap, oldlenp), &savelen,
	     sizeof(savelen))))
		return (error);
	if (SCARG(uap, old) != NULL) {
		error = lockmgr(&sysctl_memlock, LK_EXCLUSIVE, NULL);
		if (error)
			return (error);
		if (uvm_vslock(p, (void *)(u_long)SCARG(uap, old), savelen,
		    VM_PROT_READ|VM_PROT_WRITE) != KERN_SUCCESS) {
			(void) lockmgr(&sysctl_memlock, LK_RELEASE, NULL);
			return (EFAULT);
		}
		oldlen = savelen;
	}
	error = (*fn)(name + 1, SCARG(uap, namelen) - 1, 
		      (void *)(u_long)SCARG(uap, old), &oldlen, 
		      (void *)(u_long)SCARG(uap, new), SCARG(uap, newlen), p);
	if (SCARG(uap, old) != NULL) {
		uvm_vsunlock(p, (void *)(u_long)SCARG(uap, old), savelen);
		(void) lockmgr(&sysctl_memlock, LK_RELEASE, NULL);
	}
	savelen = oldlen;
	if (error)
		return (error);
	if (SCARG(uap, oldlenp))
		error = copyout(&savelen,
		    (caddr_t)(u_long)SCARG(uap, oldlenp), sizeof(savelen));
	return (error);
}

int
netbsd32_mlock(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_mlock_args /* {
		syscallarg(const netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
	} */ *uap = v;
	struct sys_mlock_args ua;

	NETBSD32TOP_UAP(addr, const void);
	NETBSD32TO64_UAP(len);
	return (sys_mlock(p, &ua, retval));
}

int
netbsd32_munlock(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_munlock_args /* {
		syscallarg(const netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
	} */ *uap = v;
	struct sys_munlock_args ua;

	NETBSD32TOP_UAP(addr, const void);
	NETBSD32TO64_UAP(len);
	return (sys_munlock(p, &ua, retval));
}

int
netbsd32_undelete(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_undelete_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_undelete_args ua;

	NETBSD32TOP_UAP(path, const char);
	return (sys_undelete(p, &ua, retval));
}

int
netbsd32_futimes(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_futimes_args /* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_timevalp_t) tptr;
	} */ *uap = v;
	int error;
	struct file *fp;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);

	error = change_utimes32((struct vnode *)fp->f_data, 
				(struct timeval *)(u_long)SCARG(uap, tptr), p);
	FILE_UNUSE(fp, p);
	return (error);
}

int
netbsd32_getpgid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_getpgid_args /* {
		syscallarg(pid_t) pid;
	} */ *uap = v;
	struct sys_getpgid_args ua;

	NETBSD32TO64_UAP(pid);
	return (sys_getpgid(p, &ua, retval));
}

int
netbsd32_reboot(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_reboot_args /* {
		syscallarg(int) opt;
		syscallarg(netbsd32_charp) bootstr;
	} */ *uap = v;
	struct sys_reboot_args ua;

	NETBSD32TO64_UAP(opt);
	NETBSD32TOP_UAP(bootstr, char);
	return (sys_reboot(p, &ua, retval));
}

int
netbsd32_poll(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_poll_args /* {
		syscallarg(netbsd32_pollfdp_t) fds;
		syscallarg(u_int) nfds;
		syscallarg(int) timeout;
	} */ *uap = v;
	struct sys_poll_args ua;

	NETBSD32TOP_UAP(fds, struct pollfd);
	NETBSD32TO64_UAP(nfds);
	NETBSD32TO64_UAP(timeout);
	return (sys_poll(p, &ua, retval));
}

#if defined(SYSVSEM)
/*
 * XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
 *
 * This is BSD.  We won't support System V IPC.
 * Too much work.
 *
 * XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
 */
int
netbsd32___semctl14(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32___semctl_args /* {
		syscallarg(int) semid;
		syscallarg(int) semnum;
		syscallarg(int) cmd;
		syscallarg(netbsd32_semunu_t *) arg;
	} */ *uap = v;
	union netbsd32_semun sem32;
	int semid = SCARG(uap, semid);
	int semnum = SCARG(uap, semnum);
	int cmd = SCARG(uap, cmd);
	union netbsd32_semun *arg = (void*)(u_long)SCARG(uap, arg);
	union netbsd32_semun real_arg;
	struct ucred *cred = p->p_ucred;
	int i, rval, eval;
	struct netbsd32_semid_ds sbuf;
	struct semid_ds *semaptr;

	semlock(p);

	semid = IPCID_TO_IX(semid);
	if (semid < 0 || semid >= seminfo.semmsl)
		return(EINVAL);

	semaptr = &sema[semid];
	if ((semaptr->sem_perm.mode & SEM_ALLOC) == 0 ||
	    semaptr->sem_perm.seq != IPCID_TO_SEQ(SCARG(uap, semid)))
		return(EINVAL);

	eval = 0;
	rval = 0;

	switch (cmd) {
	case IPC_RMID:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_M)) != 0)
			return(eval);
		semaptr->sem_perm.cuid = cred->cr_uid;
		semaptr->sem_perm.uid = cred->cr_uid;
		semtot -= semaptr->sem_nsems;
		for (i = semaptr->_sem_base - sem; i < semtot; i++)
			sem[i] = sem[i + semaptr->sem_nsems];
		for (i = 0; i < seminfo.semmni; i++) {
			if ((sema[i].sem_perm.mode & SEM_ALLOC) &&
			    sema[i]._sem_base > semaptr->_sem_base)
				sema[i]._sem_base -= semaptr->sem_nsems;
		}
		semaptr->sem_perm.mode = 0;
		semundo_clear(semid, -1);
		wakeup((caddr_t)semaptr);
		break;

	case IPC_SET:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_M)))
			return(eval);
		if ((eval = copyin(arg, &real_arg, sizeof(real_arg))) != 0)
			return(eval);
		if ((eval = copyin((caddr_t)(u_long)real_arg.buf, (caddr_t)&sbuf,
		    sizeof(sbuf))) != 0)
			return(eval);
		semaptr->sem_perm.uid = sbuf.sem_perm.uid;
		semaptr->sem_perm.gid = sbuf.sem_perm.gid;
		semaptr->sem_perm.mode = (semaptr->sem_perm.mode & ~0777) |
		    (sbuf.sem_perm.mode & 0777);
		semaptr->sem_ctime = time.tv_sec;
		break;

	case IPC_STAT:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			return(eval);
		if ((eval = copyin(arg, &real_arg, sizeof(real_arg))) != 0)
			return(eval);
		eval = copyout((caddr_t)semaptr, (caddr_t)(u_long)real_arg.buf,
		    sizeof(struct semid_ds));
		break;

	case GETNCNT:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			return(eval);
		if (semnum < 0 || semnum >= semaptr->sem_nsems)
			return(EINVAL);
		rval = semaptr->_sem_base[semnum].semncnt;
		break;

	case GETPID:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			return(eval);
		if (semnum < 0 || semnum >= semaptr->sem_nsems)
			return(EINVAL);
		rval = semaptr->_sem_base[semnum].sempid;
		break;

	case GETVAL:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			return(eval);
		if (semnum < 0 || semnum >= semaptr->sem_nsems)
			return(EINVAL);
		rval = semaptr->_sem_base[semnum].semval;
		break;

	case GETALL:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			return(eval);
		if ((eval = copyin(arg, &real_arg, sizeof(real_arg))) != 0)
			return(eval);
		for (i = 0; i < semaptr->sem_nsems; i++) {
			eval = copyout((caddr_t)&semaptr->_sem_base[i].semval,
			    &real_arg.array[i], sizeof(real_arg.array[0]));
			if (eval != 0)
				break;
		}
		break;

	case GETZCNT:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_R)))
			return(eval);
		if (semnum < 0 || semnum >= semaptr->sem_nsems)
			return(EINVAL);
		rval = semaptr->_sem_base[semnum].semzcnt;
		break;

	case SETVAL:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_W)))
			return(eval);
		if (semnum < 0 || semnum >= semaptr->sem_nsems)
			return(EINVAL);
		if ((eval = copyin(arg, &real_arg, sizeof(real_arg))) != 0)
			return(eval);
		semaptr->_sem_base[semnum].semval = real_arg.val;
		semundo_clear(semid, semnum);
		wakeup((caddr_t)semaptr);
		break;

	case SETALL:
		if ((eval = ipcperm(cred, &semaptr->sem_perm, IPC_W)))
			return(eval);
		if ((eval = copyin(arg, &real_arg, sizeof(real_arg))) != 0)
			return(eval);
		for (i = 0; i < semaptr->sem_nsems; i++) {
			eval = copyin(&real_arg.array[i],
			    (caddr_t)&semaptr->_sem_base[i].semval,
			    sizeof(real_arg.array[0]));
			if (eval != 0)
				break;
		}
		semundo_clear(semid, -1);
		wakeup((caddr_t)semaptr);
		break;

	default:
		return(EINVAL);
	}

	if (eval == 0)
		*retval = rval;
	return(eval);
#else
	return (ENOSYS);
#endif
}

int
netbsd32_semget(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_semget_args /* {
		syscallarg(netbsd32_key_t) key;
		syscallarg(int) nsems;
		syscallarg(int) semflg;
	} */ *uap = v;
	struct sys_semget_args ua;

	NETBSD32TOX_UAP(key, key_t);
	NETBSD32TO64_UAP(nsems);
	NETBSD32TO64_UAP(semflg);
	return (sys_semget(p, &ua, retval));
}

int
netbsd32_semop(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_semop_args /* {
		syscallarg(int) semid;
		syscallarg(netbsd32_sembufp_t) sops;
		syscallarg(netbsd32_size_t) nsops;
	} */ *uap = v;
	struct sys_semop_args ua;

	NETBSD32TO64_UAP(semid);
	NETBSD32TOP_UAP(sops, struct sembuf);
	NETBSD32TOX_UAP(nsops, size_t);
	return (sys_semop(p, &ua, retval));
}

int
netbsd32_semconfig(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_semconfig_args /* {
		syscallarg(int) flag;
	} */ *uap = v;
	struct sys_semconfig_args ua;

	NETBSD32TO64_UAP(flag);
	return (sys_semconfig(p, &ua, retval));
}
#endif /* SYSVSEM */

#if defined(SYSVMSG)

int
netbsd32___msgctl13(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32_msgctl_args /* {
		syscallarg(int) msqid;
		syscallarg(int) cmd;
		syscallarg(netbsd32_msqid_dsp_t) buf;
	} */ *uap = v;
	struct sys_msgctl_args ua;
	struct msqid_ds ds;
	struct netbsd32_msqid_ds *ds32p;
	int error;

	NETBSD32TO64_UAP(msqid);
	NETBSD32TO64_UAP(cmd);
	ds32p = (struct netbsd32_msqid_ds *)(u_long)SCARG(uap, buf);
	if (ds32p) {
		SCARG(&ua, buf) = NULL;
		netbsd32_to_msqid_ds(ds32p, &ds);
	} else
		SCARG(&ua, buf) = NULL;
	error = sys_msgctl(p, &ua, retval);
	if (error)
		return (error);

	if (ds32p)
		netbsd32_from_msqid_ds(&ds, ds32p);
	return (0);
#else
	return (ENOSYS);
#endif
}

int
netbsd32_msgget(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32_msgget_args /* {
		syscallarg(netbsd32_key_t) key;
		syscallarg(int) msgflg;
	} */ *uap = v;
	struct sys_msgget_args ua;

	NETBSD32TOX_UAP(key, key_t);
	NETBSD32TO64_UAP(msgflg);
	return (sys_msgget(p, &ua, retval));
#else
	return (ENOSYS);
#endif
}

int
netbsd32_msgsnd(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32_msgsnd_args /* {
		syscallarg(int) msqid;
		syscallarg(const netbsd32_voidp) msgp;
		syscallarg(netbsd32_size_t) msgsz;
		syscallarg(int) msgflg;
	} */ *uap = v;
	struct sys_msgsnd_args ua;

	NETBSD32TO64_UAP(msqid);
	NETBSD32TOP_UAP(msgp, void);
	NETBSD32TOX_UAP(msgsz, size_t);
	NETBSD32TO64_UAP(msgflg);
	return (sys_msgsnd(p, &ua, retval));
#else
	return (ENOSYS);
#endif
}

int
netbsd32_msgrcv(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32_msgrcv_args /* {
		syscallarg(int) msqid;
		syscallarg(netbsd32_voidp) msgp;
		syscallarg(netbsd32_size_t) msgsz;
		syscallarg(netbsd32_long) msgtyp;
		syscallarg(int) msgflg;
	} */ *uap = v;
	struct sys_msgrcv_args ua;
	ssize_t rt;
	int error;

	NETBSD32TO64_UAP(msqid);
	NETBSD32TOP_UAP(msgp, void);
	NETBSD32TOX_UAP(msgsz, size_t);
	NETBSD32TOX_UAP(msgtyp, long);
	NETBSD32TO64_UAP(msgflg);
	error = sys_msgrcv(p, &ua, (register_t *)&rt);
	*retval = rt;
	return (error);
#else
	return (ENOSYS);
#endif
}
#endif /* SYSVMSG */

#if defined(SYSVSHM)

int
netbsd32_shmat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32_shmat_args /* {
		syscallarg(int) shmid;
		syscallarg(const netbsd32_voidp) shmaddr;
		syscallarg(int) shmflg;
	} */ *uap = v;
	struct sys_shmat_args ua;
	void *rt;
	int error;

	NETBSD32TO64_UAP(shmid);
	NETBSD32TOP_UAP(shmaddr, void);
	NETBSD32TO64_UAP(shmflg);
	error = sys_shmat(p, &ua, (register_t *)&rt);
	*retval = rt;
	return (error);
#else
	return (ENOSYS);
#endif
}

int
netbsd32___shmctl13(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32_shmctl_args /* {
		syscallarg(int) shmid;
		syscallarg(int) cmd;
		syscallarg(netbsd32_shmid_dsp_t) buf;
	} */ *uap = v;
	struct sys_shmctl_args ua;
	struct shmid_ds ds;
	struct netbsd32_shmid_ds *ds32p;
	int error;

	NETBSD32TO64_UAP(shmid);
	NETBSD32TO64_UAP(cmd);
	ds32p = (struct netbsd32_shmid_ds *)(u_long)SCARG(uap, buf);
	if (ds32p) {
		SCARG(&ua, buf) = NULL;
		netbsd32_to_shmid_ds(ds32p, &ds);
	} else
		SCARG(&ua, buf) = NULL;
	error = sys_shmctl(p, &ua, retval);
	if (error)
		return (error);

	if (ds32p)
		netbsd32_from_shmid_ds(&ds, ds32p);
	return (0);
#else
	return (ENOSYS);
#endif
}

int
netbsd32_shmdt(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32_shmdt_args /* {
		syscallarg(const netbsd32_voidp) shmaddr;
	} */ *uap = v;
	struct sys_shmdt_args ua;

	NETBSD32TOP_UAP(shmaddr, const char);
	return (sys_shmdt(p, &ua, retval));
#else
	return (ENOSYS);
#endif
}

int
netbsd32_shmget(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32_shmget_args /* {
		syscallarg(netbsd32_key_t) key;
		syscallarg(netbsd32_size_t) size;
		syscallarg(int) shmflg;
	} */ *uap = v;
	struct sys_shmget_args ua;

	NETBSD32TOX_UAP(key, key_t)
	NETBSD32TOX_UAP(size, size_t)
	NETBSD32TO64_UAP(shmflg);
	return (sys_shmget(p, &ua, retval));
#else
	return (ENOSYS);
#endif
}
#endif /* SYSVSHM */

int
netbsd32_clock_gettime(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_clock_gettime_args /* {
		syscallarg(netbsd32_clockid_t) clock_id;
		syscallarg(netbsd32_timespecp_t) tp;
	} */ *uap = v;
	clockid_t clock_id;
	struct timeval atv;
	struct timespec ats;
	struct netbsd32_timespec ts32;

	clock_id = SCARG(uap, clock_id);
	if (clock_id != CLOCK_REALTIME)
		return (EINVAL);

	microtime(&atv);
	TIMEVAL_TO_TIMESPEC(&atv,&ats);
	netbsd32_from_timespec(&ats, &ts32);

	return copyout(&ts32, (caddr_t)(u_long)SCARG(uap, tp), sizeof(ts32));
}

int
netbsd32_clock_settime(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_clock_settime_args /* {
		syscallarg(netbsd32_clockid_t) clock_id;
		syscallarg(const netbsd32_timespecp_t) tp;
	} */ *uap = v;
	struct netbsd32_timespec ts32;
	clockid_t clock_id;
	struct timeval atv;
	struct timespec ats;
	int error;

	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
		return (error);

	clock_id = SCARG(uap, clock_id);
	if (clock_id != CLOCK_REALTIME)
		return (EINVAL);

	if ((error = copyin((caddr_t)(u_long)SCARG(uap, tp), &ts32, sizeof(ts32))) != 0)
		return (error);

	netbsd32_to_timespec(&ts32, &ats);
	TIMESPEC_TO_TIMEVAL(&atv,&ats);
	if ((error = settime(&atv)))
		return (error);

	return 0;
}

int
netbsd32_clock_getres(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_clock_getres_args /* {
		syscallarg(netbsd32_clockid_t) clock_id;
		syscallarg(netbsd32_timespecp_t) tp;
	} */ *uap = v;
	struct netbsd32_timespec ts32;
	clockid_t clock_id;
	struct timespec ts;
	int error = 0;

	clock_id = SCARG(uap, clock_id);
	if (clock_id != CLOCK_REALTIME)
		return (EINVAL);

	if (SCARG(uap, tp)) {
		ts.tv_sec = 0;
		ts.tv_nsec = 1000000000 / hz;

		netbsd32_from_timespec(&ts, &ts32);
		error = copyout(&ts, (caddr_t)(u_long)SCARG(uap, tp), sizeof(ts));
	}

	return error;
}

int
netbsd32_nanosleep(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_nanosleep_args /* {
		syscallarg(const netbsd32_timespecp_t) rqtp;
		syscallarg(netbsd32_timespecp_t) rmtp;
	} */ *uap = v;
	static int nanowait;
	struct netbsd32_timespec ts32;
	struct timespec rqt;
	struct timespec rmt;
	struct timeval atv, utv;
	int error, s, timo;

	error = copyin((caddr_t)(u_long)SCARG(uap, rqtp), (caddr_t)&ts32,
		       sizeof(ts32));
	if (error)
		return (error);

	netbsd32_to_timespec(&ts32, &rqt);
	TIMESPEC_TO_TIMEVAL(&atv,&rqt)
	if (itimerfix(&atv))
		return (EINVAL);

	s = splclock();
	timeradd(&atv,&time,&atv);
	timo = hzto(&atv);
	/* 
	 * Avoid inadvertantly sleeping forever
	 */
	if (timo == 0)
		timo = 1;
	splx(s);

	error = tsleep(&nanowait, PWAIT | PCATCH, "nanosleep", timo);
	if (error == ERESTART)
		error = EINTR;
	if (error == EWOULDBLOCK)
		error = 0;

	if (SCARG(uap, rmtp)) {
		int error;

		s = splclock();
		utv = time;
		splx(s);

		timersub(&atv, &utv, &utv);
		if (utv.tv_sec < 0)
			timerclear(&utv);

		TIMEVAL_TO_TIMESPEC(&utv,&rmt);
		netbsd32_from_timespec(&rmt, &ts32);
		error = copyout((caddr_t)&ts32, (caddr_t)(u_long)SCARG(uap,rmtp),
			sizeof(ts32));
		if (error)
			return (error);
	}

	return error;
}

int
netbsd32_fdatasync(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_fdatasync_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct sys_fdatasync_args ua;

	NETBSD32TO64_UAP(fd);

	return (sys_fdatasync(p, &ua, retval));
}

int
netbsd32___posix_rename(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32___posix_rename_args /* {
		syscallarg(const netbsd32_charp) from;
		syscallarg(const netbsd32_charp) to;
	} */ *uap = v;
	struct sys___posix_rename_args ua;

	NETBSD32TOP_UAP(from, const char);
	NETBSD32TOP_UAP(to, const char);

	return (sys___posix_rename(p, &ua, retval));
}

int
netbsd32_swapctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_swapctl_args /* {
		syscallarg(int) cmd;
		syscallarg(const netbsd32_voidp) arg;
		syscallarg(int) misc;
	} */ *uap = v;
	struct sys_swapctl_args ua;

	NETBSD32TO64_UAP(cmd);
	NETBSD32TOP_UAP(arg, const void);
	NETBSD32TO64_UAP(misc);
	return (sys_swapctl(p, &ua, retval));
}

int
netbsd32_getdents(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_getdents_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) buf;
		syscallarg(netbsd32_size_t) count;
	} */ *uap = v;
	struct file *fp;
	int error, done;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto out;
	}
	error = vn_readdir(fp, (caddr_t)(u_long)SCARG(uap, buf), UIO_USERSPACE,
			SCARG(uap, count), &done, p, 0, 0);
	*retval = done;
 out:
	FILE_UNUSE(fp, p);
	return (error);
}


int
netbsd32_minherit(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_minherit_args /* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) inherit;
	} */ *uap = v;
	struct sys_minherit_args ua;

	NETBSD32TOP_UAP(addr, void);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TO64_UAP(inherit);
	return (sys_minherit(p, &ua, retval));
}

int
netbsd32_lchmod(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_lchmod_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct sys_lchmod_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(mode);
	return (sys_lchmod(p, &ua, retval));
}

int
netbsd32_lchown(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_lchown_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct sys_lchown_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(uid);
	NETBSD32TO64_UAP(gid);
	return (sys_lchown(p, &ua, retval));
}

int
netbsd32_lutimes(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_lutimes_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_timevalp_t) tptr;
	} */ *uap = v;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, NOFOLLOW, UIO_USERSPACE, (caddr_t)(u_long)SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);

	error = change_utimes32(nd.ni_vp, (struct timeval *)(u_long)SCARG(uap, tptr), p);

	vrele(nd.ni_vp);
	return (error);
}


int
netbsd32___msync13(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32___msync13_args /* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys___msync13_args ua;

	NETBSD32TOP_UAP(addr, void);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TO64_UAP(flags);
	return (sys___msync13(p, &ua, retval));
}

int
netbsd32___stat13(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32___stat13_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_statp_t) ub;
	} */ *uap = v;
	struct netbsd32_stat sb32;
	struct stat sb;
	int error;
	struct nameidata nd;
	caddr_t sg;
	const char *path;

	path = (char *)(u_long)SCARG(uap, path);
	sg = stackgap_init(p->p_emul);
	CHECK_ALT_EXIST(p, &sg, path);

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE, path, p);
	if ((error = namei(&nd)) != 0)
		return (error);
	error = vn_stat(nd.ni_vp, &sb, p);
	vput(nd.ni_vp);
	if (error)
		return (error);
	netbsd32_from___stat13(&sb, &sb32);
	error = copyout(&sb32, (caddr_t)(u_long)SCARG(uap, ub), sizeof(sb32));
	return (error);
}

int
netbsd32___fstat13(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32___fstat13_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_statp_t) sb;
	} */ *uap = v;
	int fd = SCARG(uap, fd);
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct netbsd32_stat sb32;
	struct stat ub;
	int error = 0;

	if ((u_int)fd >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[fd]) == NULL)
		return (EBADF);
	switch (fp->f_type) {

	case DTYPE_VNODE:
		error = vn_stat((struct vnode *)fp->f_data, &ub, p);
		break;

	case DTYPE_SOCKET:
		error = soo_stat((struct socket *)fp->f_data, &ub);
		break;

	default:
		panic("fstat");
		/*NOTREACHED*/
	}
	if (error == 0) {
		netbsd32_from___stat13(&ub, &sb32);
		error = copyout(&sb32, (caddr_t)(u_long)SCARG(uap, sb), sizeof(sb32));
	}
	return (error);
}

int
netbsd32___lstat13(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32___lstat13_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_statp_t) ub;
	} */ *uap = v;
	struct netbsd32_stat sb32;
	struct stat sb;
	int error;
	struct nameidata nd;
	caddr_t sg;
	const char *path;

	path = (char *)(u_long)SCARG(uap, path);
	sg = stackgap_init(p->p_emul);
	CHECK_ALT_EXIST(p, &sg, path);

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE, path, p);
	if ((error = namei(&nd)) != 0)
		return (error);
	error = vn_stat(nd.ni_vp, &sb, p);
	vput(nd.ni_vp);
	if (error)
		return (error);
	netbsd32_from___stat13(&sb, &sb32);
	error = copyout(&sb32, (caddr_t)(u_long)SCARG(uap, ub), sizeof(sb32));
	return (error);
}

int
netbsd32___sigaltstack14(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32___sigaltstack14_args /* {
		syscallarg(const netbsd32_sigaltstackp_t) nss;
		syscallarg(netbsd32_sigaltstackp_t) oss;
	} */ *uap = v;
	struct netbsd32_sigaltstack s32;
	struct sigaltstack nss, oss;
	int error;

	if (SCARG(uap, nss)) {
		error = copyin((caddr_t)(u_long)SCARG(uap, nss), &s32, sizeof(s32));
		if (error)
			return (error);
		nss.ss_sp = (void *)(u_long)s32.ss_sp;
		nss.ss_size = (size_t)s32.ss_size;
		nss.ss_flags = s32.ss_flags;
	}
	error = sigaltstack1(p,
	    SCARG(uap, nss) ? &nss : 0, SCARG(uap, oss) ? &oss : 0);
	if (error)
		return (error);
	if (SCARG(uap, oss)) {
		s32.ss_sp = (netbsd32_voidp)(u_long)oss.ss_sp;
		s32.ss_size = (netbsd32_size_t)oss.ss_size;
		s32.ss_flags = oss.ss_flags;
		error = copyout(&s32, (caddr_t)(u_long)SCARG(uap, oss), sizeof(s32));
		if (error)
			return (error);
	}
	return (0);
}

int
netbsd32___posix_chown(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32___posix_chown_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct sys___posix_chown_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(uid);
	NETBSD32TO64_UAP(gid);
	return (sys___posix_chown(p, &ua, retval));
}

int
netbsd32___posix_fchown(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32___posix_fchown_args /* {
		syscallarg(int) fd;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct sys___posix_fchown_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(uid);
	NETBSD32TO64_UAP(gid);
	return (sys___posix_fchown(p, &ua, retval));
}

int
netbsd32___posix_lchown(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32___posix_lchown_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct sys___posix_lchown_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(uid);
	NETBSD32TO64_UAP(gid);
	return (sys___posix_lchown(p, &ua, retval));
}

int
netbsd32_getsid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_getsid_args /* {
		syscallarg(pid_t) pid;
	} */ *uap = v;
	struct sys_getsid_args ua;

	NETBSD32TO64_UAP(pid);
	return (sys_getsid(p, &ua, retval));
}

#ifdef KTRACE
int
netbsd32_fktrace(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_fktrace_args /* {
		syscallarg(const int) fd;
		syscallarg(int) ops;
		syscallarg(int) facs;
		syscallarg(int) pid;
	} */ *uap = v;
#if 0
	struct sys_fktrace_args ua;
#else
	/* XXXX */
	struct sys_fktrace_noconst_args {
		syscallarg(int) fd;
		syscallarg(int) ops;
		syscallarg(int) facs;
		syscallarg(int) pid;
	} ua;
#endif

	NETBSD32TOX_UAP(fd, int);
	NETBSD32TO64_UAP(ops);
	NETBSD32TO64_UAP(facs);
	NETBSD32TO64_UAP(pid);
	return (sys_fktrace(p, &ua, retval));
}
#endif /* KTRACE */

int
netbsd32_preadv(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_preadv_args /* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_iovecp_t) iovp;
		syscallarg(int) iovcnt;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
	} */ *uap = v;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	off_t offset;
	int error, fd = SCARG(uap, fd);

	if ((u_int)fd >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[fd]) == NULL ||
	    (fp->f_flag & FREAD) == 0)
		return (EBADF);

	vp = (struct vnode *)fp->f_data;
	if (fp->f_type != DTYPE_VNODE
	    || vp->v_type == VFIFO)
		return (ESPIPE);

	offset = SCARG(uap, offset);

	/*
	 * XXX This works because no file systems actually
	 * XXX take any action on the seek operation.
	 */
	if ((error = VOP_SEEK(vp, fp->f_offset, offset, fp->f_cred)) != 0)
		return (error);

	return (dofilereadv32(p, fd, fp, (struct netbsd32_iovec *)(u_long)SCARG(uap, iovp), SCARG(uap, iovcnt),
	    &offset, 0, retval));
}

int
netbsd32_pwritev(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_pwritev_args /* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_iovecp_t) iovp;
		syscallarg(int) iovcnt;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
	} */ *uap = v;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	struct vnode *vp;
	off_t offset;
	int error, fd = SCARG(uap, fd);

	if ((u_int)fd >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[fd]) == NULL ||
	    (fp->f_flag & FWRITE) == 0)
		return (EBADF);

	vp = (struct vnode *)fp->f_data;
	if (fp->f_type != DTYPE_VNODE
	    || vp->v_type == VFIFO)
		return (ESPIPE);

	offset = SCARG(uap, offset);

	/*
	 * XXX This works because no file systems actually
	 * XXX take any action on the seek operation.
	 */
	if ((error = VOP_SEEK(vp, fp->f_offset, offset, fp->f_cred)) != 0)
		return (error);

	return (dofilewritev32(p, fd, fp, (struct netbsd32_iovec *)(u_long)SCARG(uap, iovp), SCARG(uap, iovcnt),
	    &offset, 0, retval));
}

/* ARGSUSED */
int
netbsd32___sigaction14(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32___sigaction14_args /* {
		syscallarg(int) signum;
		syscallarg(const struct sigaction *) nsa;
		syscallarg(struct sigaction *) osa;
	} */ *uap = v;
	struct netbsd32_sigaction sa32;
	struct sigaction nsa, osa;
	int error;

	if (SCARG(uap, nsa)) {
		error = copyin((caddr_t)(u_long)SCARG(uap, nsa), 
			       &sa32, sizeof(sa32));
		if (error)
			return (error);
		nsa.sa_handler = (void *)(u_long)sa32.sa_handler;
		nsa.sa_mask = sa32.sa_mask;
		nsa.sa_flags = sa32.sa_flags;
	}
	error = sigaction1(p, SCARG(uap, signum),
	    SCARG(uap, nsa) ? &nsa : 0, SCARG(uap, osa) ? &osa : 0);
	if (error)
		return (error);
	if (SCARG(uap, osa)) {
		sa32.sa_handler = (netbsd32_voidp)(u_long)osa.sa_handler;
		sa32.sa_mask = osa.sa_mask;
		sa32.sa_flags = osa.sa_flags;
		error = copyout(&sa32, (caddr_t)(u_long)SCARG(uap, osa), sizeof(sa32));
		if (error)
			return (error);
	}
	return (0);
}

int netbsd32___sigpending14(p, v, retval) 
	struct proc *p;
	void   *v;
	register_t *retval;
{
	struct netbsd32___sigpending14_args /* {
		syscallarg(sigset_t *) set;
	} */ *uap = v;
	struct sys___sigpending14_args ua;

	NETBSD32TOP_UAP(set, sigset_t);
	return (sys___sigpending14(p, &ua, retval));
}

int netbsd32___sigprocmask14(p, v, retval) 
	struct proc *p;
	void   *v;
	register_t *retval;
{
	struct netbsd32___sigprocmask14_args /* {
		syscallarg(int) how;
		syscallarg(const sigset_t *) set;
		syscallarg(sigset_t *) oset;
	} */ *uap = v;
	struct sys___sigprocmask14_args ua;

	NETBSD32TO64_UAP(how);
	NETBSD32TOP_UAP(set, sigset_t);
	NETBSD32TOP_UAP(oset, sigset_t);
	return (sys___sigprocmask14(p, &ua, retval));
}

int netbsd32___sigsuspend14(p, v, retval) 
	struct proc *p;
	void   *v;
	register_t *retval;
{
	struct netbsd32___sigsuspend14_args /* {
		syscallarg(const sigset_t *) set;
	} */ *uap = v;
	struct sys___sigsuspend14_args ua;

	NETBSD32TOP_UAP(set, sigset_t);
	return (sys___sigsuspend14(p, &ua, retval));
};


/*
 * Find pathname of process's current directory.
 *
 * Use vfs vnode-to-name reverse cache; if that fails, fall back
 * to reading directory contents.
 */
int
getcwd_common __P((struct vnode *, struct vnode *,
		   char **, char *, int, int, struct proc *));

int netbsd32___getcwd(p, v, retval) 
	struct proc *p;
	void   *v;
	register_t *retval;
{
	struct netbsd32___getcwd_args /* {
		syscallarg(char *) bufp;
		syscallarg(size_t) length;
	} */ *uap = v;

	int     error;
	char   *path;
	char   *bp, *bend;
	int     len = (int)SCARG(uap, length);
	int	lenused;

	if (len > MAXPATHLEN*4)
		len = MAXPATHLEN*4;
	else if (len < 2)
		return ERANGE;

	path = (char *)malloc(len, M_TEMP, M_WAITOK);
	if (!path)
		return ENOMEM;

	bp = &path[len];
	bend = bp;
	*(--bp) = '\0';

	/*
	 * 5th argument here is "max number of vnodes to traverse".
	 * Since each entry takes up at least 2 bytes in the output buffer,
	 * limit it to N/2 vnodes for an N byte buffer.
	 */
#define GETCWD_CHECK_ACCESS 0x0001
	error = getcwd_common (p->p_cwdi->cwdi_cdir, NULL, &bp, path, len/2,
			       GETCWD_CHECK_ACCESS, p);

	if (error)
		goto out;
	lenused = bend - bp;
	*retval = lenused;
	/* put the result into user buffer */
	error = copyout(bp, (caddr_t)(u_long)SCARG(uap, bufp), lenused);

out:
	free(path, M_TEMP);
	return error;
}

int netbsd32_fchroot(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_fchroot_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct sys_fchroot_args ua;
	
	NETBSD32TO64_UAP(fd);
	return (sys_fchroot(p, &ua, retval));
}

/*
 * Open a file given a file handle.
 *
 * Check permissions, allocate an open file structure,
 * and call the device open routine if any.
 */
int
netbsd32_fhopen(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_fhopen_args /* {
		syscallarg(const fhandle_t *) fhp;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys_fhopen_args ua;

	NETBSD32TOP_UAP(fhp, fhandle_t);
	NETBSD32TO64_UAP(flags);
	return (sys_fhopen(p, &ua, retval));
}

int netbsd32_fhstat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_fhstat_args /* {
		syscallarg(const netbsd32_fhandlep_t) fhp;
		syscallarg(struct stat *) sb;
	} */ *uap = v;
	struct sys_fhstat_args ua;

	NETBSD32TOP_UAP(fhp, const fhandle_t);
	NETBSD32TOP_UAP(sb, struct stat);
	return (sys_fhstat(p, &ua, retval));
}

int netbsd32_fhstatfs(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_fhstatfs_args /* {
		syscallarg(const netbsd32_fhandlep_t) fhp;
		syscallarg(struct statfs *) buf;
	} */ *uap = v;
	struct sys_fhstatfs_args ua;

	NETBSD32TOP_UAP(fhp, const fhandle_t);
	NETBSD32TOP_UAP(buf, struct statfs);
	return (sys_fhstatfs(p, &ua, retval));
}

/* virtual memory syscalls */
int
netbsd32_ovadvise(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_ovadvise_args /* {
		syscallarg(int) anom;
	} */ *uap = v;
	struct sys_ovadvise_args ua;

	NETBSD32TO64_UAP(anom);
	return (sys_ovadvise(p, &ua, retval));
}

