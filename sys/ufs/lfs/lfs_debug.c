/*	$NetBSD: lfs_debug.c,v 1.12 2001/07/13 20:30:23 perseant Exp $	*/

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
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
 *	@(#)lfs_debug.c	8.1 (Berkeley) 6/11/93
 */

#ifdef DEBUG
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/mount.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

void 
lfs_dump_super(struct lfs *lfsp)
{
	int i;
	
	printf("%s%x\t%s%x\t%s%d\t%s%d\n",
	       "magic	 ", lfsp->lfs_magic,
	       "version	 ", lfsp->lfs_version,
	       "size	 ", lfsp->lfs_size,
	       "ssize	 ", lfsp->lfs_ssize);
	printf("%s%d\t%s%d\t%s%d\t%s%d\n",
	       "dsize	 ", lfsp->lfs_dsize,
	       "bsize	 ", lfsp->lfs_bsize,
	       "fsize	 ", lfsp->lfs_fsize,
	       "frag	 ", lfsp->lfs_frag);
	
	printf("%s%d\t%s%d\t%s%d\t%s%d\n",
	       "minfree	 ", lfsp->lfs_minfree,
	       "inopb	 ", lfsp->lfs_inopb,
	       "ifpb	 ", lfsp->lfs_ifpb,
	       "nindir	 ", lfsp->lfs_nindir);
	
	printf("%s%d\t%s%d\t%s%d\t%s%d\n",
	       "nseg	 ", lfsp->lfs_nseg,
	       "nspf	 ", lfsp->lfs_nspf,
	       "cleansz	 ", lfsp->lfs_cleansz,
	       "segtabsz ", lfsp->lfs_segtabsz);
	
	printf("%s%x\t%s%d\t%s%lx\t%s%d\n",
	       "segmask	 ", lfsp->lfs_segmask,
	       "segshift ", lfsp->lfs_segshift,
	       "bmask	 ", (unsigned long)lfsp->lfs_bmask,
	       "bshift	 ", lfsp->lfs_bshift);
	
	printf("%s%lu\t%s%d\t%s%lx\t%s%u\n",
	       "ffmask	 ", (unsigned long)lfsp->lfs_ffmask,
	       "ffshift	 ", lfsp->lfs_ffshift,
	       "fbmask	 ", (unsigned long)lfsp->lfs_fbmask,
	       "fbshift	 ", lfsp->lfs_fbshift);
	
	printf("%s%d\t%s%d\t%s%x\t%s%qx\n",
	       "sushift	 ", lfsp->lfs_sushift,
	       "fsbtodb	 ", lfsp->lfs_fsbtodb,
	       "cksum	 ", lfsp->lfs_cksum,
	       "maxfilesize ", (long long)lfsp->lfs_maxfilesize);
	
	printf("Superblock disk addresses:");
	for (i = 0; i < LFS_MAXNUMSB; i++)
		printf(" %x", lfsp->lfs_sboffs[i]);
	printf("\n");
	
	printf("Checkpoint Info\n");
	printf("%s%d\t%s%x\t%s%d\n",
	       "free	 ", lfsp->lfs_free,
	       "idaddr	 ", lfsp->lfs_idaddr,
	       "ifile	 ", lfsp->lfs_ifile);
	printf("%s%x\t%s%d\t%s%x\t%s%x\t%s%x\t%s%x\n",
	       "bfree	 ", lfsp->lfs_bfree,
	       "nfiles	 ", lfsp->lfs_nfiles,
	       "lastseg	 ", lfsp->lfs_lastseg,
	       "nextseg	 ", lfsp->lfs_nextseg,
	       "curseg	 ", lfsp->lfs_curseg,
	       "offset	 ", lfsp->lfs_offset);
	printf("tstamp	 %llx\n", (long long)lfsp->lfs_tstamp);
}

void
lfs_dump_dinode(struct dinode *dip)
{
	int i;
	
	printf("%s%u\t%s%d\t%s%u\t%s%u\t%s%qu\t%s%d\n",
	       "mode   ", dip->di_mode,
	       "nlink  ", dip->di_nlink,
	       "uid    ", dip->di_uid,
	       "gid    ", dip->di_gid,
	       "size   ", (long long)dip->di_size,
	       "blocks ", dip->di_blocks);
	printf("inum  %d\n", dip->di_inumber);
	printf("Direct Addresses\n");
	for (i = 0; i < NDADDR; i++) {
		printf("\t%x", dip->di_db[i]);
		if ((i % 6) == 5)
			printf("\n");
	}
	for (i = 0; i < NIADDR; i++)
		printf("\t%x", dip->di_ib[i]);
	printf("\n");
}

void
lfs_check_segsum(struct lfs *fs, struct segment *sp, char *file, int line)
{
	int actual, i;
#if 0
	static int offset; 
#endif
	
	if((actual = i = 1) == 1)
		return; /* XXXX not checking this anymore, really */
	
	if(sp->sum_bytes_left >= sizeof(FINFO) - sizeof(ufs_daddr_t)
	   && sp->fip->fi_nblocks > 512) {
		printf("%s:%d: fi_nblocks = %d\n",file,line,sp->fip->fi_nblocks);
#ifdef DDB
		Debugger();
#endif
	}
	
	if(sp->sum_bytes_left > 484) {
		printf("%s:%d: bad value (%d = -%d) for sum_bytes_left\n",
		       file, line, sp->sum_bytes_left, fs->lfs_sumsize-sp->sum_bytes_left);
		panic("too many bytes");
	}
	
	actual = fs->lfs_sumsize
		/* amount taken up by FINFOs */
		- ((char *)&(sp->fip->fi_blocks[sp->fip->fi_nblocks]) - (char *)(sp->segsum))
			/* amount taken up by inode blocks */
			- sizeof(ufs_daddr_t)*((sp->ninodes+INOPB(fs)-1) / INOPB(fs));
#if 0
	if(actual - sp->sum_bytes_left < offset) 
	{  
		printf("%s:%d: offset changed %d -> %d\n", file, line,
		       offset, actual-sp->sum_bytes_left);
		offset = actual - sp->sum_bytes_left;
		/* panic("byte mismatch"); */
	}
#endif
#if 0
	if(actual != sp->sum_bytes_left)
		printf("%s:%d: warning: segsum miscalc at %d (-%d => %d)\n",
		       file, line, sp->sum_bytes_left,
		       fs->lfs_sumsize-sp->sum_bytes_left,
		       actual);
#endif
	if(sp->sum_bytes_left > 0
	   && ((char *)(sp->segsum))[fs->lfs_sumsize
				     - sizeof(ufs_daddr_t) * ((sp->ninodes+INOPB(fs)-1) / INOPB(fs))
				     - sp->sum_bytes_left] != '\0') {
		printf("%s:%d: warning: segsum overwrite at %d (-%d => %d)\n",
		       file, line, sp->sum_bytes_left,
		       fs->lfs_sumsize-sp->sum_bytes_left,
		       actual);
#ifdef DDB
		Debugger();
#endif
	}
}

void
lfs_check_bpp(struct lfs *fs, struct segment *sp, char *file, int line)
{
	daddr_t blkno;
	struct buf **bpp;
	struct vnode *devvp;
	
	devvp = VTOI(fs->lfs_ivnode)->i_devvp;
	blkno = (*(sp->bpp))->b_blkno;
	for(bpp=sp->bpp; bpp < sp->cbpp; bpp++) {
		if((*bpp)->b_blkno != blkno) {
			if((*bpp)->b_vp == devvp) {
				printf("Oops, would misplace raw block 0x%x at "
				       "0x%x\n",
				       (*bpp)->b_blkno,
				       blkno);
			} else {
				printf("%s:%d: misplace ino %d lbn %d at "
				       "0x%x instead of 0x%x\n",
				       file, line,
				       VTOI((*bpp)->b_vp)->i_number, (*bpp)->b_lblkno,
				       blkno,
				       (*bpp)->b_blkno);
			}
		}
		blkno += fsbtodb(fs, btofsb(fs, (*bpp)->b_bcount));
	}
}
#endif /* DEBUG */
