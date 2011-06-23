/*	$NetBSD: sync_vnops.c,v 1.28.6.1 2011/06/23 14:20:24 cherry Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sync_vnops.c,v 1.28.6.1 2011/06/23 14:20:24 cherry Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/errno.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/syncfs/syncfs.h>

int (**sync_vnodeop_p)(void *);
const struct vnodeopv_entry_desc sync_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_close_desc, sync_close },		/* close */
	{ &vop_fsync_desc, sync_fsync },		/* fsync */
	{ &vop_inactive_desc, sync_inactive },		/* inactive */
	{ &vop_reclaim_desc, sync_reclaim },		/* reclaim */
	{ &vop_lock_desc, sync_lock },			/* lock */
	{ &vop_unlock_desc, sync_unlock },		/* unlock */
	{ &vop_print_desc, sync_print },		/* print */
	{ &vop_islocked_desc, sync_islocked },		/* islocked */
	{ &vop_putpages_desc, sync_putpages },		/* islocked */
	{ NULL, NULL }
};

const struct vnodeopv_desc sync_vnodeop_opv_desc =
	{ &sync_vnodeop_p, sync_vnodeop_entries };

/*
 * Return delay factor appropriate for the given file system.   For
 * WAPBL we use the sync vnode to burst out metadata updates: sync
 * those file systems more frequently.
 */
static inline int
sync_delay(struct mount *mp)
{

	return mp->mnt_wapbl != NULL ? metadelay : syncdelay;
}

/*
 * Create a new filesystem syncer vnode for the specified mount point.
 */
int
vfs_allocate_syncvnode(struct mount *mp)
{
	struct vnode *vp;
	static int start, incr, next;
	int error, vdelay;

	/* Allocate a new vnode */
	error = getnewvnode(VT_VFS, mp, sync_vnodeop_p, NULL, &vp);
	if (error) {
		return error;
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
	mutex_enter(vp->v_interlock);
	vdelay = sync_delay(mp);
	vn_syncer_add_to_worklist(vp, vdelay > 0 ? next % vdelay : 0);
	mutex_exit(vp->v_interlock);
	mp->mnt_syncer = vp;
	return (0);
}

/*
 * Destroy the filesystem syncer vnode for the specified mount point.
 */
void
vfs_deallocate_syncvnode(struct mount *mp)
{
	struct vnode *vp;

	vp = mp->mnt_syncer;
	mp->mnt_syncer = NULL;
	mutex_enter(vp->v_interlock);
	vn_syncer_remove_from_worklist(vp);
	vp->v_writecount = 0;
	mutex_exit(vp->v_interlock);
	vgone(vp);
}

/*
 * Do a lazy sync of the filesystem.
 */
int
sync_fsync(void *v)
{
	struct vop_fsync_args /* {
		struct vnode *a_vp;
		kauth_cred_t a_cred;
		int a_flags;
		off_t offlo;
		off_t offhi;
	} */ *ap = v;
	struct vnode *syncvp = ap->a_vp;
	struct mount *mp = syncvp->v_mount;

	/*
	 * We only need to do something if this is a lazy evaluation.
	 */
	if (!(ap->a_flags & FSYNC_LAZY))
		return (0);

	/*
	 * Move ourselves to the back of the sync list.
	 */
	mutex_enter(syncvp->v_interlock);
	vn_syncer_add_to_worklist(syncvp, sync_delay(mp));
	mutex_exit(syncvp->v_interlock);

	/*
	 * Walk the list of vnodes pushing all that are dirty and
	 * not already on the sync list.
	 */
	if (vfs_busy(mp, NULL) == 0) {
		VFS_SYNC(mp, MNT_LAZY, ap->a_cred);
		vfs_unbusy(mp, false, NULL);
	}
	return (0);
}

/*
 * The syncer vnode is no longer needed and is being decommissioned.
 */
int
sync_inactive(void *v)
{
	struct vop_inactive_args /* {
		struct vnode *a_vp;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	VOP_UNLOCK(vp);
	return (0);
}

int
sync_reclaim(void *v)
{

	return (0);
}

/*
 * Print out a syncer vnode.
 */
int
sync_print(void *v)
{

	printf("syncer vnode\n");
	return (0);
}
