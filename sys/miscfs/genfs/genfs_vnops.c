/*	$NetBSD: genfs_vnops.c,v 1.13.2.4 2001/01/05 17:36:47 bouyer Exp $	*/

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
#include <sys/fcntl.h>
#include <sys/malloc.h>
#include <sys/poll.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/specfs/specdev.h>

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
		off_t offlo;
		off_t offhi;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	int wait;

	wait = (ap->a_flags & FSYNC_WAIT) != 0;
	vflushbuf(vp, wait);
	if ((ap->a_flags & FSYNC_DATAONLY) != 0)
		return (0);
	else
		return (VOP_UPDATE(vp, NULL, NULL, wait ? UPDATE_WAIT : 0));
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
		PNBUF_PUT(ap->a_cnp->cn_pnbuf);
	return (0);
}

int
genfs_fcntl(v)
	void *v;
{
	struct vop_fcntl_args /* {
		struct vnode *a_vp;
		u_int a_command;
		caddr_t a_data;
		int a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;

	if (ap->a_command == F_SETFL)
		return (0);
	else
		return (EOPNOTSUPP);
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
 * Eliminate all activity associated with the requested vnode
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

	off_t eof, offset, origoffset, startoffset, endoffset, raoffset;
	daddr_t lbn, blkno;
	int s, i, error, npages, orignpages, npgs, run, ridx, pidx, pcount;
	int fs_bshift, fs_bsize, dev_bshift, dev_bsize;
	int flags = ap->a_flags;
	size_t bytes, iobytes, tailbytes, totalbytes, skipbytes;
	vaddr_t kva;
	struct buf *bp, *mbp;
	struct vnode *vp = ap->a_vp;
	struct uvm_object *uobj = &vp->v_uvm.u_obj;
	struct vm_page *pgs[16];			/* XXXUBC 16 */
	struct ucred *cred = curproc->p_ucred;		/* XXXUBC curproc */
	boolean_t async = (flags & PGO_SYNCIO) == 0;
	boolean_t write = (ap->a_access_type & VM_PROT_WRITE) != 0;
	boolean_t sawhole = FALSE;
	UVMHIST_FUNC("genfs_getpages"); UVMHIST_CALLED(ubchist);

	/* XXXUBC temp limit */
	if (*ap->a_count > 16) {
		return EINVAL;
	}

	error = VOP_SIZE(vp, vp->v_uvm.u_size, &eof);
	if (error) {
		return error;
	}

#ifdef DIAGNOSTIC
	if (ap->a_centeridx < 0 || ap->a_centeridx > *ap->a_count) {
		panic("genfs_getpages: centeridx %d out of range",
		      ap->a_centeridx);
	}
	if (ap->a_offset & (PAGE_SIZE - 1) || ap->a_offset < 0) {
		panic("genfs_getpages: offset 0x%x", (int)ap->a_offset);
	}
	if (*ap->a_count < 0) {
		panic("genfs_getpages: count %d < 0", *ap->a_count);
	}
#endif

	/*
	 * Bounds-check the request.
	 */

	error = 0;
	origoffset = ap->a_offset;

	if (origoffset + (ap->a_centeridx << PAGE_SHIFT) >= eof &&
	    (flags & PGO_PASTEOF) == 0) {
		if ((flags & PGO_LOCKED) == 0) {
			simple_unlock(&uobj->vmobjlock);
		}
		UVMHIST_LOG(ubchist, "off 0x%x count %d goes past EOF 0x%x",
			    origoffset, *ap->a_count, eof,0);
		return EINVAL;
	}

	/*
	 * For PGO_LOCKED requests, just return whatever's in memory.
	 */

	if (flags & PGO_LOCKED) {
		uvn_findpages(uobj, origoffset, ap->a_count, ap->a_m,
			      UFP_NOWAIT|UFP_NOALLOC|UFP_NORDONLY);

		return ap->a_m[ap->a_centeridx] == NULL ? EBUSY : 0;
	}

	/* vnode is VOP_LOCKed, uobj is locked */

	if (write && (vp->v_flag & VONWORKLST) == 0) {
		vn_syncer_add_to_worklist(vp, filedelay);
	}

	/*
	 * find the requested pages and make some simple checks.
	 * leave space in the page array for a whole block.
	 */

	fs_bshift = vp->v_mount->mnt_fs_bshift;
	fs_bsize = 1 << fs_bshift;
	dev_bshift = vp->v_mount->mnt_dev_bshift;
	dev_bsize = 1 << dev_bshift;
	KASSERT((eof & (dev_bsize - 1)) == 0);

	orignpages = min(*ap->a_count,
	    round_page(eof - origoffset) >> PAGE_SHIFT);
	if (flags & PGO_PASTEOF) {
		orignpages = *ap->a_count;
	}
	npages = orignpages;
	startoffset = origoffset & ~(fs_bsize - 1);
	endoffset = round_page((origoffset + (npages << PAGE_SHIFT)
				+ fs_bsize - 1) & ~(fs_bsize - 1));
	endoffset = min(endoffset, round_page(eof));
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

		for (i = 0; i < npages; i++) {
			struct vm_page *pg = pgs[ridx + i];

			if (pg->flags & PG_FAKE) {
				uvm_pagezero(pg);
				pg->flags &= ~(PG_FAKE);
			}
			pg->flags &= ~(PG_RDONLY);
		}
		goto out;
	}

	/*
	 * if the pages are already resident, just return them.
	 */

	for (i = 0; i < npages; i++) {
		struct vm_page *pg = pgs[ridx + i];

		if ((pg->flags & PG_FAKE) ||
		    (write && (pg->flags & PG_RDONLY))) {
			break;
		}
	}
	if (i == npages) {
		UVMHIST_LOG(ubchist, "returning cached pages", 0,0,0,0);
		raoffset = origoffset + (orignpages << PAGE_SHIFT);
		goto raout;
	}

	/*
	 * the page wasn't resident and we're not overwriting,
	 * so we're going to have to do some i/o.
	 * find any additional pages needed to cover the expanded range.
	 */

	if (startoffset != origoffset) {

		/*
		 * XXXUBC we need to avoid deadlocks caused by locking
		 * additional pages at lower offsets than pages we
		 * already have locked.  for now, unlock them all and
		 * start over.
		 */

		for (i = 0; i < npages; i++) {
			struct vm_page *pg = pgs[ridx + i];

			if (pg->flags & PG_FAKE) {
				pg->flags |= PG_RELEASED;
			}
		}
		uvm_page_unbusy(&pgs[ridx], npages);
		memset(pgs, 0, sizeof(pgs));

		UVMHIST_LOG(ubchist, "reset npages start 0x%x end 0x%x",
			    startoffset, endoffset, 0,0);
		npages = (endoffset - startoffset) >> PAGE_SHIFT;
		npgs = npages;
		uvn_findpages(uobj, startoffset, &npgs, pgs, UFP_ALL);
	}
	simple_unlock(&uobj->vmobjlock);

	/*
	 * read the desired page(s).
	 */

	totalbytes = npages << PAGE_SHIFT;
	bytes = min(totalbytes, eof - startoffset);
	tailbytes = totalbytes - bytes;
	skipbytes = 0;

	kva = uvm_pagermapin(pgs, npages, UVMPAGER_MAPIN_WAITOK |
			     UVMPAGER_MAPIN_READ);

	s = splbio();
	mbp = pool_get(&bufpool, PR_WAITOK);
	splx(s);
	mbp->b_bufsize = totalbytes;
	mbp->b_data = (void *)kva;
	mbp->b_resid = mbp->b_bcount = bytes;
	mbp->b_flags = B_BUSY|B_READ| (async ? B_CALL : 0);
	mbp->b_iodone = uvm_aio_biodone;
	mbp->b_vp = vp;
	LIST_INIT(&mbp->b_dep);

	/*
	 * if EOF is in the middle of the last page, zero the part past EOF.
	 */

	if (tailbytes > 0 && (pgs[bytes >> PAGE_SHIFT]->flags & PG_FAKE)) {
		memset((void *)(kva + bytes), 0, tailbytes);
	}

	/*
	 * now loop over the pages, reading as needed.
	 */

	if (write) {
		lockmgr(&vp->v_glock, LK_EXCLUSIVE, NULL);
	} else {
		lockmgr(&vp->v_glock, LK_SHARED, NULL);
	}

	bp = NULL;
	for (offset = startoffset;
	     bytes > 0;
	     offset += iobytes, bytes -= iobytes) {

		/*
		 * skip pages which don't need to be read.
		 */

		pidx = (offset - startoffset) >> PAGE_SHIFT;
		while ((pgs[pidx]->flags & PG_FAKE) == 0) {
			size_t b;

			KASSERT((offset & (PAGE_SIZE - 1)) == 0);
			b = min(PAGE_SIZE, bytes);
			offset += b;
			bytes -= b;
			skipbytes += b;
			pidx++;
			UVMHIST_LOG(ubchist, "skipping, new offset 0x%x",
				    offset, 0,0,0);
			if (bytes == 0) {
				goto loopdone;
			}
		}

		/*
		 * bmap the file to find out the blkno to read from and
		 * how much we can read in one i/o.  if bmap returns an error,
		 * skip the rest of the top-level i/o.
		 */

		lbn = offset >> fs_bshift;
		error = VOP_BMAP(vp, lbn, NULL, &blkno, &run);
		if (error) {
			UVMHIST_LOG(ubchist, "VOP_BMAP lbn 0x%x -> %d\n",
				    lbn, error,0,0);
			skipbytes += bytes;
			goto loopdone;
		}

		/*
		 * see how many pages can be read with this i/o.
		 * reduce the i/o size if necessary to avoid
		 * overwriting pages with valid data.
		 */

		iobytes = min(((lbn + 1 + run) << fs_bshift) - offset, bytes);
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
			memset((char *)kva + (offset - startoffset), 0,
			       iobytes);
			skipbytes += iobytes;

			if (!write) {
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
			bp->b_data = (char *)kva + offset - startoffset;
			bp->b_resid = bp->b_bcount = iobytes;
			bp->b_flags = B_BUSY|B_READ|B_CALL;
			bp->b_iodone = uvm_aio_biodone1;
			bp->b_vp = vp;
			LIST_INIT(&bp->b_dep);
		}
		bp->b_lblkno = 0;
		bp->b_private = mbp;

		/* adjust physical blkno for partial blocks */
		bp->b_blkno = blkno + ((offset - (lbn << fs_bshift)) >>
				       dev_bshift);

		UVMHIST_LOG(ubchist, "bp %p offset 0x%x bcount 0x%x blkno 0x%x",
			    bp, offset, iobytes, bp->b_blkno);

		VOP_STRATEGY(bp);
	}

loopdone:
	if (skipbytes) {
		s = splbio();
		if (error) {
			mbp->b_flags |= B_ERROR;
			mbp->b_error = error;
		}
		mbp->b_resid -= skipbytes;
		if (mbp->b_resid == 0) {
			biodone(mbp);
		}
		splx(s);
	}

	if (async) {
		UVMHIST_LOG(ubchist, "returning PEND",0,0,0,0);
		lockmgr(&vp->v_glock, LK_RELEASE, NULL);
		return EINPROGRESS;
	}
	if (bp != NULL) {
		error = biowait(mbp);
	}
	s = splbio();
	pool_put(&bufpool, mbp);
	splx(s);
	uvm_pagermapout(kva, npages);
	raoffset = startoffset + totalbytes;

	/*
	 * if this we encountered a hole then we have to do a little more work.
	 * for read faults, we marked the page PG_RDONLY so that future
	 * write accesses to the page will fault again.
	 * for write faults, we must make sure that the backing store for
	 * the page is completely allocated while the pages are locked.
	 */

	if (error == 0 && sawhole && write) {
		error = VOP_BALLOCN(vp, startoffset, npages << PAGE_SHIFT,
				   cred, 0);
		if (error) {
			UVMHIST_LOG(ubchist, "balloc lbn 0x%x -> %d",
				    lbn, error,0,0);
			lockmgr(&vp->v_glock, LK_RELEASE, NULL);
			simple_lock(&uobj->vmobjlock);
			goto out;
		}
	}
	lockmgr(&vp->v_glock, LK_RELEASE, NULL);
	simple_lock(&uobj->vmobjlock);

	/*
	 * see if we want to start any readahead.
	 * XXXUBC for now, just read the next 128k on 64k boundaries.
	 * this is pretty nonsensical, but it is 50% faster than reading
	 * just the next 64k.
	 */

raout:
	if (!error && !async && !write && ((int)raoffset & 0xffff) == 0 &&
	    PAGE_SHIFT <= 16) {
		int racount;

		racount = 1 << (16 - PAGE_SHIFT);
		(void) VOP_GETPAGES(vp, raoffset, NULL, &racount, 0,
				    VM_PROT_READ, 0, 0);
		simple_lock(&uobj->vmobjlock);

		racount = 1 << (16 - PAGE_SHIFT);
		(void) VOP_GETPAGES(vp, raoffset + 0x10000, NULL, &racount, 0,
				    VM_PROT_READ, 0, 0);
		simple_lock(&uobj->vmobjlock);
	}

	/*
	 * we're almost done!  release the pages...
	 * for errors, we free the pages.
	 * otherwise we activate them and mark them as valid and clean.
	 * also, unbusy pages that were not actually requested.
	 */

out:
	if (error) {
		uvm_lock_pageq();
		for (i = 0; i < npages; i++) {
			if (pgs[i] == NULL) {
				continue;
			}
			UVMHIST_LOG(ubchist, "examining pg %p flags 0x%x",
				    pgs[i], pgs[i]->flags, 0,0);
			if ((pgs[i]->flags & PG_FAKE) == 0) {
				continue;
			}
			if (pgs[i]->flags & PG_WANTED) {
				wakeup(pgs[i]);
			}
			uvm_pagefree(pgs[i]);
		}
		uvm_unlock_pageq();
		simple_unlock(&uobj->vmobjlock);
		UVMHIST_LOG(ubchist, "returning error %d", error,0,0,0);
		return error;
	}

	UVMHIST_LOG(ubchist, "succeeding, npages %d", npages,0,0,0);
	for (i = 0; i < npages; i++) {
		if (pgs[i] == NULL) {
			continue;
		}
		UVMHIST_LOG(ubchist, "examining pg %p flags 0x%x",
			    pgs[i], pgs[i]->flags, 0,0);
		if (pgs[i]->flags & PG_FAKE) {
			UVMHIST_LOG(ubchist, "unfaking pg %p offset 0x%x",
				    pgs[i], pgs[i]->offset,0,0);
			pgs[i]->flags &= ~(PG_FAKE);
			pmap_clear_modify(pgs[i]);
			pmap_clear_reference(pgs[i]);
		}
		if (write) {
			pgs[i]->flags &= ~(PG_RDONLY);
		}
		if (i < ridx || i >= ridx + orignpages || async) {
			UVMHIST_LOG(ubchist, "unbusy pg %p offset 0x%x",
				    pgs[i], pgs[i]->offset,0,0);
			if (pgs[i]->flags & PG_WANTED) {
				wakeup(pgs[i]);
			}
			if (pgs[i]->wire_count == 0) {
				uvm_pageactivate(pgs[i]);
			}
			pgs[i]->flags &= ~(PG_WANTED|PG_BUSY);
			UVM_PAGE_OWN(pgs[i], NULL);
		}
	}
	simple_unlock(&uobj->vmobjlock);
	if (ap->a_m != NULL) {
		memcpy(ap->a_m, &pgs[ridx],
		       orignpages * sizeof(struct vm_page *));
	}
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

	int s, error, error2, npages, run;
	int fs_bshift, dev_bshift, dev_bsize;
	vaddr_t kva;
	off_t eof, offset, startoffset;
	size_t bytes, iobytes, skipbytes;
	daddr_t lbn, blkno;
	struct vm_page *pg;
	struct buf *mbp, *bp;
	struct vnode *vp = ap->a_vp;
	boolean_t async = (ap->a_flags & PGO_SYNCIO) == 0;
	UVMHIST_FUNC("genfs_putpages"); UVMHIST_CALLED(ubchist);

	simple_unlock(&vp->v_uvm.u_obj.vmobjlock);

	error = VOP_SIZE(vp, vp->v_uvm.u_size, &eof);
	if (error) {
		return error;
	}

	error = error2 = 0;
	npages = ap->a_count;
	fs_bshift = vp->v_mount->mnt_fs_bshift;
	dev_bshift = vp->v_mount->mnt_dev_bshift;
	dev_bsize = 1 << dev_bshift;
	KASSERT((eof & (dev_bsize - 1)) == 0);

	pg = ap->a_m[0];
	startoffset = pg->offset;
	bytes = min(npages << PAGE_SHIFT, eof - startoffset);
	skipbytes = 0;
	KASSERT(bytes != 0);

	kva = uvm_pagermapin(ap->a_m, npages, UVMPAGER_MAPIN_WAITOK);

	s = splbio();
	vp->v_numoutput += 2;
	mbp = pool_get(&bufpool, PR_WAITOK);
	UVMHIST_LOG(ubchist, "vp %p mbp %p num now %d bytes 0x%x",
		    vp, mbp, vp->v_numoutput, bytes);
	splx(s);
	mbp->b_bufsize = npages << PAGE_SHIFT;
	mbp->b_data = (void *)kva;
	mbp->b_resid = mbp->b_bcount = bytes;
	mbp->b_flags = B_BUSY|B_WRITE|B_AGE |
		(async ? B_CALL : 0) |
		(curproc == uvm.pagedaemon_proc ? B_PDAEMON : 0);
	mbp->b_iodone = uvm_aio_biodone;
	mbp->b_vp = vp;
	LIST_INIT(&mbp->b_dep);

	bp = NULL;
	for (offset = startoffset;
	     bytes > 0;
	     offset += iobytes, bytes -= iobytes) {
		lbn = offset >> fs_bshift;
		error = VOP_BMAP(vp, lbn, NULL, &blkno, &run);
		if (error) {
			UVMHIST_LOG(ubchist, "VOP_BMAP() -> %d", error,0,0,0);
			skipbytes += bytes;
			bytes = 0;
			break;
		}

		iobytes = min(((lbn + 1 + run) << fs_bshift) - offset, bytes);
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
			bp->b_data = (char *)kva +
				(vaddr_t)(offset - pg->offset);
			bp->b_resid = bp->b_bcount = iobytes;
			bp->b_flags = B_BUSY|B_WRITE|B_CALL|B_ASYNC;
			bp->b_iodone = uvm_aio_biodone1;
			bp->b_vp = vp;
			LIST_INIT(&bp->b_dep);
		}
		bp->b_lblkno = 0;
		bp->b_private = mbp;

		/* adjust physical blkno for partial blocks */
		bp->b_blkno = blkno + ((offset - (lbn << fs_bshift)) >>
				       dev_bshift);
		UVMHIST_LOG(ubchist, "vp %p offset 0x%x bcount 0x%x blkno 0x%x",
			    vp, offset, bp->b_bcount, bp->b_blkno);
		VOP_STRATEGY(bp);
	}
	if (skipbytes) {
		UVMHIST_LOG(ubchist, "skipbytes %d", bytes, 0,0,0);
		s = splbio();
		mbp->b_resid -= skipbytes;
		if (mbp->b_resid == 0) {
			biodone(mbp);
		}
		splx(s);
	}
	if (async) {
		UVMHIST_LOG(ubchist, "returning PEND", 0,0,0,0);
		return EINPROGRESS;
	}
	if (bp != NULL) {
		UVMHIST_LOG(ubchist, "waiting for mbp %p", mbp,0,0,0);
		error2 = biowait(mbp);
	}
	if (bioops.io_pageiodone) {
		(*bioops.io_pageiodone)(mbp);
	}
	s = splbio();
	vwakeup(mbp);
	pool_put(&bufpool, mbp);
	splx(s);
	uvm_pagermapout(kva, npages);
	UVMHIST_LOG(ubchist, "returning, error %d", error,0,0,0);
	return error ? error : error2;
}

int
genfs_size(v)
	void *v;
{
	struct vop_size_args /* {
		struct vnode *a_vp;
		off_t a_size;
		off_t *a_eobp;
	} */ *ap = v;
	int bsize;

	bsize = 1 << ap->a_vp->v_mount->mnt_fs_bshift;
	*ap->a_eobp = (ap->a_size + bsize - 1) & ~(bsize - 1);
	return 0;
}
