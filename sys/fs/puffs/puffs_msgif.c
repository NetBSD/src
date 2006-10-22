/*	$NetBSD: puffs_msgif.c,v 1.1 2006/10/22 22:43:23 pooka Exp $	*/

/*
 * Copyright (c) 2005, 2006  Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Google Summer of Code program and the Ulla Tuominen Foundation.
 * The Google SoC project was mentored by Bill Studenmund.
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
__KERNEL_RCSID(0, "$NetBSD: puffs_msgif.c,v 1.1 2006/10/22 22:43:23 pooka Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/socketvar.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/lock.h>
#include <sys/poll.h>

#include <fs/puffs/puffs_msgif.h>
#include <fs/puffs/puffs_sys.h>

#include <miscfs/syncfs/syncfs.h> /* XXX: for syncer_lock reference */


/*
 * kernel-user-kernel waitqueues
 */

static int touser(struct puffs_mount *, struct puffs_park *, unsigned int,
		  struct vnode *, struct vnode *);

unsigned int
puffs_getreqid(struct puffs_mount *pmp)
{
	unsigned int rv;

	simple_lock(&pmp->pmp_lock);
	rv = pmp->pmp_nextreq++;
	simple_unlock(&pmp->pmp_lock);

	return rv;
}

/* vfs request */
int
puffs_vfstouser(struct puffs_mount *pmp, int optype, void *kbuf, size_t buflen)
{
	struct puffs_req preq;
	struct puffs_park park;

	memset(&preq, 0, sizeof(struct puffs_req));

	preq.preq_opclass = PUFFSOP_VFS; 
	preq.preq_optype = optype;

	park.park_kernbuf = kbuf;
	park.park_buflen = buflen;
	park.park_copylen = buflen;
	park.park_flags = 0;
	park.park_preq = &preq;

	return touser(pmp, &park, puffs_getreqid(pmp), NULL, NULL);
}

/*
 * vnode level request
 */
int
puffs_vntouser(struct puffs_mount *pmp, int optype,
	void *kbuf, size_t buflen, void *cookie,
	struct vnode *vp1, struct vnode *vp2)
{
	struct puffs_req preq;
	struct puffs_park park;

	memset(&preq, 0, sizeof(struct puffs_req));

	preq.preq_opclass = PUFFSOP_VN; 
	preq.preq_optype = optype;
	preq.preq_cookie = cookie;

	park.park_kernbuf = kbuf;
	park.park_buflen = buflen;
	park.park_copylen = buflen;
	park.park_flags = 0;
	park.park_preq = &preq;

	return touser(pmp, &park, puffs_getreqid(pmp), vp1, vp2);
}

/*
 * vnode level request, caller-controller req id
 */
int
puffs_vntouser_req(struct puffs_mount *pmp, int optype,
	void *kbuf, size_t buflen, void *cookie, unsigned int reqid,
	struct vnode *vp1, struct vnode *vp2)
{
	struct puffs_req preq;
	struct puffs_park park;

	memset(&preq, 0, sizeof(struct puffs_req));

	preq.preq_opclass = PUFFSOP_VN; 
	preq.preq_optype = optype;
	preq.preq_cookie = cookie;

	park.park_kernbuf = kbuf;
	park.park_buflen = buflen;
	park.park_copylen = buflen;
	park.park_flags = 0;
	park.park_preq = &preq;

	return touser(pmp, &park, reqid, vp1, vp2);
}

/*
 * vnode level request, copy routines can adjust "kernbuf"
 */
int
puffs_vntouser_adjbuf(struct puffs_mount *pmp, int optype,
	void **kbuf, size_t *buflen, size_t copylen, void *cookie,
	struct vnode *vp1, struct vnode *vp2)
{
	struct puffs_req preq;
	struct puffs_park park;
	int error;

	memset(&preq, 0, sizeof(struct puffs_req));

	preq.preq_opclass = PUFFSOP_VN; 
	preq.preq_optype = optype;
	preq.preq_cookie = cookie;

	park.park_kernbuf = *kbuf;
	park.park_buflen = *buflen;
	park.park_copylen = copylen;
	park.park_flags = PUFFS_REQFLAG_ADJBUF;
	park.park_preq = &preq;

	error = touser(pmp, &park, puffs_getreqid(pmp), vp1, vp2);
	*kbuf = park.park_kernbuf;
	*buflen = park.park_buflen;

	return error;
}

/*
 * Wait for the userspace ping-pong game in calling process context.
 *
 * This unlocks vnodes if they are supplied.  vp1 is the vnode
 * before in the locking order, i.e. the one which must be locked
 * before accessing vp2.  This is done here so that operations are
 * already ordered in the queue when vnodes are unlocked (I'm not
 * sure if that's really necessary, but it can't hurt).  Okok, maybe
 * there's a slight ugly-factor also, but let's not worry about that.
 */
static int
touser(struct puffs_mount *pmp, struct puffs_park *park, unsigned int reqid,
	struct vnode *vp1, struct vnode *vp2)
{

	simple_lock(&pmp->pmp_lock);
	if (pmp->pmp_status != PUFFSTAT_RUNNING
	    && pmp->pmp_status != PUFFSTAT_MOUNTING) {
		simple_unlock(&pmp->pmp_lock);
		return ENXIO;
	}

	park->park_preq->preq_id = reqid;

	TAILQ_INSERT_TAIL(&pmp->pmp_req_touser, park, park_entries);
	pmp->pmp_req_touser_waiters++;

	/*
	 * Don't do unlock-relock dance yet.  There are a couple of
	 * unsolved issues with it.  If we don't unlock, we can have
	 * processes wanting vn_lock in case userspace hangs.  But
	 * that can be "solved" by killing the userspace process.  It
	 * would of course be nicer to have antilocking in the userspace
	 * interface protocol itself.. your patience will be rewarded.
	 */
#if 0
	/* unlock */
	if (vp2)
		VOP_UNLOCK(vp2, 0);
	if (vp1)
		VOP_UNLOCK(vp1, 0);
#endif

	/*
	 * XXX: does releasing the lock here cause trouble?  Can't hold
	 * it, because otherwise the below would cause locking against
	 * oneself-problems in the kqueue stuff
	 */
	simple_unlock(&pmp->pmp_lock);

	wakeup(&pmp->pmp_req_touser);
	selnotify(pmp->pmp_sel, 0);

	ltsleep(park, PUSER, "puffs1", 0, NULL);

#if 0
	/* relock */
	if (vp1)
		KASSERT(vn_lock(vp1, LK_EXCLUSIVE | LK_RETRY) == 0);
	if (vp2)
		KASSERT(vn_lock(vp2, LK_EXCLUSIVE | LK_RETRY) == 0);
#endif

	return park->park_preq->preq_rv;
}

/*
 * We're dead, kaput, RIP, slightly more than merely pining for the
 * fjords, belly-up, fallen, lifeless, finished, expired, gone to meet
 * our maker, ceased to be, etcetc.  YASD.  It's a dead FS!
 */
void
puffs_userdead(struct puffs_mount *pmp)
{
	struct puffs_park *park;

	simple_lock(&pmp->pmp_lock);

	/*
	 * Mark filesystem status as dying so that operations don't
	 * attempt to march to userspace any longer.
	 */
	pmp->pmp_status = PUFFSTAT_DYING;

	/* and wakeup processes waiting for a reply from userspace */
	TAILQ_FOREACH(park, &pmp->pmp_req_replywait, park_entries) {
		park->park_preq->preq_rv = ENXIO;
		TAILQ_REMOVE(&pmp->pmp_req_replywait, park, park_entries);
		wakeup(park);
	}

	/* wakeup waiters for completion of vfs/vnode requests */
	TAILQ_FOREACH(park, &pmp->pmp_req_touser, park_entries) {
		park->park_preq->preq_rv = ENXIO;
		TAILQ_REMOVE(&pmp->pmp_req_touser, park, park_entries);
		wakeup(park);
	}

	simple_unlock(&pmp->pmp_lock);
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

static int puffs_fop_read(struct file *, off_t *, struct uio *,
			  kauth_cred_t, int);
static int puffs_fop_write(struct file *, off_t *, struct uio *,
			   kauth_cred_t, int);
static int puffs_fop_ioctl(struct file*, u_long, void *, struct lwp *);
static int puffs_fop_poll(struct file *, int, struct lwp *);
static int puffs_fop_close(struct file *, struct lwp *);
static int puffs_fop_kqfilter(struct file *, struct knote *);


/* fd routines, for cloner */
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

	panic("puffscdclose\n");

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


static int
puffs_fop_read(struct file *fp, off_t *off, struct uio *uio,
	kauth_cred_t cred, int flags)
{

	printf("READ\n");
	return ENODEV;
}

static int
puffs_fop_write(struct file *fp, off_t *off, struct uio *uio,
	kauth_cred_t cred, int flags)
{

	printf("WRITE\n");
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
 *
 * XXX: if userspace is terminated by a signal, this will be
 * called only after the signal is delivered (i.e. after someone tries
 * to access the file system).  Also, the first one for a delivery
 * will get a free bounce-bounce ride before it can be notified
 * that the fs is dead.  I'm not terribly concerned about optimizing
 * this for speed ...
 */
static int
puffs_fop_close(struct file *fp, struct lwp *l)
{
	struct puffs_instance *pi;
	struct puffs_mount *pmp;
	struct mount *mp;

	DPRINTF(("puffs_fop_close: device closed, force filesystem unmount\n"));

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
		pi = FPTOPI(fp);
		FREE(pi, M_PUFFS);
		return 0;
	}

	/*
	 * So we have a reference.  Proceed to unwrap the file system.
	 */
	mp = PMPTOMP(pmp);
	simple_unlock(&pi_lock);

	/*
	 * Detach from VFS.  First do necessary XXX-dance (from
	 * sys_unmount() & other callers of dounmount()
	 *
	 * XXX Freeze syncer.  Must do this before locking the
	 * mount point.  See dounmount() for details.
	 */
	lockmgr(&syncer_lock, LK_EXCLUSIVE, NULL);

	/*
	 * The only way vfs_busy() will fail for us is if the filesystem
	 * is already a goner.
	 * XXX: skating on the thin ice of modern calling conventions ...
	 */
	if (vfs_busy(mp, 0, 0)) {
		lockmgr(&syncer_lock, LK_RELEASE, NULL);
		return 0;
	}

	/* Once we have the mount point, unmount() can't interfere */
	puffs_userdead(pmp);
	dounmount(mp, MNT_FORCE, l);

	return 0;
}

static int puffsgetop(struct puffs_mount *, struct puffs_req *, int);
static int puffsputop(struct puffs_mount *, struct puffs_req *);
static int puffssizeop(struct puffs_mount *, struct puffs_sizeop *);

static int
puffs_fop_ioctl(struct file *fp, u_long cmd, void *data, struct lwp *l)
{
	struct puffs_mount *pmp = FPTOPMP(fp);

	if (pmp == PMP_EMBRYO || pmp == PMP_DEAD) {
		printf("puffs_fop_ioctl: puffs %p, not mounted\n", pmp);
		return ENOENT;
	}

	switch (cmd) {
	case PUFFSGETOP:
		return puffsgetop(pmp, data, fp->f_flag & FNONBLOCK);
		break;

	case PUFFSPUTOP:
		return puffsputop(pmp, data);
		break;

	case PUFFSSIZEOP:
		return puffssizeop(pmp, data);
		break;

	case PUFFSMOUNTOP:
		return puffs_start2(pmp, data);

	/* already done in sys_ioctl() */
	case FIONBIO:
		return 0;

	default:
		return EINVAL;

	}
}

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
 * ioctl handlers
 */

static int
puffsgetop(struct puffs_mount *pmp, struct puffs_req *preq, int nonblock)
{
	struct puffs_park *park;
	int error;

	simple_lock(&pmp->pmp_lock);
 again:
	if (pmp->pmp_status != PUFFSTAT_RUNNING) {
		simple_unlock(&pmp->pmp_lock);
		return ENXIO;
	}
	if (TAILQ_EMPTY(&pmp->pmp_req_touser)) {
		if (nonblock) {
			simple_unlock(&pmp->pmp_lock);
			return EWOULDBLOCK;
		}
		ltsleep(&pmp->pmp_req_touser, PUSER, "puffs2", 0,
		    &pmp->pmp_lock);
		goto again;
	}

	park = TAILQ_FIRST(&pmp->pmp_req_touser);
	if (preq->preq_auxlen < park->park_copylen) {
		simple_unlock(&pmp->pmp_lock);
		return E2BIG;
	}
	TAILQ_REMOVE(&pmp->pmp_req_touser, park, park_entries);
	pmp->pmp_req_touser_waiters--;
	simple_unlock(&pmp->pmp_lock);

	preq->preq_id = park->park_preq->preq_id;
	preq->preq_opclass = park->park_preq->preq_opclass;
	preq->preq_optype = park->park_preq->preq_optype;
	preq->preq_cookie = park->park_preq->preq_cookie;
	preq->preq_auxlen = park->park_copylen;

	if ((error = copyout(park->park_kernbuf, preq->preq_aux,
	    park->park_copylen)) != 0) {
		/*
		 * ok, user server is probably trying to cheat.
		 * stuff op back & return error to user
		 */
		 simple_lock(&pmp->pmp_lock);
		 TAILQ_INSERT_HEAD(&pmp->pmp_req_touser, park, park_entries);
		 simple_unlock(&pmp->pmp_lock);
		 return error;
	}
	simple_lock(&pmp->pmp_lock);
	TAILQ_INSERT_TAIL(&pmp->pmp_req_replywait, park, park_entries);
	simple_unlock(&pmp->pmp_lock);

	return 0;
}

static int
puffsputop(struct puffs_mount *pmp, struct puffs_req *preq)
{
	struct puffs_park *park;
	size_t copylen;
	int error;

	simple_lock(&pmp->pmp_lock);
	TAILQ_FOREACH(park, &pmp->pmp_req_replywait, park_entries) {
		if (park->park_preq->preq_id == preq->preq_id) {
			TAILQ_REMOVE(&pmp->pmp_req_replywait, park,
			    park_entries);
			break;
		}
	}
	simple_unlock(&pmp->pmp_lock);

	if (park == NULL)
		return EINVAL;

	/*
	 * check size of incoming transmission.  allow to allocate a
	 * larger kernel buffer only if it was specified by the caller
	 * by setting preq->preq_auxadj.  Else, just copy whatever the
	 * kernel buffer size is unless.
	 *
	 * However, don't allow ludicrously large buffers
	 */
	copylen = preq->preq_auxlen;
	if (copylen > pmp->pmp_req_maxsize) {
#ifdef DIAGNOSTIC
		printf("puffsputop: outrageous user buf size: %zu\n", copylen);
#endif
		error = EFAULT;
		goto out;
	}

	if (park->park_buflen < copylen &&
	    park->park_flags & PUFFS_REQFLAG_ADJBUF) {
		free(park->park_kernbuf, M_PUFFS);
		park->park_kernbuf = malloc(copylen, M_PUFFS, M_WAITOK);
		park->park_buflen = copylen;
	}

	error = copyin(preq->preq_aux, park->park_kernbuf, copylen);

	/*
	 * if copyin botched, inform both userspace and the vnodeop
	 * desperately waiting for information
	 */
 out:
	if (error)
		park->park_preq->preq_rv = error;
	else
		park->park_preq->preq_rv = preq->preq_rv;
	wakeup(park);

	return error;
}

/* this is probably going to die away at some point? */
static int
puffssizeop(struct puffs_mount *pmp, struct puffs_sizeop *psop_user)
{
	struct puffs_sizepark *pspark;
	void *kernbuf;
	size_t copylen;
	int error;

	/* locate correct op */
	simple_lock(&pmp->pmp_lock);
	TAILQ_FOREACH(pspark, &pmp->pmp_req_sizepark, pkso_entries) {
		if (pspark->pkso_reqid == psop_user->pso_reqid) {
			TAILQ_REMOVE(&pmp->pmp_req_sizepark, pspark,
			    pkso_entries);
			break;
		}
	}
	simple_unlock(&pmp->pmp_lock);

	if (pspark == NULL)
		return EINVAL;

	error = 0;
	copylen = MIN(pspark->pkso_bufsize, psop_user->pso_bufsize);

	/*
	 * XXX: uvm stuff to avoid bouncy-bouncy copying?
	 */
	if (PUFFS_SIZEOP_UIO(pspark->pkso_reqtype)) {
		kernbuf = malloc(copylen, M_PUFFS, M_WAITOK | M_ZERO);
		if (pspark->pkso_reqtype == PUFFS_SIZEOPREQ_UIO_IN) {
			error = copyin(psop_user->pso_userbuf,
			    kernbuf, copylen);
			if (error) {
				printf("psop ERROR1 %d\n", error);
				goto escape;
			}
		}
		error = uiomove(kernbuf, copylen, pspark->pkso_uio);
		if (error) {
			printf("uiomove from kernel %p, len %d failed: %d\n",
			    kernbuf, (int)copylen, error);
			goto escape;
		}
			
		if (pspark->pkso_reqtype == PUFFS_SIZEOPREQ_UIO_OUT) {
			error = copyout(kernbuf,
			    psop_user->pso_userbuf, copylen);
			if (error) {
				printf("psop ERROR2 %d\n", error);
				goto escape;
			}
		}
 escape:
		free(kernbuf, M_PUFFS);
	} else if (PUFFS_SIZEOP_BUF(pspark->pkso_reqtype)) {
		copylen = MAX(pspark->pkso_bufsize, psop_user->pso_bufsize);
		if (pspark->pkso_reqtype == PUFFS_SIZEOPREQ_BUF_IN) {
			error = copyin(psop_user->pso_userbuf,
			pspark->pkso_copybuf, copylen);
		} else {
			error = copyout(pspark->pkso_copybuf,
			    psop_user->pso_userbuf, copylen);
		}
	}
#ifdef DIAGNOSTIC
	else
		panic("puffssizeop: invalid reqtype %d\n",
		    pspark->pkso_reqtype);
#endif /* DIAGNOSTIC */

	return error;
}
