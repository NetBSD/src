/*	$NetBSD: exfatfs_rename.c,v 1.1.2.2 2024/07/01 22:15:21 perseant Exp $	*/

/*-
 * Copyright (c) 2011, 2022 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: exfatfs_rename.c,v 1.1.2.2 2024/07/01 22:15:21 perseant Exp $");

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/kauth.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/vnode_if.h>

#include <miscfs/genfs/genfs.h>

#include <fs/exfatfs/exfatfs.h>
#include <fs/exfatfs/exfatfs_cksum.h>
#include <fs/exfatfs/exfatfs_conv.h>
#include <fs/exfatfs/exfatfs_dirent.h>
#include <fs/exfatfs/exfatfs_extern.h>
#include <fs/exfatfs/exfatfs_inode.h>
#include <fs/exfatfs/exfatfs_mount.h>
#include <fs/exfatfs/exfatfs_rename.h>
#include <fs/exfatfs/exfatfs_vnops.h>

/* #define EXFATFS_RENAME_DEBUG */

#ifdef EXFATFS_RENAME_DEBUG
# define DPRINTF(x) printf x
#else
# define DPRINTF(x) __nothing
#endif

/*
 * Forward declarations
 */

static int exfatfs_sane_rename(struct vnode *, struct componentname *,
    struct vnode *, struct componentname *,
    kauth_cred_t, bool);
static bool exfatfs_rmdired_p(struct vnode *);
static int exfatfs_gro_lock_directory(struct mount *, struct vnode *);
static void deep_copy(struct xfinode *, struct xfinode *);
#if 0
static void deep_print(const char *label, struct xfinode *xip);
#endif /* 0 */

static const struct genfs_rename_ops exfatfs_genfs_rename_ops;

/*
 * exfatfs_rename: The hairiest vop, with the insanest API.  Defer to
 * genfs_insane_rename immediately.
 */
int
exfatfs_rename(void *v)
{
#ifdef EXFATFS_RENAME_DEBUG
	struct vop_rename_args  /* {
		struct vnode *a_fdvp;
		struct vnode *a_fvp;
		struct componentname *a_fcnp;
		struct vnode *a_tdvp;
		struct vnode *a_tvp;
		struct componentname *a_tcnp;
	} */ *ap = v;
	struct vnode *fdvp = ap->a_fdvp;
	struct vnode *fvp = ap->a_fvp;
	struct vnode *tdvp = ap->a_tdvp;
	struct vnode *tvp = ap->a_tvp;
	printf("rename: fdvp %p ino 0x%lx\n", fdvp, INUM(VTOXI(fdvp)));
	printf("        fvp  %p ino 0x%lx\n", fvp,  INUM(VTOXI(fvp)));
	printf("        tdvp %p ino 0x%lx\n", tdvp, INUM(VTOXI(tdvp)));
	printf("        tvp  %p ino 0x%lx\n", tvp,
	       (tvp == NULL ? 0 : INUM(VTOXI(tvp))));
#endif	
	return genfs_insane_rename(v, &exfatfs_sane_rename);
}

/*
 * exfatfs_sane_rename: The hairiest vop, with the saner API.
 *
 * Arguments:
 *
 * . fdvp (from directory vnode),
 * . fcnp (from component name),
 * . tdvp (to directory vnode), and
 * . tcnp (to component name).
 *
 * fdvp and tdvp must be referenced and unlocked.
 */
static int
exfatfs_sane_rename(
    struct vnode *fdvp, struct componentname *fcnp,
    struct vnode *tdvp, struct componentname *tcnp,
    kauth_cred_t cred, bool posixly_correct)
{
	struct exfatfs_lookup_results fmlr, tmlr;

	return genfs_sane_rename(&exfatfs_genfs_rename_ops,
	    fdvp, fcnp, &fmlr, tdvp, tcnp, &tmlr,
	    cred, posixly_correct);
}

/*
 * exfatfs_gro_directory_empty_p: Return true if the directory vp is
 * empty.  dvp is its parent.
 *
 * vp and dvp must be locked and referenced.
 */
static bool
exfatfs_gro_directory_empty_p(struct mount *mp, kauth_cred_t cred,
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

	DPRINTF(("exfatfs_gro_directory_empty_p(%p, %p, %p, %p) = %d\n",
		 mp, cred, vp, dvp, (GET_DSE_DATALENGTH(VTOXI(vp)) == 0)));
	return (GET_DSE_DATALENGTH(VTOXI(vp)) == 0);
}

/*
 * Return a UFS-like mode for vp.
 */
static mode_t
exfatfs_vnode_mode(struct vnode *vp)
{
	struct exfatfs *fs;
	mode_t mode;

	KASSERT(vp != NULL);

	fs = VTOXI(vp)->xi_fs;
	KASSERT(fs != NULL);

	if (ISREADONLY(VTOXI(vp)))
		mode = S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;
	else
		mode = S_IRWXU|S_IRWXG|S_IRWXO;
	mode &= (vp->v_type == VDIR ? fs->xf_dirmask : fs->xf_mask);

	return mode;
}

/*
 * exfatfs_gro_rename_check_possible: Check whether renaming fvp in fdvp
 * to tvp in tdvp is possible independent of credentials.
 */
static int
exfatfs_gro_rename_check_possible(struct mount *mp,
    struct vnode *fdvp, struct vnode *fvp,
    struct vnode *tdvp, struct vnode *tvp)
{

	(void)mp;
	(void)fdvp;
	(void)fvp;
	(void)tdvp;
	(void)tvp;
	KASSERT(mp != NULL);
	KASSERT(fdvp != NULL);
	KASSERT(fvp != NULL);
	KASSERT(tdvp != NULL);
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

	/* It's always possible: no error.  */
	return 0;
}

/*
 * exfatfs_gro_rename_check_permitted: ...
 */
static int
exfatfs_gro_rename_check_permitted(struct mount *mp, kauth_cred_t cred,
    struct vnode *fdvp, struct vnode *fvp,
    struct vnode *tdvp, struct vnode *tvp)
{
	struct exfatfs *fs;

	KASSERT(mp != NULL);
	KASSERT(fdvp != NULL);
	KASSERT(fvp != NULL);
	KASSERT(tdvp != NULL);
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

	fs = MPTOXMP(mp)->xm_fs;
	KASSERT(fs != NULL);

	return genfs_ufslike_rename_check_permitted(cred,
	    fdvp, exfatfs_vnode_mode(fdvp), fs->xf_uid,
	    fvp, fs->xf_uid,
	    tdvp, exfatfs_vnode_mode(tdvp), fs->xf_uid,
	    tvp, (tvp? fs->xf_uid : 0));
}

/*
 * exfatfs_gro_remove_check_possible: ...
 */
static int
exfatfs_gro_remove_check_possible(struct mount *mp,
    struct vnode *dvp, struct vnode *vp)
{

	KASSERT(mp != NULL);
	KASSERT(dvp != NULL);
	KASSERT(vp != NULL);
	KASSERT(dvp != vp);
	KASSERT(dvp->v_mount == mp);
	KASSERT(vp->v_mount == mp);
	KASSERT(VOP_ISLOCKED(dvp) == LK_EXCLUSIVE);
	KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE);

	/* It's always possible: no error.  */
	return 0;
}

/*
 * exfatfs_gro_remove_check_permitted: ...
 */
static int
exfatfs_gro_remove_check_permitted(struct mount *mp, kauth_cred_t cred,
    struct vnode *dvp, struct vnode *vp)
{
	struct exfatfs *fs;

	KASSERT(mp != NULL);
	KASSERT(dvp != NULL);
	KASSERT(vp != NULL);
	KASSERT(dvp != vp);
	KASSERT(dvp->v_mount == mp);
	KASSERT(vp->v_mount == mp);
	KASSERT(VOP_ISLOCKED(dvp) == LK_EXCLUSIVE);
	KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE);

	fs = MPTOXMP(mp)->xm_fs;
	KASSERT(fs != NULL);

	return genfs_ufslike_remove_check_permitted(cred,
	    dvp, exfatfs_vnode_mode(dvp), fs->xf_uid, vp, fs->xf_uid);
}

/*
 * exfatfs_gro_rename: Actually perform the rename operation.
 */
static int
exfatfs_gro_rename(struct mount *mp, kauth_cred_t cred,
    struct vnode *fdvp, struct componentname *fcnp,
    void *fde, struct vnode *fvp,
    struct vnode *tdvp, struct componentname *tcnp,
    void *tde, struct vnode *tvp, nlink_t *tvp_nlinkp)
{
	struct exfatfs *fs;
#if 0
	struct exfatfs_lookup_results *fmlr = fde;
	struct exfatfs_lookup_results *tmlr = tde;
#endif /* 0 */
	struct xfinode *txip = NULL, *fxip, *tdxip, *fdxip;
	struct xfinode *ofxip = NULL;
	struct exfatfs_dirent_key new_key;
	bool directory_p, reparent_p;
	uint16_t toname[EXFATFS_NAMEMAX + 1];
	int tonamelen;
	int error;

	KASSERT(mp != NULL);
	KASSERT(fdvp != NULL);
	KASSERT(fcnp != NULL);
#if 0
	KASSERT(fmlr != NULL);
	KASSERT(tmlr != NULL);
	KASSERT(fmlr != tmlr);
#endif /* 0 */
	KASSERT(fvp != NULL);
	KASSERT(tdvp != NULL);
	KASSERT(tcnp != NULL);
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

	tdxip = VTOXI(tdvp);
	if (tvp == NULL)
		txip = NULL;
	else
		txip = VTOXI(tvp);
	fdxip = VTOXI(fdvp);
	fxip = VTOXI(fvp);
	fs = fdxip->xi_fs;

	KASSERT(tdxip != NULL);
	KASSERT(fdxip != NULL);
	KASSERT(tvp == NULL || txip != NULL);
	KASSERT(fxip != NULL);
	KASSERT(fs != NULL);

	DPRINTF(("exfatfs_gro_rename(mp=%p, cred=%p, fdvp=%p, fcnp=%p, fde=%p,"
		 " fvp=%p,\n", mp, cred, fdvp, fcnp, fde, fvp));
	DPRINTF(("    tdvp=%p, tcnp=%p, tde=%p, tvp=%p, tvp_nlinkp=%p)\n",
		 tdvp, tcnp, tde, tvp, tvp_nlinkp));

	DPRINTF((" fxip=%p, txip=%p\n", fxip, txip));

	DPRINTF((" enter with references fdvp=%d, fvp=%d, tdvp=%d, tvp=%d\n",
		 (int)vrefcnt(fdvp), (int)vrefcnt(fvp), (int)vrefcnt(tdvp),
		 (tvp == NULL ? 0 : vrefcnt(tvp))));
	
	directory_p = (fvp->v_type == VDIR);
	KASSERT(directory_p == (ISDIRECTORY(VTOXI(fvp)) != 0));
	KASSERT((tvp == NULL) || (directory_p == (tvp->v_type == VDIR)));
	KASSERT((tvp == NULL) || (directory_p ==
				  (ISDIRECTORY(VTOXI(fvp)) != 0)));

	reparent_p = (fdvp != tdvp);
	KASSERT(reparent_p == (INUM(VTOXI(fdvp)) != INUM(VTOXI(tdvp))));

	DPRINTF((" directory_p=%d reparent_p=%d\n", directory_p, reparent_p));

	if (directory_p && reparent_p)
		cache_purge(fdvp);
	
	/*
	 * Remove directory entry for the destination file.
	 * Its blocks will be removed by VOP_INACTIVE.
	 */
	if (tvp != NULL) {
		cache_purge(tdvp);
		DPRINTF(("remove txip ino 0x%lx datasize %lx start 0x%x\n",
			 INUM(txip), GET_DSE_DATALENGTH(txip),
			 GET_DSE_FIRSTCLUSTER(txip)));
		if ((error = exfatfs_deactivate(txip, true)) != 0) {
			DPRINTF((" deactivate txip returned %d\n", error));
			goto restoretvp;
		}
		DPRINTF((" now txip key %x/%x/%p\n", txip->xi_key.dk_dirclust,
			 txip->xi_key.dk_diroffset, txip->xi_key.dk_dirgen));
	}

	/*
	 * First write a new entry in the destination directory and
	 * mark the entry in the source directory as deleted.  Then
	 * move the denode to its new location in the filesystem.
	 */

	/* Remember fxip's name and location */
	ofxip = exfatfs_newxfinode(fs, 0, 0);
	memset(ofxip, 0, sizeof(*ofxip));
	deep_copy(ofxip, fxip);
	DPRINTF(("ofxip = 0x%lx\n", (unsigned long)INUM(ofxip)));
#if 0
	deep_print("ofxip", ofxip);
	deep_print("fxip", fxip);
#endif
	
	/* Set new name */
	tonamelen = exfatfs_utf8ucs2str(tcnp->cn_nameptr, tcnp->cn_namelen,
					toname, EXFATFS_NAMEMAX);
	exfatfs_set_file_name(fxip, toname, tonamelen);

#if 0
	deep_print("fxip after setname", fxip);
#endif

	/* Find space in the destination and assign location to fxip */
	if ((error = exfatfs_findempty(tdvp, fxip)) != 0)
		goto restoretvp;
	DPRINTF(("fxip = 0x%lx\n", (unsigned long)INUM(fxip)));
	KASSERT(INUM(fxip) != INUM(ofxip));
	DPRINTF((" new inum is 0x%lx\n", (unsigned long)INUM(fxip)));

	/* Write the new entry to disk */
	if ((error = exfatfs_writeback(fxip)) != 0) {
		/*
		 * Failed to write the new entry.
		 * Reactivate our working copy of fxip
		 * and restore tvp.
		 */
		DPRINTF((" writeback(fxip) returned %d\n", error));
		goto reactivateoldname;
	}

	/* Delete the old entry */
	if ((error = exfatfs_deactivate(ofxip, false)) != 0) {
		/*
		 * A file has two names.  In exFAT that means the FS is
		 * corrupt.  We failed to delete the old name so logically
		 * we should delete the new name.
		 */
		DPRINTF((" deactivate ofxip returned %d\n", error));
		goto deletenewname;
	}

	/* XXX why "if (!directory_p)"? */
	if (1 || !directory_p) {
		/* Temporarily restore old key to satisfy hash lookup */
		new_key = fxip->xi_key;
		fxip->xi_key = ofxip->xi_key;

		/* Re-key to match its new on-disk location */
		exfatfs_rekey(fvp, &new_key);
	}
	exfatfs_freexfinode(ofxip);

	if (tvp != NULL)
		*tvp_nlinkp = 0;
	genfs_rename_cache_purge(fdvp, fvp, tdvp, tvp);

	/* Switch parent vp */
	vrele(fxip->xi_parentvp);
	fxip->xi_parentvp = tdvp;
	vref(fxip->xi_parentvp);
	
	DPRINTF((" exit with references fdvp=%d, fvp=%d, tdvp=%d, tvp=%d\n",
		 (int)vrefcnt(fdvp), (int)vrefcnt(fvp), (int)vrefcnt(tdvp),
		 (tvp == NULL ? 0 : vrefcnt(tvp))));
	DPRINTF((" returning 0\n"));
	return 0;

	/*
	 * Error returns: put everything back in reverse order.
	 */
deletenewname:
	exfatfs_deactivate(fxip, false);
	
reactivateoldname:
	/* Back out changes to fxip, freeing the existing contents first */
	deep_copy(fxip, ofxip);

restoretvp:
	if (ofxip != NULL)
		exfatfs_freexfinode(ofxip);
	
	if (tvp != NULL) {
		exfatfs_activate(txip, true);
		*tvp_nlinkp = 1;
	}
	genfs_rename_cache_purge(fdvp, fvp, tdvp, tvp);
	
	DPRINTF((" errexit with references fdvp=%d, fvp=%d, tdvp=%d, tvp=%d\n",
		 (int)vrefcnt(fdvp), (int)vrefcnt(fvp), (int)vrefcnt(tdvp),
		 (tvp == NULL ? 0 : vrefcnt(tvp))));
	return error;
}

/*
 * exfatfs_gro_remove: Rename an object over another link to itself,
 * effectively removing just the original link.
 */
static int
exfatfs_gro_remove(struct mount *mp, kauth_cred_t cred,
    struct vnode *dvp, struct componentname *cnp, void *de, struct vnode *vp,
    nlink_t *tvp_nlinkp)
{
	struct exfatfs_lookup_results *mlr = de;
	int error;

	KASSERT(mp != NULL);
	KASSERT(dvp != NULL);
	KASSERT(cnp != NULL);
	KASSERT(mlr != NULL);
	KASSERT(vp != NULL);
	KASSERT(dvp != vp);
	KASSERT(dvp->v_mount == mp);
	KASSERT(vp->v_mount == mp);
	KASSERT(dvp->v_type == VDIR);
	KASSERT(vp->v_type != VDIR);
	KASSERT(VOP_ISLOCKED(dvp) == LK_EXCLUSIVE);
	KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE);

	error = exfatfs_deactivate(VTOXI(vp), true);

	*tvp_nlinkp = (error ? 1 : 0);

	return error;
}

/*
 * exfatfs_gro_lookup: Look up and save the lookup results.
 */
static int
exfatfs_gro_lookup(struct mount *mp, struct vnode *dvp,
    struct componentname *cnp, void *de_ret, struct vnode **vp_ret)
{
	struct exfatfs_lookup_results *mlr_ret = de_ret;
	struct vnode *vp;
	int error;

	(void)mp;
	KASSERT(mp != NULL);
	KASSERT(dvp != NULL);
	KASSERT(cnp != NULL);
	KASSERT(mlr_ret != NULL);
	KASSERT(vp_ret != NULL);
	KASSERT(VOP_ISLOCKED(dvp) == LK_EXCLUSIVE);

	/* Kludge cargo-culted from dholland's ufs_rename.  */
	cnp->cn_flags &=~ MODMASK;
	cnp->cn_flags |= (LOCKPARENT | LOCKLEAF);

	error = relookup(dvp, &vp, cnp, 0);
	if ((error == 0) && (vp == NULL)) {
		error = ENOENT;
		goto out;
	}
	if (error)
		return error;

	/*
	 * Thanks to VFS insanity, relookup locks vp, which screws us
	 * in various ways.
	 */
	VOP_UNLOCK(vp);

out:
	*mlr_ret = VTOXI(dvp)->xi_crap;
	*vp_ret = vp;
	return error;
}

/*
 * exfatfs_rmdired_p: Check whether the directory vp has been rmdired.
 *
 * vp must be locked and referenced.
 */
static bool
exfatfs_rmdired_p(struct vnode *vp)
{

	KASSERT(vp != NULL);
	KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE);
	KASSERT(vp->v_type == VDIR);

	return (VTOXI(vp)->xi_refcnt == 0);
}

/*
 * exfatfs_gro_genealogy: Analyze the genealogy of the source and target
 * directories.
 */
static int
exfatfs_gro_genealogy(struct mount *mp, kauth_cred_t cred,
    struct vnode *fdvp, struct vnode *tdvp,
    struct vnode **intermediate_node_ret)
{
	struct vnode *vp, *dvp, *dotdotvp;
	struct xfinode *fdxip = VTOXI(fdvp);
	int error;

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
	error = exfatfs_gro_lock_directory(mp, tdvp);
	if (error)
		return error;

	vp = tdvp;
	vref(vp);

	for (;;) {
		KASSERT(vp->v_type == VDIR);

		/* Did we hit the root without finding fdvp?  */
		if ((vp->v_vflag & VV_ROOT) != 0) {
			vput(vp);
			*intermediate_node_ret = NULL;
			return 0;
		}

		/* Look up ".." vnode */
		dotdotvp = VTOXI(vp)->xi_parentvp;
		
		/* Did we find that fdvp is an ancestor?  */
		if (INUM(fdxip) == INUM(VTOXI(dotdotvp))) {
			/* Unlock vp, but keep it referenced.  */
			VOP_UNLOCK(vp);
			*intermediate_node_ret = vp; /* as UFS */
			return 0;
		}

		/* Neither -- keep ascending the family tree.  */
		dvp = dotdotvp;
		vref(dvp); /* Keep vcache_get semantics */
		vput(vp);

		error = vn_lock(dvp, LK_EXCLUSIVE);
		if (error) {
			vrele(dvp);
			return error;
		}

		KASSERT(dvp != NULL);
		KASSERT(VOP_ISLOCKED(dvp) == LK_EXCLUSIVE);
		vp = dvp;

		KASSERT(vp->v_type == VDIR);

		if (exfatfs_rmdired_p(vp)) {
			vput(vp);
			return ENOENT;
		}
	}
}

#if 0
static void
deep_print(const char *label, struct xfinode *xip)
{
	int i;

	printf("  %s: %p\n", label, xip);
	for (i = 0; i < EXFATFS_MAXDIRENT; i++) {
		if (xip->xi_direntp[i] != NULL) {
			printf("   %d: %p\n", i, xip->xi_direntp[i]);
		}
	}
}
#endif

static void
deep_copy(struct xfinode *out, struct xfinode *in)
{
	int i;

	/* Free dirent entries before overwriting */
	for (i = 0; i < EXFATFS_MAXDIRENT; i++) {
		if (out->xi_direntp[i] != NULL) {
			exfatfs_freedirent(out->xi_direntp[i]);
			out->xi_direntp[i] = NULL;
		}
	}

	*out = *in;

	for (i = 0; i < EXFATFS_MAXDIRENT; i++) {
		if (in->xi_direntp[i] != NULL) {
			out->xi_direntp[i] = exfatfs_newdirent();
			*(out->xi_direntp[i]) = *(in->xi_direntp[i]);
		} else
			out->xi_direntp[i] = NULL;
	}
}



/*
 * exfatfs_gro_lock_directory: Lock the directory vp, but fail if it has
 * been rmdir'd.
 */
static int
exfatfs_gro_lock_directory(struct mount *mp, struct vnode *vp)
{
	int error;

	(void)mp;
	KASSERT(vp != NULL);

	error = vn_lock(vp, LK_EXCLUSIVE);
	if (error)
		return error;

	KASSERT(mp != NULL);
	KASSERT(vp->v_mount == mp);

	if (exfatfs_rmdired_p(vp)) {
		VOP_UNLOCK(vp);
		printf("exfatfs_rmdired_p[2] interpreted as ENOENT\n");
		return ENOENT;
	}

	return 0;
}

static const struct genfs_rename_ops exfatfs_genfs_rename_ops = {
	.gro_directory_empty_p		= exfatfs_gro_directory_empty_p,
	.gro_rename_check_possible	= exfatfs_gro_rename_check_possible,
	.gro_rename_check_permitted	= exfatfs_gro_rename_check_permitted,
	.gro_remove_check_possible	= exfatfs_gro_remove_check_possible,
	.gro_remove_check_permitted	= exfatfs_gro_remove_check_permitted,
	.gro_rename			= exfatfs_gro_rename,
	.gro_remove			= exfatfs_gro_remove,
	.gro_lookup			= exfatfs_gro_lookup,
	.gro_genealogy			= exfatfs_gro_genealogy,
	.gro_lock_directory		= exfatfs_gro_lock_directory,
};
