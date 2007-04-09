/*	$NetBSD: kern_ktrace.c,v 1.119.2.3 2007/04/09 22:10:02 ad Exp $	*/

/*
 * Copyright (c) 1989, 1993
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
 *	@(#)kern_ktrace.c	8.5 (Berkeley) 5/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_ktrace.c,v 1.119.2.3 2007/04/09 22:10:02 ad Exp $");

#include "opt_ktrace.h"
#include "opt_compat_mach.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/ktrace.h>
#include <sys/kmem.h>
#include <sys/syslog.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/callout.h>
#include <sys/kauth.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

#ifdef KTRACE

/*
 * TODO:
 *	- need better error reporting?
 *	- userland utility to sort ktrace.out by timestamp.
 *	- keep minimum information in ktrace_entry when rest of alloc failed.
 *	- per trace control of configurable parameters.
 */

struct ktrace_entry {
	TAILQ_ENTRY(ktrace_entry) kte_list;
	struct	ktr_header kte_kth;
	void	*kte_buf;
	size_t	kte_bufsz;	
#define	KTE_SPACE		32
	uint8_t kte_space[KTE_SPACE];
};

struct ktr_desc {
	TAILQ_ENTRY(ktr_desc) ktd_list;
	int ktd_flags;
#define	KTDF_WAIT		0x0001
#define	KTDF_DONE		0x0002
#define	KTDF_BLOCKING		0x0004
#define	KTDF_INTERACTIVE	0x0008
	int ktd_error;
#define	KTDE_ENOMEM		0x0001
#define	KTDE_ENOSPC		0x0002
	int ktd_errcnt;
	int ktd_ref;			/* # of reference */
	int ktd_qcount;			/* # of entry in the queue */

	/*
	 * Params to control behaviour.
	 */
	int ktd_delayqcnt;		/* # of entry allowed to delay */
	int ktd_wakedelay;		/* delay of wakeup in *tick* */
	int ktd_intrwakdl;		/* ditto, but when interactive */

	struct file *ktd_fp;		/* trace output file */
	struct lwp *ktd_lwp;		/* our kernel thread */
	TAILQ_HEAD(, ktrace_entry) ktd_queue;
	struct callout ktd_wakch;	/* delayed wakeup */
	kcondvar_t ktd_sync_cv;
	kcondvar_t ktd_cv;
};

static int	ktealloc(struct ktrace_entry **, void **, struct lwp *, int,
			 size_t);
static void	ktrwrite(struct ktr_desc *, struct ktrace_entry *);
static int	ktrace_common(struct lwp *, int, int, int, struct file *);
static int	ktrops(struct lwp *, struct proc *, int, int,
		    struct ktr_desc *);
static int	ktrsetchildren(struct lwp *, struct proc *, int, int,
		    struct ktr_desc *);
static int	ktrcanset(struct lwp *, struct proc *);
static int	ktrsamefile(struct file *, struct file *);

static struct ktr_desc *
		ktd_lookup(struct file *);
static void	ktdrel(struct ktr_desc *);
static void	ktdref(struct ktr_desc *);
static void	ktraddentry(struct lwp *, struct ktrace_entry *, int);
/* Flags for ktraddentry (3rd arg) */
#define	KTA_NOWAIT		0x0000
#define	KTA_WAITOK		0x0001
#define	KTA_LARGE		0x0002
static void	ktefree(struct ktrace_entry *);
static void	ktd_logerrl(struct ktr_desc *, int);
static void	ktrace_thread(void *);
static int	ktrderefall(struct ktr_desc *, int);

/*
 * Default vaules.
 */
#define	KTD_MAXENTRY		1000	/* XXX: tune */
#define	KTD_TIMEOUT		5	/* XXX: tune */
#define	KTD_DELAYQCNT		100	/* XXX: tune */
#define	KTD_WAKEDELAY		5000	/* XXX: tune */
#define	KTD_INTRWAKDL		100	/* XXX: tune */

/*
 * Patchable variables.
 */
int ktd_maxentry = KTD_MAXENTRY;	/* max # of entry in the queue */
int ktd_timeout = KTD_TIMEOUT;		/* timeout in seconds */
int ktd_delayqcnt = KTD_DELAYQCNT;	/* # of entry allowed to delay */
int ktd_wakedelay = KTD_WAKEDELAY;	/* delay of wakeup in *ms* */
int ktd_intrwakdl = KTD_INTRWAKDL;	/* ditto, but when interactive */

kmutex_t ktrace_mutex;
static TAILQ_HEAD(, ktr_desc) ktdq = TAILQ_HEAD_INITIALIZER(ktdq);

MALLOC_DEFINE(M_KTRACE, "ktrace", "ktrace data buffer");
POOL_INIT(kte_pool, sizeof(struct ktrace_entry), 0, 0, 0,
    "ktepl", &pool_allocator_nointr, IPL_NONE);

static inline void
ktd_wakeup(struct ktr_desc *ktd)
{

	callout_stop(&ktd->ktd_wakch);
	cv_wakeup(&ktd->ktd_cv);		/* XXXSMP */
}

static void
ktd_logerrl(struct ktr_desc *ktd, int error)
{

	ktd->ktd_error |= error;
	ktd->ktd_errcnt++;
}

#if 0
static void
ktd_logerr(struct proc *p, int error)
{
	struct ktr_desc *ktd;

	KASSERT(mutex_owned(&ktrace_mutex));

	ktd = p->p_tracep;
	if (ktd == NULL)
		return;

	ktd_logerrl(ktd, error);
}
#endif

static inline int
ktrenter(struct lwp *l)
{

	if ((l->l_pflag & LP_KTRACTIVE) != 0)
		return 1;
	l->l_pflag |= LP_KTRACTIVE;
	return 0;
}

static inline void
ktrexit(struct lwp *l)
{

	l->l_pflag &= ~LP_KTRACTIVE;
}

/*
 * Initialise the ktrace system.
 */
void
ktrinit(void)
{

	mutex_init(&ktrace_mutex, MUTEX_DEFAULT, IPL_NONE);
}

/*
 * Release a reference.  Called with ktrace_mutex held.
 */
void
ktdrel(struct ktr_desc *ktd)
{

	KASSERT(mutex_owned(&ktrace_mutex));

	KDASSERT(ktd->ktd_ref != 0);
	KASSERT(ktd->ktd_ref > 0);
	if (--ktd->ktd_ref <= 0) {
		ktd->ktd_flags |= KTDF_DONE;
		cv_broadcast(&ktd->ktd_cv);
	}
}

void
ktdref(struct ktr_desc *ktd)
{

	KASSERT(mutex_owned(&ktrace_mutex));

	ktd->ktd_ref++;
}

struct ktr_desc *
ktd_lookup(struct file *fp)
{
	struct ktr_desc *ktd;

	KASSERT(mutex_owned(&ktrace_mutex));

	for (ktd = TAILQ_FIRST(&ktdq); ktd != NULL;
	    ktd = TAILQ_NEXT(ktd, ktd_list)) {
		if (ktrsamefile(ktd->ktd_fp, fp)) {
			ktd->ktd_ref++;
			break;
		}
	}

	return (ktd);
}

void
ktraddentry(struct lwp *l, struct ktrace_entry *kte, int flags)
{
	struct proc *p = l->l_proc;
	struct ktr_desc *ktd;
#ifdef DEBUG
	struct timeval t1, t2;
#endif

	mutex_enter(&ktrace_mutex);

	if (p->p_traceflag & KTRFAC_TRC_EMUL) {
		/* Add emulation trace before first entry for this process */
		p->p_traceflag &= ~KTRFAC_TRC_EMUL;
		mutex_exit(&ktrace_mutex);
		ktrexit(l);
		ktremul(l);
		(void)ktrenter(l);
		mutex_enter(&ktrace_mutex);
	}

	/*
	 * Tracing may be canceled while we were sleeping waiting for
	 * memory.
	 */
	ktd = p->p_tracep;
	if (ktd == NULL)
		goto freekte;

	/*
	 * Bump reference count so that the object will remain while
	 * we are here.  Note that the trace is controlled by other
	 * process.
	 */
	ktdref(ktd);

	if (ktd->ktd_flags & KTDF_DONE)
		goto relktd;

	if (ktd->ktd_qcount > ktd_maxentry) {
		ktd_logerrl(ktd, KTDE_ENOSPC);
		goto relktd;
	}
	TAILQ_INSERT_TAIL(&ktd->ktd_queue, kte, kte_list);
	ktd->ktd_qcount++;
	if (ktd->ktd_flags & KTDF_BLOCKING)
		goto skip_sync;

	if (flags & KTA_WAITOK &&
	    (/* flags & KTA_LARGE */0 || ktd->ktd_flags & KTDF_WAIT ||
	    ktd->ktd_qcount > ktd_maxentry >> 1))
		/*
		 * Sync with writer thread since we're requesting rather
		 * big one or many requests are pending.
		 */
		do {
			ktd->ktd_flags |= KTDF_WAIT;
			ktd_wakeup(ktd);
#ifdef DEBUG
			getmicrouptime(&t1);
#endif
			if (cv_timedwait(&ktd->ktd_sync_cv, &ktrace_mutex,
			    ktd_timeout * hz) != 0) {
				ktd->ktd_flags |= KTDF_BLOCKING;
				/*
				 * Maybe the writer thread is blocking
				 * completely for some reason, but
				 * don't stop target process forever.
				 */
				log(LOG_NOTICE, "ktrace timeout\n");
				break;
			}
#ifdef DEBUG
			getmicrouptime(&t2);
			timersub(&t2, &t1, &t2);
			if (t2.tv_sec > 0)
				log(LOG_NOTICE,
				    "ktrace long wait: %ld.%06ld\n",
				    t2.tv_sec, t2.tv_usec);
#endif
		} while (p->p_tracep == ktd &&
		    (ktd->ktd_flags & (KTDF_WAIT | KTDF_DONE)) == KTDF_WAIT);
	else {
		/* Schedule delayed wakeup */
		if (ktd->ktd_qcount > ktd->ktd_delayqcnt)
			ktd_wakeup(ktd);	/* Wakeup now */
		else if (!callout_pending(&ktd->ktd_wakch))
			callout_reset(&ktd->ktd_wakch,
			    ktd->ktd_flags & KTDF_INTERACTIVE ?
			    ktd->ktd_intrwakdl : ktd->ktd_wakedelay,
			    (void (*)(void *))ktd_wakeup, ktd);
	}

skip_sync:
	ktdrel(ktd);
	mutex_exit(&ktrace_mutex);
	ktrexit(l);
	return;

relktd:
	ktdrel(ktd);

freekte:
	mutex_exit(&ktrace_mutex);
	ktefree(kte);
	ktrexit(l);
}

void
ktefree(struct ktrace_entry *kte)
{

	if (kte->kte_buf != kte->kte_space)
		kmem_free(kte->kte_buf, kte->kte_bufsz);
	pool_put(&kte_pool, kte);
}

/*
 * "deep" compare of two files for the purposes of clearing a trace.
 * Returns true if they're the same open file, or if they point at the
 * same underlying vnode/socket.
 */

int
ktrsamefile(struct file *f1, struct file *f2)
{

	return ((f1 == f2) ||
	    ((f1 != NULL) && (f2 != NULL) &&
		(f1->f_type == f2->f_type) &&
		(f1->f_data == f2->f_data)));
}

void
ktrderef(struct proc *p)
{
	struct ktr_desc *ktd = p->p_tracep;

	KASSERT(mutex_owned(&ktrace_mutex));

	p->p_traceflag = 0;
	if (ktd == NULL)
		return;
	p->p_tracep = NULL;

	cv_broadcast(&ktd->ktd_sync_cv);
	ktdrel(ktd);
}

void
ktradref(struct proc *p)
{
	struct ktr_desc *ktd = p->p_tracep;

	KASSERT(mutex_owned(&ktrace_mutex));

	ktdref(ktd);
}

int
ktrderefall(struct ktr_desc *ktd, int auth)
{
	struct lwp *curl = curlwp;
	struct proc *p;
	int error = 0;

	mutex_enter(&proclist_lock);
	PROCLIST_FOREACH(p, &allproc) {
		if (p->p_tracep != ktd)
			continue;
		mutex_enter(&p->p_mutex);
		mutex_enter(&ktrace_mutex);
		if (p->p_tracep == ktd) {
			if (!auth || ktrcanset(curl, p))
				ktrderef(p);
			else
				error = EPERM;
		}
		mutex_exit(&ktrace_mutex);
		mutex_exit(&p->p_mutex);
	}
	mutex_exit(&proclist_lock);

	return error;
}

int
ktealloc(struct ktrace_entry **ktep, void **bufp, struct lwp *l, int type,
	 size_t sz)
{
	struct proc *p = l->l_proc;
	struct ktrace_entry *kte;
	struct ktr_header *kth;
	void *buf;

	if (ktrenter(l))
		return EAGAIN;

	kte = pool_get(&kte_pool, PR_WAITOK);
	if (sz > sizeof(kte->kte_space)) {
		if ((buf = kmem_alloc(sz, KM_SLEEP)) == NULL) {
			pool_put(&kte_pool, kte);
			ktrexit(l);
			return ENOMEM;
		}
	} else
		buf = kte->kte_space;

	kte->kte_bufsz = sz;
	kte->kte_buf = buf;

	kth = &kte->kte_kth;
	(void)memset(kth, 0, sizeof(*kth));
	kth->ktr_len = sz;
	kth->ktr_type = type;
	kth->ktr_pid = p->p_pid;
	memcpy(kth->ktr_comm, p->p_comm, MAXCOMLEN);
	kth->ktr_version = KTRFAC_VERSION(p->p_traceflag);

	switch (KTRFAC_VERSION(p->p_traceflag)) {
	case 0:
		/* This is the original format */
		microtime(&kth->ktr_tv);
		break;
	case 1:
		kth->ktr_lid = l->l_lid;
		nanotime(&kth->ktr_time);
		break;
	default:
		break;
	}

	*ktep = kte;
	*bufp = buf;

	return 0;
}

void
ktrsyscall(struct lwp *l, register_t code, register_t realcode,
    const struct sysent *callp, register_t args[])
{
	struct proc *p = l->l_proc;
	struct ktrace_entry *kte;
	struct ktr_syscall *ktp;
	register_t *argp;
	int argsize;
	size_t len;
	u_int i;

	if (callp == NULL)
		callp = p->p_emul->e_sysent;

	argsize = callp[code].sy_argsize;
#ifdef _LP64
	if (p->p_flag & PK_32)
		argsize = argsize << 1;
#endif
	len = sizeof(struct ktr_syscall) + argsize;

	if (ktealloc(&kte, (void *)&ktp, l, KTR_SYSCALL, len))
		return;

	ktp->ktr_code = realcode;
	ktp->ktr_argsize = argsize;
	argp = (register_t *)(ktp + 1);
	for (i = 0; i < (argsize / sizeof(*argp)); i++)
		*argp++ = args[i];

	ktraddentry(l, kte, KTA_WAITOK);
}

void
ktrsysret(struct lwp *l, register_t code, int error, register_t *retval)
{
	struct ktrace_entry *kte;
	struct ktr_sysret *ktp;

	if (ktealloc(&kte, (void *)&ktp, l, KTR_SYSRET,
	    sizeof(struct ktr_sysret)))
		return;

	ktp->ktr_code = code;
	ktp->ktr_eosys = 0;			/* XXX unused */
	ktp->ktr_error = error;
	ktp->ktr_retval = retval ? retval[0] : 0;
	ktp->ktr_retval_1 = retval ? retval[1] : 0;

	ktraddentry(l, kte, KTA_WAITOK);
}

/*
 * XXX: ndp->ni_pathlen should be passed.
 */
void
ktrnamei(struct lwp *l, char *path)
{

	ktrkmem(l, KTR_NAMEI, path, strlen(path));
}

void
ktremul(struct lwp *l)
{
	const char *emul = l->l_proc->p_emul->e_name;

	ktrkmem(l, KTR_EMUL, emul, strlen(emul));
}

void
ktrkmem(struct lwp *l, int type, const void *bf, size_t len)
{
	struct ktrace_entry *kte;
	void *buf;

	if (ktealloc(&kte, &buf, l, type, len))
		return;
	memcpy(buf, bf, len);
	ktraddentry(l, kte, KTA_WAITOK);
}

void
ktrgenio(struct lwp *l, int fd, enum uio_rw rw, struct iovec *iov,
	 int len, int error)
{
	struct ktrace_entry *kte;
	struct ktr_genio *ktp;
	int resid = len, cnt;
	void *cp;
	int buflen;

	if (error)
		return;

 next:
	buflen = min(PAGE_SIZE, resid + sizeof(struct ktr_genio));

	if (ktealloc(&kte, (void *)&ktp, l, KTR_GENIO, buflen))
		return;

	ktp->ktr_fd = fd;
	ktp->ktr_rw = rw;

	cp = (void *)(ktp + 1);
	buflen -= sizeof(struct ktr_genio);
	kte->kte_kth.ktr_len = sizeof(struct ktr_genio);

	while (buflen > 0) {
		cnt = min(iov->iov_len, buflen);
		if (copyin(iov->iov_base, cp, cnt) != 0)
			goto out;
		kte->kte_kth.ktr_len += cnt;
		buflen -= cnt;
		resid -= cnt;
		iov->iov_len -= cnt;
		if (iov->iov_len == 0)
			iov++;
		else
			iov->iov_base = (char *)iov->iov_base + cnt;
	}

	/*
	 * Don't push so many entry at once.  It will cause kmem map
	 * shortage.
	 */
	ktraddentry(l, kte, KTA_WAITOK | KTA_LARGE);
	if (resid > 0) {
		if (curcpu()->ci_schedstate.spc_flags & SPCF_SHOULDYIELD) {
			(void)ktrenter(l);
			preempt();
			ktrexit(l);
		}

		goto next;
	}

	return;

out:
	ktefree(kte);
	ktrexit(l);
}

void
ktrpsig(struct lwp *l, int sig, sig_t action, const sigset_t *mask,
    const ksiginfo_t *ksi)
{
	struct ktrace_entry *kte;
	struct {
		struct ktr_psig	kp;
		siginfo_t	si;
	} *kbuf;

	if (ktealloc(&kte, (void *)&kbuf, l, KTR_PSIG, sizeof(*kbuf)))
		return;

	kbuf->kp.signo = (char)sig;
	kbuf->kp.action = action;
	kbuf->kp.mask = *mask;

	if (ksi) {
		kbuf->kp.code = KSI_TRAPCODE(ksi);
		(void)memset(&kbuf->si, 0, sizeof(kbuf->si));
		kbuf->si._info = ksi->ksi_info;
		kte->kte_kth.ktr_len = sizeof(*kbuf);
	} else {
		kbuf->kp.code = 0;
		kte->kte_kth.ktr_len = sizeof(struct ktr_psig);
	}

	ktraddentry(l, kte, KTA_WAITOK);
}

void
ktrcsw(struct lwp *l, int out, int user)
{
	struct proc *p = l->l_proc;
	struct ktrace_entry *kte;
	struct ktr_csw *kc;

	/*
	 * Don't record context switches resulting from blocking on 
	 * locks; it's too easy to get duff results.
	 */
	if (l->l_syncobj == &mutex_syncobj || l->l_syncobj == &rw_syncobj)
		return;

	/*
	 * We can't sleep if we're already going to sleep (if original
	 * condition is met during sleep, we hang up).
	 *
	 * XXX This is not ideal: it would be better to maintain a pool
	 * of ktes and actually push this to the kthread when context
	 * switch happens, however given the points where we are called
	 * from that is difficult to do. 
	 */
	if (out) {
		if (ktrenter(l))
			return;

		switch (KTRFAC_VERSION(p->p_traceflag)) {
		case 0:
			/* This is the original format */
			microtime(&l->l_ktrcsw.tv);
			l->l_pflag |= LP_KTRCSW;
			break;
		case 1:
			nanotime(&l->l_ktrcsw.ts);
			l->l_pflag |= LP_KTRCSW;
			break;
		default:
			break;
		}

		if (user)
			l->l_pflag |= LP_KTRCSWUSER;
		else
			l->l_pflag &= ~LP_KTRCSWUSER;

		ktrexit(l);
		return;
	}

	/*
	 * On the way back in, we need to record twice: once for entry, and
	 * once for exit.
	 */
	if ((l->l_pflag & LP_KTRCSW) != 0) {
		l->l_pflag &= ~LP_KTRCSW;

		if (ktealloc(&kte, (void *)&kc, l, KTR_CSW, sizeof(*kc)))
			return;

		kc->out = 1;
		kc->user = ((l->l_pflag & LP_KTRCSWUSER) != 0);

		switch (KTRFAC_VERSION(p->p_traceflag)) {
		case 0:
			/* This is the original format */
			memcpy(&kte->kte_kth.ktr_tv, &l->l_ktrcsw.tv,
			    sizeof(kte->kte_kth.ktr_tv));
			break;
		case 1:
			memcpy(&kte->kte_kth.ktr_time, &l->l_ktrcsw.ts,
			    sizeof(kte->kte_kth.ktr_time));
			break;
		default:
			break;
		}

		ktraddentry(l, kte, KTA_WAITOK);
	}

	if (ktealloc(&kte, (void *)&kc, l, KTR_CSW, sizeof(*kc)))
		return;

	kc->out = 0;
	kc->user = user;

	ktraddentry(l, kte, KTA_WAITOK);
}

int
ktruser(struct lwp *l, const char *id, void *addr, size_t len, int ustr)
{
	struct ktrace_entry *kte;
	struct ktr_user *ktp;
	void *user_dta;
	int error;

	if (len > KTR_USER_MAXLEN)
		return ENOSPC;

	error = ktealloc(&kte, (void *)&ktp, l, KTR_USER, sizeof(*ktp) + len);
	if (error != 0)
		return error;

	if (ustr) {
		if (copyinstr(id, ktp->ktr_id, KTR_USER_MAXIDLEN, NULL) != 0)
			ktp->ktr_id[0] = '\0';
	} else
		strncpy(ktp->ktr_id, id, KTR_USER_MAXIDLEN);
	ktp->ktr_id[KTR_USER_MAXIDLEN-1] = '\0';

	user_dta = (void *)(ktp + 1);
	if ((error = copyin(addr, (void *)user_dta, len)) != 0)
		len = 0;

	ktraddentry(l, kte, KTA_WAITOK);
	return error;
}

void
ktrmmsg(struct lwp *l, const void *msgh, size_t size)
{
	ktrkmem(l, KTR_MMSG, msgh, size);
}

void
ktrmool(struct lwp *l, const void *kaddr, size_t size, const void *uaddr)
{
	struct ktrace_entry *kte;
	struct ktr_mool *kp;
	struct ktr_mool *bf;

	if (ktealloc(&kte, (void *)&kp, l, KTR_MOOL, size + sizeof(*kp)))
		return;

	kp->uaddr = uaddr;
	kp->size = size;
	bf = kp + 1; /* Skip uaddr and size */
	(void)memcpy(bf, kaddr, size);

	ktraddentry(l, kte, KTA_WAITOK);
}

void
ktrmib(struct lwp *l, const int *name, u_int namelen)
{
	struct ktrace_entry *kte;
	int *namep;
	size_t size;

	size = namelen * sizeof(*name);

	if (ktealloc(&kte, (void *)&namep, l, KTR_MIB, size))
		return;

	(void)memcpy(namep, name, namelen * sizeof(*name));

	ktraddentry(l, kte, KTA_WAITOK);
}

/* Interface and common routines */

int
ktrace_common(struct lwp *curl, int ops, int facs, int pid, struct file *fp)
{
	struct proc *curp;
	struct proc *p;
	struct pgrp *pg;
	struct ktr_desc *ktd = NULL;
	int ret = 0;
	int error = 0;
	int descend;

	curp = curl->l_proc;
	descend = ops & KTRFLAG_DESCEND;
	facs = facs & ~((unsigned) KTRFAC_ROOT);

	(void)ktrenter(curl);

	switch (KTROP(ops)) {

	case KTROP_CLEARFILE:
		/*
		 * Clear all uses of the tracefile
		 */
		mutex_enter(&ktrace_mutex);
		ktd = ktd_lookup(fp);
		mutex_exit(&ktrace_mutex);
		if (ktd == NULL)
			goto done;
		error = ktrderefall(ktd, 1);
		goto done;

	case KTROP_SET:
		mutex_enter(&ktrace_mutex);
		ktd = ktd_lookup(fp);
		mutex_exit(&ktrace_mutex);
		if (ktd == NULL) {
			ktd = kmem_alloc(sizeof(*ktd), KM_SLEEP);
			TAILQ_INIT(&ktd->ktd_queue);
			callout_init(&ktd->ktd_wakch);
			cv_init(&ktd->ktd_cv, "ktrwait");
			cv_init(&ktd->ktd_sync_cv, "ktrsync");
			ktd->ktd_flags = ktd->ktd_qcount =
			    ktd->ktd_error = ktd->ktd_errcnt = 0;
			ktd->ktd_ref = 1;
			ktd->ktd_delayqcnt = ktd_delayqcnt;
			ktd->ktd_wakedelay = mstohz(ktd_wakedelay);
			ktd->ktd_intrwakdl = mstohz(ktd_intrwakdl);
			/*
			 * XXX: not correct.  needs an way to detect
			 * whether ktruss or ktrace.
			 */
			if (fp->f_type == DTYPE_PIPE)
				ktd->ktd_flags |= KTDF_INTERACTIVE;

			error = kthread_create1(PRI_NONE, true, ktrace_thread,
			    ktd, &ktd->ktd_lwp, "ktrace");
			if (error != 0) {
				kmem_free(ktd, sizeof(*ktd));
				goto done;
			}

			mutex_enter(&fp->f_lock);
			fp->f_count++;
			mutex_exit(&fp->f_lock);
			ktd->ktd_fp = fp;

			mutex_enter(&ktrace_mutex);
			if (ktd_lookup(fp) != NULL) {
				ktdrel(ktd);
				ktd = NULL;
			} else
				TAILQ_INSERT_TAIL(&ktdq, ktd, ktd_list);
			if (ktd == NULL)
				cv_wait(&lbolt, &ktrace_mutex);
			mutex_exit(&ktrace_mutex);
			if (ktd == NULL)
				goto done;
		}
		break;

	case KTROP_CLEAR:
		break;
	}

	/*
	 * need something to (un)trace (XXX - why is this here?)
	 */
	if (!facs) {
		error = EINVAL;
		goto done;
	}

	/*
	 * do it
	 */
	mutex_enter(&proclist_lock);
	if (pid < 0) {
		/*
		 * by process group
		 */
		pg = pg_find(-pid, PFIND_LOCKED);
		if (pg == NULL)
			error = ESRCH;
		else {
			LIST_FOREACH(p, &pg->pg_members, p_pglist) {
				if (descend)
					ret |= ktrsetchildren(curl, p, ops,
					    facs, ktd);
				else
					ret |= ktrops(curl, p, ops, facs,
					    ktd);
			}
		}

	} else {
		/*
		 * by pid
		 */
		p = p_find(pid, PFIND_LOCKED);
		if (p == NULL)
			error = ESRCH;
		else if (descend)
			ret |= ktrsetchildren(curl, p, ops, facs, ktd);
		else
			ret |= ktrops(curl, p, ops, facs, ktd);
	}
	mutex_exit(&proclist_lock);	/* taken by p{g}_find */
	if (error == 0 && !ret)
		error = EPERM;
done:
	if (ktd != NULL) {
		if (error != 0) {
			/*
			 * Wakeup the thread so that it can be die if we
			 * can't trace any process.
			 */
			ktd_wakeup(ktd);
		}
		if (KTROP(ops) == KTROP_SET || KTROP(ops) == KTROP_CLEARFILE) {
			mutex_enter(&ktrace_mutex);
			ktdrel(ktd);
			mutex_exit(&ktrace_mutex);
		}
	}
	ktrexit(curl);
	return (error);
}

/*
 * fktrace system call
 */
/* ARGSUSED */
int
sys_fktrace(struct lwp *l, void *v, register_t *retval)
{
	struct sys_fktrace_args /* {
		syscallarg(int) fd;
		syscallarg(int) ops;
		syscallarg(int) facs;
		syscallarg(int) pid;
	} */ *uap = v;
	struct file *fp = NULL;
	struct filedesc *fdp = l->l_proc->p_fd;
	int error;

	fdp = l->l_proc->p_fd;
	if ((fp = fd_getfile(fdp, SCARG(uap, fd))) == NULL)
		return (EBADF);

	FILE_USE(fp);

	if ((fp->f_flag & FWRITE) == 0)
		error = EBADF;
	else
		error = ktrace_common(l, SCARG(uap, ops),
		    SCARG(uap, facs), SCARG(uap, pid), fp);

	FILE_UNUSE(fp, l);

	return error;
}

/*
 * ktrace system call
 */
/* ARGSUSED */
int
sys_ktrace(struct lwp *l, void *v, register_t *retval)
{
	struct sys_ktrace_args /* {
		syscallarg(const char *) fname;
		syscallarg(int) ops;
		syscallarg(int) facs;
		syscallarg(int) pid;
	} */ *uap = v;
	struct vnode *vp = NULL;
	struct file *fp = NULL;
	struct nameidata nd;
	int error = 0;
	int fd;

	if (ktrenter(l))
		return EAGAIN;

	if (KTROP(SCARG(uap, ops)) != KTROP_CLEAR) {
		/*
		 * an operation which requires a file argument.
		 */
		NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, fname),
		    l);
		if ((error = vn_open(&nd, FREAD|FWRITE, 0)) != 0) {
			ktrexit(l);
			return (error);
		}
		vp = nd.ni_vp;
		VOP_UNLOCK(vp, 0);
		if (vp->v_type != VREG) {
			(void) vn_close(vp, FREAD|FWRITE, l->l_cred, l);
			ktrexit(l);
			return (EACCES);
		}
		/*
		 * XXX This uses up a file descriptor slot in the
		 * tracing process for the duration of this syscall.
		 * This is not expected to be a problem.  If
		 * falloc(NULL, ...) DTRT we could skip that part, but
		 * that would require changing its interface to allow
		 * the caller to pass in a ucred..
		 *
		 * This will FILE_USE the fp it returns, if any.
		 * Keep it in use until we return.
		 */
		if ((error = falloc(l, &fp, &fd)) != 0)
			goto done;

		fp->f_flag = FWRITE;
		fp->f_type = DTYPE_VNODE;
		fp->f_ops = &vnops;
		fp->f_data = (void *)vp;
		FILE_SET_MATURE(fp);
		vp = NULL;
	}
	error = ktrace_common(l, SCARG(uap, ops), SCARG(uap, facs),
	    SCARG(uap, pid), fp);
done:
	if (vp != NULL)
		(void) vn_close(vp, FWRITE, l->l_cred, l);
	if (fp != NULL) {
		FILE_UNUSE(fp, l);	/* release file */
		fdrelease(l, fd); 	/* release fd table slot */
	}
	return (error);
}

int
ktrops(struct lwp *curl, struct proc *p, int ops, int facs,
    struct ktr_desc *ktd)
{
	int vers = ops & KTRFAC_VER_MASK;
	int error = 0;

	mutex_enter(&p->p_mutex);
	mutex_enter(&ktrace_mutex);

	if (!ktrcanset(curl, p))
		goto out;

	switch (vers) {
	case KTRFACv0:
	case KTRFACv1:
		break;
	default:
		error = EINVAL;
		goto out;
	}

	if (KTROP(ops) == KTROP_SET) {
		if (p->p_tracep != ktd) {
			/*
			 * if trace file already in use, relinquish
			 */
			ktrderef(p);
			p->p_tracep = ktd;
			ktradref(p);
		}
		p->p_traceflag |= facs;
		if (kauth_authorize_generic(curl->l_cred,
		    KAUTH_GENERIC_ISSUSER, NULL) == 0)
			p->p_traceflag |= KTRFAC_ROOT;
	} else {
		/* KTROP_CLEAR */
		if (((p->p_traceflag &= ~facs) & KTRFAC_MASK) == 0) {
			/* no more tracing */
			ktrderef(p);
		}
	}

	if (p->p_traceflag)
		p->p_traceflag |= vers;
	/*
	 * Emit an emulation record, every time there is a ktrace
	 * change/attach request.
	 */
	if (KTRPOINT(p, KTR_EMUL))
		p->p_traceflag |= KTRFAC_TRC_EMUL;
#ifdef __HAVE_SYSCALL_INTERN
	(*p->p_emul->e_syscall_intern)(p);
#endif

 out:
 	mutex_exit(&ktrace_mutex);
 	mutex_exit(&p->p_mutex);

	return (1);
}

int
ktrsetchildren(struct lwp *curl, struct proc *top, int ops, int facs,
    struct ktr_desc *ktd)
{
	struct proc *p;
	int ret = 0;

	KASSERT(mutex_owned(&proclist_lock));

	p = top;
	for (;;) {
		ret |= ktrops(curl, p, ops, facs, ktd);
		/*
		 * If this process has children, descend to them next,
		 * otherwise do any siblings, and if done with this level,
		 * follow back up the tree (but not past top).
		 */
		if (LIST_FIRST(&p->p_children) != NULL) {
			p = LIST_FIRST(&p->p_children);
			continue;
		}
		for (;;) {
			if (p == top)
				return (ret);
			if (LIST_NEXT(p, p_sibling) != NULL) {
				p = LIST_NEXT(p, p_sibling);
				break;
			}
			p = p->p_pptr;
		}
	}
	/*NOTREACHED*/
}

void
ktrwrite(struct ktr_desc *ktd, struct ktrace_entry *kte)
{
	struct uio auio;
	struct iovec aiov[64], *iov;
	struct ktrace_entry *top = kte;
	struct ktr_header *kth;
	struct file *fp = ktd->ktd_fp;
	int error;
next:
	auio.uio_iov = iov = &aiov[0];
	auio.uio_offset = 0;
	auio.uio_rw = UIO_WRITE;
	auio.uio_resid = 0;
	auio.uio_iovcnt = 0;
	UIO_SETUP_SYSSPACE(&auio);
	do {
		kth = &kte->kte_kth;

		if (kth->ktr_version == 0) {
			/*
			 * Convert back to the old format fields
			 */
			TIMESPEC_TO_TIMEVAL(&kth->ktr_tv, &kth->ktr_time);
			kth->ktr_unused = NULL;
		}
		iov->iov_base = (void *)kth;
		iov++->iov_len = sizeof(struct ktr_header);
		auio.uio_resid += sizeof(struct ktr_header);
		auio.uio_iovcnt++;
		if (kth->ktr_len > 0) {
			iov->iov_base = kte->kte_buf;
			iov++->iov_len = kth->ktr_len;
			auio.uio_resid += kth->ktr_len;
			auio.uio_iovcnt++;
		}
	} while ((kte = TAILQ_NEXT(kte, kte_list)) != NULL &&
	    auio.uio_iovcnt < sizeof(aiov) / sizeof(aiov[0]) - 1);

again:
	mutex_enter(&fp->f_lock);
	FILE_USE(fp);
	error = (*fp->f_ops->fo_write)(fp, &fp->f_offset, &auio,
	    fp->f_cred, FOF_UPDATE_OFFSET);
	FILE_UNUSE(fp, NULL);
	switch (error) {

	case 0:
		if (auio.uio_resid > 0)
			goto again;
		if (kte != NULL)
			goto next;
		break;

	case EWOULDBLOCK:
		kpause("ktrzzz", false, 1, NULL);
		goto again;

	default:
		/*
		 * If error encountered, give up tracing on this
		 * vnode.  Don't report EPIPE as this can easily
		 * happen with fktrace()/ktruss.
		 */
#ifndef DEBUG
		if (error != EPIPE)
#endif
			log(LOG_NOTICE,
			    "ktrace write failed, errno %d, tracing stopped\n",
			    error);
		(void)ktrderefall(ktd, 0);
	}

	while ((kte = top) != NULL) {
		top = TAILQ_NEXT(top, kte_list);
		ktefree(kte);
	}
}

void
ktrace_thread(void *arg)
{
	struct ktr_desc *ktd = arg;
	struct file *fp = ktd->ktd_fp;
	struct ktrace_entry *kte;
	int ktrerr, errcnt;

	mutex_enter(&ktrace_mutex);
	for (;;) {
		kte = TAILQ_FIRST(&ktd->ktd_queue);
		if (kte == NULL) {
			if (ktd->ktd_flags & KTDF_WAIT) {
				ktd->ktd_flags &= ~(KTDF_WAIT | KTDF_BLOCKING);
				cv_broadcast(&ktd->ktd_sync_cv);
			}
			if (ktd->ktd_ref == 0)
				break;
			cv_wait(&ktd->ktd_cv, &ktrace_mutex);
			continue;
		}
		TAILQ_INIT(&ktd->ktd_queue);
		ktd->ktd_qcount = 0;
		ktrerr = ktd->ktd_error;
		errcnt = ktd->ktd_errcnt;
		ktd->ktd_error = ktd->ktd_errcnt = 0;
		mutex_exit(&ktrace_mutex);

		if (ktrerr) {
			log(LOG_NOTICE,
			    "ktrace failed, fp %p, error 0x%x, total %d\n",
			    fp, ktrerr, errcnt);
		}
		ktrwrite(ktd, kte);
		mutex_enter(&ktrace_mutex);
	}

	TAILQ_REMOVE(&ktdq, ktd, ktd_list);
	mutex_exit(&ktrace_mutex);

	mutex_enter(&fp->f_lock);
	FILE_USE(fp);

	/*
	 * ktrace file descriptor can't be watched (are not visible to
	 * userspace), so no kqueue stuff here
	 * XXX: The above comment is wrong, because the fktrace file
	 * descriptor is available in userland.
	 */
	closef(fp, NULL);

	callout_stop(&ktd->ktd_wakch);
	kmem_free(ktd, sizeof(*ktd));

	kthread_exit(0);
}

/*
 * Return true if caller has permission to set the ktracing state
 * of target.  Essentially, the target can't possess any
 * more permissions than the caller.  KTRFAC_ROOT signifies that
 * root previously set the tracing status on the target process, and
 * so, only root may further change it.
 *
 * TODO: check groups.  use caller effective gid.
 */
int
ktrcanset(struct lwp *calll, struct proc *targetp)
{
	KASSERT(mutex_owned(&targetp->p_mutex));
	KASSERT(mutex_owned(&ktrace_mutex));

	if (kauth_authorize_process(calll->l_cred, KAUTH_PROCESS_CANKTRACE,
	    targetp, NULL, NULL, NULL) == 0)
		return (1);

	return (0);
}
#endif /* KTRACE */

/*
 * Put user defined entry to ktrace records.
 */
int
sys_utrace(struct lwp *l, void *v, register_t *retval)
{
#ifdef KTRACE
	struct sys_utrace_args /* {
		syscallarg(const char *) label;
		syscallarg(void *) addr;
		syscallarg(size_t) len;
	} */ *uap = v;
	struct proc *p = l->l_proc;

	if (!KTRPOINT(p, KTR_USER))
		return (0);

	return ktruser(l, SCARG(uap, label), SCARG(uap, addr),
	    SCARG(uap, len), 1);
#else /* !KTRACE */
	return ENOSYS;
#endif /* KTRACE */
}
