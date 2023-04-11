/*	$NetBSD: lfs_pages.c,v 1.27 2023/04/11 14:50:47 riastradh Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2001, 2002, 2003, 2019 The NetBSD Foundation, Inc.
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
 *	@(#)lfs_vnops.c	8.13 (Berkeley) 6/10/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lfs_pages.c,v 1.27 2023/04/11 14:50:47 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#include "opt_uvm_page_trkown.h"
#endif

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
#include <sys/pool.h>
#include <sys/signalvar.h>
#include <sys/kauth.h>
#include <sys/syslog.h>
#include <sys/fstrans.h>

#include <miscfs/fifofs/fifo.h>
#include <miscfs/genfs/genfs.h>
#include <miscfs/specfs/specdev.h>

#include <ufs/lfs/ulfs_inode.h>
#include <ufs/lfs/ulfsmount.h>
#include <ufs/lfs/ulfs_bswap.h>
#include <ufs/lfs/ulfs_extern.h>

#include <uvm/uvm.h>
#include <uvm/uvm_page.h>
#include <uvm/uvm_pager.h>
#include <uvm/uvm_pmap.h>
#include <uvm/uvm_stat.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_accessors.h>
#include <ufs/lfs/lfs_kernel.h>
#include <ufs/lfs/lfs_extern.h>

extern kcondvar_t lfs_writerd_cv;

static int check_dirty(struct lfs *, struct vnode *, off_t, off_t, off_t, int, int, struct vm_page **);

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
		mutex_enter(&lfs_lock);
		LFS_SET_UINO(VTOI(ap->a_vp), IN_MODIFIED);
		mutex_exit(&lfs_lock);
	}

	/*
	 * we're relying on the fact that genfs_getpages() always read in
	 * entire filesystem blocks.
	 */
	return genfs_getpages(v);
}

/*
 * Wait for a page to become unbusy, possibly printing diagnostic messages
 * as well.
 *
 * Called with vp->v_uobj.vmobjlock held; return with it held.
 */
static void
wait_for_page(struct vnode *vp, struct vm_page *pg, const char *label)
{
	KASSERT(rw_write_held(vp->v_uobj.vmobjlock));
	if ((pg->flags & PG_BUSY) == 0)
		return;		/* Nothing to wait for! */

#if defined(DEBUG) && defined(UVM_PAGE_TRKOWN)
	static struct vm_page *lastpg;

	if (label != NULL && pg != lastpg) {
		if (pg->owner_tag) {
			printf("lfs_putpages[%d.%d]: %s: page %p owner %d.%d [%s]\n",
			       curproc->p_pid, curlwp->l_lid, label,
			       pg, pg->owner, pg->lowner, pg->owner_tag);
		} else {
			printf("lfs_putpages[%d.%d]: %s: page %p unowned?!\n",
			       curproc->p_pid, curlwp->l_lid, label, pg);
		}
	}
	lastpg = pg;
#endif

	uvm_pagewait(pg, vp->v_uobj.vmobjlock, "lfsput");
	rw_enter(vp->v_uobj.vmobjlock, RW_WRITER);
}

/*
 * This routine is called by lfs_putpages() when it can't complete the
 * write because a page is busy.  This means that either (1) someone,
 * possibly the pagedaemon, is looking at this page, and will give it up
 * presently; or (2) we ourselves are holding the page busy in the
 * process of being written (either gathered or actually on its way to
 * disk).  We don't need to give up the segment lock, but we might need
 * to call lfs_writeseg() to expedite the page's journey to disk.
 *
 * Called with vp->v_uobj.vmobjlock held; return with it held.
 */
/* #define BUSYWAIT */
static void
write_and_wait(struct lfs *fs, struct vnode *vp, struct vm_page *pg,
	       int seglocked, const char *label)
{
	KASSERT(rw_write_held(vp->v_uobj.vmobjlock));
#ifndef BUSYWAIT
	struct inode *ip = VTOI(vp);
	struct segment *sp = fs->lfs_sp;
	int count = 0;

	if (pg == NULL)
		return;

	while (pg->flags & PG_BUSY &&
	    pg->uobject == &vp->v_uobj) {
		rw_exit(vp->v_uobj.vmobjlock);
		if (sp->cbpp - sp->bpp > 1) {
			/* Write gathered pages */
			lfs_updatemeta(sp);
			lfs_release_finfo(fs);
			(void) lfs_writeseg(fs, sp);

			/*
			 * Reinitialize FIP
			 */
			KASSERT(sp->vp == vp);
			lfs_acquire_finfo(fs, ip->i_number,
					  ip->i_gen);
		}
		++count;
		rw_enter(vp->v_uobj.vmobjlock, RW_WRITER);
		wait_for_page(vp, pg, label);
	}
	if (label != NULL && count > 1) {
		DLOG((DLOG_PAGE, "lfs_putpages[%d]: %s: %sn = %d\n",
		      curproc->p_pid, label, (count > 0 ? "looping, " : ""),
		      count));
	}
#else
	preempt(1);
#endif
	KASSERT(rw_write_held(vp->v_uobj.vmobjlock));
}

/*
 * Make sure that for all pages in every block in the given range,
 * either all are dirty or all are clean.  If any of the pages
 * we've seen so far are dirty, put the vnode on the paging chain,
 * and mark it IN_PAGING.
 *
 * If checkfirst != 0, don't check all the pages but return at the
 * first dirty page.
 */
static int
check_dirty(struct lfs *fs, struct vnode *vp,
	    off_t startoffset, off_t endoffset, off_t blkeof,
	    int flags, int checkfirst, struct vm_page **pgp)
{
	struct vm_page *pgs[MAXBSIZE / MIN_PAGE_SIZE], *pg;
	off_t soff = 0; /* XXX: gcc */
	voff_t off;
	int i;
	int nonexistent;
	int any_dirty;	/* number of dirty pages */
	int dirty;	/* number of dirty pages in a block */
	int tdirty;
	int pages_per_block = lfs_sb_getbsize(fs) >> PAGE_SHIFT;
	int pagedaemon = (curlwp == uvm.pagedaemon_lwp);

	KASSERT(rw_write_held(vp->v_uobj.vmobjlock));
	ASSERT_MAYBE_SEGLOCK(fs);
  top:
	any_dirty = 0;

	soff = startoffset;
	KASSERT((soff & (lfs_sb_getbsize(fs) - 1)) == 0);
	while (soff < MIN(blkeof, endoffset)) {

		/*
		 * Mark all pages in extended range busy; find out if any
		 * of them are dirty.
		 */
		nonexistent = dirty = 0;
		for (i = 0; i == 0 || i < pages_per_block; i++) {
			KASSERT(rw_write_held(vp->v_uobj.vmobjlock));
			off = soff + (i << PAGE_SHIFT);
			pgs[i] = pg = uvm_pagelookup(&vp->v_uobj, off);
			if (pg == NULL) {
				++nonexistent;
				continue;
			}
			KASSERT(pg != NULL);

			/*
			 * If we're holding the segment lock, we can deadlock
			 * against a process that has our page and is waiting
			 * for the cleaner, while the cleaner waits for the
			 * segment lock.  Just bail in that case.
			 */
			if ((pg->flags & PG_BUSY) &&
			    (pagedaemon || LFS_SEGLOCK_HELD(fs))) {
				if (i > 0)
					uvm_page_unbusy(pgs, i);
				DLOG((DLOG_PAGE, "lfs_putpages: avoiding 3-way or pagedaemon deadlock\n"));
				if (pgp)
					*pgp = pg;
				KASSERT(rw_write_held(vp->v_uobj.vmobjlock));
				return -1;
			}

			while (pg->flags & PG_BUSY) {
				wait_for_page(vp, pg, NULL);
				KASSERT(rw_write_held(vp->v_uobj.vmobjlock));
				if (i > 0)
					uvm_page_unbusy(pgs, i);
				KASSERT(rw_write_held(vp->v_uobj.vmobjlock));
				goto top;
			}
			pg->flags |= PG_BUSY;
			UVM_PAGE_OWN(pg, "lfs_putpages");

			pmap_page_protect(pg, VM_PROT_NONE);
			tdirty =
			    uvm_pagegetdirty(pg) != UVM_PAGE_STATUS_CLEAN &&
			    (uvm_pagegetdirty(pg) == UVM_PAGE_STATUS_DIRTY ||
			    pmap_clear_modify(pg));
			dirty += tdirty;
		}
		if ((pages_per_block > 0 && nonexistent >= pages_per_block) ||
		    (pages_per_block == 0 && nonexistent > 0)) {
			soff += MAX(PAGE_SIZE, lfs_sb_getbsize(fs));
			continue;
		}

		any_dirty += dirty;
		KASSERT(nonexistent == 0);
		KASSERT(rw_write_held(vp->v_uobj.vmobjlock));

		/*
		 * If any are dirty make all dirty; unbusy them,
		 * but if we were asked to clean, wire them so that
		 * the pagedaemon doesn't bother us about them while
		 * they're on their way to disk.
		 */
		for (i = 0; i == 0 || i < pages_per_block; i++) {
			KASSERT(rw_write_held(vp->v_uobj.vmobjlock));
			pg = pgs[i];
			KASSERT(!(uvm_pagegetdirty(pg) != UVM_PAGE_STATUS_DIRTY
			    && (pg->flags & PG_DELWRI)));
			KASSERT(pg->flags & PG_BUSY);
			if (dirty) {
				uvm_pagemarkdirty(pg, UVM_PAGE_STATUS_DIRTY);
				if (flags & PGO_FREE) {
					/*
					 * Wire the page so that
					 * pdaemon doesn't see it again.
					 */
					uvm_pagelock(pg);
					uvm_pagewire(pg);
					uvm_pageunlock(pg);

					/* Suspended write flag */
					pg->flags |= PG_DELWRI;
				}
			}
			pg->flags &= ~PG_BUSY;
			uvm_pagelock(pg);
			uvm_pagewakeup(pg);
			uvm_pageunlock(pg);
			UVM_PAGE_OWN(pg, NULL);
		}

		if (checkfirst && any_dirty)
			break;

		soff += MAX(PAGE_SIZE, lfs_sb_getbsize(fs));
	}

	KASSERT(rw_write_held(vp->v_uobj.vmobjlock));
	return any_dirty;
}

/*
 * lfs_putpages functions like genfs_putpages except that
 *
 * (1) It needs to bounds-check the incoming requests to ensure that
 *     they are block-aligned; if they are not, expand the range and
 *     do the right thing in case, e.g., the requested range is clean
 *     but the expanded range is dirty.
 *
 * (2) It needs to explicitly send blocks to be written when it is done.
 *     If VOP_PUTPAGES is called without the seglock held, we simply take
 *     the seglock and let lfs_segunlock wait for us.
 *     XXX There might be a bad situation if we have to flush a vnode while
 *     XXX lfs_markv is in operation.  As of this writing we panic in this
 *     XXX case.
 *
 * Assumptions:
 *
 * (1) The caller does not hold any pages in this vnode busy.  If it does,
 *     there is a danger that when we expand the page range and busy the
 *     pages we will deadlock.
 *
 * (2) We are called with vp->v_uobj.vmobjlock held; we must return with it
 *     released.
 *
 * (3) We don't absolutely have to free pages right away, provided that
 *     the request does not have PGO_SYNCIO.  When the pagedaemon gives
 *     us a request with PGO_FREE, we take the pages out of the paging
 *     queue and wake up the writer, which will handle freeing them for us.
 *
 *     We ensure that for any filesystem block, all pages for that
 *     block are either resident or not, even if those pages are higher
 *     than EOF; that means that we will be getting requests to free
 *     "unused" pages above EOF all the time, and should ignore them.
 *
 * (4) If we are called with PGO_LOCKED, the finfo array we are to write
 *     into has been set up for us by lfs_writefile.  If not, we will
 *     have to handle allocating and/or freeing an finfo entry.
 *
 * XXX note that we're (ab)using PGO_LOCKED as "seglock held".
 */

/* How many times to loop before we should start to worry */
#define TOOMANY 4

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
	bool seglocked, sync, pagedaemon, reclaim;
	struct vm_page *pg, *busypg;
	UVMHIST_FUNC("lfs_putpages"); UVMHIST_CALLED(ubchist);
	struct mount *trans_mp;
	int oreclaim = 0;
	int donewriting = 0;
#ifdef DEBUG
	int debug_n_again, debug_n_dirtyclean;
#endif

	vp = ap->a_vp;
	ip = VTOI(vp);
	fs = ip->i_lfs;
	sync = (ap->a_flags & PGO_SYNCIO) != 0;
	reclaim = (ap->a_flags & PGO_RECLAIM) != 0;
	pagedaemon = (curlwp == uvm.pagedaemon_lwp);
	trans_mp = NULL;

	KASSERT(rw_write_held(vp->v_uobj.vmobjlock));

	/* Putpages does nothing for metadata. */
	if (vp == fs->lfs_ivnode || vp->v_type != VREG) {
		rw_exit(vp->v_uobj.vmobjlock);
		return 0;
	}

retry:
	/*
	 * If there are no pages, don't do anything.
	 */
	if (vp->v_uobj.uo_npages == 0) {
		mutex_enter(vp->v_interlock);
		if ((vp->v_iflag & VI_ONWORKLST) &&
		    LIST_FIRST(&vp->v_dirtyblkhd) == NULL) {
			vn_syncer_remove_from_worklist(vp);
		}
		mutex_exit(vp->v_interlock);
		if (trans_mp)
			fstrans_done(trans_mp);
		rw_exit(vp->v_uobj.vmobjlock);
		
		/* Remove us from paging queue, if we were on it */
		mutex_enter(&lfs_lock);
		if (ip->i_state & IN_PAGING) {
			ip->i_state &= ~IN_PAGING;
			TAILQ_REMOVE(&fs->lfs_pchainhd, ip, i_lfs_pchain);
		}
		mutex_exit(&lfs_lock);

		KASSERT(!rw_write_held(vp->v_uobj.vmobjlock));
		return 0;
	}

	blkeof = lfs_blkroundup(fs, ip->i_size);

	/*
	 * Ignore requests to free pages past EOF but in the same block
	 * as EOF, unless the vnode is being reclaimed or the request
	 * is synchronous.  (If the request is sync, it comes from
	 * lfs_truncate.)
	 *
	 * To avoid being flooded with this request, make these pages
	 * look "active".
	 */
	if (!sync && !reclaim &&
	    ap->a_offlo >= ip->i_size && ap->a_offlo < blkeof) {
		origoffset = ap->a_offlo;
		for (off = origoffset; off < blkeof; off += lfs_sb_getbsize(fs)) {
			pg = uvm_pagelookup(&vp->v_uobj, off);
			KASSERT(pg != NULL);
			while (pg->flags & PG_BUSY) {
				uvm_pagewait(pg, vp->v_uobj.vmobjlock, "lfsput2");
				rw_enter(vp->v_uobj.vmobjlock, RW_WRITER);
				/* XXX Page can't change identity here? */
				KDASSERT(pg ==
				    uvm_pagelookup(&vp->v_uobj, off));
			}
			uvm_pagelock(pg);
			uvm_pageactivate(pg);
			uvm_pageunlock(pg);
		}
		ap->a_offlo = blkeof;
		if (ap->a_offhi > 0 && ap->a_offhi <= ap->a_offlo) {
			rw_exit(vp->v_uobj.vmobjlock);
			return 0;
		}
	}

	/*
	 * Extend page range to start and end at block boundaries.
	 * (For the purposes of VOP_PUTPAGES, fragments don't exist.)
	 */
	origoffset = ap->a_offlo;
	origendoffset = ap->a_offhi;
	startoffset = origoffset & ~(lfs_sb_getbmask(fs));
	max_endoffset = (trunc_page(LLONG_MAX) >> lfs_sb_getbshift(fs))
					       << lfs_sb_getbshift(fs);

	if (origendoffset == 0 || ap->a_flags & PGO_ALLPAGES) {
		endoffset = max_endoffset;
		origendoffset = endoffset;
	} else {
		origendoffset = round_page(ap->a_offhi);
		endoffset = round_page(lfs_blkroundup(fs, origendoffset));
	}

	KASSERT(startoffset > 0 || endoffset >= startoffset);
	if (startoffset == endoffset) {
		/* Nothing to do, why were we called? */
		rw_exit(vp->v_uobj.vmobjlock);
		DLOG((DLOG_PAGE, "lfs_putpages: startoffset = endoffset = %"
		      PRId64 "\n", startoffset));
		return 0;
	}

	ap->a_offlo = startoffset;
	ap->a_offhi = endoffset;

	/*
	 * If not cleaning, just send the pages through genfs_putpages
	 * to be returned to the pool.
	 */
	if (!(ap->a_flags & PGO_CLEANIT)) {
		DLOG((DLOG_PAGE, "lfs_putpages: no cleanit vn %p ino %d (flags %x)\n",
		      vp, (int)ip->i_number, ap->a_flags));
		int r = genfs_putpages(v);
		KASSERT(!rw_write_held(vp->v_uobj.vmobjlock));
		return r;
	}

	if (trans_mp /* && (ap->a_flags & PGO_CLEANIT) != 0 */) {
		if (pagedaemon) {
			/* Pagedaemon must not sleep here. */
			trans_mp = vp->v_mount;
			error = fstrans_start_nowait(trans_mp);
			if (error) {
				rw_exit(vp->v_uobj.vmobjlock);
				return error;
			}
		} else {
			/*
			 * Cannot use vdeadcheck() here as this operation
			 * usually gets used from VOP_RECLAIM().  Test for
			 * change of v_mount instead and retry on change.
			 */
			rw_exit(vp->v_uobj.vmobjlock);
			trans_mp = vp->v_mount;
			fstrans_start(trans_mp);
			if (vp->v_mount != trans_mp) {
				fstrans_done(trans_mp);
				trans_mp = NULL;
			}
		}
		rw_enter(vp->v_uobj.vmobjlock, RW_WRITER);
		goto retry;
	}

	/* Set PGO_BUSYFAIL to avoid deadlocks */
	ap->a_flags |= PGO_BUSYFAIL;

	/*
	 * Likewise, if we are asked to clean but the pages are not
	 * dirty, we can just free them using genfs_putpages.
	 */
#ifdef DEBUG
	debug_n_dirtyclean = 0;
#endif
	do {
		int r;
		KASSERT(rw_write_held(vp->v_uobj.vmobjlock));

		/* Count the number of dirty pages */
		r = check_dirty(fs, vp, startoffset, endoffset, blkeof,
				ap->a_flags, 1, NULL);
		if (r < 0) {
			/* Pages are busy with another process */
			rw_exit(vp->v_uobj.vmobjlock);
			error = EDEADLK;
			goto out;
		}
		if (r > 0) /* Some pages are dirty */
			break;

		/*
		 * Sometimes pages are dirtied between the time that
		 * we check and the time we try to clean them.
		 * Instruct lfs_gop_write to return EDEADLK in this case
		 * so we can write them properly.
		 */
		ip->i_lfs_iflags |= LFSI_NO_GOP_WRITE;
		r = genfs_do_putpages(vp, startoffset, endoffset,
				       ap->a_flags & ~PGO_SYNCIO, &busypg);
		ip->i_lfs_iflags &= ~LFSI_NO_GOP_WRITE;
		if (r != EDEADLK) {
			KASSERT(!rw_write_held(vp->v_uobj.vmobjlock));
 			error = r;
			goto out;
		}

		/* One of the pages was busy.  Start over. */
		rw_enter(vp->v_uobj.vmobjlock, RW_WRITER);
		wait_for_page(vp, busypg, "dirtyclean");
#ifdef DEBUG
		++debug_n_dirtyclean;
#endif
	} while(1);

#ifdef DEBUG
	if (debug_n_dirtyclean > TOOMANY)
		DLOG((DLOG_PAGE, "lfs_putpages: dirtyclean: looping, n = %d\n",
		      debug_n_dirtyclean));
#endif

	/*
	 * Dirty and asked to clean.
	 *
	 * Pagedaemon can't actually write LFS pages; wake up
	 * the writer to take care of that.  The writer will
	 * notice the pager inode queue and act on that.
	 *
	 * XXX We must drop the vp->interlock before taking the lfs_lock or we
	 * get a nasty deadlock with lfs_flush_pchain().
	 */
	if (pagedaemon) {
		rw_exit(vp->v_uobj.vmobjlock);
		mutex_enter(&lfs_lock);
		if (!(ip->i_state & IN_PAGING)) {
			ip->i_state |= IN_PAGING;
			TAILQ_INSERT_TAIL(&fs->lfs_pchainhd, ip, i_lfs_pchain);
		}
		cv_broadcast(&lfs_writerd_cv);
		mutex_exit(&lfs_lock);
		preempt();
		KASSERT(!rw_write_held(vp->v_uobj.vmobjlock));
		error = EWOULDBLOCK;
		goto out;
	}

	/*
	 * If this is a file created in a recent dirop, we can't flush its
	 * inode until the dirop is complete.  Drain dirops, then flush the
	 * filesystem (taking care of any other pending dirops while we're
	 * at it).
	 */
	if ((ap->a_flags & (PGO_CLEANIT|PGO_LOCKED)) == PGO_CLEANIT &&
	    (vp->v_uflag & VU_DIROP)) {
		DLOG((DLOG_PAGE, "lfs_putpages: flushing VU_DIROP\n"));

		/*
		 * NB: lfs_flush_fs can recursively call lfs_putpages,
		 * but it won't reach this branch because it passes
		 * PGO_LOCKED.
		 */

		rw_exit(vp->v_uobj.vmobjlock);
		mutex_enter(&lfs_lock);
		lfs_flush_fs(fs, sync ? SEGM_SYNC : 0);
		mutex_exit(&lfs_lock);
		rw_enter(vp->v_uobj.vmobjlock, RW_WRITER);

		/*
		 * The flush will have cleaned out this vnode as well,
		 *  no need to do more to it.
		 *  XXX then why are we falling through and continuing?
		 */

		/*
		 * XXX State may have changed while we dropped the
		 * lock; start over just in case.  The above comment
		 * suggests this should maybe instead be goto out.
		 */
		goto retry;
	}

	/*
	 * This is it.	We are going to write some pages.  From here on
	 * down it's all just mechanics.
	 *
	 * Don't let genfs_putpages wait; lfs_segunlock will wait for us.
	 */
	ap->a_flags &= ~PGO_SYNCIO;

	/*
	 * If we've already got the seglock, flush the node and return.
	 * The FIP has already been set up for us by lfs_writefile,
	 * and FIP cleanup and lfs_updatemeta will also be done there,
	 * unless genfs_putpages returns EDEADLK; then we must flush
	 * what we have, and correct FIP and segment header accounting.
	 */
  get_seglock:
	/*
	 * If we are not called with the segment locked, lock it.
	 * Account for a new FIP in the segment header, and set sp->vp.
	 * (This should duplicate the setup at the top of lfs_writefile().)
	 */
	seglocked = (ap->a_flags & PGO_LOCKED) != 0;
	if (!seglocked) {
		rw_exit(vp->v_uobj.vmobjlock);
		error = lfs_seglock(fs, SEGM_PROT | (sync ? SEGM_SYNC : 0));
		if (error != 0) {
			KASSERT(!rw_write_held(vp->v_uobj.vmobjlock));
 			goto out;
		}
		rw_enter(vp->v_uobj.vmobjlock, RW_WRITER);
		lfs_acquire_finfo(fs, ip->i_number, ip->i_gen);
	}
	sp = fs->lfs_sp;
	KASSERT(sp->vp == NULL);
	sp->vp = vp;

	/* Note segments written by reclaim; only for debugging */
	mutex_enter(vp->v_interlock);
	if (vdead_check(vp, VDEAD_NOWAIT) != 0) {
		sp->seg_flags |= SEGM_RECLAIM;
		fs->lfs_reclino = ip->i_number;
	}
	mutex_exit(vp->v_interlock);

	/*
	 * Ensure that the partial segment is marked SS_DIROP if this
	 * vnode is a DIROP.
	 */
	if (!seglocked && vp->v_uflag & VU_DIROP) {
		SEGSUM *ssp = sp->segsum;

		lfs_ss_setflags(fs, ssp,
				lfs_ss_getflags(fs, ssp) | (SS_DIROP|SS_CONT));
	}

	/*
	 * Loop over genfs_putpages until all pages are gathered.
	 * genfs_putpages() drops the interlock, so reacquire it if necessary.
	 * Whenever we lose the interlock we have to rerun check_dirty, as
	 * well, since more pages might have been dirtied in our absence.
	 */
#ifdef DEBUG
	debug_n_again = 0;
#endif
	do {
		busypg = NULL;
		KASSERT(rw_write_held(vp->v_uobj.vmobjlock));
		if (check_dirty(fs, vp, startoffset, endoffset, blkeof,
				ap->a_flags, 0, &busypg) < 0) {
			write_and_wait(fs, vp, busypg, seglocked, NULL);
			if (!seglocked) {
				rw_exit(vp->v_uobj.vmobjlock);
				lfs_release_finfo(fs);
				lfs_segunlock(fs);
				rw_enter(vp->v_uobj.vmobjlock, RW_WRITER);
			}
			sp->vp = NULL;
			goto get_seglock;
		}
	
		busypg = NULL;
		oreclaim = (ap->a_flags & PGO_RECLAIM);
		ap->a_flags &= ~PGO_RECLAIM;
		error = genfs_do_putpages(vp, startoffset, endoffset,
					   ap->a_flags, &busypg);
		ap->a_flags |= oreclaim;
	
		if (error == EDEADLK || error == EAGAIN) {
			DLOG((DLOG_PAGE, "lfs_putpages: genfs_putpages returned"
			      " %d ino %d off %jx (seg %d)\n", error,
			      ip->i_number, (uintmax_t)lfs_sb_getoffset(fs),
			      lfs_dtosn(fs, lfs_sb_getoffset(fs))));

			if (oreclaim) {
				rw_enter(vp->v_uobj.vmobjlock, RW_WRITER);
				write_and_wait(fs, vp, busypg, seglocked, "again");
				rw_exit(vp->v_uobj.vmobjlock);
			} else {
				if ((sp->seg_flags & SEGM_SINGLE) &&
				    lfs_sb_getcurseg(fs) != fs->lfs_startseg)
					donewriting = 1;
			}
		} else if (error) {
			DLOG((DLOG_PAGE, "lfs_putpages: genfs_putpages returned"
			      " %d ino %d off %jx (seg %d)\n", error,
			      (int)ip->i_number, (uintmax_t)lfs_sb_getoffset(fs),
			      lfs_dtosn(fs, lfs_sb_getoffset(fs))));
		}
		/* genfs_do_putpages loses the interlock */
#ifdef DEBUG
		++debug_n_again;
#endif
		if (oreclaim && error == EAGAIN) {
			DLOG((DLOG_PAGE, "vp %p ino %d vi_flags %x a_flags %x avoiding vclean panic\n",
			      vp, (int)ip->i_number, vp->v_iflag, ap->a_flags));
			rw_enter(vp->v_uobj.vmobjlock, RW_WRITER);
		}
		if (error == EDEADLK)
			rw_enter(vp->v_uobj.vmobjlock, RW_WRITER);
	} while (error == EDEADLK || (oreclaim && error == EAGAIN));
#ifdef DEBUG
	if (debug_n_again > TOOMANY)
		DLOG((DLOG_PAGE, "lfs_putpages: again: looping, n = %d\n", debug_n_again));
#endif

	KASSERT(sp != NULL && sp->vp == vp);
	if (!seglocked && !donewriting) {
		sp->vp = NULL;

		/* Write indirect blocks as well */
		lfs_gather(fs, fs->lfs_sp, vp, lfs_match_indir);
		lfs_gather(fs, fs->lfs_sp, vp, lfs_match_dindir);
		lfs_gather(fs, fs->lfs_sp, vp, lfs_match_tindir);

		KASSERT(sp->vp == NULL);
		sp->vp = vp;
	}

	/*
	 * Blocks are now gathered into a segment waiting to be written.
	 * All that's left to do is update metadata, and write them.
	 */
	lfs_updatemeta(sp);
	KASSERT(sp->vp == vp);
	sp->vp = NULL;

	/*
	 * If we were called from lfs_writefile, we don't need to clean up
	 * the FIP or unlock the segment lock.	We're done.
	 */
	if (seglocked) {
		KASSERT(!rw_write_held(vp->v_uobj.vmobjlock));
		goto out;
	}

	/* Clean up FIP and send it to disk. */
	lfs_release_finfo(fs);
	lfs_writeseg(fs, fs->lfs_sp);

	/*
	 * Remove us from paging queue if we wrote all our pages.
	 */
	if (origendoffset == 0 || ap->a_flags & PGO_ALLPAGES) {
		mutex_enter(&lfs_lock);
		if (ip->i_state & IN_PAGING) {
			ip->i_state &= ~IN_PAGING;
			TAILQ_REMOVE(&fs->lfs_pchainhd, ip, i_lfs_pchain);
		}
		mutex_exit(&lfs_lock);
	}

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
		mutex_enter(vp->v_interlock);
		while (vp->v_numoutput > 0) {
			DLOG((DLOG_PAGE, "lfs_putpages: ino %d sleeping on"
			      " num %d\n", ip->i_number, vp->v_numoutput));
			cv_wait(&vp->v_cv, vp->v_interlock);
		}
		mutex_exit(vp->v_interlock);
	}

out:;
	if (trans_mp)
		fstrans_done(trans_mp);
	KASSERT(!rw_write_held(vp->v_uobj.vmobjlock));
	return error;
}

