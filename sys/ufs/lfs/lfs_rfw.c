/*	$NetBSD: lfs_rfw.c,v 1.39 2025/10/14 00:13:31 perseant Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: lfs_rfw.c,v 1.39 2025/10/14 00:13:31 perseant Exp $");

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
#include <sys/file.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <sys/sysctl.h>
#include <sys/conf.h>
#include <sys/kauth.h>

#include <miscfs/specfs/specdev.h>

#include <ufs/lfs/ulfs_quotacommon.h>
#include <ufs/lfs/ulfs_inode.h>
#include <ufs/lfs/ulfsmount.h>
#include <ufs/lfs/ulfs_extern.h>

#include <uvm/uvm_extern.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_accessors.h>
#include <ufs/lfs/lfs_kernel.h>
#include <ufs/lfs/lfs_extern.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/genfs/genfs_node.h>

/*
 * Roll-forward code.
 */
static daddr_t check_segsum(struct lfs *, daddr_t, u_int64_t,
    kauth_cred_t, int, int *, struct lwp *);

static bool all_selector(void *, struct vnode *);
static void drop_vnode_pages(struct mount *, struct lwp *);
static int update_inogen(struct lfs *, daddr_t);
static void update_inoblk_copy_dinode(struct lfs *, union lfs_dinode *, const union lfs_dinode *);

extern int lfs_do_rfw;
int rblkcnt;
int lfs_rfw_max_psegs = 0;

/*
 * Allocate a particular inode with a particular version number, freeing
 * any previous versions of this inode that may have gone before.
 * Used by the roll-forward code.
 *
 * XXX this function does not have appropriate locking to be used on a live fs;
 * XXX but something similar could probably be used for an "undelete" call.
 *
 * Called with the Ifile inode locked.
 */
int
lfs_rf_valloc(struct lfs *fs, ino_t ino, int vers, struct lwp *l,
	      struct vnode **vpp, union lfs_dinode *dip)
{
	struct vattr va;
	struct vnode *vp;
	struct inode *ip;
	int error;

	KASSERT(ino > LFS_IFILE_INUM);
	ASSERT_SEGLOCK(fs); /* XXX it doesn't, really */

	/*
	 * First, just try a vget. If the version number is the one we want,
	 * we don't have to do anything else.  If the version number is wrong,
	 * take appropriate action.
	 */
	error = VFS_VGET(fs->lfs_ivnode->v_mount, ino, LK_EXCLUSIVE, &vp);
	if (error == 0) {
		DLOG((DLOG_RF, "lfs_rf_valloc[1]: ino %d vp %p\n",
			(int)ino, vp));

		*vpp = vp;
		ip = VTOI(vp);
		DLOG((DLOG_RF, "  ip->i_gen=%jd dip nlink %jd seeking"
			" version %jd\n", (intmax_t)ip->i_gen,
			(intmax_t)(dip == NULL ? -1
				: lfs_dino_getnlink(fs, dip)), (intmax_t)vers));
		if (ip->i_gen == vers) {
			/*
			 * We have what we wanted already.
			 */
			DLOG((DLOG_RF, "  pre-existing\n"));
			return 0;
		} else if (ip->i_gen < vers && dip != NULL
			&& lfs_dino_getnlink(fs, dip) > 0) {
			/*
			 * We have found a newer version.  Truncate
			 * the old vnode to zero and re-initialize
			 * from the given dinode.
			 */
			DLOG((DLOG_RF, "  replace old version %jd\n",
				(intmax_t)ip->i_gen));
			lfs_truncate(vp, (off_t)0, 0, NOCRED);
			ip->i_gen = vers;
			vp->v_type = IFTOVT(lfs_dino_getmode(fs, dip));
			update_inoblk_copy_dinode(fs, ip->i_din, dip);
			LFS_SET_UINO(ip, IN_CHANGE | IN_UPDATE);
			return 0;
		} else {
			/*
			 * Not the right version and nothing to
			 * initialize from.  Don't recover this data.
			 */
			DLOG((DLOG_RF, "ino %d: sought version %d, got %d\n",
				(int)ino, (int)vers,
				(int)lfs_dino_getgen(fs, ip->i_din)));
			vput(vp);
			*vpp = NULLVP;
			return EEXIST;
		}
	}

	/*
	 * No version of this inode was found in the cache.
	 * Make a new one from the dinode.  We will add data blocks
	 * as they come in, so scrub any block addresses off of the
	 * inode and reset block counts to zero.
	 */
	if (dip == NULL)
		return ENOENT;

	vattr_null(&va);
	va.va_type = IFTOVT(lfs_dino_getmode(fs, dip));
	va.va_mode = lfs_dino_getmode(fs, dip) & ALLPERMS;
	va.va_fileid = ino;
	va.va_gen = vers;
	error = vcache_new(fs->lfs_ivnode->v_mount, NULL, &va, NOCRED, NULL,
	    &vp);
	if (error)
		return error;
	error = vn_lock(vp, LK_EXCLUSIVE);
	if (error)
		goto err;

	ip = VTOI(vp);
	update_inoblk_copy_dinode(fs, ip->i_din, dip);

	DLOG((DLOG_RF, "lfs_valloc[2] ino %d vp %p size=%lld effnblks=%d,"
		" blocks=%d\n", (int)ino, vp, (long long)ip->i_size,
		(int)ip->i_lfs_effnblks,
		(int)lfs_dino_getblocks(fs, ip->i_din)));
	*vpp = vp;
	return 0;

err:
	vrele(vp);
	*vpp = NULLVP;
	return error;
}

/*
 * Load the appropriate indirect block, and change the appropriate pointer.
 * Mark the block dirty.  Do segment and avail accounting.
 */
static int
update_meta(struct lfs *fs, ino_t ino, int vers, daddr_t lbn,
	    daddr_t ndaddr, size_t size, struct lwp *l)
{
	int error;
	struct vnode *vp;
	struct inode *ip;
	daddr_t odaddr;
	struct indir a[ULFS_NIADDR];
	int num;
	struct buf *bp;
	SEGUSE *sup;
	u_int64_t newsize, loff;

	KASSERT(lbn >= 0);	/* no indirect blocks */
	KASSERT(ino > LFS_IFILE_INUM);
	
	DLOG((DLOG_RF, "update_meta: ino %d lbn %d size %d at 0x%jx\n",
	      (int)ino, (int)lbn, (int)size, (uintmax_t)ndaddr));

	if ((error = lfs_rf_valloc(fs, ino, vers, l, &vp, NULL)) != 0)
		return error;
	ip = VTOI(vp);

	/*
	 * If block already exists, note its new location
	 * but do not account it as new.
	 */
	ulfs_bmaparray(vp, lbn, &odaddr, &a[0], &num, NULL, NULL);
	if (odaddr == UNASSIGNED) {
		if ((error = lfs_balloc(vp, (lbn << lfs_sb_getbshift(fs)),
					size, NOCRED, 0, &bp)) != 0) {
			vput(vp);
			return (error);
		}
		/* No need to write, the block is already on disk */
		if (bp->b_oflags & BO_DELWRI) {
			LFS_UNLOCK_BUF(bp);
			/* Account recovery of the previous version */
			lfs_sb_addavail(fs, lfs_btofsb(fs, bp->b_bcount));
		}
		brelse(bp, BC_INVAL);
		DLOG((DLOG_RF, "balloc ip->i_lfs_effnblks = %d,"
			" lfs_dino_getblocks(fs, ip->i_din) = %d\n",
			(int)ip->i_lfs_effnblks,
			(int)lfs_dino_getblocks(fs, ip->i_din)));
	} else {
		/* XXX fragextend? */
		DLOG((DLOG_RF, "block exists, no balloc\n"));
	}

	/*
	 * Extend the file, if it is not large enough already.
	 * XXX this is not exactly right, we don't know how much of the
	 * XXX last block is actually used.
	 */
	loff = lfs_lblktosize(fs, lbn);
	if (loff >= (ULFS_NDADDR << lfs_sb_getbshift(fs))) {
		/* No fragments */
		newsize = loff + 1;
	} else {
		/* Subtract only a fragment to account for block size */
		newsize = loff + size - lfs_fsbtob(fs, 1) + 1;
	}
	
	if (ip->i_size < newsize) {
		DLOG((DLOG_RF, "ino %d size %d -> %d\n",
		      (int)ino, (int)ip->i_size, (int)newsize));
		lfs_dino_setsize(fs, ip->i_din, newsize);
		ip->i_size = newsize;
		/*
		 * tell vm our new size for the case the inode won't
		 * appear later.
		 */
		uvm_vnp_setsize(vp, newsize);
	}

	lfs_update_single(fs, NULL, vp, lbn, ndaddr, size);

	LFS_SEGENTRY(sup, fs, lfs_dtosn(fs, ndaddr), bp);
	sup->su_nbytes += size;
	LFS_WRITESEGENTRY(sup, fs, lfs_dtosn(fs, ndaddr), bp);

	/* differences here should be due to UNWRITTEN indirect blocks. */
	if (vp->v_type != VLNK) {
		if (!(ip->i_lfs_effnblks >= lfs_dino_getblocks(fs, ip->i_din))
#if 0
		    || !(lfs_lblkno(fs, ip->i_size) > ULFS_NDADDR ||
			 ip->i_lfs_effnblks == lfs_dino_getblocks(fs, ip->i_din))
#endif /* 0 */
			) {
			vprint("vnode", vp);
			printf("effnblks=%jd dino_getblocks=%jd\n",
			       (intmax_t)ip->i_lfs_effnblks,
			       (intmax_t)lfs_dino_getblocks(fs, ip->i_din));
		}
		KASSERT(ip->i_lfs_effnblks >= lfs_dino_getblocks(fs, ip->i_din));
#if 0
		KASSERT(lfs_lblkno(fs, ip->i_size) > ULFS_NDADDR ||
			ip->i_lfs_effnblks == lfs_dino_getblocks(fs, ip->i_din));
#endif /* 0 */
	}

#ifdef DEBUG
	/* Now look again to make sure it worked */
	ulfs_bmaparray(vp, lbn, &odaddr, &a[0], &num, NULL, NULL);
	if (LFS_DBTOFSB(fs, odaddr) != ndaddr)
		DLOG((DLOG_RF, "update_meta: failed setting ino %jd lbn %jd"
		      " to %jd\n", (intmax_t)ino, (intmax_t)lbn, (intmax_t)ndaddr));
#endif /* DEBUG */
	vput(vp);
	return 0;
}

/*
 * Copy some the fields of the dinode as needed by update_inoblk().
 */
static void
update_inoblk_copy_dinode(struct lfs *fs,
    union lfs_dinode *dstu, const union lfs_dinode *srcu)
{
	if (fs->lfs_is64) {
		struct lfs64_dinode *dst = &dstu->u_64;
		const struct lfs64_dinode *src = &srcu->u_64;
		unsigned i;

		/*
		 * Copy everything but the block pointers and di_blocks.
		 * XXX what about di_extb?
		 */
		dst->di_mode = src->di_mode;
		dst->di_nlink = src->di_nlink;
		dst->di_uid = src->di_uid;
		dst->di_gid = src->di_gid;
		dst->di_blksize = src->di_blksize;
		dst->di_size = src->di_size;
		dst->di_atime = src->di_atime;
		dst->di_mtime = src->di_mtime;
		dst->di_ctime = src->di_ctime;
		dst->di_birthtime = src->di_birthtime;
		dst->di_mtimensec = src->di_mtimensec;
		dst->di_atimensec = src->di_atimensec;
		dst->di_ctimensec = src->di_ctimensec;
		dst->di_birthnsec = src->di_birthnsec;
		dst->di_gen = src->di_gen;
		dst->di_kernflags = src->di_kernflags;
		dst->di_flags = src->di_flags;
		dst->di_extsize = src->di_extsize;
		dst->di_modrev = src->di_modrev;
		dst->di_inumber = src->di_inumber;
		for (i = 0; i < __arraycount(src->di_spare); i++) {
			dst->di_spare[i] = src->di_spare[i];
		}
		/* Short symlinks store their data in di_db. */
		if ((src->di_mode & LFS_IFMT) == LFS_IFLNK
		    && src->di_size < lfs_sb_getmaxsymlinklen(fs)) {
			memcpy(dst->di_db, src->di_db, src->di_size);
		}
	} else {
		struct lfs32_dinode *dst = &dstu->u_32;
		const struct lfs32_dinode *src = &srcu->u_32;

		/* Get mode, link count, size, and times */
		memcpy(dst, src, offsetof(struct lfs32_dinode, di_db[0]));

		/* Then the rest, except di_blocks */
		dst->di_flags = src->di_flags;
		dst->di_gen = src->di_gen;
		dst->di_uid = src->di_uid;
		dst->di_gid = src->di_gid;
		dst->di_modrev = src->di_modrev;

		/* Short symlinks store their data in di_db. */
		if ((src->di_mode & LFS_IFMT) == LFS_IFLNK
		    && src->di_size < lfs_sb_getmaxsymlinklen(fs)) {
			memcpy(dst->di_db, src->di_db, src->di_size);
		}
	}
}

static int
update_inoblk(struct lfs *fs, daddr_t offset, kauth_cred_t cred,
	      struct lwp *l)
{
	struct vnode *devvp, *vp;
	struct inode *ip;
	union lfs_dinode *dip;
	struct buf *dbp, *ibp;
	int error;
	daddr_t daddr;
	IFILE *ifp;
	SEGUSE *sup;
	unsigned i, num;
	uint32_t gen, osn, nsn;
	char *buf;

	devvp = VTOI(fs->lfs_ivnode)->i_devvp;

	/*
	 * Get the inode, update times and perms.
	 * DO NOT update disk blocks, we do that separately.
	 */
	error = bread(devvp, LFS_FSBTODB(fs, offset), lfs_sb_getibsize(fs),
	    0, &dbp);
	if (error) {
		DLOG((DLOG_RF, "update_inoblk: bread returned %d\n", error));
		return error;
	}
	buf = malloc(dbp->b_bcount, M_SEGMENT, M_WAITOK);
	memcpy(buf, dbp->b_data, dbp->b_bcount);
	brelse(dbp, BC_AGE);
	num = LFS_INOPB(fs);
	for (i = num; i-- > 0; ) {
		dip = DINO_IN_BLOCK(fs, buf, i);
		if (lfs_dino_getinumber(fs, dip) <= LFS_IFILE_INUM)
			continue;

		/* Check generation number */
		LFS_IENTRY(ifp, fs, lfs_dino_getinumber(fs, dip), ibp);
		gen = lfs_if_getversion(fs, ifp);
		brelse(ibp, 0);
		if (lfs_dino_getgen(fs, dip) < gen) {
			continue;
		}

		/*
		 * This inode is the newest generation.  Load it.
		 */
		error = lfs_rf_valloc(fs, lfs_dino_getinumber(fs, dip),
				      lfs_dino_getgen(fs, dip),
				      l, &vp, dip);
		if (error) {
			DLOG((DLOG_RF, "update_inoblk: lfs_rf_valloc"
			      " returned %d\n", error));
			continue;
		}
		ip = VTOI(vp);
		if (lfs_dino_getsize(fs, dip) != ip->i_size
		    && vp->v_type != VLNK) {
			/* XXX What should we do sith symlinks? */
			DLOG((DLOG_RF, "  ino %jd size %jd -> %jd\n",
				(intmax_t)lfs_dino_getinumber(fs, dip),
				(intmax_t)ip->i_size,
				(intmax_t)lfs_dino_getsize(fs, dip)));
			lfs_truncate(vp, lfs_dino_getsize(fs, dip), 0,
				     NOCRED);
		}
		update_inoblk_copy_dinode(fs, ip->i_din, dip);

		ip->i_flags = lfs_dino_getflags(fs, dip);
		ip->i_gen = lfs_dino_getgen(fs, dip);
		ip->i_uid = lfs_dino_getuid(fs, dip);
		ip->i_gid = lfs_dino_getgid(fs, dip);

		ip->i_mode = lfs_dino_getmode(fs, dip);
		ip->i_nlink = lfs_dino_getnlink(fs, dip);
		ip->i_size = lfs_dino_getsize(fs, dip);

		LFS_SET_UINO(ip, IN_CHANGE | IN_UPDATE);

		/* Re-initialize to get type right */
		ulfs_vinit(vp->v_mount, lfs_specop_p, lfs_fifoop_p,
			  &vp);

		/* Record change in location */
		LFS_IENTRY(ifp, fs, lfs_dino_getinumber(fs, dip), ibp);
		daddr = lfs_if_getdaddr(fs, ifp);
		lfs_if_setdaddr(fs, ifp, offset);
		error = LFS_BWRITE_LOG(ibp); /* Ifile */
		/* And do segment accounting */
		osn = lfs_dtosn(fs, daddr);
		nsn = lfs_dtosn(fs, offset);
		if (DADDR_IS_BAD(daddr) || osn != nsn) {
			if (!DADDR_IS_BAD(daddr)) {
				LFS_SEGENTRY(sup, fs, osn, ibp);
				sup->su_nbytes -= DINOSIZE(fs);
				LFS_WRITESEGENTRY(sup, fs, osn, ibp);
			}
			LFS_SEGENTRY(sup, fs, nsn, ibp);
			sup->su_nbytes += DINOSIZE(fs);
			LFS_WRITESEGENTRY(sup, fs, nsn, ibp);
		}
		vput(vp);
	}
	free(buf, M_SEGMENT);
	
	return 0;
}

/*
 * Note the highest generation number of each inode in the Ifile.
 * This allows us to skip processing data for intermediate versions.
 */
static int
update_inogen(struct lfs *fs, daddr_t offset)
{
	struct vnode *devvp;
	union lfs_dinode *dip;
	struct buf *dbp, *ibp;
	int error;
	IFILE *ifp;
	unsigned i, num;

	devvp = VTOI(fs->lfs_ivnode)->i_devvp;

	/* Read inode block */
	error = bread(devvp, LFS_FSBTODB(fs, offset), lfs_sb_getibsize(fs),
	    0, &dbp);
	if (error) {
		DLOG((DLOG_RF, "update_inoblk: bread returned %d\n", error));
		return error;
	}

	/* Check each inode against ifile entry */
	num = LFS_INOPB(fs);
	for (i = num; i-- > 0; ) {
		dip = DINO_IN_BLOCK(fs, dbp->b_data, i);
		if (lfs_dino_getinumber(fs, dip) == LFS_IFILE_INUM)
			continue;

		/* Update generation number */
		LFS_IENTRY(ifp, fs, lfs_dino_getinumber(fs, dip), ibp);
		if (lfs_if_getversion(fs, ifp) < lfs_dino_getgen(fs, dip))
			lfs_if_setversion(fs, ifp, lfs_dino_getgen(fs, dip));
		error = LFS_BWRITE_LOG(ibp); /* Ifile */
		if (error)
			break;
	}
	brelse(dbp, BC_AGE);

	return error;
}

#define CHECK_CKSUM	1	/* Check the checksum to make sure it's valid */
#define CHECK_GEN	2	/* Update highest generation number */
#define CHECK_INODES	3	/* Read and process inodes */
#define CHECK_DATA	4	/* Identify and process data blocks */

static daddr_t
check_segsum(struct lfs *fs, daddr_t offset, u_int64_t nextserial,
	     kauth_cred_t cred, int phase, int *pseg_flags, struct lwp *l)
{
	struct vnode *devvp;
	struct buf *bp, *dbp;
	int error, ninos, i, j;
	SEGSUM *ssp;
	daddr_t prevoffset;
	IINFO *iip;
	FINFO *fip;
	SEGUSE *sup;
	size_t size;
	uint32_t datasum, foundsum;
	char *buf;

	devvp = VTOI(fs->lfs_ivnode)->i_devvp;
	
	/*
	 * If this is segment 0, skip the label.
	 * If the segment has a superblock and we're at the top
	 * of the segment, skip the superblock.
	 */
	if (offset == lfs_sb_gets0addr(fs))
		offset += lfs_btofsb(fs, LFS_LABELPAD);
	if (lfs_sntod(fs, lfs_dtosn(fs, offset)) == offset) {
		LFS_SEGENTRY(sup, fs, lfs_dtosn(fs, offset), bp);
		if (sup->su_flags & SEGUSE_SUPERBLOCK)
			offset += lfs_btofsb(fs, LFS_SBPAD);
		brelse(bp, 0);
	}

	/* Read in the segment summary */
	error = bread(devvp, LFS_FSBTODB(fs, offset), lfs_sb_getsumsize(fs),
	    0, &bp);
	if (error)
		return -1;
	buf = malloc(bp->b_bcount, M_SEGMENT, M_WAITOK);
	memcpy(buf, bp->b_data, bp->b_bcount);
	brelse(bp, BC_AGE);

	ssp = (SEGSUM *)buf;

	/*
	 * Phase I: Check summary checksum.
	 */
	if (phase == CHECK_CKSUM) {
		size_t sumstart;

		sumstart = lfs_ss_getsumstart(fs);
		if (lfs_ss_getsumsum(fs, ssp) !=
		    cksum((char *)ssp + sumstart,
			  lfs_sb_getsumsize(fs) - sumstart)) {
			DLOG((DLOG_RF, "Sumsum error at 0x%" PRIx64 "\n",
				offset));
			offset = -1;
			goto err;
		}
#if 0
		/*
		 * Under normal conditions, we should never be producing
		 * a partial segment with neither inode blocks nor data blocks.
		 * However, these do sometimes appear and they need not
		 * prevent us from continuing.
		 */
		if (lfs_ss_getnfinfo(fs, ssp) == 0 &&
		    lfs_ss_getninos(fs, ssp) == 0) {
			DLOG((DLOG_RF, "Empty pseg at 0x%" PRIx64 "\n",
				offset));
			offset = -1;
			goto err;
		}
#endif /* 0 */
		if (lfs_sb_getversion(fs) == 1) {
			if (lfs_ss_getcreate(fs, ssp) < lfs_sb_gettstamp(fs)) {
				DLOG((DLOG_RF, "Old data at 0x%" PRIx64 "\n", offset));
				offset = -1;
				goto err;
			}
		} else {
			if (lfs_ss_getserial(fs, ssp) != nextserial) {
				DLOG((DLOG_RF, "Serial number at 0x%jx given as 0x%jx,"
				      " expected 0x%jx\n", (intmax_t)offset,
				      (intmax_t)lfs_ss_getserial(fs, ssp),
				      (intmax_t)nextserial));
				offset = -1;
				goto err;
			}
			if (lfs_ss_getident(fs, ssp) != lfs_sb_getident(fs)) {
				DLOG((DLOG_RF, "Incorrect fsid (0x%x vs 0x%x) at 0x%"
				      PRIx64 "\n", lfs_ss_getident(fs, ssp),
				      lfs_sb_getident(fs), offset));
				offset = -1;
				goto err;
			}
		}
	}
	if (pseg_flags)
		*pseg_flags = lfs_ss_getflags(fs, ssp);
	prevoffset = offset;
	offset += lfs_btofsb(fs, lfs_sb_getsumsize(fs));

	/* Handle individual blocks */
	foundsum = 0;
	ninos = howmany(lfs_ss_getninos(fs, ssp), LFS_INOPB(fs));
	iip = SEGSUM_IINFOSTART(fs, buf);
	fip = SEGSUM_FINFOBASE(fs, (SEGSUM *)buf);
	for (i = 0; i < lfs_ss_getnfinfo(fs, ssp) || ninos; ++i) {
		/* Inode block? */
		if (ninos && lfs_ii_getblock(fs, iip) == offset) {
			if (phase == CHECK_CKSUM) {
				/* Read in the head and add to the buffer */
				error = bread(devvp, LFS_FSBTODB(fs, offset),
					lfs_sb_getbsize(fs), 0, &dbp);
				if (error) {
					offset = -1;
					goto err;
				}
				foundsum = lfs_cksum_part(dbp->b_data,
					sizeof(uint32_t), foundsum);
				brelse(dbp, BC_AGE);
			}
			if (phase == CHECK_GEN) {
				if ((error = update_inogen(fs, offset))
				    != 0) {
					offset = -1;
					goto err;
				}
			}
			if (phase == CHECK_INODES) {
				if ((error = update_inoblk(fs, offset, cred, l))
				    != 0) {
					offset = -1;
					goto err;
				}
			}
			offset += lfs_btofsb(fs, lfs_sb_getibsize(fs));
			iip = NEXTLOWER_IINFO(fs, iip);
			--ninos;
			--i; /* compensate for ++i in loop header */
			continue;
		}

		/* File block */
		size = lfs_sb_getbsize(fs);
		for (j = 0; j < lfs_fi_getnblocks(fs, fip); ++j) {
			if (j == lfs_fi_getnblocks(fs, fip) - 1)
				size = lfs_fi_getlastlength(fs, fip);
			if (phase == CHECK_CKSUM) {
				error = bread(devvp, LFS_FSBTODB(fs, offset),
						size, 0, &dbp);
				if (error) {
					offset = -1;
					goto err;
				}
				foundsum = lfs_cksum_part(dbp->b_data,
					  sizeof(uint32_t), foundsum);
				brelse(dbp, BC_AGE);
			}
			/* Account for and update any direct blocks */
			if (phase == CHECK_DATA &&
			   lfs_fi_getino(fs, fip) > LFS_IFILE_INUM &&
			   lfs_fi_getblock(fs, fip, j) >= 0) {
				update_meta(fs, lfs_fi_getino(fs, fip),
					    lfs_fi_getversion(fs, fip),
					    lfs_fi_getblock(fs, fip, j),
					    offset, size, l);
				++rblkcnt;
			}
			offset += lfs_btofsb(fs, size);
		}

		fip = NEXT_FINFO(fs, fip);
	}

	/* Checksum the array, compare */
	if (phase == CHECK_CKSUM) {
		datasum = lfs_ss_getdatasum(fs, ssp);
		foundsum = lfs_cksum_fold(foundsum);
		if (datasum != foundsum) {
			DLOG((DLOG_RF, "Datasum error at 0x%" PRIx64
			      " (wanted %x got %x)\n",
			      offset, datasum, foundsum));
			offset = -1;
			goto err;
		}
	}

	if (phase == CHECK_CKSUM)
		lfs_sb_subavail(fs, offset - prevoffset);
	else {
		/* Don't clog the buffer queue */
		mutex_enter(&lfs_lock);
		if (locked_queue_count > LFS_MAX_BUFS ||
		    locked_queue_bytes > LFS_MAX_BYTES) {
			lfs_flush(fs, SEGM_CKP, 0);
		}
		mutex_exit(&lfs_lock);
	}

	/*
	 * If we're at the end of the segment, move to the next.
	 * A partial segment needs space for a segment header (1 fsb)
	 * and a full block ("frag" fsb).  Thus, adding "frag" fsb should
	 * still be within the current segment (whereas frag + 1 might
	 * be at the start of the next segment).
	 *
	 * This needs to match the definition of LFS_PARTIAL_FITS
	 * in lfs_segment.c.
	 */
	if (lfs_dtosn(fs, offset + lfs_sb_getfrag(fs))
	    != lfs_dtosn(fs, offset)) {
		if (lfs_dtosn(fs, offset) == lfs_dtosn(fs, lfs_ss_getnext(fs,
									ssp))) {
			printf("WHOA! at 0x%jx/seg %jd moving to 0x%jx/seg %jd\n",
			       (intmax_t)offset,
			       (intmax_t)lfs_dtosn(fs, offset),
			       (intmax_t)lfs_ss_getnext(fs, ssp),
			       (intmax_t)lfs_dtosn(fs, lfs_ss_getnext(fs, ssp)));
			offset = -1;
			goto err;
		}
		offset = lfs_ss_getnext(fs, ssp);
		DLOG((DLOG_RF, "LFS roll forward: moving to offset 0x%" PRIx64
		       " -> segment %d\n", offset, lfs_dtosn(fs,offset)));
	}

    err:
	free(buf, M_SEGMENT);
	
	return offset;
}

void
lfs_roll_forward(struct lfs *fs, struct mount *mp, struct lwp *l)
{
	int flags, dirty, phase;
	daddr_t startoffset, offset, nextoffset, endpseg;
	u_int64_t nextserial, startserial, endserial;
	int sn, curseg;
	struct proc *p;
	kauth_cred_t cred;
	SEGUSE *sup;
	struct buf *bp;

	p = l ? l->l_proc : NULL;
	cred = p ? p->p_cred : NOCRED;

	/*
	 * Roll forward.
	 *
	 * We don't roll forward for v1 filesystems, because
	 * of the danger that the clock was turned back between the last
	 * checkpoint and crash.  This would roll forward garbage.
	 *
	 * v2 filesystems don't have this problem because they use a
	 * monotonically increasing serial number instead of a timestamp.
	 */
	rblkcnt = 0;
	if ((lfs_sb_getpflags(fs) & LFS_PF_CLEAN) || !lfs_do_rfw
	    || lfs_sb_getversion(fs) <= 1 || p == NULL)
		return;

	DLOG((DLOG_RF, "%s: begin roll forward at serial 0x%jx\n",
		lfs_sb_getfsmnt(fs), (intmax_t)lfs_sb_getserial(fs)));
	DEBUG_CHECK_FREELIST(fs);
		
	/*
	 * Phase I: Find the address of the last good partial
	 * segment that was written after the checkpoint.  Mark
	 * the segments in question dirty, so they won't be
	 * reallocated.
	 */
	endpseg = startoffset = offset = lfs_sb_getoffset(fs);
	flags = 0x0;
	DLOG((DLOG_RF, "LFS roll forward phase 1: start at offset 0x%"
	      PRIx64 "\n", offset));
	LFS_SEGENTRY(sup, fs, lfs_dtosn(fs, offset), bp);
	if (!(sup->su_flags & SEGUSE_DIRTY))
		lfs_sb_subnclean(fs, 1);
	sup->su_flags |= SEGUSE_DIRTY;
	LFS_WRITESEGENTRY(sup, fs, lfs_dtosn(fs, offset), bp);
			
	startserial = lfs_sb_getserial(fs);
	endserial = nextserial = startserial + 1;
	while ((nextoffset = check_segsum(fs, offset, nextserial,
	    cred, CHECK_CKSUM, &flags, l)) > 0) {
		if (lfs_sntod(fs, offset) != lfs_sntod(fs, nextoffset)) {
			LFS_SEGENTRY(sup, fs, lfs_dtosn(fs, offset),
				     bp);
			if (!(sup->su_flags & SEGUSE_DIRTY))
				lfs_sb_subnclean(fs, 1);
			sup->su_flags |= SEGUSE_DIRTY;
			LFS_WRITESEGENTRY(sup, fs, lfs_dtosn(fs, offset), bp);
		}

		DLOG((DLOG_RF, "LFS roll forward phase 1: offset=0x%jx"
			" serial=0x%jx\n", (intmax_t)nextoffset,
			(intmax_t)nextserial));
		if (flags & SS_DIROP) {
			DLOG((DLOG_RF, "lfs_mountfs: dirops at 0x%"
			      PRIx64 "\n", offset));
			if (!(flags & SS_CONT)) {
			     DLOG((DLOG_RF, "lfs_mountfs: dirops end "
				   "at 0x%" PRIx64 "\n", offset));
			}
		}
		offset = nextoffset;
		++nextserial;
		
		if (!(flags & SS_CONT)) {
			endpseg = nextoffset;
			endserial = nextserial;
		}
		if (lfs_rfw_max_psegs > 0
		    && nextserial > startserial + lfs_rfw_max_psegs)
			break;
	}
	if (flags & SS_CONT) {
		DLOG((DLOG_RF, "LFS roll forward: warning: incomplete "
			"dirops discarded (0x%jx < 0x%jx)\n",
			endpseg, nextoffset));
	}
	if (lfs_sb_getversion(fs) > 1)
		lfs_sb_setserial(fs, endserial);
	DLOG((DLOG_RF, "LFS roll forward phase 1: completed: "
	      "endpseg=0x%" PRIx64 "\n", endpseg));
	offset = startoffset;
	if (offset != endpseg) {
		/* Don't overwrite what we're trying to preserve */
		lfs_sb_setoffset(fs, endpseg);
		lfs_sb_setcurseg(fs, lfs_sntod(fs, lfs_dtosn(fs, endpseg)));
		for (sn = curseg = lfs_dtosn(fs, lfs_sb_getcurseg(fs));;) {
			sn = (sn + 1) % lfs_sb_getnseg(fs);
			/* XXX could we just fail to roll forward? */
			if (sn == curseg)
				panic("lfs_mountfs: no clean segments");
			LFS_SEGENTRY(sup, fs, sn, bp);
			dirty = (sup->su_flags & SEGUSE_DIRTY);
			brelse(bp, 0);
			if (!dirty)
				break;
		}
		lfs_sb_setnextseg(fs, lfs_sntod(fs, sn));
		/* Explicitly set this segment dirty */
		LFS_SEGENTRY(sup, fs, lfs_dtosn(fs, endpseg), bp);
		sup->su_flags |= SEGUSE_DIRTY | SEGUSE_ACTIVE;
		LFS_WRITESEGENTRY(sup, fs, lfs_dtosn(fs, endpseg), bp);


		/*
		 * Phase II: Identify the highest generation of each
		 * inode.
		 *
		 * Phase III: Update inodes.  We end up with the
		 * last version of each inode present, and can ignore
		 * data blocks belonging to previous versions.
		 *
		 * Phase IV: Roll forward, updating data blocks.
		 */
		for (phase = CHECK_GEN; phase <= CHECK_DATA; ++phase) {
			offset = startoffset;
			nextserial = startserial + 1;
			printf("LFS roll forward phase %d beginning\n", phase);
			while (offset > 0 && offset != endpseg) {
				if (phase == CHECK_DATA) {
					DLOG((DLOG_RF, "LFS roll forward"
					" phase %d: offset=0x%jx"
					" serial=0x%jx\n",
					phase, (intmax_t)offset,
					(intmax_t)nextserial));
				}
				offset = check_segsum(fs, offset,
			    		nextserial, cred,
					phase, NULL, l);
				++nextserial;
				DEBUG_CHECK_FREELIST(fs);
			}
		}

		/*
		 * Finish: flush our changes to disk.
		 */
		lfs_sb_setserial(fs, endserial);

		lfs_segwrite(mp, SEGM_CKP | SEGM_SYNC);
		DLOG((DLOG_RF, "lfs_mountfs: roll forward "
		      "examined %jd blocks\n",
		      (intmax_t)(endpseg - startoffset)));
	}

	/* Get rid of our vnodes, except the ifile */
	drop_vnode_pages(mp, l);
	DLOG((DLOG_RF, "LFS roll forward complete\n"));
	printf("%s: roll forward recovered %d data blocks\n",
		lfs_sb_getfsmnt(fs), rblkcnt);

	/*
	 * At this point we have no more changes to write to disk.
	 * Reset the "avail" count to match the segments as they
	 * appear on disk, and the clean segment count.
	 */
	lfs_reset_avail(fs);
}

static bool
all_selector(void *cl, struct vnode *vp)
{  
	return true;
}


/*
 * Dump any pages from vnodes that may have been put on
 * during truncation.
 */
static void
drop_vnode_pages(struct mount *mp, struct lwp *l)
{       
       struct vnode_iterator *marker;
       struct lfs *fs;
       struct vnode *vp;
      
       fs = VFSTOULFS(mp)->um_lfs;
       vfs_vnode_iterator_init(mp, &marker);
       while ((vp = vfs_vnode_iterator_next(marker,
               all_selector, NULL)) != NULL) {
               if (vp == fs->lfs_ivnode)
                       continue;
               VOP_LOCK(vp, LK_EXCLUSIVE | LK_RETRY);
               uvm_vnp_setsize(vp, 0);
               uvm_vnp_setsize(vp, VTOI(vp)->i_size);
               VOP_UNLOCK(vp);
               vrele(vp);
       }
       vfs_vnode_iterator_destroy(marker);
}     

