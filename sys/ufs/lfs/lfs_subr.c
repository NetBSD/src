/*	$NetBSD: lfs_subr.c,v 1.33 2003/02/20 04:27:24 perseant Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2001, 2002, 2003 The NetBSD Foundation, Inc.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 *	@(#)lfs_subr.c	8.4 (Berkeley) 5/8/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lfs_subr.c,v 1.33 2003/02/20 04:27:24 perseant Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/proc.h>

#include <ufs/ufs/inode.h>
#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

#include <uvm/uvm.h>

/*
 * Return buffer with the contents of block "offset" from the beginning of
 * directory "ip".  If "res" is non-zero, fill it in with a pointer to the
 * remaining space in the directory.
 */
int
lfs_blkatoff(void *v)
{
	struct vop_blkatoff_args /* {
		struct vnode *a_vp;
		off_t a_offset;
		char **a_res;
		struct buf **a_bpp;
		} */ *ap = v;
	struct lfs *fs;
	struct inode *ip;
	struct buf *bp;
	daddr_t lbn;
	int bsize, error;
	
	ip = VTOI(ap->a_vp);
	fs = ip->i_lfs;
	lbn = lblkno(fs, ap->a_offset);
	bsize = blksize(fs, ip, lbn);
	
	*ap->a_bpp = NULL;
	if ((error = bread(ap->a_vp, lbn, bsize, NOCRED, &bp)) != 0) {
		brelse(bp);
		return (error);
	}
	if (ap->a_res)
		*ap->a_res = (char *)bp->b_data + blkoff(fs, ap->a_offset);
	*ap->a_bpp = bp;
	return (0);
}

#ifdef LFS_DEBUG_MALLOC
char *lfs_res_names[LFS_NB_COUNT] = {
	"summary",
	"superblock",
	"ifile block",
	"cluster",
	"clean",
};
#endif

int lfs_res_qty[LFS_NB_COUNT] = {
	LFS_N_SUMMARIES,
	LFS_N_SBLOCKS,
	LFS_N_IBLOCKS,
	LFS_N_CLUSTERS,
	LFS_N_CLEAN,
};

void
lfs_setup_resblks(struct lfs *fs)
{
	int i, j;
	int maxbpp;

	fs->lfs_resblk = (res_t *)malloc(LFS_N_TOTAL * sizeof(res_t), M_SEGMENT,
					  M_WAITOK);
	for (i = 0; i < LFS_N_TOTAL; i++) {
		fs->lfs_resblk[i].inuse = 0;
		fs->lfs_resblk[i].p = NULL;
	}
	for (i = 0; i < LFS_RESHASH_WIDTH; i++)
		LIST_INIT(fs->lfs_reshash + i);

	/*
	 * These types of allocations can be larger than a page,
	 * so we can't use the pool subsystem for them.
	 */
	for (i = 0, j = 0; j < LFS_N_SUMMARIES; j++, i++)
		fs->lfs_resblk[i].p = malloc(fs->lfs_sumsize, M_SEGMENT,
					    M_WAITOK);
	for (j = 0; j < LFS_N_SBLOCKS; j++, i++)
		fs->lfs_resblk[i].p = malloc(LFS_SBPAD, M_SEGMENT, M_WAITOK);
	for (j = 0; j < LFS_N_IBLOCKS; j++, i++)
		fs->lfs_resblk[i].p = malloc(fs->lfs_bsize, M_SEGMENT, M_WAITOK);
	for (j = 0; j < LFS_N_CLUSTERS; j++, i++)
		fs->lfs_resblk[i].p = malloc(MAXPHYS, M_SEGMENT, M_WAITOK);
	for (j = 0; j < LFS_N_CLEAN; j++, i++)
		fs->lfs_resblk[i].p = malloc(MAXPHYS, M_SEGMENT, M_WAITOK);

	/*
	 * Initialize pools for small types (XXX is BPP small?)
	 */
	maxbpp = ((fs->lfs_sumsize - SEGSUM_SIZE(fs)) / sizeof(int32_t) + 2);
	maxbpp = MIN(maxbpp, fs->lfs_ssize / fs->lfs_fsize + 2);
	pool_init(&fs->lfs_bpppool, maxbpp * sizeof(struct buf *), 0, 0,
		LFS_N_BPP, "lfsbpppl", &pool_allocator_nointr);
	pool_init(&fs->lfs_clpool, sizeof(struct lfs_cluster), 0, 0,
		LFS_N_CL, "lfsclpl", &pool_allocator_nointr);
	pool_init(&fs->lfs_segpool, sizeof(struct segment), 0, 0,
		LFS_N_SEG, "lfssegpool", &pool_allocator_nointr);
}

void
lfs_free_resblks(struct lfs *fs)
{
	int i;

	pool_destroy(&fs->lfs_bpppool);
	pool_destroy(&fs->lfs_segpool);
	pool_destroy(&fs->lfs_clpool);

	for (i = 0; i < LFS_N_TOTAL; i++) {
		while(fs->lfs_resblk[i].inuse)
			tsleep(&fs->lfs_resblk, PRIBIO + 1, "lfs_free", 0);
		if (fs->lfs_resblk[i].p != NULL)
			free(fs->lfs_resblk[i].p, M_SEGMENT);
	}
	free(fs->lfs_resblk, M_SEGMENT);
}

static unsigned int
lfs_mhash(void *vp)
{
	return (unsigned int)(((unsigned long)vp) >> 2) % LFS_RESHASH_WIDTH;
}

/*
 * Return memory of the given size for the given purpose, or use one of a
 * number of spare last-resort buffers, if malloc returns NULL.
 */ 
void *
lfs_malloc(struct lfs *fs, size_t size, int type)
{
	struct lfs_res_blk *re;
	void *r;
	int i, s, start;
	unsigned int h;

	/* If no mem allocated for this type, it just waits */
	if (lfs_res_qty[type] == 0)
		return malloc(size, M_SEGMENT, M_WAITOK);

	/* Otherwise try a quick malloc, and if it works, great */
	if ((r = malloc(size, M_SEGMENT, M_NOWAIT)) != NULL)
		return r;

	/*
	 * If malloc returned NULL, we are forced to use one of our
	 * reserve blocks.  We have on hand at least one summary block,
	 * at least one cluster block, at least one superblock,
	 * and several indirect blocks.
	 */
	/* skip over blocks of other types */
	for (i = 0, start = 0; i < type; i++)
		start += lfs_res_qty[i];
	while (r == NULL) {
		for (i = 0; i < lfs_res_qty[type]; i++) {
			if (fs->lfs_resblk[start + i].inuse == 0) {
				re = fs->lfs_resblk + start + i;
				re->inuse = 1;
				r = re->p;
				h = lfs_mhash(r);
				s = splbio();
				LIST_INSERT_HEAD(&fs->lfs_reshash[h], re, res);
				splx(s);
				return r;
			}
		}
#ifdef LFS_DEBUG_MALLOC
		printf("sleeping on %s (%d)\n", lfs_res_names[type], lfs_res_qty[type]);
#endif
		tsleep(&fs->lfs_resblk, PVM, "lfs_malloc", 0);
#ifdef LFS_DEBUG_MALLOC
		printf("done sleeping on %s\n", lfs_res_names[type]);
#endif
	}
	/* NOTREACHED */
	return r;
}

void
lfs_free(struct lfs *fs, void *p, int type)
{
	int s;
	unsigned int h;
	res_t *re;
#ifdef DEBUG
	int i;
#endif

	h = lfs_mhash(p);
	s = splbio();
	LIST_FOREACH(re, &fs->lfs_reshash[h], res) {
		if (re->p == p) {
			KASSERT(re->inuse == 1);
			LIST_REMOVE(re, res);
			re->inuse = 0;
			wakeup(&fs->lfs_resblk);
			splx(s);
			return;
		}
	}
#ifdef DEBUG
	for (i = 0; i < LFS_N_TOTAL; i++) {
		if (fs->lfs_resblk[i].p == p)
			panic("lfs_free: inconsist reserved block");
	}
#endif
	splx(s);

	/*
	 * If we didn't find it, free it.
	 */
	free(p, M_SEGMENT);
}

/*
 * lfs_seglock --
 *	Single thread the segment writer.
 */
int
lfs_seglock(struct lfs *fs, unsigned long flags)
{
	struct segment *sp;
	
	if (fs->lfs_seglock) {
		if (fs->lfs_lockpid == curproc->p_pid) {
			++fs->lfs_seglock;
			fs->lfs_sp->seg_flags |= flags;
			return 0;
		} else if (flags & SEGM_PAGEDAEMON)
			return EWOULDBLOCK;
		else while (fs->lfs_seglock)
			(void)tsleep(&fs->lfs_seglock, PRIBIO + 1,
				     "lfs seglock", 0);
	}
	
	fs->lfs_seglock = 1;
	fs->lfs_lockpid = curproc->p_pid;
	
	/* Drain fragment size changes out */
	lockmgr(&fs->lfs_fraglock, LK_EXCLUSIVE, 0);

	sp = fs->lfs_sp = pool_get(&fs->lfs_segpool, PR_WAITOK);
	sp->bpp = pool_get(&fs->lfs_bpppool, PR_WAITOK);
	sp->seg_flags = flags;
	sp->vp = NULL;
	sp->seg_iocount = 0;
	(void) lfs_initseg(fs);
	
	/*
	 * Keep a cumulative count of the outstanding I/O operations.  If the
	 * disk drive catches up with us it could go to zero before we finish,
	 * so we artificially increment it by one until we've scheduled all of
	 * the writes we intend to do.
	 */
	++fs->lfs_iocount;
	return 0;
}

static void lfs_unmark_dirop(struct lfs *);

static void
lfs_unmark_dirop(struct lfs *fs)
{
	struct inode *ip, *nip;
	struct vnode *vp;
	extern int lfs_dirvcount;

	for (ip = TAILQ_FIRST(&fs->lfs_dchainhd); ip != NULL; ip = nip) {
		nip = TAILQ_NEXT(ip, i_lfs_dchain);
		vp = ITOV(ip);

		if (VOP_ISLOCKED(vp) &&
			   vp->v_lock.lk_lockholder != curproc->p_pid) {
			continue;
		}
		if ((VTOI(vp)->i_flag & IN_ADIROP) == 0) {
			--lfs_dirvcount;
			vp->v_flag &= ~VDIROP;
			TAILQ_REMOVE(&fs->lfs_dchainhd, ip, i_lfs_dchain);
			wakeup(&lfs_dirvcount);
			fs->lfs_unlockvp = vp;
			vrele(vp);
			fs->lfs_unlockvp = NULL;
		}
	}
}

#ifndef LFS_NO_AUTO_SEGCLEAN
static void
lfs_auto_segclean(struct lfs *fs)
{
	int i, error;

	/*
	 * Now that we've swapped lfs_activesb, but while we still
	 * hold the segment lock, run through the segment list marking
	 * the empty ones clean.
	 * XXX - do we really need to do them all at once?
	 */
	for (i = 0; i < fs->lfs_nseg; i++) {
		if ((fs->lfs_suflags[0][i] &
		     (SEGUSE_ACTIVE | SEGUSE_DIRTY | SEGUSE_EMPTY)) ==
		    (SEGUSE_DIRTY | SEGUSE_EMPTY) &&
		    (fs->lfs_suflags[1][i] &
		     (SEGUSE_ACTIVE | SEGUSE_DIRTY | SEGUSE_EMPTY)) ==
		    (SEGUSE_DIRTY | SEGUSE_EMPTY)) {

			if ((error = lfs_do_segclean(fs, i)) != 0) {
#ifdef DEBUG
				printf("lfs_auto_segclean: lfs_do_segclean returned %d for seg %d\n", error, i);
#endif /* DEBUG */
			}
		}
		fs->lfs_suflags[1 - fs->lfs_activesb][i] =
			fs->lfs_suflags[fs->lfs_activesb][i];
	}
}
#endif /* LFS_AUTO_SEGCLEAN */

/*
 * lfs_segunlock --
 *	Single thread the segment writer.
 */
void
lfs_segunlock(struct lfs *fs)
{
	struct segment *sp;
	unsigned long sync, ckp;
	struct buf *bp;
#ifdef LFS_MALLOC_SUMMARY
	extern int locked_queue_count;
	extern long locked_queue_bytes;
#endif
	
	sp = fs->lfs_sp;

	if (fs->lfs_seglock == 1) {
		if ((sp->seg_flags & SEGM_PROT) == 0)
			lfs_unmark_dirop(fs);
		sync = sp->seg_flags & SEGM_SYNC;
		ckp = sp->seg_flags & SEGM_CKP;
		if (sp->bpp != sp->cbpp) {
			/* Free allocated segment summary */
			fs->lfs_offset -= btofsb(fs, fs->lfs_sumsize);
			bp = *sp->bpp;
#ifdef LFS_MALLOC_SUMMARY
			lfs_freebuf(fs, bp);
#else
			s = splbio();
			bremfree(bp);
			bp->b_flags |= B_DONE|B_INVAL;
			bp->b_flags &= ~B_DELWRI;
			reassignbuf(bp,bp->b_vp);
			splx(s);
			brelse(bp);
#endif
		} else
			printf ("unlock to 0 with no summary");

		pool_put(&fs->lfs_bpppool, sp->bpp);
		sp->bpp = NULL;
		/* The sync case holds a reference in `sp' to be freed below */
		if (!sync)
			pool_put(&fs->lfs_segpool, sp);
		fs->lfs_sp = NULL;

		/*
		 * If the I/O count is non-zero, sleep until it reaches zero.
		 * At the moment, the user's process hangs around so we can
		 * sleep.
		 */
		if (--fs->lfs_iocount == 0) {
			lfs_countlocked(&locked_queue_count,
					&locked_queue_bytes, "lfs_segunlock");
			wakeup(&locked_queue_count);
			wakeup(&fs->lfs_iocount);
		}
		/*
		 * If we're not checkpointing, we don't have to block
		 * other processes to wait for a synchronous write
		 * to complete.
		 */
		if (!ckp) {
			--fs->lfs_seglock;
			fs->lfs_lockpid = 0;
			wakeup(&fs->lfs_seglock);
		}
		/*
		 * We let checkpoints happen asynchronously.  That means
		 * that during recovery, we have to roll forward between
		 * the two segments described by the first and second
		 * superblocks to make sure that the checkpoint described
		 * by a superblock completed.
		 */
		while (ckp && sync && fs->lfs_iocount)
			(void)tsleep(&fs->lfs_iocount, PRIBIO + 1,
				     "lfs_iocount", 0);
		while (sync && sp->seg_iocount) {
			(void)tsleep(&sp->seg_iocount, PRIBIO + 1,
				     "seg_iocount", 0);
			/* printf("sleeping on iocount %x == %d\n", sp, sp->seg_iocount); */
		}
		if (sync)
			pool_put(&fs->lfs_segpool, sp);
		if (ckp) {
			fs->lfs_nactive = 0;
			/* If we *know* everything's on disk, write both sbs */
			/* XXX should wait for this one	 */
			if (sync)
				lfs_writesuper(fs, fs->lfs_sboffs[fs->lfs_activesb]);
			lfs_writesuper(fs, fs->lfs_sboffs[1 - fs->lfs_activesb]);
#ifndef LFS_NO_AUTO_SEGCLEAN
			lfs_auto_segclean(fs);
#endif
			fs->lfs_activesb = 1 - fs->lfs_activesb;
			--fs->lfs_seglock;
			fs->lfs_lockpid = 0;
			wakeup(&fs->lfs_seglock);
		}
		/* Reenable fragment size changes */
		lockmgr(&fs->lfs_fraglock, LK_RELEASE, 0);
	} else if (fs->lfs_seglock == 0) {
		panic ("Seglock not held");
	} else {
		--fs->lfs_seglock;
	}
}
