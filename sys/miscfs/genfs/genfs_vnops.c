/*	$NetBSD: genfs_vnops.c,v 1.150.4.1 2007/07/11 20:10:40 mjf Exp $	*/

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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: genfs_vnops.c,v 1.150.4.1 2007/07/11 20:10:40 mjf Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/kmem.h>
#include <sys/poll.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/kauth.h>
#include <sys/fstrans.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/genfs/genfs_node.h>
#include <miscfs/specfs/specdev.h>

#include <uvm/uvm.h>
#include <uvm/uvm_pager.h>

static int genfs_do_directio(struct vmspace *, vaddr_t, size_t, struct vnode *,
    off_t, enum uio_rw);
static void genfs_dio_iodone(struct buf *);

static int genfs_do_io(struct vnode *, off_t, vaddr_t, size_t, int, enum uio_rw,
    void (*)(struct buf *));
static inline void genfs_rel_pages(struct vm_page **, int);
static void filt_genfsdetach(struct knote *);
static int filt_genfsread(struct knote *, long);
static int filt_genfsvnode(struct knote *, long);

#define MAX_READ_PAGES	16 	/* XXXUBC 16 */

int genfs_maxdio = MAXPHYS;

int
genfs_poll(void *v)
{
	struct vop_poll_args /* {
		struct vnode *a_vp;
		int a_events;
		struct lwp *a_l;
	} */ *ap = v;

	return (ap->a_events & (POLLIN | POLLOUT | POLLRDNORM | POLLWRNORM));
}

int
genfs_seek(void *v)
{
	struct vop_seek_args /* {
		struct vnode *a_vp;
		off_t a_oldoff;
		off_t a_newoff;
		kauth_cred_t cred;
	} */ *ap = v;

	if (ap->a_newoff < 0)
		return (EINVAL);

	return (0);
}

int
genfs_abortop(void *v)
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
genfs_fcntl(void *v)
{
	struct vop_fcntl_args /* {
		struct vnode *a_vp;
		u_int a_command;
		void *a_data;
		int a_fflag;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;

	if (ap->a_command == F_SETFL)
		return (0);
	else
		return (EOPNOTSUPP);
}

/*ARGSUSED*/
int
genfs_badop(void *v)
{

	panic("genfs: bad op");
}

/*ARGSUSED*/
int
genfs_nullop(void *v)
{

	return (0);
}

/*ARGSUSED*/
int
genfs_einval(void *v)
{

	return (EINVAL);
}

/*
 * Called when an fs doesn't support a particular vop.
 * This takes care to vrele, vput, or vunlock passed in vnodes.
 */
int
genfs_eopnotsupp(void *v)
{
	struct vop_generic_args /*
		struct vnodeop_desc *a_desc;
		/ * other random data follows, presumably * /
	} */ *ap = v;
	struct vnodeop_desc *desc = ap->a_desc;
	struct vnode *vp, *vp_last = NULL;
	int flags, i, j, offset;

	flags = desc->vdesc_flags;
	for (i = 0; i < VDESC_MAX_VPS; flags >>=1, i++) {
		if ((offset = desc->vdesc_vp_offsets[i]) == VDESC_NO_OFFSET)
			break;	/* stop at end of list */
		if ((j = flags & VDESC_VP0_WILLPUT)) {
			vp = *VOPARG_OFFSETTO(struct vnode **, offset, ap);

			/* Skip if NULL */
			if (!vp)
				continue;

			switch (j) {
			case VDESC_VP0_WILLPUT:
				/* Check for dvp == vp cases */
				if (vp == vp_last)
					vrele(vp);
				else {
					vput(vp);
					vp_last = vp;
				}
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
genfs_ebadf(void *v)
{

	return (EBADF);
}

/* ARGSUSED */
int
genfs_enoioctl(void *v)
{

	return (EPASSTHROUGH);
}


/*
 * Eliminate all activity associated with the requested vnode
 * and with all vnodes aliased to the requested vnode.
 */
int
genfs_revoke(void *v)
{
	struct vop_revoke_args /* {
		struct vnode *a_vp;
		int a_flags;
	} */ *ap = v;
	struct vnode *vp, *vq;
	struct lwp *l = curlwp;		/* XXX */

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
			ltsleep(vp, PINOD|PNORELOCK, "vop_revokeall", 0,
				&vp->v_interlock);
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
	vgonel(vp, l);
	return (0);
}

/*
 * Lock the node.
 */
int
genfs_lock(void *v)
{
	struct vop_lock_args /* {
		struct vnode *a_vp;
		int a_flags;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	return (lockmgr(vp->v_vnlock, ap->a_flags, &vp->v_interlock));
}

/*
 * Unlock the node.
 */
int
genfs_unlock(void *v)
{
	struct vop_unlock_args /* {
		struct vnode *a_vp;
		int a_flags;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	return (lockmgr(vp->v_vnlock, ap->a_flags | LK_RELEASE,
	    &vp->v_interlock));
}

/*
 * Return whether or not the node is locked.
 */
int
genfs_islocked(void *v)
{
	struct vop_islocked_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	return (lockstatus(vp->v_vnlock));
}

/*
 * Stubs to use when there is no locking to be done on the underlying object.
 */
int
genfs_nolock(void *v)
{
	struct vop_lock_args /* {
		struct vnode *a_vp;
		int a_flags;
		struct lwp *a_l;
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
genfs_nounlock(void *v)
{

	return (0);
}

int
genfs_noislocked(void *v)
{

	return (0);
}

/*
 * Local lease check.
 */
int
genfs_lease_check(void *v)
{

	return (0);
}

int
genfs_mmap(void *v)
{

	return (0);
}

static inline void
genfs_rel_pages(struct vm_page **pgs, int npages)
{
	int i;

	for (i = 0; i < npages; i++) {
		struct vm_page *pg = pgs[i];

		if (pg == NULL || pg == PGO_DONTCARE)
			continue;
		if (pg->flags & PG_FAKE) {
			pg->flags |= PG_RELEASED;
		}
	}
	uvm_lock_pageq();
	uvm_page_unbusy(pgs, npages);
	uvm_unlock_pageq();
}

/*
 * generic VM getpages routine.
 * Return PG_BUSY pages for the given range,
 * reading from backing store if necessary.
 */

int
genfs_getpages(void *v)
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

	off_t newsize, diskeof, memeof;
	off_t offset, origoffset, startoffset, endoffset;
	daddr_t lbn, blkno;
	int i, error, npages, orignpages, npgs, run, ridx, pidx, pcount;
	int fs_bshift, fs_bsize, dev_bshift;
	int flags = ap->a_flags;
	size_t bytes, iobytes, tailstart, tailbytes, totalbytes, skipbytes;
	vaddr_t kva;
	struct buf *bp, *mbp;
	struct vnode *vp = ap->a_vp;
	struct vnode *devvp;
	struct genfs_node *gp = VTOG(vp);
	struct uvm_object *uobj = &vp->v_uobj;
	struct vm_page *pg, **pgs, *pgs_onstack[MAX_READ_PAGES];
	int pgs_size;
	kauth_cred_t cred = curlwp->l_cred;		/* XXXUBC curlwp */
	bool async = (flags & PGO_SYNCIO) == 0;
	bool write = (ap->a_access_type & VM_PROT_WRITE) != 0;
	bool sawhole = false;
	bool has_trans = false;
	bool overwrite = (flags & PGO_OVERWRITE) != 0;
	bool blockalloc = write && (flags & PGO_NOBLOCKALLOC) == 0;
	voff_t origvsize;
	UVMHIST_FUNC("genfs_getpages"); UVMHIST_CALLED(ubchist);

	UVMHIST_LOG(ubchist, "vp %p off 0x%x/%x count %d",
	    vp, ap->a_offset >> 32, ap->a_offset, *ap->a_count);

	KASSERT(vp->v_type == VREG || vp->v_type == VDIR ||
	    vp->v_type == VLNK || vp->v_type == VBLK);

	/* XXXUBC temp limit */
	if (*ap->a_count > MAX_READ_PAGES) {
		panic("genfs_getpages: too many pages");
	}

	pgs = pgs_onstack;
	pgs_size = sizeof(pgs_onstack);

startover:
	error = 0;
	origvsize = vp->v_size;
	origoffset = ap->a_offset;
	orignpages = *ap->a_count;
	GOP_SIZE(vp, origvsize, &diskeof, 0);
	if (flags & PGO_PASTEOF) {
#if defined(DIAGNOSTIC)
		off_t writeeof;
#endif /* defined(DIAGNOSTIC) */

		newsize = MAX(origvsize,
		    origoffset + (orignpages << PAGE_SHIFT));
		GOP_SIZE(vp, newsize, &memeof, GOP_SIZE_MEM);
#if defined(DIAGNOSTIC)
		GOP_SIZE(vp, vp->v_writesize, &writeeof, GOP_SIZE_MEM);
		if (newsize > round_page(writeeof)) {
			panic("%s: past eof", __func__);
		}
#endif /* defined(DIAGNOSTIC) */
	} else {
		GOP_SIZE(vp, origvsize, &memeof, GOP_SIZE_MEM);
	}
	KASSERT(ap->a_centeridx >= 0 || ap->a_centeridx <= orignpages);
	KASSERT((origoffset & (PAGE_SIZE - 1)) == 0 && origoffset >= 0);
	KASSERT(orignpages > 0);

	/*
	 * Bounds-check the request.
	 */

	if (origoffset + (ap->a_centeridx << PAGE_SHIFT) >= memeof) {
		if ((flags & PGO_LOCKED) == 0) {
			simple_unlock(&uobj->vmobjlock);
		}
		UVMHIST_LOG(ubchist, "off 0x%x count %d goes past EOF 0x%x",
		    origoffset, *ap->a_count, memeof,0);
		error = EINVAL;
		goto out_err;
	}

	/* uobj is locked */

	if ((flags & PGO_NOTIMESTAMP) == 0 &&
	    (vp->v_type != VBLK ||
	    (vp->v_mount->mnt_flag & MNT_NODEVMTIME) == 0)) {
		int updflags = 0;

		if ((vp->v_mount->mnt_flag & MNT_NOATIME) == 0) {
			updflags = GOP_UPDATE_ACCESSED;
		}
		if (write) {
			updflags |= GOP_UPDATE_MODIFIED;
		}
		if (updflags != 0) {
			GOP_MARKUPDATE(vp, updflags);
		}
	}

	if (write) {
		gp->g_dirtygen++;
		if ((vp->v_flag & VONWORKLST) == 0) {
			vn_syncer_add_to_worklist(vp, filedelay);
		}
		if ((vp->v_flag & (VWRITEMAP|VWRITEMAPDIRTY)) == VWRITEMAP) {
			vp->v_flag |= VWRITEMAPDIRTY;
		}
	}

	/*
	 * For PGO_LOCKED requests, just return whatever's in memory.
	 */

	if (flags & PGO_LOCKED) {
		int nfound;

		npages = *ap->a_count;
#if defined(DEBUG)
		for (i = 0; i < npages; i++) {
			pg = ap->a_m[i];
			KASSERT(pg == NULL || pg == PGO_DONTCARE);
		}
#endif /* defined(DEBUG) */
		nfound = uvn_findpages(uobj, origoffset, &npages,
		    ap->a_m, UFP_NOWAIT|UFP_NOALLOC|(write ? UFP_NORDONLY : 0));
		KASSERT(npages == *ap->a_count);
		if (nfound == 0) {
			error = EBUSY;
			goto out_err;
		}
		if (!rw_tryenter(&gp->g_glock, RW_READER)) {
			genfs_rel_pages(ap->a_m, npages);

			/*
			 * restore the array.
			 */

			for (i = 0; i < npages; i++) {
				pg = ap->a_m[i];

				if (pg != NULL || pg != PGO_DONTCARE) {
					ap->a_m[i] = NULL;
				}
			}
		} else {
			rw_exit(&gp->g_glock);
		}
		error = (ap->a_m[ap->a_centeridx] == NULL ? EBUSY : 0);
		goto out_err;
	}
	simple_unlock(&uobj->vmobjlock);

	/*
	 * find the requested pages and make some simple checks.
	 * leave space in the page array for a whole block.
	 */

	if (vp->v_type != VBLK) {
		fs_bshift = vp->v_mount->mnt_fs_bshift;
		dev_bshift = vp->v_mount->mnt_dev_bshift;
	} else {
		fs_bshift = DEV_BSHIFT;
		dev_bshift = DEV_BSHIFT;
	}
	fs_bsize = 1 << fs_bshift;

	orignpages = MIN(orignpages,
	    round_page(memeof - origoffset) >> PAGE_SHIFT);
	npages = orignpages;
	startoffset = origoffset & ~(fs_bsize - 1);
	endoffset = round_page((origoffset + (npages << PAGE_SHIFT) +
	    fs_bsize - 1) & ~(fs_bsize - 1));
	endoffset = MIN(endoffset, round_page(memeof));
	ridx = (origoffset - startoffset) >> PAGE_SHIFT;

	pgs_size = sizeof(struct vm_page *) *
	    ((endoffset - startoffset) >> PAGE_SHIFT);
	if (pgs_size > sizeof(pgs_onstack)) {
		pgs = kmem_zalloc(pgs_size, async ? KM_NOSLEEP : KM_SLEEP);
		if (pgs == NULL) {
			pgs = pgs_onstack;
			error = ENOMEM;
			goto out_err;
		}
	} else {
		/* pgs == pgs_onstack */
		memset(pgs, 0, pgs_size);
	}
	UVMHIST_LOG(ubchist, "ridx %d npages %d startoff %ld endoff %ld",
	    ridx, npages, startoffset, endoffset);

	if (!has_trans) {
		fstrans_start(vp->v_mount, FSTRANS_SHARED);
		has_trans = true;
	}

	/*
	 * hold g_glock to prevent a race with truncate.
	 *
	 * check if our idea of v_size is still valid.
	 */

	if (blockalloc) {
		rw_enter(&gp->g_glock, RW_WRITER);
	} else {
		rw_enter(&gp->g_glock, RW_READER);
	}
	simple_lock(&uobj->vmobjlock);
	if (vp->v_size < origvsize) {
		rw_exit(&gp->g_glock);
		if (pgs != pgs_onstack)
			kmem_free(pgs, pgs_size);
		goto startover;
	}

	if (uvn_findpages(uobj, origoffset, &npages, &pgs[ridx],
	    async ? UFP_NOWAIT : UFP_ALL) != orignpages) {
		rw_exit(&gp->g_glock);
		KASSERT(async != 0);
		genfs_rel_pages(&pgs[ridx], orignpages);
		simple_unlock(&uobj->vmobjlock);
		error = EBUSY;
		goto out_err;
	}

	/*
	 * if the pages are already resident, just return them.
	 */

	for (i = 0; i < npages; i++) {
		struct vm_page *pg1 = pgs[ridx + i];

		if ((pg1->flags & PG_FAKE) ||
		    (blockalloc && (pg1->flags & PG_RDONLY))) {
			break;
		}
	}
	if (i == npages) {
		rw_exit(&gp->g_glock);
		UVMHIST_LOG(ubchist, "returning cached pages", 0,0,0,0);
		npages += ridx;
		goto out;
	}

	/*
	 * if PGO_OVERWRITE is set, don't bother reading the pages.
	 */

	if (overwrite) {
		rw_exit(&gp->g_glock);
		UVMHIST_LOG(ubchist, "PGO_OVERWRITE",0,0,0,0);

		for (i = 0; i < npages; i++) {
			struct vm_page *pg1 = pgs[ridx + i];

			pg1->flags &= ~(PG_RDONLY|PG_CLEAN);
		}
		npages += ridx;
		goto out;
	}

	/*
	 * the page wasn't resident and we're not overwriting,
	 * so we're going to have to do some i/o.
	 * find any additional pages needed to cover the expanded range.
	 */

	npages = (endoffset - startoffset) >> PAGE_SHIFT;
	if (startoffset != origoffset || npages != orignpages) {

		/*
		 * we need to avoid deadlocks caused by locking
		 * additional pages at lower offsets than pages we
		 * already have locked.  unlock them all and start over.
		 */

		genfs_rel_pages(&pgs[ridx], orignpages);
		memset(pgs, 0, pgs_size);

		UVMHIST_LOG(ubchist, "reset npages start 0x%x end 0x%x",
		    startoffset, endoffset, 0,0);
		npgs = npages;
		if (uvn_findpages(uobj, startoffset, &npgs, pgs,
		    async ? UFP_NOWAIT : UFP_ALL) != npages) {
			rw_exit(&gp->g_glock);
			KASSERT(async != 0);
			genfs_rel_pages(pgs, npages);
			simple_unlock(&uobj->vmobjlock);
			error = EBUSY;
			goto out_err;
		}
	}
	simple_unlock(&uobj->vmobjlock);

	/*
	 * read the desired page(s).
	 */

	totalbytes = npages << PAGE_SHIFT;
	bytes = MIN(totalbytes, MAX(diskeof - startoffset, 0));
	tailbytes = totalbytes - bytes;
	skipbytes = 0;

	kva = uvm_pagermapin(pgs, npages,
	    UVMPAGER_MAPIN_READ | UVMPAGER_MAPIN_WAITOK);

	mbp = getiobuf();
	mbp->b_bufsize = totalbytes;
	mbp->b_data = (void *)kva;
	mbp->b_resid = mbp->b_bcount = bytes;
	mbp->b_flags = B_BUSY|B_READ| (async ? B_CALL|B_ASYNC : 0);
	mbp->b_iodone = (async ? uvm_aio_biodone : 0);
	mbp->b_vp = vp;
	if (async)
		BIO_SETPRIO(mbp, BPRIO_TIMELIMITED);
	else
		BIO_SETPRIO(mbp, BPRIO_TIMECRITICAL);

	/*
	 * if EOF is in the middle of the range, zero the part past EOF.
	 * skip over pages which are not PG_FAKE since in that case they have
	 * valid data that we need to preserve.
	 */

	tailstart = bytes;
	while (tailbytes > 0) {
		const int len = PAGE_SIZE - (tailstart & PAGE_MASK);

		KASSERT(len <= tailbytes);
		if ((pgs[tailstart >> PAGE_SHIFT]->flags & PG_FAKE) != 0) {
			memset((void *)(kva + tailstart), 0, len);
			UVMHIST_LOG(ubchist, "tailbytes %p 0x%x 0x%x",
			    kva, tailstart, len, 0);
		}
		tailstart += len;
		tailbytes -= len;
	}

	/*
	 * now loop over the pages, reading as needed.
	 */

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
			if ((pgs[pidx]->flags & PG_RDONLY)) {
				sawhole = true;
			}
			b = MIN(PAGE_SIZE, bytes);
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
		error = VOP_BMAP(vp, lbn, &devvp, &blkno, &run);
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

		iobytes = MIN((((off_t)lbn + 1 + run) << fs_bshift) - offset,
		    bytes);
		if (offset + iobytes > round_page(offset)) {
			pcount = 1;
			while (pidx + pcount < npages &&
			    pgs[pidx + pcount]->flags & PG_FAKE) {
				pcount++;
			}
			iobytes = MIN(iobytes, (pcount << PAGE_SHIFT) -
			    (offset - trunc_page(offset)));
		}

		/*
		 * if this block isn't allocated, zero it instead of
		 * reading it.  unless we are going to allocate blocks,
		 * mark the pages we zeroed PG_RDONLY.
		 */

		if (blkno < 0) {
			int holepages = (round_page(offset + iobytes) -
			    trunc_page(offset)) >> PAGE_SHIFT;
			UVMHIST_LOG(ubchist, "lbn 0x%x -> HOLE", lbn,0,0,0);

			sawhole = true;
			memset((char *)kva + (offset - startoffset), 0,
			    iobytes);
			skipbytes += iobytes;

			for (i = 0; i < holepages; i++) {
				if (write) {
					pgs[pidx + i]->flags &= ~PG_CLEAN;
				}
				if (!blockalloc) {
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
			bp = getiobuf();
			nestiobuf_setup(mbp, bp, offset - startoffset, iobytes);
		}
		bp->b_lblkno = 0;

		/* adjust physical blkno for partial blocks */
		bp->b_blkno = blkno + ((offset - ((off_t)lbn << fs_bshift)) >>
		    dev_bshift);

		UVMHIST_LOG(ubchist,
		    "bp %p offset 0x%x bcount 0x%x blkno 0x%x",
		    bp, offset, iobytes, bp->b_blkno);

		VOP_STRATEGY(devvp, bp);
	}

loopdone:
	nestiobuf_done(mbp, skipbytes, error);
	if (async) {
		UVMHIST_LOG(ubchist, "returning 0 (async)",0,0,0,0);
		rw_exit(&gp->g_glock);
		error = 0;
		goto out_err;
	}
	if (bp != NULL) {
		error = biowait(mbp);
	}
	putiobuf(mbp);
	uvm_pagermapout(kva, npages);

	/*
	 * if this we encountered a hole then we have to do a little more work.
	 * for read faults, we marked the page PG_RDONLY so that future
	 * write accesses to the page will fault again.
	 * for write faults, we must make sure that the backing store for
	 * the page is completely allocated while the pages are locked.
	 */

	if (!error && sawhole && blockalloc) {
		error = GOP_ALLOC(vp, startoffset, npages << PAGE_SHIFT, 0,
		    cred);
		UVMHIST_LOG(ubchist, "gop_alloc off 0x%x/0x%x -> %d",
		    startoffset, npages << PAGE_SHIFT, error,0);
		if (!error) {
			for (i = 0; i < npages; i++) {
				if (pgs[i] == NULL) {
					continue;
				}
				pgs[i]->flags &= ~(PG_CLEAN|PG_RDONLY);
				UVMHIST_LOG(ubchist, "mark dirty pg %p",
				    pgs[i],0,0,0);
			}
		}
	}
	rw_exit(&gp->g_glock);
	simple_lock(&uobj->vmobjlock);

	/*
	 * we're almost done!  release the pages...
	 * for errors, we free the pages.
	 * otherwise we activate them and mark them as valid and clean.
	 * also, unbusy pages that were not actually requested.
	 */

	if (error) {
		for (i = 0; i < npages; i++) {
			if (pgs[i] == NULL) {
				continue;
			}
			UVMHIST_LOG(ubchist, "examining pg %p flags 0x%x",
			    pgs[i], pgs[i]->flags, 0,0);
			if (pgs[i]->flags & PG_FAKE) {
				pgs[i]->flags |= PG_RELEASED;
			}
		}
		uvm_lock_pageq();
		uvm_page_unbusy(pgs, npages);
		uvm_unlock_pageq();
		simple_unlock(&uobj->vmobjlock);
		UVMHIST_LOG(ubchist, "returning error %d", error,0,0,0);
		goto out_err;
	}

out:
	UVMHIST_LOG(ubchist, "succeeding, npages %d", npages,0,0,0);
	error = 0;
	uvm_lock_pageq();
	for (i = 0; i < npages; i++) {
		pg = pgs[i];
		if (pg == NULL) {
			continue;
		}
		UVMHIST_LOG(ubchist, "examining pg %p flags 0x%x",
		    pg, pg->flags, 0,0);
		if (pg->flags & PG_FAKE && !overwrite) {
			pg->flags &= ~(PG_FAKE);
			pmap_clear_modify(pgs[i]);
		}
		KASSERT(!write || !blockalloc || (pg->flags & PG_RDONLY) == 0);
		if (i < ridx || i >= ridx + orignpages || async) {
			UVMHIST_LOG(ubchist, "unbusy pg %p offset 0x%x",
			    pg, pg->offset,0,0);
			if (pg->flags & PG_WANTED) {
				wakeup(pg);
			}
			if (pg->flags & PG_FAKE) {
				KASSERT(overwrite);
				uvm_pagezero(pg);
			}
			if (pg->flags & PG_RELEASED) {
				uvm_pagefree(pg);
				continue;
			}
			uvm_pageenqueue(pg);
			pg->flags &= ~(PG_WANTED|PG_BUSY|PG_FAKE);
			UVM_PAGE_OWN(pg, NULL);
		}
	}
	uvm_unlock_pageq();
	simple_unlock(&uobj->vmobjlock);
	if (ap->a_m != NULL) {
		memcpy(ap->a_m, &pgs[ridx],
		    orignpages * sizeof(struct vm_page *));
	}

out_err:
	if (pgs != pgs_onstack)
		kmem_free(pgs, pgs_size);
	if (has_trans)
		fstrans_done(vp->v_mount);
	return (error);
}

/*
 * generic VM putpages routine.
 * Write the given range of pages to backing store.
 *
 * => "offhi == 0" means flush all pages at or after "offlo".
 * => object should be locked by caller.  we return with the
 *      object unlocked.
 * => if PGO_CLEANIT or PGO_SYNCIO is set, we may block (due to I/O).
 *	thus, a caller might want to unlock higher level resources
 *	(e.g. vm_map) before calling flush.
 * => if neither PGO_CLEANIT nor PGO_SYNCIO is set, we will not block
 * => if PGO_ALLPAGES is set, then all pages in the object will be processed.
 * => NOTE: we rely on the fact that the object's memq is a TAILQ and
 *	that new pages are inserted on the tail end of the list.   thus,
 *	we can make a complete pass through the object in one go by starting
 *	at the head and working towards the tail (new pages are put in
 *	front of us).
 * => NOTE: we are allowed to lock the page queues, so the caller
 *	must not be holding the page queue lock.
 *
 * note on "cleaning" object and PG_BUSY pages:
 *	this routine is holding the lock on the object.   the only time
 *	that it can run into a PG_BUSY page that it does not own is if
 *	some other process has started I/O on the page (e.g. either
 *	a pagein, or a pageout).    if the PG_BUSY page is being paged
 *	in, then it can not be dirty (!PG_CLEAN) because no one has
 *	had a chance to modify it yet.    if the PG_BUSY page is being
 *	paged out then it means that someone else has already started
 *	cleaning the page for us (how nice!).    in this case, if we
 *	have syncio specified, then after we make our pass through the
 *	object we need to wait for the other PG_BUSY pages to clear
 *	off (i.e. we need to do an iosync).   also note that once a
 *	page is PG_BUSY it must stay in its object until it is un-busyed.
 *
 * note on page traversal:
 *	we can traverse the pages in an object either by going down the
 *	linked list in "uobj->memq", or we can go over the address range
 *	by page doing hash table lookups for each address.    depending
 *	on how many pages are in the object it may be cheaper to do one
 *	or the other.   we set "by_list" to true if we are using memq.
 *	if the cost of a hash lookup was equal to the cost of the list
 *	traversal we could compare the number of pages in the start->stop
 *	range to the total number of pages in the object.   however, it
 *	seems that a hash table lookup is more expensive than the linked
 *	list traversal, so we multiply the number of pages in the
 *	range by an estimate of the relatively higher cost of the hash lookup.
 */

int
genfs_putpages(void *v)
{
	struct vop_putpages_args /* {
		struct vnode *a_vp;
		voff_t a_offlo;
		voff_t a_offhi;
		int a_flags;
	} */ *ap = v;

	return genfs_do_putpages(ap->a_vp, ap->a_offlo, ap->a_offhi,
	    ap->a_flags, NULL);
}

int
genfs_do_putpages(struct vnode *vp, off_t startoff, off_t endoff, int flags,
	struct vm_page **busypg)
{
	struct uvm_object *uobj = &vp->v_uobj;
	struct simplelock *slock = &uobj->vmobjlock;
	off_t off;
	/* Even for strange MAXPHYS, the shift rounds down to a page */
#define maxpages (MAXPHYS >> PAGE_SHIFT)
	int i, s, error, npages, nback;
	int freeflag;
	struct vm_page *pgs[maxpages], *pg, *nextpg, *tpg, curmp, endmp;
	bool wasclean, by_list, needs_clean, yld;
	bool async = (flags & PGO_SYNCIO) == 0;
	bool pagedaemon = curlwp == uvm.pagedaemon_lwp;
	struct lwp *l = curlwp ? curlwp : &lwp0;
	struct genfs_node *gp = VTOG(vp);
	int dirtygen;
	bool modified = false;
	bool has_trans = false;
	bool cleanall;

	UVMHIST_FUNC("genfs_putpages"); UVMHIST_CALLED(ubchist);

	KASSERT(flags & (PGO_CLEANIT|PGO_FREE|PGO_DEACTIVATE));
	KASSERT((startoff & PAGE_MASK) == 0 && (endoff & PAGE_MASK) == 0);
	KASSERT(startoff < endoff || endoff == 0);

	UVMHIST_LOG(ubchist, "vp %p pages %d off 0x%x len 0x%x",
	    vp, uobj->uo_npages, startoff, endoff - startoff);

	KASSERT((vp->v_flag & VONWORKLST) != 0 ||
	    (vp->v_flag & VWRITEMAPDIRTY) == 0);
	if (uobj->uo_npages == 0) {
		s = splbio();
		if (vp->v_flag & VONWORKLST) {
			vp->v_flag &= ~VWRITEMAPDIRTY;
			if (LIST_FIRST(&vp->v_dirtyblkhd) == NULL)
				vn_syncer_remove_from_worklist(vp);
		}
		splx(s);
		simple_unlock(slock);
		return (0);
	}

	/*
	 * the vnode has pages, set up to process the request.
	 */

	if ((flags & PGO_CLEANIT) != 0) {
		simple_unlock(slock);
		if (pagedaemon) {
			error = fstrans_start_nowait(vp->v_mount, FSTRANS_LAZY);
			if (error)
				return error;
		} else
			fstrans_start(vp->v_mount, FSTRANS_LAZY);
		has_trans = true;
		simple_lock(slock);
	}

	error = 0;
	s = splbio();
	simple_lock(&global_v_numoutput_slock);
	wasclean = (vp->v_numoutput == 0);
	simple_unlock(&global_v_numoutput_slock);
	splx(s);
	off = startoff;
	if (endoff == 0 || flags & PGO_ALLPAGES) {
		endoff = trunc_page(LLONG_MAX);
	}
	by_list = (uobj->uo_npages <=
	    ((endoff - startoff) >> PAGE_SHIFT) * UVM_PAGE_HASH_PENALTY);

#if !defined(DEBUG)
	/*
	 * if this vnode is known not to have dirty pages,
	 * don't bother to clean it out.
	 */

	if ((vp->v_flag & VONWORKLST) == 0) {
		if ((flags & (PGO_FREE|PGO_DEACTIVATE)) == 0) {
			goto skip_scan;
		}
		flags &= ~PGO_CLEANIT;
	}
#endif /* !defined(DEBUG) */

	/*
	 * start the loop.  when scanning by list, hold the last page
	 * in the list before we start.  pages allocated after we start
	 * will be added to the end of the list, so we can stop at the
	 * current last page.
	 */

	cleanall = (flags & PGO_CLEANIT) != 0 && wasclean &&
	    startoff == 0 && endoff == trunc_page(LLONG_MAX) &&
	    (vp->v_flag & VONWORKLST) != 0;
	dirtygen = gp->g_dirtygen;
	freeflag = pagedaemon ? PG_PAGEOUT : PG_RELEASED;
	if (by_list) {
		curmp.uobject = uobj;
		curmp.offset = (voff_t)-1;
		curmp.flags = PG_BUSY;
		endmp.uobject = uobj;
		endmp.offset = (voff_t)-1;
		endmp.flags = PG_BUSY;
		pg = TAILQ_FIRST(&uobj->memq);
		TAILQ_INSERT_TAIL(&uobj->memq, &endmp, listq);
		uvm_lwp_hold(l);
	} else {
		pg = uvm_pagelookup(uobj, off);
	}
	nextpg = NULL;
	while (by_list || off < endoff) {

		/*
		 * if the current page is not interesting, move on to the next.
		 */

		KASSERT(pg == NULL || pg->uobject == uobj);
		KASSERT(pg == NULL ||
		    (pg->flags & (PG_RELEASED|PG_PAGEOUT)) == 0 ||
		    (pg->flags & PG_BUSY) != 0);
		if (by_list) {
			if (pg == &endmp) {
				break;
			}
			if (pg->offset < startoff || pg->offset >= endoff ||
			    pg->flags & (PG_RELEASED|PG_PAGEOUT)) {
				if (pg->flags & (PG_RELEASED|PG_PAGEOUT)) {
					wasclean = false;
				}
				pg = TAILQ_NEXT(pg, listq);
				continue;
			}
			off = pg->offset;
		} else if (pg == NULL || pg->flags & (PG_RELEASED|PG_PAGEOUT)) {
			if (pg != NULL) {
				wasclean = false;
			}
			off += PAGE_SIZE;
			if (off < endoff) {
				pg = uvm_pagelookup(uobj, off);
			}
			continue;
		}

		/*
		 * if the current page needs to be cleaned and it's busy,
		 * wait for it to become unbusy.
		 */

		yld = (l->l_cpu->ci_schedstate.spc_flags &
		    SPCF_SHOULDYIELD) && !pagedaemon;
		if (pg->flags & PG_BUSY || yld) {
			UVMHIST_LOG(ubchist, "busy %p", pg,0,0,0);
			if (flags & PGO_BUSYFAIL && pg->flags & PG_BUSY) {
				UVMHIST_LOG(ubchist, "busyfail %p", pg, 0,0,0);
				error = EDEADLK;
				if (busypg != NULL)
					*busypg = pg;
				break;
			}
			KASSERT(!pagedaemon);
			if (by_list) {
				TAILQ_INSERT_BEFORE(pg, &curmp, listq);
				UVMHIST_LOG(ubchist, "curmp next %p",
				    TAILQ_NEXT(&curmp, listq), 0,0,0);
			}
			if (yld) {
				simple_unlock(slock);
				preempt();
				simple_lock(slock);
			} else {
				pg->flags |= PG_WANTED;
				UVM_UNLOCK_AND_WAIT(pg, slock, 0, "genput", 0);
				simple_lock(slock);
			}
			if (by_list) {
				UVMHIST_LOG(ubchist, "after next %p",
				    TAILQ_NEXT(&curmp, listq), 0,0,0);
				pg = TAILQ_NEXT(&curmp, listq);
				TAILQ_REMOVE(&uobj->memq, &curmp, listq);
			} else {
				pg = uvm_pagelookup(uobj, off);
			}
			continue;
		}

		/*
		 * if we're freeing, remove all mappings of the page now.
		 * if we're cleaning, check if the page is needs to be cleaned.
		 */

		if (flags & PGO_FREE) {
			pmap_page_protect(pg, VM_PROT_NONE);
		} else if (flags & PGO_CLEANIT) {

			/*
			 * if we still have some hope to pull this vnode off
			 * from the syncer queue, write-protect the page.
			 */

			if (cleanall && wasclean &&
			    gp->g_dirtygen == dirtygen) {

				/*
				 * uobj pages get wired only by uvm_fault
				 * where uobj is locked.
				 */

				if (pg->wire_count == 0) {
					pmap_page_protect(pg,
					    VM_PROT_READ|VM_PROT_EXECUTE);
				} else {
					cleanall = false;
				}
			}
		}

		if (flags & PGO_CLEANIT) {
			needs_clean = pmap_clear_modify(pg) ||
			    (pg->flags & PG_CLEAN) == 0;
			pg->flags |= PG_CLEAN;
		} else {
			needs_clean = false;
		}

		/*
		 * if we're cleaning, build a cluster.
		 * the cluster will consist of pages which are currently dirty,
		 * but they will be returned to us marked clean.
		 * if not cleaning, just operate on the one page.
		 */

		if (needs_clean) {
			KDASSERT((vp->v_flag & VONWORKLST));
			wasclean = false;
			memset(pgs, 0, sizeof(pgs));
			pg->flags |= PG_BUSY;
			UVM_PAGE_OWN(pg, "genfs_putpages");

			/*
			 * first look backward.
			 */

			npages = MIN(maxpages >> 1, off >> PAGE_SHIFT);
			nback = npages;
			uvn_findpages(uobj, off - PAGE_SIZE, &nback, &pgs[0],
			    UFP_NOWAIT|UFP_NOALLOC|UFP_DIRTYONLY|UFP_BACKWARD);
			if (nback) {
				memmove(&pgs[0], &pgs[npages - nback],
				    nback * sizeof(pgs[0]));
				if (npages - nback < nback)
					memset(&pgs[nback], 0,
					    (npages - nback) * sizeof(pgs[0]));
				else
					memset(&pgs[npages - nback], 0,
					    nback * sizeof(pgs[0]));
			}

			/*
			 * then plug in our page of interest.
			 */

			pgs[nback] = pg;

			/*
			 * then look forward to fill in the remaining space in
			 * the array of pages.
			 */

			npages = maxpages - nback - 1;
			uvn_findpages(uobj, off + PAGE_SIZE, &npages,
			    &pgs[nback + 1],
			    UFP_NOWAIT|UFP_NOALLOC|UFP_DIRTYONLY);
			npages += nback + 1;
		} else {
			pgs[0] = pg;
			npages = 1;
			nback = 0;
		}

		/*
		 * apply FREE or DEACTIVATE options if requested.
		 */

		if (flags & (PGO_DEACTIVATE|PGO_FREE)) {
			uvm_lock_pageq();
		}
		for (i = 0; i < npages; i++) {
			tpg = pgs[i];
			KASSERT(tpg->uobject == uobj);
			if (by_list && tpg == TAILQ_NEXT(pg, listq))
				pg = tpg;
			if (tpg->offset < startoff || tpg->offset >= endoff)
				continue;
			if (flags & PGO_DEACTIVATE && tpg->wire_count == 0) {
				(void) pmap_clear_reference(tpg);
				uvm_pagedeactivate(tpg);
			} else if (flags & PGO_FREE) {
				pmap_page_protect(tpg, VM_PROT_NONE);
				if (tpg->flags & PG_BUSY) {
					tpg->flags |= freeflag;
					if (pagedaemon) {
						uvmexp.paging++;
						uvm_pagedequeue(tpg);
					}
				} else {

					/*
					 * ``page is not busy''
					 * implies that npages is 1
					 * and needs_clean is false.
					 */

					nextpg = TAILQ_NEXT(tpg, listq);
					uvm_pagefree(tpg);
					if (pagedaemon)
						uvmexp.pdfreed++;
				}
			}
		}
		if (flags & (PGO_DEACTIVATE|PGO_FREE)) {
			uvm_unlock_pageq();
		}
		if (needs_clean) {
			modified = true;

			/*
			 * start the i/o.  if we're traversing by list,
			 * keep our place in the list with a marker page.
			 */

			if (by_list) {
				TAILQ_INSERT_AFTER(&uobj->memq, pg, &curmp,
				    listq);
			}
			simple_unlock(slock);
			error = GOP_WRITE(vp, pgs, npages, flags);
			simple_lock(slock);
			if (by_list) {
				pg = TAILQ_NEXT(&curmp, listq);
				TAILQ_REMOVE(&uobj->memq, &curmp, listq);
			}
			if (error) {
				break;
			}
			if (by_list) {
				continue;
			}
		}

		/*
		 * find the next page and continue if there was no error.
		 */

		if (by_list) {
			if (nextpg) {
				pg = nextpg;
				nextpg = NULL;
			} else {
				pg = TAILQ_NEXT(pg, listq);
			}
		} else {
			off += (npages - nback) << PAGE_SHIFT;
			if (off < endoff) {
				pg = uvm_pagelookup(uobj, off);
			}
		}
	}
	if (by_list) {
		TAILQ_REMOVE(&uobj->memq, &endmp, listq);
		uvm_lwp_rele(l);
	}

	if (modified && (vp->v_flag & VWRITEMAPDIRTY) != 0 &&
	    (vp->v_type != VBLK ||
	    (vp->v_mount->mnt_flag & MNT_NODEVMTIME) == 0)) {
		GOP_MARKUPDATE(vp, GOP_UPDATE_MODIFIED);
	}

	/*
	 * if we're cleaning and there was nothing to clean,
	 * take us off the syncer list.  if we started any i/o
	 * and we're doing sync i/o, wait for all writes to finish.
	 */

	s = splbio();
	if (cleanall && wasclean && gp->g_dirtygen == dirtygen &&
	    (vp->v_flag & VONWORKLST) != 0) {
		vp->v_flag &= ~VWRITEMAPDIRTY;
		if (LIST_FIRST(&vp->v_dirtyblkhd) == NULL)
			vn_syncer_remove_from_worklist(vp);
	}
	splx(s);

#if !defined(DEBUG)
skip_scan:
#endif /* !defined(DEBUG) */
	if (!wasclean && !async) {
		s = splbio();
		/*
		 * XXX - we want simple_unlock(&global_v_numoutput_slock);
		 *	 but the slot in ltsleep() is taken!
		 * XXX - try to recover from missed wakeups with a timeout..
		 *	 must think of something better.
		 */
		while (vp->v_numoutput != 0) {
			vp->v_flag |= VBWAIT;
			UVM_UNLOCK_AND_WAIT(&vp->v_numoutput, slock, false,
			    "genput2", hz);
			simple_lock(slock);
		}
		splx(s);
	}
	simple_unlock(slock);

	if (has_trans)
		fstrans_done(vp->v_mount);

	return (error);
}

int
genfs_gop_write(struct vnode *vp, struct vm_page **pgs, int npages, int flags)
{
	off_t off;
	vaddr_t kva;
	size_t len;
	int error;
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(ubchist);

	UVMHIST_LOG(ubchist, "vp %p pgs %p npages %d flags 0x%x",
	    vp, pgs, npages, flags);

	off = pgs[0]->offset;
	kva = uvm_pagermapin(pgs, npages,
	    UVMPAGER_MAPIN_WRITE | UVMPAGER_MAPIN_WAITOK);
	len = npages << PAGE_SHIFT;

	error = genfs_do_io(vp, off, kva, len, flags, UIO_WRITE,
			    uvm_aio_biodone);

	return error;
}

/*
 * Backend routine for doing I/O to vnode pages.  Pages are already locked
 * and mapped into kernel memory.  Here we just look up the underlying
 * device block addresses and call the strategy routine.
 */

static int
genfs_do_io(struct vnode *vp, off_t off, vaddr_t kva, size_t len, int flags,
    enum uio_rw rw, void (*iodone)(struct buf *))
{
	int s, error, run;
	int fs_bshift, dev_bshift;
	off_t eof, offset, startoffset;
	size_t bytes, iobytes, skipbytes;
	daddr_t lbn, blkno;
	struct buf *mbp, *bp;
	struct vnode *devvp;
	bool async = (flags & PGO_SYNCIO) == 0;
	bool write = rw == UIO_WRITE;
	int brw = write ? B_WRITE : B_READ;
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(ubchist);

	UVMHIST_LOG(ubchist, "vp %p kva %p len 0x%x flags 0x%x",
	    vp, kva, len, flags);

	KASSERT(vp->v_size <= vp->v_writesize);
	GOP_SIZE(vp, vp->v_writesize, &eof, 0);
	if (vp->v_type != VBLK) {
		fs_bshift = vp->v_mount->mnt_fs_bshift;
		dev_bshift = vp->v_mount->mnt_dev_bshift;
	} else {
		fs_bshift = DEV_BSHIFT;
		dev_bshift = DEV_BSHIFT;
	}
	error = 0;
	startoffset = off;
	bytes = MIN(len, eof - startoffset);
	skipbytes = 0;
	KASSERT(bytes != 0);

	if (write) {
		s = splbio();
		simple_lock(&global_v_numoutput_slock);
		vp->v_numoutput += 2;
		simple_unlock(&global_v_numoutput_slock);
		splx(s);
	}
	mbp = getiobuf();
	UVMHIST_LOG(ubchist, "vp %p mbp %p num now %d bytes 0x%x",
	    vp, mbp, vp->v_numoutput, bytes);
	mbp->b_bufsize = len;
	mbp->b_data = (void *)kva;
	mbp->b_resid = mbp->b_bcount = bytes;
	mbp->b_flags = B_BUSY | brw | B_AGE | (async ? (B_CALL | B_ASYNC) : 0);
	mbp->b_iodone = iodone;
	mbp->b_vp = vp;
	if (curlwp == uvm.pagedaemon_lwp)
		BIO_SETPRIO(mbp, BPRIO_TIMELIMITED);
	else if (async)
		BIO_SETPRIO(mbp, BPRIO_TIMENONCRITICAL);
	else
		BIO_SETPRIO(mbp, BPRIO_TIMECRITICAL);

	bp = NULL;
	for (offset = startoffset;
	    bytes > 0;
	    offset += iobytes, bytes -= iobytes) {
		lbn = offset >> fs_bshift;
		error = VOP_BMAP(vp, lbn, &devvp, &blkno, &run);
		if (error) {
			UVMHIST_LOG(ubchist, "VOP_BMAP() -> %d", error,0,0,0);
			skipbytes += bytes;
			bytes = 0;
			break;
		}

		iobytes = MIN((((off_t)lbn + 1 + run) << fs_bshift) - offset,
		    bytes);
		if (blkno == (daddr_t)-1) {
			if (!write) {
				memset((char *)kva + (offset - startoffset), 0,
				   iobytes);
			}
			skipbytes += iobytes;
			continue;
		}

		/* if it's really one i/o, don't make a second buf */
		if (offset == startoffset && iobytes == bytes) {
			bp = mbp;
		} else {
			UVMHIST_LOG(ubchist, "vp %p bp %p num now %d",
			    vp, bp, vp->v_numoutput, 0);
			bp = getiobuf();
			nestiobuf_setup(mbp, bp, offset - startoffset, iobytes);
		}
		bp->b_lblkno = 0;

		/* adjust physical blkno for partial blocks */
		bp->b_blkno = blkno + ((offset - ((off_t)lbn << fs_bshift)) >>
		    dev_bshift);
		UVMHIST_LOG(ubchist,
		    "vp %p offset 0x%x bcount 0x%x blkno 0x%x",
		    vp, offset, bp->b_bcount, bp->b_blkno);

		VOP_STRATEGY(devvp, bp);
	}
	if (skipbytes) {
		UVMHIST_LOG(ubchist, "skipbytes %d", skipbytes, 0,0,0);
	}
	nestiobuf_done(mbp, skipbytes, error);
	if (async) {
		UVMHIST_LOG(ubchist, "returning 0 (async)", 0,0,0,0);
		return (0);
	}
	UVMHIST_LOG(ubchist, "waiting for mbp %p", mbp,0,0,0);
	error = biowait(mbp);
	s = splbio();
	(*iodone)(mbp);
	splx(s);
	UVMHIST_LOG(ubchist, "returning, error %d", error,0,0,0);
	return (error);
}

/*
 * VOP_PUTPAGES() for vnodes which never have pages.
 */

int
genfs_null_putpages(void *v)
{
	struct vop_putpages_args /* {
		struct vnode *a_vp;
		voff_t a_offlo;
		voff_t a_offhi;
		int a_flags;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	KASSERT(vp->v_uobj.uo_npages == 0);
	simple_unlock(&vp->v_interlock);
	return (0);
}

void
genfs_node_init(struct vnode *vp, const struct genfs_ops *ops)
{
	struct genfs_node *gp = VTOG(vp);

	rw_init(&gp->g_glock);
	gp->g_op = ops;
}

void
genfs_node_destroy(struct vnode *vp)
{
	struct genfs_node *gp = VTOG(vp);

	rw_destroy(&gp->g_glock);
}

void
genfs_size(struct vnode *vp, off_t size, off_t *eobp, int flags)
{
	int bsize;

	bsize = 1 << vp->v_mount->mnt_fs_bshift;
	*eobp = (size + bsize - 1) & ~(bsize - 1);
}

int
genfs_compat_getpages(void *v)
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

	off_t origoffset;
	struct vnode *vp = ap->a_vp;
	struct uvm_object *uobj = &vp->v_uobj;
	struct vm_page *pg, **pgs;
	vaddr_t kva;
	int i, error, orignpages, npages;
	struct iovec iov;
	struct uio uio;
	kauth_cred_t cred = curlwp->l_cred;
	bool write = (ap->a_access_type & VM_PROT_WRITE) != 0;

	error = 0;
	origoffset = ap->a_offset;
	orignpages = *ap->a_count;
	pgs = ap->a_m;

	if (write && (vp->v_flag & VONWORKLST) == 0) {
		vn_syncer_add_to_worklist(vp, filedelay);
	}
	if (ap->a_flags & PGO_LOCKED) {
		uvn_findpages(uobj, origoffset, ap->a_count, ap->a_m,
		    UFP_NOWAIT|UFP_NOALLOC| (write ? UFP_NORDONLY : 0));

		return (ap->a_m[ap->a_centeridx] == NULL ? EBUSY : 0);
	}
	if (origoffset + (ap->a_centeridx << PAGE_SHIFT) >= vp->v_size) {
		simple_unlock(&uobj->vmobjlock);
		return (EINVAL);
	}
	if ((ap->a_flags & PGO_SYNCIO) == 0) {
		simple_unlock(&uobj->vmobjlock);
		return 0;
	}
	npages = orignpages;
	uvn_findpages(uobj, origoffset, &npages, pgs, UFP_ALL);
	simple_unlock(&uobj->vmobjlock);
	kva = uvm_pagermapin(pgs, npages,
	    UVMPAGER_MAPIN_READ | UVMPAGER_MAPIN_WAITOK);
	for (i = 0; i < npages; i++) {
		pg = pgs[i];
		if ((pg->flags & PG_FAKE) == 0) {
			continue;
		}
		iov.iov_base = (char *)kva + (i << PAGE_SHIFT);
		iov.iov_len = PAGE_SIZE;
		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;
		uio.uio_offset = origoffset + (i << PAGE_SHIFT);
		uio.uio_rw = UIO_READ;
		uio.uio_resid = PAGE_SIZE;
		UIO_SETUP_SYSSPACE(&uio);
		/* XXX vn_lock */
		error = VOP_READ(vp, &uio, 0, cred);
		if (error) {
			break;
		}
		if (uio.uio_resid) {
			memset(iov.iov_base, 0, uio.uio_resid);
		}
	}
	uvm_pagermapout(kva, npages);
	simple_lock(&uobj->vmobjlock);
	uvm_lock_pageq();
	for (i = 0; i < npages; i++) {
		pg = pgs[i];
		if (error && (pg->flags & PG_FAKE) != 0) {
			pg->flags |= PG_RELEASED;
		} else {
			pmap_clear_modify(pg);
			uvm_pageactivate(pg);
		}
	}
	if (error) {
		uvm_page_unbusy(pgs, npages);
	}
	uvm_unlock_pageq();
	simple_unlock(&uobj->vmobjlock);
	return (error);
}

int
genfs_compat_gop_write(struct vnode *vp, struct vm_page **pgs, int npages,
    int flags)
{
	off_t offset;
	struct iovec iov;
	struct uio uio;
	kauth_cred_t cred = curlwp->l_cred;
	struct buf *bp;
	vaddr_t kva;
	int s, error;

	offset = pgs[0]->offset;
	kva = uvm_pagermapin(pgs, npages,
	    UVMPAGER_MAPIN_WRITE | UVMPAGER_MAPIN_WAITOK);

	iov.iov_base = (void *)kva;
	iov.iov_len = npages << PAGE_SHIFT;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = offset;
	uio.uio_rw = UIO_WRITE;
	uio.uio_resid = npages << PAGE_SHIFT;
	UIO_SETUP_SYSSPACE(&uio);
	/* XXX vn_lock */
	error = VOP_WRITE(vp, &uio, 0, cred);

	s = splbio();
	V_INCR_NUMOUTPUT(vp);
	splx(s);

	bp = getiobuf();
	bp->b_flags = B_BUSY | B_WRITE | B_AGE;
	bp->b_vp = vp;
	bp->b_lblkno = offset >> vp->v_mount->mnt_fs_bshift;
	bp->b_data = (char *)kva;
	bp->b_bcount = npages << PAGE_SHIFT;
	bp->b_bufsize = npages << PAGE_SHIFT;
	bp->b_resid = 0;
	if (error) {
		bp->b_flags |= B_ERROR;
		bp->b_error = error;
	}
	uvm_aio_aiodone(bp);
	return (error);
}

/*
 * Process a uio using direct I/O.  If we reach a part of the request
 * which cannot be processed in this fashion for some reason, just return.
 * The caller must handle some additional part of the request using
 * buffered I/O before trying direct I/O again.
 */

void
genfs_directio(struct vnode *vp, struct uio *uio, int ioflag)
{
	struct vmspace *vs;
	struct iovec *iov;
	vaddr_t va;
	size_t len;
	const int mask = DEV_BSIZE - 1;
	int error;

	/*
	 * We only support direct I/O to user space for now.
	 */

	if (VMSPACE_IS_KERNEL_P(uio->uio_vmspace)) {
		return;
	}

	/*
	 * If the vnode is mapped, we would need to get the getpages lock
	 * to stabilize the bmap, but then we would get into trouble whil e
	 * locking the pages if the pages belong to this same vnode (or a
	 * multi-vnode cascade to the same effect).  Just fall back to
	 * buffered I/O if the vnode is mapped to avoid this mess.
	 */

	if (vp->v_flag & VMAPPED) {
		return;
	}

	/*
	 * Do as much of the uio as possible with direct I/O.
	 */

	vs = uio->uio_vmspace;
	while (uio->uio_resid) {
		iov = uio->uio_iov;
		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		va = (vaddr_t)iov->iov_base;
		len = MIN(iov->iov_len, genfs_maxdio);
		len &= ~mask;

		/*
		 * If the next chunk is smaller than DEV_BSIZE or extends past
		 * the current EOF, then fall back to buffered I/O.
		 */

		if (len == 0 || uio->uio_offset + len > vp->v_size) {
			return;
		}

		/*
		 * Check alignment.  The file offset must be at least
		 * sector-aligned.  The exact constraint on memory alignment
		 * is very hardware-dependent, but requiring sector-aligned
		 * addresses there too is safe.
		 */

		if (uio->uio_offset & mask || va & mask) {
			return;
		}
		error = genfs_do_directio(vs, va, len, vp, uio->uio_offset,
					  uio->uio_rw);
		if (error) {
			break;
		}
		iov->iov_base = (char *)iov->iov_base + len;
		iov->iov_len -= len;
		uio->uio_offset += len;
		uio->uio_resid -= len;
	}
}

/*
 * Iodone routine for direct I/O.  We don't do much here since the request is
 * always synchronous, so the caller will do most of the work after biowait().
 */

static void
genfs_dio_iodone(struct buf *bp)
{
	int s;

	KASSERT((bp->b_flags & B_ASYNC) == 0);
	s = splbio();
	if ((bp->b_flags & (B_READ | B_AGE)) == B_AGE) {
		vwakeup(bp);
	}
	putiobuf(bp);
	splx(s);
}

/*
 * Process one chunk of a direct I/O request.
 */

static int
genfs_do_directio(struct vmspace *vs, vaddr_t uva, size_t len, struct vnode *vp,
    off_t off, enum uio_rw rw)
{
	struct vm_map *map;
	struct pmap *upm, *kpm;
	size_t klen = round_page(uva + len) - trunc_page(uva);
	off_t spoff, epoff;
	vaddr_t kva, puva;
	paddr_t pa;
	vm_prot_t prot;
	int error, rv, poff, koff;
	const int pgoflags = PGO_CLEANIT | PGO_SYNCIO |
		(rw == UIO_WRITE ? PGO_FREE : 0);

	/*
	 * For writes, verify that this range of the file already has fully
	 * allocated backing store.  If there are any holes, just punt and
	 * make the caller take the buffered write path.
	 */

	if (rw == UIO_WRITE) {
		daddr_t lbn, elbn, blkno;
		int bsize, bshift, run;

		bshift = vp->v_mount->mnt_fs_bshift;
		bsize = 1 << bshift;
		lbn = off >> bshift;
		elbn = (off + len + bsize - 1) >> bshift;
		while (lbn < elbn) {
			error = VOP_BMAP(vp, lbn, NULL, &blkno, &run);
			if (error) {
				return error;
			}
			if (blkno == (daddr_t)-1) {
				return ENOSPC;
			}
			lbn += 1 + run;
		}
	}

	/*
	 * Flush any cached pages for parts of the file that we're about to
	 * access.  If we're writing, invalidate pages as well.
	 */

	spoff = trunc_page(off);
	epoff = round_page(off + len);
	simple_lock(&vp->v_interlock);
	error = VOP_PUTPAGES(vp, spoff, epoff, pgoflags);
	if (error) {
		return error;
	}

	/*
	 * Wire the user pages and remap them into kernel memory.
	 */

	prot = rw == UIO_READ ? VM_PROT_READ | VM_PROT_WRITE : VM_PROT_READ;
	error = uvm_vslock(vs, (void *)uva, len, prot);
	if (error) {
		return error;
	}

	map = &vs->vm_map;
	upm = vm_map_pmap(map);
	kpm = vm_map_pmap(kernel_map);
	kva = uvm_km_alloc(kernel_map, klen, 0,
			   UVM_KMF_VAONLY | UVM_KMF_WAITVA);
	puva = trunc_page(uva);
	for (poff = 0; poff < klen; poff += PAGE_SIZE) {
		rv = pmap_extract(upm, puva + poff, &pa);
		KASSERT(rv);
		pmap_enter(kpm, kva + poff, pa, prot, prot | PMAP_WIRED);
	}
	pmap_update(kpm);

	/*
	 * Do the I/O.
	 */

	koff = uva - trunc_page(uva);
	error = genfs_do_io(vp, off, kva + koff, len, PGO_SYNCIO, rw,
			    genfs_dio_iodone);

	/*
	 * Tear down the kernel mapping.
	 */

	pmap_remove(kpm, kva, kva + klen);
	pmap_update(kpm);
	uvm_km_free(kernel_map, kva, klen, UVM_KMF_VAONLY);

	/*
	 * Unwire the user pages.
	 */

	uvm_vsunlock(vs, (void *)uva, len);
	return error;
}


static void
filt_genfsdetach(struct knote *kn)
{
	struct vnode *vp = (struct vnode *)kn->kn_hook;

	/* XXXLUKEM lock the struct? */
	SLIST_REMOVE(&vp->v_klist, kn, knote, kn_selnext);
}

static int
filt_genfsread(struct knote *kn, long hint)
{
	struct vnode *vp = (struct vnode *)kn->kn_hook;

	/*
	 * filesystem is gone, so set the EOF flag and schedule
	 * the knote for deletion.
	 */
	if (hint == NOTE_REVOKE) {
		kn->kn_flags |= (EV_EOF | EV_ONESHOT);
		return (1);
	}

	/* XXXLUKEM lock the struct? */
	kn->kn_data = vp->v_size - kn->kn_fp->f_offset;
        return (kn->kn_data != 0);
}

static int
filt_genfsvnode(struct knote *kn, long hint)
{

	if (kn->kn_sfflags & hint)
		kn->kn_fflags |= hint;
	if (hint == NOTE_REVOKE) {
		kn->kn_flags |= EV_EOF;
		return (1);
	}
	return (kn->kn_fflags != 0);
}

static const struct filterops genfsread_filtops =
	{ 1, NULL, filt_genfsdetach, filt_genfsread };
static const struct filterops genfsvnode_filtops =
	{ 1, NULL, filt_genfsdetach, filt_genfsvnode };

int
genfs_kqfilter(void *v)
{
	struct vop_kqfilter_args /* {
		struct vnode	*a_vp;
		struct knote	*a_kn;
	} */ *ap = v;
	struct vnode *vp;
	struct knote *kn;

	vp = ap->a_vp;
	kn = ap->a_kn;
	switch (kn->kn_filter) {
	case EVFILT_READ:
		kn->kn_fop = &genfsread_filtops;
		break;
	case EVFILT_VNODE:
		kn->kn_fop = &genfsvnode_filtops;
		break;
	default:
		return (1);
	}

	kn->kn_hook = vp;

	/* XXXLUKEM lock the struct? */
	SLIST_INSERT_HEAD(&vp->v_klist, kn, kn_selnext);

	return (0);
}

void
genfs_node_wrlock(struct vnode *vp)
{
	struct genfs_node *gp = VTOG(vp);

	rw_enter(&gp->g_glock, RW_WRITER);
}

void
genfs_node_rdlock(struct vnode *vp)
{
	struct genfs_node *gp = VTOG(vp);

	rw_enter(&gp->g_glock, RW_READER);
}

void
genfs_node_unlock(struct vnode *vp)
{
	struct genfs_node *gp = VTOG(vp);

	rw_exit(&gp->g_glock);
}
