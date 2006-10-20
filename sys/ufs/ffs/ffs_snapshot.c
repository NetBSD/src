/*	$NetBSD: ffs_snapshot.c,v 1.34 2006/10/20 18:58:12 reinoud Exp $	*/

/*
 * Copyright 2000 Marshall Kirk McKusick. All Rights Reserved.
 *
 * Further information about snapshots can be obtained from:
 *
 *	Marshall Kirk McKusick		http://www.mckusick.com/softdep/
 *	1614 Oxford Street		mckusick@mckusick.com
 *	Berkeley, CA 94709-1608		+1-510-843-9542
 *	USA
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY MARSHALL KIRK MCKUSICK ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL MARSHALL KIRK MCKUSICK BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ffs_snapshot.c	8.11 (McKusick) 7/23/00
 *
 *	from FreeBSD: ffs_snapshot.c,v 1.79 2004/02/13 02:02:06 kuriyama Exp
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ffs_snapshot.c,v 1.34 2006/10/20 18:58:12 reinoud Exp $");

#if defined(_KERNEL_OPT)
#include "opt_ffs.h"
#endif

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/sched.h>
#include <sys/stat.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/vnode.h>
#include <sys/kauth.h>

#include <miscfs/specfs/specdev.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufs_extern.h>
#include <ufs/ufs/ufs_bswap.h>

#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

/* FreeBSD -> NetBSD conversion */
#define KERNCRED	lwp0.l_cred
#define ufs1_daddr_t	int32_t
#define ufs2_daddr_t	int64_t
#define ufs_lbn_t	daddr_t
#define VI_MTX(v)	(&(v)->v_interlock)
#define VI_LOCK(v)	simple_lock(&(v)->v_interlock)
#define VI_UNLOCK(v)	simple_unlock(&(v)->v_interlock)
#define MNT_ILOCK(v)	simple_lock(&mntvnode_slock)
#define MNT_IUNLOCK(v)	simple_unlock(&mntvnode_slock)

#if !defined(FFS_NO_SNAPSHOT)
static int cgaccount(int, struct vnode *, caddr_t, int);
static int expunge_ufs1(struct vnode *, struct inode *, struct fs *,
    int (*)(struct vnode *, ufs1_daddr_t *, ufs1_daddr_t *, struct fs *,
    ufs_lbn_t, int), int);
static int indiracct_ufs1(struct vnode *, struct vnode *, int,
    ufs1_daddr_t, ufs_lbn_t, ufs_lbn_t, ufs_lbn_t, ufs_lbn_t, struct fs *,
    int (*)(struct vnode *, ufs1_daddr_t *, ufs1_daddr_t *, struct fs *,
    ufs_lbn_t, int), int);
static int fullacct_ufs1(struct vnode *, ufs1_daddr_t *, ufs1_daddr_t *,
    struct fs *, ufs_lbn_t, int);
static int snapacct_ufs1(struct vnode *, ufs1_daddr_t *, ufs1_daddr_t *,
    struct fs *, ufs_lbn_t, int);
static int mapacct_ufs1(struct vnode *, ufs1_daddr_t *, ufs1_daddr_t *,
    struct fs *, ufs_lbn_t, int);
static int expunge_ufs2(struct vnode *, struct inode *, struct fs *,
    int (*)(struct vnode *, ufs2_daddr_t *, ufs2_daddr_t *, struct fs *,
    ufs_lbn_t, int), int);
static int indiracct_ufs2(struct vnode *, struct vnode *, int,
    ufs2_daddr_t, ufs_lbn_t, ufs_lbn_t, ufs_lbn_t, ufs_lbn_t, struct fs *,
    int (*)(struct vnode *, ufs2_daddr_t *, ufs2_daddr_t *, struct fs *,
    ufs_lbn_t, int), int);
static int fullacct_ufs2(struct vnode *, ufs2_daddr_t *, ufs2_daddr_t *,
    struct fs *, ufs_lbn_t, int);
static int snapacct_ufs2(struct vnode *, ufs2_daddr_t *, ufs2_daddr_t *,
    struct fs *, ufs_lbn_t, int);
static int mapacct_ufs2(struct vnode *, ufs2_daddr_t *, ufs2_daddr_t *,
    struct fs *, ufs_lbn_t, int);
#endif /* !defined(FFS_NO_SNAPSHOT) */

static int ffs_copyonwrite(void *, struct buf *);
static int readfsblk(struct vnode *, caddr_t, ufs2_daddr_t);
static int __unused readvnblk(struct vnode *, caddr_t, ufs2_daddr_t);
static int writevnblk(struct vnode *, caddr_t, ufs2_daddr_t);
static inline int cow_enter(void);
static inline void cow_leave(int);
static inline ufs2_daddr_t db_get(struct inode *, int);
static inline void db_assign(struct inode *, int, ufs2_daddr_t);
static inline ufs2_daddr_t idb_get(struct inode *, caddr_t, int);
static inline void idb_assign(struct inode *, caddr_t, int, ufs2_daddr_t);

#ifdef DEBUG
static int snapdebug = 0;
#endif

/*
 * Create a snapshot file and initialize it for the filesystem.
 * Vnode is locked on entry and return.
 */
int
ffs_snapshot(struct mount *mp __unused, struct vnode *vp __unused,
    struct timespec *ctime __unused)
{
#if defined(FFS_NO_SNAPSHOT)
	return EOPNOTSUPP;
}
#else /* defined(FFS_NO_SNAPSHOT) */
	ufs2_daddr_t numblks, blkno, *blkp, snaplistsize = 0, *snapblklist;
	int error, ns, cg, snaploc;
	int i, s, size, len, loc;
	int flag = mp->mnt_flag;
	struct timeval starttime;
#ifdef DEBUG
	struct timeval endtime;
#endif
	struct timespec ts;
	long redo = 0;
	int32_t *lp;
	void *space;
	caddr_t sbbuf = NULL;
	struct ufsmount *ump = VFSTOUFS(mp);
	struct fs *copy_fs = NULL, *fs = ump->um_fs;
	struct lwp *l = curlwp;
	struct inode *ip, *xp;
	struct buf *bp, *ibp, *nbp;
	struct vattr vat;
	struct vnode *xvp, *devvp;

	ns = UFS_FSNEEDSWAP(fs);
	/*
	 * Need to serialize access to snapshot code per filesystem.
	 */
	/*
	 * If the vnode already is a snapshot, return.
	 */
	if (VTOI(vp)->i_flags & SF_SNAPSHOT) {
		if (ctime) {
			ctime->tv_sec = DIP(VTOI(vp), mtime);
			ctime->tv_nsec = DIP(VTOI(vp), mtimensec);
		}
		return 0;
	}
	/*
	 * Check mount, exclusive reference and owner.
	 */
	if (vp->v_mount != mp)
		return EXDEV;
	if (vp->v_usecount != 1 || vp->v_writecount != 0)
		return EBUSY;
	if (kauth_authorize_generic(l->l_cred, KAUTH_GENERIC_ISSUSER,
	    &l->l_acflag) != 0 &&
	    VTOI(vp)->i_uid != kauth_cred_geteuid(l->l_cred))
		return EACCES;

	if (vp->v_size != 0) {
		error = ffs_truncate(vp, 0, 0, NOCRED, l);
		if (error)
			return error;
	}
	/*
	 * Assign a snapshot slot in the superblock.
	 */
	for (snaploc = 0; snaploc < FSMAXSNAP; snaploc++)
		if (fs->fs_snapinum[snaploc] == 0)
			break;
	if (snaploc == FSMAXSNAP)
		return (ENOSPC);
	ip = VTOI(vp);
	devvp = ip->i_devvp;
	/*
	 * Write an empty list of preallocated blocks to the end of
	 * the snapshot to set size to at least that of the filesystem.
	 */
	numblks = howmany(fs->fs_size, fs->fs_frag);
	blkno = 1;
	blkno = ufs_rw64(blkno, ns);
	error = vn_rdwr(UIO_WRITE, vp,
	    (caddr_t)&blkno, sizeof(blkno), lblktosize(fs, (off_t)numblks),
	    UIO_SYSSPACE, IO_NODELOCKED|IO_UNIT, l->l_cred, NULL, NULL);
	if (error)
		goto out;
	/*
	 * Preallocate critical data structures so that we can copy
	 * them in without further allocation after we suspend all
	 * operations on the filesystem. We would like to just release
	 * the allocated buffers without writing them since they will
	 * be filled in below once we are ready to go, but this upsets
	 * the soft update code, so we go ahead and write the new buffers.
	 *
	 * Allocate all indirect blocks and mark all of them as not
	 * needing to be copied.
	 */
	for (blkno = NDADDR; blkno < numblks; blkno += NINDIR(fs)) {
		error = ffs_balloc(vp, lblktosize(fs, (off_t)blkno),
		    fs->fs_bsize, l->l_cred, B_METAONLY, &ibp);
		if (error)
			goto out;
		bawrite(ibp);
	}
	/*
	 * Allocate copies for the superblock and its summary information.
	 */
	error = ffs_balloc(vp, fs->fs_sblockloc, fs->fs_sbsize, KERNCRED,
	    0, &nbp);
	if (error)
		goto out;
	bawrite(nbp);
	blkno = fragstoblks(fs, fs->fs_csaddr);
	len = howmany(fs->fs_cssize, fs->fs_bsize);
	for (loc = 0; loc < len; loc++) {
		error = ffs_balloc(vp, lblktosize(fs, (off_t)(blkno + loc)),
		    fs->fs_bsize, KERNCRED, 0, &nbp);
		if (error)
			goto out;
		bawrite(nbp);
	}
	/*
	 * Copy all the cylinder group maps. Although the
	 * filesystem is still active, we hope that only a few
	 * cylinder groups will change between now and when we
	 * suspend operations. Thus, we will be able to quickly
	 * touch up the few cylinder groups that changed during
	 * the suspension period.
	 */
	len = howmany(fs->fs_ncg, NBBY);
	fs->fs_active = malloc(len, M_DEVBUF, M_WAITOK | M_ZERO);
	for (cg = 0; cg < fs->fs_ncg; cg++) {
		if ((error = ffs_balloc(vp, lfragtosize(fs, cgtod(fs, cg)),
		    fs->fs_bsize, KERNCRED, 0, &nbp)) != 0)
			goto out;
		error = cgaccount(cg, vp, nbp->b_data, 1);
		bawrite(nbp);
		if (error)
			goto out;
	}
	/*
	 * Change inode to snapshot type file.
	 */
	ip->i_flags |= SF_SNAPSHOT;
	DIP_ASSIGN(ip, flags, ip->i_flags);
	ip->i_flag |= IN_CHANGE | IN_UPDATE;
	/*
	 * Ensure that the snapshot is completely on disk.
	 * Since we have marked it as a snapshot it is safe to
	 * unlock it as no process will be allowed to write to it.
	 */
	if ((error = VOP_FSYNC(vp, KERNCRED, FSYNC_WAIT, 0, 0, l)) != 0)
		goto out;
	VOP_UNLOCK(vp, 0);
	/*
	 * All allocations are done, so we can now snapshot the system.
	 *
	 * Suspend operation on filesystem.
	 */
	if ((error = vfs_write_suspend(vp->v_mount, PUSER|PCATCH, 0)) != 0) {
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
		goto out;
	}
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	getmicrotime(&starttime);
	/*
	 * First, copy all the cylinder group maps that have changed.
	 */
	for (cg = 0; cg < fs->fs_ncg; cg++) {
		if (ACTIVECG_ISSET(fs, cg))
			continue;
		redo++;
		if ((error = ffs_balloc(vp, lfragtosize(fs, cgtod(fs, cg)),
		    fs->fs_bsize, KERNCRED, 0, &nbp)) != 0)
			goto out1;
		error = cgaccount(cg, vp, nbp->b_data, 2);
		bawrite(nbp);
		if (error)
			goto out1;
	}
	/*
	 * Grab a copy of the superblock and its summary information.
	 * We delay writing it until the suspension is released below.
	 */
	sbbuf = malloc(fs->fs_bsize, M_UFSMNT, M_WAITOK);
	loc = blkoff(fs, fs->fs_sblockloc);
	if (loc > 0)
		bzero(&sbbuf[0], loc);
	copy_fs = (struct fs *)(sbbuf + loc);
	bcopy(fs, copy_fs, fs->fs_sbsize);
	size = fs->fs_bsize < SBLOCKSIZE ? fs->fs_bsize : SBLOCKSIZE;
	if (fs->fs_sbsize < size)
		bzero(&sbbuf[loc + fs->fs_sbsize], size - fs->fs_sbsize);
	size = blkroundup(fs, fs->fs_cssize);
	if (fs->fs_contigsumsize > 0)
		size += fs->fs_ncg * sizeof(int32_t);
	space = malloc((u_long)size, M_UFSMNT, M_WAITOK);
	copy_fs->fs_csp = space;
	bcopy(fs->fs_csp, copy_fs->fs_csp, fs->fs_cssize);
	space = (char *)space + fs->fs_cssize;
	loc = howmany(fs->fs_cssize, fs->fs_fsize);
	i = fs->fs_frag - loc % fs->fs_frag;
	len = (i == fs->fs_frag) ? 0 : i * fs->fs_fsize;
	if (len > 0) {
		if ((error = bread(devvp, fsbtodb(fs, fs->fs_csaddr + loc),
		    len, KERNCRED, &bp)) != 0) {
			brelse(bp);
			free(copy_fs->fs_csp, M_UFSMNT);
			goto out1;
		}
		bcopy(bp->b_data, space, (u_int)len);
		space = (char *)space + len;
		bp->b_flags |= B_INVAL | B_NOCACHE;
		brelse(bp);
	}
	if (fs->fs_contigsumsize > 0) {
		copy_fs->fs_maxcluster = lp = space;
		for (i = 0; i < fs->fs_ncg; i++)
			*lp++ = fs->fs_contigsumsize;
	}
	/*
	 * We must check for active files that have been unlinked
	 * (e.g., with a zero link count). We have to expunge all
	 * trace of these files from the snapshot so that they are
	 * not reclaimed prematurely by fsck or unnecessarily dumped.
	 * We turn off the MNTK_SUSPENDED flag to avoid a panic from
	 * spec_strategy about writing on a suspended filesystem.
	 * Note that we skip unlinked snapshot files as they will
	 * be handled separately below.
	 *
	 * We also calculate the needed size for the snapshot list.
	 */
	snaplistsize = fs->fs_ncg + howmany(fs->fs_cssize, fs->fs_bsize) +
	    FSMAXSNAP + 1 /* superblock */ + 1 /* last block */ + 1 /* size */;
	MNT_ILOCK(mp);
loop:
	TAILQ_FOREACH(xvp, &mp->mnt_vnodelist, v_mntvnodes) {
		/*
		 * Make sure this vnode wasn't reclaimed in getnewvnode().
		 * Start over if it has (it won't be on the list anymore).
		 */
		if (xvp->v_mount != mp)
			goto loop;
		VI_LOCK(xvp);
		MNT_IUNLOCK(mp);
		if ((xvp->v_flag & VXLOCK) ||
		    xvp->v_usecount == 0 || xvp->v_type == VNON ||
		    (VTOI(xvp)->i_flags & SF_SNAPSHOT)) {
			VI_UNLOCK(xvp);
			MNT_ILOCK(mp);
			continue;
		}
		if (vn_lock(xvp, LK_EXCLUSIVE | LK_INTERLOCK) != 0) {
			MNT_ILOCK(mp);
			goto loop;
		}
#ifdef DEBUG
		if (snapdebug)
			vprint("ffs_snapshot: busy vnode", xvp);
#endif
		if (VOP_GETATTR(xvp, &vat, l->l_cred, l) == 0 &&
		    vat.va_nlink > 0) {
			VOP_UNLOCK(xvp, 0);
			MNT_ILOCK(mp);
			continue;
		}
		xp = VTOI(xvp);
		if (ffs_checkfreefile(copy_fs, vp, xp->i_number)) {
			VOP_UNLOCK(xvp, 0);
			MNT_ILOCK(mp);
			continue;
		}
		/*
		 * If there is a fragment, clear it here.
		 */
		blkno = 0;
		loc = howmany(xp->i_size, fs->fs_bsize) - 1;
		if (loc < NDADDR) {
			len = fragroundup(fs, blkoff(fs, xp->i_size));
			if (len > 0 && len < fs->fs_bsize) {
				ffs_blkfree(copy_fs, vp, db_get(xp, loc),
				    len, xp->i_number);
				blkno = db_get(xp, loc);
				db_assign(xp, loc, 0);
			}
		}
		snaplistsize += 1;
		if (xp->i_ump->um_fstype == UFS1)
			error = expunge_ufs1(vp, xp, copy_fs, fullacct_ufs1,
			    BLK_NOCOPY);
		else
			error = expunge_ufs2(vp, xp, copy_fs, fullacct_ufs2,
			    BLK_NOCOPY);
		if (blkno)
			db_assign(xp, loc, blkno);
		if (!error)
			error = ffs_freefile(copy_fs, vp, xp->i_number,
			    xp->i_mode);
		VOP_UNLOCK(xvp, 0);
		if (error) {
			free(copy_fs->fs_csp, M_UFSMNT);
			goto out1;
		}
		MNT_ILOCK(mp);
	}
	MNT_IUNLOCK(mp);
	/*
	 * If there already exist snapshots on this filesystem, grab a
	 * reference to their shared lock. If this is the first snapshot
	 * on this filesystem, we need to allocate a lock for the snapshots
	 * to share. In either case, acquire the snapshot lock and give
	 * up our original private lock.
	 */
	VI_LOCK(devvp);
	if ((xp = TAILQ_FIRST(&ump->um_snapshots)) != NULL) {
		struct lock *lkp;

		lkp = ITOV(xp)->v_vnlock;
		VI_UNLOCK(devvp);
		VI_LOCK(vp);
		vp->v_vnlock = lkp;
	} else {
		struct lock *lkp;

		VI_UNLOCK(devvp);
		MALLOC(lkp, struct lock *, sizeof(struct lock), M_UFSMNT,
		    M_WAITOK);
		lockinit(lkp, PVFS, "snaplk", 0, LK_CANRECURSE);
		VI_LOCK(vp);
		vp->v_vnlock = lkp;
	}
	vn_lock(vp, LK_INTERLOCK | LK_EXCLUSIVE | LK_RETRY);
	transferlockers(&vp->v_lock, vp->v_vnlock);
	lockmgr(&vp->v_lock, LK_RELEASE, NULL);
	/*
	 * If this is the first snapshot on this filesystem, then we need
	 * to allocate the space for the list of preallocated snapshot blocks.
	 * This list will be refined below, but this preliminary one will
	 * keep us out of deadlock until the full one is ready.
	 */
	if (xp == NULL) {
		snapblklist = malloc(
		    snaplistsize * sizeof(ufs2_daddr_t), M_UFSMNT, M_WAITOK);
		blkp = &snapblklist[1];
		*blkp++ = lblkno(fs, fs->fs_sblockloc);
		blkno = fragstoblks(fs, fs->fs_csaddr);
		for (cg = 0; cg < fs->fs_ncg; cg++) {
			if (fragstoblks(fs, cgtod(fs, cg)) > blkno)
				break;
			*blkp++ = fragstoblks(fs, cgtod(fs, cg));
		}
		len = howmany(fs->fs_cssize, fs->fs_bsize);
		for (loc = 0; loc < len; loc++)
			*blkp++ = blkno + loc;
		for (; cg < fs->fs_ncg; cg++)
			*blkp++ = fragstoblks(fs, cgtod(fs, cg));
		snapblklist[0] = blkp - snapblklist;
		VI_LOCK(devvp);
		if (ump->um_snapblklist != NULL)
			panic("ffs_snapshot: non-empty list");
		ump->um_snapblklist = snapblklist;
		VI_UNLOCK(devvp);
	}
	/*
	 * Record snapshot inode. Since this is the newest snapshot,
	 * it must be placed at the end of the list.
	 */
	VI_LOCK(devvp);
	fs->fs_snapinum[snaploc] = ip->i_number;
	if (ip->i_nextsnap.tqe_prev != 0)
		panic("ffs_snapshot: %llu already on list",
		    (unsigned long long)ip->i_number);
	TAILQ_INSERT_TAIL(&ump->um_snapshots, ip, i_nextsnap);
	VI_UNLOCK(devvp);
	if (xp == NULL)
		vn_cow_establish(devvp, ffs_copyonwrite, devvp);
	vp->v_flag |= VSYSTEM;
out1:
	/*
	 * Resume operation on filesystem.
	 */
	vfs_write_resume(vp->v_mount);
	/*
	 * Set the mtime to the time the snapshot has been taken.
	 */
	TIMEVAL_TO_TIMESPEC(&starttime, &ts);
	if (ctime)
		*ctime = ts;
	DIP_ASSIGN(ip, mtime, ts.tv_sec);
	DIP_ASSIGN(ip, mtimensec, ts.tv_nsec);
	ip->i_flag |= IN_CHANGE | IN_UPDATE;

#ifdef DEBUG
	if (starttime.tv_sec > 0) {
		getmicrotime(&endtime);
		timersub(&endtime, &starttime, &endtime);
		printf("%s: suspended %ld.%03ld sec, redo %ld of %d\n",
		    vp->v_mount->mnt_stat.f_mntonname, (long)endtime.tv_sec,
		    endtime.tv_usec / 1000, redo, fs->fs_ncg);
	}
#endif
	if (error)
		goto out;
	/*
	 * Copy allocation information from all the snapshots in
	 * this snapshot and then expunge them from its view.
	 */
	TAILQ_FOREACH(xp, &ump->um_snapshots, i_nextsnap) {
		if (xp == ip)
			break;
		if (xp->i_ump->um_fstype == UFS1)
			error = expunge_ufs1(vp, xp, fs, snapacct_ufs1,
			    BLK_SNAP);
		else
			error = expunge_ufs2(vp, xp, fs, snapacct_ufs2,
			    BLK_SNAP);
		if (error) {
			fs->fs_snapinum[snaploc] = 0;
			goto done;
		}
	}
	/*
	 * Allocate space for the full list of preallocated snapshot blocks.
	 */
	snapblklist = malloc(snaplistsize * sizeof(ufs2_daddr_t),
	    M_UFSMNT, M_WAITOK);
	ip->i_snapblklist = &snapblklist[1];
	/*
	 * Expunge the blocks used by the snapshots from the set of
	 * blocks marked as used in the snapshot bitmaps. Also, collect
	 * the list of allocated blocks in i_snapblklist.
	 */
	if (ip->i_ump->um_fstype == UFS1)
		error = expunge_ufs1(vp, ip, copy_fs, mapacct_ufs1, BLK_SNAP);
	else
		error = expunge_ufs2(vp, ip, copy_fs, mapacct_ufs2, BLK_SNAP);
	if (error) {
		fs->fs_snapinum[snaploc] = 0;
		FREE(snapblklist, M_UFSMNT);
		goto done;
	}
	if (snaplistsize < ip->i_snapblklist - snapblklist)
		panic("ffs_snapshot: list too small");
	snaplistsize = ip->i_snapblklist - snapblklist;
	snapblklist[0] = snaplistsize;
	ip->i_snapblklist = &snapblklist[0];
	/*
	 * Write out the list of allocated blocks to the end of the snapshot.
	 */
	for (i = 0; i < snaplistsize; i++)
		snapblklist[i] = ufs_rw64(snapblklist[i], ns);
	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)snapblklist,
	    snaplistsize*sizeof(ufs2_daddr_t), lblktosize(fs, (off_t)numblks),
	    UIO_SYSSPACE, IO_NODELOCKED|IO_UNIT, l->l_cred, NULL, NULL);
	for (i = 0; i < snaplistsize; i++)
		snapblklist[i] = ufs_rw64(snapblklist[i], ns);
	if (error) {
		fs->fs_snapinum[snaploc] = 0;
		FREE(snapblklist, M_UFSMNT);
		goto done;
	}
	/*
	 * Write the superblock and its summary information
	 * to the snapshot.
	 */
	blkno = fragstoblks(fs, fs->fs_csaddr);
	len = howmany(fs->fs_cssize, fs->fs_bsize);
	space = copy_fs->fs_csp;
#ifdef FFS_EI
	if (ns) {
		ffs_sb_swap(copy_fs, copy_fs);
		ffs_csum_swap(space, space, fs->fs_cssize);
	}
#endif
	for (loc = 0; loc < len; loc++) {
		error = bread(vp, blkno + loc, fs->fs_bsize, KERNCRED, &nbp);
		if (error) {
			brelse(nbp);
			fs->fs_snapinum[snaploc] = 0;
			FREE(snapblklist, M_UFSMNT);
			goto done;
		}
		bcopy(space, nbp->b_data, fs->fs_bsize);
		space = (char *)space + fs->fs_bsize;
		bawrite(nbp);
	}
	/*
	 * As this is the newest list, it is the most inclusive, so
	 * should replace the previous list. If this is the first snapshot
	 * free the preliminary list.
	 */
	VI_LOCK(devvp);
	space = ump->um_snapblklist;
	ump->um_snapblklist = snapblklist;
	VI_UNLOCK(devvp);
	if (TAILQ_FIRST(&ump->um_snapshots) == ip)
		FREE(space, M_UFSMNT);
done:
	free(copy_fs->fs_csp, M_UFSMNT);
	if (!error) {
		error = bread(vp, lblkno(fs, fs->fs_sblockloc), fs->fs_bsize,
		    KERNCRED, &nbp);
		if (error) {
			brelse(nbp);
			fs->fs_snapinum[snaploc] = 0;
		}
		bcopy(sbbuf, nbp->b_data, fs->fs_bsize);
		bawrite(nbp);
	}
out:
	/*
	 * Invalidate and free all pages on the snapshot vnode.
	 * All metadata has been written through the buffer cache.
	 * Clean all dirty buffers now to avoid UBC inconsistencies.
	 */
	if (!error) {
		simple_lock(&vp->v_interlock);
		error = VOP_PUTPAGES(vp, 0, 0,
		    PGO_ALLPAGES|PGO_CLEANIT|PGO_SYNCIO|PGO_FREE);
	}
	if (!error) {
		s = splbio();
		for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
			nbp = LIST_NEXT(bp, b_vnbufs);
			simple_lock(&bp->b_interlock);
			splx(s);
			if ((bp->b_flags & (B_DELWRI|B_BUSY)) != B_DELWRI)
				panic("ffs_snapshot: not dirty or busy, bp %p",
				    bp);
			bp->b_flags |= B_BUSY|B_VFLUSH;
			if (LIST_FIRST(&bp->b_dep) == NULL)
				bp->b_flags |= B_NOCACHE;
			simple_unlock(&bp->b_interlock);
			bwrite(bp);
			s = splbio();
		}
		simple_lock(&global_v_numoutput_slock);
		while (vp->v_numoutput) {
			vp->v_flag |= VBWAIT;
			ltsleep((caddr_t)&vp->v_numoutput, PRIBIO+1,
			    "snapflushbuf", 0, &global_v_numoutput_slock);
		}
		simple_unlock(&global_v_numoutput_slock);
		splx(s);
	}
	if (sbbuf)
		free(sbbuf, M_UFSMNT);
	if (fs->fs_active != 0) {
		FREE(fs->fs_active, M_DEVBUF);
		fs->fs_active = 0;
	}
	mp->mnt_flag = flag;
	if (error)
		(void) ffs_truncate(vp, (off_t)0, 0, NOCRED, l);
	else
		vref(vp);
	return (error);
}

/*
 * Copy a cylinder group map. All the unallocated blocks are marked
 * BLK_NOCOPY so that the snapshot knows that it need not copy them
 * if they are later written. If passno is one, then this is a first
 * pass, so only setting needs to be done. If passno is 2, then this
 * is a revision to a previous pass which must be undone as the
 * replacement pass is done.
 */
static int
cgaccount(int cg, struct vnode *vp, caddr_t data, int passno)
{
	struct buf *bp, *ibp;
	struct inode *ip;
	struct cg *cgp;
	struct fs *fs;
	ufs2_daddr_t base, numblks;
	int error, len, loc, ns, indiroff;

	ip = VTOI(vp);
	fs = ip->i_fs;
	ns = UFS_FSNEEDSWAP(fs);
	error = bread(ip->i_devvp, fsbtodb(fs, cgtod(fs, cg)),
		(int)fs->fs_cgsize, KERNCRED, &bp);
	if (error) {
		brelse(bp);
		return (error);
	}
	cgp = (struct cg *)bp->b_data;
	if (!cg_chkmagic(cgp, ns)) {
		brelse(bp);
		return (EIO);
	}
	ACTIVECG_SET(fs, cg);

	bcopy(bp->b_data, data, fs->fs_cgsize);
	brelse(bp);
	if (fs->fs_cgsize < fs->fs_bsize)
		bzero(&data[fs->fs_cgsize],
		    fs->fs_bsize - fs->fs_cgsize);
	numblks = howmany(fs->fs_size, fs->fs_frag);
	len = howmany(fs->fs_fpg, fs->fs_frag);
	base = cg * fs->fs_fpg / fs->fs_frag;
	if (base + len >= numblks)
		len = numblks - base - 1;
	loc = 0;
	if (base < NDADDR) {
		for ( ; loc < NDADDR; loc++) {
			if (ffs_isblock(fs, cg_blksfree(cgp, ns), loc))
				db_assign(ip, loc, BLK_NOCOPY);
			else if (db_get(ip, loc) == BLK_NOCOPY) {
				if (passno == 2)
					db_assign(ip, loc, 0);
				else if (passno == 1)
					panic("ffs_snapshot: lost direct block");
			}
		}
	}
	if ((error = ffs_balloc(vp, lblktosize(fs, (off_t)(base + loc)),
	    fs->fs_bsize, KERNCRED, B_METAONLY, &ibp)) != 0)
		return (error);
	indiroff = (base + loc - NDADDR) % NINDIR(fs);
	for ( ; loc < len; loc++, indiroff++) {
		if (indiroff >= NINDIR(fs)) {
			bawrite(ibp);
			if ((error = ffs_balloc(vp,
			    lblktosize(fs, (off_t)(base + loc)),
			    fs->fs_bsize, KERNCRED, B_METAONLY, &ibp)) != 0)
				return (error);
			indiroff = 0;
		}
		if (ffs_isblock(fs, cg_blksfree(cgp, ns), loc))
			idb_assign(ip, ibp->b_data, indiroff, BLK_NOCOPY);
		else if (idb_get(ip, ibp->b_data, indiroff) == BLK_NOCOPY) {
			if (passno == 2)
				idb_assign(ip, ibp->b_data, indiroff, 0);
			else if (passno == 1)
				panic("ffs_snapshot: lost indirect block");
		}
	}
	bdwrite(ibp);
	return (0);
}

/*
 * Before expunging a snapshot inode, note all the
 * blocks that it claims with BLK_SNAP so that fsck will
 * be able to account for those blocks properly and so
 * that this snapshot knows that it need not copy them
 * if the other snapshot holding them is freed. This code
 * is reproduced once each for UFS1 and UFS2.
 */
static int
expunge_ufs1(struct vnode *snapvp, struct inode *cancelip, struct fs *fs,
    int (*acctfunc)(struct vnode *, ufs1_daddr_t *, ufs1_daddr_t *,
		    struct fs *, ufs_lbn_t, int),
    int expungetype)
{
	int i, s, error, ns, indiroff;
	ufs_lbn_t lbn, rlbn;
	ufs2_daddr_t len, blkno, numblks, blksperindir;
	struct ufs1_dinode *dip;
	struct buf *bp;
	caddr_t bf;

	ns = UFS_FSNEEDSWAP(fs);
	/*
	 * Prepare to expunge the inode. If its inode block has not
	 * yet been copied, then allocate and fill the copy.
	 */
	lbn = fragstoblks(fs, ino_to_fsba(fs, cancelip->i_number));
	blkno = 0;
	if (lbn < NDADDR) {
		blkno = db_get(VTOI(snapvp), lbn);
	} else {
		s = cow_enter();
		error = ffs_balloc(snapvp, lblktosize(fs, (off_t)lbn),
		   fs->fs_bsize, KERNCRED, B_METAONLY, &bp);
		cow_leave(s);
		if (error)
			return (error);
		indiroff = (lbn - NDADDR) % NINDIR(fs);
		blkno = idb_get(VTOI(snapvp), bp->b_data, indiroff);
		brelse(bp);
	}
	bf = malloc(fs->fs_bsize, M_UFSMNT, M_WAITOK);
	if (blkno != 0)
		error = readvnblk(snapvp, bf, lbn);
	else
		error = readfsblk(snapvp, bf, lbn);
	if (error) {
		free(bf, M_UFSMNT);
		return error;
	}
	/*
	 * Set a snapshot inode to be a zero length file, regular files
	 * to be completely unallocated.
	 */
	dip = (struct ufs1_dinode *)bf + ino_to_fsbo(fs, cancelip->i_number);
	if (expungetype == BLK_NOCOPY)
		dip->di_mode = 0;
	dip->di_size = 0;
	dip->di_blocks = 0;
	dip->di_flags =
	    ufs_rw32(ufs_rw32(dip->di_flags, ns) & ~SF_SNAPSHOT, ns);
	bzero(&dip->di_db[0], (NDADDR + NIADDR) * sizeof(ufs1_daddr_t));
	error = writevnblk(snapvp, bf, lbn);
	free(bf, M_UFSMNT);
	if (error)
		return error;
	/*
	 * Now go through and expunge all the blocks in the file
	 * using the function requested.
	 */
	numblks = howmany(cancelip->i_size, fs->fs_bsize);
	if ((error = (*acctfunc)(snapvp, &cancelip->i_ffs1_db[0],
	    &cancelip->i_ffs1_db[NDADDR], fs, 0, expungetype)))
		return (error);
	if ((error = (*acctfunc)(snapvp, &cancelip->i_ffs1_ib[0],
	    &cancelip->i_ffs1_ib[NIADDR], fs, -1, expungetype)))
		return (error);
	blksperindir = 1;
	lbn = -NDADDR;
	len = numblks - NDADDR;
	rlbn = NDADDR;
	for (i = 0; len > 0 && i < NIADDR; i++) {
		error = indiracct_ufs1(snapvp, ITOV(cancelip), i,
		    ufs_rw32(cancelip->i_ffs1_ib[i], ns), lbn, rlbn, len,
		    blksperindir, fs, acctfunc, expungetype);
		if (error)
			return (error);
		blksperindir *= NINDIR(fs);
		lbn -= blksperindir + 1;
		len -= blksperindir;
		rlbn += blksperindir;
	}
	return (0);
}

/*
 * Descend an indirect block chain for vnode cancelvp accounting for all
 * its indirect blocks in snapvp.
 */
static int
indiracct_ufs1(struct vnode *snapvp, struct vnode *cancelvp, int level,
    ufs1_daddr_t blkno, ufs_lbn_t lbn, ufs_lbn_t rlbn, ufs_lbn_t remblks,
    ufs_lbn_t blksperindir, struct fs *fs,
    int (*acctfunc)(struct vnode *, ufs1_daddr_t *, ufs1_daddr_t *,
		    struct fs *, ufs_lbn_t, int),
    int expungetype)
{
	int error, ns, num, i;
	ufs_lbn_t subblksperindir;
	struct indir indirs[NIADDR + 2];
	ufs1_daddr_t last, *bap;
	struct buf *bp;

	ns = UFS_FSNEEDSWAP(fs);

	if (blkno == 0) {
		if (expungetype == BLK_NOCOPY)
			return (0);
		panic("indiracct_ufs1: missing indir");
	}
	if ((error = ufs_getlbns(cancelvp, rlbn, indirs, &num)) != 0)
		return (error);
	if (lbn != indirs[num - 1 - level].in_lbn || num < 2)
		panic("indiracct_ufs1: botched params");
	/*
	 * We have to expand bread here since it will deadlock looking
	 * up the block number for any blocks that are not in the cache.
	 */
	bp = getblk(cancelvp, lbn, fs->fs_bsize, 0, 0);
	bp->b_blkno = fsbtodb(fs, blkno);
	if ((bp->b_flags & (B_DONE | B_DELWRI)) == 0 &&
	    (error = readfsblk(bp->b_vp, bp->b_data, fragstoblks(fs, blkno)))) {
		brelse(bp);
		return (error);
	}
	/*
	 * Account for the block pointers in this indirect block.
	 */
	last = howmany(remblks, blksperindir);
	if (last > NINDIR(fs))
		last = NINDIR(fs);
	bap = malloc(fs->fs_bsize, M_DEVBUF, M_WAITOK);
	bcopy(bp->b_data, (caddr_t)bap, fs->fs_bsize);
	brelse(bp);
	error = (*acctfunc)(snapvp, &bap[0], &bap[last], fs,
	    level == 0 ? rlbn : -1, expungetype);
	if (error || level == 0)
		goto out;
	/*
	 * Account for the block pointers in each of the indirect blocks
	 * in the levels below us.
	 */
	subblksperindir = blksperindir / NINDIR(fs);
	for (lbn++, level--, i = 0; i < last; i++) {
		error = indiracct_ufs1(snapvp, cancelvp, level,
		    ufs_rw32(bap[i], ns), lbn, rlbn, remblks, subblksperindir,
		    fs, acctfunc, expungetype);
		if (error)
			goto out;
		rlbn += blksperindir;
		lbn -= blksperindir;
		remblks -= blksperindir;
	}
out:
	FREE(bap, M_DEVBUF);
	return (error);
}

/*
 * Do both snap accounting and map accounting.
 */
static int
fullacct_ufs1(struct vnode *vp, ufs1_daddr_t *oldblkp, ufs1_daddr_t *lastblkp,
    struct fs *fs, ufs_lbn_t lblkno,
    int exptype /* BLK_SNAP or BLK_NOCOPY */)
{
	int error;

	if ((error = snapacct_ufs1(vp, oldblkp, lastblkp, fs, lblkno, exptype)))
		return (error);
	return (mapacct_ufs1(vp, oldblkp, lastblkp, fs, lblkno, exptype));
}

/*
 * Identify a set of blocks allocated in a snapshot inode.
 */
static int
snapacct_ufs1(struct vnode *vp, ufs1_daddr_t *oldblkp, ufs1_daddr_t *lastblkp,
    struct fs *fs, ufs_lbn_t lblkno __unused,
    int expungetype /* BLK_SNAP or BLK_NOCOPY */)
{
	struct inode *ip = VTOI(vp);
	ufs1_daddr_t blkno, *blkp;
	ufs_lbn_t lbn;
	struct buf *ibp;
	int error, ns;

	ns = UFS_FSNEEDSWAP(fs);

	for ( ; oldblkp < lastblkp; oldblkp++) {
		blkno = ufs_rw32(*oldblkp, ns);
		if (blkno == 0 || blkno == BLK_NOCOPY || blkno == BLK_SNAP)
			continue;
		lbn = fragstoblks(fs, blkno);
		if (lbn < NDADDR) {
			blkp = &ip->i_ffs1_db[lbn];
			ip->i_flag |= IN_CHANGE | IN_UPDATE;
		} else {
			error = ffs_balloc(vp, lblktosize(fs, (off_t)lbn),
			    fs->fs_bsize, KERNCRED, B_METAONLY, &ibp);
			if (error)
				return (error);
			blkp = &((ufs1_daddr_t *)(ibp->b_data))
			    [(lbn - NDADDR) % NINDIR(fs)];
		}
		/*
		 * If we are expunging a snapshot vnode and we
		 * find a block marked BLK_NOCOPY, then it is
		 * one that has been allocated to this snapshot after
		 * we took our current snapshot and can be ignored.
		 */
		blkno = ufs_rw32(*blkp, ns);
		if (expungetype == BLK_SNAP && blkno == BLK_NOCOPY) {
			if (lbn >= NDADDR)
				brelse(ibp);
		} else {
			if (blkno != 0)
				panic("snapacct_ufs1: bad block");
			*blkp = ufs_rw32(expungetype, ns);
			if (lbn >= NDADDR)
				bdwrite(ibp);
		}
	}
	return (0);
}

/*
 * Account for a set of blocks allocated in a snapshot inode.
 */
static int
mapacct_ufs1(struct vnode *vp, ufs1_daddr_t *oldblkp, ufs1_daddr_t *lastblkp,
    struct fs *fs, ufs_lbn_t lblkno, int expungetype)
{
	ufs1_daddr_t blkno;
	struct inode *ip;
	ino_t inum;
	int acctit, ns;

	ns = UFS_FSNEEDSWAP(fs);
	ip = VTOI(vp);
	inum = ip->i_number;
	if (lblkno == -1)
		acctit = 0;
	else
		acctit = 1;
	for ( ; oldblkp < lastblkp; oldblkp++, lblkno++) {
		blkno = ufs_rw32(*oldblkp, ns);
		if (blkno == 0 || blkno == BLK_NOCOPY)
			continue;
		if (acctit && expungetype == BLK_SNAP && blkno != BLK_SNAP)
			*ip->i_snapblklist++ = lblkno;
		if (blkno == BLK_SNAP)
			blkno = blkstofrags(fs, lblkno);
		ffs_blkfree(fs, vp, blkno, fs->fs_bsize, inum);
	}
	return (0);
}

/*
 * Before expunging a snapshot inode, note all the
 * blocks that it claims with BLK_SNAP so that fsck will
 * be able to account for those blocks properly and so
 * that this snapshot knows that it need not copy them
 * if the other snapshot holding them is freed. This code
 * is reproduced once each for UFS1 and UFS2.
 */
static int
expunge_ufs2(struct vnode *snapvp, struct inode *cancelip, struct fs *fs,
    int (*acctfunc)(struct vnode *, ufs2_daddr_t *, ufs2_daddr_t *,
		    struct fs *, ufs_lbn_t, int),
    int expungetype)
{
	int i, s, error, ns, indiroff;
	ufs_lbn_t lbn, rlbn;
	ufs2_daddr_t len, blkno, numblks, blksperindir;
	struct ufs2_dinode *dip;
	struct buf *bp;
	caddr_t bf;

	ns = UFS_FSNEEDSWAP(fs);
	/*
	 * Prepare to expunge the inode. If its inode block has not
	 * yet been copied, then allocate and fill the copy.
	 */
	lbn = fragstoblks(fs, ino_to_fsba(fs, cancelip->i_number));
	blkno = 0;
	if (lbn < NDADDR) {
		blkno = db_get(VTOI(snapvp), lbn);
	} else {
		s = cow_enter();
		error = ffs_balloc(snapvp, lblktosize(fs, (off_t)lbn),
		   fs->fs_bsize, KERNCRED, B_METAONLY, &bp);
		cow_leave(s);
		if (error)
			return (error);
		indiroff = (lbn - NDADDR) % NINDIR(fs);
		blkno = idb_get(VTOI(snapvp), bp->b_data, indiroff);
		brelse(bp);
	}
	bf = malloc(fs->fs_bsize, M_UFSMNT, M_WAITOK);
	if (blkno != 0)
		error = readvnblk(snapvp, bf, lbn);
	else
		error = readfsblk(snapvp, bf, lbn);
	if (error) {
		free(bf, M_UFSMNT);
		return error;
	}
	/*
	 * Set a snapshot inode to be a zero length file, regular files
	 * to be completely unallocated.
	 */
	dip = (struct ufs2_dinode *)bf + ino_to_fsbo(fs, cancelip->i_number);
	if (expungetype == BLK_NOCOPY)
		dip->di_mode = 0;
	dip->di_size = 0;
	dip->di_blocks = 0;
	dip->di_flags =
	    ufs_rw32(ufs_rw32(dip->di_flags, ns) & ~SF_SNAPSHOT, ns);
	bzero(&dip->di_db[0], (NDADDR + NIADDR) * sizeof(ufs2_daddr_t));
	error = writevnblk(snapvp, bf, lbn);
	free(bf, M_UFSMNT);
	if (error)
		return error;
	/*
	 * Now go through and expunge all the blocks in the file
	 * using the function requested.
	 */
	numblks = howmany(cancelip->i_size, fs->fs_bsize);
	if ((error = (*acctfunc)(snapvp, &cancelip->i_ffs2_db[0],
	    &cancelip->i_ffs2_db[NDADDR], fs, 0, expungetype)))
		return (error);
	if ((error = (*acctfunc)(snapvp, &cancelip->i_ffs2_ib[0],
	    &cancelip->i_ffs2_ib[NIADDR], fs, -1, expungetype)))
		return (error);
	blksperindir = 1;
	lbn = -NDADDR;
	len = numblks - NDADDR;
	rlbn = NDADDR;
	for (i = 0; len > 0 && i < NIADDR; i++) {
		error = indiracct_ufs2(snapvp, ITOV(cancelip), i,
		    ufs_rw64(cancelip->i_ffs2_ib[i], ns), lbn, rlbn, len,
		    blksperindir, fs, acctfunc, expungetype);
		if (error)
			return (error);
		blksperindir *= NINDIR(fs);
		lbn -= blksperindir + 1;
		len -= blksperindir;
		rlbn += blksperindir;
	}
	return (0);
}

/*
 * Descend an indirect block chain for vnode cancelvp accounting for all
 * its indirect blocks in snapvp.
 */
static int
indiracct_ufs2(struct vnode *snapvp, struct vnode *cancelvp, int level,
    ufs2_daddr_t blkno, ufs_lbn_t lbn, ufs_lbn_t rlbn, ufs_lbn_t remblks,
    ufs_lbn_t blksperindir, struct fs *fs,
    int (*acctfunc)(struct vnode *, ufs2_daddr_t *, ufs2_daddr_t *,
		    struct fs *, ufs_lbn_t, int),
    int expungetype)
{
	int error, ns, num, i;
	ufs_lbn_t subblksperindir;
	struct indir indirs[NIADDR + 2];
	ufs2_daddr_t last, *bap;
	struct buf *bp;

	ns = UFS_FSNEEDSWAP(fs);

	if (blkno == 0) {
		if (expungetype == BLK_NOCOPY)
			return (0);
		panic("indiracct_ufs2: missing indir");
	}
	if ((error = ufs_getlbns(cancelvp, rlbn, indirs, &num)) != 0)
		return (error);
	if (lbn != indirs[num - 1 - level].in_lbn || num < 2)
		panic("indiracct_ufs2: botched params");
	/*
	 * We have to expand bread here since it will deadlock looking
	 * up the block number for any blocks that are not in the cache.
	 */
	bp = getblk(cancelvp, lbn, fs->fs_bsize, 0, 0);
	bp->b_blkno = fsbtodb(fs, blkno);
	if ((bp->b_flags & (B_DONE | B_DELWRI)) == 0 &&
	    (error = readfsblk(bp->b_vp, bp->b_data, fragstoblks(fs, blkno)))) {
		brelse(bp);
		return (error);
	}
	/*
	 * Account for the block pointers in this indirect block.
	 */
	last = howmany(remblks, blksperindir);
	if (last > NINDIR(fs))
		last = NINDIR(fs);
	bap = malloc(fs->fs_bsize, M_DEVBUF, M_WAITOK);
	bcopy(bp->b_data, (caddr_t)bap, fs->fs_bsize);
	brelse(bp);
	error = (*acctfunc)(snapvp, &bap[0], &bap[last], fs,
	    level == 0 ? rlbn : -1, expungetype);
	if (error || level == 0)
		goto out;
	/*
	 * Account for the block pointers in each of the indirect blocks
	 * in the levels below us.
	 */
	subblksperindir = blksperindir / NINDIR(fs);
	for (lbn++, level--, i = 0; i < last; i++) {
		error = indiracct_ufs2(snapvp, cancelvp, level,
		    ufs_rw64(bap[i], ns), lbn, rlbn, remblks, subblksperindir,
		    fs, acctfunc, expungetype);
		if (error)
			goto out;
		rlbn += blksperindir;
		lbn -= blksperindir;
		remblks -= blksperindir;
	}
out:
	FREE(bap, M_DEVBUF);
	return (error);
}

/*
 * Do both snap accounting and map accounting.
 */
static int
fullacct_ufs2(struct vnode *vp, ufs2_daddr_t *oldblkp, ufs2_daddr_t *lastblkp,
    struct fs *fs, ufs_lbn_t lblkno,
    int exptype /* BLK_SNAP or BLK_NOCOPY */)
{
	int error;

	if ((error = snapacct_ufs2(vp, oldblkp, lastblkp, fs, lblkno, exptype)))
		return (error);
	return (mapacct_ufs2(vp, oldblkp, lastblkp, fs, lblkno, exptype));
}

/*
 * Identify a set of blocks allocated in a snapshot inode.
 */
static int
snapacct_ufs2(struct vnode *vp, ufs2_daddr_t *oldblkp, ufs2_daddr_t *lastblkp,
    struct fs *fs, ufs_lbn_t lblkno __unused,
    int expungetype /* BLK_SNAP or BLK_NOCOPY */)
{
	struct inode *ip = VTOI(vp);
	ufs2_daddr_t blkno, *blkp;
	ufs_lbn_t lbn;
	struct buf *ibp;
	int error, ns;

	ns = UFS_FSNEEDSWAP(fs);

	for ( ; oldblkp < lastblkp; oldblkp++) {
		blkno = ufs_rw64(*oldblkp, ns);
		if (blkno == 0 || blkno == BLK_NOCOPY || blkno == BLK_SNAP)
			continue;
		lbn = fragstoblks(fs, blkno);
		if (lbn < NDADDR) {
			blkp = &ip->i_ffs2_db[lbn];
			ip->i_flag |= IN_CHANGE | IN_UPDATE;
		} else {
			error = ffs_balloc(vp, lblktosize(fs, (off_t)lbn),
			    fs->fs_bsize, KERNCRED, B_METAONLY, &ibp);
			if (error)
				return (error);
			blkp = &((ufs2_daddr_t *)(ibp->b_data))
			    [(lbn - NDADDR) % NINDIR(fs)];
		}
		/*
		 * If we are expunging a snapshot vnode and we
		 * find a block marked BLK_NOCOPY, then it is
		 * one that has been allocated to this snapshot after
		 * we took our current snapshot and can be ignored.
		 */
		blkno = ufs_rw64(*blkp, ns);
		if (expungetype == BLK_SNAP && blkno == BLK_NOCOPY) {
			if (lbn >= NDADDR)
				brelse(ibp);
		} else {
			if (blkno != 0)
				panic("snapacct_ufs2: bad block");
			*blkp = ufs_rw64(expungetype, ns);
			if (lbn >= NDADDR)
				bdwrite(ibp);
		}
	}
	return (0);
}

/*
 * Account for a set of blocks allocated in a snapshot inode.
 */
static int
mapacct_ufs2(struct vnode *vp, ufs2_daddr_t *oldblkp, ufs2_daddr_t *lastblkp,
    struct fs *fs, ufs_lbn_t lblkno, int expungetype)
{
	ufs2_daddr_t blkno;
	struct inode *ip;
	ino_t inum;
	int acctit, ns;

	ns = UFS_FSNEEDSWAP(fs);
	ip = VTOI(vp);
	inum = ip->i_number;
	if (lblkno == -1)
		acctit = 0;
	else
		acctit = 1;
	for ( ; oldblkp < lastblkp; oldblkp++, lblkno++) {
		blkno = ufs_rw64(*oldblkp, ns);
		if (blkno == 0 || blkno == BLK_NOCOPY)
			continue;
		if (acctit && expungetype == BLK_SNAP && blkno != BLK_SNAP)
			*ip->i_snapblklist++ = lblkno;
		if (blkno == BLK_SNAP)
			blkno = blkstofrags(fs, lblkno);
		ffs_blkfree(fs, vp, blkno, fs->fs_bsize, inum);
	}
	return (0);
}
#endif /* defined(FFS_NO_SNAPSHOT) */

/*
 * Decrement extra reference on snapshot when last name is removed.
 * It will not be freed until the last open reference goes away.
 */
void
ffs_snapgone(struct inode *ip)
{
	struct ufsmount *ump = VFSTOUFS(ip->i_devvp->v_specmountpoint);
	struct inode *xp;
	struct fs *fs;
	int snaploc;

	/*
	 * Find snapshot in incore list.
	 */
	TAILQ_FOREACH(xp, &ump->um_snapshots, i_nextsnap)
		if (xp == ip)
			break;
	if (xp != NULL)
		vrele(ITOV(ip));
#ifdef DEBUG
	else if (snapdebug)
		printf("ffs_snapgone: lost snapshot vnode %llu\n",
		    (unsigned long long)ip->i_number);
#endif
	/*
	 * Delete snapshot inode from superblock. Keep list dense.
	 */
	fs = ip->i_fs;
	for (snaploc = 0; snaploc < FSMAXSNAP; snaploc++)
		if (fs->fs_snapinum[snaploc] == ip->i_number)
			break;
	if (snaploc < FSMAXSNAP) {
		for (snaploc++; snaploc < FSMAXSNAP; snaploc++) {
			if (fs->fs_snapinum[snaploc] == 0)
				break;
			fs->fs_snapinum[snaploc - 1] = fs->fs_snapinum[snaploc];
		}
		fs->fs_snapinum[snaploc - 1] = 0;
	}
}

/*
 * Prepare a snapshot file for being removed.
 */
void
ffs_snapremove(struct vnode *vp)
{
	struct inode *ip = VTOI(vp), *xp;
	struct vnode *devvp = ip->i_devvp;
	struct fs *fs = ip->i_fs;
	struct ufsmount *ump = VFSTOUFS(devvp->v_specmountpoint);
	struct lock *lkp;
	struct buf *ibp;
	ufs2_daddr_t numblks, blkno, dblk;
	int error, ns, loc, last;

	ns = UFS_FSNEEDSWAP(fs);
	/*
	 * If active, delete from incore list (this snapshot may
	 * already have been in the process of being deleted, so
	 * would not have been active).
	 *
	 * Clear copy-on-write flag if last snapshot.
	 */
	if (ip->i_nextsnap.tqe_prev != 0) {
		VI_LOCK(devvp);
		lockmgr(&vp->v_lock, LK_INTERLOCK | LK_EXCLUSIVE,
		    VI_MTX(devvp));
		VI_LOCK(devvp);
		TAILQ_REMOVE(&ump->um_snapshots, ip, i_nextsnap);
		ip->i_nextsnap.tqe_prev = 0;
		lkp = vp->v_vnlock;
		vp->v_vnlock = &vp->v_lock;
		lockmgr(lkp, LK_RELEASE, NULL);
		if (TAILQ_FIRST(&ump->um_snapshots) != 0) {
			/* Roll back the list of preallocated blocks. */
			xp = TAILQ_LAST(&ump->um_snapshots, inodelst);
			ump->um_snapblklist = xp->i_snapblklist;
			VI_UNLOCK(devvp);
		} else {
			ump->um_snapblklist = 0;
			lockmgr(lkp, LK_DRAIN|LK_INTERLOCK, VI_MTX(devvp));
			lockmgr(lkp, LK_RELEASE, NULL);
			vn_cow_disestablish(devvp, ffs_copyonwrite, devvp);
			FREE(lkp, M_UFSMNT);
		}
		FREE(ip->i_snapblklist, M_UFSMNT);
		ip->i_snapblklist = NULL;
	}
	/*
	 * Clear all BLK_NOCOPY fields. Pass any block claims to other
	 * snapshots that want them (see ffs_snapblkfree below).
	 */
	for (blkno = 1; blkno < NDADDR; blkno++) {
		dblk = db_get(ip, blkno);
		if (dblk == BLK_NOCOPY || dblk == BLK_SNAP)
			db_assign(ip, blkno, 0);
		else if ((dblk == blkstofrags(fs, blkno) &&
		     ffs_snapblkfree(fs, ip->i_devvp, dblk, fs->fs_bsize,
		     ip->i_number))) {
			DIP_ADD(ip, blocks, -btodb(fs->fs_bsize));
			db_assign(ip, blkno, 0);
		}
	}
	numblks = howmany(ip->i_size, fs->fs_bsize);
	for (blkno = NDADDR; blkno < numblks; blkno += NINDIR(fs)) {
		error = ffs_balloc(vp, lblktosize(fs, (off_t)blkno),
		    fs->fs_bsize, KERNCRED, B_METAONLY, &ibp);
		if (error)
			continue;
		if (fs->fs_size - blkno > NINDIR(fs))
			last = NINDIR(fs);
		else
			last = fs->fs_size - blkno;
		for (loc = 0; loc < last; loc++) {
			dblk = idb_get(ip, ibp->b_data, loc);
			if (dblk == BLK_NOCOPY || dblk == BLK_SNAP)
				idb_assign(ip, ibp->b_data, loc, 0);
			else if (dblk == blkstofrags(fs, blkno) &&
			    ffs_snapblkfree(fs, ip->i_devvp, dblk,
			    fs->fs_bsize, ip->i_number)) {
				DIP_ADD(ip, blocks, -btodb(fs->fs_bsize));
				idb_assign(ip, ibp->b_data, loc, 0);
			}
		}
		bawrite(ibp);
	}
	/*
	 * Clear snapshot flag and drop reference.
	 */
	ip->i_flags &= ~SF_SNAPSHOT;
	DIP_ASSIGN(ip, flags, ip->i_flags);
	ip->i_flag |= IN_CHANGE | IN_UPDATE;
}

/*
 * Notification that a block is being freed. Return zero if the free
 * should be allowed to proceed. Return non-zero if the snapshot file
 * wants to claim the block. The block will be claimed if it is an
 * uncopied part of one of the snapshots. It will be freed if it is
 * either a BLK_NOCOPY or has already been copied in all of the snapshots.
 * If a fragment is being freed, then all snapshots that care about
 * it must make a copy since a snapshot file can only claim full sized
 * blocks. Note that if more than one snapshot file maps the block,
 * we can pick one at random to claim it. Since none of the snapshots
 * can change, we are assurred that they will all see the same unmodified
 * image. When deleting a snapshot file (see ffs_snapremove above), we
 * must push any of these claimed blocks to one of the other snapshots
 * that maps it. These claimed blocks are easily identified as they will
 * have a block number equal to their logical block number within the
 * snapshot. A copied block can never have this property because they
 * must always have been allocated from a BLK_NOCOPY location.
 */
int
ffs_snapblkfree(struct fs *fs, struct vnode *devvp, ufs2_daddr_t bno,
    long size, ino_t inum __unused)
{
	struct ufsmount *ump = VFSTOUFS(devvp->v_specmountpoint);
	struct buf *ibp;
	struct inode *ip;
	struct vnode *vp = NULL, *saved_vp = NULL;
	caddr_t saved_data = NULL;
	ufs_lbn_t lbn;
	ufs2_daddr_t blkno;
	int s, indiroff = 0, snapshot_locked = 0, error = 0, claimedblk = 0;

	lbn = fragstoblks(fs, bno);
retry:
	VI_LOCK(devvp);
	TAILQ_FOREACH(ip, &ump->um_snapshots, i_nextsnap) {
		vp = ITOV(ip);
		/*
		 * Lookup block being written.
		 */
		if (lbn < NDADDR) {
			blkno = db_get(ip, lbn);
		} else {
			if (snapshot_locked == 0 &&
			    lockmgr(vp->v_vnlock,
			      LK_INTERLOCK | LK_EXCLUSIVE | LK_SLEEPFAIL,
			      VI_MTX(devvp)) != 0)
				goto retry;
			snapshot_locked = 1;
			s = cow_enter();
			error = ffs_balloc(vp, lblktosize(fs, (off_t)lbn),
			    fs->fs_bsize, KERNCRED, B_METAONLY, &ibp);
			cow_leave(s);
			if (error)
				break;
			indiroff = (lbn - NDADDR) % NINDIR(fs);
			blkno = idb_get(ip, ibp->b_data, indiroff);
		}
		/*
		 * Check to see if block needs to be copied.
		 */
		if (blkno == 0) {
			/*
			 * A block that we map is being freed. If it has not
			 * been claimed yet, we will claim or copy it (below).
			 */
			claimedblk = 1;
		} else if (blkno == BLK_SNAP) {
			/*
			 * No previous snapshot claimed the block,
			 * so it will be freed and become a BLK_NOCOPY
			 * (don't care) for us.
			 */
			if (claimedblk)
				panic("snapblkfree: inconsistent block type");
			if (snapshot_locked == 0 &&
			    lockmgr(vp->v_vnlock,
			      LK_INTERLOCK | LK_EXCLUSIVE | LK_NOWAIT,
			      VI_MTX(devvp)) != 0) {
#if 0 /* CID-2949: dead code */
				if (lbn >= NDADDR)
					brelse(ibp);
#endif 
				vn_lock(vp, LK_EXCLUSIVE | LK_SLEEPFAIL);
				goto retry;
			}
			snapshot_locked = 1;
			if (lbn < NDADDR) {
				db_assign(ip, lbn, BLK_NOCOPY);
				ip->i_flag |= IN_CHANGE | IN_UPDATE;
			} else {
				idb_assign(ip, ibp->b_data, indiroff,
				    BLK_NOCOPY);
				bwrite(ibp);
			}
			continue;
		} else /* BLK_NOCOPY or default */ {
			/*
			 * If the snapshot has already copied the block
			 * (default), or does not care about the block,
			 * it is not needed.
			 */
			if (lbn >= NDADDR)
				brelse(ibp);
			continue;
		}
		/*
		 * If this is a full size block, we will just grab it
		 * and assign it to the snapshot inode. Otherwise we
		 * will proceed to copy it. See explanation for this
		 * routine as to why only a single snapshot needs to
		 * claim this block.
		 */
		if (snapshot_locked == 0 &&
		    lockmgr(vp->v_vnlock,
		      LK_INTERLOCK | LK_EXCLUSIVE | LK_NOWAIT,
		      VI_MTX(devvp)) != 0) {
			vn_lock(vp, LK_EXCLUSIVE | LK_SLEEPFAIL);
			goto retry;
		}
		snapshot_locked = 1;
		if (size == fs->fs_bsize) {
#ifdef DEBUG
			if (snapdebug)
				printf("%s %llu lbn %" PRId64
				    "from inum %llu\n",
				    "Grabonremove: snapino",
				    (unsigned long long)ip->i_number,
				    lbn, (unsigned long long)inum);
#endif
			if (lbn < NDADDR) {
				db_assign(ip, lbn, bno);
			} else {
				idb_assign(ip, ibp->b_data, indiroff, bno);
				bwrite(ibp);
			}
			DIP_ADD(ip, blocks, btodb(size));
			ip->i_flag |= IN_CHANGE | IN_UPDATE;
			VOP_UNLOCK(vp, 0);
			return (1);
		}
		if (lbn >= NDADDR)
			brelse(ibp);
#ifdef DEBUG
		if (snapdebug)
			printf("%s%llu lbn %" PRId64 " %s %llu size %ld\n",
			    "Copyonremove: snapino ",
			    (unsigned long long)ip->i_number,
			    lbn, "for inum", (unsigned long long)inum, size);
#endif
		/*
		 * If we have already read the old block contents, then
		 * simply copy them to the new block. Note that we need
		 * to synchronously write snapshots that have not been
		 * unlinked, and hence will be visible after a crash,
		 * to ensure their integrity.
		 */
		if (saved_data) {
			error = writevnblk(vp, saved_data, lbn);
			if (error)
				break;
			continue;
		}
		/*
		 * Otherwise, read the old block contents into the buffer.
		 */
		saved_data = malloc(fs->fs_bsize, M_UFSMNT, M_WAITOK);
		saved_vp = vp;
		if ((error = readfsblk(vp, saved_data, lbn)) != 0) {
			free(saved_data, M_UFSMNT);
			saved_data = NULL;
			break;
		}
	}
	/*
	 * Note that we need to synchronously write snapshots that
	 * have not been unlinked, and hence will be visible after
	 * a crash, to ensure their integrity.
	 */
	if (saved_data) {
		error = writevnblk(saved_vp, saved_data, lbn);
		free(saved_data, M_UFSMNT);
	}
	/*
	 * If we have been unable to allocate a block in which to do
	 * the copy, then return non-zero so that the fragment will
	 * not be freed. Although space will be lost, the snapshot
	 * will stay consistent.
	 */
	if (snapshot_locked)
		VOP_UNLOCK(vp, 0);
	else
		VI_UNLOCK(devvp);
	return (error);
}

/*
 * Associate snapshot files when mounting.
 */
void
ffs_snapshot_mount(struct mount *mp)
{
	struct ufsmount *ump = VFSTOUFS(mp);
	struct vnode *devvp = ump->um_devvp;
	struct fs *fs = ump->um_fs;
	struct lwp *l = curlwp;
	struct vnode *vp;
	struct inode *ip, *xp;
	ufs2_daddr_t snaplistsize, *snapblklist;
	int i, error, ns, snaploc, loc;

	ns = UFS_FSNEEDSWAP(fs);
	/*
	 * XXX The following needs to be set before ffs_truncate or
	 * VOP_READ can be called.
	 */
	mp->mnt_stat.f_iosize = fs->fs_bsize;
	/*
	 * Process each snapshot listed in the superblock.
	 */
	vp = NULL;
	for (snaploc = 0; snaploc < FSMAXSNAP; snaploc++) {
		if (fs->fs_snapinum[snaploc] == 0)
			break;
		if ((error = VFS_VGET(mp, fs->fs_snapinum[snaploc],
		    &vp)) != 0) {
			printf("ffs_snapshot_mount: vget failed %d\n", error);
			continue;
		}
		ip = VTOI(vp);
		if ((ip->i_flags & SF_SNAPSHOT) == 0) {
			printf("ffs_snapshot_mount: non-snapshot inode %d\n",
			    fs->fs_snapinum[snaploc]);
			vput(vp);
			vp = NULL;
			for (loc = snaploc + 1; loc < FSMAXSNAP; loc++) {
				if (fs->fs_snapinum[loc] == 0)
					break;
				fs->fs_snapinum[loc - 1] = fs->fs_snapinum[loc];
			}
			fs->fs_snapinum[loc - 1] = 0;
			snaploc--;
			continue;
		}

		/*
		 * Read the block hints list. Use an empty list on
		 * read errors.
		 */
		error = vn_rdwr(UIO_READ, vp,
		    (caddr_t)&snaplistsize, sizeof(snaplistsize),
		    lblktosize(fs, howmany(fs->fs_size, fs->fs_frag)),
		    UIO_SYSSPACE, IO_NODELOCKED|IO_UNIT,
		    l->l_cred, NULL, NULL);
		if (error) {
			printf("ffs_snapshot_mount: read_1 failed %d\n", error);
			snaplistsize = 1;
		} else
			snaplistsize = ufs_rw64(snaplistsize, ns);
		snapblklist = malloc(
		    snaplistsize * sizeof(ufs2_daddr_t), M_UFSMNT, M_WAITOK);
		if (error)
			snapblklist[0] = 1;
		else {
			error = vn_rdwr(UIO_READ, vp, (caddr_t)snapblklist,
			    snaplistsize * sizeof(ufs2_daddr_t),
			    lblktosize(fs, howmany(fs->fs_size, fs->fs_frag)),
			    UIO_SYSSPACE, IO_NODELOCKED|IO_UNIT,
			    l->l_cred, NULL, NULL);
			for (i = 0; i < snaplistsize; i++)
				snapblklist[i] = ufs_rw64(snapblklist[i], ns);
			if (error) {
				printf("ffs_snapshot_mount: read_2 failed %d\n",
				    error);
				snapblklist[0] = 1;
			}
		}
		ip->i_snapblklist = &snapblklist[0];

		/*
		 * If there already exist snapshots on this filesystem, grab a
		 * reference to their shared lock. If this is the first snapshot
		 * on this filesystem, we need to allocate a lock for the
		 * snapshots to share. In either case, acquire the snapshot
		 * lock and give up our original private lock.
		 */
		VI_LOCK(devvp);
		if ((xp = TAILQ_FIRST(&ump->um_snapshots)) != NULL) {
			struct lock *lkp;

			lkp = ITOV(xp)->v_vnlock;
			VI_UNLOCK(devvp);
			VI_LOCK(vp);
			vp->v_vnlock = lkp;
		} else {
			struct lock *lkp;

			VI_UNLOCK(devvp);
			MALLOC(lkp, struct lock *, sizeof(struct lock),
			    M_UFSMNT, M_WAITOK);
			lockinit(lkp, PVFS, "snaplk", 0, LK_CANRECURSE);
			VI_LOCK(vp);
			vp->v_vnlock = lkp;
		}
		vn_lock(vp, LK_INTERLOCK | LK_EXCLUSIVE | LK_RETRY);
		transferlockers(&vp->v_lock, vp->v_vnlock);
		lockmgr(&vp->v_lock, LK_RELEASE, NULL);
		/*
		 * Link it onto the active snapshot list.
		 */
		VI_LOCK(devvp);
		if (ip->i_nextsnap.tqe_prev != 0)
			panic("ffs_snapshot_mount: %llu already on list",
			    (unsigned long long)ip->i_number);
		else
			TAILQ_INSERT_TAIL(&ump->um_snapshots, ip, i_nextsnap);
		vp->v_flag |= VSYSTEM;
		VI_UNLOCK(devvp);
		VOP_UNLOCK(vp, 0);
	}
	/*
	 * No usable snapshots found.
	 */
	if (vp == NULL)
		return;
	/*
	 * Attach the block hints list. We always want to
	 * use the list from the newest snapshot.
	*/
	xp = TAILQ_LAST(&ump->um_snapshots, inodelst);
	VI_LOCK(devvp);
	ump->um_snapblklist = xp->i_snapblklist;
	VI_UNLOCK(devvp);
	vn_cow_establish(devvp, ffs_copyonwrite, devvp);
}

/*
 * Disassociate snapshot files when unmounting.
 */
void
ffs_snapshot_unmount(struct mount *mp)
{
	struct ufsmount *ump = VFSTOUFS(mp);
	struct vnode *devvp = ump->um_devvp;
	struct lock *lkp = NULL;
	struct inode *xp;
	struct vnode *vp;

	VI_LOCK(devvp);
	while ((xp = TAILQ_FIRST(&ump->um_snapshots)) != 0) {
		vp = ITOV(xp);
		lkp = vp->v_vnlock;
		vp->v_vnlock = &vp->v_lock;
		TAILQ_REMOVE(&ump->um_snapshots, xp, i_nextsnap);
		xp->i_nextsnap.tqe_prev = 0;
		if (xp->i_snapblklist == ump->um_snapblklist)
			ump->um_snapblklist = NULL;
		VI_UNLOCK(devvp);
		FREE(xp->i_snapblklist, M_UFSMNT);
		if (xp->i_ffs_effnlink > 0)
			vrele(vp);
		VI_LOCK(devvp);
	}
	VI_UNLOCK(devvp);
	if (lkp != NULL) {
		vn_cow_disestablish(devvp, ffs_copyonwrite, devvp);
		FREE(lkp, M_UFSMNT);
	}
}

/*
 * Check for need to copy block that is about to be written,
 * copying the block if necessary.
 */
static int
ffs_copyonwrite(void *v, struct buf *bp)
{
	struct buf *ibp;
	struct fs *fs;
	struct inode *ip;
	struct vnode *devvp = v, *vp = 0, *saved_vp = NULL;
	struct ufsmount *ump = VFSTOUFS(devvp->v_specmountpoint);
	caddr_t saved_data = NULL;
	ufs2_daddr_t lbn, blkno, *snapblklist;
	int lower, upper, mid, s, ns, indiroff, snapshot_locked = 0, error = 0;

	/*
	 * Check for valid snapshots.
	 */
	VI_LOCK(devvp);
	ip = TAILQ_FIRST(&ump->um_snapshots);
	if (ip == NULL) {
		VI_UNLOCK(devvp);
		return 0;
	}
	/*
	 * First check to see if it is in the preallocated list.
	 * By doing this check we avoid several potential deadlocks.
	 */
	fs = ip->i_fs;
	ns = UFS_FSNEEDSWAP(fs);
	lbn = fragstoblks(fs, dbtofsb(fs, bp->b_blkno));
	snapblklist = ump->um_snapblklist;
	upper = ump->um_snapblklist[0] - 1;
	lower = 1;
	while (lower <= upper) {
		mid = (lower + upper) / 2;
		if (snapblklist[mid] == lbn)
			break;
		if (snapblklist[mid] < lbn)
			lower = mid + 1;
		else
			upper = mid - 1;
	}
	if (lower <= upper) {
		VI_UNLOCK(devvp);
		return 0;
	}
	/*
	 * Not in the precomputed list, so check the snapshots.
	 */
retry:
	TAILQ_FOREACH(ip, &ump->um_snapshots, i_nextsnap) {
		vp = ITOV(ip);
		/*
		 * We ensure that everything of our own that needs to be
		 * copied will be done at the time that ffs_snapshot is
		 * called. Thus we can skip the check here which can
		 * deadlock in doing the lookup in ffs_balloc.
		 */
		if (bp->b_vp == vp)
			continue;
		/*
		 * Check to see if block needs to be copied. We do not have
		 * to hold the snapshot lock while doing this lookup as it
		 * will never require any additional allocations for the
		 * snapshot inode.
		 */
		if (lbn < NDADDR) {
			blkno = db_get(ip, lbn);
		} else {
			if (snapshot_locked == 0 &&
			    lockmgr(vp->v_vnlock,
			      LK_INTERLOCK | LK_EXCLUSIVE | LK_SLEEPFAIL,
			      VI_MTX(devvp)) != 0) {
				VI_LOCK(devvp);
				goto retry;
			}
			snapshot_locked = 1;
			s = cow_enter();
			error = ffs_balloc(vp, lblktosize(fs, (off_t)lbn),
			   fs->fs_bsize, KERNCRED, B_METAONLY, &ibp);
			cow_leave(s);
			if (error)
				break;
			indiroff = (lbn - NDADDR) % NINDIR(fs);
			blkno = idb_get(ip, ibp->b_data, indiroff);
			brelse(ibp);
		}
#ifdef DIAGNOSTIC
		if (blkno == BLK_SNAP && bp->b_lblkno >= 0)
			panic("ffs_copyonwrite: bad copy block");
#endif
		if (blkno != 0)
			continue;
#ifdef DIAGNOSTIC
		if (curlwp->l_flag & L_COWINPROGRESS)
			printf("ffs_copyonwrite: recursive call\n");
#endif
		/*
		 * Allocate the block into which to do the copy. Since
		 * multiple processes may all try to copy the same block,
		 * we have to recheck our need to do a copy if we sleep
		 * waiting for the lock.
		 *
		 * Because all snapshots on a filesystem share a single
		 * lock, we ensure that we will never be in competition
		 * with another process to allocate a block.
		 */
		if (snapshot_locked == 0 &&
		    lockmgr(vp->v_vnlock,
		      LK_INTERLOCK | LK_EXCLUSIVE | LK_SLEEPFAIL,
		      VI_MTX(devvp)) != 0) {
			VI_LOCK(devvp);
			goto retry;
		}
		snapshot_locked = 1;
#ifdef DEBUG
		if (snapdebug) {
			printf("Copyonwrite: snapino %llu lbn %" PRId64 " for ",
			    (unsigned long long)ip->i_number, lbn);
			if (bp->b_vp == devvp)
				printf("fs metadata");
			else
				printf("inum %llu", (unsigned long long)
				    VTOI(bp->b_vp)->i_number);
			printf(" lblkno %" PRId64 "\n", bp->b_lblkno);
		}
#endif
		/*
		 * If we have already read the old block contents, then
		 * simply copy them to the new block. Note that we need
		 * to synchronously write snapshots that have not been
		 * unlinked, and hence will be visible after a crash,
		 * to ensure their integrity.
		 */
		if (saved_data) {
			error = writevnblk(vp, saved_data, lbn);
			if (error)
				break;
			continue;
		}
		/*
		 * Otherwise, read the old block contents into the buffer.
		 */
		saved_data = malloc(fs->fs_bsize, M_UFSMNT, M_WAITOK);
		saved_vp = vp;
		if ((error = readfsblk(vp, saved_data, lbn)) != 0) {
			free(saved_data, M_UFSMNT);
			saved_data = NULL;
			break;
		}
	}
	/*
	 * Note that we need to synchronously write snapshots that
	 * have not been unlinked, and hence will be visible after
	 * a crash, to ensure their integrity.
	 */
	if (saved_data) {
		error = writevnblk(saved_vp, saved_data, lbn);
		free(saved_data, M_UFSMNT);
	}
	if (snapshot_locked)
		VOP_UNLOCK(vp, 0);
	else
		VI_UNLOCK(devvp);
	return error;
}

/*
 * Read the specified block from disk. Vp is usually a snapshot vnode.
 */
static int
readfsblk(struct vnode *vp, caddr_t data, ufs2_daddr_t lbn)
{
	int error;
	struct inode *ip = VTOI(vp);
	struct fs *fs = ip->i_fs;
	struct buf *nbp;

	nbp = getiobuf();
	nbp->b_flags = B_READ;
	nbp->b_bcount = nbp->b_bufsize = fs->fs_bsize;
	nbp->b_error = 0;
	nbp->b_data = data;
	nbp->b_blkno = nbp->b_rawblkno = fsbtodb(fs, blkstofrags(fs, lbn));
	nbp->b_proc = NULL;
	nbp->b_dev = ip->i_devvp->v_rdev;
	nbp->b_vp = NULLVP;

	DEV_STRATEGY(nbp);

	error = biowait(nbp);

	putiobuf(nbp);

	return error;
}

/*
 * Read the specified block. Bypass UBC to prevent deadlocks.
 */
static int
readvnblk(struct vnode *vp, caddr_t data, ufs2_daddr_t lbn)
{
	int error;
	daddr_t bn;
	off_t offset;
	struct inode *ip = VTOI(vp);
	struct fs *fs = ip->i_fs;

	error = VOP_BMAP(vp, lbn, NULL, &bn, NULL);
	if (error)
		return error;

	if (bn != (daddr_t)-1) {
		offset = dbtob(bn);
		simple_lock(&vp->v_interlock);
		error = VOP_PUTPAGES(vp, trunc_page(offset),
		    round_page(offset+fs->fs_bsize),
		    PGO_CLEANIT|PGO_SYNCIO|PGO_FREE);
		if (error)
			return error;

		return readfsblk(vp, data, fragstoblks(fs, dbtofsb(fs, bn)));
	}

	bzero(data, fs->fs_bsize);

	return 0;
}

/*
 * Write the specified block. Bypass UBC to prevent deadlocks.
 */
static int
writevnblk(struct vnode *vp, caddr_t data, ufs2_daddr_t lbn)
{
	int s, error;
	off_t offset;
	struct buf *bp;
	struct inode *ip = VTOI(vp);
	struct fs *fs = ip->i_fs;

	offset = lblktosize(fs, (off_t)lbn);
	s = cow_enter();
	simple_lock(&vp->v_interlock);
	error = VOP_PUTPAGES(vp, trunc_page(offset),
	    round_page(offset+fs->fs_bsize), PGO_CLEANIT|PGO_SYNCIO|PGO_FREE);
	if (error == 0)
		error = ffs_balloc(vp, lblktosize(fs, (off_t)lbn),
		    fs->fs_bsize, KERNCRED, B_SYNC, &bp);
	cow_leave(s);
	if (error)
		return error;

	bcopy(data, bp->b_data, fs->fs_bsize);
	bp->b_flags |= B_NOCACHE;

	return bwrite(bp);
}

/*
 * Set/reset lwp's L_COWINPROGRESS flag.
 * May be called recursive.
 */
static inline int
cow_enter(void)
{
	struct lwp *l = curlwp;

	if (l->l_flag & L_COWINPROGRESS) {
		return 0;
	} else {
		l->l_flag |= L_COWINPROGRESS;
		return L_COWINPROGRESS;
	}
}

static inline void
cow_leave(int flag)
{
	struct lwp *l = curlwp;

	l->l_flag &= ~flag;
}

/*
 * Get/Put direct block from inode or buffer containing disk addresses. Take
 * care for fs type (UFS1/UFS2) and byte swapping. These functions should go
 * into a global include.
 */
static inline ufs2_daddr_t
db_get(struct inode *ip, int loc)
{
	if (ip->i_ump->um_fstype == UFS1)
		return ufs_rw32(ip->i_ffs1_db[loc], UFS_IPNEEDSWAP(ip));
	else
		return ufs_rw64(ip->i_ffs2_db[loc], UFS_IPNEEDSWAP(ip));
}

static inline void
db_assign(struct inode *ip, int loc, ufs2_daddr_t val)
{
	if (ip->i_ump->um_fstype == UFS1)
		ip->i_ffs1_db[loc] = ufs_rw32(val, UFS_IPNEEDSWAP(ip));
	else
		ip->i_ffs2_db[loc] = ufs_rw64(val, UFS_IPNEEDSWAP(ip));
}

static inline ufs2_daddr_t
idb_get(struct inode *ip, caddr_t bf, int loc)
{
	if (ip->i_ump->um_fstype == UFS1)
		return ufs_rw32(((ufs1_daddr_t *)(bf))[loc],
		    UFS_IPNEEDSWAP(ip));
	else
		return ufs_rw64(((ufs2_daddr_t *)(bf))[loc],
		    UFS_IPNEEDSWAP(ip));
}

static inline void
idb_assign(struct inode *ip, caddr_t bf, int loc, ufs2_daddr_t val)
{
	if (ip->i_ump->um_fstype == UFS1)
		((ufs1_daddr_t *)(bf))[loc] =
		    ufs_rw32(val, UFS_IPNEEDSWAP(ip));
	else
		((ufs2_daddr_t *)(bf))[loc] =
		    ufs_rw64(val, UFS_IPNEEDSWAP(ip));
}
