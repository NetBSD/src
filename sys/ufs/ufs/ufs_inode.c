/*	$NetBSD: ufs_inode.c,v 1.70.2.2 2007/12/26 21:40:03 ad Exp $	*/

/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)ufs_inode.c	8.9 (Berkeley) 5/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ufs_inode.c,v 1.70.2.2 2007/12/26 21:40:03 ad Exp $");

#if defined(_KERNEL_OPT)
#include "opt_ffs.h"
#include "opt_quota.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/kernel.h>
#include <sys/namei.h>
#include <sys/kauth.h>
#include <sys/fstrans.h>
#include <sys/kmem.h>

#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>
#ifdef UFS_DIRHASH
#include <ufs/ufs/dirhash.h>
#endif
#ifdef UFS_EXTATTR
#include <ufs/ufs/extattr.h>
#endif

#include <uvm/uvm.h>

extern int prtactive;

/*
 * Last reference to an inode.  If necessary, write or delete it.
 */
int
ufs_inactive(void *v)
{
	struct vop_inactive_args /* {
		struct vnode *a_vp;
		struct bool *a_recycle;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct inode *ip = VTOI(vp);
	struct mount *transmp;
	mode_t mode;
	int error = 0;

	if (prtactive && vp->v_usecount > 1)
		vprint("ufs_inactive: pushing active", vp);

	transmp = vp->v_mount;
	fstrans_start(transmp, FSTRANS_SHARED);
	/*
	 * Ignore inodes related to stale file handles.
	 */
	if (ip->i_mode == 0)
		goto out;
	if (ip->i_ffs_effnlink == 0 && DOINGSOFTDEP(vp))
		softdep_releasefile(ip);

	if (ip->i_nlink <= 0 && (vp->v_mount->mnt_flag & MNT_RDONLY) == 0) {
#ifdef QUOTA
		(void)chkiq(ip, -1, NOCRED, 0);
#endif
#ifdef UFS_EXTATTR
		ufs_extattr_vnode_inactive(vp, l);
#endif
		if (ip->i_size != 0) {
			error = UFS_TRUNCATE(vp, (off_t)0, 0, NOCRED);
		}
		/*
		 * Setting the mode to zero needs to wait for the inode
		 * to be written just as does a change to the link count.
		 * So, rather than creating a new entry point to do the
		 * same thing, we just use softdep_change_linkcnt().
		 */
		DIP_ASSIGN(ip, rdev, 0);
		mode = ip->i_mode;
		ip->i_mode = 0;
		ip->i_omode = mode;
		DIP_ASSIGN(ip, mode, 0);
		ip->i_flag |= IN_CHANGE | IN_UPDATE;
		if (DOINGSOFTDEP(vp))
			softdep_change_linkcnt(ip);
		/*
		 * Defer final inode free and update to ufs_reclaim().
		 */
	} else if (ip->i_flag & (IN_CHANGE | IN_UPDATE | IN_MODIFIED)) {
		UFS_UPDATE(vp, NULL, NULL, 0);
	}
out:
	/*
	 * If we are done with the inode, reclaim it
	 * so that it can be reused immediately.
	 */
	*ap->a_recycle = (ip->i_mode == 0);
	VOP_UNLOCK(vp, 0);
	fstrans_done(transmp);
	return (error);
}

/*
 * Reclaim an inode so that it can be used for other purposes.
 */
int
ufs_reclaim(struct vnode *vp)
{
	struct inode *ip = VTOI(vp);

	if (prtactive && vp->v_usecount > 1)
		vprint("ufs_reclaim: pushing active", vp);

	/*
	 * The inode must be freed and updated before being removed
	 * from its hash chain.  Other threads trying to gain a hold
	 * on the inode will be stalled because it is locked (VI_XLOCK).
	 */
	if (ip->i_nlink <= 0 && (vp->v_mount->mnt_flag & MNT_RDONLY) == 0) {
		UFS_VFREE(vp, ip->i_number, ip->i_omode);
	}
	UFS_UPDATE(vp, NULL, NULL, UPDATE_CLOSE);
	ufs_ihashrem(ip);

	/*
	 * Purge old data structures associated with the inode.
	 */
	cache_purge(vp);
	if (ip->i_devvp) {
		vrele(ip->i_devvp);
		ip->i_devvp = 0;
	}
#ifdef QUOTA
	ufsquota_free(ip);
#endif
#ifdef UFS_DIRHASH
	if (ip->i_dirhash != NULL)
		ufsdirhash_free(ip);
#endif
	return (0);
}

/*
 * allocate a range of blocks in a file.
 * after this function returns, any page entirely contained within the range
 * will map to invalid data and thus must be overwritten before it is made
 * accessible to others.
 */

int
ufs_balloc_range(struct vnode *vp, off_t off, off_t len, kauth_cred_t cred,
    int flags)
{
	off_t neweof;	/* file size after the operation */
	off_t neweob;	/* offset next to the last block after the operation */
	off_t pagestart; /* starting offset of range covered by pgs */
	off_t eob;	/* offset next to allocated blocks */
	struct uvm_object *uobj;
	int i, delta, error, npages;
	int bshift = vp->v_mount->mnt_fs_bshift;
	int bsize = 1 << bshift;
	int ppb = MAX(bsize >> PAGE_SHIFT, 1);
	struct vm_page **pgs;
	size_t pgssize;
	UVMHIST_FUNC("ufs_balloc_range"); UVMHIST_CALLED(ubchist);
	UVMHIST_LOG(ubchist, "vp %p off 0x%x len 0x%x u_size 0x%x",
		    vp, off, len, vp->v_size);

	neweof = MAX(vp->v_size, off + len);
	GOP_SIZE(vp, neweof, &neweob, 0);

	error = 0;
	uobj = &vp->v_uobj;

	/*
	 * read or create pages covering the range of the allocation and
	 * keep them locked until the new block is allocated, so there
	 * will be no window where the old contents of the new block are
	 * visible to racing threads.
	 */

	pagestart = trunc_page(off) & ~(bsize - 1);
	npages = MIN(ppb, (round_page(neweob) - pagestart) >> PAGE_SHIFT);
	pgssize = npages * sizeof(struct vm_page *);
	pgs = kmem_zalloc(pgssize, KM_SLEEP);

	mutex_enter(&uobj->vmobjlock);
	error = VOP_GETPAGES(vp, pagestart, pgs, &npages, 0,
	    VM_PROT_WRITE, 0,
	    PGO_SYNCIO|PGO_PASTEOF|PGO_NOBLOCKALLOC|PGO_NOTIMESTAMP);
	if (error) {
		goto out;
	}
	mutex_enter(&uobj->vmobjlock);
	mutex_enter(&uvm_pageqlock);
	for (i = 0; i < npages; i++) {
		UVMHIST_LOG(ubchist, "got pgs[%d] %p", i, pgs[i],0,0);
		KASSERT((pgs[i]->flags & PG_RELEASED) == 0);
		pgs[i]->flags &= ~PG_CLEAN;
		uvm_pageactivate(pgs[i]);
	}
	mutex_exit(&uvm_pageqlock);
	mutex_exit(&uobj->vmobjlock);

	/*
	 * adjust off to be block-aligned.
	 */

	delta = off & (bsize - 1);
	off -= delta;
	len += delta;

	/*
	 * now allocate the range.
	 */

	genfs_node_wrlock(vp);
	error = GOP_ALLOC(vp, off, len, flags, cred);
	genfs_node_unlock(vp);

	/*
	 * clear PG_RDONLY on any pages we are holding
	 * (since they now have backing store) and unbusy them.
	 */

	GOP_SIZE(vp, off + len, &eob, 0);
	mutex_enter(&uobj->vmobjlock);
	for (i = 0; i < npages; i++) {
		if (error) {
			pgs[i]->flags |= PG_RELEASED;
		} else if (off <= pagestart + (i << PAGE_SHIFT) &&
		    pagestart + ((i + 1) << PAGE_SHIFT) <= eob) {
			pgs[i]->flags &= ~PG_RDONLY;
		}
	}
	if (error) {
		mutex_enter(&uvm_pageqlock);
		uvm_page_unbusy(pgs, npages);
		mutex_exit(&uvm_pageqlock);
	} else {
		uvm_page_unbusy(pgs, npages);
	}
	mutex_exit(&uobj->vmobjlock);

 out:
 	kmem_free(pgs, pgssize);
	return error;
}
