/*	$NetBSD: lfs_vfsops.c,v 1.57 2000/07/05 22:25:44 perseant Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Konrad E. Schroder <perseant@hhhh.org>.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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
/*-
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
 *	@(#)lfs_vfsops.c	8.20 (Berkeley) 6/10/95
 */

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_quota.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
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
#include <sys/socket.h>
#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>

#include <miscfs/specfs/specdev.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

int lfs_mountfs __P((struct vnode *, struct mount *, struct proc *));

extern struct vnodeopv_desc lfs_vnodeop_opv_desc;
extern struct vnodeopv_desc lfs_specop_opv_desc;
extern struct vnodeopv_desc lfs_fifoop_opv_desc;

struct vnodeopv_desc *lfs_vnodeopv_descs[] = {
	&lfs_vnodeop_opv_desc,
	&lfs_specop_opv_desc,
	&lfs_fifoop_opv_desc,
	NULL,
};

struct vfsops lfs_vfsops = {
	MOUNT_LFS,
	lfs_mount,
	ufs_start,
	lfs_unmount,
	ufs_root,
	ufs_quotactl,
	lfs_statfs,
	lfs_sync,
	lfs_vget,
	lfs_fhtovp,
	lfs_vptofh,
	lfs_init,
	lfs_done,
	lfs_sysctl,
	lfs_mountroot,
	ufs_check_export,
	lfs_vnodeopv_descs,
};

struct pool lfs_inode_pool;

/*
 * Initialize the filesystem, most work done by ufs_init.
 */
void
lfs_init()
{
	ufs_init();

	/*
	 * XXX Same structure as FFS inodes?  Should we share a common pool?
	 */
	pool_init(&lfs_inode_pool, sizeof(struct inode), 0, 0, 0,
		  "lfsinopl", 0, pool_page_alloc_nointr, pool_page_free_nointr,
		  M_LFSNODE);
}

void
lfs_done()
{
	ufs_done();
	pool_destroy(&lfs_inode_pool);
}

/*
 * Called by main() when ufs is going to be mounted as root.
 */
int
lfs_mountroot()
{
	extern struct vnode *rootvp;
	struct mount *mp;
	struct proc *p = curproc;	/* XXX */
	int error;
	
	if (root_device->dv_class != DV_DISK)
		return (ENODEV);

	if (rootdev == NODEV)
	  	return (ENODEV);
	/*
	 * Get vnodes for swapdev and rootdev.
	 */
	if ((error = bdevvp(rootdev, &rootvp))) {
		printf("lfs_mountroot: can't setup bdevvp's");
		return (error);
	}
	if ((error = vfs_rootmountalloc(MOUNT_LFS, "root_device", &mp))) {
		vrele(rootvp);
		return (error);
	}
	if ((error = lfs_mountfs(rootvp, mp, p))) {
		mp->mnt_op->vfs_refcount--;
		vfs_unbusy(mp);
		free(mp, M_MOUNT);
		vrele(rootvp);
		return (error);
	}
	simple_lock(&mountlist_slock);
	CIRCLEQ_INSERT_TAIL(&mountlist, mp, mnt_list);
	simple_unlock(&mountlist_slock);
	(void)lfs_statfs(mp, &mp->mnt_stat, p);
	vfs_unbusy(mp);
	return (0);
}

/*
 * VFS Operations.
 *
 * mount system call
 */
int
lfs_mount(mp, path, data, ndp, p)
	struct mount *mp;
	const char *path;
	void *data;
	struct nameidata *ndp;
	struct proc *p;
{
	struct vnode *devvp;
	struct ufs_args args;
	struct ufsmount *ump = NULL;
	struct lfs *fs = NULL;				/* LFS */
	size_t size;
	int error;
	mode_t accessmode;

	error = copyin(data, (caddr_t)&args, sizeof (struct ufs_args));
	if (error)
		return (error);

#if 0
	/* Until LFS can do NFS right.		XXX */
	if (args.export.ex_flags & MNT_EXPORTED)
		return (EINVAL);
#endif

	/*
	 * If updating, check whether changing from read-only to
	 * read/write; if there is no device name, that's all we do.
	 */
	if (mp->mnt_flag & MNT_UPDATE) {
		ump = VFSTOUFS(mp);
		fs = ump->um_lfs;
		if (fs->lfs_ronly && (mp->mnt_flag & MNT_WANTRDWR)) {
			/*
			 * If upgrade to read-write by non-root, then verify
			 * that user has necessary permissions on the device.
			 */
			if (p->p_ucred->cr_uid != 0) {
				vn_lock(ump->um_devvp, LK_EXCLUSIVE | LK_RETRY);
				error = VOP_ACCESS(ump->um_devvp, VREAD|VWRITE,
						   p->p_ucred, p);
				VOP_UNLOCK(ump->um_devvp, 0);
				if (error)
					return (error);
			}
			fs->lfs_ronly = 0;
		}
		if (args.fspec == 0) {
			/*
			 * Process export requests.
			 */
			return (vfs_export(mp, &ump->um_export, &args.export));
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
		if (error) {
			vput(devvp);
			return (error);
		}
		VOP_UNLOCK(devvp, 0);
	}
	if ((mp->mnt_flag & MNT_UPDATE) == 0)
		error = lfs_mountfs(devvp, mp, p);		/* LFS */
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
	ump = VFSTOUFS(mp);
	fs = ump->um_lfs;					/* LFS */
	(void)copyinstr(path, fs->lfs_fsmnt, sizeof(fs->lfs_fsmnt) - 1, &size);
	bzero(fs->lfs_fsmnt + size, sizeof(fs->lfs_fsmnt) - size);
	bcopy(fs->lfs_fsmnt, mp->mnt_stat.f_mntonname, MNAMELEN);
	(void) copyinstr(args.fspec, mp->mnt_stat.f_mntfromname, MNAMELEN - 1,
			 &size);
	bzero(mp->mnt_stat.f_mntfromname + size, MNAMELEN - size);
	return (0);
}

/*
 * Common code for mount and mountroot
 * LFS specific
 */
int
lfs_mountfs(devvp, mp, p)
	struct vnode *devvp;
	struct mount *mp;
	struct proc *p;
{
	extern struct vnode *rootvp;
	struct dlfs *dfs, *adfs;
	struct lfs *fs;
	struct ufsmount *ump;
	struct vnode *vp;
	struct buf *bp, *abp;
	struct partinfo dpart;
	dev_t dev;
	int error, i, ronly, size;
	struct ucred *cred;
        SEGUSE *sup;

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
	if ((error = vinvalbuf(devvp, V_SAVE, cred, p, 0, 0)) != 0)
		return (error);

	ronly = (mp->mnt_flag & MNT_RDONLY) != 0;
	error = VOP_OPEN(devvp, ronly ? FREAD : FREAD|FWRITE, FSCRED, p);
	if (error)
		return (error);
	if (VOP_IOCTL(devvp, DIOCGPART, (caddr_t)&dpart, FREAD, cred, p) != 0)
		size = DEV_BSIZE;
	else
		size = dpart.disklab->d_secsize;

	/* Don't free random space on error. */
	bp = NULL;
	abp = NULL;
	ump = NULL;

	/* Read in the superblock. */
	error = bread(devvp, LFS_LABELPAD / size, LFS_SBPAD, cred, &bp);
	if (error)
		goto out;
	dfs = (struct dlfs *)bp->b_data;

	/* Check the basics. */
	if (dfs->dlfs_magic != LFS_MAGIC || dfs->dlfs_bsize > MAXBSIZE ||
	    dfs->dlfs_version > LFS_VERSION ||
	    dfs->dlfs_bsize < sizeof(struct dlfs)) {
		error = EINVAL;		/* XXX needs translation */
		goto out;
	}

	/*
	 * Check the second superblock to see which is newer; then mount
	 * using the older of the two.  This is necessary to ensure that
	 * the filesystem is valid if it was not unmounted cleanly.
	 */
	if (dfs->dlfs_sboffs[1] &&
	    dfs->dlfs_sboffs[1]-(LFS_LABELPAD/size) > LFS_SBPAD/size)
	{
		error = bread(devvp, dfs->dlfs_sboffs[1], LFS_SBPAD, cred, &abp);
		if (error)
			goto out;
		adfs = (struct dlfs *)abp->b_data;

		if (adfs->dlfs_tstamp < dfs->dlfs_tstamp) /* XXX 1s? */
			dfs = adfs;
	} else {
		printf("lfs_mountfs: invalid alt superblock daddr=0x%x\n",
			dfs->dlfs_sboffs[1]);
		error = EINVAL;
		goto out;
	}

	/* Allocate the mount structure, copy the superblock into it. */
	fs = malloc(sizeof(struct lfs), M_UFSMNT, M_WAITOK);
	memcpy(&fs->lfs_dlfs, dfs, sizeof(struct dlfs));
	ump = malloc(sizeof *ump, M_UFSMNT, M_WAITOK);
	memset((caddr_t)ump, 0, sizeof *ump);
	ump->um_lfs = fs;
	if (sizeof(struct lfs) < LFS_SBPAD)			/* XXX why? */
		bp->b_flags |= B_INVAL;
	brelse(bp);
	bp = NULL;
	brelse(abp);
	abp = NULL;

	/* Set up the I/O information */
	fs->lfs_iocount = 0;
	fs->lfs_diropwait = 0;
	fs->lfs_activesb = 0;
	fs->lfs_uinodes = 0;
#ifdef LFS_CANNOT_ROLLFW
	fs->lfs_sbactive = 0;
#endif
#ifdef LFS_TRACK_IOS
	for (i=0;i<LFS_THROTTLE;i++)
		fs->lfs_pending[i] = LFS_UNUSED_DADDR;
#endif
	if (fs->lfs_minfreeseg == 0)
		fs->lfs_minfreeseg = MIN_FREE_SEGS;

	/* Set up the ifile and lock aflags */
	fs->lfs_doifile = 0;
	fs->lfs_writer = 0;
	fs->lfs_dirops = 0;
	fs->lfs_nadirop = 0;
	fs->lfs_seglock = 0;
	lockinit(&fs->lfs_freelock, PINOD, "lfs_freelock", 0, 0);

	/* Set the file system readonly/modify bits. */
	fs->lfs_ronly = ronly;
	if (ronly == 0)
		fs->lfs_fmod = 1;

	/* Initialize the mount structure. */
	dev = devvp->v_rdev;
	mp->mnt_data = (qaddr_t)ump;
	mp->mnt_stat.f_fsid.val[0] = (long)dev;
	mp->mnt_stat.f_fsid.val[1] = makefstype(MOUNT_LFS);
	mp->mnt_stat.f_iosize = fs->lfs_bsize;
	mp->mnt_maxsymlinklen = fs->lfs_maxsymlinklen;
	mp->mnt_flag |= MNT_LOCAL;
	ump->um_flags = 0;
	ump->um_mountp = mp;
	ump->um_dev = dev;
	ump->um_devvp = devvp;
	ump->um_bptrtodb = 0;
	ump->um_seqinc = 1 << fs->lfs_fsbtodb;
	ump->um_nindir = fs->lfs_nindir;
	for (i = 0; i < MAXQUOTAS; i++)
		ump->um_quotas[i] = NULLVP;
	devvp->v_specmountpoint = mp;

	/*
	 * We use the ifile vnode for almost every operation.  Instead of
	 * retrieving it from the hash table each time we retrieve it here,
	 * artificially increment the reference count and keep a pointer
	 * to it in the incore copy of the superblock.
	 */
	if ((error = VFS_VGET(mp, LFS_IFILE_INUM, &vp)) != 0)
		goto out;
	fs->lfs_ivnode = vp;
	VREF(vp);
	vput(vp);

	/*
	 * Mark the current segment as ACTIVE, since we're going to 
	 * be writing to it.
	 */
        LFS_SEGENTRY(sup, fs, datosn(fs, fs->lfs_offset), bp); 
        sup->su_flags |= SEGUSE_DIRTY | SEGUSE_ACTIVE;
        (void) VOP_BWRITE(bp); 

	return (0);
out:
	if (bp)
		brelse(bp);
	if (abp)
		brelse(abp);
	vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY);
	(void)VOP_CLOSE(devvp, ronly ? FREAD : FREAD|FWRITE, cred, p);
	VOP_UNLOCK(devvp, 0);
	if (ump) {
		free(ump->um_lfs, M_UFSMNT);
		free(ump, M_UFSMNT);
		mp->mnt_data = (qaddr_t)0;
	}
	return (error);
}

/*
 * unmount system call
 */
int
lfs_unmount(mp, mntflags, p)
	struct mount *mp;
	int mntflags;
	struct proc *p;
{
	struct ufsmount *ump;
	struct lfs *fs;
	int error, flags, ronly;
	extern int lfs_allclean_wakeup;

	flags = 0;
	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;

	ump = VFSTOUFS(mp);
	fs = ump->um_lfs;
#ifdef QUOTA
	if (mp->mnt_flag & MNT_QUOTA) {
		int i;
		error = vflush(mp, fs->lfs_ivnode, SKIPSYSTEM|flags);
		if (error)
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
	if ((error = vflush(mp, fs->lfs_ivnode, flags)) != 0)
		return (error);
	fs->lfs_clean = 1;
	if ((error = VFS_SYNC(mp, 1, p->p_ucred, p)) != 0)
		return (error);
	if (fs->lfs_ivnode->v_dirtyblkhd.lh_first)
		panic("lfs_unmount: still dirty blocks on ifile vnode\n");
	vrele(fs->lfs_ivnode);
	vgone(fs->lfs_ivnode);

	ronly = !fs->lfs_ronly;
	if (ump->um_devvp->v_type != VBAD)
		ump->um_devvp->v_specmountpoint = NULL;
	vn_lock(ump->um_devvp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_CLOSE(ump->um_devvp,
	    ronly ? FREAD : FREAD|FWRITE, NOCRED, p);
	vput(ump->um_devvp);

	/* XXX KS - wake up the cleaner so it can die */
	wakeup(&fs->lfs_nextseg);
	wakeup(&lfs_allclean_wakeup);

	free(fs, M_UFSMNT);
	free(ump, M_UFSMNT);
	mp->mnt_data = (qaddr_t)0;
	mp->mnt_flag &= ~MNT_LOCAL;
	return (error);
}

/*
 * Get file system statistics.
 */
int
lfs_statfs(mp, sbp, p)
	struct mount *mp;
	struct statfs *sbp;
	struct proc *p;
{
	struct lfs *fs;
	struct ufsmount *ump;

	ump = VFSTOUFS(mp);
	fs = ump->um_lfs;
	if (fs->lfs_magic != LFS_MAGIC)
		panic("lfs_statfs: magic");

	sbp->f_type = 0;
	sbp->f_bsize = fs->lfs_fsize;
	sbp->f_iosize = fs->lfs_bsize;
	sbp->f_blocks = dbtofrags(fs, LFS_EST_NONMETA(fs));
	sbp->f_bfree = dbtofrags(fs, LFS_EST_BFREE(fs));
	sbp->f_bavail = dbtofrags(fs, (long)LFS_EST_BFREE(fs) -
				  (long)LFS_EST_RSVD(fs));
	sbp->f_files = dbtofsb(fs,fs->lfs_bfree) * INOPB(fs);
	sbp->f_ffree = sbp->f_files - fs->lfs_nfiles;
	if (sbp != &mp->mnt_stat) {
		bcopy(mp->mnt_stat.f_mntonname, sbp->f_mntonname, MNAMELEN);
		bcopy(mp->mnt_stat.f_mntfromname, sbp->f_mntfromname, MNAMELEN);
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
lfs_sync(mp, waitfor, cred, p)
	struct mount *mp;
	int waitfor;
	struct ucred *cred;
	struct proc *p;
{
	int error;
	struct lfs *fs;

	fs = ((struct ufsmount *)mp->mnt_data)->ufsmount_u.lfs;
	if (fs->lfs_ronly)
		return 0;
	while(fs->lfs_dirops)
		error = tsleep(&fs->lfs_dirops, PRIBIO + 1, "lfs_dirops", 0);
	fs->lfs_writer++;

	/* All syncs must be checkpoints until roll-forward is implemented. */
	error = lfs_segwrite(mp, SEGM_CKP | (waitfor ? SEGM_SYNC : 0));
	if(--fs->lfs_writer==0)
		wakeup(&fs->lfs_dirops);
#ifdef QUOTA
	qsync(mp);
#endif
	return (error);
}

extern struct lock ufs_hashlock;

/*
 * Look up an LFS dinode number to find its incore vnode.  If not already
 * in core, read it in from the specified device.  Return the inode locked.
 * Detection and handling of mount points must be done by the calling routine.
 */
int
lfs_vget(mp, ino, vpp)
	struct mount *mp;
	ino_t ino;
	struct vnode **vpp;
{
	struct lfs *fs;
	struct inode *ip;
	struct buf *bp;
	struct ifile *ifp;
	struct vnode *vp;
	struct ufsmount *ump;
	ufs_daddr_t daddr;
	dev_t dev;
	int error;
#ifdef LFS_ATIME_IFILE
	struct timespec ts;
#endif

	ump = VFSTOUFS(mp);
	dev = ump->um_dev;

	if ((*vpp = ufs_ihashget(dev, ino, LK_EXCLUSIVE)) != NULL)
		return (0);

	if ((error = getnewvnode(VT_LFS, mp, lfs_vnodeop_p, &vp)) != 0) {
		*vpp = NULL;
		 return (error);
	}

	do {
		if ((*vpp = ufs_ihashget(dev, ino, LK_EXCLUSIVE)) != NULL) {
			ungetnewvnode(vp);
			return (0);
		}
	} while (lockmgr(&ufs_hashlock, LK_EXCLUSIVE|LK_SLEEPFAIL, 0));

	/* Translate the inode number to a disk address. */
	fs = ump->um_lfs;
	if (ino == LFS_IFILE_INUM)
		daddr = fs->lfs_idaddr;
	else {
		LFS_IENTRY(ifp, fs, ino, bp);
		daddr = ifp->if_daddr;
#ifdef LFS_ATIME_IFILE
		ts = ifp->if_atime; /* structure copy */
#endif
		brelse(bp);
		if (daddr == LFS_UNUSED_DADDR) {
			lockmgr(&ufs_hashlock, LK_RELEASE, 0);
			return (ENOENT);
		}
	}

	/* Allocate/init new vnode/inode. */
	lfs_vcreate(mp, ino, vp);

	/*
	 * Put it onto its hash chain and lock it so that other requests for
	 * this inode will block if they arrive while we are sleeping waiting
	 * for old data structures to be purged or for the contents of the
	 * disk portion of this inode to be read.
	 */
	ip = VTOI(vp);
	ufs_ihashins(ip);
	lockmgr(&ufs_hashlock, LK_RELEASE, 0);

	/*
	 * XXX
	 * This may not need to be here, logically it should go down with
	 * the i_devvp initialization.
	 * Ask Kirk.
	 */
	ip->i_lfs = ump->um_lfs;

	/* Read in the disk contents for the inode, copy into the inode. */
	error = bread(ump->um_devvp, daddr, (int)fs->lfs_bsize, NOCRED, &bp);
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
	ip->i_din.ffs_din = *lfs_ifind(fs, ino, bp);
	ip->i_ffs_effnlink = ip->i_ffs_nlink;
	ip->i_lfs_effnblks = ip->i_ffs_blocks;
#ifdef LFS_ATIME_IFILE
	ip->i_ffs_atime = ts.tv_sec;
	ip->i_ffs_atimensec = ts.tv_nsec;
#endif
	brelse(bp);

	/*
	 * Initialize the vnode from the inode, check for aliases.  In all
	 * cases re-init ip, the underlying vnode/inode may have changed.
	 */
	error = ufs_vinit(mp, lfs_specop_p, lfs_fifoop_p, &vp);
	if (error) {
		vput(vp);
		*vpp = NULL;
		return (error);
	}
	if(vp->v_type == VNON) {
		printf("lfs_vget: ino %d is type VNON! (ifmt %o)\n", ip->i_number, (ip->i_ffs_mode&IFMT)>>12);
#ifdef DDB
		Debugger();
#endif
	}
	/*
	 * Finish inode initialization now that aliasing has been resolved.
	 */
	ip->i_devvp = ump->um_devvp;
	VREF(ip->i_devvp);
	*vpp = vp;

	return (0);
}

/*
 * File handle to vnode
 *
 * Have to be really careful about stale file handles:
 * - check that the inode number is valid
 * - call lfs_vget() to get the locked inode
 * - check for an unallocated inode (i_mode == 0)
 *
 * XXX
 * use ifile to see if inode is allocated instead of reading off disk
 * what is the relationship between my generational number and the NFS
 * generational number.
 */
int
lfs_fhtovp(mp, fhp, vpp)
	struct mount *mp;
	struct fid *fhp;
	struct vnode **vpp;
{
	struct ufid *ufhp;

	ufhp = (struct ufid *)fhp;
	if (ufhp->ufid_ino < ROOTINO)
		return (ESTALE);
	return (ufs_fhtovp(mp, ufhp, vpp));
}

/*
 * Vnode pointer to File handle
 */
/* ARGSUSED */
int
lfs_vptofh(vp, fhp)
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

int
lfs_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	extern int lfs_writeindir, lfs_dostats, lfs_clean_vnhead;
	extern struct lfs_stats lfs_stats;
	int error;

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);

	switch (name[0]) {
	case LFS_WRITEINDIR:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
				   &lfs_writeindir));
	case LFS_CLEAN_VNHEAD:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
				   &lfs_clean_vnhead));
	case LFS_DOSTATS:
		if((error = sysctl_int(oldp, oldlenp, newp, newlen,
				       &lfs_dostats)))
			return error;
		if(lfs_dostats == 0)
			memset(&lfs_stats,0,sizeof(lfs_stats));
		return 0;
	case LFS_STATS:
		return (sysctl_rdstruct(oldp, oldlenp, newp,
					&lfs_stats, sizeof(lfs_stats)));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}
