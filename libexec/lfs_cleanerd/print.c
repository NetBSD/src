/*	$NetBSD: print.c,v 1.15 2003/08/07 09:46:44 agc Exp $	*/

/*-
 * Copyright (c) 1992, 1993
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
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "from: @(#)print.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: print.c,v 1.15 2003/08/07 09:46:44 agc Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#include <sys/time.h>

#include <ufs/ufs/dinode.h>
#include <ufs/lfs/lfs.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <syslog.h>

#include "clean.h"

extern int debug;
extern u_long cksum(void *, size_t);	/* XXX */

/*
 * Print out a summary block; return number of blocks in segment; 0
 * for empty segment or corrupt segment.
 * Returns a pointer to the array of inode addresses.
 */

/* XXX ondisk32 */
int
dump_summary(struct lfs *lfsp, SEGSUM *sp, u_long flags, int32_t **iaddrp, daddr_t addr)
{
	int i, j, blk, numblocks, accino=0;
	/* XXX ondisk32 */
	int32_t *dp, ddp, *idp;
	u_int32_t *datap;
	int size;
	FINFO *fp;
	u_int32_t ck;

	blk=0;
	datap = (u_int32_t *)malloc(segtod(lfsp, 1) * sizeof(u_int32_t));
	if(datap==NULL) {
	        syslog(LOG_WARNING, "cannot allocate %d in dump_summary",
		       (int)(segtod(lfsp, 1) * sizeof(u_int32_t)));
		return(-1);
	}

	if (sp->ss_sumsum != (ck = cksum(&sp->ss_datasum, 
	    lfsp->lfs_sumsize - sizeof(sp->ss_sumsum)))) {
	        syslog(LOG_WARNING, "sumsum checksum mismatch: should be %d, found %d\n",
		       (int) sp->ss_sumsum, (int) ck);
		free(datap);
		return(-1);
	}

	if (flags & DUMP_SUM_HEADER) {
                syslog(LOG_DEBUG, "    %s0x%X\t%s%d\t%s%d\n    %s0x%X\t%s0x%X",
			"next     ", sp->ss_next,
			"nfinfo   ", sp->ss_nfinfo,
			"ninos    ", sp->ss_ninos,
			"sumsum   ", sp->ss_sumsum,
			"datasum  ", sp->ss_datasum );
		syslog(LOG_DEBUG, "\tcreate   %s", ctime(
			(lfsp->lfs_version == 1 ? (time_t *)&sp->ss_ident : 
			(time_t *)&sp->ss_create)));
	}

	numblocks = (sp->ss_ninos + INOPB(lfsp) - 1) / INOPB(lfsp);

	/* Dump out inode disk addresses */
	if (flags & DUMP_INODE_ADDRS)
                syslog(LOG_DEBUG, "    Inode addresses:");

	/* XXX ondisk32 */
	idp = dp = (int32_t *)((caddr_t)sp + lfsp->lfs_sumsize);
	--idp;
	for (--dp, i = 0; i < howmany(sp->ss_ninos,INOPB(lfsp)); --dp) {
		if (flags & DUMP_INODE_ADDRS)
                        syslog(LOG_DEBUG, "\t0x%lx", (u_long)*dp);
		++i;
	}
	if (iaddrp) {
		*iaddrp = dp;
	}

	ddp = addr + btofsb(lfsp, lfsp->lfs_sumsize);
	if (lfsp->lfs_version == 1)
		fp = (FINFO *)(((char *)sp) + sizeof(SEGSUM_V1));
	else
		fp = (FINFO *)(sp + 1);
	for (i = 0; i < sp->ss_nfinfo; ++i) {
		/* Add any intervening Inode blocks to our checksum array */
		/* printf("finfo %d: ddp=%lx, *idp=%lx\n",i,ddp,*idp); */
		while(ddp == *idp) {
			 /* printf(" [ino %lx]",ddp); */
			datap[blk++] = *(u_int32_t*)((caddr_t)sp + fsbtob(lfsp, ddp-addr));
			--idp;
			ddp += btofsb(lfsp, lfsp->lfs_ibsize);
			accino++;
		}
		for(j=0;j<fp->fi_nblocks;j++) {
			if(j==fp->fi_nblocks-1) {
				size = btofsb(lfsp, fp->fi_lastlength);
				/* printf(" %lx:%d",ddp,size); */
			} else {
				size = btofsb(lfsp, lfsp->lfs_bsize);
				/* printf(" %lx/%d",ddp,size); */
			}
			datap[blk++] = *(u_int32_t*)((caddr_t)sp + fsbtob(lfsp, ddp-addr));
			ddp += size;
		}
		numblocks += fp->fi_nblocks;
		if (flags & DUMP_FINFOS) {
			syslog(LOG_DEBUG, "    %s%d version %d nblocks %d\n",
			    "FINFO for inode: ", fp->fi_ino,
			    fp->fi_version, fp->fi_nblocks);
			dp = &(fp->fi_blocks[0]);
			for (j = 0; j < fp->fi_nblocks; j++, dp++) {
                            syslog(LOG_DEBUG, "\t%d", *dp);
			}
		} else
			dp = &fp->fi_blocks[fp->fi_nblocks];
		fp = (FINFO *)dp;
		/* printf("\n"); */
	}
	/* Add lagging inode blocks too */
	/* printf("end: ddp=%lx, *idp=%lx\n",ddp,*idp); */
	while(*idp >= ddp && accino < howmany(sp->ss_ninos,INOPB(lfsp))) {
		ddp = *idp;
		/* printf(" [ino %lx]",ddp); */
		datap[blk++] = *(u_int32_t*)((caddr_t)sp + fsbtob(lfsp, ddp-addr));
		--idp;
		accino++;
	}
	/* printf("\n"); */

	if(accino != howmany(sp->ss_ninos,lfsp->lfs_inopb)) {
		syslog(LOG_DEBUG,"Oops, given %d inodes got %d\n", howmany(sp->ss_ninos,lfsp->lfs_inopb), accino);
	}
	if(blk!=numblocks) {
		syslog(LOG_DEBUG,"Oops, blk=%d numblocks=%d\n",blk,numblocks);
	}
	/* check data/inode block(s) checksum too */
	if ((ck=cksum ((void *)datap, numblocks * sizeof(u_int32_t))) != sp->ss_datasum) {
                syslog(LOG_DEBUG, "Bad data checksum: given %lu, got %lu",
		       (unsigned long)sp->ss_datasum, (unsigned long)ck);
		free(datap);
		return 0;
        }
	free(datap);

	return (numblocks);
}

void
dump_cleaner_info(void *ipage)
{
	CLEANERINFO *cip;

        if(debug <= 1)
            return;

	cip = (CLEANERINFO *)ipage;
	syslog(LOG_DEBUG,"segments clean\t%d\tsegments dirty\t%d\n\n",
	    cip->clean, cip->dirty);
}

void
dump_super(struct lfs *lfsp)
{
	int i;

        if(debug < 2)
            return;

	syslog(LOG_DEBUG,"%s0x%X\t%s0x%X\t%s%d\t%s%d\n",
		"magic    ", lfsp->lfs_magic,
		"version  ", lfsp->lfs_version,
		"size     ", lfsp->lfs_size,
		"ssize    ", lfsp->lfs_ssize);
	syslog(LOG_DEBUG, "%s%d\t\t%s%d\t%s%d\t%s%d\n",
		"dsize    ", lfsp->lfs_dsize,
		"bsize    ", lfsp->lfs_bsize,
		"fsize    ", lfsp->lfs_fsize,
		"frag     ", lfsp->lfs_frag);

	syslog(LOG_DEBUG, "%s%d\t\t%s%d\t%s%d\t%s%d\n",
		"minfree  ", lfsp->lfs_minfree,
		"inopb    ", lfsp->lfs_inopb,
		"ifpb     ", lfsp->lfs_ifpb,
		"nindir   ", lfsp->lfs_nindir);

	syslog(LOG_DEBUG, "%s%d\t\t%s%d\t%s%d\t%s%d\n",
		"nseg     ", lfsp->lfs_nseg,
		"nspf     ", lfsp->lfs_nspf,
		"cleansz  ", lfsp->lfs_cleansz,
		"segtabsz ", lfsp->lfs_segtabsz);

	syslog(LOG_DEBUG, "%s0x%X\t%s%d\t%s0x%llX\t%s%lu\n",
		"segmask  ", lfsp->lfs_segmask,
		"segshift ", lfsp->lfs_segshift,
		"bmask    ", (long long)lfsp->lfs_bmask,
		"bshift   ", (u_long)lfsp->lfs_bshift);

	syslog(LOG_DEBUG, "%s0x%llX\t\t%s%lu\t%s0x%llX\t%s%lu\n",
		"ffmask   ", (long long)lfsp->lfs_ffmask,
		"ffshift  ", (u_long)lfsp->lfs_ffshift,
		"fbmask   ", (long long)lfsp->lfs_fbmask,
		"fbshift  ", (u_long)lfsp->lfs_fbshift);

	syslog(LOG_DEBUG, "%s%d\t\t%s0x%X\t%s0x%llx\n",
		"fsbtodb  ", lfsp->lfs_fsbtodb,
		"cksum    ", lfsp->lfs_cksum,
		"maxfilesize  ", (long long)lfsp->lfs_maxfilesize);

	syslog(LOG_DEBUG, "Superblock disk addresses:\t");
	for (i = 0; i < LFS_MAXNUMSB; i++) {
		syslog(LOG_DEBUG, " 0x%X", lfsp->lfs_sboffs[i]);
	}

	syslog(LOG_DEBUG, "Checkpoint Info\n");
	syslog(LOG_DEBUG, "%s%d\t%s0x%X\t%s%d\n",
		"freehd   ", lfsp->lfs_freehd,
		"idaddr   ", lfsp->lfs_idaddr,
		"ifile    ", lfsp->lfs_ifile);
	syslog(LOG_DEBUG, "%s%d\t%s%d\t%s%d\n",
		"bfree    ", lfsp->lfs_bfree,
		"avail    ", lfsp->lfs_avail,
		"uinodes  ", lfsp->lfs_uinodes);
	syslog(LOG_DEBUG, "%s%d\t%s0x%X\t%s0x%X\n%s0x%X\t%s0x%X\t",
		"nfiles   ", lfsp->lfs_nfiles,
		"lastseg  ", lfsp->lfs_lastseg,
		"nextseg  ", lfsp->lfs_nextseg,
		"curseg   ", lfsp->lfs_curseg,
		"offset   ", lfsp->lfs_offset);
	syslog(LOG_DEBUG, "tstamp   %s", ctime((time_t *)&lfsp->lfs_tstamp));
	syslog(LOG_DEBUG, "\nIn-Memory Information\n");
	syslog(LOG_DEBUG, "%s%d\t%s0x%X\t%s%d\t%s%d\t%s%d\n",
		"seglock  ", lfsp->lfs_seglock,
		"iocount  ", lfsp->lfs_iocount,
		"writer   ", lfsp->lfs_writer,
		"dirops   ", lfsp->lfs_dirops,
		"doifile  ", lfsp->lfs_doifile );
	syslog(LOG_DEBUG, "%s%d\t%s%d\t%s0x%X\t%s%d\n",
		"nactive  ", lfsp->lfs_nactive,
		"fmod     ", lfsp->lfs_fmod,
	        "pflags   ", lfsp->lfs_pflags,
		"ronly    ", lfsp->lfs_ronly);
}
