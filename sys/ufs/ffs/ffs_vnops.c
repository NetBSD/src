/*	$NetBSD: ffs_vnops.c,v 1.86.4.4 2007/05/27 00:14:03 ad Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)ffs_vnops.c	8.15 (Berkeley) 5/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ffs_vnops.c,v 1.86.4.4 2007/05/27 00:14:03 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/resourcevar.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/event.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/pool.h>
#include <sys/signalvar.h>
#include <sys/kauth.h>
#include <sys/fstrans.h>

#include <miscfs/fifofs/fifo.h>
#include <miscfs/genfs/genfs.h>
#include <miscfs/specfs/specdev.h>

#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/ufs_extern.h>
#include <ufs/ufs/ufsmount.h>

#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

#include <uvm/uvm.h>

static int ffs_full_fsync(void *);

/* Global vfs data structures for ufs. */
int (**ffs_vnodeop_p)(void *);
const struct vnodeopv_entry_desc ffs_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, ufs_lookup },		/* lookup */
	{ &vop_create_desc, ufs_create },		/* create */
	{ &vop_whiteout_desc, ufs_whiteout },		/* whiteout */
	{ &vop_mknod_desc, ufs_mknod },			/* mknod */
	{ &vop_open_desc, ufs_open },			/* open */
	{ &vop_close_desc, ufs_close },			/* close */
	{ &vop_access_desc, ufs_access },		/* access */
	{ &vop_getattr_desc, ufs_getattr },		/* getattr */
	{ &vop_setattr_desc, ufs_setattr },		/* setattr */
	{ &vop_read_desc, ffs_read },			/* read */
	{ &vop_write_desc, ffs_write },			/* write */
	{ &vop_lease_desc, ufs_lease_check },		/* lease */
	{ &vop_ioctl_desc, ufs_ioctl },			/* ioctl */
	{ &vop_fcntl_desc, ufs_fcntl },			/* fcntl */
	{ &vop_poll_desc, ufs_poll },			/* poll */
	{ &vop_kqfilter_desc, genfs_kqfilter },		/* kqfilter */
	{ &vop_revoke_desc, ufs_revoke },		/* revoke */
	{ &vop_mmap_desc, ufs_mmap },			/* mmap */
	{ &vop_fsync_desc, ffs_fsync },			/* fsync */
	{ &vop_seek_desc, ufs_seek },			/* seek */
	{ &vop_remove_desc, ufs_remove },		/* remove */
	{ &vop_link_desc, ufs_link },			/* link */
	{ &vop_rename_desc, ufs_rename },		/* rename */
	{ &vop_mkdir_desc, ufs_mkdir },			/* mkdir */
	{ &vop_rmdir_desc, ufs_rmdir },			/* rmdir */
	{ &vop_symlink_desc, ufs_symlink },		/* symlink */
	{ &vop_readdir_desc, ufs_readdir },		/* readdir */
	{ &vop_readlink_desc, ufs_readlink },		/* readlink */
	{ &vop_abortop_desc, ufs_abortop },		/* abortop */
	{ &vop_inactive_desc, ufs_inactive },		/* inactive */
	{ &vop_reclaim_desc, ffs_reclaim },		/* reclaim */
	{ &vop_lock_desc, ufs_lock },			/* lock */
	{ &vop_unlock_desc, ufs_unlock },		/* unlock */
	{ &vop_bmap_desc, ufs_bmap },			/* bmap */
	{ &vop_strategy_desc, ufs_strategy },		/* strategy */
	{ &vop_print_desc, ufs_print },			/* print */
	{ &vop_islocked_desc, ufs_islocked },		/* islocked */
	{ &vop_pathconf_desc, ufs_pathconf },		/* pathconf */
	{ &vop_advlock_desc, ufs_advlock },		/* advlock */
	{ &vop_bwrite_desc, vn_bwrite },		/* bwrite */
	{ &vop_getpages_desc, ffs_getpages },		/* getpages */
	{ &vop_putpages_desc, genfs_putpages },		/* putpages */
	{ &vop_openextattr_desc, ffs_openextattr },	/* openextattr */
	{ &vop_closeextattr_desc, ffs_closeextattr },	/* closeextattr */
	{ &vop_getextattr_desc, ffs_getextattr },	/* getextattr */
	{ &vop_setextattr_desc, ffs_setextattr },	/* setextattr */
	{ &vop_listextattr_desc, ffs_listextattr },	/* listextattr */
	{ &vop_deleteextattr_desc, ffs_deleteextattr },	/* deleteextattr */
	{ NULL, NULL }
};
const struct vnodeopv_desc ffs_vnodeop_opv_desc =
	{ &ffs_vnodeop_p, ffs_vnodeop_entries };

int (**ffs_specop_p)(void *);
const struct vnodeopv_entry_desc ffs_specop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, spec_lookup },		/* lookup */
	{ &vop_create_desc, spec_create },		/* create */
	{ &vop_mknod_desc, spec_mknod },		/* mknod */
	{ &vop_open_desc, spec_open },			/* open */
	{ &vop_close_desc, ufsspec_close },		/* close */
	{ &vop_access_desc, ufs_access },		/* access */
	{ &vop_getattr_desc, ufs_getattr },		/* getattr */
	{ &vop_setattr_desc, ufs_setattr },		/* setattr */
	{ &vop_read_desc, ufsspec_read },		/* read */
	{ &vop_write_desc, ufsspec_write },		/* write */
	{ &vop_lease_desc, spec_lease_check },		/* lease */
	{ &vop_ioctl_desc, spec_ioctl },		/* ioctl */
	{ &vop_fcntl_desc, ufs_fcntl },			/* fcntl */
	{ &vop_poll_desc, spec_poll },			/* poll */
	{ &vop_kqfilter_desc, spec_kqfilter },		/* kqfilter */
	{ &vop_revoke_desc, spec_revoke },		/* revoke */
	{ &vop_mmap_desc, spec_mmap },			/* mmap */
	{ &vop_fsync_desc, ffs_fsync },			/* fsync */
	{ &vop_seek_desc, spec_seek },			/* seek */
	{ &vop_remove_desc, spec_remove },		/* remove */
	{ &vop_link_desc, spec_link },			/* link */
	{ &vop_rename_desc, spec_rename },		/* rename */
	{ &vop_mkdir_desc, spec_mkdir },		/* mkdir */
	{ &vop_rmdir_desc, spec_rmdir },		/* rmdir */
	{ &vop_symlink_desc, spec_symlink },		/* symlink */
	{ &vop_readdir_desc, spec_readdir },		/* readdir */
	{ &vop_readlink_desc, spec_readlink },		/* readlink */
	{ &vop_abortop_desc, spec_abortop },		/* abortop */
	{ &vop_inactive_desc, ufs_inactive },		/* inactive */
	{ &vop_reclaim_desc, ffs_reclaim },		/* reclaim */
	{ &vop_lock_desc, ufs_lock },			/* lock */
	{ &vop_unlock_desc, ufs_unlock },		/* unlock */
	{ &vop_bmap_desc, spec_bmap },			/* bmap */
	{ &vop_strategy_desc, spec_strategy },		/* strategy */
	{ &vop_print_desc, ufs_print },			/* print */
	{ &vop_islocked_desc, ufs_islocked },		/* islocked */
	{ &vop_pathconf_desc, spec_pathconf },		/* pathconf */
	{ &vop_advlock_desc, spec_advlock },		/* advlock */
	{ &vop_bwrite_desc, vn_bwrite },		/* bwrite */
	{ &vop_getpages_desc, spec_getpages },		/* getpages */
	{ &vop_putpages_desc, spec_putpages },		/* putpages */
	{ &vop_openextattr_desc, ffs_openextattr },	/* openextattr */
	{ &vop_closeextattr_desc, ffs_closeextattr },	/* closeextattr */
	{ &vop_getextattr_desc, ffs_getextattr },	/* getextattr */
	{ &vop_setextattr_desc, ffs_setextattr },	/* setextattr */
	{ &vop_listextattr_desc, ffs_listextattr },	/* listextattr */
	{ &vop_deleteextattr_desc, ffs_deleteextattr },	/* deleteextattr */
	{ NULL, NULL }
};
const struct vnodeopv_desc ffs_specop_opv_desc =
	{ &ffs_specop_p, ffs_specop_entries };

int (**ffs_fifoop_p)(void *);
const struct vnodeopv_entry_desc ffs_fifoop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, fifo_lookup },		/* lookup */
	{ &vop_create_desc, fifo_create },		/* create */
	{ &vop_mknod_desc, fifo_mknod },		/* mknod */
	{ &vop_open_desc, fifo_open },			/* open */
	{ &vop_close_desc, ufsfifo_close },		/* close */
	{ &vop_access_desc, ufs_access },		/* access */
	{ &vop_getattr_desc, ufs_getattr },		/* getattr */
	{ &vop_setattr_desc, ufs_setattr },		/* setattr */
	{ &vop_read_desc, ufsfifo_read },		/* read */
	{ &vop_write_desc, ufsfifo_write },		/* write */
	{ &vop_lease_desc, fifo_lease_check },		/* lease */
	{ &vop_ioctl_desc, fifo_ioctl },		/* ioctl */
	{ &vop_fcntl_desc, ufs_fcntl },			/* fcntl */
	{ &vop_poll_desc, fifo_poll },			/* poll */
	{ &vop_kqfilter_desc, fifo_kqfilter },		/* kqfilter */
	{ &vop_revoke_desc, fifo_revoke },		/* revoke */
	{ &vop_mmap_desc, fifo_mmap },			/* mmap */
	{ &vop_fsync_desc, ffs_fsync },			/* fsync */
	{ &vop_seek_desc, fifo_seek },			/* seek */
	{ &vop_remove_desc, fifo_remove },		/* remove */
	{ &vop_link_desc, fifo_link },			/* link */
	{ &vop_rename_desc, fifo_rename },		/* rename */
	{ &vop_mkdir_desc, fifo_mkdir },		/* mkdir */
	{ &vop_rmdir_desc, fifo_rmdir },		/* rmdir */
	{ &vop_symlink_desc, fifo_symlink },		/* symlink */
	{ &vop_readdir_desc, fifo_readdir },		/* readdir */
	{ &vop_readlink_desc, fifo_readlink },		/* readlink */
	{ &vop_abortop_desc, fifo_abortop },		/* abortop */
	{ &vop_inactive_desc, ufs_inactive },		/* inactive */
	{ &vop_reclaim_desc, ffs_reclaim },		/* reclaim */
	{ &vop_lock_desc, ufs_lock },			/* lock */
	{ &vop_unlock_desc, ufs_unlock },		/* unlock */
	{ &vop_bmap_desc, fifo_bmap },			/* bmap */
	{ &vop_strategy_desc, fifo_strategy },		/* strategy */
	{ &vop_print_desc, ufs_print },			/* print */
	{ &vop_islocked_desc, ufs_islocked },		/* islocked */
	{ &vop_pathconf_desc, fifo_pathconf },		/* pathconf */
	{ &vop_advlock_desc, fifo_advlock },		/* advlock */
	{ &vop_bwrite_desc, vn_bwrite },		/* bwrite */
	{ &vop_putpages_desc, fifo_putpages }, 		/* putpages */
	{ &vop_openextattr_desc, ffs_openextattr },	/* openextattr */
	{ &vop_closeextattr_desc, ffs_closeextattr },	/* closeextattr */
	{ &vop_getextattr_desc, ffs_getextattr },	/* getextattr */
	{ &vop_setextattr_desc, ffs_setextattr },	/* setextattr */
	{ &vop_listextattr_desc, ffs_listextattr },	/* listextattr */
	{ &vop_deleteextattr_desc, ffs_deleteextattr },	/* deleteextattr */
	{ NULL, NULL }
};
const struct vnodeopv_desc ffs_fifoop_opv_desc =
	{ &ffs_fifoop_p, ffs_fifoop_entries };

#include <ufs/ufs/ufs_readwrite.c>

int
ffs_fsync(void *v)
{
	struct vop_fsync_args /* {
		struct vnode *a_vp;
		kauth_cred_t a_cred;
		int a_flags;
		off_t a_offlo;
		off_t a_offhi;
		struct lwp *a_l;
	} */ *ap = v;
	struct buf *bp;
	int num, error, i;
	struct indir ia[NIADDR + 1];
	int bsize;
	daddr_t blk_high;
	struct vnode *vp;

	vp = ap->a_vp;

	if ((error = fstrans_start(vp->v_mount, FSTRANS_LAZY)) != 0)
		return error;
	/*
	 * XXX no easy way to sync a range in a file with softdep.
	 */
	if ((ap->a_offlo == 0 && ap->a_offhi == 0) || DOINGSOFTDEP(vp) ||
	    (vp->v_type != VREG)) {
		error = ffs_full_fsync(v);
		goto out;
	}

	bsize = vp->v_mount->mnt_stat.f_iosize;
	blk_high = ap->a_offhi / bsize;
	if (ap->a_offhi % bsize != 0)
		blk_high++;

	/*
	 * First, flush all pages in range.
	 */

	mutex_enter(&vp->v_interlock);
	error = VOP_PUTPAGES(vp, trunc_page(ap->a_offlo),
	    round_page(ap->a_offhi), PGO_CLEANIT |
	    ((ap->a_flags & FSYNC_WAIT) ? PGO_SYNCIO : 0));
	if (error) {
		goto out;
	}

	/*
	 * Then, flush indirect blocks.
	 */

	if (blk_high >= NDADDR) {
		error = ufs_getlbns(vp, blk_high, ia, &num);
		if (error)
			goto out;

		mutex_enter(&bqueue_lock);
		for (i = 0; i < num; i++) {
			if ((bp = incore(vp, ia[i].in_lbn)) == NULL)
				continue;
			mutex_enter(&bp->b_interlock);
			if ((bp->b_flags & (B_BUSY | B_DELWRI)) != B_DELWRI) {
				mutex_exit(&bp->b_interlock);
				continue;
			}
			bp->b_flags |= B_BUSY | B_VFLUSH;
			mutex_exit(&bp->b_interlock);
			mutex_exit(&bqueue_lock);
			bawrite(bp);
			mutex_enter(&bqueue_lock);
		}
		mutex_exit(&bqueue_lock);
	}

	if (ap->a_flags & FSYNC_WAIT) {
		mutex_enter(&global_v_numoutput_lock);
		while (vp->v_numoutput > 0)
			cv_wait(&vp->v_outputcv, &global_v_numoutput_lock);
		mutex_exit(&global_v_numoutput_lock);
	}

	error = ffs_update(vp, NULL, NULL,
	    ((ap->a_flags & (FSYNC_WAIT | FSYNC_DATAONLY)) == FSYNC_WAIT)
	    ? UPDATE_WAIT : 0);

	if (error == 0 && ap->a_flags & FSYNC_CACHE) {
		int l = 0;
		VOP_IOCTL(VTOI(vp)->i_devvp, DIOCCACHESYNC, &l, FWRITE,
			ap->a_l->l_cred, ap->a_l);
	}

out:
	fstrans_done(vp->v_mount);
	return error;
}

/*
 * Synch an open file.
 */
/* ARGSUSED */
static int
ffs_full_fsync(void *v)
{
	struct vop_fsync_args /* {
		struct vnode *a_vp;
		kauth_cred_t a_cred;
		int a_flags;
		off_t a_offlo;
		off_t a_offhi;
		struct lwp *a_l;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct buf *bp, *nbp;
	int error, passes, skipmeta, inodedeps_only, waitfor;

	if (vp->v_type == VBLK &&
	    vp->v_specmountpoint != NULL &&
	    (vp->v_specmountpoint->mnt_flag & MNT_SOFTDEP))
		softdep_fsync_mountdev(vp);

	inodedeps_only = DOINGSOFTDEP(vp) && (ap->a_flags & FSYNC_RECLAIM)
	    && vp->v_uobj.uo_npages == 0 && LIST_EMPTY(&vp->v_dirtyblkhd);

	/*
	 * Flush all dirty data associated with a vnode.
	 */

	if (vp->v_type == VREG || vp->v_type == VBLK) {
		mutex_enter(&vp->v_interlock);
		error = VOP_PUTPAGES(vp, 0, 0, PGO_ALLPAGES | PGO_CLEANIT |
		    ((ap->a_flags & FSYNC_WAIT) ? PGO_SYNCIO : 0) |
		    (fstrans_getstate(vp->v_mount) == FSTRANS_SUSPENDING ?
			PGO_FREE : 0));
		if (error) {
			return error;
		}
	}

	passes = NIADDR + 1;
	skipmeta = 0;
	if (ap->a_flags & FSYNC_WAIT)
		skipmeta = 1;

loop:
	LIST_FOREACH(bp, &vp->v_dirtyblkhd, b_vnbufs) {
		mutex_enter(&bp->b_interlock);
		bp->b_flags &= ~B_SCANNED;
		mutex_exit(&bp->b_interlock);
	}
	for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);
		mutex_enter(&bp->b_interlock);
		if (bp->b_flags & (B_BUSY | B_SCANNED)) {
			mutex_exit(&bp->b_interlock);
			continue;
		}
		if ((bp->b_flags & B_DELWRI) == 0)
			panic("ffs_fsync: not dirty");
		if (skipmeta && bp->b_lblkno < 0) {
			mutex_exit(&bp->b_interlock);
			continue;
		}
		bp->b_flags |= B_BUSY | B_VFLUSH | B_SCANNED;
		mutex_exit(&bp->b_interlock);
		/*
		 * On our final pass through, do all I/O synchronously
		 * so that we can find out if our flush is failing
		 * because of write errors.
		 */
		if (passes > 0 || !(ap->a_flags & FSYNC_WAIT))
			(void) bawrite(bp);
		else if ((error = bwrite(bp)) != 0)
			return (error);
		/*
		 * Since we may have slept during the I/O, we need
		 * to start from a known point.
		 */
		nbp = LIST_FIRST(&vp->v_dirtyblkhd);
	}
	if (skipmeta) {
		skipmeta = 0;
		goto loop;
	}
	if (ap->a_flags & FSYNC_WAIT) {
		mutex_enter(&global_v_numoutput_lock);
		while (vp->v_numoutput) {
			cv_wait(&vp->v_outputcv, &global_v_numoutput_lock);
		}
		mutex_exit(&global_v_numoutput_lock);

		/*
		 * Ensure that any filesystem metadata associated
		 * with the vnode has been written.
		 */
		if ((error = softdep_sync_metadata(ap)) != 0)
			return (error);

		if (!LIST_EMPTY(&vp->v_dirtyblkhd)) {
			/*
			* Block devices associated with filesystems may
			* have new I/O requests posted for them even if
			* the vnode is locked, so no amount of trying will
			* get them clean. Thus we give block devices a
			* good effort, then just give up. For all other file
			* types, go around and try again until it is clean.
			*/
			if (passes > 0) {
				passes--;
				goto loop;
			}
#ifdef DIAGNOSTIC
			if (vp->v_type != VBLK)
				vprint("ffs_fsync: dirty", vp);
#endif
		}
	}

	if (inodedeps_only)
		waitfor = 0;
	else
		waitfor = (ap->a_flags & FSYNC_WAIT) ? UPDATE_WAIT : 0;
	error = ffs_update(vp, NULL, NULL, waitfor);

	if (error == 0 && ap->a_flags & FSYNC_CACHE) {
		int i = 0;
		VOP_IOCTL(VTOI(vp)->i_devvp, DIOCCACHESYNC, &i, FWRITE,
			ap->a_l->l_cred, ap->a_l);
	}

	return error;
}

/*
 * Reclaim an inode so that it can be used for other purposes.
 */
int
ffs_reclaim(void *v)
{
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
		struct lwp *a_l;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct inode *ip = VTOI(vp);
	struct mount *mp = vp->v_mount;
	struct ufsmount *ump = ip->i_ump;
	void *data;
	int error;

	if ((error = fstrans_start(mp, FSTRANS_LAZY)) != 0)
		return error;
	if ((error = ufs_reclaim(vp, ap->a_l)) != 0) {
		fstrans_done(mp);
		return (error);
	}
	if (ip->i_din.ffs1_din != NULL) {
		if (ump->um_fstype == UFS1)
			pool_put(&ffs_dinode1_pool, ip->i_din.ffs1_din);
		else
			pool_put(&ffs_dinode2_pool, ip->i_din.ffs2_din);
	}
	/*
	 * To interlock with ffs_sync().
	 */
	genfs_node_destroy(vp);
	mutex_enter(&vp->v_interlock);
	data = vp->v_data;
	vp->v_data = NULL;
	mutex_exit(&vp->v_interlock);

	/*
	 * XXX MFS ends up here, too, to free an inode.  Should we create
	 * XXX a separate pool for MFS inodes?
	 */
	pool_put(&ffs_inode_pool, data);
	fstrans_done(mp);
	return (0);
}

int
ffs_getpages(void *v)
{
	struct vop_getpages_args /* {
		struct vnode *a_vp;
		voff_t a_offset;
		struct vm_page **a_m;
		int *a_count;
		int a_centeridx;
		vm_prot_t a_access_type;
		int a_advice;
		int a_flags;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct inode *ip = VTOI(vp);
	struct fs *fs = ip->i_fs;

	/*
	 * don't allow a softdep write to create pages for only part of a block.
	 * the dependency tracking requires that all pages be in memory for
	 * a block involved in a dependency.
	 */

	if (ap->a_flags & PGO_OVERWRITE &&
	    (blkoff(fs, ap->a_offset) != 0 ||
	     blkoff(fs, *ap->a_count << PAGE_SHIFT) != 0) &&
	    DOINGSOFTDEP(ap->a_vp)) {
		if ((ap->a_flags & PGO_LOCKED) == 0) {
			mutex_exit(&vp->v_interlock);
		}
		return EINVAL;
	}
	return genfs_getpages(v);
}

/*
 * Return the last logical file offset that should be written for this file
 * if we're doing a write that ends at "size".
 */

void
ffs_gop_size(struct vnode *vp, off_t size, off_t *eobp, int flags)
{
	struct inode *ip = VTOI(vp);
	struct fs *fs = ip->i_fs;
	daddr_t olbn, nlbn;

	olbn = lblkno(fs, ip->i_size);
	nlbn = lblkno(fs, size);
	if (nlbn < NDADDR && olbn <= nlbn) {
		*eobp = fragroundup(fs, size);
	} else {
		*eobp = blkroundup(fs, size);
	}
}

int
ffs_openextattr(void *v)
{
	struct vop_openextattr_args /* {
		struct vnode *a_vp;
		kauth_cred_t a_cred;
		struct proc *a_p;
	} */ *ap = v;
	struct inode *ip = VTOI(ap->a_vp);
	struct fs *fs = ip->i_fs;

	/* Not supported for UFS1 file systems. */
	if (fs->fs_magic == FS_UFS1_MAGIC)
		return (EOPNOTSUPP);

	/* XXX Not implemented for UFS2 file systems. */
	return (EOPNOTSUPP);
}

int
ffs_closeextattr(void *v)
{
	struct vop_closeextattr_args /* {
		struct vnode *a_vp;
		int a_commit;
		kauth_cred_t a_cred;
		struct proc *a_p;
	} */ *ap = v;
	struct inode *ip = VTOI(ap->a_vp);
	struct fs *fs = ip->i_fs;

	/* Not supported for UFS1 file systems. */
	if (fs->fs_magic == FS_UFS1_MAGIC)
		return (EOPNOTSUPP);

	/* XXX Not implemented for UFS2 file systems. */
	return (EOPNOTSUPP);
}

int
ffs_getextattr(void *v)
{
	struct vop_getextattr_args /* {
		struct vnode *a_vp;
		int a_attrnamespace;
		const char *a_name;
		struct uio *a_uio;
		size_t *a_size;
		kauth_cred_t a_cred;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct inode *ip = VTOI(vp);
	struct fs *fs = ip->i_fs;

	if (fs->fs_magic == FS_UFS1_MAGIC) {
#ifdef UFS_EXTATTR
		int error;

		if ((error = fstrans_start(vp->v_mount, FSTRANS_SHARED)) != 0)
			return error;
		error = ufs_getextattr(ap);
		fstrans_done(vp->v_mount);
		return error;
#else
		return (EOPNOTSUPP);
#endif
	}

	/* XXX Not implemented for UFS2 file systems. */
	return (EOPNOTSUPP);
}

int
ffs_setextattr(void *v)
{
	struct vop_setextattr_args /* {
		struct vnode *a_vp;
		int a_attrnamespace;
		const char *a_name;
		struct uio *a_uio;
		kauth_cred_t a_cred;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct inode *ip = VTOI(vp);
	struct fs *fs = ip->i_fs;

	if (fs->fs_magic == FS_UFS1_MAGIC) {
#ifdef UFS_EXTATTR
		int error;

		if ((error = fstrans_start(vp->v_mount, FSTRANS_SHARED)) != 0)
			return error;
		error = ufs_setextattr(ap);
		fstrans_done(vp->v_mount);
		return error;
#else
		return (EOPNOTSUPP);
#endif
	}

	/* XXX Not implemented for UFS2 file systems. */
	return (EOPNOTSUPP);
}

int
ffs_listextattr(void *v)
{
	struct vop_listextattr_args /* {
		struct vnode *a_vp;
		int a_attrnamespace;
		struct uio *a_uio;
		size_t *a_size;
		kauth_cred_t a_cred;
		struct proc *a_p;
	} */ *ap = v;
	struct inode *ip = VTOI(ap->a_vp);
	struct fs *fs = ip->i_fs;

	/* Not supported for UFS1 file systems. */
	if (fs->fs_magic == FS_UFS1_MAGIC)
		return (EOPNOTSUPP);

	/* XXX Not implemented for UFS2 file systems. */
	return (EOPNOTSUPP);
}

int
ffs_deleteextattr(void *v)
{
	struct vop_deleteextattr_args /* {
		struct vnode *a_vp;
		int a_attrnamespace;
		kauth_cred_t a_cred;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct inode *ip = VTOI(vp);
	struct fs *fs = ip->i_fs;

	if (fs->fs_magic == FS_UFS1_MAGIC) {
#ifdef UFS_EXTATTR
		int error;

		if ((error = fstrans_start(vp->v_mount, FSTRANS_SHARED)) != 0)
			return error;
		error = ufs_deleteextattr(ap);
		fstrans_done(vp->v_mount);
		return error;
#else
		return (EOPNOTSUPP);
#endif
	}

	/* XXX Not implemented for UFS2 file systems. */
	return (EOPNOTSUPP);
}
