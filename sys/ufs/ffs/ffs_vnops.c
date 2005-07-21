/*	$NetBSD: ffs_vnops.c,v 1.71 2005/07/21 22:00:08 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ffs_vnops.c,v 1.71 2005/07/21 22:00:08 yamt Exp $");

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

static int ffs_full_fsync __P((void *));

/* Global vfs data structures for ufs. */
int (**ffs_vnodeop_p) __P((void *));
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
	{ &vop_blkatoff_desc, ffs_blkatoff },		/* blkatoff */
	{ &vop_valloc_desc, ffs_valloc },		/* valloc */
	{ &vop_balloc_desc, ffs_balloc },		/* balloc */
	{ &vop_reallocblks_desc, ffs_reallocblks },	/* reallocblks */
	{ &vop_vfree_desc, ffs_vfree },			/* vfree */
	{ &vop_truncate_desc, ffs_truncate },		/* truncate */
	{ &vop_update_desc, ffs_update },		/* update */
	{ &vop_bwrite_desc, vn_bwrite },		/* bwrite */
	{ &vop_getpages_desc, ffs_getpages },		/* getpages */
	{ &vop_putpages_desc, ffs_putpages },		/* putpages */
	{ NULL, NULL }
};
const struct vnodeopv_desc ffs_vnodeop_opv_desc =
	{ &ffs_vnodeop_p, ffs_vnodeop_entries };

int (**ffs_specop_p) __P((void *));
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
	{ &vop_blkatoff_desc, spec_blkatoff },		/* blkatoff */
	{ &vop_valloc_desc, spec_valloc },		/* valloc */
	{ &vop_reallocblks_desc, spec_reallocblks },	/* reallocblks */
	{ &vop_vfree_desc, ffs_vfree },			/* vfree */
	{ &vop_truncate_desc, spec_truncate },		/* truncate */
	{ &vop_update_desc, ffs_update },		/* update */
	{ &vop_bwrite_desc, vn_bwrite },		/* bwrite */
	{ &vop_getpages_desc, spec_getpages },		/* getpages */
	{ &vop_putpages_desc, spec_putpages },		/* putpages */
	{ NULL, NULL }
};
const struct vnodeopv_desc ffs_specop_opv_desc =
	{ &ffs_specop_p, ffs_specop_entries };

int (**ffs_fifoop_p) __P((void *));
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
	{ &vop_blkatoff_desc, fifo_blkatoff },		/* blkatoff */
	{ &vop_valloc_desc, fifo_valloc },		/* valloc */
	{ &vop_reallocblks_desc, fifo_reallocblks },	/* reallocblks */
	{ &vop_vfree_desc, ffs_vfree },			/* vfree */
	{ &vop_truncate_desc, fifo_truncate },		/* truncate */
	{ &vop_update_desc, ffs_update },		/* update */
	{ &vop_bwrite_desc, vn_bwrite },		/* bwrite */
	{ &vop_putpages_desc, fifo_putpages }, 		/* putpages */
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
		struct ucred *a_cred;
		int a_flags;
		off_t a_offlo;
		off_t a_offhi;
		struct proc *a_p;
	} */ *ap = v;
	struct buf *bp;
	int s, num, error, i;
	struct indir ia[NIADDR + 1];
	int bsize;
	daddr_t blk_high;
	struct vnode *vp;

	/*
	 * XXX no easy way to sync a range in a file with softdep.
	 */
	if ((ap->a_offlo == 0 && ap->a_offhi == 0) || DOINGSOFTDEP(ap->a_vp) ||
			(ap->a_vp->v_type != VREG))
		return ffs_full_fsync(v);

	vp = ap->a_vp;

	bsize = ap->a_vp->v_mount->mnt_stat.f_iosize;
	blk_high = ap->a_offhi / bsize;
	if (ap->a_offhi % bsize != 0)
		blk_high++;

	/*
	 * First, flush all pages in range.
	 */

	simple_lock(&vp->v_interlock);
	error = VOP_PUTPAGES(vp, trunc_page(ap->a_offlo),
	    round_page(ap->a_offhi), PGO_CLEANIT |
	    ((ap->a_flags & FSYNC_WAIT) ? PGO_SYNCIO : 0));
	if (error) {
		return error;
	}

	/*
	 * Then, flush indirect blocks.
	 */

	s = splbio();
	if (blk_high >= NDADDR) {
		error = ufs_getlbns(vp, blk_high, ia, &num);
		if (error) {
			splx(s);
			return error;
		}
		for (i = 0; i < num; i++) {
			bp = incore(vp, ia[i].in_lbn);
			if (bp != NULL) {
				simple_lock(&bp->b_interlock);
				if (!(bp->b_flags & B_BUSY) && (bp->b_flags & B_DELWRI)) {
					bp->b_flags |= B_BUSY | B_VFLUSH;
					simple_unlock(&bp->b_interlock);
					splx(s);
					bawrite(bp);
					s = splbio();
				} else {
					simple_unlock(&bp->b_interlock);
				}
			}
		}
	}

	if (ap->a_flags & FSYNC_WAIT) {
		simple_lock(&global_v_numoutput_slock);
		while (vp->v_numoutput > 0) {
			vp->v_flag |= VBWAIT;
			ltsleep(&vp->v_numoutput, PRIBIO + 1, "fsync_range", 0,
				&global_v_numoutput_slock);
		}
		simple_unlock(&global_v_numoutput_slock);
	}
	splx(s);

	error = VOP_UPDATE(vp, NULL, NULL,
	    ((ap->a_flags & (FSYNC_WAIT | FSYNC_DATAONLY)) == FSYNC_WAIT)
	    ? UPDATE_WAIT : 0);

	if (error == 0 && ap->a_flags & FSYNC_CACHE) {
		int l = 0;
		VOP_IOCTL(VTOI(vp)->i_devvp, DIOCCACHESYNC, &l, FWRITE,
			ap->a_p->p_ucred, ap->a_p);
	}

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
		struct ucred *a_cred;
		int a_flags;
		off_t a_offlo;
		off_t a_offhi;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct buf *bp, *nbp;
	int s, error, passes, skipmeta, inodedeps_only, waitfor;

	if (vp->v_type == VBLK &&
	    vp->v_specmountpoint != NULL &&
	    (vp->v_specmountpoint->mnt_flag & MNT_SOFTDEP))
		softdep_fsync_mountdev(vp);

	inodedeps_only = DOINGSOFTDEP(vp) && (ap->a_flags & FSYNC_RECLAIM)
	    && vp->v_uobj.uo_npages == 0 && LIST_EMPTY(&vp->v_dirtyblkhd);

	/*
	 * Flush all dirty data associated with a vnode.
	 */

	if (vp->v_type == VREG || vp->v_type == VBLK || vp->v_type == VCHR) {
		simple_lock(&vp->v_interlock);
		error = VOP_PUTPAGES(vp, 0, 0, PGO_ALLPAGES | PGO_CLEANIT |
		    ((ap->a_flags & FSYNC_WAIT) ? PGO_SYNCIO : 0));
		if (error) {
			return error;
		}
	}

	passes = NIADDR + 1;
	skipmeta = 0;
	if (ap->a_flags & FSYNC_WAIT)
		skipmeta = 1;
	s = splbio();

loop:
	LIST_FOREACH(bp, &vp->v_dirtyblkhd, b_vnbufs)
		bp->b_flags &= ~B_SCANNED;
	for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);
		simple_lock(&bp->b_interlock);
		if (bp->b_flags & (B_BUSY | B_SCANNED)) {
			simple_unlock(&bp->b_interlock);
			continue;
		}
		if ((bp->b_flags & B_DELWRI) == 0)
			panic("ffs_fsync: not dirty");
		if (skipmeta && bp->b_lblkno < 0) {
			simple_unlock(&bp->b_interlock);
			continue;
		}
		simple_unlock(&bp->b_interlock);
		bp->b_flags |= B_BUSY | B_VFLUSH | B_SCANNED;
		splx(s);
		/*
		 * On our final pass through, do all I/O synchronously
		 * so that we can find out if our flush is failing
		 * because of write errors.
		 */
		if (passes > 0 || !(ap->a_flags & FSYNC_WAIT))
			(void) bawrite(bp);
		else if ((error = bwrite(bp)) != 0)
			return (error);
		s = splbio();
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
		simple_lock(&global_v_numoutput_slock);
		while (vp->v_numoutput) {
			vp->v_flag |= VBWAIT;
			(void) ltsleep(&vp->v_numoutput, PRIBIO + 1,
			    "ffsfsync", 0, &global_v_numoutput_slock);
		}
		simple_unlock(&global_v_numoutput_slock);
		splx(s);

		/*
		 * Ensure that any filesystem metadata associated
		 * with the vnode has been written.
		 */
		if ((error = softdep_sync_metadata(ap)) != 0)
			return (error);

		s = splbio();
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
	splx(s);

	if (inodedeps_only)
		waitfor = 0;
	else
		waitfor = (ap->a_flags & FSYNC_WAIT) ? UPDATE_WAIT : 0;
	error = VOP_UPDATE(vp, NULL, NULL, waitfor);

	if (error == 0 && ap->a_flags & FSYNC_CACHE) {
		int i = 0;
		VOP_IOCTL(VTOI(vp)->i_devvp, DIOCCACHESYNC, &i, FWRITE,
			ap->a_p->p_ucred, ap->a_p);
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
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct inode *ip = VTOI(vp);
	struct ufsmount *ump = ip->i_ump;
	int error;

	if ((error = ufs_reclaim(vp, ap->a_p)) != 0)
		return (error);
	if (ip->i_din.ffs1_din != NULL) {
		if (ump->um_fstype == UFS1)
			pool_put(&ffs_dinode1_pool, ip->i_din.ffs1_din);
		else
			pool_put(&ffs_dinode2_pool, ip->i_din.ffs2_din);
	}
	/*
	 * XXX MFS ends up here, too, to free an inode.  Should we create
	 * XXX a separate pool for MFS inodes?
	 */
	pool_put(&ffs_inode_pool, vp->v_data);
	vp->v_data = NULL;
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
			simple_unlock(&vp->v_interlock);
		}
		return EINVAL;
	}
	return genfs_getpages(v);
}

int
ffs_putpages(void *v)
{
	struct vop_putpages_args /* {
		struct vnode *a_vp;
		voff_t a_offlo;
		voff_t a_offhi;
		int a_flags;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct uvm_object *uobj = &vp->v_uobj;
	struct inode *ip = VTOI(vp);
	struct fs *fs = ip->i_fs;
	struct vm_page *pg;
	off_t off;

	if (!DOINGSOFTDEP(vp) || (ap->a_flags & PGO_CLEANIT) == 0) {
		return genfs_putpages(v);
	}

	/*
	 * for softdep files, force the pages in a block to be written together.
	 * if we're the pagedaemon and we would have to wait for other pages,
	 * just fail the request.  the pagedaemon will pick a different page.
	 */

	ap->a_offlo &= ~fs->fs_qbmask;
	ap->a_offhi = blkroundup(fs, ap->a_offhi);
	if (curproc == uvm.pagedaemon_proc) {
		for (off = ap->a_offlo; off < ap->a_offhi; off += PAGE_SIZE) {
			pg = uvm_pagelookup(uobj, off);

			/*
			 * we only have missing pages here because the
			 * calculation of offhi above doesn't account for
			 * fragments.  so once we see one missing page,
			 * the rest should be missing as well, but we'll
			 * check for the rest just to be paranoid.
			 */

			if (pg == NULL) {
				continue;
			}
			if (pg->flags & PG_BUSY) {
				simple_unlock(&uobj->vmobjlock);
				return EBUSY;
			}
		}
	}
	return genfs_putpages(v);
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

	KASSERT(flags & (GOP_SIZE_READ | GOP_SIZE_WRITE));
	KASSERT((flags & (GOP_SIZE_READ | GOP_SIZE_WRITE))
		!= (GOP_SIZE_READ | GOP_SIZE_WRITE));

	olbn = lblkno(fs, ip->i_size);
	nlbn = lblkno(fs, size);
	if (nlbn < NDADDR && olbn <= nlbn) {
		*eobp = fragroundup(fs, size);
	} else {
		*eobp = blkroundup(fs, size);
	}
}
