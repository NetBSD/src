/*	$NetBSD: event.h,v 1.1.1.1.2.4 2001/09/08 16:48:17 thorpej Exp $	*/
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
 *	$FreeBSD: src/sys/sys/event.h,v 1.12 2001/02/24 01:44:03 jlemon Exp $
 */

#ifndef _SYS_EVENT_H_
#define	_SYS_EVENT_H_

#include <sys/inttypes.h>		/* for uintptr_t */

#define	EVFILT_READ		0
#define	EVFILT_WRITE		1
#define	EVFILT_AIO		2	/* attached to aio requests */
#define	EVFILT_VNODE		3	/* attached to vnodes */
#define	EVFILT_PROC		4	/* attached to struct proc */
#define	EVFILT_SIGNAL		5	/* attached to struct proc */
#define	EVFILT_SYSCOUNT		6	/* number of filters */

#define	EV_SET(kevp, a, b, c, d, e, f)					\
do {									\
	(kevp)->ident = (a);						\
	(kevp)->filter = (b);						\
	(kevp)->flags = (c);						\
	(kevp)->fflags = (d);						\
	(kevp)->data = (e);						\
	(kevp)->udata = (f);						\
} while (/* CONSTCOND */ 0)


struct kevent {
	uintptr_t	ident;		/* identifier for this event */
	uint32_t	filter;		/* filter for event */
	uint32_t	flags;		/* action flags for kqueue */
	uint32_t	fflags;		/* filter flag value */
	uintptr_t	data;		/* filter data value */
	void		*udata;		/* opaque user data identifier */
};

/* actions */
#define	EV_ADD		0x0001		/* add event to kq (implies ENABLE) */
#define	EV_DELETE	0x0002		/* delete event from kq */
#define	EV_ENABLE	0x0004		/* enable event */
#define	EV_DISABLE	0x0008		/* disable event (not reported) */

/* flags */
#define	EV_ONESHOT	0x0010		/* only report one occurrence */
#define	EV_CLEAR	0x0020		/* clear event state after reporting */

#define	EV_SYSFLAGS	0xF000		/* reserved by system */
#define	EV_FLAG1	0x2000		/* filter-specific flag */

/* returned values */
#define	EV_EOF		0x8000		/* EOF detected */
#define	EV_ERROR	0x4000		/* error, data contains errno */

/*
 * data/hint flags for EVFILT_{READ|WRITE}, shared with userspace
 */
#define	NOTE_LOWAT	0x0001		/* low water mark */

/*
 * data/hint flags for EVFILT_VNODE, shared with userspace
 */
#define	NOTE_DELETE	0x0001			/* vnode was removed */
#define	NOTE_WRITE	0x0002			/* data contents changed */
#define	NOTE_EXTEND	0x0004			/* size increased */
#define	NOTE_ATTRIB	0x0008			/* attributes changed */
#define	NOTE_LINK	0x0010			/* link count changed */
#define	NOTE_RENAME	0x0020			/* vnode was renamed */
#define	NOTE_REVOKE	0x0040			/* vnode access was revoked */

/*
 * data/hint flags for EVFILT_PROC, shared with userspace
 */
#define	NOTE_EXIT	0x80000000		/* process exited */
#define	NOTE_FORK	0x40000000		/* process forked */
#define	NOTE_EXEC	0x20000000		/* process exec'd */
#define	NOTE_PCTRLMASK	0xf0000000		/* mask for hint bits */
#define	NOTE_PDATAMASK	0x000fffff		/* mask for pid */

/* additional flags for EVFILT_PROC */
#define	NOTE_TRACK	0x00000001		/* follow across forks */
#define	NOTE_TRACKERR	0x00000002		/* could not track child */
#define	NOTE_CHILD	0x00000004		/* am a child process */

/*
 * This is currently visible to userland to work around broken
 * programs which pull in <sys/proc.h> or <sys/select.h>.
 */
#include <sys/queue.h> 
struct knote;
SLIST_HEAD(klist, knote);


/*
 * ioctl(2)s supported on kqueue descriptors.
 */
#include <sys/ioctl.h>

struct kfilter_mapping {
	char		*name;		/* name to lookup or return */
	size_t		len;		/* length of name */
	uint32_t	filter;		/* filter to lookup or return */
};

					/* map filter to name (max size len) */ 
#define KFILTER_BYFILTER	_IOWR('k', 0, struct kfilter_mapping)
					/* map name to filter (len ignored) */
#define KFILTER_BYNAME		_IOWR('k', 1, struct kfilter_mapping)

#if 1	/* XXXLUKEM - debug only; remove from production code */
					/*
					 * register name, mapping filtops
					 * to those of filter, returning new
					 * number in filter
					 */
#define KFILTER_REGISTER	_IOWR('k', 2, struct kfilter_mapping)
					/* unregister name */
#define KFILTER_UNREGISTER	_IOWR('k', 3, struct kfilter_mapping)
#endif


#ifdef _KERNEL

#define	KNOTE(list, hint)	if ((list) != NULL) knote(list, hint)

/*
 * Flag indicating hint is a signal.  Used by EVFILT_SIGNAL, and also
 * shared by EVFILT_PROC  (all knotes attached to p->p_klist)
 */
#define	NOTE_SIGNAL	0x08000000

/*
 * Callback methods for each filter type.
 */
struct filterops {
	int	f_isfd;			/* true if ident == filedescriptor */
	int	(*f_attach)	__P((struct knote *kn));
					/* called when knote is ADDed */
	void	(*f_detach)	__P((struct knote *kn));
					/* called when knote is DELETEd */
	int	(*f_event)	__P((struct knote *kn, long hint));
					/* called when event is triggered */
};

struct knote {
	SLIST_ENTRY(knote)	kn_link;	/* for fd */
	SLIST_ENTRY(knote)	kn_selnext;	/* for struct selinfo */
	TAILQ_ENTRY(knote)	kn_tqe;
	struct kqueue		*kn_kq;		/* which queue we are on */
	struct kevent		kn_kevent;
	uint32_t		kn_status;
	uint32_t		kn_sfflags;	/* saved filter flags */
	uintptr_t		kn_sdata;	/* saved data field */
	union {
		struct file	*p_fp;		/* file data pointer */
		struct proc	*p_proc;	/* proc pointer */
		void		*p_opaque;	/* opaque/misc pointer */
	} kn_ptr;
	const struct filterops	*kn_fop;
	caddr_t			kn_hook;

#define	KN_ACTIVE	0x01			/* event has been triggered */
#define	KN_QUEUED	0x02			/* event is on queue */
#define	KN_DISABLED	0x04			/* event is disabled */
#define	KN_DETACHED	0x08			/* knote is detached */

#define	kn_id		kn_kevent.ident
#define	kn_filter	kn_kevent.filter
#define	kn_flags	kn_kevent.flags
#define	kn_fflags	kn_kevent.fflags
#define	kn_data		kn_kevent.data
#define	kn_fp		kn_ptr.p_fp
};

struct proc;

void		kqueue_init(void);

extern void	knote(struct klist *list, long hint);
extern void	knote_remove(struct proc *p, struct klist *list);
extern void	knote_fdclose(struct proc *p, int fd);
extern int 	kqueue_register(struct kqueue *kq,
		    struct kevent *kev, struct proc *p);

extern int	kfilter_register(const char *name,
		    const struct filterops *filtops, int *retfilter);
extern int	kfilter_unregister(const char *name);

extern int	filt_seltrue(struct knote *kn, long hint);

#else 	/* !_KERNEL */

#include <sys/cdefs.h>
struct timespec;

__BEGIN_DECLS
#if !defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE)
int	kqueue __P((void));
int	kevent __P((int kq, const struct kevent *changelist, int nchanges,
		    struct kevent *eventlist, int nevents,
		    const struct timespec *timeout));
#endif /* !_POSIX_C_SOURCE */
__END_DECLS

#endif /* !_KERNEL */

#endif /* !_SYS_EVENT_H_ */
