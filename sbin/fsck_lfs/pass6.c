/* $NetBSD: pass6.c,v 1.3 2003/04/02 10:39:28 fvdl Exp $	 */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/buf.h>
#include <sys/mount.h>

#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#define vnode uvnode
#include <ufs/lfs/lfs.h>
#undef vnode

#include <assert.h>
#include <err.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "bufcache.h"
#include "vnode.h"
#include "lfs.h"
#include "segwrite.h"

#include "fsck.h"
#include "extern.h"
#include "fsutil.h"

extern u_int32_t cksum(void *, size_t);
extern u_int32_t lfs_sb_cksum(struct dlfs *);

extern ufs_daddr_t badblk;
extern SEGUSE *seg_table;

/*
 * Our own copy of lfs_update_single so we can account in seg_table
 * as well as the Ifile; and so we can add the blocks to their new
 * segment.
 *
 * Change the given block's address to ndaddr, finding its previous
 * location using ufs_bmaparray().
 *
 * Account for this change in the segment table.
 */
static void
rfw_update_single(struct uvnode *vp, daddr_t lbn, ufs_daddr_t ndaddr, int size)
{
	SEGUSE *sup;
	struct ubuf *bp;
	struct indir a[NIADDR + 2], *ap;
	struct inode *ip;
	daddr_t daddr, ooff;
	int num, error;
	int i, bb, osize, obb;
	u_int32_t oldsn, sn;

	ip = VTOI(vp);

	error = ufs_bmaparray(fs, vp, lbn, &daddr, a, &num);
	if (error)
		errx(1, "lfs_updatemeta: ufs_bmaparray returned %d"
		     " looking up lbn %" PRId64 "\n", error, lbn);
	if (daddr > 0)
		daddr = dbtofsb(fs, daddr);

	bb = fragstofsb(fs, numfrags(fs, size));
	switch (num) {
	case 0:
		ooff = ip->i_ffs1_db[lbn];
		if (ooff <= 0)
			ip->i_ffs1_blocks += bb;
		else {
			/* possible fragment truncation or extension */
			obb = btofsb(fs, ip->i_lfs_fragsize[lbn]);
			ip->i_ffs1_blocks += (bb - obb);
		}
		ip->i_ffs1_db[lbn] = ndaddr;
		break;
	case 1:
		ooff = ip->i_ffs1_ib[a[0].in_off];
		if (ooff <= 0)
			ip->i_ffs1_blocks += bb;
		ip->i_ffs1_ib[a[0].in_off] = ndaddr;
		break;
	default:
		ap = &a[num - 1];
		if (bread(vp, ap->in_lbn, fs->lfs_bsize, NULL, &bp))
			errx(1, "lfs_updatemeta: bread bno %" PRId64,
			    ap->in_lbn);

		ooff = ((ufs_daddr_t *) bp->b_data)[ap->in_off];
		if (ooff <= 0)
			ip->i_ffs1_blocks += bb;
		((ufs_daddr_t *) bp->b_data)[ap->in_off] = ndaddr;
		(void) VOP_BWRITE(bp);
	}

	/*
	 * Update segment usage information, based on old size
	 * and location.
	 */
	if (daddr > 0) {
		oldsn = dtosn(fs, daddr);
		if (lbn >= 0 && lbn < NDADDR)
			osize = ip->i_lfs_fragsize[lbn];
		else
			osize = fs->lfs_bsize;
		LFS_SEGENTRY(sup, fs, oldsn, bp);
		seg_table[oldsn].su_nbytes -= osize;
		sup->su_nbytes -= osize;
		if (!(bp->b_flags & B_GATHERED))
			fs->lfs_flags |= LFS_IFDIRTY;
		LFS_WRITESEGENTRY(sup, fs, oldsn, bp);
		for (i = 0; i < btofsb(fs, osize); i++)
			clrbmap(daddr + i);
	}

	/* Add block to its new segment */
	sn = dtosn(fs, ndaddr);
	LFS_SEGENTRY(sup, fs, sn, bp);
	seg_table[sn].su_nbytes += size;
	sup->su_nbytes += size;
	if (!(bp->b_flags & B_GATHERED))
		fs->lfs_flags |= LFS_IFDIRTY;
	LFS_WRITESEGENTRY(sup, fs, sn, bp);
	for (i = 0; i < btofsb(fs, size); i++)
		setbmap(daddr + i);

	/* Check bfree accounting as well */
	if (daddr < 0) {
		fs->lfs_bfree -= btofsb(fs, size);
	} else if (size != osize) {
		fs->lfs_bfree -= (bb - obb);
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
 * Remove the vnode from the cache, including any blocks it
 * may hold.  Account the blocks.  Finally account the removal
 * of the inode from its segment.
 */
static void
remove_ino(struct uvnode *vp, ino_t ino)
{
	IFILE *ifp;
	SEGUSE *sup;
	struct ubuf *bp, *sbp;
	struct inodesc idesc;
	ufs_daddr_t daddr;
	int obfree;

	obfree = fs->lfs_bfree;
	LFS_IENTRY(ifp, fs, ino, bp);
	daddr = ifp->if_daddr;
	brelse(bp);

	if (vp == NULL && daddr > 0) {
		vp = lfs_raw_vget(fs, ino, fs->lfs_ivnode->v_fd, daddr);
	}

	if (daddr > 0) {
		LFS_SEGENTRY(sup, fs, dtosn(fs, ifp->if_daddr), sbp);
		sup->su_nbytes -= DINODE1_SIZE;
		VOP_BWRITE(sbp);
		seg_table[dtosn(fs, ifp->if_daddr)].su_nbytes -= DINODE1_SIZE;
	}

	/* Do on-disk accounting */
	if (vp) {
		idesc.id_number = ino;
		idesc.id_func = pass4check; /* Delete dinode and blocks */
		idesc.id_type = ADDR;
		idesc.id_lblkno = 0;
		clri(&idesc, "unknown", 2); /* XXX magic number 2 */

		/* Get rid of this vnode for good */
		vnode_destroy(vp);
	}
}

/*
 * Use FIP records to update blocks, if the generation number matches.
 */
static void
pass6harvest(ufs_daddr_t daddr, FINFO *fip)
{
	struct uvnode *vp;
	int i;
	size_t size;

	vp = vget(fs, fip->fi_ino);
	if (vp && vp != fs->lfs_ivnode &&
	    VTOI(vp)->i_ffs1_gen == fip->fi_version) {
		for (i = 0; i < fip->fi_nblocks; i++) {
			size = (i == fip->fi_nblocks - 1 ?
				fip->fi_lastlength : fs->lfs_bsize);
			rfw_update_single(vp, fip->fi_blocks[i], daddr, size);
			daddr += btofsb(fs, size);
		}
	}
}

/*
 * Check validity of blocks on roll-forward inodes.
 */
int
pass6check(struct inodesc * idesc)
{
	int i, sn, anyout, anynew;

	/* Check that the blocks do not lie within clean segments. */
	anyout = anynew = 0;
	for (i = 0; i < fragstofsb(fs, idesc->id_numfrags); i++) {
		sn = dtosn(fs, idesc->id_blkno + i);
		if (sn < 0 || sn >= fs->lfs_nseg ||
		    (seg_table[sn].su_flags & SEGUSE_DIRTY) == 0) {
			anyout = 1;
			break;
		}
		if (seg_table[sn].su_flags & SEGUSE_ACTIVE) {
			if (sn != dtosn(fs, fs->lfs_offset) ||
			    idesc->id_blkno > fs->lfs_offset) {
				++anynew;
			}
		}
		if (!anynew) {
			/* Clear so pass1check won't be surprised */
			clrbmap(idesc->id_blkno + i);
			seg_table[sn].su_nbytes -= fsbtob(fs, 1);
		}
	}
	if (anyout) {
		blkerror(idesc->id_number, "BAD", idesc->id_blkno);
		if (badblk++ >= MAXBAD) {
			pwarn("EXCESSIVE BAD BLKS I=%u",
			      idesc->id_number);
			if (preen)
				pwarn(" (SKIPPING)\n");
			else if (reply("CONTINUE") == 0)
				err(8, "%s", "");
			return (STOP);
		}
	}

	return pass1check(idesc);
}

/*
 * Add a new block to the Ifile, to accommodate future file creations.
 */
static int
extend_ifile(void)
{
	struct uvnode *vp;
	struct inode *ip;
	IFILE *ifp;
	IFILE_V1 *ifp_v1;
	struct ubuf *bp, *cbp;
	daddr_t i, blkno, max;
	ino_t oldlast;
	CLEANERINFO *cip;

	vp = fs->lfs_ivnode;
	ip = VTOI(vp);
	blkno = lblkno(fs, ip->i_ffs1_size);

	bp = getblk(vp, blkno, fs->lfs_bsize);	/* XXX VOP_BALLOC() */
	ip->i_ffs1_size += fs->lfs_bsize;
	
	i = (blkno - fs->lfs_segtabsz - fs->lfs_cleansz) *
		fs->lfs_ifpb;
	LFS_GET_HEADFREE(fs, cip, cbp, &oldlast);
	LFS_PUT_HEADFREE(fs, cip, cbp, i);
	max = i + fs->lfs_ifpb;
	maxino = max;
	fs->lfs_bfree -= btofsb(fs, fs->lfs_bsize);

	if (fs->lfs_version == 1) {
		for (ifp_v1 = (IFILE_V1 *)bp->b_data; i < max; ++ifp_v1) {
			ifp_v1->if_version = 1;
			ifp_v1->if_daddr = LFS_UNUSED_DADDR;
			ifp_v1->if_nextfree = ++i;
		}
		ifp_v1--;
		ifp_v1->if_nextfree = oldlast;
	} else {
		for (ifp = (IFILE *)bp->b_data; i < max; ++ifp) {
			ifp->if_version = 1;
			ifp->if_daddr = LFS_UNUSED_DADDR;
			ifp->if_nextfree = ++i;
		}
		ifp--;
		ifp->if_nextfree = oldlast;
	}
	LFS_PUT_TAILFREE(fs, cip, cbp, max - 1);

	LFS_BWRITE_LOG(bp);

	return 0;
}

/*
 * Give a previously allocated inode a new address; do segment
 * accounting if necessary.
 *
 * Caller has ensured that this inode is not on the free list, so no
 * free list accounting is done.
 */
static void
readdress_inode(ino_t thisino, ufs_daddr_t daddr)
{
	IFILE *ifp;
	SEGUSE *sup;
	struct ubuf *bp;
	int sn;
	ufs_daddr_t odaddr;

	LFS_IENTRY(ifp, fs, thisino, bp);
	odaddr = ifp->if_daddr;
	ifp->if_daddr = daddr;
	VOP_BWRITE(bp);

	sn = dtosn(fs, odaddr);
	LFS_SEGENTRY(sup, fs, sn, bp);
	sup->su_nbytes -= DINODE1_SIZE;
	VOP_BWRITE(bp);
	seg_table[sn].su_nbytes -= DINODE1_SIZE;

	sn = dtosn(fs, daddr);
	LFS_SEGENTRY(sup, fs, sn, bp);
	sup->su_nbytes += DINODE1_SIZE;
	VOP_BWRITE(bp);
	seg_table[sn].su_nbytes += DINODE1_SIZE;
}

/*
 * Allocate the given inode from the free list.
 */
static void
alloc_inode(ino_t thisino, ufs_daddr_t daddr)
{
	ino_t ino, nextfree;
	IFILE *ifp;
	SEGUSE *sup;
	struct ubuf *bp;

	LFS_IENTRY(ifp, fs, thisino, bp);
	nextfree = ifp->if_nextfree;
	ifp->if_nextfree = 0;
	ifp->if_daddr = daddr;
	VOP_BWRITE(bp);

	while (thisino > (lblkno(fs, VTOI(fs->lfs_ivnode)->i_ffs1_size) -
			  fs->lfs_segtabsz - fs->lfs_cleansz + 1) *
	       fs->lfs_ifpb) {
		extend_ifile();
	}

	if (fs->lfs_freehd == thisino) {
		fs->lfs_freehd = nextfree;
		sbdirty();
		if (nextfree == 0) {
			extend_ifile();
		}
	} else {
		ino = fs->lfs_freehd;
		while (ino) {
			LFS_IENTRY(ifp, fs, ino, bp);
			assert(ifp->if_nextfree != ino);
			if (ifp->if_nextfree == thisino) {
				ifp->if_nextfree = nextfree;
				VOP_BWRITE(bp);
				break;
			} else
				ino = ifp->if_nextfree;
			brelse(bp);
		}
	}
	
	/* Account for new location */
	LFS_SEGENTRY(sup, fs, dtosn(fs, daddr), bp);
	sup->su_nbytes += DINODE1_SIZE;
	VOP_BWRITE(bp);
	seg_table[dtosn(fs, daddr)].su_nbytes += DINODE1_SIZE;
}

/*
 * Roll forward from the last verified checkpoint.
 *
 * Basic strategy:
 *
 * Run through the summaries finding the last valid partial segment.
 * Note segment numbers as we go.  For each inode that we find, compare
 * its generation number; if newer than old inode's (or if old inode is
 * USTATE), change to that inode.  Recursively look at inode blocks that
 * do not have their old disk addresses.  These addresses must lie in
 * segments we have seen already in our roll forward.
 *
 * A second pass through the past-checkpoint area verifies the validity
 * of these new blocks, as well as updating other blocks that do not
 * have corresponding new inodes (but their generation number must match
 * the old generation number).
 */
void
pass6(void)
{
	ufs_daddr_t daddr, ibdaddr, odaddr, lastgood, nextseg, *idaddrp;
	struct uvnode *vp, *devvp;
	CLEANERINFO *cip;
	SEGUSE *sup;
	SEGSUM *sp;
	struct ubuf *bp, *ibp, *sbp, *cbp;
	struct ufs1_dinode *dp;
	struct inodesc idesc;
	int i, j, bc;
	ufs_daddr_t hassuper;

	devvp = fs->lfs_unlockvp;

	/* Find last valid partial segment */
	lastgood = try_verify(fs, devvp, 0, debug);
	if (lastgood == fs->lfs_offset) {
		if (debug)
			pwarn("not rolling forward, nothing to recover\n");
		return;
	}

	if (!preen && reply("roll forward") == 0)
		return;

	if (debug)
		pwarn("rolling forward between %" PRIx32 " and %" PRIx32 "\n",
			fs->lfs_offset, lastgood);
	/*
	 * Pass 1: find inode blocks.  We ignore the Ifile inode but accept
	 * changes to any other inode.
	 */

	daddr = fs->lfs_offset;
	nextseg = fs->lfs_nextseg;
	while (daddr != lastgood) {
		seg_table[dtosn(fs, daddr)].su_flags |= SEGUSE_DIRTY | SEGUSE_ACTIVE;
		LFS_SEGENTRY(sup, fs, dtosn(fs, daddr), sbp);
		sup->su_flags |= SEGUSE_DIRTY;
		VOP_BWRITE(sbp);
		hassuper = 0;
	  oncemore:
		/* Read in summary block */
		bread(devvp, fsbtodb(fs, daddr), fs->lfs_sumsize, NULL, &bp);
		sp = (SEGSUM *)bp->b_data;

		/* Could be a superblock instead of a segment summary. */
		if (sntod(fs, dtosn(fs, daddr)) == daddr &&
		    (sp->ss_magic != SS_MAGIC ||
		     sp->ss_sumsum != cksum(&sp->ss_datasum, fs->lfs_sumsize -
			sizeof(sp->ss_sumsum)))) {
			brelse(bp);
			daddr += btofsb(fs, LFS_SBPAD);
			hassuper = 1;
			goto oncemore;
		}

		/* We have verified that this is a good summary. */
		LFS_SEGENTRY(sup, fs, dtosn(fs, daddr), sbp);
		++sup->su_nsums;
		VOP_BWRITE(sbp);
		fs->lfs_bfree -= btofsb(fs, fs->lfs_sumsize);
		fs->lfs_dmeta += btofsb(fs, fs->lfs_sumsize);
		sbdirty();
		nextseg = sp->ss_next;
		if (sntod(fs, dtosn(fs, daddr)) == daddr +
		    hassuper * btofsb(fs, LFS_SBPAD) &&
		    dtosn(fs, daddr) != dtosn(fs, fs->lfs_offset)) {
			--fs->lfs_nclean;
			sbdirty();
		}

		/* Find inodes, look at generation number. */
		if (sp->ss_ninos) {
			LFS_SEGENTRY(sup, fs, dtosn(fs, daddr), sbp);
			sup->su_ninos += howmany(sp->ss_ninos, INOPB(fs));
			VOP_BWRITE(sbp);
			fs->lfs_dmeta += btofsb(fs, howmany(sp->ss_ninos,
							    INOPB(fs)) *
						fs->lfs_ibsize);
		}
		idaddrp = ((ufs_daddr_t *)((char *)bp->b_data + fs->lfs_sumsize));
		for (i = 0; i < howmany(sp->ss_ninos, INOPB(fs)); i++) {
			ino_t inums[INOPB(fs) + 1];
			
			for (j = 0; j < INOPB(fs) + 1; j++)
				inums[j] = 0;
			ibdaddr = *--idaddrp;
			fs->lfs_bfree -= btofsb(fs, fs->lfs_ibsize);
			sbdirty();
			bread(devvp, fsbtodb(fs, ibdaddr), fs->lfs_ibsize,
			      NOCRED, &ibp);
			j = 0;
			for (dp = (struct ufs1_dinode *)ibp->b_data;
			     dp < (struct ufs1_dinode *)ibp->b_data + INOPB(fs);
			     ++dp) {
				if (dp->di_u.inumber == 0 ||
				    dp->di_u.inumber == fs->lfs_ifile)
					continue;
				/* Basic sanity checks */
				if (dp->di_nlink < 0 ||
				    dp->di_u.inumber < 0 ||
				    dp->di_size < 0) {
					pwarn("bad inode at %" PRIx32 "\n",
						ibdaddr);
					brelse(ibp);
					brelse(bp);
					goto out;
				}

				vp = vget(fs, dp->di_u.inumber);

				/*
				 * Four cases:
				 * (1) Invalid inode (nlink == 0).
				 *     If currently allocated, remove.
				 */
				if (dp->di_nlink == 0) {
					remove_ino(vp, dp->di_u.inumber);
					continue;
				}
				/*
				 * (2) New valid inode, previously free.
				 *     Nothing to do except account
				 *     the inode itself, done after the
				 *     loop.
				 */
				if (vp == NULL) {
					inums[j++] = dp->di_u.inumber;
					continue;
				}
				/*
				 * (3) Valid new version of previously
				 *     allocated inode.  Delete old file
				 *     and proceed as in (2).
				 */
				if (vp && VTOI(vp)->i_ffs1_gen < dp->di_gen) {
					remove_ino(vp, dp->di_u.inumber);
					inums[j++] = dp->di_u.inumber;
					continue;
				}
				/*
				 * (4) Same version of previously
				 *     allocated inode.  Move inode to
				 *     this location, account inode change
				 *     only.  We'll pick up any new
				 *     blocks when we do the block pass.
				 */
				if (vp && VTOI(vp)->i_ffs1_gen == dp->di_gen) {
					readdress_inode(dp->di_u.inumber, ibdaddr);

					/* Update with new info */
					VTOD(vp)->di_mode = dp->di_mode;
					VTOD(vp)->di_nlink = dp->di_nlink;
					/* XXX size is important */
					VTOD(vp)->di_size = dp->di_size;
					VTOD(vp)->di_atime = dp->di_atime;
					VTOD(vp)->di_atimensec = dp->di_atimensec;
					VTOD(vp)->di_mtime = dp->di_mtime;
					VTOD(vp)->di_mtimensec = dp->di_mtimensec;
					VTOD(vp)->di_ctime = dp->di_ctime;
					VTOD(vp)->di_ctimensec = dp->di_ctimensec;
					VTOD(vp)->di_flags = dp->di_flags;
					VTOD(vp)->di_uid = dp->di_uid;
					VTOD(vp)->di_gid = dp->di_gid;
					inodirty(VTOI(vp));
				}
			}
			brelse(ibp);
			for (j = 0; inums[j]; j++) {
				alloc_inode(inums[j], ibdaddr);
				vp = lfs_raw_vget(fs, inums[j],
					      devvp->v_fd, ibdaddr);
				/* We'll get the blocks later */
				memset(VTOD(vp)->di_db, 0, (NDADDR + NIADDR) *
				       sizeof(ufs_daddr_t));
				VTOD(vp)->di_blocks = 0;

				vp->v_flag |= VDIROP;
				inodirty(VTOI(vp));
			}
		}

		bc = check_summary(fs, sp, daddr, debug, devvp, NULL);
		if (bc == 0) {
			brelse(bp);
			break;
		}
		odaddr = daddr;
		daddr += btofsb(fs, fs->lfs_sumsize + bc);
		if (dtosn(fs, odaddr) != dtosn(fs, daddr) ||
		    dtosn(fs, daddr) != dtosn(fs, daddr +
			btofsb(fs, fs->lfs_sumsize + fs->lfs_bsize))) {
			daddr = ((SEGSUM *)bp->b_data)->ss_next;
		}
		brelse(bp);
	}
    out:

	/*
	 * Check our new vnodes.  Any blocks must lie in segments that
	 * we've seen before (SEGUSE_DIRTY or SEGUSE_RFW); and the rest
	 * of the pass 1 checks as well.
	 */
	memset(&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = ADDR;
	idesc.id_func = pass6check;
	idesc.id_lblkno = 0;
	LIST_FOREACH(vp, &vnodelist, v_mntvnodes) {
		if ((vp->v_flag & VDIROP) == 0)
			--n_files; /* Don't double count */
		checkinode(VTOI(vp)->i_number, &idesc);
	}

	/*
	 * Second pass.  Run through FINFO entries looking for blocks
	 * with the same generation number as files we've seen before.
	 * If they have it, pretend like we just wrote them.  We don't
	 * do the pretend-write, though, if we've already seen them
	 * (the accounting would have been done for us already).
	 */
	daddr = fs->lfs_offset;
	while (daddr != lastgood) {
		if (!(seg_table[dtosn(fs, daddr)].su_flags & SEGUSE_DIRTY)) {
			seg_table[dtosn(fs, daddr)].su_flags |= SEGUSE_DIRTY;
			LFS_SEGENTRY(sup, fs, dtosn(fs, daddr), sbp);
			sup->su_flags |= SEGUSE_DIRTY;
			VOP_BWRITE(sbp);
		}
	  oncemore2:
		/* Read in summary block */
		bread(devvp, fsbtodb(fs, daddr), fs->lfs_sumsize, NULL, &bp);
		sp = (SEGSUM *)bp->b_data;

		/* Could be a superblock instead of a segment summary. */
		if (sntod(fs, dtosn(fs, daddr)) == daddr &&
		    (sp->ss_magic != SS_MAGIC ||
		     sp->ss_sumsum != cksum(&sp->ss_datasum, fs->lfs_sumsize -
			sizeof(sp->ss_sumsum)))) {
			brelse(bp);
			daddr += btofsb(fs, LFS_SBPAD);
			goto oncemore2;
		}

		bc = check_summary(fs, sp, daddr, debug, devvp, pass6harvest);
		if (bc == 0) {
			brelse(bp);
			break;
		}
		odaddr = daddr;
		daddr += btofsb(fs, fs->lfs_sumsize + bc);
		fs->lfs_avail -= btofsb(fs, fs->lfs_sumsize + bc);
		if (dtosn(fs, odaddr) != dtosn(fs, daddr) ||
		    dtosn(fs, daddr) != dtosn(fs, daddr +
			btofsb(fs, fs->lfs_sumsize + fs->lfs_bsize))) {
			fs->lfs_avail -= sntod(fs, dtosn(fs, daddr) + 1) - daddr;
			daddr = ((SEGSUM *)bp->b_data)->ss_next;
		}
		LFS_CLEANERINFO(cip, fs, cbp);
		LFS_SYNC_CLEANERINFO(cip, fs, cbp, 0);
		bp->b_flags |= B_AGE;
		brelse(bp);
	}

	/* Update offset to point at correct location */
	fs->lfs_offset = lastgood;
	fs->lfs_curseg = sntod(fs, dtosn(fs, lastgood));
	fs->lfs_nextseg = nextseg;

	if (!preen) {
		/* Run pass 5 again (it's quick anyway). */
		pwarn("** Phase 6b - Recheck Segment Block Accounting\n");
		pass5();
	}
}
