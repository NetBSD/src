/*	$NetBSD: genfs_vnops.c,v 1.11.4.7 1999/08/31 21:03:44 perseant Exp $	*/

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
 */

#include "opt_nfsserver.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/poll.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/specfs/specdev.h>

#include <vm/vm.h>
#include <uvm/uvm.h>
#include <uvm/uvm_pager.h>

#ifdef NFSSERVER
#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nqnfs.h>
#include <nfs/nfs_var.h>
#endif

int
genfs_poll(v)
	void *v;
{
	struct vop_poll_args /* {
		struct vnode *a_vp;
		int a_events;
		struct proc *a_p;
	} */ *ap = v;

	return (ap->a_events & (POLLIN | POLLOUT | POLLRDNORM | POLLWRNORM));
}

int
genfs_fsync(v)
	void *v;
{
	struct vop_fsync_args /* {
		struct vnode *a_vp;
		struct ucred *a_cred;
		int a_flags;
		struct proc *a_p;
	} */ *ap = v;
	register struct vnode *vp = ap->a_vp;
	int wait;

	wait = (ap->a_flags & FSYNC_WAIT) != 0;
	vflushbuf(vp, wait);
	if ((ap->a_flags & FSYNC_DATAONLY) != 0)
		return (0);
	else
		return (VOP_UPDATE(ap->a_vp, NULL, NULL, wait));
}

int
genfs_seek(v)
	void *v;
{
	struct vop_seek_args /* {
		struct vnode *a_vp;
		off_t a_oldoff;
		off_t a_newoff;
		struct ucred *a_ucred;
	} */ *ap = v;

	if (ap->a_newoff < 0)
		return (EINVAL);

	return (0);
}

int
genfs_abortop(v)
	void *v;
{
	struct vop_abortop_args /* {
		struct vnode *a_dvp;
		struct componentname *a_cnp;
	} */ *ap = v;
 
	if ((ap->a_cnp->cn_flags & (HASBUF | SAVESTART)) == HASBUF)
		FREE(ap->a_cnp->cn_pnbuf, M_NAMEI);
	return (0);
}

/*ARGSUSED*/
int
genfs_badop(v)
	void *v;
{

	panic("genfs: bad op");
}

/*ARGSUSED*/
int
genfs_nullop(v)
	void *v;
{

	return (0);
}

/*ARGSUSED*/
int
genfs_einval(v)
	void *v;
{

	return (EINVAL);
}

/*ARGSUSED*/
int
genfs_eopnotsupp(v)
	void *v;
{

	return (EOPNOTSUPP);
}

/*
 * Called when an fs doesn't support a particular vop but the vop needs to
 * vrele, vput, or vunlock passed in vnodes.
 */
int
genfs_eopnotsupp_rele(v)
	void *v;
{
	struct vop_generic_args /*
		struct vnodeop_desc *a_desc;
		/ * other random data follows, presumably * / 
	} */ *ap = v;
	struct vnodeop_desc *desc = ap->a_desc;
	struct vnode *vp;
	int flags, i, j, offset;

	flags = desc->vdesc_flags;
	for (i = 0; i < VDESC_MAX_VPS; flags >>=1, i++) {
		if ((offset = desc->vdesc_vp_offsets[i]) == VDESC_NO_OFFSET)
			break;	/* stop at end of list */
		if ((j = flags & VDESC_VP0_WILLPUT)) {
			vp = *VOPARG_OFFSETTO(struct vnode**,offset,ap);
			switch (j) {
			case VDESC_VP0_WILLPUT:
				vput(vp);
				break;
			case VDESC_VP0_WILLUNLOCK:
				VOP_UNLOCK(vp, 0);
				break;
			case VDESC_VP0_WILLRELE:
				vrele(vp);
				break;
			}
		}
	}

	return (EOPNOTSUPP);
}

/*ARGSUSED*/
int
genfs_ebadf(v)
	void *v;
{

	return (EBADF);
}

/* ARGSUSED */
int
genfs_enoioctl(v)
	void *v;
{

	return (ENOTTY);
}


/*
 * Eliminate all activity associated with  the requested vnode
 * and with all vnodes aliased to the requested vnode.
 */
int
genfs_revoke(v)
	void *v;
{
	struct vop_revoke_args /* {
		struct vnode *a_vp;
		int a_flags;
	} */ *ap = v;
	struct vnode *vp, *vq;
	struct proc *p = curproc;	/* XXX */

#ifdef DIAGNOSTIC
	if ((ap->a_flags & REVOKEALL) == 0)
		panic("genfs_revoke: not revokeall");
#endif

	vp = ap->a_vp;
	simple_lock(&vp->v_interlock);

	if (vp->v_flag & VALIASED) {
		/*
		 * If a vgone (or vclean) is already in progress,
		 * wait until it is done and return.
		 */
		if (vp->v_flag & VXLOCK) {
			vp->v_flag |= VXWANT;
			simple_unlock(&vp->v_interlock);
			tsleep((caddr_t)vp, PINOD, "vop_revokeall", 0);
			return (0);
		}
		/*
		 * Ensure that vp will not be vgone'd while we
		 * are eliminating its aliases.
		 */
		vp->v_flag |= VXLOCK;
		simple_unlock(&vp->v_interlock);
		while (vp->v_flag & VALIASED) {
			simple_lock(&spechash_slock);
			for (vq = *vp->v_hashchain; vq; vq = vq->v_specnext) {
				if (vq->v_rdev != vp->v_rdev ||
				    vq->v_type != vp->v_type || vp == vq)
					continue;
				simple_unlock(&spechash_slock);
				vgone(vq);
				break;
			}
			if (vq == NULLVP)
				simple_unlock(&spechash_slock);
		}
		/*
		 * Remove the lock so that vgone below will
		 * really eliminate the vnode after which time
		 * vgone will awaken any sleepers.
		 */
		simple_lock(&vp->v_interlock);
		vp->v_flag &= ~VXLOCK;
	}
	vgonel(vp, p);
	return (0);
}

/*
 * Lock the node.
 */
int
genfs_lock(v)
	void *v;
{
	struct vop_lock_args /* {
		struct vnode *a_vp;
		int a_flags;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	return (lockmgr(&vp->v_lock, ap->a_flags, &vp->v_interlock));
}

/*
 * Unlock the node.
 */
int
genfs_unlock(v)
	void *v;
{
	struct vop_unlock_args /* {
		struct vnode *a_vp;
		int a_flags;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	return (lockmgr(&vp->v_lock, ap->a_flags | LK_RELEASE,
		&vp->v_interlock));
}

/*
 * Return whether or not the node is locked.
 */
int
genfs_islocked(v)
	void *v;
{
	struct vop_islocked_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	return (lockstatus(&vp->v_lock));
}

/*
 * Stubs to use when there is no locking to be done on the underlying object.
 */
int
genfs_nolock(v)
	void *v;
{
	struct vop_lock_args /* {
		struct vnode *a_vp;
		int a_flags;
		struct proc *a_p;
	} */ *ap = v;

	/*
	 * Since we are not using the lock manager, we must clear
	 * the interlock here.
	 */
	if (ap->a_flags & LK_INTERLOCK)
		simple_unlock(&ap->a_vp->v_interlock);
	return (0);
}

int
genfs_nounlock(v)
	void *v;
{
	return (0);
}

int
genfs_noislocked(v)
	void *v;
{
	return (0);
}

/*
 * Local lease check for NFS servers.  Just set up args and let
 * nqsrv_getlease() do the rest.  If NFSSERVER is not in the kernel,
 * this is a null operation.
 */
int
genfs_lease_check(v)
	void *v;
{
#ifdef NFSSERVER
	struct vop_lease_args /* {
		struct vnode *a_vp;
		struct proc *a_p;
		struct ucred *a_cred;
		int a_flag;
	} */ *ap = v;
	u_int32_t duration = 0;
	int cache;
	u_quad_t frev;

	(void) nqsrv_getlease(ap->a_vp, &duration, ND_CHECK | ap->a_flag,
	    NQLOCALSLP, ap->a_p, (struct mbuf *)0, &cache, &frev, ap->a_cred);
	return (0);
#else
	return (0);
#endif /* NFSSERVER */
}

/*
 * generic VM getpages routine.
 * Return PG_BUSY pages for the given range,
 * reading from backing store if necessary.
 */

int
genfs_getpages(v)
	void *v;
{
	struct vop_getpages_args /* {
		struct vnode *a_vp;
		voff_t a_offset;
		vm_page_t *a_m;
		int *a_count;
		int a_centeridx;
		vm_prot_t a_access_type;
		int a_advice;
		int a_flags;
	} */ *ap = v;

	off_t eof, offset, origoffset, startoffset, endoffset;
	daddr_t lbn, blkno;
	int s, i, error, npages, npgs, run, ridx, pidx, pcount;
	int bsize, bshift, dev_bshift, dev_bsize;
	int flags = ap->a_flags;
	size_t bytes, iobytes, tailbytes, totalbytes, skipbytes;
	boolean_t sawhole = FALSE;
	char *kva;
	struct buf *bp, *mbp;
	struct vnode *vp = ap->a_vp;
	struct uvm_object *uobj = &vp->v_uvm.u_obj;
	struct vm_page *pgs[16];			/* XXX 16 */
	struct ucred *cred = curproc->p_ucred;		/* XXX curproc */
	UVMHIST_FUNC("genfs_getpages"); UVMHIST_CALLED(ubchist);

#ifdef DIAGNOSTIC
	if (ap->a_centeridx < 0 || ap->a_centeridx > *ap->a_count) {
		panic("genfs_getpages: centeridx %d out of range",
		      ap->a_centeridx);
	}
	if (ap->a_offset & (PAGE_SIZE - 1)) {
		panic("genfs_getpages: offset 0x%x", (int)ap->a_offset);
	}
	if (*ap->a_count < 0) {
		panic("genfs_getpages: count %d < 0", *ap->a_count);
	}
#endif

	/*
	 * Bounds-check the request.
	 */

	eof = vp->v_uvm.u_size;
	if (ap->a_offset >= eof) {
		if ((flags & PGO_LOCKED) == 0) {
			simple_unlock(&uobj->vmobjlock);
		}
		UVMHIST_LOG(ubchist, "off 0x%x count %d goes past EOF 0x%x",
			    (int)ap->a_offset, *ap->a_count, (int)eof,0);
		return EINVAL;
	}

	/*
	 * For PGO_LOCKED requests, just return whatever's in memory.
	 */

	if (flags & PGO_LOCKED) {
		uvn_findpages(uobj, ap->a_offset, ap->a_count, ap->a_m,
			      UFP_NOWAIT|UFP_NOALLOC|UFP_NORDONLY);

		return ap->a_m[ap->a_centeridx] == NULL ? EBUSY : 0;
	}

	if (ap->a_offset + ((*ap->a_count - 1) << PAGE_SHIFT) >= eof) {
		panic("genfs_getpages: non LOCKED req past EOF vp %p", vp);
	}

	/* vnode is VOP_LOCKed, uobj is locked */

	error = 0;

	/*
	 * find the requested pages and make some simple checks.
	 * leave space in the page array for a whole block.
	 */

	bshift = vp->v_mount->mnt_fs_bshift;
	bsize = 1 << bshift;
	dev_bshift = vp->v_mount->mnt_dev_bshift;
	dev_bsize = 1 << dev_bshift;

	npages = *ap->a_count;
	origoffset = ap->a_offset;
	startoffset = origoffset & ~((off_t)bsize - 1);
	endoffset = round_page((origoffset + (npages << PAGE_SHIFT)
				+ bsize - 1) & ~((off_t)bsize - 1));
	ridx = (origoffset - startoffset) >> PAGE_SHIFT;

	memset(pgs, 0, sizeof(pgs));
	uvn_findpages(uobj, origoffset, &npages, &pgs[ridx], UFP_ALL);

	/*
	 * if PGO_OVERWRITE is set, don't bother reading the pages.
	 * PGO_OVERWRITE also means that the caller guarantees
	 * that the pages already have backing store allocated.
	 */

	if (flags & PGO_OVERWRITE) {
		UVMHIST_LOG(ubchist, "PGO_OVERWRITE",0,0,0,0);

		/* XXX for now, zero the page if we allocated it */
		for (i = 0; i < npages; i++) {
			struct vm_page *pg = pgs[ridx + i];
			if (pg->flags & PG_FAKE) {
				uvm_pagezero(pg);
				pg->flags &= ~PG_FAKE;
			}
		}

		simple_unlock(&uobj->vmobjlock);
		goto out;
	}

	/*
	 * if the pages are already resident, just return them.
	 */

	for (i = 0; i < npages; i++) {
		struct vm_page *pg = pgs[ridx + i];

		if ((pg->flags & PG_FAKE) != 0 ||
		    ((ap->a_access_type & VM_PROT_WRITE) &&
		      (pg->flags & PG_RDONLY))) {
			break;
		}
	}
	if (i == npages) {
		UVMHIST_LOG(ubchist, "returning cached pages", 0,0,0,0);
		simple_unlock(&uobj->vmobjlock);
		goto out;
	}

	/*
	 * the page wasn't resident and we're not overwriting,
	 * so we're going to have to do some i/o.
	 * find any additional pages needed to cover the expanded range.
	 */

	if (startoffset != origoffset) {
		UVMHIST_LOG(ubchist, "reset npages start 0x%x end 0x%x",
			    (int)startoffset, (int)endoffset, 0,0);
		npages = (endoffset - startoffset) >> PAGE_SHIFT;
		if (npages == 0) {
			panic("XXX getpages npages = 0");
		}
		npgs = npages;
		uvn_findpages(uobj, startoffset, &npgs, pgs, UFP_ALL);
	}
	simple_unlock(&uobj->vmobjlock);

	/*
	 * read the desired page(s).
	 */

	totalbytes = npages << PAGE_SHIFT;
	bytes = min(totalbytes,
		    (vp->v_uvm.u_size - startoffset + dev_bsize - 1) &
		    ~(dev_bsize - 1));
	tailbytes = totalbytes - bytes;
	skipbytes = 0;

	kva = (void *)uvm_pagermapin(pgs, npages, M_WAITOK);

	s = splbio();
	mbp = pool_get(&bufpool, PR_WAITOK);
	splx(s);
	mbp->b_bufsize = bytes;
	mbp->b_data = kva;
	mbp->b_resid = mbp->b_bcount = bytes;
	mbp->b_flags = B_BUSY|B_READ| (flags & PGO_SYNCIO ? 0 : B_CALL);
	mbp->b_iodone = uvm_aio_biodone;
	mbp->b_vp = vp;

	/*
	 * if EOF is in the middle of the last page, zero the part past EOF.
	 */

	if (tailbytes > 0) {
		memset(kva + bytes, 0, tailbytes);
	}

	/*
	 * now loop over the pages, reading as needed.
	 */

	bp = NULL;
	offset = startoffset;
	for (; bytes > 0; offset += iobytes, bytes -= iobytes) {

		/*
		 * skip pages which don't need to be read.
		 */

		pidx = (offset - startoffset) >> PAGE_SHIFT;
		while ((pgs[pidx]->flags & PG_FAKE) == 0) {
			size_t b;

			if (offset & (PAGE_SIZE - 1)) {
				panic("genfs_getpages: skipping from middle "
				      "of page");
			}

			b = min(PAGE_SIZE, bytes);
			offset += b;
			bytes -= b;
			skipbytes += b;
			pidx++;
			UVMHIST_LOG(ubchist, "skipping, new offset 0x%x",
				    (int)offset, 0,0,0);
			if (bytes == 0) {
				goto loopdone;
			}
		}

		/*
		 * bmap the file to find out the blkno to read from and
		 * how much we can read in one i/o.  if bmap returns an error,
		 * skip the rest of the top-level i/o.
		 */

		lbn = offset >> bshift;
		error = VOP_BMAP(vp, lbn, NULL, &blkno, &run);
		if (error) {
			UVMHIST_LOG(ubchist, "VOP_BMAP lbn 0x%x -> %d\n",
				    lbn, error,0,0);
			skipbytes += bytes;
			tailbytes = 0;
			goto loopdone;
		}

		/*
		 * see how many pages can be read with this i/o.
		 * reduce the i/o size if necessary.
		 */

		iobytes = min(((lbn + 1 + run) << bshift) - offset, bytes);
		if (offset + iobytes > round_page(offset)) {
			pcount = 1;
			while (pidx + pcount < npages &&
			       pgs[pidx + pcount]->flags & PG_FAKE) {
				pcount++;
			}
			iobytes = min(iobytes, (pcount << PAGE_SHIFT) -
				      (offset - trunc_page(offset)));
		}

		/*
		 * if this block isn't allocated, zero it instead of reading it.
		 * if this is a read access, mark the pages we zeroed PG_RDONLY.
		 */

		if (blkno < 0) {
			UVMHIST_LOG(ubchist, "lbn 0x%x -> HOLE", lbn,0,0,0);

			sawhole = TRUE;
			memset(kva + (offset - startoffset), 0, iobytes);

			if (ap->a_access_type == VM_PROT_READ) {
				int holepages =
					(round_page(offset + iobytes) - 
					 trunc_page(offset)) >> PAGE_SHIFT;
				for (i = 0; i < holepages; i++) {
					pgs[pidx + i]->flags |= PG_RDONLY;
				}
			}
			continue;
		}

		/*
		 * allocate a sub-buf for this piece of the i/o
		 * (or just use mbp if there's only 1 piece),
		 * and start it going.
		 */

		if (offset == startoffset && iobytes == bytes) {
			bp = mbp;
		} else {
			s = splbio();
			bp = pool_get(&bufpool, PR_WAITOK);
			splx(s);
			bp->b_data = kva + offset - startoffset;
			bp->b_resid = bp->b_bcount = iobytes;
			bp->b_flags = B_BUSY|B_READ|B_CALL;
			bp->b_iodone = uvm_aio_biodone1;
			bp->b_vp = vp;
		}
		bp->b_lblkno = 0;
		bp->b_private = mbp;

		/* adjust physical blkno for partial blocks */
		bp->b_blkno = blkno + ((offset - (lbn << bshift)) >>
				       dev_bshift);

		UVMHIST_LOG(ubchist, "bp %p offset 0x%x bcount 0x%x blkno 0x%x",
			    bp, (int)offset, (int)iobytes, bp->b_blkno);

		VOP_STRATEGY(bp);
	}

loopdone:
	s = splbio();
	if (skipbytes) {
		mbp->b_resid -= skipbytes;
		if (mbp->b_resid == 0) {
			biodone(mbp);
		}
	}
	splx(s);
	if ((flags & PGO_SYNCIO) == 0) {
		UVMHIST_LOG(ubchist, "returning PEND",0,0,0,0);
		return EINPROGRESS;
	}
	if (bp != NULL) {
		error = biowait(mbp);
	}
	s = splbio();
	pool_put(&bufpool, mbp);
	splx(s);
	for (i = 0; i < npages; i++) {
		UVMHIST_LOG(ubchist, "pgs[%d][0] = 0x%x",
			    i, *(int *)(kva + (i << PAGE_SHIFT)), 0,0);
	}
	uvm_pagermapout((vaddr_t)kva, npages);

	/*
	 * if this we encountered a hole then we have to do a little more work.
	 * for read faults, we must mark the page PG_RDONLY so that future
	 * write accesses to the page will fault again.
	 * for write faults, we must make sure that the backing store for
	 * the page is completely allocated.
	 */

	if (sawhole && ap->a_access_type == VM_PROT_WRITE) {
		error = VOP_BALLOC(vp, startoffset, npages << PAGE_SHIFT,
				   cred, 0);
		if (error) {
			UVMHIST_LOG(ubchist, "balloc lbn 0x%x -> %d",
				    lbn, error,0,0);
			goto out;
		}
	}

	/*
	 * see if we want to start any readahead.
	 * XXX writeme
	 */

	/*
	 * we're almost done!  release the pages...
	 * for errors, we free the pages.
	 * otherwise we activate them and mark them as valid and clean.
	 * also, unbusy all but the center page.
	 */

out:
	if (error) {
		simple_lock(&uobj->vmobjlock);
		for (i = 0; i < npages; i++) {
			UVMHIST_LOG(ubchist, "examining pg %p flags 0x%x",
				    pgs[i], pgs[i]->flags, 0,0);
			if (pgs[i]->flags & PG_FAKE) {
				if (pgs[i]->flags & PG_WANTED) {
					wakeup(pgs[i]);
				}
				uvm_pagefree(pgs[i]);
			}
		}
		simple_unlock(&uobj->vmobjlock);
		UVMHIST_LOG(ubchist, "returning error %d", error,0,0,0);
		return error;
	}

	UVMHIST_LOG(ubchist, "succeeding, npages %d", npages,0,0,0);
	simple_lock(&uobj->vmobjlock);
	for (i = 0; i < npages; i++) {
		if (pgs[i] == NULL) {
			continue;
		}
		UVMHIST_LOG(ubchist, "examining pg %p flags 0x%x",
			    pgs[i], pgs[i]->flags, 0,0);
		if (pgs[i]->flags & PG_FAKE) {
			UVMHIST_LOG(ubchist, "unfaking pg %p offset 0x%x",
				    pgs[i], (int)pgs[i]->offset,0,0);
			pgs[i]->flags &= ~(PG_FAKE);
			pmap_clear_modify(PMAP_PGARG(pgs[i]));
			pmap_clear_reference(PMAP_PGARG(pgs[i]));
		}
		if (i < ridx || i >= ridx + *ap->a_count) {
			UVMHIST_LOG(ubchist, "unbusy pg %p offset 0x%x",
				    pgs[i], (int)pgs[i]->offset,0,0);
			/*
			KASSERT((pgs[i]->flags & PG_RELEASED) == 0);
			*/

			if (pgs[i]->flags & PG_WANTED) {
				wakeup(pgs[i]);
			}
			pgs[i]->flags &= ~(PG_WANTED|PG_BUSY);
			UVM_PAGE_OWN(pgs[i], NULL);
		}
	}
	simple_unlock(&uobj->vmobjlock);
	memcpy(ap->a_m, &pgs[ridx], *ap->a_count * sizeof(struct vm_page *));
	return 0;
}

/*
 * generic VM putpages routine.
 * Write the given range of pages to backing store.
 */
int
genfs_putpages(v)
	void *v;
{
	struct vop_putpages_args /* {
		struct vnode *a_vp;
		struct vm_page **a_m;
		int a_count;
		int a_flags;
		int *a_rtvals;
	} */ *ap = v;

	int s, error, npages, bshift, dev_bshift, dev_bsize, run;
	char * kva;
	off_t offset, startoffset;
	size_t bytes, iobytes, skipbytes;
	daddr_t lbn, blkno;
	struct vm_page *pg;
	struct buf *mbp, *bp;
	struct vnode *vp = ap->a_vp;
	UVMHIST_FUNC("genfs_putpages"); UVMHIST_CALLED(ubchist);

	error = 0;
	npages = ap->a_count;
	bshift = vp->v_mount->mnt_fs_bshift;
	dev_bshift = vp->v_mount->mnt_dev_bshift;
	dev_bsize = 1 << dev_bshift;

	pg = ap->a_m[0];
	startoffset = pg->offset;
	bytes = min(npages << PAGE_SHIFT,
		    (vp->v_uvm.u_size - startoffset + dev_bsize - 1) &
		    ~((off_t)dev_bsize - 1));
	skipbytes = 0;

	if (bytes == 0) {
		panic("genfs_putpages: bytes == 0??? vp %p", vp);
	}

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
	mbp->b_flags = B_BUSY|B_WRITE| ((ap->a_flags & PGO_SYNCIO) ? 0 : B_CALL) |
		(curproc == uvm.pagedaemon_proc ? B_PDAEMON : 0);
	mbp->b_iodone = uvm_aio_biodone;
	mbp->b_vp = vp;

	bp = NULL;
	offset = startoffset;
	for (; bytes > 0; offset += iobytes, bytes -= iobytes) {
		lbn = offset >> bshift;
		error = VOP_BMAP(vp, lbn, NULL, &blkno, &run);
		if (error) {
			UVMHIST_LOG(ubchist, "VOP_BMAP() -> %d", error,0,0,0);
			goto errout;
		}

		iobytes = min(((lbn + 1 + run) << bshift) - offset, bytes);
		if (blkno == (daddr_t)-1) {
			skipbytes += iobytes;
			continue;
		}

		/* if it's really one i/o, don't make a second buf */
		if (offset == startoffset && iobytes == bytes) {
			bp = mbp;
		} else {
			s = splbio();
			vp->v_numoutput++;
			bp = pool_get(&bufpool, PR_WAITOK);
			UVMHIST_LOG(ubchist, "vp %p bp %p num now %d",
				    vp, bp, vp->v_numoutput, 0);
			splx(s);
			bp->b_data = kva + offset - pg->offset;
			bp->b_resid = bp->b_bcount = iobytes;
			bp->b_flags = B_BUSY|B_WRITE|B_CALL;
			bp->b_iodone = uvm_aio_biodone1;
			bp->b_vp = vp;
		}
		bp->b_lblkno = 0;
		bp->b_private = mbp;

		/* adjust physical blkno for partial blocks */
		bp->b_blkno = blkno + ((offset - (lbn << bshift)) >>
				       dev_bshift);
		UVMHIST_LOG(ubchist, "vp %p offset 0x%x bcount 0x%x blkno 0x%x",
			    vp, (int)offset, (int)bp->b_bcount,
			    (int)bp->b_blkno);
		VOP_STRATEGY(bp);
	}
	s = splbio();
	if (skipbytes) {
		mbp->b_resid -= skipbytes;
		if (mbp->b_resid == 0) {
			biodone(mbp);
		}
	}
	splx(s);
	if (!(ap->a_flags & PGO_SYNCIO)) {
		return EINPROGRESS;
	}

errout:
	if (bp != NULL) {
		error = biowait(mbp);
	}
	s = splbio();
	pool_put(&bufpool, mbp);
	splx(s);
	uvm_pagermapout((vaddr_t)kva, npages);
	UVMHIST_LOG(ubchist, "returning, error %d", error,0,0,0);
	return error;
}
