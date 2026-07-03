/*	$NetBSD: bad144.c,v 1.1 2026/07/03 21:27:40 thorpej Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
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
 * Copyright (c) 1982, 1986, 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
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
 *      @(#)dkbad.c     8.2 (Berkeley) 1/12/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bad144.c,v 1.1 2026/07/03 21:27:40 thorpej Exp $");

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/dkbad.h>
#include <sys/errno.h>
#include <sys/kmem.h>
#include <sys/systm.h>

/*
 * General description of DEC standard 144, from bad144(8):
 *
 *   The format of the information is specified by DEC standard 144, as
 *   follows.  The bad sector information is located in the first 5 even
 *   numbered sectors of the last track of the disk pack.  There are five
 *   identical copies of the information, described by the dkbad structure.
 *
 *   Replacement sectors are allocated starting with the first sector before
 *   the bad sector information and working backwards towards the beginning of
 *   the disk.  A maximum of 126 bad sectors are supported.  The position of
 *   the bad sector in the bad sector table determines the replacement sector
 *   to which it corresponds.  The bad sectors must be listed in ascending
 *   order.
 *
 *   (Additional commentary: this is not always true; some disk drivers may
 *   use alternate cylinders for bad sector replacements.  This is why we
 *   do not perform the actual remapping here.)
 *
 *   The bad sector information and replacement sectors are conventionally
 *   only accessible through the ``c'' file system partition of the disk.  If
 *   that partition is used for a file system, the user is responsible for
 *   making sure that it does not overlap the bad sector information or any
 *   replacement sectors.  Thus, one track plus 126 sectors must be reserved
 *   to allow use of all of the possible bad sector replacements.
 *
 * NOTE ABOUT BYTE ORDER: You would think that because this on-disk format
 * originated at DEC that the fields would be nominally stored in little-
 * endian byte order, and in historical BSD usage (including 386BSD back
 * in the day), that held true.  HOWEVER, bad144 is also used by the Xylogics
 * disk drivers, on Sun3 and SPARC systems which are big-endian and those
 * drivers historically did NOT byte-swap the entries in the table (this also
 * seems to be consistent with how SunOS versions that supported SMD drives
 * also treated bad144).  So, we have ended up with a mixed bag, and I have
 * no interest in disturbing that particular sleeping dog.
 */

struct bad144_context {
	/* Provided by bad144 client at setup time. */
	uint32_t	bc_secsize;	/* drive sector size */
	uint32_t	bc_ncyl;	/* drive data cylinders */
	uint32_t	bc_ntrkcyl;	/* drive tracks (heads) per cylinder */
	uint32_t	bc_nsectrk;	/* drive sectors per track */

	struct dkbad	bc_bt;		/* the actual bad sector table */
	daddr_t		bc_bb[NBT_BAD+1];/* LBA-ized version */
};

/*
 * bad144_init --
 *	Initialize bad144 sector forwarding context.  Returns a context
 *	to be used for bad144 remapping, or NULL if bad144 can't be
 *	used on this drive due to geometry parameters being out of range.
 */
struct bad144_context *
bad144_init(uint32_t secsize, uint32_t ncyl, uint32_t ntrkcyl, uint32_t nsectrk)
{
	int i;

	KASSERT(secsize != 0);
	KASSERT(ncyl != 0);
	KASSERT(ntrkcyl != 0);
	KASSERT(nsectrk != 0);

	if (ncyl > BAD144_MAXCYLNO ||
	    ntrkcyl > BAD144_MAXTRKNO ||
	    nsectrk > BAD144_MAXSECNO) {
		/* Can't do bad144 on this drive. */
		return NULL;
	}

	struct bad144_context *ctx = kmem_zalloc(sizeof(*ctx), KM_SLEEP);

	ctx->bc_secsize = secsize;
	ctx->bc_ncyl = ncyl;
	ctx->bc_ntrkcyl = ntrkcyl;
	ctx->bc_nsectrk = nsectrk;

	for (i = 0; i < NBT_BAD; i++) {
		ctx->bc_bt.bt_bad[i].bt_cyl =
		    ctx->bc_bt.bt_bad[i].bt_trksec = 0xffff;
		ctx->bc_bb[i] = (daddr_t)-1;
	}
	/* There is an extra slot to terminate the list. */
	ctx->bc_bb[NBT_BAD] = (daddr_t)-1;

	return ctx;
}

/*
 * bad144_fini --
 *	Dispose of a bad144 sector forwarding context.
 */
void
bad144_fini(struct bad144_context * const ctx)
{
	if (ctx != NULL) {
		kmem_free(ctx, sizeof(*ctx));
	}
}

/*
 * This empty slot check comes from Chuck Cranor's Xylogics SMD
 * drivers.  It differs from the usual -1 check for cylinder,
 * but presumably he did it this way because bad144 tables like
 * this are found in the wild ??  (N.B. old SunOS versions seem
 * to initialize bt_cyl to 0xffff but don't bother with bt_trksec?)
 */
#define	EMPTY_SLOT(b)				\
	((bt->bt_bad[i].bt_cyl == 0xffff ||	\
	  bt->bt_bad[i].bt_cyl == 0) &&		\
	 bt->bt_bad[i].bt_trksec == 0xffff)

/*
 * bad144_set --
 *	Set the bad144 sector fowarding table from the provided
 *	buffer.
 */
int
bad144_set(struct bad144_context * const ctx, const struct dkbad * const bt)
{
	int i;
	daddr_t key, last_key;

	if (ctx == NULL) {
		return ENOTSUP;
	}

	KASSERT(bt != NULL);

	for (last_key = 0, i = 0; i < NBT_BAD; i++) {
		/*
		 * All bad sector records are packed into the beginning
		 * of the table.  Once we see an empty slot, there should
		 * be no more records.  Verified below.
		 */
		if (EMPTY_SLOT(&bt->bt_bad[i])) {
			break;
		}
		/*
		 * Verify the cylinder / head / sector values in the
		 * table do not exceed the drive's geometry.
		 */
		if (bt->bt_bad[i].bt_cyl >= ctx->bc_ncyl ||
		    (bt->bt_bad[i].bt_trksec >> 8) >= ctx->bc_ntrkcyl ||
		    (bt->bt_bad[i].bt_trksec & 0xff) >= ctx->bc_nsectrk) {
			goto bad_table;
		}
		/*
		 * Verify the table lists bad sectors in ascending order
		 * and that are no duplicates.
		 */
		key = ((daddr_t)bt->bt_bad[i].bt_cyl << 16) +
		    bt->bt_bad[i].bt_trksec;
		if (key <= last_key) {
			goto bad_table;
		}
		last_key = key;
	}

	/*
	 * Once we see an empty slot, we must only see empty
	 * slots.
	 */
	for (; i < NBT_BAD; i++) {
		if (!EMPTY_SLOT(&bt->bt_bad[i])) {
			goto bad_table;
		}
	}

	memcpy(&ctx->bc_bt, bt, sizeof(ctx->bc_bt));
	for (i = 0; i < NBT_BAD; i++) {
		if (EMPTY_SLOT(&bt->bt_bad[i])) {
			ctx->bc_bb[i] = (daddr_t)-1;
			continue;
		}
		ctx->bc_bb[i] =
		    (bt->bt_bad[i].bt_cyl * (ctx->bc_ntrkcyl * ctx->bc_nsectrk))
		  + ((bt->bt_bad[i].bt_trksec >> 8) * ctx->bc_nsectrk)
		  + (bt->bt_bad[i].bt_trksec & 0xff);
	}

	return 0;

 bad_table:
	return EINVAL;
}

#define	DKBAD_MAGIC	0x4321

/*
 * bad144_load --
 *	Find a bad sector table and load it into the context.
 */
int
bad144_load(struct bad144_context * const ctx, dev_t dev,
    void (*strat)(struct buf *))
{
	struct buf *bp;
	int error, i = 0;

	if (ctx == NULL) {
		return ESRCH;
	}

	uint32_t nseccyl = ctx->bc_ntrkcyl * ctx->bc_nsectrk;
	uint32_t nsecunit = ctx->bc_ncyl * nseccyl;

	bp = geteblk(ctx->bc_secsize);
	bp->b_dev = dev;

	do {
		bp->b_oflags &= ~BO_DONE;
		bp->b_flags |= B_READ;
		bp->b_blkno = nsecunit - ctx->bc_nsectrk + i;
		if (ctx->bc_secsize > DEV_BSIZE) {
			bp->b_blkno *= ctx->bc_secsize / DEV_BSIZE;
		} else {
			bp->b_blkno /= DEV_BSIZE / ctx->bc_secsize;
		}
		bp->b_bcount = ctx->bc_secsize;
		bp->b_cylinder = ctx->bc_ncyl - 1;
		(*strat)(bp);

		if ((error = biowait(bp)) == 0) {
			struct dkbad *db = (struct dkbad *)bp->b_data;
			if (db->bt_mbz == 0 &&
			    db->bt_flag == DKBAD_MAGIC) {
				error = bad144_set(ctx, db);
				break;
			} else {
				error = ESRCH;
			}
		}
	} while (error != 0 && (i += 2) < 10 && i < ctx->bc_nsectrk);

	brelse(bp, BC_INVAL);
	return error;
}

/*
 * bad144_isbad_chs --
 *	Search the bad sector table looking for the specified sector.
 *	Return the index if found, -1 if not found.
 *
 *	This one does CHS addressing and is essentially the traditional
 *	isbad() routine.
 */
int
bad144_isbad_chs(const struct bad144_context * const ctx,
    uint32_t cyl, uint32_t trk, uint32_t sec)
{
	const struct dkbad * const bt = &ctx->bc_bt;
	int i;
	daddr_t key, tkey;

	if (ctx == NULL) {
		return -1;
	}

	key = ((daddr_t)cyl << 16) + (trk << 8) + sec;
	for (i = 0; i < NBT_BAD; i++) {
		tkey = ((daddr_t)bt->bt_bad[i].bt_cyl << 16) +
		    bt->bt_bad[i].bt_trksec;
		if (key == tkey) {
			return i;
		}
		if (EMPTY_SLOT(i) || key < tkey) {
			break;
		}
	}
	return -1;
}

/*
 * bad144_isbad_lba --
 *	Search the bad sector table looking for the specified sector.
 *	Return the index if found, -1 if not found.
 *
 *	This one does LBA addressing, and checks a block range,
 *	also returning the distance to the bad sector.
 */
int
bad144_isbad_lba(const struct bad144_context * const ctx, daddr_t blk,
    int nblks, int *distancep)
{
	daddr_t blkdiff;
	int i;

	KASSERT(blk >= 0);
	KASSERT(nblks > 0);
	KASSERT(distancep != NULL);

	if (ctx == NULL) {
		return -1;
	}

	for (i = 0; (blkdiff = ctx->bc_bb[i]) != (daddr_t)-1; i++) {
		blkdiff -= blk;
		if (blkdiff < 0) {
			continue;
		}
		if (blkdiff < nblks) {
			/*
			 * There is a bad block within this transfer.
			 * If the distance is 0, then it's the first
			 * block in the transfer (i.e. "blk").  Otherwise,
			 * the distance indicates how many blocks can
			 * be transferred before the remap must be done.
			 */
			KASSERT(blkdiff < INT_MAX);
			*distancep = (int)blkdiff;
			return i;
		}
	}
	return -1;
}
