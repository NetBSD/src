/*	$NetBSD: genfs_vnops.c,v 1.11.4.1 1999/07/04 01:44:43 chs Exp $	*/

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
 * Stubs to use when there is no locking to be done on the underlying object.
 * A minimal shared lock is necessary to ensure that the underlying object
 * is not revoked while an operation is in progress. So, an active shared
 * count is maintained in an auxillary vnode lock structure.
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

#ifdef notyet
	/*
	 * This code cannot be used until all the non-locking filesystems
	 * (notably NFS) are converted to properly lock and release nodes.
	 * Also, certain vnode operations change the locking state within
	 * the operation (create, mknod, remove, link, rename, mkdir, rmdir,
	 * and symlink). Ideally these operations should not change the
	 * lock state, but should be changed to let the caller of the
	 * function unlock them. Otherwise all intermediate vnode layers
	 * (such as union, umapfs, etc) must catch these functions to do
	 * the necessary locking at their layer. Note that the inactive
	 * and lookup operations also change their lock state, but this 
	 * cannot be avoided, so these two operations will always need
	 * to be handled in intermediate layers.
	 */
	struct vnode *vp = ap->a_vp;
	int vnflags, flags = ap->a_flags;

	if (vp->v_vnlock == NULL) {
		if ((flags & LK_TYPE_MASK) == LK_DRAIN)
			return (0);
		MALLOC(vp->v_vnlock, struct lock *, sizeof(struct lock),
		    M_VNODE, M_WAITOK);
		lockinit(vp->v_vnlock, PVFS, "vnlock", 0, 0);
	}
	switch (flags & LK_TYPE_MASK) {
	case LK_DRAIN:
		vnflags = LK_DRAIN;
		break;
	case LK_EXCLUSIVE:
	case LK_SHARED:
		vnflags = LK_SHARED;
		break;
	case LK_UPGRADE:
	case LK_EXCLUPGRADE:
	case LK_DOWNGRADE:
		return (0);
	case LK_RELEASE:
	default:
		panic("vop_nolock: bad operation %d", flags & LK_TYPE_MASK);
	}
	if (flags & LK_INTERLOCK)
		vnflags |= LK_INTERLOCK;
	return(lockmgr(vp->v_vnlock, vnflags, &vp->v_interlock));
#else /* for now */
	/*
	 * Since we are not using the lock manager, we must clear
	 * the interlock here.
	 */
	if (ap->a_flags & LK_INTERLOCK)
		simple_unlock(&ap->a_vp->v_interlock);
	return (0);
#endif
}

/*
 * Decrement the active use count.
 */
int
genfs_nounlock(v)
	void *v;
{
	struct vop_unlock_args /* {
		struct vnode *a_vp;
		int a_flags;
		struct proc *a_p;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	if (vp->v_vnlock == NULL)
		return (0);
	return (lockmgr(vp->v_vnlock, LK_RELEASE, NULL));
}

/*
 * Return whether or not the node is in use.
 */
int
genfs_noislocked(v)
	void *v;
{
	struct vop_islocked_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	if (vp->v_vnlock == NULL)
		return (0);
	return (lockstatus(vp->v_vnlock));
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
		vaddr_t a_offset;
		vm_page_t *a_m;
		int *a_count;
		int a_centeridx;
		vm_prot_t a_access_type;
		int a_advice;
		int a_flags;
	} */ *ap = v;

	off_t offset, origoffset;
	daddr_t lbn, blkno;
	int s, i, error, npages, cidx, bsize, bshift, run;
	int dev_bshift, dev_bsize;
	int flags = ap->a_flags;
	size_t bytes, iobytes, tailbytes;
	char *kva;
	struct buf *bp, *mbp;
	struct vnode *vp = ap->a_vp;
	struct uvm_object *uobj = &vp->v_uvm.u_obj;
	struct vm_page *pg, *pgs[16];  /* XXX 16 */
	struct ucred *cred = curproc->p_ucred;
	UVMHIST_FUNC("genfs_getpages"); UVMHIST_CALLED(ubchist);

#ifdef DIAGNOSTIC
	if (ap->a_centeridx < 0 || ap->a_centeridx > *ap->a_count) {
		panic("genfs_getpages: centeridx %d out of range",
		      ap->a_centeridx);
	}
	if (ap->a_offset & (PAGE_SIZE - 1)) {
		panic("genfs_getpages: offset 0x%x", (int)ap->a_offset);
	}
#endif

	/*
	 * Bounds-check the request.
	 */

	if (ap->a_offset >= vp->v_uvm.u_size) {
		if ((flags & PGO_LOCKED) == 0) {
			simple_unlock(&uobj->vmobjlock);
		}
		return VM_PAGER_BAD;
	}

	/*
	 * For PGO_LOCKED requests, just return whatever's in memory.
	 */

	if (flags & PGO_LOCKED) {
		uvn_findpages(uobj, ap->a_offset, ap->a_count, ap->a_m,
			      UFP_NOWAIT|UFP_NOALLOC|UFP_NORDONLY);

		return ap->a_m[ap->a_centeridx] == NULL ?
			VM_PAGER_UNLOCK : VM_PAGER_OK;
	}

	/* vnode is VOP_LOCKed, uobj is locked */

	error = 0;

	/*
	 * XXX do the findpages for our 1 page first,
	 * change asyncget to take the one page as an arg and
	 * pretend that its findpages found it.
	 */

	/*
	 * kick off a big read first to get some readahead, then
	 * get the one page we wanted.
	 */

	if ((flags & PGO_OVERWRITE) == 0 &&
	    (ap->a_offset & (MAXBSIZE - 1)) == 0) {
		/*
		 * XXX pretty sure unlocking here is wrong.
		 */
		simple_unlock(&uobj->vmobjlock);
		uvm_vnp_asyncget(vp, ap->a_offset, MAXBSIZE);
		simple_lock(&uobj->vmobjlock);
	}

	/*
	 * find the page we want.
	 */

	origoffset = offset = ap->a_offset + (ap->a_centeridx << PAGE_SHIFT);
	npages = 1;
	pg = NULL;
	uvn_findpages(uobj, offset, &npages, &pg, 0);
	simple_unlock(&uobj->vmobjlock);

	/*
	 * if the page is already resident, just return it.
	 */

	if ((pg->flags & PG_FAKE) == 0 &&
	    !((ap->a_access_type & VM_PROT_WRITE) && (pg->flags & PG_RDONLY))) {
		ap->a_m[ap->a_centeridx] = pg;
		return VM_PAGER_OK;
	}
	UVMHIST_LOG(ubchist, "pg %p flags 0x%x access_type 0x%x",
		    pg, (int)pg->flags, (int)ap->a_access_type, 0);

	/*
	 * don't bother reading the page if we're just going to
	 * overwrite it.
	 */

	if (flags & PGO_OVERWRITE) {
		UVMHIST_LOG(ubchist, "PGO_OVERWRITE",0,0,0,0);

		/* XXX for now, zero the page */
		if (pg->flags & PG_FAKE) {
			uvm_pagezero(pg);
		}

		goto out;
	}

	/*
	 * ok, really read the desired page.
	 */

	bshift = vp->v_mount->mnt_fs_bshift;
	bsize = 1 << bshift;
	dev_bshift = vp->v_mount->mnt_dev_bshift;
	dev_bsize = 1 << dev_bshift;
	bytes = min(*ap->a_count << PAGE_SHIFT,
		    (vp->v_uvm.u_size - offset + dev_bsize - 1) &
		    ~(dev_bsize - 1));
	tailbytes = (*ap->a_count << PAGE_SHIFT) - bytes;

	kva = (void *)uvm_pagermapin(&pg, 1, M_WAITOK);

	s = splbio();
	mbp = pool_get(&bufpool, PR_WAITOK);
	splx(s);
	mbp->b_bufsize = bytes;
	mbp->b_data = (void *)kva;
	mbp->b_bcount = bytes;
	mbp->b_flags = B_BUSY|B_READ| (flags & PGO_SYNCIO ? 0 : B_CALL);
	mbp->b_iodone = uvm_aio_biodone;
	mbp->b_vp = vp;

	bp = NULL;
	for (; bytes > 0; offset += iobytes, bytes -= iobytes) {
		lbn = offset >> bshift;

		/*
		 * bmap the file to find out the blkno to read from and
		 * how much we can read in one i/o.
		 */

		error = VOP_BMAP(vp, lbn, NULL, &blkno, &run);
		if (error) {
			UVMHIST_LOG(ubchist, "VOP_BMAP lbn 0x%x -> %d\n",
				    lbn, error,0,0);
			goto errout;
		}
		iobytes = min(((lbn + 1 + run) << bshift) - offset, bytes);

		if (blkno == (daddr_t)-1) {
			UVMHIST_LOG(ubchist, "lbn 0x%x -> HOLE", lbn,0,0,0);

			/*
			 * for read faults, we can skip the block allocation
			 * by marking the page PG_RDONLY and PG_CLEAN.
			 */

			if ((ap->a_access_type & VM_PROT_WRITE) == 0) {
				memset(kva + (offset - origoffset), 0,
				       min(1 << bshift, PAGE_SIZE -
					   (offset - origoffset)));

				pg->flags |= PG_CLEAN|PG_RDONLY;
				UVMHIST_LOG(ubchist, "setting PG_RDONLY",
					    0,0,0,0);
				continue;
			}

			/*
			 * for write faults, we must allocate backing store
			 * now and make sure the block is zeroed.
			 */

			error = VOP_BALLOC(vp, offset, bsize, cred, NULL, 0);
			if (error) {
				UVMHIST_LOG(ubchist, "balloc lbn 0x%x -> %d",
					    lbn, error,0,0);
				goto errout;
			}

			error = VOP_BMAP(vp, lbn, NULL, &blkno, NULL);
			if (error) {
				UVMHIST_LOG(ubchist, "bmap2 lbn 0x%x -> %d",
					    lbn, error,0,0);
				goto errout;
			}

			simple_lock(&uobj->vmobjlock);
			npages = max(bsize >> PAGE_SHIFT, 1);
			if (npages > 1) {
				int idx = (offset - (lbn << bshift)) >> bshift;
				pgs[idx] = PGO_DONTCARE;
				uvn_findpages(uobj, offset &
					      ~((off_t)bsize - 1),
					      &npages, pgs, 0);
				pgs[idx] = pg;
				for (i = 0; i < npages; i++) {
					uvm_pagezero(pgs[i]);
					uvm_pageactivate(pgs[i]);

					/*
					 * don't bother clearing mod/ref,
					 * the block is being modified anyways.
					 */

					pgs[i]->flags &= ~(PG_FAKE|PG_RDONLY);
				}
			} else {
				memset(kva + (offset - origoffset), 0, bsize);
			}

/* XXX is this cidx stuff right? */
			cidx = (offset >> PAGE_SHIFT) -
				(origoffset >> PAGE_SHIFT);
			pg = pgs[cidx];
			pgs[cidx] = NULL;
			uvm_pager_dropcluster(uobj, NULL, pgs, &npages, 0, 0);
			simple_unlock(&uobj->vmobjlock);
			UVMHIST_LOG(ubchist, "cleared pages",0,0,0,0);
			continue;
		}

		/*
		 * allocate a sub-buf for this piece of the i/o
		 * (or just use mbp if there's only 1 piece),
		 * and start it going.
		 */

		if (bp == NULL && iobytes == bytes) {
			bp = mbp;
		} else {
			s = splbio();
			bp = pool_get(&bufpool, PR_WAITOK);
			splx(s);
			bp->b_data = (void *)(kva + offset - pg->offset);
			bp->b_bcount = iobytes;
			bp->b_flags = B_BUSY|B_READ|B_CALL;
			bp->b_iodone = uvm_aio_biodone1;
			bp->b_vp = vp;
		}
		bp->b_lblkno = 0;
		bp->b_private = mbp;

		/* adjust physical blkno for partial blocks */
		bp->b_blkno = blkno + ((offset - (lbn << bshift)) >>
				       dev_bshift);
		UVMHIST_LOG(ubchist, "vp %p bp %p offset 0x%x blkno 0x%x",
			    vp, bp, (int)offset, bp->b_blkno);

		VOP_STRATEGY(bp);
	}

	/*
	 * if EOF is in the middle of this page, zero the part past EOF.
	 */

	if (tailbytes > 0) {
		memset(kva + (offset - origoffset), 0, tailbytes);
	}

errout:
	if ((flags & PGO_SYNCIO) == 0) {
		return VM_PAGER_PEND;
	}
	if (bp != NULL) {
		error = biowait(mbp);
	}
	s = splbio();
	pool_put(&bufpool, mbp);
	splx(s);
	uvm_pagermapout((vaddr_t)kva, *ap->a_count);

out:
	if (error) {
		simple_lock(&uobj->vmobjlock);
		if (pg->flags & PG_WANTED) {
			wakeup(pg);
		}
		UVM_PAGE_OWN(pg, NULL);
		uvm_lock_pageq();
		uvm_pagefree(pg);
		uvm_unlock_pageq();
		simple_unlock(&uobj->vmobjlock);
	} else { 
		pg->flags &= ~(PG_FAKE);
		pmap_clear_modify(PMAP_PGARG(pg));
		pmap_clear_reference(PMAP_PGARG(pg));
		uvm_pageactivate(pg);
		ap->a_m[ap->a_centeridx] = pg;
	}
	UVMHIST_LOG(ubchist, "returning, error %d", error,0,0,0);
	return error ? VM_PAGER_ERROR : VM_PAGER_OK;
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
		int a_sync;
		int *a_rtvals;
	} */ *ap = v;

	int s, error, bshift, dev_bshift, dev_bsize, run;
	vaddr_t kva;
	off_t offset;
	size_t bytes, iobytes;
	daddr_t lbn, blkno;
	struct vm_page *pg;
	struct buf *mbp, *bp;
	struct vnode *vp = ap->a_vp;
	UVMHIST_FUNC("genfs_putpages"); UVMHIST_CALLED(ubchist);

	bshift = vp->v_mount->mnt_fs_bshift;
	dev_bshift = vp->v_mount->mnt_dev_bshift;
	dev_bsize = 1 << dev_bshift;

	pg = ap->a_m[0];
	kva = uvm_pagermapin(ap->a_m, ap->a_count, M_WAITOK);
	if (kva == 0) {
		return VM_PAGER_AGAIN;
	}
	offset = pg->offset;
	bytes = min(ap->a_count << PAGE_SHIFT,
		    (vp->v_uvm.u_size - offset + dev_bsize - 1) &
		    ~(dev_bsize - 1));

	s = splbio();
	vp->v_numoutput++;
	mbp = pool_get(&bufpool, PR_WAITOK);
	splx(s);
	mbp->b_bufsize = ap->a_count << PAGE_SHIFT;
	mbp->b_data = (void *)kva;
	mbp->b_bcount = bytes;
	mbp->b_flags = B_BUSY|B_WRITE| (ap->a_sync ? 0 : B_CALL) |
		(curproc == uvm.pagedaemon_proc ? B_PDAEMON : 0);

	mbp->b_iodone = uvm_aio_biodone;
	mbp->b_vp = vp;

	bp = NULL;
	for (; bytes > 0; offset += iobytes, bytes -= iobytes) {
		lbn = offset >> bshift;
		error = VOP_BMAP(vp, lbn, NULL, &blkno, &run);
		if (error) {
			UVMHIST_LOG(ubchist, "genfs_balloc -> %d", error,0,0,0);
			goto errout;
		}

		/* this could be ifdef DIAGNOSTIC, but it's really important */
		if (blkno == (daddr_t)-1) {
			panic("genfs_putpages: no backing store vp %p lbn 0x%x",
			      vp, lbn);
		}

		/* if it's really one i/o, don't make a second buf */
		iobytes = min(((lbn + 1 + run) << bshift) - offset, bytes);
		if (bp == NULL && iobytes == bytes) {
			bp = mbp;
		} else {
			s = splbio();
			vp->v_numoutput++;
			bp = pool_get(&bufpool, PR_WAITOK);
			splx(s);
			bp->b_data = (char *)kva +
				(vsize_t)(offset - pg->offset);
			bp->b_bcount = iobytes;
			bp->b_flags = B_BUSY|B_WRITE|B_CALL;
			bp->b_iodone = uvm_aio_biodone1;
			bp->b_vp = vp;
		}
		bp->b_lblkno = 0;
		bp->b_private = mbp;

		/* adjust physical blkno for partial blocks */
		bp->b_blkno = blkno + ((offset - (lbn << bshift)) >>
				       dev_bshift);
		UVMHIST_LOG(ubchist, "pg %p vp %p offset 0x%x blkno 0x%x",
			    pg, vp, (int)offset, bp->b_blkno);

		VOP_STRATEGY(bp);
	}

	if (!ap->a_sync) {
		return VM_PAGER_PEND;
	}

errout:
	if (bp != NULL) {
		error = biowait(mbp);
	}
	s = splbio();
	pool_put(&bufpool, mbp);
	splx(s);
	uvm_pagermapout(kva, ap->a_count);
	UVMHIST_LOG(ubchist, "returning, error %d", error,0,0,0);

	return error ? VM_PAGER_ERROR : VM_PAGER_OK;
}
