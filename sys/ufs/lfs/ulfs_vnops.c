/*	$NetBSD: ulfs_vnops.c,v 1.52 2017/10/28 00:37:13 pgoyette Exp $	*/
/*  from NetBSD: ufs_vnops.c,v 1.232 2016/05/19 18:32:03 riastradh Exp  */

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wasabi Systems, Inc.
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
 * Copyright (c) 1982, 1986, 1989, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ufs_vnops.c	8.28 (Berkeley) 7/31/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ulfs_vnops.c,v 1.52 2017/10/28 00:37:13 pgoyette Exp $");

#if defined(_KERNEL_OPT)
#include "opt_lfs.h"
#include "opt_quota.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/resourcevar.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/kmem.h>
#include <sys/malloc.h>
#include <sys/dirent.h>
#include <sys/lockf.h>
#include <sys/kauth.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/fifofs/fifo.h>
#include <miscfs/genfs/genfs.h>

#include <ufs/lfs/lfs_extern.h>
#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_accessors.h>

#include <ufs/lfs/ulfs_inode.h>
#include <ufs/lfs/ulfsmount.h>
#include <ufs/lfs/ulfs_bswap.h>
#include <ufs/lfs/ulfs_extern.h>
#ifdef LFS_DIRHASH
#include <ufs/lfs/ulfs_dirhash.h>
#endif

#include <uvm/uvm.h>

static int ulfs_chmod(struct vnode *, int, kauth_cred_t, struct lwp *);
static int ulfs_chown(struct vnode *, uid_t, gid_t, kauth_cred_t,
    struct lwp *);

/*
 * Open called.
 *
 * Nothing to do.
 */
/* ARGSUSED */
int
ulfs_open(void *v)
{
	struct vop_open_args /* {
		struct vnode	*a_vp;
		int		a_mode;
		kauth_cred_t	a_cred;
	} */ *ap = v;

	KASSERT(VOP_ISLOCKED(ap->a_vp) == LK_EXCLUSIVE);

	/*
	 * Files marked append-only must be opened for appending.
	 */
	if ((VTOI(ap->a_vp)->i_flags & APPEND) &&
	    (ap->a_mode & (FWRITE | O_APPEND)) == FWRITE)
		return (EPERM);
	return (0);
}

static int
ulfs_check_possible(struct vnode *vp, struct inode *ip, mode_t mode,
    kauth_cred_t cred)
{
#if defined(LFS_QUOTA) || defined(LFS_QUOTA2)
	int error;
#endif

	/*
	 * Disallow write attempts on read-only file systems;
	 * unless the file is a socket, fifo, or a block or
	 * character device resident on the file system.
	 */
	if (mode & VWRITE) {
		switch (vp->v_type) {
		case VDIR:
		case VLNK:
		case VREG:
			if (vp->v_mount->mnt_flag & MNT_RDONLY)
				return (EROFS);
#if defined(LFS_QUOTA) || defined(LFS_QUOTA2)
			error = lfs_chkdq(ip, 0, cred, 0);
			if (error != 0)
				return error;
#endif
			break;
		case VBAD:
		case VBLK:
		case VCHR:
		case VSOCK:
		case VFIFO:
		case VNON:
		default:
			break;
		}
	}

	/* If it is a snapshot, nobody gets access to it. */
	if ((ip->i_flags & SF_SNAPSHOT))
		return (EPERM);
	/* If immutable bit set, nobody gets to write it. */
	if ((mode & VWRITE) && (ip->i_flags & IMMUTABLE))
		return (EPERM);

	return 0;
}

static int
ulfs_check_permitted(struct vnode *vp, struct inode *ip, mode_t mode,
    kauth_cred_t cred)
{

	return kauth_authorize_vnode(cred, KAUTH_ACCESS_ACTION(mode, vp->v_type,
	    ip->i_mode & ALLPERMS), vp, NULL, genfs_can_access(vp->v_type,
	    ip->i_mode & ALLPERMS, ip->i_uid, ip->i_gid, mode, cred));
}

int
ulfs_access(void *v)
{
	struct vop_access_args /* {
		struct vnode	*a_vp;
		int		a_mode;
		kauth_cred_t	a_cred;
	} */ *ap = v;
	struct vnode	*vp;
	struct inode	*ip;
	mode_t		mode;
	int		error;

	vp = ap->a_vp;
	mode = ap->a_mode;

	KASSERT(VOP_ISLOCKED(vp));

	ip = VTOI(vp);

	error = ulfs_check_possible(vp, ip, mode, ap->a_cred);
	if (error)
		return error;

	error = ulfs_check_permitted(vp, ip, mode, ap->a_cred);

	return error;
}

/*
 * Set attribute vnode op. called from several syscalls
 */
int
ulfs_setattr(void *v)
{
	struct vop_setattr_args /* {
		struct vnode	*a_vp;
		struct vattr	*a_vap;
		kauth_cred_t	a_cred;
	} */ *ap = v;
	struct vattr	*vap;
	struct vnode	*vp;
	struct inode	*ip;
	struct lfs	*fs;
	kauth_cred_t	cred;
	struct lwp	*l;
	int		error;
	kauth_action_t	action;
	bool		changing_sysflags;

	vap = ap->a_vap;
	vp = ap->a_vp;
	cred = ap->a_cred;
	l = curlwp;
	action = KAUTH_VNODE_WRITE_FLAGS;
	changing_sysflags = false;

	KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE);

	ip = VTOI(vp);
	fs = ip->i_lfs;

	/*
	 * Check for unsettable attributes.
	 */
	if ((vap->va_type != VNON) || (vap->va_nlink != VNOVAL) ||
	    (vap->va_fsid != VNOVAL) || (vap->va_fileid != VNOVAL) ||
	    (vap->va_blocksize != VNOVAL) || (vap->va_rdev != VNOVAL) ||
	    ((int)vap->va_bytes != VNOVAL) || (vap->va_gen != VNOVAL)) {
		return (EINVAL);
	}

	if (vap->va_flags != VNOVAL) {
		if (vp->v_mount->mnt_flag & MNT_RDONLY) {
			error = EROFS;
			goto out;
		}

		/* Snapshot flag cannot be set or cleared */
		if ((vap->va_flags & (SF_SNAPSHOT | SF_SNAPINVAL)) !=
		    (ip->i_flags & (SF_SNAPSHOT | SF_SNAPINVAL))) {
			error = EPERM;
			goto out;
		}

		if (ip->i_flags & (SF_IMMUTABLE | SF_APPEND)) {
			action |= KAUTH_VNODE_HAS_SYSFLAGS;
		}

		if ((vap->va_flags & SF_SETTABLE) !=
		    (ip->i_flags & SF_SETTABLE)) {
			action |= KAUTH_VNODE_WRITE_SYSFLAGS;
			changing_sysflags = true;
		}

		error = kauth_authorize_vnode(cred, action, vp, NULL,
		    genfs_can_chflags(cred, vp->v_type, ip->i_uid,
		    changing_sysflags));
		if (error)
			goto out;

		if (changing_sysflags) {
			ip->i_flags = vap->va_flags;
			DIP_ASSIGN(ip, flags, ip->i_flags);
		} else {
			ip->i_flags &= SF_SETTABLE;
			ip->i_flags |= (vap->va_flags & UF_SETTABLE);
			DIP_ASSIGN(ip, flags, ip->i_flags);
		}
		ip->i_state |= IN_CHANGE;
		if (vap->va_flags & (IMMUTABLE | APPEND)) {
			error = 0;
			goto out;
		}
	}
	if (ip->i_flags & (IMMUTABLE | APPEND)) {
		error = EPERM;
		goto out;
	}
	/*
	 * Go through the fields and update iff not VNOVAL.
	 */
	if (vap->va_uid != (uid_t)VNOVAL || vap->va_gid != (gid_t)VNOVAL) {
		if (vp->v_mount->mnt_flag & MNT_RDONLY) {
			error = EROFS;
			goto out;
		}
		error = ulfs_chown(vp, vap->va_uid, vap->va_gid, cred, l);
		if (error)
			goto out;
	}
	if (vap->va_size != VNOVAL) {
		/*
		 * Disallow write attempts on read-only file systems;
		 * unless the file is a socket, fifo, or a block or
		 * character device resident on the file system.
		 */
		switch (vp->v_type) {
		case VDIR:
			error = EISDIR;
			goto out;
		case VCHR:
		case VBLK:
		case VFIFO:
			break;
		case VREG:
			if (vp->v_mount->mnt_flag & MNT_RDONLY) {
				error = EROFS;
				goto out;
			}
			if ((ip->i_flags & SF_SNAPSHOT) != 0) {
				error = EPERM;
				goto out;
			}
			error = lfs_truncate(vp, vap->va_size, 0, cred);
			if (error)
				goto out;
			break;
		default:
			error = EOPNOTSUPP;
			goto out;
		}
	}
	ip = VTOI(vp);
	if (vap->va_atime.tv_sec != VNOVAL || vap->va_mtime.tv_sec != VNOVAL ||
	    vap->va_birthtime.tv_sec != VNOVAL) {
		if (vp->v_mount->mnt_flag & MNT_RDONLY) {
			error = EROFS;
			goto out;
		}
		if ((ip->i_flags & SF_SNAPSHOT) != 0) {
			error = EPERM;
			goto out;
		}
		error = kauth_authorize_vnode(cred, KAUTH_VNODE_WRITE_TIMES, vp,
		    NULL, genfs_can_chtimes(vp, vap->va_vaflags, ip->i_uid, cred));
		if (error)
			goto out;
		if (vap->va_atime.tv_sec != VNOVAL)
			if (!(vp->v_mount->mnt_flag & MNT_NOATIME))
				ip->i_state |= IN_ACCESS;
		if (vap->va_mtime.tv_sec != VNOVAL) {
			ip->i_state |= IN_CHANGE | IN_UPDATE;
			if (vp->v_mount->mnt_flag & MNT_RELATIME)
				ip->i_state |= IN_ACCESS;
		}
		if (vap->va_birthtime.tv_sec != VNOVAL) {
			lfs_dino_setbirthtime(fs, ip->i_din,
					      &vap->va_birthtime);
		}
		error = lfs_update(vp, &vap->va_atime, &vap->va_mtime, 0);
		if (error)
			goto out;
	}
	error = 0;
	if (vap->va_mode != (mode_t)VNOVAL) {
		if (vp->v_mount->mnt_flag & MNT_RDONLY) {
			error = EROFS;
			goto out;
		}
		if ((ip->i_flags & SF_SNAPSHOT) != 0 &&
		    (vap->va_mode & (S_IXUSR | S_IWUSR | S_IXGRP | S_IWGRP |
		     S_IXOTH | S_IWOTH))) {
			error = EPERM;
			goto out;
		}
		error = ulfs_chmod(vp, (int)vap->va_mode, cred, l);
	}
	VN_KNOTE(vp, NOTE_ATTRIB);
out:
	return (error);
}

/*
 * Change the mode on a file.
 * Inode must be locked before calling.
 */
static int
ulfs_chmod(struct vnode *vp, int mode, kauth_cred_t cred, struct lwp *l)
{
	struct inode	*ip;
	int		error;

	KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE);

	ip = VTOI(vp);

	error = kauth_authorize_vnode(cred, KAUTH_VNODE_WRITE_SECURITY, vp,
	    NULL, genfs_can_chmod(vp->v_type, cred, ip->i_uid, ip->i_gid, mode));
	if (error)
		return (error);

	ip->i_mode &= ~ALLPERMS;
	ip->i_mode |= (mode & ALLPERMS);
	ip->i_state |= IN_CHANGE;
	DIP_ASSIGN(ip, mode, ip->i_mode);
	return (0);
}

/*
 * Perform chown operation on inode ip;
 * inode must be locked prior to call.
 */
static int
ulfs_chown(struct vnode *vp, uid_t uid, gid_t gid, kauth_cred_t cred,
    	struct lwp *l)
{
	struct inode	*ip;
	int		error = 0;
#if defined(LFS_QUOTA) || defined(LFS_QUOTA2)
	uid_t		ouid;
	gid_t		ogid;
	int64_t		change;
#endif

	KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE);

	ip = VTOI(vp);
	error = 0;

	if (uid == (uid_t)VNOVAL)
		uid = ip->i_uid;
	if (gid == (gid_t)VNOVAL)
		gid = ip->i_gid;

	error = kauth_authorize_vnode(cred, KAUTH_VNODE_CHANGE_OWNERSHIP, vp,
	    NULL, genfs_can_chown(cred, ip->i_uid, ip->i_gid, uid, gid));
	if (error)
		return (error);

#if defined(LFS_QUOTA) || defined(LFS_QUOTA2)
	ogid = ip->i_gid;
	ouid = ip->i_uid;
	change = DIP(ip, blocks);
	(void) lfs_chkdq(ip, -change, cred, 0);
	(void) lfs_chkiq(ip, -1, cred, 0);
#endif
	ip->i_gid = gid;
	DIP_ASSIGN(ip, gid, gid);
	ip->i_uid = uid;
	DIP_ASSIGN(ip, uid, uid);
#if defined(LFS_QUOTA) || defined(LFS_QUOTA2)
	if ((error = lfs_chkdq(ip, change, cred, 0)) == 0) {
		if ((error = lfs_chkiq(ip, 1, cred, 0)) == 0)
			goto good;
		else
			(void) lfs_chkdq(ip, -change, cred, FORCE);
	}
	ip->i_gid = ogid;
	DIP_ASSIGN(ip, gid, ogid);
	ip->i_uid = ouid;
	DIP_ASSIGN(ip, uid, ouid);
	(void) lfs_chkdq(ip, change, cred, FORCE);
	(void) lfs_chkiq(ip, 1, cred, FORCE);
	return (error);
 good:
#endif /* LFS_QUOTA || LFS_QUOTA2 */
	ip->i_state |= IN_CHANGE;
	return (0);
}

int
ulfs_remove(void *v)
{
	struct vop_remove_v2_args /* {
		struct vnode		*a_dvp;
		struct vnode		*a_vp;
		struct componentname	*a_cnp;
	} */ *ap = v;
	struct vnode	*vp, *dvp;
	struct inode	*ip;
	int		error;
	struct ulfs_lookup_results *ulr;

	dvp = ap->a_dvp;
	vp = ap->a_vp;

	KASSERT(VOP_ISLOCKED(dvp) == LK_EXCLUSIVE);
	KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE);
	KASSERT(dvp->v_mount == vp->v_mount);

	ip = VTOI(vp);

	/* XXX should handle this material another way */
	ulr = &VTOI(dvp)->i_crap;
	ULFS_CHECK_CRAPCOUNTER(VTOI(dvp));

	if (vp->v_type == VDIR || (ip->i_flags & (IMMUTABLE | APPEND)) ||
	    (VTOI(dvp)->i_flags & APPEND))
		error = EPERM;
	else {
		error = ulfs_dirremove(dvp, ulr,
				      ip, ap->a_cnp->cn_flags, 0);
	}
	VN_KNOTE(vp, NOTE_DELETE);
	VN_KNOTE(dvp, NOTE_WRITE);
	if (dvp == vp)
		vrele(vp);
	else
		vput(vp);
	return (error);
}

/*
 * ulfs_link: create hard link.
 */
int
ulfs_link(void *v)
{
	struct vop_link_v2_args /* {
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap = v;
	struct vnode *dvp = ap->a_dvp;
	struct vnode *vp = ap->a_vp;
	struct componentname *cnp = ap->a_cnp;
	struct inode *ip;
	int error;
	struct ulfs_lookup_results *ulr;

	KASSERT(VOP_ISLOCKED(dvp) == LK_EXCLUSIVE);
	KASSERT(dvp != vp);
	KASSERT(vp->v_type != VDIR);

	/* XXX should handle this material another way */
	ulr = &VTOI(dvp)->i_crap;
	ULFS_CHECK_CRAPCOUNTER(VTOI(dvp));

	error = vn_lock(vp, LK_EXCLUSIVE);
	if (error) {
		VOP_ABORTOP(dvp, cnp);
		goto out2;
	}
	if (vp->v_mount != dvp->v_mount) {
		error = ENOENT;
		VOP_ABORTOP(dvp, cnp);
		goto out2;
	}
	ip = VTOI(vp);
	if ((nlink_t)ip->i_nlink >= LINK_MAX) {
		VOP_ABORTOP(dvp, cnp);
		error = EMLINK;
		goto out1;
	}
	if (ip->i_flags & (IMMUTABLE | APPEND)) {
		VOP_ABORTOP(dvp, cnp);
		error = EPERM;
		goto out1;
	}
	ip->i_nlink++;
	DIP_ASSIGN(ip, nlink, ip->i_nlink);
	ip->i_state |= IN_CHANGE;
	error = lfs_update(vp, NULL, NULL, UPDATE_DIROP);
	if (!error) {
		error = ulfs_direnter(dvp, ulr, vp,
				      cnp, ip->i_number, LFS_IFTODT(ip->i_mode), NULL);
	}
	if (error) {
		ip->i_nlink--;
		DIP_ASSIGN(ip, nlink, ip->i_nlink);
		ip->i_state |= IN_CHANGE;
	}
 out1:
	VOP_UNLOCK(vp);
 out2:
	VN_KNOTE(vp, NOTE_LINK);
	VN_KNOTE(dvp, NOTE_WRITE);
	return (error);
}

/*
 * whiteout vnode call
 */
int
ulfs_whiteout(void *v)
{
	struct vop_whiteout_args /* {
		struct vnode		*a_dvp;
		struct componentname	*a_cnp;
		int			a_flags;
	} */ *ap = v;
	struct vnode		*dvp = ap->a_dvp;
	struct componentname	*cnp = ap->a_cnp;
	int			error;
	struct ulfsmount	*ump = VFSTOULFS(dvp->v_mount);
	struct lfs *fs = ump->um_lfs;
	struct ulfs_lookup_results *ulr;

	KASSERT(VOP_ISLOCKED(dvp) == LK_EXCLUSIVE);

	/* XXX should handle this material another way */
	ulr = &VTOI(dvp)->i_crap;
	ULFS_CHECK_CRAPCOUNTER(VTOI(dvp));

	error = 0;
	switch (ap->a_flags) {
	case LOOKUP:
		/* 4.4 format directories support whiteout operations */
		if (fs->um_maxsymlinklen > 0)
			return (0);
		return (EOPNOTSUPP);

	case CREATE:
		/* create a new directory whiteout */
		KASSERTMSG((fs->um_maxsymlinklen > 0),
		    "ulfs_whiteout: old format filesystem");

		error = ulfs_direnter(dvp, ulr, NULL,
				      cnp, ULFS_WINO, LFS_DT_WHT,  NULL);
		break;

	case DELETE:
		/* remove an existing directory whiteout */
		KASSERTMSG((fs->um_maxsymlinklen > 0),
		    "ulfs_whiteout: old format filesystem");

		cnp->cn_flags &= ~DOWHITEOUT;
		error = ulfs_dirremove(dvp, ulr, NULL, cnp->cn_flags, 0);
		break;
	default:
		panic("ulfs_whiteout: unknown op");
		/* NOTREACHED */
	}
	return (error);
}

int
ulfs_rmdir(void *v)
{
	struct vop_rmdir_v2_args /* {
		struct vnode		*a_dvp;
		struct vnode		*a_vp;
		struct componentname	*a_cnp;
	} */ *ap = v;
	struct vnode		*vp, *dvp;
	struct componentname	*cnp;
	struct inode		*ip, *dp;
	int			error;
	struct ulfs_lookup_results *ulr;

	dvp = ap->a_dvp;
	vp = ap->a_vp;
	cnp = ap->a_cnp;

	KASSERT(VOP_ISLOCKED(dvp) == LK_EXCLUSIVE);
	KASSERT(VOP_ISLOCKED(vp) == LK_EXCLUSIVE);

	dp = VTOI(dvp);
	ip = VTOI(vp);

	/* XXX should handle this material another way */
	ulr = &dp->i_crap;
	ULFS_CHECK_CRAPCOUNTER(dp);

	/*
	 * No rmdir "." or of mounted directories please.
	 */
	if (dp == ip || vp->v_mountedhere != NULL) {
		if (dp == ip)
			vrele(vp);
		else
			vput(vp);
		return (EINVAL);
	}

	/*
	 * Do not remove a directory that is in the process of being renamed.
	 * Verify that the directory is empty (and valid). (Rmdir ".." won't
	 * be valid since ".." will contain a reference to the current
	 * directory and thus be non-empty.)
	 */
	error = 0;
	if (ip->i_nlink != 2 ||
	    !ulfs_dirempty(ip, dp->i_number, cnp->cn_cred)) {
		error = ENOTEMPTY;
		goto out;
	}
	if ((dp->i_flags & APPEND) ||
		(ip->i_flags & (IMMUTABLE | APPEND))) {
		error = EPERM;
		goto out;
	}
	/*
	 * Delete reference to directory before purging
	 * inode.  If we crash in between, the directory
	 * will be reattached to lost+found,
	 */
	error = ulfs_dirremove(dvp, ulr, ip, cnp->cn_flags, 1);
	if (error) {
		goto out;
	}
	VN_KNOTE(dvp, NOTE_WRITE | NOTE_LINK);
	cache_purge(dvp);
	/*
	 * Truncate inode.  The only stuff left in the directory is "." and
	 * "..".  The "." reference is inconsequential since we're quashing
	 * it.
	 */
	dp->i_nlink--;
	DIP_ASSIGN(dp, nlink, dp->i_nlink);
	dp->i_state |= IN_CHANGE;
	ip->i_nlink--;
	DIP_ASSIGN(ip, nlink, ip->i_nlink);
	ip->i_state |= IN_CHANGE;
	error = lfs_truncate(vp, (off_t)0, IO_SYNC, cnp->cn_cred);
	cache_purge(vp);
#ifdef LFS_DIRHASH
	if (ip->i_dirhash != NULL)
		ulfsdirhash_free(ip);
#endif
 out:
	VN_KNOTE(vp, NOTE_DELETE);
	vput(vp);
	return (error);
}

/*
 * Vnode op for reading directories.
 *
 * This routine handles converting from the on-disk directory format
 * "struct lfs_direct" to the in-memory format "struct dirent" as well as
 * byte swapping the entries if necessary.
 */
int
ulfs_readdir(void *v)
{
	struct vop_readdir_args /* {
		struct vnode	*a_vp;
		struct uio	*a_uio;
		kauth_cred_t	a_cred;
		int		*a_eofflag;
		off_t		**a_cookies;
		int		*a_ncookies;
	} */ *ap = v;

	/* vnode and fs */
	struct vnode	*vp = ap->a_vp;
	struct ulfsmount *ump = VFSTOULFS(vp->v_mount);
	struct lfs *fs = ump->um_lfs;
	/* caller's buffer */
	struct uio	*calleruio = ap->a_uio;
	off_t		startoffset, endoffset;
	size_t		callerbytes;
	off_t		curoffset;
	/* dirent production buffer */
	char		*direntbuf;
	size_t		direntbufmax;
	struct dirent	*dirent, *stopdirent;
	/* output cookies array */
	off_t		*cookies;
	size_t		numcookies, maxcookies;
	/* disk buffer */
	off_t		physstart, physend;
	size_t		skipstart, dropend;
	char		*rawbuf;
	size_t		rawbufmax, rawbytes;
	struct uio	rawuio;
	struct iovec	rawiov;
	LFS_DIRHEADER	*rawdp, *stoprawdp;
	/* general */
	int		error;

	KASSERT(VOP_ISLOCKED(vp));

	/* figure out where we want to read */
	callerbytes = calleruio->uio_resid;
	startoffset = calleruio->uio_offset;
	endoffset = startoffset + callerbytes;

	if (callerbytes < _DIRENT_MINSIZE(dirent)) {
		/* no room for even one struct dirent */
		return EINVAL;
	}

	/* round start and end down to block boundaries */
	physstart = startoffset & ~(off_t)(fs->um_dirblksiz - 1);
	physend = endoffset & ~(off_t)(fs->um_dirblksiz - 1);
	skipstart = startoffset - physstart;
	dropend = endoffset - physend;

	if (callerbytes - dropend < LFS_DIRECTSIZ(fs, 0)) {
		/* no room for even one dirheader + name */
		return EINVAL;
	}

	/* how much to actually read */
	rawbufmax = callerbytes + skipstart - dropend;

	/* read it */
	rawbuf = kmem_alloc(rawbufmax, KM_SLEEP);
	rawiov.iov_base = rawbuf;
	rawiov.iov_len = rawbufmax;
	rawuio.uio_iov = &rawiov;
	rawuio.uio_iovcnt = 1;
	rawuio.uio_offset = physstart;
	rawuio.uio_resid = rawbufmax;
	UIO_SETUP_SYSSPACE(&rawuio);
	rawuio.uio_rw = UIO_READ;
	error = VOP_READ(vp, &rawuio, 0, ap->a_cred);
	if (error != 0) {
		kmem_free(rawbuf, rawbufmax);
		return error;
	}
	rawbytes = rawbufmax - rawuio.uio_resid;

	/* the raw entries to iterate over */
	rawdp = (LFS_DIRHEADER *)(void *)rawbuf;
	stoprawdp = (LFS_DIRHEADER *)(void *)&rawbuf[rawbytes];

	/* allocate space to produce dirents into */
	direntbufmax = callerbytes;
	direntbuf = kmem_alloc(direntbufmax, KM_SLEEP);

	/* the dirents to iterate over */
	dirent = (struct dirent *)(void *)direntbuf;
	stopdirent = (struct dirent *)(void *)&direntbuf[direntbufmax];

	/* the output "cookies" (seek positions of directory entries) */
	if (ap->a_cookies) {
		numcookies = 0;
		maxcookies = rawbytes / LFS_DIRECTSIZ(fs, 1);
		cookies = malloc(maxcookies * sizeof(*cookies),
		    M_TEMP, M_WAITOK);
	} else {
		/* XXX: GCC */
		maxcookies = 0;
		cookies = NULL;
	}

	/* now produce the dirents */
	curoffset = calleruio->uio_offset;
	while (rawdp < stoprawdp) {
		if (skipstart > 0) {
			/* drain skipstart */
			if (lfs_dir_getreclen(fs, rawdp) <= skipstart) {
				skipstart -= lfs_dir_getreclen(fs, rawdp);
				rawdp = LFS_NEXTDIR(fs, rawdp);
				continue;
			}
			/* caller's start position wasn't on an entry */
			error = EINVAL;
			goto out;
		}
		if (lfs_dir_getreclen(fs, rawdp) == 0) {
			struct dirent *save = dirent;
			dirent->d_reclen = _DIRENT_MINSIZE(dirent);
			dirent = _DIRENT_NEXT(dirent);
			save->d_reclen = 0;
			rawdp = stoprawdp;
			break;
		}

		/* copy the header */
		dirent->d_type = lfs_dir_gettype(fs, rawdp);
		dirent->d_namlen = lfs_dir_getnamlen(fs, rawdp);
		dirent->d_reclen = _DIRENT_RECLEN(dirent, dirent->d_namlen);

		/* stop if there isn't room for the name AND another header */
		if ((char *)(void *)dirent + dirent->d_reclen +
		    _DIRENT_MINSIZE(dirent) > (char *)(void *)stopdirent)
			break;

		/* copy the name (and inode (XXX: why after the test?)) */
		dirent->d_fileno = lfs_dir_getino(fs, rawdp);
		(void)memcpy(dirent->d_name, lfs_dir_nameptr(fs, rawdp),
			     dirent->d_namlen);
		memset(&dirent->d_name[dirent->d_namlen], 0,
		    dirent->d_reclen - _DIRENT_NAMEOFF(dirent)
		    - dirent->d_namlen);

		/* onward */
		curoffset += lfs_dir_getreclen(fs, rawdp);
		if (ap->a_cookies) {
			KASSERT(numcookies < maxcookies);
			cookies[numcookies++] = curoffset;
		}
		dirent = _DIRENT_NEXT(dirent);
		rawdp = LFS_NEXTDIR(fs, rawdp);
	}

	/* transfer the dirents to the caller's buffer */
	callerbytes = ((char *)(void *)dirent - direntbuf);
	error = uiomove(direntbuf, callerbytes, calleruio);

out:
	calleruio->uio_offset = curoffset;
	if (ap->a_cookies) {
		if (error) {
			free(cookies, M_TEMP);
			*ap->a_cookies = NULL;
			*ap->a_ncookies = 0;
		} else {
			*ap->a_cookies = cookies;
			*ap->a_ncookies = numcookies;
		}
	}
	kmem_free(direntbuf, direntbufmax);
	kmem_free(rawbuf, rawbufmax);
	*ap->a_eofflag = VTOI(vp)->i_size <= calleruio->uio_offset;
	return error;
}

/*
 * Return target name of a symbolic link
 */
int
ulfs_readlink(void *v)
{
	struct vop_readlink_args /* {
		struct vnode	*a_vp;
		struct uio	*a_uio;
		kauth_cred_t	a_cred;
	} */ *ap = v;
	struct vnode	*vp = ap->a_vp;
	struct inode	*ip = VTOI(vp);
	struct ulfsmount *ump = VFSTOULFS(vp->v_mount);
	struct lfs *fs = ump->um_lfs;
	int		isize;

	KASSERT(VOP_ISLOCKED(vp));

	/*
	 * The test against um_maxsymlinklen is off by one; it should
	 * theoretically be <=, not <. However, it cannot be changed
	 * as that would break compatibility with existing fs images.
	 */

	isize = ip->i_size;
	if (isize < fs->um_maxsymlinklen ||
	    (fs->um_maxsymlinklen == 0 && DIP(ip, blocks) == 0)) {
		uiomove((char *)SHORTLINK(ip), isize, ap->a_uio);
		return (0);
	}
	return (lfs_bufrd(vp, ap->a_uio, 0, ap->a_cred));
}

/*
 * Print out the contents of an inode.
 */
int
ulfs_print(void *v)
{
	struct vop_print_args /* {
		struct vnode	*a_vp;
	} */ *ap = v;
	struct vnode	*vp;
	struct inode	*ip;

	vp = ap->a_vp;
	ip = VTOI(vp);
	printf("tag VT_ULFS, ino %llu, on dev %llu, %llu",
	    (unsigned long long)ip->i_number,
	    (unsigned long long)major(ip->i_dev),
	    (unsigned long long)minor(ip->i_dev));
	printf(" flags 0x%x, nlink %d\n",
	    ip->i_state, ip->i_nlink);
	printf("\tmode 0%o, owner %d, group %d, size %qd",
	    ip->i_mode, ip->i_uid, ip->i_gid,
	    (long long)ip->i_size);
	if (vp->v_type == VFIFO)
		VOCALL(fifo_vnodeop_p, VOFFSET(vop_print), v);
	printf("\n");
	return (0);
}

/*
 * Read wrapper for special devices.
 */
int
ulfsspec_read(void *v)
{
	struct vop_read_args /* {
		struct vnode	*a_vp;
		struct uio	*a_uio;
		int		a_ioflag;
		kauth_cred_t	a_cred;
	} */ *ap = v;

	KASSERT(VOP_ISLOCKED(ap->a_vp));

	/*
	 * Set access flag.
	 */
	if ((ap->a_vp->v_mount->mnt_flag & MNT_NODEVMTIME) == 0)
		VTOI(ap->a_vp)->i_state |= IN_ACCESS;
	return (VOCALL (spec_vnodeop_p, VOFFSET(vop_read), ap));
}

/*
 * Write wrapper for special devices.
 */
int
ulfsspec_write(void *v)
{
	struct vop_write_args /* {
		struct vnode	*a_vp;
		struct uio	*a_uio;
		int		a_ioflag;
		kauth_cred_t	a_cred;
	} */ *ap = v;

	KASSERT(VOP_ISLOCKED(ap->a_vp) == LK_EXCLUSIVE);

	/*
	 * Set update and change flags.
	 */
	if ((ap->a_vp->v_mount->mnt_flag & MNT_NODEVMTIME) == 0)
		VTOI(ap->a_vp)->i_state |= IN_MODIFY;
	return (VOCALL (spec_vnodeop_p, VOFFSET(vop_write), ap));
}

/*
 * Read wrapper for fifo's
 */
int
ulfsfifo_read(void *v)
{
	struct vop_read_args /* {
		struct vnode	*a_vp;
		struct uio	*a_uio;
		int		a_ioflag;
		kauth_cred_t	a_cred;
	} */ *ap = v;

	KASSERT(VOP_ISLOCKED(ap->a_vp));

	/*
	 * Set access flag.
	 */
	VTOI(ap->a_vp)->i_state |= IN_ACCESS;
	return (VOCALL (fifo_vnodeop_p, VOFFSET(vop_read), ap));
}

/*
 * Write wrapper for fifo's.
 */
int
ulfsfifo_write(void *v)
{
	struct vop_write_args /* {
		struct vnode	*a_vp;
		struct uio	*a_uio;
		int		a_ioflag;
		kauth_cred_t	a_cred;
	} */ *ap = v;

	KASSERT(VOP_ISLOCKED(ap->a_vp) == LK_EXCLUSIVE);

	/*
	 * Set update and change flags.
	 */
	VTOI(ap->a_vp)->i_state |= IN_MODIFY;
	return (VOCALL (fifo_vnodeop_p, VOFFSET(vop_write), ap));
}

/*
 * Return POSIX pathconf information applicable to ulfs filesystems.
 */
int
ulfs_pathconf(void *v)
{
	struct vop_pathconf_args /* {
		struct vnode	*a_vp;
		int		a_name;
		register_t	*a_retval;
	} */ *ap = v;

	switch (ap->a_name) {
	case _PC_LINK_MAX:
		*ap->a_retval = LINK_MAX;
		return (0);
	case _PC_NAME_MAX:
		*ap->a_retval = LFS_MAXNAMLEN;
		return (0);
	case _PC_PATH_MAX:
		*ap->a_retval = PATH_MAX;
		return (0);
	case _PC_PIPE_BUF:
		*ap->a_retval = PIPE_BUF;
		return (0);
	case _PC_CHOWN_RESTRICTED:
		*ap->a_retval = 1;
		return (0);
	case _PC_NO_TRUNC:
		*ap->a_retval = 1;
		return (0);
	case _PC_SYNC_IO:
		*ap->a_retval = 1;
		return (0);
	case _PC_FILESIZEBITS:
		*ap->a_retval = 42;
		return (0);
	case _PC_SYMLINK_MAX:
		*ap->a_retval = MAXPATHLEN;
		return (0);
	case _PC_2_SYMLINKS:
		*ap->a_retval = 1;
		return (0);
	default:
		return (EINVAL);
	}
	/* NOTREACHED */
}

/*
 * Advisory record locking support
 */
int
ulfs_advlock(void *v)
{
	struct vop_advlock_args /* {
		struct vnode	*a_vp;
		void *		a_id;
		int		a_op;
		struct flock	*a_fl;
		int		a_flags;
	} */ *ap = v;
	struct inode *ip;

	ip = VTOI(ap->a_vp);
	return lf_advlock(ap, &ip->i_lockf, ip->i_size);
}

/*
 * Initialize the vnode associated with a new inode, handle aliased
 * vnodes.
 */
void
ulfs_vinit(struct mount *mntp, int (**specops)(void *), int (**fifoops)(void *),
	struct vnode **vpp)
{
	struct timeval	tv;
	struct inode	*ip;
	struct vnode	*vp;
	dev_t		rdev;
	struct ulfsmount *ump;

	vp = *vpp;
	ip = VTOI(vp);
	switch(vp->v_type = IFTOVT(ip->i_mode)) {
	case VCHR:
	case VBLK:
		vp->v_op = specops;
		ump = ip->i_ump;
		// XXX clean this up
		if (ump->um_fstype == ULFS1)
			rdev = (dev_t)ulfs_rw32(ip->i_din->u_32.di_rdev,
			    ULFS_MPNEEDSWAP(ump->um_lfs));
		else
			rdev = (dev_t)ulfs_rw64(ip->i_din->u_64.di_rdev,
			    ULFS_MPNEEDSWAP(ump->um_lfs));
		spec_node_init(vp, rdev);
		break;
	case VFIFO:
		vp->v_op = fifoops;
		break;
	case VNON:
	case VBAD:
	case VSOCK:
	case VLNK:
	case VDIR:
	case VREG:
		break;
	}
	if (ip->i_number == ULFS_ROOTINO)
                vp->v_vflag |= VV_ROOT;
	/*
	 * Initialize modrev times
	 */
	getmicrouptime(&tv);
	ip->i_modrev = (uint64_t)(uint)tv.tv_sec << 32
			| tv.tv_usec * 4294u;
	*vpp = vp;
}

/*
 * Allocate len bytes at offset off.
 */
int
ulfs_gop_alloc(struct vnode *vp, off_t off, off_t len, int flags,
    kauth_cred_t cred)
{
        struct inode *ip = VTOI(vp);
        int error, delta, bshift, bsize;
        UVMHIST_FUNC("ulfs_gop_alloc"); UVMHIST_CALLED(ubchist);

	KASSERT(genfs_node_wrlocked(vp));

        error = 0;
        bshift = vp->v_mount->mnt_fs_bshift;
        bsize = 1 << bshift;

        delta = off & (bsize - 1);
        off -= delta;
        len += delta;

        while (len > 0) {
                bsize = MIN(bsize, len);

                error = lfs_balloc(vp, off, bsize, cred, flags, NULL);
                if (error) {
                        goto out;
                }

                /*
                 * increase file size now, lfs_balloc() requires that
                 * EOF be up-to-date before each call.
                 */

                if (ip->i_size < off + bsize) {
                        UVMHIST_LOG(ubchist, "vp %#jx old 0x%jx new 0x%jx",
                            (uintptr_t)vp, ip->i_size, off + bsize, 0);
                        ip->i_size = off + bsize;
			DIP_ASSIGN(ip, size, ip->i_size);
                }

                off += bsize;
                len -= bsize;
        }

out:
	return error;
}

void
ulfs_gop_markupdate(struct vnode *vp, int flags)
{
	u_int32_t mask = 0;

	if ((flags & GOP_UPDATE_ACCESSED) != 0) {
		mask = IN_ACCESS;
	}
	if ((flags & GOP_UPDATE_MODIFIED) != 0) {
		if (vp->v_type == VREG) {
			mask |= IN_CHANGE | IN_UPDATE;
		} else {
			mask |= IN_MODIFY;
		}
	}
	if (mask) {
		struct inode *ip = VTOI(vp);

		ip->i_state |= mask;
	}
}

int
ulfs_bufio(enum uio_rw rw, struct vnode *vp, void *buf, size_t len, off_t off,
    int ioflg, kauth_cred_t cred, size_t *aresid, struct lwp *l)
{
	struct iovec iov;
	struct uio uio;
	int error;

	KASSERT(ISSET(ioflg, IO_NODELOCKED));
	KASSERT(VOP_ISLOCKED(vp));
	KASSERT(rw != UIO_WRITE || VOP_ISLOCKED(vp) == LK_EXCLUSIVE);

	iov.iov_base = buf;
	iov.iov_len = len;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_resid = len;
	uio.uio_offset = off;
	uio.uio_rw = rw;
	UIO_SETUP_SYSSPACE(&uio);

	switch (rw) {
	case UIO_READ:
		error = lfs_bufrd(vp, &uio, ioflg, cred);
		break;
	case UIO_WRITE:
		error = lfs_bufwr(vp, &uio, ioflg, cred);
		break;
	default:
		panic("invalid uio rw: %d", (int)rw);
	}

	if (aresid)
		*aresid = uio.uio_resid;
	else if (uio.uio_resid && error == 0)
		error = EIO;

	KASSERT(VOP_ISLOCKED(vp));
	KASSERT(rw != UIO_WRITE || VOP_ISLOCKED(vp) == LK_EXCLUSIVE);
	return error;
}
