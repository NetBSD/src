/*	$NetBSD: disksubr.c,v 1.1.1.1 1995/03/26 07:12:18 leo Exp $	*/

/*
 * Copyright (c) 1995 Leo Weppelman.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Leo Weppelman.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <machine/tospart.h>

#define b_cylin b_resid
#define baddr(bp) (void *)((bp)->b_un.b_addr)

/*
 * Stash TOS partition info in here before handling.
 */
typedef struct {
	u_long	nblocks;
	u_long	blkoff;
	u_long	type;
} TMP_PART;

#define	ROOT_FLAG	0x8000	/* Added in TMP_PART.type when NBR	*/

static char *rd_tosparts __P((TMP_PART *,dev_t,void (*)(),struct disklabel *));
static int	get_type __P((u_char *));

/* XXX unknown function but needed for /sys/scsi to link */
int
dk_establish()
{
	return(-1);
}

/*
 * Attempt to read a disk label from a device
 * using the indicated stategy routine.
 * The label must be partly set up before this:
 * secpercyl and anything required in the strategy routine
 * (e.g., sector size) must be filled in before calling us.
 * Returns null on success and an error string on failure.
 */
char *
readdisklabel(dev, strat, lp, clp)
dev_t			dev;
void			(*strat)();
struct disklabel	*lp;
struct cpu_disklabel	*clp;
{
	TMP_PART	sizes[TOS_MAXPART];
	char		*msg;
	int		i;
	int		usr_part = RAW_PART;

	bzero(sizes, sizeof(sizes));
	if((msg = rd_tosparts(sizes, dev, strat, lp)) != NULL)
		return(msg);

	/*
	 * give some guarnteed validity to
	 * the disklabel
	 */
	if(lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff;
	if(lp->d_secpercyl == 0)
		return("Zero secpercyl");
	lp->d_npartitions = RAW_PART + 1;

	for(i = 0; i < MAXPARTITIONS; i++) {
		if(i == RAW_PART)
			continue;
		lp->d_partitions[i].p_size   = 0;
		lp->d_partitions[i].p_offset = 0;
	}

	/*
	 * Now map the partition table from TOS to the NetBSD table.
	 *
	 * This means:
	 *  Part 0   : Root
	 *  Part 1   : Swap
	 *  Part 2   : Whole disk
	 *  Part 3.. : User partitions
	 *
	 * When more than one root partition is found, the only the first one 
	 * will be recognized as such. The others are mapped as user partitions.
	 */
	lp->d_partitions[RAW_PART].p_size   = sizes[0].nblocks;
	lp->d_partitions[RAW_PART].p_offset = sizes[0].blkoff;

	for(i = 0; sizes[i].nblocks; i++) {
		int		have_root = 0;
		int		pno;

		if(!have_root && (sizes[i].type & ROOT_FLAG)) {
			lp->d_partitions[0].p_size   = sizes[i].nblocks;
			lp->d_partitions[0].p_offset = sizes[i].blkoff;
			lp->d_partitions[0].p_fstype = FS_BSDFFS;
			have_root++;
			continue;
		}
		switch(sizes[i].type &= ~ROOT_FLAG) {
			case FS_SWAP:
					pno = 1;
					break;
			case FS_BSDFFS:
			case FS_MSDOS:
					pno = ++usr_part;
					break;
			default:
					continue;
		}
		if(pno >= MAXPARTITIONS)
			break; /* XXX */
		lp->d_partitions[pno].p_size   = sizes[i].nblocks;
		lp->d_partitions[pno].p_offset = sizes[i].blkoff;
		lp->d_partitions[pno].p_fstype = sizes[i].type;
	}
	lp->d_npartitions = usr_part + 1;
	lp->d_secperunit  = sizes[0].nblocks;

	/*
	 * calulate new checksum.
	 */
	lp->d_magic = lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);

	return(NULL);
}

/*
 * Check new disk label for sensibility
 * before setting it.
 */
int
setdisklabel(olp, nlp, openmask, clp)
register struct disklabel *olp, *nlp;
u_long openmask;
struct cpu_disklabel *clp;
{
	return (EINVAL);
}

/*
 * Write disk label back to device after modification.
 */
int
writedisklabel(dev, strat, lp, clp)
	dev_t dev;
	void (*strat)();
	register struct disklabel *lp;
	struct cpu_disklabel *clp;
{
	return(EINVAL);
}


int
bounds_check_with_label(bp, lp, wlabel)
struct buf		*bp;
struct disklabel	*lp;
int			wlabel;
{
	struct partition	*pp;
	long			maxsz, sz;

	pp = &lp->d_partitions[DISKPART(bp->b_dev)];
	if(bp->b_flags & B_RAW) {
		maxsz = pp->p_size * (lp->d_secsize / DEV_BSIZE);
		sz = (bp->b_bcount + DEV_BSIZE - 1) >> DEV_BSHIFT;
	}
	else {
		maxsz = pp->p_size;
		sz = (bp->b_bcount + lp->d_secsize - 1) / lp->d_secsize;
	}

	if(bp->b_blkno < 0 || bp->b_blkno + sz > maxsz) {
		if(bp->b_blkno == maxsz) {
			/* 
			 * trying to get one block beyond return EOF.
			 */
			bp->b_resid = bp->b_bcount;
			return(0);
		}
		sz = maxsz - bp->b_blkno;
		if(sz <= 0 || bp->b_blkno < 0) {
			bp->b_error = EINVAL;
			bp->b_flags |= B_ERROR;
			return(-1);
		}
		/* 
		 * adjust count down
		 */
		if(bp->b_flags & B_RAW)
			bp->b_bcount = sz << DEV_BSHIFT;
		else bp->b_bcount = sz * lp->d_secsize;
	}

	/*
	 * calc cylinder for disksort to order transfers with
	 */
	bp->b_cylin = (bp->b_blkno + pp->p_offset) / lp->d_secpercyl;
	return(1);
}

/*
 * Read a GEM-partition info and translate it to our temporary partitions array.
 * Returns 0 if an error occured, 1 if all went ok.
 */
#define	BP_SETUP(bp, block) {						\
			bp->b_blkno  = block;				\
			bp->b_cylin  = block / lp->d_secpercyl;		\
			bp->b_bcount = TOS_BSIZE;			\
			bp->b_flags  = B_BUSY | B_READ;			\
		}

static char *
rd_tosparts(sizes, dev, strat, lp)
TMP_PART		*sizes;
dev_t			dev;
void			(*strat)();
struct disklabel	*lp;
{
	GEM_ROOT	*g_root;
	GEM_PART	g_local[NGEM_PARTS];
	struct buf	*bp;
	int		pno  = 1;
	char		*msg = NULL;
	int		i;

	/*
	 * Get a buffer first, so we can read the disk.
	 */
	bp = (void*)geteblk(TOS_BSIZE);
	bp->b_dev = MAKEDISKDEV(major(dev), DISKUNIT(dev), RAW_PART);
	
	/*
	 * Read root sector
	 */
	BP_SETUP(bp, TOS_BBLOCK);
	strat(bp);
	if(biowait(bp)) {
		msg = "I/O error";
		goto done;
	}

	/*
	 * Make local copy of partition info, we may need to re-use
	 * the buffer in case of 'XGM' partitions.
	 */
	g_root  = (GEM_ROOT*)baddr(bp);
	bcopy(g_root->parts, g_local, NGEM_PARTS*sizeof(GEM_PART));

	/*
	 * Partition 0 contains whole disk!
	 */
	sizes[0].nblocks = g_root->hd_siz;
	sizes[0].blkoff  = 0;
	sizes[0].type    = FS_UNUSED;

	for(i = 0; i < NGEM_PARTS; i++) {
	    if(!(g_local[i].p_flg & 1)) 
		continue;
	    if(!strncmp(g_local[i].p_id, "XGM", 3)) {
		int	j;
		daddr_t	new_root = g_local[i].p_st;

		/*
		 * Loop through extended partition list
		 */
		for(;;) {
		    BP_SETUP(bp, new_root);
		    strat(bp);
		    if(biowait(bp)) {
				msg = "I/O error";
				goto done;
		    }
		    for(j = 0; j < NGEM_PARTS; j++) {
			if(!(g_root->parts[j].p_flg & 1))
				continue;
			if(!strncmp(g_root->parts[j].p_id, "XGM", 3)) {
			    new_root = g_local[i].p_st + g_root->parts[j].p_st;
			    break;
			}
			else {
			    sizes[pno].nblocks=g_root->parts[j].p_size;
			    sizes[pno].blkoff =g_root->parts[j].p_st+new_root;
			    sizes[pno].type   =get_type(g_root->parts[j].p_id);
			    pno++;
			    if(pno >= TOS_MAXPART)
				break;
			}
		    }
		    if(j == NGEM_PARTS)
			break;
		}
	    }
	    else {
		sizes[pno].nblocks = g_local[i].p_size;
		sizes[pno].blkoff  = g_local[i].p_st;
		sizes[pno].type    = get_type(g_local[i].p_id);
		pno++;
	    }
	}
	/*
	 * Check sensibility of partition info
	 */
	for(i = 2; i < pno; i++) {
		if(sizes[i].blkoff < (sizes[i-1].blkoff + sizes[i-1].nblocks)) {
			msg = "Partion table bad (overlap)";
			goto done;
		}
		if((sizes[i].blkoff + sizes[i].nblocks) > sizes[0].nblocks) {
			msg = "Partion table bad (extends beyond disk)";
			goto done;
		}
	}
done:
	bp->b_flags = B_INVAL | B_AGE | B_READ;
	brelse(bp);
	return(msg);
}

/*
 * Translate a TOS partition type to a NetBSD partition type.
 */
#define	ID_CMP(a,b)	((a[0] == b[0]) && (a[1] == b[1]) && (a[2] == b[2]))

static int
get_type(p_id)
u_char	*p_id;
{
	if(ID_CMP(p_id, "GEM") || ID_CMP(p_id, "BGM"))
		return(FS_MSDOS); /* XXX - should be compatible */
	if(ID_CMP(p_id, "NBU"))
		return(FS_BSDFFS);
	if(ID_CMP(p_id, "NBR"))
		return(FS_BSDFFS|ROOT_FLAG);
	if(ID_CMP(p_id, "NBS"))
		return(FS_SWAP);
	return(FS_OTHER);
}
