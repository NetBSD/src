/*	$NetBSD: lfs_balloc.c,v 1.70.12.4 2017/12/03 11:39:22 jdolecek Exp $	*/

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
 * Copyright (c) 1989, 1991, 1993
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
 *	@(#)lfs_balloc.c	8.4 (Berkeley) 5/8/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lfs_balloc.c,v 1.70.12.4 2017/12/03 11:39:22 jdolecek Exp $");

#if defined(_KERNEL_OPT)
#include "opt_quota.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/resourcevar.h>
#include <sys/tree.h>
#include <sys/trace.h>
#include <sys/kauth.h>

#include <miscfs/specfs/specdev.h>

#include <ufs/lfs/ulfs_quotacommon.h>
#include <ufs/lfs/ulfs_inode.h>
#include <ufs/lfs/ulfsmount.h>
#include <ufs/lfs/ulfs_extern.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_accessors.h>
#include <ufs/lfs/lfs_extern.h>
#include <ufs/lfs/lfs_kernel.h>

#include <uvm/uvm.h>

static int lfs_fragextend(struct vnode *, int, int, daddr_t, struct buf **,
			  kauth_cred_t);

u_int64_t locked_fakequeue_count;

/*
 * Allocate a block, and do inode and filesystem block accounting for
 * it and for any indirect blocks that may need to be created in order
 * to handle this block.
 *
 * Blocks which have never been accounted for (i.e., which "do not
 * exist") have disk address 0, which is translated by ulfs_bmap to
 * the special value UNASSIGNED == -1, as in historical FFS-related
 * code.
 *
 * Blocks which have been accounted for but which have not yet been
 * written to disk are given the new special disk address UNWRITTEN ==
 * -2, so that they can be differentiated from completely new blocks.
 *
 * Note: it seems that bpp is passed as NULL for blocks that are file
 * pages that will be handled by UVM and not the buffer cache.
 *
 * XXX: locking?
 */
/* VOP_BWRITE ULFS_NIADDR+2 times */
int
lfs_balloc(struct vnode *vp, off_t startoffset, int iosize, kauth_cred_t cred,
    int flags, struct buf **bpp)
{
	int offset;
	daddr_t daddr, idaddr;
	struct buf *ibp, *bp;
	struct inode *ip;
	struct lfs *fs;
	struct indir indirs[ULFS_NIADDR+2], *idp;
	daddr_t	lbn, lastblock;
	int bcount;
	int error, frags, i, nsize, osize, num;

	ip = VTOI(vp);
	fs = ip->i_lfs;

	/* Declare to humans that we might have the seglock here */
	ASSERT_MAYBE_SEGLOCK(fs);


	/* offset within block */
	offset = lfs_blkoff(fs, startoffset);

	/* This is usually but not always exactly the block size */
	KASSERT(iosize <= lfs_sb_getbsize(fs));

	/* block number (within file) */
	lbn = lfs_lblkno(fs, startoffset);

	/*
	 * This checks for whether pending stuff needs to be flushed
	 * out and potentially waits. It's been disabled since UBC
	 * support was added to LFS in 2003. -- dholland 20160806
	 */
	/* (void)lfs_check(vp, lbn, 0); */


	/*
	 * Three cases: it's a block beyond the end of file, it's a block in
	 * the file that may or may not have been assigned a disk address or
	 * we're writing an entire block.
	 *
	 * Note, if the daddr is UNWRITTEN, the block already exists in
	 * the cache (it was read or written earlier).	If so, make sure
	 * we don't count it as a new block or zero out its contents. If
	 * it did not, make sure we allocate any necessary indirect
	 * blocks.
	 *
	 * If we are writing a block beyond the end of the file, we need to
	 * check if the old last block was a fragment.	If it was, we need
	 * to rewrite it.
	 */

	if (bpp)
		*bpp = NULL;

	/* Last block number in file */
	lastblock = lfs_lblkno(fs, ip->i_size);

	if (lastblock < ULFS_NDADDR && lastblock < lbn) {
		/*
		 * The file is small enough to have fragments, and we're
		 * allocating past EOF.
		 *
		 * If the last block was a fragment we need to rewrite it
		 * as a full block.
		 */
		osize = lfs_blksize(fs, ip, lastblock);
		if (osize < lfs_sb_getbsize(fs) && osize > 0) {
			if ((error = lfs_fragextend(vp, osize, lfs_sb_getbsize(fs),
						    lastblock,
						    (bpp ? &bp : NULL), cred)))
				return (error);
			/* Update the file size with what we just did (only) */
			ip->i_size = (lastblock + 1) * lfs_sb_getbsize(fs);
			lfs_dino_setsize(fs, ip->i_din, ip->i_size);
			uvm_vnp_setsize(vp, ip->i_size);
			ip->i_state |= IN_CHANGE | IN_UPDATE;
			/* if we got a buffer for this, write it out now */
			if (bpp)
				(void) VOP_BWRITE(bp->b_vp, bp);
		}
	}

	/*
	 * If the block we are writing is a direct block, it's the last
	 * block in the file, and offset + iosize is less than a full
	 * block, we can write one or more fragments.  There are two cases:
	 * the block is brand new and we should allocate it the correct
	 * size or it already exists and contains some fragments and
	 * may need to extend it.
	 */
	if (lbn < ULFS_NDADDR && lfs_lblkno(fs, ip->i_size) <= lbn) {
		osize = lfs_blksize(fs, ip, lbn);
		nsize = lfs_fragroundup(fs, offset + iosize);
		if (lfs_lblktosize(fs, lbn) >= ip->i_size) {
			/* Brand new block or fragment */
			frags = lfs_numfrags(fs, nsize);
			if (!ISSPACE(fs, frags, cred))
				return ENOSPC;
			if (bpp) {
				*bpp = bp = getblk(vp, lbn, nsize, 0, 0);
				bp->b_blkno = UNWRITTEN;
				if (flags & B_CLRBUF)
					clrbuf(bp);
			}

			/*
			 * Update the effective block count (this count
			 * includes blocks that don't have an on-disk
			 * presence or location yet)
			 */
			ip->i_lfs_effnblks += frags;

			/* account for the space we're taking */
			mutex_enter(&lfs_lock);
			lfs_sb_subbfree(fs, frags);
			mutex_exit(&lfs_lock);

			/* update the inode */
			lfs_dino_setdb(fs, ip->i_din, lbn, UNWRITTEN);
		} else {
			/* extending a block that already has fragments */

			if (nsize <= osize) {
				/* No need to extend */
				if (bpp && (error = bread(vp, lbn, osize,
				    0, &bp)))
					return error;
			} else {
				/* Extend existing block */
				if ((error =
				     lfs_fragextend(vp, osize, nsize, lbn,
						    (bpp ? &bp : NULL), cred)))
					return error;
			}
			if (bpp)
				*bpp = bp;
		}
		return 0;
	}

	/*
	 * Look up what's already here.
	 */

	error = ulfs_bmaparray(vp, lbn, &daddr, &indirs[0], &num, NULL, NULL);
	if (error)
		return (error);

	KASSERT(daddr <= LFS_MAX_DADDR(fs));

	/*
	 * Do byte accounting all at once, so we can gracefully fail *before*
	 * we start assigning blocks.
	 */
	frags = fs->um_seqinc;
	bcount = 0; /* number of frags we need */
	if (daddr == UNASSIGNED) {
		/* no block yet, going to need a whole block */
		bcount = frags;
	}
	for (i = 1; i < num; ++i) {
		if (!indirs[i].in_exists) {
			/* need an indirect block at this level */
			bcount += frags;
		}
	}
	if (ISSPACE(fs, bcount, cred)) {
		/* update the superblock's free block count */
		mutex_enter(&lfs_lock);
		lfs_sb_subbfree(fs, bcount);
		mutex_exit(&lfs_lock);
		/* update the file's effective block count */
		ip->i_lfs_effnblks += bcount;
	} else {
		/* whoops, no can do */
		return ENOSPC;
	}

	if (daddr == UNASSIGNED) {
		/*
		 * There is nothing here yet.
		 */

		/*
		 * If there's no indirect block in the inode, change it
		 * to UNWRITTEN to indicate that it exists but doesn't
		 * have an on-disk address yet.
		 *
		 * (Question: where's the block data initialized?)
		 */
		if (num > 0 && lfs_dino_getib(fs, ip->i_din, indirs[0].in_off) == 0) {
			lfs_dino_setib(fs, ip->i_din, indirs[0].in_off, UNWRITTEN);
		}

		/*
		 * If we need more layers of indirect blocks, create what
		 * we need.
		 */
		if (num > 1) {
			/*
			 * The outermost indirect block address is the one
			 * in the inode, so fetch that.
			 */
			idaddr = lfs_dino_getib(fs, ip->i_din, indirs[0].in_off);
			/*
			 * For each layer of indirection...
			 */
			for (i = 1; i < num; ++i) {
				/*
				 * Get a buffer for the indirect block data.
				 *
				 * (XXX: the logic here seems twisted. What's
				 * wrong with testing in_exists first and then
				 * doing either bread or getblk to get a
				 * buffer?)
				 */
				ibp = getblk(vp, indirs[i].in_lbn,
				    lfs_sb_getbsize(fs), 0,0);
				if (!indirs[i].in_exists) {
					/*
					 * There isn't actually a block here,
					 * so clear the buffer data and mark
					 * the address of the block as
					 * UNWRITTEN.
					 */
					clrbuf(ibp);
					ibp->b_blkno = UNWRITTEN;
				} else if (!(ibp->b_oflags & (BO_DELWRI | BO_DONE))) {
					/*
					 * Otherwise read it in.
					 */
					ibp->b_blkno = LFS_FSBTODB(fs, idaddr);
					ibp->b_flags |= B_READ;
					VOP_STRATEGY(vp, ibp);
					biowait(ibp);
				}

				/*
				 * Now this indirect block exists, but
				 * the next one down may not yet. If
				 * so, set it to UNWRITTEN. This keeps
				 * the accounting straight.
				 */
				if (lfs_iblock_get(fs, ibp->b_data, indirs[i].in_off) == 0)
					lfs_iblock_set(fs, ibp->b_data, indirs[i].in_off,
						UNWRITTEN);

				/* get the block for the next iteration */
				idaddr = lfs_iblock_get(fs, ibp->b_data, indirs[i].in_off);

				if (vp == fs->lfs_ivnode) {
					LFS_ENTER_LOG("balloc", __FILE__,
						__LINE__, indirs[i].in_lbn,
						ibp->b_flags, curproc->p_pid);
				}
				/*
				 * Write out the updated indirect block. Note
				 * that this writes it out even if we didn't
				 * modify it - ultimately because the final
				 * block didn't exist we'll need to write a
				 * new version of all the blocks that lead to
				 * it. Hopefully all that gets in before any
				 * actual disk I/O so we don't end up writing
				 * any of them twice... this is currently not
				 * very clear.
				 */
				if ((error = VOP_BWRITE(ibp->b_vp, ibp)))
					return error;
			}
		}
	}


	/*
	 * Get the existing block from the cache, if requested.
	 */
	if (bpp)
		*bpp = bp = getblk(vp, lbn, lfs_blksize(fs, ip, lbn), 0, 0);

	/*
	 * Do accounting on blocks that represent pages.
	 */
	if (!bpp)
		lfs_register_block(vp, lbn);

	/*
	 * The block we are writing may be a brand new block
	 * in which case we need to do accounting.
	 *
	 * We can tell a truly new block because ulfs_bmaparray will say
	 * it is UNASSIGNED. Once we allocate it we will assign it the
	 * disk address UNWRITTEN.
	 */
	if (daddr == UNASSIGNED) {
		if (bpp) {
			if (flags & B_CLRBUF)
				clrbuf(bp);

			/* Note the new address */
			bp->b_blkno = UNWRITTEN;
		}

		switch (num) {
		    case 0:
			/* direct block - update the inode */
			lfs_dino_setdb(fs, ip->i_din, lbn, UNWRITTEN);
			break;
		    case 1:
			/*
			 * using a single indirect block - update the inode
			 *
			 * XXX: is this right? We already set this block
			 * pointer above. I think we want to be writing *in*
			 * the single indirect block and this case shouldn't
			 * exist. (just case 0 and default)
			 *    -- dholland 20160806
			 */
			lfs_dino_setib(fs, ip->i_din, indirs[0].in_off, UNWRITTEN);
			break;
		    default:
			/*
			 * using multiple indirect blocks - update the
			 * innermost one
			 */
			idp = &indirs[num - 1];
			if (bread(vp, idp->in_lbn, lfs_sb_getbsize(fs),
				  B_MODIFY, &ibp))
				panic("lfs_balloc: bread bno %lld",
				    (long long)idp->in_lbn);
			lfs_iblock_set(fs, ibp->b_data, idp->in_off, UNWRITTEN);

			if (vp == fs->lfs_ivnode) {
				LFS_ENTER_LOG("balloc", __FILE__,
					__LINE__, idp->in_lbn,
					ibp->b_flags, curproc->p_pid);
			}

			VOP_BWRITE(ibp->b_vp, ibp);
		}
	} else if (bpp && !(bp->b_oflags & (BO_DONE|BO_DELWRI))) {
		/*
		 * Not a brand new block, also not in the cache;
		 * read it in from disk.
		 */
		if (iosize == lfs_sb_getbsize(fs))
			/* Optimization: I/O is unnecessary. */
			bp->b_blkno = daddr;
		else {
			/*
			 * We need to read the block to preserve the
			 * existing bytes.
			 */
			bp->b_blkno = daddr;
			bp->b_flags |= B_READ;
			VOP_STRATEGY(vp, bp);
			return (biowait(bp));
		}
	}

	return (0);
}

/*
 * Extend a file that uses fragments with more fragments.
 *
 * XXX: locking?
 */
/* VOP_BWRITE 1 time */
static int
lfs_fragextend(struct vnode *vp, int osize, int nsize, daddr_t lbn,
    struct buf **bpp, kauth_cred_t cred)
{
	struct inode *ip;
	struct lfs *fs;
	long frags;
	int error;
	size_t obufsize;

	/* XXX move this to a header file */
	/* (XXX: except it's not clear what purpose it serves) */
	extern long locked_queue_bytes;

	ip = VTOI(vp);
	fs = ip->i_lfs;

	/*
	 * XXX: is there some reason we know more about the seglock
	 * state here than at the top of lfs_balloc?
	 */
	ASSERT_NO_SEGLOCK(fs);

	/* number of frags we're adding */
	frags = (long)lfs_numfrags(fs, nsize - osize);

	error = 0;

	/*
	 * Get the seglock so we don't enlarge blocks while a segment
	 * is being written.  If we're called with bpp==NULL, though,
	 * we are only pretending to change a buffer, so we don't have to
	 * lock.
	 *
	 * XXX: the above comment is lying, as fs->lfs_fraglock is not
	 * the segment lock.
	 */
    top:
	if (bpp) {
		rw_enter(&fs->lfs_fraglock, RW_READER);
	}

	/* check if we actually have enough frags available */
	if (!ISSPACE(fs, frags, cred)) {
		error = ENOSPC;
		goto out;
	}

	/*
	 * If we are not asked to actually return the block, all we need
	 * to do is allocate space for it.  UBC will handle dirtying the
	 * appropriate things and making sure it all goes to disk.
	 * Don't bother to read in that case.
	 */
	if (bpp && (error = bread(vp, lbn, osize, 0, bpp))) {
		goto out;
	}
#if defined(LFS_QUOTA) || defined(LFS_QUOTA2)
	if ((error = lfs_chkdq(ip, frags, cred, 0))) {
		if (bpp)
			brelse(*bpp, 0);
		goto out;
	}
#endif
	/*
	 * Adjust accounting for lfs_avail.  If there's not enough room,
	 * we will have to wait for the cleaner, which we can't do while
	 * holding a block busy or while holding the seglock.  In that case,
	 * release both and start over after waiting.
	 */

	if (bpp && ((*bpp)->b_oflags & BO_DELWRI)) {
		if (!lfs_fits(fs, frags)) {
			if (bpp)
				brelse(*bpp, 0);
#if defined(LFS_QUOTA) || defined(LFS_QUOTA2)
			lfs_chkdq(ip, -frags, cred, 0);
#endif
			rw_exit(&fs->lfs_fraglock);
			lfs_availwait(fs, frags);
			goto top;
		}
		lfs_sb_subavail(fs, frags);
	}

	/* decrease the free block count in the superblock */
	mutex_enter(&lfs_lock);
	lfs_sb_subbfree(fs, frags);
	mutex_exit(&lfs_lock);
	/* increase the file's effective block count */
	ip->i_lfs_effnblks += frags;
	/* mark the inode dirty */
	ip->i_state |= IN_CHANGE | IN_UPDATE;

	if (bpp) {
		obufsize = (*bpp)->b_bufsize;
		allocbuf(*bpp, nsize, 1);

		/* Adjust locked-list accounting */
		if (((*bpp)->b_flags & B_LOCKED) != 0 &&
		    (*bpp)->b_iodone == NULL) {
			mutex_enter(&lfs_lock);
			locked_queue_bytes += (*bpp)->b_bufsize - obufsize;
			mutex_exit(&lfs_lock);
		}

		/* zero the new space */
		memset((char *)((*bpp)->b_data) + osize, 0, (u_int)(nsize - osize));
	}

    out:
	if (bpp) {
		rw_exit(&fs->lfs_fraglock);
	}
	return (error);
}

static inline int
lge(struct lbnentry *a, struct lbnentry *b)
{
	return a->lbn - b->lbn;
}

SPLAY_PROTOTYPE(lfs_splay, lbnentry, entry, lge);

SPLAY_GENERATE(lfs_splay, lbnentry, entry, lge);

/*
 * Record this lbn as being "write pending".  We used to have this information
 * on the buffer headers, but since pages don't have buffer headers we
 * record it here instead.
 */
void
lfs_register_block(struct vnode *vp, daddr_t lbn)
{
	struct lfs *fs;
	struct inode *ip;
	struct lbnentry *lbp;

	ip = VTOI(vp);

	/* Don't count metadata */
	if (lbn < 0 || vp->v_type != VREG || ip->i_number == LFS_IFILE_INUM)
		return;

	fs = ip->i_lfs;

	ASSERT_NO_SEGLOCK(fs);

	/* If no space, wait for the cleaner */
	lfs_availwait(fs, lfs_btofsb(fs, 1 << lfs_sb_getbshift(fs)));

	lbp = (struct lbnentry *)pool_get(&lfs_lbnentry_pool, PR_WAITOK);
	lbp->lbn = lbn;
	mutex_enter(&lfs_lock);
	if (SPLAY_INSERT(lfs_splay, &ip->i_lfs_lbtree, lbp) != NULL) {
		mutex_exit(&lfs_lock);
		/* Already there */
		pool_put(&lfs_lbnentry_pool, lbp);
		return;
	}

	++ip->i_lfs_nbtree;
	fs->lfs_favail += lfs_btofsb(fs, (1 << lfs_sb_getbshift(fs)));
	fs->lfs_pages += lfs_sb_getbsize(fs) >> PAGE_SHIFT;
	++locked_fakequeue_count;
	lfs_subsys_pages += lfs_sb_getbsize(fs) >> PAGE_SHIFT;
	mutex_exit(&lfs_lock);
}

static void
lfs_do_deregister(struct lfs *fs, struct inode *ip, struct lbnentry *lbp)
{
	ASSERT_MAYBE_SEGLOCK(fs);

	mutex_enter(&lfs_lock);
	--ip->i_lfs_nbtree;
	SPLAY_REMOVE(lfs_splay, &ip->i_lfs_lbtree, lbp);
	if (fs->lfs_favail > lfs_btofsb(fs, (1 << lfs_sb_getbshift(fs))))
		fs->lfs_favail -= lfs_btofsb(fs, (1 << lfs_sb_getbshift(fs)));
	fs->lfs_pages -= lfs_sb_getbsize(fs) >> PAGE_SHIFT;
	if (locked_fakequeue_count > 0)
		--locked_fakequeue_count;
	lfs_subsys_pages -= lfs_sb_getbsize(fs) >> PAGE_SHIFT;
	mutex_exit(&lfs_lock);

	pool_put(&lfs_lbnentry_pool, lbp);
}

void
lfs_deregister_block(struct vnode *vp, daddr_t lbn)
{
	struct lfs *fs;
	struct inode *ip;
	struct lbnentry *lbp;
	struct lbnentry tmp;

	ip = VTOI(vp);

	/* Don't count metadata */
	if (lbn < 0 || vp->v_type != VREG || ip->i_number == LFS_IFILE_INUM)
		return;

	fs = ip->i_lfs;
	tmp.lbn = lbn;
	lbp = SPLAY_FIND(lfs_splay, &ip->i_lfs_lbtree, &tmp);
	if (lbp == NULL)
		return;

	lfs_do_deregister(fs, ip, lbp);
}

void
lfs_deregister_all(struct vnode *vp)
{
	struct lbnentry *lbp, *nlbp;
	struct lfs_splay *hd;
	struct lfs *fs;
	struct inode *ip;

	ip = VTOI(vp);
	fs = ip->i_lfs;
	hd = &ip->i_lfs_lbtree;

	for (lbp = SPLAY_MIN(lfs_splay, hd); lbp != NULL; lbp = nlbp) {
		nlbp = SPLAY_NEXT(lfs_splay, hd, lbp);
		lfs_do_deregister(fs, ip, lbp);
	}
}
