/*	$NetBSD: sparc32_netbsd.c,v 1.3 1998/08/29 18:16:57 eeh Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/timex.h>

#include <vm/vm.h>
#include <sys/syscallargs.h>

#include <net/if.h>

#include <compat/sparc32/sparc32.h>
#include <compat/sparc32/sparc32_syscallargs.h>

/* converters for structures that we need */
static __inline void
sparc32_from_timeval(tv, tv32)
	struct timeval *tv;
	struct sparc32_timeval *tv32;
{

	tv32->tv_sec = (sparc32_long)tv->tv_sec;
	tv32->tv_usec = (sparc32_long)tv->tv_usec;
}

static __inline void
sparc32_to_timeval(tv32, tv)
	struct sparc32_timeval *tv32;
	struct timeval *tv;
{

	tv->tv_sec = (long)tv32->tv_sec;
	tv->tv_usec = (long)tv32->tv_usec;
}

static __inline void
sparc32_from_itimerval(itv, itv32)
	struct itimerval *itv;
	struct sparc32_itimerval *itv32;
{

	sparc32_from_timeval(&itv->it_interval, &itv32->it_interval);
	sparc32_from_timeval(&itv->it_value, &itv32->it_value);
}

static __inline void
sparc32_to_itimerval(itv32, itv)
	struct sparc32_itimerval *itv32;
	struct itimerval *itv;
{

	sparc32_to_timeval(&itv->it_interval, &itv32->it_interval);
	sparc32_to_timeval(&itv->it_value, &itv32->it_value);
}

static __inline void
sparc32_to_timespec(s32p, p)
	struct sparc32_timespec *s32p;
	struct timespec *p;
{

	p->tv_sec = s32p->tv_sec;
	p->tv_nsec = (long)s32p->tv_nsec;
}

static __inline void
sparc32_from_timespec(p, s32p)
	struct timespec *p;
	struct sparc32_timespec *s32p;
{

	s32p->tv_sec = p->tv_sec;
	s32p->tv_nsec = (sparc32_long)p->tv_nsec;
}

static __inline void
sparc32_from_rusage(rup, ru32p)
	struct rusage *rup;
	struct sparc32_rusage *ru32p;
{

	sparc32_from_timeval(&rup->ru_utime, &ru32p->ru_utime);
	sparc32_from_timeval(&rup->ru_stime, &ru32p->ru_stime);
#define C(var)	ru32p->var = (sparc32_long)rup->var
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
sparc32_to_rusage(ru32p, rup)
	struct sparc32_rusage *ru32p;
	struct rusage *rup;
{

	sparc32_to_timeval(&ru32p->ru_utime, &rup->ru_utime);
	sparc32_to_timeval(&ru32p->ru_stime, &rup->ru_stime);
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

static __inline void
sparc32_to_iovec(iov32p, iovp, len)
	struct sparc32_iovec *iov32p;
	struct iovec *iovp;
	int len;
{
	int i;

	for (i = 0; i < len; i++, iovp++, iov32p++) {
		iovp->iov_base = (void *)(u_long)iov32p->iov_base;
		iovp->iov_len = (size_t)iov32p->iov_len;
	}
}

/* assumes that mhp's msg_iov has been allocated */
static __inline void
sparc32_to_msghdr(mhp32, mhp)
	struct sparc32_msghdr *mhp32;
	struct msghdr *mhp;
{

	mhp->msg_name = (caddr_t)(u_long)mhp32->msg_name;
	mhp->msg_namelen = mhp32->msg_namelen;
	mhp->msg_iovlen = (size_t)mhp32->msg_iovlen;
	mhp->msg_control = (caddr_t)(u_long)mhp32->msg_control;
	mhp->msg_controllen = mhp32->msg_controllen;
	mhp->msg_flags = mhp32->msg_flags;
	sparc32_to_iovec(mhp32->msg_iov, mhp->msg_iov, mhp->msg_iovlen);
}

static __inline void
sparc32_from_statfs(sbp, sb32p)
	struct statfs *sbp;
	struct sparc32_statfs *sb32p;
{

	sb32p->f_type = sbp->f_type;
	sb32p->f_flags = sbp->f_flags;
	sb32p->f_bsize = (sparc32_long)sbp->f_bsize;
	sb32p->f_iosize = (sparc32_long)sbp->f_iosize;
	sb32p->f_blocks = (sparc32_long)sbp->f_blocks;
	sb32p->f_bfree = (sparc32_long)sbp->f_bfree;
	sb32p->f_bavail = (sparc32_long)sbp->f_bavail;
	sb32p->f_files = (sparc32_long)sbp->f_files;
	sb32p->f_ffree = (sparc32_long)sbp->f_ffree;
	sb32p->f_fsid = sbp->f_fsid;
	sb32p->f_owner = sbp->f_owner;
	strncpy(sbp->f_fstypename, sb32p->f_fstypename, MFSNAMELEN);
	strncpy(sbp->f_fstypename, sb32p->f_fstypename, MNAMELEN);
	strncpy(sbp->f_fstypename, sb32p->f_mntfromname, MNAMELEN);
}

static __inline void
sparc32_from_timex(txp, tx32p)
	struct timex *txp;
	struct sparc32_timex *tx32p;
{

	tx32p->modes = txp->modes;
	tx32p->offset = (sparc32_long)txp->offset;
	tx32p->freq = (sparc32_long)txp->freq;
	tx32p->maxerror = (sparc32_long)txp->maxerror;
	tx32p->esterror = (sparc32_long)txp->esterror;
	tx32p->status = txp->status;
	tx32p->constant = (sparc32_long)txp->constant;
	tx32p->precision = (sparc32_long)txp->precision;
	tx32p->tolerance = (sparc32_long)txp->tolerance;
	tx32p->ppsfreq = (sparc32_long)txp->ppsfreq;
	tx32p->jitter = (sparc32_long)txp->jitter;
	tx32p->shift = txp->shift;
	tx32p->stabil = (sparc32_long)txp->stabil;
	tx32p->jitcnt = (sparc32_long)txp->jitcnt;
	tx32p->calcnt = (sparc32_long)txp->calcnt;
	tx32p->errcnt = (sparc32_long)txp->errcnt;
	tx32p->stbcnt = (sparc32_long)txp->stbcnt;
}

static __inline void
sparc32_to_timex(tx32p, txp)
	struct sparc32_timex *tx32p;
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
sparc32_from___stat13(sbp, sb32p)
	struct stat *sbp;
	struct sparc32_stat *sb32p;
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
	sb32p->st_atimespec.tv_sec = sbp->st_atimespec.tv_sec;
	sb32p->st_atimespec.tv_nsec = (sparc32_long)sbp->st_atimespec.tv_nsec;
	sb32p->st_mtimespec.tv_sec = sbp->st_mtimespec.tv_sec;
	sb32p->st_mtimespec.tv_nsec = (sparc32_long)sbp->st_mtimespec.tv_nsec;
	sb32p->st_ctimespec.tv_sec = sbp->st_ctimespec.tv_sec;
	sb32p->st_ctimespec.tv_nsec = (sparc32_long)sbp->st_ctimespec.tv_nsec;
	sb32p->st_blksize = sbp->st_blksize;
	sb32p->st_blocks = sbp->st_blocks;
	sb32p->st_flags = sbp->st_flags;
	sb32p->st_gen = sbp->st_gen;
}

static __inline void
sparc32_to_ipc_perm(ip32p, ipp)
	struct sparc32_ipc_perm *ip32p;
	struct ipc_perm *ipp;
{

	ipp->cuid = ip32p->cuid;
	ipp->cgid = ip32p->cgid;
	ipp->uid = ip32p->uid;
	ipp->gid = ip32p->gid;
	ipp->mode = ip32p->mode;
	ipp->seq = ip32p->seq;
	ipp->key = (key_t)ip32p->key;
}

static __inline void
sparc32_from_ipc_perm(ipp, ip32p)
	struct ipc_perm *ipp;
	struct sparc32_ipc_perm *ip32p;
{

	ip32p->cuid = ipp->cuid;
	ip32p->cgid = ipp->cgid;
	ip32p->uid = ipp->uid;
	ip32p->gid = ipp->gid;
	ip32p->mode = ipp->mode;
	ip32p->seq = ipp->seq;
	ip32p->key = (sparc32_key_t)ipp->key;
}

static __inline void
sparc32_to_msg(m32p, mp)
	struct sparc32_msg *m32p;
	struct msg *mp;
{

	mp->msg_next = (struct msg *)(u_long)m32p->msg_next;
	mp->msg_type = (long)m32p->msg_type;
	mp->msg_ts = m32p->msg_ts;
	mp->msg_spot = m32p->msg_spot;
}

static __inline void
sparc32_from_msg(mp, m32p)
	struct msg *mp;
	struct sparc32_msg *m32p;
{

	m32p->msg_next = (sparc32_msgp_t)(u_long)mp->msg_next;
	m32p->msg_type = (sparc32_long)mp->msg_type;
	m32p->msg_ts = mp->msg_ts;
	m32p->msg_spot = mp->msg_spot;
}

static __inline void
sparc32_to_msqid_ds(ds32p, dsp)
	struct sparc32_msqid_ds *ds32p;
	struct msqid_ds *dsp;
{

	sparc32_to_ipc_perm(&ds32p->msg_perm, &dsp->msg_perm);
	sparc32_to_msg(&ds32p->msg_first, &dsp->msg_first);
	sparc32_to_msg(&ds32p->msg_last, &dsp->msg_last);
	dsp->msg_cbytes = (u_long)ds32p->msg_cbytes;
	dsp->msg_qnum = (u_long)ds32p->msg_qnum;
	dsp->msg_qbytes = (u_long)ds32p->msg_qbytes;
	dsp->msg_lspid = ds32p->msg_lspid;
	dsp->msg_lrpid = ds32p->msg_lrpid;
	dsp->msg_rtime = (time_t)ds32p->msg_rtime;
	dsp->msg_stime = (time_t)ds32p->msg_stime;
	dsp->msg_ctime = (time_t)ds32p->msg_ctime;
}

static __inline void
sparc32_from_msqid_ds(dsp, ds32p)
	struct msqid_ds *dsp;
	struct sparc32_msqid_ds *ds32p;
{

	sparc32_from_ipc_perm(&dsp->msg_perm, &ds32p->msg_perm);
	sparc32_from_msg(&dsp->msg_first, &ds32p->msg_first);
	sparc32_from_msg(&dsp->msg_last, &ds32p->msg_last);
	ds32p->msg_cbytes = (sparc32_u_long)dsp->msg_cbytes;
	ds32p->msg_qnum = (sparc32_u_long)dsp->msg_qnum;
	ds32p->msg_qbytes = (sparc32_u_long)dsp->msg_qbytes;
	ds32p->msg_lspid = dsp->msg_lspid;
	ds32p->msg_lrpid = dsp->msg_lrpid;
	ds32p->msg_rtime = dsp->msg_rtime;
	ds32p->msg_stime = dsp->msg_stime;
	ds32p->msg_ctime = dsp->msg_ctime;
}

static __inline void
sparc32_to_shmid_ds(ds32p, dsp)
	struct sparc32_shmid_ds *ds32p;
	struct shmid_ds *dsp;
{

	sparc32_to_ipc_perm(&ds32p->shm_perm, &dsp->shm_perm);
	dsp->shm_segsz = ds32p->shm_segsz;
	dsp->shm_lpid = ds32p->shm_lpid;
	dsp->shm_cpid = ds32p->shm_cpid;
	dsp->shm_nattch = ds32p->shm_nattch;
	dsp->shm_atime = (long)ds32p->shm_atime;
	dsp->shm_dtime = (long)ds32p->shm_dtime;
	dsp->shm_ctime = (long)ds32p->shm_ctime;
	dsp->shm_internal = (void *)(u_long)ds32p->shm_internal;
}

static __inline void
sparc32_from_shmid_ds(dsp, ds32p)
	struct shmid_ds *dsp;
	struct sparc32_shmid_ds *ds32p;
{

	sparc32_from_ipc_perm(&dsp->shm_perm, &ds32p->shm_perm);
	ds32p->shm_segsz = dsp->shm_segsz;
	ds32p->shm_lpid = dsp->shm_lpid;
	ds32p->shm_cpid = dsp->shm_cpid;
	ds32p->shm_nattch = dsp->shm_nattch;
	ds32p->shm_atime = (sparc32_long)dsp->shm_atime;
	ds32p->shm_dtime = (sparc32_long)dsp->shm_dtime;
	ds32p->shm_ctime = (sparc32_long)dsp->shm_ctime;
	ds32p->shm_internal = (sparc32_voidp)(u_long)dsp->shm_internal;
}

static __inline void
sparc32_to_semid_ds(s32dsp, dsp)
	struct  sparc32_semid_ds *s32dsp;
	struct  semid_ds *dsp;
{

	sparc32_from_ipc_perm(&dsp->sem_perm, &s32dsp->sem_perm);
	dsp->sem_base = (struct sem *)(u_long)s32dsp->sem_base;
	dsp->sem_nsems = s32dsp->sem_nsems;
	dsp->sem_otime = s32dsp->sem_otime;
	dsp->sem_ctime = s32dsp->sem_ctime;
}

static __inline void
sparc32_from_semid_ds(dsp, s32dsp)
	struct  semid_ds *dsp;
	struct  sparc32_semid_ds *s32dsp;
{

	sparc32_to_ipc_perm(&s32dsp->sem_perm, &dsp->sem_perm);
	s32dsp->sem_base = (sparc32_semp_t)(u_long)dsp->sem_base;
	s32dsp->sem_nsems = dsp->sem_nsems;
	s32dsp->sem_otime = dsp->sem_otime;
	s32dsp->sem_ctime = dsp->sem_ctime;
}

/*
 * below are all the standard NetBSD system calls, in the 32bit
 * environment, witht he necessary conversions to 64bit before
 * calling the real syscall.
 */

int
compat_sparc32_read(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_read_args /* {
		syscallarg(int) fd;
		syscallarg(sparc32_voidp) buf;
		syscallarg(sparc32_size_t) nbyte;
	} */ *uap = v;
	struct sys_read_args ua;
	ssize_t rt;
	int error;

	SPARC32TO64_UAP(fd);
	SPARC32TOP_UAP(buf, void *);
	SPARC32TOX_UAP(nbyte, size_t);
	error = sys_read(p, &ua, (register_t *)&rt);
	*(sparc32_ssize_t *)retval = rt;

	return (error);
}

int
compat_sparc32_write(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_write_args /* {
		syscallarg(int) fd;
		syscallarg(const sparc32_voidp) buf;
		syscallarg(sparc32_size_t) nbyte;
	} */ *uap = v;
	struct sys_write_args ua;
	ssize_t rt;
	int error;

	SPARC32TO64_UAP(fd);
	SPARC32TOP_UAP(buf, void *);
	SPARC32TOX_UAP(nbyte, size_t);
	error = sys_write(p, &ua, (register_t *)&rt);
	*(sparc32_ssize_t *)retval = rt;

	return (error);
}

int
compat_sparc32_open(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_open_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(int) flags;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct sys_open_args ua;
	caddr_t sg;

	SPARC32TOP_UAP(path, const char);
	SPARC32TO64_UAP(flags);
	SPARC32TO64_UAP(mode);
	sg = stackgap_init(p->p_emul);
	SPARC32_CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));

	return (sys_open(p, &ua, retval));
}

int
compat_sparc32_wait4(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_wait4_args /* {
		syscallarg(int) pid;
		syscallarg(sparc32_intp) status;
		syscallarg(int) options;
		syscallarg(sparc32_rusagep_t) rusage;
	} */ *uap = v;
	struct sys_wait4_args ua;
	struct rusage ru;
	struct sparc32_rusage *ru32p;
	int error;

	SPARC32TO64_UAP(pid);
	SPARC32TOP_UAP(status, int);
	SPARC32TO64_UAP(options);
	ru32p = (struct sparc32_rusage *)(u_long)SCARG(uap, rusage);
	if (ru32p) {
		SCARG(&ua, rusage) = &ru;
		sparc32_to_rusage(ru32p, &ru);
	} else
		SCARG(&ua, rusage) = NULL;

	error = sys_wait4(p, &ua, retval);
	if (ru32p)
		sparc32_from_rusage(&ru, ru32p);

	return (error);
}

int
compat_sparc32_link(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_link_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(const sparc32_charp) link;
	} */ *uap = v;
	struct sys_link_args ua;

	SPARC32TOP_UAP(path, const char);
	SPARC32TOP_UAP(link, const char);
	return (sys_link(p, &ua, retval));
}

int
compat_sparc32_unlink(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_unlink_args /* {
		syscallarg(const sparc32_charp) path;
	} */ *uap = v;
	struct sys_unlink_args ua;

	SPARC32TOP_UAP(path, const char);

	return (sys_unlink(p, &ua, retval));
}

int
compat_sparc32_chdir(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_chdir_args /* {
		syscallarg(const sparc32_charp) path;
	} */ *uap = v;
	struct sys_chdir_args ua;

	SPARC32TOP_UAP(path, const char);

	return (sys_chdir(p, &ua, retval));
}

int
compat_sparc32_mknod(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_mknod_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(mode_t) mode;
		syscallarg(dev_t) dev;
	} */ *uap = v;
	struct sys_mknod_args ua;

	SPARC32TOP_UAP(path, const char);
	SPARC32TO64_UAP(dev);
	SPARC32TO64_UAP(mode);

	return (sys_mknod(p, &ua, retval));
}

int
compat_sparc32_chmod(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_chmod_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct sys_chmod_args ua;

	SPARC32TOP_UAP(path, const char);
	SPARC32TO64_UAP(mode);

	return (sys_chmod(p, &ua, retval));
}

int
compat_sparc32_chown(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_chown_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct sys_chown_args ua;

	SPARC32TOP_UAP(path, const char);
	SPARC32TO64_UAP(uid);
	SPARC32TO64_UAP(gid);

	return (sys_chown(p, &ua, retval));
}

int
compat_sparc32_break(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_break_args /* {
		syscallarg(sparc32_charp) nsize;
	} */ *uap = v;
	struct sys_obreak_args ua;

	SCARG(&ua, nsize) = (char *)(u_long)SCARG(uap, nsize);
	SPARC32TOP_UAP(nsize, char);
	return (sys_obreak(p, &ua, retval));
}

int
compat_sparc32_getfsstat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_getfsstat_args /* {
		syscallarg(sparc32_statfsp_t) buf;
		syscallarg(sparc32_long) bufsize;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys_getfsstat_args ua;
	struct statfs sb;
	struct sparc32_statfs *sb32p;
	int error;

	sb32p = (struct sparc32_statfs *)(u_long)SCARG(uap, buf);
	if (sb32p)
		SCARG(&ua, buf) = &sb;
	else
		SCARG(&ua, buf) = NULL;
	SPARC32TOX_UAP(bufsize, long);
	SPARC32TO64_UAP(flags);
	error = sys_getfsstat(p, &ua, retval);
	if (error)
		return (error);

	if (sb32p)
		sparc32_from_statfs(&sb, sb32p);
	return (0);
}

int
compat_sparc32_mount(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_mount_args /* {
		syscallarg(const sparc32_charp) type;
		syscallarg(const sparc32_charp) path;
		syscallarg(int) flags;
		syscallarg(sparc32_voidp) data;
	} */ *uap = v;
	struct sys_mount_args ua;

	SPARC32TOP_UAP(type, const char);
	SPARC32TOP_UAP(path, const char);
	SPARC32TO64_UAP(flags);
	SPARC32TOP_UAP(data, void);
	return (sys_mount(p, &ua, retval));
}

int
compat_sparc32_unmount(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_unmount_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys_unmount_args ua;

	SPARC32TOP_UAP(path, const char);
	SPARC32TO64_UAP(flags);
	return (sys_unmount(p, &ua, retval));
}

int
compat_sparc32_ptrace(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_ptrace_args /* {
		syscallarg(int) req;
		syscallarg(pid_t) pid;
		syscallarg(sparc32_caddr_t) addr;
		syscallarg(int) data;
	} */ *uap = v;
	struct sys_ptrace_args ua;

	SPARC32TO64_UAP(req);
	SPARC32TO64_UAP(pid);
	SPARC32TOX64_UAP(addr, caddr_t);
	SPARC32TO64_UAP(data);
	return (sys_ptrace(p, &ua, retval));
}

int
compat_sparc32_recvmsg(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_recvmsg_args /* {
		syscallarg(int) s;
		syscallarg(sparc32_msghdrp_t) msg;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys_recvmsg_args ua;
	struct msghdr mh;
	struct msghdr *mhp = &mh;
	struct sparc32_msghdr *mhp32;
	ssize_t rt;
	int error;

	SPARC32TO64_UAP(s);
	SPARC32TO64_UAP(flags);

	SCARG(&ua, msg) = mhp;
	mhp32 = (struct sparc32_msghdr *)(u_long)SCARG(uap, msg);
	/* sparc32_msghdr needs the iov pre-allocated */
	MALLOC(mhp->msg_iov, struct iovec *,
	    sizeof(struct iovec) * mhp32->msg_iovlen, M_TEMP, M_WAITOK);
	sparc32_to_msghdr(mhp32, mhp);

	error = sys_recvmsg(p, &ua, (register_t *)&rt);
	FREE(mhp->msg_iov, M_TEMP);
	*(sparc32_ssize_t *)retval = rt;
	return (error);
}

int
compat_sparc32_sendmsg(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_sendmsg_args /* {
		syscallarg(int) s;
		syscallarg(const sparc32_msghdrp_t) msg;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys_sendmsg_args ua;
	struct msghdr mh;
	struct msghdr *mhp = &mh;
	struct sparc32_msghdr *mhp32;
	ssize_t rt;
	int error;

	SPARC32TO64_UAP(s);
	SPARC32TO64_UAP(flags);

	SCARG(&ua, msg) = mhp;
	mhp32 = (struct sparc32_msghdr *)(u_long)SCARG(uap, msg);
	/* sparc32_msghdr needs the iov pre-allocated */
	MALLOC(mhp->msg_iov, struct iovec *,
	    sizeof(struct iovec) * mhp32->msg_iovlen, M_TEMP, M_WAITOK);
	sparc32_to_msghdr(mhp32, mhp);

	error = sys_sendmsg(p, &ua, (register_t *)&rt);
	FREE(mhp->msg_iov, M_TEMP);
	*(sparc32_ssize_t *)retval = rt;
	return (error);
}

int
compat_sparc32_recvfrom(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_recvfrom_args /* {
		syscallarg(int) s;
		syscallarg(sparc32_voidp) buf;
		syscallarg(sparc32_size_t) len;
		syscallarg(int) flags;
		syscallarg(sparc32_sockaddrp_t) from;
		syscallarg(sparc32_intp) fromlenaddr;
	} */ *uap = v;
	struct sys_recvfrom_args ua;
	off_t rt;
	int error;

	SPARC32TO64_UAP(s);
	SPARC32TOP_UAP(buf, void);
	SPARC32TOX_UAP(len, size_t);
	SPARC32TO64_UAP(flags);
	SPARC32TOP_UAP(from, struct sockaddr);
	SPARC32TOP_UAP(fromlenaddr, int);

	error = sys_recvfrom(p, &ua, (register_t *)&rt);
	*(sparc32_ssize_t *)retval = rt;
	return (error);
}

int
compat_sparc32_sendto(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_sendto_args /* {
		syscallarg(int) s;
		syscallarg(const sparc32_voidp) buf;
		syscallarg(sparc32_size_t) len;
		syscallarg(int) flags;
		syscallarg(const sparc32_sockaddrp_t) to;
		syscallarg(int) tolen;
	} */ *uap = v;
	struct sys_sendto_args ua;
	off_t rt;
	int error;

	SPARC32TO64_UAP(s);
	SPARC32TOP_UAP(buf, void);
	SPARC32TOX_UAP(len, size_t);
	SPARC32TO64_UAP(flags);
	SPARC32TOP_UAP(to, struct sockaddr);
	SPARC32TOX_UAP(tolen, int);

	error = sys_sendto(p, &ua, (register_t *)&rt);
	*(sparc32_ssize_t *)retval = rt;
	return (error);
}

int
compat_sparc32_socketpair(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_socketpair_args /* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
		syscallarg(sparc32_intp) rsv;
	} */ *uap = v;
	struct sys_socketpair_args ua;

	SPARC32TO64_UAP(domain);
	SPARC32TO64_UAP(type);
	SPARC32TO64_UAP(protocol);
	SPARC32TOP_UAP(rsv, int);
	return (sys_socketpair(p, &ua, retval));
}

int
compat_sparc32_accept(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_accept_args /* {
		syscallarg(int) s;
		syscallarg(sparc32_sockaddrp_t) name;
		syscallarg(sparc32_intp) anamelen;
	} */ *uap = v;
	struct sys_accept_args ua;

	SPARC32TO64_UAP(s);
	SPARC32TOP_UAP(name, struct sockaddr);
	SPARC32TOP_UAP(anamelen, int);
	return (sys_accept(p, &ua, retval));
}

int
compat_sparc32_getpeername(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_getpeername_args /* {
		syscallarg(int) fdes;
		syscallarg(sparc32_sockaddrp_t) asa;
		syscallarg(sparc32_intp) alen;
	} */ *uap = v;
	struct sys_getpeername_args ua;

	SPARC32TO64_UAP(fdes);
	SPARC32TOP_UAP(asa, struct sockaddr);
	SPARC32TOP_UAP(alen, int);
	return (sys_getpeername(p, &ua, retval));
}

int
compat_sparc32_getsockname(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_getsockname_args /* {
		syscallarg(int) fdes;
		syscallarg(sparc32_sockaddrp_t) asa;
		syscallarg(sparc32_intp) alen;
	} */ *uap = v;
	struct sys_getsockname_args ua;

	SPARC32TO64_UAP(fdes);
	SPARC32TOP_UAP(asa, struct sockaddr);
	SPARC32TOP_UAP(alen, int);
	return (sys_getsockname(p, &ua, retval));
}

int
compat_sparc32_access(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_access_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys_access_args ua;
	caddr_t sg;

	SPARC32TOP_UAP(path, const char);
	SPARC32TO64_UAP(flags);
	sg = stackgap_init(p->p_emul);
	SPARC32_CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));

	return (sys_access(p, &ua, retval));
}

int
compat_sparc32_chflags(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_chflags_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(sparc32_u_long) flags;
	} */ *uap = v;
	struct sys_chflags_args ua;

	SPARC32TOP_UAP(path, const char);
	SPARC32TO64_UAP(flags);

	return (sys_chflags(p, &ua, retval));
}

int
compat_sparc32_fchflags(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_fchflags_args /* {
		syscallarg(int) fd;
		syscallarg(sparc32_u_long) flags;
	} */ *uap = v;
	struct sys_fchflags_args ua;

	SPARC32TO64_UAP(fd);
	SPARC32TO64_UAP(flags);

	return (sys_fchflags(p, &ua, retval));
}

int
compat_sparc32_profil(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_profil_args /* {
		syscallarg(sparc32_caddr_t) samples;
		syscallarg(sparc32_size_t) size;
		syscallarg(sparc32_u_long) offset;
		syscallarg(u_int) scale;
	} */ *uap = v;
	struct sys_profil_args ua;

	SPARC32TOX64_UAP(samples, caddr_t);
	SPARC32TOX_UAP(size, size_t);
	SPARC32TOX_UAP(offset, u_long);
	SPARC32TO64_UAP(scale);
	return (sys_profil(p, &ua, retval));
}

int
compat_sparc32_ktrace(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_ktrace_args /* {
		syscallarg(const sparc32_charp) fname;
		syscallarg(int) ops;
		syscallarg(int) facs;
		syscallarg(int) pid;
	} */ *uap = v;
	struct sys_ktrace_args ua;

	SPARC32TOP_UAP(fname, const char);
	SPARC32TO64_UAP(ops);
	SPARC32TO64_UAP(facs);
	SPARC32TO64_UAP(pid);
	return (sys_ktrace(p, &ua, retval));
}

int
compat_sparc32_sigaction(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_sigaction_args /* {
		syscallarg(int) signum;
		syscallarg(const sparc32_sigactionp_t) nsa;
		syscallarg(sparc32_sigactionp_t) osa;
	} */ *uap = v;
	struct sys_sigaction_args ua;
	struct sigaction nsa, osa;
	struct sparc32_sigaction *sa32p;
	int error;

	SPARC32TO64_UAP(signum);
	if (SCARG(uap, osa))
		SCARG(&ua, osa) = &osa;
	else
		SCARG(&ua, osa) = NULL;
	if (SCARG(uap, nsa)) {
		SCARG(&ua, nsa) = &nsa;
		sa32p = (struct sparc32_sigaction *)(u_long)SCARG(uap, nsa);
		nsa.sa_handler = (void *)(u_long)sa32p->sa_handler;
		nsa.sa_mask = sa32p->sa_mask;
		nsa.sa_flags = sa32p->sa_flags;
	} else
		SCARG(&ua, nsa) = NULL;
	SCARG(&ua, nsa) = &osa;
	error = sys_sigaction(p, &ua, retval);
	if (error)
		return (error);

	if (SCARG(uap, osa)) {
		sa32p = (struct sparc32_sigaction *)(u_long)SCARG(uap, osa);
		sa32p->sa_handler = (sparc32_sigactionp_t)(u_long)osa.sa_handler;
		sa32p->sa_mask = osa.sa_mask;
		sa32p->sa_flags = osa.sa_flags;
	}

	return (0);
}

int
compat_sparc32___getlogin(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32___getlogin_args /* {
		syscallarg(sparc32_charp) namebuf;
		syscallarg(u_int) namelen;
	} */ *uap = v;
	struct sys___getlogin_args ua;

	SPARC32TOP_UAP(namebuf, char);
	SPARC32TO64_UAP(namelen);
	return (sys___getlogin(p, &ua, retval));
}

int
compat_sparc32_setlogin(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_setlogin_args /* {
		syscallarg(const sparc32_charp) namebuf;
	} */ *uap = v;
	struct sys_setlogin_args ua;

	SPARC32TOP_UAP(namebuf, char);
	return (sys_setlogin(p, &ua, retval));
}

int
compat_sparc32_acct(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_acct_args /* {
		syscallarg(const sparc32_charp) path;
	} */ *uap = v;
	struct sys_acct_args ua;

	SPARC32TOP_UAP(path, const char);
	return (sys_acct(p, &ua, retval));
}

int
compat_sparc32_revoke(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_revoke_args /* {
		syscallarg(const sparc32_charp) path;
	} */ *uap = v;
	struct sys_revoke_args ua;
	caddr_t sg;

	SPARC32TOP_UAP(path, const char);
	sg = stackgap_init(p->p_emul);
	SPARC32_CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));

	return (sys_revoke(p, &ua, retval));
}

int
compat_sparc32_symlink(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_symlink_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(const sparc32_charp) link;
	} */ *uap = v;
	struct sys_symlink_args ua;

	SPARC32TOP_UAP(path, const char);
	SPARC32TOP_UAP(link, const char);

	return (sys_symlink(p, &ua, retval));
}

int
compat_sparc32_readlink(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_readlink_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(sparc32_charp) buf;
		syscallarg(sparc32_size_t) count;
	} */ *uap = v;
	struct sys_readlink_args ua;
	caddr_t sg;

	SPARC32TOP_UAP(path, const char);
	SPARC32TOP_UAP(buf, char);
	SPARC32TOX_UAP(count, size_t);
	sg = stackgap_init(p->p_emul);
	SPARC32_CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));

	return (sys_readlink(p, &ua, retval));
}

int
compat_sparc32_execve(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_execve_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(sparc32_charpp) argp;
		syscallarg(sparc32_charpp) envp;
	} */ *uap = v;
	struct sys_execve_args ua;
	caddr_t sg;

	SPARC32TOP_UAP(path, const char);
	SPARC32TOP_UAP(argp, char *);
	SPARC32TOP_UAP(envp, char *);
	sg = stackgap_init(p->p_emul);
	SPARC32_CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));

	return (sys_execve(p, &ua, retval));
}

int
compat_sparc32_chroot(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_chroot_args /* {
		syscallarg(const sparc32_charp) path;
	} */ *uap = v;
	struct sys_chroot_args ua;

	SPARC32TOP_UAP(path, const char);
	return (sys_chroot(p, &ua, retval));
}

int
compat_sparc32_munmap(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_munmap_args /* {
		syscallarg(sparc32_voidp) addr;
		syscallarg(sparc32_size_t) len;
	} */ *uap = v;
	struct sys_munmap_args ua;

	SPARC32TOP_UAP(addr, void);
	SPARC32TOX_UAP(len, size_t);
	return (sys_munmap(p, &ua, retval));
}

int
compat_sparc32_mprotect(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_mprotect_args /* {
		syscallarg(sparc32_voidp) addr;
		syscallarg(sparc32_size_t) len;
		syscallarg(int) prot;
	} */ *uap = v;
	struct sys_mprotect_args ua;

	SPARC32TOP_UAP(addr, void);
	SPARC32TOX_UAP(len, size_t);
	SPARC32TO64_UAP(prot);
	return (sys_mprotect(p, &ua, retval));
}

int
compat_sparc32_madvise(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_madvise_args /* {
		syscallarg(sparc32_voidp) addr;
		syscallarg(sparc32_size_t) len;
		syscallarg(int) behav;
	} */ *uap = v;
	struct sys_madvise_args ua;

	SPARC32TOP_UAP(addr, void);
	SPARC32TOX_UAP(len, size_t);
	SPARC32TO64_UAP(behav);
	return (sys_madvise(p, &ua, retval));
}

int
compat_sparc32_mincore(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_mincore_args /* {
		syscallarg(sparc32_caddr_t) addr;
		syscallarg(sparc32_size_t) len;
		syscallarg(sparc32_charp) vec;
	} */ *uap = v;
	struct sys_mincore_args ua;

	SPARC32TOX64_UAP(addr, caddr_t);
	SPARC32TOX_UAP(len, size_t);
	SPARC32TOP_UAP(vec, char);
	return (sys_mincore(p, &ua, retval));
}

int
compat_sparc32_getgroups(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_getgroups_args /* {
		syscallarg(int) gidsetsize;
		syscallarg(sparc32_gid_tp) gidset;
	} */ *uap = v;
	struct sys_getgroups_args ua;

	SPARC32TO64_UAP(gidsetsize);
	SPARC32TOP_UAP(gidset, gid_t);
	return (sys_getgroups(p, &ua, retval));
}

int
compat_sparc32_setgroups(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_setgroups_args /* {
		syscallarg(int) gidsetsize;
		syscallarg(const sparc32_gid_tp) gidset;
	} */ *uap = v;
	struct sys_setgroups_args ua;

	SPARC32TO64_UAP(gidsetsize);
	SPARC32TOP_UAP(gidset, gid_t);
	return (sys_setgroups(p, &ua, retval));
}

int
compat_sparc32_setitimer(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_setitimer_args /* {
		syscallarg(int) which;
		syscallarg(const sparc32_itimervalp_t) itv;
		syscallarg(sparc32_itimervalp_t) oitv;
	} */ *uap = v;
	struct sys_setitimer_args ua;
	struct itimerval iit, oit;
	struct sparc32_itimerval *s32itp;
	int error;

	SPARC32TO64_UAP(which);
	s32itp = (struct sparc32_itimerval *)(u_long)SCARG(uap, itv);
	if (s32itp == NULL) {
		SCARG(&ua, itv) = &iit;
		sparc32_to_itimerval(s32itp, &iit);
	} else
		SCARG(&ua, itv) = NULL;
	s32itp = (struct sparc32_itimerval *)(u_long)SCARG(uap, oitv);
	if (s32itp)
		SCARG(&ua, oitv) = &oit;
	else
		SCARG(&ua, oitv) = NULL;

	error = sys_setitimer(p, &ua, retval);
	if (error)
		return (error);

	if (s32itp)
		sparc32_from_itimerval(&oit, s32itp);
	return (0);
}

int
compat_sparc32_getitimer(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_getitimer_args /* {
		syscallarg(int) which;
		syscallarg(sparc32_itimervalp_t) itv;
	} */ *uap = v;
	struct sys_getitimer_args ua;
	struct itimerval it;
	struct sparc32_itimerval *s32itp;
	int error;

	SPARC32TO64_UAP(which);
	s32itp = (struct sparc32_itimerval *)(u_long)SCARG(uap, itv);
	if (s32itp == NULL)
		SCARG(&ua, itv) = &it;
	else
		SCARG(&ua, itv) = NULL;

	error = sys_getitimer(p, &ua, retval);
	if (error)
		return (error);

	if (s32itp)
		sparc32_from_itimerval(&it, s32itp);
	return (0);
}

int
compat_sparc32_fcntl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_fcntl_args /* {
		syscallarg(int) fd;
		syscallarg(int) cmd;
		syscallarg(sparc32_voidp) arg;
	} */ *uap = v;
	struct sys_fcntl_args ua;

	SPARC32TO64_UAP(fd);
	SPARC32TO64_UAP(cmd);
	SPARC32TOP_UAP(arg, void);
	return (sys_fcntl(p, &ua, retval));
}

int
compat_sparc32_select(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_select_args /* {
		syscallarg(int) nd;
		syscallarg(sparc32_fd_setp_t) in;
		syscallarg(sparc32_fd_setp_t) ou;
		syscallarg(sparc32_fd_setp_t) ex;
		syscallarg(sparc32_timevalp_t) tv;
	} */ *uap = v;
	struct sys_select_args ua;
	struct timeval tv;
	struct sparc32_timeval *tv32p;
	int error;

	SPARC32TO64_UAP(nd);
	SPARC32TOP_UAP(in, fd_set);
	SPARC32TOP_UAP(ou, fd_set);
	SPARC32TOP_UAP(ex, fd_set);
	tv32p = (struct sparc32_timeval *)(u_long)SCARG(uap, tv);
	if (tv32p)
		sparc32_to_timeval(tv32p, &tv);
	SCARG(&ua, tv) = &tv;

	error = sys_select(p, &ua, retval);
	if (tv32p)
		sparc32_from_timeval(&tv, tv32p);

	return (error);
}

int
compat_sparc32_connect(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_connect_args /* {
		syscallarg(int) s;
		syscallarg(const sparc32_sockaddrp_t) name;
		syscallarg(int) namelen;
	} */ *uap = v;
	struct sys_connect_args ua;

	SPARC32TO64_UAP(s);
	SPARC32TOP_UAP(name, struct sockaddr);
	SPARC32TO64_UAP(namelen);
	return (sys_connect(p, &ua, retval));
}

#undef DEBUG
int
compat_sparc32_sigreturn(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_sigreturn_args /* {
		syscallarg(struct sparc32_sigcontext *) sigcntxp;
	} */ *uap = v;
	struct sparc32_sigcontext *scp;
	struct sparc32_sigcontext sc;
	register struct trapframe *tf;
	struct rwindow32 *rwstack, *kstack;

	/* First ensure consistent stack state (see sendsig). */
	write_user_windows();
	if (rwindow_save(p)) {
#ifdef DEBUG
		printf("sigreturn: rwindow_save(%p) failed, sending SIGILL\n", p);
		Debugger();
#endif
		sigexit(p, SIGILL);
	}
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW) {
		printf("sigreturn: %s[%d], sigcntxp %p\n",
		    p->p_comm, p->p_pid, SCARG(uap, sigcntxp));
		if (sigdebug & SDB_DDB) Debugger();
	}
#endif
	scp = SCARG(uap, sigcntxp);
 	if ((int)scp & 3 || (copyin((caddr_t)scp, &sc, sizeof sc) != 0))
#ifdef DEBUG
	{
		printf("sigreturn: copyin failed\n");
		Debugger();
		return (EINVAL);
	}
#else
		return (EINVAL);
#endif
	tf = p->p_md.md_tf;
	/*
	 * Only the icc bits in the psr are used, so it need not be
	 * verified.  pc and npc must be multiples of 4.  This is all
	 * that is required; if it holds, just do it.
	 */
	if (((sc.sc_pc | sc.sc_npc) & 3) != 0)
#ifdef DEBUG
	{
		printf("sigreturn: pc %p or npc %p invalid\n", sc.sc_pc, sc.sc_npc);
		Debugger();
		return (EINVAL);
	}
#else
		return (EINVAL);
#endif
	/* take only psr ICC field */
	tf->tf_tstate = (int64_t)(tf->tf_tstate & ~TSTATE_CCR) | PSRCC_TO_TSTATE(sc.sc_psr);
	tf->tf_pc = (int64_t)sc.sc_pc;
	tf->tf_npc = (int64_t)sc.sc_npc;
	tf->tf_global[1] = (int64_t)sc.sc_g1;
	tf->tf_out[0] = (int64_t)sc.sc_o0;
	tf->tf_out[6] = (int64_t)sc.sc_sp;
	rwstack = (struct rwindow32 *)tf->tf_out[6];
	kstack = (struct rwindow32 *)(((caddr_t)tf)-CCFSZ);
	for (i=0; i<8; i++) {
		int tmp;
		if (copyin((caddr_t)&rwstack->rw_local[i], &tmp, sizeof tmp)) {
			printf("sigreturn: cannot load \%l%d from %p\n", i, &rwstack->rw_local[i]);
			Debugger();
		}
		tf->tf_local[i] = (int64_t)tmp;
		if (copyin((caddr_t)&rwstack->rw_in[i], &tmp, sizeof tmp)) {
			printf("sigreturn: cannot load \%i%d from %p\n", i, &rwstack->rw_in[i]);
			Debugger();
		}
		tf->tf_in[i] = (int)tmp;
	}
#ifdef DEBUG
	/* Need to sync tf locals and ins with stack to prevent panic */
	{
		int i;

		kstack = (struct rwindow32 *)tf->tf_out[6];
		for (i=0; i<8; i++) {
			tf->tf_local[i] = fuword(&kstack->rw_local[i]);
			tf->tf_in[i] = fuword(&kstack->rw_in[i]);
		}
	}
#endif
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW) {
		printf("sys_sigreturn: return trapframe pc=%p sp=%p tstate=%x\n",
		       (int)tf->tf_pc, (int)tf->tf_out[6], (int)tf->tf_tstate);
		if (sigdebug & SDB_DDB) Debugger();
	}
#endif
	if (sc.sc_onstack & 1)
		p->p_sigacts->ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigacts->ps_sigstk.ss_flags &= ~SS_ONSTACK;
	p->p_sigmask = sc.sc_mask & ~sigcantmask;
	return (EJUSTRETURN);
}


int
compat_sparc32_bind(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_bind_args /* {
		syscallarg(int) s;
		syscallarg(const sparc32_sockaddrp_t) name;
		syscallarg(int) namelen;
	} */ *uap = v;
	struct sys_connect_args ua;

	SPARC32TO64_UAP(s);
	SPARC32TOP_UAP(name, struct sockaddr);
	SPARC32TO64_UAP(namelen);
	return (sys_connect(p, &ua, retval));
}

int
compat_sparc32_setsockopt(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_setsockopt_args /* {
		syscallarg(int) s;
		syscallarg(int) level;
		syscallarg(int) name;
		syscallarg(const sparc32_voidp) val;
		syscallarg(int) valsize;
	} */ *uap = v;
	struct sys_setsockopt_args ua;

	SPARC32TO64_UAP(s);
	SPARC32TO64_UAP(level);
	SPARC32TO64_UAP(name);
	SPARC32TOP_UAP(val, void);
	SPARC32TO64_UAP(valsize);
	return (sys_setsockopt(p, &ua, retval));
}

int
compat_sparc32_gettimeofday(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_gettimeofday_args /* {
		syscallarg(sparc32_timevalp_t) tp;
		syscallarg(sparc32_timezonep_t) tzp;
	} */ *uap = v;
	struct sys_gettimeofday_args ua;
	struct timeval tv;
	struct sparc32_timeval *tv32p;
	int error;

	tv32p = (struct sparc32_timeval *)(u_long)SCARG(uap, tp);
	if (tv32p) {
		SCARG(&ua, tp) = &tv;
		sparc32_to_timeval(tv32p, &tv);
	} else
		SCARG(&ua, tp) = NULL;
	SPARC32TOP_UAP(tzp, struct timezone)
		
	error = sys_gettimeofday(p, &ua, retval);
	if (error)
		return (error);

	if (tv32p)
		sparc32_from_timeval(&tv, tv32p);
	
	return (0);
}

int
compat_sparc32_settimeofday(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_settimeofday_args /* {
		syscallarg(const sparc32_timevalp_t) tv;
		syscallarg(const sparc32_timezonep_t) tzp;
	} */ *uap = v;
	struct sys_settimeofday_args ua;
	struct timeval tv;
	struct sparc32_timeval *tv32p;
	int error;

	tv32p = (struct sparc32_timeval *)(u_long)SCARG(uap, tv);
	if (tv32p) {
		SCARG(&ua, tv) = &tv;
		sparc32_to_timeval(tv32p, &tv);
	} else
		SCARG(&ua, tv) = NULL;
	SPARC32TOP_UAP(tzp, struct timezone)
		
	error = sys_settimeofday(p, &ua, retval);
	if (error)
		return (error);

	if (tv32p)
		sparc32_from_timeval(&tv, tv32p);
	
	return (0);
}

int
compat_sparc32_getrusage(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_getrusage_args /* {
		syscallarg(int) who;
		syscallarg(sparc32_rusagep_t) rusage;
	} */ *uap = v;
	struct sys_getrusage_args ua;
	struct rusage ru;
	struct sparc32_rusage *ru32p;
	int error;

	SPARC32TO64_UAP(who);
	ru32p = (struct sparc32_rusage *)(u_long)SCARG(uap, rusage);
	if (ru32p) {
		SCARG(&ua, rusage) = &ru;
		sparc32_to_rusage(ru32p, &ru);
	} else
		SCARG(&ua, rusage) = NULL;

	error = sys_getrusage(p, &ua, retval);
	if (ru32p)
		sparc32_from_rusage(&ru, ru32p);

	return (error);
}

int
compat_sparc32_getsockopt(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_getsockopt_args /* {
		syscallarg(int) s;
		syscallarg(int) level;
		syscallarg(int) name;
		syscallarg(sparc32_voidp) val;
		syscallarg(sparc32_intp) avalsize;
	} */ *uap = v;
	struct sys_getsockopt_args ua;

	SPARC32TO64_UAP(s);
	SPARC32TO64_UAP(level);
	SPARC32TO64_UAP(name);
	SPARC32TOP_UAP(val, void)
	SPARC32TOP_UAP(avalsize, int);
	return (sys_getsockopt(p, &ua, retval));
}

int
compat_sparc32_readv(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_readv_args /* {
		syscallarg(int) fd;
		syscallarg(const sparc32_iovecp_t) iovp;
		syscallarg(int) iovcnt;
	} */ *uap = v;
	struct sys_readv_args ua;
	struct iovec *iov;
	ssize_t rt;
	int error;

	SPARC32TO64_UAP(fd)
	SPARC32TO64_UAP(iovcnt);
	MALLOC(iov, struct iovec *,
	    sizeof(struct iovec) * SCARG(uap, iovcnt), M_TEMP, M_WAITOK);
	sparc32_to_iovec((struct sparc32_iovec *)(u_long)SCARG(uap, iovp), iov,
	    SCARG(uap, iovcnt));
	SCARG(&ua, iovp) = iov;

	error = sys_readv(p, &ua, (register_t *)&rt);
	FREE(iov, M_TEMP);
	*(sparc32_ssize_t *)retval = rt;
	return (error);
}

int
compat_sparc32_writev(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_writev_args /* {
		syscallarg(int) fd;
		syscallarg(const sparc32_iovecp_t) iovp;
		syscallarg(int) iovcnt;
	} */ *uap = v;
	struct sys_writev_args ua;
	struct iovec *iov;
	ssize_t rt;
	int error;

	SPARC32TO64_UAP(fd)
	SPARC32TO64_UAP(iovcnt);
	MALLOC(iov, struct iovec *, sizeof(struct iovec) * SCARG(uap, iovcnt),
	    M_TEMP, M_WAITOK);
	sparc32_to_iovec((struct sparc32_iovec *)(u_long)SCARG(uap, iovp), iov,
	    SCARG(uap, iovcnt));
	SCARG(&ua, iovp) = iov;

	error = sys_writev(p, &ua, (register_t *)&rt);
	FREE(iov, M_TEMP);
	*(sparc32_ssize_t *)retval = rt;
	return (error);
}

int
compat_sparc32_rename(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_rename_args /* {
		syscallarg(const sparc32_charp) from;
		syscallarg(const sparc32_charp) to;
	} */ *uap = v;
	struct sys_rename_args ua;

	SPARC32TOP_UAP(from, const char)
	SPARC32TOP_UAP(to, const char)
	return (sys_rename(p, &ua, retval));
}

int
compat_sparc32_mkfifo(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_mkfifo_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct sys_mkfifo_args ua;

	SPARC32TOP_UAP(path, const char)
	SPARC32TO64_UAP(mode);
	return (sys_mkfifo(p, &ua, retval));
}

int
compat_sparc32_mkdir(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_mkdir_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct sys_mkdir_args ua;

	SPARC32TOP_UAP(path, const char)
	SPARC32TO64_UAP(mode);
	return (sys_mkdir(p, &ua, retval));
}

int
compat_sparc32_rmdir(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_rmdir_args /* {
		syscallarg(const sparc32_charp) path;
	} */ *uap = v;
	struct sys_rmdir_args ua;

	SPARC32TOP_UAP(path, const char);
	return (sys_rmdir(p, &ua, retval));
}

int
compat_sparc32_utimes(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_utimes_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(const sparc32_timevalp_t) tptr;
	} */ *uap = v;
	struct sys_utimes_args ua;
	struct timeval tv;
	struct sparc32_timeval *tv32p;

	SPARC32TOP_UAP(path, const char);
	tv32p = (struct sparc32_timeval *)(u_long)SCARG(uap, tptr);
	if (tv32p) {
		SCARG(&ua, tptr) = &tv;
		sparc32_to_timeval(tv32p, &tv);
	} else
		SCARG(&ua, tptr) = NULL;

	return (sys_utimes(p, &ua, retval));
}

int
compat_sparc32_adjtime(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_adjtime_args /* {
		syscallarg(const sparc32_timevalp_t) delta;
		syscallarg(sparc32_timevalp_t) olddelta;
	} */ *uap = v;
	struct sys_adjtime_args ua;
	struct timeval tv, otv;
	struct sparc32_timeval *tv32p, *otv32p;
	int error;

	tv32p = (struct sparc32_timeval *)(u_long)SCARG(uap, delta);
	otv32p = (struct sparc32_timeval *)(u_long)SCARG(uap, olddelta);

	if (tv32p) {
		SCARG(&ua, delta) = &tv;
		sparc32_to_timeval(tv32p, &tv);
	} else
		SCARG(&ua, delta) = NULL;
	if (otv32p)
		SCARG(&ua, olddelta) = &otv;
	else
		SCARG(&ua, olddelta) = NULL;
	error = sys_adjtime(p, &ua, retval);
	if (error)
		return (error);

	if (otv32p)
		sparc32_from_timeval(&otv, otv32p);
	return (0);
}

int
compat_sparc32_quotactl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_quotactl_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(int) cmd;
		syscallarg(int) uid;
		syscallarg(sparc32_caddr_t) arg;
	} */ *uap = v;
	struct sys_quotactl_args ua;

	SPARC32TOP_UAP(path, const char);
	SPARC32TO64_UAP(cmd);
	SPARC32TO64_UAP(uid);
	SPARC32TOX64_UAP(arg, caddr_t);
	return (sys_quotactl(p, &ua, retval));
}

int
compat_sparc32_nfssvc(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_nfssvc_args /* {
		syscallarg(int) flag;
		syscallarg(sparc32_voidp) argp;
	} */ *uap = v;
	struct sys_nfssvc_args ua;

	SPARC32TO64_UAP(flag);
	SPARC32TOP_UAP(argp, void);
	return (sys_nfssvc(p, &ua, retval));
}

int
compat_sparc32_statfs(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_statfs_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(sparc32_statfsp_t) buf;
	} */ *uap = v;
	struct sys_statfs_args ua;
	struct statfs sb;
	struct sparc32_statfs *sb32p;
	caddr_t sg;
	int error;

	SPARC32TOP_UAP(path, const char);
	sb32p = (struct sparc32_statfs *)(u_long)SCARG(uap, buf);
	if (sb32p)
		SCARG(&ua, buf) = &sb;
	else
		SCARG(&ua, buf) = NULL;
	sg = stackgap_init(p->p_emul);
	SPARC32_CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));
	error = sys_statfs(p, &ua, retval);
	if (error)
		return (error);

	if (sb32p)
		sparc32_from_statfs(&sb, sb32p);
	return (0);
}

int
compat_sparc32_fstatfs(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_fstatfs_args /* {
		syscallarg(int) fd;
		syscallarg(sparc32_statfsp_t) buf;
	} */ *uap = v;
	struct sys_fstatfs_args ua;
	struct statfs sb;
	struct sparc32_statfs *sb32p;
	int error;

	SPARC32TO64_UAP(fd);
	sb32p = (struct sparc32_statfs *)(u_long)SCARG(uap, buf);
	if (sb32p)
		SCARG(&ua, buf) = &sb;
	else
		SCARG(&ua, buf) = NULL;
	error = sys_fstatfs(p, &ua, retval);
	if (error)
		return (error);

	if (sb32p)
		sparc32_from_statfs(&sb, sb32p);
	return (0);
}

int
compat_sparc32_getfh(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_getfh_args /* {
		syscallarg(const sparc32_charp) fname;
		syscallarg(sparc32_fhandlep_t) fhp;
	} */ *uap = v;
	struct sys_getfh_args ua;

	SPARC32TOP_UAP(fname, const char);
	SPARC32TOP_UAP(fhp, struct fhandle);
	return (sys_getfh(p, &ua, retval));
}

int
compat_sparc32_sysarch(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_sysarch_args /* {
		syscallarg(int) op;
		syscallarg(sparc32_voidp) parms;
	} */ *uap = v;
	struct sys_sysarch_args ua;

	SPARC32TO64_UAP(op);
	SPARC32TOP_UAP(parms, void);
	return (sys_getfh(p, &ua, retval));
}

int
compat_sparc32_pread(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_pread_args /* {
		syscallarg(int) fd;
		syscallarg(sparc32_voidp) buf;
		syscallarg(sparc32_size_t) nbyte;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
	} */ *uap = v;
	struct sys_pread_args ua;
	ssize_t rt;
	int error;

	SPARC32TO64_UAP(fd);
	SPARC32TOP_UAP(buf, void);
	SPARC32TOX_UAP(nbyte, size_t);
	SPARC32TO64_UAP(pad);
	SPARC32TO64_UAP(offset);
	error = sys_pread(p, &ua, (register_t *)&rt);
	*(sparc32_ssize_t *)retval = rt;
	return (error);
}

int
compat_sparc32_pwrite(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_pwrite_args /* {
		syscallarg(int) fd;
		syscallarg(const sparc32_voidp) buf;
		syscallarg(sparc32_size_t) nbyte;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
	} */ *uap = v;
	struct sys_pwrite_args ua;
	ssize_t rt;
	int error;

	SPARC32TO64_UAP(fd);
	SPARC32TOP_UAP(buf, void);
	SPARC32TOX_UAP(nbyte, size_t);
	SPARC32TO64_UAP(pad);
	SPARC32TO64_UAP(offset);
	error = sys_pwrite(p, &ua, (register_t *)&rt);
	*(sparc32_ssize_t *)retval = rt;
	return (error);
}

int
compat_sparc32_ntp_gettime(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_ntp_gettime_args /* {
		syscallarg(sparc32_ntptimevalp_t) ntvp;
	} */ *uap = v;
	struct sys_ntp_gettime_args ua;
	struct ntptimeval ntv;
	struct sparc32_ntptimeval *ntv32p;
	int error;

	ntv32p = (struct sparc32_ntptimeval *)(u_long)SCARG(uap, ntvp);
	if (ntv32p)
		SCARG(&ua, ntvp) = &ntv;
	else
		SCARG(&ua, ntvp) = NULL;
	error = sys_ntp_gettime(p, &ua, retval);
	if (error)
		return (error);

	if (ntv32p) {
		sparc32_from_timeval(&ntv, ntv32p);
		ntv32p->maxerror = (sparc32_long)ntv.maxerror;
		ntv32p->esterror = (sparc32_long)ntv.esterror;
	}
	return (0);
}

int
compat_sparc32_ntp_adjtime(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_ntp_adjtime_args /* {
		syscallarg(sparc32_timexp_t) tp;
	} */ *uap = v;
	struct sys_ntp_adjtime_args ua;
	struct timex tx;
	struct sparc32_timex *tx32p;
	int error;

	tx32p = (struct sparc32_timex *)(u_long)SCARG(uap, tp);
	if (tx32p) {
		SCARG(&ua, tp) = &tx;
		sparc32_to_timex(tx32p, &tx);
	} else
		SCARG(&ua, tp) = NULL;
	error = sys_ntp_adjtime(p, &ua, retval);
	if (error)
		return (error);

	if (tx32p)
		sparc32_from_timeval(&tx, tx32p);
	return (0);
}

int
compat_sparc32_lfs_bmapv(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_lfs_bmapv_args /* {
		syscallarg(sparc32_fsid_tp_t) fsidp;
		syscallarg(sparc32_block_infop_t) blkiov;
		syscallarg(int) blkcnt;
	} */ *uap = v;
#if 0
	struct sys_lfs_bmapv_args ua;

	SPARC32TOP_UAP(fdidp, struct fsid);
	SPARC32TO64_UAP(blkcnt);
	/* XXX finish me */
#else

	return (ENOSYS);	/* XXX */
#endif
}

int
compat_sparc32_lfs_markv(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_lfs_markv_args /* {
		syscallarg(sparc32_fsid_tp_t) fsidp;
		syscallarg(sparc32_block_infop_t) blkiov;
		syscallarg(int) blkcnt;
	} */ *uap = v;

	return (ENOSYS);	/* XXX */
}

int
compat_sparc32_lfs_segclean(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_lfs_segclean_args /* {
		syscallarg(sparc32_fsid_tp_t) fsidp;
		syscallarg(sparc32_u_long) segment;
	} */ *uap = v;
	return (ENOSYS);	/* XXX */
}

int
compat_sparc32_lfs_segwait(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_lfs_segwait_args /* {
		syscallarg(sparc32_fsid_tp_t) fsidp;
		syscallarg(sparc32_timevalp_t) tv;
	} */ *uap = v;
	return (ENOSYS);	/* XXX */
}

int
compat_sparc32_pathconf(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_pathconf_args /* {
		syscallarg(int) fd;
		syscallarg(int) name;
	} */ *uap = v;
	struct sys_pathconf_args ua;
	long rt;
	int error;

	SPARC32TOP_UAP(path, const char);
	SPARC32TO64_UAP(name);
	error = sys_pathconf(p, &ua, (register_t *)&rt);
	*(sparc32_long *)retval = (sparc32_long)rt;
	return (error);
}

int
compat_sparc32_fpathconf(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_fpathconf_args /* {
		syscallarg(int) fd;
		syscallarg(int) name;
	} */ *uap = v;
	struct sys_fpathconf_args ua;
	long rt;
	int error;

	SPARC32TO64_UAP(fd);
	SPARC32TO64_UAP(name);
	error = sys_fpathconf(p, &ua, (register_t *)&rt);
	*(sparc32_long *)retval = (sparc32_long)rt;
	return (error);
}

int
compat_sparc32_getrlimit(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_getrlimit_args /* {
		syscallarg(int) which;
		syscallarg(sparc32_rlimitp_t) rlp;
	} */ *uap = v;
	struct sys_getrlimit_args ua;

	SPARC32TO64_UAP(which);
	SPARC32TOP_UAP(rlp, struct rlimit);
	return (sys_getrlimit(p, &ua, retval));
}

int
compat_sparc32_setrlimit(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_setrlimit_args /* {
		syscallarg(int) which;
		syscallarg(const sparc32_rlimitp_t) rlp;
	} */ *uap = v;
	struct sys_setrlimit_args ua;

	SPARC32TO64_UAP(which);
	SPARC32TOP_UAP(rlp, struct rlimit);
	return (sys_setrlimit(p, &ua, retval));
}

int
compat_sparc32_mmap(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_mmap_args /* {
		syscallarg(sparc32_voidp) addr;
		syscallarg(sparc32_size_t) len;
		syscallarg(int) prot;
		syscallarg(int) flags;
		syscallarg(int) fd;
		syscallarg(sparc32_long) pad;
		syscallarg(off_t) pos;
	} */ *uap = v;
	struct sys_mmap_args ua;
	void *rt;
	int error;

	SPARC32TOP_UAP(addr, void);
	SPARC32TOX_UAP(len, size_t);
	SPARC32TO64_UAP(prot);
	SPARC32TO64_UAP(flags);
	SPARC32TO64_UAP(fd);
	SPARC32TOX_UAP(pad, long);
	SPARC32TOX_UAP(pos, off_t);
	error = sys_mmap(p, &ua, (register_t *)&rt);
	if ((long)rt > (long)UINT_MAX)
		printf("compat_sparc32_mmap: retval out of range: 0x%qx",
		    rt);
	*retval = (sparc32_voidp)(u_long)rt;
	return (error);
}

int
compat_sparc32_truncate(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_truncate_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(int) pad;
		syscallarg(off_t) length;
	} */ *uap = v;
	struct sys_truncate_args ua;

	SPARC32TOP_UAP(path, const char);
	SPARC32TO64_UAP(pad);
	SPARC32TO64_UAP(length);
	return (sys_truncate(p, &ua, retval));
}

int
compat_sparc32___sysctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32___sysctl_args /* {
		syscallarg(sparc32_intp) name;
		syscallarg(u_int) namelen;
		syscallarg(sparc32_voidp) old;
		syscallarg(sparc32_size_tp) oldlenp;
		syscallarg(sparc32_voidp) new;
		syscallarg(sparc32_size_t) newlen;
	} */ *uap = v;
	struct sys___sysctl_args ua;

	SPARC32TO64_UAP(namelen);
	SPARC32TOP_UAP(name, int);
	SPARC32TOP_UAP(old, void);
	SPARC32TOP_UAP(oldlenp, size_t);
	SPARC32TOP_UAP(new, void);
	SPARC32TOX_UAP(newlen, size_t);
	return (sys___sysctl(p, &ua, retval));
}

int
compat_sparc32_mlock(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_mlock_args /* {
		syscallarg(const sparc32_voidp) addr;
		syscallarg(sparc32_size_t) len;
	} */ *uap = v;
	struct sys_mlock_args ua;

	SPARC32TOP_UAP(addr, const void);
	SPARC32TO64_UAP(len);
	return (sys_mlock(p, &ua, retval));
}

int
compat_sparc32_munlock(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_munlock_args /* {
		syscallarg(const sparc32_voidp) addr;
		syscallarg(sparc32_size_t) len;
	} */ *uap = v;
	struct sys_munlock_args ua;

	SPARC32TOP_UAP(addr, const void);
	SPARC32TO64_UAP(len);
	return (sys_munlock(p, &ua, retval));
}

int
compat_sparc32_undelete(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_undelete_args /* {
		syscallarg(const sparc32_charp) path;
	} */ *uap = v;
	struct sys_undelete_args ua;

	SPARC32TOP_UAP(path, const char);
	return (sys_undelete(p, &ua, retval));
}

int
compat_sparc32_futimes(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_futimes_args /* {
		syscallarg(int) fd;
		syscallarg(const sparc32_timevalp_t) tptr;
	} */ *uap = v;
	struct sys_futimes_args ua;
	struct timeval tv;
	struct sparc32_timeval *tv32p;

	SPARC32TO64_UAP(fd);
	tv32p = (struct sparc32_timeval *)(u_long)SCARG(uap, tptr);
	if (tv32p) {
		SCARG(&ua, tptr) = &tv;
		sparc32_to_timeval(tv32p, &tv);
	} else
		SCARG(&ua, tptr) = NULL;
	return (sys_futimes(p, &ua, retval));
}

int
compat_sparc32_reboot(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_reboot_args /* {
		syscallarg(int) opt;
		syscallarg(sparc32_charp) bootstr;
	} */ *uap = v;
	struct sys_reboot_args ua;

	SPARC32TO64_UAP(opt);
	SPARC32TOP_UAP(bootstr, char);
	return (sys_reboot(p, &ua, retval));
}

int
compat_sparc32_poll(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_poll_args /* {
		syscallarg(sparc32_pollfdp_t) fds;
		syscallarg(u_int) nfds;
		syscallarg(int) timeout;
	} */ *uap = v;
	struct sys_poll_args ua;

	SPARC32TOP_UAP(fds, struct pollfd);
	SPARC32TO64_UAP(nfds);
	SPARC32TO64_UAP(timeout);
	return (sys_poll(p, &ua, retval));
}

int
compat_sparc32___semctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32___semctl_args /* {
		syscallarg(int) semid;
		syscallarg(int) semnum;
		syscallarg(int) cmd;
		syscallarg(sparc32_semunu_t) arg;
	} */ *uap = v;
	struct sys___semctl_args ua;
	struct semid_ds ds, *dsp = &ds;
	union sparc32_semun *sem32p;
	int error;

	SPARC32TO64_UAP(semid);
	SPARC32TO64_UAP(semnum);
	SPARC32TO64_UAP(cmd);
	sem32p = (union sparc32_semun *)(u_long)SCARG(uap, arg);
	if (sem32p) {
		SCARG(&ua, arg)->buf = dsp;
		switch (SCARG(uap, cmd)) {
		case IPC_SET:
			sparc32_to_semid_ds(sem32p->buf, &ds);
			break;
		case SETVAL:
			SCARG(&ua, arg)->val = sem32p->val;
			break;
		case SETALL:
			SCARG(&ua, arg)->array = (u_short *)(u_long)sem32p->array;
			break;
		}
	} else
		SCARG(&ua, arg) = NULL;

	error = sys___semctl(p, &ua, retval);
	if (error)
		return (error);

	if (sem32p) {
		switch (SCARG(uap, cmd)) {
		case IPC_STAT:
			sparc32_from_semid_ds(&ds, sem32p->buf);
			break;
		}
	}
	return (0);
}

int
compat_sparc32_semget(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_semget_args /* {
		syscallarg(sparc32_key_t) key;
		syscallarg(int) nsems;
		syscallarg(int) semflg;
	} */ *uap = v;
	struct sys_semget_args ua;

	SPARC32TOX_UAP(key, key_t);
	SPARC32TO64_UAP(nsems);
	SPARC32TO64_UAP(semflg);
	return (sys_semget(p, &ua, retval));
}

int
compat_sparc32_semop(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_semop_args /* {
		syscallarg(int) semid;
		syscallarg(sparc32_sembufp_t) sops;
		syscallarg(sparc32_size_t) nsops;
	} */ *uap = v;
	struct sys_semop_args ua;

	SPARC32TO64_UAP(semid);
	SPARC32TOP_UAP(sops, struct sembuf);
	SPARC32TOX_UAP(nsops, size_t);
	return (sys_semop(p, &ua, retval));
}

int
compat_sparc32_msgctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_msgctl_args /* {
		syscallarg(int) msqid;
		syscallarg(int) cmd;
		syscallarg(sparc32_msqid_dsp_t) buf;
	} */ *uap = v;
	struct sys_msgctl_args ua;
	struct msqid_ds ds;
	struct sparc32_msqid_ds *ds32p;
	int error;

	SPARC32TO64_UAP(msqid);
	SPARC32TO64_UAP(cmd);
	ds32p = (struct sparc32_msqid_ds *)(u_long)SCARG(uap, buf);
	if (ds32p) {
		SCARG(&ua, buf) = NULL;
		sparc32_to_msqid_ds(ds32p, &ds);
	} else
		SCARG(&ua, buf) = NULL;
	error = sys_msgctl(p, &ua, retval);
	if (error)
		return (error);

	if (ds32p)
		sparc32_from_msqid_ds(&ds, ds32p);
	return (0);
}

int
compat_sparc32_msgget(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_msgget_args /* {
		syscallarg(sparc32_key_t) key;
		syscallarg(int) msgflg;
	} */ *uap = v;
	struct sys_msgget_args ua;

	SPARC32TOX_UAP(key, key_t);
	SPARC32TO64_UAP(msgflg);
	return (sys_msgget(p, &ua, retval));
}

int
compat_sparc32_msgsnd(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_msgsnd_args /* {
		syscallarg(int) msqid;
		syscallarg(const sparc32_voidp) msgp;
		syscallarg(sparc32_size_t) msgsz;
		syscallarg(int) msgflg;
	} */ *uap = v;
	struct sys_msgsnd_args ua;

	SPARC32TO64_UAP(msqid);
	SPARC32TOP_UAP(msgp, void);
	SPARC32TOX_UAP(msgsz, size_t);
	SPARC32TO64_UAP(msgflg);
	return (sys_msgsnd(p, &ua, retval));
}

int
compat_sparc32_msgrcv(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_msgrcv_args /* {
		syscallarg(int) msqid;
		syscallarg(sparc32_voidp) msgp;
		syscallarg(sparc32_size_t) msgsz;
		syscallarg(sparc32_long) msgtyp;
		syscallarg(int) msgflg;
	} */ *uap = v;
	struct sys_msgrcv_args ua;
	ssize_t rt;
	int error;

	SPARC32TO64_UAP(msqid);
	SPARC32TOP_UAP(msgp, void);
	SPARC32TOX_UAP(msgsz, size_t);
	SPARC32TOX_UAP(msgtyp, long);
	SPARC32TO64_UAP(msgflg);
	error = sys_msgrcv(p, &ua, (register_t *)&rt);
	*(sparc32_ssize_t *)retval = rt;
	return (error);
}

int
compat_sparc32_shmat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_shmat_args /* {
		syscallarg(int) shmid;
		syscallarg(const sparc32_voidp) shmaddr;
		syscallarg(int) shmflg;
	} */ *uap = v;
	struct sys_shmat_args ua;
	void *rt;
	int error;

	SPARC32TO64_UAP(shmid);
	SPARC32TOP_UAP(shmaddr, void);
	SPARC32TO64_UAP(shmflg);
	error = sys_shmat(p, &ua, (register_t *)&rt);
	*retval = (sparc32_voidp)(u_long)rt;
	return (error);
}

int
compat_sparc32_shmctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_shmctl_args /* {
		syscallarg(int) shmid;
		syscallarg(int) cmd;
		syscallarg(sparc32_shmid_dsp_t) buf;
	} */ *uap = v;
	struct sys_shmctl_args ua;
	struct shmid_ds ds;
	struct sparc32_shmid_ds *ds32p;
	int error;

	SPARC32TO64_UAP(shmid);
	SPARC32TO64_UAP(cmd);
	ds32p = (struct sparc32_shmid_ds *)(u_long)SCARG(uap, buf);
	if (ds32p) {
		SCARG(&ua, buf) = NULL;
		sparc32_to_shmid_ds(ds32p, &ds);
	} else
		SCARG(&ua, buf) = NULL;
	error = sys_shmctl(p, &ua, retval);
	if (error)
		return (error);

	if (ds32p)
		sparc32_from_shmid_ds(&ds, ds32p);
	return (0);
}

int
compat_sparc32_shmdt(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_shmdt_args /* {
		syscallarg(const sparc32_voidp) shmaddr;
	} */ *uap = v;
	struct sys_shmdt_args ua;

	SPARC32TOP_UAP(shmaddr, const char);
	return (sys_shmdt(p, &ua, retval));
}

int
compat_sparc32_shmget(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_shmget_args /* {
		syscallarg(sparc32_key_t) key;
		syscallarg(sparc32_size_t) size;
		syscallarg(int) shmflg;
	} */ *uap = v;
	struct sys_shmget_args ua;

	SPARC32TOX_UAP(key, key_t)
	SPARC32TOX_UAP(size, size_t)
	SPARC32TO64_UAP(shmflg);
	return (sys_shmget(p, &ua, retval));
}

int
compat_sparc32_clock_gettime(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_clock_gettime_args /* {
		syscallarg(sparc32_clockid_t) clock_id;
		syscallarg(sparc32_timespecp_t) tp;
	} */ *uap = v;
	struct sys_clock_gettime_args ua;
	struct timespec ts;
	struct sparc32_timespec *ts32p;
	int error;

	SPARC32TO64_UAP(clock_id);
	ts32p = (struct sparc32_timespec *)(u_long)SCARG(uap, tp);
	if (ts32p)
		SCARG(&ua, tp) = &ts;
	else
		SCARG(&ua, tp) = NULL;
	error = sys_clock_gettime(p, &ua, retval);
	if (error)
		return (error);

	if (ts32p)
		sparc32_from_timespec(&ts, ts32p);
	return (0);
}

int
compat_sparc32_clock_settime(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_clock_settime_args /* {
		syscallarg(sparc32_clockid_t) clock_id;
		syscallarg(const sparc32_timespecp_t) tp;
	} */ *uap = v;
	struct sys_clock_settime_args ua;
	struct timespec ts;
	struct sparc32_timespec *ts32p;

	SPARC32TO64_UAP(clock_id);
	ts32p = (struct sparc32_timespec *)(u_long)SCARG(uap, tp);
	if (ts32p) {
		SCARG(&ua, tp) = &ts;
		sparc32_to_timespec(ts32p, &ts);
	} else
		SCARG(&ua, tp) = NULL;
	return (sys_clock_settime(p, &ua, retval));
}

int
compat_sparc32_clock_getres(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_clock_getres_args /* {
		syscallarg(sparc32_clockid_t) clock_id;
		syscallarg(sparc32_timespecp_t) tp;
	} */ *uap = v;
	struct sys_clock_getres_args ua;
	struct timespec ts;
	struct sparc32_timespec *ts32p;
	int error;

	SPARC32TO64_UAP(clock_id);
	ts32p = (struct sparc32_timespec *)(u_long)SCARG(uap, tp);
	if (ts32p)
		SCARG(&ua, tp) = &ts;
	else
		SCARG(&ua, tp) = NULL;
	error = sys_clock_getres(p, &ua, retval);
	if (error)
		return (error);

	if (ts32p)
		sparc32_from_timespec(&ts, ts32p);
	return (0);
}

int
compat_sparc32_nanosleep(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_nanosleep_args /* {
		syscallarg(const sparc32_timespecp_t) rqtp;
		syscallarg(sparc32_timespecp_t) rmtp;
	} */ *uap = v;
	struct sys_nanosleep_args ua;
	struct timespec qts, mts;
	struct sparc32_timespec *qts32p, *mts32p;
	int error;
	
	qts32p = (struct sparc32_timespec *)(u_long)SCARG(uap, rqtp);
	mts32p = (struct sparc32_timespec *)(u_long)SCARG(uap, rmtp);
	if (qts32p) {
		SCARG(&ua, rqtp) = &qts;
		sparc32_to_timespec(qts32p, &qts);
	} else
		SCARG(&ua, rqtp) = NULL;
	if (mts32p) {
		SCARG(&ua, rmtp) = &mts;
	} else
		SCARG(&ua, rmtp) = NULL;
	error = sys_nanosleep(p, &ua, retval);
	if (error)
		return (error);

	if (mts32p)
		sparc32_from_timespec(&mts, mts32p);
	return (0);
}

int
compat_sparc32___posix_rename(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32___posix_rename_args /* {
		syscallarg(const sparc32_charp) from;
		syscallarg(const sparc32_charp) to;
	} */ *uap = v;
	struct sys___posix_rename_args ua;

	SPARC32TOP_UAP(from, const char)
	SPARC32TOP_UAP(to, const char)
	return (sys___posix_rename(p, &ua, retval));
}

int
compat_sparc32_swapctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_swapctl_args /* {
		syscallarg(int) cmd;
		syscallarg(const sparc32_voidp) arg;
		syscallarg(int) misc;
	} */ *uap = v;
	struct sys_swapctl_args ua;

	SPARC32TO64_UAP(cmd);
	SPARC32TOP_UAP(arg, const void);
	SPARC32TO64_UAP(misc);
	return (sys_swapctl(p, &ua, retval));
}

int
compat_sparc32_getdents(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_getdents_args /* {
		syscallarg(int) fd;
		syscallarg(sparc32_charp) buf;
		syscallarg(sparc32_size_t) count;
	} */ *uap = v;
	struct sys_getdents_args ua;

	SPARC32TO64_UAP(fd);
	SPARC32TOP_UAP(buf, char);
	SPARC32TOX_UAP(count, size_t);
	return (sys_getdents(p, &ua, retval));
}

int
compat_sparc32_minherit(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_minherit_args /* {
		syscallarg(sparc32_voidp) addr;
		syscallarg(sparc32_size_t) len;
		syscallarg(int) inherit;
	} */ *uap = v;
	struct sys_minherit_args ua;

	SPARC32TOP_UAP(addr, void);
	SPARC32TOX_UAP(len, size_t);
	SPARC32TO64_UAP(inherit);
	return (sys_minherit(p, &ua, retval));
}

int
compat_sparc32_lchmod(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_lchmod_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct sys_lchmod_args ua;

	SPARC32TOP_UAP(path, const char);
	SPARC32TO64_UAP(mode);
	return (sys_lchmod(p, &ua, retval));
}

int
compat_sparc32_lchown(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_lchown_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct sys_lchown_args ua;

	SPARC32TOP_UAP(path, const char);
	SPARC32TO64_UAP(uid);
	SPARC32TO64_UAP(gid);
	return (sys_lchown(p, &ua, retval));
}

int
compat_sparc32_lutimes(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_lutimes_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(const sparc32_timevalp_t) tptr;
	} */ *uap = v;
	struct sys_lutimes_args ua;
	struct timeval tv;
	struct sparc32_timeval *tv32p;

	SPARC32TOP_UAP(path, const char);
	tv32p = (struct sparc32_timeval *)(u_long)SCARG(uap, tptr);
	if (tv32p) {
		SCARG(&ua, tptr) = &tv;
		sparc32_to_timeval(tv32p, &tv);
	}
		SCARG(&ua, tptr) = NULL;
	return (sys_lutimes(p, &ua, retval));
}

int
compat_sparc32___msync13(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32___msync13_args /* {
		syscallarg(sparc32_voidp) addr;
		syscallarg(sparc32_size_t) len;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys___msync13_args ua;

	SPARC32TOP_UAP(addr, void);
	SPARC32TOX_UAP(len, size_t);
	SPARC32TO64_UAP(flags);
	return (sys___msync13(p, &ua, retval));
}

int
compat_sparc32___stat13(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32___stat13_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(sparc32_statp_t) ub;
	} */ *uap = v;
	struct sys___stat13_args ua;
	struct stat sb;
	struct sparc32_stat *sb32p;
	caddr_t sg;
	int error;

	SPARC32TOP_UAP(path, const char);
	sb32p = (struct sparc32_stat *)(u_long)SCARG(uap, ub);
	if (sb32p)
		SCARG(&ua, ub) = &sb;
	else
		SCARG(&ua, ub) = NULL;
	sg = stackgap_init(p->p_emul);
	SPARC32_CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));

	error = sys___stat13(p, &ua, retval);
	if (error)
		return (error);

	if (sb32p)
		sparc32_from___stat13(&sb, sb32p);
	return (0);
}

int
compat_sparc32___fstat13(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32___fstat13_args /* {
		syscallarg(int) fd;
		syscallarg(sparc32_statp_t) sb;
	} */ *uap = v;
	struct sys___fstat13_args ua;
	struct stat sb;
	struct sparc32_stat *sb32p;
	int error;

	SPARC32TO64_UAP(fd);
	sb32p = (struct sparc32_stat *)(u_long)SCARG(uap, sb);
	if (sb32p)
		SCARG(&ua, sb) = &sb;
	else
		SCARG(&ua, sb) = NULL;
	error = sys___fstat13(p, &ua, retval);
	if (error)
		return (error);

	if (sb32p)
		sparc32_from___stat13(&sb, sb32p);
	return (0);
}

int
compat_sparc32___lstat13(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32___lstat13_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(sparc32_statp_t) ub;
	} */ *uap = v;

	struct sys___lstat13_args ua;
	struct stat sb;
	struct sparc32_stat *sb32p;
	caddr_t sg;
	int error;

	SPARC32TOP_UAP(path, const char);
	sb32p = (struct sparc32_stat *)(u_long)SCARG(uap, ub);
	if (sb32p)
		SCARG(&ua, ub) = &sb;
	else
		SCARG(&ua, ub) = NULL;
	sg = stackgap_init(p->p_emul);
	SPARC32_CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));
	error = sys___lstat13(p, &ua, retval);
	if (error)
		return (error);

	if (sb32p)
		sparc32_from___stat13(&sb, sb32p);
	return (0);
}

int
compat_sparc32___sigaltstack14(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32___sigaltstack14_args /* {
		syscallarg(const sparc32_sigaltstackp_t) nss;
		syscallarg(sparc32_sigaltstackp_t) oss;
	} */ *uap = v;
	struct sys___sigaltstack14_args ua;
	struct sparc32_sigaltstack *nss32, *oss32;
	struct sigaltstack nss, oss;
	int error;

	nss32 = (struct sparc32_sigaltstack *)(u_long)SCARG(uap, nss);
	oss32 = (struct sparc32_sigaltstack *)(u_long)SCARG(uap, oss);
	if (nss32) {
		SCARG(&ua, nss) = &nss;
		nss.ss_sp = (void *)(u_long)nss32->ss_sp;
		nss.ss_size = (size_t)nss32->ss_size;
		nss.ss_flags = nss32->ss_flags;
	} else
		SCARG(&ua, nss) = NULL;
	if (oss32)
		SCARG(&ua, oss) = &oss;
	else
		SCARG(&ua, oss) = NULL;

	error = sys___sigaltstack14(p, &ua, retval);
	if (error)
		return (error);

	if (oss32) {
		oss32->ss_sp = (sparc32_voidp)(u_long)oss.ss_sp;
		oss32->ss_size = (sparc32_size_t)oss.ss_size;
		oss32->ss_flags = oss.ss_flags;
	}
	return (0);
}

int
compat_sparc32___posix_chown(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32___posix_chown_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct sys___posix_chown_args ua;

	SPARC32TOP_UAP(path, const char);
	SPARC32TO64_UAP(uid);
	SPARC32TO64_UAP(gid);
	return (sys___posix_chown(p, &ua, retval));
}

int
compat_sparc32___posix_lchown(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32___posix_lchown_args /* {
		syscallarg(const sparc32_charp) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct sys___posix_lchown_args ua;

	SPARC32TOP_UAP(path, const char);
	SPARC32TO64_UAP(uid);
	SPARC32TO64_UAP(gid);
	return (sys___posix_lchown(p, &ua, retval));
}

int
compat_sparc32_preadv(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_preadv_args /* {
		syscallarg(int) fd;
		syscallarg(const sparc32_iovecp_t) iovp;
		syscallarg(int) iovcnt;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
	} */ *uap = v;
	struct sys_preadv_args ua;
	struct iovec *iov;
	ssize_t rt;
	int error;

	SPARC32TO64_UAP(fd);
	SPARC32TO64_UAP(iovcnt);
	SPARC32TO64_UAP(pad);
	SPARC32TO64_UAP(offset);
	MALLOC(iov, struct iovec *, sizeof(struct iovec) * SCARG(uap, iovcnt),
	    M_TEMP, M_WAITOK);
	sparc32_to_iovec((struct sparc32_iovec *)(u_long)SCARG(uap, iovp), iov,
	    SCARG(uap, iovcnt));
	SCARG(&ua, iovp) = iov;

	error = sys_preadv(p, &ua, (register_t *)&rt);
	FREE(iov, M_TEMP);
	*(sparc32_ssize_t *)retval = rt;
	return (error);
}

int
compat_sparc32_pwritev(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_sparc32_pwritev_args /* {
		syscallarg(int) fd;
		syscallarg(const sparc32_iovecp_t) iovp;
		syscallarg(int) iovcnt;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
	} */ *uap = v;
	struct sys_pwritev_args ua;
	struct iovec *iov;
	ssize_t rt;
	int error;

	SPARC32TO64_UAP(fd);
	SPARC32TO64_UAP(iovcnt);
	SPARC32TO64_UAP(pad);
	SPARC32TO64_UAP(offset);
	MALLOC(iov, struct iovec *, sizeof(struct iovec) * SCARG(uap, iovcnt),
	    M_TEMP, M_WAITOK);
	sparc32_to_iovec((struct sparc32_iovec *)(u_long)SCARG(uap, iovp), iov,
	    SCARG(uap, iovcnt));
	SCARG(&ua, iovp) = iov;

	error = sys_pwritev(p, &ua, (register_t *)&rt);
	FREE(iov, M_TEMP);
	*(sparc32_ssize_t *)retval = rt;
	return (error);
}
