/*	$NetBSD: smbfs_kq.c,v 1.2 2003/03/24 06:39:51 jdolecek Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jaromir Dolecek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: smbfs_kq.c,v 1.2 2003/03/24 06:39:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/fcntl.h>
#include <sys/mount.h>
#include <sys/unistd.h>
#include <sys/vnode.h>
#include <sys/lockf.h>
#include <sys/malloc.h>
#include <sys/kthread.h>
#include <sys/file.h>

#include <machine/limits.h>

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>

#include <netsmb/smb.h>
#include <netsmb/smb_conn.h>
#include <netsmb/smb_subr.h>

#include <fs/smbfs/smbfs.h>
#include <fs/smbfs/smbfs_node.h>
#include <fs/smbfs/smbfs_subr.h>

#include <miscfs/genfs/genfs.h>

struct kevq {
	SLIST_ENTRY(kevq)	kev_link;
	struct vnode		*vp;
	u_int			usecount;
	u_int			flags;
#define KEVQ_BUSY	0x01	/* currently being processed */
#define KEVQ_WANT	0x02	/* want to change this entry */
	struct timespec		omtime;	/* old modification time */
	struct timespec		octime;	/* old change time */
	nlink_t			onlink;	/* old number of references to file */
};
SLIST_HEAD(kevqlist, kevq);

static struct lock smbfskevq_lock;
static struct proc *psmbfskq;
static struct kevqlist kevlist = SLIST_HEAD_INITIALIZER(kevlist);

void
smbfs_kqinit(void)
{
	lockinit(&smbfskevq_lock, PSOCK, "smbkqlck", 0, 0);
}

/*
 * This quite simplistic routine periodically checks for server changes
 * of any of the watched files every NFS_MINATTRTIME/2 seconds.
 * Only changes in size, modification time, change time and nlinks
 * are being checked, everything else is ignored.
 * The routine only calls VOP_GETATTR() when it's likely it would get
 * some new data, i.e. when the vnode expires from attrcache. This
 * should give same result as periodically running stat(2) from userland,
 * while keeping CPU/network usage low, and still provide proper kevent
 * semantics.
 * The poller thread is created when first vnode is added to watch list,
 * and exits when the watch list is empty. The overhead of thread creation
 * isn't really important, neither speed of attach and detach of knote.
 */
/* ARGSUSED */
static void
smbfs_kqpoll(void *arg)
{
	struct kevq *ke;
	struct vattr attr;
	int error;
	struct proc *p = psmbfskq;
	u_quad_t osize;

	for(;;) {
		lockmgr(&smbfskevq_lock, LK_EXCLUSIVE, NULL);
		SLIST_FOREACH(ke, &kevlist, kev_link) {
			/* skip if still in attrcache */
			if (smbfs_attr_cachelookup(ke->vp, &attr) != ENOENT)
				continue;

			/*
			 * Mark entry busy, release lock and check
			 * for changes.
			 */
			ke->flags |= KEVQ_BUSY;
			lockmgr(&smbfskevq_lock, LK_RELEASE, NULL);

			/* save v_size, smbfs_getattr() updates it */
			osize = ke->vp->v_size;

			error = VOP_GETATTR(ke->vp, &attr, p->p_ucred, p);
			if (error)
				continue;

			/* following is a bit fragile, but about best
			 * we can get */
			if (ke->vp->v_type != VDIR && attr.va_size != osize) {
				int extended = (attr.va_size > osize);
				VN_KNOTE(ke->vp, NOTE_WRITE
					| (extended ? NOTE_EXTEND : 0));
				ke->omtime = attr.va_mtime;
			} else if (attr.va_mtime.tv_sec != ke->omtime.tv_sec
			    || attr.va_mtime.tv_nsec != ke->omtime.tv_nsec) {
				VN_KNOTE(ke->vp, NOTE_WRITE);
				ke->omtime = attr.va_mtime;
			}

			if (attr.va_ctime.tv_sec != ke->octime.tv_sec
			    || attr.va_ctime.tv_nsec != ke->octime.tv_nsec) {
				VN_KNOTE(ke->vp, NOTE_ATTRIB);
				ke->octime = attr.va_ctime;
			}

			if (attr.va_nlink != ke->onlink) {
				VN_KNOTE(ke->vp, NOTE_LINK);
				ke->onlink = attr.va_nlink;
			}

			lockmgr(&smbfskevq_lock, LK_EXCLUSIVE, NULL);
			ke->flags &= ~KEVQ_BUSY;
			if (ke->flags & KEVQ_WANT) {
				ke->flags &= ~KEVQ_WANT;
				wakeup(ke);
			}
		}

		if (SLIST_EMPTY(&kevlist)) {
			/* Nothing more to watch, exit */
			psmbfskq = NULL;
			lockmgr(&smbfskevq_lock, LK_RELEASE, NULL);
			kthread_exit(0);
		}
		lockmgr(&smbfskevq_lock, LK_RELEASE, NULL);

		/* wait a while before checking for changes again */
		tsleep(psmbfskq, PSOCK, "smbkqpw",
			SMBFS_ATTRTIMO * hz / 2);

	}
}

static void
filt_smbfsdetach(struct knote *kn)
{
	struct vnode *vp = (struct vnode *)kn->kn_hook;
	struct kevq *ke;

	/* XXXLUKEM lock the struct? */
	SLIST_REMOVE(&vp->v_klist, kn, knote, kn_selnext);

	/* Remove the vnode from watch list */
	lockmgr(&smbfskevq_lock, LK_EXCLUSIVE, NULL);
	SLIST_FOREACH(ke, &kevlist, kev_link) {
		if (ke->vp == vp) {
			while (ke->flags & KEVQ_BUSY) {
				ke->flags |= KEVQ_WANT;
				lockmgr(&smbfskevq_lock, LK_RELEASE, NULL);
				(void) tsleep(ke, PSOCK, "smbkqdet", 0);
				lockmgr(&smbfskevq_lock, LK_EXCLUSIVE, NULL);
			}

			if (ke->usecount > 1) {
				/* keep, other kevents need this */
				ke->usecount--;
			} else {
				/* last user, g/c */
				SLIST_REMOVE(&kevlist, ke, kevq, kev_link);
				FREE(ke, M_KEVENT);
			}
			break;
		}
	}
	lockmgr(&smbfskevq_lock, LK_RELEASE, NULL);
}

static int
filt_smbfsread(struct knote *kn, long hint)
{
	struct vnode *vp = (struct vnode *)kn->kn_hook;

	/*
	 * filesystem is gone, so set the EOF flag and schedule
	 * the knote for deletion.
	 */
	if (hint == NOTE_REVOKE) {
		kn->kn_flags |= (EV_EOF | EV_ONESHOT);
		return (1);
	}

	/* There is no size info for directories */
	if (((struct vnode *)kn->kn_hook)->v_type == VDIR) {
		/*
		 * This is kind of hackish, since we need to
		 * set the flag when we are called with the hint
		 * to make confirming call from kern_event.c
		 * succeed too, but need to unset it afterwards
		 * so that the directory wouldn't stay flagged
		 * as changed.
		 * XXX perhaps just fail for directories?
		 */
		if (hint & NOTE_WRITE) {
			kn->kn_fflags |= NOTE_WRITE;
			return (1);
		} else if (hint == 0 && (kn->kn_fflags & NOTE_WRITE)) {
			kn->kn_fflags &= ~NOTE_WRITE;
			return (1);
		} else
			return (0);
	}

	/* XXXLUKEM lock the struct? */
	kn->kn_data = vp->v_size - kn->kn_fp->f_offset;
        return (kn->kn_data != 0);
}

static int
filt_smbfsvnode(struct knote *kn, long hint)
{

	if (kn->kn_sfflags & hint)
		kn->kn_fflags |= hint;
	if (hint == NOTE_REVOKE) {
		kn->kn_flags |= EV_EOF;
		return (1);
	}
	return (kn->kn_fflags != 0);
}

static const struct filterops smbfsread_filtops = 
	{ 1, NULL, filt_smbfsdetach, filt_smbfsread };
static const struct filterops smbfsvnode_filtops = 
	{ 1, NULL, filt_smbfsdetach, filt_smbfsvnode };

int
smbfs_kqfilter(void *v)
{
	struct vop_kqfilter_args /* {
		struct vnode	*a_vp;
		struct knote	*a_kn;
	} */ *ap = v;
	struct vnode *vp;
	struct knote *kn;
	struct kevq *ke;
	int error = 0;
	struct vattr attr;
	struct proc *p = curproc;	/* XXX */

	vp = ap->a_vp;
	kn = ap->a_kn;
	switch (kn->kn_filter) {
	case EVFILT_READ:
		kn->kn_fop = &smbfsread_filtops;
		break;
	case EVFILT_VNODE:
		kn->kn_fop = &smbfsvnode_filtops;
		break;
	default:
		return (1);
	}

	kn->kn_hook = vp;

	/* XXXLUKEM lock the struct? */
	SLIST_INSERT_HEAD(&vp->v_klist, kn, kn_selnext);

	/*
	 * Put the vnode to watched list.
	 */
	
	/*
	 * Fetch current attributes. It's only needed when the vnode
	 * is not watched yet, but we need to do this without lock
	 * held. This is likely cheap due to attrcache, so do it now.
	 */ 
	memset(&attr, 0, sizeof(attr));
	(void) VOP_GETATTR(vp, &attr, p->p_ucred, p);

	lockmgr(&smbfskevq_lock, LK_EXCLUSIVE, NULL);

	/* ensure the poller is running */
	if (!psmbfskq) {
		error = kthread_create1(smbfs_kqpoll, NULL, &psmbfskq,
				"smbkqpoll");
		if (error)
			goto out;
	}

	SLIST_FOREACH(ke, &kevlist, kev_link) {
		if (ke->vp == vp)
			break;
	}

	if (ke) {
		/* already watched, so just bump usecount */
		ke->usecount++;
	} else {
		/* need a new one */
		MALLOC(ke, struct kevq *, sizeof(struct kevq), M_KEVENT,
			M_WAITOK);
		ke->vp = vp;
		ke->usecount = 1;
		ke->flags = 0;
		ke->omtime = attr.va_mtime;
		ke->octime = attr.va_ctime;
		ke->onlink = attr.va_nlink;
		SLIST_INSERT_HEAD(&kevlist, ke, kev_link);
	}

	/* kick the poller */
	wakeup(psmbfskq);

    out:
	lockmgr(&smbfskevq_lock, LK_RELEASE, NULL);

	return (error);
}
