/*	$NetBSD: lfs_vnops.c,v 1.25.2.1.2.5 1999/08/31 21:03:46 perseant Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
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

#include <vm/vm.h>
#include <uvm/uvm.h>
#include <uvm/uvm_pager.h> /* For PGO_* flags */

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
int (**lfs_vnodeop_p) __P((void *));
struct vnodeopv_entry_desc lfs_vnodeop_entries[] = {
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
	{ &vop_poll_desc, ufs_poll },			/* poll */
	{ &vop_revoke_desc, ufs_revoke },		/* revoke */
	{ &vop_mmap_desc, ufs_mmap },			/* mmap */
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
	{ &vop_inactive_desc, ufs_inactive },		/* inactive */
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
	{ &vop_getpages_desc, lfs_getpages },		/* getpages */
	{ &vop_putpages_desc, lfs_putpages },		/* putpages */
	{ (struct vnodeop_desc*)NULL, (int(*) __P((void *)))NULL }
};
struct vnodeopv_desc lfs_vnodeop_opv_desc =
	{ &lfs_vnodeop_p, lfs_vnodeop_entries };

int (**lfs_specop_p) __P((void *));
struct vnodeopv_entry_desc lfs_specop_entries[] = {
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
	{ &vop_inactive_desc, ufs_inactive },		/* inactive */
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
	{ &vop_bwrite_desc, lfs_bwrite },		/* bwrite */
	{ (struct vnodeop_desc*)NULL, (int(*) __P((void *)))NULL }
};
struct vnodeopv_desc lfs_specop_opv_desc =
	{ &lfs_specop_p, lfs_specop_entries };

int (**lfs_fifoop_p) __P((void *));
struct vnodeopv_entry_desc lfs_fifoop_entries[] = {
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
	{ &vop_inactive_desc, ufs_inactive },		/* inactive */
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
	{ (struct vnodeop_desc*)NULL, (int(*) __P((void *)))NULL }
};
struct vnodeopv_desc lfs_fifoop_opv_desc =
	{ &lfs_fifoop_p, lfs_fifoop_entries };

#define	LFS_READWRITE
#include <ufs/ufs/ufs_readwrite.c>
#undef	LFS_READWRITE

/*
 * Synch an open file.
 */
/* ARGSUSED */
int
lfs_fsync(v)
	void *v;
{
	struct vop_fsync_args /* {
		struct vnode *a_vp;
		struct ucred *a_cred;
		int a_flags;
		struct proc *a_p;
	} */ *ap = v;
	
	return (VOP_UPDATE(ap->a_vp, NULL, NULL,
			   (ap->a_flags & FSYNC_WAIT) != 0 ? LFS_SYNC : 0)); /* XXX */
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
#define	SET_DIROP(fs) lfs_set_dirop(fs)
static int lfs_set_dirop __P((struct lfs *));

static int lfs_set_dirop(fs)
	struct lfs *fs;
{
	int error;

	while (fs->lfs_writer || fs->lfs_dirvcount>LFS_MAXDIROP) {
		if(fs->lfs_writer)
			tsleep(&fs->lfs_dirops, PRIBIO + 1, "lfs_dirop", 0);
		if(fs->lfs_dirvcount > LFS_MAXDIROP) {		
#ifdef DEBUG_LFS
			printf("(dirvcount=%d)\n",fs->lfs_dirvcount); 
#endif
			if((error=tsleep(&fs->lfs_dirvcount, PCATCH|PUSER, "lfs_maxdirop", 0))!=0)
				return error;
		}							
	}								
	++fs->lfs_dirops;						
	fs->lfs_doifile = 1;						

	return 0;
}

#define	SET_ENDOP(fs,vp,str) {						\
	--(fs)->lfs_dirops;						\
	if (!(fs)->lfs_dirops) {					\
		wakeup(&(fs)->lfs_writer);				\
		lfs_check((vp),LFS_UNUSED_LBN,0);			\
	}								\
}

#define	MARK_VNODE(dvp)  do {                                           \
        if(!((dvp)->v_flag & VDIROP)) {					\
                lfs_vref(dvp);						\
		++VTOI((dvp))->i_lfs->lfs_dirvcount;			\
	}								\
        (dvp)->v_flag |= VDIROP;					\
} while(0)

#define MAYBE_INACTIVE(fs,vp) do {                                      \
        if((vp) && ((vp)->v_flag & VDIROP) && (vp)->v_usecount == 1     \
           && VTOI(vp) && VTOI(vp)->i_ffs_nlink == 0)                   \
        {                                                               \
		if (VOP_LOCK((vp), LK_EXCLUSIVE) == 0) { 		\
                        VOP_INACTIVE((vp),curproc);                     \
		}                                                       \
        }                                                               \
} while(0)

int
lfs_symlink(v)
	void *v;
{
	struct vop_symlink_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
		char *a_target;
	} */ *ap = v;
	int ret;

	if((ret=SET_DIROP(VTOI(ap->a_dvp)->i_lfs))!=0)
		return ret;
	MARK_VNODE(ap->a_dvp);
	ret = ufs_symlink(ap);
	MAYBE_INACTIVE(VTOI(ap->a_dvp)->i_lfs,*(ap->a_vpp)); /* XXX KS */
	SET_ENDOP(VTOI(ap->a_dvp)->i_lfs,ap->a_dvp,"symilnk");
	return (ret);
}

int
lfs_mknod(v)
	void *v;
{
	struct vop_mknod_args	/* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
		} */ *ap = v;
	int ret;

	if((ret=SET_DIROP(VTOI(ap->a_dvp)->i_lfs))!=0)
		return ret;
	MARK_VNODE(ap->a_dvp);
	ret = ufs_mknod(ap);
	MAYBE_INACTIVE(VTOI(ap->a_dvp)->i_lfs,*(ap->a_vpp)); /* XXX KS */
	SET_ENDOP(VTOI(ap->a_dvp)->i_lfs,ap->a_dvp,"mknod");
	return (ret);
}

int
lfs_create(v)
	void *v;
{
	struct vop_create_args	/* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap = v;
	int ret;

	if((ret=SET_DIROP(VTOI(ap->a_dvp)->i_lfs))!=0)
		return ret;
	MARK_VNODE(ap->a_dvp);
	ret = ufs_create(ap);
	MAYBE_INACTIVE(VTOI(ap->a_dvp)->i_lfs,*(ap->a_vpp)); /* XXX KS */
	SET_ENDOP(VTOI(ap->a_dvp)->i_lfs,ap->a_dvp,"create");
	return (ret);
}

int
lfs_whiteout(v)
	void *v;
{
	struct vop_whiteout_args /* {
		struct vnode *a_dvp;
		struct componentname *a_cnp;
		int a_flags;
	} */ *ap = v;
	int ret;

	if((ret=SET_DIROP(VTOI(ap->a_dvp)->i_lfs))!=0)
		return ret;
	MARK_VNODE(ap->a_dvp);
	ret = ufs_whiteout(ap);
	SET_ENDOP(VTOI(ap->a_dvp)->i_lfs,ap->a_dvp,"whiteout");
	return (ret);
}

int
lfs_mkdir(v)
	void *v;
{
	struct vop_mkdir_args	/* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap = v;
	int ret;

	if((ret=SET_DIROP(VTOI(ap->a_dvp)->i_lfs))!=0)
		return ret;
	MARK_VNODE(ap->a_dvp);
	ret = ufs_mkdir(ap);
	MAYBE_INACTIVE(VTOI(ap->a_dvp)->i_lfs,*(ap->a_vpp)); /* XXX KS */
	SET_ENDOP(VTOI(ap->a_dvp)->i_lfs,ap->a_dvp,"mkdir");
	return (ret);
}

int
lfs_remove(v)
	void *v;
{
	struct vop_remove_args	/* {
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap = v;
	int ret;
	if((ret=SET_DIROP(VTOI(ap->a_dvp)->i_lfs))!=0)
		return ret;
	MARK_VNODE(ap->a_dvp);
	MARK_VNODE(ap->a_vp);
	ret = ufs_remove(ap);
	MAYBE_INACTIVE(VTOI(ap->a_dvp)->i_lfs,ap->a_vp);
	SET_ENDOP(VTOI(ap->a_dvp)->i_lfs,ap->a_dvp,"remove");
	return (ret);
}

int
lfs_rmdir(v)
	void *v;
{
	struct vop_rmdir_args	/* {
		struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap = v;
	int ret;

	if((ret=SET_DIROP(VTOI(ap->a_dvp)->i_lfs))!=0)
		return ret;
	MARK_VNODE(ap->a_dvp);
	MARK_VNODE(ap->a_vp);
	ret = ufs_rmdir(ap);
	MAYBE_INACTIVE(VTOI(ap->a_dvp)->i_lfs,ap->a_vp);
	SET_ENDOP(VTOI(ap->a_dvp)->i_lfs,ap->a_dvp,"rmdir");
	return (ret);
}

int
lfs_link(v)
	void *v;
{
	struct vop_link_args	/* {
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap = v;
	int ret;

	if((ret=SET_DIROP(VTOI(ap->a_dvp)->i_lfs))!=0)
		return ret;
	MARK_VNODE(ap->a_dvp);
	ret = ufs_link(ap);
	SET_ENDOP(VTOI(ap->a_dvp)->i_lfs,ap->a_dvp,"link");
	return (ret);
}

int
lfs_rename(v)
	void *v;
{
	struct vop_rename_args	/* {
		struct vnode *a_fdvp;
		struct vnode *a_fvp;
		struct componentname *a_fcnp;
		struct vnode *a_tdvp;
		struct vnode *a_tvp;
		struct componentname *a_tcnp;
	} */ *ap = v;
	int ret;
	
	if((ret=SET_DIROP(VTOI(ap->a_fdvp)->i_lfs))!=0)
		return ret;
	MARK_VNODE(ap->a_fdvp);
	MARK_VNODE(ap->a_tdvp);
	ret = ufs_rename(ap);
	MAYBE_INACTIVE(VTOI(ap->a_dvp)->i_lfs,ap->a_fvp);
	MAYBE_INACTIVE(VTOI(ap->a_dvp)->i_lfs,ap->a_tvp);
	SET_ENDOP(VTOI(ap->a_fdvp)->i_lfs,ap->a_fdvp,"rename");
	return (ret);
}

/* XXX hack to avoid calling ITIMES in getattr */
int
lfs_getattr(v)
	void *v;
{
	struct vop_getattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;
	register struct vnode *vp = ap->a_vp;
	register struct inode *ip = VTOI(vp);
	register struct vattr *vap = ap->a_vap;
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
	vap->va_size = ip->i_ffs_size;
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
	vap->va_bytes = dbtob((u_quad_t)ip->i_ffs_blocks);
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
lfs_close(v)
	void *v;
{
	struct vop_close_args /* {
		struct vnode *a_vp;
		int  a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;
	register struct vnode *vp = ap->a_vp;
	register struct inode *ip = VTOI(vp);
	int mod;
	struct timespec ts;

	simple_lock(&vp->v_interlock);
	if (vp->v_usecount > 1) {
		mod = ip->i_flag & IN_MODIFIED;
		TIMEVAL_TO_TIMESPEC(&time, &ts);
		LFS_ITIMES(ip, &ts, &ts, &ts);
		if (!mod && ip->i_flag & IN_MODIFIED)
			ip->i_lfs->lfs_uinodes++;
	}
	simple_unlock(&vp->v_interlock);
	return (0);
}

/*
 * Reclaim an inode so that it can be used for other purposes.
 */
int lfs_no_inactive = 0;

int
lfs_reclaim(v)
	void *v;
{
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	int error;

	if ((error = ufs_reclaim(vp, ap->a_p)))
		return (error);
	pool_put(&lfs_inode_pool, vp->v_data);
	vp->v_data = NULL;
	return (0);
}

#define BUFHEAD_THROTTLE 128
static unsigned long bufhead_count, bufhead_rem;

static void
lfs_bufcallback(struct buf *bp)
{
	int s;

	s = splbio();
	if(bp->b_vp)
		brelvp(bp);
	--bufhead_count;
	splx(s);

	uvm_aio_biodone1(bp); /* maybe set mbp->b_iodone = aio_aiodone */
	return;
}

/*
 * LFS-specific VM putpages routine.
 * Encapsulate the pages in buffer headers, and call lfs_segwrite to write
 * them out.
 *
 * Mostly taken from genfs_putpages.
 * 
 * XXX Assumes that fs block size is <= page size, in order to avoid
 * XXX having to read partial blocks in from disk.
 *
 * Ideally we would use pagemove() and not have this requirement
 * (or just call lfs_segwrite with some notation of which pages
 * were busy).
 */

#define SET_FLUSHING(fs,vp) (fs)->lfs_flushvp = (vp)
#define IS_FLUSHING(fs,vp)  ((fs)->lfs_flushvp == (vp))
#define CLR_FLUSHING(fs,vp) (fs)->lfs_flushvp = NULL

/* Statistics Counters */
extern int lfs_dostats;
extern struct lfs_stats lfs_stats;

/*
 * Utility function for flushing a single vnode, in a case where
 * lfs_segwrite might deadlock.  Assumes that we hold the seglock.
 * Excerpted from lfs_vflush. (XXX should be combined)
 */
static inline void
lfs_writevnode(struct lfs *fs, struct vnode *vp)
{
	struct segment *sp;
	struct inode *ip;

	ip = VTOI(vp);
	sp = fs->lfs_sp;

	do {
		do {
			if (vp->v_dirtyblkhd.lh_first != NULL)
				lfs_writefile(fs, sp, vp);
		} while (lfs_writeinode(fs, sp, ip));
	} while (lfs_writeseg(fs, sp) && ip->i_number == LFS_IFILE_INUM);
	
	if(lfs_dostats) {
		++lfs_stats.nwrites;
		if (sp->seg_flags & SEGM_SYNC)
			++lfs_stats.nsync_writes;
		if (sp->seg_flags & SEGM_CKP)
			++lfs_stats.ncheckpoints;
	}
}

int
lfs_putpages(v)
	void *v;
{
	struct vop_putpages_args /* {
		struct vnode *a_vp;
		struct vm_page **a_m;
		int a_count;
		int a_flags;
		int *a_rtvals;
	} */ *ap = v;

	int s, error, npages, bshift, dev_bshift, dev_bsize;
	char * kva;
	off_t offset, startoffset;
	size_t bytes, iobytes, skipbytes;
	daddr_t lbn;
	struct vm_page *pg;
#if 0
	struct vm_page *vmpp, *vmpp2;
#endif
	struct buf *mbp, *bp;
	struct vnode *vp = ap->a_vp;
	struct lfs *fs;
	struct inode *ip;

	UVMHIST_FUNC("lfs_putpages"); UVMHIST_CALLED(ubchist);

	ip = VTOI(vp);
	fs = ip->i_lfs;

	lfs_seglock(fs,((ap->a_flags & PGO_SYNCIO) ? SEGM_SYNC : 0));

	/* If we're flushing, segwrite needs to know not to wait */
	if(ap->a_flags & PGO_ALLPAGES) {
		SET_FLUSHING(fs,vp);
	}

	error = 0;
	npages = ap->a_count;
	bshift = vp->v_mount->mnt_fs_bshift;
	dev_bshift = vp->v_mount->mnt_dev_bshift;
	dev_bsize = 1 << dev_bshift;

	pg = ap->a_m[0];
	startoffset = pg->offset;
	bytes = npages << PAGE_SHIFT;
	skipbytes = 0;

	if (bytes == 0) {
		panic("lfs_putpages: bytes == 0??? vp %p", vp);
	}

#if 0
	/*
	 * Fix start/end addresses to write entire blocks/fragments
	 * XXX This can deadlock us; can the pager do this klustering
	 * for us?
	 */
	lbn = startoffset >> bshift;
	if(startoffset != (lbn << bshift)) {
		printf("Reading %d bytes at beginning of putpages\n",
			startoffset - (lbn<<bshift));
		/* Make sure the request is valid - XXX deadlock */
		VOP_GETPAGES(vp, (lbn<<bshift), &vmpp, (startoffset - (lbn<<bshift)) >> pgshift, &newpages,
			0, VM_PROT_DEFAULT, 0, 0);
		bytes += (startoffset - (lbn << bshift));
	        startoffset = lbn << bshift;
		pg = vmpp[0];
		printf("Done reading beginning.\n");
	}
	if(bytes & fs->lfs_bmask) {
		new_lastsize = blksize(fs, ip, startoffset + bytes);
		diff = new_lastsize - (bytes & fs->lfs_bmask);
		if(diff) {
			printf("Reading %d bytes at end of putpages\n", diff);
			/* Make sure request is valid - XXX deadlock */
			VOP_GETPAGES(vp, startoffset+bytes, &vmpp2, diff, &newpages, 0,
				VM_PROT_DEFAULT, 0, 0);
			bytes += diff;
			printf("Done reading end.\n");
		}
	}
#endif

	kva = (void *)uvm_pagermapin(ap->a_m, npages, M_WAITOK);

	s = splbio();
	vp->v_numoutput++;
	mbp = pool_get(&bufpool, PR_WAITOK);
	UVMHIST_LOG(ubchist, "master vp %p bp %p num now %d",
		    vp, mbp, vp->v_numoutput, 0);
	splx(s);
	mbp->b_bufsize = npages << PAGE_SHIFT;
	mbp->b_data = kva;
	mbp->b_resid = mbp->b_bcount = bytes;
	mbp->b_flags = B_BUSY|B_WRITE|(ap->a_flags & PGO_SYNCIO ? 0 : B_CALL)|
		(curproc == uvm.pagedaemon_proc ? B_PDAEMON : 0);
	mbp->b_iodone = uvm_aio_biodone;
	mbp->b_vp = vp;

	bp = NULL;

	offset = startoffset;

	for (; bytes > 0; offset += iobytes, bytes -= iobytes) {
		lbn = offset >> bshift;
		iobytes = min((1<<bshift), bytes);

		if(bufhead_rem > BUFHEAD_THROTTLE) {
			if(ap->a_flags & PGO_ALLPAGES)
				lfs_writevnode(fs, vp);
			else
				lfs_segwrite(vp->v_mount, 0);
			bufhead_rem = 0;
			/* Don't sleep here, there's no point XXX ? */
		}

		++bufhead_rem;
		s = splbio();
		++bufhead_count;
		/* vp->v_numoutput++; */
		bp = pool_get(&bufpool, PR_WAITOK);
		UVMHIST_LOG(ubchist, "vp %p bp %p num now %d",
			    vp, bp, vp->v_numoutput, 0);
		memset(bp,0,sizeof(*bp));
		bgetvp(vp, bp);
		splx(s);

		bp->b_data = kva + offset - pg->offset;
		bp->b_resid = bp->b_bcount = bp->b_bufsize = iobytes;
		bp->b_flags = B_BUSY|B_WRITE|B_CALL;
		bp->b_iodone = lfs_bufcallback /* uvm_aio_biodone1 */ ;
		bp->b_lblkno = bp->b_blkno = lbn;
		bp->b_private = mbp;

#ifdef DIAGNOSTIC
		if(offset != (lbn<<bshift)) {
			printf("off=0x%lx, lbn<<bshift=0x%lx\n",
				(unsigned long)offset,
				(unsigned long)(lbn<<bshift));
			panic("Oops, fs bsize is %u but page size is apparently %u\n", (unsigned)1<<bshift, (unsigned)bp->b_bcount);
		}
		if(iobytes != (1<<bshift)) {
			printf("iobytes=0x%lx, 1<<bshift=0x%lx\n",
				(unsigned long)iobytes,
				(unsigned long)(1<<bshift));
			panic("Oops, fs bsize is %u but page size is apparently %u\n", (unsigned)1<<bshift, (unsigned)bp->b_bcount);
		}
#endif
		VOP_BWRITE(bp);
	}

	/*
	 * The pager wants these buffers to be written right now (async may
	 * be okay, but definitely start right away).
	 *
	 * Call segwrite to start the write.
	 */
	if (skipbytes) {
		s = splbio();
		mbp->b_resid -= skipbytes;
		if (mbp->b_resid == 0)
			biodone(mbp);
		splx(s);
	}
	if(bufhead_rem) {
		if(ap->a_flags & PGO_ALLPAGES)
			lfs_writevnode(fs, vp);
		else
			lfs_segwrite(vp->v_mount,
				     ((ap->a_flags & PGO_SYNCIO) ?
				      SEGM_SYNC : 0));
		bufhead_rem = 0;
		if (!(ap->a_flags & PGO_SYNCIO)) {
			CLR_FLUSHING(fs,vp);
			lfs_segunlock(fs);
			return EINPROGRESS;
		}
	}

	/*
	 * By the time we reach this point, all buffers should be recovered,
	 * and the mbp should be waiting for aiodoned to reclaim it.  We're writing
	 * synchronously, so we have to wait for this to happen.
	 */
 /* errout: */
	CLR_FLUSHING(fs,vp);
	lfs_segunlock(fs);

	if (bufhead_count) {
		error = biowait(mbp);
	}
	s = splbio();
	pool_put(&bufpool, mbp);
	splx(s);

	uvm_pagermapout((vaddr_t)kva, npages);
	UVMHIST_LOG(ubchist, "returning, error %d", error,0,0,0);
	return error;
}
