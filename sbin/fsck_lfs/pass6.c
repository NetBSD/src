/* $NetBSD: pass6.c,v 1.24.6.1 2013/02/25 00:28:07 tls Exp $	 */

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
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>

#include "bufcache.h"
#include "vnode.h"
#include "lfs_user.h"
#include "segwrite.h"

#include "fsck.h"
#include "extern.h"
#include "fsutil.h"

extern u_int32_t cksum(void *, size_t);
extern u_int32_t lfs_sb_cksum(struct dlfs *);

extern ufs_daddr_t badblk;
extern SEGUSE *seg_table;

static int nnewblocks;

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
	struct indir a[UFS_NIADDR + 2], *ap;
	struct inode *ip;
	daddr_t daddr, ooff;
	int num, error;
	int i, osize = 0;
	int frags, ofrags = 0;
	u_int32_t oldsn, sn;

	ip = VTOI(vp);
	ip->i_flag |= IN_MODIFIED;

	error = ufs_bmaparray(fs, vp, lbn, &daddr, a, &num);
	if (error)
		errx(1, "lfs_updatemeta: ufs_bmaparray returned %d"
		     " looking up lbn %" PRId64 "\n", error, lbn);
	if (daddr > 0)
		daddr = dbtofsb(fs, daddr);

	frags = numfrags(fs, size);
	switch (num) {
	case 0:
		ooff = ip->i_ffs1_db[lbn];
		if (ooff <= 0)
			ip->i_ffs1_blocks += frags;
		else {
			/* possible fragment truncation or extension */
			ofrags = numfrags(fs, ip->i_lfs_fragsize[lbn]);
			ip->i_ffs1_blocks += (frags - ofrags);
		}
		ip->i_ffs1_db[lbn] = ndaddr;
		break;
	case 1:
		ooff = ip->i_ffs1_ib[a[0].in_off];
		if (ooff <= 0)
			ip->i_ffs1_blocks += frags;
		ip->i_ffs1_ib[a[0].in_off] = ndaddr;
		break;
	default:
		ap = &a[num - 1];
		if (bread(vp, ap->in_lbn, fs->lfs_bsize, NULL, 0, &bp))
			errx(1, "lfs_updatemeta: bread bno %" PRId64,
			    ap->in_lbn);

		ooff = ((ufs_daddr_t *) bp->b_data)[ap->in_off];
		if (ooff <= 0)
			ip->i_ffs1_blocks += frags;
		((ufs_daddr_t *) bp->b_data)[ap->in_off] = ndaddr;
		(void) VOP_BWRITE(bp);
	}

	/*
	 * Update segment usage information, based on old size
	 * and location.
	 */
	if (daddr > 0) {
		oldsn = dtosn(fs, daddr);
		if (lbn >= 0 && lbn < UFS_NDADDR)
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

	/* If block is beyond EOF, update size */
	if (lbn >= 0 && ip->i_ffs1_size <= (lbn << fs->lfs_bshift)) {
		ip->i_ffs1_size = (lbn << fs->lfs_bshift) + 1;
	}

	/* If block frag size is too large for old EOF, update size */
	if (lbn < UFS_NDADDR) {
		off_t minsize;

		minsize = (lbn << fs->lfs_bshift);
		minsize += (size - fs->lfs_fsize) + 1;
		if (ip->i_ffs1_size < minsize)
			ip->i_ffs1_size = minsize;
	}

	/* Count for the user */
	++nnewblocks;

	/* Add block to its new segment */
	sn = dtosn(fs, ndaddr);
	LFS_SEGENTRY(sup, fs, sn, bp);
	seg_table[sn].su_nbytes += size;
	sup->su_nbytes += size;
	if (!(bp->b_flags & B_GATHERED))
		fs->lfs_flags |= LFS_IFDIRTY;
	LFS_WRITESEGENTRY(sup, fs, sn, bp);
	for (i = 0; i < btofsb(fs, size); i++)
#ifndef VERBOSE_BLOCKMAP
		setbmap(daddr + i);
#else
		setbmap(daddr + i, ip->i_number);
#endif

	/* Check bfree accounting as well */
	if (daddr <= 0) {
		fs->lfs_bfree -= btofsb(fs, size);
	} else if (size != osize) {
		fs->lfs_bfree -= (frags - ofrags);
	}

	/*
	 * Now that this block has a new address, and its old
	 * segment no longer owns it, we can forget about its
	 * old size.
	 */
	if (lbn >= 0 && lbn < UFS_NDADDR)
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
	CLEANERINFO *cip;
	struct ubuf *bp, *sbp, *cbp;
	struct inodesc idesc;
	ufs_daddr_t daddr;
	int obfree;

	if (debug)
		pwarn("remove ino %d\n", (int)ino);

	obfree = fs->lfs_bfree;
	LFS_IENTRY(ifp, fs, ino, bp);
	daddr = ifp->if_daddr;
	if (daddr > 0) {
		ifp->if_daddr = 0x0;

		LFS_GET_HEADFREE(fs, cip, cbp, &(ifp->if_nextfree));
		VOP_BWRITE(bp);
		LFS_PUT_HEADFREE(fs, cip, cbp, ino);
		sbdirty();

		if (vp == NULL)
			vp = lfs_raw_vget(fs, ino, fs->lfs_ivnode->v_fd, daddr);

		LFS_SEGENTRY(sup, fs, dtosn(fs, daddr), sbp);
		sup->su_nbytes -= DINODE1_SIZE;
		VOP_BWRITE(sbp);
		seg_table[dtosn(fs, daddr)].su_nbytes -= DINODE1_SIZE;
	} else
		brelse(bp, 0);

	/* Do on-disk accounting */
	if (vp) {
		idesc.id_number = ino;
		idesc.id_func = pass4check; /* Delete dinode and blocks */
		idesc.id_type = ADDR;
		idesc.id_lblkno = 0;
		clri(&idesc, "unknown", 2); /* XXX magic number 2 */
		/* vp has been destroyed */
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
			if (debug)
				pwarn("ino %lld lbn %lld -> 0x%lx\n",
					(long long)fip->fi_ino,
					(long long)fip->fi_blocks[i],
					(long)daddr);
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

	/* Brand new blocks are always OK */
	if (idesc->id_blkno == UNWRITTEN)
		return KEEPON;

	/* Check that the blocks do not lie within clean segments. */
	anyout = anynew = 0;
	for (i = 0; i < idesc->id_numfrags; i++) {
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
			pwarn("EXCESSIVE BAD BLKS I=%llu",
			    (unsigned long long)idesc->id_number);
			if (preen)
				pwarn(" (SKIPPING)\n");
			else if (reply("CONTINUE") == 0)
				err(EEXIT, "%s", "");
			return (STOP);
		}
	}

	return pass1check(idesc);
}

static void
account_indir(struct uvnode *vp, struct ufs1_dinode *dp, daddr_t ilbn, daddr_t daddr, int lvl)
{
	struct ubuf *bp;
	int32_t *dap, *odap, *buf, *obuf;
	daddr_t lbn;

	if (lvl == 0)
		lbn = -ilbn;
	else
		lbn = ilbn + 1;
	bread(fs->lfs_devvp, fsbtodb(fs, daddr), fs->lfs_bsize, NULL, 0, &bp);
	buf = emalloc(fs->lfs_bsize);
	memcpy(buf, bp->b_data, fs->lfs_bsize);
	brelse(bp, 0);

	obuf = emalloc(fs->lfs_bsize);
	if (vp) {
		bread(vp, ilbn, fs->lfs_bsize, NULL, 0, &bp);
		memcpy(obuf, bp->b_data, fs->lfs_bsize);
		brelse(bp, 0);
	} else
		memset(obuf, 0, fs->lfs_bsize);

	for (dap = buf, odap = obuf;
	     dap < (int32_t *)((char *)buf + fs->lfs_bsize);
	     ++dap, ++odap) {
		if (*dap > 0 && *dap != *odap) {
			rfw_update_single(vp, lbn, *dap, dblksize(fs, dp, lbn));
			if (lvl > 0)
				account_indir(vp, dp, lbn, *dap, lvl - 1);
		}
		if (lvl == 0)
			++lbn;
		else if (lvl == 1)
			lbn -= NINDIR(fs);
		else if (lvl == 2)
			lbn -= NINDIR(fs) * NINDIR(fs);
	}

	free(obuf);
	free(buf);
}

/*
 * Account block changes between new found inode and existing inode.
 */
static void
account_block_changes(struct ufs1_dinode *dp)
{
	int i;
	daddr_t lbn, off, odaddr;
	struct uvnode *vp;
	struct inode *ip;

	vp = vget(fs, dp->di_inumber);
	ip = (vp ? VTOI(vp) : NULL);

	/* Check direct block holdings between existing and new */
	for (i = 0; i < UFS_NDADDR; i++) {
		odaddr = (ip ? ip->i_ffs1_db[i] : 0x0);
		if (dp->di_db[i] > 0 && dp->di_db[i] != odaddr)
			rfw_update_single(vp, i, dp->di_db[i],
					  dblksize(fs, dp, i));
	}

	/* Check indirect block holdings between existing and new */
	off = 0;
	for (i = 0; i < UFS_NIADDR; i++) {
		odaddr = (ip ? ip->i_ffs1_ib[i] : 0x0);
		if (dp->di_ib[i] > 0 && dp->di_ib[i] != odaddr) {
			lbn = -(UFS_NDADDR + off + i);
			rfw_update_single(vp, i, dp->di_ib[i], fs->lfs_bsize);
			account_indir(vp, dp, lbn, dp->di_ib[i], i);
		}
		if (off == 0)
			off = NINDIR(fs);
		else
			off *= NINDIR(fs);
	}
}

/*
 * Give a previously allocated inode a new address; do segment
 * accounting if necessary.
 *
 * Caller has ensured that this inode is not on the free list, so no
 * free list accounting is done.
 */
static void
readdress_inode(struct ufs1_dinode *dp, ufs_daddr_t daddr)
{
	IFILE *ifp;
	SEGUSE *sup;
	struct ubuf *bp;
	int sn;
	ufs_daddr_t odaddr;
	ino_t thisino = dp->di_inumber;
	struct uvnode *vp;

	/* Recursively check all block holdings, account changes */
	account_block_changes(dp);

	/* Move ifile pointer to this location */
	LFS_IENTRY(ifp, fs, thisino, bp);
	odaddr = ifp->if_daddr;
	assert(odaddr != 0);
	ifp->if_daddr = daddr;
	VOP_BWRITE(bp);

	if (debug)
		pwarn("readdress ino %d from 0x%x to 0x%x mode %o nlink %d\n",
			(int)dp->di_inumber,
			(unsigned)odaddr,
			(unsigned)daddr,
			(int)dp->di_mode, (int)dp->di_nlink);

	/* Copy over preexisting in-core inode, if any */
	vp = vget(fs, thisino);
	memcpy(VTOI(vp)->i_din.ffs1_din, dp, sizeof(*dp));

	/* Finally account the inode itself */
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
	ino_t ino, nextfree, oldhead;
	IFILE *ifp;
	SEGUSE *sup;
	struct ubuf *bp, *cbp;
	CLEANERINFO *cip;

	if (debug)
		pwarn("allocating ino %d at 0x%x\n", (int)thisino,
			(unsigned)daddr);
	while (thisino >= maxino) {
		extend_ifile(fs);
	}

	LFS_IENTRY(ifp, fs, thisino, bp);
	if (ifp->if_daddr != 0) {
		pwarn("allocated inode %lld already allocated\n",
			(long long)thisino);
	}
	nextfree = ifp->if_nextfree;
	ifp->if_nextfree = 0;
	ifp->if_daddr = daddr;
	VOP_BWRITE(bp);

	LFS_GET_HEADFREE(fs, cip, cbp, &oldhead);
	if (oldhead == thisino) {
		LFS_PUT_HEADFREE(fs, cip, cbp, nextfree);
		sbdirty();
		if (nextfree == 0) {
			extend_ifile(fs);
		}
	} else {
		/* Search the free list for this inode */
		ino = oldhead;
		while (ino) {
			LFS_IENTRY(ifp, fs, ino, bp);
			assert(ifp->if_nextfree != ino);
			if (ifp->if_nextfree == thisino) {
				ifp->if_nextfree = nextfree;
				VOP_BWRITE(bp);
				if (nextfree == 0)
					LFS_PUT_TAILFREE(fs, cip, cbp, ino);
				break;
			} else
				ino = ifp->if_nextfree;
			brelse(bp, 0);
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
	int i, j, bc, hassuper;
	int nnewfiles, ndelfiles, nmvfiles;
	int sn, curseg;
	char *ibbuf;
	long lastserial;

	devvp = fs->lfs_devvp;

	/* If we can't roll forward because of created files, don't try */
	if (no_roll_forward) {
		if (debug)
			pwarn("not rolling forward due to possible allocation conflict\n");
		return;
	}

	/* Find last valid partial segment */
	lastgood = try_verify(fs, devvp, 0, debug);
	if (lastgood == fs->lfs_offset) {
		if (debug)
			pwarn("not rolling forward, nothing to recover\n");
		return;
	}

	if (debug)
		pwarn("could roll forward from 0x%" PRIx32 " to 0x%" PRIx32 "\n",
			fs->lfs_offset, lastgood);

	if (!preen && reply("ROLL FORWARD") == 0)
		return;
	/*
	 * Pass 1: find inode blocks.  We ignore the Ifile inode but accept
	 * changes to any other inode.
	 */

	ibbuf = emalloc(fs->lfs_ibsize);
	nnewfiles = ndelfiles = nmvfiles = nnewblocks = 0;
	daddr = fs->lfs_offset;
	nextseg = fs->lfs_nextseg;
	hassuper = 0;
	lastserial = 0;
	while (daddr != lastgood) {
		seg_table[dtosn(fs, daddr)].su_flags |= SEGUSE_DIRTY | SEGUSE_ACTIVE;
		LFS_SEGENTRY(sup, fs, dtosn(fs, daddr), sbp);
		sup->su_flags |= SEGUSE_DIRTY;
		VOP_BWRITE(sbp);

		/* Could be a superblock */
		if (sntod(fs, dtosn(fs, daddr)) == daddr) {
			if (daddr == fs->lfs_start) {
				++hassuper;
				daddr += btofsb(fs, LFS_LABELPAD);
			}
			for (i = 0; i < LFS_MAXNUMSB; i++) {
				if (daddr == fs->lfs_sboffs[i]) {
					++hassuper;
					daddr += btofsb(fs, LFS_SBPAD);	
				}
				if (daddr < fs->lfs_sboffs[i])
					break;
			}
		}
		
		/* Read in summary block */
		bread(devvp, fsbtodb(fs, daddr), fs->lfs_sumsize, NULL, 0, &bp);
		sp = (SEGSUM *)bp->b_data;
		if (debug)
			pwarn("sum at 0x%x: ninos=%d nfinfo=%d\n",
				(unsigned)daddr, (int)sp->ss_ninos,
				(int)sp->ss_nfinfo);

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
			ino_t *inums;
			
			inums = ecalloc(INOPB(fs) + 1, sizeof(*inums));
			ibdaddr = *--idaddrp;
			fs->lfs_bfree -= btofsb(fs, fs->lfs_ibsize);
			sbdirty();
			bread(devvp, fsbtodb(fs, ibdaddr), fs->lfs_ibsize,
			      NOCRED, 0, &ibp);
			memcpy(ibbuf, ibp->b_data, fs->lfs_ibsize);
			brelse(ibp, 0);

			j = 0;
			for (dp = (struct ufs1_dinode *)ibbuf;
			     dp < (struct ufs1_dinode *)ibbuf + INOPB(fs);
			     ++dp) {
				if (dp->di_u.inumber == 0 ||
				    dp->di_u.inumber == fs->lfs_ifile)
					continue;
				/* Basic sanity checks */
				if (dp->di_nlink < 0 
#if 0
				    || dp->di_u.inumber < 0
				    || dp->di_size < 0
#endif
				) {
					pwarn("BAD INODE AT 0x%" PRIx32 "\n",
						ibdaddr);
					brelse(bp, 0);
					free(inums);
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
					++ndelfiles;
					continue;
				}
				/*
				 * (2) New valid inode, previously free.
				 *     Nothing to do except account
				 *     the inode itself, done after the
				 *     loop.
				 */
				if (vp == NULL) {
					if (!(sp->ss_flags & SS_DIROP))
						pfatal("NEW FILE IN NON-DIROP PARTIAL SEGMENT");
					else {
						inums[j++] = dp->di_u.inumber;
						nnewfiles++;
					}
					continue;
				}
				/*
				 * (3) Valid new version of previously
				 *     allocated inode.  Delete old file
				 *     and proceed as in (2).
				 */
				if (vp && VTOI(vp)->i_ffs1_gen < dp->di_gen) {
					remove_ino(vp, dp->di_u.inumber);
					if (!(sp->ss_flags & SS_DIROP))
						pfatal("NEW FILE VERSION IN NON-DIROP PARTIAL SEGMENT");
					else {
						inums[j++] = dp->di_u.inumber;
						ndelfiles++;
						nnewfiles++;
					}
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
					nmvfiles++;
					readdress_inode(dp, ibdaddr);

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
			for (j = 0; inums[j]; j++) {
				alloc_inode(inums[j], ibdaddr);
				vp = lfs_raw_vget(fs, inums[j],
					      devvp->v_fd, ibdaddr);
				/* We'll get the blocks later */
				if (debug)
					pwarn("alloc ino %d nlink %d\n",
						(int)inums[j], VTOD(vp)->di_nlink);
				memset(VTOD(vp)->di_db, 0, (UFS_NDADDR + UFS_NIADDR) *
				       sizeof(ufs_daddr_t));
				VTOD(vp)->di_blocks = 0;

				vp->v_uflag |= VU_DIROP;
				inodirty(VTOI(vp));
			}
			free(inums);
		}

		bc = check_summary(fs, sp, daddr, debug, devvp, NULL);
		if (bc == 0) {
			pwarn("unexpected bad seg ptr at 0x%x with serial=%d\n",
				(int)daddr, (int)sp->ss_serial);
			brelse(bp, 0);
			break;
		} else {
			if (debug)
				pwarn("good seg ptr at 0x%x with serial=%d\n",
					(int)daddr, (int)sp->ss_serial);
			lastserial = sp->ss_serial;
		}
		odaddr = daddr;
		daddr += btofsb(fs, fs->lfs_sumsize + bc);
		if (dtosn(fs, odaddr) != dtosn(fs, daddr) ||
		    dtosn(fs, daddr) != dtosn(fs, daddr +
			btofsb(fs, fs->lfs_sumsize + fs->lfs_bsize) - 1)) {
			daddr = ((SEGSUM *)bp->b_data)->ss_next;
		}
		brelse(bp, 0);
	}

    out:
	free(ibbuf);

	/* Set serial here, just to be sure (XXX should be right already) */
	fs->lfs_serial = lastserial + 1;

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
		if ((vp->v_uflag & VU_DIROP) == 0)
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

		/* Could be a superblock */
		if (sntod(fs, dtosn(fs, daddr)) == daddr) {
			if (daddr == fs->lfs_start)
				daddr += btofsb(fs, LFS_LABELPAD);
			for (i = 0; i < LFS_MAXNUMSB; i++) {
				if (daddr == fs->lfs_sboffs[i]) {
					daddr += btofsb(fs, LFS_SBPAD);	
				}
				if (daddr < fs->lfs_sboffs[i])
					break;
			}
		}
		
		/* Read in summary block */
		bread(devvp, fsbtodb(fs, daddr), fs->lfs_sumsize, NULL, 0, &bp);
		sp = (SEGSUM *)bp->b_data;
		bc = check_summary(fs, sp, daddr, debug, devvp, pass6harvest);
		if (bc == 0) {
			pwarn("unexpected bad seg ptr [2] at 0x%x with serial=%d\n",
				(int)daddr, (int)sp->ss_serial);
			brelse(bp, 0);
			break;
		}
		odaddr = daddr;
		daddr += btofsb(fs, fs->lfs_sumsize + bc);
		fs->lfs_avail -= btofsb(fs, fs->lfs_sumsize + bc);
		if (dtosn(fs, odaddr) != dtosn(fs, daddr) ||
		    dtosn(fs, daddr) != dtosn(fs, daddr +
			btofsb(fs, fs->lfs_sumsize + fs->lfs_bsize) - 1)) {
			fs->lfs_avail -= sntod(fs, dtosn(fs, daddr) + 1) - daddr;
			daddr = ((SEGSUM *)bp->b_data)->ss_next;
		}
		LFS_CLEANERINFO(cip, fs, cbp);
		LFS_SYNC_CLEANERINFO(cip, fs, cbp, 0);
		bp->b_flags |= B_AGE;
		brelse(bp, 0);
	}

	/* Final address could also be a superblock */
	if (sntod(fs, dtosn(fs, lastgood)) == lastgood) {
		if (lastgood == fs->lfs_start)
			lastgood += btofsb(fs, LFS_LABELPAD);
		for (i = 0; i < LFS_MAXNUMSB; i++) {
			if (lastgood == fs->lfs_sboffs[i])
				lastgood += btofsb(fs, LFS_SBPAD);	
			if (lastgood < fs->lfs_sboffs[i])
				break;
		}
	}
		
	/* Update offset to point at correct location */
	fs->lfs_offset = lastgood;
	fs->lfs_curseg = sntod(fs, dtosn(fs, lastgood));
	for (sn = curseg = dtosn(fs, fs->lfs_curseg);;) {
		sn = (sn + 1) % fs->lfs_nseg;
		if (sn == curseg)
			errx(1, "no clean segments");
		LFS_SEGENTRY(sup, fs, sn, bp);
		if ((sup->su_flags & SEGUSE_DIRTY) == 0) {
			sup->su_flags |= SEGUSE_DIRTY | SEGUSE_ACTIVE;
			VOP_BWRITE(bp);
			break;
		}
		brelse(bp, 0);
	}
	fs->lfs_nextseg = sntod(fs, sn);

	if (preen) {
		if (ndelfiles)
			pwarn("roll forward deleted %d file%s\n", ndelfiles,
				(ndelfiles > 1 ? "s" : ""));
		if (nnewfiles)
			pwarn("roll forward added %d file%s\n", nnewfiles,
				(nnewfiles > 1 ? "s" : ""));
		if (nmvfiles)
			pwarn("roll forward relocated %d inode%s\n", nmvfiles,
				(nmvfiles > 1 ? "s" : ""));
		if (nnewblocks)
			pwarn("roll forward verified %d data block%s\n", nnewblocks,
				(nnewblocks > 1 ? "s" : ""));
		if (ndelfiles == 0 && nnewfiles == 0 && nmvfiles == 0 &&
		    nnewblocks == 0)
			pwarn("roll forward produced nothing new\n");
	}

	if (!preen) {
		/* Run pass 5 again (it's quick anyway). */
		pwarn("** Phase 6b - Recheck Segment Block Accounting\n");
		pass5();
	}

	/* Likewise for pass 0 */
	if (!preen)
		pwarn("** Phase 6c - Recheck Inode Free List\n");
	pass0();
}
