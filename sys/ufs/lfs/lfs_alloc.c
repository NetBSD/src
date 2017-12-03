/*	$NetBSD: lfs_alloc.c,v 1.112.2.4 2017/12/03 11:39:22 jdolecek Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2001, 2002, 2003, 2007 The NetBSD Foundation, Inc.
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
 * Copyright (c) 1991, 1993
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
 *	@(#)lfs_alloc.c	8.4 (Berkeley) 1/4/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lfs_alloc.c,v 1.112.2.4 2017/12/03 11:39:22 jdolecek Exp $");

#if defined(_KERNEL_OPT)
#include "opt_quota.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/lock.h>
#include <sys/vnode.h>
#include <sys/syslog.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/kauth.h>

#include <ufs/lfs/ulfs_quotacommon.h>
#include <ufs/lfs/ulfs_inode.h>
#include <ufs/lfs/ulfsmount.h>
#include <ufs/lfs/ulfs_extern.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_accessors.h>
#include <ufs/lfs/lfs_extern.h>
#include <ufs/lfs/lfs_kernel.h>

/* Constants for inode free bitmap */
#define BMSHIFT 5	/* 2 ** 5 = 32 */
#define BMMASK  ((1 << BMSHIFT) - 1)
#define SET_BITMAP_FREE(F, I) do { \
	DLOG((DLOG_ALLOC, "lfs: ino %d wrd %d bit %d set\n", (int)(I), 	\
	     (int)((I) >> BMSHIFT), (int)((I) & BMMASK)));		\
	(F)->lfs_ino_bitmap[(I) >> BMSHIFT] |= (1 << ((I) & BMMASK));	\
} while (0)
#define CLR_BITMAP_FREE(F, I) do { \
	DLOG((DLOG_ALLOC, "lfs: ino %d wrd %d bit %d clr\n", (int)(I), 	\
	     (int)((I) >> BMSHIFT), (int)((I) & BMMASK)));		\
	(F)->lfs_ino_bitmap[(I) >> BMSHIFT] &= ~(1 << ((I) & BMMASK));	\
} while(0)

#define ISSET_BITMAP_FREE(F, I) \
	((F)->lfs_ino_bitmap[(I) >> BMSHIFT] & (1 << ((I) & BMMASK)))

/*
 * Add a new block to the Ifile, to accommodate future file creations.
 * Called with the segment lock held.
 */
int
lfs_extend_ifile(struct lfs *fs, kauth_cred_t cred)
{
	struct vnode *vp;
	struct inode *ip;
	IFILE64 *ifp64;
	IFILE32 *ifp32;
	IFILE_V1 *ifp_v1;
	struct buf *bp, *cbp;
	int error;
	daddr_t i, blkno, xmax;
	ino_t oldlast, maxino;
	CLEANERINFO *cip;

	ASSERT_SEGLOCK(fs);

	/* XXX should check or assert that we aren't readonly. */

	/*
	 * Get a block and extend the ifile inode. Leave the buffer for
	 * the block in bp.
	 */

	vp = fs->lfs_ivnode;
	ip = VTOI(vp);
	blkno = lfs_lblkno(fs, ip->i_size);
	if ((error = lfs_balloc(vp, ip->i_size, lfs_sb_getbsize(fs), cred, 0,
				&bp)) != 0) {
		return (error);
	}
	ip->i_size += lfs_sb_getbsize(fs);
	lfs_dino_setsize(fs, ip->i_din, ip->i_size);
	uvm_vnp_setsize(vp, ip->i_size);

	/*
	 * Compute the new number of inodes, and reallocate the in-memory
	 * inode freemap.
	 */

	maxino = ((ip->i_size >> lfs_sb_getbshift(fs)) - lfs_sb_getcleansz(fs) -
		  lfs_sb_getsegtabsz(fs)) * lfs_sb_getifpb(fs);
	fs->lfs_ino_bitmap = (lfs_bm_t *)
		realloc(fs->lfs_ino_bitmap, ((maxino + BMMASK) >> BMSHIFT) *
			sizeof(lfs_bm_t), M_SEGMENT, M_WAITOK);
	KASSERT(fs->lfs_ino_bitmap != NULL);

	/* first new inode number */
	i = (blkno - lfs_sb_getsegtabsz(fs) - lfs_sb_getcleansz(fs)) *
		lfs_sb_getifpb(fs);

	/*
	 * We insert the new inodes at the head of the free list.
	 * Under normal circumstances, the free list is empty here,
	 * so we are also incidentally placing them at the end (which
	 * we must do if we are to keep them in order).
	 */
	LFS_GET_HEADFREE(fs, cip, cbp, &oldlast);
	LFS_PUT_HEADFREE(fs, cip, cbp, i);
	KASSERTMSG((lfs_sb_getfreehd(fs) != LFS_UNUSED_INUM),
	    "inode 0 allocated [2]");

	/* inode number to stop at (XXX: why *x*max?) */
	xmax = i + lfs_sb_getifpb(fs);

	/*
	 * Initialize the ifile block.
	 *
	 * XXX: these loops should be restructured to use the accessor
	 * functions instead of using cutpaste polymorphism.
	 */

	if (fs->lfs_is64) {
		for (ifp64 = (IFILE64 *)bp->b_data; i < xmax; ++ifp64) {
			SET_BITMAP_FREE(fs, i);
			ifp64->if_version = 1;
			ifp64->if_daddr = LFS_UNUSED_DADDR;
			ifp64->if_nextfree = ++i;
		}
		ifp64--;
		ifp64->if_nextfree = oldlast;
	} else if (lfs_sb_getversion(fs) > 1) {
		for (ifp32 = (IFILE32 *)bp->b_data; i < xmax; ++ifp32) {
			SET_BITMAP_FREE(fs, i);
			ifp32->if_version = 1;
			ifp32->if_daddr = LFS_UNUSED_DADDR;
			ifp32->if_nextfree = ++i;
		}
		ifp32--;
		ifp32->if_nextfree = oldlast;
	} else {
		for (ifp_v1 = (IFILE_V1 *)bp->b_data; i < xmax; ++ifp_v1) {
			SET_BITMAP_FREE(fs, i);
			ifp_v1->if_version = 1;
			ifp_v1->if_daddr = LFS_UNUSED_DADDR;
			ifp_v1->if_nextfree = ++i;
		}
		ifp_v1--;
		ifp_v1->if_nextfree = oldlast;
	}
	LFS_PUT_TAILFREE(fs, cip, cbp, xmax - 1);

	/*
	 * Write out the new block.
	 */

	(void) LFS_BWRITE_LOG(bp); /* Ifile */

	return 0;
}

/*
 * Allocate an inode for a new file.
 *
 * Takes the segment lock. Also (while holding it) takes lfs_lock
 * to frob fs->lfs_fmod.
 *
 * XXX: the mode argument is unused; should just get rid of it.
 */
/* ARGSUSED */
/* VOP_BWRITE 2i times */
int
lfs_valloc(struct vnode *pvp, int mode, kauth_cred_t cred,
    ino_t *ino, int *gen)
{
	struct lfs *fs;
	struct buf *bp, *cbp;
	IFILE *ifp;
	int error;
	CLEANERINFO *cip;

	fs = VTOI(pvp)->i_lfs;
	if (fs->lfs_ronly)
		return EROFS;

	ASSERT_NO_SEGLOCK(fs);

	lfs_seglock(fs, SEGM_PROT);

	/* Get the head of the freelist. */
	LFS_GET_HEADFREE(fs, cip, cbp, ino);

	/* paranoia */
	KASSERT(*ino != LFS_UNUSED_INUM && *ino != LFS_IFILE_INUM);
	DLOG((DLOG_ALLOC, "lfs_valloc: allocate inode %" PRId64 "\n",
	     *ino));

	/* Update the in-memory inode freemap */
	CLR_BITMAP_FREE(fs, *ino);

	/*
	 * Fetch the ifile entry and make sure the inode is really
	 * free.
	 */
	LFS_IENTRY(ifp, fs, *ino, bp);
	if (lfs_if_getdaddr(fs, ifp) != LFS_UNUSED_DADDR)
		panic("lfs_valloc: inuse inode %" PRId64 " on the free list",
		    *ino);

	/* Update the inode freelist head in the superblock. */
	LFS_PUT_HEADFREE(fs, cip, cbp, lfs_if_getnextfree(fs, ifp));
	DLOG((DLOG_ALLOC, "lfs_valloc: headfree %" PRId64 " -> %ju\n",
	     *ino, (uintmax_t)lfs_if_getnextfree(fs, ifp)));

	/*
	 * Retrieve the version number from the ifile entry. It was
	 * bumped by vfree, so don't bump it again.
	 */
	*gen = lfs_if_getversion(fs, ifp);

	/* Done with ifile entry */
	brelse(bp, 0);

	if (lfs_sb_getfreehd(fs) == LFS_UNUSED_INUM) {
		/*
		 * No more inodes; extend the ifile so that the next
		 * lfs_valloc will succeed.
		 */
		if ((error = lfs_extend_ifile(fs, cred)) != 0) {
			/* restore the freelist */
			LFS_PUT_HEADFREE(fs, cip, cbp, *ino);

			/* unlock and return */
			lfs_segunlock(fs);
			return error;
		}
	}
	KASSERTMSG((lfs_sb_getfreehd(fs) != LFS_UNUSED_INUM),
	    "inode 0 allocated [3]");

	/* Set superblock modified bit */
	mutex_enter(&lfs_lock);
	fs->lfs_fmod = 1;
	mutex_exit(&lfs_lock);

	/* increment file count */
	lfs_sb_addnfiles(fs, 1);

	/* done */
	lfs_segunlock(fs);
	return 0;
}

/*
 * Allocate an inode for a new file, with given inode number and
 * version.
 *
 * Called in the same context as lfs_valloc and therefore shares the
 * same locking assumptions.
 */
int
lfs_valloc_fixed(struct lfs *fs, ino_t ino, int vers)
{
	IFILE *ifp;
	struct buf *bp, *cbp;
	ino_t headino, thisino, oldnext;
	CLEANERINFO *cip;

	if (fs->lfs_ronly)
		return EROFS;

	ASSERT_NO_SEGLOCK(fs);

	lfs_seglock(fs, SEGM_PROT);

	/*
	 * If the ifile is too short to contain this inum, extend it.
	 *
	 * XXX: lfs_extend_ifile should take a size instead of always
	 * doing just one block at time.
	 */
	while (VTOI(fs->lfs_ivnode)->i_size <= (ino /
		lfs_sb_getifpb(fs) + lfs_sb_getcleansz(fs) + lfs_sb_getsegtabsz(fs))
		<< lfs_sb_getbshift(fs)) {
		lfs_extend_ifile(fs, NOCRED);
	}

	/*
	 * fetch the ifile entry; get the inode freelist next pointer,
	 * and set the version as directed.
	 */
	LFS_IENTRY(ifp, fs, ino, bp);
	oldnext = lfs_if_getnextfree(fs, ifp);
	lfs_if_setversion(fs, ifp, vers);
	brelse(bp, 0);

	/* Get head of inode freelist */
	LFS_GET_HEADFREE(fs, cip, cbp, &headino);
	if (headino == ino) {
		/* Easy case: the inode we wanted was at the head */
		LFS_PUT_HEADFREE(fs, cip, cbp, oldnext);
	} else {
		ino_t nextfree;

		/* Have to find the desired inode in the freelist... */

		thisino = headino;
		while (1) {
			/* read this ifile entry */
			LFS_IENTRY(ifp, fs, thisino, bp);
			nextfree = lfs_if_getnextfree(fs, ifp);
			/* stop if we find it or we hit the end */
			if (nextfree == ino ||
			    nextfree == LFS_UNUSED_INUM)
				break;
			/* nope, keep going... */
			thisino = nextfree;
			brelse(bp, 0);
		}
		if (nextfree == LFS_UNUSED_INUM) {
			/* hit the end -- this inode is not available */
			brelse(bp, 0);
			lfs_segunlock(fs);
			return ENOENT;
		}
		/* found it; update the next pointer */
		lfs_if_setnextfree(fs, ifp, oldnext);
		/* write the ifile block */
		LFS_BWRITE_LOG(bp);
	}

	/* done */
	lfs_segunlock(fs);
	return 0;
}

#if 0
/*
 * Find the highest-numbered allocated inode.
 * This will be used to shrink the Ifile.
 */
static inline ino_t
lfs_last_alloc_ino(struct lfs *fs)
{
	ino_t ino, maxino;

	maxino = ((fs->lfs_ivnode->v_size >> lfs_sb_getbshift(fs)) -
		  lfs_sb_getcleansz(fs) - lfs_sb_getsegtabsz(fs)) *
		lfs_sb_getifpb(fs);
	for (ino = maxino - 1; ino > LFS_UNUSED_INUM; --ino) {
		if (ISSET_BITMAP_FREE(fs, ino) == 0)
			break;
	}
	return ino;
}
#endif

/*
 * Find the previous (next lowest numbered) free inode, if any.
 * If there is none, return LFS_UNUSED_INUM.
 *
 * XXX: locking?
 */
static inline ino_t
lfs_freelist_prev(struct lfs *fs, ino_t ino)
{
	ino_t tino, bound, bb, freehdbb;

	if (lfs_sb_getfreehd(fs) == LFS_UNUSED_INUM) {
		/* No free inodes at all */
		return LFS_UNUSED_INUM;
	}

	/* Search our own word first */
	bound = ino & ~BMMASK;
	for (tino = ino - 1; tino >= bound && tino > LFS_UNUSED_INUM; tino--)
		if (ISSET_BITMAP_FREE(fs, tino))
			return tino;
	/* If there are no lower words to search, just return */
	if (ino >> BMSHIFT == 0)
		return LFS_UNUSED_INUM;

	/*
	 * Find a word with a free inode in it.  We have to be a bit
	 * careful here since ino_t is unsigned.
	 */
	freehdbb = (lfs_sb_getfreehd(fs) >> BMSHIFT);
	for (bb = (ino >> BMSHIFT) - 1; bb >= freehdbb && bb > 0; --bb)
		if (fs->lfs_ino_bitmap[bb])
			break;
	if (fs->lfs_ino_bitmap[bb] == 0)
		return LFS_UNUSED_INUM;

	/* Search the word we found */
	for (tino = (bb << BMSHIFT) | BMMASK; tino >= (bb << BMSHIFT) &&
	     tino > LFS_UNUSED_INUM; tino--)
		if (ISSET_BITMAP_FREE(fs, tino))
			break;

	/* Avoid returning reserved inode numbers */
	if (tino <= LFS_IFILE_INUM)
		tino = LFS_UNUSED_INUM;

	return tino;
}

/*
 * Free an inode.
 *
 * Takes lfs_seglock. Also (independently) takes vp->v_interlock.
 */
/* ARGUSED */
/* VOP_BWRITE 2i times */
int
lfs_vfree(struct vnode *vp, ino_t ino, int mode)
{
	SEGUSE *sup;
	CLEANERINFO *cip;
	struct buf *cbp, *bp;
	IFILE *ifp;
	struct inode *ip;
	struct lfs *fs;
	daddr_t old_iaddr;
	ino_t otail;

	/* Get the inode number and file system. */
	ip = VTOI(vp);
	fs = ip->i_lfs;
	ino = ip->i_number;

	/* XXX: assert not readonly */

	ASSERT_NO_SEGLOCK(fs);
	DLOG((DLOG_ALLOC, "lfs_vfree: free ino %lld\n", (long long)ino));

	/* Drain of pending writes */
	mutex_enter(vp->v_interlock);
	while (lfs_sb_getversion(fs) > 1 && WRITEINPROG(vp)) {
		cv_wait(&vp->v_cv, vp->v_interlock);
	}
	mutex_exit(vp->v_interlock);

	lfs_seglock(fs, SEGM_PROT);

	/*
	 * If the inode was in a dirop, it isn't now.
	 *
	 * XXX: why are (v_uflag & VU_DIROP) and (ip->i_state & IN_ADIROP)
	 * not updated together in one function? (and why do both exist,
	 * anyway?)
	 */
	UNMARK_VNODE(vp);

	mutex_enter(&lfs_lock);
	if (vp->v_uflag & VU_DIROP) {
		vp->v_uflag &= ~VU_DIROP;
		--lfs_dirvcount;
		--fs->lfs_dirvcount;
		TAILQ_REMOVE(&fs->lfs_dchainhd, ip, i_lfs_dchain);
		wakeup(&fs->lfs_dirvcount);
		wakeup(&lfs_dirvcount);
		mutex_exit(&lfs_lock);
		vrele(vp);

		/*
		 * If this inode is not going to be written any more, any
		 * segment accounting left over from its truncation needs
		 * to occur at the end of the next dirops flush.  Attach
		 * them to the fs-wide list for that purpose.
		 */
		if (LIST_FIRST(&ip->i_lfs_segdhd) != NULL) {
			struct segdelta *sd;
	
			while((sd = LIST_FIRST(&ip->i_lfs_segdhd)) != NULL) {
				LIST_REMOVE(sd, list);
				LIST_INSERT_HEAD(&fs->lfs_segdhd, sd, list);
			}
		}
	} else {
		/*
		 * If it's not a dirop, we can finalize right away.
		 */
		mutex_exit(&lfs_lock);
		lfs_finalize_ino_seguse(fs, ip);
	}

	/* it is no longer an unwritten inode, so update the counts */
	mutex_enter(&lfs_lock);
	LFS_CLR_UINO(ip, IN_ACCESSED|IN_CLEANING|IN_MODIFIED);
	mutex_exit(&lfs_lock);

	/* Turn off all inode modification flags */
	ip->i_state &= ~IN_ALLMOD;

	/* Mark it deleted */
	ip->i_lfs_iflags |= LFSI_DELETED;
	
	/* Mark it free in the in-memory inode freemap */
	SET_BITMAP_FREE(fs, ino);

	/*
	 * Set the ifile's inode entry to unused, increment its version number
	 * and link it onto the free chain.
	 */

	/* fetch the ifile entry */
	LFS_IENTRY(ifp, fs, ino, bp);

	/* update the on-disk address (to "nowhere") */
	old_iaddr = lfs_if_getdaddr(fs, ifp);
	lfs_if_setdaddr(fs, ifp, LFS_UNUSED_DADDR);

	/* bump the version */
	lfs_if_setversion(fs, ifp, lfs_if_getversion(fs, ifp) + 1);

	if (lfs_sb_getversion(fs) == 1) {
		ino_t nextfree;

		/* insert on freelist */
		LFS_GET_HEADFREE(fs, cip, cbp, &nextfree);
		lfs_if_setnextfree(fs, ifp, nextfree);
		LFS_PUT_HEADFREE(fs, cip, cbp, ino);

		/* write the ifile block */
		(void) LFS_BWRITE_LOG(bp); /* Ifile */
	} else {
		ino_t tino, onf;

		/*
		 * Clear the freelist next pointer and write the ifile
		 * block. XXX: why? I'm sure there must be a reason but
		 * it seems both silly and dangerous.
		 */
		lfs_if_setnextfree(fs, ifp, LFS_UNUSED_INUM);
		(void) LFS_BWRITE_LOG(bp); /* Ifile */

		/*
		 * Insert on freelist in order.
		 */

		/* Find the next lower (by number) free inode */
		tino = lfs_freelist_prev(fs, ino);

		if (tino == LFS_UNUSED_INUM) {
			ino_t nextfree;

			/*
			 * There isn't one; put us on the freelist head.
			 */

			/* reload the ifile block */
			LFS_IENTRY(ifp, fs, ino, bp);
			/* update the list */
			LFS_GET_HEADFREE(fs, cip, cbp, &nextfree);
			lfs_if_setnextfree(fs, ifp, nextfree);
			LFS_PUT_HEADFREE(fs, cip, cbp, ino);
			DLOG((DLOG_ALLOC, "lfs_vfree: headfree %lld -> %lld\n",
			     (long long)nextfree, (long long)ino));
			/* write the ifile block */
			LFS_BWRITE_LOG(bp); /* Ifile */

			/* If the list was empty, set tail too */
			LFS_GET_TAILFREE(fs, cip, cbp, &otail);
			if (otail == LFS_UNUSED_INUM) {
				LFS_PUT_TAILFREE(fs, cip, cbp, ino);
				DLOG((DLOG_ALLOC, "lfs_vfree: tailfree %lld "
				      "-> %lld\n", (long long)otail,
				      (long long)ino));
			}
		} else {
			/*
			 * Insert this inode into the list after tino.
			 * We hold the segment lock so we don't have to
			 * worry about blocks being written out of order.
			 */

			DLOG((DLOG_ALLOC, "lfs_vfree: insert ino %lld "
			      " after %lld\n", ino, tino));

			/* load the previous inode's ifile block */
			LFS_IENTRY(ifp, fs, tino, bp);
			/* update the list pointer */
			onf = lfs_if_getnextfree(fs, ifp);
			lfs_if_setnextfree(fs, ifp, ino);
			/* write the block */
			LFS_BWRITE_LOG(bp);	/* Ifile */

			/* load this inode's ifile block */
			LFS_IENTRY(ifp, fs, ino, bp);
			/* update the list pointer */
			lfs_if_setnextfree(fs, ifp, onf);
			/* write the block */
			LFS_BWRITE_LOG(bp);	/* Ifile */

			/* If we're last, put us on the tail */
			if (onf == LFS_UNUSED_INUM) {
				LFS_GET_TAILFREE(fs, cip, cbp, &otail);
				LFS_PUT_TAILFREE(fs, cip, cbp, ino);
				DLOG((DLOG_ALLOC, "lfs_vfree: tailfree %lld "
				      "-> %lld\n", (long long)otail,
				      (long long)ino));
			}
		}
	}
	/* XXX: shouldn't this check be further up *before* we trash the fs? */
	KASSERTMSG((ino != LFS_UNUSED_INUM), "inode 0 freed");

	/*
	 * Update the segment summary for the segment where the on-disk
	 * copy used to be.
	 */
	if (old_iaddr != LFS_UNUSED_DADDR) {
		/* load it */
		LFS_SEGENTRY(sup, fs, lfs_dtosn(fs, old_iaddr), bp);
		/* the number of bytes in the segment should not become < 0 */
		KASSERTMSG((sup->su_nbytes >= DINOSIZE(fs)),
		    "lfs_vfree: negative byte count"
		    " (segment %" PRIu32 " short by %d)\n",
		    lfs_dtosn(fs, old_iaddr),
		    (int)DINOSIZE(fs) - sup->su_nbytes);
		/* update the number of bytes in the segment */
		sup->su_nbytes -= DINOSIZE(fs);
		/* write the segment entry */
		LFS_WRITESEGENTRY(sup, fs, lfs_dtosn(fs, old_iaddr), bp); /* Ifile */
	}

	/* Set superblock modified bit. */
	mutex_enter(&lfs_lock);
	fs->lfs_fmod = 1;
	mutex_exit(&lfs_lock);

	/* Decrement file count. */
	lfs_sb_subnfiles(fs, 1);

	lfs_segunlock(fs);

	return (0);
}

/*
 * Sort the freelist and set up the free-inode bitmap.
 * To be called by lfs_mountfs().
 *
 * Takes the segmenet lock.
 */
void
lfs_order_freelist(struct lfs *fs)
{
	CLEANERINFO *cip;
	IFILE *ifp = NULL;
	struct buf *bp;
	ino_t ino, firstino, lastino, maxino;
#ifdef notyet
	struct vnode *vp;
#endif
	
	ASSERT_NO_SEGLOCK(fs);
	lfs_seglock(fs, SEGM_PROT);

	/* largest inode on fs */
	maxino = ((fs->lfs_ivnode->v_size >> lfs_sb_getbshift(fs)) -
		  lfs_sb_getcleansz(fs) - lfs_sb_getsegtabsz(fs)) * lfs_sb_getifpb(fs);

	/* allocate the in-memory inode freemap */
	/* XXX: assert that fs->lfs_ino_bitmap is null here */
	fs->lfs_ino_bitmap =
		malloc(((maxino + BMMASK) >> BMSHIFT) * sizeof(lfs_bm_t),
		       M_SEGMENT, M_WAITOK | M_ZERO);
	KASSERT(fs->lfs_ino_bitmap != NULL);

	/*
	 * Scan the ifile.
	 */

	firstino = lastino = LFS_UNUSED_INUM;
	for (ino = 0; ino < maxino; ino++) {
		/* Load this inode's ifile entry. */
		if (ino % lfs_sb_getifpb(fs) == 0)
			LFS_IENTRY(ifp, fs, ino, bp);
		else
			LFS_IENTRY_NEXT(ifp, fs);

		/* Don't put zero or ifile on the free list */
		if (ino == LFS_UNUSED_INUM || ino == LFS_IFILE_INUM)
			continue;

#ifdef notyet
		/*
		 * Address orphaned files.
		 *
		 * The idea of this is to free inodes belonging to
		 * files that were unlinked but not reclaimed, I guess
		 * because if we're going to scan the whole ifile
		 * anyway it costs very little to do this. I don't
		 * immediately see any reason this should be disabled,
		 * but presumably it doesn't work... not sure what
		 * happens to such files currently. -- dholland 20160806
		 */
		if (lfs_if_getnextfree(fs, ifp) == LFS_ORPHAN_NEXTFREE &&
		    VFS_VGET(fs->lfs_ivnode->v_mount, ino, &vp) == 0) {
			unsigned segno;

			/* get the segment the inode in on disk  */
			segno = lfs_dtosn(fs, lfs_if_getdaddr(fs, ifp));

			/* truncate the inode */
			lfs_truncate(vp, 0, 0, NOCRED);
			vput(vp);

			/* load the segment summary */
			LFS_SEGENTRY(sup, fs, segno, bp);
			/* update the number of bytes in the segment */
			KASSERT(sup->su_nbytes >= DINOSIZE(fs));
			sup->su_nbytes -= DINOSIZE(fs);
			/* write the segment summary */
			LFS_WRITESEGENTRY(sup, fs, segno, bp);

			/* Drop the on-disk address */
			lfs_if_setdaddr(fs, ifp, LFS_UNUSED_DADDR);
			/* write the ifile entry */
			LFS_BWRITE_LOG(bp);

			/*
			 * and reload it (XXX: why? I guess
			 * LFS_BWRITE_LOG drops it...)
			 */
			LFS_IENTRY(ifp, fs, ino, bp);

			/* Fall through to next if block */
		}
#endif

		if (lfs_if_getdaddr(fs, ifp) == LFS_UNUSED_DADDR) {

			/*
			 * This inode is free. Put it on the free list.
			 */

			if (firstino == LFS_UNUSED_INUM) {
				/* XXX: assert lastino == LFS_UNUSED_INUM? */
				/* remember the first free inode */
				firstino = ino;
			} else {
				/* release this inode's ifile entry */
				brelse(bp, 0);

				/* XXX: assert lastino != LFS_UNUSED_INUM? */

				/* load lastino's ifile entry */
				LFS_IENTRY(ifp, fs, lastino, bp);
				/* set the list pointer */
				lfs_if_setnextfree(fs, ifp, ino);
				/* write the block */
				LFS_BWRITE_LOG(bp);

				/* reload this inode's ifile entry */
				LFS_IENTRY(ifp, fs, ino, bp);
			}
			/* remember the last free inode seen so far */
			lastino = ino;

			/* Mark this inode free in the in-memory freemap */
			SET_BITMAP_FREE(fs, ino);
		}

		/* If moving to the next ifile block, release the buffer. */
		if ((ino + 1) % lfs_sb_getifpb(fs) == 0)
			brelse(bp, 0);
	}

	/* Write the freelist head and tail pointers */
	/* XXX: do we need to mark the superblock dirty? */
	LFS_PUT_HEADFREE(fs, cip, bp, firstino);
	LFS_PUT_TAILFREE(fs, cip, bp, lastino);

	/* done */
	lfs_segunlock(fs);
}

/*
 * Mark a file orphaned (unlinked but not yet reclaimed) by inode
 * number. Do this with a magic freelist next pointer.
 *
 * XXX: howzabout some locking?
 */
void
lfs_orphan(struct lfs *fs, ino_t ino)
{
	IFILE *ifp;
	struct buf *bp;

	LFS_IENTRY(ifp, fs, ino, bp);
	lfs_if_setnextfree(fs, ifp, LFS_ORPHAN_NEXTFREE);
	LFS_BWRITE_LOG(bp);
}
