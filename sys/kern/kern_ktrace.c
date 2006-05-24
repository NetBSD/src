/*	$NetBSD: kern_ktrace.c,v 1.101.6.1 2006/05/24 15:50:40 tron Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: kern_ktrace.c,v 1.101.6.1 2006/05/24 15:50:40 tron Exp $");

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
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/callout.h>
#include <sys/kauth.h>

#include <sys/mount.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

#ifdef KTRACE

/*
 * XXX:
 *	- need better error reporting?
 *	- p->p_tracep access lock.  lock p_lock, lock ktd if !NULL, inc ref.
 *	- userland utility to sort ktrace.out by timestamp.
 *	- keep minimum information in ktrace_entry when rest of alloc failed.
 *	- enlarge ktrace_entry so that small entry won't require additional
 *	  alloc?
 *	- per trace control of configurable parameters.
 */

struct ktrace_entry {
	TAILQ_ENTRY(ktrace_entry) kte_list;
	struct ktr_header kte_kth;
	void *kte_buf;
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
	struct proc *ktd_proc;		/* our kernel thread */
	TAILQ_HEAD(, ktrace_entry) ktd_queue;
	struct callout ktd_wakch;	/* delayed wakeup */
	struct simplelock ktd_slock;
};

static void	ktrinitheader(struct ktr_header *, struct lwp *, int);
static void	ktrwrite(struct ktr_desc *, struct ktrace_entry *);
static int	ktrace_common(struct proc *, int, int, int, struct file *);
static int	ktrops(struct proc *, struct proc *, int, int,
		    struct ktr_desc *);
static int	ktrsetchildren(struct proc *, struct proc *, int, int,
		    struct ktr_desc *);
static int	ktrcanset(struct proc *, struct proc *);
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
static void	ktd_logerr(struct proc *, int);
static void	ktrace_thread(void *);

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

static struct simplelock ktdq_slock = SIMPLELOCK_INITIALIZER;
static TAILQ_HEAD(, ktr_desc) ktdq = TAILQ_HEAD_INITIALIZER(ktdq);

MALLOC_DEFINE(M_KTRACE, "ktrace", "ktrace data buffer");
POOL_INIT(kte_pool, sizeof(struct ktrace_entry), 0, 0, 0,
    "ktepl", &pool_allocator_nointr);

static inline void
ktd_wakeup(struct ktr_desc *ktd)
{

	callout_stop(&ktd->ktd_wakch);
	wakeup(ktd);
}

static void
ktd_logerrl(struct ktr_desc *ktd, int error)
{

	ktd->ktd_error |= error;
	ktd->ktd_errcnt++;
}

static void
ktd_logerr(struct proc *p, int error)
{
	struct ktr_desc *ktd = p->p_tracep;

	if (ktd == NULL)
		return;

	simple_lock(&ktd->ktd_slock);
	ktd_logerrl(ktd, error);
	simple_unlock(&ktd->ktd_slock);
}

/*
 * Release a reference.  Called with ktd_slock held.
 */
void
ktdrel(struct ktr_desc *ktd)
{

	KDASSERT(ktd->ktd_ref != 0);
	KASSERT(ktd->ktd_ref > 0);
	if (--ktd->ktd_ref <= 0) {
		ktd->ktd_flags |= KTDF_DONE;
		wakeup(ktd);
	}
	simple_unlock(&ktd->ktd_slock);
}

void
ktdref(struct ktr_desc *ktd)
{

	simple_lock(&ktd->ktd_slock);
	ktd->ktd_ref++;
	simple_unlock(&ktd->ktd_slock);
}

struct ktr_desc *
ktd_lookup(struct file *fp)
{
	struct ktr_desc *ktd;

	simple_lock(&ktdq_slock);
	for (ktd = TAILQ_FIRST(&ktdq); ktd != NULL;
	    ktd = TAILQ_NEXT(ktd, ktd_list)) {
		simple_lock(&ktd->ktd_slock);
		if (ktrsamefile(ktd->ktd_fp, fp)) {
			ktd->ktd_ref++;
			simple_unlock(&ktd->ktd_slock);
			break;
		}
		simple_unlock(&ktd->ktd_slock);
	}
	simple_unlock(&ktdq_slock);
	return (ktd);
}

void
ktraddentry(struct lwp *l, struct ktrace_entry *kte, int flags)
{
	struct proc *p = l->l_proc;
	struct ktr_desc *ktd;
#ifdef DEBUG
	struct timeval t;
	int s;
#endif

	if (p->p_traceflag & KTRFAC_TRC_EMUL) {
		/* Add emulation trace before first entry for this process */
		p->p_traceflag &= ~KTRFAC_TRC_EMUL;
		ktremul(l);
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

	simple_lock(&ktd->ktd_slock);
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
			s = splclock();
			t = mono_time;
			splx(s);
#endif
			if (ltsleep(&ktd->ktd_flags, PWAIT, "ktrsync",
			    ktd_timeout * hz, &ktd->ktd_slock) != 0) {
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
			s = splclock();
			timersub(&mono_time, &t, &t);
			splx(s);
			if (t.tv_sec > 0)
				log(LOG_NOTICE,
				    "ktrace long wait: %ld.%06ld\n",
				    t.tv_sec, t.tv_usec);
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
			    (void (*)(void *))wakeup, ktd);
	}

skip_sync:
	ktdrel(ktd);
	return;

relktd:
	ktdrel(ktd);

freekte:
	ktefree(kte);
}

void
ktefree(struct ktrace_entry *kte)
{

	if (kte->kte_buf != NULL)
		free(kte->kte_buf, M_KTRACE);
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

	p->p_traceflag = 0;
	if (ktd == NULL)
		return;
	p->p_tracep = NULL;

	simple_lock(&ktd->ktd_slock);
	wakeup(&ktd->ktd_flags);
	ktdrel(ktd);
}

void
ktradref(struct proc *p)
{
	struct ktr_desc *ktd = p->p_tracep;

	ktdref(ktd);
}

void
ktrinitheader(struct ktr_header *kth, struct lwp *l, int type)
{
	struct proc *p = l->l_proc;

	(void)memset(kth, 0, sizeof(*kth));
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
}

void
ktrsyscall(struct lwp *l, register_t code, register_t realcode,
    const struct sysent *callp, register_t args[])
{
	struct proc *p = l->l_proc;
	struct ktrace_entry *kte;
	struct ktr_header *kth;
	struct ktr_syscall *ktp;
	register_t *argp;
	int argsize;
	size_t len;
	u_int i;

	if (callp == NULL)
		callp = p->p_emul->e_sysent;

	argsize = callp[code].sy_argsize;
#ifdef _LP64
	if (p->p_flag & P_32)
		argsize = argsize << 1;
#endif
	len = sizeof(struct ktr_syscall) + argsize;

	p->p_traceflag |= KTRFAC_ACTIVE;
	kte = pool_get(&kte_pool, PR_WAITOK);
	kth = &kte->kte_kth;
	ktrinitheader(kth, l, KTR_SYSCALL);

	ktp = malloc(len, M_KTRACE, M_WAITOK);
	ktp->ktr_code = realcode;
	ktp->ktr_argsize = argsize;
	argp = (register_t *)(ktp + 1);
	for (i = 0; i < (argsize / sizeof(*argp)); i++)
		*argp++ = args[i];
	kth->ktr_len = len;
	kte->kte_buf = ktp;

	ktraddentry(l, kte, KTA_WAITOK);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrsysret(struct lwp *l, register_t code, int error, register_t *retval)
{
	struct proc *p = l->l_proc;
	struct ktrace_entry *kte;
	struct ktr_header *kth;
	struct ktr_sysret *ktp;

	p->p_traceflag |= KTRFAC_ACTIVE;
	kte = pool_get(&kte_pool, PR_WAITOK);
	kth = &kte->kte_kth;
	ktrinitheader(kth, l, KTR_SYSRET);

	ktp = malloc(sizeof(struct ktr_sysret), M_KTRACE, M_WAITOK);
	ktp->ktr_code = code;
	ktp->ktr_eosys = 0;			/* XXX unused */
	ktp->ktr_error = error;
	ktp->ktr_retval = retval ? retval[0] : 0;
	ktp->ktr_retval_1 = retval ? retval[1] : 0;

	kth->ktr_len = sizeof(struct ktr_sysret);
	kte->kte_buf = ktp;

	ktraddentry(l, kte, KTA_WAITOK);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
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
 	struct proc *p = l->l_proc;
	struct ktrace_entry *kte;
	struct ktr_header *kth;

	p->p_traceflag |= KTRFAC_ACTIVE;
	kte = pool_get(&kte_pool, PR_WAITOK);
	kth = &kte->kte_kth;
	ktrinitheader(kth, l, type);

	kth->ktr_len = len;
	kte->kte_buf = malloc(len, M_KTRACE, M_WAITOK);
	memcpy(kte->kte_buf, bf, len);

	ktraddentry(l, kte, KTA_WAITOK);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrgenio(struct lwp *l, int fd, enum uio_rw rw, struct iovec *iov,
    int len, int error)
{
	struct proc *p = l->l_proc;
	struct ktrace_entry *kte;
	struct ktr_header *kth;
	struct ktr_genio *ktp;
	int resid = len, cnt;
	caddr_t cp;
	int buflen;

	if (error)
		return;

	p->p_traceflag |= KTRFAC_ACTIVE;

next:
	buflen = min(PAGE_SIZE, resid + sizeof(struct ktr_genio));

	kte = pool_get(&kte_pool, PR_WAITOK);
	kth = &kte->kte_kth;
	ktrinitheader(kth, l, KTR_GENIO);

	ktp = malloc(buflen, M_KTRACE, M_WAITOK);
	ktp->ktr_fd = fd;
	ktp->ktr_rw = rw;

	kte->kte_buf = ktp;

	cp = (caddr_t)(ktp + 1);
	buflen -= sizeof(struct ktr_genio);
	kth->ktr_len = sizeof(struct ktr_genio);

	while (buflen > 0) {
		cnt = min(iov->iov_len, buflen);
		if (copyin(iov->iov_base, cp, cnt) != 0)
			goto out;
		kth->ktr_len += cnt;
		buflen -= cnt;
		resid -= cnt;
		iov->iov_len -= cnt;
		if (iov->iov_len == 0)
			iov++;
		else
			iov->iov_base = (caddr_t)iov->iov_base + cnt;
	}

	/*
	 * Don't push so many entry at once.  It will cause kmem map
	 * shortage.
	 */
	ktraddentry(l, kte, KTA_WAITOK | KTA_LARGE);
	if (resid > 0) {
#if 0 /* XXX NJWLWP */
		KDASSERT(p->p_cpu != NULL);
		KDASSERT(p->p_cpu == curcpu());
#endif
		/* XXX NJWLWP */
		if (curcpu()->ci_schedstate.spc_flags & SPCF_SHOULDYIELD)
			preempt(1);

		goto next;
	}

	p->p_traceflag &= ~KTRFAC_ACTIVE;
	return;

out:
	ktefree(kte);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrpsig(struct lwp *l, int sig, sig_t action, const sigset_t *mask,
    const ksiginfo_t *ksi)
{
	struct proc *p = l->l_proc;
	struct ktrace_entry *kte;
	struct ktr_header *kth;
	struct {
		struct ktr_psig	kp;
		siginfo_t	si;
	} *kbuf;

	p->p_traceflag |= KTRFAC_ACTIVE;
	kte = pool_get(&kte_pool, PR_WAITOK);
	kth = &kte->kte_kth;
	ktrinitheader(kth, l, KTR_PSIG);

	kbuf = malloc(sizeof(*kbuf), M_KTRACE, M_WAITOK);
	kbuf->kp.signo = (char)sig;
	kbuf->kp.action = action;
	kbuf->kp.mask = *mask;
	kte->kte_buf = kbuf;
	if (ksi) {
		kbuf->kp.code = KSI_TRAPCODE(ksi);
		(void)memset(&kbuf->si, 0, sizeof(kbuf->si));
		kbuf->si._info = ksi->ksi_info;
		kth->ktr_len = sizeof(*kbuf);
	} else {
		kbuf->kp.code = 0;
		kth->ktr_len = sizeof(struct ktr_psig);
	}

	ktraddentry(l, kte, KTA_WAITOK);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrcsw(struct lwp *l, int out, int user)
{
	struct proc *p = l->l_proc;
	struct ktrace_entry *kte;
	struct ktr_header *kth;
	struct ktr_csw *kc;

	p->p_traceflag |= KTRFAC_ACTIVE;

	/*
	 * We can't sleep if we're already going to sleep (if original
	 * condition is met during sleep, we hang up).
	 */
	kte = pool_get(&kte_pool, out ? PR_NOWAIT : PR_WAITOK);
	if (kte == NULL) {
		ktd_logerr(p, KTDE_ENOMEM);
		goto out;
	}
	kth = &kte->kte_kth;
	ktrinitheader(kth, l, KTR_CSW);

	kc = malloc(sizeof(struct ktr_csw), M_KTRACE,
	    out ? M_NOWAIT : M_WAITOK);
	if (kc == NULL) {
		ktd_logerr(p, KTDE_ENOMEM);
		goto free_kte;
	}
	kc->out = out;
	kc->user = user;
	kth->ktr_len = sizeof(struct ktr_csw);
	kte->kte_buf = kc;

	ktraddentry(l, kte, out ? KTA_NOWAIT : KTA_WAITOK);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
	return;

free_kte:
	pool_put(&kte_pool, kte);
out:
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktruser(struct lwp *l, const char *id, void *addr, size_t len, int ustr)
{
	struct proc *p = l->l_proc;
	struct ktrace_entry *kte;
	struct ktr_header *kth;
	struct ktr_user *ktp;
	caddr_t user_dta;

	p->p_traceflag |= KTRFAC_ACTIVE;
	kte = pool_get(&kte_pool, PR_WAITOK);
	kth = &kte->kte_kth;
	ktrinitheader(kth, l, KTR_USER);

	ktp = malloc(sizeof(struct ktr_user) + len, M_KTRACE, M_WAITOK);
	if (ustr) {
		if (copyinstr(id, ktp->ktr_id, KTR_USER_MAXIDLEN, NULL) != 0)
			ktp->ktr_id[0] = '\0';
	} else
		strncpy(ktp->ktr_id, id, KTR_USER_MAXIDLEN);
	ktp->ktr_id[KTR_USER_MAXIDLEN-1] = '\0';

	user_dta = (caddr_t)(ktp + 1);
	if (copyin(addr, (void *)user_dta, len) != 0)
		len = 0;

	kth->ktr_len = sizeof(struct ktr_user) + len;
	kte->kte_buf = ktp;

	ktraddentry(l, kte, KTA_WAITOK);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrmmsg(struct lwp *l, const void *msgh, size_t size)
{
	ktrkmem(l, KTR_MMSG, msgh, size);
}

void
ktrmool(struct lwp *l, const void *kaddr, size_t size, const void *uaddr)
{
	struct proc *p = l->l_proc;
	struct ktrace_entry *kte;
	struct ktr_header *kth;
	struct ktr_mool *kp;
	struct ktr_mool *bf;

	p->p_traceflag |= KTRFAC_ACTIVE;
	kte = pool_get(&kte_pool, PR_WAITOK);
	kth = &kte->kte_kth;
	ktrinitheader(kth, l, KTR_MOOL);

	kp = malloc(size + sizeof(*kp), M_KTRACE, M_WAITOK);
	kp->uaddr = uaddr;
	kp->size = size;
	bf = kp + 1; /* Skip uaddr and size */
	(void)memcpy(bf, kaddr, size);

	kth->ktr_len = size + sizeof(*kp);
	kte->kte_buf = kp;

	ktraddentry(l, kte, KTA_WAITOK);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrsaupcall(struct lwp *l, int type, int nevent, int nint, void *sas,
    void *ap)
{
	struct proc *p = l->l_proc;
	struct ktrace_entry *kte;
	struct ktr_header *kth;
	struct ktr_saupcall *ktp;
	size_t len;
	struct sa_t **sapp;
	int i;

	p->p_traceflag |= KTRFAC_ACTIVE;
	kte = pool_get(&kte_pool, PR_WAITOK);
	kth = &kte->kte_kth;
	ktrinitheader(kth, l, KTR_SAUPCALL);

	len = sizeof(struct ktr_saupcall);
	ktp = malloc(len + sizeof(struct sa_t) * (nevent + nint + 1), M_KTRACE,
	    M_WAITOK);

	ktp->ktr_type = type;
	ktp->ktr_nevent = nevent;
	ktp->ktr_nint = nint;
	ktp->ktr_sas = sas;
	ktp->ktr_ap = ap;
	/*
	 *  Copy the sa_t's
	 */
	sapp = (struct sa_t **) sas;

	for (i = nevent + nint; i >= 0; i--) {
		if (copyin(*sapp, (char *)ktp + len, sizeof(struct sa_t)) == 0)
			len += sizeof(struct sa_t);
		sapp++;
	}

	kth->ktr_len = len;
	kte->kte_buf = ktp;

	ktraddentry(l, kte, KTA_WAITOK);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

/* Interface and common routines */

int
ktrace_common(struct proc *curp, int ops, int facs, int pid, struct file *fp)
{
	struct proc *p;
	struct pgrp *pg;
	struct ktr_desc *ktd = NULL;
	int ret = 0;
	int error = 0;
	int descend;

	curp->p_traceflag |= KTRFAC_ACTIVE;
	descend = ops & KTRFLAG_DESCEND;
	facs = facs & ~((unsigned) KTRFAC_ROOT);

	switch (KTROP(ops)) {

	case KTROP_CLEARFILE:
		/*
		 * Clear all uses of the tracefile
		 */

		ktd = ktd_lookup(fp);
		if (ktd == NULL)
			goto done;

		proclist_lock_read();
		PROCLIST_FOREACH(p, &allproc) {
			if (p->p_tracep == ktd) {
				if (ktrcanset(curp, p))
					ktrderef(p);
				else
					error = EPERM;
			}
		}
		proclist_unlock_read();
		goto done;

	case KTROP_SET:
		ktd = ktd_lookup(fp);
		if (ktd == NULL) {
			ktd = malloc(sizeof(struct ktr_desc),
			    M_KTRACE, M_WAITOK);
			TAILQ_INIT(&ktd->ktd_queue);
			simple_lock_init(&ktd->ktd_slock);
			callout_init(&ktd->ktd_wakch);
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

			error = kthread_create1(ktrace_thread, ktd,
			    &ktd->ktd_proc, "ktr %p", ktd);
			if (error != 0) {
				free(ktd, M_KTRACE);
				goto done;
			}

			simple_lock(&fp->f_slock);
			fp->f_count++;
			simple_unlock(&fp->f_slock);
			ktd->ktd_fp = fp;

			simple_lock(&ktdq_slock);
			TAILQ_INSERT_TAIL(&ktdq, ktd, ktd_list);
			simple_unlock(&ktdq_slock);
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
	if (pid < 0) {
		/*
		 * by process group
		 */
		pg = pg_find(-pid, PFIND_UNLOCK_FAIL);
		if (pg == NULL) {
			error = ESRCH;
			goto done;
		}
		LIST_FOREACH(p, &pg->pg_members, p_pglist) {
			if (descend)
				ret |= ktrsetchildren(curp, p, ops, facs, ktd);
			else
				ret |= ktrops(curp, p, ops, facs, ktd);
		}

	} else {
		/*
		 * by pid
		 */
		p = p_find(pid, PFIND_UNLOCK_FAIL);
		if (p == NULL) {
			error = ESRCH;
			goto done;
		}
		if (descend)
			ret |= ktrsetchildren(curp, p, ops, facs, ktd);
		else
			ret |= ktrops(curp, p, ops, facs, ktd);
	}
	proclist_unlock_read();	/* taken by p{g}_find */
	if (!ret)
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
			    simple_lock(&ktd->ktd_slock);
			    ktdrel(ktd);
		}
	}
	curp->p_traceflag &= ~KTRFAC_ACTIVE;
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
	struct proc *curp;
	struct file *fp = NULL;
	struct filedesc *fdp = l->l_proc->p_fd;
	int error;

	curp = l->l_proc;
	fdp = curp->p_fd;
	if ((fp = fd_getfile(fdp, SCARG(uap, fd))) == NULL)
		return (EBADF);

	FILE_USE(fp);

	if ((fp->f_flag & FWRITE) == 0)
		error = EBADF;
	else
		error = ktrace_common(curp, SCARG(uap, ops),
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
	struct proc *curp = l->l_proc;
	struct vnode *vp = NULL;
	struct file *fp = NULL;
	struct nameidata nd;
	int error = 0;
	int fd;

	curp->p_traceflag |= KTRFAC_ACTIVE;
	if (KTROP(SCARG(uap, ops)) != KTROP_CLEAR) {
		/*
		 * an operation which requires a file argument.
		 */
		NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, fname),
		    l);
		if ((error = vn_open(&nd, FREAD|FWRITE, 0)) != 0) {
			curp->p_traceflag &= ~KTRFAC_ACTIVE;
			return (error);
		}
		vp = nd.ni_vp;
		VOP_UNLOCK(vp, 0);
		if (vp->v_type != VREG) {
			(void) vn_close(vp, FREAD|FWRITE, curp->p_cred, l);
			curp->p_traceflag &= ~KTRFAC_ACTIVE;
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
		if ((error = falloc(curp, &fp, &fd)) != 0)
			goto done;

		fp->f_flag = FWRITE;
		fp->f_type = DTYPE_VNODE;
		fp->f_ops = &vnops;
		fp->f_data = (caddr_t)vp;
		FILE_SET_MATURE(fp);
		vp = NULL;
	}
	error = ktrace_common(curp, SCARG(uap, ops), SCARG(uap, facs),
	    SCARG(uap, pid), fp);
done:
	if (vp != NULL)
		(void) vn_close(vp, FWRITE, curp->p_cred, l);
	if (fp != NULL) {
		FILE_UNUSE(fp, l);	/* release file */
		fdrelease(l, fd); 	/* release fd table slot */
	}
	return (error);
}

int
ktrops(struct proc *curp, struct proc *p, int ops, int facs,
    struct ktr_desc *ktd)
{

	int vers = ops & KTRFAC_VER_MASK;

	if (!ktrcanset(curp, p))
		return (0);

	switch (vers) {
	case KTRFACv0:
	case KTRFACv1:
		break;
	default:
		return EINVAL;
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
		if (kauth_cred_geteuid(curp->p_cred) == 0)
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

	return (1);
}

int
ktrsetchildren(struct proc *curp, struct proc *top, int ops, int facs,
    struct ktr_desc *ktd)
{
	struct proc *p;
	int ret = 0;

	p = top;
	for (;;) {
		ret |= ktrops(curp, p, ops, facs, ktd);
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
	struct proc *p;
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
		iov->iov_base = (caddr_t)kth;
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
	simple_lock(&fp->f_slock);
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
		preempt(1);
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
		proclist_lock_read();
		PROCLIST_FOREACH(p, &allproc) {
			if (p->p_tracep == ktd)
				ktrderef(p);
		}
		proclist_unlock_read();
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

	for (;;) {
		simple_lock(&ktd->ktd_slock);
		kte = TAILQ_FIRST(&ktd->ktd_queue);
		if (kte == NULL) {
			if (ktd->ktd_flags & KTDF_WAIT) {
				ktd->ktd_flags &= ~(KTDF_WAIT | KTDF_BLOCKING);
				wakeup(&ktd->ktd_flags);
			}
			if (ktd->ktd_ref == 0)
				break;
			ltsleep(ktd, PWAIT | PNORELOCK, "ktrwait", 0,
			    &ktd->ktd_slock);
			continue;
		}
		TAILQ_INIT(&ktd->ktd_queue);
		ktd->ktd_qcount = 0;
		ktrerr = ktd->ktd_error;
		errcnt = ktd->ktd_errcnt;
		ktd->ktd_error = ktd->ktd_errcnt = 0;
		simple_unlock(&ktd->ktd_slock);

		if (ktrerr) {
			log(LOG_NOTICE,
			    "ktrace failed, fp %p, error 0x%x, total %d\n",
			    fp, ktrerr, errcnt);
		}
		ktrwrite(ktd, kte);
	}
	simple_unlock(&ktd->ktd_slock);

	simple_lock(&ktdq_slock);
	TAILQ_REMOVE(&ktdq, ktd, ktd_list);
	simple_unlock(&ktdq_slock);

	simple_lock(&fp->f_slock);
	FILE_USE(fp);

	/*
	 * ktrace file descriptor can't be watched (are not visible to
	 * userspace), so no kqueue stuff here
	 * XXX: The above comment is wrong, because the fktrace file
	 * descriptor is available in userland.
	 */
	closef(fp, NULL);

	callout_stop(&ktd->ktd_wakch);
	free(ktd, M_KTRACE);

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
ktrcanset(struct proc *callp, struct proc *targetp)
{
	kauth_cred_t caller = callp->p_cred;
	kauth_cred_t target = targetp->p_cred;

	if ((kauth_cred_geteuid(caller) == kauth_cred_getuid(target) &&
	    kauth_cred_getuid(target) == kauth_cred_getsvuid(target) &&
	    kauth_cred_getgid(caller) == kauth_cred_getgid(target) &&	/* XXX */
	    kauth_cred_getgid(target) == kauth_cred_getsvgid(target) &&
	    (targetp->p_traceflag & KTRFAC_ROOT) == 0 &&
	    (targetp->p_flag & P_SUGID) == 0) ||
	    kauth_cred_geteuid(caller) == 0)
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

	if (SCARG(uap, len) > KTR_USER_MAXLEN)
		return (EINVAL);

	ktruser(l, SCARG(uap, label), SCARG(uap, addr), SCARG(uap, len), 1);

	return (0);
#else /* !KTRACE */
	return ENOSYS;
#endif /* KTRACE */
}
