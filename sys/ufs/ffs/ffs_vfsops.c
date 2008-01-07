/*	$NetBSD: ffs_vfsops.c,v 1.217 2008/01/07 16:56:27 ad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ffs_vfsops.c,v 1.217 2008/01/07 16:56:27 ad Exp $");

#if defined(_KERNEL_OPT)
#include "opt_ffs.h"
#include "opt_quota.h"
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
#include <sys/kauth.h>
#include <sys/fstrans.h>

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

extern kmutex_t ufs_hashlock;

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
	sizeof (struct ufs_args),
	ffs_mount,
	ufs_start,
	ffs_unmount,
	ufs_root,
	ufs_quotactl,
	ffs_statvfs,
	ffs_sync,
	ffs_vget,
	ffs_fhtovp,
	ffs_vptofh,
	ffs_init,
	ffs_reinit,
	ffs_done,
	ffs_mountroot,
	ffs_snapshot,
	ffs_extattrctl,
	ffs_suspendctl,
	ffs_vnodeopv_descs,
	0,
	{ NULL, NULL },
};
VFS_ATTACH(ffs_vfsops);

static const struct genfs_ops ffs_genfsops = {
	.gop_size = ffs_gop_size,
	.gop_alloc = ufs_gop_alloc,
	.gop_write = genfs_gop_write,
	.gop_markupdate = ufs_gop_markupdate,
};

static const struct ufs_ops ffs_ufsops = {
	.uo_itimes = ffs_itimes,
	.uo_update = ffs_update,
	.uo_truncate = ffs_truncate,
	.uo_valloc = ffs_valloc,
	.uo_vfree = ffs_vfree,
	.uo_balloc = ffs_balloc,
};

pool_cache_t ffs_inode_cache;
pool_cache_t ffs_dinode1_cache;
pool_cache_t ffs_dinode2_cache;

static void ffs_oldfscompat_read(struct fs *, struct ufsmount *, daddr_t);
static void ffs_oldfscompat_write(struct fs *, struct ufsmount *);

/*
 * Called by main() when ffs is going to be mounted as root.
 */

int
ffs_mountroot(void)
{
	struct fs *fs;
	struct mount *mp;
	struct lwp *l = curlwp;			/* XXX */
	struct ufsmount *ump;
	int error;

	if (device_class(root_device) != DV_DISK)
		return (ENODEV);

	if ((error = vfs_rootmountalloc(MOUNT_FFS, "root_device", &mp))) {
		vrele(rootvp);
		return (error);
	}
	if ((error = ffs_mountfs(rootvp, mp, l)) != 0) {
		mp->mnt_op->vfs_refcount--;
		vfs_unbusy(mp);
		vfs_destroy(mp);
		return (error);
	}
	mutex_enter(&mountlist_lock);
	CIRCLEQ_INSERT_TAIL(&mountlist, mp, mnt_list);
	mutex_exit(&mountlist_lock);
	ump = VFSTOUFS(mp);
	fs = ump->um_fs;
	memset(fs->fs_fsmnt, 0, sizeof(fs->fs_fsmnt));
	(void)copystr(mp->mnt_stat.f_mntonname, fs->fs_fsmnt, MNAMELEN - 1, 0);
	(void)ffs_statvfs(mp, &mp->mnt_stat);
	vfs_unbusy(mp);
	setrootfstime((time_t)fs->fs_time);
	return (0);
}

/*
 * VFS Operations.
 *
 * mount system call
 */
int
ffs_mount(struct mount *mp, const char *path, void *data, size_t *data_len)
{
	struct lwp *l = curlwp;
	struct nameidata nd;
	struct vnode *devvp = NULL;
	struct ufs_args *args = data;
	struct ufsmount *ump = NULL;
	struct fs *fs;
	int error = 0, flags, update;
	mode_t accessmode;

	if (*data_len < sizeof *args)
		return EINVAL;

	if (mp->mnt_flag & MNT_GETARGS) {
		ump = VFSTOUFS(mp);
		if (ump == NULL)
			return EIO;
		args->fspec = NULL;
		*data_len = sizeof *args;
		return 0;
	}

#if !defined(SOFTDEP)
	mp->mnt_flag &= ~MNT_SOFTDEP;
#endif

	update = mp->mnt_flag & MNT_UPDATE;

	/* Check arguments */
	if (args->fspec != NULL) {
		/*
		 * Look up the name and verify that it's sane.
		 */
		NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, args->fspec);
		if ((error = namei(&nd)) != 0)
			return (error);
		devvp = nd.ni_vp;

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
			ump = VFSTOUFS(mp);
			if (devvp != ump->um_devvp) {
				if (devvp->v_rdev != ump->um_devvp->v_rdev)
					error = EINVAL;
				else {
					vrele(devvp);
					devvp = ump->um_devvp;
					vref(devvp);
				}
			}
		}
	} else {
		if (!update) {
			/* New mounts must have a filename for the device */
			return (EINVAL);
		} else {
			/* Use the extant mount */
			ump = VFSTOUFS(mp);
			devvp = ump->um_devvp;
			vref(devvp);
		}
	}
	if ((mp->mnt_flag & MNT_SOFTDEP) != 0)
		devvp->v_uflag |= VU_SOFTDEP;

	/*
	 * If mount by non-root, then verify that user has necessary
	 * permissions on the device.
	 */
	if (error == 0 && kauth_authorize_generic(l->l_cred,
	    KAUTH_GENERIC_ISSUSER, NULL) != 0) {
		accessmode = VREAD;
		if (update ?
		    (mp->mnt_iflag & IMNT_WANTRDWR) != 0 :
		    (mp->mnt_flag & MNT_RDONLY) == 0)
			accessmode |= VWRITE;
		vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
		error = VOP_ACCESS(devvp, accessmode, l->l_cred);
		VOP_UNLOCK(devvp, 0);
	}

	if (error) {
		vrele(devvp);
		return (error);
	}

	if (!update) {
		int xflags;

		/*
		 * Disallow multiple mounts of the same device.
		 * Disallow mounting of a device that is currently in use
		 * (except for root, which might share swap device for
		 * miniroot).
		 */
		error = vfs_mountedon(devvp);
		if (error)
			goto fail;
		if (vcount(devvp) > 1 && devvp != rootvp) {
			error = EBUSY;
			goto fail;
		}
		if (mp->mnt_flag & MNT_RDONLY)
			xflags = FREAD;
		else
			xflags = FREAD|FWRITE;
		error = VOP_OPEN(devvp, xflags, FSCRED);
		if (error)
			goto fail;
		error = ffs_mountfs(devvp, mp, l);
		if (error) {
			vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
			(void)VOP_CLOSE(devvp, xflags, NOCRED);
			VOP_UNLOCK(devvp, 0);
			goto fail;
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

		ump = VFSTOUFS(mp);
		fs = ump->um_fs;
		if (fs->fs_ronly == 0 && (mp->mnt_flag & MNT_RDONLY)) {
			/*
			 * Changing from r/w to r/o
			 */
			flags = WRITECLOSE;
			if (mp->mnt_flag & MNT_FORCE)
				flags |= FORCECLOSE;
			if (mp->mnt_flag & MNT_SOFTDEP)
				error = softdep_flushfiles(mp, flags, l);
			else
				error = ffs_flushfiles(mp, flags, l);
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
			error = softdep_flushfiles(mp, flags, l);
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
			error = ffs_flushfiles(mp, flags, l);
#else
			mp->mnt_flag &= ~MNT_SOFTDEP;
#endif
		}

		if (mp->mnt_flag & MNT_RELOAD) {
			error = ffs_reload(mp, l->l_cred, l);
			if (error)
				return (error);
		}

		if (fs->fs_ronly && (mp->mnt_iflag & IMNT_WANTRDWR)) {
			/*
			 * Changing from read-only to read/write
			 */
			fs->fs_ronly = 0;
			fs->fs_clean <<= 1;
			fs->fs_fmod = 1;
			if ((fs->fs_flags & FS_DOSOFTDEP)) {
				error = softdep_mount(devvp, mp, fs,
				    l->l_cred);
				if (error)
					return (error);
			}
			if (fs->fs_snapinum[0] != 0)
				ffs_snapshot_mount(mp);
		}
		if (args->fspec == NULL)
			return EINVAL;
		if ((mp->mnt_flag & (MNT_SOFTDEP | MNT_ASYNC)) ==
		    (MNT_SOFTDEP | MNT_ASYNC)) {
			printf("%s fs uses soft updates, ignoring async mode\n",
			    fs->fs_fsmnt);
			mp->mnt_flag &= ~MNT_ASYNC;
		}
	}

	error = set_statvfs_info(path, UIO_USERSPACE, args->fspec,
	    UIO_USERSPACE, mp->mnt_op->vfs_name, mp, l);
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
			fs->fs_time = time_second;
		else {
			printf("%s: file system not clean (fs_clean=%x); please fsck(8)\n",
			    mp->mnt_stat.f_mntfromname, fs->fs_clean);
			printf("%s: lost blocks %" PRId64 " files %d\n",
			    mp->mnt_stat.f_mntfromname, fs->fs_pendingblocks,
			    fs->fs_pendinginodes);
		}
		(void) ffs_cgupdate(ump, MNT_WAIT);
	}
	return (error);

fail:
	vrele(devvp);
	return (error);
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
ffs_reload(struct mount *mp, kauth_cred_t cred, struct lwp *l)
{
	struct vnode *vp, *mvp, *devvp;
	struct inode *ip;
	void *space;
	struct buf *bp;
	struct fs *fs, *newfs;
	struct partinfo dpart;
	int i, blks, size, error;
	int32_t *lp;
	struct ufsmount *ump;
	daddr_t sblockloc;

	if ((mp->mnt_flag & MNT_RDONLY) == 0)
		return (EINVAL);

	ump = VFSTOUFS(mp);
	/*
	 * Step 1: invalidate all cached meta-data.
	 */
	devvp = ump->um_devvp;
	vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
	error = vinvalbuf(devvp, 0, cred, l, 0, 0);
	VOP_UNLOCK(devvp, 0);
	if (error)
		panic("ffs_reload: dirty1");
	/*
	 * Step 2: re-read superblock from disk.
	 */
	fs = ump->um_fs;
	if (VOP_IOCTL(devvp, DIOCGPART, &dpart, FREAD, NOCRED) != 0)
		size = DEV_BSIZE;
	else
		size = dpart.disklab->d_secsize;
	/* XXX we don't handle possibility that superblock moved. */
	error = bread(devvp, fs->fs_sblockloc / size, fs->fs_sbsize,
		      NOCRED, &bp);
	if (error) {
		brelse(bp, 0);
		return (error);
	}
	newfs = malloc(fs->fs_sbsize, M_UFSMNT, M_WAITOK);
	memcpy(newfs, bp->b_data, fs->fs_sbsize);
#ifdef FFS_EI
	if (ump->um_flags & UFS_NEEDSWAP) {
		ffs_sb_swap((struct fs*)bp->b_data, newfs);
		fs->fs_flags |= FS_SWAPPED;
	} else
#endif
		fs->fs_flags &= ~FS_SWAPPED;
	if ((newfs->fs_magic != FS_UFS1_MAGIC &&
	     newfs->fs_magic != FS_UFS2_MAGIC)||
	     newfs->fs_bsize > MAXBSIZE ||
	     newfs->fs_bsize < sizeof(struct fs)) {
		brelse(bp, 0);
		free(newfs, M_UFSMNT);
		return (EIO);		/* XXX needs translation */
	}
	/* Store off old fs_sblockloc for fs_oldfscompat_read. */
	sblockloc = fs->fs_sblockloc;
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
	memcpy(fs, newfs, (u_int)fs->fs_sbsize);
	brelse(bp, 0);
	free(newfs, M_UFSMNT);

	/* Recheck for apple UFS filesystem */
	ump->um_flags &= ~UFS_ISAPPLEUFS;
	/* First check to see if this is tagged as an Apple UFS filesystem
	 * in the disklabel
	 */
	if ((VOP_IOCTL(devvp, DIOCGPART, &dpart, FREAD, cred) == 0) &&
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
		if (error) {
			brelse(bp, 0);
			return (error);
		}
		error = ffs_appleufs_validate(fs->fs_fsmnt,
			(struct appleufslabel *)bp->b_data,NULL);
		if (error == 0)
			ump->um_flags |= UFS_ISAPPLEUFS;
		brelse(bp, 0);
		bp = NULL;
	}
#else
	if (ump->um_flags & UFS_ISAPPLEUFS)
		return (EIO);
#endif

	if (UFS_MPISAPPLEUFS(ump)) {
		/* see comment about NeXT below */
		ump->um_maxsymlinklen = APPLEUFS_MAXSYMLINKLEN;
		ump->um_dirblksiz = APPLEUFS_DIRBLKSIZ;
		mp->mnt_iflag |= IMNT_DTYPE;
	} else {
		ump->um_maxsymlinklen = fs->fs_maxsymlinklen;
		ump->um_dirblksiz = DIRBLKSIZ;
		if (ump->um_maxsymlinklen > 0)
			mp->mnt_iflag |= IMNT_DTYPE;
		else
			mp->mnt_iflag &= ~IMNT_DTYPE;
	}
	ffs_oldfscompat_read(fs, ump, sblockloc);
	mutex_enter(&ump->um_lock);
	ump->um_maxfilesize = fs->fs_maxfilesize;
	if (fs->fs_pendingblocks != 0 || fs->fs_pendinginodes != 0) {
		fs->fs_pendingblocks = 0;
		fs->fs_pendinginodes = 0;
	}
	mutex_exit(&ump->um_lock);

	ffs_statvfs(mp, &mp->mnt_stat);
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
			brelse(bp, 0);
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
		brelse(bp, 0);
	}
	if ((fs->fs_flags & FS_DOSOFTDEP))
		softdep_mount(devvp, mp, fs, cred);
	if (fs->fs_snapinum[0] != 0)
		ffs_snapshot_mount(mp);
	/*
	 * We no longer know anything about clusters per cylinder group.
	 */
	if (fs->fs_contigsumsize > 0) {
		lp = fs->fs_maxcluster;
		for (i = 0; i < fs->fs_ncg; i++)
			*lp++ = fs->fs_contigsumsize;
	}

	/* Allocate a marker vnode. */
	if ((mvp = vnalloc(mp)) == NULL)
		return ENOMEM;
	/*
	 * NOTE: not using the TAILQ_FOREACH here since in this loop vgone()
	 * and vclean() can be called indirectly
	 */
	mutex_enter(&mntvnode_lock);
 loop:
	for (vp = TAILQ_FIRST(&mp->mnt_vnodelist); vp; vp = vunmark(mvp)) {
		vmark(mvp, vp);
		if (vp->v_mount != mp || vismarker(vp))
			continue;
		/*
		 * Step 4: invalidate all inactive vnodes.
		 */
		if (vrecycle(vp, &mntvnode_lock, l)) {
			mutex_enter(&mntvnode_lock);
			(void)vunmark(mvp);
			goto loop;
		}
		/*
		 * Step 5: invalidate all cached file data.
		 */
		mutex_enter(&vp->v_interlock);
		mutex_exit(&mntvnode_lock);
		if (vget(vp, LK_EXCLUSIVE | LK_INTERLOCK)) {
			(void)vunmark(mvp);
			goto loop;
		}
		if (vinvalbuf(vp, 0, cred, l, 0, 0))
			panic("ffs_reload: dirty2");
		/*
		 * Step 6: re-read inode data for all active vnodes.
		 */
		ip = VTOI(vp);
		error = bread(devvp, fsbtodb(fs, ino_to_fsba(fs, ip->i_number)),
			      (int)fs->fs_bsize, NOCRED, &bp);
		if (error) {
			brelse(bp, 0);
			vput(vp);
			(void)vunmark(mvp);
			break;
		}
		ffs_load_inode(bp, ip, fs, ip->i_number);
		ip->i_ffs_effnlink = ip->i_nlink;
		brelse(bp, 0);
		vput(vp);
		mutex_enter(&mntvnode_lock);
	}
	mutex_exit(&mntvnode_lock);
	vnfree(mvp);
	return (error);
}

/*
 * Possible superblock locations ordered from most to least likely.
 */
static const int sblock_try[] = SBLOCKSEARCH;

/*
 * Common code for mount and mountroot
 */
int
ffs_mountfs(struct vnode *devvp, struct mount *mp, struct lwp *l)
{
	struct ufsmount *ump;
	struct buf *bp;
	struct fs *fs;
	dev_t dev;
	struct partinfo dpart;
	void *space;
	daddr_t sblockloc, fsblockloc;
	int blks, fstype;
	int error, i, size, ronly, bset = 0;
#ifdef FFS_EI
	int needswap = 0;		/* keep gcc happy */
#endif
	int32_t *lp;
	kauth_cred_t cred;
	u_int32_t sbsize = 8192;	/* keep gcc happy*/

	dev = devvp->v_rdev;
	cred = l ? l->l_cred : NOCRED;

	/* Flush out any old buffers remaining from a previous use. */
	vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
	error = vinvalbuf(devvp, V_SAVE, cred, l, 0, 0);
	VOP_UNLOCK(devvp, 0);
	if (error)
		return (error);

	ronly = (mp->mnt_flag & MNT_RDONLY) != 0;
	if (VOP_IOCTL(devvp, DIOCGPART, &dpart, FREAD, cred) != 0)
		size = DEV_BSIZE;
	else
		size = dpart.disklab->d_secsize;

	bp = NULL;
	ump = NULL;
	fs = NULL;
	sblockloc = 0;
	fstype = 0;

	error = fstrans_mount(mp);
	if (error)
		return error;

	/*
	 * Try reading the superblock in each of its possible locations.
	 */
	for (i = 0; ; i++) {
		if (bp != NULL) {
			brelse(bp, BC_NOCACHE);
			bp = NULL;
		}
		if (sblock_try[i] == -1) {
			error = EINVAL;
			fs = NULL;
			goto out;
		}
		error = bread(devvp, sblock_try[i] / size, SBLOCKSIZE, cred,
			      &bp);
		if (error) {
			fs = NULL;
			goto out;
		}
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
#ifdef FFS_EI
			needswap = 0;
		} else if (fs->fs_magic == bswap32(FS_UFS2_MAGIC)) {
			sbsize = bswap32(fs->fs_sbsize);
			fstype = UFS2;
			needswap = 1;
#endif
		} else
			continue;


		/* fs->fs_sblockloc isn't defined for old filesystems */
		if (fstype == UFS1 && !(fs->fs_old_flags & FS_FLAGS_UPDATED)) {
			if (sblockloc == SBLOCK_UFS2)
				/*
				 * This is likely to be the first alternate
				 * in a filesystem with 64k blocks.
				 * Don't use it.
				 */
				continue;
			fsblockloc = sblockloc;
		} else {
			fsblockloc = fs->fs_sblockloc;
#ifdef FFS_EI
			if (needswap)
				fsblockloc = bswap64(fsblockloc);
#endif
		}

		/* Check we haven't found an alternate superblock */
		if (fsblockloc != sblockloc)
			continue;

		/* Validate size of superblock */
		if (sbsize > MAXBSIZE || sbsize < sizeof(struct fs))
			continue;

		/* Ok seems to be a good superblock */
		break;
	}

	fs = malloc((u_long)sbsize, M_UFSMNT, M_WAITOK);
	memcpy(fs, bp->b_data, sbsize);

	ump = malloc(sizeof *ump, M_UFSMNT, M_WAITOK);
	memset(ump, 0, sizeof *ump);
	mutex_init(&ump->um_lock, MUTEX_DEFAULT, IPL_NONE);
	ump->um_fs = fs;
	ump->um_ops = &ffs_ufsops;

#ifdef FFS_EI
	if (needswap) {
		ffs_sb_swap((struct fs*)bp->b_data, fs);
		fs->fs_flags |= FS_SWAPPED;
	} else
#endif
		fs->fs_flags &= ~FS_SWAPPED;

	ffs_oldfscompat_read(fs, ump, sblockloc);
	ump->um_maxfilesize = fs->fs_maxfilesize;

	if (fs->fs_pendingblocks != 0 || fs->fs_pendinginodes != 0) {
		fs->fs_pendingblocks = 0;
		fs->fs_pendinginodes = 0;
	}

	ump->um_fstype = fstype;
	if (fs->fs_sbsize < SBLOCKSIZE)
		brelse(bp, BC_INVAL);
	else
		brelse(bp, 0);
	bp = NULL;

	/* First check to see if this is tagged as an Apple UFS filesystem
	 * in the disklabel
	 */
	if ((VOP_IOCTL(devvp, DIOCGPART, &dpart, FREAD, cred) == 0) &&
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
		brelse(bp, 0);
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
		if (error) {
			bset = BC_INVAL;
			goto out;
		}
		brelse(bp, BC_INVAL);
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
			goto out;
		}
#ifdef FFS_EI
		if (needswap)
			ffs_csum_swap((struct csum *)bp->b_data,
				(struct csum *)space, size);
		else
#endif
			memcpy(space, bp->b_data, (u_int)size);

		space = (char *)space + size;
		brelse(bp, 0);
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
	fs->fs_active = NULL;
	mp->mnt_data = ump;
	mp->mnt_stat.f_fsidx.__fsid_val[0] = (long)dev;
	mp->mnt_stat.f_fsidx.__fsid_val[1] = makefstype(MOUNT_FFS);
	mp->mnt_stat.f_fsid = mp->mnt_stat.f_fsidx.__fsid_val[0];
	mp->mnt_stat.f_namemax = FFS_MAXNAMLEN;
	if (UFS_MPISAPPLEUFS(ump)) {
		/* NeXT used to keep short symlinks in the inode even
		 * when using FS_42INODEFMT.  In that case fs->fs_maxsymlinklen
		 * is probably -1, but we still need to be able to identify
		 * short symlinks.
		 */
		ump->um_maxsymlinklen = APPLEUFS_MAXSYMLINKLEN;
		ump->um_dirblksiz = APPLEUFS_DIRBLKSIZ;
		mp->mnt_iflag |= IMNT_DTYPE;
	} else {
		ump->um_maxsymlinklen = fs->fs_maxsymlinklen;
		ump->um_dirblksiz = DIRBLKSIZ;
		if (ump->um_maxsymlinklen > 0)
			mp->mnt_iflag |= IMNT_DTYPE;
		else
			mp->mnt_iflag &= ~IMNT_DTYPE;
	}
	mp->mnt_fs_bshift = fs->fs_bshift;
	mp->mnt_dev_bshift = DEV_BSHIFT;	/* XXX */
	mp->mnt_flag |= MNT_LOCAL;
	mp->mnt_iflag |= IMNT_MPSAFE;
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
	if (ronly == 0 && fs->fs_snapinum[0] != 0)
		ffs_snapshot_mount(mp);
#ifdef UFS_EXTATTR
	/*
	 * Initialize file-backed extended attributes on UFS1 file
	 * systems.
	 */
	if (ump->um_fstype == UFS1) {
		ufs_extattr_uepm_init(&ump->um_extattr);
#ifdef UFS_EXTATTR_AUTOSTART
		/*
		 * XXX Just ignore errors.  Not clear that we should
		 * XXX fail the mount in this case.
		 */
		(void) ufs_extattr_autostart(mp, l);
#endif
	}
#endif /* UFS_EXTATTR */
	return (0);
out:
	fstrans_unmount(mp);
	if (fs)
		free(fs, M_UFSMNT);
	devvp->v_specmountpoint = NULL;
	if (bp)
		brelse(bp, bset);
	if (ump) {
		if (ump->um_oldfscompat)
			free(ump->um_oldfscompat, M_UFSMNT);
		mutex_destroy(&ump->um_lock);
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
ffs_oldfscompat_read(struct fs *fs, struct ufsmount *ump, daddr_t sblockloc)
{
	off_t maxfilesize;
	int32_t *extrasave;

	if ((fs->fs_magic != FS_UFS1_MAGIC) ||
	    (fs->fs_old_flags & FS_FLAGS_UPDATED))
		return;

	if (!ump->um_oldfscompat)
		ump->um_oldfscompat = malloc(512 + 3*sizeof(int32_t),
		    M_UFSMNT, M_WAITOK);

	memcpy(ump->um_oldfscompat, &fs->fs_old_postbl_start, 512);
	extrasave = ump->um_oldfscompat;
	extrasave += 512/sizeof(int32_t);
	extrasave[0] = fs->fs_old_npsect;
	extrasave[1] = fs->fs_old_interleave;
	extrasave[2] = fs->fs_old_trackskew;

	/* These fields will be overwritten by their
	 * original values in fs_oldfscompat_write, so it is harmless
	 * to modify them here.
	 */
	fs->fs_cstotal.cs_ndir = fs->fs_old_cstotal.cs_ndir;
	fs->fs_cstotal.cs_nbfree = fs->fs_old_cstotal.cs_nbfree;
	fs->fs_cstotal.cs_nifree = fs->fs_old_cstotal.cs_nifree;
	fs->fs_cstotal.cs_nffree = fs->fs_old_cstotal.cs_nffree;

	fs->fs_maxbsize = fs->fs_bsize;
	fs->fs_time = fs->fs_old_time;
	fs->fs_size = fs->fs_old_size;
	fs->fs_dsize = fs->fs_old_dsize;
	fs->fs_csaddr = fs->fs_old_csaddr;
	fs->fs_sblockloc = sblockloc;

        fs->fs_flags = fs->fs_old_flags | (fs->fs_flags & FS_INTERNAL);

	if (fs->fs_old_postblformat == FS_42POSTBLFMT) {
		fs->fs_old_nrpos = 8;
		fs->fs_old_npsect = fs->fs_old_nsect;
		fs->fs_old_interleave = 1;
		fs->fs_old_trackskew = 0;
	}

	if (fs->fs_old_inodefmt < FS_44INODEFMT) {
		fs->fs_maxfilesize = (u_quad_t) 1LL << 39;
		fs->fs_qbmask = ~fs->fs_bmask;
		fs->fs_qfmask = ~fs->fs_fmask;
	}

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
ffs_oldfscompat_write(struct fs *fs, struct ufsmount *ump)
{
	int32_t *extrasave;

	if ((fs->fs_magic != FS_UFS1_MAGIC) ||
	    (fs->fs_old_flags & FS_FLAGS_UPDATED))
		return;

	fs->fs_old_time = fs->fs_time;
	fs->fs_old_cstotal.cs_ndir = fs->fs_cstotal.cs_ndir;
	fs->fs_old_cstotal.cs_nbfree = fs->fs_cstotal.cs_nbfree;
	fs->fs_old_cstotal.cs_nifree = fs->fs_cstotal.cs_nifree;
	fs->fs_old_cstotal.cs_nffree = fs->fs_cstotal.cs_nffree;
	fs->fs_old_flags = fs->fs_flags;

#if 0
	if (bigcgs) {
		fs->fs_cgsize = fs->fs_save_cgsize;
	}
#endif

	memcpy(&fs->fs_old_postbl_start, ump->um_oldfscompat, 512);
	extrasave = ump->um_oldfscompat;
	extrasave += 512/sizeof(int32_t);
	fs->fs_old_npsect = extrasave[0];
	fs->fs_old_interleave = extrasave[1];
	fs->fs_old_trackskew = extrasave[2];

}

/*
 * unmount system call
 */
int
ffs_unmount(struct mount *mp, int mntflags)
{
	struct lwp *l = curlwp;
	struct ufsmount *ump = VFSTOUFS(mp);
	struct fs *fs = ump->um_fs;
	int error, flags, penderr;

	penderr = 0;
	flags = 0;
	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;
#ifdef UFS_EXTATTR
	if (ump->um_fstype == UFS1) {
		error = ufs_extattr_stop(mp, l);
		if (error) {
			if (error != EOPNOTSUPP)
				printf("%s: ufs_extattr_stop returned %d\n",
				    fs->fs_fsmnt, error);
		} else
			ufs_extattr_uepm_destroy(&ump->um_extattr);
	}
#endif /* UFS_EXTATTR */
	if (mp->mnt_flag & MNT_SOFTDEP) {
		if ((error = softdep_flushfiles(mp, flags, l)) != 0)
			return (error);
	} else {
		if ((error = ffs_flushfiles(mp, flags, l)) != 0)
			return (error);
	}
	mutex_enter(&ump->um_lock);
	if (fs->fs_pendingblocks != 0 || fs->fs_pendinginodes != 0) {
		printf("%s: unmount pending error: blocks %" PRId64
		       " files %d\n",
		    fs->fs_fsmnt, fs->fs_pendingblocks, fs->fs_pendinginodes);
		fs->fs_pendingblocks = 0;
		fs->fs_pendinginodes = 0;
		penderr = 1;
	}
	mutex_exit(&ump->um_lock);
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
	(void)VOP_CLOSE(ump->um_devvp, fs->fs_ronly ? FREAD : FREAD|FWRITE,
		NOCRED);
	vput(ump->um_devvp);
	free(fs->fs_csp, M_UFSMNT);
	free(fs, M_UFSMNT);
	if (ump->um_oldfscompat != NULL)
		free(ump->um_oldfscompat, M_UFSMNT);
	softdep_unmount(mp);
	mutex_destroy(&ump->um_lock);
	free(ump, M_UFSMNT);
	mp->mnt_data = NULL;
	mp->mnt_flag &= ~MNT_LOCAL;
	fstrans_unmount(mp);
	return (0);
}

/*
 * Flush out all the files in a filesystem.
 */
int
ffs_flushfiles(struct mount *mp, int flags, struct lwp *l)
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
			quotaoff(l, mp, i);
		}
		/*
		 * Here we fall through to vflush again to ensure
		 * that we have gotten rid of all the system vnodes.
		 */
	}
#endif
	if ((error = vflush(mp, 0, SKIPSYSTEM | flags)) != 0)
		return (error);
	ffs_snapshot_unmount(mp);
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
	error = VOP_FSYNC(ump->um_devvp, l->l_cred, FSYNC_WAIT, 0, 0);
	VOP_UNLOCK(ump->um_devvp, 0);
	return (error);
}

/*
 * Get file system statistics.
 */
int
ffs_statvfs(struct mount *mp, struct statvfs *sbp)
{
	struct ufsmount *ump;
	struct fs *fs;

	ump = VFSTOUFS(mp);
	fs = ump->um_fs;
	mutex_enter(&ump->um_lock);
	sbp->f_bsize = fs->fs_bsize;
	sbp->f_frsize = fs->fs_fsize;
	sbp->f_iosize = fs->fs_bsize;
	sbp->f_blocks = fs->fs_dsize;
	sbp->f_bfree = blkstofrags(fs, fs->fs_cstotal.cs_nbfree) +
		fs->fs_cstotal.cs_nffree + dbtofsb(fs, fs->fs_pendingblocks);
	sbp->f_bresvd = ((u_int64_t) fs->fs_dsize * (u_int64_t)
	    fs->fs_minfree) / (u_int64_t) 100;
	if (sbp->f_bfree > sbp->f_bresvd)
		sbp->f_bavail = sbp->f_bfree - sbp->f_bresvd;
	else
		sbp->f_bavail = 0;
	sbp->f_files =  fs->fs_ncg * fs->fs_ipg - ROOTINO;
	sbp->f_ffree = fs->fs_cstotal.cs_nifree + fs->fs_pendinginodes;
	sbp->f_favail = sbp->f_ffree;
	sbp->f_fresvd = 0;
	mutex_exit(&ump->um_lock);
	copy_statvfs_info(sbp, mp);

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
ffs_sync(struct mount *mp, int waitfor, kauth_cred_t cred)
{
	struct lwp *l = curlwp;
	struct vnode *vp, *mvp;
	struct inode *ip;
	struct ufsmount *ump = VFSTOUFS(mp);
	struct fs *fs;
	int error, count, allerror = 0;

	fs = ump->um_fs;
	if (fs->fs_fmod != 0 && fs->fs_ronly != 0) {		/* XXX */
		printf("fs = %s\n", fs->fs_fsmnt);
		panic("update: rofs mod");
	}

	/* Allocate a marker vnode. */
	if ((mvp = vnalloc(mp)) == NULL)
		return (ENOMEM);

	fstrans_start(mp, FSTRANS_SHARED);
	/*
	 * Write back each (modified) inode.
	 */
	mutex_enter(&mntvnode_lock);
loop:
	/*
	 * NOTE: not using the TAILQ_FOREACH here since in this loop vgone()
	 * and vclean() can be called indirectly
	 */
	for (vp = TAILQ_FIRST(&mp->mnt_vnodelist); vp; vp = vunmark(mvp)) {
		vmark(mvp, vp);
		/*
		 * If the vnode that we are about to sync is no longer
		 * associated with this mount point, start over.
		 */
		if (vp->v_mount != mp || vismarker(vp))
			continue;
		mutex_enter(&vp->v_interlock);
		ip = VTOI(vp);
		if (ip == NULL || (vp->v_iflag & (VI_XLOCK|VI_CLEAN)) != 0 ||
		    vp->v_type == VNON || ((ip->i_flag &
		    (IN_CHANGE | IN_UPDATE | IN_MODIFIED)) == 0 &&
		    LIST_EMPTY(&vp->v_dirtyblkhd) &&
		    UVM_OBJ_IS_CLEAN(&vp->v_uobj)))
		{
			mutex_exit(&vp->v_interlock);
			continue;
		}
		if (vp->v_type == VBLK &&
		    fstrans_getstate(mp) == FSTRANS_SUSPENDING) {
			mutex_exit(&vp->v_interlock);
			continue;
		}
		mutex_exit(&mntvnode_lock);
		error = vget(vp, LK_EXCLUSIVE | LK_NOWAIT | LK_INTERLOCK);
		if (error) {
			mutex_enter(&mntvnode_lock);
			if (error == ENOENT) {
				(void)vunmark(mvp);
				goto loop;
			}
			continue;
		}
		if (vp->v_type == VREG && waitfor == MNT_LAZY)
			error = ffs_update(vp, NULL, NULL, 0);
		else
			error = VOP_FSYNC(vp, cred,
			    waitfor == MNT_WAIT ? FSYNC_WAIT : 0, 0, 0);
		if (error)
			allerror = error;
		vput(vp);
		mutex_enter(&mntvnode_lock);
	}
	mutex_exit(&mntvnode_lock);
	/*
	 * Force stale file system control information to be flushed.
	 */
	if (waitfor == MNT_WAIT && (ump->um_mountp->mnt_flag & MNT_SOFTDEP)) {
		if ((error = softdep_flushworklist(ump->um_mountp, &count, l)))
			allerror = error;
		/* Flushed work items may create new vnodes to clean */
		if (allerror == 0 && count) {
			mutex_enter(&mntvnode_lock);
			goto loop;
		}
	}
	if (waitfor != MNT_LAZY && (ump->um_devvp->v_numoutput > 0 ||
	    !LIST_EMPTY(&ump->um_devvp->v_dirtyblkhd))) {
		vn_lock(ump->um_devvp, LK_EXCLUSIVE | LK_RETRY);
		if ((error = VOP_FSYNC(ump->um_devvp, cred,
		    waitfor == MNT_WAIT ? FSYNC_WAIT : 0, 0, 0)) != 0)
			allerror = error;
		VOP_UNLOCK(ump->um_devvp, 0);
		if (allerror == 0 && waitfor == MNT_WAIT) {
			mutex_enter(&mntvnode_lock);
			goto loop;
		}
	}
#ifdef QUOTA
	qsync(mp);
#endif
	/*
	 * Write back modified superblock.
	 */
	if (fs->fs_fmod != 0) {
		fs->fs_fmod = 0;
		fs->fs_time = time_second;
		if ((error = ffs_cgupdate(ump, waitfor)))
			allerror = error;
	}
	fstrans_done(mp);
	vnfree(mvp);
	return (allerror);
}

/*
 * Look up a FFS dinode number to find its incore vnode, otherwise read it
 * in from disk.  If it is in core, wait for the lock bit to clear, then
 * return the inode locked.  Detection and handling of mount points must be
 * done by the calling routine.
 */
int
ffs_vget(struct mount *mp, ino_t ino, struct vnode **vpp)
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

 retry:
	if ((*vpp = ufs_ihashget(dev, ino, LK_EXCLUSIVE)) != NULL)
		return (0);

	/* Allocate a new vnode/inode. */
	if ((error = getnewvnode(VT_UFS, mp, ffs_vnodeop_p, &vp)) != 0) {
		*vpp = NULL;
		return (error);
	}
	ip = pool_cache_get(ffs_inode_cache, PR_WAITOK);

	/*
	 * If someone beat us to it, put back the freshly allocated
	 * vnode/inode pair and retry.
	 */
	mutex_enter(&ufs_hashlock);
	if (ufs_ihashget(dev, ino, 0) != NULL) {
		mutex_exit(&ufs_hashlock);
		ungetnewvnode(vp);
		pool_cache_put(ffs_inode_cache, ip);
		goto retry;
	}

	vp->v_vflag |= VV_LOCKSWORK;
	if ((mp->mnt_flag & MNT_SOFTDEP) != 0)
		vp->v_uflag |= VU_SOFTDEP;

	/*
	 * XXX MFS ends up here, too, to allocate an inode.  Should we
	 * XXX create another pool for MFS inodes?
	 */

	memset(ip, 0, sizeof(struct inode));
	vp->v_data = ip;
	ip->i_vnode = vp;
	ip->i_ump = ump;
	ip->i_fs = fs = ump->um_fs;
	ip->i_dev = dev;
	ip->i_number = ino;
	LIST_INIT(&ip->i_pcbufhd);
#ifdef QUOTA
	ufsquota_init(ip);
#endif

	/*
	 * Initialize genfs node, we might proceed to destroy it in
	 * error branches.
	 */
	genfs_node_init(vp, &ffs_genfsops);

	/*
	 * Put it onto its hash chain and lock it so that other requests for
	 * this inode will block if they arrive while we are sleeping waiting
	 * for old data structures to be purged or for the contents of the
	 * disk portion of this inode to be read.
	 */

	ufs_ihashins(ip);
	mutex_exit(&ufs_hashlock);

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
		brelse(bp, 0);
		*vpp = NULL;
		return (error);
	}
	if (ip->i_ump->um_fstype == UFS1)
		ip->i_din.ffs1_din = pool_cache_get(ffs_dinode1_cache,
		    PR_WAITOK);
	else
		ip->i_din.ffs2_din = pool_cache_get(ffs_dinode2_cache,
		    PR_WAITOK);
	ffs_load_inode(bp, ip, fs, ino);
	if (DOINGSOFTDEP(vp))
		softdep_load_inodeblock(ip);
	else
		ip->i_ffs_effnlink = ip->i_nlink;
	brelse(bp, 0);

	/*
	 * Initialize the vnode from the inode, check for aliases.
	 * Note that the underlying vnode may have changed.
	 */

	ufs_vinit(mp, ffs_specop_p, ffs_fifoop_p, &vp);

	/*
	 * Finish inode initialization now that aliasing has been resolved.
	 */

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
ffs_fhtovp(struct mount *mp, struct fid *fhp, struct vnode **vpp)
{
	struct ufid ufh;
	struct fs *fs;

	if (fhp->fid_len != sizeof(struct ufid))
		return EINVAL;

	memcpy(&ufh, fhp, sizeof(ufh));
	fs = VFSTOUFS(mp)->um_fs;
	if (ufh.ufid_ino < ROOTINO ||
	    ufh.ufid_ino >= fs->fs_ncg * fs->fs_ipg)
		return (ESTALE);
	return (ufs_fhtovp(mp, &ufh, vpp));
}

/*
 * Vnode pointer to File handle
 */
/* ARGSUSED */
int
ffs_vptofh(struct vnode *vp, struct fid *fhp, size_t *fh_size)
{
	struct inode *ip;
	struct ufid ufh;

	if (*fh_size < sizeof(struct ufid)) {
		*fh_size = sizeof(struct ufid);
		return E2BIG;
	}
	ip = VTOI(vp);
	*fh_size = sizeof(struct ufid);
	memset(&ufh, 0, sizeof(ufh));
	ufh.ufid_len = sizeof(struct ufid);
	ufh.ufid_ino = ip->i_number;
	ufh.ufid_gen = ip->i_gen;
	memcpy(fhp, &ufh, sizeof(ufh));
	return (0);
}

void
ffs_init(void)
{
	if (ffs_initcount++ > 0)
		return;

	ffs_inode_cache = pool_cache_init(sizeof(struct inode), 0, 0, 0,
	    "ffsino", NULL, IPL_NONE, NULL, NULL, NULL);
	ffs_dinode1_cache = pool_cache_init(sizeof(struct ufs1_dinode), 0, 0, 0,
	    "ffsdino1", NULL, IPL_NONE, NULL, NULL, NULL);
	ffs_dinode2_cache = pool_cache_init(sizeof(struct ufs2_dinode), 0, 0, 0,
	    "ffsdino2", NULL, IPL_NONE, NULL, NULL, NULL);
	softdep_initialize();
	ffs_snapshot_init();
	ufs_init();
}

void
ffs_reinit(void)
{
	softdep_reinitialize();
	ufs_reinit();
}

void
ffs_done(void)
{
	if (--ffs_initcount > 0)
		return;

	/* XXX softdep cleanup ? */
	ffs_snapshot_fini();
	ufs_done();
	pool_cache_destroy(ffs_dinode2_cache);
	pool_cache_destroy(ffs_dinode1_cache);
	pool_cache_destroy(ffs_inode_cache);
}

SYSCTL_SETUP(sysctl_vfs_ffs_setup, "sysctl vfs.ffs subtree setup")
{
#if 0
	extern int doasyncfree;
#endif
	extern int ffs_log_changeopt;

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "vfs", NULL,
		       NULL, 0, NULL, 0,
		       CTL_VFS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "ffs",
		       SYSCTL_DESCR("Berkeley Fast File System"),
		       NULL, 0, NULL, 0,
		       CTL_VFS, 1, CTL_EOL);

	/*
	 * @@@ should we even bother with these first three?
	 */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "doclusterread", NULL,
		       sysctl_notavail, 0, NULL, 0,
		       CTL_VFS, 1, FFS_CLUSTERREAD, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "doclusterwrite", NULL,
		       sysctl_notavail, 0, NULL, 0,
		       CTL_VFS, 1, FFS_CLUSTERWRITE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "doreallocblks", NULL,
		       sysctl_notavail, 0, NULL, 0,
		       CTL_VFS, 1, FFS_REALLOCBLKS, CTL_EOL);
#if 0
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "doasyncfree",
		       SYSCTL_DESCR("Release dirty blocks asynchronously"),
		       NULL, 0, &doasyncfree, 0,
		       CTL_VFS, 1, FFS_ASYNCFREE, CTL_EOL);
#endif
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "log_changeopt",
		       SYSCTL_DESCR("Log changes in optimization strategy"),
		       NULL, 0, &ffs_log_changeopt, 0,
		       CTL_VFS, 1, FFS_LOG_CHANGEOPT, CTL_EOL);
}

/*
 * Write a superblock and associated information back to disk.
 */
int
ffs_sbupdate(struct ufsmount *mp, int waitfor)
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

	memcpy(bp->b_data, fs, fs->fs_sbsize);

	ffs_oldfscompat_write((struct fs *)bp->b_data, mp);
#ifdef FFS_EI
	if (mp->um_flags & UFS_NEEDSWAP)
		ffs_sb_swap((struct fs *)bp->b_data, (struct fs *)bp->b_data);
#endif
	fs->fs_flags |= saveflag;

	if (waitfor == MNT_WAIT)
		error = bwrite(bp);
	else
		bawrite(bp);
	return (error);
}

int
ffs_cgupdate(struct ufsmount *mp, int waitfor)
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

int
ffs_extattrctl(struct mount *mp, int cmd, struct vnode *vp,
    int attrnamespace, const char *attrname)
{
#ifdef UFS_EXTATTR
	/*
	 * File-backed extended attributes are only supported on UFS1.
	 * UFS2 has native extended attributes.
	 */
	if (VFSTOUFS(mp)->um_fstype == UFS1)
		return (ufs_extattrctl(mp, cmd, vp, attrnamespace, attrname));
#endif
	return (vfs_stdextattrctl(mp, cmd, vp, attrnamespace, attrname));
}

int
ffs_suspendctl(struct mount *mp, int cmd)
{
	int error;
	struct lwp *l = curlwp;

	switch (cmd) {
	case SUSPEND_SUSPEND:
		if ((error = fstrans_setstate(mp, FSTRANS_SUSPENDING)) != 0)
			return error;
		error = ffs_sync(mp, MNT_WAIT, l->l_proc->p_cred);
		if (error == 0)
			error = fstrans_setstate(mp, FSTRANS_SUSPENDED);
		if (error != 0) {
			(void) fstrans_setstate(mp, FSTRANS_NORMAL);
			return error;
		}
		return 0;

	case SUSPEND_RESUME:
		return fstrans_setstate(mp, FSTRANS_NORMAL);

	default:
		return EINVAL;
	}
}
