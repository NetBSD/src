/*	$NetBSD: ufs_vnops.c,v 1.139.6.1 2006/05/24 15:50:48 tron Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ufs_vnops.c,v 1.139.6.1 2006/05/24 15:50:48 tron Exp $");

#if defined(_KERNEL_OPT)
#include "opt_ffs.h"
#include "opt_quota.h"
#include "fs_lfs.h"
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
#include <sys/malloc.h>
#include <sys/dirent.h>
#include <sys/lockf.h>
#include <sys/kauth.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/fifofs/fifo.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_bswap.h>
#include <ufs/ufs/ufs_extern.h>
#ifdef UFS_DIRHASH
#include <ufs/ufs/dirhash.h>
#endif
#include <ufs/ext2fs/ext2fs_extern.h>
#include <ufs/ffs/ffs_extern.h>
#include <ufs/lfs/lfs_extern.h>

#include <uvm/uvm.h>

static int ufs_chmod(struct vnode *, int, kauth_cred_t, struct proc *);
static int ufs_chown(struct vnode *, uid_t, gid_t, kauth_cred_t,
    struct proc *);

/*
 * A virgin directory (no blushing please).
 */
static const struct dirtemplate mastertemplate = {
	0,	12,		DT_DIR,	1,	".",
	0,	DIRBLKSIZ - 12,	DT_DIR,	2,	".."
};

/*
 * Create a regular file
 */
int
ufs_create(void *v)
{
	struct vop_create_args /* {
		struct vnode		*a_dvp;
		struct vnode		**a_vpp;
		struct componentname	*a_cnp;
		struct vattr		*a_vap;
	} */ *ap = v;
	int	error;

	error =
	    ufs_makeinode(MAKEIMODE(ap->a_vap->va_type, ap->a_vap->va_mode),
			  ap->a_dvp, ap->a_vpp, ap->a_cnp);
	if (error)
		return (error);
	VN_KNOTE(ap->a_dvp, NOTE_WRITE);
	return (0);
}

/*
 * Mknod vnode call
 */
/* ARGSUSED */
int
ufs_mknod(void *v)
{
	struct vop_mknod_args /* {
		struct vnode		*a_dvp;
		struct vnode		**a_vpp;
		struct componentname	*a_cnp;
		struct vattr		*a_vap;
	} */ *ap = v;
	struct vattr	*vap;
	struct vnode	**vpp;
	struct inode	*ip;
	int		error;
	struct mount	*mp;
	ino_t		ino;

	vap = ap->a_vap;
	vpp = ap->a_vpp;
	if ((error =
	    ufs_makeinode(MAKEIMODE(vap->va_type, vap->va_mode),
	    ap->a_dvp, vpp, ap->a_cnp)) != 0)
		return (error);
	VN_KNOTE(ap->a_dvp, NOTE_WRITE);
	ip = VTOI(*vpp);
	mp  = (*vpp)->v_mount;
	ino = ip->i_number;
	ip->i_flag |= IN_ACCESS | IN_CHANGE | IN_UPDATE;
	if (vap->va_rdev != VNOVAL) {
		struct ufsmount *ump = ip->i_ump;
		/*
		 * Want to be able to use this to make badblock
		 * inodes, so don't truncate the dev number.
		 */
		if (ump->um_fstype == UFS1)
			ip->i_ffs1_rdev = ufs_rw32(vap->va_rdev,
			    UFS_MPNEEDSWAP(ump));
		else
			ip->i_ffs2_rdev = ufs_rw64(vap->va_rdev,
			    UFS_MPNEEDSWAP(ump));
	}
	/*
	 * Remove inode so that it will be reloaded by VFS_VGET and
	 * checked to see if it is an alias of an existing entry in
	 * the inode cache.
	 */
	vput(*vpp);
	(*vpp)->v_type = VNON;
	vgone(*vpp);
	error = VFS_VGET(mp, ino, vpp);
	if (error != 0) {
		*vpp = NULL;
		return (error);
	}
	return (0);
}

/*
 * Open called.
 *
 * Nothing to do.
 */
/* ARGSUSED */
int
ufs_open(void *v)
{
	struct vop_open_args /* {
		struct vnode	*a_vp;
		int		a_mode;
		kauth_cred_t	a_cred;
		struct lwp	*a_l;
	} */ *ap = v;

	/*
	 * Files marked append-only must be opened for appending.
	 */
	if ((VTOI(ap->a_vp)->i_flags & APPEND) &&
	    (ap->a_mode & (FWRITE | O_APPEND)) == FWRITE)
		return (EPERM);
	return (0);
}

/*
 * Close called.
 *
 * Update the times on the inode.
 */
/* ARGSUSED */
int
ufs_close(void *v)
{
	struct vop_close_args /* {
		struct vnode	*a_vp;
		int		a_fflag;
		kauth_cred_t	a_cred;
		struct lwp	*a_l;
	} */ *ap = v;
	struct vnode	*vp;
	struct inode	*ip;

	vp = ap->a_vp;
	ip = VTOI(vp);
	simple_lock(&vp->v_interlock);
	if (vp->v_usecount > 1)
		UFS_ITIMES(vp, NULL, NULL, NULL);
	simple_unlock(&vp->v_interlock);
	return (0);
}

int
ufs_access(void *v)
{
	struct vop_access_args /* {
		struct vnode	*a_vp;
		int		a_mode;
		kauth_cred_t	a_cred;
		struct lwp	*a_l;
	} */ *ap = v;
	struct vnode	*vp;
	struct inode	*ip;
	mode_t		mode;
#ifdef QUOTA
	int		error;
#endif

	vp = ap->a_vp;
	ip = VTOI(vp);
	mode = ap->a_mode;
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
#ifdef QUOTA
			if ((error = getinoquota(ip)) != 0)
				return (error);
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

	/* If immutable bit set, nobody gets to write it. */
	if ((mode & VWRITE) && (ip->i_flags & (IMMUTABLE | SF_SNAPSHOT)))
		return (EPERM);

	return (vaccess(vp->v_type, ip->i_mode & ALLPERMS,
		ip->i_uid, ip->i_gid, mode, ap->a_cred));
}

/* ARGSUSED */
int
ufs_getattr(void *v)
{
	struct vop_getattr_args /* {
		struct vnode	*a_vp;
		struct vattr	*a_vap;
		kauth_cred_t	a_cred;
		struct lwp	*a_l;
	} */ *ap = v;
	struct vnode	*vp;
	struct inode	*ip;
	struct vattr	*vap;

	vp = ap->a_vp;
	ip = VTOI(vp);
	vap = ap->a_vap;
	UFS_ITIMES(vp, NULL, NULL, NULL);

	/*
	 * Copy from inode table
	 */
	vap->va_fsid = ip->i_dev;
	vap->va_fileid = ip->i_number;
	vap->va_mode = ip->i_mode & ALLPERMS;
	vap->va_nlink = ip->i_ffs_effnlink;
	vap->va_uid = ip->i_uid;
	vap->va_gid = ip->i_gid;
	vap->va_size = vp->v_size;
	if (ip->i_ump->um_fstype == UFS1) {
		vap->va_rdev = (dev_t)ufs_rw32(ip->i_ffs1_rdev,
		    UFS_MPNEEDSWAP(ip->i_ump));
		vap->va_atime.tv_sec = ip->i_ffs1_atime;
		vap->va_atime.tv_nsec = ip->i_ffs1_atimensec;
		vap->va_mtime.tv_sec = ip->i_ffs1_mtime;
		vap->va_mtime.tv_nsec = ip->i_ffs1_mtimensec;
		vap->va_ctime.tv_sec = ip->i_ffs1_ctime;
		vap->va_ctime.tv_nsec = ip->i_ffs1_ctimensec;
		vap->va_birthtime.tv_sec = 0;
		vap->va_birthtime.tv_nsec = 0;
		vap->va_bytes = dbtob((u_quad_t)ip->i_ffs1_blocks);
	} else {
		vap->va_rdev = (dev_t)ufs_rw64(ip->i_ffs2_rdev,
		    UFS_MPNEEDSWAP(ip->i_ump));
		vap->va_atime.tv_sec = ip->i_ffs2_atime;
		vap->va_atime.tv_nsec = ip->i_ffs2_atimensec;
		vap->va_mtime.tv_sec = ip->i_ffs2_mtime;
		vap->va_mtime.tv_nsec = ip->i_ffs2_mtimensec;
		vap->va_ctime.tv_sec = ip->i_ffs2_ctime;
		vap->va_ctime.tv_nsec = ip->i_ffs2_ctimensec;
		vap->va_birthtime.tv_sec = ip->i_ffs2_birthtime;
		vap->va_birthtime.tv_nsec = ip->i_ffs2_birthnsec;
		vap->va_bytes = dbtob(ip->i_ffs2_blocks);
	}
	vap->va_gen = ip->i_gen;
	vap->va_flags = ip->i_flags;

	/* this doesn't belong here */
	if (vp->v_type == VBLK)
		vap->va_blocksize = BLKDEV_IOSIZE;
	else if (vp->v_type == VCHR)
		vap->va_blocksize = MAXBSIZE;
	else
		vap->va_blocksize = vp->v_mount->mnt_stat.f_iosize;
	vap->va_type = vp->v_type;
	vap->va_filerev = ip->i_modrev;
	return (0);
}

/*
 * Set attribute vnode op. called from several syscalls
 */
int
ufs_setattr(void *v)
{
	struct vop_setattr_args /* {
		struct vnode	*a_vp;
		struct vattr	*a_vap;
		kauth_cred_t	a_cred;
		struct lwp	*a_l;
	} */ *ap = v;
	struct vattr	*vap;
	struct vnode	*vp;
	struct inode	*ip;
	kauth_cred_t	cred;
	struct lwp	*l;
	int		error;

	vap = ap->a_vap;
	vp = ap->a_vp;
	ip = VTOI(vp);
	cred = ap->a_cred;
	l = ap->a_l;

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
		if (vp->v_mount->mnt_flag & MNT_RDONLY)
			return (EROFS);
		if (kauth_cred_geteuid(cred) != ip->i_uid &&
		    (error = kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER,
					       &l->l_proc->p_acflag)))
			return (error);
		if (kauth_cred_geteuid(cred) == 0) {
			if ((ip->i_flags & (SF_IMMUTABLE | SF_APPEND)) &&
			    securelevel > 0)
				return (EPERM);
			/* Snapshot flag cannot be set or cleared */
			if ((vap->va_flags & SF_SNAPSHOT) !=
			    (ip->i_flags & SF_SNAPSHOT))
				return (EPERM);
			ip->i_flags = vap->va_flags;
			DIP_ASSIGN(ip, flags, ip->i_flags);
		} else {
			if ((ip->i_flags & (SF_IMMUTABLE | SF_APPEND)) ||
			    (vap->va_flags & UF_SETTABLE) != vap->va_flags)
				return (EPERM);
			if ((ip->i_flags & SF_SETTABLE) !=
			    (vap->va_flags & SF_SETTABLE))
				return (EPERM);
			ip->i_flags &= SF_SETTABLE;
			ip->i_flags |= (vap->va_flags & UF_SETTABLE);
			DIP_ASSIGN(ip, flags, ip->i_flags);
		}
		ip->i_flag |= IN_CHANGE;
		if (vap->va_flags & (IMMUTABLE | APPEND))
			return (0);
	}
	if (ip->i_flags & (IMMUTABLE | APPEND))
		return (EPERM);
	/*
	 * Go through the fields and update iff not VNOVAL.
	 */
	if (vap->va_uid != (uid_t)VNOVAL || vap->va_gid != (gid_t)VNOVAL) {
		if (vp->v_mount->mnt_flag & MNT_RDONLY)
			return (EROFS);
		error = ufs_chown(vp, vap->va_uid, vap->va_gid, cred, l->l_proc);
		if (error)
			return (error);
	}
	if (vap->va_size != VNOVAL) {
		/*
		 * Disallow write attempts on read-only file systems;
		 * unless the file is a socket, fifo, or a block or
		 * character device resident on the file system.
		 */
		switch (vp->v_type) {
		case VDIR:
			return (EISDIR);
		case VCHR:
		case VBLK:
		case VFIFO:
			break;
		case VREG:
			if (vp->v_mount->mnt_flag & MNT_RDONLY)
				 return (EROFS);
			if ((ip->i_flags & SF_SNAPSHOT) != 0)
				return (EPERM);
			error = UFS_TRUNCATE(vp, vap->va_size, 0, cred, l);
			if (error)
				return (error);
			break;
		default:
			return (EOPNOTSUPP);
		}
	}
	ip = VTOI(vp);
	if (vap->va_atime.tv_sec != VNOVAL || vap->va_mtime.tv_sec != VNOVAL ||
	    vap->va_birthtime.tv_sec != VNOVAL) {
		if (vp->v_mount->mnt_flag & MNT_RDONLY)
			return (EROFS);
		if ((ip->i_flags & SF_SNAPSHOT) != 0)
			return (EPERM);
		if (kauth_cred_geteuid(cred) != ip->i_uid &&
		    (error = kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER,
					       &l->l_proc->p_acflag)) &&
		    ((vap->va_vaflags & VA_UTIMES_NULL) == 0 ||
		    (error = VOP_ACCESS(vp, VWRITE, cred, l))))
			return (error);
		if (vap->va_atime.tv_sec != VNOVAL)
			if (!(vp->v_mount->mnt_flag & MNT_NOATIME))
				ip->i_flag |= IN_ACCESS;
		if (vap->va_mtime.tv_sec != VNOVAL)
			ip->i_flag |= IN_CHANGE | IN_UPDATE;
		if (vap->va_birthtime.tv_sec != VNOVAL &&
		    ip->i_ump->um_fstype == UFS2) {
			ip->i_ffs2_birthtime = vap->va_birthtime.tv_sec;
			ip->i_ffs2_birthnsec = vap->va_birthtime.tv_nsec;
		}
		error = UFS_UPDATE(vp, &vap->va_atime, &vap->va_mtime, 0);
		if (error)
			return (error);
	}
	error = 0;
	if (vap->va_mode != (mode_t)VNOVAL) {
		if (vp->v_mount->mnt_flag & MNT_RDONLY)
			return (EROFS);
		if ((ip->i_flags & SF_SNAPSHOT) != 0 &&
		    (vap->va_mode & (S_IXUSR | S_IWUSR | S_IXGRP | S_IWGRP |
		     S_IXOTH | S_IWOTH)))
			return (EPERM);
		error = ufs_chmod(vp, (int)vap->va_mode, cred, l->l_proc);
	}
	VN_KNOTE(vp, NOTE_ATTRIB);
	return (error);
}

/*
 * Change the mode on a file.
 * Inode must be locked before calling.
 */
static int
ufs_chmod(struct vnode *vp, int mode, kauth_cred_t cred, struct proc *p)
{
	struct inode	*ip;
	int		error, ismember = 0;

	ip = VTOI(vp);
	if (kauth_cred_geteuid(cred) != ip->i_uid &&
	    (error = kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER,
				       &p->p_acflag)))
		return (error);
	if (kauth_cred_geteuid(cred)) {
		if (vp->v_type != VDIR && (mode & S_ISTXT))
			return (EFTYPE);
		if ((kauth_cred_ismember_gid(cred, ip->i_gid, &ismember) != 0 ||
		    !ismember) && (mode & ISGID))
			return (EPERM);
	}
	ip->i_mode &= ~ALLPERMS;
	ip->i_mode |= (mode & ALLPERMS);
	ip->i_flag |= IN_CHANGE;
	DIP_ASSIGN(ip, mode, ip->i_mode);
	return (0);
}

/*
 * Perform chown operation on inode ip;
 * inode must be locked prior to call.
 */
static int
ufs_chown(struct vnode *vp, uid_t uid, gid_t gid, kauth_cred_t cred,
    	struct proc *p)
{
	struct inode	*ip;
	int		error, ismember = 0;
#ifdef QUOTA
	uid_t		ouid;
	gid_t		ogid;
	int		i;
	int64_t		change;
#endif
	ip = VTOI(vp);
	error = 0;

	if (uid == (uid_t)VNOVAL)
		uid = ip->i_uid;
	if (gid == (gid_t)VNOVAL)
		gid = ip->i_gid;
	/*
	 * If we don't own the file, are trying to change the owner
	 * of the file, or are not a member of the target group,
	 * the caller's credentials must imply super-user privilege
	 * or the call fails.
	 */
	if ((kauth_cred_geteuid(cred) != ip->i_uid || uid != ip->i_uid ||
	    (gid != ip->i_gid &&
	     !(kauth_cred_getegid(cred) == gid ||
	      (kauth_cred_ismember_gid(cred, gid, &ismember) == 0 &&
	      ismember)))) &&
	    ((error = kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER,
					&p->p_acflag)) != 0))
		return (error);

#ifdef QUOTA
	ogid = ip->i_gid;
	ouid = ip->i_uid;
	if ((error = getinoquota(ip)) != 0)
		return (error);
	if (ouid == uid) {
		dqrele(vp, ip->i_dquot[USRQUOTA]);
		ip->i_dquot[USRQUOTA] = NODQUOT;
	}
	if (ogid == gid) {
		dqrele(vp, ip->i_dquot[GRPQUOTA]);
		ip->i_dquot[GRPQUOTA] = NODQUOT;
	}
	change = DIP(ip, blocks);
	(void) chkdq(ip, -change, cred, CHOWN);
	(void) chkiq(ip, -1, cred, CHOWN);
	for (i = 0; i < MAXQUOTAS; i++) {
		dqrele(vp, ip->i_dquot[i]);
		ip->i_dquot[i] = NODQUOT;
	}
#endif
	ip->i_gid = gid;
	DIP_ASSIGN(ip, gid, gid);
	ip->i_uid = uid;
	DIP_ASSIGN(ip, uid, uid);
#ifdef QUOTA
	if ((error = getinoquota(ip)) == 0) {
		if (ouid == uid) {
			dqrele(vp, ip->i_dquot[USRQUOTA]);
			ip->i_dquot[USRQUOTA] = NODQUOT;
		}
		if (ogid == gid) {
			dqrele(vp, ip->i_dquot[GRPQUOTA]);
			ip->i_dquot[GRPQUOTA] = NODQUOT;
		}
		if ((error = chkdq(ip, change, cred, CHOWN)) == 0) {
			if ((error = chkiq(ip, 1, cred, CHOWN)) == 0)
				goto good;
			else
				(void) chkdq(ip, -change, cred, CHOWN|FORCE);
		}
		for (i = 0; i < MAXQUOTAS; i++) {
			dqrele(vp, ip->i_dquot[i]);
			ip->i_dquot[i] = NODQUOT;
		}
	}
	ip->i_gid = ogid;
	DIP_ASSIGN(ip, gid, ogid);
	ip->i_uid = ouid;
	DIP_ASSIGN(ip, uid, ouid);
	if (getinoquota(ip) == 0) {
		if (ouid == uid) {
			dqrele(vp, ip->i_dquot[USRQUOTA]);
			ip->i_dquot[USRQUOTA] = NODQUOT;
		}
		if (ogid == gid) {
			dqrele(vp, ip->i_dquot[GRPQUOTA]);
			ip->i_dquot[GRPQUOTA] = NODQUOT;
		}
		(void) chkdq(ip, change, cred, FORCE|CHOWN);
		(void) chkiq(ip, 1, cred, FORCE|CHOWN);
		(void) getinoquota(ip);
	}
	return (error);
 good:
	if (getinoquota(ip))
		panic("chown: lost quota");
#endif /* QUOTA */
	ip->i_flag |= IN_CHANGE;
	return (0);
}

int
ufs_remove(void *v)
{
	struct vop_remove_args /* {
		struct vnode		*a_dvp;
		struct vnode		*a_vp;
		struct componentname	*a_cnp;
	} */ *ap = v;
	struct vnode	*vp, *dvp;
	struct inode	*ip;
	int		error;

	vp = ap->a_vp;
	dvp = ap->a_dvp;
	ip = VTOI(vp);
	if (vp->v_type == VDIR || (ip->i_flags & (IMMUTABLE | APPEND)) ||
	    (VTOI(dvp)->i_flags & APPEND))
		error = EPERM;
	else
		error = ufs_dirremove(dvp, ip, ap->a_cnp->cn_flags, 0);
	VN_KNOTE(vp, NOTE_DELETE);
	VN_KNOTE(dvp, NOTE_WRITE);
	if (dvp == vp)
		vrele(vp);
	else
		vput(vp);
	vput(dvp);
	return (error);
}

/*
 * link vnode call
 */
int
ufs_link(void *v)
{
	struct vop_link_args /* {
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap = v;
	struct vnode		*vp, *dvp;
	struct componentname	*cnp;
	struct inode		*ip;
	struct direct		*newdir;
	int			error;

	dvp = ap->a_dvp;
	vp = ap->a_vp;
	cnp = ap->a_cnp;
#ifdef DIAGNOSTIC
	if ((cnp->cn_flags & HASBUF) == 0)
		panic("ufs_link: no name");
#endif
	if (vp->v_type == VDIR) {
		VOP_ABORTOP(dvp, cnp);
		error = EPERM;
		goto out2;
	}
	if (dvp->v_mount != vp->v_mount) {
		VOP_ABORTOP(dvp, cnp);
		error = EXDEV;
		goto out2;
	}
	if (dvp != vp && (error = vn_lock(vp, LK_EXCLUSIVE))) {
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
	ip->i_ffs_effnlink++;
	ip->i_nlink++;
	DIP_ASSIGN(ip, nlink, ip->i_nlink);
	ip->i_flag |= IN_CHANGE;
	if (DOINGSOFTDEP(vp))
		softdep_change_linkcnt(ip);
	error = UFS_UPDATE(vp, NULL, NULL, UPDATE_DIROP);
	if (!error) {
		newdir = pool_get(&ufs_direct_pool, PR_WAITOK);
		ufs_makedirentry(ip, cnp, newdir);
		error = ufs_direnter(dvp, vp, newdir, cnp, NULL);
		pool_put(&ufs_direct_pool, newdir);
	}
	if (error) {
		ip->i_ffs_effnlink--;
		ip->i_nlink--;
		DIP_ASSIGN(ip, nlink, ip->i_nlink);
		ip->i_flag |= IN_CHANGE;
		if (DOINGSOFTDEP(vp))
			softdep_change_linkcnt(ip);
	}
	PNBUF_PUT(cnp->cn_pnbuf);
 out1:
	if (dvp != vp)
		VOP_UNLOCK(vp, 0);
 out2:
	VN_KNOTE(vp, NOTE_LINK);
	VN_KNOTE(dvp, NOTE_WRITE);
	vput(dvp);
	return (error);
}

/*
 * whiteout vnode call
 */
int
ufs_whiteout(void *v)
{
	struct vop_whiteout_args /* {
		struct vnode		*a_dvp;
		struct componentname	*a_cnp;
		int			a_flags;
	} */ *ap = v;
	struct vnode		*dvp = ap->a_dvp;
	struct componentname	*cnp = ap->a_cnp;
	struct direct		*newdir;
	int			error;
	struct ufsmount		*ump = VFSTOUFS(dvp->v_mount);

	error = 0;
	switch (ap->a_flags) {
	case LOOKUP:
		/* 4.4 format directories support whiteout operations */
		if (ump->um_maxsymlinklen > 0)
			return (0);
		return (EOPNOTSUPP);

	case CREATE:
		/* create a new directory whiteout */
#ifdef DIAGNOSTIC
		if ((cnp->cn_flags & SAVENAME) == 0)
			panic("ufs_whiteout: missing name");
		if (ump->um_maxsymlinklen <= 0)
			panic("ufs_whiteout: old format filesystem");
#endif

		newdir = pool_get(&ufs_direct_pool, PR_WAITOK);
		newdir->d_ino = WINO;
		newdir->d_namlen = cnp->cn_namelen;
		memcpy(newdir->d_name, cnp->cn_nameptr,
		    (size_t)cnp->cn_namelen);
		newdir->d_name[cnp->cn_namelen] = '\0';
		newdir->d_type = DT_WHT;
		error = ufs_direnter(dvp, NULL, newdir, cnp, NULL);
		pool_put(&ufs_direct_pool, newdir);
		break;

	case DELETE:
		/* remove an existing directory whiteout */
#ifdef DIAGNOSTIC
		if (ump->um_maxsymlinklen <= 0)
			panic("ufs_whiteout: old format filesystem");
#endif

		cnp->cn_flags &= ~DOWHITEOUT;
		error = ufs_dirremove(dvp, NULL, cnp->cn_flags, 0);
		break;
	default:
		panic("ufs_whiteout: unknown op");
		/* NOTREACHED */
	}
	if (cnp->cn_flags & HASBUF) {
		PNBUF_PUT(cnp->cn_pnbuf);
		cnp->cn_flags &= ~HASBUF;
	}
	return (error);
}


/*
 * Rename system call.
 * 	rename("foo", "bar");
 * is essentially
 *	unlink("bar");
 *	link("foo", "bar");
 *	unlink("foo");
 * but ``atomically''.  Can't do full commit without saving state in the
 * inode on disk which isn't feasible at this time.  Best we can do is
 * always guarantee the target exists.
 *
 * Basic algorithm is:
 *
 * 1) Bump link count on source while we're linking it to the
 *    target.  This also ensure the inode won't be deleted out
 *    from underneath us while we work (it may be truncated by
 *    a concurrent `trunc' or `open' for creation).
 * 2) Link source to destination.  If destination already exists,
 *    delete it first.
 * 3) Unlink source reference to inode if still around. If a
 *    directory was moved and the parent of the destination
 *    is different from the source, patch the ".." entry in the
 *    directory.
 */
int
ufs_rename(void *v)
{
	struct vop_rename_args  /* {
		struct vnode		*a_fdvp;
		struct vnode		*a_fvp;
		struct componentname	*a_fcnp;
		struct vnode		*a_tdvp;
		struct vnode		*a_tvp;
		struct componentname	*a_tcnp;
	} */ *ap = v;
	struct vnode		*tvp, *tdvp, *fvp, *fdvp;
	struct componentname	*tcnp, *fcnp;
	struct inode		*ip, *xp, *dp;
	struct direct		*newdir;
	int			doingdirectory, oldparent, newparent, error;

	tvp = ap->a_tvp;
	tdvp = ap->a_tdvp;
	fvp = ap->a_fvp;
	fdvp = ap->a_fdvp;
	tcnp = ap->a_tcnp;
	fcnp = ap->a_fcnp;
	doingdirectory = oldparent = newparent = error = 0;

#ifdef DIAGNOSTIC
	if ((tcnp->cn_flags & HASBUF) == 0 ||
	    (fcnp->cn_flags & HASBUF) == 0)
		panic("ufs_rename: no name");
#endif
	/*
	 * Check for cross-device rename.
	 */
	if ((fvp->v_mount != tdvp->v_mount) ||
	    (tvp && (fvp->v_mount != tvp->v_mount))) {
		error = EXDEV;
 abortit:
		VOP_ABORTOP(tdvp, tcnp); /* XXX, why not in NFS? */
		if (tdvp == tvp)
			vrele(tdvp);
		else
			vput(tdvp);
		if (tvp)
			vput(tvp);
		VOP_ABORTOP(fdvp, fcnp); /* XXX, why not in NFS? */
		vrele(fdvp);
		vrele(fvp);
		return (error);
	}

	/*
	 * Check if just deleting a link name.
	 */
	if (tvp && ((VTOI(tvp)->i_flags & (IMMUTABLE | APPEND)) ||
	    (VTOI(tdvp)->i_flags & APPEND))) {
		error = EPERM;
		goto abortit;
	}
	if (fvp == tvp) {
		if (fvp->v_type == VDIR) {
			error = EINVAL;
			goto abortit;
		}

		/* Release destination completely. */
		VOP_ABORTOP(tdvp, tcnp);
		vput(tdvp);
		vput(tvp);

		/* Delete source. */
		vrele(fvp);
		fcnp->cn_flags &= ~(MODMASK | SAVESTART);
		fcnp->cn_flags |= LOCKPARENT | LOCKLEAF;
		fcnp->cn_nameiop = DELETE;
		if ((error = relookup(fdvp, &fvp, fcnp))){
			/* relookup blew away fdvp */
			return (error);
		}
		return (VOP_REMOVE(fdvp, fvp, fcnp));
	}
	if ((error = vn_lock(fvp, LK_EXCLUSIVE)) != 0)
		goto abortit;
	dp = VTOI(fdvp);
	ip = VTOI(fvp);
	if ((nlink_t) ip->i_nlink >= LINK_MAX) {
		VOP_UNLOCK(fvp, 0);
		error = EMLINK;
		goto abortit;
	}
	if ((ip->i_flags & (IMMUTABLE | APPEND)) ||
		(dp->i_flags & APPEND)) {
		VOP_UNLOCK(fvp, 0);
		error = EPERM;
		goto abortit;
	}
	if ((ip->i_mode & IFMT) == IFDIR) {
		/*
		 * Avoid ".", "..", and aliases of "." for obvious reasons.
		 */
		if ((fcnp->cn_namelen == 1 && fcnp->cn_nameptr[0] == '.') ||
		    dp == ip ||
		    (fcnp->cn_flags & ISDOTDOT) ||
		    (tcnp->cn_flags & ISDOTDOT) ||
		    (ip->i_flag & IN_RENAME)) {
			VOP_UNLOCK(fvp, 0);
			error = EINVAL;
			goto abortit;
		}
		ip->i_flag |= IN_RENAME;
		oldparent = dp->i_number;
		doingdirectory = 1;
	}
	VN_KNOTE(fdvp, NOTE_WRITE);		/* XXXLUKEM/XXX: right place? */
	/* vrele(fdvp); */

	/*
	 * When the target exists, both the directory
	 * and target vnodes are returned locked.
	 */
	dp = VTOI(tdvp);
	xp = NULL;
	if (tvp)
		xp = VTOI(tvp);

	/*
	 * 1) Bump link count while we're moving stuff
	 *    around.  If we crash somewhere before
	 *    completing our work, the link count
	 *    may be wrong, but correctable.
	 */
	ip->i_ffs_effnlink++;
	ip->i_nlink++;
	DIP_ASSIGN(ip, nlink, ip->i_nlink);
	ip->i_flag |= IN_CHANGE;
	if (DOINGSOFTDEP(fvp))
		softdep_change_linkcnt(ip);
	if ((error = UFS_UPDATE(fvp, NULL, NULL, UPDATE_DIROP)) != 0) {
		VOP_UNLOCK(fvp, 0);
		goto bad;
	}

	/*
	 * If ".." must be changed (ie the directory gets a new
	 * parent) then the source directory must not be in the
	 * directory hierarchy above the target, as this would
	 * orphan everything below the source directory. Also
	 * the user must have write permission in the source so
	 * as to be able to change "..". We must repeat the call
	 * to namei, as the parent directory is unlocked by the
	 * call to checkpath().
	 */
	error = VOP_ACCESS(fvp, VWRITE, tcnp->cn_cred, tcnp->cn_lwp);
	VOP_UNLOCK(fvp, 0);
	if (oldparent != dp->i_number)
		newparent = dp->i_number;
	if (doingdirectory && newparent) {
		if (error)	/* write access check above */
			goto bad;
		if (xp != NULL)
			vput(tvp);
		vref(tdvp);	/* compensate for the ref checkpath looses */
		if ((error = ufs_checkpath(ip, dp, tcnp->cn_cred)) != 0) {
			vrele(tdvp);
			goto out;
		}
		tcnp->cn_flags &= ~SAVESTART;
		if ((error = relookup(tdvp, &tvp, tcnp)) != 0)
			goto out;
		dp = VTOI(tdvp);
		xp = NULL;
		if (tvp)
			xp = VTOI(tvp);
	}
	/*
	 * 2) If target doesn't exist, link the target
	 *    to the source and unlink the source.
	 *    Otherwise, rewrite the target directory
	 *    entry to reference the source inode and
	 *    expunge the original entry's existence.
	 */
	if (xp == NULL) {
		if (dp->i_dev != ip->i_dev)
			panic("rename: EXDEV");
		/*
		 * Account for ".." in new directory.
		 * When source and destination have the same
		 * parent we don't fool with the link count.
		 */
		if (doingdirectory && newparent) {
			if ((nlink_t)dp->i_nlink >= LINK_MAX) {
				error = EMLINK;
				goto bad;
			}
			dp->i_ffs_effnlink++;
			dp->i_nlink++;
			DIP_ASSIGN(dp, nlink, dp->i_nlink);
			dp->i_flag |= IN_CHANGE;
			if (DOINGSOFTDEP(tdvp))
				softdep_change_linkcnt(dp);
			if ((error = UFS_UPDATE(tdvp, NULL, NULL,
			    UPDATE_DIROP)) != 0) {
				dp->i_ffs_effnlink--;
				dp->i_nlink--;
				DIP_ASSIGN(dp, nlink, dp->i_nlink);
				dp->i_flag |= IN_CHANGE;
				if (DOINGSOFTDEP(tdvp))
					softdep_change_linkcnt(dp);
				goto bad;
			}
		}
		newdir = pool_get(&ufs_direct_pool, PR_WAITOK);
		ufs_makedirentry(ip, tcnp, newdir);
		error = ufs_direnter(tdvp, NULL, newdir, tcnp, NULL);
		pool_put(&ufs_direct_pool, newdir);
		if (error != 0) {
			if (doingdirectory && newparent) {
				dp->i_ffs_effnlink--;
				dp->i_nlink--;
				DIP_ASSIGN(dp, nlink, dp->i_nlink);
				dp->i_flag |= IN_CHANGE;
				if (DOINGSOFTDEP(tdvp))
					softdep_change_linkcnt(dp);
				(void)UFS_UPDATE(tdvp, NULL, NULL,
						 UPDATE_WAIT|UPDATE_DIROP);
			}
			goto bad;
		}
		VN_KNOTE(tdvp, NOTE_WRITE);
		vput(tdvp);
	} else {
		if (xp->i_dev != dp->i_dev || xp->i_dev != ip->i_dev)
			panic("rename: EXDEV");
		/*
		 * Short circuit rename(foo, foo).
		 */
		if (xp->i_number == ip->i_number)
			panic("rename: same file");
		/*
		 * If the parent directory is "sticky", then the user must
		 * own the parent directory, or the destination of the rename,
		 * otherwise the destination may not be changed (except by
		 * root). This implements append-only directories.
		 */
		if ((dp->i_mode & S_ISTXT) && kauth_cred_geteuid(tcnp->cn_cred) != 0 &&
		    kauth_cred_geteuid(tcnp->cn_cred) != dp->i_uid &&
		    xp->i_uid != kauth_cred_geteuid(tcnp->cn_cred)) {
			error = EPERM;
			goto bad;
		}
		/*
		 * Target must be empty if a directory and have no links
		 * to it. Also, ensure source and target are compatible
		 * (both directories, or both not directories).
		 */
		if ((xp->i_mode & IFMT) == IFDIR) {
			if (xp->i_ffs_effnlink > 2 ||
			    !ufs_dirempty(xp, dp->i_number, tcnp->cn_cred)) {
				error = ENOTEMPTY;
				goto bad;
			}
			if (!doingdirectory) {
				error = ENOTDIR;
				goto bad;
			}
			cache_purge(tdvp);
		} else if (doingdirectory) {
			error = EISDIR;
			goto bad;
		}
		if ((error = ufs_dirrewrite(dp, xp, ip->i_number,
		    IFTODT(ip->i_mode), doingdirectory && newparent ?
		    newparent : doingdirectory, IN_CHANGE | IN_UPDATE)) != 0)
			goto bad;
		if (doingdirectory) {
			if (!newparent) {
				dp->i_ffs_effnlink--;
				if (DOINGSOFTDEP(tdvp))
					softdep_change_linkcnt(dp);
			}
			xp->i_ffs_effnlink--;
			if (DOINGSOFTDEP(tvp))
				softdep_change_linkcnt(xp);
		}
		if (doingdirectory && !DOINGSOFTDEP(tvp)) {
			/*
			 * Truncate inode. The only stuff left in the directory
			 * is "." and "..". The "." reference is inconsequential
			 * since we are quashing it. We have removed the "."
			 * reference and the reference in the parent directory,
			 * but there may be other hard links. The soft
			 * dependency code will arrange to do these operations
			 * after the parent directory entry has been deleted on
			 * disk, so when running with that code we avoid doing
			 * them now.
			 */
			if (!newparent) {
				dp->i_nlink--;
				DIP_ASSIGN(dp, nlink, dp->i_nlink);
				dp->i_flag |= IN_CHANGE;
			}
			xp->i_nlink--;
			DIP_ASSIGN(xp, nlink, xp->i_nlink);
			xp->i_flag |= IN_CHANGE;
			if ((error = UFS_TRUNCATE(tvp, (off_t)0, IO_SYNC,
			    tcnp->cn_cred, tcnp->cn_lwp)))
				goto bad;
		}
		VN_KNOTE(tdvp, NOTE_WRITE);
		vput(tdvp);
		VN_KNOTE(tvp, NOTE_DELETE);
		vput(tvp);
		xp = NULL;
	}

	/*
	 * 3) Unlink the source.
	 */
	fcnp->cn_flags &= ~(MODMASK | SAVESTART);
	fcnp->cn_flags |= LOCKPARENT | LOCKLEAF;
	if ((error = relookup(fdvp, &fvp, fcnp))) {
		vrele(ap->a_fvp);
		return (error);
	}
	if (fvp != NULL) {
		xp = VTOI(fvp);
		dp = VTOI(fdvp);
	} else {
		/*
		 * From name has disappeared.
		 */
		if (doingdirectory)
			panic("rename: lost dir entry");
		vrele(ap->a_fvp);
		return (0);
	}
	/*
	 * Ensure that the directory entry still exists and has not
	 * changed while the new name has been entered. If the source is
	 * a file then the entry may have been unlinked or renamed. In
	 * either case there is no further work to be done. If the source
	 * is a directory then it cannot have been rmdir'ed; The IRENAME
	 * flag ensures that it cannot be moved by another rename or removed
	 * by a rmdir.
	 */
	if (xp != ip) {
		if (doingdirectory)
			panic("rename: lost dir entry");
	} else {
		/*
		 * If the source is a directory with a
		 * new parent, the link count of the old
		 * parent directory must be decremented
		 * and ".." set to point to the new parent.
		 */
		if (doingdirectory && newparent) {
			xp->i_offset = mastertemplate.dot_reclen;
			ufs_dirrewrite(xp, dp, newparent, DT_DIR, 0, IN_CHANGE);
			cache_purge(fdvp);
		}
		error = ufs_dirremove(fdvp, xp, fcnp->cn_flags, 0);
		xp->i_flag &= ~IN_RENAME;
	}
	VN_KNOTE(fvp, NOTE_RENAME);
	if (dp)
		vput(fdvp);
	if (xp)
		vput(fvp);
	vrele(ap->a_fvp);
	return (error);

	/* exit routines from steps 1 & 2 */
 bad:
	if (xp)
		vput(ITOV(xp));
	vput(ITOV(dp));
 out:
	if (doingdirectory)
		ip->i_flag &= ~IN_RENAME;
	if (vn_lock(fvp, LK_EXCLUSIVE) == 0) {
		ip->i_ffs_effnlink--;
		ip->i_nlink--;
		DIP_ASSIGN(ip, nlink, ip->i_nlink);
		ip->i_flag |= IN_CHANGE;
		ip->i_flag &= ~IN_RENAME;
		if (DOINGSOFTDEP(fvp))
			softdep_change_linkcnt(ip);
		vput(fvp);
	} else
		vrele(fvp);
	vrele(fdvp);
	return (error);
}

/*
 * Mkdir system call
 */
int
ufs_mkdir(void *v)
{
	struct vop_mkdir_args /* {
		struct vnode		*a_dvp;
		struct vnode		**a_vpp;
		struct componentname	*a_cnp;
		struct vattr		*a_vap;
	} */ *ap = v;
	struct vnode		*dvp = ap->a_dvp, *tvp;
	struct vattr		*vap = ap->a_vap;
	struct componentname	*cnp = ap->a_cnp;
	struct inode		*ip, *dp = VTOI(dvp);
	struct buf		*bp;
	struct dirtemplate	dirtemplate;
	struct direct		*newdir;
	int			error, dmode, blkoff;
	struct ufsmount		*ump = dp->i_ump;
	int			dirblksiz = ump->um_dirblksiz;

#ifdef DIAGNOSTIC
	if ((cnp->cn_flags & HASBUF) == 0)
		panic("ufs_mkdir: no name");
#endif
	if ((nlink_t)dp->i_nlink >= LINK_MAX) {
		error = EMLINK;
		goto out;
	}
	dmode = vap->va_mode & ACCESSPERMS;
	dmode |= IFDIR;
	/*
	 * Must simulate part of ufs_makeinode here to acquire the inode,
	 * but not have it entered in the parent directory. The entry is
	 * made later after writing "." and ".." entries.
	 */
	if ((error = UFS_VALLOC(dvp, dmode, cnp->cn_cred, ap->a_vpp)) != 0)
		goto out;
	tvp = *ap->a_vpp;
	ip = VTOI(tvp);
	ip->i_uid = kauth_cred_geteuid(cnp->cn_cred);
	DIP_ASSIGN(ip, uid, ip->i_uid);
	ip->i_gid = dp->i_gid;
	DIP_ASSIGN(ip, gid, ip->i_gid);
#ifdef QUOTA
	if ((error = getinoquota(ip)) ||
	    (error = chkiq(ip, 1, cnp->cn_cred, 0))) {
		PNBUF_PUT(cnp->cn_pnbuf);
		UFS_VFREE(tvp, ip->i_number, dmode);
		vput(tvp);
		vput(dvp);
		return (error);
	}
#endif
	ip->i_flag |= IN_ACCESS | IN_CHANGE | IN_UPDATE;
	ip->i_mode = dmode;
	DIP_ASSIGN(ip, mode, dmode);
	tvp->v_type = VDIR;	/* Rest init'd in getnewvnode(). */
	ip->i_ffs_effnlink = 2;
	ip->i_nlink = 2;
	DIP_ASSIGN(ip, nlink, 2);
	if (DOINGSOFTDEP(tvp))
		softdep_change_linkcnt(ip);
	if (cnp->cn_flags & ISWHITEOUT) {
		ip->i_flags |= UF_OPAQUE;
		DIP_ASSIGN(ip, flags, ip->i_flags);
	}

	/*
	 * Bump link count in parent directory to reflect work done below.
	 * Should be done before reference is created so cleanup is
	 * possible if we crash.
	 */
	dp->i_ffs_effnlink++;
	dp->i_nlink++;
	DIP_ASSIGN(dp, nlink, dp->i_nlink);
	dp->i_flag |= IN_CHANGE;
	if (DOINGSOFTDEP(dvp))
		softdep_change_linkcnt(dp);
	if ((error = UFS_UPDATE(dvp, NULL, NULL, UPDATE_DIROP)) != 0)
		goto bad;

	/*
	 * Initialize directory with "." and ".." from static template.
	 */
	dirtemplate = mastertemplate;
	dirtemplate.dotdot_reclen = dirblksiz - dirtemplate.dot_reclen;
	dirtemplate.dot_ino = ufs_rw32(ip->i_number, UFS_MPNEEDSWAP(ump));
	dirtemplate.dotdot_ino = ufs_rw32(dp->i_number, UFS_MPNEEDSWAP(ump));
	dirtemplate.dot_reclen = ufs_rw16(dirtemplate.dot_reclen,
	    UFS_MPNEEDSWAP(ump));
	dirtemplate.dotdot_reclen = ufs_rw16(dirtemplate.dotdot_reclen,
	    UFS_MPNEEDSWAP(ump));
	if (ump->um_maxsymlinklen <= 0) {
#if BYTE_ORDER == LITTLE_ENDIAN
		if (UFS_MPNEEDSWAP(ump) == 0)
#else
		if (UFS_MPNEEDSWAP(ump) != 0)
#endif
		{
			dirtemplate.dot_type = dirtemplate.dot_namlen;
			dirtemplate.dotdot_type = dirtemplate.dotdot_namlen;
			dirtemplate.dot_namlen = dirtemplate.dotdot_namlen = 0;
		} else
			dirtemplate.dot_type = dirtemplate.dotdot_type = 0;
	}
	if ((error = UFS_BALLOC(tvp, (off_t)0, dirblksiz, cnp->cn_cred,
	    B_CLRBUF, &bp)) != 0)
		goto bad;
	ip->i_size = dirblksiz;
	DIP_ASSIGN(ip, size, dirblksiz);
	ip->i_flag |= IN_CHANGE | IN_UPDATE;
	uvm_vnp_setsize(tvp, ip->i_size);
	memcpy((caddr_t)bp->b_data, (caddr_t)&dirtemplate, sizeof dirtemplate);
	if (DOINGSOFTDEP(tvp)) {
		/*
		 * Ensure that the entire newly allocated block is a
		 * valid directory so that future growth within the
		 * block does not have to ensure that the block is
		 * written before the inode.
		 */
		blkoff = dirblksiz;
		while (blkoff < bp->b_bcount) {
			((struct direct *)
			  (bp->b_data + blkoff))->d_reclen = dirblksiz;
			blkoff += dirblksiz;
		}
	}
	/*
	 * Directory set up, now install it's entry in the parent directory.
	 *
	 * If we are not doing soft dependencies, then we must write out the
	 * buffer containing the new directory body before entering the new
	 * name in the parent. If we are doing soft dependencies, then the
	 * buffer containing the new directory body will be passed to and
	 * released in the soft dependency code after the code has attached
	 * an appropriate ordering dependency to the buffer which ensures that
	 * the buffer is written before the new name is written in the parent.
	 */
	if (!DOINGSOFTDEP(tvp) && ((error = VOP_BWRITE(bp)) != 0))
		goto bad;
	if ((error = UFS_UPDATE(tvp, NULL, NULL, UPDATE_DIROP)) != 0) {
		if (DOINGSOFTDEP(tvp))
			(void)VOP_BWRITE(bp);
		goto bad;
	}
	newdir = pool_get(&ufs_direct_pool, PR_WAITOK);
	ufs_makedirentry(ip, cnp, newdir);
	error = ufs_direnter(dvp, tvp, newdir, cnp, bp);
	pool_put(&ufs_direct_pool, newdir);
 bad:
	if (error == 0) {
		VN_KNOTE(dvp, NOTE_WRITE | NOTE_LINK);
	} else {
		dp->i_ffs_effnlink--;
		dp->i_nlink--;
		DIP_ASSIGN(dp, nlink, dp->i_nlink);
		dp->i_flag |= IN_CHANGE;
		if (DOINGSOFTDEP(dvp))
			softdep_change_linkcnt(dp);
		/*
		 * No need to do an explicit UFS_TRUNCATE here, vrele will
		 * do this for us because we set the link count to 0.
		 */
		ip->i_ffs_effnlink = 0;
		ip->i_nlink = 0;
		DIP_ASSIGN(ip, nlink, 0);
		ip->i_flag |= IN_CHANGE;
#ifdef LFS
		/* If IN_ADIROP, account for it */
		lfs_unmark_vnode(tvp);
#endif
		if (DOINGSOFTDEP(tvp))
			softdep_change_linkcnt(ip);
		vput(tvp);
	}
 out:
	PNBUF_PUT(cnp->cn_pnbuf);
	vput(dvp);
	return (error);
}

/*
 * Rmdir system call.
 */
int
ufs_rmdir(void *v)
{
	struct vop_rmdir_args /* {
		struct vnode		*a_dvp;
		struct vnode		*a_vp;
		struct componentname	*a_cnp;
	} */ *ap = v;
	struct vnode		*vp, *dvp;
	struct componentname	*cnp;
	struct inode		*ip, *dp;
	int			error;

	vp = ap->a_vp;
	dvp = ap->a_dvp;
	cnp = ap->a_cnp;
	ip = VTOI(vp);
	dp = VTOI(dvp);
	/*
	 * No rmdir "." or of mounted directories please.
	 */
	if (dp == ip || vp->v_mountedhere != NULL) {
		vrele(dvp);
		if (vp->v_mountedhere != NULL)
			VOP_UNLOCK(dvp, 0);
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
	if (ip->i_flag & IN_RENAME) {
		error = EINVAL;
		goto out;
	}
	if (ip->i_ffs_effnlink != 2 ||
	    !ufs_dirempty(ip, dp->i_number, cnp->cn_cred)) {
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
	if (DOINGSOFTDEP(vp)) {
		dp->i_ffs_effnlink--;
		ip->i_ffs_effnlink--;
		softdep_change_linkcnt(dp);
		softdep_change_linkcnt(ip);
	}
	error = ufs_dirremove(dvp, ip, cnp->cn_flags, 1);
	if (error) {
		if (DOINGSOFTDEP(vp)) {
			dp->i_ffs_effnlink++;
			ip->i_ffs_effnlink++;
			softdep_change_linkcnt(dp);
			softdep_change_linkcnt(ip);
		}
		goto out;
	}
	VN_KNOTE(dvp, NOTE_WRITE | NOTE_LINK);
	cache_purge(dvp);
	/*
	 * Truncate inode.  The only stuff left in the directory is "." and
	 * "..".  The "." reference is inconsequential since we're quashing
	 * it. The soft dependency code will arrange to do these operations
	 * after the parent directory entry has been deleted on disk, so
	 * when running with that code we avoid doing them now.
	 */
	if (!DOINGSOFTDEP(vp)) {
		dp->i_nlink--;
		dp->i_ffs_effnlink--;
		DIP_ASSIGN(dp, nlink, dp->i_nlink);
		dp->i_flag |= IN_CHANGE;
		ip->i_nlink--;
		ip->i_ffs_effnlink--;
		DIP_ASSIGN(ip, nlink, ip->i_nlink);
		ip->i_flag |= IN_CHANGE;
		error = UFS_TRUNCATE(vp, (off_t)0, IO_SYNC, cnp->cn_cred,
		    cnp->cn_lwp);
	}
	cache_purge(vp);
#ifdef UFS_DIRHASH
	if (ip->i_dirhash != NULL)
		ufsdirhash_free(ip);
#endif
 out:
	VN_KNOTE(vp, NOTE_DELETE);
	vput(dvp);
	vput(vp);
	return (error);
}

/*
 * symlink -- make a symbolic link
 */
int
ufs_symlink(void *v)
{
	struct vop_symlink_args /* {
		struct vnode		*a_dvp;
		struct vnode		**a_vpp;
		struct componentname	*a_cnp;
		struct vattr		*a_vap;
		char			*a_target;
	} */ *ap = v;
	struct vnode	*vp, **vpp;
	struct inode	*ip;
	int		len, error;

	vpp = ap->a_vpp;
	error = ufs_makeinode(IFLNK | ap->a_vap->va_mode, ap->a_dvp,
			      vpp, ap->a_cnp);
	if (error)
		return (error);
	VN_KNOTE(ap->a_dvp, NOTE_WRITE);
	vp = *vpp;
	len = strlen(ap->a_target);
	ip = VTOI(vp);
	if (len < ip->i_ump->um_maxsymlinklen) {
		memcpy((char *)SHORTLINK(ip), ap->a_target, len);
		ip->i_size = len;
		DIP_ASSIGN(ip, size, len);
		uvm_vnp_setsize(vp, ip->i_size);
		ip->i_flag |= IN_CHANGE | IN_UPDATE;
	} else
		error = vn_rdwr(UIO_WRITE, vp, ap->a_target, len, (off_t)0,
		    UIO_SYSSPACE, IO_NODELOCKED, ap->a_cnp->cn_cred, NULL,
		    NULL);
	if (error)
		vput(vp);
	return (error);
}

/*
 * Vnode op for reading directories.
 *
 * This routine handles converting from the on-disk directory format
 * "struct direct" to the in-memory format "struct dirent" as well as
 * byte swapping the entries if necessary.
 */
int
ufs_readdir(void *v)
{
	struct vop_readdir_args /* {
		struct vnode	*a_vp;
		struct uio	*a_uio;
		kauth_cred_t	a_cred;
		int		*a_eofflag;
		off_t		**a_cookies;
		int		*ncookies;
	} */ *ap = v;
	struct vnode	*vp = ap->a_vp;
	struct direct	*cdp, *ecdp;
	struct dirent	*ndp;
	char		*cdbuf, *ndbuf, *endp;
	struct uio	auio, *uio;
	struct iovec	aiov;
	int		error;
	size_t		count, ccount, rcount;
	off_t		off, *ccp;
	struct ufsmount	*ump = VFSTOUFS(vp->v_mount);
	int nswap = UFS_MPNEEDSWAP(ump);
#if BYTE_ORDER == LITTLE_ENDIAN
	int needswap = ump->um_maxsymlinklen <= 0 && nswap == 0;
#else
	int needswap = ump->um_maxsymlinklen <= 0 && nswap != 0;
#endif
	uio = ap->a_uio;
	count = uio->uio_resid;
	rcount = count - ((uio->uio_offset + count) & (ump->um_dirblksiz - 1));

	if (rcount < _DIRENT_MINSIZE(cdp) || count < _DIRENT_MINSIZE(ndp))
		return EINVAL;

	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = uio->uio_offset;
	auio.uio_resid = rcount;
	UIO_SETUP_SYSSPACE(&auio);
	auio.uio_rw = UIO_READ;
	cdbuf = malloc(rcount, M_TEMP, M_WAITOK);
	aiov.iov_base = cdbuf;
	aiov.iov_len = rcount;
	error = VOP_READ(vp, &auio, 0, ap->a_cred);
	if (error != 0) {
		free(cdbuf, M_TEMP);
		return error;
	}

	rcount = rcount - auio.uio_resid;

	cdp = (struct direct *)(void *)cdbuf;
	ecdp = (struct direct *)(void *)&cdbuf[rcount];

	ndbuf = malloc(count, M_TEMP, M_WAITOK);
	ndp = (struct dirent *)(void *)ndbuf;
	endp = &ndbuf[count];

	off = uio->uio_offset;
	if (ap->a_cookies) {
		ccount = rcount / _DIRENT_RECLEN(cdp, 1);
		ccp = *(ap->a_cookies) = malloc(ccount * sizeof(*ccp),
		    M_TEMP, M_WAITOK);
	} else {
		/* XXX: GCC */
		ccount = 0;
		ccp = NULL;
	}

	while (cdp < ecdp) {
		cdp->d_reclen = ufs_rw16(cdp->d_reclen, nswap);
		if (cdp->d_reclen == 0) {
			struct dirent *ondp = ndp;
			ndp->d_reclen = _DIRENT_MINSIZE(ndp);
			ndp = _DIRENT_NEXT(ndp);
			ondp->d_reclen = 0;
			cdp = ecdp;
			break;
		}
		if (needswap) {
			ndp->d_type = cdp->d_namlen;
			ndp->d_namlen = cdp->d_type;
		} else {
			ndp->d_type = cdp->d_type;
			ndp->d_namlen = cdp->d_namlen;
		}
		ndp->d_reclen = _DIRENT_RECLEN(ndp, ndp->d_namlen);
		if ((char *)(void *)ndp + ndp->d_reclen +
		    _DIRENT_MINSIZE(ndp) > endp)
			break;
		ndp->d_fileno = ufs_rw32(cdp->d_ino, nswap);
		(void)memcpy(ndp->d_name, cdp->d_name, ndp->d_namlen);
		memset(&ndp->d_name[ndp->d_namlen], 0,
		    ndp->d_reclen - _DIRENT_NAMEOFF(ndp) - ndp->d_namlen);
		off += cdp->d_reclen;
		if (ap->a_cookies) {
			KASSERT(ccp - *(ap->a_cookies) < ccount);
			*(ccp++) = off;
		}
		ndp = _DIRENT_NEXT(ndp);
		cdp = _DIRENT_NEXT(cdp);
	}

	if (cdp >= ecdp)
		off = uio->uio_offset + rcount;

	count = ((char *)(void *)ndp - ndbuf);
	error = uiomove(ndbuf, count, uio);

	if (ap->a_cookies) {
		if (error)
			free(*(ap->a_cookies), M_TEMP);
		else
			*ap->a_ncookies = ccp - *(ap->a_cookies);
	}
	uio->uio_offset = off;
	free(ndbuf, M_TEMP);
	free(cdbuf, M_TEMP);
	*ap->a_eofflag = VTOI(vp)->i_size <= uio->uio_offset;
	return error;
}

/*
 * Return target name of a symbolic link
 */
int
ufs_readlink(void *v)
{
	struct vop_readlink_args /* {
		struct vnode	*a_vp;
		struct uio	*a_uio;
		kauth_cred_t	a_cred;
	} */ *ap = v;
	struct vnode	*vp = ap->a_vp;
	struct inode	*ip = VTOI(vp);
	struct ufsmount	*ump = VFSTOUFS(vp->v_mount);
	int		isize;

	isize = ip->i_size;
	if (isize < ump->um_maxsymlinklen ||
	    (ump->um_maxsymlinklen == 0 && DIP(ip, blocks) == 0)) {
		uiomove((char *)SHORTLINK(ip), isize, ap->a_uio);
		return (0);
	}
	return (VOP_READ(vp, ap->a_uio, 0, ap->a_cred));
}

/*
 * Calculate the logical to physical mapping if not done already,
 * then call the device strategy routine.
 */
int
ufs_strategy(void *v)
{
	struct vop_strategy_args /* {
		struct vnode *a_vp;
		struct buf *a_bp;
	} */ *ap = v;
	struct buf	*bp;
	struct vnode	*vp;
	struct inode	*ip;
	int		error;

	bp = ap->a_bp;
	vp = ap->a_vp;
	ip = VTOI(vp);
	if (vp->v_type == VBLK || vp->v_type == VCHR)
		panic("ufs_strategy: spec");
	KASSERT(bp->b_bcount != 0);
	if (bp->b_blkno == bp->b_lblkno) {
		error = VOP_BMAP(vp, bp->b_lblkno, NULL, &bp->b_blkno,
				 NULL);
		if (error) {
			bp->b_error = error;
			bp->b_flags |= B_ERROR;
			biodone(bp);
			return (error);
		}
		if (bp->b_blkno == -1) /* no valid data */
			clrbuf(bp);
	}
	if (bp->b_blkno < 0) { /* block is not on disk */
		biodone(bp);
		return (0);
	}
	vp = ip->i_devvp;
	return (VOP_STRATEGY(vp, bp));
}

/*
 * Print out the contents of an inode.
 */
int
ufs_print(void *v)
{
	struct vop_print_args /* {
		struct vnode	*a_vp;
	} */ *ap = v;
	struct vnode	*vp;
	struct inode	*ip;

	vp = ap->a_vp;
	ip = VTOI(vp);
	printf("tag VT_UFS, ino %llu, on dev %d, %d",
	    (unsigned long long)ip->i_number,
	    major(ip->i_dev), minor(ip->i_dev));
	printf(" flags 0x%x, effnlink %d, nlink %d\n",
	    ip->i_flag, ip->i_ffs_effnlink, ip->i_nlink);
	printf("\tmode 0%o, owner %d, group %d, size %qd",
	    ip->i_mode, ip->i_uid, ip->i_gid,
	    (long long)ip->i_size);
	if (vp->v_type == VFIFO)
		fifo_printinfo(vp);
	lockmgr_printinfo(&vp->v_lock);
	printf("\n");
	return (0);
}

/*
 * Read wrapper for special devices.
 */
int
ufsspec_read(void *v)
{
	struct vop_read_args /* {
		struct vnode	*a_vp;
		struct uio	*a_uio;
		int		a_ioflag;
		kauth_cred_t	a_cred;
	} */ *ap = v;

	/*
	 * Set access flag.
	 */
	if ((ap->a_vp->v_mount->mnt_flag & MNT_NODEVMTIME) == 0)
		VTOI(ap->a_vp)->i_flag |= IN_ACCESS;
	return (VOCALL (spec_vnodeop_p, VOFFSET(vop_read), ap));
}

/*
 * Write wrapper for special devices.
 */
int
ufsspec_write(void *v)
{
	struct vop_write_args /* {
		struct vnode	*a_vp;
		struct uio	*a_uio;
		int		a_ioflag;
		kauth_cred_t	a_cred;
	} */ *ap = v;

	/*
	 * Set update and change flags.
	 */
	if ((ap->a_vp->v_mount->mnt_flag & MNT_NODEVMTIME) == 0)
		VTOI(ap->a_vp)->i_flag |= IN_MODIFY;
	return (VOCALL (spec_vnodeop_p, VOFFSET(vop_write), ap));
}

/*
 * Close wrapper for special devices.
 *
 * Update the times on the inode then do device close.
 */
int
ufsspec_close(void *v)
{
	struct vop_close_args /* {
		struct vnode	*a_vp;
		int		a_fflag;
		kauth_cred_t	a_cred;
		struct lwp	*a_l;
	} */ *ap = v;
	struct vnode	*vp;
	struct inode	*ip;

	vp = ap->a_vp;
	ip = VTOI(vp);
	simple_lock(&vp->v_interlock);
	if (vp->v_usecount > 1)
		UFS_ITIMES(vp, NULL, NULL, NULL);
	simple_unlock(&vp->v_interlock);
	return (VOCALL (spec_vnodeop_p, VOFFSET(vop_close), ap));
}

/*
 * Read wrapper for fifo's
 */
int
ufsfifo_read(void *v)
{
	struct vop_read_args /* {
		struct vnode	*a_vp;
		struct uio	*a_uio;
		int		a_ioflag;
		kauth_cred_t	a_cred;
	} */ *ap = v;

	/*
	 * Set access flag.
	 */
	VTOI(ap->a_vp)->i_flag |= IN_ACCESS;
	return (VOCALL (fifo_vnodeop_p, VOFFSET(vop_read), ap));
}

/*
 * Write wrapper for fifo's.
 */
int
ufsfifo_write(void *v)
{
	struct vop_write_args /* {
		struct vnode	*a_vp;
		struct uio	*a_uio;
		int		a_ioflag;
		kauth_cred_t	a_cred;
	} */ *ap = v;

	/*
	 * Set update and change flags.
	 */
	VTOI(ap->a_vp)->i_flag |= IN_MODIFY;
	return (VOCALL (fifo_vnodeop_p, VOFFSET(vop_write), ap));
}

/*
 * Close wrapper for fifo's.
 *
 * Update the times on the inode then do device close.
 */
int
ufsfifo_close(void *v)
{
	struct vop_close_args /* {
		struct vnode	*a_vp;
		int		a_fflag;
		kauth_cred_t	a_cred;
		struct lwp	*a_l;
	} */ *ap = v;
	struct vnode	*vp;
	struct inode	*ip;

	vp = ap->a_vp;
	ip = VTOI(vp);
	simple_lock(&vp->v_interlock);
	if (ap->a_vp->v_usecount > 1)
		UFS_ITIMES(vp, NULL, NULL, NULL);
	simple_unlock(&vp->v_interlock);
	return (VOCALL (fifo_vnodeop_p, VOFFSET(vop_close), ap));
}

/*
 * Return POSIX pathconf information applicable to ufs filesystems.
 */
int
ufs_pathconf(void *v)
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
		*ap->a_retval = NAME_MAX;
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
	default:
		return (EINVAL);
	}
	/* NOTREACHED */
}

/*
 * Advisory record locking support
 */
int
ufs_advlock(void *v)
{
	struct vop_advlock_args /* {
		struct vnode	*a_vp;
		caddr_t		a_id;
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
ufs_vinit(struct mount *mntp, int (**specops)(void *), int (**fifoops)(void *),
	struct vnode **vpp)
{
	struct inode	*ip;
	struct vnode	*vp, *nvp;
	dev_t		rdev;
	struct ufsmount	*ump;

	vp = *vpp;
	ip = VTOI(vp);
	switch(vp->v_type = IFTOVT(ip->i_mode)) {
	case VCHR:
	case VBLK:
		vp->v_op = specops;
		ump = ip->i_ump;
		if (ump->um_fstype == UFS1)
			rdev = (dev_t)ufs_rw32(ip->i_ffs1_rdev,
			    UFS_MPNEEDSWAP(ump));
		else
			rdev = (dev_t)ufs_rw64(ip->i_ffs2_rdev,
			    UFS_MPNEEDSWAP(ump));
		if ((nvp = checkalias(vp, rdev, mntp)) != NULL) {
			/*
			 * Discard unneeded vnode, but save its inode.
			 */
			nvp->v_data = vp->v_data;
			vp->v_data = NULL;
			/* XXX spec_vnodeops has no locking, do it explicitly */
			VOP_UNLOCK(vp, 0);
			vp->v_op = spec_vnodeop_p;
			vp->v_flag &= ~VLOCKSWORK;
			vrele(vp);
			vgone(vp);
			lockmgr(&nvp->v_lock, LK_EXCLUSIVE, &nvp->v_interlock);
			/*
			 * Reinitialize aliased inode.
			 */
			vp = nvp;
			ip->i_vnode = vp;
		}
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
	if (ip->i_number == ROOTINO)
                vp->v_flag |= VROOT;
	/*
	 * Initialize modrev times
	 */
	ip->i_modrev = (uint64_t)(uint)mono_time.tv_sec << 32
			| mono_time.tv_usec * 4294u;
	*vpp = vp;
}

/*
 * Allocate a new inode.
 */
int
ufs_makeinode(int mode, struct vnode *dvp, struct vnode **vpp,
	struct componentname *cnp)
{
	struct inode	*ip, *pdir;
	struct direct	*newdir;
	struct vnode	*tvp;
	int		error, ismember = 0;

	pdir = VTOI(dvp);
#ifdef DIAGNOSTIC
	if ((cnp->cn_flags & HASBUF) == 0)
		panic("ufs_makeinode: no name");
#endif
	if ((mode & IFMT) == 0)
		mode |= IFREG;

	if ((error = UFS_VALLOC(dvp, mode, cnp->cn_cred, vpp)) != 0) {
		PNBUF_PUT(cnp->cn_pnbuf);
		vput(dvp);
		return (error);
	}
	tvp = *vpp;
	ip = VTOI(tvp);
	ip->i_gid = pdir->i_gid;
	DIP_ASSIGN(ip, gid, ip->i_gid);
	ip->i_uid = kauth_cred_geteuid(cnp->cn_cred);
	DIP_ASSIGN(ip, uid, ip->i_uid);
#ifdef QUOTA
	if ((error = getinoquota(ip)) ||
	    (error = chkiq(ip, 1, cnp->cn_cred, 0))) {
		UFS_VFREE(tvp, ip->i_number, mode);
		vput(tvp);
		PNBUF_PUT(cnp->cn_pnbuf);
		vput(dvp);
		return (error);
	}
#endif
	ip->i_flag |= IN_ACCESS | IN_CHANGE | IN_UPDATE;
	ip->i_mode = mode;
	DIP_ASSIGN(ip, mode, mode);
	tvp->v_type = IFTOVT(mode);	/* Rest init'd in getnewvnode(). */
	ip->i_ffs_effnlink = 1;
	ip->i_nlink = 1;
	DIP_ASSIGN(ip, nlink, 1);
	if (DOINGSOFTDEP(tvp))
		softdep_change_linkcnt(ip);
	if ((ip->i_mode & ISGID) && (kauth_cred_ismember_gid(cnp->cn_cred,
	    ip->i_gid, &ismember) != 0 || !ismember) &&
	    kauth_authorize_generic(cnp->cn_cred, KAUTH_GENERIC_ISSUSER, NULL)) {
		ip->i_mode &= ~ISGID;
		DIP_ASSIGN(ip, mode, ip->i_mode);
	}

	if (cnp->cn_flags & ISWHITEOUT) {
		ip->i_flags |= UF_OPAQUE;
		DIP_ASSIGN(ip, flags, ip->i_flags);
	}

	/*
	 * Make sure inode goes to disk before directory entry.
	 */
	if ((error = UFS_UPDATE(tvp, NULL, NULL, UPDATE_DIROP)) != 0)
		goto bad;
	newdir = pool_get(&ufs_direct_pool, PR_WAITOK);
	ufs_makedirentry(ip, cnp, newdir);
	error = ufs_direnter(dvp, tvp, newdir, cnp, NULL);
	pool_put(&ufs_direct_pool, newdir);
	if (error)
		goto bad;
	if ((cnp->cn_flags & SAVESTART) == 0)
		PNBUF_PUT(cnp->cn_pnbuf);
	vput(dvp);
	*vpp = tvp;
	return (0);

 bad:
	/*
	 * Write error occurred trying to update the inode
	 * or the directory so must deallocate the inode.
	 */
	ip->i_ffs_effnlink = 0;
	ip->i_nlink = 0;
	DIP_ASSIGN(ip, nlink, 0);
	ip->i_flag |= IN_CHANGE;
#ifdef LFS
	/* If IN_ADIROP, account for it */
	lfs_unmark_vnode(tvp);
#endif
	if (DOINGSOFTDEP(tvp))
		softdep_change_linkcnt(ip);
	tvp->v_type = VNON;		/* explodes later if VBLK */
	vput(tvp);
	PNBUF_PUT(cnp->cn_pnbuf);
	vput(dvp);
	return (error);
}

/*
 * Allocate len bytes at offset off.
 */
int
ufs_gop_alloc(struct vnode *vp, off_t off, off_t len, int flags,
    kauth_cred_t cred)
{
        struct inode *ip = VTOI(vp);
        int error, delta, bshift, bsize;
        UVMHIST_FUNC("ufs_gop_alloc"); UVMHIST_CALLED(ubchist);

        error = 0;
        bshift = vp->v_mount->mnt_fs_bshift;
        bsize = 1 << bshift;

        delta = off & (bsize - 1);
        off -= delta;
        len += delta;

        while (len > 0) {
                bsize = MIN(bsize, len);

                error = UFS_BALLOC(vp, off, bsize, cred, flags, NULL);
                if (error) {
                        goto out;
                }

                /*
                 * increase file size now, UFS_BALLOC() requires that
                 * EOF be up-to-date before each call.
                 */

                if (ip->i_size < off + bsize) {
                        UVMHIST_LOG(ubchist, "vp %p old 0x%x new 0x%x",
                            vp, ip->i_size, off + bsize, 0);
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
ufs_gop_markupdate(struct vnode *vp, int flags)
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

		ip->i_flag |= mask;
	}
}
