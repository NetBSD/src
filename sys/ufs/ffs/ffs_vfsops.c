/*	$NetBSD: ffs_vfsops.c,v 1.119 2003/08/07 16:34:31 agc Exp $	*/

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
 *	@(#)ffs_vfsops.c	8.31 (Berkeley) 5/20/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ffs_vfsops.c,v 1.119 2003/08/07 16:34:31 agc Exp $");

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
#include <sys/conf.h>

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

extern const struct vnodeopv_desc ffs_vnodeop_opv_desc;
extern const struct vnodeopv_desc ffs_specop_opv_desc;
extern const struct vnodeopv_desc ffs_fifoop_opv_desc;

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
	ffs_reinit,
	ffs_done,
	ffs_sysctl,
	ffs_mountroot,
	ufs_check_export,
	ffs_vnodeopv_descs,
};

struct genfs_ops ffs_genfsops = {
	ffs_gop_size,
	ufs_gop_alloc,
	genfs_gop_write,
};

struct pool ffs_inode_pool;
struct pool ffs_dinode1_pool;
struct pool ffs_dinode2_pool;

static void ffs_oldfscompat_read(struct fs *, struct ufsmount *,
				   daddr_t);
static void ffs_oldfscompat_write(struct fs *, struct ufsmount *);

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
	struct vnode *devvp = NULL;
	struct ufs_args args;
	struct ufsmount *ump = NULL;
	struct fs *fs;
	int error, flags, update;
	mode_t accessmode;

	if (mp->mnt_flag & MNT_GETARGS) {
		ump = VFSTOUFS(mp);
		if (ump == NULL)
			return EIO;
		args.fspec = NULL;
		vfs_showexport(mp, &args.export, &ump->um_export);
		return copyout(&args, data, sizeof(args));
	}
	error = copyin(data, &args, sizeof (struct ufs_args));
	if (error)
		return (error);

#if !defined(SOFTDEP)
	mp->mnt_flag &= ~MNT_SOFTDEP;
#endif

	update = mp->mnt_flag & MNT_UPDATE;

	/* Check arguments */
	if (update) {
		/* Use the extant mount */
		ump = VFSTOUFS(mp);
		devvp = ump->um_devvp;
		if (args.fspec == NULL)
			vref(devvp);
	} else {
		/* New mounts must have a filename for the device */
		if (args.fspec == NULL)
			return (EINVAL);
	}

	if (args.fspec != NULL) {
		/*
		 * Look up the name and verify that it's sane.
		 */
		NDINIT(ndp, LOOKUP, FOLLOW, UIO_USERSPACE, args.fspec, p);
		if ((error = namei(ndp)) != 0)
			return (error);
		devvp = ndp->ni_vp;

		if (!update) {
			/*
			 * Be sure this is a valid block device
			 */
			if (devvp->v_type != VBLK)
				error = ENOTBLK;
			else if (bdevsw_lookup(devvp->v_rdev) == NULL)
				error = ENXIO;
		} else {
			/*
			 * Be sure we're still naming the same device
			 * used for our initial mount
			 */
			if (devvp != ump->um_devvp)
				error = EINVAL;
		}
	}

	/*
	 * If mount by non-root, then verify that user has necessary
	 * permissions on the device.
	 */
	if (error == 0 && p->p_ucred->cr_uid != 0) {
		accessmode = VREAD;
		if (update ?
		    (mp->mnt_flag & MNT_WANTRDWR) != 0 :
		    (mp->mnt_flag & MNT_RDONLY) == 0)
			accessmode |= VWRITE;
		vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
		error = VOP_ACCESS(devvp, accessmode, p->p_ucred, p);
		VOP_UNLOCK(devvp, 0);
	}

	if (error) {
		vrele(devvp);
		return (error);
	}

	if (!update) {
		error = ffs_mountfs(devvp, mp, p);
		if (error) {
			vrele(devvp);
			return (error);
		}

		ump = VFSTOUFS(mp);
		fs = ump->um_fs;
		if ((mp->mnt_flag & (MNT_SOFTDEP | MNT_ASYNC)) ==
		    (MNT_SOFTDEP | MNT_ASYNC)) {
			printf("%s fs uses soft updates, "
			    "ignoring async mode\n",
			    fs->fs_fsmnt);
			mp->mnt_flag &= ~MNT_ASYNC;
		}
	} else {
		/*
		 * Update the mount.
		 */

		/*
		 * The initial mount got a reference on this
		 * device, so drop the one obtained via
		 * namei(), above.
		 */
		vrele(devvp);

		fs = ump->um_fs;
		if (fs->fs_ronly == 0 && (mp->mnt_flag & MNT_RDONLY)) {
			/*
			 * Changing from r/w to r/o
			 */
			flags = WRITECLOSE;
			if (mp->mnt_flag & MNT_FORCE)
				flags |= FORCECLOSE;
			if (mp->mnt_flag & MNT_SOFTDEP)
				error = softdep_flushfiles(mp, flags, p);
			else
				error = ffs_flushfiles(mp, flags, p);
			if (fs->fs_pendingblocks != 0 ||
			    fs->fs_pendinginodes != 0) {
				printf("%s: update error: blocks %" PRId64
				       " files %d\n",
				    fs->fs_fsmnt, fs->fs_pendingblocks,
				    fs->fs_pendinginodes);
				fs->fs_pendingblocks = 0;
				fs->fs_pendinginodes = 0;
			}
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
			error = ffs_reload(mp, p->p_ucred, p);
			if (error)
				return (error);
		}

		if (fs->fs_ronly && (mp->mnt_flag & MNT_WANTRDWR)) {
			/*
			 * Changing from read-only to read/write
			 */
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

	error = set_statfs_info(path, UIO_USERSPACE, args.fspec,
	    UIO_USERSPACE, mp, p);
	if (error == 0)
		(void)strncpy(fs->fs_fsmnt, mp->mnt_stat.f_mntonname,
		    sizeof(fs->fs_fsmnt));
	if (mp->mnt_flag & MNT_SOFTDEP)
		fs->fs_flags |= FS_DOSOFTDEP;
	else
		fs->fs_flags &= ~FS_DOSOFTDEP;
	if (fs->fs_fmod != 0) {	/* XXX */
		fs->fs_fmod = 0;
		if (fs->fs_clean & FS_WASCLEAN)
			fs->fs_time = time.tv_sec;
		else {
			printf("%s: file system not clean (fs_clean=%x); please fsck(8)\n",
			    mp->mnt_stat.f_mntfromname, fs->fs_clean);
			printf("%s: lost blocks %" PRId64 " files %d\n",
			    mp->mnt_stat.f_mntfromname, fs->fs_pendingblocks,
			    fs->fs_pendinginodes);
		}
		(void) ffs_cgupdate(ump, MNT_WAIT);
	}
	return error;
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
	void *space;
	struct buf *bp;
	struct fs *fs, *newfs;
	struct partinfo dpart;
	daddr_t sblockloc;
	int i, blks, size, error;
	int32_t *lp;
	struct ufsmount *ump;

	if ((mountp->mnt_flag & MNT_RDONLY) == 0)
		return (EINVAL);

	ump = VFSTOUFS(mountp);
	/*
	 * Step 1: invalidate all cached meta-data.
	 */
	devvp = ump->um_devvp;
	vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
	error = vinvalbuf(devvp, 0, cred, p, 0, 0);
	VOP_UNLOCK(devvp, 0);
	if (error)
		panic("ffs_reload: dirty1");
	/*
	 * Step 2: re-read superblock from disk.
	 */
	fs = ump->um_fs;
	if (VOP_IOCTL(devvp, DIOCGPART, &dpart, FREAD, NOCRED, p) != 0)
		size = DEV_BSIZE;
	else
		size = dpart.disklab->d_secsize;
	error = bread(devvp, fs->fs_sblockloc / size, fs->fs_sbsize,
		      NOCRED, &bp);
	if (error) {
		brelse(bp);
		return (error);
	}
	newfs = malloc(fs->fs_sbsize, M_UFSMNT, M_WAITOK);
	memcpy(newfs, bp->b_data, fs->fs_sbsize);
#ifdef SUPPORT_42POSTBLFMT_WRITE
	if (fs->fs_old_postblformat == FS_42POSTBLFMT)
		memcpy(ump->um_opostsave, &newfs->fs_old_postbl_start, 256);
#endif
#ifdef FFS_EI
	if (ump->um_flags & UFS_NEEDSWAP) {
		ffs_sb_swap((struct fs*)bp->b_data, newfs);
		fs->fs_flags |= FS_SWAPPED;
	}
#endif
	if ((newfs->fs_magic != FS_UFS1_MAGIC &&	
	     newfs->fs_magic != FS_UFS2_MAGIC)||
	     newfs->fs_bsize > MAXBSIZE ||
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
	newfs->fs_csp = fs->fs_csp;
	newfs->fs_maxcluster = fs->fs_maxcluster;
	newfs->fs_contigdirs = fs->fs_contigdirs;
	newfs->fs_ronly = fs->fs_ronly;
	newfs->fs_active = fs->fs_active;
	sblockloc = fs->fs_sblockloc;
	memcpy(fs, newfs, (u_int)fs->fs_sbsize);
	brelse(bp);
	free(newfs, M_UFSMNT);

	/* Recheck for apple UFS filesystem */
	VFSTOUFS(mountp)->um_flags &= ~UFS_ISAPPLEUFS;
	/* First check to see if this is tagged as an Apple UFS filesystem
	 * in the disklabel
	 */
	if ((VOP_IOCTL(devvp, DIOCGPART, &dpart, FREAD, cred, p) == 0) &&
		(dpart.part->p_fstype == FS_APPLEUFS)) {
		VFSTOUFS(mountp)->um_flags |= UFS_ISAPPLEUFS;
	}
#ifdef APPLE_UFS
	else {
		/* Manually look for an apple ufs label, and if a valid one
		 * is found, then treat it like an Apple UFS filesystem anyway
		 */
		error = bread(devvp, (daddr_t)(APPLEUFS_LABEL_OFFSET / size),
			APPLEUFS_LABEL_SIZE, cred, &bp);
		if (error) {
			brelse(bp);
			return (error);
		}
		error = ffs_appleufs_validate(fs->fs_fsmnt,
			(struct appleufslabel *)bp->b_data,NULL);
		if (error == 0) {
			VFSTOUFS(mountp)->um_flags |= UFS_ISAPPLEUFS;
		}
		brelse(bp);
		bp = NULL;
	}
#else
	if (VFSTOUFS(mountp)->um_flags & UFS_ISAPPLEUFS)
		return (EIO);
#endif

	mountp->mnt_maxsymlinklen = fs->fs_maxsymlinklen;
	if (UFS_MPISAPPLEUFS(mountp)) {
		/* see comment about NeXT below */
		mountp->mnt_maxsymlinklen = APPLEUFS_MAXSYMLINKLEN;
	}
	ffs_oldfscompat_read(fs, VFSTOUFS(mountp), fs->fs_sblockloc);
	if (fs->fs_pendingblocks != 0 || fs->fs_pendinginodes != 0) {
		fs->fs_pendingblocks = 0;
		fs->fs_pendinginodes = 0;
	}

	ffs_statfs(mountp, &mountp->mnt_stat, p);
	/*
	 * Step 3: re-read summary information from disk.
	 */
	blks = howmany(fs->fs_cssize, fs->fs_fsize);
	space = fs->fs_csp;
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
			ffs_csum_swap((struct csum *)bp->b_data,
			    (struct csum *)space, size);
		else
#endif
			memcpy(space, bp->b_data, (size_t)size);
		space = (char *)space + size;
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
		ffs_load_inode(bp, ip, fs, ip->i_number);
		ip->i_ffs_effnlink = ip->i_nlink;
		brelse(bp);
		vput(vp);
		simple_lock(&mntvnode_slock);
	}
	simple_unlock(&mntvnode_slock);
	return (0);
}

/*
 * Possible superblock locations ordered from most to least likely.
 */
static int sblock_try[] = SBLOCKSEARCH;

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
	void *space;
	daddr_t sblockloc, fsblockloc;
	int blks, fstype;
	int error, i, size, ronly;
#ifdef FFS_EI
	int needswap = 0;		/* keep gcc happy */
#endif
	int32_t *lp;
	struct ucred *cred;
	u_int32_t sbsize = 8192;	/* keep gcc happy*/

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
	if (VOP_IOCTL(devvp, DIOCGPART, &dpart, FREAD, cred, p) != 0)
		size = DEV_BSIZE;
	else
		size = dpart.disklab->d_secsize;

	bp = NULL;
	ump = NULL;
	fs = NULL;
	fsblockloc = sblockloc = 0;
	fstype = 0;

	/*
	 * Try reading the superblock in each of its possible locations.		 */
	for (i = 0; sblock_try[i] != -1; i++) {
		error = bread(devvp, sblock_try[i] / size, SBLOCKSIZE, cred,
			      &bp);
		if (error)
			goto out;
		fs = (struct fs*)bp->b_data;
		fsblockloc = sblockloc = sblock_try[i];
		if (fs->fs_magic == FS_UFS1_MAGIC) {
			sbsize = fs->fs_sbsize;
			fstype = UFS1;
#ifdef FFS_EI
			needswap = 0;
		} else if (fs->fs_magic == bswap32(FS_UFS1_MAGIC)) {
			sbsize = bswap32(fs->fs_sbsize);
			fstype = UFS1;
			needswap = 1;
#endif
		} else if (fs->fs_magic == FS_UFS2_MAGIC) {
			sbsize = fs->fs_sbsize;
			fstype = UFS2;
			fsblockloc = fs->fs_sblockloc;
#ifdef FFS_EI
			needswap = 0;
		} else if (fs->fs_magic == bswap32(FS_UFS2_MAGIC)) {
			sbsize = bswap32(fs->fs_sbsize);
			fstype = UFS2;
			fsblockloc = bswap64(fs->fs_sblockloc);
			needswap = 1;
#endif
		} else
			goto next_sblock;

		if ((fsblockloc == sblockloc ||
		     (fs->fs_old_flags & FS_FLAGS_UPDATED) == 0)
		    && sbsize <= MAXBSIZE && sbsize >= sizeof(struct fs))
			break;

next_sblock:
		bp->b_flags |= B_NOCACHE;
		brelse(bp);
		bp = NULL;
	}

	if (sblock_try[i] == -1) {
		error = EINVAL;
		goto out;
	}

	fs = malloc((u_long)sbsize, M_UFSMNT, M_WAITOK);
	memcpy(fs, bp->b_data, sbsize);

	ump = malloc(sizeof *ump, M_UFSMNT, M_WAITOK);
	memset(ump, 0, sizeof *ump);
	ump->um_fs = fs;

	if (fs->fs_old_postblformat == FS_42POSTBLFMT) {
#ifdef SUPPORT_FS_42POSTBLFMT_WRITE
		ump->um_opostsave = malloc(256, M_UFSMNT, M_WAITOK);
		memcpy(ump->um_opostsave, &fs->fs_old_postbl_start, 256);
#else
		if ((mp->mnt_flag & MNT_RDONLY) == 0) {
			error = EROFS;
			goto out2;
		}
#endif
	}

#ifdef FFS_EI
	if (needswap) {
		ffs_sb_swap((struct fs*)bp->b_data, fs);
		fs->fs_flags |= FS_SWAPPED;
	}
#endif

	if (fs->fs_pendingblocks != 0 || fs->fs_pendinginodes != 0) {
		fs->fs_pendingblocks = 0;
		fs->fs_pendinginodes = 0;
	}

	ump->um_fstype = fstype;
	if (fs->fs_sbsize < SBLOCKSIZE)
		bp->b_flags |= B_INVAL;
	brelse(bp);
	bp = NULL;

	ffs_oldfscompat_read(fs, ump, sblockloc);

	/* First check to see if this is tagged as an Apple UFS filesystem
	 * in the disklabel
	 */
	if ((VOP_IOCTL(devvp, DIOCGPART, &dpart, FREAD, cred, p) == 0) &&
		(dpart.part->p_fstype == FS_APPLEUFS)) {
		ump->um_flags |= UFS_ISAPPLEUFS;
	}
#ifdef APPLE_UFS
	else {
		/* Manually look for an apple ufs label, and if a valid one
		 * is found, then treat it like an Apple UFS filesystem anyway
		 */
		error = bread(devvp, (daddr_t)(APPLEUFS_LABEL_OFFSET / size),
			APPLEUFS_LABEL_SIZE, cred, &bp);
		if (error)
			goto out;
		error = ffs_appleufs_validate(fs->fs_fsmnt,
			(struct appleufslabel *)bp->b_data,NULL);
		if (error == 0) {
			ump->um_flags |= UFS_ISAPPLEUFS;
		}
		brelse(bp);
		bp = NULL;
	}
#else
	if (ump->um_flags & UFS_ISAPPLEUFS) {
		error = EINVAL;
		goto out;
	}
#endif

	/*
	 * verify that we can access the last block in the fs
	 * if we're mounting read/write.
	 */

	if (!ronly) {
		error = bread(devvp, fsbtodb(fs, fs->fs_size - 1), fs->fs_fsize,
		    cred, &bp);
		if (bp->b_bcount != fs->fs_fsize)
			error = EINVAL;
		bp->b_flags |= B_INVAL;
		if (error)
			goto out;
		brelse(bp);
		bp = NULL;
	}

	fs->fs_ronly = ronly;
	if (ronly == 0) {
		fs->fs_clean <<= 1;
		fs->fs_fmod = 1;
	}
	size = fs->fs_cssize;
	blks = howmany(size, fs->fs_fsize);
	if (fs->fs_contigsumsize > 0)
		size += fs->fs_ncg * sizeof(int32_t);
	size += fs->fs_ncg * sizeof(*fs->fs_contigdirs);
	space = malloc((u_long)size, M_UFSMNT, M_WAITOK);
	fs->fs_csp = space;
	for (i = 0; i < blks; i += fs->fs_frag) {
		size = fs->fs_bsize;
		if (i + fs->fs_frag > blks)
			size = (blks - i) * fs->fs_fsize;
		error = bread(devvp, fsbtodb(fs, fs->fs_csaddr + i), size,
			      cred, &bp);
		if (error) {
			free(fs->fs_csp, M_UFSMNT);
			goto out2;
		}
#ifdef FFS_EI
		if (needswap)
			ffs_csum_swap((struct csum *)bp->b_data,
				(struct csum *)space, size);
		else
#endif
			memcpy(space, bp->b_data, (u_int)size);
			
		space = (char *)space + size;
		brelse(bp);
		bp = NULL;
	}
	if (fs->fs_contigsumsize > 0) {
		fs->fs_maxcluster = lp = space;
		for (i = 0; i < fs->fs_ncg; i++)
			*lp++ = fs->fs_contigsumsize;
		space = lp;
	}
	size = fs->fs_ncg * sizeof(*fs->fs_contigdirs);
	fs->fs_contigdirs = space;
	space = (char *)space + size;
	memset(fs->fs_contigdirs, 0, size);
		/* Compatibility for old filesystems - XXX */
	if (fs->fs_avgfilesize <= 0)
		fs->fs_avgfilesize = AVFILESIZ;
	if (fs->fs_avgfpdir <= 0)
		fs->fs_avgfpdir = AFPDIR;
	mp->mnt_data = ump;
	mp->mnt_stat.f_fsid.val[0] = (long)dev;
	mp->mnt_stat.f_fsid.val[1] = makefstype(MOUNT_FFS);
	mp->mnt_maxsymlinklen = fs->fs_maxsymlinklen;
	if (UFS_MPISAPPLEUFS(mp)) {
		/* NeXT used to keep short symlinks in the inode even
		 * when using FS_42INODEFMT.  In that case fs->fs_maxsymlinklen
		 * is probably -1, but we still need to be able to identify
		 * short symlinks.
		 */
		mp->mnt_maxsymlinklen = APPLEUFS_MAXSYMLINKLEN;
	}
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
	if (ronly == 0 && (fs->fs_flags & FS_DOSOFTDEP)) {
		error = softdep_mount(devvp, mp, fs, cred);
		if (error) {
			free(fs->fs_csp, M_UFSMNT);
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
		mp->mnt_data = NULL;
	}
	return (error);
}

/*
 * Sanity checks for loading old filesystem superblocks.
 * See ffs_oldfscompat_write below for unwound actions.
 *
 * XXX - Parts get retired eventually.
 * Unfortunately new bits get added.
 */
static void
ffs_oldfscompat_read(fs, ump, sblockloc)
	struct fs *fs;
	struct ufsmount *ump;
	daddr_t sblockloc;
{
	off_t maxfilesize;

	if (fs->fs_magic != FS_UFS1_MAGIC)
		return;

	/*
	 * If not yet done, update fs_flags location and value of fs_sblockloc.
	 */
	if ((fs->fs_old_flags & FS_FLAGS_UPDATED) == 0) {
		fs->fs_flags = fs->fs_old_flags;
		fs->fs_old_flags |= FS_FLAGS_UPDATED;
		fs->fs_sblockloc = sblockloc;
	}

	/*
	 * If the new fields haven't been set yet, or if the filesystem
	 * was mounted and modified by an old kernel, use the old csum
	 * totals.
	 */
	if (fs->fs_maxbsize != fs->fs_bsize || fs->fs_time < fs->fs_old_time) {
		fs->fs_cstotal.cs_ndir = fs->fs_old_cstotal.cs_ndir;
		fs->fs_cstotal.cs_nbfree = fs->fs_old_cstotal.cs_nbfree;
		fs->fs_cstotal.cs_nifree = fs->fs_old_cstotal.cs_nifree;
		fs->fs_cstotal.cs_nffree = fs->fs_old_cstotal.cs_nffree;
	}

	/*
	 * If not yet done, update UFS1 superblock with new wider fields.
	 */
	if (fs->fs_maxbsize != fs->fs_bsize) {
		fs->fs_maxbsize = fs->fs_bsize;
		fs->fs_time = fs->fs_old_time;
		fs->fs_size = fs->fs_old_size;
		fs->fs_dsize = fs->fs_old_dsize;
		fs->fs_csaddr = fs->fs_old_csaddr;
	}

	if (fs->fs_old_inodefmt < FS_44INODEFMT) {
		fs->fs_maxfilesize = (u_quad_t) 1LL << 39;
		fs->fs_qbmask = ~fs->fs_bmask;
		fs->fs_qfmask = ~fs->fs_fmask;
	}

	ump->um_savedmaxfilesize = fs->fs_maxfilesize;
	maxfilesize = (u_int64_t)0x80000000 * fs->fs_bsize - 1;
	if (fs->fs_maxfilesize > maxfilesize)
		fs->fs_maxfilesize = maxfilesize;

	/* Compatibility for old filesystems */
	if (fs->fs_avgfilesize <= 0)
		fs->fs_avgfilesize = AVFILESIZ;
	if (fs->fs_avgfpdir <= 0)
		fs->fs_avgfpdir = AFPDIR;
#if 0
	if (bigcgs) {
		fs->fs_save_cgsize = fs->fs_cgsize;
		fs->fs_cgsize = fs->fs_bsize;
	}
#endif
}

/*
 * Unwinding superblock updates for old filesystems.
 * See ffs_oldfscompat_read above for details.
 *
 * XXX - Parts get retired eventually.
 * Unfortunately new bits get added.
 */
static void
ffs_oldfscompat_write(fs, ump)
	struct fs *fs;
	struct ufsmount *ump;
{
	if (fs->fs_magic != FS_UFS1_MAGIC)
		return;

	/*
	 * OS X somehow still seems to use this field and panic.
	 * Just set it to zero.
	 */
	if (ump->um_flags & UFS_ISAPPLEUFS)
		fs->fs_old_nrpos = 0;
	/*
	 * Copy back UFS2 updated fields that UFS1 inspects.
	 */

	fs->fs_old_time = fs->fs_time;
	fs->fs_old_cstotal.cs_ndir = fs->fs_cstotal.cs_ndir;
	fs->fs_old_cstotal.cs_nbfree = fs->fs_cstotal.cs_nbfree;
	fs->fs_old_cstotal.cs_nifree = fs->fs_cstotal.cs_nifree;
	fs->fs_old_cstotal.cs_nffree = fs->fs_cstotal.cs_nffree;
	fs->fs_maxfilesize = ump->um_savedmaxfilesize;

#if 0
	if (bigcgs) {
		fs->fs_cgsize = fs->fs_save_cgsize;
		fs->fs_save_cgsize = 0;
	}
#endif
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
	int error, flags, penderr;

	penderr = 0;
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
	if (fs->fs_pendingblocks != 0 || fs->fs_pendinginodes != 0) {
		printf("%s: unmount pending error: blocks %" PRId64
		       " files %d\n",
		    fs->fs_fsmnt, fs->fs_pendingblocks, fs->fs_pendinginodes);
		fs->fs_pendingblocks = 0;
		fs->fs_pendinginodes = 0;
		penderr = 1;
	}
	if (fs->fs_ronly == 0 &&
	    ffs_cgupdate(ump, MNT_WAIT) == 0 &&
	    fs->fs_clean & FS_WASCLEAN) {
		/*
		 * XXXX don't mark fs clean in the case of softdep
		 * pending block errors, until they are fixed.
		 */
		if (penderr == 0) {
			if (mp->mnt_flag & MNT_SOFTDEP)
				fs->fs_flags &= ~FS_DOSOFTDEP;
			fs->fs_clean = FS_ISCLEAN;
		}
		fs->fs_fmod = 0;
		(void) ffs_sbupdate(ump, MNT_WAIT);
	}
	if (ump->um_devvp->v_type != VBAD)
		ump->um_devvp->v_specmountpoint = NULL;
	vn_lock(ump->um_devvp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_CLOSE(ump->um_devvp, fs->fs_ronly ? FREAD : FREAD|FWRITE,
		NOCRED, p);
	vput(ump->um_devvp);
	free(fs->fs_csp, M_UFSMNT);
	free(fs, M_UFSMNT);
#ifdef SUPPORT_FS_42POSTBLFMT_WRITE
	if (ump->um_opostsave != NULL)
		free(ump->um_opostsave, M_UFSMNT);
#endif
	free(ump, M_UFSMNT);
	mp->mnt_data = NULL;
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
#ifdef COMPAT_09
	sbp->f_type = 1;
#else
	sbp->f_type = 0;
#endif
	sbp->f_bsize = fs->fs_fsize;
	sbp->f_iosize = fs->fs_bsize;
	sbp->f_blocks = fs->fs_dsize;
	sbp->f_bfree = blkstofrags(fs, fs->fs_cstotal.cs_nbfree) +
		fs->fs_cstotal.cs_nffree + dbtofsb(fs, fs->fs_pendingblocks);
	sbp->f_bavail = (long) (((u_int64_t) fs->fs_dsize * (u_int64_t)
	    (100 - fs->fs_minfree) / (u_int64_t) 100) -
	    (u_int64_t) (fs->fs_dsize - sbp->f_bfree));
	sbp->f_files =  fs->fs_ncg * fs->fs_ipg - ROOTINO;
	sbp->f_ffree = fs->fs_cstotal.cs_nifree + fs->fs_pendinginodes;
	copy_statfs_info(sbp, mp);
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
		     vp->v_uobj.uo_npages == 0))
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
	memset(ip, 0, sizeof(struct inode));
	vp->v_data = ip;
	ip->i_vnode = vp;
	ip->i_ump = ump;
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
	if (ip->i_ump->um_fstype == UFS1)
		ip->i_din.ffs1_din = pool_get(&ffs_dinode1_pool, PR_WAITOK);
	else
		ip->i_din.ffs2_din = pool_get(&ffs_dinode2_pool, PR_WAITOK);
	ffs_load_inode(bp, ip, fs, ino);
	if (DOINGSOFTDEP(vp))
		softdep_load_inodeblock(ip);
	else
		ip->i_ffs_effnlink = ip->i_nlink;
	brelse(bp);

	/*
	 * Initialize the vnode from the inode, check for aliases.
	 * Note that the underlying vnode may have changed.
	 */

	ufs_vinit(mp, ffs_specop_p, ffs_fifoop_p, &vp);

	/*
	 * Finish inode initialization now that aliasing has been resolved.
	 */

	genfs_node_init(vp, &ffs_genfsops);
	ip->i_devvp = ump->um_devvp;
	VREF(ip->i_devvp);

	/*
	 * Ensure that uid and gid are correct. This is a temporary
	 * fix until fsck has been changed to do the update.
	 */

	if (fs->fs_old_inodefmt < FS_44INODEFMT) {		/* XXX */
		ip->i_uid = ip->i_ffs1_ouid;			/* XXX */
		ip->i_gid = ip->i_ffs1_ogid;			/* XXX */
	}							/* XXX */
	uvm_vnp_setsize(vp, ip->i_size);
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
	ufhp->ufid_gen = ip->i_gen;
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
	    &pool_allocator_nointr);
	pool_init(&ffs_dinode1_pool, sizeof(struct ufs1_dinode), 0, 0, 0,
	    "dino1pl", &pool_allocator_nointr);
	pool_init(&ffs_dinode2_pool, sizeof(struct ufs2_dinode), 0, 0, 0,
	    "dino2pl", &pool_allocator_nointr);
}

void
ffs_reinit()
{
	softdep_reinitialize();
	ufs_reinit();
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
	extern int doasyncfree;
	extern int ffs_log_changeopt;

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
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
	int error = 0;
	u_int32_t saveflag;

	bp = getblk(mp->um_devvp,
	    fs->fs_sblockloc >> (fs->fs_fshift - fs->fs_fsbtodb),
	    (int)fs->fs_sbsize, 0, 0);
	saveflag = fs->fs_flags & FS_INTERNAL;
	fs->fs_flags &= ~FS_INTERNAL;

	if (fs->fs_magic == FS_UFS1_MAGIC && fs->fs_sblockloc != SBLOCK_UFS1 &&
	    (fs->fs_flags & FS_FLAGS_UPDATED) == 0) {
		printf("%s: correcting fs_sblockloc from %" PRId64 " to %d\n",
		    fs->fs_fsmnt, fs->fs_sblockloc, SBLOCK_UFS1);
		fs->fs_sblockloc = SBLOCK_UFS1;
	}

	if (fs->fs_magic == FS_UFS2_MAGIC && fs->fs_sblockloc != SBLOCK_UFS2 &&
	    (fs->fs_flags & FS_FLAGS_UPDATED) == 0) {
		printf("%s: correcting fs_sblockloc from %" PRId64 " to %d\n",
		    fs->fs_fsmnt, fs->fs_sblockloc, SBLOCK_UFS2);
		fs->fs_sblockloc = SBLOCK_UFS2;
	}

	memcpy(bp->b_data, fs, fs->fs_sbsize);

	ffs_oldfscompat_write((struct fs *)bp->b_data, mp);
#ifdef FFS_EI
	if (mp->um_flags & UFS_NEEDSWAP)
		ffs_sb_swap(fs, (struct fs *)bp->b_data);
#endif
#ifdef SUPPORT_FS_42POSTBLFMT_WRITE
	if (fs->fs_old_postblformat == FS_42POSTBLFMT)
		memcpy(&((struct fs *)bp->b_data)->fs_old_postbl_start,
		    mp->um_opostsave, 256);
#endif
	fs->fs_flags |= saveflag;

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
	void *space;
	int i, size, error = 0, allerror = 0;

	allerror = ffs_sbupdate(mp, waitfor);
	blks = howmany(fs->fs_cssize, fs->fs_fsize);
	space = fs->fs_csp;
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
		space = (char *)space + size;
		if (waitfor == MNT_WAIT)
			error = bwrite(bp);
		else
			bawrite(bp);
	}
	if (!allerror && error)
		allerror = error;
	return (allerror);
}
