/*	$NetBSD: pass0.c,v 1.3 1999/07/03 19:55:03 kleink Exp $	*/

/*
 * Copyright (c) 1998 Konrad E. Schroder.
 * Copyright (c) 1980, 1986, 1993
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
 */

#include <sys/param.h>
#include <sys/time.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <sys/mount.h>
#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fsck.h"
#include "extern.h"
#include "fsutil.h"

/* Flags for check_segment */
#define CKSEG_VERBOSE     1
#define CKSEG_IGNORECLEAN 2

extern int fake_cleanseg;
void check_segment(int, int, daddr_t, struct lfs *, int, int (*)(struct lfs *, SEGSUM *, int, int, daddr_t, daddr_t));
static int check_summary(struct lfs *, SEGSUM *, int, int, daddr_t, daddr_t);
SEGUSE *lfs_gseguse(int);

struct dinode **din_table;
SEGUSE *seg_table;
/*
 * Pass 0.  Check the LFS partial segments for valid checksums, correcting
 * if necessary.  Also check for valid offsets for inode and finfo blocks.
 */
/* XXX more could be done here---consistency between inode-held blocks and
   finfo blocks, for one thing. */

#define dbshift (sblock.lfs_bshift - sblock.lfs_fsbtodb)

void pass0()
{
    int i;
    daddr_t seg_addr;

    /*
     * Run through the Ifile, to count and load the inodes into
     * a dynamic inode table.
     */
    
    /*
     * Check the segments
     */

    seg_addr = sblock.lfs_sboffs[0];
    for(i=0; i < sblock.lfs_nseg; i++)
    {
	    seg_table[i] = *(lfs_gseguse(i));
	    check_segment(fsreadfd, i, seg_addr, &sblock,
			  CKSEG_IGNORECLEAN|CKSEG_VERBOSE, check_summary);
	    seg_addr += (sblock.lfs_ssize << sblock.lfs_fsbtodb);
    }
#if 0
    /* check to see which inodes we didn't get */
    printf("Unavailable inodes: ");
    for(i=1;i<maxino;i++) /* XXX ino 0 is never present */
        if(din_table[i]==NULL)
        {
            struct ifile *ifp;

            ifp = lfs_ientry(i);
            printf("(%d should be at daddr 0x%x)",i,ifp->if_daddr);
        }
    putchar('\n');
#endif
}

static void dump_segsum(SEGSUM *sump, daddr_t addr)
{
    printf("Dump partial summary block 0x%x\n",addr);
    printf("\tsumsum:  %x (%d)\n",sump->ss_sumsum,sump->ss_sumsum);
    printf("\tdatasum: %x (%d)\n",sump->ss_datasum,sump->ss_datasum);
    printf("\tnext:    %x (%d)\n",sump->ss_next,sump->ss_next);
    printf("\tcreate:  %x (%d)\n",sump->ss_create,sump->ss_create);
    printf("\tnfinfo:  %x (%d)\n",sump->ss_nfinfo,sump->ss_nfinfo);
    printf("\tninos:   %x (%d)\n",sump->ss_ninos,sump->ss_ninos);
    printf("\tflags:   %c%c\n",
           sump->ss_flags&SS_DIROP?'d':'-',
           sump->ss_flags&SS_CONT?'c':'-');
}

void check_segment(int fd, int segnum, daddr_t addr, struct lfs *fs, int flags, int (*func)(struct lfs *, SEGSUM *, int, int, daddr_t, daddr_t))
{
    struct lfs *sbp;
    SEGSUM *sump=NULL;
    SEGUSE su;
    struct bufarea *bp = NULL;
    int psegnum=0, ninos=0;
    off_t sum_offset, db_ssize;
    int bc;

    db_ssize = sblock.lfs_ssize << sblock.lfs_fsbtodb;
    su = *(lfs_gseguse(segnum)); /* structure copy */

    if(fake_cleanseg >= 0 && segnum == fake_cleanseg)
        seg_table[segnum].su_flags &= ~SEGUSE_DIRTY;

    /* printf("Seg at 0x%x\n",addr); */
    if((flags & CKSEG_VERBOSE) && segnum*db_ssize + fs->lfs_sboffs[0] != addr)
        pwarn("WARNING: segment begins at 0x%qx, should be 0x%qx\n",
              (long long unsigned)addr, 
			  (long long unsigned)(segnum*db_ssize + fs->lfs_sboffs[0]));
    sum_offset = ((off_t)addr << dbshift);

    /* If this segment should have a superblock, look for one */
    if(su.su_flags & SEGUSE_SUPERBLOCK) {
        bp = getddblk(sum_offset >> dbshift, LFS_SBPAD);
        sum_offset += LFS_SBPAD;

        /* check for a superblock -- XXX this is crude */
        sbp = (struct lfs *)(bp->b_un.b_buf);
        if(sbp->lfs_magic==LFS_MAGIC) {
#if 0
            if(sblock.lfs_tstamp == sbp->lfs_tstamp
               && memcmp(sbp,&sblock,sizeof(*sbp))
	       && (flags & CKSEG_VERBOSE))
                pwarn("SUPERBLOCK MISMATCH SEGMENT %d\n", segnum);
#endif
        } else {
		if(flags & CKSEG_VERBOSE)
			pwarn("SEGMENT %d SUPERBLOCK INVALID\n",segnum);
		/* XXX allow to fix */
        }
        bp->b_flags &= ~B_INUSE;
    }
    /* XXX need to also check whether this one *should* be dirty */
    if((flags & CKSEG_IGNORECLEAN) && (su.su_flags & SEGUSE_DIRTY)==0)
        return;

    while(1) {
        if(su.su_nsums <= psegnum)
            break;
        bp = getddblk(sum_offset >> dbshift, LFS_SUMMARY_SIZE);
        sump = (SEGSUM *)(bp->b_un.b_buf);
	if (sump->ss_magic != SS_MAGIC) {
		if(flags & CKSEG_VERBOSE)
			printf("PARTIAL SEGMENT %d SEGMENT %d BAD PARTIAL SEGMENT MAGIC (0x%x should be 0x%x)\n",
			       psegnum, segnum, sump->ss_magic, SS_MAGIC);
                bp->b_flags &= ~B_INUSE;
		break;
	}
        if(sump->ss_sumsum != cksum(&sump->ss_datasum, LFS_SUMMARY_SIZE - sizeof(sump->ss_sumsum))) {
		if(flags & CKSEG_VERBOSE) {
			/* Corrupt partial segment */
			pwarn("CORRUPT PARTIAL SEGMENT %d/%d OF SEGMENT %d AT BLK 0x%qx",
			      psegnum, su.su_nsums, segnum, 
				  (unsigned long long)sum_offset>>dbshift);
			if(db_ssize < (sum_offset>>dbshift) - addr)
				pwarn(" (+0x%qx/0x%qx)",
				      (unsigned long long)(((sum_offset>>dbshift) - addr) - 
										   db_ssize),
				      (unsigned long long)db_ssize);
			else
				pwarn(" (-0x%qx/0x%qx)",
				      (unsigned long long)(db_ssize - 
										   ((sum_offset>>dbshift) - addr)),
				      (unsigned long long)db_ssize);
			pwarn("\n");
			dump_segsum(sump,sum_offset>>dbshift);
		}
		/* XXX fix it maybe */
		bp->b_flags &= ~B_INUSE;
		break; /* XXX could be throwing away data, but if this segsum
			* is invalid, how to know where the next summary
			* begins? */
        }
	/*
	 * Good partial segment
	 */
	bc = (*func)(&sblock, sump, segnum, psegnum, addr, (daddr_t)(sum_offset>>dbshift));
	if(bc) {
                sum_offset += LFS_SUMMARY_SIZE + bc;
                ninos += (sump->ss_ninos + INOPB(&sblock)-1)/INOPB(&sblock);
                psegnum++;
	} else {
                bp->b_flags &= ~B_INUSE;
                break;
	}
        bp->b_flags &= ~B_INUSE;
    }
    if(flags & CKSEG_VERBOSE) {
	    if(ninos != su.su_ninos)
		    pwarn("SEGMENT %d has %d ninos, not %d\n",
			  segnum, ninos, su.su_ninos);
	    if(psegnum != su.su_nsums)
		    pwarn("SEGMENT %d has %d summaries, not %d\n",
			  segnum, psegnum, su.su_nsums);
    }

    return;
}

static int check_summary(struct lfs *fs, SEGSUM *sp, int sn, int pn, daddr_t seg_addr, daddr_t pseg_addr)
{
    FINFO *fp;
    int bc; /* Bytes in partial segment */
    daddr_t *dp;
    struct dinode *ip;
    struct bufarea *bp;
    int i, j, n;

    /* printf("Pseg at 0x%x, %d inos, %d finfos\n",addr>>dbshift,sp->ss_ninos,sp->ss_nfinfo); */
    /* We've already checked the sumsum, just do the data bounds and sum */
    bc = ((sp->ss_ninos + INOPB(fs) - 1) / INOPB(fs)) << fs->lfs_bshift;
    dp = (daddr_t *)sp;
    dp += LFS_SUMMARY_SIZE / sizeof(daddr_t);
    dp--;
    i=0;
#if 0
    if(sp->ss_ninos==0 && sp->ss_nfinfo==0) {
        pwarn("SEGMENT %d PSEG %d IS EMPTY\n",sn,pn);
        return 0;
    }
#endif
    while(i<sp->ss_ninos)
    {
        if(*dp < seg_addr || *dp > seg_addr + (sblock.lfs_ssize<<dbshift))
        {
            printf("Disk address 0x%x but should be in 0x%x--0x%x\n",
                   *dp, seg_addr, seg_addr+(sblock.lfs_ssize<<dbshift));
        }
        bp = getddblk(*dp, (1<<fs->lfs_bshift));
        ip = (struct dinode *)bp->b_un.b_buf;
        for(j=0; i<sp->ss_ninos && j<INOPB(fs); j++) {
            /* check range */
            if(ip[j].di_inumber < 0 || ip[j].di_inumber > maxino)
            {
                pwarn("BAD INUM %d IN SEGMENT %d PARTIAL %d\n",
                      ip[j].di_inumber, sn, pn);
                /* XXX Allow to fix (zero) this */
            } else {
                /* take a record of the highest-generation of each inode */
                n = ip[j].di_inumber;
#if 1
                if(din_table[n]==NULL || din_table[n]->di_gen < ip[j].di_gen) {
                    if(!din_table[n])
                        din_table[n] = (struct dinode *)
                            malloc(sizeof(struct dinode));
                    memcpy(din_table[n],&ip[j],sizeof(struct dinode));
                }
#endif
            }
            i++;
        }
        bp->b_flags &= ~B_INUSE;
        dp--;
    }
    fp = (FINFO *)(sp + 1);
    for(i=0; i<sp->ss_nfinfo; i++) {
        bc +=  fp->fi_lastlength + ((fp->fi_nblocks-1) << fs->lfs_bshift);
        /* XXX for now, ignore mismatches between segments' FINFO and IFILE */
        dp = &(fp->fi_blocks[0]);
        for(j=0; j<fp->fi_nblocks; j++) {
            /* XXX check values against inodes' known block size */
            dp++;
        }
        fp = (FINFO *)dp;
    }
    return bc;
}
