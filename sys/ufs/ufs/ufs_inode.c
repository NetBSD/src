/*	$NetBSD: ufs_inode.c,v 1.13.4.2 1999/07/11 05:51:40 chs Exp $	*/

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
 *	@(#)ufs_inode.c	8.9 (Berkeley) 5/14/95
 */

#include "opt_quota.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/kernel.h>
#include <sys/namei.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

#include <uvm/uvm.h>

/*
 * Last reference to an inode.  If necessary, write or delete it.
 */
int
ufs_inactive(v)
	void *v;
{
	struct vop_inactive_args /* {
		struct vnode *a_vp;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct inode *ip = VTOI(vp);
	struct proc *p = ap->a_p;
	int mode, error = 0;
	extern int prtactive;

	if (prtactive && vp->v_usecount != 0)
		vprint("ffs_inactive: pushing active", vp);

	/*
	 * Ignore inodes related to stale file handles.
	 */
	if (ip->i_ffs_mode == 0)
		goto out;

	if (ip->i_ffs_nlink <= 0 && (vp->v_mount->mnt_flag & MNT_RDONLY) == 0) {
#ifdef QUOTA
		if (!getinoquota(ip))
			(void)chkiq(ip, -1, NOCRED, 0);
#endif
		error = VOP_TRUNCATE(vp, (off_t)0, 0, NOCRED, p);
		ip->i_ffs_rdev = 0;
		mode = ip->i_ffs_mode;
		ip->i_ffs_mode = 0;
		ip->i_flag |= IN_CHANGE | IN_UPDATE;
		VOP_VFREE(vp, ip->i_number, mode);
	}

	if (ip->i_flag & (IN_ACCESS | IN_CHANGE | IN_MODIFIED | IN_UPDATE))
		VOP_UPDATE(vp, NULL, NULL, 0);
out:
	VOP_UNLOCK(vp, 0);
	/*
	 * If we are done with the inode, reclaim it
	 * so that it can be reused immediately.
	 */
	if (ip->i_ffs_mode == 0)
		vrecycle(vp, (struct simplelock *)0, p);
	return (error);
}

/*
 * Reclaim an inode so that it can be used for other purposes.
 */
int
ufs_reclaim(vp, p)
	struct vnode *vp;
	struct proc *p;
{
	register struct inode *ip;
	extern int prtactive;

	if (prtactive && vp->v_usecount != 0)
		vprint("ufs_reclaim: pushing active", vp);
	/*
	 * Remove the inode from its hash chain.
	 */
	ip = VTOI(vp);
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
	{
		int i;
		for (i = 0; i < MAXQUOTAS; i++) {
			if (ip->i_dquot[i] != NODQUOT) {
				dqrele(vp, ip->i_dquot[i]);
				ip->i_dquot[i] = NODQUOT;
			}
		}
	}
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
ufs_balloc_range(vp, off, len, cred, flags)
	struct vnode *vp;
	off_t off, len;
	struct ucred *cred;
	int flags;
{
	off_t pagestart, pageend;
	struct uvm_object *uobj = &vp->v_uvm.u_obj;
	struct inode *ip = VTOI(vp);
	int bshift, bsize, delta, error;
	struct vm_page *pg1, *pg2;
	int npages;
	UVMHIST_FUNC("ufs_balloc_range"); UVMHIST_CALLED(ubchist);

	UVMHIST_LOG(ubchist, "vp %p off 0x%x len 0x%x u_size 0x%x",
		    vp, (int)off, (int)len, (int)vp->v_uvm.u_size);

	bshift = vp->v_mount->mnt_fs_bshift;
	bsize = 1 << bshift;

	/*
	 * if the range does not start on a page boundary,
	 * cache the first page if the file so the page will contain
	 * the correct data.  hold the page busy while we allocate
	 * the backing store for the range.
	 */

	error = 0;
	pg1 = pg2 = NULL;
	pagestart = trunc_page(off);
	if (off != pagestart && pagestart < ip->i_ffs_size) {
		npages = 1;
		simple_lock(&uobj->vmobjlock);
		error = VOP_GETPAGES(vp, pagestart, &pg1, &npages, 0,
				     VM_PROT_READ, 0, PGO_SYNCIO);
		if (error) {
			goto errout;
		}
		UVMHIST_LOG(ubchist, "got first pg %p", pg1,0,0,0);
	}

	/*
	 * similarly if the range does not end on a page boundary.
	 */

	pageend = trunc_page(off + len);
	if (off + len != pageend && pagestart != pageend &&
	    pageend < ip->i_ffs_size) {
		npages = 1;
		simple_lock(&uobj->vmobjlock);
		error = VOP_GETPAGES(vp, pageend, &pg2, &npages, 0,
				     VM_PROT_READ, 0, PGO_SYNCIO);
		if (error) {
			goto errout;
		}
		UVMHIST_LOG(ubchist, "got second pg %p",
			    pg2, (int)(off + len), (int)ip->i_ffs_size,0);
	}

	/*
	 * adjust off to be block-aligned.
	 */

	delta = off & (bsize - 1);
	off -= delta;
	len += delta;

	/*
	 * now allocate the range a block at a time.
	 */

	while (len > 0) {
		bsize = min(bsize, len);

		if ((error = VOP_BALLOC(vp, off, bsize, cred, flags, NULL))) {
			goto errout;
		}

		/*
		 * increase file size now, VOP_BALLOC() requires that
		 * EOF be up-to-date before each call.
		 */

		if (ip->i_ffs_size < off + bsize) {
			ip->i_ffs_size = off + bsize;
			uvm_vnp_setsize(ITOV(ip), ip->i_ffs_size);
		}

		len -= bsize;
		off += bsize;
	}

	/*
	 * unbusy any pages we are holding.
	 */

errout:
	simple_lock(&uobj->vmobjlock);
	if (pg1 != NULL) {
		if (pg1->flags & PG_WANTED) {
			wakeup(pg1);
		}
		if (pg1->flags & PG_RELEASED) {
			UVMHIST_LOG(ubchist, "releasing pg %p", pg1,0,0,0);
			uobj->pgops->pgo_releasepg(pg1, NULL);
		} else {
			UVMHIST_LOG(ubchist, "unbusing pg %p", pg1,0,0,0);
			pg1->flags &= ~(PG_WANTED|PG_BUSY);
			UVM_PAGE_OWN(pg1, NULL);
		}
	}
	if (pg2 != NULL) {
		if (pg2->flags & PG_WANTED) {
			wakeup(pg2);
		}
		if (pg2->flags & PG_RELEASED) {
			UVMHIST_LOG(ubchist, "releasing pg %p", pg2,0,0,0);
			uobj->pgops->pgo_releasepg(pg2, NULL);
		} else {
			UVMHIST_LOG(ubchist, "unbusing pg %p", pg2,0,0,0);
			pg2->flags &= ~(PG_WANTED|PG_BUSY);
			UVM_PAGE_OWN(pg2, NULL);
		}
	}
	simple_unlock(&uobj->vmobjlock);
	return error;
}
