/*	$NetBSD: ufs_inode.c,v 1.13.4.3 1999/07/31 18:52:38 chs Exp $	*/

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
	off_t eof, pagestart, pageend;
	struct uvm_object *uobj;
	struct inode *ip = VTOI(vp);
	int i, delta, error, npages1, npages2;
	int bshift = vp->v_mount->mnt_fs_bshift;
	int bsize = 1 << bshift;
	int ppb = max(bsize >> PAGE_SHIFT, PAGE_SIZE >> bshift);
	struct vm_page *pgs1[ppb], *pgs2[ppb];
	UVMHIST_FUNC("ufs_balloc_range"); UVMHIST_CALLED(ubchist);
	UVMHIST_LOG(ubchist, "vp %p off 0x%x len 0x%x u_size 0x%x",
		    vp, (int)off, (int)len, (int)vp->v_uvm.u_size);

	/*
	 * if the range does not start on a page and block boundary,
	 * cache the first block if the file so the page(s) will contain
	 * the correct data.  hold the page(s) busy while we allocate
	 * the backing store for the range.
	 */

	uobj = &vp->v_uvm.u_obj;
	eof = max(vp->v_uvm.u_size, off + len);
	vp->v_uvm.u_size = eof;
	UVMHIST_LOG(ubchist, "new eof 0x%x", (int)eof,0,0,0);
	error = 0;
	pgs1[0] = pgs2[0] = NULL;
	pagestart = trunc_page(off) & ~(bsize - 1);
	if (off != pagestart) {
		npages1 = min(ppb, (round_page(eof) - pagestart) >>
			      PAGE_SHIFT);
		memset(pgs1, 0, npages1);
		simple_lock(&uobj->vmobjlock);
		error = VOP_GETPAGES(vp, pagestart, pgs1, &npages1, 0,
				     VM_PROT_READ, 0, PGO_SYNCIO);
		if (error) {
			goto errout;
		}
		for (i = 0; i < npages1; i++) {
			UVMHIST_LOG(ubchist, "got pgs1[%d] %p", i, pgs1[i],0,0);
		}
	}

	/*
	 * similarly if the range does not end on a page and block boundary.
	 */

	pageend = trunc_page(off + len) & ~(bsize - 1);
	if (off + len < ip->i_ffs_size &&
	    off + len != pageend &&
	    pagestart != pageend) {
		npages2 = min(ppb, (round_page(eof) - pageend) >>
			      PAGE_SHIFT);
		memset(pgs2, 0, npages2);
		simple_lock(&uobj->vmobjlock);
		error = VOP_GETPAGES(vp, pageend, pgs2, &npages2, 0,
				     VM_PROT_READ, 0, PGO_SYNCIO);
		if (error) {
			goto errout;
		}
		for (i = 0; i < npages2; i++) {
			UVMHIST_LOG(ubchist, "got pgs2[%d] %p", i, pgs2[i],0,0);
		}
	}

	/*
	 * adjust off to be block-aligned.
	 */

	delta = off & (bsize - 1);
	off -= delta;
	len += delta;

	/*
	 * now allocate the range.
	 */

	if ((error = VOP_BALLOC(vp, off, len, cred, flags))) {
		goto errout;
	}

	/*
	 * unbusy any pages we are holding.
	 */

errout:
	simple_lock(&uobj->vmobjlock);
	if (pgs1[0] != NULL) {
		uvm_page_unbusy(pgs1, npages1);
	}
	if (pgs2[0] != NULL) {
		uvm_page_unbusy(pgs2, npages2);
	}
	simple_unlock(&uobj->vmobjlock);
	return error;
}
