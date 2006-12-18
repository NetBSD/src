/* $NetBSD: puffs_transport.c,v 1.1.2.3 2006/12/18 11:42:15 yamt Exp $ */

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
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: puffs_transport.c,v 1.1.2.3 2006/12/18 11:42:15 yamt Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/malloc.h>
#include <sys/poll.h>
#include <sys/socketvar.h>

#include <fs/puffs/puffs_sys.h>

#include <miscfs/syncfs/syncfs.h> /* XXX: for syncer_lock reference */


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

/* protects both the list and the contents of the list elements */
static struct simplelock pi_lock = SIMPLELOCK_INITIALIZER;

static int get_pi_idx(struct puffs_instance *);


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
	simple_lock(&pmp->pmp_lock);
	if (!TAILQ_EMPTY(&pmp->pmp_req_touser))
		revents |= PUFFPOLL_EVSET;
	else
		selrecord(l, pmp->pmp_sel);
	simple_unlock(&pmp->pmp_lock);

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
	simple_lock(&pi_lock);
	pmp = FPTOPMP(fp);
	/*
	 * First check if the fs was never mounted.  In that case
	 * remove the instance from the list.  If mount is attempted later,
	 * it will simply fail.
	 */
	if (pmp == PMP_EMBRYO) {
		pi = FPTOPI(fp);
		TAILQ_REMOVE(&puffs_ilist, pi, pi_entries);
		simple_unlock(&pi_lock);
		FREE(pi, M_PUFFS);

		DPRINTF(("puffs_fop_close: pmp associated with fp %p was "
		    "embryonic\n", fp));

		return 0;
	}

	/*
	 * Next, analyze unmount was called and the instance is dead.
	 * In this case we can just free the structure and go home, it
	 * was removed from the list by puffs_nukebypmp().
	 */
	if (pmp == PMP_DEAD) {
		/* would be nice, but don't have a reference to it ... */
		/* KASSERT(pmp_status == PUFFSTAT_DYING); */
		simple_unlock(&pi_lock);

		DPRINTF(("puffs_fop_close: pmp associated with fp %p was "
		    "dead\n", fp));

		goto out;
	}

	/*
	 * So we have a reference.  Proceed to unwrap the file system.
	 */
	mp = PMPTOMP(pmp);
	simple_unlock(&pi_lock);

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
	simple_lock(&pmp->pmp_lock);
	if (pmp->pmp_unmounting) {
		ltsleep(&pmp->pmp_unmounting, PNORELOCK | PVFS, "puffsum",
		    0, &pmp->pmp_lock);
		DPRINTF(("puffs_fop_close: unmount was in progress for pmp %p, "
		    "restart\n", pmp));
		goto restart;
	}
	simple_unlock(&pmp->pmp_lock);

	/*
	 * Detach from VFS.  First do necessary XXX-dance (from
	 * sys_unmount() & other callers of dounmount()
	 *
	 * XXX Freeze syncer.  Must do this before locking the
	 * mount point.  See dounmount() for details.
	 *
	 * XXX2: take a reference to the mountpoint before starting to
	 * wait for syncer_lock.  Otherwise the mointpoint can be
	 * wiped out while we wait.
	 */
	simple_lock(&mp->mnt_slock);
	mp->mnt_wcnt++;
	simple_unlock(&mp->mnt_slock);

	lockmgr(&syncer_lock, LK_EXCLUSIVE, NULL);

	simple_lock(&mp->mnt_slock);
	mp->mnt_wcnt--;
	if (mp->mnt_wcnt == 0)
		wakeup(&mp->mnt_wcnt);
	gone = mp->mnt_iflag & IMNT_GONE;
	simple_unlock(&mp->mnt_slock);
	if (gone) {
		lockmgr(&syncer_lock, LK_RELEASE, NULL);
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
		lockmgr(&syncer_lock, LK_RELEASE, NULL);
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
puffs_fop_ioctl(struct file *fp, u_long cmd, void *data, struct lwp *l)
{
	struct puffs_mount *pmp = FPTOPMP(fp);
	int rv;

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

	case PUFFSSTARTOP:
		rv = puffs_start2(pmp, data);
		break;

	/* already done in sys_ioctl() */
	case FIONBIO:
		rv = 0;
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

	simple_lock(&pi_lock);
	SLIST_REMOVE(&pi->pi_sel.sel_klist, kn, knote, kn_selnext);
	simple_unlock(&pi_lock);
}

static int
filt_puffsioctl(struct knote *kn, long hint)
{
	struct puffs_instance *pi = kn->kn_hook;
	struct puffs_mount *pmp;
	int error;

	error = 0;
	simple_lock(&pi_lock);
	pmp = pi->pi_pmp;
	if (pmp == PMP_EMBRYO || pmp == PMP_DEAD)
		error = 1;
	simple_unlock(&pi_lock);
	if (error)
		return 0;

	simple_lock(&pmp->pmp_lock);
	kn->kn_data = pmp->pmp_req_touser_waiters;
	simple_unlock(&pmp->pmp_lock);

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

	simple_lock(&pi_lock);
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	simple_unlock(&pi_lock);

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

	simple_lock(&pi_lock);
	idx = get_pi_idx(pi);
	if (idx == PUFFS_CLONER) {
		simple_unlock(&pi_lock);
		FREE(pi, M_PUFFS);
		FILE_UNUSE(fp, l);
		ffree(fp);
		return EBUSY;
	}

	pi->pi_pid = l->l_proc->p_pid;
	pi->pi_idx = idx;
	simple_unlock(&pi_lock);

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

	simple_lock(&pi_lock);
	TAILQ_FOREACH(pi, &puffs_ilist, pi_entries) {
		if (pi->pi_pid == pid && pi->pi_pmp == PMP_EMBRYO) {
			pi->pi_pmp = pmp;
			pi->pi_fd = fd;
			pmp->pmp_sel = &pi->pi_sel;
			rv = 0;
			break;
		    }
	}
	simple_unlock(&pi_lock);

	return rv;
}

/*
 * Remove mount point from list of instances.  Called from unmount.
 */
void
puffs_nukebypmp(struct puffs_mount *pmp)
{
	struct puffs_instance *pi;

	simple_lock(&pi_lock);
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

	simple_unlock(&pi_lock);

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
