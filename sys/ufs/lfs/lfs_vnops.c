/*	$NetBSD: lfs_vnops.c,v 1.54.2.1 2001/10/01 12:48:30 fvdl Exp $	*/

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/resourcevar.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/signalvar.h>

#include <miscfs/fifofs/fifo.h>
#include <miscfs/genfs/genfs.h>
#include <miscfs/specfs/specdev.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

/* Global vfs data structures for lfs. */
int (**lfs_vnodeop_p)(void *);
const struct vnodeopv_entry_desc lfs_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, ufs_lookup },		/* lookup */
	{ &vop_create_desc, lfs_create },		/* create */
	{ &vop_whiteout_desc, lfs_whiteout },		/* whiteout */
	{ &vop_mknod_desc, lfs_mknod },			/* mknod */
	{ &vop_open_desc, ufs_open },			/* open */
	{ &vop_close_desc, lfs_close },			/* close */
	{ &vop_access_desc, ufs_access },		/* access */
	{ &vop_getattr_desc, lfs_getattr },		/* getattr */
	{ &vop_setattr_desc, ufs_setattr },		/* setattr */
	{ &vop_read_desc, lfs_read },			/* read */
	{ &vop_write_desc, lfs_write },			/* write */
	{ &vop_lease_desc, ufs_lease_check },		/* lease */
	{ &vop_ioctl_desc, ufs_ioctl },			/* ioctl */
	{ &vop_fcntl_desc, ufs_fcntl },			/* fcntl */
	{ &vop_poll_desc, ufs_poll },			/* poll */
	{ &vop_revoke_desc, ufs_revoke },		/* revoke */
#if 0 /* XXX until LFS is fixed */
	{ &vop_mmap_desc, ufs_mmap },			/* mmap */
#endif
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
	{ &vop_strategy_desc, ufs_strategy },		/* strategy */
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
	{ &vop_close_desc, ufsspec_close },		/* close */
	{ &vop_access_desc, ufs_access },		/* access */
	{ &vop_getattr_desc, lfs_getattr },		/* getattr */
	{ &vop_setattr_desc, ufs_setattr },		/* setattr */
	{ &vop_read_desc, ufsspec_read },		/* read */
	{ &vop_write_desc, ufsspec_write },		/* write */
	{ &vop_lease_desc, spec_lease_check },		/* lease */
	{ &vop_ioctl_desc, spec_ioctl },		/* ioctl */
	{ &vop_fcntl_desc, ufs_fcntl },			/* fcntl */
	{ &vop_poll_desc, spec_poll },			/* poll */
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
	{ &vop_close_desc, ufsfifo_close },		/* close */
	{ &vop_access_desc, ufs_access },		/* access */
	{ &vop_getattr_desc, lfs_getattr },		/* getattr */
	{ &vop_setattr_desc, ufs_setattr },		/* setattr */
	{ &vop_read_desc, ufsfifo_read },		/* read */
	{ &vop_write_desc, ufsfifo_write },		/* write */
	{ &vop_lease_desc, fifo_lease_check },		/* lease */
	{ &vop_ioctl_desc, fifo_ioctl },		/* ioctl */
	{ &vop_fcntl_desc, ufs_fcntl },			/* fcntl */
	{ &vop_poll_desc, fifo_poll },			/* poll */
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
	{ &vop_putpages_desc, fifo_putpages }, 		/* putpages */
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
	
	/* Ignore the trickle syncer */
	if (ap->a_flags & FSYNC_LAZY)
		return 0;

	return (VOP_UPDATE(ap->a_vp, NULL, NULL,
			   (ap->a_flags & FSYNC_WAIT) != 0 ? UPDATE_WAIT : 0));
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
	struct inode *ip = VTOI(ap->a_vp);

	if (ip->i_flag & IN_ADIROP)
		--ip->i_lfs->lfs_nadirop;
	ip->i_flag &= ~IN_ADIROP;
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
#define	SET_DIROP(vp) lfs_set_dirop(vp)
static int lfs_set_dirop(struct vnode *);
extern int lfs_dirvcount;

static int
lfs_set_dirop(struct vnode *vp)
{
	struct lfs *fs;
	int error;

	fs = VTOI(vp)->i_lfs;
	/*
	 * We might need one directory block plus supporting indirect blocks,
	 * plus an inode block and ifile page for the new vnode.
	 */
	if ((error = lfs_reserve(fs, vp, btofsb(fs, (NIADDR + 3) << fs->lfs_bshift))) != 0)
		return (error);
	if (fs->lfs_dirops == 0)
		lfs_check(vp, LFS_UNUSED_LBN, 0);
	while (fs->lfs_writer || lfs_dirvcount > LFS_MAXDIROP) {
		if(fs->lfs_writer)
			tsleep(&fs->lfs_dirops, PRIBIO + 1, "lfs_dirop", 0);
		if(lfs_dirvcount > LFS_MAXDIROP && fs->lfs_dirops==0) {
                	++fs->lfs_writer;
                	lfs_flush(fs, 0);
                	if(--fs->lfs_writer==0)
                        	wakeup(&fs->lfs_dirops);
		}

		if(lfs_dirvcount > LFS_MAXDIROP) {		
#ifdef DEBUG_LFS
			printf("lfs_set_dirop: sleeping with dirops=%d, "
			       "dirvcount=%d\n", fs->lfs_dirops,
			       lfs_dirvcount); 
#endif
			if((error = tsleep(&lfs_dirvcount, PCATCH|PUSER,
					   "lfs_maxdirop", 0)) !=0) {
				lfs_reserve(fs, vp, -btofsb(fs, (NIADDR + 3) << fs->lfs_bshift));
				return error;
			}
		}							
	}								
	++fs->lfs_dirops;						
	fs->lfs_doifile = 1;						

	/* Hold a reference so SET_ENDOP will be happy */
	lfs_vref(vp);

	return 0;
}

#define	SET_ENDOP(fs,vp,str) {						\
	--(fs)->lfs_dirops;						\
	if (!(fs)->lfs_dirops) {					\
		if ((fs)->lfs_nadirop) {				\
			panic("SET_ENDOP: %s: no dirops but nadirop=%d\n", \
			      (str), (fs)->lfs_nadirop);		\
		}							\
		wakeup(&(fs)->lfs_writer);				\
		lfs_check((vp),LFS_UNUSED_LBN,0);			\
	}								\
	lfs_reserve((fs), vp, -btofsb((fs), (NIADDR + 3) << (fs)->lfs_bshift)); /* XXX */	\
	lfs_vunref(vp);							\
}

#define	MARK_VNODE(dvp)  do {                                           \
        if (!((dvp)->v_flag & VDIROP)) {				\
                (void)lfs_vref(dvp);					\
		++lfs_dirvcount;					\
	}								\
        (dvp)->v_flag |= VDIROP;					\
	if (!(VTOI(dvp)->i_flag & IN_ADIROP)) {				\
		++VTOI(dvp)->i_lfs->lfs_nadirop;			\
	}								\
	VTOI(dvp)->i_flag |= IN_ADIROP;					\
} while(0)

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
                ip->i_ffs_rdev = ufs_rw32(vap->va_rdev,
                    UFS_MPNEEDSWAP((*vpp)->v_mount));
#else
                ip->i_ffs_rdev = vap->va_rdev;
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
	if ((error = VOP_FSYNC(*vpp, NOCRED, FSYNC_WAIT, 0, 0, curproc)) != 0) {
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

	if((error = SET_DIROP(ap->a_dvp)) != 0) {
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
lfs_whiteout(void *v)
{
	struct vop_whiteout_args /* {
		struct vnode *a_dvp;
		struct componentname *a_cnp;
		int a_flags;
	} */ *ap = v;
	int error;

	if ((error = SET_DIROP(ap->a_dvp)) != 0)
		/* XXX no unlock here? */
		return error;
	MARK_VNODE(ap->a_dvp);
	error = ufs_whiteout(ap);
	UNMARK_VNODE(ap->a_dvp);
	SET_ENDOP(VTOI(ap->a_dvp)->i_lfs,ap->a_dvp,"whiteout");
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

	if((error = SET_DIROP(ap->a_dvp)) != 0) {
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
	if ((error = SET_DIROP(dvp)) != 0) {
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

	/*
	 * If ufs_remove failed, vp doesn't need to be VDIROP any more.
	 * If it succeeded, we can go ahead and wipe out vp, since
	 * its loss won't appear on disk until checkpoint, and by then
	 * dvp will have been written, completing the dirop.
	 */
	--lfs_dirvcount;
	vp->v_flag &= ~VDIROP;
	wakeup(&lfs_dirvcount);
	vrele(vp);

	SET_ENDOP(VTOI(dvp)->i_lfs,dvp,"remove");
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
	int error;

	if ((error = SET_DIROP(ap->a_dvp)) != 0) {
		vrele(ap->a_dvp);
		if (ap->a_vp->v_mountedhere != NULL)
			VOP_UNLOCK(ap->a_dvp, 0);
		vput(ap->a_vp);
		return error;
	}
	MARK_VNODE(ap->a_dvp);
	MARK_VNODE(ap->a_vp);
	error = ufs_rmdir(ap);
	UNMARK_VNODE(ap->a_dvp);
	UNMARK_VNODE(ap->a_vp);

	/*
	 * If ufs_rmdir failed, vp doesn't need to be VDIROP any more.
	 * If it succeeded, we can go ahead and wipe out vp, since
	 * its loss won't appear on disk until checkpoint, and by then
	 * dvp will have been written, completing the dirop.
	 */
	--lfs_dirvcount;
	ap->a_vp->v_flag &= ~VDIROP;
	wakeup(&lfs_dirvcount);
	vrele(ap->a_vp);

	SET_ENDOP(VTOI(ap->a_dvp)->i_lfs,ap->a_dvp,"rmdir");
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
	int error;
	struct lfs *fs;

	fs = VTOI(ap->a_fdvp)->i_lfs;
	tvp = ap->a_tvp;
	tdvp = ap->a_tdvp;
	fvp = ap->a_fvp;
	fdvp = ap->a_fdvp;

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
	if ((error = SET_DIROP(fdvp))!=0)
		goto errout;
	MARK_VNODE(fdvp);
	MARK_VNODE(tdvp);
	error = ufs_rename(ap);
	UNMARK_VNODE(fdvp);
	UNMARK_VNODE(tdvp);
	SET_ENDOP(fs,fdvp,"rename");
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
	vap->va_mode = ip->i_ffs_mode & ~IFMT;
	vap->va_nlink = ip->i_ffs_nlink;
	vap->va_uid = ip->i_ffs_uid;
	vap->va_gid = ip->i_ffs_gid;
	vap->va_rdev = (dev_t)ip->i_ffs_rdev;
	vap->va_size = vp->v_size;
	vap->va_atime.tv_sec = ip->i_ffs_atime;
	vap->va_atime.tv_nsec = ip->i_ffs_atimensec;
	vap->va_mtime.tv_sec = ip->i_ffs_mtime;
	vap->va_mtime.tv_nsec = ip->i_ffs_mtimensec;
	vap->va_ctime.tv_sec = ip->i_ffs_ctime;
	vap->va_ctime.tv_nsec = ip->i_ffs_ctimensec;
	vap->va_flags = ip->i_ffs_flags;
	vap->va_gen = ip->i_ffs_gen;
	/* this doesn't belong here */
	if (vp->v_type == VBLK)
		vap->va_blocksize = BLKDEV_IOSIZE;
	else if (vp->v_type == VCHR)
		vap->va_blocksize = MAXBSIZE;
	else
		vap->va_blocksize = vp->v_mount->mnt_stat.f_iosize;
	vap->va_bytes = fsbtob(fs, (u_quad_t)ip->i_ffs_blocks);
	vap->va_type = vp->v_type;
	vap->va_filerev = ip->i_modrev;
	return (0);
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

	simple_lock(&vp->v_interlock);
	if (vp->v_usecount > 1) {
		TIMEVAL_TO_TIMESPEC(&time, &ts);
		LFS_ITIMES(ip, &ts, &ts, &ts);
	}
	simple_unlock(&vp->v_interlock);
	return (0);
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
	int error;

	LFS_CLR_UINO(VTOI(vp), IN_ALLMOD);
	if ((error = ufs_reclaim(vp, ap->a_p)))
		return (error);
	pool_put(&lfs_inode_pool, vp->v_data);
	vp->v_data = NULL;
	return (0);
}
