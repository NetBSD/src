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
__KERNEL_RCSID(0, "$NetBSD: ffs_snapshot_stub.c,v 1.1.2.2 2005/02/12 18:17:56 yamt Exp $");

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
 * Vnode is locked on entry and return.
 */
int
ffs_snapshot(struct mount *mp, struct vnode *vp, struct timespec *ctime)
{
	return ENODEV;
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
ffs_snapblkfree(struct fs *fs, struct vnode *devvp, daddr_t bno, long size, ino_t inum)
{
	return 0;
}

/*
 * Associate snapshot files when mounting.
 */
void
ffs_snapshot_mount(struct mount *mp)
{
	struct ufsmount *ump = VFSTOUFS(mp);
	struct fs *fs = ump->um_fs;
	int snaploc;

	printf("ffs_snapshot_mount: snapshots not supported, being deleted\n");

	for (snaploc = 0; snaploc < FSMAXSNAP; snaploc++)
		fs->fs_snapinum[snaploc] = 0;
}

/*
 * Disassociate snapshot files when unmounting.
 */
void
ffs_snapshot_unmount(struct mount *mp)
{
}
