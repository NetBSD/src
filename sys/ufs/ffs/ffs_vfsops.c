/*	$NetBSD: ffs_vfsops.c,v 1.81 2001/05/30 11:57:18 mrg Exp $	*/

/*
 * Copyright (c) 1989, 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)ffs_vfsops.c	8.31 (Berkeley) 5/20/95
 */

#if defined(_KERNEL_OPT)
#include "opt_ffs.h"
#include "opt_quota.h"
#include "opt_compat_netbsd.h"
#include "opt_softdep.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/socket.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/mbuf.h>
#include <sys/file.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/lock.h>
#include <sys/sysctl.h>

#include <miscfs/specfs/specdev.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/ufs_extern.h>
#include <ufs/ufs/ufs_bswap.h>

#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

/* how many times ffs_init() was called */
int ffs_initcount = 0;

extern struct lock ufs_hashlock;

extern struct vnodeopv_desc ffs_vnodeop_opv_desc;
extern struct vnodeopv_desc ffs_specop_opv_desc;
extern struct vnodeopv_desc ffs_fifoop_opv_desc;

const struct vnodeopv_desc * const ffs_vnodeopv_descs[] = {
	&ffs_vnodeop_opv_desc,
	&ffs_specop_opv_desc,
	&ffs_fifoop_opv_desc,
	NULL,
};

struct vfsops ffs_vfsops = {
	MOUNT_FFS,
	ffs_mount,
	ufs_start,
	ffs_unmount,
	ufs_root,
	ufs_quotactl,
	ffs_statfs,
	ffs_sync,
	ffs_vget,
	ffs_fhtovp,
	ffs_vptofh,
	ffs_init,
	ffs_done,
	ffs_sysctl,
	ffs_mountroot,
	ufs_check_export,
	ffs_vnodeopv_descs,
};

struct pool ffs_inode_pool;

/*
 * Called by main() when ffs is going to be mounted as root.
 */

int
ffs_mountroot()
{
	struct fs *fs;
	struct mount *mp;
	struct proc *p = curproc;	/* XXX */
	struct ufsmount *ump;
	int error;

	if (root_device->dv_class != DV_DISK)
		return (ENODEV);

	/*
	 * Get vnodes for rootdev.
	 */
	if (bdevvp(rootdev, &rootvp))
		panic("ffs_mountroot: can't setup bdevvp's");

	if ((error = vfs_rootmountalloc(MOUNT_FFS, "root_device", &mp))) {
		vrele(rootvp);
		return (error);
	}
	if ((error = ffs_mountfs(rootvp, mp, p)) != 0) {
		mp->mnt_op->vfs_refcount--;
		vfs_unbusy(mp);
		free(mp, M_MOUNT);
		vrele(rootvp);
		return (error);
	}
	simple_lock(&mountlist_slock);
	CIRCLEQ_INSERT_TAIL(&mountlist, mp, mnt_list);
	simple_unlock(&mountlist_slock);
	ump = VFSTOUFS(mp);
	fs = ump->um_fs;
	memset(fs->fs_fsmnt, 0, sizeof(fs->fs_fsmnt));
	(void)copystr(mp->mnt_stat.f_mntonname, fs->fs_fsmnt, MNAMELEN - 1, 0);
	(void)ffs_statfs(mp, &mp->mnt_stat, p);
	vfs_unbusy(mp);
	inittodr(fs->fs_time);
	return (0);
}

/*
 * VFS Operations.
 *
 * mount system call
 */
int
ffs_mount(mp, path, data, ndp, p)
	struct mount *mp;
	const char *path;
	void *data;
	struct nameidata *ndp;
	struct proc *p;
{
	struct vnode *devvp;
	struct ufs_args args;
	struct ufsmount *ump = NULL;
	struct fs *fs;
	size_t size;
	int error, flags;
	mode_t accessmode;

	error = copyin(data, (caddr_t)&args, sizeof (struct ufs_args));
	if (error)
		return (error);

#if !defined(SOFTDEP)
	mp->mnt_flag &= ~MNT_SOFTDEP;
#endif

	/*
	 * If updating, check whether changing from read-only to
	 * read/write; if there is no device name, that's all we do.
	 */
	if (mp->mnt_flag & MNT_UPDATE) {
		ump = VFSTOUFS(mp);
		fs = ump->um_fs;
		if (fs->fs_ronly == 0 && (mp->mnt_flag & MNT_RDONLY)) {
			flags = WRITECLOSE;
			if (mp->mnt_flag & MNT_FORCE)
				flags |= FORCECLOSE;
			if (mp->mnt_flag & MNT_SOFTDEP)
				error = softdep_flushfiles(mp, flags, p);
			else
				error = ffs_flushfiles(mp, flags, p);
			if (error == 0 &&
			    ffs_cgupdate(ump, MNT_WAIT) == 0 &&
			    fs->fs_clean & FS_WASCLEAN) {
				if (mp->mnt_flag & MNT_SOFTDEP)
					fs->fs_flags &= ~FS_DOSOFTDEP;
				fs->fs_clean = FS_ISCLEAN;
				(void) ffs_sbupdate(ump, MNT_WAIT);
			}
			if (error)
				return (error);
			fs->fs_ronly = 1;
			fs->fs_fmod = 0;
		}

		/*
		 * Flush soft dependencies if disabling it via an update
		 * mount. This may leave some items to be processed,
		 * so don't do this yet XXX.
		 */
		if ((fs->fs_flags & FS_DOSOFTDEP) &&
		    !(mp->mnt_flag & MNT_SOFTDEP) && fs->fs_ronly == 0) {
#ifdef notyet
			flags = WRITECLOSE;
			if (mp->mnt_flag & MNT_FORCE)
				flags |= FORCECLOSE;
			error = softdep_flushfiles(mp, flags, p);
			if (error == 0 && ffs_cgupdate(ump, MNT_WAIT) == 0)
				fs->fs_flags &= ~FS_DOSOFTDEP;
				(void) ffs_sbupdate(ump, MNT_WAIT);
#elif defined(SOFTDEP)
			mp->mnt_flag |= MNT_SOFTDEP;
#endif
		}

		/*
		 * When upgrading to a softdep mount, we must first flush
		 * all vnodes. (not done yet -- see above)
		 */
		if (!(fs->fs_flags & FS_DOSOFTDEP) &&
		    (mp->mnt_flag & MNT_SOFTDEP) && fs->fs_ronly == 0) {
#ifdef notyet
			flags = WRITECLOSE;
			if (mp->mnt_flag & MNT_FORCE)
				flags |= FORCECLOSE;
			error = ffs_flushfiles(mp, flags, p);
#else
			mp->mnt_flag &= ~MNT_SOFTDEP;
#endif
		}

		if (mp->mnt_flag & MNT_RELOAD) {
			error = ffs_reload(mp, ndp->ni_cnd.cn_cred, p);
			if (error)
				return (error);
		}
		if (fs->fs_ronly && (mp->mnt_flag & MNT_WANTRDWR)) {
			/*
			 * If upgrade to read-write by non-root, then verify
			 * that user has necessary permissions on the device.
			 */
			devvp = ump->um_devvp;
			if (p->p_ucred->cr_uid != 0) {
				vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
				error = VOP_ACCESS(devvp, VREAD | VWRITE,
						   p->p_ucred, p);
				VOP_UNLOCK(devvp, 0);
				if (error)
					return (error);
			}
			fs->fs_ronly = 0;
			fs->fs_clean <<= 1;
			fs->fs_fmod = 1;
			if ((fs->fs_flags & FS_DOSOFTDEP)) {
				error = softdep_mount(devvp, mp, fs,
				    p->p_ucred);
				if (error)
					return (error);
			}
		}
		if (args.fspec == 0) {
			/*
			 * Process export requests.
			 */
			return (vfs_export(mp, &ump->um_export, &args.export));
		}
		if ((mp->mnt_flag & (MNT_SOFTDEP | MNT_ASYNC)) ==
		    (MNT_SOFTDEP | MNT_ASYNC)) {
			printf("%s fs uses soft updates, ignoring async mode\n",
			    fs->fs_fsmnt);
			mp->mnt_flag &= ~MNT_ASYNC;
		}
	}
	/*
	 * Not an update, or updating the name: look up the name
	 * and verify that it refers to a sensible block device.
	 */
	NDINIT(ndp, LOOKUP, FOLLOW, UIO_USERSPACE, args.fspec, p);
	if ((error = namei(ndp)) != 0)
		return (error);
	devvp = ndp->ni_vp;

	if (devvp->v_type != VBLK) {
		vrele(devvp);
		return (ENOTBLK);
	}
	if (major(devvp->v_rdev) >= nblkdev) {
		vrele(devvp);
		return (ENXIO);
	}
	/*
	 * If mount by non-root, then verify that user has necessary
	 * permissions on the device.
	 */
	if (p->p_ucred->cr_uid != 0) {
		accessmode = VREAD;
		if ((mp->mnt_flag & MNT_RDONLY) == 0)
			accessmode |= VWRITE;
		vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
		error = VOP_ACCESS(devvp, accessmode, p->p_ucred, p);
		VOP_UNLOCK(devvp, 0);
		if (error) {
			vrele(devvp);
			return (error);
		}
	}
	if ((mp->mnt_flag & MNT_UPDATE) == 0) {
		error = ffs_mountfs(devvp, mp, p);
		if (!error) {
			ump = VFSTOUFS(mp);
			fs = ump->um_fs;
			if ((mp->mnt_flag & (MNT_SOFTDEP | MNT_ASYNC)) ==
			    (MNT_SOFTDEP | MNT_ASYNC)) {
				printf("%s fs uses soft updates, "
				       "ignoring async mode\n",
				    fs->fs_fsmnt);
				mp->mnt_flag &= ~MNT_ASYNC;
			}
		}
	}
	else {
		if (devvp != ump->um_devvp)
			error = EINVAL;	/* needs translation */
		else
			vrele(devvp);
	}
	if (error) {
		vrele(devvp);
		return (error);
	}
	(void) copyinstr(path, fs->fs_fsmnt, sizeof(fs->fs_fsmnt) - 1, &size);
	memset(fs->fs_fsmnt + size, 0, sizeof(fs->fs_fsmnt) - size);
	memcpy(mp->mnt_stat.f_mntonname, fs->fs_fsmnt, MNAMELEN);
	(void) copyinstr(args.fspec, mp->mnt_stat.f_mntfromname, MNAMELEN - 1, 
	    &size);
	memset(mp->mnt_stat.f_mntfromname + size, 0, MNAMELEN - size);
	if (mp->mnt_flag & MNT_SOFTDEP)
		fs->fs_flags |= FS_DOSOFTDEP;
	else
		fs->fs_flags &= ~FS_DOSOFTDEP;
	if (fs->fs_fmod != 0) {	/* XXX */
		fs->fs_fmod = 0;
		if (fs->fs_clean & FS_WASCLEAN)
			fs->fs_time = time.tv_sec;
		else
			printf("%s: file system not clean (fs_flags=%x); please fsck(8)\n",
			    mp->mnt_stat.f_mntfromname, fs->fs_clean);
		(void) ffs_cgupdate(ump, MNT_WAIT);
	}
	return (0);
}

/*
 * Reload all incore data for a filesystem (used after running fsck on
 * the root filesystem and finding things to fix). The filesystem must
 * be mounted read-only.
 *
 * Things to do to update the mount:
 *	1) invalidate all cached meta-data.
 *	2) re-read superblock from disk.
 *	3) re-read summary information from disk.
 *	4) invalidate all inactive vnodes.
 *	5) invalidate all cached file data.
 *	6) re-read inode data for all active vnodes.
 */
int
ffs_reload(mountp, cred, p)
	struct mount *mountp;
	struct ucred *cred;
	struct proc *p;
{
	struct vnode *vp, *nvp, *devvp;
	struct inode *ip;
	struct buf *bp;
	struct fs *fs, *newfs;
	struct partinfo dpart;
	int i, blks, size, error;
	int32_t *lp;
	caddr_t cp;

	if ((mountp->mnt_flag & MNT_RDONLY) == 0)
		return (EINVAL);
	/*
	 * Step 1: invalidate all cached meta-data.
	 */
	devvp = VFSTOUFS(mountp)->um_devvp;
	vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
	error = vinvalbuf(devvp, 0, cred, p, 0, 0);
	VOP_UNLOCK(devvp, 0);
	if (error)
		panic("ffs_reload: dirty1");
	/*
	 * Step 2: re-read superblock from disk.
	 */
	if (VOP_IOCTL(devvp, DIOCGPART, (caddr_t)&dpart, FREAD, NOCRED, p) != 0)
		size = DEV_BSIZE;
	else
		size = dpart.disklab->d_secsize;
	error = bread(devvp, (ufs_daddr_t)(SBOFF / size), SBSIZE, NOCRED, &bp);
	if (error) {
		brelse(bp);
		return (error);
	}
	fs = VFSTOUFS(mountp)->um_fs;
	newfs = malloc(fs->fs_sbsize, M_UFSMNT, M_WAITOK);
	memcpy(newfs, bp->b_data, fs->fs_sbsize);
#ifdef FFS_EI
	if (VFSTOUFS(mountp)->um_flags & UFS_NEEDSWAP) {
		ffs_sb_swap((struct fs*)bp->b_data, newfs, 0);
		fs->fs_flags |= FS_SWAPPED;
	}
#endif
	if (newfs->fs_magic != FS_MAGIC || newfs->fs_bsize > MAXBSIZE ||
	    newfs->fs_bsize < sizeof(struct fs)) {
		brelse(bp);
		free(newfs, M_UFSMNT);
		return (EIO);		/* XXX needs translation */
	}
	/* 
	 * Copy pointer fields back into superblock before copying in	XXX
	 * new superblock. These should really be in the ufsmount.	XXX
	 * Note that important parameters (eg fs_ncg) are unchanged.
	 */
	memcpy(&newfs->fs_csp[0], &fs->fs_csp[0], sizeof(fs->fs_csp));
	newfs->fs_maxcluster = fs->fs_maxcluster;
	newfs->fs_ronly = fs->fs_ronly;
	memcpy(fs, newfs, (u_int)fs->fs_sbsize);
	if (fs->fs_sbsize < SBSIZE)
		bp->b_flags |= B_INVAL;
	brelse(bp);
	free(newfs, M_UFSMNT);
	mountp->mnt_maxsymlinklen = fs->fs_maxsymlinklen;
	ffs_oldfscompat(fs);
	ffs_statfs(mountp, &mountp->mnt_stat, p);
	/*
	 * Step 3: re-read summary information from disk.
	 */
	blks = howmany(fs->fs_cssize, fs->fs_fsize);
	for (i = 0; i < blks; i += fs->fs_frag) {
		size = fs->fs_bsize;
		if (i + fs->fs_frag > blks)
			size = (blks - i) * fs->fs_fsize;
		error = bread(devvp, fsbtodb(fs, fs->fs_csaddr + i), size,
			      NOCRED, &bp);
		if (error) {
			brelse(bp);
			return (error);
		}
#ifdef FFS_EI
		if (UFS_FSNEEDSWAP(fs))
			ffs_csum_swap((struct csum*)bp->b_data,
			    (struct csum*)fs->fs_csp[fragstoblks(fs, i)], size);
		else
#endif
			memcpy(fs->fs_csp[fragstoblks(fs, i)], bp->b_data,
			    (size_t)size);
		brelse(bp);
	}
	if ((fs->fs_flags & FS_DOSOFTDEP))
		softdep_mount(devvp, mountp, fs, cred);
	/*
	 * We no longer know anything about clusters per cylinder group.
	 */
	if (fs->fs_contigsumsize > 0) {
		lp = fs->fs_maxcluster;
		for (i = 0; i < fs->fs_ncg; i++)
			*lp++ = fs->fs_contigsumsize;
	}

loop:
	simple_lock(&mntvnode_slock);
	for (vp = mountp->mnt_vnodelist.lh_first; vp != NULL; vp = nvp) {
		if (vp->v_mount != mountp) {
			simple_unlock(&mntvnode_slock);
			goto loop;
		}
		nvp = vp->v_mntvnodes.le_next;
		/*
		 * Step 4: invalidate all inactive vnodes.
		 */
		if (vrecycle(vp, &mntvnode_slock, p))
			goto loop;
		/*
		 * Step 5: invalidate all cached file data.
		 */
		simple_lock(&vp->v_interlock);
		simple_unlock(&mntvnode_slock);
		if (vget(vp, LK_EXCLUSIVE | LK_INTERLOCK))
			goto loop;
		if (vinvalbuf(vp, 0, cred, p, 0, 0))
			panic("ffs_reload: dirty2");
		/*
		 * Step 6: re-read inode data for all active vnodes.
		 */
		ip = VTOI(vp);
		error = bread(devvp, fsbtodb(fs, ino_to_fsba(fs, ip->i_number)),
			      (int)fs->fs_bsize, NOCRED, &bp);
		if (error) {
			brelse(bp);
			vput(vp);
			return (error);
		}
		cp = (caddr_t)bp->b_data +
		    (ino_to_fsbo(fs, ip->i_number) * DINODE_SIZE);
#ifdef FFS_EI
		if (UFS_FSNEEDSWAP(fs))
			ffs_dinode_swap((struct dinode *)cp,
			    &ip->i_din.ffs_din);
		else
#endif
			memcpy(&ip->i_din.ffs_din, cp, DINODE_SIZE);
		ip->i_ffs_effnlink = ip->i_ffs_nlink;
		brelse(bp);
		vput(vp);
		simple_lock(&mntvnode_slock);
	}
	simple_unlock(&mntvnode_slock);
	return (0);
}

/*
 * Common code for mount and mountroot
 */
int
ffs_mountfs(devvp, mp, p)
	struct vnode *devvp;
	struct mount *mp;
	struct proc *p;
{
	struct ufsmount *ump;
	struct buf *bp;
	struct fs *fs;
	dev_t dev;
	struct partinfo dpart;
	caddr_t base, space;
	int blks;
	int error, i, size, ronly;
#ifdef FFS_EI
	int needswap;
#endif
	int32_t *lp;
	struct ucred *cred;
	u_int64_t maxfilesize;					/* XXX */
	u_int32_t sbsize;

	dev = devvp->v_rdev;
	cred = p ? p->p_ucred : NOCRED;
	/*
	 * Disallow multiple mounts of the same device.
	 * Disallow mounting of a device that is currently in use
	 * (except for root, which might share swap device for miniroot).
	 * Flush out any old buffers remaining from a previous use.
	 */
	if ((error = vfs_mountedon(devvp)) != 0)
		return (error);
	if (vcount(devvp) > 1 && devvp != rootvp)
		return (EBUSY);
	vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
	error = vinvalbuf(devvp, V_SAVE, cred, p, 0, 0);
	VOP_UNLOCK(devvp, 0);
	if (error)
		return (error);

	ronly = (mp->mnt_flag & MNT_RDONLY) != 0;
	error = VOP_OPEN(devvp, ronly ? FREAD : FREAD|FWRITE, FSCRED, p);
	if (error)
		return (error);
	if (VOP_IOCTL(devvp, DIOCGPART, (caddr_t)&dpart, FREAD, cred, p) != 0)
		size = DEV_BSIZE;
	else
		size = dpart.disklab->d_secsize;

	bp = NULL;
	ump = NULL;
	error = bread(devvp, (ufs_daddr_t)(SBOFF / size), SBSIZE, cred, &bp);
	if (error)
		goto out;

	fs = (struct fs*)bp->b_data;
	if (fs->fs_magic == FS_MAGIC) {
		sbsize = fs->fs_sbsize;
#ifdef FFS_EI
		needswap = 0;
	} else if (fs->fs_magic == bswap32(FS_MAGIC)) {
		sbsize = bswap32(fs->fs_sbsize);
		needswap = 1;
#endif
	} else {
		error = EINVAL;
		goto out;
	}
	if (sbsize > MAXBSIZE || sbsize < sizeof(struct fs)) {
		error = EINVAL;
		goto out;
	}

	fs = malloc((u_long)sbsize, M_UFSMNT, M_WAITOK);
	memcpy(fs, bp->b_data, sbsize);
#ifdef FFS_EI
	if (needswap) {
		ffs_sb_swap((struct fs*)bp->b_data, fs, 0);
		fs->fs_flags |= FS_SWAPPED;
	}
#endif
	ffs_oldfscompat(fs);

	if (fs->fs_bsize > MAXBSIZE || fs->fs_bsize < sizeof(struct fs)) {
		error = EINVAL;
		goto out;
	}
	 /* make sure cylinder group summary area is a reasonable size. */
	if (fs->fs_cgsize == 0 || fs->fs_cpg == 0 ||
	    fs->fs_ncg > fs->fs_ncyl / fs->fs_cpg + 1 ||
	    fs->fs_cssize >
	    fragroundup(fs, fs->fs_ncg * sizeof(struct csum))) {
		error = EINVAL;		/* XXX needs translation */
		goto out2;
	}
	/* XXX updating 4.2 FFS superblocks trashes rotational layout tables */
	if (fs->fs_postblformat == FS_42POSTBLFMT && !ronly) {
		error = EROFS;		/* XXX what should be returned? */
		goto out2;
	}

	ump = malloc(sizeof *ump, M_UFSMNT, M_WAITOK);
	memset((caddr_t)ump, 0, sizeof *ump);
	ump->um_fs = fs;
	if (fs->fs_sbsize < SBSIZE)
		bp->b_flags |= B_INVAL;
	brelse(bp);
	bp = NULL;
	fs->fs_ronly = ronly;
	if (ronly == 0) {
		fs->fs_clean <<= 1;
		fs->fs_fmod = 1;
	}
	size = fs->fs_cssize;
	blks = howmany(size, fs->fs_fsize);
	if (fs->fs_contigsumsize > 0)
		size += fs->fs_ncg * sizeof(int32_t);
	base = space = malloc((u_long)size, M_UFSMNT, M_WAITOK);
	for (i = 0; i < blks; i += fs->fs_frag) {
		size = fs->fs_bsize;
		if (i + fs->fs_frag > blks)
			size = (blks - i) * fs->fs_fsize;
		error = bread(devvp, fsbtodb(fs, fs->fs_csaddr + i), size,
			      cred, &bp);
		if (error) {
			free(base, M_UFSMNT);
			goto out2;
		}
#ifdef FFS_EI
		if (needswap)
			ffs_csum_swap((struct csum*)bp->b_data,
				(struct csum*)space, size);
		else
#endif
			memcpy(space, bp->b_data, (u_int)size);
			
		fs->fs_csp[fragstoblks(fs, i)] = (struct csum *)space;
		space += size;
		brelse(bp);
		bp = NULL;
	}
	if (fs->fs_contigsumsize > 0) {
		fs->fs_maxcluster = lp = (int32_t *)space;
		for (i = 0; i < fs->fs_ncg; i++)
			*lp++ = fs->fs_contigsumsize;
	}
	mp->mnt_data = (qaddr_t)ump;
	mp->mnt_stat.f_fsid.val[0] = (long)dev;
	mp->mnt_stat.f_fsid.val[1] = makefstype(MOUNT_FFS);
	mp->mnt_maxsymlinklen = fs->fs_maxsymlinklen;
	mp->mnt_fs_bshift = fs->fs_bshift;
	mp->mnt_dev_bshift = DEV_BSHIFT;	/* XXX */
	mp->mnt_flag |= MNT_LOCAL;
#ifdef FFS_EI
	if (needswap)
		ump->um_flags |= UFS_NEEDSWAP;
#endif
	ump->um_mountp = mp;
	ump->um_dev = dev;
	ump->um_devvp = devvp;
	ump->um_nindir = fs->fs_nindir;
	ump->um_lognindir = ffs(fs->fs_nindir) - 1;
	ump->um_bptrtodb = fs->fs_fsbtodb;
	ump->um_seqinc = fs->fs_frag;
	for (i = 0; i < MAXQUOTAS; i++)
		ump->um_quotas[i] = NULLVP;
	devvp->v_specmountpoint = mp;
	ump->um_savedmaxfilesize = fs->fs_maxfilesize;		/* XXX */
	maxfilesize = (u_int64_t)0x80000000 * fs->fs_bsize - 1;	/* XXX */
	if (fs->fs_maxfilesize > maxfilesize)			/* XXX */
		fs->fs_maxfilesize = maxfilesize;		/* XXX */
	if (ronly == 0 && (fs->fs_flags & FS_DOSOFTDEP)) {
		error = softdep_mount(devvp, mp, fs, cred);
		if (error) {
			free(base, M_UFSMNT);
			goto out;
		}
	}
	return (0);
out2:
	free(fs, M_UFSMNT);
out:
	devvp->v_specmountpoint = NULL;
	if (bp)
		brelse(bp);
	vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
	(void)VOP_CLOSE(devvp, ronly ? FREAD : FREAD|FWRITE, cred, p);
	VOP_UNLOCK(devvp, 0);
	if (ump) {
		free(ump, M_UFSMNT);
		mp->mnt_data = (qaddr_t)0;
	}
	return (error);
}

/*
 * Sanity checks for old file systems.
 *
 * XXX - goes away some day.
 */
int
ffs_oldfscompat(fs)
	struct fs *fs;
{
	int i;

	fs->fs_npsect = max(fs->fs_npsect, fs->fs_nsect);	/* XXX */
	fs->fs_interleave = max(fs->fs_interleave, 1);		/* XXX */
	if (fs->fs_postblformat == FS_42POSTBLFMT)		/* XXX */
		fs->fs_nrpos = 8;				/* XXX */
	if (fs->fs_inodefmt < FS_44INODEFMT) {			/* XXX */
		u_int64_t sizepb = fs->fs_bsize;		/* XXX */
								/* XXX */
		fs->fs_maxfilesize = fs->fs_bsize * NDADDR - 1;	/* XXX */
		for (i = 0; i < NIADDR; i++) {			/* XXX */
			sizepb *= NINDIR(fs);			/* XXX */
			fs->fs_maxfilesize += sizepb;		/* XXX */
		}						/* XXX */
		fs->fs_qbmask = ~fs->fs_bmask;			/* XXX */
		fs->fs_qfmask = ~fs->fs_fmask;			/* XXX */
	}							/* XXX */
	return (0);
}

/*
 * unmount system call
 */
int
ffs_unmount(mp, mntflags, p)
	struct mount *mp;
	int mntflags;
	struct proc *p;
{
	struct ufsmount *ump;
	struct fs *fs;
	int error, flags;

	flags = 0;
	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;
	if (mp->mnt_flag & MNT_SOFTDEP) {
		if ((error = softdep_flushfiles(mp, flags, p)) != 0)
			return (error);
	} else {
		if ((error = ffs_flushfiles(mp, flags, p)) != 0)
			return (error);
	}
	ump = VFSTOUFS(mp);
	fs = ump->um_fs;
	if (fs->fs_ronly == 0 &&
	    ffs_cgupdate(ump, MNT_WAIT) == 0 &&
	    fs->fs_clean & FS_WASCLEAN) {
		if (mp->mnt_flag & MNT_SOFTDEP)
			fs->fs_flags &= ~FS_DOSOFTDEP;
		fs->fs_clean = FS_ISCLEAN;
		(void) ffs_sbupdate(ump, MNT_WAIT);
	}
	if (ump->um_devvp->v_type != VBAD)
		ump->um_devvp->v_specmountpoint = NULL;
	vn_lock(ump->um_devvp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_CLOSE(ump->um_devvp, fs->fs_ronly ? FREAD : FREAD|FWRITE,
		NOCRED, p);
	vput(ump->um_devvp);
	free(fs->fs_csp[0], M_UFSMNT);
	free(fs, M_UFSMNT);
	free(ump, M_UFSMNT);
	mp->mnt_data = (qaddr_t)0;
	mp->mnt_flag &= ~MNT_LOCAL;
	return (error);
}

/*
 * Flush out all the files in a filesystem.
 */
int
ffs_flushfiles(mp, flags, p)
	struct mount *mp;
	int flags;
	struct proc *p;
{
	extern int doforce;
	struct ufsmount *ump;
	int error;

	if (!doforce)
		flags &= ~FORCECLOSE;
	ump = VFSTOUFS(mp);
#ifdef QUOTA
	if (mp->mnt_flag & MNT_QUOTA) {
		int i;
		if ((error = vflush(mp, NULLVP, SKIPSYSTEM|flags)) != 0)
			return (error);
		for (i = 0; i < MAXQUOTAS; i++) {
			if (ump->um_quotas[i] == NULLVP)
				continue;
			quotaoff(p, mp, i);
		}
		/*
		 * Here we fall through to vflush again to ensure
		 * that we have gotten rid of all the system vnodes.
		 */
	}
#endif
	/*
	 * Flush all the files.
	 */
	error = vflush(mp, NULLVP, flags);
	if (error)
		return (error);
	/*
	 * Flush filesystem metadata.
	 */
	vn_lock(ump->um_devvp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_FSYNC(ump->um_devvp, p->p_ucred, FSYNC_WAIT, 0, 0, p);
	VOP_UNLOCK(ump->um_devvp, 0);
	return (error);
}

/*
 * Get file system statistics.
 */
int
ffs_statfs(mp, sbp, p)
	struct mount *mp;
	struct statfs *sbp;
	struct proc *p;
{
	struct ufsmount *ump;
	struct fs *fs;

	ump = VFSTOUFS(mp);
	fs = ump->um_fs;
	if (fs->fs_magic != FS_MAGIC)
		panic("ffs_statfs");
#ifdef COMPAT_09
	sbp->f_type = 1;
#else
	sbp->f_type = 0;
#endif
	sbp->f_bsize = fs->fs_fsize;
	sbp->f_iosize = fs->fs_bsize;
	sbp->f_blocks = fs->fs_dsize;
	sbp->f_bfree = fs->fs_cstotal.cs_nbfree * fs->fs_frag +
		fs->fs_cstotal.cs_nffree;
	sbp->f_bavail = (long) (((u_int64_t) fs->fs_dsize * (u_int64_t)
	    (100 - fs->fs_minfree) / (u_int64_t) 100) -
	    (u_int64_t) (fs->fs_dsize - sbp->f_bfree));
	sbp->f_files =  fs->fs_ncg * fs->fs_ipg - ROOTINO;
	sbp->f_ffree = fs->fs_cstotal.cs_nifree;
	if (sbp != &mp->mnt_stat) {
		memcpy(sbp->f_mntonname, mp->mnt_stat.f_mntonname, MNAMELEN);
		memcpy(sbp->f_mntfromname, mp->mnt_stat.f_mntfromname, MNAMELEN);
	}
	strncpy(sbp->f_fstypename, mp->mnt_op->vfs_name, MFSNAMELEN);
	return (0);
}

/*
 * Go through the disk queues to initiate sandbagged IO;
 * go through the inodes to write those that have been modified;
 * initiate the writing of the super block if it has been modified.
 *
 * Note: we are always called with the filesystem marked `MPBUSY'.
 */
int
ffs_sync(mp, waitfor, cred, p)
	struct mount *mp;
	int waitfor;
	struct ucred *cred;
	struct proc *p;
{
	struct vnode *vp, *nvp;
	struct inode *ip;
	struct ufsmount *ump = VFSTOUFS(mp);
	struct fs *fs;
	int error, allerror = 0;

	fs = ump->um_fs;
	if (fs->fs_fmod != 0 && fs->fs_ronly != 0) {		/* XXX */
		printf("fs = %s\n", fs->fs_fsmnt);
		panic("update: rofs mod");
	}
	/*
	 * Write back each (modified) inode.
	 */
	simple_lock(&mntvnode_slock);
loop:
	for (vp = LIST_FIRST(&mp->mnt_vnodelist); vp != NULL; vp = nvp) {
		/*
		 * If the vnode that we are about to sync is no longer
		 * associated with this mount point, start over.
		 */
		if (vp->v_mount != mp)
			goto loop;
		simple_lock(&vp->v_interlock);
		nvp = LIST_NEXT(vp, v_mntvnodes);
		ip = VTOI(vp);
		if (vp->v_type == VNON ||
		    ((ip->i_flag &
		      (IN_ACCESS | IN_CHANGE | IN_UPDATE | IN_MODIFIED | IN_ACCESSED)) == 0 &&
		     LIST_EMPTY(&vp->v_dirtyblkhd) &&
		     vp->v_uvm.u_obj.uo_npages == 0))
		{
			simple_unlock(&vp->v_interlock);
			continue;
		}
		simple_unlock(&mntvnode_slock);
		error = vget(vp, LK_EXCLUSIVE | LK_NOWAIT | LK_INTERLOCK);
		if (error) {
			simple_lock(&mntvnode_slock);
			if (error == ENOENT)
				goto loop;
			continue;
		}
		if ((error = VOP_FSYNC(vp, cred,
		    waitfor == MNT_WAIT ? FSYNC_WAIT : 0, 0, 0, p)) != 0)
			allerror = error;
		vput(vp);
		simple_lock(&mntvnode_slock);
	}
	simple_unlock(&mntvnode_slock);
	/*
	 * Force stale file system control information to be flushed.
	 */
	if (waitfor != MNT_LAZY) {
		if (ump->um_mountp->mnt_flag & MNT_SOFTDEP)
			waitfor = MNT_NOWAIT;
		vn_lock(ump->um_devvp, LK_EXCLUSIVE | LK_RETRY);
		if ((error = VOP_FSYNC(ump->um_devvp, cred,
		    waitfor == MNT_WAIT ? FSYNC_WAIT : 0, 0, 0, p)) != 0)
			allerror = error;
		VOP_UNLOCK(ump->um_devvp, 0);
	}
#ifdef QUOTA
	qsync(mp);
#endif
	/*
	 * Write back modified superblock.
	 */
	if (fs->fs_fmod != 0) {
		fs->fs_fmod = 0;
		fs->fs_time = time.tv_sec;
		if ((error = ffs_cgupdate(ump, waitfor)))
			allerror = error;
	}
	return (allerror);
}

/*
 * Look up a FFS dinode number to find its incore vnode, otherwise read it
 * in from disk.  If it is in core, wait for the lock bit to clear, then
 * return the inode locked.  Detection and handling of mount points must be
 * done by the calling routine.
 */
int
ffs_vget(mp, ino, vpp)
	struct mount *mp;
	ino_t ino;
	struct vnode **vpp;
{
	struct fs *fs;
	struct inode *ip;
	struct ufsmount *ump;
	struct buf *bp;
	struct vnode *vp;
	dev_t dev;
	int error;
	caddr_t cp;

	ump = VFSTOUFS(mp);
	dev = ump->um_dev;

	if ((*vpp = ufs_ihashget(dev, ino, LK_EXCLUSIVE)) != NULL)
		return (0);

	/* Allocate a new vnode/inode. */
	if ((error = getnewvnode(VT_UFS, mp, ffs_vnodeop_p, &vp)) != 0) {
		*vpp = NULL;
		return (error);
	}

	/*
	 * If someone beat us to it while sleeping in getnewvnode(),
	 * push back the freshly allocated vnode we don't need, and return.
	 */
	do {
		if ((*vpp = ufs_ihashget(dev, ino, LK_EXCLUSIVE)) != NULL) {
			ungetnewvnode(vp);
			return (0);
		}
	} while (lockmgr(&ufs_hashlock, LK_EXCLUSIVE|LK_SLEEPFAIL, 0));

	/*
	 * XXX MFS ends up here, too, to allocate an inode.  Should we
	 * XXX create another pool for MFS inodes?
	 */
	ip = pool_get(&ffs_inode_pool, PR_WAITOK);
	memset((caddr_t)ip, 0, sizeof(struct inode));
	vp->v_data = ip;
	ip->i_vnode = vp;
	ip->i_fs = fs = ump->um_fs;
	ip->i_dev = dev;
	ip->i_number = ino;
	LIST_INIT(&ip->i_pcbufhd);
#ifdef QUOTA
	{
		int i;

		for (i = 0; i < MAXQUOTAS; i++)
			ip->i_dquot[i] = NODQUOT;
	}
#endif
	/*
	 * Put it onto its hash chain and lock it so that other requests for
	 * this inode will block if they arrive while we are sleeping waiting
	 * for old data structures to be purged or for the contents of the
	 * disk portion of this inode to be read.
	 */
	ufs_ihashins(ip);
	lockmgr(&ufs_hashlock, LK_RELEASE, 0);

	/* Read in the disk contents for the inode, copy into the inode. */
	error = bread(ump->um_devvp, fsbtodb(fs, ino_to_fsba(fs, ino)),
		      (int)fs->fs_bsize, NOCRED, &bp);
	if (error) {
		/*
		 * The inode does not contain anything useful, so it would
		 * be misleading to leave it on its hash chain. With mode
		 * still zero, it will be unlinked and returned to the free
		 * list by vput().
		 */
		vput(vp);
		brelse(bp);
		*vpp = NULL;
		return (error);
	}
	cp = (caddr_t)bp->b_data + (ino_to_fsbo(fs, ino) * DINODE_SIZE);
#ifdef FFS_EI
	if (UFS_FSNEEDSWAP(fs))
		ffs_dinode_swap((struct dinode *)cp, &ip->i_din.ffs_din);
	else 
#endif
		memcpy(&ip->i_din.ffs_din, cp, DINODE_SIZE);
	if (DOINGSOFTDEP(vp))
		softdep_load_inodeblock(ip);
	else
		ip->i_ffs_effnlink = ip->i_ffs_nlink;
	brelse(bp);

	/*
	 * Initialize the vnode from the inode, check for aliases.
	 * Note that the underlying vnode may have changed.
	 */
	error = ufs_vinit(mp, ffs_specop_p, ffs_fifoop_p, &vp);
	if (error) {
		vput(vp);
		*vpp = NULL;
		return (error);
	}
	/*
	 * Finish inode initialization now that aliasing has been resolved.
	 */
	ip->i_devvp = ump->um_devvp;
	VREF(ip->i_devvp);
	/*
	 * Ensure that uid and gid are correct. This is a temporary
	 * fix until fsck has been changed to do the update.
	 */
	if (fs->fs_inodefmt < FS_44INODEFMT) {			/* XXX */
		ip->i_ffs_uid = ip->i_din.ffs_din.di_ouid;	/* XXX */
		ip->i_ffs_gid = ip->i_din.ffs_din.di_ogid;	/* XXX */
	}							/* XXX */
	uvm_vnp_setsize(vp, ip->i_ffs_size);

	*vpp = vp;
	return (0);
}

/*
 * File handle to vnode
 *
 * Have to be really careful about stale file handles:
 * - check that the inode number is valid
 * - call ffs_vget() to get the locked inode
 * - check for an unallocated inode (i_mode == 0)
 * - check that the given client host has export rights and return
 *   those rights via. exflagsp and credanonp
 */
int
ffs_fhtovp(mp, fhp, vpp)
	struct mount *mp;
	struct fid *fhp;
	struct vnode **vpp;
{
	struct ufid *ufhp;
	struct fs *fs;

	ufhp = (struct ufid *)fhp;
	fs = VFSTOUFS(mp)->um_fs;
	if (ufhp->ufid_ino < ROOTINO ||
	    ufhp->ufid_ino >= fs->fs_ncg * fs->fs_ipg)
		return (ESTALE);
	return (ufs_fhtovp(mp, ufhp, vpp));
}

/*
 * Vnode pointer to File handle
 */
/* ARGSUSED */
int
ffs_vptofh(vp, fhp)
	struct vnode *vp;
	struct fid *fhp;
{
	struct inode *ip;
	struct ufid *ufhp;

	ip = VTOI(vp);
	ufhp = (struct ufid *)fhp;
	ufhp->ufid_len = sizeof(struct ufid);
	ufhp->ufid_ino = ip->i_number;
	ufhp->ufid_gen = ip->i_ffs_gen;
	return (0);
}

void
ffs_init()
{
	if (ffs_initcount++ > 0)
		return;

	softdep_initialize();
	ufs_init();

	pool_init(&ffs_inode_pool, sizeof(struct inode), 0, 0, 0, "ffsinopl",
	    0, pool_page_alloc_nointr, pool_page_free_nointr, M_FFSNODE);
}

void
ffs_done()
{
	if (--ffs_initcount > 0)
		return;

	/* XXX softdep cleanup ? */
	ufs_done();
	pool_destroy(&ffs_inode_pool);
}

int
ffs_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	extern int doclusterread, doclusterwrite, doreallocblks, doasyncfree;
	extern int ffs_log_changeopt;

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case FFS_CLUSTERREAD:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
		    &doclusterread));
	case FFS_CLUSTERWRITE:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
		    &doclusterwrite));
	case FFS_REALLOCBLKS:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
		    &doreallocblks));
	case FFS_ASYNCFREE:
		return (sysctl_int(oldp, oldlenp, newp, newlen, &doasyncfree));
	case FFS_LOG_CHANGEOPT:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
			&ffs_log_changeopt));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

/*
 * Write a superblock and associated information back to disk.
 */
int
ffs_sbupdate(mp, waitfor)
	struct ufsmount *mp;
	int waitfor;
{
	struct fs *fs = mp->um_fs;
	struct buf *bp;
	int i, error = 0;
	int32_t saved_nrpos = fs->fs_nrpos;
	int64_t saved_qbmask = fs->fs_qbmask;
	int64_t saved_qfmask = fs->fs_qfmask;
	u_int64_t saved_maxfilesize = fs->fs_maxfilesize;
	u_int8_t saveflag;

	/* Restore compatibility to old file systems.		   XXX */
	if (fs->fs_postblformat == FS_42POSTBLFMT)		/* XXX */
		fs->fs_nrpos = -1;		/* XXX */
	if (fs->fs_inodefmt < FS_44INODEFMT) {			/* XXX */
		int32_t *lp, tmp;				/* XXX */
								/* XXX */
		lp = (int32_t *)&fs->fs_qbmask;	/* XXX nuke qfmask too */
		tmp = lp[4];					/* XXX */
		for (i = 4; i > 0; i--)				/* XXX */
			lp[i] = lp[i-1];			/* XXX */
		lp[0] = tmp;					/* XXX */
	}							/* XXX */
	fs->fs_maxfilesize = mp->um_savedmaxfilesize;	/* XXX */

	bp = getblk(mp->um_devvp, SBOFF >> (fs->fs_fshift - fs->fs_fsbtodb),
	    (int)fs->fs_sbsize, 0, 0);
	saveflag = fs->fs_flags & FS_INTERNAL;
	fs->fs_flags &= ~FS_INTERNAL;
	memcpy(bp->b_data, fs, fs->fs_sbsize);
#ifdef FFS_EI
	if (mp->um_flags & UFS_NEEDSWAP)
		ffs_sb_swap(fs, (struct fs*)bp->b_data, 1);
#endif

	fs->fs_flags |= saveflag;
	fs->fs_nrpos = saved_nrpos; /* XXX */
	fs->fs_qbmask = saved_qbmask; /* XXX */
	fs->fs_qfmask = saved_qfmask; /* XXX */
	fs->fs_maxfilesize = saved_maxfilesize; /* XXX */

	if (waitfor == MNT_WAIT)
		error = bwrite(bp);
	else
		bawrite(bp);
	return (error);
}

int
ffs_cgupdate(mp, waitfor)
	struct ufsmount *mp;
	int waitfor;
{
	struct fs *fs = mp->um_fs;
	struct buf *bp;
	int blks;
	caddr_t space;
	int i, size, error = 0, allerror = 0;

	allerror = ffs_sbupdate(mp, waitfor);
	blks = howmany(fs->fs_cssize, fs->fs_fsize);
	space = (caddr_t)fs->fs_csp[0];
	for (i = 0; i < blks; i += fs->fs_frag) {
		size = fs->fs_bsize;
		if (i + fs->fs_frag > blks)
			size = (blks - i) * fs->fs_fsize;
		bp = getblk(mp->um_devvp, fsbtodb(fs, fs->fs_csaddr + i),
		    size, 0, 0);
#ifdef FFS_EI
		if (mp->um_flags & UFS_NEEDSWAP)
			ffs_csum_swap((struct csum*)space,
			    (struct csum*)bp->b_data, size);
		else
#endif
			memcpy(bp->b_data, space, (u_int)size);
		space += size;
		if (waitfor == MNT_WAIT)
			error = bwrite(bp);
		else
			bawrite(bp);
	}
	if (!allerror && error)
		allerror = error;
	return (allerror);
}
