/*	$NetBSD: lfs_vnops.c,v 1.102 2003/04/02 10:39:42 fvdl Exp $	*/

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
/*
 * Copyright (c) 1986, 1989, 1991, 1993, 1995
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
 *	@(#)lfs_vnops.c	8.13 (Berkeley) 6/10/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lfs_vnops.c,v 1.102 2003/04/02 10:39:42 fvdl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/resourcevar.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/signalvar.h>

#include <miscfs/fifofs/fifo.h>
#include <miscfs/genfs/genfs.h>
#include <miscfs/specfs/specdev.h>

#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

#include <uvm/uvm.h>
#include <uvm/uvm_pmap.h>
#include <uvm/uvm_stat.h>
#include <uvm/uvm_pager.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

extern pid_t lfs_writer_daemon;
extern int lfs_subsys_pages;
extern int lfs_dirvcount;
extern struct simplelock lfs_subsys_lock;

/* Global vfs data structures for lfs. */
int (**lfs_vnodeop_p)(void *);
const struct vnodeopv_entry_desc lfs_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, ufs_lookup },		/* lookup */
	{ &vop_create_desc, lfs_create },		/* create */
	{ &vop_whiteout_desc, ufs_whiteout },		/* whiteout */
	{ &vop_mknod_desc, lfs_mknod },			/* mknod */
	{ &vop_open_desc, ufs_open },			/* open */
	{ &vop_close_desc, lfs_close },			/* close */
	{ &vop_access_desc, ufs_access },		/* access */
	{ &vop_getattr_desc, lfs_getattr },		/* getattr */
	{ &vop_setattr_desc, lfs_setattr },		/* setattr */
	{ &vop_read_desc, lfs_read },			/* read */
	{ &vop_write_desc, lfs_write },			/* write */
	{ &vop_lease_desc, ufs_lease_check },		/* lease */
	{ &vop_ioctl_desc, ufs_ioctl },			/* ioctl */
	{ &vop_fcntl_desc, lfs_fcntl },			/* fcntl */
	{ &vop_poll_desc, ufs_poll },			/* poll */
	{ &vop_kqfilter_desc, genfs_kqfilter },		/* kqfilter */
	{ &vop_revoke_desc, ufs_revoke },		/* revoke */
	{ &vop_mmap_desc, lfs_mmap },			/* mmap */
	{ &vop_fsync_desc, lfs_fsync },			/* fsync */
	{ &vop_seek_desc, ufs_seek },			/* seek */
	{ &vop_remove_desc, lfs_remove },		/* remove */
	{ &vop_link_desc, lfs_link },			/* link */
	{ &vop_rename_desc, lfs_rename },		/* rename */
	{ &vop_mkdir_desc, lfs_mkdir },			/* mkdir */
	{ &vop_rmdir_desc, lfs_rmdir },			/* rmdir */
	{ &vop_symlink_desc, lfs_symlink },		/* symlink */
	{ &vop_readdir_desc, ufs_readdir },		/* readdir */
	{ &vop_readlink_desc, ufs_readlink },		/* readlink */
	{ &vop_abortop_desc, ufs_abortop },		/* abortop */
	{ &vop_inactive_desc, lfs_inactive },		/* inactive */
	{ &vop_reclaim_desc, lfs_reclaim },		/* reclaim */
	{ &vop_lock_desc, ufs_lock },			/* lock */
	{ &vop_unlock_desc, ufs_unlock },		/* unlock */
	{ &vop_bmap_desc, ufs_bmap },			/* bmap */
	{ &vop_strategy_desc, lfs_strategy },		/* strategy */
	{ &vop_print_desc, ufs_print },			/* print */
	{ &vop_islocked_desc, ufs_islocked },		/* islocked */
	{ &vop_pathconf_desc, ufs_pathconf },		/* pathconf */
	{ &vop_advlock_desc, ufs_advlock },		/* advlock */
	{ &vop_blkatoff_desc, lfs_blkatoff },		/* blkatoff */
	{ &vop_valloc_desc, lfs_valloc },		/* valloc */
	{ &vop_balloc_desc, lfs_balloc },		/* balloc */
	{ &vop_vfree_desc, lfs_vfree },			/* vfree */
	{ &vop_truncate_desc, lfs_truncate },		/* truncate */
	{ &vop_update_desc, lfs_update },		/* update */
	{ &vop_bwrite_desc, lfs_bwrite },		/* bwrite */
	{ &vop_getpages_desc, lfs_getpages },		/* getpages */
	{ &vop_putpages_desc, lfs_putpages },		/* putpages */
	{ NULL, NULL }
};
const struct vnodeopv_desc lfs_vnodeop_opv_desc =
	{ &lfs_vnodeop_p, lfs_vnodeop_entries };

int (**lfs_specop_p)(void *);
const struct vnodeopv_entry_desc lfs_specop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, spec_lookup },		/* lookup */
	{ &vop_create_desc, spec_create },		/* create */
	{ &vop_mknod_desc, spec_mknod },		/* mknod */
	{ &vop_open_desc, spec_open },			/* open */
	{ &vop_close_desc, lfsspec_close },		/* close */
	{ &vop_access_desc, ufs_access },		/* access */
	{ &vop_getattr_desc, lfs_getattr },		/* getattr */
	{ &vop_setattr_desc, lfs_setattr },		/* setattr */
	{ &vop_read_desc, ufsspec_read },		/* read */
	{ &vop_write_desc, ufsspec_write },		/* write */
	{ &vop_lease_desc, spec_lease_check },		/* lease */
	{ &vop_ioctl_desc, spec_ioctl },		/* ioctl */
	{ &vop_fcntl_desc, ufs_fcntl },			/* fcntl */
	{ &vop_poll_desc, spec_poll },			/* poll */
	{ &vop_kqfilter_desc, spec_kqfilter },		/* kqfilter */
	{ &vop_revoke_desc, spec_revoke },		/* revoke */
	{ &vop_mmap_desc, spec_mmap },			/* mmap */
	{ &vop_fsync_desc, spec_fsync },		/* fsync */
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
	{ &vop_inactive_desc, lfs_inactive },		/* inactive */
	{ &vop_reclaim_desc, lfs_reclaim },		/* reclaim */
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
	{ &vop_vfree_desc, lfs_vfree },			/* vfree */
	{ &vop_truncate_desc, spec_truncate },		/* truncate */
	{ &vop_update_desc, lfs_update },		/* update */
	{ &vop_bwrite_desc, vn_bwrite },		/* bwrite */
	{ &vop_getpages_desc, spec_getpages },		/* getpages */
	{ &vop_putpages_desc, spec_putpages },		/* putpages */
	{ NULL, NULL }
};
const struct vnodeopv_desc lfs_specop_opv_desc =
	{ &lfs_specop_p, lfs_specop_entries };

int (**lfs_fifoop_p)(void *);
const struct vnodeopv_entry_desc lfs_fifoop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, fifo_lookup },		/* lookup */
	{ &vop_create_desc, fifo_create },		/* create */
	{ &vop_mknod_desc, fifo_mknod },		/* mknod */
	{ &vop_open_desc, fifo_open },			/* open */
	{ &vop_close_desc, lfsfifo_close },		/* close */
	{ &vop_access_desc, ufs_access },		/* access */
	{ &vop_getattr_desc, lfs_getattr },		/* getattr */
	{ &vop_setattr_desc, lfs_setattr },		/* setattr */
	{ &vop_read_desc, ufsfifo_read },		/* read */
	{ &vop_write_desc, ufsfifo_write },		/* write */
	{ &vop_lease_desc, fifo_lease_check },		/* lease */
	{ &vop_ioctl_desc, fifo_ioctl },		/* ioctl */
	{ &vop_fcntl_desc, ufs_fcntl },			/* fcntl */
	{ &vop_poll_desc, fifo_poll },			/* poll */
	{ &vop_kqfilter_desc, fifo_kqfilter },		/* kqfilter */
	{ &vop_revoke_desc, fifo_revoke },		/* revoke */
	{ &vop_mmap_desc, fifo_mmap },			/* mmap */
	{ &vop_fsync_desc, fifo_fsync },		/* fsync */
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
	{ &vop_inactive_desc, lfs_inactive },		/* inactive */
	{ &vop_reclaim_desc, lfs_reclaim },		/* reclaim */
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
	{ &vop_vfree_desc, lfs_vfree },			/* vfree */
	{ &vop_truncate_desc, fifo_truncate },		/* truncate */
	{ &vop_update_desc, lfs_update },		/* update */
	{ &vop_bwrite_desc, lfs_bwrite },		/* bwrite */
	{ &vop_putpages_desc, fifo_putpages },		/* putpages */
	{ NULL, NULL }
};
const struct vnodeopv_desc lfs_fifoop_opv_desc =
	{ &lfs_fifoop_p, lfs_fifoop_entries };

/*
 * A function version of LFS_ITIMES, for the UFS functions which call ITIMES
 */
void
lfs_itimes(struct inode *ip, struct timespec *acc, struct timespec *mod, struct timespec *cre)
{
	LFS_ITIMES(ip, acc, mod, cre);
}

#define	LFS_READWRITE
#include <ufs/ufs/ufs_readwrite.c>
#undef	LFS_READWRITE

/*
 * Synch an open file.
 */
/* ARGSUSED */
int
lfs_fsync(void *v)
{
	struct vop_fsync_args /* {
		struct vnode *a_vp;
		struct ucred *a_cred;
		int a_flags;
		off_t offlo;
		off_t offhi;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	int error, wait;

	/*
	 * Trickle sync checks for need to do a checkpoint after possible
	 * activity from the pagedaemon.
	 */
	if (ap->a_flags & FSYNC_LAZY) {
		wakeup(&lfs_writer_daemon);
		return 0;
	}

	wait = (ap->a_flags & FSYNC_WAIT);
	do {
#ifdef DEBUG
		struct buf *bp;
#endif

		simple_lock(&vp->v_interlock);
		error = VOP_PUTPAGES(vp, trunc_page(ap->a_offlo),
				round_page(ap->a_offhi),
				PGO_CLEANIT | (wait ? PGO_SYNCIO : 0));
		if (error)
			return error;
		error = VOP_UPDATE(vp, NULL, NULL, wait ? UPDATE_WAIT : 0);
		if (wait && error == 0 && !VPISEMPTY(vp)) {
#ifdef DEBUG
			printf("lfs_fsync: reflushing ino %d\n",
				VTOI(vp)->i_number);
			printf("vflags %x iflags %x npages %d\n",
				vp->v_flag, VTOI(vp)->i_flag,
				vp->v_uobj.uo_npages);
			LIST_FOREACH(bp, &vp->v_dirtyblkhd, b_vnbufs)
				printf("%" PRId64 " (%lx)", bp->b_lblkno,
					bp->b_flags);
			printf("\n");
#endif
			LFS_SET_UINO(VTOI(vp), IN_MODIFIED);
		}
	} while (wait && error == 0 && !VPISEMPTY(vp));

	return error;
}

/*
 * Take IN_ADIROP off, then call ufs_inactive.
 */
int
lfs_inactive(void *v)
{
	struct vop_inactive_args /* {
		struct vnode *a_vp;
		struct proc *a_p;
	} */ *ap = v;

	KASSERT(VTOI(ap->a_vp)->i_nlink == VTOI(ap->a_vp)->i_ffs_effnlink);

	lfs_unmark_vnode(ap->a_vp);

	/*
	 * The Ifile is only ever inactivated on unmount.
	 * Streamline this process by not giving it more dirty blocks.
	 */
	if (VTOI(ap->a_vp)->i_number == LFS_IFILE_INUM) {
		LFS_CLR_UINO(VTOI(ap->a_vp), IN_ALLMOD);
		VOP_UNLOCK(ap->a_vp, 0);
		return 0;
	}

	return ufs_inactive(v);
}

/*
 * These macros are used to bracket UFS directory ops, so that we can
 * identify all the pages touched during directory ops which need to
 * be ordered and flushed atomically, so that they may be recovered.
 */
/*
 * XXX KS - Because we have to mark nodes VDIROP in order to prevent
 * the cache from reclaiming them while a dirop is in progress, we must
 * also manage the number of nodes so marked (otherwise we can run out).
 * We do this by setting lfs_dirvcount to the number of marked vnodes; it
 * is decremented during segment write, when VDIROP is taken off.
 */
#define	SET_DIROP(vp)		SET_DIROP2((vp), NULL)
#define	SET_DIROP2(vp, vp2)	lfs_set_dirop((vp), (vp2))
static int lfs_set_dirop(struct vnode *, struct vnode *);
extern int lfs_dirvcount;
extern int lfs_do_flush;

#define	NRESERVE(fs)	(btofsb(fs, (NIADDR + 3 + (2 * NIADDR + 3)) << fs->lfs_bshift))

static int
lfs_set_dirop(struct vnode *vp, struct vnode *vp2)
{
	struct lfs *fs;
	int error;

	KASSERT(VOP_ISLOCKED(vp));
	KASSERT(vp2 == NULL || VOP_ISLOCKED(vp2));

	fs = VTOI(vp)->i_lfs;
	/*
	 * We might need one directory block plus supporting indirect blocks,
	 * plus an inode block and ifile page for the new vnode.
	 */
	if ((error = lfs_reserve(fs, vp, vp2, NRESERVE(fs))) != 0)
		return (error);

	if (fs->lfs_dirops == 0)
		lfs_check(vp, LFS_UNUSED_LBN, 0);
	while (fs->lfs_writer || lfs_dirvcount > LFS_MAX_DIROP) {
		if (fs->lfs_writer)
			tsleep(&fs->lfs_dirops, PRIBIO + 1, "lfs_sdirop", 0);
		if (lfs_dirvcount > LFS_MAX_DIROP && fs->lfs_dirops == 0) {
			wakeup(&lfs_writer_daemon);
			preempt(NULL);
		}

		if (lfs_dirvcount > LFS_MAX_DIROP) {
#ifdef DEBUG_LFS
			printf("lfs_set_dirop: sleeping with dirops=%d, "
			       "dirvcount=%d\n", fs->lfs_dirops,
			       lfs_dirvcount); 
#endif
			if ((error = tsleep(&lfs_dirvcount, PCATCH|PUSER,
					   "lfs_maxdirop", 0)) != 0) {
				goto unreserve;
			}
		}							
	}								
	++fs->lfs_dirops;						
	fs->lfs_doifile = 1;						

	/* Hold a reference so SET_ENDOP will be happy */
	vref(vp);
	if (vp2)
		vref(vp2);

	return 0;

unreserve:
	lfs_reserve(fs, vp, vp2, -NRESERVE(fs));
	return error;
}

#define	SET_ENDOP(fs, vp, str)	SET_ENDOP2((fs), (vp), NULL, (str))
#define	SET_ENDOP2(fs, vp, vp2, str) {					\
	--(fs)->lfs_dirops;						\
	if (!(fs)->lfs_dirops) {					\
		if ((fs)->lfs_nadirop) {				\
			panic("SET_ENDOP: %s: no dirops but nadirop=%d", \
			      (str), (fs)->lfs_nadirop);		\
		}							\
		wakeup(&(fs)->lfs_writer);				\
		lfs_check((vp),LFS_UNUSED_LBN,0);			\
	}								\
	lfs_reserve((fs), vp, vp2, -NRESERVE(fs)); /* XXX */		\
	vrele(vp);							\
	if (vp2)							\
		vrele(vp2);						\
}

#define	MARK_VNODE(dvp)	 do {						\
	struct inode *_ip = VTOI(dvp);					\
	struct lfs *_fs = _ip->i_lfs;					\
									\
	if (!((dvp)->v_flag & VDIROP)) {				\
		(void)lfs_vref(dvp);					\
		++lfs_dirvcount;					\
		TAILQ_INSERT_TAIL(&_fs->lfs_dchainhd, _ip, i_lfs_dchain); \
	}								\
	(dvp)->v_flag |= VDIROP;					\
	if (!(_ip->i_flag & IN_ADIROP)) {				\
		++_fs->lfs_nadirop;					\
	}								\
	_ip->i_flag |= IN_ADIROP;					\
} while (0)

#define UNMARK_VNODE(vp) lfs_unmark_vnode(vp)

void lfs_unmark_vnode(struct vnode *vp)
{
	struct inode *ip;

	ip = VTOI(vp);

	if (ip->i_flag & IN_ADIROP)
		--ip->i_lfs->lfs_nadirop;
	ip->i_flag &= ~IN_ADIROP;
}

int
lfs_symlink(void *v)
{
	struct vop_symlink_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
		char *a_target;
	} */ *ap = v;
	int error;

	if ((error = SET_DIROP(ap->a_dvp)) != 0) {
		vput(ap->a_dvp);
		return error;
	}
	MARK_VNODE(ap->a_dvp);
	error = ufs_symlink(ap);
	UNMARK_VNODE(ap->a_dvp);
	if (*(ap->a_vpp))
		UNMARK_VNODE(*(ap->a_vpp));
	SET_ENDOP(VTOI(ap->a_dvp)->i_lfs,ap->a_dvp,"symlink");
	return (error);
}

int
lfs_mknod(void *v)
{
	struct vop_mknod_args	/* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
		} */ *ap = v;
	struct vattr *vap = ap->a_vap;
	struct vnode **vpp = ap->a_vpp;
	struct inode *ip;
	int error;
	struct mount	*mp;	
	ino_t		ino;

	if ((error = SET_DIROP(ap->a_dvp)) != 0) {
		vput(ap->a_dvp);
		return error;
	}
	MARK_VNODE(ap->a_dvp);
	error = ufs_makeinode(MAKEIMODE(vap->va_type, vap->va_mode),
	    ap->a_dvp, vpp, ap->a_cnp);
	UNMARK_VNODE(ap->a_dvp);
	if (*(ap->a_vpp))
		UNMARK_VNODE(*(ap->a_vpp));

	/* Either way we're done with the dirop at this point */
	SET_ENDOP(VTOI(ap->a_dvp)->i_lfs,ap->a_dvp,"mknod");

	if (error)
		return (error);

	ip = VTOI(*vpp);
	mp  = (*vpp)->v_mount;
	ino = ip->i_number;
	ip->i_flag |= IN_ACCESS | IN_CHANGE | IN_UPDATE;
	if (vap->va_rdev != VNOVAL) {
		/*
		 * Want to be able to use this to make badblock
		 * inodes, so don't truncate the dev number.
		 */
#if 0
		ip->i_ffs1_rdev = ufs_rw32(vap->va_rdev,
		    UFS_MPNEEDSWAP((*vpp)->v_mount));
#else
		ip->i_ffs1_rdev = vap->va_rdev;
#endif
	}
	/*
	 * Call fsync to write the vnode so that we don't have to deal with
	 * flushing it when it's marked VDIROP|VXLOCK.
	 *
	 * XXX KS - If we can't flush we also can't call vgone(), so must
	 * return.  But, that leaves this vnode in limbo, also not good.
	 * Can this ever happen (barring hardware failure)?
	 */
	if ((error = VOP_FSYNC(*vpp, NOCRED, FSYNC_WAIT, 0, 0, 
	    curproc)) != 0) {
		printf("Couldn't fsync in mknod (ino %d)---what do I do?\n",
		       VTOI(*vpp)->i_number);
		return (error);
	}
	/*
	 * Remove vnode so that it will be reloaded by VFS_VGET and
	 * checked to see if it is an alias of an existing entry in
	 * the inode cache.
	 */
	/* Used to be vput, but that causes us to call VOP_INACTIVE twice. */
	VOP_UNLOCK(*vpp, 0);
	lfs_vunref(*vpp);
	(*vpp)->v_type = VNON;
	vgone(*vpp);
	error = VFS_VGET(mp, ino, vpp);
	if (error != 0) {
		*vpp = NULL;
		return (error);
	}
	return (0);
}

int
lfs_create(void *v)
{
	struct vop_create_args	/* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap = v;
	int error;

	if ((error = SET_DIROP(ap->a_dvp)) != 0) {
		vput(ap->a_dvp);
		return error;
	}
	MARK_VNODE(ap->a_dvp);
	error = ufs_create(ap);
	UNMARK_VNODE(ap->a_dvp);
	if (*(ap->a_vpp))
		UNMARK_VNODE(*(ap->a_vpp));
	SET_ENDOP(VTOI(ap->a_dvp)->i_lfs,ap->a_dvp,"create");
	return (error);
}

int
lfs_mkdir(void *v)
{
	struct vop_mkdir_args	/* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap = v;
	int error;

	if ((error = SET_DIROP(ap->a_dvp)) != 0) {
		vput(ap->a_dvp);
		return error;
	}
	MARK_VNODE(ap->a_dvp);
	error = ufs_mkdir(ap);
	UNMARK_VNODE(ap->a_dvp);
	if (*(ap->a_vpp))
		UNMARK_VNODE(*(ap->a_vpp));
	SET_ENDOP(VTOI(ap->a_dvp)->i_lfs,ap->a_dvp,"mkdir");
	return (error);
}

int
lfs_remove(void *v)
{
	struct vop_remove_args	/* {
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap = v;
	struct vnode *dvp, *vp;
	int error;

	dvp = ap->a_dvp;
	vp = ap->a_vp;
	if ((error = SET_DIROP2(dvp, vp)) != 0) {
		if (dvp == vp)
			vrele(vp);
		else
			vput(vp);
		vput(dvp);
		return error;
	}
	MARK_VNODE(dvp);
	MARK_VNODE(vp);
	error = ufs_remove(ap);
	UNMARK_VNODE(dvp);
	UNMARK_VNODE(vp);

	SET_ENDOP2(VTOI(dvp)->i_lfs, dvp, vp, "remove");
	return (error);
}

int
lfs_rmdir(void *v)
{
	struct vop_rmdir_args	/* {
		struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap = v;
	struct vnode *vp;
	int error;

	vp = ap->a_vp;
	if ((error = SET_DIROP2(ap->a_dvp, ap->a_vp)) != 0) {
		vrele(ap->a_dvp);
		if (ap->a_vp != ap->a_dvp)
			VOP_UNLOCK(ap->a_dvp, 0);
		vput(vp);
		return error;
	}
	MARK_VNODE(ap->a_dvp);
	MARK_VNODE(vp);
	error = ufs_rmdir(ap);
	UNMARK_VNODE(ap->a_dvp);
	UNMARK_VNODE(vp);

	SET_ENDOP2(VTOI(ap->a_dvp)->i_lfs, ap->a_dvp, vp, "rmdir");
	return (error);
}

int
lfs_link(void *v)
{
	struct vop_link_args	/* {
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap = v;
	int error;

	if ((error = SET_DIROP(ap->a_dvp)) != 0) {
		vput(ap->a_dvp);
		return error;
	}
	MARK_VNODE(ap->a_dvp);
	error = ufs_link(ap);
	UNMARK_VNODE(ap->a_dvp);
	SET_ENDOP(VTOI(ap->a_dvp)->i_lfs,ap->a_dvp,"link");
	return (error);
}

int
lfs_rename(void *v)
{
	struct vop_rename_args	/* {
		struct vnode *a_fdvp;
		struct vnode *a_fvp;
		struct componentname *a_fcnp;
		struct vnode *a_tdvp;
		struct vnode *a_tvp;
		struct componentname *a_tcnp;
	} */ *ap = v;
	struct vnode *tvp, *fvp, *tdvp, *fdvp;
	struct componentname *tcnp, *fcnp;
	int error;
	struct lfs *fs;

	fs = VTOI(ap->a_fdvp)->i_lfs;
	tvp = ap->a_tvp;
	tdvp = ap->a_tdvp;
	tcnp = ap->a_tcnp;
	fvp = ap->a_fvp;
	fdvp = ap->a_fdvp;
	fcnp = ap->a_fcnp;

	/*
	 * Check for cross-device rename.
	 * If it is, we don't want to set dirops, just error out.
	 * (In particular note that MARK_VNODE(tdvp) will DTWT on
	 * a cross-device rename.)
	 *
	 * Copied from ufs_rename.
	 */
	if ((fvp->v_mount != tdvp->v_mount) ||
	    (tvp && (fvp->v_mount != tvp->v_mount))) {
		error = EXDEV;
		goto errout;
	}

	/*
	 * Check to make sure we're not renaming a vnode onto itself
	 * (deleting a hard link by renaming one name onto another);
	 * if we are we can't recursively call VOP_REMOVE since that
	 * would leave us with an unaccounted-for number of live dirops.
	 *
	 * Inline the relevant section of ufs_rename here, *before*
	 * calling SET_DIROP2.
	 */
	if (tvp && ((VTOI(tvp)->i_flags & (IMMUTABLE | APPEND)) ||
	    (VTOI(tdvp)->i_flags & APPEND))) {
		error = EPERM;
		goto errout;
	}
	if (fvp == tvp) {
		if (fvp->v_type == VDIR) {
			error = EINVAL;
			goto errout;
		}

		/* Release destination completely. */
		VOP_ABORTOP(tdvp, tcnp);
		vput(tdvp);
		vput(tvp);

		/* Delete source. */
		vrele(fvp);
		fcnp->cn_flags &= ~(MODMASK | SAVESTART);
		fcnp->cn_flags |= LOCKPARENT | LOCKLEAF;
		fcnp->cn_nameiop = DELETE;
		if ((error = relookup(fdvp, &fvp, fcnp))){
			/* relookup blew away fdvp */
			return (error);
		}
		return (VOP_REMOVE(fdvp, fvp, fcnp));
	}

	if ((error = SET_DIROP2(tdvp, tvp)) != 0)
		goto errout;
	MARK_VNODE(fdvp);
	MARK_VNODE(tdvp);
	MARK_VNODE(fvp);
	if (tvp) {
		MARK_VNODE(tvp);
	}
	
	error = ufs_rename(ap);
	UNMARK_VNODE(fdvp);
	UNMARK_VNODE(tdvp);
	UNMARK_VNODE(fvp);
	if (tvp) {
		UNMARK_VNODE(tvp);
	}
	SET_ENDOP2(fs, tdvp, tvp, "rename");
	return (error);

    errout:
	VOP_ABORTOP(tdvp, ap->a_tcnp); /* XXX, why not in NFS? */
	if (tdvp == tvp)
		vrele(tdvp);
	else
		vput(tdvp);
	if (tvp)
		vput(tvp);
	VOP_ABORTOP(fdvp, ap->a_fcnp); /* XXX, why not in NFS? */
	vrele(fdvp);
	vrele(fvp);
	return (error);
}

/* XXX hack to avoid calling ITIMES in getattr */
int
lfs_getattr(void *v)
{
	struct vop_getattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct inode *ip = VTOI(vp);
	struct vattr *vap = ap->a_vap;
	struct lfs *fs = ip->i_lfs;
	/*
	 * Copy from inode table
	 */
	vap->va_fsid = ip->i_dev;
	vap->va_fileid = ip->i_number;
	vap->va_mode = ip->i_mode & ~IFMT;
	vap->va_nlink = ip->i_nlink;
	vap->va_uid = ip->i_uid;
	vap->va_gid = ip->i_gid;
	vap->va_rdev = (dev_t)ip->i_ffs1_rdev;
	vap->va_size = vp->v_size;
	vap->va_atime.tv_sec = ip->i_ffs1_atime;
	vap->va_atime.tv_nsec = ip->i_ffs1_atimensec;
	vap->va_mtime.tv_sec = ip->i_ffs1_mtime;
	vap->va_mtime.tv_nsec = ip->i_ffs1_mtimensec;
	vap->va_ctime.tv_sec = ip->i_ffs1_ctime;
	vap->va_ctime.tv_nsec = ip->i_ffs1_ctimensec;
	vap->va_flags = ip->i_flags;
	vap->va_gen = ip->i_gen;
	/* this doesn't belong here */
	if (vp->v_type == VBLK)
		vap->va_blocksize = BLKDEV_IOSIZE;
	else if (vp->v_type == VCHR)
		vap->va_blocksize = MAXBSIZE;
	else
		vap->va_blocksize = vp->v_mount->mnt_stat.f_iosize;
	vap->va_bytes = fsbtob(fs, (u_quad_t)ip->i_lfs_effnblks);
	vap->va_type = vp->v_type;
	vap->va_filerev = ip->i_modrev;
	return (0);
}

/*
 * Check to make sure the inode blocks won't choke the buffer
 * cache, then call ufs_setattr as usual.
 */
int
lfs_setattr(void *v)
{
	struct vop_getattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	lfs_check(vp, LFS_UNUSED_LBN, 0);
	return ufs_setattr(v);
}

/*
 * Close called
 *
 * XXX -- we were using ufs_close, but since it updates the
 * times on the inode, we might need to bump the uinodes
 * count.
 */
/* ARGSUSED */
int
lfs_close(void *v)
{
	struct vop_close_args /* {
		struct vnode *a_vp;
		int  a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct inode *ip = VTOI(vp);
	struct timespec ts;

	if (vp == ip->i_lfs->lfs_ivnode &&
	    vp->v_mount->mnt_flag & MNT_UNMOUNT)
		return 0;

	if (vp->v_usecount > 1 && vp != ip->i_lfs->lfs_ivnode) {
		TIMEVAL_TO_TIMESPEC(&time, &ts);
		LFS_ITIMES(ip, &ts, &ts, &ts);
	}
	return (0);
}

/*
 * Close wrapper for special devices.
 *
 * Update the times on the inode then do device close.
 */
int
lfsspec_close(void *v)
{
	struct vop_close_args /* {
		struct vnode	*a_vp;
		int		a_fflag;
		struct ucred	*a_cred;
		struct proc	*a_p;
	} */ *ap = v;
	struct vnode	*vp;
	struct inode	*ip;
	struct timespec	ts;

	vp = ap->a_vp;
	ip = VTOI(vp);
	if (vp->v_usecount > 1) {
		TIMEVAL_TO_TIMESPEC(&time, &ts);
		LFS_ITIMES(ip, &ts, &ts, &ts);
	}
	return (VOCALL (spec_vnodeop_p, VOFFSET(vop_close), ap));
}

/*
 * Close wrapper for fifo's.
 *
 * Update the times on the inode then do device close.
 */
int
lfsfifo_close(void *v)
{
	struct vop_close_args /* {
		struct vnode	*a_vp;
		int		a_fflag;
		struct ucred	*a_cred;
		struct proc	*a_p;
	} */ *ap = v;
	struct vnode	*vp;
	struct inode	*ip;
	struct timespec	ts;

	vp = ap->a_vp;
	ip = VTOI(vp);
	if (ap->a_vp->v_usecount > 1) {
		TIMEVAL_TO_TIMESPEC(&time, &ts);
		LFS_ITIMES(ip, &ts, &ts, &ts);
	}
	return (VOCALL (fifo_vnodeop_p, VOFFSET(vop_close), ap));
}

/*
 * Reclaim an inode so that it can be used for other purposes.
 */
int lfs_no_inactive = 0;

int
lfs_reclaim(void *v)
{
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct inode *ip = VTOI(vp);
	int error;

	KASSERT(ip->i_nlink == ip->i_ffs_effnlink);

	LFS_CLR_UINO(ip, IN_ALLMOD);
	if ((error = ufs_reclaim(vp, ap->a_p)))
		return (error);
	pool_put(&lfs_dinode_pool, VTOI(vp)->i_din.ffs1_din);
	pool_put(&lfs_inoext_pool, ip->inode_ext.lfs);
	ip->inode_ext.lfs = NULL;
	pool_put(&lfs_inode_pool, vp->v_data);
	vp->v_data = NULL;
	return (0);
}

/*
 * Read a block from a storage device.
 * In order to avoid reading blocks that are in the process of being
 * written by the cleaner---and hence are not mutexed by the normal
 * buffer cache / page cache mechanisms---check for collisions before
 * reading.
 *
 * We inline ufs_strategy to make sure that the VOP_BMAP occurs *before*
 * the active cleaner test.
 *
 * XXX This code assumes that lfs_markv makes synchronous checkpoints.
 */
int
lfs_strategy(void *v)
{
	struct vop_strategy_args /* {
		struct buf *a_bp;
	} */ *ap = v;
	struct buf	*bp;
	struct lfs	*fs;
	struct vnode	*vp;
	struct inode	*ip;
	daddr_t		tbn;
	int		i, sn, error, slept;

	bp = ap->a_bp;
	vp = bp->b_vp;
	ip = VTOI(vp);
	fs = ip->i_lfs;

	/* lfs uses its strategy routine only for read */
	KASSERT(bp->b_flags & B_READ);

	if (vp->v_type == VBLK || vp->v_type == VCHR)
		panic("lfs_strategy: spec");
	KASSERT(bp->b_bcount != 0);
	if (bp->b_blkno == bp->b_lblkno) {
		error = VOP_BMAP(vp, bp->b_lblkno, NULL, &bp->b_blkno,
				 NULL);
		if (error) {
			bp->b_error = error;
			bp->b_flags |= B_ERROR;
			biodone(bp);
			return (error);
		}
		if ((long)bp->b_blkno == -1) /* no valid data */
			clrbuf(bp);
	}
	if ((long)bp->b_blkno < 0) { /* block is not on disk */
		biodone(bp);
		return (0);
	}

	slept = 1;
	simple_lock(&fs->lfs_interlock);
	while (slept && fs->lfs_seglock) {
		simple_unlock(&fs->lfs_interlock);
		/*
		 * Look through list of intervals.
		 * There will only be intervals to look through
		 * if the cleaner holds the seglock.
		 * Since the cleaner is synchronous, we can trust
		 * the list of intervals to be current.
		 */
		tbn = dbtofsb(fs, bp->b_blkno);
		sn = dtosn(fs, tbn);
		slept = 0;
		for (i = 0; i < fs->lfs_cleanind; i++) {
			if (sn == dtosn(fs, fs->lfs_cleanint[i]) &&
			    tbn >= fs->lfs_cleanint[i]) {
#ifdef DEBUG_LFS
				printf("lfs_strategy: ino %d lbn %" PRId64
				       " ind %d sn %d fsb %" PRIx32
				       " given sn %d fsb %" PRIx64 "\n",
					ip->i_number, bp->b_lblkno, i,
					dtosn(fs, fs->lfs_cleanint[i]),
					fs->lfs_cleanint[i], sn, tbn);
				printf("lfs_strategy: sleeping on ino %d lbn %"
				       PRId64 "\n", ip->i_number, bp->b_lblkno);
#endif
				tsleep(&fs->lfs_seglock, PRIBIO+1,
					"lfs_strategy", 0);
				/* Things may be different now; start over. */
				slept = 1;
				break;
			}
		}
		simple_lock(&fs->lfs_interlock);
	}
	simple_unlock(&fs->lfs_interlock);

	vp = ip->i_devvp;
	bp->b_dev = vp->v_rdev;
	VOCALL (vp->v_op, VOFFSET(vop_strategy), ap);
	return (0);
}

static void
lfs_flush_dirops(struct lfs *fs)
{
	struct inode *ip, *nip;
	struct vnode *vp;
	extern int lfs_dostats;
	struct segment *sp;
	int needunlock;

	if (fs->lfs_ronly)
		return;

	if (TAILQ_FIRST(&fs->lfs_dchainhd) == NULL)
		return;

	/* XXX simplelock fs->lfs_dirops */
	while (fs->lfs_dirops > 0) {
		++fs->lfs_diropwait;  
		tsleep(&fs->lfs_writer, PRIBIO+1, "pndirop", 0);
		--fs->lfs_diropwait; 
	}
	/* disallow dirops during flush */
	fs->lfs_writer++;

	if (lfs_dostats)
		++lfs_stats.flush_invoked;

	/*
	 * Inline lfs_segwrite/lfs_writevnodes, but just for dirops.
	 * Technically this is a checkpoint (the on-disk state is valid)
	 * even though we are leaving out all the file data.
	 */
	lfs_imtime(fs);
	lfs_seglock(fs, SEGM_CKP);
	sp = fs->lfs_sp;

	/*
	 * lfs_writevnodes, optimized to get dirops out of the way.
	 * Only write dirops, and don't flush files' pages, only
	 * blocks from the directories.
	 *
	 * We don't need to vref these files because they are
	 * dirops and so hold an extra reference until the
	 * segunlock clears them of that status.
	 *
	 * We don't need to check for IN_ADIROP because we know that
	 * no dirops are active.
	 *
	 */
	for (ip = TAILQ_FIRST(&fs->lfs_dchainhd); ip != NULL; ip = nip) {
		nip = TAILQ_NEXT(ip, i_lfs_dchain);
		vp = ITOV(ip);

		/*
		 * All writes to directories come from dirops; all
		 * writes to files' direct blocks go through the page
		 * cache, which we're not touching.  Reads to files
		 * and/or directories will not be affected by writing
		 * directory blocks inodes and file inodes.  So we don't
		 * really need to lock.  If we don't lock, though,
		 * make sure that we don't clear IN_MODIFIED
		 * unnecessarily.
		 */
		if (vp->v_flag & VXLOCK)
			continue;
		if (vn_lock(vp, LK_EXCLUSIVE | LK_CANRECURSE |
			    LK_NOWAIT) == 0) {
			needunlock = 1;
		} else {
			printf("lfs_flush_dirops: flushing locked ino %d\n", 
			       VTOI(vp)->i_number);
			needunlock = 0;
		}
		if (vp->v_type != VREG &&
		    ((ip->i_flag & IN_ALLMOD) || !VPISEMPTY(vp))) {
			lfs_writefile(fs, sp, vp);
			if (!VPISEMPTY(vp) && !WRITEINPROG(vp) &&
			    !(ip->i_flag & IN_ALLMOD)) {
				LFS_SET_UINO(ip, IN_MODIFIED);
			}
		}
		(void) lfs_writeinode(fs, sp, ip);
		if (needunlock)
			VOP_UNLOCK(vp, 0);
		else
			LFS_SET_UINO(ip, IN_MODIFIED);
	}
	/* We've written all the dirops there are */
	((SEGSUM *)(sp->segsum))->ss_flags &= ~(SS_CONT);
	(void) lfs_writeseg(fs, sp);
	lfs_segunlock(fs);
	
	if (--fs->lfs_writer == 0)
		wakeup(&fs->lfs_dirops);
}

/*
 * Provide a fcntl interface to sys_lfs_{segwait,bmapv,markv}.
 */
int
lfs_fcntl(void *v)
{
        struct vop_fcntl_args /* {
                struct vnode *a_vp;
                u_long a_command;
                caddr_t  a_data;
                int  a_fflag;
                struct ucred *a_cred;
                struct proc *a_p;
        } */ *ap = v;
	struct timeval *tvp;
	BLOCK_INFO *blkiov;
	CLEANERINFO *cip;
	int blkcnt, error, oclean;
	struct lfs_fcntl_markv blkvp;
	fsid_t *fsidp;
	struct lfs *fs;
	struct buf *bp;
	daddr_t off;

	/* Only respect LFS fcntls on fs root or Ifile */
	if (VTOI(ap->a_vp)->i_number != ROOTINO &&
	    VTOI(ap->a_vp)->i_number != LFS_IFILE_INUM) {
		return ufs_fcntl(v);
	}

	/* Avoid locking a draining lock */
	if (ap->a_vp->v_mount->mnt_flag & MNT_UNMOUNT) {
		return ESHUTDOWN;
	}

	fs = VTOI(ap->a_vp)->i_lfs;
	fsidp = &ap->a_vp->v_mount->mnt_stat.f_fsid;

	switch (ap->a_command) {
	    case LFCNSEGWAITALL:
		fsidp = NULL;
		/* FALLSTHROUGH */
	    case LFCNSEGWAIT:
		tvp = (struct timeval *)ap->a_data;
		simple_lock(&fs->lfs_interlock);
		++fs->lfs_sleepers;
		simple_unlock(&fs->lfs_interlock);
		VOP_UNLOCK(ap->a_vp, 0);

		error = lfs_segwait(fsidp, tvp);

		VOP_LOCK(ap->a_vp, LK_EXCLUSIVE);
		simple_lock(&fs->lfs_interlock);
		if (--fs->lfs_sleepers == 0)
			wakeup(&fs->lfs_sleepers);
		simple_unlock(&fs->lfs_interlock);
		return error;

	    case LFCNBMAPV:
	    case LFCNMARKV:
		if ((error = suser(ap->a_p->p_ucred, &ap->a_p->p_acflag)) != 0)
			return (error);
		blkvp = *(struct lfs_fcntl_markv *)ap->a_data;

		blkcnt = blkvp.blkcnt;
		if ((u_int) blkcnt > LFS_MARKV_MAXBLKCNT)
			return (EINVAL);
		blkiov = malloc(blkcnt * sizeof(BLOCK_INFO), M_SEGMENT, M_WAITOK);
		if ((error = copyin(blkvp.blkiov, blkiov,
		     blkcnt * sizeof(BLOCK_INFO))) != 0) {
			free(blkiov, M_SEGMENT);
			return error;
		}

		simple_lock(&fs->lfs_interlock);
		++fs->lfs_sleepers;
		simple_unlock(&fs->lfs_interlock);
		VOP_UNLOCK(ap->a_vp, 0);
		if (ap->a_command == LFCNBMAPV)
			error = lfs_bmapv(ap->a_p, fsidp, blkiov, blkcnt);
		else /* LFCNMARKV */
			error = lfs_markv(ap->a_p, fsidp, blkiov, blkcnt);
		if (error == 0)
			error = copyout(blkiov, blkvp.blkiov,
					blkcnt * sizeof(BLOCK_INFO));
		VOP_LOCK(ap->a_vp, LK_EXCLUSIVE);
		simple_lock(&fs->lfs_interlock);
		if (--fs->lfs_sleepers == 0)
			wakeup(&fs->lfs_sleepers);
		simple_unlock(&fs->lfs_interlock);
		free(blkiov, M_SEGMENT);
		return error;

	    case LFCNRECLAIM:
		/*
		 * Flush dirops and write Ifile, allowing empty segments
		 * to be immediately reclaimed.
		 */
		off = fs->lfs_offset;
		lfs_seglock(fs, SEGM_FORCE_CKP | SEGM_CKP);
		lfs_flush_dirops(fs);
		LFS_CLEANERINFO(cip, fs, bp);
		oclean = cip->clean;
		LFS_SYNC_CLEANERINFO(cip, fs, bp, 1);
		lfs_segwrite(ap->a_vp->v_mount, SEGM_FORCE_CKP);
		lfs_segunlock(fs);

#ifdef DEBUG_LFS
		LFS_CLEANERINFO(cip, fs, bp);
		oclean = cip->clean;
		printf("lfs_fcntl: reclaim wrote %" PRId64 " blocks, cleaned "
			"%" PRId32 " segments (activesb %d)\n",
			fs->lfs_offset - off, cip->clean - oclean,
			fs->lfs_activesb);
		LFS_SYNC_CLEANERINFO(cip, fs, bp, 0);
#endif

		return 0;

	    default:
		return ufs_fcntl(v);
	}
	return 0;
}

int
lfs_getpages(void *v)
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

	if (VTOI(ap->a_vp)->i_number == LFS_IFILE_INUM &&
	    (ap->a_access_type & VM_PROT_WRITE) != 0) {
		return EPERM;
	}
	if ((ap->a_access_type & VM_PROT_WRITE) != 0) {
		LFS_SET_UINO(VTOI(ap->a_vp), IN_MODIFIED);
	}
	return genfs_getpages(v);
}

/*
 * Make sure that for all pages in every block in the given range,
 * either all are dirty or all are clean.  If any of the pages
 * we've seen so far are dirty, put the vnode on the paging chain,
 * and mark it IN_PAGING.
 */
static int
check_dirty(struct lfs *fs, struct vnode *vp,
	    off_t startoffset, off_t endoffset, off_t blkeof,
	    int flags)
{
	int by_list;
	struct vm_page *curpg, *pgs[MAXBSIZE / PAGE_SIZE], *pg;
	struct lwp *l = curlwp ? curlwp : &lwp0;
	off_t soff;
	voff_t off;
	int i, dirty, tdirty, nonexistent, any_dirty;
	int pages_per_block = fs->lfs_bsize >> PAGE_SHIFT;

  top:
	by_list = (vp->v_uobj.uo_npages <=
		   ((endoffset - startoffset) >> PAGE_SHIFT) *
		   UVM_PAGE_HASH_PENALTY);
	any_dirty = 0;

	if (by_list) {
		curpg = TAILQ_FIRST(&vp->v_uobj.memq);
		PHOLD(l);
	} else {
		soff = startoffset;
	}
	while (by_list || soff < MIN(blkeof, endoffset)) {
		if (by_list) {
			if (pages_per_block > 1) {
				while (curpg && (curpg->offset & fs->lfs_bmask))
					curpg = TAILQ_NEXT(curpg, listq);
			}
			if (curpg == NULL)
				break;
			soff = curpg->offset;
		}

		/*
		 * Mark all pages in extended range busy; find out if any
		 * of them are dirty.
		 */
		nonexistent = dirty = 0;
		for (i = 0; i == 0 || i < pages_per_block; i++) {
			if (by_list && pages_per_block <= 1) {
				pgs[i] = pg = curpg;
			} else {
				off = soff + (i << PAGE_SHIFT);
				pgs[i] = pg = uvm_pagelookup(&vp->v_uobj, off);
				if (pg == NULL) {
					++nonexistent;
					continue;
				}
			}
			KASSERT(pg != NULL);
			while (pg->flags & PG_BUSY) {
				pg->flags |= PG_WANTED;
				UVM_UNLOCK_AND_WAIT(pg, &vp->v_interlock, 0,
						    "lfsput", 0);
				simple_lock(&vp->v_interlock);
				if (by_list) {
					if (i > 0)
						uvm_page_unbusy(pgs, i);
					goto top;
				}
			}
			pg->flags |= PG_BUSY;
			UVM_PAGE_OWN(pg, "lfs_putpages");

			pmap_page_protect(pg, VM_PROT_NONE);
			tdirty = (pmap_clear_modify(pg) ||
				  (pg->flags & PG_CLEAN) == 0);
			dirty += tdirty;
		}
		if (pages_per_block > 0 && nonexistent >= pages_per_block) {
			if (by_list) {
				curpg = TAILQ_NEXT(curpg, listq);
			} else {
				soff += fs->lfs_bsize;
			}
			continue;
		}

		any_dirty += dirty;
		KASSERT(nonexistent == 0);

		/*
		 * If any are dirty make all dirty; unbusy them,
		 * but if we were asked to clean, wire them so that
		 * the pagedaemon doesn't bother us about them while
		 * they're on their way to disk.
		 */
		for (i = 0; i == 0 || i < pages_per_block; i++) {
			pg = pgs[i];
			KASSERT(!((pg->flags & PG_CLEAN) && (pg->flags & PG_DELWRI)));
			if (dirty) {
				pg->flags &= ~PG_CLEAN;
				if (flags & PGO_FREE) {
					/* XXXUBC need better way to update */
					simple_lock(&lfs_subsys_lock);
					lfs_subsys_pages += MIN(1, pages_per_block);
					simple_unlock(&lfs_subsys_lock);
					/*
					 * Wire the page so that
					 * pdaemon doesn't see it again.
					 */
					uvm_lock_pageq();
					uvm_pagewire(pg);
					uvm_unlock_pageq();

					/* Suspended write flag */
					pg->flags |= PG_DELWRI;
				}
			}
			if (pg->flags & PG_WANTED)
				wakeup(pg);
			pg->flags &= ~(PG_WANTED|PG_BUSY);
			UVM_PAGE_OWN(pg, NULL);
		}

		if (by_list) {
			curpg = TAILQ_NEXT(curpg, listq);
		} else {
			soff += MAX(PAGE_SIZE, fs->lfs_bsize);
		}
	}
	if (by_list) {
		PRELE(l);
	}

	/*
	 * If any pages were dirty, mark this inode as "pageout requested",
	 * and put it on the paging queue.
	 * XXXUBC locking (check locking on dchainhd too)
	 */
#ifdef notyet
	if (any_dirty) {
		if (!(ip->i_flags & IN_PAGING)) {
			ip->i_flags |= IN_PAGING;
			TAILQ_INSERT_TAIL(&fs->lfs_pchainhd, ip, i_lfs_pchain);
		}
	}
#endif
	return any_dirty;
}

/*
 * lfs_putpages functions like genfs_putpages except that
 * 
 * (1) It needs to bounds-check the incoming requests to ensure that
 *     they are block-aligned; if they are not, expand the range and
 *     do the right thing in case, e.g., the requested range is clean
 *     but the expanded range is dirty.
 * (2) It needs to explicitly send blocks to be written when it is done.
 *     VOP_PUTPAGES is not ever called with the seglock held, so
 *     we simply take the seglock and let lfs_segunlock wait for us.
 *     XXX Actually we can be called with the seglock held, if we have
 *     XXX to flush a vnode while lfs_markv is in operation.  As of this
 *     XXX writing we panic in this case.
 *
 * Assumptions:
 *
 * (1) The caller does not hold any pages in this vnode busy.  If it does,
 *     there is a danger that when we expand the page range and busy the
 *     pages we will deadlock.
 * (2) We are called with vp->v_interlock held; we must return with it
 *     released.
 * (3) We don't absolutely have to free pages right away, provided that
 *     the request does not have PGO_SYNCIO.  When the pagedaemon gives
 *     us a request with PGO_FREE, we take the pages out of the paging
 *     queue and wake up the writer, which will handle freeing them for us.
 *
 *     We ensure that for any filesystem block, all pages for that
 *     block are either resident or not, even if those pages are higher
 *     than EOF; that means that we will be getting requests to free
 *     "unused" pages above EOF all the time, and should ignore them.
 */

int
lfs_putpages(void *v)
{
	int error;
	struct vop_putpages_args /* {
		struct vnode *a_vp;
		voff_t a_offlo;
		voff_t a_offhi;
		int a_flags;
	} */ *ap = v;
	struct vnode *vp;
	struct inode *ip;
	struct lfs *fs;
	struct segment *sp;
	off_t origoffset, startoffset, endoffset, origendoffset, blkeof;
	off_t off, max_endoffset;
	int pages_per_block;
	int s, sync, dirty, pagedaemon;
	struct vm_page *pg;
	UVMHIST_FUNC("lfs_putpages"); UVMHIST_CALLED(ubchist);

	vp = ap->a_vp;
	ip = VTOI(vp);
	fs = ip->i_lfs;
	sync = (ap->a_flags & PGO_SYNCIO);
	pagedaemon = (curproc == uvm.pagedaemon_proc);

	/* Putpages does nothing for metadata. */
	if (vp == fs->lfs_ivnode || vp->v_type != VREG) {
		simple_unlock(&vp->v_interlock);
		return 0;
	}

	/*
	 * If there are no pages, don't do anything.
	 */
	if (vp->v_uobj.uo_npages == 0) {
		s = splbio();
		if (LIST_FIRST(&vp->v_dirtyblkhd) == NULL &&
		    (vp->v_flag & VONWORKLST)) {
			vp->v_flag &= ~VONWORKLST;
			LIST_REMOVE(vp, v_synclist);
		}
		splx(s);
		simple_unlock(&vp->v_interlock);
		return 0;
	}

	blkeof = blkroundup(fs, ip->i_size);

	/*
	 * Ignore requests to free pages past EOF but in the same block
	 * as EOF, unless the request is synchronous. (XXX why sync?)
	 * XXXUBC Make these pages look "active" so the pagedaemon won't
	 * XXXUBC bother us with them again.
	 */
	if (!sync && ap->a_offlo >= ip->i_size && ap->a_offlo < blkeof) {
		origoffset = ap->a_offlo;
		for (off = origoffset; off < blkeof; off += fs->lfs_bsize) {
			pg = uvm_pagelookup(&vp->v_uobj, off);
			KASSERT(pg != NULL);
			while (pg->flags & PG_BUSY) {
				pg->flags |= PG_WANTED;
				UVM_UNLOCK_AND_WAIT(pg, &vp->v_interlock, 0,
						    "lfsput2", 0);
				simple_lock(&vp->v_interlock);
			}
			uvm_lock_pageq();
			uvm_pageactivate(pg);
			uvm_unlock_pageq();
		}
		ap->a_offlo = blkeof;
		if (ap->a_offhi > 0 && ap->a_offhi <= ap->a_offlo) {
			simple_unlock(&vp->v_interlock);
			return 0;
		}
	}

	/*
	 * Extend page range to start and end at block boundaries.
	 * (For the purposes of VOP_PUTPAGES, fragments don't exist.)
	 */
	pages_per_block = fs->lfs_bsize >> PAGE_SHIFT;
	origoffset = ap->a_offlo;
	origendoffset = ap->a_offhi;
	startoffset = origoffset & ~(fs->lfs_bmask);
	max_endoffset = (trunc_page(LLONG_MAX) >> fs->lfs_bshift)
					       << fs->lfs_bshift;

	if (origendoffset == 0 || ap->a_flags & PGO_ALLPAGES) {
		endoffset = max_endoffset;
		origendoffset = endoffset;
	} else {
		origendoffset = round_page(ap->a_offhi);
		endoffset = round_page(blkroundup(fs, origendoffset));
	}

	KASSERT(startoffset > 0 || endoffset >= startoffset);
	if (startoffset == endoffset) {
		/* Nothing to do, why were we called? */
		simple_unlock(&vp->v_interlock);
#ifdef DEBUG
		printf("lfs_putpages: startoffset = endoffset = %" PRId64 "\n",
			startoffset);
#endif
		return 0;
	}

	ap->a_offlo = startoffset;
	ap->a_offhi = endoffset;

	if (!(ap->a_flags & PGO_CLEANIT))
		return genfs_putpages(v);

	/*
	 * Make sure that all pages in any given block are dirty, or
	 * none of them are.  Find out if any of the pages we've been
	 * asked about are dirty.  If none are dirty, send them on
	 * through genfs_putpages(), albeit with adjusted offsets.
	 * XXXUBC I am assuming here that they can't be dirtied in
	 * XXXUBC the meantime, but I bet that's wrong.
	 */
	dirty = check_dirty(fs, vp, startoffset, endoffset, blkeof, ap->a_flags);
	if (!dirty)
		return genfs_putpages(v);
		
	/*
	 * Dirty and asked to clean.
	 *
	 * Pagedaemon can't actually write LFS pages; wake up
	 * the writer to take care of that.  The writer will
	 * notice the pager inode queue and act on that.
	 */
	if (pagedaemon) {
		++fs->lfs_pdflush;
		wakeup(&lfs_writer_daemon);
		simple_unlock(&vp->v_interlock);
		return EWOULDBLOCK;
	}

	/*
	 * If this is a file created in a recent dirop, we can't flush its
	 * inode until the dirop is complete.  Drain dirops, then flush the
	 * filesystem (taking care of any other pending dirops while we're
	 * at it).
	 */
	if ((ap->a_flags & (PGO_CLEANIT|PGO_LOCKED)) == PGO_CLEANIT &&
	    (vp->v_flag & VDIROP)) {
		int locked;

		/* printf("putpages to clean VDIROP, flushing\n"); */
		while (fs->lfs_dirops > 0) {
			++fs->lfs_diropwait;
			tsleep(&fs->lfs_writer, PRIBIO+1, "ppdirop", 0);
			--fs->lfs_diropwait;
		}
		++fs->lfs_writer;
		locked = VOP_ISLOCKED(vp) && /* XXX */
			vp->v_lock.lk_lockholder == curproc->p_pid;
		if (locked)
			VOP_UNLOCK(vp, 0);
		simple_unlock(&vp->v_interlock);
		
		lfs_flush_fs(fs, sync ? SEGM_SYNC : 0);
		
		simple_lock(&vp->v_interlock);
		if (locked)
			VOP_LOCK(vp, LK_EXCLUSIVE);
		if (--fs->lfs_writer == 0)
			wakeup(&fs->lfs_dirops);

		/* XXX the flush should have taken care of this one too! */
	}


	/*
	 * This is it.	We are going to write some pages.  From here on
	 * down it's all just mechanics.
	 *
	 * If there are more than one page per block, we don't want to get
	 * caught locking them backwards; so set PGO_BUSYFAIL to avoid
	 * deadlocks.  Also, don't let genfs_putpages wait;
	 * lfs_segunlock will wait for us, if need be.
	 */
	ap->a_flags &= ~PGO_SYNCIO;
	if (pages_per_block > 1)
		ap->a_flags |= PGO_BUSYFAIL;

	/*
	 * If we've already got the seglock, flush the node and return.
	 * The FIP has already been set up for us by lfs_writefile,
	 * and FIP cleanup and lfs_updatemeta will also be done there,
	 * unless genfs_putpages returns EDEADLK; then we must flush
	 * what we have, and correct FIP and segment header accounting.
	 */
	if (ap->a_flags & PGO_LOCKED) {
		sp = fs->lfs_sp;
		sp->vp = vp;

		while ((error = genfs_putpages(v)) == EDEADLK) {
#ifdef DEBUG_LFS
			printf("lfs_putpages: genfs_putpages returned EDEADLK"
			       " ino %d off %x (seg %d)\n",
			       ip->i_number, fs->lfs_offset,
			       dtosn(fs, fs->lfs_offset));
#endif
			/* If nothing to write, short-circuit */
			if (sp->cbpp - sp->bpp == 1) {
				preempt(NULL);
				simple_lock(&vp->v_interlock);
				continue;
			}
			/* Write gathered pages */
			lfs_updatemeta(sp);
			(void) lfs_writeseg(fs, sp);
 
			/* Reinitialize brand new FIP and add us to it */
			sp->vp = vp;
			sp->fip->fi_version = ip->i_gen;
			sp->fip->fi_ino = ip->i_number;
			/* Add us to the new segment summary. */
			++((SEGSUM *)(sp->segsum))->ss_nfinfo;
			sp->sum_bytes_left -=
				sizeof(struct finfo) - sizeof(int32_t);

			/* Give the write a chance to complete */
			preempt(NULL);
			simple_lock(&vp->v_interlock);
		}
		return error;
	}

	simple_unlock(&vp->v_interlock);
	/*
	 * Take the seglock, because we are going to be writing pages.
	 */
	if ((error = lfs_seglock(fs, SEGM_PROT | (sync ? SEGM_SYNC : 0))) != 0)
		return error;

	/*
	 * VOP_PUTPAGES should not be called while holding the seglock.
	 * XXXUBC fix lfs_markv, or do this properly.
	 */
	/* KASSERT(fs->lfs_seglock == 1); */

	/*
	 * We assume we're being called with sp->fip pointing at blank space.
	 * Account for a new FIP in the segment header, and set sp->vp.
	 * (This should duplicate the setup at the top of lfs_writefile().)
	 */
	sp = fs->lfs_sp;
	if (sp->seg_bytes_left < fs->lfs_bsize ||
	    sp->sum_bytes_left < sizeof(struct finfo))
		(void) lfs_writeseg(fs, fs->lfs_sp); 
 
	sp->sum_bytes_left -= sizeof(struct finfo) - sizeof(int32_t);
	++((SEGSUM *)(sp->segsum))->ss_nfinfo;
	sp->vp = vp;
 
	if (vp->v_flag & VDIROP)
		((SEGSUM *)(sp->segsum))->ss_flags |= (SS_DIROP|SS_CONT);
 
	sp->fip->fi_nblocks = 0;
	sp->fip->fi_ino = ip->i_number;
	sp->fip->fi_version = ip->i_gen;

	/*
	 * Loop through genfs_putpages until all pages are gathered.
	 * genfs_putpages() drops the interlock, so reacquire it if necessary.
	 */
	simple_lock(&vp->v_interlock);
	while ((error = genfs_putpages(v)) == EDEADLK) {
#ifdef DEBUG_LFS
		printf("lfs_putpages: genfs_putpages returned EDEADLK [2]"
		       " ino %d off %x (seg %d)\n",
		       ip->i_number, fs->lfs_offset,
		       dtosn(fs, fs->lfs_offset));
#endif
		/* If nothing to write, short-circuit */
		if (sp->cbpp - sp->bpp == 1) {
			preempt(NULL);
			simple_lock(&vp->v_interlock);
			continue;
		}
		/* Write gathered pages */
		lfs_updatemeta(sp);
		(void) lfs_writeseg(fs, sp);
 
		/*
		 * Reinitialize brand new FIP and add us to it.
		 * (This should duplicate the fixup in lfs_gatherpages().)
		 */
		sp->vp = vp;
		sp->fip->fi_version = ip->i_gen;
		sp->fip->fi_ino = ip->i_number;
		/* Add us to the new segment summary. */
		++((SEGSUM *)(sp->segsum))->ss_nfinfo;
		sp->sum_bytes_left -=
			sizeof(struct finfo) - sizeof(int32_t);

		/* Give the write a chance to complete */
		preempt(NULL);
		simple_lock(&vp->v_interlock);
	}

	/*
	 * Blocks are now gathered into a segment waiting to be written.
	 * All that's left to do is update metadata, and write them.
	 */
	lfs_updatemeta(fs->lfs_sp);
	fs->lfs_sp->vp = NULL;
	/*
	 * Clean up FIP, since we're done writing this file.
	 * This should duplicate cleanup at the end of lfs_writefile().
	 */
	if (sp->fip->fi_nblocks != 0) {
		sp->fip = (FINFO*)((caddr_t)sp->fip + sizeof(struct finfo) +
			sizeof(int32_t) * (sp->fip->fi_nblocks - 1));
		sp->start_lbp = &sp->fip->fi_blocks[0];
	} else {
		sp->sum_bytes_left += sizeof(FINFO) - sizeof(int32_t);
		--((SEGSUM *)(sp->segsum))->ss_nfinfo;
	}
	lfs_writeseg(fs, fs->lfs_sp);

	/*
	 * XXX - with the malloc/copy writeseg, the pages are freed by now
	 * even if we don't wait (e.g. if we hold a nested lock).  This
	 * will not be true if we stop using malloc/copy.
	 */
	KASSERT(fs->lfs_sp->seg_flags & SEGM_PROT);
	lfs_segunlock(fs);

	/*
	 * Wait for v_numoutput to drop to zero.  The seglock should
	 * take care of this, but there is a slight possibility that
	 * aiodoned might not have got around to our buffers yet.
	 */
	if (sync) {
		int s;

		s = splbio();
		simple_lock(&global_v_numoutput_slock);
		while (vp->v_numoutput > 0) {
#ifdef DEBUG
			printf("ino %d sleeping on num %d\n",
				ip->i_number, vp->v_numoutput);
#endif
			vp->v_flag |= VBWAIT;
			ltsleep(&vp->v_numoutput, PRIBIO + 1, "lfs_vn", 0,
			    &global_v_numoutput_slock);
		}
		simple_unlock(&global_v_numoutput_slock);
		splx(s);
	}
	return error;
}

/*
 * Return the last logical file offset that should be written for this file
 * if we're doing a write that ends at "size".	If writing, we need to know
 * about sizes on disk, i.e. fragments if there are any; if reading, we need
 * to know about entire blocks.
 */
void
lfs_gop_size(struct vnode *vp, off_t size, off_t *eobp, int flags)
{
	struct inode *ip = VTOI(vp);
	struct lfs *fs = ip->i_lfs; 
	daddr_t olbn, nlbn;

	KASSERT(flags & (GOP_SIZE_READ | GOP_SIZE_WRITE));
	KASSERT((flags & (GOP_SIZE_READ | GOP_SIZE_WRITE)) 
		!= (GOP_SIZE_READ | GOP_SIZE_WRITE));

	olbn = lblkno(fs, ip->i_size);
	nlbn = lblkno(fs, size);
	if ((flags & GOP_SIZE_WRITE) && nlbn < NDADDR && olbn <= nlbn) {
		*eobp = fragroundup(fs, size);
	} else {
		*eobp = blkroundup(fs, size);
	}
}

#ifdef DEBUG
void lfs_dump_vop(void *);

void
lfs_dump_vop(void *v)
{
	struct vop_putpages_args /* {
		struct vnode *a_vp;
		voff_t a_offlo;
		voff_t a_offhi;
		int a_flags;
	} */ *ap = v;

	vfs_vnode_print(ap->a_vp, 0, printf);
	lfs_dump_dinode(VTOI(ap->a_vp)->i_din.ffs1_din);
}
#endif

int
lfs_mmap(void *v)
{
	struct vop_mmap_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		int a_fflags;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;

	if (VTOI(ap->a_vp)->i_number == LFS_IFILE_INUM)
		return EOPNOTSUPP;
	return ufs_mmap(v);
}
