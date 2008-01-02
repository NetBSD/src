/*	$NetBSD: putter.c,v 1.6.6.1 2008/01/02 21:55:08 bouyer Exp $	*/

/*
 * Copyright (c) 2006, 2007  Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Ulla Tuominen Foundation and the Finnish Cultural Foundation and the
 * Research Foundation of Helsinki University of Technology
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

/*
 * Pass-to-Userspace TransporTER: generic kernel-user request-response
 * transport interface.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: putter.c,v 1.6.6.1 2008/01/02 21:55:08 bouyer Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/kmem.h>
#include <sys/poll.h>
#include <sys/socketvar.h>

#include <dev/putter/putter_sys.h>

/*
 * Configuration data.
 *
 * This is static-size for now.  Will be redone for devfs.
 */

#define PUTTER_CONFSIZE 16

static struct putter_config {
	int	pc_minor;
	int	(*pc_config)(int, int, int);
} putterconf[PUTTER_CONFSIZE];

static int
putter_configure(dev_t dev, int flags, int fmt, int fd)
{
	struct putter_config *pc;

	/* are we the catch-all node? */
	if (minor(dev) == PUTTER_MINOR_WILDCARD
	    || minor(dev) == PUTTER_MINOR_COMPAT)
		return 0;

	/* nopes?  try to configure us */
	for (pc = putterconf; pc->pc_config; pc++)
		if (minor(dev) == pc->pc_minor)
			return pc->pc_config(fd, flags, fmt);
	return ENXIO;
}

int
putter_register(putter_config_fn pcfn, int minor)
{
	int i;

	for (i = 0; i < PUTTER_CONFSIZE; i++)
		if (putterconf[i].pc_config == NULL)
			break;
	if (i == PUTTER_CONFSIZE)
		return EBUSY;

	putterconf[i].pc_minor = minor;
	putterconf[i].pc_config = pcfn;
	return 0;
}

/*
 * putter instance structures.  these are always allocated and freed
 * from the context of the transport user.
 */
struct putter_instance {
	pid_t			pi_pid;
	int			pi_idx;
	int			pi_fd;
	struct selinfo		pi_sel;

	void			*pi_private;
	struct putter_ops	*pi_pop;

	uint8_t			*pi_curput;
	size_t			pi_curres;
	void			*pi_curopaq;

	TAILQ_ENTRY(putter_instance) pi_entries;
};
#define PUTTER_EMBRYO ((void *)-1)	/* before attach	*/
#define PUTTER_DEAD ((void *)-2)	/* after detach		*/

static TAILQ_HEAD(, putter_instance) putter_ilist
    = TAILQ_HEAD_INITIALIZER(putter_ilist);

static int get_pi_idx(struct putter_instance *);

#ifdef DEBUG
#ifndef PUTTERDEBUG
#define PUTTERDEBUG
#endif
#endif

#ifdef PUTTERDEBUG
int putterdebug = 0;
#define DPRINTF(x) if (putterdebug > 0) printf x
#define DPRINTF_VERBOSE(x) if (putterdebug > 1) printf x
#else
#define DPRINTF(x)
#define DPRINTF_VERBOSE(x)
#endif

/*
 * public init / deinit
 */

/* protects both the list and the contents of the list elements */
static kmutex_t pi_mtx;

void putterattach(void);

void
putterattach()
{

	mutex_init(&pi_mtx, MUTEX_DEFAULT, IPL_NONE);
}

#if 0
void
putter_destroy()
{

	mutex_destroy(&pi_mtx);
}
#endif

/*
 * fd routines, for cloner
 */
static int putter_fop_read(struct file *, off_t *, struct uio *,
			   kauth_cred_t, int);
static int putter_fop_write(struct file *, off_t *, struct uio *,
			    kauth_cred_t, int);
static int putter_fop_ioctl(struct file*, u_long, void *, struct lwp *);
static int putter_fop_poll(struct file *, int, struct lwp *);
static int putter_fop_close(struct file *, struct lwp *);
static int putter_fop_kqfilter(struct file *, struct knote *);


static const struct fileops putter_fileops = {
	putter_fop_read,
	putter_fop_write,
	putter_fop_ioctl,
	fnullop_fcntl,
	putter_fop_poll,
	fbadop_stat,
	putter_fop_close,
	putter_fop_kqfilter
};

static int
putter_fop_read(struct file *fp, off_t *off, struct uio *uio,
	kauth_cred_t cred, int flags)
{
	struct putter_instance *pi = fp->f_data;
	size_t origres, moved;
	int error;

	if (pi->pi_private == PUTTER_EMBRYO || pi->pi_private == PUTTER_DEAD) {
		printf("putter_fop_read: private %d not inited\n", pi->pi_idx);
		return ENOENT;
	}

	if (pi->pi_curput == NULL) {
		error = pi->pi_pop->pop_getout(pi->pi_private, uio->uio_resid,
		    fp->f_flag & O_NONBLOCK, &pi->pi_curput,
		    &pi->pi_curres, &pi->pi_curopaq);
		if (error)
			return error;
	}

	origres = uio->uio_resid;
	error = uiomove(pi->pi_curput, pi->pi_curres, uio);
	moved = origres - uio->uio_resid;
	DPRINTF(("putter_fop_read (%p): moved %zu bytes from %p, error %d\n",
	    pi, moved, pi->pi_curput, error));

	KASSERT(pi->pi_curres >= moved);
	pi->pi_curres -= moved;
	pi->pi_curput += moved;

	if (pi->pi_curres == 0) {
		pi->pi_pop->pop_releaseout(pi->pi_private,
		    pi->pi_curopaq, error);
		pi->pi_curput = NULL;
	}

	return error;
}

static int
putter_fop_write(struct file *fp, off_t *off, struct uio *uio,
	kauth_cred_t cred, int flags)
{
	struct putter_instance *pi = fp->f_data;
	struct putter_hdr pth;
	uint8_t *buf;
	size_t frsize;
	int error;

	DPRINTF(("putter_fop_write (%p): writing response, resid %zu\n",
	    pi->pi_private, uio->uio_resid));

	if (pi->pi_private == PUTTER_EMBRYO || pi->pi_private == PUTTER_DEAD) {
		printf("putter_fop_write: putter %d not inited\n", pi->pi_idx);
		return ENOENT;
	}

	error = uiomove(&pth, sizeof(struct putter_hdr), uio);
	if (error)
		return error;

	/* Sorry mate, the kernel doesn't buffer. */
	frsize = pth.pth_framelen - sizeof(struct putter_hdr);
	if (uio->uio_resid < frsize)
		return EINVAL;

	buf = kmem_alloc(frsize + sizeof(struct putter_hdr), KM_SLEEP);
	memcpy(buf, &pth, sizeof(pth));
	error = uiomove(buf+sizeof(struct putter_hdr), frsize, uio);
	if (error == 0) {
		pi->pi_pop->pop_dispatch(pi->pi_private,
		    (struct putter_hdr *)buf);
	}
	kmem_free(buf, frsize + sizeof(struct putter_hdr));

	return error;
}

/*
 * Poll query interface.  The question is only if an event
 * can be read from us.
 */
#define PUTTERPOLL_EVSET (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI)
static int
putter_fop_poll(struct file *fp, int events, struct lwp *l)
{
	struct putter_instance *pi = fp->f_data;
	int revents;

	if (pi->pi_private == PUTTER_EMBRYO || pi->pi_private == PUTTER_DEAD) {
		printf("putter_fop_ioctl: putter %d not inited\n", pi->pi_idx);
		return ENOENT;
	}

	revents = events & (POLLOUT | POLLWRNORM | POLLWRBAND);
	if ((events & PUTTERPOLL_EVSET) == 0)
		return revents;

	/* check queue */
	if (pi->pi_pop->pop_waitcount(pi->pi_private))
		revents |= PUTTERPOLL_EVSET;
	else
		selrecord(l, &pi->pi_sel);

	return revents;
}

/*
 * device close = forced unmount.
 *
 * unmounting is a frightfully complex operation to avoid races
 */
static int
putter_fop_close(struct file *fp, struct lwp *l)
{
	struct putter_instance *pi = fp->f_data;
	int rv;

	DPRINTF(("putter_fop_close: device closed\n"));

 restart:
	mutex_enter(&pi_mtx);
	/*
	 * First check if the fs was never mounted.  In that case
	 * remove the instance from the list.  If mount is attempted later,
	 * it will simply fail.
	 */
	if (pi->pi_private == PUTTER_EMBRYO) {
		TAILQ_REMOVE(&putter_ilist, pi, pi_entries);
		mutex_exit(&pi_mtx);

		DPRINTF(("putter_fop_close: data associated with fp %p was "
		    "embryonic\n", fp));

		goto out;
	}

	/*
	 * Next, analyze if unmount was called and the instance is dead.
	 * In this case we can just free the structure and go home, it
	 * was removed from the list by putter_rmprivate().
	 */
	if (pi->pi_private == PUTTER_DEAD) {
		mutex_exit(&pi_mtx);

		DPRINTF(("putter_fop_close: putter associated with fp %p (%d) "
		    "dead, freeing\n", fp, pi->pi_idx));

		goto out;
	}

	/*
	 * So we have a reference.  Proceed to unwrap the file system.
	 */
	mutex_exit(&pi_mtx);

	/* hmm?  suspicious locking? */
	while ((rv = pi->pi_pop->pop_close(pi->pi_private)) == ERESTART)
		goto restart;

 out:
	/*
	 * Finally, release the instance information.  It was already
	 * removed from the list by putter_rmprivate() and we know it's
	 * dead, so no need to lock.
	 */
	kmem_free(pi, sizeof(struct putter_instance));

	return 0;
}

static int
putter_fop_ioctl(struct file *fp, u_long cmd, void *data, struct lwp *l)
{

	/*
	 * work already done in sys_ioctl().  skip sanity checks to enable
	 * setting non-blocking fd without yet having mounted the fs
	 */
	if (cmd == FIONBIO)
		return 0;

	return EINVAL;
}

/* kqueue stuff */

static void
filt_putterdetach(struct knote *kn)
{
	struct putter_instance *pi = kn->kn_hook;

	mutex_enter(&pi_mtx);
	SLIST_REMOVE(&pi->pi_sel.sel_klist, kn, knote, kn_selnext);
	mutex_exit(&pi_mtx);
}

static int
filt_putter(struct knote *kn, long hint)
{
	struct putter_instance *pi = kn->kn_hook;
	int error;

	error = 0;
	mutex_enter(&pi_mtx);
	if (pi->pi_private == PUTTER_EMBRYO || pi->pi_private == PUTTER_DEAD)
		error = 1;
	mutex_exit(&pi_mtx);
	if (error)
		return 0;

	kn->kn_data = pi->pi_pop->pop_waitcount(pi->pi_private);

	return kn->kn_data != 0;
}

static const struct filterops putter_filtops =
	{ 1, NULL, filt_putterdetach, filt_putter };

static int
putter_fop_kqfilter(struct file *fp, struct knote *kn)
{
	struct putter_instance *pi = fp->f_data;
	struct klist *klist;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &pi->pi_sel.sel_klist;
		kn->kn_fop = &putter_filtops;
		kn->kn_hook = pi;

		mutex_enter(&pi_mtx);
		SLIST_INSERT_HEAD(klist, kn, kn_selnext);
		mutex_exit(&pi_mtx);

		break;
	case EVFILT_WRITE:
		kn->kn_fop = &seltrue_filtops;
		break;
	default:
		return EINVAL;
	}

	return 0;
}

/*
 * Device routines.  These are for when /dev/puffs is initially
 * opened before it has been cloned.
 */

dev_type_open(puttercdopen);
dev_type_close(puttercdclose);
dev_type_ioctl(puttercdioctl);

/* dev */
const struct cdevsw putter_cdevsw = {
	puttercdopen,	puttercdclose,	noread,		nowrite,
	noioctl,	nostop,		notty,		nopoll,
	nommap,		nokqfilter,	D_OTHER
};
int
puttercdopen(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct putter_instance *pi;
	struct file *fp;
	int error, fd, idx;

	pi = kmem_alloc(sizeof(struct putter_instance), KM_SLEEP);
	mutex_enter(&pi_mtx);
	idx = get_pi_idx(pi);

	pi->pi_pid = l->l_proc->p_pid;
	pi->pi_idx = idx;
	pi->pi_curput = NULL;
	pi->pi_curres = 0;
	pi->pi_curopaq = NULL;
	selinit(&pi->pi_sel);
	mutex_exit(&pi_mtx);

	if ((error = falloc(l, &fp, &fd)) != 0)
		goto bad1;

	if ((error = putter_configure(dev, flags, fmt, fd)) != 0)
		goto bad2;

	DPRINTF(("puttercdopen: registered embryonic pmp for pid: %d\n",
	    pi->pi_pid));

	error = fdclone(l, fp, fd, FREAD|FWRITE, &putter_fileops, pi);
	KASSERT(error = EMOVEFD);
	return error;

 bad2:
	FILE_UNUSE(fp, l);
	fdremove(l->l_proc->p_fd, fd);
	ffree(fp);
 bad1:
	putter_detach(pi);
	kmem_free(pi, sizeof(struct putter_instance));
	return error;
}

int
puttercdclose(dev_t dev, int flags, int fmt, struct lwp *l)
{

	panic("puttercdclose impossible\n");

	return 0;
}


/*
 * Set the private structure for the file descriptor.  This is
 * typically done immediately when the counterpart has knowledge
 * about the private structure's address and the file descriptor
 * (e.g. vfs mount routine).
 *
 * We only want to make sure that the caller had the right to open the
 * device, we don't so much care about which context it gets in case
 * the same process opened multiple (since they are equal at this point).
 */
struct putter_instance *
putter_attach(pid_t pid, int fd, void *ppriv, struct putter_ops *pop)
{
	struct putter_instance *pi = NULL;

	mutex_enter(&pi_mtx);
	TAILQ_FOREACH(pi, &putter_ilist, pi_entries) {
		if (pi->pi_pid == pid && pi->pi_private == PUTTER_EMBRYO) {
			pi->pi_private = ppriv;
			pi->pi_fd = fd;
			pi->pi_pop = pop;
			break;
		    }
	}
	mutex_exit(&pi_mtx);

	DPRINTF(("putter_setprivate: pi at %p (%d/%d)\n", pi,
	    pi ? pi->pi_pid : 0, pi ? pi->pi_fd : 0));

	return pi;
}

/*
 * Remove fp <-> private mapping.
 */
void
putter_detach(struct putter_instance *pi)
{

	mutex_enter(&pi_mtx);
	TAILQ_REMOVE(&putter_ilist, pi, pi_entries);
	pi->pi_private = PUTTER_DEAD;
	mutex_exit(&pi_mtx);

	DPRINTF(("putter_nukebypmp: nuked %p\n", pi));
}

void
putter_notify(struct putter_instance *pi)
{

	selnotify(&pi->pi_sel, 0);
}

/* search sorted list of instances for free minor, sorted insert arg */
static int
get_pi_idx(struct putter_instance *pi_i)
{
	struct putter_instance *pi;
	int i;

	KASSERT(mutex_owned(&pi_mtx));

	i = 0;
	TAILQ_FOREACH(pi, &putter_ilist, pi_entries) {
		if (i != pi->pi_idx)
			break;
		i++;
	}

	pi_i->pi_private = PUTTER_EMBRYO;

	if (pi == NULL)
		TAILQ_INSERT_TAIL(&putter_ilist, pi_i, pi_entries);
	else
		TAILQ_INSERT_BEFORE(pi, pi_i, pi_entries);

	return i;
}
