/*	$NetBSD: ffs_wapbl.c,v 1.1.2.1 2008/06/10 14:51:23 simonb Exp $	*/

/*-
 * Copyright (c) 2003,2006,2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wasabi Systems, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ffs_wapbl.c,v 1.1.2.1 2008/06/10 14:51:23 simonb Exp $");

#if defined(_KERNEL_OPT)
#include "opt_ffs.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/file.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/kauth.h>
#include <sys/wapbl.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufs_wapbl.h>

#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

static int ffs_wapbl_log_position(struct mount *, struct fs *, struct vnode *,
    daddr_t *, size_t *, size_t *);

/* This function is invoked after a log is replayed to
 * disk to perform logical cleanup actions as described by
 * the log
 */
void
ffs_wapbl_replay_finish(struct mount *mp)
{
	struct wapbl_replay *wr = mp->mnt_wapbl_replay;
	int i;
	int error;

	if (!wr)
		return;

	KDASSERT((mp->mnt_flag & MNT_RDONLY) == 0);

	for (i=0;i<wr->wr_inodescnt;i++) {
		struct vnode *vp;
		struct inode *ip;
		error = VFS_VGET(mp, wr->wr_inodes[i].wr_inumber, &vp);
		if (error) {
			printf("ffs_wapbl_replay_finish: "
			    "unable to cleanup inode %" PRIu32 "\n",
			    wr->wr_inodes[i].wr_inumber);
			continue;
		}
		ip = VTOI(vp);
		KDASSERT(wr->wr_inodes[i].wr_inumber == ip->i_number);
		printf("ffs_wapbl_replay_finish: "
		    "cleaning inode %" PRIu64 " size=%" PRIu64 " mode=%o nlink=%d\n",
		    ip->i_number, ip->i_size, ip->i_mode, ip->i_nlink);
		KASSERT(ip->i_nlink == 0);

		/*
		 * The journal may have left partially allocated inodes in mode
		 * zero.  This may occur if a crash occurs betweeen the node
		 * allocation in ffs_nodeallocg and when the node is properly
		 * initialized in ufs_makeinode.  If so, just dallocate them.
		 */
		if (ip->i_mode == 0) {
			UFS_WAPBL_BEGIN(mp);
			ffs_vfree(vp, ip->i_number, wr->wr_inodes[i].wr_imode);
			UFS_WAPBL_END(mp);
		}
		vput(vp);
	}
	mp->mnt_wapbl_replay = 0;
	wapbl_replay_free(wr);
}

/* Callback for wapbl */
void
ffs_wapbl_sync_metadata(struct mount *mp, daddr_t *deallocblks,
	int *dealloclens, int dealloccnt)
{
	struct ufsmount *ump = VFSTOUFS(mp);
	struct fs *fs = ump->um_fs;
	int i, error;

#ifdef WAPBL_DEBUG_INODES
	ufs_wapbl_verify_inodes(mp, "ffs_wapbl_sync_metadata");
#endif

	for (i = 0; i< dealloccnt; i++) {
		/*
		 * blkfree errors are unreported, might silently fail
		 * if it cannot read the cylinder group block
		 */
		ffs_blkfree(fs, ump->um_devvp,
		    dbtofsb(fs, deallocblks[i]), dealloclens[i], -1);
	}

	fs->fs_fmod = 0;
	fs->fs_time = time_second;
	error = ffs_cgupdate(ump, 0);
	KASSERT(error == 0);
}

void
ffs_wapbl_abort_sync_metadata(struct mount *mp, daddr_t *deallocblks,
	int *dealloclens, int dealloccnt)
{
	struct ufsmount *ump = VFSTOUFS(mp);
	struct fs *fs = ump->um_fs;
	int i;

	/*
	 * I suppose we could dig around for an in use inode, but
	 * its not really used by ffs_blkalloc, so we just fake
	 * the couple of fields that it touches.
	 */
	struct inode in;
	in.i_fs = fs;
	in.i_devvp = ump->um_devvp;
	in.i_dev = ump->um_dev;
	in.i_number = -1;
	in.i_uid = 0;
	for (i = 0; i < dealloccnt; i++) {
		/*
		 * Since the above blkfree may have failed, this blkalloc might
		 * fail as well, so don't check its error.  Note that if the
		 * blkfree succeeded above, then this shouldn't fail because
		 * the buffer will be locked in the current transaction.
		 */
		ffs_blkalloc(&in, dbtofsb(fs, deallocblks[i]),
		    dealloclens[i]);
	}
}


static int
ffs_wapbl_log_position(struct mount *mp, struct fs *fs, struct vnode *devvp,
    daddr_t* startp, size_t* countp, size_t* blksizep)
{
	int error;
	struct partinfo dpart;
	daddr_t logstart =  fsbtodb(fs, fs->fs_size);
	daddr_t logend ;
	size_t blksize;

	/* Use space after filesystem on partition for log */

	error = VOP_IOCTL(devvp, DIOCGPART, &dpart, FREAD, FSCRED);
	if (!error) {
		logend  = dpart.part->p_size;
		blksize = dpart.disklab->d_secsize;
	} else {
		struct dkwedge_info dkw;
		error = VOP_IOCTL(devvp, DIOCGWEDGEINFO, &dkw, FREAD, FSCRED);
		if (error)
			return error;

		blksize = DEV_BSIZE;
		logend = dkw.dkw_size;
	}

	KDASSERT(blksize != 0);
	KDASSERT(logstart <= logend);

	*startp = logstart;
	*countp = (logend - logstart);
	*blksizep = blksize;

	return 0;
}

int
ffs_wapbl_start(struct mount *mp)
{
	struct ufsmount *ump = VFSTOUFS(mp);
	struct fs *fs = ump->um_fs;
	struct vnode *devvp = ump->um_devvp;
	daddr_t off;
	size_t count;
	size_t blksize;
	int error;

	if (mp->mnt_wapbl == 0) {
		if (mp->mnt_flag & MNT_LOG) {
			KDASSERT(fs->fs_ronly == 0);

			error = ffs_wapbl_log_position(mp, fs, devvp, &off,
			    &count, &blksize);
			if (error)
				return error;

			error = wapbl_start(&mp->mnt_wapbl, mp, devvp, off,
			    count, blksize, mp->mnt_wapbl_replay,
			    ffs_wapbl_sync_metadata,
			    ffs_wapbl_abort_sync_metadata);
			if (error)
				return error;

                        mp->mnt_wapbl_op = &wapbl_ops;

#ifdef WAPBL_DEBUG
			printf("%s: enabling logging\n", fs->fs_fsmnt);
#endif

			if ((fs->fs_flags & FS_DOWAPBL) == 0) {
				UFS_WAPBL_BEGIN(mp);
				fs->fs_flags |= FS_DOWAPBL;
				error = ffs_sbupdate(ump, MNT_WAIT);
				KASSERT(error == 0);
				UFS_WAPBL_END(mp);
				error = wapbl_flush(mp->mnt_wapbl, 1);
				if (error) {
					ffs_wapbl_stop(mp, MNT_FORCE);
					return error;
				}
			}
		} else if (fs->fs_flags & FS_DOWAPBL) {
			fs->fs_fmod = 1;
			fs->fs_flags &= ~FS_DOWAPBL;
		}
	}

	/*
	 * It is recommended that you finish replay with logging enabled.
	 * However, even if logging is not enabled, the remaining log
	 * replay should be safely recoverable with an fsck, so perform
	 * it anyway.
	 */
	if ((fs->fs_ronly == 0) && mp->mnt_wapbl_replay) {
		int saveflag = mp->mnt_flag & MNT_RDONLY;
		/*
		 * Make sure MNT_RDONLY is not set so that the inode
		 * cleanup in ufs_inactive will actually do its work.
		 */
		mp->mnt_flag &= ~MNT_RDONLY;
		ffs_wapbl_replay_finish(mp);
		mp->mnt_flag |= saveflag;
		KASSERT(fs->fs_ronly == 0);
	}

	return 0;
}

int
ffs_wapbl_stop(struct mount *mp, int force)
{
	struct ufsmount *ump = VFSTOUFS(mp);
	struct fs *fs = ump->um_fs;
	int error;

	if (mp->mnt_wapbl) {
		KDASSERT(fs->fs_ronly == 0);

		/*
		 * Make sure turning off FS_DOWAPBL is only removed
		 * as the only change in the final flush since otherwise
		 * a transaction may reorder writes.
		 */
		error = wapbl_flush(mp->mnt_wapbl, 1);
		if (error && !force)
			return error;
		if (error && force)
			goto forceout;
		error = UFS_WAPBL_BEGIN(mp);
		if (error && !force)
			return error;
		if (error && force)
			goto forceout;
		KASSERT(fs->fs_flags & FS_DOWAPBL);
		fs->fs_flags &= ~FS_DOWAPBL;
		error = ffs_sbupdate(ump, MNT_WAIT);
		KASSERT(error == 0);
		UFS_WAPBL_END(mp);
	forceout:
		error = wapbl_stop(mp->mnt_wapbl, force);
		if (error) {
			KASSERT(!force);
			fs->fs_flags |= FS_DOWAPBL;
			return error;
		}
		fs->fs_flags &= ~FS_DOWAPBL; /* Repeat in case of forced error */
		mp->mnt_wapbl = 0;

#ifdef WAPBL_DEBUG
		printf("%s: disabled logging\n", fs->fs_fsmnt);
#endif
	}

	return 0;
}

int
ffs_wapbl_replay_start(struct mount *mp, struct fs *fs, struct vnode *devvp)
{
	int error;
	daddr_t off;
	size_t count;
	size_t blksize;

	error =  ffs_wapbl_log_position(mp, fs, devvp, &off, &count, &blksize);
	if (error)
		return error;

	/* Use space after filesystem on partition for log */
	error = wapbl_replay_start(&mp->mnt_wapbl_replay, devvp, off,
		count, blksize);
	if (error)
		return error;

        mp->mnt_wapbl_op = &wapbl_ops;

	return 0;
}
