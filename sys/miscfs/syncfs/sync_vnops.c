/*	$NetBSD: sync_vnops.c,v 1.2 1999/11/15 18:49:10 fvdl Exp $	*/

/*
 * Copyright 1997 Marshall Kirk McKusick. All Rights Reserved.
 *
 * This code is derived from work done by Greg Ganger at the
 * University of Michigan.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. None of the names of McKusick, Ganger, or the University of Michigan
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MARSHALL KIRK MCKUSICK ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL MARSHALL KIRK MCKUSICK BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/errno.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/syncfs/syncfs.h>

int (**sync_vnodeop_p) __P((void *));
struct vnodeopv_entry_desc sync_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_close_desc, sync_close },		/* close */
	{ &vop_fsync_desc, sync_fsync },		/* fsync */
	{ &vop_inactive_desc, sync_inactive },		/* inactive */
	{ &vop_reclaim_desc, sync_reclaim },		/* reclaim */
	{ &vop_lock_desc, sync_lock },			/* lock */
	{ &vop_unlock_desc, sync_unlock },		/* unlock */
	{ &vop_print_desc, sync_print },		/* print */
	{ &vop_islocked_desc, sync_islocked },		/* islocked */
	{ (struct vnodeop_desc*)NULL, (int(*) __P((void *)))NULL }
};

struct vnodeopv_desc sync_vnodeop_opv_desc =
	{ &sync_vnodeop_p, sync_vnodeop_entries };

/*
 * Create a new filesystem syncer vnode for the specified mount point.
 */
int
vfs_allocate_syncvnode(mp)
	struct mount *mp;
{
	struct vnode *vp;
	static long start, incr, next;
	int error;

	/* Allocate a new vnode */
	if ((error = getnewvnode(VT_VFS, mp, sync_vnodeop_p, &vp)) != 0) {
		mp->mnt_syncer = NULL;
		return (error);
	}
	vp->v_writecount = 1;
	vp->v_type = VNON;
	/*
	 * Place the vnode onto the syncer worklist. We attempt to
	 * scatter them about on the list so that they will go off
	 * at evenly distributed times even if all the filesystems
	 * are mounted at once.
	 */
	next += incr;
	if (next == 0 || next > syncer_maxdelay) {
		start /= 2;
		incr /= 2;
		if (start == 0) {
			start = syncer_maxdelay / 2;
			incr = syncer_maxdelay;
		}
		next = start;
	}
	vn_syncer_add_to_worklist(vp, syncdelay > 0 ? next % syncdelay : 0);
	mp->mnt_syncer = vp;
	return (0);
}

/*
 * Do a lazy sync of the filesystem.
 */
int
sync_fsync(v)
	void *v;
{
	struct vop_fsync_args /* {
		struct vnode *a_vp;
		struct ucred *a_cred;
		int a_flags;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *syncvp = ap->a_vp;
	struct mount *mp = syncvp->v_mount;
	int asyncflag;

	/*
	 * We only need to do something if this is a lazy evaluation.
	 */
	if (!(ap->a_flags & FSYNC_LAZY))
		return (0);

	/*
	 * Move ourselves to the back of the sync list.
	 */
	vn_syncer_add_to_worklist(syncvp, syncdelay);

	/*
	 * Walk the list of vnodes pushing all that are dirty and
	 * not already on the sync list.
	 */
	simple_lock(&mountlist_slock);
	if (vfs_busy(mp, LK_NOWAIT, &mountlist_slock) == 0) {
		asyncflag = mp->mnt_flag & MNT_ASYNC;
		mp->mnt_flag &= ~MNT_ASYNC;
		VFS_SYNC(mp, MNT_LAZY, ap->a_cred, ap->a_p);
		if (asyncflag)
			mp->mnt_flag |= MNT_ASYNC;
		vfs_unbusy(mp);
	} else
		simple_unlock(&mountlist_slock);
	return (0);
}

/*
 * The syncer vnode is no longer needed and is being decommissioned.
 */
int
sync_inactive(v)
	void *v;
{
	struct vop_inactive_args /* {
		struct vnode *a_vp;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	VOP_UNLOCK(vp, 0);
	if (vp->v_usecount == 0)
		vgone(vp);
	return (0);
}

int
sync_reclaim(v)
	void *v;
{
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	int s;

	s = splbio();
	vp->v_mount->mnt_syncer = NULL;
	LIST_REMOVE(vp, v_synclist);
	splx(s);

	return 0;
}

/*
 * Print out a syncer vnode.
 */
int
sync_print(v)
	void *v;
{
	struct vop_print_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	printf("syncer vnode");
	if (vp->v_vnlock != NULL)
		lockmgr_printinfo(vp->v_vnlock);
	printf("\n");
	return (0);
}
