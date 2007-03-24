/*	$NetBSD: netbsd32_conv.h,v 1.14.20.2 2007/03/24 14:55:14 yamt Exp $	*/

/*
 * Copyright (c) 1998, 2001 Matthew R. Green
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

#ifndef _COMPAT_NETBSD32_NETBSD32_CONV_H_
#define _COMPAT_NETBSD32_NETBSD32_CONV_H_

/*
 * Though COMPAT_OLDSOCK is needed only for COMPAT_43, SunOS, Linux,
 * HP-UX, FreeBSD, Ultrix, OSF1, we define it unconditionally so that
 * this would be LKM-safe.
 */
#define COMPAT_OLDSOCK /* used by <sys/socket.h> */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#define msg __msg /* Don't ask me! */
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <sys/event.h>

#include <compat/sys/dirent.h>

#include <compat/netbsd32/netbsd32.h>

static __inline void netbsd32_from_timeval __P((const struct timeval *, struct netbsd32_timeval *));
static __inline void netbsd32_to_timeval __P((const struct netbsd32_timeval *, struct timeval *));
static __inline void netbsd32_from_itimerval __P((const struct itimerval *, struct netbsd32_itimerval *));
static __inline void netbsd32_to_itimerval __P((const struct netbsd32_itimerval *, struct itimerval *));
static __inline void netbsd32_to_timespec __P((const struct netbsd32_timespec *, struct timespec *));
static __inline void netbsd32_from_timespec __P((const struct timespec *, struct netbsd32_timespec *));
static __inline void netbsd32_from_rusage __P((const struct rusage *, struct netbsd32_rusage *));
static __inline void netbsd32_to_rusage __P((const struct netbsd32_rusage *, struct rusage *));
static __inline int netbsd32_to_iovecin __P((const struct netbsd32_iovec *, struct iovec *, int));
static __inline void netbsd32_to_msghdr __P((const struct netbsd32_msghdr *, struct msghdr *));
static __inline void netbsd32_from_msghdr __P((struct netbsd32_msghdr *, const struct msghdr *));
static __inline void netbsd32_from_statvfs __P((const struct statvfs *, struct netbsd32_statvfs *));
static __inline void netbsd32_from_timex __P((const struct timex *, struct netbsd32_timex *));
static __inline void netbsd32_to_timex __P((const struct netbsd32_timex *, struct timex *));
static __inline void netbsd32_from___stat13 __P((const struct stat *, struct netbsd32_stat13 *));
static __inline void netbsd32_from___stat30 __P((const struct stat *, struct netbsd32_stat *));
static __inline void netbsd32_to_ipc_perm __P((const struct netbsd32_ipc_perm *, struct ipc_perm *));
static __inline void netbsd32_from_ipc_perm __P((const struct ipc_perm *, struct netbsd32_ipc_perm *));
static __inline void netbsd32_to_msg __P((const struct netbsd32_msg *, struct msg *));
static __inline void netbsd32_from_msg __P((const struct msg *, struct netbsd32_msg *));
static __inline void netbsd32_to_msqid_ds __P((const struct netbsd32_msqid_ds *, struct msqid_ds *));
static __inline void netbsd32_from_msqid_ds __P((const struct msqid_ds *, struct netbsd32_msqid_ds *));
static __inline void netbsd32_to_shmid_ds __P((const struct netbsd32_shmid_ds *, struct shmid_ds *));
static __inline void netbsd32_from_shmid_ds __P((const struct shmid_ds *, struct netbsd32_shmid_ds *));
static __inline void netbsd32_to_semid_ds __P((const struct  netbsd32_semid_ds *, struct  semid_ds *));
static __inline void netbsd32_from_semid_ds __P((const struct  semid_ds *, struct  netbsd32_semid_ds *));
static __inline void netbsd32_from_loadavg __P((struct netbsd32_loadavg *, const struct loadavg *));
static __inline void netbsd32_to_sigevent(const struct netbsd32_sigevent *, struct sigevent *);
static __inline int  netbsd32_to_dirent12(char *, int);

/* converters for structures that we need */
static __inline void
netbsd32_from_timeval(tv, tv32)
	const struct timeval *tv;
	struct netbsd32_timeval *tv32;
{

	tv32->tv_sec = (netbsd32_long)tv->tv_sec;
	tv32->tv_usec = (netbsd32_long)tv->tv_usec;
}

static __inline void
netbsd32_to_timeval(tv32, tv)
	const struct netbsd32_timeval *tv32;
	struct timeval *tv;
{

	tv->tv_sec = (long)tv32->tv_sec;
	tv->tv_usec = (long)tv32->tv_usec;
}

static __inline void
netbsd32_from_itimerval(itv, itv32)
	const struct itimerval *itv;
	struct netbsd32_itimerval *itv32;
{

	netbsd32_from_timeval(&itv->it_interval,
			     &itv32->it_interval);
	netbsd32_from_timeval(&itv->it_value,
			     &itv32->it_value);
}

static __inline void
netbsd32_to_itimerval(itv32, itv)
	const struct netbsd32_itimerval *itv32;
	struct itimerval *itv;
{

	netbsd32_to_timeval(&itv32->it_interval, &itv->it_interval);
	netbsd32_to_timeval(&itv32->it_value, &itv->it_value);
}

static __inline void
netbsd32_to_timespec(s32p, p)
	const struct netbsd32_timespec *s32p;
	struct timespec *p;
{

	p->tv_sec = (time_t)s32p->tv_sec;
	p->tv_nsec = (long)s32p->tv_nsec;
}

static __inline void
netbsd32_from_timespec(p, s32p)
	const struct timespec *p;
	struct netbsd32_timespec *s32p;
{

	s32p->tv_sec = (netbsd32_time_t)p->tv_sec;
	s32p->tv_nsec = (netbsd32_long)p->tv_nsec;
}

static __inline void
netbsd32_from_rusage(rup, ru32p)
	const struct rusage *rup;
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
	const struct netbsd32_rusage *ru32p;
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
	const struct netbsd32_iovec *iov32p;
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
		if ((error = copyin(&iov32p->iov_base, &iov_base, sizeof(iov_base))))
		    return (error);
		if ((error = copyin(&iov32p->iov_len, &iov_len, sizeof(iov_len))))
		    return (error);
		iovp->iov_base = (void *)(u_long)iov_base;
		iovp->iov_len = (size_t)iov_len;
	}
	return error;
}

/* msg_iov must be done separately */
static __inline void
netbsd32_to_msghdr(mhp32, mhp)
	const struct netbsd32_msghdr *mhp32;
	struct msghdr *mhp;
{

	mhp->msg_name = NETBSD32PTR64(mhp32->msg_name);
	mhp->msg_namelen = mhp32->msg_namelen;
	mhp->msg_iovlen = (size_t)mhp32->msg_iovlen;
	mhp->msg_control = NETBSD32PTR64(mhp32->msg_control);
	mhp->msg_controllen = mhp32->msg_controllen;
	mhp->msg_flags = mhp32->msg_flags;
}

/* msg_iov must be done separately */
static __inline void
netbsd32_from_msghdr(mhp32, mhp)
	struct netbsd32_msghdr *mhp32;
	const struct msghdr *mhp;
{

	mhp32->msg_name = mhp32->msg_name;
	mhp32->msg_namelen = mhp32->msg_namelen;
	mhp32->msg_iovlen = mhp32->msg_iovlen;
	mhp32->msg_control = mhp32->msg_control;
	mhp32->msg_controllen = mhp->msg_controllen;
	mhp32->msg_flags = mhp->msg_flags;
}

static __inline void
netbsd32_from_statvfs(sbp, sb32p)
	const struct statvfs *sbp;
	struct netbsd32_statvfs *sb32p;
{
	sb32p->f_flag = sbp->f_flag;
	sb32p->f_bsize = (netbsd32_u_long)sbp->f_bsize;
	sb32p->f_frsize = (netbsd32_u_long)sbp->f_frsize;
	sb32p->f_iosize = (netbsd32_u_long)sbp->f_iosize;
	sb32p->f_blocks = sbp->f_blocks;
	sb32p->f_bfree = sbp->f_bfree;
	sb32p->f_bavail = sbp->f_bavail;
	sb32p->f_bresvd = sbp->f_bresvd;
	sb32p->f_files = sbp->f_files;
	sb32p->f_ffree = sbp->f_ffree;
	sb32p->f_favail = sbp->f_favail;
	sb32p->f_fresvd = sbp->f_fresvd;
	sb32p->f_syncreads = sbp->f_syncreads;
	sb32p->f_syncwrites = sbp->f_syncwrites;
	sb32p->f_asyncreads = sbp->f_asyncreads;
	sb32p->f_asyncwrites = sbp->f_asyncwrites;
	sb32p->f_fsidx = sbp->f_fsidx;
	sb32p->f_fsid = (netbsd32_u_long)sbp->f_fsid;
	sb32p->f_namemax = (netbsd32_u_long)sbp->f_namemax;
	sb32p->f_owner = sbp->f_owner;
	sb32p->f_spare[0] = 0;
	sb32p->f_spare[1] = 0;
	sb32p->f_spare[2] = 0;
	sb32p->f_spare[3] = 0;
#if 1
	/* May as well do the whole batch in one go */
	memcpy(sb32p->f_fstypename, sbp->f_fstypename,
	    sizeof(sb32p->f_fstypename) + sizeof(sb32p->f_mntonname) +
	    sizeof(sb32p->f_mntfromname));
#else
	/* If we want to be careful */
	memcpy(sb32p->f_fstypename, sbp->f_fstypename, sizeof(sb32p->f_fstypename));
	memcpy(sb32p->f_mntonname, sbp->f_mntonname, sizeof(sb32p->f_mntonname));
	memcpy(sb32p->f_mntfromname, sbp->f_mntfromname, sizeof(sb32p->f_mntfromname));
#endif
}

static __inline void
netbsd32_from_timex(txp, tx32p)
	const struct timex *txp;
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
	const struct netbsd32_timex *tx32p;
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
	const struct stat *sbp;
	struct netbsd32_stat13 *sb32p;
{
	sb32p->st_dev = sbp->st_dev;
	sb32p->st_ino = sbp->st_ino;
	sb32p->st_mode = sbp->st_mode;
	sb32p->st_nlink = sbp->st_nlink;
	sb32p->st_uid = sbp->st_uid;
	sb32p->st_gid = sbp->st_gid;
	sb32p->st_rdev = sbp->st_rdev;
	sb32p->st_size = sbp->st_size;
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
netbsd32_from___stat30(sbp, sb32p)
	const struct stat *sbp;
	struct netbsd32_stat *sb32p;
{
	sb32p->st_dev = sbp->st_dev;
	sb32p->st_ino = sbp->st_ino;
	sb32p->st_mode = sbp->st_mode;
	sb32p->st_nlink = sbp->st_nlink;
	sb32p->st_uid = sbp->st_uid;
	sb32p->st_gid = sbp->st_gid;
	sb32p->st_rdev = sbp->st_rdev;
	sb32p->st_size = sbp->st_size;
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
	const struct netbsd32_ipc_perm *ip32p;
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
	const struct ipc_perm *ipp;
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
	const struct netbsd32_msg *m32p;
	struct msg *mp;
{

	mp->msg_next = NETBSD32PTR64(m32p->msg_next);
	mp->msg_type = (long)m32p->msg_type;
	mp->msg_ts = m32p->msg_ts;
	mp->msg_spot = m32p->msg_spot;
}

static __inline void
netbsd32_from_msg(mp, m32p)
	const struct msg *mp;
	struct netbsd32_msg *m32p;
{

	NETBSD32PTR32(m32p->msg_next, mp->msg_next);
	m32p->msg_type = (netbsd32_long)mp->msg_type;
	m32p->msg_ts = mp->msg_ts;
	m32p->msg_spot = mp->msg_spot;
}

static __inline void
netbsd32_to_msqid_ds(ds32p, dsp)
	const struct netbsd32_msqid_ds *ds32p;
	struct msqid_ds *dsp;
{

	netbsd32_to_ipc_perm(&ds32p->msg_perm, &dsp->msg_perm);
	netbsd32_to_msg(NETBSD32PTR64(ds32p->_msg_first), dsp->_msg_first);
	netbsd32_to_msg(NETBSD32PTR64(ds32p->_msg_last), dsp->_msg_last);
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
	const struct msqid_ds *dsp;
	struct netbsd32_msqid_ds *ds32p;
{

	netbsd32_from_ipc_perm(&dsp->msg_perm, &ds32p->msg_perm);
	netbsd32_from_msg(dsp->_msg_first, NETBSD32PTR64(ds32p->_msg_first));
	netbsd32_from_msg(dsp->_msg_last, NETBSD32PTR64(ds32p->_msg_last));
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
	const struct netbsd32_shmid_ds *ds32p;
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
	dsp->_shm_internal = NETBSD32PTR64(ds32p->_shm_internal);
}

static __inline void
netbsd32_from_shmid_ds(dsp, ds32p)
	const struct shmid_ds *dsp;
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
	NETBSD32PTR32(ds32p->_shm_internal, dsp->_shm_internal);
}

static __inline void
netbsd32_to_semid_ds(s32dsp, dsp)
	const struct  netbsd32_semid_ds *s32dsp;
	struct  semid_ds *dsp;
{

	netbsd32_to_ipc_perm(&s32dsp->sem_perm, &dsp->sem_perm);
	dsp->_sem_base = NETBSD32PTR64(s32dsp->_sem_base);
	dsp->sem_nsems = s32dsp->sem_nsems;
	dsp->sem_otime = s32dsp->sem_otime;
	dsp->sem_ctime = s32dsp->sem_ctime;
}

static __inline void
netbsd32_from_semid_ds(dsp, s32dsp)
	const struct  semid_ds *dsp;
	struct  netbsd32_semid_ds *s32dsp;
{

	netbsd32_from_ipc_perm(&dsp->sem_perm, &s32dsp->sem_perm);
	NETBSD32PTR32(s32dsp->_sem_base, dsp->_sem_base);
	s32dsp->sem_nsems = dsp->sem_nsems;
	s32dsp->sem_otime = dsp->sem_otime;
	s32dsp->sem_ctime = dsp->sem_ctime;
}

static __inline void
netbsd32_from_loadavg(av32, av)
	struct netbsd32_loadavg *av32;
	const struct loadavg *av;
{

	av32->ldavg[0] = av->ldavg[0];
	av32->ldavg[1] = av->ldavg[1];
	av32->ldavg[2] = av->ldavg[2];
	av32->fscale = (netbsd32_long)av->fscale;
}

static __inline void
netbsd32_to_kevent(struct netbsd32_kevent *ke32, struct kevent *ke)
{
	ke->ident = ke32->ident;
	ke->filter = ke32->filter;
	ke->flags = ke32->flags;
	ke->fflags = ke32->fflags;
	ke->data = ke32->data;
	ke->udata = ke32->udata;
}

static __inline void
netbsd32_from_kevent(struct kevent *ke, struct netbsd32_kevent *ke32)
{
	ke32->ident = ke->ident;
	ke32->filter = ke->filter;
	ke32->flags = ke->flags;
	ke32->fflags = ke->fflags;
	ke32->data = ke->data;
	ke32->udata = ke->udata;
}

static __inline void
netbsd32_to_sigevent(const struct netbsd32_sigevent *ev32, struct sigevent *ev)
{
	ev->sigev_notify = ev32->sigev_notify;
	ev->sigev_signo = ev32->sigev_signo;
	/*
	 * XXX sival_ptr, sigev_notify_function and
	 *     sigev_notify_attributes are  currently unused
	 */
	ev->sigev_value.sival_int = ev32->sigev_value.sival_int;
	ev->sigev_notify_function = NETBSD32PTR64(ev32->sigev_notify_function);
	ev->sigev_notify_attributes = NETBSD32PTR64(ev32->sigev_notify_attributes);
}

static __inline int
netbsd32_to_dirent12(char *buf, int nbytes)
{
	struct dirent *ndp, *nndp, *endp;
	struct dirent12 *odp;

	odp = (struct dirent12 *)(void *)buf;
	ndp = (struct dirent *)(void *)buf;
	endp = (struct dirent *)(void *)&buf[nbytes];

	/*
	 * In-place conversion. This works because odp
	 * is smaller than ndp, but it has to be done
	 * in the right sequence.
	 */
	for (; ndp < endp; ndp = nndp) {
		nndp = _DIRENT_NEXT(ndp);
		odp->d_fileno = (u_int32_t)ndp->d_fileno;
		if (ndp->d_namlen >= sizeof(odp->d_name))
			odp->d_namlen = sizeof(odp->d_name) - 1;
		else
			odp->d_namlen = (u_int8_t)ndp->d_namlen;
		odp->d_type = ndp->d_type;
		(void)memcpy(odp->d_name, ndp->d_name, (size_t)odp->d_namlen);
		odp->d_name[odp->d_namlen] = '\0';
		odp->d_reclen = _DIRENT_SIZE(odp);
		odp = _DIRENT_NEXT(odp);
	}
	return ((char *)(void *)odp) - buf;
}

#endif /* _COMPAT_NETBSD32_NETBSD32_CONV_H_ */
