/*	$NetBSD: lfs_segment.c,v 1.118 2003/04/01 14:58:43 yamt Exp $	*/

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
 *	@(#)lfs_segment.c	8.10 (Berkeley) 6/10/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lfs_segment.c,v 1.118 2003/04/01 14:58:43 yamt Exp $");

#define ivndebug(vp,str) printf("ino %d: %s\n",VTOI(vp)->i_number,(str))

#if defined(_KERNEL_OPT)
#include "opt_ddb.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/kernel.h>
#include <sys/resourcevar.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/mount.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/fifofs/fifo.h>

#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>

MALLOC_DEFINE(M_SEGMENT, "LFS segment", "Segment for LFS");

extern int count_lock_queue(void);
extern struct simplelock vnode_free_list_slock;		/* XXX */

static void lfs_generic_callback(struct buf *, void (*)(struct buf *));
static void lfs_super_aiodone(struct buf *);
static void lfs_cluster_aiodone(struct buf *);
static void lfs_cluster_callback(struct buf *);

/*
 * Determine if it's OK to start a partial in this segment, or if we need
 * to go on to a new segment.
 */
#define	LFS_PARTIAL_FITS(fs) \
	((fs)->lfs_fsbpseg - ((fs)->lfs_offset - (fs)->lfs_curseg) > \
	fragstofsb((fs), (fs)->lfs_frag))

void	 lfs_callback(struct buf *);
int	 lfs_gather(struct lfs *, struct segment *,
	     struct vnode *, int (*)(struct lfs *, struct buf *));
int	 lfs_gatherblock(struct segment *, struct buf *, int *);
void	 lfs_iset(struct inode *, daddr_t, time_t);
int	 lfs_match_fake(struct lfs *, struct buf *);
int	 lfs_match_data(struct lfs *, struct buf *);
int	 lfs_match_dindir(struct lfs *, struct buf *);
int	 lfs_match_indir(struct lfs *, struct buf *);
int	 lfs_match_tindir(struct lfs *, struct buf *);
void	 lfs_newseg(struct lfs *);
/* XXX ondisk32 */
void	 lfs_shellsort(struct buf **, int32_t *, int, int);
void	 lfs_supercallback(struct buf *);
void	 lfs_updatemeta(struct segment *);
int	 lfs_vref(struct vnode *);
void	 lfs_vunref(struct vnode *);
void	 lfs_writefile(struct lfs *, struct segment *, struct vnode *);
int	 lfs_writeinode(struct lfs *, struct segment *, struct inode *);
int	 lfs_writeseg(struct lfs *, struct segment *);
void	 lfs_writesuper(struct lfs *, daddr_t);
int	 lfs_writevnodes(struct lfs *fs, struct mount *mp,
	    struct segment *sp, int dirops);

int	lfs_allclean_wakeup;		/* Cleaner wakeup address. */
int	lfs_writeindir = 1;		/* whether to flush indir on non-ckp */
int	lfs_clean_vnhead = 0;		/* Allow freeing to head of vn list */
int	lfs_dirvcount = 0;		/* # active dirops */

/* Statistics Counters */
int lfs_dostats = 1;
struct lfs_stats lfs_stats;

extern int locked_queue_count;
extern long locked_queue_bytes;

/* op values to lfs_writevnodes */
#define	VN_REG		0
#define	VN_DIROP	1
#define	VN_EMPTY	2
#define VN_CLEAN	3

#define LFS_MAX_ACTIVE		10

/*
 * XXX KS - Set modification time on the Ifile, so the cleaner can
 * read the fs mod time off of it.  We don't set IN_UPDATE here,
 * since we don't really need this to be flushed to disk (and in any
 * case that wouldn't happen to the Ifile until we checkpoint).
 */
void
lfs_imtime(struct lfs *fs)
{
	struct timespec ts;
	struct inode *ip;
	
	TIMEVAL_TO_TIMESPEC(&time, &ts);
	ip = VTOI(fs->lfs_ivnode);
	ip->i_ffs_mtime = ts.tv_sec;
	ip->i_ffs_mtimensec = ts.tv_nsec;
}

/*
 * Ifile and meta data blocks are not marked busy, so segment writes MUST be
 * single threaded.  Currently, there are two paths into lfs_segwrite, sync()
 * and getnewbuf().  They both mark the file system busy.  Lfs_vflush()
 * explicitly marks the file system busy.  So lfs_segwrite is safe.  I think.
 */

#define SET_FLUSHING(fs,vp) (fs)->lfs_flushvp = (vp)
#define IS_FLUSHING(fs,vp)  ((fs)->lfs_flushvp == (vp))
#define CLR_FLUSHING(fs,vp) (fs)->lfs_flushvp = NULL

int
lfs_vflush(struct vnode *vp)
{
	struct inode *ip;
	struct lfs *fs;
	struct segment *sp;
	struct buf *bp, *nbp, *tbp, *tnbp;
	int error, s;
	int flushed;
#if 0
	int redo;
#endif

	ip = VTOI(vp);
	fs = VFSTOUFS(vp->v_mount)->um_lfs;

	if (ip->i_flag & IN_CLEANING) {
#ifdef DEBUG_LFS
		ivndebug(vp,"vflush/in_cleaning");
#endif
		LFS_CLR_UINO(ip, IN_CLEANING);
		LFS_SET_UINO(ip, IN_MODIFIED);

		/*
		 * Toss any cleaning buffers that have real counterparts
		 * to avoid losing new data.
		 */
		s = splbio();
		for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
			nbp = LIST_NEXT(bp, b_vnbufs);
			if (!LFS_IS_MALLOC_BUF(bp))
				continue;
			/*
			 * Look for pages matching the range covered
			 * by cleaning blocks.  It's okay if more dirty
			 * pages appear, so long as none disappear out
			 * from under us.
			 */
			if (bp->b_lblkno > 0 && vp->v_type == VREG &&
			    vp != fs->lfs_ivnode) {
				struct vm_page *pg;
				voff_t off;

				simple_lock(&vp->v_interlock);
				for (off = lblktosize(fs, bp->b_lblkno);
				     off < lblktosize(fs, bp->b_lblkno + 1);
				     off += PAGE_SIZE) {
					pg = uvm_pagelookup(&vp->v_uobj, off);
					if (pg == NULL)
						continue;
					if ((pg->flags & PG_CLEAN) == 0 ||
					    pmap_is_modified(pg)) {
						fs->lfs_avail += btofsb(fs,
							bp->b_bcount);
						wakeup(&fs->lfs_avail);
						lfs_freebuf(fs, bp);
						bp = NULL;
						goto nextbp;
					}
				}
				simple_unlock(&vp->v_interlock);
			}
			for (tbp = LIST_FIRST(&vp->v_dirtyblkhd); tbp;
			    tbp = tnbp)
			{
				tnbp = LIST_NEXT(tbp, b_vnbufs);
				if (tbp->b_vp == bp->b_vp
				   && tbp->b_lblkno == bp->b_lblkno
				   && tbp != bp)
				{
					fs->lfs_avail += btofsb(fs,
						bp->b_bcount);
					wakeup(&fs->lfs_avail);
					lfs_freebuf(fs, bp);
					bp = NULL;
					break;
				}
			}
		    nextbp:
			;
		}
		splx(s);
	}

	/* If the node is being written, wait until that is done */
	s = splbio();
	if (WRITEINPROG(vp)) {
#ifdef DEBUG_LFS
		ivndebug(vp,"vflush/writeinprog");
#endif
		tsleep(vp, PRIBIO+1, "lfs_vw", 0);
	}
	splx(s);

	/* Protect against VXLOCK deadlock in vinvalbuf() */
	lfs_seglock(fs, SEGM_SYNC);

	/* If we're supposed to flush a freed inode, just toss it */
	/* XXX - seglock, so these buffers can't be gathered, right? */
	if (ip->i_ffs_mode == 0) {
		printf("lfs_vflush: ino %d is freed, not flushing\n",
			ip->i_number);
		s = splbio();
		for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
			nbp = LIST_NEXT(bp, b_vnbufs);
			if (bp->b_flags & B_DELWRI) { /* XXX always true? */
				fs->lfs_avail += btofsb(fs, bp->b_bcount);
				wakeup(&fs->lfs_avail);
			}
			/* Copied from lfs_writeseg */
			if (bp->b_flags & B_CALL) {
				biodone(bp);
			} else {
				bremfree(bp);
				LFS_UNLOCK_BUF(bp);
				bp->b_flags &= ~(B_ERROR | B_READ | B_DELWRI |
					 B_GATHERED);
				bp->b_flags |= B_DONE;
				reassignbuf(bp, vp);
				brelse(bp);  
			}
		}
		splx(s);
		LFS_CLR_UINO(ip, IN_CLEANING);
		LFS_CLR_UINO(ip, IN_MODIFIED | IN_ACCESSED);
		ip->i_flag &= ~IN_ALLMOD;
		printf("lfs_vflush: done not flushing ino %d\n",
			ip->i_number);
		lfs_segunlock(fs);
		return 0;
	}

	SET_FLUSHING(fs,vp);
	if (fs->lfs_nactive > LFS_MAX_ACTIVE ||
	    (fs->lfs_sp->seg_flags & SEGM_CKP)) {
		error = lfs_segwrite(vp->v_mount, SEGM_CKP | SEGM_SYNC);
		CLR_FLUSHING(fs,vp);
		lfs_segunlock(fs);
		return error;
	}
	sp = fs->lfs_sp;

	flushed = 0;
	if (VPISEMPTY(vp)) {
		lfs_writevnodes(fs, vp->v_mount, sp, VN_EMPTY);
		++flushed;
	} else if ((ip->i_flag & IN_CLEANING) &&
		  (fs->lfs_sp->seg_flags & SEGM_CLEAN)) {
#ifdef DEBUG_LFS
		ivndebug(vp,"vflush/clean");
#endif
		lfs_writevnodes(fs, vp->v_mount, sp, VN_CLEAN);
		++flushed;
	} else if (lfs_dostats) {
		if (!VPISEMPTY(vp) || (VTOI(vp)->i_flag & IN_ALLMOD))
			++lfs_stats.vflush_invoked;
#ifdef DEBUG_LFS
		ivndebug(vp,"vflush");
#endif
	}

#ifdef DIAGNOSTIC
	/* XXX KS This actually can happen right now, though it shouldn't(?) */
	if (vp->v_flag & VDIROP) {
		printf("lfs_vflush: flushing VDIROP, this shouldn\'t be\n");
		/* panic("VDIROP being flushed...this can\'t happen"); */
	}
	if (vp->v_usecount < 0) {
		printf("usecount=%ld\n", (long)vp->v_usecount);
		panic("lfs_vflush: usecount<0");
	}
#endif

#if 1
	do {
		do {
			if (LIST_FIRST(&vp->v_dirtyblkhd) != NULL)
				lfs_writefile(fs, sp, vp);
		} while (lfs_writeinode(fs, sp, ip));
	} while (lfs_writeseg(fs, sp) && ip->i_number == LFS_IFILE_INUM);
#else
	if (flushed && vp != fs->lfs_ivnode)
		lfs_writeseg(fs, sp);
	else do {
		fs->lfs_flags &= ~LFS_IFDIRTY;
		lfs_writefile(fs, sp, vp);
		redo = lfs_writeinode(fs, sp, ip);
		redo += lfs_writeseg(fs, sp);
		redo += (fs->lfs_flags & LFS_IFDIRTY);
	} while (redo && vp == fs->lfs_ivnode);
#endif
	if (lfs_dostats) {
		++lfs_stats.nwrites;
		if (sp->seg_flags & SEGM_SYNC)
			++lfs_stats.nsync_writes;
		if (sp->seg_flags & SEGM_CKP)
			++lfs_stats.ncheckpoints;
	}
	/*
	 * If we were called from somewhere that has already held the seglock
	 * (e.g., lfs_markv()), the lfs_segunlock will not wait for
	 * the write to complete because we are still locked.
	 * Since lfs_vflush() must return the vnode with no dirty buffers,
	 * we must explicitly wait, if that is the case.
	 *
	 * We compare the iocount against 1, not 0, because it is
	 * artificially incremented by lfs_seglock().
	 */
	simple_lock(&fs->lfs_interlock);
	if (fs->lfs_seglock > 1) {
		simple_unlock(&fs->lfs_interlock);
		while (fs->lfs_iocount > 1)
			(void)tsleep(&fs->lfs_iocount, PRIBIO + 1,
				     "lfs_vflush", 0);
	} else
		simple_unlock(&fs->lfs_interlock);

	lfs_segunlock(fs);

	CLR_FLUSHING(fs,vp);
	return (0);
}

#ifdef DEBUG_LFS_VERBOSE
# define vndebug(vp,str) if (VTOI(vp)->i_flag & IN_CLEANING) printf("not writing ino %d because %s (op %d)\n",VTOI(vp)->i_number,(str),op)
#else
# define vndebug(vp,str)
#endif

int
lfs_writevnodes(struct lfs *fs, struct mount *mp, struct segment *sp, int op)
{
	struct inode *ip;
	struct vnode *vp, *nvp;
	int inodes_written = 0, only_cleaning;

#ifndef LFS_NO_BACKVP_HACK
	/* BEGIN HACK */
#define	VN_OFFSET	(((caddr_t)&LIST_NEXT(vp, v_mntvnodes)) - (caddr_t)vp)
#define	BACK_VP(VP)	((struct vnode *)(((caddr_t)(VP)->v_mntvnodes.le_prev) - VN_OFFSET))
#define	BEG_OF_VLIST	((struct vnode *)(((caddr_t)&(LIST_FIRST(&mp->mnt_vnodelist))) - VN_OFFSET))
	
	/* Find last vnode. */
 loop:	for (vp = LIST_FIRST(&mp->mnt_vnodelist);
	     vp && LIST_NEXT(vp, v_mntvnodes) != NULL;
	     vp = LIST_NEXT(vp, v_mntvnodes));
	for (; vp && vp != BEG_OF_VLIST; vp = nvp) {
		nvp = BACK_VP(vp);
#else
	loop:
	for (vp = LIST_FIRST(&mp->mnt_vnodelist); vp; vp = nvp) {
		nvp = LIST_NEXT(vp, v_mntvnodes);
#endif
		/*
		 * If the vnode that we are about to sync is no longer
		 * associated with this mount point, start over.
		 */
		if (vp->v_mount != mp) {
			printf("lfs_writevnodes: starting over\n");
			/*
			 * After this, pages might be busy
			 * due to our own previous putpages.
			 * Start actual segment write here to avoid deadlock.
			 */
			(void)lfs_writeseg(fs, sp);
			goto loop;
		}
		
		if (vp->v_type == VNON) {
			continue;
		}

		ip = VTOI(vp);
		if ((op == VN_DIROP && !(vp->v_flag & VDIROP)) ||
		    (op != VN_DIROP && op != VN_CLEAN && (vp->v_flag & VDIROP))) {
			vndebug(vp,"dirop");
			continue;
		}
		
		if (op == VN_EMPTY && !VPISEMPTY(vp)) {
			vndebug(vp,"empty");
			continue;
		}

		if (op == VN_CLEAN && ip->i_number != LFS_IFILE_INUM
		   && vp != fs->lfs_flushvp
		   && !(ip->i_flag & IN_CLEANING)) {
			vndebug(vp,"cleaning");
			continue;
		}

		if (lfs_vref(vp)) {
			vndebug(vp,"vref");
			continue;
		}

		only_cleaning = 0;
		/*
		 * Write the inode/file if dirty and it's not the IFILE.
		 */
		if ((ip->i_flag & IN_ALLMOD) || !VPISEMPTY(vp)) {
			only_cleaning = ((ip->i_flag & IN_ALLMOD) == IN_CLEANING);

			if (ip->i_number != LFS_IFILE_INUM)
				lfs_writefile(fs, sp, vp);
			if (!VPISEMPTY(vp)) {
				if (WRITEINPROG(vp)) {
#ifdef DEBUG_LFS
					ivndebug(vp,"writevnodes/write2");
#endif
				} else if (!(ip->i_flag & IN_ALLMOD)) {
#ifdef DEBUG_LFS
					printf("<%d>",ip->i_number);
#endif
					LFS_SET_UINO(ip, IN_MODIFIED);
				}
			}
			(void) lfs_writeinode(fs, sp, ip);
			inodes_written++;
		}

		if (lfs_clean_vnhead && only_cleaning)
			lfs_vunref_head(vp);
		else
			lfs_vunref(vp);
	}
	return inodes_written;
}

/*
 * Do a checkpoint.
 */
int
lfs_segwrite(struct mount *mp, int flags)
{
	struct buf *bp;
	struct inode *ip;
	struct lfs *fs;
	struct segment *sp;
	struct vnode *vp;
	SEGUSE *segusep;
	int do_ckp, did_ckp, error, s;
	unsigned n, segleft, maxseg, sn, i, curseg;
	int writer_set = 0;
	int dirty;
	int redo;
	
	fs = VFSTOUFS(mp)->um_lfs;

	if (fs->lfs_ronly)
		return EROFS;

	lfs_imtime(fs);

	/*
	 * Allocate a segment structure and enough space to hold pointers to
	 * the maximum possible number of buffers which can be described in a
	 * single summary block.
	 */
	do_ckp = (flags & SEGM_CKP) || fs->lfs_nactive > LFS_MAX_ACTIVE;
	lfs_seglock(fs, flags | (do_ckp ? SEGM_CKP : 0));
	sp = fs->lfs_sp;

	/*
	 * If lfs_flushvp is non-NULL, we are called from lfs_vflush,
	 * in which case we have to flush *all* buffers off of this vnode.
	 * We don't care about other nodes, but write any non-dirop nodes
	 * anyway in anticipation of another getnewvnode().
	 *
	 * If we're cleaning we only write cleaning and ifile blocks, and
	 * no dirops, since otherwise we'd risk corruption in a crash.
	 */
	if (sp->seg_flags & SEGM_CLEAN)
		lfs_writevnodes(fs, mp, sp, VN_CLEAN);
	else if (!(sp->seg_flags & SEGM_FORCE_CKP)) {
		lfs_writevnodes(fs, mp, sp, VN_REG);
		if (!fs->lfs_dirops || !fs->lfs_flushvp) {
			while (fs->lfs_dirops)
				if ((error = tsleep(&fs->lfs_writer, PRIBIO + 1,
						"lfs writer", 0)))
				{
					printf("segwrite mysterious error\n");
					/* XXX why not segunlock? */
					pool_put(&fs->lfs_bpppool, sp->bpp);
					sp->bpp = NULL;
					pool_put(&fs->lfs_segpool, sp);
					sp = fs->lfs_sp = NULL;
					return (error);
				}
			fs->lfs_writer++;
			writer_set = 1;
			lfs_writevnodes(fs, mp, sp, VN_DIROP);
			((SEGSUM *)(sp->segsum))->ss_flags &= ~(SS_CONT);
		}
	}	

	/*
	 * If we are doing a checkpoint, mark everything since the
	 * last checkpoint as no longer ACTIVE.
	 */
	if (do_ckp) {
		segleft = fs->lfs_nseg;
		curseg = 0;
		for (n = 0; n < fs->lfs_segtabsz; n++) {
			dirty = 0;
			if (bread(fs->lfs_ivnode, 
			    fs->lfs_cleansz + n, fs->lfs_bsize, NOCRED, &bp))

				panic("lfs_segwrite: ifile read");
			segusep = (SEGUSE *)bp->b_data;
			maxseg = min(segleft, fs->lfs_sepb);
			for (i = 0; i < maxseg; i++) {
				sn = curseg + i;
				if (sn != fs->lfs_curseg &&
				    segusep->su_flags & SEGUSE_ACTIVE) {
					segusep->su_flags &= ~SEGUSE_ACTIVE;
					--fs->lfs_nactive;
					++dirty;
				}
				fs->lfs_suflags[fs->lfs_activesb][sn] =
					segusep->su_flags;
				if (fs->lfs_version > 1)
					++segusep;
				else
					segusep = (SEGUSE *)
						((SEGUSE_V1 *)segusep + 1);
			}
				
			if (dirty)
				error = LFS_BWRITE_LOG(bp); /* Ifile */
			else
				brelse(bp);
			segleft -= fs->lfs_sepb;
			curseg += fs->lfs_sepb;
		}
	}

	did_ckp = 0;
	if (do_ckp || fs->lfs_doifile) {
		do {
			vp = fs->lfs_ivnode;

#ifdef DEBUG
			LFS_ENTER_LOG("pretend", __FILE__, __LINE__, 0, 0);
#endif
			fs->lfs_flags &= ~LFS_IFDIRTY;

			ip = VTOI(vp);

			if (LIST_FIRST(&vp->v_dirtyblkhd) != NULL)
				lfs_writefile(fs, sp, vp);

			if (ip->i_flag & IN_ALLMOD)
				++did_ckp;
			redo = lfs_writeinode(fs, sp, ip);
			redo += lfs_writeseg(fs, sp);
			redo += (fs->lfs_flags & LFS_IFDIRTY);
		} while (redo && do_ckp);

		/*
		 * Unless we are unmounting, the Ifile may continue to have
		 * dirty blocks even after a checkpoint, due to changes to
		 * inodes' atime.  If we're checkpointing, it's "impossible"
		 * for other parts of the Ifile to be dirty after the loop
		 * above, since we hold the segment lock.
		 */
		s = splbio();
		if (LIST_EMPTY(&vp->v_dirtyblkhd)) {
			LFS_CLR_UINO(ip, IN_ALLMOD);
		}
#ifdef DIAGNOSTIC
		else if (do_ckp) {
			LIST_FOREACH(bp, &vp->v_dirtyblkhd, b_vnbufs) {
				if (bp->b_lblkno < fs->lfs_cleansz +
				    fs->lfs_segtabsz &&
				    !(bp->b_flags & B_GATHERED)) {
					panic("dirty blocks");
				}
			}
		}
#endif
		splx(s);
	} else {
		(void) lfs_writeseg(fs, sp);
	}
	
	/* Note Ifile no longer needs to be written */
	fs->lfs_doifile = 0;
	if (writer_set && --fs->lfs_writer == 0)
		wakeup(&fs->lfs_dirops);

	/*
	 * If we didn't write the Ifile, we didn't really do anything.
	 * That means that (1) there is a checkpoint on disk and (2)
	 * nothing has changed since it was written.
	 *
	 * Take the flags off of the segment so that lfs_segunlock
	 * doesn't have to write the superblock either.
	 */
	if (do_ckp && !did_ckp) {
		sp->seg_flags &= ~SEGM_CKP;
	}

	if (lfs_dostats) {
		++lfs_stats.nwrites;
		if (sp->seg_flags & SEGM_SYNC)
			++lfs_stats.nsync_writes;
		if (sp->seg_flags & SEGM_CKP)
			++lfs_stats.ncheckpoints;
	}
	lfs_segunlock(fs);
	return (0);
}

/*
 * Write the dirty blocks associated with a vnode.
 */
void
lfs_writefile(struct lfs *fs, struct segment *sp, struct vnode *vp)
{
	struct buf *bp;
	struct finfo *fip;
	struct inode *ip;
	IFILE *ifp;
	int i, frag;
	
	ip = VTOI(vp);

	if (sp->seg_bytes_left < fs->lfs_bsize ||
	    sp->sum_bytes_left < sizeof(struct finfo))
		(void) lfs_writeseg(fs, sp);
	
	sp->sum_bytes_left -= FINFOSIZE;
	++((SEGSUM *)(sp->segsum))->ss_nfinfo;

	if (vp->v_flag & VDIROP)
		((SEGSUM *)(sp->segsum))->ss_flags |= (SS_DIROP|SS_CONT);
	
	fip = sp->fip;
	fip->fi_nblocks = 0;
	fip->fi_ino = ip->i_number;
	LFS_IENTRY(ifp, fs, fip->fi_ino, bp);
	fip->fi_version = ifp->if_version;
	brelse(bp);
	
	if (sp->seg_flags & SEGM_CLEAN) {
		lfs_gather(fs, sp, vp, lfs_match_fake);
		/*
		 * For a file being flushed, we need to write *all* blocks.
		 * This means writing the cleaning blocks first, and then
		 * immediately following with any non-cleaning blocks.
		 * The same is true of the Ifile since checkpoints assume
		 * that all valid Ifile blocks are written.
		 */
		if (IS_FLUSHING(fs,vp) || vp == fs->lfs_ivnode) {
			lfs_gather(fs, sp, vp, lfs_match_data);
			/*
			 * Don't call VOP_PUTPAGES: if we're flushing,
			 * we've already done it, and the Ifile doesn't
			 * use the page cache.
			 */
		}
	} else {
		lfs_gather(fs, sp, vp, lfs_match_data);
		/*
		 * If we're flushing, we've already called VOP_PUTPAGES
		 * so don't do it again.  Otherwise, we want to write
		 * everything we've got.
		 */
		if (!IS_FLUSHING(fs, vp)) {
			simple_lock(&vp->v_interlock);
			VOP_PUTPAGES(vp, 0, 0,
				     PGO_CLEANIT | PGO_ALLPAGES | PGO_LOCKED |
				     PGO_BUSYFAIL);
		}
	}

	/*
	 * It may not be necessary to write the meta-data blocks at this point,
	 * as the roll-forward recovery code should be able to reconstruct the
	 * list.
	 *
	 * We have to write them anyway, though, under two conditions: (1) the
	 * vnode is being flushed (for reuse by vinvalbuf); or (2) we are
	 * checkpointing.
	 *
	 * BUT if we are cleaning, we might have indirect blocks that refer to
	 * new blocks not being written yet, in addition to fragments being
	 * moved out of a cleaned segment.  If that is the case, don't
	 * write the indirect blocks, or the finfo will have a small block
	 * in the middle of it!
	 * XXX in this case isn't the inode size wrong too?
	 */
	frag = 0;
	if (sp->seg_flags & SEGM_CLEAN) {
		for (i = 0; i < NDADDR; i++)
			if (ip->i_lfs_fragsize[i] > 0 &&
			    ip->i_lfs_fragsize[i] < fs->lfs_bsize)
				++frag;
	}
#ifdef DIAGNOSTIC
	if (frag > 1)
		panic("lfs_writefile: more than one fragment!");
#endif
	if (IS_FLUSHING(fs, vp) ||
	    (frag == 0 && (lfs_writeindir || (sp->seg_flags & SEGM_CKP)))) {
		lfs_gather(fs, sp, vp, lfs_match_indir);
		lfs_gather(fs, sp, vp, lfs_match_dindir);
		lfs_gather(fs, sp, vp, lfs_match_tindir);
	}
	fip = sp->fip;
	if (fip->fi_nblocks != 0) {
		sp->fip = (FINFO*)((caddr_t)fip + FINFOSIZE +
				   sizeof(int32_t) * (fip->fi_nblocks));
		sp->start_lbp = &sp->fip->fi_blocks[0];
	} else {
		sp->sum_bytes_left += FINFOSIZE;
		--((SEGSUM *)(sp->segsum))->ss_nfinfo;
	}
}

int
lfs_writeinode(struct lfs *fs, struct segment *sp, struct inode *ip)
{
	struct buf *bp, *ibp;
	struct dinode *cdp;
	IFILE *ifp;
	SEGUSE *sup;
	daddr_t daddr;
	int32_t *daddrp;	/* XXX ondisk32 */
	ino_t ino;
	int error, i, ndx, fsb = 0;
	int redo_ifile = 0;
	struct timespec ts;
	int gotblk = 0;
	
	if (!(ip->i_flag & IN_ALLMOD))
		return (0);
	
	/* Allocate a new inode block if necessary. */
	if ((ip->i_number != LFS_IFILE_INUM || sp->idp == NULL) && sp->ibp == NULL) {
		/* Allocate a new segment if necessary. */
		if (sp->seg_bytes_left < fs->lfs_ibsize ||
		    sp->sum_bytes_left < sizeof(int32_t))
			(void) lfs_writeseg(fs, sp);

		/* Get next inode block. */
		daddr = fs->lfs_offset;
		fs->lfs_offset += btofsb(fs, fs->lfs_ibsize);
		sp->ibp = *sp->cbpp++ =
			getblk(VTOI(fs->lfs_ivnode)->i_devvp, fsbtodb(fs, daddr),
			       fs->lfs_ibsize, 0, 0);
		gotblk++;

		/* Zero out inode numbers */
		for (i = 0; i < INOPB(fs); ++i)
			((struct dinode *)sp->ibp->b_data)[i].di_inumber = 0;

		++sp->start_bpp;
		fs->lfs_avail -= btofsb(fs, fs->lfs_ibsize);
		/* Set remaining space counters. */
		sp->seg_bytes_left -= fs->lfs_ibsize;
		sp->sum_bytes_left -= sizeof(int32_t);
		ndx = fs->lfs_sumsize / sizeof(int32_t) -
			sp->ninodes / INOPB(fs) - 1;
		((int32_t *)(sp->segsum))[ndx] = daddr;
	}

	/* Update the inode times and copy the inode onto the inode page. */
	TIMEVAL_TO_TIMESPEC(&time, &ts);
	/* XXX kludge --- don't redirty the ifile just to put times on it */
	if (ip->i_number != LFS_IFILE_INUM)
		LFS_ITIMES(ip, &ts, &ts, &ts);

	/*
	 * If this is the Ifile, and we've already written the Ifile in this
	 * partial segment, just overwrite it (it's not on disk yet) and
	 * continue.
	 *
	 * XXX we know that the bp that we get the second time around has
	 * already been gathered.
	 */
	if (ip->i_number == LFS_IFILE_INUM && sp->idp) {
		*(sp->idp) = ip->i_din.ffs_din;
		ip->i_lfs_osize = ip->i_ffs_size;
		return 0;
	}

	bp = sp->ibp;
	cdp = ((struct dinode *)bp->b_data) + (sp->ninodes % INOPB(fs));
	*cdp = ip->i_din.ffs_din;
#ifdef LFS_IFILE_FRAG_ADDRESSING
	if (fs->lfs_version > 1)
		fsb = (sp->ninodes % INOPB(fs)) / INOPF(fs);
#endif

	/*
	 * If we are cleaning, ensure that we don't write UNWRITTEN disk
	 * addresses to disk; possibly revert the inode size.
	 * XXX By not writing these blocks, we are making the lfs_avail
	 * XXX count on disk wrong by the same amount.	We should be
	 * XXX able to "borrow" from lfs_avail and return it after the
	 * XXX Ifile is written.  See also in lfs_writeseg.
	 */
	if (ip->i_lfs_effnblks != ip->i_ffs_blocks) {
		cdp->di_size = ip->i_lfs_osize;
#ifdef DEBUG_LFS
		printf("lfs_writeinode: cleansing ino %d (%d != %d)\n",
		       ip->i_number, ip->i_lfs_effnblks, ip->i_ffs_blocks);
#endif
		for (daddrp = cdp->di_db; daddrp < cdp->di_ib + NIADDR;
		     daddrp++) {
			if (*daddrp == UNWRITTEN) {
#ifdef DEBUG_LFS
				printf("lfs_writeinode: wiping UNWRITTEN\n");
#endif
				*daddrp = 0;
			}
		}
	} else {
		/* If all blocks are goig to disk, update the "size on disk" */
		ip->i_lfs_osize = ip->i_ffs_size;
	}
	
	if (ip->i_flag & IN_CLEANING)
		LFS_CLR_UINO(ip, IN_CLEANING);
	else {
		/* XXX IN_ALLMOD */
		LFS_CLR_UINO(ip, IN_ACCESSED | IN_ACCESS | IN_CHANGE |
			     IN_UPDATE);
		if (ip->i_lfs_effnblks == ip->i_ffs_blocks)
			LFS_CLR_UINO(ip, IN_MODIFIED);
#ifdef DEBUG_LFS
		else
			printf("lfs_writeinode: ino %d: real blks=%d, "
			       "eff=%d\n", ip->i_number, ip->i_ffs_blocks,
			       ip->i_lfs_effnblks);
#endif
	}

	if (ip->i_number == LFS_IFILE_INUM) /* We know sp->idp == NULL */
		sp->idp = ((struct dinode *)bp->b_data) + 
			(sp->ninodes % INOPB(fs));
	if (gotblk) {
		LFS_LOCK_BUF(bp);
		brelse(bp);
	}
	
	/* Increment inode count in segment summary block. */
	++((SEGSUM *)(sp->segsum))->ss_ninos;
	
	/* If this page is full, set flag to allocate a new page. */
	if (++sp->ninodes % INOPB(fs) == 0)
		sp->ibp = NULL;
	
	/*
	 * If updating the ifile, update the super-block.  Update the disk
	 * address and access times for this inode in the ifile.
	 */
	ino = ip->i_number;
	if (ino == LFS_IFILE_INUM) {
		daddr = fs->lfs_idaddr;
		fs->lfs_idaddr = dbtofsb(fs, bp->b_blkno);
	} else {
		LFS_IENTRY(ifp, fs, ino, ibp);
		daddr = ifp->if_daddr;
		ifp->if_daddr = dbtofsb(fs, bp->b_blkno) + fsb;
#ifdef LFS_DEBUG_NEXTFREE
		if (ino > 3 && ifp->if_nextfree) {
			vprint("lfs_writeinode",ITOV(ip));
			printf("lfs_writeinode: updating free ino %d\n",
				ip->i_number);
		}
#endif
		error = LFS_BWRITE_LOG(ibp); /* Ifile */
	}
	
	/*
	 * The inode's last address should not be in the current partial
	 * segment, except under exceptional circumstances (lfs_writevnodes
	 * had to start over, and in the meantime more blocks were written
	 * to a vnode).	 Both inodes will be accounted to this segment
	 * in lfs_writeseg so we need to subtract the earlier version
	 * here anyway.	 The segment count can temporarily dip below
	 * zero here; keep track of how many duplicates we have in 
	 * "dupino" so we don't panic below.
	 */
	if (daddr >= fs->lfs_lastpseg && daddr <= dbtofsb(fs, bp->b_blkno)) {
		++sp->ndupino;
		printf("lfs_writeinode: last inode addr in current pseg "
		       "(ino %d daddr 0x%llx) ndupino=%d\n", ino,
			(long long)daddr, sp->ndupino);
	}
	/*
	 * Account the inode: it no longer belongs to its former segment,
	 * though it will not belong to the new segment until that segment
	 * is actually written.
	 */
	if (daddr != LFS_UNUSED_DADDR) {
		u_int32_t oldsn = dtosn(fs, daddr);
#ifdef DIAGNOSTIC
		int ndupino = (sp->seg_number == oldsn) ? sp->ndupino : 0;
#endif
		LFS_SEGENTRY(sup, fs, oldsn, bp);
#ifdef DIAGNOSTIC
		if (sup->su_nbytes + DINODE_SIZE * ndupino < DINODE_SIZE) {
			printf("lfs_writeinode: negative bytes "
			       "(segment %" PRIu32 " short by %d, "
			       "oldsn=%" PRIu32 ", cursn=%" PRIu32
			       ", daddr=%" PRId64 ", su_nbytes=%u, "
			       "ndupino=%d)\n",
			       dtosn(fs, daddr),
			       (int)DINODE_SIZE * (1 - sp->ndupino)
				   - sup->su_nbytes,
			       oldsn, sp->seg_number, daddr,
			       (unsigned int)sup->su_nbytes,
			       sp->ndupino);
			panic("lfs_writeinode: negative bytes");
			sup->su_nbytes = DINODE_SIZE;
		}
#endif
#ifdef DEBUG_SU_NBYTES
		printf("seg %d -= %d for ino %d inode\n", 
		       dtosn(fs, daddr), DINODE_SIZE, ino);
#endif
		sup->su_nbytes -= DINODE_SIZE;
		redo_ifile =
			(ino == LFS_IFILE_INUM && !(bp->b_flags & B_GATHERED));
		if (redo_ifile)
			fs->lfs_flags |= LFS_IFDIRTY;
		LFS_WRITESEGENTRY(sup, fs, oldsn, bp); /* Ifile */
	}
	return (redo_ifile);
}

int
lfs_gatherblock(struct segment *sp, struct buf *bp, int *sptr)
{
	struct lfs *fs;
	int version;
	int j, blksinblk;

	/*
	 * If full, finish this segment.  We may be doing I/O, so
	 * release and reacquire the splbio().
	 */
#ifdef DIAGNOSTIC
	if (sp->vp == NULL)
		panic ("lfs_gatherblock: Null vp in segment");
#endif
	fs = sp->fs;
	blksinblk = howmany(bp->b_bcount, fs->lfs_bsize);
	if (sp->sum_bytes_left < sizeof(int32_t) * blksinblk ||
	    sp->seg_bytes_left < bp->b_bcount) {
		if (sptr)
			splx(*sptr);
		lfs_updatemeta(sp);
		
		version = sp->fip->fi_version;
		(void) lfs_writeseg(fs, sp);
		
		sp->fip->fi_version = version;
		sp->fip->fi_ino = VTOI(sp->vp)->i_number;
		/* Add the current file to the segment summary. */
		++((SEGSUM *)(sp->segsum))->ss_nfinfo;
		sp->sum_bytes_left -= FINFOSIZE;
		
		if (sptr)
			*sptr = splbio();
		return (1);
	}
	
#ifdef DEBUG
	if (bp->b_flags & B_GATHERED) {
		printf("lfs_gatherblock: already gathered! Ino %d,"
		       " lbn %" PRId64 "\n",
		       sp->fip->fi_ino, bp->b_lblkno);
		return (0);
	}
#endif
	/* Insert into the buffer list, update the FINFO block. */
	bp->b_flags |= B_GATHERED;
	bp->b_flags &= ~B_DONE;

	*sp->cbpp++ = bp;
	for (j = 0; j < blksinblk; j++)
		sp->fip->fi_blocks[sp->fip->fi_nblocks++] = bp->b_lblkno + j;
	
	sp->sum_bytes_left -= sizeof(int32_t) * blksinblk;
	sp->seg_bytes_left -= bp->b_bcount;
	return (0);
}

int
lfs_gather(struct lfs *fs, struct segment *sp, struct vnode *vp, int (*match)(struct lfs *, struct buf *))
{
	struct buf *bp, *nbp;
	int s, count = 0;
	
	sp->vp = vp;
	s = splbio();

#ifndef LFS_NO_BACKBUF_HACK
/* This is a hack to see if ordering the blocks in LFS makes a difference. */
# define	BUF_OFFSET	(((caddr_t)&LIST_NEXT(bp, b_vnbufs)) - (caddr_t)bp)
# define	BACK_BUF(BP)	((struct buf *)(((caddr_t)(BP)->b_vnbufs.le_prev) - BUF_OFFSET))
# define	BEG_OF_LIST	((struct buf *)(((caddr_t)&LIST_FIRST(&vp->v_dirtyblkhd)) - BUF_OFFSET))
/* Find last buffer. */
loop:	for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp && LIST_NEXT(bp, b_vnbufs) != NULL;
	    bp = LIST_NEXT(bp, b_vnbufs));
	for (; bp && bp != BEG_OF_LIST; bp = nbp) {
		nbp = BACK_BUF(bp);
#else /* LFS_NO_BACKBUF_HACK */
loop:	for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);
#endif /* LFS_NO_BACKBUF_HACK */
		if ((bp->b_flags & (B_BUSY|B_GATHERED)) || !match(fs, bp)) {
#ifdef DEBUG_LFS
			if (vp == fs->lfs_ivnode && (bp->b_flags & (B_BUSY|B_GATHERED)) == B_BUSY)
				printf("(%" PRId64 ":%lx)", bp->b_lblkno, bp->b_flags);
#endif
			continue;
		}
		if (vp->v_type == VBLK) {
			/* For block devices, just write the blocks. */
			/* XXX Do we really need to even do this? */
#ifdef DEBUG_LFS
			if (count == 0)
				printf("BLK(");
			printf(".");
#endif
			/* Get the block before bwrite, so we don't corrupt the free list */
			bp->b_flags |= B_BUSY;
			bremfree(bp);
			bwrite(bp);
		} else {
#ifdef DIAGNOSTIC
# ifdef LFS_USE_B_INVAL
			if ((bp->b_flags & (B_CALL|B_INVAL)) == B_INVAL) {
				printf("lfs_gather: lbn %" PRId64 " is "
					"B_INVAL\n", bp->b_lblkno);
				VOP_PRINT(bp->b_vp);
			}
# endif /* LFS_USE_B_INVAL */
			if (!(bp->b_flags & B_DELWRI))
				panic("lfs_gather: bp not B_DELWRI");
			if (!(bp->b_flags & B_LOCKED)) {
				printf("lfs_gather: lbn %" PRId64 " blk "
					"%" PRId64 " not B_LOCKED\n", 
					bp->b_lblkno,
					dbtofsb(fs, bp->b_blkno));
				VOP_PRINT(bp->b_vp);
				panic("lfs_gather: bp not B_LOCKED");
			}
#endif
			if (lfs_gatherblock(sp, bp, &s)) {
				goto loop;
			}
		}
		count++;
	}
	splx(s);
#ifdef DEBUG_LFS
	if (vp->v_type == VBLK && count)
		printf(")\n");
#endif
	lfs_updatemeta(sp);
	sp->vp = NULL;
	return count;
}

#if DEBUG
# define DEBUG_OOFF(n) do {						\
	if (ooff == 0) {						\
		printf("lfs_updatemeta[%d]: warning: writing "		\
			"ino %d lbn %" PRId64 " at 0x%" PRIx32		\
			", was 0x0 (or %" PRId64 ")\n",			\
			(n), ip->i_number, lbn, ndaddr, daddr);		\
	}								\
} while (0)
#else
# define DEBUG_OOFF(n)
#endif

/*
 * Change the given block's address to ndaddr, finding its previous
 * location using ufs_bmaparray().
 *
 * Account for this change in the segment table.
 */
void
lfs_update_single(struct lfs *fs, struct segment *sp, daddr_t lbn,
		  int32_t ndaddr, int size)
{
	SEGUSE *sup;
	struct buf *bp;
	struct indir a[NIADDR + 2], *ap;
	struct inode *ip;
	struct vnode *vp;
	daddr_t daddr, ooff;
	int num, error;
	int bb, osize, obb;
	
	vp = sp->vp;
	ip = VTOI(vp);

	error = ufs_bmaparray(vp, lbn, &daddr, a, &num, NULL);
	if (error)
		panic("lfs_updatemeta: ufs_bmaparray returned %d", error);

	KASSERT(daddr <= LFS_MAX_DADDR);
	if (daddr > 0)
		daddr = dbtofsb(fs, daddr);
	
	bb = fragstofsb(fs, numfrags(fs, size));
	switch (num) {
	    case 0:
		    ooff = ip->i_ffs_db[lbn];
		    DEBUG_OOFF(0);
		    if (ooff == UNWRITTEN)
			    ip->i_ffs_blocks += bb;
		    else {
			    /* possible fragment truncation or extension */
			    obb = btofsb(fs, ip->i_lfs_fragsize[lbn]);
			    ip->i_ffs_blocks += (bb - obb);
		    }
		    ip->i_ffs_db[lbn] = ndaddr;
		    break;
	    case 1:
		    ooff = ip->i_ffs_ib[a[0].in_off];
		    DEBUG_OOFF(1);
		    if (ooff == UNWRITTEN)
			    ip->i_ffs_blocks += bb;
		    ip->i_ffs_ib[a[0].in_off] = ndaddr;
		    break;
	    default:
		    ap = &a[num - 1];
		    if (bread(vp, ap->in_lbn, fs->lfs_bsize, NOCRED, &bp))
			    panic("lfs_updatemeta: bread bno %" PRId64,
				  ap->in_lbn);

		    /* XXX ondisk32 */
		    ooff = ((int32_t *)bp->b_data)[ap->in_off];
		    DEBUG_OOFF(num);
		    if (ooff == UNWRITTEN)
			    ip->i_ffs_blocks += bb;
		    /* XXX ondisk32 */
		    ((int32_t *)bp->b_data)[ap->in_off] = ndaddr;
		    (void) VOP_BWRITE(bp);
	}

	/*
	 * Though we'd rather it couldn't, this *can* happen right now
	 * if cleaning blocks and regular blocks coexist.
	 */
	/* KASSERT(daddr < fs->lfs_lastpseg || daddr > ndaddr); */

	/*
	 * Update segment usage information, based on old size
	 * and location.
	 */
	if (daddr > 0) {
		u_int32_t oldsn = dtosn(fs, daddr);
#ifdef DIAGNOSTIC
		int ndupino = (sp->seg_number == oldsn) ?
			sp->ndupino : 0;
#endif
		KASSERT(oldsn >= 0 && oldsn < fs->lfs_nseg);
		if (lbn >= 0 && lbn < NDADDR)
			osize = ip->i_lfs_fragsize[lbn];
		else
			osize = fs->lfs_bsize;
		LFS_SEGENTRY(sup, fs, oldsn, bp);
#ifdef DIAGNOSTIC
		if (sup->su_nbytes + DINODE_SIZE * ndupino < osize) {
			printf("lfs_updatemeta: negative bytes "
			       "(segment %" PRIu32 " short by %" PRId64
			       ")\n", dtosn(fs, daddr),
			       (int64_t)osize -
			       (DINODE_SIZE * sp->ndupino +
				sup->su_nbytes));
			printf("lfs_updatemeta: ino %d, lbn %" PRId64
			       ", addr = 0x%" PRIx64 "\n",
			       VTOI(sp->vp)->i_number, lbn, daddr);
			printf("lfs_updatemeta: ndupino=%d\n", ndupino);
			panic("lfs_updatemeta: negative bytes");
			sup->su_nbytes = osize - DINODE_SIZE * sp->ndupino;
		}
#endif
#ifdef DEBUG_SU_NBYTES
		printf("seg %" PRIu32 " -= %d for ino %d lbn %" PRId64
		       " db 0x%" PRIx64 "\n",
		       dtosn(fs, daddr), osize,
		       VTOI(sp->vp)->i_number, lbn, daddr);
#endif
		sup->su_nbytes -= osize;
		if (!(bp->b_flags & B_GATHERED))
			fs->lfs_flags |= LFS_IFDIRTY;
		LFS_WRITESEGENTRY(sup, fs, oldsn, bp);
	}
	/*
	 * Now that this block has a new address, and its old
	 * segment no longer owns it, we can forget about its
	 * old size.
	 */
	if (lbn >= 0 && lbn < NDADDR)
		ip->i_lfs_fragsize[lbn] = size;
}

/*
 * Update the metadata that points to the blocks listed in the FINFO
 * array.
 */
void
lfs_updatemeta(struct segment *sp)
{
	struct buf *sbp;
	struct lfs *fs;
	struct vnode *vp;
	daddr_t lbn;
	int i, nblocks, num;
	int bb;
	int bytesleft, size;
	
	vp = sp->vp;
	nblocks = &sp->fip->fi_blocks[sp->fip->fi_nblocks] - sp->start_lbp;
	KASSERT(nblocks >= 0);
	if (vp == NULL || nblocks == 0)
		return;

	/*
	 * This count may be high due to oversize blocks from lfs_gop_write.
	 * Correct for this. (XXX we should be able to keep track of these.)
	 */
	fs = sp->fs;
	for (i = 0; i < nblocks; i++) {
		if (sp->start_bpp[i] == NULL) {
			printf("nblocks = %d, not %d\n", i, nblocks);
			nblocks = i;
			break;
		}
		num = howmany(sp->start_bpp[i]->b_bcount, fs->lfs_bsize);
		KASSERT(sp->start_bpp[i]->b_lblkno >= 0 || num == 1);
		nblocks -= num - 1;
	}

	KASSERT(vp->v_type == VREG ||
	   nblocks == &sp->fip->fi_blocks[sp->fip->fi_nblocks] - sp->start_lbp);
	KASSERT(nblocks == sp->cbpp - sp->start_bpp);
	
	/*
	 * Sort the blocks.
	 *
	 * We have to sort even if the blocks come from the
	 * cleaner, because there might be other pending blocks on the
	 * same inode...and if we don't sort, and there are fragments
	 * present, blocks may be written in the wrong place.
	 */
	lfs_shellsort(sp->start_bpp, sp->start_lbp, nblocks, fs->lfs_bsize);
	
	/*
	 * Record the length of the last block in case it's a fragment.
	 * If there are indirect blocks present, they sort last.  An
	 * indirect block will be lfs_bsize and its presence indicates
	 * that you cannot have fragments.
	 *
	 * XXX This last is a lie.  A cleaned fragment can coexist with
	 * XXX a later indirect block.	This will continue to be
	 * XXX true until lfs_markv is fixed to do everything with
	 * XXX fake blocks (including fake inodes and fake indirect blocks).
	 */
	sp->fip->fi_lastlength = ((sp->start_bpp[nblocks - 1]->b_bcount - 1) &
		fs->lfs_bmask) + 1;
	
	/*
	 * Assign disk addresses, and update references to the logical
	 * block and the segment usage information.
	 */
	for (i = nblocks; i--; ++sp->start_bpp) {
		sbp = *sp->start_bpp;
		lbn = *sp->start_lbp;
		KASSERT(sbp->b_lblkno == lbn);

		sbp->b_blkno = fsbtodb(fs, fs->lfs_offset);

		/*
		 * If we write a frag in the wrong place, the cleaner won't
		 * be able to correctly identify its size later, and the
		 * segment will be uncleanable.	 (Even worse, it will assume
		 * that the indirect block that actually ends the list
		 * is of a smaller size!)
		 */
		if ((sbp->b_bcount & fs->lfs_bmask) && i != 0)
			panic("lfs_updatemeta: fragment is not last block");
		
		/*
		 * For each subblock in this possibly oversized block,
		 * update its address on disk.
		 */
		KASSERT(lbn >= 0 || sbp->b_bcount == fs->lfs_bsize);
		for (bytesleft = sbp->b_bcount; bytesleft > 0;
		     bytesleft -= fs->lfs_bsize) {
			size = MIN(bytesleft, fs->lfs_bsize);
			bb = fragstofsb(fs, numfrags(fs, size));
			lbn = *sp->start_lbp++;
			lfs_update_single(fs, sp, lbn, fs->lfs_offset, size);
			fs->lfs_offset += bb;
		}

	}
}

/*
 * Start a new segment.
 */
int
lfs_initseg(struct lfs *fs)
{
	struct segment *sp;
	SEGUSE *sup;
	SEGSUM *ssp;
	struct buf *bp, *sbp;
	int repeat;
	
	sp = fs->lfs_sp;

	repeat = 0;

	/* Advance to the next segment. */
	if (!LFS_PARTIAL_FITS(fs)) {
		/* lfs_avail eats the remaining space */
		fs->lfs_avail -= fs->lfs_fsbpseg - (fs->lfs_offset -
						   fs->lfs_curseg);
		/* Wake up any cleaning procs waiting on this file system. */
		wakeup(&lfs_allclean_wakeup);
		wakeup(&fs->lfs_nextseg);
		lfs_newseg(fs);
		repeat = 1;
		fs->lfs_offset = fs->lfs_curseg;
	
		sp->seg_number = dtosn(fs, fs->lfs_curseg);
		sp->seg_bytes_left = fsbtob(fs, fs->lfs_fsbpseg);

		/*
		 * If the segment contains a superblock, update the offset
		 * and summary address to skip over it.
		 */
		LFS_SEGENTRY(sup, fs, sp->seg_number, bp);
		if (sup->su_flags & SEGUSE_SUPERBLOCK) {
			fs->lfs_offset += btofsb(fs, LFS_SBPAD);
			sp->seg_bytes_left -= LFS_SBPAD;
		}
		brelse(bp);
		/* Segment zero could also contain the labelpad */
		if (fs->lfs_version > 1 && sp->seg_number == 0 &&
		    fs->lfs_start < btofsb(fs, LFS_LABELPAD)) {
			fs->lfs_offset += btofsb(fs, LFS_LABELPAD) - fs->lfs_start;
			sp->seg_bytes_left -= LFS_LABELPAD - fsbtob(fs, fs->lfs_start);
		}
	} else {
		sp->seg_number = dtosn(fs, fs->lfs_curseg);
		sp->seg_bytes_left = fsbtob(fs, fs->lfs_fsbpseg -
				      (fs->lfs_offset - fs->lfs_curseg));
	}
	fs->lfs_lastpseg = fs->lfs_offset;
	
	/* Record first address of this partial segment */
	if (sp->seg_flags & SEGM_CLEAN) {
		fs->lfs_cleanint[fs->lfs_cleanind] = fs->lfs_offset;
		if (++fs->lfs_cleanind >= LFS_MAX_CLEANIND) {
			/* "1" is the artificial inc in lfs_seglock */
			while (fs->lfs_iocount > 1) {
				tsleep(&fs->lfs_iocount, PRIBIO + 1, "lfs_initseg", 0);
			}
			fs->lfs_cleanind = 0;
		}
	}

	sp->fs = fs;
	sp->ibp = NULL;
	sp->idp = NULL;
	sp->ninodes = 0;
	sp->ndupino = 0;

	/* Get a new buffer for SEGSUM and enter it into the buffer list. */
	sp->cbpp = sp->bpp;
	sbp = *sp->cbpp = lfs_newbuf(fs, VTOI(fs->lfs_ivnode)->i_devvp,
				     fsbtodb(fs, fs->lfs_offset), fs->lfs_sumsize, LFS_NB_SUMMARY);
	sp->segsum = (*sp->cbpp)->b_data;
	memset(sp->segsum, 0, fs->lfs_sumsize);
	sp->start_bpp = ++sp->cbpp;
	fs->lfs_offset += btofsb(fs, fs->lfs_sumsize);
	
	/* Set point to SEGSUM, initialize it. */
	ssp = sp->segsum;
	ssp->ss_next = fs->lfs_nextseg;
	ssp->ss_nfinfo = ssp->ss_ninos = 0;
	ssp->ss_magic = SS_MAGIC;

	/* Set pointer to first FINFO, initialize it. */
	sp->fip = (struct finfo *)((caddr_t)sp->segsum + SEGSUM_SIZE(fs));
	sp->fip->fi_nblocks = 0;
	sp->start_lbp = &sp->fip->fi_blocks[0];
	sp->fip->fi_lastlength = 0;
	
	sp->seg_bytes_left -= fs->lfs_sumsize;
	sp->sum_bytes_left = fs->lfs_sumsize - SEGSUM_SIZE(fs);

	return (repeat);
}

/*
 * Return the next segment to write.
 */
void
lfs_newseg(struct lfs *fs)
{
	CLEANERINFO *cip;
	SEGUSE *sup;
	struct buf *bp;
	int curseg, isdirty, sn;
	
	LFS_SEGENTRY(sup, fs, dtosn(fs, fs->lfs_nextseg), bp);
#ifdef DEBUG_SU_NBYTES
	printf("lfs_newseg: seg %d := 0 in newseg\n",	/* XXXDEBUG */
	       dtosn(fs, fs->lfs_nextseg)); /* XXXDEBUG */
#endif
	sup->su_flags |= SEGUSE_DIRTY | SEGUSE_ACTIVE;
	sup->su_nbytes = 0;
	sup->su_nsums = 0;
	sup->su_ninos = 0;
	LFS_WRITESEGENTRY(sup, fs, dtosn(fs, fs->lfs_nextseg), bp);

	LFS_CLEANERINFO(cip, fs, bp);
	--cip->clean;
	++cip->dirty;
	fs->lfs_nclean = cip->clean;
	LFS_SYNC_CLEANERINFO(cip, fs, bp, 1);

	fs->lfs_lastseg = fs->lfs_curseg;
	fs->lfs_curseg = fs->lfs_nextseg;
	for (sn = curseg = dtosn(fs, fs->lfs_curseg) + fs->lfs_interleave;;) {
		sn = (sn + 1) % fs->lfs_nseg;
		if (sn == curseg)
			panic("lfs_nextseg: no clean segments");
		LFS_SEGENTRY(sup, fs, sn, bp);
		isdirty = sup->su_flags & SEGUSE_DIRTY;
		/* Check SEGUSE_EMPTY as we go along */
		if (isdirty && sup->su_nbytes == 0 && !(sup->su_flags & SEGUSE_EMPTY))
			LFS_WRITESEGENTRY(sup, fs, sn, bp);
		else
			brelse(bp);

		if (!isdirty)
			break;
	}
	
	++fs->lfs_nactive;
	fs->lfs_nextseg = sntod(fs, sn);
	if (lfs_dostats) {
		++lfs_stats.segsused;
	}
}

#define BQUEUES 4 /* XXX */
#define BQ_EMPTY 3 /* XXX */
extern TAILQ_HEAD(bqueues, buf) bufqueues[BQUEUES];
extern struct simplelock bqueue_slock;

#define	BUFHASH(dvp, lbn)	\
	(&bufhashtbl[((long)(dvp) / sizeof(*(dvp)) + (int)(lbn)) & bufhash])
extern LIST_HEAD(bufhashhdr, buf) invalhash;
/*
 * Insq/Remq for the buffer hash lists.
 */
#define	binshash(bp, dp)	LIST_INSERT_HEAD(dp, bp, b_hash)
#define	bremhash(bp)		LIST_REMOVE(bp, b_hash)

static struct buf *
lfs_newclusterbuf(struct lfs *fs, struct vnode *vp, daddr_t addr, int n)
{
	struct lfs_cluster *cl;
	struct buf **bpp, *bp;
	int s;

	cl = (struct lfs_cluster *)pool_get(&fs->lfs_clpool, PR_WAITOK);
	bpp = (struct buf **)pool_get(&fs->lfs_bpppool, PR_WAITOK);
	memset(cl, 0, sizeof(*cl));
	cl->fs = fs;
	cl->bpp = bpp;
	cl->bufcount = 0;
	cl->bufsize = 0;

	/* If this segment is being written synchronously, note that */
	if (fs->lfs_sp->seg_flags & SEGM_SYNC) {
		cl->flags |= LFS_CL_SYNC;
		cl->seg = fs->lfs_sp;
		++cl->seg->seg_iocount;
		/* printf("+ %x => %d\n", cl->seg, cl->seg->seg_iocount); */
	}

	/* Get an empty buffer header, or maybe one with something on it */
	s = splbio();
	simple_lock(&bqueue_slock);
	if ((bp = bufqueues[BQ_EMPTY].tqh_first) != NULL) {
		simple_lock(&bp->b_interlock);
		bremfree(bp);
		/* clear out various other fields */
		bp->b_flags = B_BUSY;
		bp->b_dev = NODEV;
		bp->b_blkno = bp->b_lblkno = 0;
		bp->b_error = 0;
		bp->b_resid = 0;
		bp->b_bcount = 0;
		
		/* nuke any credentials we were holding */
		/* XXXXXX */
	
		bremhash(bp);

		/* disassociate us from our vnode, if we had one... */
		if (bp->b_vp)
			brelvp(bp);
	}
	while (!bp)
		bp = getnewbuf(0, 0);
	bgetvp(vp, bp);
	binshash(bp,&invalhash);
	simple_unlock(&bp->b_interlock);
	simple_unlock(&bqueue_slock);
	splx(s);
	bp->b_bcount = 0;
	bp->b_blkno = bp->b_lblkno = addr;

	bp->b_flags |= B_CALL;
	bp->b_iodone = lfs_cluster_callback;
	cl->saveaddr = bp->b_saveaddr; /* XXX is this ever used? */
	bp->b_saveaddr = (caddr_t)cl;

	return bp;
}

int
lfs_writeseg(struct lfs *fs, struct segment *sp)
{
	struct buf **bpp, *bp, *cbp, *newbp;
	SEGUSE *sup;
	SEGSUM *ssp;
	dev_t i_dev;
	char *datap, *dp;
	int i, s;
	int do_again, nblocks, byteoffset;
	size_t el_size;
	struct lfs_cluster *cl;
	int (*strategy)(void *);
	struct vop_strategy_args vop_strategy_a;
	u_short ninos;
	struct vnode *devvp;
	char *p;
	struct vnode *vp;
	int32_t *daddrp;	/* XXX ondisk32 */
	int changed;
#if defined(DEBUG) && defined(LFS_PROPELLER)
	static int propeller;
	char propstring[4] = "-\\|/";
	
	printf("%c\b",propstring[propeller++]);
	if (propeller == 4)
		propeller = 0;
#endif

	/*
	 * If there are no buffers other than the segment summary to write
	 * and it is not a checkpoint, don't do anything.  On a checkpoint,
	 * even if there aren't any buffers, you need to write the superblock.
	 */
	if ((nblocks = sp->cbpp - sp->bpp) == 1)
		return (0);
	
	i_dev = VTOI(fs->lfs_ivnode)->i_dev;
	devvp = VTOI(fs->lfs_ivnode)->i_devvp;

	/* Update the segment usage information. */
	LFS_SEGENTRY(sup, fs, sp->seg_number, bp);
	
	/* Loop through all blocks, except the segment summary. */
	for (bpp = sp->bpp; ++bpp < sp->cbpp; ) {
		if ((*bpp)->b_vp != devvp) {
			sup->su_nbytes += (*bpp)->b_bcount;
#ifdef DEBUG_SU_NBYTES
		printf("seg %" PRIu32 " += %ld for ino %d lbn %" PRId64
		    " db 0x%" PRIx64 "\n", sp->seg_number, (*bpp)->b_bcount,
		    VTOI((*bpp)->b_vp)->i_number, (*bpp)->b_lblkno,
		    (*bpp)->b_blkno);
#endif
		}
	}
	
	ssp = (SEGSUM *)sp->segsum;
	
	ninos = (ssp->ss_ninos + INOPB(fs) - 1) / INOPB(fs);
#ifdef DEBUG_SU_NBYTES
	printf("seg %d += %d for %d inodes\n",	 /* XXXDEBUG */
	       sp->seg_number, ssp->ss_ninos * DINODE_SIZE,
	       ssp->ss_ninos);
#endif
	sup->su_nbytes += ssp->ss_ninos * DINODE_SIZE;
	/* sup->su_nbytes += fs->lfs_sumsize; */
	if (fs->lfs_version == 1)
		sup->su_olastmod = time.tv_sec;
	else
		sup->su_lastmod = time.tv_sec;
	sup->su_ninos += ninos;
	++sup->su_nsums;
	fs->lfs_dmeta += (btofsb(fs, fs->lfs_sumsize) + btofsb(fs, ninos *
							 fs->lfs_ibsize));
	fs->lfs_avail -= btofsb(fs, fs->lfs_sumsize);

	do_again = !(bp->b_flags & B_GATHERED);
	LFS_WRITESEGENTRY(sup, fs, sp->seg_number, bp); /* Ifile */

	/*
	 * Mark blocks B_BUSY, to prevent then from being changed between
	 * the checksum computation and the actual write.
	 *
	 * If we are cleaning, check indirect blocks for UNWRITTEN, and if
	 * there are any, replace them with copies that have UNASSIGNED
	 * instead.
	 */
	for (bpp = sp->bpp, i = nblocks - 1; i--;) {
		++bpp;
		bp = *bpp;
		if (bp->b_flags & B_CALL) { /* UBC or malloced buffer */
			bp->b_flags |= B_BUSY;
			continue;
		}
	    again:
		s = splbio();
		if (bp->b_flags & B_BUSY) {
#ifdef DEBUG
			printf("lfs_writeseg: avoiding potential data summary "
			       "corruption for ino %d, lbn %" PRId64 "\n",
			       VTOI(bp->b_vp)->i_number, bp->b_lblkno);
#endif
			bp->b_flags |= B_WANTED;
			tsleep(bp, (PRIBIO + 1), "lfs_writeseg", 0);
			splx(s);
			goto again;
		}
		bp->b_flags |= B_BUSY;
		splx(s);
		/*
		 * Check and replace indirect block UNWRITTEN bogosity.
		 * XXX See comment in lfs_writefile.
		 */
		if (bp->b_lblkno < 0 && bp->b_vp != devvp && bp->b_vp &&
		   VTOI(bp->b_vp)->i_ffs_blocks !=
		   VTOI(bp->b_vp)->i_lfs_effnblks) {
#ifdef DEBUG_LFS
			printf("lfs_writeseg: cleansing ino %d (%d != %d)\n",
			       VTOI(bp->b_vp)->i_number,
			       VTOI(bp->b_vp)->i_lfs_effnblks,
			       VTOI(bp->b_vp)->i_ffs_blocks);
#endif
			/* Make a copy we'll make changes to */
			newbp = lfs_newbuf(fs, bp->b_vp, bp->b_lblkno,
					   bp->b_bcount, LFS_NB_IBLOCK);
			newbp->b_blkno = bp->b_blkno;
			memcpy(newbp->b_data, bp->b_data,
			       newbp->b_bcount);

			changed = 0;
			/* XXX ondisk32 */
			for (daddrp = (int32_t *)(newbp->b_data);
			     daddrp < (int32_t *)(newbp->b_data +
						  newbp->b_bcount); daddrp++) {
				if (*daddrp == UNWRITTEN) {
#ifdef DEBUG_LFS
					off_t doff;
					int32_t ioff;
					
					ioff = daddrp - (int32_t *)(newbp->b_data);
					doff = (-bp->b_lblkno + ioff) * fs->lfs_bsize;
					printf("ino %d lbn %" PRId64 " entry %d off %" PRIx64 "\n",
					       VTOI(bp->b_vp)->i_number,
					       bp->b_lblkno, ioff, doff);
					if (bp->b_vp->v_type == VREG) {
						/*
						 * What is up with this page?
						 */
						struct vm_page *pg;
						for (; doff / fs->lfs_bsize == (-bp->b_lblkno + ioff); doff += PAGE_SIZE) {
							pg = uvm_pagelookup(&bp->b_vp->v_uobj, doff);
							if (pg == NULL)
								printf("  page at %" PRIx64 " is NULL\n", doff);
							else
								printf("  page at %" PRIx64 " flags 0x%x pqflags 0x%x\n", doff, pg->flags, pg->pqflags);
						}
					}
#endif /* DEBUG_LFS */
					++changed;
					*daddrp = 0;
				}
			}
			/*
			 * Get rid of the old buffer.  Don't mark it clean,
			 * though, if it still has dirty data on it.
			 */
			if (changed) {
#ifdef DEBUG_LFS
				printf("lfs_writeseg: replacing UNWRITTEN(%d):"
					" bp = %p newbp = %p\n", changed, bp,
					newbp);
#endif
				*bpp = newbp;
				bp->b_flags &= ~(B_ERROR | B_GATHERED | B_DONE);
				if (bp->b_flags & B_CALL) {
					printf("lfs_writeseg: indir bp should not be B_CALL\n");
					s = splbio();
					biodone(bp);
					splx(s);
					bp = NULL;
				} else {
					/* Still on free list, leave it there */
					s = splbio();
					bp->b_flags &= ~B_BUSY;
					if (bp->b_flags & B_WANTED)
						wakeup(bp);
					splx(s);
					/*
					 * We have to re-decrement lfs_avail
					 * since this block is going to come
					 * back around to us in the next
					 * segment.
					 */
					fs->lfs_avail -= btofsb(fs, bp->b_bcount);
				}
			} else {
				lfs_freebuf(fs, newbp);
			}
		}
	}
	/*
	 * Compute checksum across data and then across summary; the first
	 * block (the summary block) is skipped.  Set the create time here
	 * so that it's guaranteed to be later than the inode mod times.
	 *
	 * XXX
	 * Fix this to do it inline, instead of malloc/copy.
	 */
	datap = dp = pool_get(&fs->lfs_bpppool, PR_WAITOK);
	if (fs->lfs_version == 1)
		el_size = sizeof(u_long);
	else
		el_size = sizeof(u_int32_t);
	for (bpp = sp->bpp, i = nblocks - 1; i--; ) {
		++bpp;
		/* Loop through gop_write cluster blocks */
		for (byteoffset = 0; byteoffset < (*bpp)->b_bcount;
		     byteoffset += fs->lfs_bsize) {
#ifdef LFS_USE_B_INVAL
			if (((*bpp)->b_flags & (B_CALL | B_INVAL)) ==
			    (B_CALL | B_INVAL)) {
				if (copyin((caddr_t)(*bpp)->b_saveaddr +
					   byteoffset, dp, el_size)) {
					panic("lfs_writeseg: copyin failed [1]: "
						"ino %d blk %" PRId64,
						VTOI((*bpp)->b_vp)->i_number,
						(*bpp)->b_lblkno);
				}
			} else
#endif /* LFS_USE_B_INVAL */
			{
				memcpy(dp, (*bpp)->b_data + byteoffset,
				       el_size);
			}
			dp += el_size;
		}
	}
	if (fs->lfs_version == 1)
		ssp->ss_ocreate = time.tv_sec;
	else {
		ssp->ss_create = time.tv_sec;
		ssp->ss_serial = ++fs->lfs_serial;
		ssp->ss_ident  = fs->lfs_ident;
	}
	ssp->ss_datasum = cksum(datap, dp - datap);
	ssp->ss_sumsum =
	    cksum(&ssp->ss_datasum, fs->lfs_sumsize - sizeof(ssp->ss_sumsum));
	pool_put(&fs->lfs_bpppool, datap);
	datap = dp = NULL;
#ifdef DIAGNOSTIC
	if (fs->lfs_bfree < btofsb(fs, ninos * fs->lfs_ibsize) + btofsb(fs, fs->lfs_sumsize))
		panic("lfs_writeseg: No diskspace for summary");
#endif
	fs->lfs_bfree -= (btofsb(fs, ninos * fs->lfs_ibsize) +
			  btofsb(fs, fs->lfs_sumsize));

	strategy = devvp->v_op[VOFFSET(vop_strategy)];

	/*
	 * When we simply write the blocks we lose a rotation for every block
	 * written.  To avoid this problem, we cluster the buffers into a
	 * chunk and write the chunk.  MAXPHYS is the largest size I/O
	 * devices can handle, use that for the size of the chunks.
	 *
	 * Blocks that are already clusters (from GOP_WRITE), however, we
	 * don't bother to copy into other clusters.
	 */

#define CHUNKSIZE MAXPHYS

	if (devvp == NULL)
		panic("devvp is NULL");
	for (bpp = sp->bpp, i = nblocks; i;) {
		cbp = lfs_newclusterbuf(fs, devvp, (*bpp)->b_blkno, i);
		cl = (struct lfs_cluster *)cbp->b_saveaddr;

		cbp->b_dev = i_dev;
		cbp->b_flags |= B_ASYNC | B_BUSY;
		cbp->b_bcount = 0;

		cl->olddata = cbp->b_data;
#if defined(DEBUG) && defined(DIAGNOSTIC)
		if (bpp - sp->bpp > (fs->lfs_sumsize - SEGSUM_SIZE(fs))
		    / sizeof(int32_t)) {
			panic("lfs_writeseg: real bpp overwrite");
		}
		if (bpp - sp->bpp > fs->lfs_ssize / fs->lfs_fsize) {
			panic("lfs_writeseg: theoretical bpp overwrite");
		}
#endif

		/*
		 * Construct the cluster.
		 */
		++fs->lfs_iocount;
		while (i && cbp->b_bcount < CHUNKSIZE) {
			bp = *bpp;

			if (bp->b_bcount > (CHUNKSIZE - cbp->b_bcount))
				break;
			if (cbp->b_bcount > 0 && !(cl->flags & LFS_CL_MALLOC))
				break;

			/* Clusters from GOP_WRITE are expedited */
			if (bp->b_bcount > fs->lfs_bsize) {
				if (cbp->b_bcount > 0)
					/* Put in its own buffer */
					break;
				else {
					cbp->b_data = bp->b_data;
				}
			} else if (cbp->b_bcount == 0) {
				p = cbp->b_data = lfs_malloc(fs, CHUNKSIZE,
							     LFS_NB_CLUSTER);
				cl->flags |= LFS_CL_MALLOC;
			}
#ifdef DIAGNOSTIC
			if (dtosn(fs, dbtofsb(fs, bp->b_blkno +
					      btodb(bp->b_bcount - 1))) !=
			    sp->seg_number) {
				printf("blk size %ld daddr %" PRIx64 " not in seg %d\n",
					bp->b_bcount, bp->b_blkno,
					sp->seg_number);
				panic("segment overwrite");
			}
#endif

#ifdef LFS_USE_B_INVAL
			/*
			 * Fake buffers from the cleaner are marked as B_INVAL.
			 * We need to copy the data from user space rather than
			 * from the buffer indicated.
			 * XXX == what do I do on an error?
			 */
			if ((bp->b_flags & (B_CALL|B_INVAL)) == (B_CALL|B_INVAL)) {
				if (copyin(bp->b_saveaddr, p, bp->b_bcount))
					panic("lfs_writeseg: copyin failed [2]");
			} else
#endif /* LFS_USE_B_INVAL */
			if (cl->flags & LFS_CL_MALLOC) {
				bcopy(bp->b_data, p, bp->b_bcount);
			}
  
			p += bp->b_bcount;
			cbp->b_bcount += bp->b_bcount;
			cl->bufsize += bp->b_bcount;

			bp->b_flags &= ~(B_ERROR | B_READ | B_DELWRI | B_DONE);
			cl->bpp[cl->bufcount++] = bp;
			vp = bp->b_vp;
			s = splbio();
			V_INCR_NUMOUTPUT(vp);
			splx(s);

			bpp++;
			i--;
		}
		s = splbio();
		V_INCR_NUMOUTPUT(devvp);
		splx(s);
		vop_strategy_a.a_desc = VDESC(vop_strategy);
		vop_strategy_a.a_bp = cbp;
		(strategy)(&vop_strategy_a);
		curproc->p_stats->p_ru.ru_oublock++;
	}

	if (lfs_dostats) {
		++lfs_stats.psegwrites;
		lfs_stats.blocktot += nblocks - 1;
		if (fs->lfs_sp->seg_flags & SEGM_SYNC)
			++lfs_stats.psyncwrites;
		if (fs->lfs_sp->seg_flags & SEGM_CLEAN) {
			++lfs_stats.pcleanwrites;
			lfs_stats.cleanblocks += nblocks - 1;
		}
	}
	return (lfs_initseg(fs) || do_again);
}

void
lfs_writesuper(struct lfs *fs, daddr_t daddr)
{
	struct buf *bp;
	dev_t i_dev;
	int (*strategy)(void *);
	int s;
	struct vop_strategy_args vop_strategy_a;

	/*
	 * If we can write one superblock while another is in
	 * progress, we risk not having a complete checkpoint if we crash.
	 * So, block here if a superblock write is in progress.
	 */
	s = splbio();
	while (fs->lfs_sbactive) {
		tsleep(&fs->lfs_sbactive, PRIBIO+1, "lfs sb", 0);
	}
	fs->lfs_sbactive = daddr;
	splx(s);
	i_dev = VTOI(fs->lfs_ivnode)->i_dev;
	strategy = VTOI(fs->lfs_ivnode)->i_devvp->v_op[VOFFSET(vop_strategy)];

	/* Set timestamp of this version of the superblock */
	if (fs->lfs_version == 1)
		fs->lfs_otstamp = time.tv_sec;
	fs->lfs_tstamp = time.tv_sec;

	/* Checksum the superblock and copy it into a buffer. */
	fs->lfs_cksum = lfs_sb_cksum(&(fs->lfs_dlfs));
	bp = lfs_newbuf(fs, VTOI(fs->lfs_ivnode)->i_devvp, fsbtodb(fs, daddr), LFS_SBPAD, LFS_NB_SBLOCK);
	memset(bp->b_data + sizeof(struct dlfs), 0, LFS_SBPAD - sizeof(struct dlfs));
	*(struct dlfs *)bp->b_data = fs->lfs_dlfs;
	
	bp->b_dev = i_dev;
	bp->b_flags |= B_BUSY | B_CALL | B_ASYNC;
	bp->b_flags &= ~(B_DONE | B_ERROR | B_READ | B_DELWRI);
	bp->b_iodone = lfs_supercallback;
	/* XXX KS - same nasty hack as above */
	bp->b_saveaddr = (caddr_t)fs;

	vop_strategy_a.a_desc = VDESC(vop_strategy);
	vop_strategy_a.a_bp = bp;
	curproc->p_stats->p_ru.ru_oublock++;
	s = splbio();
	V_INCR_NUMOUTPUT(bp->b_vp);
	splx(s);
	++fs->lfs_iocount;
	(strategy)(&vop_strategy_a);
}

/*
 * Logical block number match routines used when traversing the dirty block
 * chain.
 */
int
lfs_match_fake(struct lfs *fs, struct buf *bp)
{
	return LFS_IS_MALLOC_BUF(bp);
}

#if 0
int
lfs_match_real(struct lfs *fs, struct buf *bp)
{
	return (lfs_match_data(fs, bp) && !lfs_match_fake(fs, bp));
}
#endif

int
lfs_match_data(struct lfs *fs, struct buf *bp)
{
	return (bp->b_lblkno >= 0);
}

int
lfs_match_indir(struct lfs *fs, struct buf *bp)
{
	daddr_t lbn;

	lbn = bp->b_lblkno;
	return (lbn < 0 && (-lbn - NDADDR) % NINDIR(fs) == 0);
}

int
lfs_match_dindir(struct lfs *fs, struct buf *bp)
{
	daddr_t lbn;

	lbn = bp->b_lblkno;
	return (lbn < 0 && (-lbn - NDADDR) % NINDIR(fs) == 1);
}

int
lfs_match_tindir(struct lfs *fs, struct buf *bp)
{
	daddr_t lbn;

	lbn = bp->b_lblkno;
	return (lbn < 0 && (-lbn - NDADDR) % NINDIR(fs) == 2);
}

/*
 * XXX - The only buffers that are going to hit these functions are the
 * segment write blocks, or the segment summaries, or the superblocks.
 * 
 * All of the above are created by lfs_newbuf, and so do not need to be
 * released via brelse.
 */
void
lfs_callback(struct buf *bp)
{
	struct lfs *fs;

	fs = (struct lfs *)bp->b_saveaddr;
	lfs_freebuf(fs, bp);
}

static void
lfs_super_aiodone(struct buf *bp)
{
	struct lfs *fs;

	fs = (struct lfs *)bp->b_saveaddr;
	fs->lfs_sbactive = 0;
	wakeup(&fs->lfs_sbactive);
	if (--fs->lfs_iocount <= 1)
		wakeup(&fs->lfs_iocount);
	lfs_freebuf(fs, bp);
}

static void
lfs_cluster_aiodone(struct buf *bp)
{
	struct lfs_cluster *cl;
	struct lfs *fs;
	struct buf *tbp, *fbp;
	struct vnode *vp, *devvp;
	struct inode *ip;
	int s, error=0;
	char *cp;
	extern int locked_queue_count;
	extern long locked_queue_bytes;

	if (bp->b_flags & B_ERROR)
		error = bp->b_error;

	cl = (struct lfs_cluster *)bp->b_saveaddr;
	fs = cl->fs;
	devvp = VTOI(fs->lfs_ivnode)->i_devvp;
	bp->b_saveaddr = cl->saveaddr;

	cp = (char *)bp->b_data + cl->bufsize;
	/* Put the pages back, and release the buffer */
	while (cl->bufcount--) {
		tbp = cl->bpp[cl->bufcount];
		if (error) {
			tbp->b_flags |= B_ERROR;
			tbp->b_error = error;
		}

		/*
		 * We're done with tbp.	 If it has not been re-dirtied since
		 * the cluster was written, free it.  Otherwise, keep it on
		 * the locked list to be written again.
		 */
		vp = tbp->b_vp;

		if ((tbp->b_flags & (B_LOCKED | B_DELWRI)) == B_LOCKED)
			LFS_UNLOCK_BUF(tbp);

		tbp->b_flags &= ~B_GATHERED;

		LFS_BCLEAN_LOG(fs, tbp);

		if (!(tbp->b_flags & B_CALL)) {
			bremfree(tbp);
			s = splbio();
			if (vp)
				reassignbuf(tbp, vp);
			splx(s);
			tbp->b_flags |= B_ASYNC; /* for biodone */
		}
#ifdef DIAGNOSTIC
		if (tbp->b_flags & B_DONE) {
			printf("blk %d biodone already (flags %lx)\n",
				cl->bufcount, (long)tbp->b_flags);
		}
#endif
		if (tbp->b_flags & (B_BUSY | B_CALL)) {
			if ((tbp->b_flags & B_CALL) && !LFS_IS_MALLOC_BUF(tbp)) {
				/* printf("flags 0x%lx\n", tbp->b_flags); */
				/*
				 * A buffer from the page daemon.
				 * We use the same iodone as it does,
				 * so we must manually disassociate its
				 * buffers from the vp.
				 */
				if (tbp->b_vp) {
					/* This is just silly */
					s = splbio();
					brelvp(tbp);
					tbp->b_vp = vp;
					splx(s);
				}
				/* Put it back the way it was */
				tbp->b_flags |= B_ASYNC;
				/* Master buffers have B_AGE */
				if (tbp->b_private == tbp)
					tbp->b_flags |= B_AGE;
			}
			s = splbio();
			biodone(tbp);

			/*
			 * If this is the last block for this vnode, but
			 * there are other blocks on its dirty list,
			 * set IN_MODIFIED/IN_CLEANING depending on what
			 * sort of block.  Only do this for our mount point,
			 * not for, e.g., inode blocks that are attached to
			 * the devvp.
			 * XXX KS - Shouldn't we set *both* if both types
			 * of blocks are present (traverse the dirty list?)
			 */
			simple_lock(&global_v_numoutput_slock);
			if (vp != devvp && vp->v_numoutput == 0 &&
			    (fbp = LIST_FIRST(&vp->v_dirtyblkhd)) != NULL) {
				ip = VTOI(vp);
#ifdef DEBUG_LFS
				printf("lfs_cluster_aiodone: marking ino %d\n",
				       ip->i_number);
#endif
				if (LFS_IS_MALLOC_BUF(fbp))
					LFS_SET_UINO(ip, IN_CLEANING);
				else
					LFS_SET_UINO(ip, IN_MODIFIED);
			}
			simple_unlock(&global_v_numoutput_slock);
			splx(s);
			wakeup(vp);
		}
	}

	/* Fix up the cluster buffer, and release it */
	if (cl->flags & LFS_CL_MALLOC)
		lfs_free(fs, bp->b_data, LFS_NB_CLUSTER);
	bp->b_data = cl->olddata;
	bp->b_bcount = 0;
	bp->b_iodone = NULL;
	bp->b_flags &= ~B_DELWRI;
	bp->b_flags |= B_DONE;
	s = splbio();
	reassignbuf(bp, bp->b_vp);
	splx(s);
	brelse(bp);

	/* Note i/o done */
	if (cl->flags & LFS_CL_SYNC) {
		if (--cl->seg->seg_iocount == 0) 
			wakeup(&cl->seg->seg_iocount);
		/* printf("- %x => %d\n", cl->seg, cl->seg->seg_iocount); */
	}
#ifdef DIAGNOSTIC
	if (fs->lfs_iocount == 0)
		panic("lfs_cluster_aiodone: zero iocount");
#endif
	if (--fs->lfs_iocount <= 1)
		wakeup(&fs->lfs_iocount);

	pool_put(&fs->lfs_bpppool, cl->bpp);
	cl->bpp = NULL;
	pool_put(&fs->lfs_clpool, cl);
}

static void
lfs_generic_callback(struct buf *bp, void (*aiodone)(struct buf *))
{
	/* reset b_iodone for when this is a single-buf i/o. */
	bp->b_iodone = aiodone;

	simple_lock(&uvm.aiodoned_lock);	/* locks uvm.aio_done */
	TAILQ_INSERT_TAIL(&uvm.aio_done, bp, b_freelist);
	wakeup(&uvm.aiodoned);
	simple_unlock(&uvm.aiodoned_lock);
}

static void
lfs_cluster_callback(struct buf *bp)
{
	lfs_generic_callback(bp, lfs_cluster_aiodone);
}

void
lfs_supercallback(struct buf *bp)
{
	lfs_generic_callback(bp, lfs_super_aiodone);
}

/*
 * Shellsort (diminishing increment sort) from Data Structures and
 * Algorithms, Aho, Hopcraft and Ullman, 1983 Edition, page 290;
 * see also Knuth Vol. 3, page 84.  The increments are selected from
 * formula (8), page 95.  Roughly O(N^3/2).
 */
/*
 * This is our own private copy of shellsort because we want to sort
 * two parallel arrays (the array of buffer pointers and the array of
 * logical block numbers) simultaneously.  Note that we cast the array
 * of logical block numbers to a unsigned in this routine so that the
 * negative block numbers (meta data blocks) sort AFTER the data blocks.
 */

void
lfs_shellsort(struct buf **bp_array, int32_t *lb_array, int nmemb, int size)
{
	static int __rsshell_increments[] = { 4, 1, 0 };
	int incr, *incrp, t1, t2;
	struct buf *bp_temp;

#ifdef DEBUG
	incr = 0;
	for (t1 = 0; t1 < nmemb; t1++) {
		for (t2 = 0; t2 * size < bp_array[t1]->b_bcount; t2++) {
			if (lb_array[incr++] != bp_array[t1]->b_lblkno + t2) {
				/* dump before panic */
				printf("lfs_shellsort: nmemb=%d, size=%d\n",
				    nmemb, size);
				incr = 0;
				for (t1 = 0; t1 < nmemb; t1++) {
					const struct buf *bp = bp_array[t1];

					printf("bp[%d]: lbn=%" PRIu64 ", size=%"
					    PRIu64 "\n", t1,
					    (uint64_t)bp->b_bcount,
					    (uint64_t)bp->b_lblkno);
					printf("lbns:");
					for (t2 = 0; t2 * size < bp->b_bcount;
					    t2++) {
						printf(" %" PRId32,
						    lb_array[incr++]);
					}
					printf("\n");
				}
				panic("lfs_shellsort: inconsistent input");
			}
		}
	}
#endif

	for (incrp = __rsshell_increments; (incr = *incrp++) != 0;)
		for (t1 = incr; t1 < nmemb; ++t1)
			for (t2 = t1 - incr; t2 >= 0;)
				if ((u_int32_t)bp_array[t2]->b_lblkno >
				    (u_int32_t)bp_array[t2 + incr]->b_lblkno) {
					bp_temp = bp_array[t2];
					bp_array[t2] = bp_array[t2 + incr];
					bp_array[t2 + incr] = bp_temp;
					t2 -= incr;
				} else
					break;

	/* Reform the list of logical blocks */
	incr = 0;
	for (t1 = 0; t1 < nmemb; t1++) {
		for (t2 = 0; t2 * size < bp_array[t1]->b_bcount; t2++) {
			lb_array[incr++] = bp_array[t1]->b_lblkno + t2;
		}
	}
}

/*
 * Check VXLOCK.  Return 1 if the vnode is locked.  Otherwise, vget it.
 */
int
lfs_vref(struct vnode *vp)
{
	/*
	 * If we return 1 here during a flush, we risk vinvalbuf() not
	 * being able to flush all of the pages from this vnode, which
	 * will cause it to panic.  So, return 0 if a flush is in progress.
	 */
	if (vp->v_flag & VXLOCK) {
		if (IS_FLUSHING(VTOI(vp)->i_lfs,vp)) {
			return 0;
		}
		return (1);
	}
	return (vget(vp, 0));
}

/*
 * This is vrele except that we do not want to VOP_INACTIVE this vnode. We
 * inline vrele here to avoid the vn_lock and VOP_INACTIVE call at the end.
 */
void
lfs_vunref(struct vnode *vp)
{
	/*
	 * Analogous to lfs_vref, if the node is flushing, fake it.
	 */
	if ((vp->v_flag & VXLOCK) && IS_FLUSHING(VTOI(vp)->i_lfs,vp)) {
		return;
	}

	simple_lock(&vp->v_interlock);
#ifdef DIAGNOSTIC
	if (vp->v_usecount <= 0) {
		printf("lfs_vunref: inum is %d\n", VTOI(vp)->i_number);
		printf("lfs_vunref: flags are 0x%lx\n", (u_long)vp->v_flag);
		printf("lfs_vunref: usecount = %ld\n", (long)vp->v_usecount);
		panic("lfs_vunref: v_usecount<0");
	}
#endif
	vp->v_usecount--;
	if (vp->v_usecount > 0) {
		simple_unlock(&vp->v_interlock);
		return;
	}
	/*
	 * insert at tail of LRU list
	 */
	simple_lock(&vnode_free_list_slock);
	if (vp->v_holdcnt > 0)
		TAILQ_INSERT_TAIL(&vnode_hold_list, vp, v_freelist);
	else
		TAILQ_INSERT_TAIL(&vnode_free_list, vp, v_freelist);
	simple_unlock(&vnode_free_list_slock);
	simple_unlock(&vp->v_interlock);
}

/*
 * We use this when we have vnodes that were loaded in solely for cleaning.
 * There is no reason to believe that these vnodes will be referenced again
 * soon, since the cleaning process is unrelated to normal filesystem
 * activity.  Putting cleaned vnodes at the tail of the list has the effect
 * of flushing the vnode LRU.  So, put vnodes that were loaded only for
 * cleaning at the head of the list, instead.
 */
void
lfs_vunref_head(struct vnode *vp)
{
	simple_lock(&vp->v_interlock);
#ifdef DIAGNOSTIC
	if (vp->v_usecount == 0) {
		panic("lfs_vunref: v_usecount<0");
	}
#endif
	vp->v_usecount--;
	if (vp->v_usecount > 0) {
		simple_unlock(&vp->v_interlock);
		return;
	}
	/*
	 * insert at head of LRU list
	 */
	simple_lock(&vnode_free_list_slock);
	if (vp->v_holdcnt > 0)
		TAILQ_INSERT_TAIL(&vnode_hold_list, vp, v_freelist);
	else
		TAILQ_INSERT_HEAD(&vnode_free_list, vp, v_freelist);
	simple_unlock(&vnode_free_list_slock);
	simple_unlock(&vp->v_interlock);
}

