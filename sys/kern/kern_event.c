/*	$NetBSD: kern_event.c,v 1.1.1.1.2.2 2001/09/07 15:54:17 thorpej Exp $	*/
/*-
 * Copyright (c) 1999,2000,2001 Jonathan Lemon <jlemon@FreeBSD.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/kern/kern_event.c,v 1.27 2001/07/05 17:10:44 rwatson Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h> 
#include <sys/unistd.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/queue.h>
#include <sys/event.h>
#include <sys/eventvar.h>
#include <sys/poll.h>
#include <sys/pool.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/mount.h>
#include <sys/filedesc.h>
#include <sys/syscallargs.h>

static int	kqueue_scan(struct file *fp, int maxevents,
		    struct kevent *ulistp, const struct timespec *timeout,
		    struct proc *p, register_t *retval);
static void	kqueue_wakeup(struct kqueue *kq);

static int	kqueue_read(struct file *fp, off_t *offset, struct uio *uio,
		    struct ucred *cred, int flags);
static int	kqueue_write(struct file *fp, off_t *offset, struct uio *uio,
		    struct ucred *cred, int flags);
static int	kqueue_ioctl(struct file *fp, u_long com, caddr_t data,
		    struct proc *p);
static int	kqueue_fcntl(struct file *fp, u_int com, caddr_t data,
		    struct proc *p);
static int	kqueue_poll(struct file *fp, int events, struct proc *p);
static int	kqueue_kqfilter(struct file *fp, struct knote *kn);
static int	kqueue_stat(struct file *fp, struct stat *sp, struct proc *p);
static int	kqueue_close(struct file *fp, struct proc *p);

static struct fileops kqueueops = {
	kqueue_read, kqueue_write, kqueue_ioctl, kqueue_fcntl, kqueue_poll,
	kqueue_stat, kqueue_close, kqueue_kqfilter
};

static void	knote_attach(struct knote *kn, struct filedesc *fdp);
static void	knote_drop(struct knote *kn, struct proc *p);
static void	knote_enqueue(struct knote *kn);
static void	knote_dequeue(struct knote *kn);
static struct knote *knote_alloc(void);
static void	knote_free(struct knote *kn);

static void	filt_kqdetach(struct knote *kn);
static int	filt_kqueue(struct knote *kn, long hint);
static int	filt_procattach(struct knote *kn);
static void	filt_procdetach(struct knote *kn);
static int	filt_proc(struct knote *kn, long hint);
static int	filt_fileattach(struct knote *kn);

static struct filterops kqread_filtops =
	{ 1, NULL, filt_kqdetach, filt_kqueue };
static struct filterops proc_filtops =
	{ 0, filt_procattach, filt_procdetach, filt_proc };
static struct filterops file_filtops =
	{ 1, filt_fileattach, NULL, NULL };

struct pool	kqueue_pool;
struct pool	knote_pool;

#define	KNOTE_ACTIVATE(kn)						\
do {									\
	kn->kn_status |= KN_ACTIVE;					\
	if ((kn->kn_status & (KN_QUEUED | KN_DISABLED)) == 0)		\
		knote_enqueue(kn);					\
} while(0)

#define	KN_HASHSIZE		64		/* XXX should be tunable */
#define	KN_HASH(val, mask)	(((val) ^ (val >> 8)) & (mask))

extern struct filterops sig_filtops;

/*
 * Table for for all system-defined filters.
 * These should be listed in the numeric order of the EVFILT_* defines.
 * If filtops is NULL, the filter isn't implemented in NetBSD.
 * End of list is when name is NULL.
 */
struct kfilter {
	char		 *name;		/* name of filter */
	uint32_t	  filter;	/* id of filter */
	struct filterops *filtops;	/* operations for filter */
};

		/* System defined filters */
static struct kfilter sys_kfilters[] = {
	{ "EVFILT_READ",	EVFILT_READ,	&file_filtops },
	{ "EVFILT_WRITE",	EVFILT_WRITE,	&file_filtops },
	{ "EVFILT_AIO",		EVFILT_AIO,	NULL },
	{ "EVFILT_VNODE",	EVFILT_VNODE,	&file_filtops },
	{ "EVFILT_PROC",	EVFILT_PROC,	&proc_filtops },
	{ "EVFILT_SIGNAL",	EVFILT_SIGNAL,	&sig_filtops },
	{ NULL,			0,		NULL },	/* end of list */ 
};

		/* User defined kfilters */
static struct kfilter	*user_kfilters;		/* array */
static int		user_kfilterc;		/* current offset */
static int		user_kfiltermaxc;	/* max size so far */

static struct kfilter *kfilter_byname(const char *);
static struct kfilter *kfilter_byfilter(uint32_t);

/*
 * kqueue_init:
 *
 *	Initialize the kqueue/knote facility.
 */
void
kqueue_init(void)
{

	pool_init(&kqueue_pool, sizeof(struct kqueue), 0, 0, 0, "kqueuepl",
	    0, pool_page_alloc_nointr, pool_page_free_nointr, M_KEVENT);
	pool_init(&knote_pool, sizeof(struct knote), 0, 0, 0, "knotepl",
	    0, pool_page_alloc_nointr, pool_page_free_nointr, M_KEVENT);
}

/*
 * Find kfilter entry by name, or NULL if not found.
 */
static struct kfilter *
kfilter_byname(const char *name)
{
	struct kfilter *kfilter;
	int i;

	kfilter = sys_kfilters;		/* first look in system kfilters */
	while (kfilter != NULL) {
		for (i = 0; kfilter[i].name != NULL; i++) {
					/* search for matching name */
			if (kfilter[i].name[0] != '\0' &&
			    (strcmp(name, kfilter[i].name) == 0))
				return (&kfilter[i]);
		}
					/* swap to user kfilters */
		if (kfilter == sys_kfilters)
			kfilter = user_kfilters;
		else
			kfilter = NULL;
	}
	return (NULL);
}

/*
 * Find kfilter entry by filter id, or NULL if not found.
 * Assumes entries are indexed in filter id order, for speed.
 */
static struct kfilter *
kfilter_byfilter(uint32_t filter)
{
	struct kfilter *kfilter;

	if (filter < EVFILT_SYSCOUNT)	/* it's a system filter */
		kfilter = &sys_kfilters[filter];
	else if (user_kfilters != NULL &&
	    filter < EVFILT_SYSCOUNT + user_kfilterc)
					/* it's a user filter */
		kfilter = &user_kfilters[filter - EVFILT_SYSCOUNT];
	else
		return (NULL);		/* out of range */
	KASSERT(kfilter->filter == filter);	/* sanity check! */
	return (kfilter);
}

/*
 * Register a new kfilter. Stores the entry in user_kfilters.
 * Returns 0 if operation succeeded, or an appropriate errno(2) otherwise.
 * If retfilter != NULL, the new filterid is returned in it.
 */
int
kfilter_register(const char *name, struct filterops *filtops, int *retfilter)
{
	struct kfilter *kfilter;
	int len;

	if (name == NULL || name[0] == '\0' || filtops == NULL)
		return (EINVAL);	/* invalid args */
	kfilter = kfilter_byname(name);
	if (kfilter != NULL)		/* already exists */
		return (EEXIST);
	if (user_kfilterc > 0xffffffff - EVFILT_SYSCOUNT)
		return (EINVAL);	/* too many */

					/* need to grow user_kfilters */
	if (user_kfilterc + 1 > user_kfiltermaxc) {
					/*
					 * grow in KFILTER_EXTENT chunks. use
					 * malloc(9), because we want to
					 * traverse user_kfilters as an array.
					 */
		user_kfiltermaxc += KFILTER_EXTENT;
		kfilter = malloc(user_kfiltermaxc * sizeof(struct filter *),
		    M_KEVENT, M_WAITOK);
					/* copy existing user_kfilters */
		if (user_kfilters != NULL)
			memcpy((caddr_t)kfilter, (caddr_t)user_kfilters,
			    user_kfilterc * sizeof(struct kfilter *));
					/* zero new sections */
		memset((caddr_t)kfilter +
		    user_kfilterc * sizeof(struct kfilter *), 0,
		    (user_kfiltermaxc - user_kfilterc) *
		    sizeof(struct kfilter *));
					/* switch to new kfilter */
		if (user_kfilters != NULL)
			FREE(user_kfilters, M_KEVENT);
		user_kfilters = kfilter;
	}
	len = strlen(name) + 1;		/* copy name */
	user_kfilters[user_kfilterc].name = (char *)
	    malloc(len, M_KEVENT, M_WAITOK);
	memcpy(user_kfilters[user_kfilterc].name, name, len);
	user_kfilters[user_kfilterc].filter = user_kfilterc + EVFILT_SYSCOUNT;
	len = sizeof(struct filterops);	/* copy filtops */
	user_kfilters[user_kfilterc].filtops = (struct filterops *)
	    malloc(len, M_KEVENT, M_WAITOK);
	memcpy(user_kfilters[user_kfilterc].filtops, filtops, len);
	if (retfilter != NULL)
		*retfilter = user_kfilters[user_kfilterc].filter;
	user_kfilterc++;		/* finally, increment count */
	return (0);
}

/*
 * Unregister a kfilter previously registered with kfilter_register.
 * This retains the filter id, but clears the name and frees filtops (filter
 * operations), so that the number isn't reused during a boot.
 * Returns 0 if operation succeeded, or an appropriate errno(2) otherwise.
 */
int
kfilter_unregister(const char *name)
{
	struct kfilter *kfilter;

	if (name == NULL || name[0] == '\0')
		return (EINVAL);	/* invalid name */
	kfilter = kfilter_byname(name);
	if (kfilter == NULL)		/* not found */
		return (ENOENT);
	if (kfilter->filter < EVFILT_SYSCOUNT)
		return (EINVAL);	/* can't detach system filters */

	if (kfilter->name[0] != '\0') {
		free(kfilter->name, M_KEVENT);
		kfilter->name = "";	/* mark as `not implemented' */
	}
	if (kfilter->filtops != NULL) {
		free(kfilter->filtops, M_KEVENT);
		kfilter->filtops = NULL; /* mark as `not implemented' */
	}
	return (0);
}


/*
 * Filter attach method for EVFILT_READ and EVFILT_WRITE on normal file
 * descriptors. Calls struct fileops kqfilter method for given file descriptor.
 */
static int
filt_fileattach(struct knote *kn)
{
	struct file *fp;

	fp = kn->kn_fp;
	return ((*fp->f_ops->fo_kqfilter)(fp, kn));
}

/*
 * Filter detach method for EVFILT_READ on kqueue descriptor.
 */
static void
filt_kqdetach(struct knote *kn)
{
	struct kqueue *kq;

	kq = (struct kqueue *)kn->kn_fp->f_data;
	SLIST_REMOVE(&kq->kq_sel.si_klist, kn, knote, kn_selnext);
}

/*
 * Filter event method for EVFILT_READ on kqueue descriptor.
 */
/*ARGSUSED*/
static int
filt_kqueue(struct knote *kn, long hint)
{
	struct kqueue *kq;

	kq = (struct kqueue *)kn->kn_fp->f_data;
	kn->kn_data = kq->kq_count;
	return (kn->kn_data > 0);
}

/*
 * Filter attach method for EVFILT_PROC.
 */
static int
filt_procattach(struct knote *kn)
{
	struct proc *p;

	p = pfind(kn->kn_id);
	if (p == NULL)
		return (ESRCH);

	kn->kn_ptr.p_proc = p;
	kn->kn_flags |= EV_CLEAR;	/* automatically set */

	/*
	 * internal flag indicating registration done by kernel
	 */
	if (kn->kn_flags & EV_FLAG1) {
		kn->kn_data = kn->kn_sdata;	/* ppid */
		kn->kn_fflags = NOTE_CHILD;
		kn->kn_flags &= ~EV_FLAG1;
	}

	/* XXXLUKEM */
	/* XXX lock the proc here while adding to the list? */
	SLIST_INSERT_HEAD(&p->p_klist, kn, kn_selnext);

	return (0);
}

/*
 * Filter detach method for EVFILT_PROC.
 *
 * The knote may be attached to a different process, which may exit,
 * leaving nothing for the knote to be attached to.  So when the process
 * exits, the knote is marked as DETACHED and also flagged as ONESHOT so
 * it will be deleted when read out.  However, as part of the knote deletion,
 * this routine is called, so a check is needed to avoid actually performing
 * a detach, because the original process does not exist any more.
 */
static void
filt_procdetach(struct knote *kn)
{
	struct proc *p;

	p = kn->kn_ptr.p_proc;
	if (kn->kn_status & KN_DETACHED)
		return;

	/* XXXLUKEM */
	/* XXX locking?  this might modify another process. */
	SLIST_REMOVE(&p->p_klist, kn, knote, kn_selnext);
}

/*
 * Filter event method for EVFILT_PROC.
 */
static int
filt_proc(struct knote *kn, long hint)
{
	u_int event;

	/*
	 * mask off extra data
	 */
	event = (u_int)hint & NOTE_PCTRLMASK;

	/*
	 * if the user is interested in this event, record it.
	 */
	if (kn->kn_sfflags & event)
		kn->kn_fflags |= event;

	/*
	 * process is gone, so flag the event as finished.
	 */
	if (event == NOTE_EXIT) {
		kn->kn_status |= KN_DETACHED;
		kn->kn_flags |= (EV_EOF | EV_ONESHOT); 
		return (1);
	}

	/*
	 * process forked, and user wants to track the new process,
	 * so attach a new knote to it, and immediately report an
	 * event with the parent's pid.
	 */
	if ((event == NOTE_FORK) && (kn->kn_sfflags & NOTE_TRACK)) {
		struct kevent kev;
		int error;

		/*
		 * register knote with new process.
		 */
		kev.ident = hint & NOTE_PDATAMASK;	/* pid */
		kev.filter = kn->kn_filter;
		kev.flags = kn->kn_flags | EV_ADD | EV_ENABLE | EV_FLAG1;
		kev.fflags = kn->kn_sfflags;
		kev.data = kn->kn_id;			/* parent */
		kev.udata = kn->kn_kevent.udata;	/* preserve udata */
		error = kqueue_register(kn->kn_kq, &kev, NULL);
		if (error)
			kn->kn_fflags |= NOTE_TRACKERR;
	}

	return (kn->kn_fflags != 0);
}

/*
 * kqueue(2) system call.
 */
int
sys_kqueue(struct proc *p, void *v, register_t *retval)
{
	struct filedesc	*fdp;
	struct kqueue	*kq;
	struct file	*fp;
	int		fd, error;

	fdp = p->p_fd;
	error = falloc(p, &fp, &fd);	/* setup a new file descriptor */
	if (error)
		return (error);
	fp->f_flag = FREAD | FWRITE;
	fp->f_type = DTYPE_KQUEUE;
	fp->f_ops = &kqueueops;
	kq = pool_get(&kqueue_pool, PR_WAITOK);
	memset((char *)kq, 0, sizeof(struct kqueue));
	TAILQ_INIT(&kq->kq_head);
	fp->f_data = (caddr_t)kq;	/* store the kqueue with the fp */
	*retval = fd;
	if (fdp->fd_knlistsize < 0)
		fdp->fd_knlistsize = 0;	/* this process has a kq */
	kq->kq_fdp = fdp;
	FILE_SET_MATURE(fp);
	FILE_UNUSE(fp, p);		/* falloc() does FILE_USE() */
	return (error);
}

/*
 * kevent(2) system call.
 */
int
sys_kevent(struct proc *p, void *v, register_t *retval)
{
	struct sys_kevent_args /* {
		syscallarg(int) fd;
		syscallarg(const struct kevent *) changelist;
		syscallarg(int) nchanges;
		syscallarg(struct kevent *) eventlist;
		syscallarg(int) nevents;
		syscallarg(const struct timespec *) timeout;
	} */ *uap = v;
	struct filedesc	*fdp;
	struct kevent	*kevp;
	struct kqueue	*kq;
	struct file	*fp;
	struct timespec	ts;
	int		i, n, nerrors, error;

	fdp = p->p_fd;			/* check that we're dealing with a kq */
	if ((u_int)SCARG(uap, fd) >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[SCARG(uap, fd)]) == NULL ||
	    (fp->f_type != DTYPE_KQUEUE))
		return (EBADF);

	FILE_USE(fp);

	if (SCARG(uap, timeout) != NULL) {
		error = copyin(SCARG(uap, timeout), &ts, sizeof(ts));
		if (error)
			goto done;
		SCARG(uap, timeout) = &ts;
	}

	kq = (struct kqueue *)fp->f_data;
	nerrors = 0;

				/* traverse list of events to register */
	while (SCARG(uap, nchanges) > 0) {
				/* copyin a maximum of KQ_EVENTS at each pass */
		n = MIN(SCARG(uap, nchanges), KQ_NEVENTS);
		error = copyin(SCARG(uap, changelist), kq->kq_kev,
		    n * sizeof(struct kevent));
		if (error)
			goto done;
		for (i = 0; i < n; i++) {
			kevp = &kq->kq_kev[i];
			kevp->flags &= ~EV_SYSFLAGS;
					/* register each knote */
			error = kqueue_register(kq, kevp, p);
			if (error) {
				if (SCARG(uap, nevents) != 0) {
					kevp->flags = EV_ERROR;
					kevp->data = error;
					error = copyout((caddr_t)kevp,
					    (caddr_t)SCARG(uap, eventlist),
					    sizeof(*kevp));
					if (error)
						goto done;
					SCARG(uap, eventlist)++;
					SCARG(uap, nevents)--;
					nerrors++;
				} else {
					goto done;
				}
			}
		}
		SCARG(uap, nchanges) -= n;	/* update the results */
		SCARG(uap, changelist) += n;
	}
	if (nerrors) {
		*retval = nerrors;
		error = 0;
		goto done;
	}

					/* actually scan through the events */
	error = kqueue_scan(fp, SCARG(uap, nevents), SCARG(uap, eventlist),
	    SCARG(uap, timeout), p, retval);
 done:
	FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Register a given kevent kev onto the kqueue
 */
int
kqueue_register(struct kqueue *kq, struct kevent *kev, struct proc *p)
{
	struct filedesc	*fdp;
	struct kfilter	*kfilter;
	struct file	*fp;
	struct knote	*kn;
	int		s, error;

	fdp = kq->kq_fdp;
	fp = NULL;
	kn = NULL;
	error = 0;
	kfilter = kfilter_byfilter(kev->filter);
	if (kfilter == NULL || kfilter->filtops == NULL)
		return (EINVAL);	/* filter not found nor implemented */

					/* search if knote already exists */
	if (kfilter->filtops->f_isfd) {	/* monitoring a file descriptor */
		if ((u_int)kev->ident >= fdp->fd_nfiles ||
		    (fp = fdp->fd_ofiles[kev->ident]) == NULL)
			return (EBADF);	/* validate descriptor */
		FILE_USE(fp);

		if (kev->ident < fdp->fd_knlistsize) {
			SLIST_FOREACH(kn, &fdp->fd_knlist[kev->ident], kn_link)
				if (kq == kn->kn_kq &&
				    kev->filter == kn->kn_filter)
					break;
		}
	} else {
					/*
					 * not monitoring a file descriptor, so
					 * lookup knotes in internal hash table
					 */
		if (fdp->fd_knhashmask != 0) {
			struct klist *list;
			
			list = &fdp->fd_knhash[
			    KN_HASH((u_long)kev->ident, fdp->fd_knhashmask)];
			SLIST_FOREACH(kn, list, kn_link)
				if (kev->ident == kn->kn_id &&
				    kq == kn->kn_kq &&
				    kev->filter == kn->kn_filter)
					break;
		}
	}

	if (kn == NULL && ((kev->flags & EV_ADD) == 0)) {
		error = ENOENT;		/* filter not found */
		goto done;
	}

	/*
	 * kn now contains the matching knote, or NULL if no match
	 */
	if (kev->flags & EV_ADD) {		/* add knote */

		if (kn == NULL) {		/* create new knote */
			kn = knote_alloc();
			if (kn == NULL) {
				error = ENOMEM;
				goto done;
			}
			kn->kn_fp = fp;
			kn->kn_kq = kq;
			kn->kn_fop = kfilter->filtops;

			/*
			 * apply reference count to knote structure, and
			 * do not release it at the end of this routine.
			 */
			fp = NULL;

			kn->kn_sfflags = kev->fflags;
			kn->kn_sdata = kev->data;
			kev->fflags = 0;
			kev->data = 0;
			kn->kn_kevent = *kev;

			knote_attach(kn, fdp);
			if ((error = kfilter->filtops->f_attach(kn)) != 0) {
				knote_drop(kn, p);
				goto done;
			}
		} else {			/* modify existing knote */
			/*
			 * The user may change some filter values after the
			 * initial EV_ADD, but doing so will not reset any 
			 * filter which have already been triggered.
			 */
			kn->kn_sfflags = kev->fflags;
			kn->kn_sdata = kev->data;
			kn->kn_kevent.udata = kev->udata;
		}

		s = splhigh();
		if (kn->kn_fop->f_event(kn, 0))
			KNOTE_ACTIVATE(kn);
		splx(s);

	} else if (kev->flags & EV_DELETE) {	/* delete knote */
		kn->kn_fop->f_detach(kn);
		knote_drop(kn, p);
		goto done;
	}

						/* disable knote */
	if ((kev->flags & EV_DISABLE) &&
	    ((kn->kn_status & KN_DISABLED) == 0)) {
		s = splhigh();
		kn->kn_status |= KN_DISABLED;
		splx(s);
	}

						/* enable knote */
	if ((kev->flags & EV_ENABLE) && (kn->kn_status & KN_DISABLED)) {
		s = splhigh();
		kn->kn_status &= ~KN_DISABLED;
		if ((kn->kn_status & KN_ACTIVE) &&
		    ((kn->kn_status & KN_QUEUED) == 0))
			knote_enqueue(kn);
		splx(s);
	}

 done:
	if (fp != NULL)
		FILE_UNUSE(fp, p);
	return (error);
}

/*
 * Scan through the list of events on fp (for a maximum of maxevents),
 * returning the results in to ulistp. Timeout is determined by tsp; if
 * NULL, wait indefinitely, if 0 valued, perform a poll, otherwise wait
 * as appropriate.
 */
static int
kqueue_scan(struct file *fp, int maxevents, struct kevent *ulistp,
	const struct timespec *tsp, struct proc *p, register_t *retval)
{
	struct kqueue	*kq;
	struct kevent	*kevp;
	struct timeval	atv;
	struct knote	*kn, marker;
	int		s, count, timeout, nkev, error;

	kq = (struct kqueue *)fp->f_data;
	count = maxevents;
	nkev = error = 0;
	if (count == 0)
		goto done;

	if (tsp != NULL) {			/* timeout supplied */
		TIMESPEC_TO_TIMEVAL(&atv, tsp);
		if (itimerfix(&atv)) {
			error = EINVAL;
			goto done;
		}
		s = splclock();
		timeradd(&atv, &time, &atv);	/* calc. time to wait until */
		splx(s);
		if (tsp->tv_sec == 0 && tsp->tv_nsec == 0)
			timeout = -1;		/* perform a poll */
		else 
			timeout = hzto(&atv);	/* calculate hz till timeout */
	} else {
		atv.tv_sec = 0;			/* no timeout, wait forever */
		atv.tv_usec = 0;
		timeout = 0;
	}
	goto start;

 retry:
	if (atv.tv_sec || atv.tv_usec) {	/* timeout requested */
		s = splclock();
		if (timercmp(&time, &atv, >=)) {
			splx(s);	
			goto done;		/* timeout reached */
		}
		splx(s);
		timeout = hzto(&atv);		/* recalc. timeout remaining */
	}

 start:
	kevp = kq->kq_kev;
	s = splhigh();
	if (kq->kq_count == 0) {
		if (timeout < 0) { 
			error = EWOULDBLOCK;
		} else {
			kq->kq_state |= KQ_SLEEP;
			error = tsleep(kq, PSOCK | PCATCH, "kqread", timeout);
		}
		splx(s);
		if (error == 0)
			goto retry;
		/* don't restart after signals... */
		if (error == ERESTART)
			error = EINTR;
		else if (error == EWOULDBLOCK)
			error = 0;
		goto done;
	}

	TAILQ_INSERT_TAIL(&kq->kq_head, &marker, kn_tqe); 
						/* mark end of knote list */
	while (count) {				/* while user wants data ... */
		kn = TAILQ_FIRST(&kq->kq_head);	/* get next knote */
		TAILQ_REMOVE(&kq->kq_head, kn, kn_tqe); 
		if (kn == &marker) {		/* if it's our marker, stop */
			splx(s);
			if (count == maxevents)
				goto retry;
			goto done;
		}
		if (kn->kn_status & KN_DISABLED) {
						/* don't want disabled events */
			kn->kn_status &= ~KN_QUEUED;
			kq->kq_count--;
			continue;
		}
		if ((kn->kn_flags & EV_ONESHOT) == 0 &&
		    kn->kn_fop->f_event(kn, 0) == 0) {
					/*
					 * non-ONESHOT event that hasn't
					 * triggered again, so de-queue.
					 */
			kn->kn_status &= ~(KN_QUEUED | KN_ACTIVE);
			kq->kq_count--;
			continue;
		}
		*kevp = kn->kn_kevent;
		kevp++;
		nkev++;
		if (kn->kn_flags & EV_ONESHOT) {
				/* delete ONESHOT events after retrieval */
			kn->kn_status &= ~KN_QUEUED;
			kq->kq_count--;
			splx(s);
			kn->kn_fop->f_detach(kn);
			knote_drop(kn, p);
			s = splhigh();
		} else if (kn->kn_flags & EV_CLEAR) {
				/* clear state after retrieval */
			kn->kn_data = 0;
			kn->kn_fflags = 0;
			kn->kn_status &= ~(KN_QUEUED | KN_ACTIVE);
			kq->kq_count--;
		} else {
				/* add event back on list */
			TAILQ_INSERT_TAIL(&kq->kq_head, kn, kn_tqe); 
		}
		count--;
		if (nkev == KQ_NEVENTS) {
					/* do copyouts in KQ_NEVENTS chunks */
			splx(s);
			error = copyout((caddr_t)&kq->kq_kev, (caddr_t)ulistp,
			    sizeof(struct kevent) * nkev);
			ulistp += nkev;
			nkev = 0;
			kevp = kq->kq_kev;
			s = splhigh();
			if (error)
				break;
		}
	}
					/* remove marker */
	TAILQ_REMOVE(&kq->kq_head, &marker, kn_tqe); 
	splx(s);
 done:
	if (nkev != 0)			/* copyout remaining events */
		error = copyout((caddr_t)&kq->kq_kev, (caddr_t)ulistp,
		    sizeof(struct kevent) * nkev);
	*retval = maxevents - count;
	return (error);
}

/*
 * struct fileops read method for a kqueue descriptor.
 * Not implemented.
 * XXX: This could be expanded to call kqueue_scan, if desired.
 */
/*ARGSUSED*/
static int
kqueue_read(struct file *fp, off_t *offset, struct uio *uio,
	struct ucred *cred, int flags)
{

	return (ENXIO);
}

/*
 * struct fileops write method for a kqueue descriptor.
 * Not implemented.
 */
/*ARGSUSED*/
static int
kqueue_write(struct file *fp, off_t *offset, struct uio *uio,
	struct ucred *cred, int flags)
{

	return (ENXIO);
}

/*
 * struct fileops ioctl method for a kqueue descriptor.
 *
 * Two ioctls are currently supported. They both use struct kfilter_mapping:
 *	KFILTER_BYNAME		find name for filter, and return result in
 *				name, which is of size len.
 *	KFILTER_BYFILTER	find filter for name. len is ignored.
 */
/*ARGSUSED*/
static int
kqueue_ioctl(struct file *fp, u_long com, caddr_t data, struct proc *p)
{
	struct kfilter_mapping	*km;
	struct kfilter		*kfilter;
	char			*name;
	int			error;

	km = (struct kfilter_mapping *)data; 
	error = 0;

	switch (com) {
	case KFILTER_BYFILTER:	/* convert filter -> name */
		kfilter = kfilter_byfilter(km->filter);
		if (kfilter != NULL)
			error = copyoutstr(kfilter->name, km->name, km->len,
			    NULL);
		else
			error = ENOENT;
		break;

	case KFILTER_BYNAME:	/* convert name -> filter */
		MALLOC(name, char *, KFILTER_MAXNAME, M_KEVENT, M_WAITOK);
		error = copyinstr(km->name, name, KFILTER_MAXNAME, NULL);
		if (error) {
			free(name, M_KEVENT);
			break;
		}
		kfilter = kfilter_byname(name);
		if (kfilter != NULL)
			km->filter = kfilter->filter;
		else
			error = ENOENT;
		free(name, M_KEVENT);
		break;

#if 1		/* XXXLUKEM - test register & unregister */
	case KFILTER_REGISTER:
	case KFILTER_UNREGISTER:
		MALLOC(name, char *, KFILTER_MAXNAME, M_KEVENT, M_WAITOK);
		error = copyinstr(km->name, name, KFILTER_MAXNAME, NULL);
		if (error) {
			free(name, M_KEVENT);
			break;
		}
		if (com == KFILTER_REGISTER) {
			kfilter = kfilter_byfilter(km->filter);
			if (kfilter != NULL) {
				error = kfilter_register(name,
				    kfilter->filtops, &km->filter);
			} else
				error = ENOENT;
		} else
			error = kfilter_unregister(name);
		free(name, M_KEVENT);
		break;
#endif

	default:
		error = ENOTTY;

	}
	return (error);
}

/*
 * struct fileops fcntl method for a kqueue descriptor.
 * Not implemented.
 */
/*ARGSUSED*/
static int
kqueue_fcntl(struct file *fp, u_int com, caddr_t data, struct proc *p)
{

	return (ENOTTY);
}

/*
 * struct fileops poll method for a kqueue descriptor.
 * Determine if kqueue has events pending.
 */
/*ARGSUSED*/
static int
kqueue_poll(struct file *fp, int events, struct proc *p)
{
	struct kqueue	*kq;
	int		revents, s;

	kq = (struct kqueue *)fp->f_data;
	revents = 0;
	s = splnet();		/* XXXLUKEM: is this correct? */
	if (events & (POLLIN | POLLRDNORM)) {
		if (kq->kq_count) {
			revents |= events & (POLLIN | POLLRDNORM);
		} else {
				/* XXXLUKEM: splsched() for next? */
			selrecord(p, &kq->kq_sel);
			kq->kq_state |= KQ_SEL;
		}
	}
	splx(s);
	return (revents);
}

/*
 * struct fileops stat method for a kqueue descriptor.
 * Returns dummy info, with st_size being number of events pending.
 */
/*ARGSUSED*/
static int
kqueue_stat(struct file *fp, struct stat *st, struct proc *p)
{
	struct kqueue	*kq;

	kq = (struct kqueue *)fp->f_data;
	memset((void *)st, 0, sizeof(*st));
	st->st_size = kq->kq_count;
	st->st_blksize = sizeof(struct kevent);
	st->st_mode = S_IFIFO;
	return (0);
}

/*
 * struct fileops close method for a kqueue descriptor.
 * Cleans up kqueue.
 */
/*ARGSUSED*/
static int
kqueue_close(struct file *fp, struct proc *p)
{
	struct kqueue	*kq;
	struct filedesc	*fdp;
	struct knote	**knp, *kn, *kn0;
	int		i;

	kq = (struct kqueue *)fp->f_data;
	fdp = p->p_fd;
	for (i = 0; i < fdp->fd_knlistsize; i++) {
		knp = &SLIST_FIRST(&fdp->fd_knlist[i]);
		kn = *knp;
		while (kn != NULL) {
			kn0 = SLIST_NEXT(kn, kn_link);
			if (kq == kn->kn_kq) {
				kn->kn_fop->f_detach(kn);
				FILE_UNUSE(kn->kn_fp, p);
				knote_free(kn);
				*knp = kn0;
			} else {
				knp = &SLIST_NEXT(kn, kn_link);
			}
			kn = kn0;
		}
	}
	if (fdp->fd_knhashmask != 0) {
		for (i = 0; i < fdp->fd_knhashmask + 1; i++) {
			knp = &SLIST_FIRST(&fdp->fd_knhash[i]);
			kn = *knp;
			while (kn != NULL) {
				kn0 = SLIST_NEXT(kn, kn_link);
				if (kq == kn->kn_kq) {
					kn->kn_fop->f_detach(kn);
		/* XXX non-fd release of kn->kn_ptr */
					knote_free(kn);
					*knp = kn0;
				} else {
					knp = &SLIST_NEXT(kn, kn_link);
				}
				kn = kn0;
			}
		}
	}
	pool_put(&kqueue_pool, kq);
	fp->f_data = NULL;

	return (0);
}

/*
 * wakeup a kqueue
 */
static void
kqueue_wakeup(struct kqueue *kq)
{

	if (kq->kq_state & KQ_SLEEP) {		/* if currently sleeping ...  */
		kq->kq_state &= ~KQ_SLEEP;
		wakeup(kq);			/* ... wakeup */
	}
	if (kq->kq_state & KQ_SEL) {		/* if currently polling ... */
		kq->kq_state &= ~KQ_SEL;
		selwakeup(&kq->kq_sel);		/* ... selwakeup */
	}
	KNOTE(&kq->kq_sel.si_klist, 0);
}

/*
 * struct fileops kqfilter method for a kqueue descriptor.
 * Event triggered when monitored kqueue changes.
 */
/*ARGSUSED*/
static int
kqueue_kqfilter(struct file *fp, struct knote *kn)
{
	struct kqueue *kq;

	kq = (struct kqueue *)kn->kn_fp->f_data;
	if (kn->kn_filter != EVFILT_READ)
		return (1);
	kn->kn_fop = &kqread_filtops;
	SLIST_INSERT_HEAD(&kq->kq_sel.si_klist, kn, kn_selnext);
	return (0);
}


/*
 * Walk down a list of knotes, activating them if their event has triggered.
 */
void
knote(struct klist *list, long hint)
{
	struct knote *kn;

	SLIST_FOREACH(kn, list, kn_selnext)
		if (kn->kn_fop->f_event(kn, hint))
			KNOTE_ACTIVATE(kn);
}

/*
 * Remove all knotes from a specified klist
 */
void
knote_remove(struct proc *p, struct klist *list)
{
	struct knote *kn;

	while ((kn = SLIST_FIRST(list)) != NULL) {
		kn->kn_fop->f_detach(kn);
		knote_drop(kn, p);
	}
}

/*
 * Remove all knotes referencing a specified fd
 */
void
knote_fdclose(struct proc *p, int fd)
{
	struct filedesc	*fdp;
	struct klist	*list;

	fdp = p->p_fd;
	list = &fdp->fd_knlist[fd];
	knote_remove(p, list);
}

/*
 * Attach a new knote to a file descriptor
 */
static void
knote_attach(struct knote *kn, struct filedesc *fdp)
{
	struct klist	*list;
	int		size;

	if (! kn->kn_fop->f_isfd) {
					/*
					 * if knote is not on an fd, store
					 * on internal hash table.
					 */
		if (fdp->fd_knhashmask == 0)
			fdp->fd_knhash = hashinit(KN_HASHSIZE, HASH_LIST,
			    M_KEVENT, M_WAITOK, &fdp->fd_knhashmask);
		list = &fdp->fd_knhash[KN_HASH(kn->kn_id, fdp->fd_knhashmask)];
		goto done;
	}

					/*
					 * otherwise, knote is on an fd.
					 * knotes are stored in fd_knlist
					 * indexed by kn->kn_id.
					 */
	if (fdp->fd_knlistsize <= kn->kn_id) {
						/* expand list if too small */
		size = fdp->fd_knlistsize;
		while (size <= kn->kn_id)
			size += KQ_EXTENT;	/* grow in KQ_EXTENT chunks */
		list = malloc(size * sizeof(struct klist *), M_KEVENT,M_WAITOK);
						/* copy existing knlist */
		memcpy((caddr_t)list, (caddr_t)fdp->fd_knlist,
		    fdp->fd_knlistsize * sizeof(struct klist *));
						/* zero new sections */
		memset((caddr_t)list +
		    fdp->fd_knlistsize * sizeof(struct klist *), 0,
		    (size - fdp->fd_knlistsize) * sizeof(struct klist *));
		if (fdp->fd_knlist != NULL)	/* switch to new knlist */
			FREE(fdp->fd_knlist, M_KEVENT);
		fdp->fd_knlistsize = size;
		fdp->fd_knlist = list;
	}
	list = &fdp->fd_knlist[kn->kn_id];	/* get list head for this fd */
 done:
	SLIST_INSERT_HEAD(list, kn, kn_link);	/* add new knote */
	kn->kn_status = 0;
}

/*
 * Drop knote.
 * Should be called at spl == 0, since we don't want to hold spl
 * while calling FILE_UNUSE and free.
 */
static void
knote_drop(struct knote *kn, struct proc *p)
{
	struct filedesc	*fdp;
	struct klist	*list;

	fdp = p->p_fd;
	if (kn->kn_fop->f_isfd)
		list = &fdp->fd_knlist[kn->kn_id];
	else
		list = &fdp->fd_knhash[KN_HASH(kn->kn_id, fdp->fd_knhashmask)];

	SLIST_REMOVE(list, kn, knote, kn_link);
	if (kn->kn_status & KN_QUEUED)
		knote_dequeue(kn);
	if (kn->kn_fop->f_isfd)
		FILE_UNUSE(kn->kn_fp, p);
	knote_free(kn);
}


/*
 * Queue new event for knote.
 */
static void
knote_enqueue(struct knote *kn)
{
	struct kqueue	*kq;
	int		s;

	kq = kn->kn_kq;
	s = splhigh();
	KASSERT((kn->kn_status & KN_QUEUED) == 0);

	TAILQ_INSERT_TAIL(&kq->kq_head, kn, kn_tqe); 
	kn->kn_status |= KN_QUEUED;
	kq->kq_count++;
	splx(s);
	kqueue_wakeup(kq);
}

/*
 * Dequeue event for knote.
 */
static void
knote_dequeue(struct knote *kn)
{
	struct kqueue	*kq;
	int		s;

	kq = kn->kn_kq;
	s = splhigh();
	KASSERT(kn->kn_status & KN_QUEUED);

	TAILQ_REMOVE(&kq->kq_head, kn, kn_tqe); 
	kn->kn_status &= ~KN_QUEUED;
	kq->kq_count--;
	splx(s);
}

/*
 * Create a new knote.
 */
static struct knote *
knote_alloc(void)
{

	return (pool_get(&knote_pool, PR_WAITOK));
}

/*
 * Free a knote.
 */
static void
knote_free(struct knote *kn)
{

	pool_put(&knote_pool, kn);
}
