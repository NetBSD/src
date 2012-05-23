/*	$NetBSD: tmpfs_rename.c,v 1.2.2.2 2012/05/23 10:08:09 yamt Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R Campbell.
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
 * tmpfs rename
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tmpfs_rename.c,v 1.2.2.2 2012/05/23 10:08:09 yamt Exp $");

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/kauth.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <sys/vnode_if.h>

#include <miscfs/genfs/genfs.h>

#include <fs/tmpfs/tmpfs_vnops.h>
#include <fs/tmpfs/tmpfs.h>

/*
 * Forward declarations
 */

static int tmpfs_sane_rename(struct vnode *, struct componentname *,
    struct vnode *, struct componentname *,
    kauth_cred_t, bool);
static bool tmpfs_rmdired_p(struct vnode *);
static int tmpfs_gro_lock_directory(struct mount *, struct vnode *);

static const struct genfs_rename_ops tmpfs_genfs_rename_ops;

/*
 * tmpfs_sane_rename: The hairiest vop, with the saner API.
 *
 * Arguments:
 *
 * . fdvp (from directory vnode),
 * . fcnp (from component name),
 * . tdvp (to directory vnode),
 * . tcnp (to component name),
 * . cred (credentials structure), and
 * . posixly_correct (flag for behaviour if target & source link same file).
 *
 * fdvp and tdvp may be the same, and must be referenced and unlocked.
 */
static int
tmpfs_sane_rename(
    struct vnode *fdvp, struct componentname *fcnp,
    struct vnode *tdvp, struct componentname *tcnp,
    kauth_cred_t cred, bool posixly_correct)
{
	struct tmpfs_dirent *fdirent, *tdirent;

	return genfs_sane_rename(&tmpfs_genfs_rename_ops,
	    fdvp, fcnp, &fdirent, tdvp, tcnp, &tdirent,
	    cred, posixly_correct);
}

/*
 * tmpfs_rename: The hairiest vop, with the insanest API.  Defer to
 * genfs_insane_rename immediately.
 */
int
tmpfs_rename(void *v)
{

	return genfs_insane_rename(v, &tmpfs_sane_rename);
}

/*
 * tmpfs_gro_directory_empty_p: Return true if the directory vp is
 * empty.  dvp is its parent.
 *
 * vp and dvp must be locked and referenced.
 */
static bool
tmpfs_gro_directory_empty_p(struct mount *mp, kauth_cred_t cred,
    struct vnode *vp, struct vnode *dvp)
{

	(void)mp;
	(void)cred;
	(void)dvp;
	KASSERT(mp != NULL);
	KASSERT(vp != NULL);
	KASSERT(dvp != NULL);
	KASSERT(vp != dvp);
	KASSERT(vp->v_mount == mp);
	KASSERT(dvp->v_mount == mp);
	KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE);
	KASSERT(VOP_ISLOCKED(dvp) == LK_EXCLUSIVE);

	return (VP_TO_TMPFS_NODE(vp)->tn_size == 0);
}

/*
 * tmpfs_gro_rename_check_possible: Check whether a rename is possible
 * independent of credentials.
 */
static int
tmpfs_gro_rename_check_possible(struct mount *mp,
    struct vnode *fdvp, struct vnode *fvp,
    struct vnode *tdvp, struct vnode *tvp)
{

	(void)mp;
	KASSERT(mp != NULL);
	KASSERT(fdvp != NULL);
	KASSERT(fvp != NULL);
	KASSERT(tdvp != NULL);
	KASSERT(fdvp != fvp);
	KASSERT(fdvp != tvp);
	KASSERT(tdvp != fvp);
	KASSERT(tdvp != tvp);
	KASSERT(fvp != tvp);
	KASSERT(fdvp->v_type == VDIR);
	KASSERT(tdvp->v_type == VDIR);
	KASSERT(fdvp->v_mount == mp);
	KASSERT(fvp->v_mount == mp);
	KASSERT(tdvp->v_mount == mp);
	KASSERT((tvp == NULL) || (tvp->v_mount == mp));
	KASSERT(VOP_ISLOCKED(fdvp) == LK_EXCLUSIVE);
	KASSERT(VOP_ISLOCKED(fvp) == LK_EXCLUSIVE);
	KASSERT(VOP_ISLOCKED(tdvp) == LK_EXCLUSIVE);
	KASSERT((tvp == NULL) || (VOP_ISLOCKED(tvp) == LK_EXCLUSIVE));

	return genfs_ufslike_rename_check_possible(
	    VP_TO_TMPFS_NODE(fdvp)->tn_flags, VP_TO_TMPFS_NODE(fvp)->tn_flags,
	    VP_TO_TMPFS_NODE(tdvp)->tn_flags,
	    (tvp? VP_TO_TMPFS_NODE(tvp)->tn_flags : 0), (tvp != NULL),
	    IMMUTABLE, APPEND);
}

/*
 * tmpfs_gro_rename_check_permitted: Check whether a rename is
 * permitted given our credentials.
 */
static int
tmpfs_gro_rename_check_permitted(struct mount *mp, kauth_cred_t cred,
    struct vnode *fdvp, struct vnode *fvp,
    struct vnode *tdvp, struct vnode *tvp)
{

	(void)mp;
	KASSERT(mp != NULL);
	KASSERT(fdvp != NULL);
	KASSERT(fvp != NULL);
	KASSERT(tdvp != NULL);
	KASSERT(fdvp != fvp);
	KASSERT(fdvp != tvp);
	KASSERT(tdvp != fvp);
	KASSERT(tdvp != tvp);
	KASSERT(fvp != tvp);
	KASSERT(fdvp->v_type == VDIR);
	KASSERT(tdvp->v_type == VDIR);
	KASSERT(fdvp->v_mount == mp);
	KASSERT(fvp->v_mount == mp);
	KASSERT(tdvp->v_mount == mp);
	KASSERT((tvp == NULL) || (tvp->v_mount == mp));
	KASSERT(VOP_ISLOCKED(fdvp) == LK_EXCLUSIVE);
	KASSERT(VOP_ISLOCKED(fvp) == LK_EXCLUSIVE);
	KASSERT(VOP_ISLOCKED(tdvp) == LK_EXCLUSIVE);
	KASSERT((tvp == NULL) || (VOP_ISLOCKED(tvp) == LK_EXCLUSIVE));

	return genfs_ufslike_rename_check_permitted(cred,
	    fdvp, VP_TO_TMPFS_NODE(fdvp)->tn_mode,
	    VP_TO_TMPFS_NODE(fdvp)->tn_uid,
	    fvp, VP_TO_TMPFS_NODE(fvp)->tn_uid,
	    tdvp, VP_TO_TMPFS_NODE(tdvp)->tn_mode,
	    VP_TO_TMPFS_NODE(tdvp)->tn_uid,
	    tvp, (tvp? VP_TO_TMPFS_NODE(tvp)->tn_uid : 0));
}

/*
 * tmpfs_gro_remove_check_possible: Check whether a remove is possible
 * independent of credentials.
 */
static int
tmpfs_gro_remove_check_possible(struct mount *mp,
    struct vnode *dvp, struct vnode *vp)
{

	(void)mp;
	KASSERT(mp != NULL);
	KASSERT(dvp != NULL);
	KASSERT(vp != NULL);
	KASSERT(dvp != vp);
	KASSERT(dvp->v_type == VDIR);
	KASSERT(vp->v_type != VDIR);
	KASSERT(dvp->v_mount == mp);
	KASSERT(vp->v_mount == mp);
	KASSERT(VOP_ISLOCKED(dvp) == LK_EXCLUSIVE);
	KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE);

	return genfs_ufslike_remove_check_possible(
	    VP_TO_TMPFS_NODE(dvp)->tn_flags, VP_TO_TMPFS_NODE(vp)->tn_flags,
	    IMMUTABLE, APPEND);
}

/*
 * tmpfs_gro_remove_check_permitted: Check whether a remove is
 * permitted given our credentials.
 */
static int
tmpfs_gro_remove_check_permitted(struct mount *mp, kauth_cred_t cred,
    struct vnode *dvp, struct vnode *vp)
{

	(void)mp;
	KASSERT(mp != NULL);
	KASSERT(dvp != NULL);
	KASSERT(vp != NULL);
	KASSERT(dvp != vp);
	KASSERT(dvp->v_type == VDIR);
	KASSERT(vp->v_type != VDIR);
	KASSERT(dvp->v_mount == mp);
	KASSERT(vp->v_mount == mp);
	KASSERT(VOP_ISLOCKED(dvp) == LK_EXCLUSIVE);
	KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE);

	return genfs_ufslike_remove_check_permitted(cred,
	    dvp, VP_TO_TMPFS_NODE(dvp)->tn_mode, VP_TO_TMPFS_NODE(dvp)->tn_uid,
	    vp, VP_TO_TMPFS_NODE(vp)->tn_uid);
}

/*
 * tmpfs_gro_rename: Actually perform the rename operation.
 */
static int
tmpfs_gro_rename(struct mount *mp, kauth_cred_t cred,
    struct vnode *fdvp, struct componentname *fcnp,
    void *fde, struct vnode *fvp,
    struct vnode *tdvp, struct componentname *tcnp,
    void *tde, struct vnode *tvp)
{
	struct tmpfs_dirent **fdep = fde;
	struct tmpfs_dirent **tdep = tde;
	char *newname;

	(void)cred;
	KASSERT(mp != NULL);
	KASSERT(fdvp != NULL);
	KASSERT(fcnp != NULL);
	KASSERT(fdep != NULL);
	KASSERT(fvp != NULL);
	KASSERT(tdvp != NULL);
	KASSERT(tcnp != NULL);
	KASSERT(tdep != NULL);
	KASSERT(fdep != tdep);
	KASSERT((*fdep) != (*tdep));
	KASSERT((*fdep) != NULL);
	KASSERT((*fdep)->td_node == VP_TO_TMPFS_NODE(fvp));
	KASSERT((tvp == NULL) || ((*tdep) != NULL));
	KASSERT((tvp == NULL) || ((*tdep)->td_node == VP_TO_TMPFS_NODE(tvp)));
	KASSERT(fdvp != fvp);
	KASSERT(fdvp != tvp);
	KASSERT(tdvp != fvp);
	KASSERT(tdvp != tvp);
	KASSERT(fvp != tvp);
	KASSERT(fdvp->v_mount == mp);
	KASSERT(fvp->v_mount == mp);
	KASSERT(tdvp->v_mount == mp);
	KASSERT((tvp == NULL) || (tvp->v_mount == mp));
	KASSERT(VOP_ISLOCKED(fdvp) == LK_EXCLUSIVE);
	KASSERT(VOP_ISLOCKED(fvp) == LK_EXCLUSIVE);
	KASSERT(VOP_ISLOCKED(tdvp) == LK_EXCLUSIVE);
	KASSERT((tvp == NULL) || (VOP_ISLOCKED(tvp) == LK_EXCLUSIVE));

	if (tmpfs_strname_neqlen(fcnp, tcnp)) {
		newname = tmpfs_strname_alloc(VFS_TO_TMPFS(mp),
		    tcnp->cn_namelen);
		if (newname == NULL)
			return ENOSPC;
	} else {
		newname = NULL;
	}

	/*
	 * If we are moving from one directory to another, detach the
	 * source entry and reattach it to the target directory.
	 */
	if (fdvp != tdvp) {
		tmpfs_dir_detach(fdvp, *fdep);
		tmpfs_dir_attach(tdvp, *fdep, VP_TO_TMPFS_NODE(fvp));
	} else if (tvp == NULL) {
		/*
		 * We are changing the directory.  tmpfs_dir_attach and
		 * tmpfs_dir_detach note the events for us, but for
		 * this case we don't call them, so we must note the
		 * event explicitly.
		 */
		VN_KNOTE(fdvp, NOTE_WRITE);
	}

	/*
	 * If we are replacing an existing target entry, delete it.
	 *
	 * XXX What if the target is a directory with whiteout entries?
	 */
	if (tvp != NULL) {
		KASSERT((*tdep) != NULL);
		KASSERT((*tdep)->td_node == VP_TO_TMPFS_NODE(tvp));
		KASSERT((fvp->v_type == VDIR) == (tvp->v_type == VDIR));
		if (tvp->v_type == VDIR) {
			KASSERT(VP_TO_TMPFS_NODE(tvp)->tn_size == 0);
			KASSERT(VP_TO_TMPFS_NODE(tvp)->tn_links == 2);

			/*
			 * Decrement the extra link count for `.' so
			 * the vnode will be recycled when released.
			 *
			 * XXX Why not just release directory vnodes
			 * when their link count is 1 instead of 0 in
			 * tmpfs_inactive, since `.' is being treated
			 * specially anyway?
			 */
			VP_TO_TMPFS_NODE(tvp)->tn_links--;
		}
		tmpfs_dir_detach(tdvp, *tdep);
		tmpfs_free_dirent(VFS_TO_TMPFS(mp), *tdep);
	}

	/*
	 * Update the directory entry's name if necessary, and flag
	 * metadata updates.  A memory allocation failure here is not
	 * OK because we've already committed some changes that we
	 * can't back out at this point, hence the early allocation
	 * above.
	 */
	if (newname != NULL) {
		KASSERT(tcnp->cn_namelen <= TMPFS_MAXNAMLEN);

		tmpfs_strname_free(VFS_TO_TMPFS(mp), (*fdep)->td_name,
		    (*fdep)->td_namelen);
		(*fdep)->td_namelen = (uint16_t)tcnp->cn_namelen;
		(void)memcpy(newname, tcnp->cn_nameptr, tcnp->cn_namelen);
		(*fdep)->td_name = newname;

		VP_TO_TMPFS_NODE(fvp)->tn_status |= TMPFS_NODE_CHANGED;
		VP_TO_TMPFS_NODE(tdvp)->tn_status |= TMPFS_NODE_MODIFIED;
	}

	VN_KNOTE(fvp, NOTE_RENAME);

#if 0				/* XXX */
	genfs_rename_cache_purge(fdvp, fvp, tdvp, tvp);
#endif

	return 0;
}

/*
 * tmpfs_gro_remove: Rename an object over another link to itself,
 * effectively removing just the original link.
 */
static int
tmpfs_gro_remove(struct mount *mp, kauth_cred_t cred,
    struct vnode *dvp, struct componentname *cnp, void *de, struct vnode *vp)
{
	struct tmpfs_dirent **dep = de;

	(void)vp;
	KASSERT(mp != NULL);
	KASSERT(dvp != NULL);
	KASSERT(cnp != NULL);
	KASSERT(dep != NULL);
	KASSERT(vp != NULL);
	KASSERT(dvp != vp);
	KASSERT(dvp->v_mount == mp);
	KASSERT(vp->v_mount == mp);
	KASSERT(dvp->v_type == VDIR);
	KASSERT(vp->v_type != VDIR);
	KASSERT(VOP_ISLOCKED(dvp) == LK_EXCLUSIVE);
	KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE);

	tmpfs_dir_detach(dvp, *dep);
	tmpfs_free_dirent(VFS_TO_TMPFS(mp), *dep);

	return 0;
}

/*
 * tmpfs_gro_lookup: Look up and save the lookup results.
 */
static int
tmpfs_gro_lookup(struct mount *mp, struct vnode *dvp,
    struct componentname *cnp, void *de_ret, struct vnode **vp_ret)
{
	struct tmpfs_dirent *dirent, **dep_ret = de_ret;
	struct vnode *vp;
	int error;

	(void)mp;
	KASSERT(mp != NULL);
	KASSERT(dvp != NULL);
	KASSERT(cnp != NULL);
	KASSERT(dep_ret != NULL);
	KASSERT(vp_ret != NULL);
	KASSERT(VOP_ISLOCKED(dvp) == LK_EXCLUSIVE);

	dirent = tmpfs_dir_lookup(VP_TO_TMPFS_NODE(dvp), cnp);
	if (dirent == NULL)
		return ENOENT;

	mutex_enter(&dirent->td_node->tn_vlock);
	error = tmpfs_vnode_get(mp, dirent->td_node, &vp);
	/* Note: tmpfs_vnode_get always releases dirent->td_node->tn_vlock.  */
	if (error)
		return error;

	KASSERT(vp != NULL);
	KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE);

	/*
	 * tmpfs_vnode_get returns this locked for us, which would be
	 * convenient but for lossage in other file systems.  So, for
	 * hysterical raisins, we have to unlock it here.  The caller
	 * will lock it again, and we don't rely on any interesting
	 * invariants in the interim, beyond that it won't vanish from
	 * under us, which it won't because it's referenced.
	 *
	 * XXX Once namei is fixed, we can change the genfs_rename
	 * protocol so that this unlock is not necessary.
	 */
	VOP_UNLOCK(vp);

	*dep_ret = dirent;
	*vp_ret = vp;
	return 0;
}

/*
 * tmpfs_rmdired_p: Check whether the directory vp has been rmdired.
 *
 * vp must be locked and referenced.
 */
static bool
tmpfs_rmdired_p(struct vnode *vp)
{

	KASSERT(vp != NULL);
	KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE);
	KASSERT(vp->v_type == VDIR);

	return (VP_TO_TMPFS_NODE(vp)->tn_spec.tn_dir.tn_parent == NULL);
}

/*
 * tmpfs_gro_genealogy: Analyze the genealogy of the source and target
 * directories.
 */
static int
tmpfs_gro_genealogy(struct mount *mp, kauth_cred_t cred,
    struct vnode *fdvp, struct vnode *tdvp,
    struct vnode **intermediate_node_ret)
{
	struct vnode *vp;
	struct tmpfs_node *dnode;
	int error;

	(void)cred;
	KASSERT(mp != NULL);
	KASSERT(fdvp != NULL);
	KASSERT(tdvp != NULL);
	KASSERT(fdvp != tdvp);
	KASSERT(intermediate_node_ret != NULL);
	KASSERT(fdvp->v_mount == mp);
	KASSERT(tdvp->v_mount == mp);
	KASSERT(fdvp->v_type == VDIR);
	KASSERT(tdvp->v_type == VDIR);

	/*
	 * We need to provisionally lock tdvp to keep rmdir from
	 * deleting it -- or any ancestor -- at an inopportune moment.
	 */
	error = tmpfs_gro_lock_directory(mp, tdvp);
	if (error)
		return error;

	vp = tdvp;
	vref(vp);

	for (;;) {
		KASSERT(vp != NULL);
		KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE);
		KASSERT(vp->v_mount == mp);
		KASSERT(vp->v_type == VDIR);
		KASSERT(!tmpfs_rmdired_p(vp));

		dnode = VP_TO_TMPFS_NODE(vp)->tn_spec.tn_dir.tn_parent;

		/*
		 * If dnode is null then vp has been rmdir'd, which is
		 * not supposed to happen because we have it locked.
		 */
		KASSERT(dnode != NULL);

		/* Did we hit the root without finding fdvp?  */
		if (dnode == VP_TO_TMPFS_NODE(vp)) {
			vput(vp);
			*intermediate_node_ret = NULL;
			return 0;
		}

		/* Did we find that fdvp is an ancestor of tdvp? */
		if (dnode == VP_TO_TMPFS_NODE(fdvp)) {
			KASSERT(dnode->tn_vnode == fdvp);
			/* Unlock vp, but keep it referenced.  */
			VOP_UNLOCK(vp);
			*intermediate_node_ret = vp;
			return 0;
		}

		/* Neither -- keep ascending the family tree.  */
		mutex_enter(&dnode->tn_vlock);
		vput(vp);
		vp = NULL;	/* Just in case, for the kassert above...  */
		error = tmpfs_vnode_get(mp, dnode, &vp);
		if (error)
			return error;
	}
}

/*
 * tmpfs_gro_lock_directory: Lock the directory vp, but fail if it has
 * been rmdir'd.
 */
static int
tmpfs_gro_lock_directory(struct mount *mp, struct vnode *vp)
{

	(void)mp;
	KASSERT(mp != NULL);
	KASSERT(vp != NULL);
	KASSERT(vp->v_mount == mp);

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);

	if (tmpfs_rmdired_p(vp)) {
		VOP_UNLOCK(vp);
		return ENOENT;
	}

	return 0;
}

static const struct genfs_rename_ops tmpfs_genfs_rename_ops = {
	.gro_directory_empty_p		= tmpfs_gro_directory_empty_p,
	.gro_rename_check_possible	= tmpfs_gro_rename_check_possible,
	.gro_rename_check_permitted	= tmpfs_gro_rename_check_permitted,
	.gro_remove_check_possible	= tmpfs_gro_remove_check_possible,
	.gro_remove_check_permitted	= tmpfs_gro_remove_check_permitted,
	.gro_rename			= tmpfs_gro_rename,
	.gro_remove			= tmpfs_gro_remove,
	.gro_lookup			= tmpfs_gro_lookup,
	.gro_genealogy			= tmpfs_gro_genealogy,
	.gro_lock_directory		= tmpfs_gro_lock_directory,
};
