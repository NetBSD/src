/* $NetBSD: puffs_transport.c,v 1.19 2007/06/06 01:55:00 pooka Exp $ */

/*
 * Copyright (c) 2006  Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Ulla Tuominen Foundation.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: puffs_transport.c,v 1.19 2007/06/06 01:55:00 pooka Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/fstrans.h>
#include <sys/kthread.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/poll.h>
#include <sys/socketvar.h>

#include <uvm/uvm_param.h>

#include <fs/puffs/puffs_sys.h>

#include <miscfs/syncfs/syncfs.h> /* XXX: for syncer_mutex reference */


/*
 * puffs instance structures.  these are always allocated and freed
 * from the context of the device node / fileop code.
 */
struct puffs_instance {
	pid_t pi_pid;
	int pi_idx;
	int pi_fd;
	struct puffs_mount *pi_pmp;
	struct selinfo pi_sel;

	TAILQ_ENTRY(puffs_instance) pi_entries;
};
#define PMP_EMBRYO ((struct puffs_mount *)-1)	/* before mount	*/
#define PMP_DEAD ((struct puffs_mount *)-2)	/* goner	*/

static TAILQ_HEAD(, puffs_instance) puffs_ilist
    = TAILQ_HEAD_INITIALIZER(puffs_ilist);

static int get_pi_idx(struct puffs_instance *);


/*
 * public init / deinit
 */

/* protects both the list and the contents of the list elements */
static kmutex_t pi_mtx;

void
puffs_transport_init()
{

	mutex_init(&pi_mtx, MUTEX_DEFAULT, IPL_NONE);
}

void
puffs_transport_destroy()
{

	mutex_destroy(&pi_mtx);
}

/*
 * fd routines, for cloner
 */
static int puffs_fop_read(struct file *, off_t *, struct uio *,
			  kauth_cred_t, int);
static int puffs_fop_write(struct file *, off_t *, struct uio *,
			   kauth_cred_t, int);
static int puffs_fop_ioctl(struct file*, u_long, void *, struct lwp *);
static int puffs_fop_poll(struct file *, int, struct lwp *);
static int puffs_fop_close(struct file *, struct lwp *);
static int puffs_fop_kqfilter(struct file *, struct knote *);


static const struct fileops puffs_fileops = {
	puffs_fop_read,
	puffs_fop_write,
	puffs_fop_ioctl,
	fnullop_fcntl,
	puffs_fop_poll,
	fbadop_stat,
	puffs_fop_close,
	puffs_fop_kqfilter
};

static int
puffs_fop_read(struct file *fp, off_t *off, struct uio *uio,
	kauth_cred_t cred, int flags)
{

	return ENODEV;
}

static int
puffs_fop_write(struct file *fp, off_t *off, struct uio *uio,
	kauth_cred_t cred, int flags)
{

	return ENODEV;
}

/*
 * Poll query interface.  The question is only if an event
 * can be read from us (and by read I mean ioctl... ugh).
 */
#define PUFFPOLL_EVSET (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI)
static int
puffs_fop_poll(struct file *fp, int events, struct lwp *l)
{
	struct puffs_mount *pmp = FPTOPMP(fp);
	int revents;

	if (pmp == PMP_EMBRYO || pmp == PMP_DEAD) {
		printf("puffs_fop_ioctl: puffs %p, not mounted\n", pmp);
		return ENOENT;
	}

	revents = events & (POLLOUT | POLLWRNORM | POLLWRBAND);
	if ((events & PUFFPOLL_EVSET) == 0)
		return revents;

	/* check queue */
	mutex_enter(&pmp->pmp_lock);
	if (!TAILQ_EMPTY(&pmp->pmp_req_touser))
		revents |= PUFFPOLL_EVSET;
	else
		selrecord(l, pmp->pmp_sel);
	mutex_exit(&pmp->pmp_lock);

	return revents;
}

/*
 * device close = forced unmount.
 *
 * unmounting is a frightfully complex operation to avoid races
 */
static int
puffs_fop_close(struct file *fp, struct lwp *l)
{
	struct puffs_instance *pi;
	struct puffs_mount *pmp;
	struct mount *mp;
	int gone, rv;

	DPRINTF(("puffs_fop_close: device closed, force filesystem unmount\n"));

 restart:
	mutex_enter(&pi_mtx);
	pmp = FPTOPMP(fp);
	/*
	 * First check if the fs was never mounted.  In that case
	 * remove the instance from the list.  If mount is attempted later,
	 * it will simply fail.
	 */
	if (pmp == PMP_EMBRYO) {
		pi = FPTOPI(fp);
		TAILQ_REMOVE(&puffs_ilist, pi, pi_entries);
		mutex_exit(&pi_mtx);
		FREE(pi, M_PUFFS);

		DPRINTF(("puffs_fop_close: pmp associated with fp %p was "
		    "embryonic\n", fp));

		return 0;
	}

	/*
	 * Next, analyze if unmount was called and the instance is dead.
	 * In this case we can just free the structure and go home, it
	 * was removed from the list by puffs_nukebypmp().
	 */
	if (pmp == PMP_DEAD) {
		/* would be nice, but don't have a reference to it ... */
		/* KASSERT(pmp_status == PUFFSTAT_DYING); */
		mutex_exit(&pi_mtx);

		DPRINTF(("puffs_fop_close: pmp associated with fp %p was "
		    "dead\n", fp));

		goto out;
	}

	/*
	 * So we have a reference.  Proceed to unwrap the file system.
	 */
	mp = PMPTOMP(pmp);
	mutex_exit(&pi_mtx);

	/* hmm?  suspicious locking? */

	mutex_enter(&pmp->pmp_lock);
	puffs_mp_reference(pmp);

	/*
	 * Free the waiting callers before proceeding any further.
	 * The syncer might be jogging around in this file system
	 * currently.  If we allow it to go to the userspace of no
	 * return while trying to get the syncer lock, well ...
	 * synclk: I feel happy, I feel fine.
	 * lockmgr: You're not fooling anyone, you know.
	 */
	puffs_userdead(pmp);

	/*
	 * Make sure someone from puffs_unmount() isn't currently in
	 * userspace.  If we don't take this precautionary step,
	 * they might notice that the mountpoint has disappeared
	 * from under them once they return.  Especially note that we
	 * cannot simply test for an unmounter before calling
	 * dounmount(), since it might be possible that that particular
	 * invocation of unmount was called without MNT_FORCE.  Here we
	 * *must* make sure unmount succeeds.  Also, restart is necessary
	 * since pmp isn't locked.  We might end up with PMP_DEAD after
	 * restart and exit from there.
	 */
	if (pmp->pmp_unmounting) {
		cv_wait(&pmp->pmp_unmounting_cv, &pmp->pmp_lock);
		puffs_mp_release(pmp);
		mutex_exit(&pmp->pmp_lock);
		DPRINTF(("puffs_fop_close: unmount was in progress for pmp %p, "
		    "restart\n", pmp));
		goto restart;
	}

	/* Won't access pmp from here anymore */
	puffs_mp_release(pmp);
	mutex_exit(&pmp->pmp_lock);

	/*
	 * Detach from VFS.  First do necessary XXX-dance (from
	 * sys_unmount() & other callers of dounmount()
	 *
	 * XXX Freeze syncer.  Must do this before locking the
	 * mount point.  See dounmount() for details.
	 *
	 * XXX2: take a reference to the mountpoint before starting to
	 * wait for syncer_mutex.  Otherwise the mointpoint can be
	 * wiped out while we wait.
	 */
	simple_lock(&mp->mnt_slock);
	mp->mnt_wcnt++;
	simple_unlock(&mp->mnt_slock);

	mutex_enter(&syncer_mutex);

	simple_lock(&mp->mnt_slock);
	mp->mnt_wcnt--;
	if (mp->mnt_wcnt == 0)
		wakeup(&mp->mnt_wcnt);
	gone = mp->mnt_iflag & IMNT_GONE;
	simple_unlock(&mp->mnt_slock);
	if (gone) {
		mutex_exit(&syncer_mutex);
		goto out;
	}

	/*
	 * microscopic race condition here (although not with the current
	 * kernel), but can't really fix it without starting a crusade
	 * against vfs_busy(), so let it be, let it be, let it be
	 */

	/*
	 * The only way vfs_busy() will fail for us is if the filesystem
	 * is already a goner.
	 * XXX: skating on the thin ice of modern calling conventions ...
	 */
	if (vfs_busy(mp, 0, 0)) {
		mutex_exit(&syncer_mutex);
		goto out;
	}

	/*
	 * Once we have the mount point, unmount() can't interfere..
	 * or at least in theory it shouldn't.  dounmount() reentracy
	 * might require some visiting at some point.
	 */
	rv = dounmount(mp, MNT_FORCE, l);
	KASSERT(rv == 0);

 out:
	/*
	 * Finally, release the instance information.  It was already
	 * removed from the list by puffs_nukebypmp() and we know it's
	 * dead, so no need to lock.
	 */
	pi = FPTOPI(fp);
	FREE(pi, M_PUFFS);

	return 0;
}

static int
puffs_flush(struct puffs_mount *pmp, struct puffs_flush *pf)
{
	struct vnode *vp;
	voff_t offlo, offhi;
	int rv, flags = 0;

	/* XXX: slurry */
	if (pf->pf_op == PUFFS_INVAL_NAMECACHE_ALL) {
		cache_purgevfs(PMPTOMP(pmp));
		return 0;
	}

	rv = 0;

	/*
	 * Get vnode, don't lock it.  Namecache is protected by its own lock
	 * and we have a reference to protect against premature harvesting.
	 *
	 * The node we want here might be locked and the op is in
	 * userspace waiting for us to complete ==> deadlock.  Another
	 * reason we need to eventually bump locking to userspace, as we
	 * will need to lock the node if we wish to do flushes.
	 */
	vp = puffs_pnode2vnode(pmp, pf->pf_cookie, 0);
	if (vp == NULL)
		return ENOENT;

	switch (pf->pf_op) {
#if 0
	/* not quite ready, yet */
	case PUFFS_INVAL_NAMECACHE_NODE:
	struct componentname *pf_cn;
	char *name;
		/* get comfortab^Wcomponentname */
		MALLOC(pf_cn, struct componentname *,
		    sizeof(struct componentname), M_PUFFS, M_WAITOK | M_ZERO);
		memset(pf_cn, 0, sizeof(struct componentname));
		break;

#endif
	case PUFFS_INVAL_NAMECACHE_DIR:
		if (vp->v_type != VDIR) {
			rv = EINVAL;
			break;
		}
		cache_purge1(vp, NULL, PURGE_CHILDREN);
		break;

	case PUFFS_INVAL_PAGECACHE_NODE_RANGE:
		flags = PGO_FREE;
		/*FALLTHROUGH*/
	case PUFFS_FLUSH_PAGECACHE_NODE_RANGE:
		if (flags == 0)
			flags = PGO_CLEANIT;

		if (pf->pf_end > vp->v_size || vp->v_type != VREG) {
			rv = EINVAL;
			break;
		}

		offlo = trunc_page(pf->pf_start);
		offhi = round_page(pf->pf_end);
		if (offhi != 0 && offlo >= offhi) {
			rv = EINVAL;
			break;
		}

		simple_lock(&vp->v_uobj.vmobjlock);
		rv = VOP_PUTPAGES(vp, offlo, offhi, flags);
		break;

	default:
		rv = EINVAL;
	}

	vrele(vp);

	return rv;
}

static void
dosuspendresume(void *arg)
{
	struct puffs_mount *pmp = arg;
	struct mount *mp;
	int rv;

	mp = PMPTOMP(pmp);
	/*
	 * XXX?  does this really do any good or is it just
	 * paranoid stupidity?  or stupid paranoia?
	 */
	if (mp->mnt_iflag & IMNT_UNMOUNT) {
		printf("puffs dosuspendresume(): detected suspend on "
		    "unmounting fs\n");
		goto out;
	}

	/* Do the dance.  Allow only one concurrent suspend */
	rv = vfs_suspend(PMPTOMP(pmp), 1);
	if (rv == 0)
		vfs_resume(PMPTOMP(pmp));

 out:
	mutex_enter(&pmp->pmp_lock);
	KASSERT(pmp->pmp_suspend == 1);
	pmp->pmp_suspend = 0;
	puffs_mp_release(pmp);
	mutex_exit(&pmp->pmp_lock);

	kthread_exit(0);
}

static int
puffs_fop_ioctl(struct file *fp, u_long cmd, void *data, struct lwp *l)
{
	struct puffs_mount *pmp = FPTOPMP(fp);
	int rv;

	/*
	 * work already done in sys_ioctl().  skip sanity checks to enable
	 * setting non-blocking fd without yet having mounted the fs
	 */
	if (cmd == FIONBIO)
		return 0;

	if (pmp == PMP_EMBRYO || pmp == PMP_DEAD) {
		printf("puffs_fop_ioctl: puffs %p, not mounted\n", pmp);
		return ENOENT;
	}

	switch (cmd) {
	case PUFFSGETOP:
		rv = puffs_getop(pmp, data, fp->f_flag & FNONBLOCK);
		break;

	case PUFFSPUTOP:
		rv =  puffs_putop(pmp, data);
		break;

#if 0 /* bitrot */
	case PUFFSSIZEOP:
		rv = puffssizeop(pmp, data);
		break;
#endif

	case PUFFSFLUSHOP:
		rv = puffs_flush(pmp, data);
		break;

	case PUFFSSUSPENDOP:
		rv = 0;
		mutex_enter(&pmp->pmp_lock);
		if (pmp->pmp_suspend || pmp->pmp_status != PUFFSTAT_RUNNING) {
			rv = EBUSY;
		} else {
			puffs_mp_reference(pmp);
			pmp->pmp_suspend = 1;
		}
		mutex_exit(&pmp->pmp_lock);
		if (rv)
			break;
		rv = kthread_create1(dosuspendresume, pmp, NULL, "puffsusp");
		break;

	default:
		rv = EINVAL;
		break;
	}

	return rv;
}

/* kqueue stuff */

static void
filt_puffsdetach(struct knote *kn)
{
	struct puffs_instance *pi = kn->kn_hook;

	mutex_enter(&pi_mtx);
	SLIST_REMOVE(&pi->pi_sel.sel_klist, kn, knote, kn_selnext);
	mutex_exit(&pi_mtx);
}

static int
filt_puffsioctl(struct knote *kn, long hint)
{
	struct puffs_instance *pi = kn->kn_hook;
	struct puffs_mount *pmp;
	int error;

	error = 0;
	mutex_enter(&pi_mtx);
	pmp = pi->pi_pmp;
	if (pmp == PMP_EMBRYO || pmp == PMP_DEAD)
		error = 1;
	mutex_exit(&pi_mtx);
	if (error)
		return 0;

	mutex_enter(&pmp->pmp_lock);
	kn->kn_data = pmp->pmp_req_touser_count;
	mutex_exit(&pmp->pmp_lock);

	return kn->kn_data != 0;
}

static const struct filterops puffsioctl_filtops =
	{ 1, NULL, filt_puffsdetach, filt_puffsioctl };

static int
puffs_fop_kqfilter(struct file *fp, struct knote *kn)
{
	struct puffs_instance *pi = fp->f_data;
	struct klist *klist;

	if (kn->kn_filter != EVFILT_READ)
		return 1;

	klist = &pi->pi_sel.sel_klist;
	kn->kn_fop = &puffsioctl_filtops;
	kn->kn_hook = pi;

	mutex_enter(&pi_mtx);
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	mutex_exit(&pi_mtx);

	return 0;
}


/*
 * Device routines
 */

dev_type_open(puffscdopen);
dev_type_close(puffscdclose);
dev_type_ioctl(puffscdioctl);

/* dev */
const struct cdevsw puffs_cdevsw = {
	puffscdopen,	puffscdclose,	noread,		nowrite,
	noioctl,	nostop,		notty,		nopoll,
	nommap,		nokqfilter,	D_OTHER
};
int
puffscdopen(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct puffs_instance *pi;
	struct file *fp;
	int error, fd, idx;

	/*
	 * XXX: decide on some security model and check permissions
	 */

	if (minor(dev) != PUFFS_CLONER)
		return ENXIO;

	if ((error = falloc(l, &fp, &fd)) != 0)
		return error;

	MALLOC(pi, struct puffs_instance *, sizeof(struct puffs_instance),
	    M_PUFFS, M_WAITOK | M_ZERO);

	mutex_enter(&pi_mtx);
	idx = get_pi_idx(pi);
	if (idx == PUFFS_CLONER) {
		mutex_exit(&pi_mtx);
		FREE(pi, M_PUFFS);
		FILE_UNUSE(fp, l);
		ffree(fp);
		return EBUSY;
	}

	pi->pi_pid = l->l_proc->p_pid;
	pi->pi_idx = idx;
	mutex_exit(&pi_mtx);

	DPRINTF(("puffscdopen: registered embryonic pmp for pid: %d\n",
	    pi->pi_pid));

	return fdclone(l, fp, fd, FREAD|FWRITE, &puffs_fileops, pi);
}

int
puffscdclose(dev_t dev, int flags, int fmt, struct lwp *l)
{

	panic("puffscdclose impossible\n");

	return 0;
}


/*
 * Set puffs_mount -pointer.  Called from puffs_mount(), which is the
 * earliest place that knows about this.
 *
 * We only want to make sure that the caller had the right to open the
 * device, we don't so much care about which context it gets in case
 * the same process opened multiple (since they are equal at this point).
 */
int
puffs_setpmp(pid_t pid, int fd, struct puffs_mount *pmp)
{
	struct puffs_instance *pi;
	int rv = 1;

	mutex_enter(&pi_mtx);
	TAILQ_FOREACH(pi, &puffs_ilist, pi_entries) {
		if (pi->pi_pid == pid && pi->pi_pmp == PMP_EMBRYO) {
			pi->pi_pmp = pmp;
			pi->pi_fd = fd;
			pmp->pmp_sel = &pi->pi_sel;
			rv = 0;
			break;
		    }
	}
	mutex_exit(&pi_mtx);

	return rv;
}

/*
 * Remove mount point from list of instances.  Called from unmount.
 */
void
puffs_nukebypmp(struct puffs_mount *pmp)
{
	struct puffs_instance *pi;

	mutex_enter(&pi_mtx);
	TAILQ_FOREACH(pi, &puffs_ilist, pi_entries) {
		if (pi->pi_pmp == pmp) {
			TAILQ_REMOVE(&puffs_ilist, pi, pi_entries);
			break;
		}
	}
	if (pi)
		pi->pi_pmp = PMP_DEAD;

#ifdef DIAGNOSTIC
	else
		panic("puffs_nukebypmp: invalid puffs_mount\n");
#endif /* DIAGNOSTIC */

	mutex_exit(&pi_mtx);

	DPRINTF(("puffs_nukebypmp: nuked %p\n", pi));
}

/* search sorted list of instances for free minor, sorted insert arg */
static int
get_pi_idx(struct puffs_instance *pi_i)
{
	struct puffs_instance *pi;
	int i;

	i = 0;
	TAILQ_FOREACH(pi, &puffs_ilist, pi_entries) {
		if (i == PUFFS_CLONER)
			return PUFFS_CLONER;
		if (i != pi->pi_idx)
			break;
		i++;
	}

	pi_i->pi_pmp = PMP_EMBRYO;

	if (pi == NULL)
		TAILQ_INSERT_TAIL(&puffs_ilist, pi_i, pi_entries);
	else
		TAILQ_INSERT_BEFORE(pi, pi_i, pi_entries);

	return i;
}
