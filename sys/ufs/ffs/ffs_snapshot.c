/*	$NetBSD: ffs_snapshot.c,v 1.55.2.2 2007/12/26 21:39:59 ad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ffs_snapshot.c,v 1.55.2.2 2007/12/26 21:39:59 ad Exp $");

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
#include <sys/fstrans.h>

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
#define VI_LOCK(v)	mutex_enter(&(v)->v_interlock)
#define VI_UNLOCK(v)	mutex_exit(&(v)->v_interlock)
#define MNT_ILOCK(v)	mutex_enter(&mntvnode_lock)
#define MNT_IUNLOCK(v)	mutex_exit(&mntvnode_lock)

#if !defined(FFS_NO_SNAPSHOT)
static int cgaccount(int, struct vnode *, void *, int);
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
static int readvnblk(struct vnode *, void *, ufs2_daddr_t);
#endif /* !defined(FFS_NO_SNAPSHOT) */

static void si_mount_dtor(void *);
static struct snap_info *si_mount_init(struct mount *);
static int ffs_copyonwrite(void *, struct buf *, bool);
static int readfsblk(struct vnode *, void *, ufs2_daddr_t);
static int writevnblk(struct vnode *, void *, ufs2_daddr_t);
static inline int cow_enter(void);
static inline void cow_leave(int);
static inline ufs2_daddr_t db_get(struct inode *, int);
static inline void db_assign(struct inode *, int, ufs2_daddr_t);
static inline ufs2_daddr_t idb_get(struct inode *, void *, int);
static inline void idb_assign(struct inode *, void *, int, ufs2_daddr_t);

struct snap_info {
	kmutex_t si_lock;			/* Lock this snapinfo */
	struct lock si_vnlock;			/* Snapshot vnode common lock */
	TAILQ_HEAD(inodelst, inode) si_snapshots; /* List of active snapshots */
	daddr_t *si_snapblklist;		/* Snapshot block hints list */
	uint32_t si_gen;			/* Incremented on change */
};

#ifdef DEBUG
static int snapdebug = 0;
#endif
static kmutex_t si_mount_init_lock;
static specificdata_key_t si_mount_data_key;

void
ffs_snapshot_init(void)
{
	int error;

	error = mount_specific_key_create(&si_mount_data_key, si_mount_dtor);
	KASSERT(error == 0);
	mutex_init(&si_mount_init_lock, MUTEX_DEFAULT, IPL_NONE);
}

void
ffs_snapshot_fini(void)
{
	mount_specific_key_delete(si_mount_data_key);
	mutex_destroy(&si_mount_init_lock);
}

static void
si_mount_dtor(void *arg)
{
	struct snap_info *si = arg;

	KASSERT(TAILQ_EMPTY(&si->si_snapshots));
	mutex_destroy(&si->si_lock);
	KASSERT(si->si_snapblklist == NULL);
	free(si, M_MOUNT);
}

static struct snap_info *
si_mount_init(struct mount *mp)
{
	struct snap_info *new;

	mutex_enter(&si_mount_init_lock);

	if ((new = mount_getspecific(mp, si_mount_data_key)) != NULL) {
		mutex_exit(&si_mount_init_lock);
		return new;
	}

	new = malloc(sizeof(*new), M_MOUNT, M_WAITOK);
	TAILQ_INIT(&new->si_snapshots);
	mutex_init(&new->si_lock, MUTEX_DEFAULT, IPL_NONE);
	new->si_gen = 0;
	new->si_snapblklist = NULL;
	mount_setspecific(mp, si_mount_data_key, new);
	mutex_exit(&si_mount_init_lock);
	return new;
}

/*
 * Create a snapshot file and initialize it for the filesystem.
 * Vnode is locked on entry and return.
 */
int
ffs_snapshot(struct mount *mp, struct vnode *vp,
    struct timespec *ctime)
{
#if defined(FFS_NO_SNAPSHOT)
	return EOPNOTSUPP;
}
#else /* defined(FFS_NO_SNAPSHOT) */
	ufs2_daddr_t numblks, blkno, *blkp, snaplistsize = 0, *snapblklist;
	int error, ns, cg, snaploc;
	int i, size, len, loc;
	int flag = mp->mnt_flag;
	struct timeval starttime;
#ifdef DEBUG
	struct timeval endtime;
#endif
	struct timespec ts;
	long redo = 0;
	int32_t *lp;
	void *space;
	void *sbbuf = NULL;
	struct fs *copy_fs = NULL, *fs = VFSTOUFS(mp)->um_fs;
	struct lwp *l = curlwp;
	struct inode *ip, *xp;
	struct buf *bp, *ibp, *nbp;
	struct vattr vat;
	struct vnode *xvp, *mvp, *devvp;
	struct snap_info *si;

	ns = UFS_FSNEEDSWAP(fs);
	if ((si = mount_getspecific(mp, si_mount_data_key)) == NULL)
		si = si_mount_init(mp);
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
	    NULL) != 0 &&
	    VTOI(vp)->i_uid != kauth_cred_geteuid(l->l_cred))
		return EACCES;

	if (vp->v_size != 0) {
		error = ffs_truncate(vp, 0, 0, NOCRED);
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
	    (void *)&blkno, sizeof(blkno), lblktosize(fs, (off_t)numblks),
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
	if ((error = VOP_FSYNC(vp, KERNCRED, FSYNC_WAIT, 0, 0)) != 0)
		goto out;
	VOP_UNLOCK(vp, 0);
	/*
	 * All allocations are done, so we can now snapshot the system.
	 *
	 * Suspend operation on filesystem.
	 */
	if ((error = vfs_suspend(vp->v_mount, 0)) != 0) {
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
		memset(sbbuf, 0, loc);
	copy_fs = (struct fs *)((char *)sbbuf + loc);
	bcopy(fs, copy_fs, fs->fs_sbsize);
	size = fs->fs_bsize < SBLOCKSIZE ? fs->fs_bsize : SBLOCKSIZE;
	if (fs->fs_sbsize < size)
		memset((char *)sbbuf + loc + fs->fs_sbsize, 0, 
		    size - fs->fs_sbsize);
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
			brelse(bp, 0);
			free(copy_fs->fs_csp, M_UFSMNT);
			goto out1;
		}
		bcopy(bp->b_data, space, (u_int)len);
		space = (char *)space + len;
		brelse(bp, BC_INVAL | BC_NOCACHE);
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
	/* Allocate a marker vnode */
	if ((mvp = valloc(mp)) == NULL) {
		error = ENOMEM;
		goto out1;
	}
	MNT_ILOCK(mp);
	/*
	 * NOTE: not using the TAILQ_FOREACH here since in this loop vgone()
	 * and vclean() can be called indirectly
	 */
	for (xvp = TAILQ_FIRST(&mp->mnt_vnodelist); xvp; xvp = vunmark(mvp)) {
		vmark(mvp, vp);
		/*
		 * Make sure this vnode wasn't reclaimed in getnewvnode().
		 * Start over if it has (it won't be on the list anymore).
		 */
		if (xvp->v_mount != mp || vismarker(xvp))
			continue;
		VI_LOCK(xvp);
		if ((xvp->v_iflag & VI_XLOCK) ||
		    xvp->v_usecount == 0 || xvp->v_type == VNON ||
		    (VTOI(xvp)->i_flags & SF_SNAPSHOT)) {
			VI_UNLOCK(xvp);
			continue;
		}
		MNT_IUNLOCK(mp);
		/*
		 * XXXAD should increase vnode ref count to prevent it
		 * disappearing or being recycled.
		 */
		VI_UNLOCK(xvp);
#ifdef DEBUG
		if (snapdebug)
			vprint("ffs_snapshot: busy vnode", xvp);
#endif
		if (VOP_GETATTR(xvp, &vat, l->l_cred) == 0 &&
		    vat.va_nlink > 0) {
			MNT_ILOCK(mp);
			continue;
		}
		xp = VTOI(xvp);
		if (ffs_checkfreefile(copy_fs, vp, xp->i_number)) {
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
		if (error) {
			free(copy_fs->fs_csp, M_UFSMNT);
			(void)vunmark(mvp);
			goto out1;
		}
		MNT_ILOCK(mp);
	}
	MNT_IUNLOCK(mp);
	vfree(mvp);
	/*
	 * If there already exist snapshots on this filesystem, grab a
	 * reference to their shared lock. If this is the first snapshot
	 * on this filesystem, we need to allocate a lock for the snapshots
	 * to share. In either case, acquire the snapshot lock and give
	 * up our original private lock.
	 */
	mutex_enter(&si->si_lock);
	if ((xp = TAILQ_FIRST(&si->si_snapshots)) != NULL) {
		VI_LOCK(vp);
		vp->v_vnlock = ITOV(xp)->v_vnlock;
	} else {
		lockinit(&si->si_vnlock, PVFS, "snaplk", 0, LK_CANRECURSE);
		VI_LOCK(vp);
		vp->v_vnlock = &si->si_vnlock;
	}
	mutex_exit(&si->si_lock);
	vn_lock(vp, LK_INTERLOCK | LK_EXCLUSIVE | LK_RETRY);
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
		mutex_enter(&si->si_lock);
		if (si->si_snapblklist != NULL)
			panic("ffs_snapshot: non-empty list");
		si->si_snapblklist = snapblklist;
	} else
		mutex_enter(&si->si_lock);
	/*
	 * Record snapshot inode. Since this is the newest snapshot,
	 * it must be placed at the end of the list.
	 */
	fs->fs_snapinum[snaploc] = ip->i_number;
	if (ip->i_nextsnap.tqe_prev != 0)
		panic("ffs_snapshot: %llu already on list",
		    (unsigned long long)ip->i_number);
	TAILQ_INSERT_TAIL(&si->si_snapshots, ip, i_nextsnap);
	if (xp == NULL)
		fscow_establish(mp, ffs_copyonwrite, devvp);
	si->si_gen++;
	mutex_exit(&si->si_lock);
	vp->v_vflag |= VV_SYSTEM;
out1:
	/*
	 * Resume operation on filesystem.
	 */
	vfs_resume(vp->v_mount);
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
	TAILQ_FOREACH(xp, &si->si_snapshots, i_nextsnap) {
		if (xp == ip)
			break;
		if (xp->i_ump->um_fstype == UFS1)
			error = expunge_ufs1(vp, xp, fs, snapacct_ufs1,
			    BLK_SNAP);
		else
			error = expunge_ufs2(vp, xp, fs, snapacct_ufs2,
			    BLK_SNAP);
		if (error == 0 && xp->i_ffs_effnlink == 0)
			error = ffs_freefile(copy_fs, vp,
			    xp->i_number, xp->i_mode);
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
	error = vn_rdwr(UIO_WRITE, vp, (void *)snapblklist,
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
			brelse(nbp, 0);
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
	mutex_enter(&si->si_lock);
	space = si->si_snapblklist;
	si->si_snapblklist = snapblklist;
	if (TAILQ_FIRST(&si->si_snapshots) == ip)
		FREE(space, M_UFSMNT);
	si->si_gen++;
	mutex_exit(&si->si_lock);
done:
	free(copy_fs->fs_csp, M_UFSMNT);
	if (!error) {
		error = bread(vp, lblkno(fs, fs->fs_sblockloc), fs->fs_bsize,
		    KERNCRED, &nbp);
		if (error) {
			brelse(nbp, 0);
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
		mutex_enter(&vp->v_interlock);
		error = VOP_PUTPAGES(vp, 0, 0,
		    PGO_ALLPAGES|PGO_CLEANIT|PGO_SYNCIO|PGO_FREE);
	}
	if (!error) {
		mutex_enter(&bufcache_lock);
		for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
			nbp = LIST_NEXT(bp, b_vnbufs);
			bp->b_cflags |= BC_BUSY|BC_VFLUSH;
			if (LIST_FIRST(&bp->b_dep) == NULL)
				bp->b_cflags |= BC_NOCACHE;
			mutex_exit(&bufcache_lock);
			bwrite(bp);
			mutex_enter(&bufcache_lock);
		}
		mutex_exit(&bufcache_lock);

		mutex_enter(&vp->v_interlock);
		while (vp->v_numoutput > 0)
			cv_wait(&vp->v_cv, &vp->v_interlock);
		mutex_exit(&vp->v_interlock);
	}
	if (sbbuf)
		free(sbbuf, M_UFSMNT);
	if (fs->fs_active != 0) {
		FREE(fs->fs_active, M_DEVBUF);
		fs->fs_active = 0;
	}
	mp->mnt_flag = flag;
	if (error)
		(void) ffs_truncate(vp, (off_t)0, 0, NOCRED);
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
cgaccount(int cg, struct vnode *vp, void *data, int passno)
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
		brelse(bp, 0);
		return (error);
	}
	cgp = (struct cg *)bp->b_data;
	if (!cg_chkmagic(cgp, ns)) {
		brelse(bp, 0);
		return (EIO);
	}
	ACTIVECG_SET(fs, cg);

	bcopy(bp->b_data, data, fs->fs_cgsize);
	brelse(bp, 0);
	if (fs->fs_cgsize < fs->fs_bsize)
		memset((char *)data + fs->fs_cgsize, 0,
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
	void *bf;

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
		brelse(bp, 0);
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
	 * or unlinked snapshots to be completely unallocated.
	 */
	dip = (struct ufs1_dinode *)bf + ino_to_fsbo(fs, cancelip->i_number);
	if (expungetype == BLK_NOCOPY || cancelip->i_ffs_effnlink == 0)
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
	if ((bp->b_oflags & (BO_DONE | BO_DELWRI)) == 0 &&
	    (error = readfsblk(bp->b_vp, bp->b_data, fragstoblks(fs, blkno)))) {
		brelse(bp, 0);
		return (error);
	}
	/*
	 * Account for the block pointers in this indirect block.
	 */
	last = howmany(remblks, blksperindir);
	if (last > NINDIR(fs))
		last = NINDIR(fs);
	bap = malloc(fs->fs_bsize, M_DEVBUF, M_WAITOK);
	bcopy(bp->b_data, (void *)bap, fs->fs_bsize);
	brelse(bp, 0);
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
    struct fs *fs, ufs_lbn_t lblkno,
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
				brelse(ibp, 0);
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
	void *bf;

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
		brelse(bp, 0);
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
	 * or unlinked snapshots to be completely unallocated.
	 */
	dip = (struct ufs2_dinode *)bf + ino_to_fsbo(fs, cancelip->i_number);
	if (expungetype == BLK_NOCOPY || cancelip->i_ffs_effnlink == 0)
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
	if ((bp->b_oflags & (BO_DONE | BO_DELWRI)) == 0 &&
	    (error = readfsblk(bp->b_vp, bp->b_data, fragstoblks(fs, blkno)))) {
		brelse(bp, 0);
		return (error);
	}
	/*
	 * Account for the block pointers in this indirect block.
	 */
	last = howmany(remblks, blksperindir);
	if (last > NINDIR(fs))
		last = NINDIR(fs);
	bap = malloc(fs->fs_bsize, M_DEVBUF, M_WAITOK);
	bcopy(bp->b_data, (void *)bap, fs->fs_bsize);
	brelse(bp, 0);
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
    struct fs *fs, ufs_lbn_t lblkno,
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
				brelse(ibp, 0);
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
	struct mount *mp = ip->i_devvp->v_specmountpoint;
	struct inode *xp;
	struct fs *fs;
	struct snap_info *si;
	int snaploc;

	if ((si = mount_getspecific(mp, si_mount_data_key)) == NULL)
		return;
	/*
	 * Find snapshot in incore list.
	 */
	mutex_enter(&si->si_lock);
	TAILQ_FOREACH(xp, &si->si_snapshots, i_nextsnap)
		if (xp == ip)
			break;
	mutex_exit(&si->si_lock);
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
	mutex_enter(&si->si_lock);
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
	si->si_gen++;
	mutex_exit(&si->si_lock);
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
	struct mount *mp = devvp->v_specmountpoint;
	struct lock *lkp;
	struct buf *ibp;
	struct snap_info *si;
	ufs2_daddr_t numblks, blkno, dblk;
	int error, ns, loc, last;

	if ((si = mount_getspecific(mp, si_mount_data_key)) == NULL)
		return;
	ns = UFS_FSNEEDSWAP(fs);
	/*
	 * If active, delete from incore list (this snapshot may
	 * already have been in the process of being deleted, so
	 * would not have been active).
	 *
	 * Clear copy-on-write flag if last snapshot.
	 */
	if (ip->i_nextsnap.tqe_prev != 0) {
		mutex_enter(&si->si_lock);
		lockmgr(&vp->v_lock, LK_EXCLUSIVE, NULL);
		TAILQ_REMOVE(&si->si_snapshots, ip, i_nextsnap);
		ip->i_nextsnap.tqe_prev = 0;
		VI_LOCK(vp);
		lkp = vp->v_vnlock;
		vp->v_vnlock = &vp->v_lock;
		lockmgr(lkp, LK_RELEASE | LK_INTERLOCK, VI_MTX(vp));
		if (TAILQ_FIRST(&si->si_snapshots) != 0) {
			/* Roll back the list of preallocated blocks. */
			xp = TAILQ_LAST(&si->si_snapshots, inodelst);
			si->si_snapblklist = xp->i_snapblklist;
		} else {
			si->si_snapblklist = 0;
			si->si_gen++;
			mutex_exit(&si->si_lock);
			lockmgr(lkp, LK_DRAIN, NULL);
			lockmgr(lkp, LK_RELEASE, NULL);
			fscow_disestablish(mp, ffs_copyonwrite, devvp);
			mutex_enter(&si->si_lock);
		}
		si->si_gen++;
		mutex_exit(&si->si_lock);
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
    long size, ino_t inum)
{
	struct mount *mp = devvp->v_specmountpoint;
	struct buf *ibp;
	struct inode *ip;
	struct vnode *vp = NULL;
	struct snap_info *si;
	void *saved_data = NULL;
	ufs_lbn_t lbn;
	ufs2_daddr_t blkno;
	uint32_t gen;
	int s, indiroff = 0, snapshot_locked = 0, error = 0, claimedblk = 0;

	if ((si = mount_getspecific(mp, si_mount_data_key)) == NULL)
		return 0;
	lbn = fragstoblks(fs, bno);
	mutex_enter(&si->si_lock);
retry:
	gen = si->si_gen;
	TAILQ_FOREACH(ip, &si->si_snapshots, i_nextsnap) {
		vp = ITOV(ip);
		if (snapshot_locked == 0) {
			mutex_exit(&si->si_lock);
			if (VOP_LOCK(vp, LK_EXCLUSIVE | LK_SLEEPFAIL) != 0) {
				mutex_enter(&si->si_lock);
				goto retry;
			}
			mutex_enter(&si->si_lock);
			snapshot_locked = 1;
			if (gen != si->si_gen)
				goto retry;
		}
		/*
		 * Lookup block being written.
		 */
		if (lbn < NDADDR) {
			blkno = db_get(ip, lbn);
		} else {
			mutex_exit(&si->si_lock);
			s = cow_enter();
			error = ffs_balloc(vp, lblktosize(fs, (off_t)lbn),
			    fs->fs_bsize, KERNCRED, B_METAONLY, &ibp);
			cow_leave(s);
			if (error) {
				mutex_enter(&si->si_lock);
				break;
			}
			indiroff = (lbn - NDADDR) % NINDIR(fs);
			blkno = idb_get(ip, ibp->b_data, indiroff);
			mutex_enter(&si->si_lock);
			if (gen != si->si_gen) {
				brelse(ibp, 0);
				goto retry;
			}
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
			if (lbn < NDADDR) {
				db_assign(ip, lbn, BLK_NOCOPY);
				ip->i_flag |= IN_CHANGE | IN_UPDATE;
			} else {
				idb_assign(ip, ibp->b_data, indiroff,
				    BLK_NOCOPY);
				mutex_exit(&si->si_lock);
				bwrite(ibp);
				mutex_enter(&si->si_lock);
				if (gen != si->si_gen)
					goto retry;
			}
			continue;
		} else /* BLK_NOCOPY or default */ {
			/*
			 * If the snapshot has already copied the block
			 * (default), or does not care about the block,
			 * it is not needed.
			 */
			if (lbn >= NDADDR)
				brelse(ibp, 0);
			continue;
		}
		/*
		 * If this is a full size block, we will just grab it
		 * and assign it to the snapshot inode. Otherwise we
		 * will proceed to copy it. See explanation for this
		 * routine as to why only a single snapshot needs to
		 * claim this block.
		 */
		if (size == fs->fs_bsize) {
#ifdef DEBUG
			if (snapdebug)
				printf("%s %llu lbn %" PRId64
				    "from inum %llu\n",
				    "Grabonremove: snapino",
				    (unsigned long long)ip->i_number,
				    lbn, (unsigned long long)inum);
#endif
			mutex_exit(&si->si_lock);
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
			brelse(ibp, 0);
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
		mutex_exit(&si->si_lock);
		if (saved_data == NULL) {
			saved_data = malloc(fs->fs_bsize, M_UFSMNT, M_WAITOK);
			if ((error = readfsblk(vp, saved_data, lbn)) != 0) {
				free(saved_data, M_UFSMNT);
				saved_data = NULL;
				mutex_enter(&si->si_lock);
				break;
			}
		}
		error = writevnblk(vp, saved_data, lbn);
		mutex_enter(&si->si_lock);
		if (error)
			break;
		if (gen != si->si_gen)
			goto retry;
	}
	mutex_exit(&si->si_lock);
	if (saved_data)
		free(saved_data, M_UFSMNT);
	/*
	 * If we have been unable to allocate a block in which to do
	 * the copy, then return non-zero so that the fragment will
	 * not be freed. Although space will be lost, the snapshot
	 * will stay consistent.
	 */
	if (snapshot_locked)
		VOP_UNLOCK(vp, 0);
	return (error);
}

/*
 * Associate snapshot files when mounting.
 */
void
ffs_snapshot_mount(struct mount *mp)
{
	struct vnode *devvp = VFSTOUFS(mp)->um_devvp;
	struct fs *fs = VFSTOUFS(mp)->um_fs;
	struct lwp *l = curlwp;
	struct vnode *vp;
	struct inode *ip, *xp;
	struct snap_info *si;
	ufs2_daddr_t snaplistsize, *snapblklist;
	int i, error, ns, snaploc, loc;

	/*
	 * No persistent snapshots on apple ufs file systems.
	 */
	if (UFS_MPISAPPLEUFS(VFSTOUFS(mp)))
		return;

	if ((si = mount_getspecific(mp, si_mount_data_key)) == NULL)
		si = si_mount_init(mp);
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
	mutex_enter(&si->si_lock);
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
		    (void *)&snaplistsize, sizeof(snaplistsize),
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
			error = vn_rdwr(UIO_READ, vp, (void *)snapblklist,
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
		if ((xp = TAILQ_FIRST(&si->si_snapshots)) != NULL) {
			VI_LOCK(vp);
			vp->v_vnlock = ITOV(xp)->v_vnlock;
		} else {
			lockinit(&si->si_vnlock, PVFS, "snaplk",
			    0, LK_CANRECURSE);
			VI_LOCK(vp);
			vp->v_vnlock = &si->si_vnlock;
		}
		vn_lock(vp, LK_INTERLOCK | LK_EXCLUSIVE | LK_RETRY);
		lockmgr(&vp->v_lock, LK_RELEASE, NULL);
		/*
		 * Link it onto the active snapshot list.
		 */
		if (ip->i_nextsnap.tqe_prev != 0)
			panic("ffs_snapshot_mount: %llu already on list",
			    (unsigned long long)ip->i_number);
		else
			TAILQ_INSERT_TAIL(&si->si_snapshots, ip, i_nextsnap);
		vp->v_vflag |= VV_SYSTEM;
		VOP_UNLOCK(vp, 0);
	}
	/*
	 * No usable snapshots found.
	 */
	if (vp == NULL) {
		mutex_exit(&si->si_lock);
		return;
	}
	/*
	 * Attach the block hints list. We always want to
	 * use the list from the newest snapshot.
	*/
	xp = TAILQ_LAST(&si->si_snapshots, inodelst);
	si->si_snapblklist = xp->i_snapblklist;
	fscow_establish(mp, ffs_copyonwrite, devvp);
	si->si_gen++;
	mutex_exit(&si->si_lock);
}

/*
 * Disassociate snapshot files when unmounting.
 */
void
ffs_snapshot_unmount(struct mount *mp)
{
	struct vnode *devvp = VFSTOUFS(mp)->um_devvp;
	struct inode *xp;
	struct vnode *vp = NULL;
	struct snap_info *si;

	if ((si = mount_getspecific(mp, si_mount_data_key)) == NULL)
		return;
	mutex_enter(&si->si_lock);
	while ((xp = TAILQ_FIRST(&si->si_snapshots)) != 0) {
		vp = ITOV(xp);
		vp->v_vnlock = &vp->v_lock;
		TAILQ_REMOVE(&si->si_snapshots, xp, i_nextsnap);
		xp->i_nextsnap.tqe_prev = 0;
		if (xp->i_snapblklist == si->si_snapblklist)
			si->si_snapblklist = NULL;
		FREE(xp->i_snapblklist, M_UFSMNT);
		if (xp->i_ffs_effnlink > 0) {
			si->si_gen++;
			mutex_exit(&si->si_lock);
			vrele(vp);
			mutex_enter(&si->si_lock);
		}
	}
	if (vp)
		fscow_disestablish(mp, ffs_copyonwrite, devvp);
	si->si_gen++;
	mutex_exit(&si->si_lock);
}

/*
 * Check for need to copy block that is about to be written,
 * copying the block if necessary.
 */
static int
ffs_copyonwrite(void *v, struct buf *bp, bool data_valid)
{
	struct buf *ibp;
	struct fs *fs;
	struct inode *ip;
	struct vnode *devvp = v, *vp = NULL;
	struct mount *mp = devvp->v_specmountpoint;
	struct snap_info *si;
	void *saved_data = NULL;
	ufs2_daddr_t lbn, blkno, *snapblklist;
	uint32_t gen;
	int lower, upper, mid, s, ns, indiroff, snapshot_locked = 0, error = 0;

	/*
	 * Check for valid snapshots.
	 */
	if ((si = mount_getspecific(mp, si_mount_data_key)) == NULL)
		return 0;
	mutex_enter(&si->si_lock);
	ip = TAILQ_FIRST(&si->si_snapshots);
	if (ip == NULL) {
		mutex_exit(&si->si_lock);
		return 0;
	}
	/*
	 * First check to see if it is in the preallocated list.
	 * By doing this check we avoid several potential deadlocks.
	 */
	fs = ip->i_fs;
	ns = UFS_FSNEEDSWAP(fs);
	lbn = fragstoblks(fs, dbtofsb(fs, bp->b_blkno));
	snapblklist = si->si_snapblklist;
	upper = si->si_snapblklist[0] - 1;
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
		mutex_exit(&si->si_lock);
		return 0;
	}
	/*
	 * Not in the precomputed list, so check the snapshots.
	 */
	 if (data_valid && bp->b_bcount == fs->fs_bsize)
		saved_data = bp->b_data;
retry:
	gen = si->si_gen;
	TAILQ_FOREACH(ip, &si->si_snapshots, i_nextsnap) {
		vp = ITOV(ip);
		/*
		 * We ensure that everything of our own that needs to be
		 * copied will be done at the time that ffs_snapshot is
		 * called. Thus we can skip the check here which can
		 * deadlock in doing the lookup in ffs_balloc.
		 */
		if (bp->b_vp == vp)
			continue;
		if (snapshot_locked == 0) {
			mutex_exit(&si->si_lock);
			if (VOP_LOCK(vp, LK_EXCLUSIVE | LK_SLEEPFAIL) != 0) {
				mutex_enter(&si->si_lock);
				goto retry;
			}
			mutex_enter(&si->si_lock);
			snapshot_locked = 1;
			if (gen != si->si_gen)
				goto retry;
		}
		/*
		 * Check to see if block needs to be copied. We do not have
		 * to hold the snapshot lock while doing this lookup as it
		 * will never require any additional allocations for the
		 * snapshot inode.
		 */
		if (lbn < NDADDR) {
			blkno = db_get(ip, lbn);
		} else {
			mutex_exit(&si->si_lock);
			s = cow_enter();
			error = ffs_balloc(vp, lblktosize(fs, (off_t)lbn),
			   fs->fs_bsize, KERNCRED, B_METAONLY, &ibp);
			cow_leave(s);
			if (error) {
				mutex_enter(&si->si_lock);
				break;
			}
			indiroff = (lbn - NDADDR) % NINDIR(fs);
			blkno = idb_get(ip, ibp->b_data, indiroff);
			brelse(ibp, 0);
			mutex_enter(&si->si_lock);
			if (gen != si->si_gen)
				goto retry;
		}
#ifdef DIAGNOSTIC
		if (blkno == BLK_SNAP && bp->b_lblkno >= 0)
			panic("ffs_copyonwrite: bad copy block");
#endif
		if (blkno != 0)
			continue;
#ifdef DIAGNOSTIC
		if (curlwp->l_pflag & LP_UFSCOW)
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
		mutex_exit(&si->si_lock);
		if (saved_data == NULL) {
			saved_data = malloc(fs->fs_bsize, M_UFSMNT, M_WAITOK);
			if ((error = readfsblk(vp, saved_data, lbn)) != 0) {
				free(saved_data, M_UFSMNT);
				saved_data = NULL;
				mutex_enter(&si->si_lock);
				break;
			}
		}
		error = writevnblk(vp, saved_data, lbn);
		mutex_enter(&si->si_lock);
		if (error)
			break;
		if (gen != si->si_gen)
			goto retry;
	}
	/*
	 * Note that we need to synchronously write snapshots that
	 * have not been unlinked, and hence will be visible after
	 * a crash, to ensure their integrity.
	 */
	mutex_exit(&si->si_lock);
	if (saved_data && saved_data != bp->b_data)
		free(saved_data, M_UFSMNT);
	if (snapshot_locked)
		VOP_UNLOCK(vp, 0);
	return error;
}

/*
 * Read the specified block from disk. Vp is usually a snapshot vnode.
 */
static int
readfsblk(struct vnode *vp, void *data, ufs2_daddr_t lbn)
{
	int error;
	struct inode *ip = VTOI(vp);
	struct fs *fs = ip->i_fs;
	struct buf *nbp;

	nbp = getiobuf(NULL, true);
	nbp->b_flags = B_READ;
	nbp->b_bcount = nbp->b_bufsize = fs->fs_bsize;
	nbp->b_error = 0;
	nbp->b_data = data;
	nbp->b_blkno = nbp->b_rawblkno = fsbtodb(fs, blkstofrags(fs, lbn));
	nbp->b_proc = NULL;
	nbp->b_dev = ip->i_devvp->v_rdev;

	bdev_strategy(nbp);

	error = biowait(nbp);

	putiobuf(nbp);

	return error;
}

#if !defined(FFS_NO_SNAPSHOT)
/*
 * Read the specified block. Bypass UBC to prevent deadlocks.
 */
static int
readvnblk(struct vnode *vp, void *data, ufs2_daddr_t lbn)
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
		mutex_enter(&vp->v_interlock);
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
#endif /* !defined(FFS_NO_SNAPSHOT) */

/*
 * Write the specified block. Bypass UBC to prevent deadlocks.
 */
static int
writevnblk(struct vnode *vp, void *data, ufs2_daddr_t lbn)
{
	int s, error;
	off_t offset;
	struct buf *bp;
	struct inode *ip = VTOI(vp);
	struct fs *fs = ip->i_fs;

	offset = lblktosize(fs, (off_t)lbn);
	s = cow_enter();
	mutex_enter(&vp->v_interlock);
	error = VOP_PUTPAGES(vp, trunc_page(offset),
	    round_page(offset+fs->fs_bsize), PGO_CLEANIT|PGO_SYNCIO|PGO_FREE);
	if (error == 0)
		error = ffs_balloc(vp, lblktosize(fs, (off_t)lbn),
		    fs->fs_bsize, KERNCRED, B_SYNC, &bp);
	cow_leave(s);
	if (error)
		return error;

	bcopy(data, bp->b_data, fs->fs_bsize);
	mutex_enter(&bufcache_lock);
	/* XXX Shouldn't need to lock for this, NOCACHE is only read later. */
	bp->b_cflags |= BC_NOCACHE;
	mutex_exit(&bufcache_lock);

	return bwrite(bp);
}

/*
 * Set/reset lwp's LP_UFSCOW flag.
 * May be called recursive.
 */
static inline int
cow_enter(void)
{
	struct lwp *l = curlwp;

	if (l->l_pflag & LP_UFSCOW) {
		return 0;
	} else {
		l->l_pflag |= LP_UFSCOW;
		return LP_UFSCOW;
	}
}

static inline void
cow_leave(int flag)
{
	struct lwp *l = curlwp;

	l->l_pflag &= ~flag;
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
idb_get(struct inode *ip, void *bf, int loc)
{
	if (ip->i_ump->um_fstype == UFS1)
		return ufs_rw32(((ufs1_daddr_t *)(bf))[loc],
		    UFS_IPNEEDSWAP(ip));
	else
		return ufs_rw64(((ufs2_daddr_t *)(bf))[loc],
		    UFS_IPNEEDSWAP(ip));
}

static inline void
idb_assign(struct inode *ip, void *bf, int loc, ufs2_daddr_t val)
{
	if (ip->i_ump->um_fstype == UFS1)
		((ufs1_daddr_t *)(bf))[loc] =
		    ufs_rw32(val, UFS_IPNEEDSWAP(ip));
	else
		((ufs2_daddr_t *)(bf))[loc] =
		    ufs_rw64(val, UFS_IPNEEDSWAP(ip));
}
