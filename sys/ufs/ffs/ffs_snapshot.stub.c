/* $NetBSD: ffs_snapshot.stub.c,v 1.1 2004/05/25 14:54:59 hannken Exp $ */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ffs_snapshot.stub.c,v 1.1 2004/05/25 14:54:59 hannken Exp $");

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

#include <miscfs/specfs/specdev.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufs_extern.h>
#include <ufs/ufs/ufs_bswap.h>

#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>


/*
 * Create a snapshot file and initialize it for the filesystem.
 */
int
ffs_snapshot(struct mount *mp, struct vnode *vp, struct timespec *ts)
{
	return EOPNOTSUPP;
}

/*
 * Decrement extra reference on snapshot when last name is removed.
 * It will not be freed until the last open reference goes away.
 */
void
ffs_snapgone(struct inode *ip)
{
}

/*
 * Prepare a snapshot file for being removed.
 */
void
ffs_snapremove(struct vnode *vp)
{
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
ffs_snapblkfree(struct fs *fs, struct vnode *devvp, daddr_t bno,
    long size, ino_t inum)
{
	return 0;
}

/*
 * Associate snapshot files when mounting.
 */
void
ffs_snapshot_mount(struct mount *mp)
{
}

/*
 * Disassociate snapshot files when unmounting.
 */
void
ffs_snapshot_unmount(struct mount *mp)
{
}
