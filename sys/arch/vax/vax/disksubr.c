/*	$NetBSD: disksubr.c,v 1.52.14.1 2012/07/04 20:41:46 jdc Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1988 Regents of the University of California.
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
 *	@(#)ufs_disksubr.c	7.16 (Berkeley) 5/4/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: disksubr.c,v 1.52.14.1 2012/07/04 20:41:46 jdc Exp $");

#include "opt_compat_ultrix.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/cpu.h>
#include <sys/dkbad.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/syslog.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

#include <machine/macros.h>

#include <dev/mscp/mscp.h> /* For disk encoding scheme */

#ifdef COMPAT_ULTRIX
#include <dev/dec/dec_boot.h>
#include <ufs/ufs/dinode.h>	/* XXX for fs.h */
#include <ufs/ffs/fs.h>		/* XXX for BBSIZE & SBSIZE */

static const char *compat_label(dev_t dev, void (*strat)(struct buf *bp),
	struct disklabel *lp, struct cpu_disklabel *osdep);
#endif /* COMPAT_ULTRIX */

/*
 * Attempt to read a disk label from a device
 * using the indicated strategy routine.
 * The label must be partly set up before this:
 * secpercyl and anything required in the strategy routine
 * (e.g., sector size) must be filled in before calling us.
 * Returns null on success and an error string on failure.
 */
const char *
readdisklabel(dev_t dev, void (*strat)(struct buf *),
    struct disklabel *lp, struct cpu_disklabel *osdep)
{
	struct buf *bp;
	struct disklabel *dlp;
	const char *msg = NULL;

	if (lp->d_npartitions == 0) { /* Assume no label */
		lp->d_secperunit = 0x1fffffff;
		lp->d_npartitions = 3;
		lp->d_partitions[2].p_size = 0x1fffffff;
		lp->d_partitions[2].p_offset = 0;
	}

	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;
	bp->b_blkno = LABELSECTOR;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;
	bp->b_cylinder = LABELSECTOR / lp->d_secpercyl;
	(*strat)(bp);
	if (biowait(bp)) {
		msg = "I/O error";
	} else {
		dlp = (struct disklabel *)((char *)bp->b_data + LABELOFFSET);
		if (dlp->d_magic != DISKMAGIC || dlp->d_magic2 != DISKMAGIC) {
			msg = "no disk label";
		} else if (dlp->d_npartitions > MAXPARTITIONS ||
		    dkcksum(dlp) != 0)
			msg = "disk label corrupted";
		else {
			*lp = *dlp;
		}
	}
	brelse(bp, 0);

#ifdef COMPAT_ULTRIX
	/*
	 * If no NetBSD label was found, check for an Ultrix label and
	 * construct tne incore label from the Ultrix partition information.
	 */
	if (msg != NULL) {
		msg = compat_label(dev, strat, lp, osdep);
		if (msg == NULL) {
			printf("WARNING: using Ultrix partition information\n");
			/* set geometry? */
		}
	}
#endif
	return (msg);
}

#ifdef COMPAT_ULTRIX
/*
 * Given a buffer bp, try and interpret it as an Ultrix disk label,
 * putting the partition info into a native NetBSD label
 */
const char *
compat_label(dev_t dev, void (*strat)(struct buf *bp), struct disklabel *lp,
	struct cpu_disklabel *osdep)
{
	dec_disklabel *dlp;
	struct buf *bp = NULL;
	const char *msg = NULL;
	uint8_t *dp;

	bp = geteblk((int)lp->d_secsize);
	dp = bp->b_data;
	bp->b_dev = dev;
	bp->b_blkno = DEC_LABEL_SECTOR;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;
	bp->b_cylinder = DEC_LABEL_SECTOR / lp->d_secpercyl;
	(*strat)(bp);

	if (biowait(bp)) {
		msg = "I/O error";
		goto done;
	}

	for (dlp = (dec_disklabel *)dp;
	    dlp <= (dec_disklabel *)(dp+DEV_BSIZE-sizeof(*dlp));
	    dlp = (dec_disklabel *)((char *)dlp + sizeof(long))) {

		int part;

		if (dlp->magic != DEC_LABEL_MAGIC) {
			if (dlp->magic != 0)
				printf("label: %x\n",dlp->magic);
			msg = ((msg != NULL) ? msg: "no disk label");
			goto done;
		}

		lp->d_magic = DEC_LABEL_MAGIC;
		lp->d_npartitions = 0;
		strncpy(lp->d_packname, "Ultrix label", 16);
		lp->d_rpm = 3600;
		lp->d_interleave = 1;
		lp->d_flags = 0;
		lp->d_bbsize = BBSIZE;
		lp->d_sbsize = 8192;
		for (part = 0;
		     part <((MAXPARTITIONS<DEC_NUM_DISK_PARTS) ?
		            MAXPARTITIONS : DEC_NUM_DISK_PARTS);
		     part++) {
			lp->d_partitions[part].p_size = dlp->map[part].num_blocks;
			lp->d_partitions[part].p_offset = dlp->map[part].start_block;
			lp->d_partitions[part].p_fsize = 1024;
			lp->d_partitions[part].p_fstype =
			    (part==1) ? FS_SWAP : FS_BSDFFS;
			lp->d_npartitions += 1;

#ifdef DIAGNOSTIC
			printf(" Ultrix label rz%d%c: start %d len %d\n",
			       DISKUNIT(dev), "abcdefgh"[part],
			       lp->d_partitions[part].p_offset,
			       lp->d_partitions[part].p_size);
#endif
		}
		break;
	}

done:
	brelse(bp, 0);
	return (msg);
}
#endif /* COMPAT_ULTRIX */

/*
 * Check new disk label for sensibility
 * before setting it.
 */
int
setdisklabel(struct disklabel *olp, struct disklabel *nlp,
    u_long openmask, struct cpu_disklabel *osdep)
{
	int i;
	struct partition *opp, *npp;

	if (nlp->d_magic != DISKMAGIC || nlp->d_magic2 != DISKMAGIC ||
	    dkcksum(nlp) != 0)
		return (EINVAL);
	while ((i = ffs(openmask)) != 0) {
		i--;
		openmask &= ~(1 << i);
		if (nlp->d_npartitions <= i)
			return (EBUSY);
		opp = &olp->d_partitions[i];
		npp = &nlp->d_partitions[i];
		if (npp->p_offset != opp->p_offset || npp->p_size < opp->p_size)
			return (EBUSY);
		/*
		 * Copy internally-set partition information
		 * if new label doesn't include it.		XXX
		 */
		if (npp->p_fstype == FS_UNUSED && opp->p_fstype != FS_UNUSED) {
			npp->p_fstype = opp->p_fstype;
			npp->p_fsize = opp->p_fsize;
			npp->p_frag = opp->p_frag;
			npp->p_cpg = opp->p_cpg;
		}
	}
	nlp->d_checksum = 0;
	nlp->d_checksum = dkcksum(nlp);
	*olp = *nlp;
	return (0);
}

/*
 * Write disk label back to device after modification.
 * Always allow writing of disk label; even if the disk is unlabeled.
 */
int
writedisklabel(dev_t dev, void (*strat)(struct buf *),
    struct disklabel *lp, struct cpu_disklabel *osdep)
{
	struct buf *bp;
	struct disklabel *dlp;
	int error = 0;

	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = MAKEDISKDEV(major(dev), DISKUNIT(dev), RAW_PART);
	bp->b_blkno = LABELSECTOR;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags |= B_READ;
	(*strat)(bp);
	if ((error = biowait(bp)))
		goto done;
	dlp = (struct disklabel *)((char *)bp->b_data + LABELOFFSET);
	memcpy(dlp, lp, sizeof(struct disklabel));
	bp->b_oflags &= ~(BO_DONE);
	bp->b_flags &= ~(B_READ);
	bp->b_flags |= B_WRITE;
	(*strat)(bp);
	error = biowait(bp);

done:
	brelse(bp, 0);
	return (error);
}

/*	
 * Print out the name of the device; ex. TK50, RA80. DEC uses a common
 * disk type encoding scheme for most of its disks.
 */   
void  
disk_printtype(int unit, int type)
{
	printf(" drive %d: %c%c", unit, (int)MSCP_MID_CHAR(2, type),
	    (int)MSCP_MID_CHAR(1, type));
	if (MSCP_MID_ECH(0, type))
		printf("%c", (int)MSCP_MID_CHAR(0, type));
	printf("%d\n", MSCP_MID_NUM(type));
}

/*
 * Be sure that the pages we want to do DMA to is actually there
 * by faking page-faults if necessary. If given a map-register address,
 * also map it in.
 */
void
disk_reallymapin(struct buf *bp, struct pte *map, int reg, int flag)
{
	struct proc *p;
	volatile pt_entry_t *io;
	pt_entry_t *pte;
	int pfnum, npf, o;
	void *addr;

	o = (int)bp->b_data & VAX_PGOFSET;
	npf = vax_btoc(bp->b_bcount + o) + 1;
	addr = bp->b_data;
	p = bp->b_proc;

	/*
	 * Get a pointer to the pte pointing out the first virtual address.
	 * Use different ways in kernel and user space.
	 */
	if ((bp->b_flags & B_PHYS) == 0) {
		pte = kvtopte(addr);
		if (p == 0)
			p = &proc0;
	} else {
		long xaddr = (long)addr;
		if (xaddr & 0x40000000)
			pte = &p->p_vmspace->vm_map.pmap->pm_p1br[xaddr &
			    ~0x40000000];
		else
			pte = &p->p_vmspace->vm_map.pmap->pm_p0br[xaddr];
	}

	if (map) {
		io = &map[reg];
		while (--npf > 0) {
			pfnum = pte->pg_pfn;
			if (pfnum == 0)
				panic("mapin zero entry");
			pte++;
			*(volatile int *)io++ = pfnum | flag;
		}
		*(volatile int *)io = 0;
	}
}
