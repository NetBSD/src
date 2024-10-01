/*	$NetBSD: linux_inotify.c,v 1.7 2024/10/01 16:41:29 riastradh Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Theodore Preduta.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_inotify.c,v 1.7 2024/10/01 16:41:29 riastradh Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/bitops.h>
#include <sys/dirent.h>
#include <sys/event.h>
#include <sys/eventvar.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/selinfo.h>
#include <sys/select.h>
#include <sys/signal.h>
#include <sys/vnode.h>

#include <sys/syscallargs.h>

#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/common/linux_fcntl.h>
#include <compat/linux/common/linux_inotify.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sched.h>
#include <compat/linux/common/linux_sem.h>
#include <compat/linux/common/linux_signal.h>

#include <compat/linux/linux_syscallargs.h>

/*
 * inotify(2).  This interface allows the user to get file system
 * events and (unlike kqueue(2)) their order is strictly preserved.
 * While nice, the API has sufficient gotchas that mean we don't want
 * to add native entry points for it.  They are:
 *
 * - Because data is returned via read(2), this API is prone to
 *   unaligned memory accesses.  There is a note in the Linux man page
 *   that says the name field of struct linux_inotify_event *can* be
 *   used for alignment purposes.  In practice, even Linux doesn't
 *   always do this, so for simplicity, we don't ever do this.
 */

#define	LINUX_INOTIFY_MAX_QUEUED	16384
#define	LINUX_INOTIFY_MAX_FROM_KEVENT	3

#if DEBUG_LINUX
#define	DPRINTF(x) uprintf x
#else
#define	DPRINTF(x) __nothing
#endif

struct inotify_entry {
	TAILQ_ENTRY(inotify_entry)	ie_entries;
	char				ie_name[NAME_MAX + 1];
	struct linux_inotify_event	ie_event;
};

struct inotify_dir_entries {
	size_t	ide_count;
	struct inotify_dir_entry {
		char	name[NAME_MAX + 1];
		ino_t	fileno;
	} ide_entries[];
};
#define	INOTIFY_DIR_ENTRIES_SIZE(count)	(sizeof(struct inotify_dir_entries) \
    + count * sizeof(struct inotify_dir_entry))

struct inotifyfd {
	int		ifd_kqfd;	/* kqueue fd used by this inotify */
					/* instance */
	struct selinfo	ifd_sel;	/* for EVFILT_READ by epoll */
	kmutex_t	ifd_lock;	/* lock for ifd_sel, ifd_wds and */
					/* ifd_nwds */

	struct inotify_dir_entries **ifd_wds;
					/* keeps track of watch descriptors */
					/* for directories: snapshot of the */
					/* directory state */
					/* for files: an inotify_dir_entries */
					/* with ide_count == 0 */
	size_t		ifd_nwds;	/* max watch descriptor that can be */
					/* stored in ifd_wds + 1 */

        TAILQ_HEAD(, inotify_entry) ifd_qhead;	/* queue of pending events */
	size_t		ifd_qcount;	/* number of pending events */
	kcondvar_t	ifd_qcv;	/* condvar for blocking reads */
	kmutex_t	ifd_qlock;	/* lock for ifd_q* and interlock */
					/* for ifd_qcv */
};

struct inotify_kevent_mask_pair {
	uint32_t inotify;
	uint32_t kevent;
};

static int	inotify_kev_fetch_changes(void *, const struct kevent *,
    struct kevent *, size_t, int);
static int	do_inotify_init(struct lwp *, register_t *, int);
static int	inotify_close_wd(struct inotifyfd *, int);
static uint32_t	inotify_mask_to_kevent_fflags(uint32_t, enum vtype);
static void	do_kevent_to_inotify(int32_t, uint32_t, uint32_t,
    struct inotify_entry *, size_t *, char *);
static int	kevent_to_inotify(struct inotifyfd *, int, enum vtype, uint32_t,
    uint32_t, struct inotify_entry *, size_t *);
static int	inotify_readdir(file_t *, struct dirent *, int *, bool);
static struct inotify_dir_entries *get_inotify_dir_entries(int, bool);

static int	inotify_filt_attach(struct knote *);
static void	inotify_filt_detach(struct knote *);
static int	inotify_filt_event(struct knote *, long);
static void	inotify_read_filt_detach(struct knote *);
static int	inotify_read_filt_event(struct knote *, long);

static int	inotify_read(file_t *, off_t *, struct uio *, kauth_cred_t, int);
static int	inotify_close(file_t *);
static int	inotify_poll(file_t *, int);
static int	inotify_kqfilter(file_t *, struct knote *);
static void	inotify_restart(file_t *);

static const char inotify_filtname[] = "LINUX_INOTIFY";
static int inotify_filtid;

/* "fake" EVFILT_VNODE that gets attached to ifd_deps */
static const struct filterops inotify_filtops = {
	.f_flags = FILTEROP_ISFD | FILTEROP_MPSAFE,
	.f_attach = inotify_filt_attach,
	.f_detach = inotify_filt_detach,
	.f_event = inotify_filt_event,
	.f_touch = NULL,
};

/* EVFILT_READ attached to inotifyfd (to support watching via epoll) */
static const struct filterops inotify_read_filtops = {
	.f_flags = FILTEROP_ISFD | FILTEROP_MPSAFE,
	.f_attach = NULL, /* attached via .fo_kqfilter */
	.f_detach = inotify_read_filt_detach,
	.f_event = inotify_read_filt_event,
	.f_touch = NULL,
};

static const struct fileops inotify_fileops = {
	.fo_name = "inotify",
	.fo_read = inotify_read,
	.fo_write = fbadop_write,
	.fo_ioctl = fbadop_ioctl,
	.fo_fcntl = fnullop_fcntl,
	.fo_poll = inotify_poll,
	.fo_stat = fbadop_stat,
	.fo_close = inotify_close,
	.fo_kqfilter = inotify_kqfilter,
	.fo_restart = inotify_restart,
	.fo_fpathconf = (void *)eopnotsupp,
};

/* basic flag translations */
static const struct inotify_kevent_mask_pair common_inotify_to_kevent[] = {
	{ .inotify = LINUX_IN_ATTRIB,		.kevent = NOTE_ATTRIB, },
	{ .inotify = LINUX_IN_CLOSE_NOWRITE,	.kevent = NOTE_CLOSE, },
	{ .inotify = LINUX_IN_OPEN,		.kevent = NOTE_OPEN, },
	{ .inotify = LINUX_IN_MOVE_SELF,	.kevent = NOTE_RENAME, },
};
static const size_t common_inotify_to_kevent_len =
    __arraycount(common_inotify_to_kevent);

static const struct inotify_kevent_mask_pair vreg_inotify_to_kevent[] = {
	{ .inotify = LINUX_IN_ACCESS,		.kevent = NOTE_READ, },
	{ .inotify = LINUX_IN_ATTRIB,		.kevent = NOTE_ATTRIB|NOTE_LINK, },
	{ .inotify = LINUX_IN_CLOSE_WRITE,	.kevent = NOTE_CLOSE_WRITE, },
	{ .inotify = LINUX_IN_MODIFY,		.kevent = NOTE_WRITE, },
};
static const size_t vreg_inotify_to_kevent_len =
    __arraycount(vreg_inotify_to_kevent);

static const struct inotify_kevent_mask_pair vdir_inotify_to_kevent[] = {
	{ .inotify = LINUX_IN_ACCESS,		.kevent = NOTE_READ, },
	{ .inotify = LINUX_IN_CREATE,		.kevent = NOTE_WRITE, },
	{ .inotify = LINUX_IN_DELETE,		.kevent = NOTE_WRITE, },
	{ .inotify = LINUX_IN_MOVED_FROM,	.kevent = NOTE_WRITE, },
	{ .inotify = LINUX_IN_MOVED_TO,		.kevent = NOTE_WRITE, },
};
static const size_t vdir_inotify_to_kevent_len =
    __arraycount(vdir_inotify_to_kevent);

static const struct inotify_kevent_mask_pair common_kevent_to_inotify[] = {
	{ .kevent = NOTE_ATTRIB,	.inotify = LINUX_IN_ATTRIB, },
	{ .kevent = NOTE_CLOSE,		.inotify = LINUX_IN_CLOSE_NOWRITE, },
	{ .kevent = NOTE_CLOSE_WRITE,	.inotify = LINUX_IN_CLOSE_WRITE, },
	{ .kevent = NOTE_OPEN,		.inotify = LINUX_IN_OPEN, },
	{ .kevent = NOTE_READ,		.inotify = LINUX_IN_ACCESS, },
	{ .kevent = NOTE_RENAME,	.inotify = LINUX_IN_MOVE_SELF, },
	{ .kevent = NOTE_REVOKE,	.inotify = LINUX_IN_UNMOUNT, },
};
static const size_t common_kevent_to_inotify_len =
    __arraycount(common_kevent_to_inotify);

static const struct inotify_kevent_mask_pair vreg_kevent_to_inotify[] = {
	{ .kevent = NOTE_DELETE|NOTE_LINK, .inotify = LINUX_IN_ATTRIB, },
	{ .kevent = NOTE_WRITE,		.inotify = LINUX_IN_MODIFY, },
};
static const size_t vreg_kevent_to_inotify_len =
    __arraycount(vreg_kevent_to_inotify);

/*
 * Register the custom kfilter for inotify.
 */
int
linux_inotify_init(void)
{
	return kfilter_register(inotify_filtname, &inotify_filtops,
	    &inotify_filtid);
}

/*
 * Unregister the custom kfilter for inotify.
 */
int
linux_inotify_fini(void)
{
	return kfilter_unregister(inotify_filtname);
}

/*
 * Copyin callback used by kevent.  This copies already converted
 * filters from kernel memory to the kevent internal kernel memory.
 * Hence the memcpy instead of copyin.
 */
static int
inotify_kev_fetch_changes(void *ctx, const struct kevent *changelist,
    struct kevent *changes, size_t index, int n)
{
	memcpy(changes, changelist + index, n * sizeof(*changes));

	return 0;
}

/*
 * Initialize a new inotify fd.
 */
static int
do_inotify_init(struct lwp *l, register_t *retval, int flags)
{
	file_t *fp;
	int error, fd;
	struct proc *p = l->l_proc;
	struct inotifyfd *ifd;
	struct sys_kqueue1_args kqa;

	if (flags & ~(LINUX_IN_ALL_FLAGS))
		return EINVAL;

	ifd = kmem_zalloc(sizeof(*ifd), KM_SLEEP);
	mutex_init(&ifd->ifd_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&ifd->ifd_qlock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&ifd->ifd_qcv, "inotify");
	selinit(&ifd->ifd_sel);
	TAILQ_INIT(&ifd->ifd_qhead);

	ifd->ifd_nwds = 1;
	ifd->ifd_wds = kmem_zalloc(ifd->ifd_nwds * sizeof(*ifd->ifd_wds),
	KM_SLEEP);

	SCARG(&kqa, flags) = 0;
	if (flags & LINUX_IN_NONBLOCK)
		SCARG(&kqa, flags) |= O_NONBLOCK;
	error = sys_kqueue1(l, &kqa, retval);
	if (error != 0)
		goto leave0;
	ifd->ifd_kqfd = *retval;

	error = fd_allocfile(&fp, &fd);
	if (error != 0)
		goto leave1;

	fp->f_flag = FREAD;
	if (flags & LINUX_IN_NONBLOCK)
		fp->f_flag |= FNONBLOCK;
	fp->f_type = DTYPE_MISC;
	fp->f_ops = &inotify_fileops;
	fp->f_data = ifd;
	fd_set_exclose(l, fd, (flags & LINUX_IN_CLOEXEC) != 0);
	fd_affix(p, fp, fd);

	*retval = fd;
	return 0;

leave1:
	KASSERT(fd_getfile(ifd->ifd_kqfd) != NULL);
	fd_close(ifd->ifd_kqfd);
leave0:
	kmem_free(ifd->ifd_wds, ifd->ifd_nwds * sizeof(*ifd->ifd_wds));
	kmem_free(ifd, sizeof(*ifd));

	mutex_destroy(&ifd->ifd_lock);
	mutex_destroy(&ifd->ifd_qlock);
	cv_destroy(&ifd->ifd_qcv);
	seldestroy(&ifd->ifd_sel);

	return error;
}

#ifndef __aarch64__
/*
 * inotify_init(2).  Initialize a new inotify fd with flags=0.
 */
int
linux_sys_inotify_init(struct lwp *l, const void *v, register_t *retval)
{
	return do_inotify_init(l, retval, 0);
}
#endif

/*
 * inotify_init(2).  Initialize a new inotify fd with the given flags.
 */
int
linux_sys_inotify_init1(struct lwp *l,
    const struct linux_sys_inotify_init1_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) flags;
	} */

	return do_inotify_init(l, retval, SCARG(uap, flags));
}

/*
 * Convert inotify mask to the fflags of an equivalent kevent.
 */
static uint32_t
inotify_mask_to_kevent_fflags(uint32_t mask, enum vtype type)
{
	const struct inotify_kevent_mask_pair *type_inotify_to_kevent;
	uint32_t fflags;
	size_t i, type_inotify_to_kevent_len;

	switch (type) {
	case VREG:
	case VDIR:
	case VLNK:
		break;

	default:
		return 0;
	}

	/* flags that all watches could have */
	fflags = NOTE_DELETE|NOTE_REVOKE;
	for (i = 0; i < common_inotify_to_kevent_len; i++)
		if (mask & common_inotify_to_kevent[i].inotify)
			fflags |= common_inotify_to_kevent[i].kevent;

	/* flags that depend on type */
	switch (type) {
	case VREG:
		type_inotify_to_kevent = vreg_inotify_to_kevent;
		type_inotify_to_kevent_len = vreg_inotify_to_kevent_len;
		break;

	case VDIR:
		type_inotify_to_kevent = vdir_inotify_to_kevent;
		type_inotify_to_kevent_len = vdir_inotify_to_kevent_len;
		break;

	default:
		type_inotify_to_kevent_len = 0;
		break;
	}
	for (i = 0; i < type_inotify_to_kevent_len; i++)
		if (mask & type_inotify_to_kevent[i].inotify)
			fflags |= type_inotify_to_kevent[i].kevent;

	return fflags;
}

/*
 * inotify_add_watch(2).  Open a fd for pathname (if desired by mask)
 * track it and add an equivalent kqueue event for it in
 * ifd->ifd_kqfd.
 */
int
linux_sys_inotify_add_watch(struct lwp *l,
    const struct linux_sys_inotify_add_watch_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const char *) pathname;
		syscallarg(uint32_t) mask;
	} */
	int wd, i, error = 0;
	file_t *fp, *wp, *cur_fp;
	struct inotifyfd *ifd;
	struct inotify_dir_entries **new_wds;
	struct knote *kn, *tmpkn;
	struct sys_open_args oa;
	struct kevent kev;
	struct vnode *wvp;
	namei_simple_flags_t sflags;
	struct kevent_ops k_ops = {
		.keo_private = NULL,
		.keo_fetch_timeout = NULL,
		.keo_fetch_changes = inotify_kev_fetch_changes,
		.keo_put_events = NULL,
	};
	const int fd = SCARG(uap, fd);
	const uint32_t mask = SCARG(uap, mask);

	if (mask & ~LINUX_IN_ADD_KNOWN)
		return EINVAL;

	fp = fd_getfile(fd);
	if (fp == NULL)
		return EBADF;

	if (fp->f_ops != &inotify_fileops) {
		/* not an inotify fd */
		error = EBADF;
		goto leave0;
	}

	ifd = fp->f_data;

	mutex_enter(&ifd->ifd_lock);

	if (mask & LINUX_IN_DONT_FOLLOW)
		sflags = NSM_NOFOLLOW_TRYEMULROOT;
	else
		sflags = NSM_FOLLOW_TRYEMULROOT;
	error = namei_simple_user(SCARG(uap, pathname), sflags, &wvp);
	if (error != 0)
		goto leave1;

	/* Check to see if we already have a descriptor to wd's file. */
        wd = -1;
	for (i = 0; i < ifd->ifd_nwds; i++) {
		if (ifd->ifd_wds[i] != NULL) {
			cur_fp = fd_getfile(i);
			if (cur_fp == NULL) {
				DPRINTF(("%s: wd=%d was closed externally\n",
				    __func__, i));
				error = EBADF;
				goto leave1;
			}
			if (cur_fp->f_type != DTYPE_VNODE) {
				DPRINTF(("%s: wd=%d was replaced "
				    "with a non-vnode\n", __func__, i));
				error = EBADF;
			}
			if (error == 0 && cur_fp->f_vnode == wvp)
				wd = i;
			fd_putfile(i);
			if (error != 0)
				goto leave1;

			if (wd != -1)
				break;
		}
	}

	if (wd == -1) {
		/*
		 * If we do not have a descriptor to wd's file, we
		 * need to open the watch descriptor.
		 */
		SCARG(&oa, path) = SCARG(uap, pathname);
		SCARG(&oa, mode) = 0;
		SCARG(&oa, flags) = O_RDONLY;
		if (mask & LINUX_IN_DONT_FOLLOW)
			SCARG(&oa, flags) |= O_NOFOLLOW;
		if (mask & LINUX_IN_ONLYDIR)
			SCARG(&oa, flags) |= O_DIRECTORY;

		error = sys_open(l, &oa, retval);
		if (error != 0)
			goto leave1;
		wd = *retval;
		wp = fd_getfile(wd);
	        KASSERT(wp != NULL);
		KASSERT(wp->f_type == DTYPE_VNODE);

		/* translate the flags */
		memset(&kev, 0, sizeof(kev));
		EV_SET(&kev, wd, inotify_filtid, EV_ADD|EV_ENABLE,
		    NOTE_DELETE|NOTE_REVOKE, 0, ifd);
		if (mask & LINUX_IN_ONESHOT)
			kev.flags |= EV_ONESHOT;
		kev.fflags |= inotify_mask_to_kevent_fflags(mask,
		    wp->f_vnode->v_type);

		error = kevent1(retval, ifd->ifd_kqfd, &kev, 1, NULL, 0, NULL,
		    &k_ops);
		if (error != 0) {
			KASSERT(fd_getfile(wd) != NULL);
			fd_close(wd);
		} else {
			/* Success! */
			*retval = wd;

			/* Resize ifd_nwds to accommodate wd. */
			if (wd+1 > ifd->ifd_nwds) {
				new_wds = kmem_zalloc(
				    (wd+1) * sizeof(*ifd->ifd_wds), KM_SLEEP);
				memcpy(new_wds, ifd->ifd_wds,
				    ifd->ifd_nwds * sizeof(*ifd->ifd_wds));

				kmem_free(ifd->ifd_wds,
				    ifd->ifd_nwds * sizeof(*ifd->ifd_wds));

				ifd->ifd_wds = new_wds;
				ifd->ifd_nwds = wd+1;
			}

			ifd->ifd_wds[wd] = get_inotify_dir_entries(wd, true);
		}
	} else {
		/*
		 * If we do have a descriptor to wd's file, try to edit
		 * the relevant knote.
		 */
		if (mask & LINUX_IN_MASK_CREATE) {
			error = EEXIST;
			goto leave1;
		}

		wp = fd_getfile(wd);
		if (wp == NULL) {
			DPRINTF(("%s: wd=%d was closed externally "
			    "(race, probably)\n", __func__, wd));
			error = EBADF;
			goto leave1;
		}
		if (wp->f_type != DTYPE_VNODE) {
			DPRINTF(("%s: wd=%d was replace with a non-vnode "
			    "(race, probably)\n", __func__, wd));
			error = EBADF;
			goto leave2;
		}

		kev.fflags = NOTE_DELETE | NOTE_REVOKE
		    | inotify_mask_to_kevent_fflags(mask, wp->f_vnode->v_type);

		mutex_enter(wp->f_vnode->v_interlock);

		/*
		 * XXX We are forced to find the appropriate knote
		 * manually because we cannot create a custom f_touch
		 * function for inotify_filtops.  See filter_touch()
		 * in kern_event.c for details.
		 */
	        SLIST_FOREACH_SAFE(kn, &wp->f_vnode->v_klist->vk_klist,
		    kn_selnext, tmpkn) {
			if (kn->kn_fop == &inotify_filtops
			    && ifd == kn->kn_kevent.udata) {
				mutex_enter(&kn->kn_kq->kq_lock);
				if (mask & LINUX_IN_MASK_ADD)
					kn->kn_sfflags |= kev.fflags;
				else
					kn->kn_sfflags = kev.fflags;
				wp->f_vnode->v_klist->vk_interest |=
				    kn->kn_sfflags;
				mutex_exit(&kn->kn_kq->kq_lock);
			}
		}

		mutex_exit(wp->f_vnode->v_interlock);

		/* Success! */
		*retval = wd;
	}

leave2:
	fd_putfile(wd);
leave1:
	mutex_exit(&ifd->ifd_lock);
leave0:
	fd_putfile(fd);
	return error;
}

/*
 * Remove a wd from ifd and close wd.
 */
static int
inotify_close_wd(struct inotifyfd *ifd, int wd)
{
	file_t *wp;
	int error;
	register_t retval;
	struct kevent kev;
	struct kevent_ops k_ops = {
		.keo_private = NULL,
		.keo_fetch_timeout = NULL,
		.keo_fetch_changes = inotify_kev_fetch_changes,
		.keo_put_events = NULL,
	};

	mutex_enter(&ifd->ifd_lock);

	KASSERT(0 <= wd && wd < ifd->ifd_nwds && ifd->ifd_wds[wd] != NULL);

	kmem_free(ifd->ifd_wds[wd],
	    INOTIFY_DIR_ENTRIES_SIZE(ifd->ifd_wds[wd]->ide_count));
	ifd->ifd_wds[wd] = NULL;

	mutex_exit(&ifd->ifd_lock);

	wp = fd_getfile(wd);
	if (wp == NULL) {
		DPRINTF(("%s: wd=%d is already closed\n", __func__, wd));
		return 0;
	}
	KASSERT(!mutex_owned(wp->f_vnode->v_interlock));

	memset(&kev, 0, sizeof(kev));
	EV_SET(&kev, wd, EVFILT_VNODE, EV_DELETE, 0, 0, 0);
	error = kevent1(&retval, ifd->ifd_kqfd, &kev, 1, NULL, 0, NULL, &k_ops);
	if (error != 0)
		DPRINTF(("%s: attempt to disable all events for wd=%d "
		    "had error=%d\n", __func__, wd, error));

	return fd_close(wd);
}

/*
 * inotify_rm_watch(2).  Close wd and remove it from ifd->ifd_wds.
 */
int
linux_sys_inotify_rm_watch(struct lwp *l,
    const struct linux_sys_inotify_rm_watch_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) wd;
	} */
	struct inotifyfd *ifd;
	file_t *fp;
	int error = 0;
	const int fd = SCARG(uap, fd);
	const int wd = SCARG(uap, wd);

	fp = fd_getfile(fd);
	if (fp == NULL)
		return EBADF;
	if (fp->f_ops != &inotify_fileops) {
		/* not an inotify fd */
		error = EINVAL;
		goto leave;
	}

	ifd = fp->f_data;
	if (wd < 0 || wd >= ifd->ifd_nwds || ifd->ifd_wds[wd] == NULL) {
		error = EINVAL;
		goto leave;
	}

	error = inotify_close_wd(ifd, wd);

leave:
	fd_putfile(fd);
	return error;
}

/*
 * Attach the inotify filter.
 */
static int
inotify_filt_attach(struct knote *kn)
{
	file_t *fp = kn->kn_obj;
	struct vnode *vp;

	KASSERT(fp->f_type == DTYPE_VNODE);
	vp = fp->f_vnode;

	/*
	 * Needs to be set so that we get the same event handling as
	 * EVFILT_VNODE.  Otherwise we don't get any events.
	 *
	 * A consequence of this is that modifications/removals of
	 * this knote need to specify EVFILT_VNODE rather than
	 * inotify_filtid.
	 */
	kn->kn_filter = EVFILT_VNODE;

	kn->kn_fop = &inotify_filtops;
	kn->kn_hook = vp;
	vn_knote_attach(vp, kn);

	return 0;
}

/*
 * Detach the inotify filter.
 */
static void
inotify_filt_detach(struct knote *kn)
{
	struct vnode *vp = (struct vnode *)kn->kn_hook;

	vn_knote_detach(vp, kn);
}

/*
 * Create a single inotify event.
 */
static void
do_kevent_to_inotify(int32_t wd, uint32_t mask, uint32_t cookie,
    struct inotify_entry *buf, size_t *nbuf, char *name)
{
	KASSERT(*nbuf < LINUX_INOTIFY_MAX_FROM_KEVENT);

	buf += *nbuf;

	memset(buf, 0, sizeof(*buf));

	buf->ie_event.wd = wd;
	buf->ie_event.mask = mask;
	buf->ie_event.cookie = cookie;

	if (name != NULL) {
		buf->ie_event.len = strlen(name) + 1;
		KASSERT(buf->ie_event.len < sizeof(buf->ie_name));
		strcpy(buf->ie_name, name);
	}

	++(*nbuf);
}

/*
 * Like vn_readdir(), but with vnode locking only if needs_lock is
 * true (to avoid double locking in some situations).
 */
static int
inotify_readdir(file_t *fp, struct dirent *dep, int *done, bool needs_lock)
{
	struct vnode *vp;
	struct iovec iov;
	struct uio uio;
	int error, eofflag;

	KASSERT(fp->f_type == DTYPE_VNODE);
	vp = fp->f_vnode;
	KASSERT(vp->v_type == VDIR);

	iov.iov_base = dep;
	iov.iov_len = sizeof(*dep);

	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_rw = UIO_READ;
	uio.uio_resid = sizeof(*dep);
	UIO_SETUP_SYSSPACE(&uio);

	mutex_enter(&fp->f_lock);
	uio.uio_offset = fp->f_offset;
	mutex_exit(&fp->f_lock);

	/* XXX: should pass whether to lock or not */
	if (needs_lock)
		vn_lock(vp, LK_SHARED | LK_RETRY);
	else
		/*
		 * XXX We need to temprarily drop v_interlock because
		 * it may be temporarily acquired by biowait().
		 */
		mutex_exit(vp->v_interlock);
	KASSERT(!mutex_owned(vp->v_interlock));
	error = VOP_READDIR(vp, &uio, fp->f_cred, &eofflag, NULL, NULL);
	if (needs_lock)
		VOP_UNLOCK(vp);
	else
		mutex_enter(vp->v_interlock);

	mutex_enter(&fp->f_lock);
	fp->f_offset = uio.uio_offset;
	mutex_exit(&fp->f_lock);

	*done = sizeof(*dep) - uio.uio_resid;
	return error;
}

/*
 * Create (and allocate) an appropriate inotify_dir_entries struct for wd to be
 * used on ifd_wds of inotifyfd.  If the entries on a directory fail to be read,
 * NULL is returned.  needs_lock indicates if the vnode's lock is not already
 * owned.
 */
static struct inotify_dir_entries *
get_inotify_dir_entries(int wd, bool needs_lock)
{
	struct dirent de;
	struct dirent *currdep;
	struct inotify_dir_entries *idep = NULL;
	file_t *wp;
	int done, error;
	size_t i, decount;

	wp = fd_getfile(wd);
	if (wp == NULL)
		return NULL;
	if (wp->f_type != DTYPE_VNODE)
		goto leave;

	/* for non-directories, we have 0 entries. */
	if (wp->f_vnode->v_type != VDIR) {
		idep = kmem_zalloc(INOTIFY_DIR_ENTRIES_SIZE(0), KM_SLEEP);
		goto leave;
	}

	mutex_enter(&wp->f_lock);
	wp->f_offset = 0;
	mutex_exit(&wp->f_lock);
	decount = 0;
	for (;;) {
		error = inotify_readdir(wp, &de, &done, needs_lock);
		if (error != 0)
			goto leave;
		if (done == 0)
			break;

		currdep = &de;
	        while ((char *)currdep < ((char *)&de) + done) {
			decount++;
			currdep = _DIRENT_NEXT(currdep);
		}
	}

	idep = kmem_zalloc(INOTIFY_DIR_ENTRIES_SIZE(decount), KM_SLEEP);
	idep->ide_count = decount;

	mutex_enter(&wp->f_lock);
	wp->f_offset = 0;
	mutex_exit(&wp->f_lock);
	for (i = 0; i < decount;) {
		error = inotify_readdir(wp, &de, &done, needs_lock);
		if (error != 0 || done == 0) {
			kmem_free(idep, INOTIFY_DIR_ENTRIES_SIZE(decount));
			idep = NULL;
			goto leave;
		}

		currdep = &de;
		while ((char *)currdep < ((char *)&de) + done) {
			idep->ide_entries[i].fileno = currdep->d_fileno;
			strcpy(idep->ide_entries[i].name, currdep->d_name);

			currdep = _DIRENT_NEXT(currdep);
			i++;
		}
	}

leave:
	fd_putfile(wd);
	return idep;
}

static size_t
find_entry(struct inotify_dir_entries *i1, struct inotify_dir_entries *i2)
{
	for (size_t i = 0; i < i2->ide_count; i++)
		if (i2->ide_entries[i].fileno != i1->ide_entries[i].fileno)
			return i;
	KASSERTMSG(0, "Entry not found");
	return -1;
}

static void
handle_write(struct inotifyfd *ifd, int wd, struct inotify_entry *buf,
    size_t *nbuf)
{
	struct inotify_dir_entries *old_idep, *new_idep;
	size_t i;

	mutex_enter(&ifd->ifd_lock);

	old_idep = ifd->ifd_wds[wd];
	KASSERT(old_idep != NULL);
	new_idep = get_inotify_dir_entries(wd, false);
	if (new_idep == NULL) {
		DPRINTF(("%s: directory for wd=%d could not be read\n",
		    __func__, wd));
		mutex_exit(&ifd->ifd_lock);
		return;
	}


	if (old_idep->ide_count < new_idep->ide_count) {
		KASSERT(old_idep->ide_count + 1 == new_idep->ide_count);

		/* Find the new entry. */
		i = find_entry(new_idep, old_idep);
		do_kevent_to_inotify(wd, LINUX_IN_CREATE, 0,
		    buf, nbuf, new_idep->ide_entries[i].name);
		goto out;
	}

	if (old_idep->ide_count > new_idep->ide_count) {
		KASSERT(old_idep->ide_count == new_idep->ide_count + 1);

		/* Find the deleted entry. */
		i = find_entry(old_idep, new_idep);

		do_kevent_to_inotify(wd, LINUX_IN_DELETE, 0,
		    buf, nbuf, old_idep->ide_entries[i].name);
		goto out;
	}

	/*
	 * XXX Because we are not watching the entire
	 * file system, the only time we know for sure
	 * that the event is a LINUX_IN_MOVED_FROM/
	 * LINUX_IN_MOVED_TO is when the move happens
	 * within a single directory...  ie. the number
	 * of directory entries has not changed.
	 *
	 * Otherwise all we can say for sure is that
	 * something was created/deleted.  So we issue a
	 * LINUX_IN_CREATE/LINUX_IN_DELETE.
	 */
	ino_t changed = new_idep->ide_entries[new_idep->ide_count - 1].fileno;

	/* Find the deleted entry. */
	for (i = 0; i < old_idep->ide_count; i++)
		if (old_idep->ide_entries[i].fileno == changed)
			break;
	KASSERT(i != old_idep->ide_count);

	do_kevent_to_inotify(wd, LINUX_IN_MOVED_FROM, changed, buf, nbuf,
	    old_idep->ide_entries[i].name);

	do_kevent_to_inotify(wd, LINUX_IN_MOVED_TO, changed, buf, nbuf,
	    new_idep->ide_entries[new_idep->ide_count - 1].name);

out:
	ifd->ifd_wds[wd] = new_idep;
	mutex_exit(&ifd->ifd_lock);
}

/*
 * Convert a kevent flags and fflags for EVFILT_VNODE to some number
 * of inotify events.
 */
static int
kevent_to_inotify(struct inotifyfd *ifd, int wd, enum vtype wtype,
    uint32_t flags, uint32_t fflags, struct inotify_entry *buf,
    size_t *nbuf)
{
	struct stat st;
	file_t *wp;
	size_t i;
	int error = 0;

	for (i = 0; i < common_kevent_to_inotify_len; i++)
		if (fflags & common_kevent_to_inotify[i].kevent)
			do_kevent_to_inotify(wd,
			    common_kevent_to_inotify[i].inotify, 0, buf, nbuf,
			    NULL);

	if (wtype == VREG) {
		for (i = 0; i < vreg_kevent_to_inotify_len; i++)
			if (fflags & vreg_kevent_to_inotify[i].kevent)
				do_kevent_to_inotify(wd,
				    vreg_kevent_to_inotify[i].inotify, 0,
				    buf, nbuf, NULL);
	} else if (wtype == VDIR) {
		for (i = 0; i < *nbuf; i++)
			if (buf[i].ie_event.mask &
			    (LINUX_IN_ACCESS|LINUX_IN_ATTRIB
		            |LINUX_IN_CLOSE|LINUX_IN_OPEN))
				buf[i].ie_event.mask |= LINUX_IN_ISDIR;

		/* Need to disambiguate the possible NOTE_WRITEs. */
		if (fflags & NOTE_WRITE)
			handle_write(ifd, wd, buf, nbuf);
	}

	/*
	 * Need to check if wd is actually has a link count of 0 to issue a
	 * LINUX_IN_DELETE_SELF.
	 */
	if (fflags & NOTE_DELETE) {
		wp = fd_getfile(wd);
		KASSERT(wp != NULL);
		KASSERT(wp->f_type == DTYPE_VNODE);
		vn_stat(wp->f_vnode, &st);
		fd_putfile(wd);

		if (st.st_nlink == 0)
			do_kevent_to_inotify(wd, LINUX_IN_DELETE_SELF, 0,
			    buf, nbuf, NULL);
	}

	/* LINUX_IN_IGNORED must be the last event issued for wd. */
	if ((flags & EV_ONESHOT) || (fflags & (NOTE_REVOKE|NOTE_DELETE))) {
		do_kevent_to_inotify(wd, LINUX_IN_IGNORED, 0, buf, nbuf, NULL);
		/*
		 * XXX in theory we could call inotify_close_wd(ifd, wd) but if
		 * we get here we must already be holding v_interlock for
		 * wd... so we can't.
		 *
		 * For simplicity we do nothing, and so wd will only be closed
		 * when the inotify fd is closed.
		 */
	}

	return error;
}

/*
 * Handle an event.  Unlike EVFILT_VNODE, we translate the event to a
 * linux_inotify_event and put it in our own custom queue.
 */
static int
inotify_filt_event(struct knote *kn, long hint)
{
        struct vnode *vp = (struct vnode *)kn->kn_hook;
	struct inotifyfd *ifd;
	struct inotify_entry *cur_ie;
	size_t nbuf, i;
	uint32_t status;
	struct inotify_entry buf[LINUX_INOTIFY_MAX_FROM_KEVENT];

	/*
	 * If KN_WILLDETACH is set then
	 * 1. kn->kn_kevent.udata has already been trashed with a
	 *    struct lwp *, so we don't have access to a real ifd
	 *    anymore, and
	 * 2. we're about to detach anyways, so we don't really care
	 *    about the events.
	 * (Also because of this we need to get ifd under the same
	 * lock as kn->kn_status.)
	 */
	mutex_enter(&kn->kn_kq->kq_lock);
	status = kn->kn_status;
	ifd = kn->kn_kevent.udata;
	mutex_exit(&kn->kn_kq->kq_lock);
	if (status & KN_WILLDETACH)
		return 0;

	/*
	 * If we don't care about the NOTEs in hint, we don't generate
	 * any events.
	 */
	hint &= kn->kn_sfflags;
	if (hint == 0)
		return 0;

	KASSERT(mutex_owned(vp->v_interlock));
	KASSERT(!mutex_owned(&ifd->ifd_lock));

	mutex_enter(&ifd->ifd_qlock);

	/*
	 * early out: there's no point even traslating the event if we
	 * have nowhere to put it (and an LINUX_IN_Q_OVERFLOW has
	 * already been added).
	 */
	if (ifd->ifd_qcount >= LINUX_INOTIFY_MAX_QUEUED)
		goto leave;

	nbuf = 0;
	(void)kevent_to_inotify(ifd, kn->kn_id, vp->v_type, kn->kn_flags,
	    hint, buf, &nbuf);
	for (i = 0; i < nbuf && ifd->ifd_qcount < LINUX_INOTIFY_MAX_QUEUED-1;
	     i++) {
		cur_ie = kmem_zalloc(sizeof(*cur_ie), KM_SLEEP);
		memcpy(cur_ie, &buf[i], sizeof(*cur_ie));

		TAILQ_INSERT_TAIL(&ifd->ifd_qhead, cur_ie, ie_entries);
		ifd->ifd_qcount++;
	}
	/* handle early overflow, by adding an overflow event to the end */
	if (i != nbuf) {
		nbuf = 0;
		cur_ie = kmem_zalloc(sizeof(*cur_ie), KM_SLEEP);
		do_kevent_to_inotify(-1, LINUX_IN_Q_OVERFLOW, 0,
		    cur_ie, &nbuf, NULL);

		TAILQ_INSERT_TAIL(&ifd->ifd_qhead, cur_ie, ie_entries);
		ifd->ifd_qcount++;
	}

	if (nbuf > 0) {
		cv_signal(&ifd->ifd_qcv);

		mutex_enter(&ifd->ifd_lock);
		selnotify(&ifd->ifd_sel, 0, NOTE_LOWAT);
		mutex_exit(&ifd->ifd_lock);
	} else
		DPRINTF(("%s: hint=%lx resulted in 0 inotify events\n",
		    __func__, hint));

leave:
	mutex_exit(&ifd->ifd_qlock);
	return 0;
}

/*
 * Read inotify events from the queue.
 */
static int
inotify_read(file_t *fp, off_t *offp, struct uio *uio, kauth_cred_t cred,
    int flags)
{
	struct inotify_entry *cur_iep;
	size_t cur_size, nread;
	int error = 0;
	struct inotifyfd *ifd = fp->f_data;

	mutex_enter(&ifd->ifd_qlock);

	if (ifd->ifd_qcount == 0) {
		if (fp->f_flag & O_NONBLOCK) {
			error = EAGAIN;
			goto leave;
		}

		while (ifd->ifd_qcount == 0) {
			/* wait until there is an event to read */
			error = cv_wait_sig(&ifd->ifd_qcv, &ifd->ifd_qlock);
			if (error != 0) {
				error = EINTR;
				goto leave;
			}
		}
	}

	KASSERT(ifd->ifd_qcount > 0);
	KASSERT(mutex_owned(&ifd->ifd_qlock));

	nread = 0;
	while (ifd->ifd_qcount > 0) {
		cur_iep = TAILQ_FIRST(&ifd->ifd_qhead);
		KASSERT(cur_iep != NULL);

		cur_size = sizeof(cur_iep->ie_event) + cur_iep->ie_event.len;
		if (cur_size > uio->uio_resid) {
			if (nread == 0)
				error = EINVAL;
			break;
		}

		error = uiomove(&cur_iep->ie_event, sizeof(cur_iep->ie_event),
		    uio);
		if (error != 0)
			break;
		error = uiomove(&cur_iep->ie_name, cur_iep->ie_event.len, uio);
		if (error != 0)
			break;

		/* cleanup */
		TAILQ_REMOVE(&ifd->ifd_qhead, cur_iep, ie_entries);
		kmem_free(cur_iep, sizeof(*cur_iep));

		nread++;
		ifd->ifd_qcount--;
	}

leave:
	/* Wake up the next reader, if the queue is not empty. */
	if (ifd->ifd_qcount > 0)
		cv_signal(&ifd->ifd_qcv);

	mutex_exit(&ifd->ifd_qlock);
	return error;
}

/*
 * Close all the file descriptors associated with fp.
 */
static int
inotify_close(file_t *fp)
{
	int error;
	size_t i;
	file_t *kqfp;
	struct inotifyfd *ifd = fp->f_data;

	for (i = 0; i < ifd->ifd_nwds; i++) {
		if (ifd->ifd_wds[i] != NULL) {
			error = inotify_close_wd(ifd, i);
			if (error != 0)
				return error;
		}
	}

	/* the reference we need to hold is ifd->ifd_kqfp */
	kqfp = fd_getfile(ifd->ifd_kqfd);
	if (kqfp == NULL) {
		DPRINTF(("%s: kqfp=%d is already closed\n", __func__,
		    ifd->ifd_kqfd));
	} else {
		error = fd_close(ifd->ifd_kqfd);
		if (error != 0)
			return error;
	}

	mutex_destroy(&ifd->ifd_lock);
	mutex_destroy(&ifd->ifd_qlock);
	cv_destroy(&ifd->ifd_qcv);
	seldestroy(&ifd->ifd_sel);

	kmem_free(ifd->ifd_wds, ifd->ifd_nwds * sizeof(*ifd->ifd_wds));
	kmem_free(ifd, sizeof(*ifd));
	fp->f_data = NULL;

	return 0;
}

/*
 * Check if there are pending read events.
 */
static int
inotify_poll(file_t *fp, int events)
{
	int revents;
	struct inotifyfd *ifd = fp->f_data;

	revents = 0;
	if (events & (POLLIN|POLLRDNORM)) {
		mutex_enter(&ifd->ifd_qlock);

		if (ifd->ifd_qcount > 0)
			revents |= events & (POLLIN|POLLRDNORM);

		mutex_exit(&ifd->ifd_qlock);
	}

	return revents;
}

/*
 * Attach EVFILT_READ to the inotify instance in fp.
 *
 * This is so you can watch inotify with epoll.  No other kqueue
 * filter needs to be supported.
 */
static int
inotify_kqfilter(file_t *fp, struct knote *kn)
{
	struct inotifyfd *ifd = fp->f_data;

	KASSERT(fp == kn->kn_obj);

	if (kn->kn_filter != EVFILT_READ)
		return EINVAL;

	kn->kn_fop = &inotify_read_filtops;
	mutex_enter(&ifd->ifd_lock);
	selrecord_knote(&ifd->ifd_sel, kn);
	mutex_exit(&ifd->ifd_lock);

	return 0;
}

/*
 * Detach a filter from an inotify instance.
 */
static void
inotify_read_filt_detach(struct knote *kn)
{
	struct inotifyfd *ifd = ((file_t *)kn->kn_obj)->f_data;

	mutex_enter(&ifd->ifd_lock);
	selremove_knote(&ifd->ifd_sel, kn);
	mutex_exit(&ifd->ifd_lock);
}

/*
 * Handle EVFILT_READ events.  Note that nothing is put in kn_data.
 */
static int
inotify_read_filt_event(struct knote *kn, long hint)
{
	struct inotifyfd *ifd = ((file_t *)kn->kn_obj)->f_data;

	if (hint != 0) {
		KASSERT(mutex_owned(&ifd->ifd_lock));
		KASSERT(mutex_owned(&ifd->ifd_qlock));
		KASSERT(hint == NOTE_LOWAT);

		kn->kn_data = ifd->ifd_qcount;
	}

	return kn->kn_data > 0;
}

/*
 * Restart the inotify instance.
 */
static void
inotify_restart(file_t *fp)
{
	struct inotifyfd *ifd = fp->f_data;

	mutex_enter(&ifd->ifd_qlock);
	cv_broadcast(&ifd->ifd_qcv);
	mutex_exit(&ifd->ifd_qlock);
}
