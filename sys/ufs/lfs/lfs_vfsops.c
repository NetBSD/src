/*	$NetBSD: lfs_vfsops.c,v 1.148 2004/04/22 10:45:56 yamt Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2001, 2002, 2003 The NetBSD Foundation, Inc.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 *	@(#)lfs_vfsops.c	8.20 (Berkeley) 6/10/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lfs_vfsops.c,v 1.148 2004/04/22 10:45:56 yamt Exp $");

#if defined(_KERNEL_OPT)
#include "opt_quota.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/kthread.h>
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
#include <sys/conf.h>

#include <miscfs/specfs/specdev.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

#include <uvm/uvm.h>
#include <uvm/uvm_stat.h>
#include <uvm/uvm_pager.h>
#include <uvm/uvm_pdaemon.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/genfs/genfs_node.h>

static int lfs_gop_write(struct vnode *, struct vm_page **, int, int);
static boolean_t lfs_issequential_hole(const struct ufsmount *,
    daddr_t, daddr_t);

static int lfs_mountfs(struct vnode *, struct mount *, struct proc *);
static daddr_t check_segsum(struct lfs *, daddr_t, u_int64_t,
    struct ucred *, int, int *, struct proc *);

extern const struct vnodeopv_desc lfs_vnodeop_opv_desc;
extern const struct vnodeopv_desc lfs_specop_opv_desc;
extern const struct vnodeopv_desc lfs_fifoop_opv_desc;

pid_t lfs_writer_daemon = 0;
int lfs_do_flush = 0;

const struct vnodeopv_desc * const lfs_vnodeopv_descs[] = {
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
	lfs_statvfs,
	lfs_sync,
	lfs_vget,
	lfs_fhtovp,
	lfs_vptofh,
	lfs_init,
	lfs_reinit,
	lfs_done,
	NULL,
	lfs_mountroot,
	ufs_check_export,
	lfs_vnodeopv_descs,
};

struct genfs_ops lfs_genfsops = {
	lfs_gop_size,
	ufs_gop_alloc,
	lfs_gop_write,
};

struct pool lfs_inode_pool;
struct pool lfs_dinode_pool;
struct pool lfs_inoext_pool;

/*
 * The writer daemon.  UVM keeps track of how many dirty pages we are holding
 * in lfs_subsys_pages; the daemon flushes the filesystem when this value
 * crosses the (user-defined) threshhold LFS_MAX_PAGES.
 */
static void
lfs_writerd(void *arg)
{
#ifdef LFS_PD
	struct mount *mp, *nmp;
	struct lfs *fs;
#endif

	lfs_writer_daemon = curproc->p_pid;

	simple_lock(&lfs_subsys_lock);
	for (;;) {
		ltsleep(&lfs_writer_daemon, PVM | PNORELOCK, "lfswriter", 0,
		    &lfs_subsys_lock);

#ifdef LFS_PD
		/*
		 * Look through the list of LFSs to see if any of them
		 * have requested pageouts.
		 */
		simple_lock(&mountlist_slock);
		for (mp = CIRCLEQ_FIRST(&mountlist); mp != (void *)&mountlist;
		     mp = nmp) {
			if (vfs_busy(mp, LK_NOWAIT, &mountlist_slock)) {
				nmp = CIRCLEQ_NEXT(mp, mnt_list);
				continue;
			}
			if (strncmp(&mp->mnt_stat.f_fstypename[0], MOUNT_LFS,
				    MFSNAMELEN) == 0) {
				fs = VFSTOUFS(mp)->um_lfs;
				if (fs->lfs_pdflush ||
				    !TAILQ_EMPTY(&fs->lfs_pchainhd)) {
					fs->lfs_pdflush = 0;
					lfs_flush_fs(fs, 0);
				}
			}

			simple_lock(&mountlist_slock);
			nmp = CIRCLEQ_NEXT(mp, mnt_list);
			vfs_unbusy(mp);
		}
		simple_unlock(&mountlist_slock);
#endif /* LFS_PD */

		/*
		 * If global state wants a flush, flush everything.
		 */
		simple_lock(&lfs_subsys_lock);
		while (lfs_do_flush || locked_queue_count > LFS_MAX_BUFS || 
			locked_queue_bytes > LFS_MAX_BYTES ||
			lfs_subsys_pages > LFS_MAX_PAGES) {

#ifdef DEBUG_LFS_FLUSH
			if (lfs_do_flush)
				printf("daemon: lfs_do_flush\n");
			if (locked_queue_count > LFS_MAX_BUFS)
				printf("daemon: lqc = %d, max %d\n",
					locked_queue_count, LFS_MAX_BUFS);
			if (locked_queue_bytes > LFS_MAX_BYTES)
				printf("daemon: lqb = %ld, max %ld\n",
					locked_queue_bytes, LFS_MAX_BYTES);
			if (lfs_subsys_pages > LFS_MAX_PAGES) 
				printf("daemon: lssp = %d, max %d\n",
					lfs_subsys_pages, LFS_MAX_PAGES);
#endif /* DEBUG_LFS_FLUSH */
			lfs_flush(NULL, SEGM_WRITERD);
			lfs_do_flush = 0;
		}
	}
	/* NOTREACHED */
}

/*
 * Initialize the filesystem, most work done by ufs_init.
 */
void
lfs_init()
{
#ifdef _LKM
	malloc_type_attach(M_SEGMENT);
#endif
	ufs_init();

	/*
	 * XXX Same structure as FFS inodes?  Should we share a common pool?
	 */
	pool_init(&lfs_inode_pool, sizeof(struct inode), 0, 0, 0,
		  "lfsinopl", &pool_allocator_nointr);
	pool_init(&lfs_dinode_pool, sizeof(struct ufs1_dinode), 0, 0, 0,
		  "lfsdinopl", &pool_allocator_nointr);
	pool_init(&lfs_inoext_pool, sizeof(struct lfs_inode_ext), 8, 0, 0,
		  "lfsinoextpl", &pool_allocator_nointr);
#ifdef DEBUG
	memset(lfs_log, 0, sizeof(lfs_log));
#endif
	simple_lock_init(&lfs_subsys_lock);
}

void
lfs_reinit()
{
	ufs_reinit();
}

void
lfs_done()
{
	ufs_done();
	pool_destroy(&lfs_inode_pool);
	pool_destroy(&lfs_dinode_pool);
	pool_destroy(&lfs_inoext_pool);
#ifdef _LKM
	malloc_type_detach(M_SEGMENT);
#endif
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
	(void)lfs_statvfs(mp, &mp->mnt_stat, p);
	vfs_unbusy(mp);
	inittodr(VFSTOUFS(mp)->um_lfs->lfs_tstamp);
	return (0);
}

/*
 * VFS Operations.
 *
 * mount system call
 */
int
lfs_mount(struct mount *mp, const char *path, void *data, struct nameidata *ndp, struct proc *p)
{
	struct vnode *devvp;
	struct ufs_args args;
	struct ufsmount *ump = NULL;
	struct lfs *fs = NULL;				/* LFS */
	int error;
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

	/*
	 * If updating, check whether changing from read-only to
	 * read/write; if there is no device name, that's all we do.
	 */
	if (mp->mnt_flag & MNT_UPDATE) {
		ump = VFSTOUFS(mp);
		fs = ump->um_lfs;
		if (fs->lfs_ronly && (mp->mnt_iflag & IMNT_WANTRDWR)) {
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
	if (bdevsw_lookup(devvp->v_rdev) == NULL) {
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
	return set_statvfs_info(path, UIO_USERSPACE, args.fspec,
	    UIO_USERSPACE, mp, p);
}

/*
 * Roll-forward code.
 */

/*
 * Load the appropriate indirect block, and change the appropriate pointer.
 * Mark the block dirty.  Do segment and avail accounting.
 */
static int
update_meta(struct lfs *fs, ino_t ino, int version, daddr_t lbn,
	    daddr_t ndaddr, size_t size, struct proc *p)
{
	int error;
	struct vnode *vp;
	struct inode *ip;
#ifdef DEBUG_LFS_RFW
	daddr_t odaddr;
	struct indir a[NIADDR];
	int num;
	int i;
#endif /* DEBUG_LFS_RFW */
	struct buf *bp;
	SEGUSE *sup;

	KASSERT(lbn >= 0);	/* no indirect blocks */

	if ((error = lfs_rf_valloc(fs, ino, version, p, &vp)) != 0) {
#ifdef DEBUG_LFS_RFW
		printf("update_meta: ino %d: lfs_rf_valloc returned %d\n", ino,
		       error);
#endif /* DEBUG_LFS_RFW */
		return error;
	}

	if ((error = VOP_BALLOC(vp, (lbn << fs->lfs_bshift), size,
				NOCRED, 0, &bp)) != 0) {
		vput(vp);
		return (error);
	}
	/* No need to write, the block is already on disk */
	if (bp->b_flags & B_DELWRI) {
		LFS_UNLOCK_BUF(bp);
		fs->lfs_avail += btofsb(fs, bp->b_bcount);
	}
	bp->b_flags |= B_INVAL;
	brelse(bp);

	/*
	 * Extend the file, if it is not large enough already.
	 * XXX this is not exactly right, we don't know how much of the
	 * XXX last block is actually used.  We hope that an inode will
	 * XXX appear later to give the correct size.
	 */
	ip = VTOI(vp);
	if (ip->i_size <= (lbn << fs->lfs_bshift)) {
		u_int64_t newsize;

		if (lbn < NDADDR)
			newsize = ip->i_ffs1_size = (lbn << fs->lfs_bshift) +
				(size - fs->lfs_fsize) + 1;
		else
			newsize = ip->i_ffs1_size = (lbn << fs->lfs_bshift) + 1;

		if (ip->i_size < newsize) {
			ip->i_size = newsize;
			/*
			 * tell vm our new size for the case the inode won't
			 * appear later.
			 */
			uvm_vnp_setsize(vp, newsize);
		}
	}

	lfs_update_single(fs, NULL, vp, lbn, ndaddr, size);

	LFS_SEGENTRY(sup, fs, dtosn(fs, ndaddr), bp);
	sup->su_nbytes += size;
	LFS_WRITESEGENTRY(sup, fs, dtosn(fs, ndaddr), bp);

	/* differences here should be due to UNWRITTEN indirect blocks. */
	KASSERT((lblkno(fs, ip->i_size) > NDADDR &&
	    ip->i_lfs_effnblks == ip->i_ffs1_blocks) ||
	    ip->i_lfs_effnblks >= ip->i_ffs1_blocks);

#ifdef DEBUG_LFS_RFW
	/* Now look again to make sure it worked */
	ufs_bmaparray(vp, lbn, &odaddr, &a[0], &num, NULL, NULL);
	for (i = num; i > 0; i--) {
		if (!a[i].in_exists)
			panic("update_meta: absent %d lv indirect block", i);
	}
	if (dbtofsb(fs, odaddr) != ndaddr)
		printf("update_meta: failed setting ino %d lbn %" PRId64
		    " to %" PRId64 "\n", ino, lbn, ndaddr);
#endif /* DEBUG_LFS_RFW */
	vput(vp);
	return 0;
}

static int
update_inoblk(struct lfs *fs, daddr_t offset, struct ucred *cred,
	      struct proc *p)
{
	struct vnode *devvp, *vp;
	struct inode *ip;
	struct ufs1_dinode *dip;
	struct buf *dbp, *ibp;
	int error;
	daddr_t daddr;
	IFILE *ifp;
	SEGUSE *sup;

	devvp = VTOI(fs->lfs_ivnode)->i_devvp;

	/*
	 * Get the inode, update times and perms.
	 * DO NOT update disk blocks, we do that separately.
	 */
	error = bread(devvp, fsbtodb(fs, offset), fs->lfs_ibsize, cred, &dbp);
	if (error) {
#ifdef DEBUG_LFS_RFW
		printf("update_inoblk: bread returned %d\n", error);
#endif
		return error;
	}
	dip = ((struct ufs1_dinode *)(dbp->b_data)) + INOPB(fs);
	while (--dip >= (struct ufs1_dinode *)dbp->b_data) {
		if (dip->di_inumber > LFS_IFILE_INUM) {
			/* printf("ino %d version %d\n", dip->di_inumber,
			       dip->di_gen); */
			error = lfs_rf_valloc(fs, dip->di_inumber, dip->di_gen,
					      p, &vp);
			if (error) {
#ifdef DEBUG_LFS_RFW
				printf("update_inoblk: lfs_rf_valloc returned %d\n", error);
#endif
				continue;
			}
			ip = VTOI(vp);
			if (dip->di_size != ip->i_size)
				VOP_TRUNCATE(vp, dip->di_size, 0, NOCRED, p);
			/* Get mode, link count, size, and times */
			memcpy(ip->i_din.ffs1_din, dip, 
			       offsetof(struct ufs1_dinode, di_db[0]));

			/* Then the rest, except di_blocks */
			ip->i_flags = ip->i_ffs1_flags = dip->di_flags;
			ip->i_gen = ip->i_ffs1_gen = dip->di_gen;
			ip->i_uid = ip->i_ffs1_uid = dip->di_uid;
			ip->i_gid = ip->i_ffs1_gid = dip->di_gid;

			ip->i_mode = ip->i_ffs1_mode;
			ip->i_nlink = ip->i_ffs_effnlink = ip->i_ffs1_nlink;
			ip->i_size = ip->i_ffs1_size;

			LFS_SET_UINO(ip, IN_CHANGE | IN_MODIFIED | IN_UPDATE);

			/* Re-initialize to get type right */
			ufs_vinit(vp->v_mount, lfs_specop_p, lfs_fifoop_p,
				  &vp);
			vput(vp);

			/* Record change in location */
			LFS_IENTRY(ifp, fs, dip->di_inumber, ibp);
			daddr = ifp->if_daddr;
			ifp->if_daddr = dbtofsb(fs, dbp->b_blkno);
			error = LFS_BWRITE_LOG(ibp); /* Ifile */
			/* And do segment accounting */
			if (dtosn(fs, daddr) != dtosn(fs, dbtofsb(fs, dbp->b_blkno))) {
				if (daddr > 0) {
					LFS_SEGENTRY(sup, fs, dtosn(fs, daddr),
						     ibp);
					sup->su_nbytes -= sizeof (struct ufs1_dinode);
					LFS_WRITESEGENTRY(sup, fs,
							  dtosn(fs, daddr),
							  ibp);
				}
				LFS_SEGENTRY(sup, fs, dtosn(fs, dbtofsb(fs, dbp->b_blkno)),
					     ibp);
				sup->su_nbytes += sizeof (struct ufs1_dinode);
				LFS_WRITESEGENTRY(sup, fs,
						  dtosn(fs, dbtofsb(fs, dbp->b_blkno)),
						  ibp);
			}
		}
	}
	dbp->b_flags |= B_AGE;
	brelse(dbp);

	return 0;
}

#define CHECK_CKSUM   0x0001  /* Check the checksum to make sure it's valid */
#define CHECK_UPDATE  0x0002  /* Update Ifile for new data blocks / inodes */

static daddr_t
check_segsum(struct lfs *fs, daddr_t offset, u_int64_t nextserial,
	     struct ucred *cred, int flags, int *pseg_flags, struct proc *p)
{
	struct vnode *devvp;
	struct buf *bp, *dbp;
	int error, nblocks = 0, ninos, i, j; /* XXX: gcc */
	SEGSUM *ssp;
	u_long *dp = NULL, *datap = NULL; /* XXX u_int32_t */
	daddr_t oldoffset;
	int32_t *iaddr;	/* XXX ondisk32 */
	FINFO *fip;
	SEGUSE *sup;
	size_t size;

	devvp = VTOI(fs->lfs_ivnode)->i_devvp;
	/*
	 * If the segment has a superblock and we're at the top
	 * of the segment, skip the superblock.
	 */
	if (sntod(fs, dtosn(fs, offset)) == offset) {
		LFS_SEGENTRY(sup, fs, dtosn(fs, offset), bp); 
		if (sup->su_flags & SEGUSE_SUPERBLOCK)
			offset += btofsb(fs, LFS_SBPAD);
		brelse(bp);
	}

	/* Read in the segment summary */
	error = bread(devvp, fsbtodb(fs, offset), fs->lfs_sumsize, cred, &bp);
	if (error)
		return -1;
	
	/* Check summary checksum */
	ssp = (SEGSUM *)bp->b_data;
	if (flags & CHECK_CKSUM) {
		if (ssp->ss_sumsum != cksum(&ssp->ss_datasum,
					   fs->lfs_sumsize -
					   sizeof(ssp->ss_sumsum))) {
#ifdef DEBUG_LFS_RFW
			printf("Sumsum error at 0x%" PRIx64 "\n", offset);
#endif
			offset = -1;
			goto err1;
		}
		if (ssp->ss_nfinfo == 0 && ssp->ss_ninos == 0) {
#ifdef DEBUG_LFS_RFW
			printf("Empty pseg at 0x%" PRIx64 "\n", offset);
#endif
			offset = -1;
			goto err1;
		}
		if (ssp->ss_create < fs->lfs_tstamp) {
#ifdef DEBUG_LFS_RFW
			printf("Old data at 0x%" PRIx64 "\n", offset);
#endif
			offset = -1;
			goto err1;
		}
	}
	if (fs->lfs_version > 1) {
		if (ssp->ss_serial != nextserial) {
#ifdef DEBUG_LFS_RFW
			printf("Unexpected serial number at 0x%" PRIx64
			    "\n", offset);
#endif
			offset = -1;
			goto err1;
		}
		if (ssp->ss_ident != fs->lfs_ident) {
#ifdef DEBUG_LFS_RFW
			printf("Incorrect fsid (0x%x vs 0x%x) at 0x%"
			    PRIx64 "\n", ssp->ss_ident, fs->lfs_ident, offset);
#endif
			offset = -1;
			goto err1;
		}
	}
	if (pseg_flags)
		*pseg_flags = ssp->ss_flags;
	oldoffset = offset;
	offset += btofsb(fs, fs->lfs_sumsize);

	ninos = howmany(ssp->ss_ninos, INOPB(fs));
	/* XXX ondisk32 */
	iaddr = (int32_t *)(bp->b_data + fs->lfs_sumsize - sizeof(int32_t));
	if (flags & CHECK_CKSUM) {
		/* Count blocks */
		nblocks = 0;
		fip = (FINFO *)(bp->b_data + SEGSUM_SIZE(fs));
		for (i = 0; i < ssp->ss_nfinfo; ++i) {
			nblocks += fip->fi_nblocks;
			if (fip->fi_nblocks <= 0)
				break;
			/* XXX ondisk32 */
			fip = (FINFO *)(((char *)fip) + FINFOSIZE +
					(fip->fi_nblocks * sizeof(int32_t)));
		}
		nblocks += ninos;
		/* Create the sum array */
		datap = dp = (u_long *)malloc(nblocks * sizeof(u_long),
					      M_SEGMENT, M_WAITOK);
	}

	/* Handle individual blocks */
	fip = (FINFO *)(bp->b_data + SEGSUM_SIZE(fs));
	for (i = 0; i < ssp->ss_nfinfo || ninos; ++i) {
		/* Inode block? */
		if (ninos && *iaddr == offset) {
			if (flags & CHECK_CKSUM) {
				/* Read in the head and add to the buffer */
				error = bread(devvp, fsbtodb(fs, offset), fs->lfs_bsize,
					      cred, &dbp);
				if (error) {
					offset = -1;
					goto err2;
				}
				(*dp++) = ((u_long *)(dbp->b_data))[0];
				dbp->b_flags |= B_AGE;
				brelse(dbp);
			}
			if (flags & CHECK_UPDATE) {
				if ((error = update_inoblk(fs, offset, cred, p))
				    != 0) {
					offset = -1;
					goto err2;
				}
			}
			offset += btofsb(fs, fs->lfs_ibsize);
			--iaddr;
			--ninos;
			--i; /* compensate */
			continue;
		}
		/* printf("check: blocks from ino %d version %d\n",
		       fip->fi_ino, fip->fi_version); */
		size = fs->lfs_bsize;
		for (j = 0; j < fip->fi_nblocks; ++j) {
			if (j == fip->fi_nblocks - 1)
				size = fip->fi_lastlength;
			if (flags & CHECK_CKSUM) {
				error = bread(devvp, fsbtodb(fs, offset), size, cred, &dbp);
				if (error) {
					offset = -1;
					goto err2;
				}
				(*dp++) = ((u_long *)(dbp->b_data))[0];
				dbp->b_flags |= B_AGE;
				brelse(dbp);
			}
			/* Account for and update any direct blocks */
			if ((flags & CHECK_UPDATE) &&
			   fip->fi_ino > LFS_IFILE_INUM &&
			   fip->fi_blocks[j] >= 0) {
				update_meta(fs, fip->fi_ino, fip->fi_version,
					    fip->fi_blocks[j], offset, size, p);
			}
			offset += btofsb(fs, size);
		}
		/* XXX ondisk32 */
		fip = (FINFO *)(((char *)fip) + FINFOSIZE
				+ fip->fi_nblocks * sizeof(int32_t));
	}
	/* Checksum the array, compare */
	if ((flags & CHECK_CKSUM) &&
	   ssp->ss_datasum != cksum(datap, nblocks * sizeof(u_long)))
	{
#ifdef DEBUG_LFS_RFW
		printf("Datasum error at 0x%" PRIx64 " (wanted %x got %x)\n",
		    offset, ssp->ss_datasum, cksum(datap, nblocks *
					      sizeof(u_long)));
#endif
		offset = -1;
		goto err2;
	}

	/* If we're at the end of the segment, move to the next */
	if (dtosn(fs, offset + btofsb(fs, fs->lfs_sumsize + fs->lfs_bsize)) !=
	   dtosn(fs, offset)) {
		if (dtosn(fs, offset) == dtosn(fs, ssp->ss_next)) {
			offset = -1;
			goto err2;
		}
		offset = ssp->ss_next;
#ifdef DEBUG_LFS_RFW
		printf("LFS roll forward: moving on to offset 0x%" PRIx64
		       " -> segment %d\n", offset, dtosn(fs,offset));
#endif
	}

	if (flags & CHECK_UPDATE) {
		fs->lfs_avail -= (offset - oldoffset);
		/* Don't clog the buffer queue */
		simple_lock(&lfs_subsys_lock);
		if (locked_queue_count > LFS_MAX_BUFS ||
		    locked_queue_bytes > LFS_MAX_BYTES) {
			lfs_flush(fs, SEGM_CKP);
		}
		simple_unlock(&lfs_subsys_lock);
	}

    err2:
	if (flags & CHECK_CKSUM)
		free(datap, M_SEGMENT);
    err1:
	bp->b_flags |= B_AGE;
	brelse(bp);

	/* XXX should we update the serial number even for bad psegs? */
	if ((flags & CHECK_UPDATE) && offset > 0 && fs->lfs_version > 1)
		fs->lfs_serial = nextserial;
	return offset;
}

/*
 * Common code for mount and mountroot
 * LFS specific
 */
int
lfs_mountfs(struct vnode *devvp, struct mount *mp, struct proc *p)
{
	extern struct vnode *rootvp;
	struct dlfs *tdfs, *dfs, *adfs;
	struct lfs *fs;
	struct ufsmount *ump;
	struct vnode *vp;
	struct buf *bp, *abp;
	struct partinfo dpart;
	dev_t dev;
	int error, i, ronly, secsize, fsbsize;
	struct ucred *cred;
	CLEANERINFO *cip;
	SEGUSE *sup;
	int flags, dirty, do_rollforward;
	daddr_t offset, oldoffset, lastgoodpseg, sb_addr;
	int sn, curseg;

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
	if (VOP_IOCTL(devvp, DIOCGPART, &dpart, FREAD, cred, p) != 0)
		secsize = DEV_BSIZE;
	else
		secsize = dpart.disklab->d_secsize;

	/* Don't free random space on error. */
	bp = NULL;
	abp = NULL;
	ump = NULL;

	sb_addr = LFS_LABELPAD / secsize;
	while (1) {
		/* Read in the superblock. */
		error = bread(devvp, sb_addr, LFS_SBPAD, cred, &bp);
		if (error)
			goto out;
		dfs = (struct dlfs *)bp->b_data;

		/* Check the basics. */
		if (dfs->dlfs_magic != LFS_MAGIC || dfs->dlfs_bsize >= MAXBSIZE ||
		    dfs->dlfs_version > LFS_VERSION ||
		    dfs->dlfs_bsize < sizeof(struct dlfs)) {
#ifdef DEBUG_LFS
			printf("lfs_mountfs: primary superblock sanity failed\n");
#endif
			error = EINVAL;		/* XXX needs translation */
			goto out;
		}
		if (dfs->dlfs_inodefmt > LFS_MAXINODEFMT)
			printf("lfs_mountfs: warning: unknown inode format %d\n",
			       dfs->dlfs_inodefmt);
	
		if (dfs->dlfs_version == 1) 
			fsbsize = secsize;
		else {
			fsbsize = 1 << (dfs->dlfs_bshift - dfs->dlfs_blktodb + 
				dfs->dlfs_fsbtodb);
			/*
			 * Could be, if the frag size is large enough, that we
			 * don't have the "real" primary superblock.  If that's
			 * the case, get the real one, and try again.
			 */
			if (sb_addr != dfs->dlfs_sboffs[0] <<
				       dfs->dlfs_fsbtodb) {
/* #ifdef DEBUG_LFS */
				printf("lfs_mountfs: sb daddr 0x%llx is not right, trying 0x%llx\n",
					(long long)sb_addr, (long long)(dfs->dlfs_sboffs[0] <<
						 dfs->dlfs_fsbtodb));
/* #endif */
				sb_addr = dfs->dlfs_sboffs[0] << 
					  dfs->dlfs_fsbtodb;
				brelse(bp);
				continue;
			}
		}
		break;
	}

	/*
	 * Check the second superblock to see which is newer; then mount
	 * using the older of the two.	This is necessary to ensure that
	 * the filesystem is valid if it was not unmounted cleanly.
	 */

	if (dfs->dlfs_sboffs[1] &&
	    dfs->dlfs_sboffs[1] - LFS_LABELPAD / fsbsize > LFS_SBPAD / fsbsize)
	{
		error = bread(devvp, dfs->dlfs_sboffs[1] * (fsbsize / secsize), 
			LFS_SBPAD, cred, &abp);
		if (error)
			goto out;
		adfs = (struct dlfs *)abp->b_data;

		if (dfs->dlfs_version == 1) {
			/* 1s resolution comparison */
			if (adfs->dlfs_tstamp < dfs->dlfs_tstamp)
				tdfs = adfs;
			else
				tdfs = dfs;
		} else {
			/* monotonic infinite-resolution comparison */
			if (adfs->dlfs_serial < dfs->dlfs_serial)
				tdfs = adfs;
			else
				tdfs = dfs;
		}

		/* Check the basics. */
		if (tdfs->dlfs_magic != LFS_MAGIC ||
		    tdfs->dlfs_bsize > MAXBSIZE ||
		    tdfs->dlfs_version > LFS_VERSION ||
		    tdfs->dlfs_bsize < sizeof(struct dlfs)) {
#ifdef DEBUG_LFS
			printf("lfs_mountfs: alt superblock sanity failed\n");
#endif
			error = EINVAL;		/* XXX needs translation */
			goto out;
		}
	} else {
#ifdef DEBUG_LFS
		printf("lfs_mountfs: invalid alt superblock daddr=0x%x\n",
			dfs->dlfs_sboffs[1]);
#endif
		error = EINVAL;
		goto out;
	}

	/* Allocate the mount structure, copy the superblock into it. */
	fs = malloc(sizeof(struct lfs), M_UFSMNT, M_WAITOK | M_ZERO);
	memcpy(&fs->lfs_dlfs, tdfs, sizeof(struct dlfs));

	/* Compatibility */
	if (fs->lfs_version < 2) {
		fs->lfs_sumsize = LFS_V1_SUMMARY_SIZE;
		fs->lfs_ibsize = fs->lfs_bsize;
		fs->lfs_start = fs->lfs_sboffs[0];
		fs->lfs_tstamp = fs->lfs_otstamp;
		fs->lfs_fsbtodb = 0;
	}

	/* Before rolling forward, lock so vget will sleep for other procs */
	fs->lfs_flags = LFS_NOTYET;
	fs->lfs_rfpid = p->p_pid;

	ump = malloc(sizeof *ump, M_UFSMNT, M_WAITOK | M_ZERO);
	ump->um_lfs = fs;
	ump->um_fstype = UFS1;
	if (sizeof(struct lfs) < LFS_SBPAD) {			/* XXX why? */
		bp->b_flags |= B_INVAL;
		abp->b_flags |= B_INVAL;
	}
	brelse(bp);
	bp = NULL;
	brelse(abp);
	abp = NULL;

	/* Set up the I/O information */
	fs->lfs_devbsize = secsize;
	fs->lfs_iocount = 0;
	fs->lfs_diropwait = 0;
	fs->lfs_activesb = 0;
	fs->lfs_uinodes = 0;
	fs->lfs_ravail = 0;
	fs->lfs_sbactive = 0;

	/* Set up the ifile and lock aflags */
	fs->lfs_doifile = 0;
	fs->lfs_writer = 0;
	fs->lfs_dirops = 0;
	fs->lfs_nadirop = 0;
	fs->lfs_seglock = 0;
	fs->lfs_pdflush = 0;
	fs->lfs_sleepers = 0;
	simple_lock_init(&fs->lfs_interlock);
	lockinit(&fs->lfs_fraglock, PINOD, "lfs_fraglock", 0, 0);

	/* Set the file system readonly/modify bits. */
	fs->lfs_ronly = ronly;
	if (ronly == 0)
		fs->lfs_fmod = 1;

	/* Initialize the mount structure. */
	dev = devvp->v_rdev;
	mp->mnt_data = ump;
	mp->mnt_stat.f_fsidx.__fsid_val[0] = (long)dev;
	mp->mnt_stat.f_fsidx.__fsid_val[1] = makefstype(MOUNT_LFS);
	mp->mnt_stat.f_fsid = mp->mnt_stat.f_fsidx.__fsid_val[0];
	mp->mnt_stat.f_namemax = MAXNAMLEN;
	mp->mnt_stat.f_iosize = fs->lfs_bsize;
	mp->mnt_maxsymlinklen = fs->lfs_maxsymlinklen;
	mp->mnt_flag |= MNT_LOCAL;
	mp->mnt_fs_bshift = fs->lfs_bshift;
	ump->um_flags = 0;
	ump->um_mountp = mp;
	ump->um_dev = dev;
	ump->um_devvp = devvp;
	ump->um_bptrtodb = fs->lfs_fsbtodb;
	ump->um_seqinc = fragstofsb(fs, fs->lfs_frag);
	ump->um_nindir = fs->lfs_nindir;
	ump->um_lognindir = ffs(fs->lfs_nindir) - 1;
	for (i = 0; i < MAXQUOTAS; i++)
		ump->um_quotas[i] = NULLVP;
	devvp->v_specmountpoint = mp;

	/* Set up reserved memory for pageout */
	lfs_setup_resblks(fs);
	/* Set up vdirop tailq */
	TAILQ_INIT(&fs->lfs_dchainhd);
	/* and paging tailq */
	TAILQ_INIT(&fs->lfs_pchainhd);

	/*
	 * We use the ifile vnode for almost every operation.  Instead of
	 * retrieving it from the hash table each time we retrieve it here,
	 * artificially increment the reference count and keep a pointer
	 * to it in the incore copy of the superblock.
	 */
	if ((error = VFS_VGET(mp, LFS_IFILE_INUM, &vp)) != 0) {
#ifdef DEBUG
		printf("lfs_mountfs: ifile vget failed, error=%d\n", error);
#endif
		goto out;
	}
	fs->lfs_ivnode = vp;
	VREF(vp);

	/* Set up segment usage flags for the autocleaner. */
	fs->lfs_nactive = 0;
	fs->lfs_suflags = (u_int32_t **)malloc(2 * sizeof(u_int32_t *),
						M_SEGMENT, M_WAITOK);
	fs->lfs_suflags[0] = (u_int32_t *)malloc(fs->lfs_nseg * sizeof(u_int32_t),
						 M_SEGMENT, M_WAITOK);
	fs->lfs_suflags[1] = (u_int32_t *)malloc(fs->lfs_nseg * sizeof(u_int32_t),
						 M_SEGMENT, M_WAITOK);
	memset(fs->lfs_suflags[1], 0, fs->lfs_nseg * sizeof(u_int32_t));
	for (i = 0; i < fs->lfs_nseg; i++) {
		int changed;

		LFS_SEGENTRY(sup, fs, i, bp);
		changed = 0;
		if (!ronly) {
			if (sup->su_nbytes == 0 &&
			    !(sup->su_flags & SEGUSE_EMPTY)) {
				sup->su_flags |= SEGUSE_EMPTY;
				++changed;
			} else if (!(sup->su_nbytes == 0) &&
				   (sup->su_flags & SEGUSE_EMPTY)) {
				sup->su_flags &= ~SEGUSE_EMPTY;
				++changed;
			}
			if (sup->su_flags & SEGUSE_ACTIVE) {
				sup->su_flags &= ~SEGUSE_ACTIVE;
				++changed;
			}
		}
		fs->lfs_suflags[0][i] = sup->su_flags;
		if (changed)
			LFS_WRITESEGENTRY(sup, fs, i, bp);
		else
			brelse(bp);
	}

	/*
	 * Roll forward.
	 *
	 * We don't automatically roll forward for v1 filesystems, because
	 * of the danger that the clock was turned back between the last
	 * checkpoint and crash.  This would roll forward garbage.
	 *
	 * v2 filesystems don't have this problem because they use a
	 * monotonically increasing serial number instead of a timestamp.
	 */
#ifdef LFS_DO_ROLLFORWARD
	do_rollforward = !fs->lfs_ronly;
#else
	do_rollforward = (fs->lfs_version > 1 && !fs->lfs_ronly &&
			  !(fs->lfs_pflags & LFS_PF_CLEAN));
#endif
	if (do_rollforward) {
		u_int64_t nextserial;
		/*
		 * Phase I: Find the address of the last good partial
		 * segment that was written after the checkpoint.  Mark
		 * the segments in question dirty, so they won't be
		 * reallocated.
		 */
		lastgoodpseg = oldoffset = offset = fs->lfs_offset;
		flags = 0x0;
#ifdef DEBUG_LFS_RFW
		printf("LFS roll forward phase 1: starting at offset 0x%"
		    PRIx64 "\n", offset);
#endif
		LFS_SEGENTRY(sup, fs, dtosn(fs, offset), bp);
		if (!(sup->su_flags & SEGUSE_DIRTY))
			--fs->lfs_nclean;
		sup->su_flags |= SEGUSE_DIRTY;
		LFS_WRITESEGENTRY(sup, fs, dtosn(fs, offset), bp);
		nextserial = fs->lfs_serial + 1;
		while ((offset = check_segsum(fs, offset, nextserial,
		    cred, CHECK_CKSUM, &flags, p)) > 0) {
			nextserial++;
			if (sntod(fs, oldoffset) != sntod(fs, offset)) {
				LFS_SEGENTRY(sup, fs, dtosn(fs, oldoffset),
					     bp); 
				if (!(sup->su_flags & SEGUSE_DIRTY))
					--fs->lfs_nclean;
				sup->su_flags |= SEGUSE_DIRTY;
				LFS_WRITESEGENTRY(sup, fs, dtosn(fs, oldoffset),
					     bp); 
			}

#ifdef DEBUG_LFS_RFW
			printf("LFS roll forward phase 1: offset=0x%"
			    PRIx64 "\n", offset);
			if (flags & SS_DIROP) {
				printf("lfs_mountfs: dirops at 0x%" PRIx64 "\n",
				       oldoffset);
				if (!(flags & SS_CONT))
					printf("lfs_mountfs: dirops end "
					       "at 0x%" PRIx64 "\n", oldoffset);
			}
#endif
			if (!(flags & SS_CONT))
				lastgoodpseg = offset;
			oldoffset = offset;
		}
#ifdef DEBUG_LFS_RFW
		if (flags & SS_CONT) {
			printf("LFS roll forward: warning: incomplete "
			       "dirops discarded\n");
		}
		printf("LFS roll forward phase 1: completed: "
		       "lastgoodpseg=0x%" PRIx64 "\n", lastgoodpseg);
#endif
		oldoffset = fs->lfs_offset;
		if (fs->lfs_offset != lastgoodpseg) {
			/* Don't overwrite what we're trying to preserve */
			offset = fs->lfs_offset;
			fs->lfs_offset = lastgoodpseg;
			fs->lfs_curseg = sntod(fs, dtosn(fs, fs->lfs_offset));
			for (sn = curseg = dtosn(fs, fs->lfs_curseg);;) {
				sn = (sn + 1) % fs->lfs_nseg;
				if (sn == curseg)
					panic("lfs_mountfs: no clean segments");
				LFS_SEGENTRY(sup, fs, sn, bp);
				dirty = (sup->su_flags & SEGUSE_DIRTY);
				brelse(bp);
				if (!dirty)
					break;
			}
			fs->lfs_nextseg = sntod(fs, sn);

			/*
			 * Phase II: Roll forward from the first superblock.
			 */
			while (offset != lastgoodpseg) {
#ifdef DEBUG_LFS_RFW
				printf("LFS roll forward phase 2: 0x%"
				    PRIx64 "\n", offset);
#endif
				offset = check_segsum(fs, offset,
				    fs->lfs_serial + 1, cred, CHECK_UPDATE,
				    NULL, p);
			}

			/*
			 * Finish: flush our changes to disk.
			 */
			lfs_segwrite(mp, SEGM_CKP | SEGM_SYNC);
			printf("lfs_mountfs: roll forward recovered %lld blocks\n",
			       (long long)(lastgoodpseg - oldoffset));
		}
#ifdef DEBUG_LFS_RFW
		printf("LFS roll forward complete\n");
#endif
	}
	/* If writing, sb is not clean; record in case of immediate crash */
	if (!fs->lfs_ronly) {
		fs->lfs_pflags &= ~LFS_PF_CLEAN;
		lfs_writesuper(fs, fs->lfs_sboffs[0]);
		lfs_writesuper(fs, fs->lfs_sboffs[1]);
	}
	
	/* Allow vget now that roll-forward is complete */
	fs->lfs_flags &= ~(LFS_NOTYET);
	wakeup(&fs->lfs_flags);

	/*
	 * Initialize the ifile cleaner info with information from 
	 * the superblock.
	 */ 
	LFS_CLEANERINFO(cip, fs, bp);
	cip->clean = fs->lfs_nclean;
	cip->dirty = fs->lfs_nseg - fs->lfs_nclean;
	cip->avail = fs->lfs_avail;
	cip->bfree = fs->lfs_bfree;
	(void) LFS_BWRITE_LOG(bp); /* Ifile */

	/*
	 * Mark the current segment as ACTIVE, since we're going to 
	 * be writing to it.
	 */
	LFS_SEGENTRY(sup, fs, dtosn(fs, fs->lfs_offset), bp); 
	sup->su_flags |= SEGUSE_DIRTY | SEGUSE_ACTIVE;
	fs->lfs_nactive++;
	LFS_WRITESEGENTRY(sup, fs, dtosn(fs, fs->lfs_offset), bp);  /* Ifile */

	/* Now that roll-forward is done, unlock the Ifile */
	vput(vp);

	/* Comment on ifile size if it is too large */
	if (fs->lfs_ivnode->v_size / fs->lfs_bsize > LFS_MAX_BUFS) {
		fs->lfs_flags |= LFS_WARNED;
		printf("lfs_mountfs: please consider increasing NBUF to at least %lld\n",
			(long long)(fs->lfs_ivnode->v_size / fs->lfs_bsize) * (nbuf / LFS_MAX_BUFS));
	}
	if (fs->lfs_ivnode->v_size > LFS_MAX_BYTES) {
		fs->lfs_flags |= LFS_WARNED;
		printf("lfs_mountfs: please consider increasing BUFPAGES to at least %lld\n",
			(long long)(fs->lfs_ivnode->v_size * bufpages / LFS_MAX_BYTES));
	}

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
		mp->mnt_data = NULL;
	}

	/* Start the pagedaemon-anticipating daemon */
	if (lfs_writer_daemon == 0 &&
	    kthread_create1(lfs_writerd, NULL, NULL, "lfs_writer") != 0)
		panic("fork lfs_writer");

	return (error);
}

/*
 * unmount system call
 */
int
lfs_unmount(struct mount *mp, int mntflags, struct proc *p)
{
	struct ufsmount *ump;
	struct lfs *fs;
	int error, flags, ronly;
	int s;

	flags = 0;
	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;

	ump = VFSTOUFS(mp);
	fs = ump->um_lfs;

	/* wake up the cleaner so it can die */
	wakeup(&fs->lfs_nextseg);
	wakeup(&lfs_allclean_wakeup);
	simple_lock(&fs->lfs_interlock);
	while (fs->lfs_sleepers)
		ltsleep(&fs->lfs_sleepers, PRIBIO + 1, "lfs_sleepers", 0,
			&fs->lfs_interlock);
	simple_unlock(&fs->lfs_interlock);

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
	if ((error = VFS_SYNC(mp, 1, p->p_ucred, p)) != 0)
		return (error);
	s = splbio();
	if (LIST_FIRST(&fs->lfs_ivnode->v_dirtyblkhd))
		panic("lfs_unmount: still dirty blocks on ifile vnode");
	splx(s);

	/* Comment on ifile size if it has become too large */
	if (!(fs->lfs_flags & LFS_WARNED)) {
		if (fs->lfs_ivnode->v_size / fs->lfs_bsize > LFS_MAX_BUFS)
			printf("lfs_unmount: please consider increasing"
				" NBUF to at least %lld\n",
				(long long)(fs->lfs_ivnode->v_size /
					    fs->lfs_bsize) *
				(long long)(nbuf / LFS_MAX_BUFS));
		if (fs->lfs_ivnode->v_size > LFS_MAX_BYTES)
			printf("lfs_unmount: please consider increasing"
				" BUFPAGES to at least %lld\n",
				(long long)(fs->lfs_ivnode->v_size *
				bufpages / LFS_MAX_BYTES));
	}

	/* Explicitly write the superblock, to update serial and pflags */
	fs->lfs_pflags |= LFS_PF_CLEAN;
	lfs_writesuper(fs, fs->lfs_sboffs[0]);
	lfs_writesuper(fs, fs->lfs_sboffs[1]);
	while (fs->lfs_iocount)
		tsleep(&fs->lfs_iocount, PRIBIO + 1, "lfs_umount", 0);

	/* Finish with the Ifile, now that we're done with it */
	vrele(fs->lfs_ivnode);
	vgone(fs->lfs_ivnode);

	ronly = !fs->lfs_ronly;
	if (ump->um_devvp->v_type != VBAD)
		ump->um_devvp->v_specmountpoint = NULL;
	vn_lock(ump->um_devvp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_CLOSE(ump->um_devvp,
	    ronly ? FREAD : FREAD|FWRITE, NOCRED, p);
	vput(ump->um_devvp);

	/* Free per-mount data structures */
	free(fs->lfs_suflags[0], M_SEGMENT);
	free(fs->lfs_suflags[1], M_SEGMENT);
	free(fs->lfs_suflags, M_SEGMENT);
	lfs_free_resblks(fs);
	free(fs, M_UFSMNT);
	free(ump, M_UFSMNT);

	mp->mnt_data = NULL;
	mp->mnt_flag &= ~MNT_LOCAL;
	return (error);
}

/*
 * Get file system statistics.
 */
int
lfs_statvfs(struct mount *mp, struct statvfs *sbp, struct proc *p)
{
	struct lfs *fs;
	struct ufsmount *ump;

	ump = VFSTOUFS(mp);
	fs = ump->um_lfs;
	if (fs->lfs_magic != LFS_MAGIC)
		panic("lfs_statvfs: magic");

	sbp->f_bsize = fs->lfs_bsize;
	sbp->f_frsize = fs->lfs_fsize;
	sbp->f_iosize = fs->lfs_bsize;
	sbp->f_blocks = fsbtofrags(fs, LFS_EST_NONMETA(fs));
	sbp->f_bfree = fsbtofrags(fs, LFS_EST_BFREE(fs));
	sbp->f_bresvd = fsbtofrags(fs, LFS_EST_RSVD(fs));
	if (sbp->f_bfree > sbp->f_bresvd)
		sbp->f_bavail = sbp->f_bfree - sbp->f_bresvd;
	else
		sbp->f_bavail = 0;
	
	sbp->f_files = fs->lfs_bfree / btofsb(fs, fs->lfs_ibsize) * INOPB(fs);
	sbp->f_ffree = sbp->f_files - fs->lfs_nfiles;
	sbp->f_favail = sbp->f_ffree;
	sbp->f_fresvd = 0;
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
lfs_sync(struct mount *mp, int waitfor, struct ucred *cred, struct proc *p)
{
	int error;
	struct lfs *fs;

	fs = VFSTOUFS(mp)->um_lfs;
	if (fs->lfs_ronly)
		return 0;
	lfs_writer_enter(fs, "lfs_dirops");

	/* All syncs must be checkpoints until roll-forward is implemented. */
	error = lfs_segwrite(mp, SEGM_CKP | (waitfor ? SEGM_SYNC : 0));
	lfs_writer_leave(fs);
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
lfs_vget(struct mount *mp, ino_t ino, struct vnode **vpp)
{
	struct lfs *fs;
	struct ufs1_dinode *dip;
	struct inode *ip;
	struct buf *bp;
	struct ifile *ifp;
	struct vnode *vp;
	struct ufsmount *ump;
	daddr_t daddr;
	dev_t dev;
	int error, retries;
	struct timespec ts;

	ump = VFSTOUFS(mp);
	dev = ump->um_dev;
	fs = ump->um_lfs;

	/*
	 * If the filesystem is not completely mounted yet, suspend
	 * any access requests (wait for roll-forward to complete).
	 */
	while ((fs->lfs_flags & LFS_NOTYET) && curproc->p_pid != fs->lfs_rfpid)
		tsleep(&fs->lfs_flags, PRIBIO+1, "lfs_notyet", 0);

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
	if (ino == LFS_IFILE_INUM)
		daddr = fs->lfs_idaddr;
	else {
		/* XXX bounds-check this too */
		LFS_IENTRY(ifp, fs, ino, bp);
		daddr = ifp->if_daddr;
		if (fs->lfs_version > 1) {
			ts.tv_sec = ifp->if_atime_sec;
			ts.tv_nsec = ifp->if_atime_nsec;
		}

		brelse(bp);
		if (daddr == LFS_UNUSED_DADDR) {
			*vpp = NULLVP;
			ungetnewvnode(vp);
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
	retries = 0;
    again:
	error = bread(ump->um_devvp, fsbtodb(fs, daddr), 
		(fs->lfs_version == 1 ? fs->lfs_bsize : fs->lfs_ibsize),
		NOCRED, &bp);
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

	dip = lfs_ifind(fs, ino, bp);
	if (dip == NULL) {
		/* Assume write has not completed yet; try again */
		bp->b_flags |= B_INVAL;
		brelse(bp);
		++retries;
		if (retries > LFS_IFIND_RETRIES) {
#ifdef DEBUG
			/* If the seglock is held look at the bpp to see
			   what is there anyway */
			if (fs->lfs_seglock > 0) {
				struct buf **bpp;
				struct ufs1_dinode *dp;
				int i;

				for (bpp = fs->lfs_sp->bpp;
				     bpp != fs->lfs_sp->cbpp; ++bpp) {
					if ((*bpp)->b_vp == fs->lfs_ivnode &&
					    bpp != fs->lfs_sp->bpp) {
						/* Inode block */
						printf("block 0x%" PRIx64 ": ",
						    (*bpp)->b_blkno);
						dp = (struct ufs1_dinode *)(*bpp)->b_data;
						for (i = 0; i < INOPB(fs); i++)
							if (dp[i].di_u.inumber)
								printf("%d ", dp[i].di_u.inumber);
						printf("\n");
					}
				}
			}
#endif
			panic("lfs_vget: dinode not found");
		}
		printf("lfs_vget: dinode %d not found, retrying...\n", ino);
		(void)tsleep(&fs->lfs_iocount, PRIBIO + 1, "lfs ifind", 1);
		goto again;
	}
	*ip->i_din.ffs1_din = *dip;
	brelse(bp);

	if (fs->lfs_version > 1) {
		ip->i_ffs1_atime = ts.tv_sec;
		ip->i_ffs1_atimensec = ts.tv_nsec;
	}

	lfs_vinit(mp, &vp);

	*vpp = vp;

	KASSERT(VOP_ISLOCKED(vp));

	return (0);
}

/*
 * File handle to vnode
 */
int
lfs_fhtovp(struct mount *mp, struct fid *fhp, struct vnode **vpp)
{
	struct lfid *lfhp;
	struct buf *bp;
	IFILE *ifp;
	int32_t daddr;
	struct lfs *fs;

	lfhp = (struct lfid *)fhp;
	if (lfhp->lfid_ino < LFS_IFILE_INUM)
		return ESTALE;

	fs = VFSTOUFS(mp)->um_lfs;
	if (lfhp->lfid_ident != fs->lfs_ident)
		return ESTALE;

	if (lfhp->lfid_ino >
	    ((VTOI(fs->lfs_ivnode)->i_ffs1_size >> fs->lfs_bshift) -
	     fs->lfs_cleansz - fs->lfs_segtabsz) * fs->lfs_ifpb)
		return ESTALE;

	if (ufs_ihashlookup(VFSTOUFS(mp)->um_dev, lfhp->lfid_ino) == NULLVP) {
		LFS_IENTRY(ifp, fs, lfhp->lfid_ino, bp);
		daddr = ifp->if_daddr;
		brelse(bp);
		if (daddr == LFS_UNUSED_DADDR)
			return ESTALE;
	}

	return (ufs_fhtovp(mp, &lfhp->lfid_ufid, vpp));
}

/*
 * Vnode pointer to File handle
 */
/* ARGSUSED */
int
lfs_vptofh(struct vnode *vp, struct fid *fhp)
{
	struct inode *ip;
	struct lfid *lfhp;

	ip = VTOI(vp);
	lfhp = (struct lfid *)fhp;
	lfhp->lfid_len = sizeof(struct lfid);
	lfhp->lfid_ino = ip->i_number;
	lfhp->lfid_gen = ip->i_gen;
	lfhp->lfid_ident = ip->i_lfs->lfs_ident;
	return (0);
}

static int
sysctl_lfs_dostats(SYSCTLFN_ARGS)
{
	extern struct lfs_stats lfs_stats;
	extern int lfs_dostats;
	int error;

	error = sysctl_lookup(SYSCTLFN_CALL(rnode));
	if (error || newp == NULL)
		return (error);

	if (lfs_dostats == 0)
		memset(&lfs_stats,0,sizeof(lfs_stats));

	return (0);
}

SYSCTL_SETUP(sysctl_vfs_lfs_setup, "sysctl vfs.lfs setup")
{
	extern int lfs_writeindir, lfs_dostats, lfs_clean_vnhead;

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "vfs", NULL,
		       NULL, 0, NULL, 0,
		       CTL_VFS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "lfs", NULL,
		       NULL, 0, NULL, 0,
		       CTL_VFS, 5, CTL_EOL);
	/*
	 * XXX the "5" above could be dynamic, thereby eliminating one
	 * more instance of the "number to vfs" mapping problem, but
	 * "2" is the order as taken from sys/mount.h
	 */

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "flushindir", NULL,
		       NULL, 0, &lfs_writeindir, 0,
		       CTL_VFS, 5, LFS_WRITEINDIR, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "clean_vnhead", NULL,
		       NULL, 0, &lfs_clean_vnhead, 0,
		       CTL_VFS, 5, LFS_CLEAN_VNHEAD, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "dostats", NULL,
		       sysctl_lfs_dostats, 0, &lfs_dostats, 0,
		       CTL_VFS, 5, LFS_DOSTATS, CTL_EOL);
}

/*
 * ufs_bmaparray callback function for writing.
 *
 * Since blocks will be written to the new segment anyway,
 * we don't care about current daddr of them.
 */
static boolean_t
lfs_issequential_hole(const struct ufsmount *ump,
    daddr_t daddr0, daddr_t daddr1)
{

	KASSERT(daddr0 == UNWRITTEN ||
	    (0 <= daddr0 && daddr0 <= LFS_MAX_DADDR));
	KASSERT(daddr1 == UNWRITTEN ||
	    (0 <= daddr1 && daddr1 <= LFS_MAX_DADDR));

	/* NOTE: all we want to know here is 'hole or not'. */
	/* NOTE: UNASSIGNED is converted to 0 by ufs_bmaparray. */

	/*
	 * treat UNWRITTENs and all resident blocks as 'contiguous'
	 */
	if (daddr0 != 0 && daddr1 != 0)
		return TRUE;

	/*
	 * both are in hole?
	 */
	if (daddr0 == 0 && daddr1 == 0)
		return TRUE; /* all holes are 'contiguous' for us. */

	return FALSE;
}

/*
 * lfs_gop_write functions exactly like genfs_gop_write, except that
 * (1) it requires the seglock to be held by its caller, and sp->fip
 *     to be properly initialized (it will return without re-initializing
 *     sp->fip, and without calling lfs_writeseg).
 * (2) it uses the remaining space in the segment, rather than VOP_BMAP,
 *     to determine how large a block it can write at once (though it does
 *     still use VOP_BMAP to find holes in the file);
 * (3) it calls lfs_gatherblock instead of VOP_STRATEGY on its blocks
 *     (leaving lfs_writeseg to deal with the cluster blocks, so we might
 *     now have clusters of clusters, ick.)
 */
static int
lfs_gop_write(struct vnode *vp, struct vm_page **pgs, int npages, int flags)
{
	int i, s, error, run;
	int fs_bshift;
	vaddr_t kva;
	off_t eof, offset, startoffset;
	size_t bytes, iobytes, skipbytes;
	daddr_t lbn, blkno;
	struct vm_page *pg;
	struct buf *mbp, *bp;
	struct vnode *devvp = VTOI(vp)->i_devvp;
	struct inode *ip = VTOI(vp);
	struct lfs *fs = ip->i_lfs;
	struct segment *sp = fs->lfs_sp;
	UVMHIST_FUNC("lfs_gop_write"); UVMHIST_CALLED(ubchist);

	/* The Ifile lives in the buffer cache */
	if (vp == fs->lfs_ivnode)
		return genfs_compat_gop_write(vp, pgs, npages, flags);

	/*
	 * Sometimes things slip past the filters in lfs_putpages,
	 * and the pagedaemon tries to write pages---problem is
	 * that the pagedaemon never acquires the segment lock.
	 *
	 * Unbusy and unclean the pages, and put them on the ACTIVE
	 * queue under the hypothesis that they couldn't have got here
	 * unless they were modified *quite* recently.
	 *
	 * XXXUBC that last statement is an oversimplification of course.
	 */
	if (!(fs->lfs_seglock) || fs->lfs_lockpid != curproc->p_pid) {
		simple_lock(&vp->v_interlock);
#ifdef DEBUG
		printf("lfs_gop_write: seglock not held\n");
#endif
		uvm_lock_pageq();
		for (i = 0; i < npages; i++) {
			pg = pgs[i];

			if (pg->flags & PG_PAGEOUT)
				uvmexp.paging--;
			if (pg->flags & PG_DELWRI) {
				uvm_pageunwire(pg);
			}
			uvm_pageactivate(pg);
			pg->flags &= ~(PG_CLEAN|PG_DELWRI|PG_PAGEOUT|PG_RELEASED);
#ifdef DEBUG_LFS
			printf("pg[%d]->flags = %x\n", i, pg->flags);
			printf("pg[%d]->pqflags = %x\n", i, pg->pqflags);
			printf("pg[%d]->uanon = %p\n", i, pg->uanon);
			printf("pg[%d]->uobject = %p\n", i, pg->uobject);
			printf("pg[%d]->wire_count = %d\n", i, pg->wire_count);
			printf("pg[%d]->loan_count = %d\n", i, pg->loan_count);
#endif
		}
		/* uvm_pageunbusy takes care of PG_BUSY, PG_WANTED */
		uvm_page_unbusy(pgs, npages);
		uvm_unlock_pageq();
		simple_unlock(&vp->v_interlock);
		return EAGAIN;
	}

	UVMHIST_LOG(ubchist, "vp %p pgs %p npages %d flags 0x%x",
	    vp, pgs, npages, flags);

	GOP_SIZE(vp, vp->v_size, &eof, GOP_SIZE_WRITE);

	if (vp->v_type == VREG)
		fs_bshift = vp->v_mount->mnt_fs_bshift;
	else
		fs_bshift = DEV_BSHIFT;
	error = 0;
	pg = pgs[0];
	startoffset = pg->offset;
	bytes = MIN(npages << PAGE_SHIFT, eof - startoffset);
	skipbytes = 0;

	/* KASSERT(bytes != 0); */
	if (bytes == 0)
		printf("ino %d bytes == 0 offset %" PRId64 "\n",
			VTOI(vp)->i_number, pgs[0]->offset);

	/* Swap PG_DELWRI for PG_PAGEOUT */
	for (i = 0; i < npages; i++)
		if (pgs[i]->flags & PG_DELWRI) {
			KASSERT(!(pgs[i]->flags & PG_PAGEOUT));
			pgs[i]->flags &= ~PG_DELWRI;
			pgs[i]->flags |= PG_PAGEOUT;
			uvmexp.paging++;
			uvm_lock_pageq();
			uvm_pageunwire(pgs[i]);
			uvm_unlock_pageq();
		}

	/*
	 * Check to make sure we're starting on a block boundary.
	 * We'll check later to make sure we always write entire
	 * blocks (or fragments).
	 */
	if (startoffset & fs->lfs_bmask)
		printf("%" PRId64 " & %" PRId64 " = %" PRId64 "\n",
			startoffset, fs->lfs_bmask,
			startoffset & fs->lfs_bmask);
	KASSERT((startoffset & fs->lfs_bmask) == 0);
	if (bytes & fs->lfs_ffmask) {
		printf("lfs_gop_write: asked to write %ld bytes\n", (long)bytes);
		panic("lfs_gop_write: non-integer blocks");
	}

	kva = uvm_pagermapin(pgs, npages,
	    UVMPAGER_MAPIN_WRITE | UVMPAGER_MAPIN_WAITOK);

	s = splbio();
	simple_lock(&global_v_numoutput_slock);
	vp->v_numoutput += 2; /* one for biodone, one for aiodone */
	simple_unlock(&global_v_numoutput_slock);
	mbp = pool_get(&bufpool, PR_WAITOK);
	splx(s);

	memset(mbp, 0, sizeof(*bp));
	BUF_INIT(mbp);
	UVMHIST_LOG(ubchist, "vp %p mbp %p num now %d bytes 0x%x",
	    vp, mbp, vp->v_numoutput, bytes);
	mbp->b_bufsize = npages << PAGE_SHIFT;
	mbp->b_data = (void *)kva;
	mbp->b_resid = mbp->b_bcount = bytes;
	mbp->b_flags = B_BUSY|B_WRITE|B_AGE|B_CALL;
	mbp->b_iodone = uvm_aio_biodone;
	mbp->b_vp = vp;

	bp = NULL;
	for (offset = startoffset;
	    bytes > 0;
	    offset += iobytes, bytes -= iobytes) {
		lbn = offset >> fs_bshift;
		error = ufs_bmaparray(vp, lbn, &blkno, NULL, NULL, &run,
		    lfs_issequential_hole);
		if (error) {
			UVMHIST_LOG(ubchist, "ufs_bmaparray() -> %d",
			    error,0,0,0);
			skipbytes += bytes;
			bytes = 0;
			break;
		}

		iobytes = MIN((((off_t)lbn + 1 + run) << fs_bshift) - offset,
		    bytes);
		if (blkno == (daddr_t)-1) {
			skipbytes += iobytes;
			continue;
		}

		/*
		 * Discover how much we can really pack into this buffer.
		 */
		/* If no room in the current segment, finish it up */
		if (sp->sum_bytes_left < sizeof(int32_t) ||
		    sp->seg_bytes_left < (1 << fs->lfs_bshift)) {
			int version;

			lfs_updatemeta(sp);

			version = sp->fip->fi_version;
			(void) lfs_writeseg(fs, sp);
			
			sp->fip->fi_version = version;
			sp->fip->fi_ino = ip->i_number;
			/* Add the current file to the segment summary. */
			++((SEGSUM *)(sp->segsum))->ss_nfinfo;
			sp->sum_bytes_left -= FINFOSIZE;
		}
		/* Check both for space in segment and space in segsum */
		iobytes = MIN(iobytes, (sp->seg_bytes_left >> fs_bshift)
					<< fs_bshift);
		iobytes = MIN(iobytes, (sp->sum_bytes_left / sizeof(int32_t))
				       << fs_bshift);
		KASSERT(iobytes > 0);

		/* if it's really one i/o, don't make a second buf */
		if (offset == startoffset && iobytes == bytes) {
			bp = mbp;
			/* printf("bp is mbp\n"); */
			/* correct overcount if there is no second buffer */
			s = splbio();
			simple_lock(&global_v_numoutput_slock);
			--vp->v_numoutput;
			simple_unlock(&global_v_numoutput_slock);
			splx(s);
		} else {
			/* printf("bp is not mbp\n"); */
			s = splbio();
			bp = pool_get(&bufpool, PR_WAITOK);
			UVMHIST_LOG(ubchist, "vp %p bp %p num now %d",
			    vp, bp, vp->v_numoutput, 0);
			splx(s);
			memset(bp, 0, sizeof(*bp));
			BUF_INIT(bp);
			bp->b_data = (char *)kva +
			    (vaddr_t)(offset - pg->offset);
			bp->b_resid = bp->b_bcount = iobytes;
			bp->b_flags = B_BUSY|B_WRITE|B_CALL;
			bp->b_iodone = uvm_aio_biodone1;
		}

		/* XXX This is silly ... is this necessary? */
		bp->b_vp = NULL;
		s = splbio();
		bgetvp(vp, bp);
		splx(s);

		bp->b_lblkno = lblkno(fs, offset);
		bp->b_private = mbp;
		if (devvp->v_type == VBLK) {
			bp->b_dev = devvp->v_rdev;
		}
		VOP_BWRITE(bp);
		while (lfs_gatherblock(sp, bp, NULL))
			continue;
	}

	if (skipbytes) {
		UVMHIST_LOG(ubchist, "skipbytes %d", skipbytes, 0,0,0);
		s = splbio();
		if (error) {
			mbp->b_flags |= B_ERROR;
			mbp->b_error = error;
		}
		mbp->b_resid -= skipbytes;
		if (mbp->b_resid == 0) {
			biodone(mbp);
		}
		splx(s);
	}
	UVMHIST_LOG(ubchist, "returning 0", 0,0,0,0);
	return (0);
}

/*
 * finish vnode/inode initialization.
 * used by lfs_vget and lfs_fastvget.
 */
void
lfs_vinit(struct mount *mp, struct vnode **vpp)
{
	struct vnode *vp = *vpp;
	struct inode *ip = VTOI(vp);
	struct ufsmount *ump = VFSTOUFS(mp);
	int i;

	ip->i_mode = ip->i_ffs1_mode;
	ip->i_ffs_effnlink = ip->i_nlink = ip->i_ffs1_nlink;
	ip->i_lfs_osize = ip->i_size = ip->i_ffs1_size;
	ip->i_flags = ip->i_ffs1_flags;
	ip->i_gen = ip->i_ffs1_gen;
	ip->i_uid = ip->i_ffs1_uid;
	ip->i_gid = ip->i_ffs1_gid;

	ip->i_lfs_effnblks = ip->i_ffs1_blocks;

	/*
	 * Initialize the vnode from the inode, check for aliases.  In all
	 * cases re-init ip, the underlying vnode/inode may have changed.
	 */
	ufs_vinit(mp, lfs_specop_p, lfs_fifoop_p, &vp);

	memset(ip->i_lfs_fragsize, 0, NDADDR * sizeof(*ip->i_lfs_fragsize));
	if (vp->v_type != VLNK ||
	    VTOI(vp)->i_size >= vp->v_mount->mnt_maxsymlinklen) {
		struct lfs *fs = ump->um_lfs;
#ifdef DEBUG
		for (i = (ip->i_size + fs->lfs_bsize - 1) >> fs->lfs_bshift;
		    i < NDADDR; i++) {
			if (ip->i_ffs1_db[i] != 0) {
inconsistent:
				lfs_dump_dinode(ip->i_din.ffs1_din);
				panic("inconsistent inode");
			}
		}
		for ( ; i < NDADDR + NIADDR; i++) {
			if (ip->i_ffs1_ib[i - NDADDR] != 0) {
				goto inconsistent;
			}
		}
#endif /* DEBUG */
		for (i = 0; i < NDADDR; i++)
			if (ip->i_ffs1_db[i] != 0)
				ip->i_lfs_fragsize[i] = blksize(fs, ip, i);
	}

#ifdef DEBUG
	if (vp->v_type == VNON) {
		printf("lfs_vinit: ino %d is type VNON! (ifmt=%o)\n",
		       ip->i_number, (ip->i_mode & IFMT) >> 12);
		lfs_dump_dinode(ip->i_din.ffs1_din);
#ifdef DDB
		Debugger();
#endif /* DDB */
	}
#endif /* DEBUG */

	/*
	 * Finish inode initialization now that aliasing has been resolved.
	 */

	ip->i_devvp = ump->um_devvp;
	VREF(ip->i_devvp);
	genfs_node_init(vp, &lfs_genfsops);
	uvm_vnp_setsize(vp, ip->i_size);

	*vpp = vp;
}
