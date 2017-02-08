/*	$NetBSD: mkfs.c,v 1.128 2017/02/08 16:11:40 rin Exp $	*/

/*
 * Copyright (c) 1980, 1989, 1993
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

/*
 * Copyright (c) 2002 Networks Associates Technology, Inc.
 * All rights reserved.
 *
 * This software was developed for the FreeBSD Project by Marshall
 * Kirk McKusick and Network Associates Laboratories, the Security
 * Research Division of Network Associates, Inc. under DARPA/SPAWAR
 * contract N66001-01-C-8035 ("CBOSS"), as part of the DARPA CHATS
 * research program
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

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)mkfs.c	8.11 (Berkeley) 5/3/95";
#else
__RCSID("$NetBSD: mkfs.c,v 1.128 2017/02/08 16:11:40 rin Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/ufs_bswap.h>
#include <ufs/ufs/quota2.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>

#include <err.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>

#ifndef STANDALONE
#include <stdio.h>
#endif

#include "extern.h"

union dinode {
	struct ufs1_dinode dp1;
	struct ufs2_dinode dp2;
};

static void initcg(int, const struct timeval *);
static int fsinit(const struct timeval *, mode_t, uid_t, gid_t);
union Buffer;
static int makedir(union Buffer *, struct direct *, int);
static daddr_t alloc(int, int);
static void iput(union dinode *, ino_t);
static void rdfs(daddr_t, int, void *);
static void wtfs(daddr_t, int, void *);
static int isblock(struct fs *, unsigned char *, int);
static void clrblock(struct fs *, unsigned char *, int);
static void setblock(struct fs *, unsigned char *, int);
static int ilog2(int);
static void zap_old_sblock(int);
#ifdef MFS
static void *mkfs_malloc(size_t size);
#endif

/*
 * make file system for cylinder-group style file systems
 */
#define	UMASK		0755

union {
	struct fs fs;
	char data[SBLOCKSIZE];
} *fsun;
#define	sblock	fsun->fs

union Buffer {
	struct quota2_header q2h;
	char data[MAXBSIZE];
};

struct	csum *fscs_0;		/* first block of cylinder summaries */
struct	csum *fscs_next;	/* place for next summary */
struct	csum *fscs_end;		/* end of summary buffer */
struct	csum *fscs_reset;	/* place for next summary after write */
uint	fs_csaddr;		/* fragment number to write to */

union {
	struct cg cg;
	char pad[MAXBSIZE];
} *cgun;
#define	acg	cgun->cg

#define DIP(dp, field) \
	((sblock.fs_magic == FS_UFS1_MAGIC) ? \
	(dp)->dp1.di_##field : (dp)->dp2.di_##field)

#define EXT2FS_SBOFF	1024	/* XXX: SBOFF in <ufs/ext2fs/ext2fs.h> */

char *iobuf;
int iobufsize;			/* size to end of 2nd inode block */
int iobuf_memsize;		/* Actual buffer size */

int	fsi, fso;

static void
fserr(int num)
{
#ifdef GARBAGE
	extern int Gflag;

	if (Gflag)
		return;
#endif
	exit(num);
}

void
mkfs(const char *fsys, int fi, int fo,
    mode_t mfsmode, uid_t mfsuid, gid_t mfsgid)
{
	uint fragsperinodeblk, ncg, u;
	uint cgzero;
	uint64_t inodeblks, cgall;
	int32_t cylno, i, csfrags;
	int inodes_per_cg;
	struct timeval tv;
	long long sizepb;
	int len, col, delta, fld_width, max_cols;
	struct winsize winsize;

#ifndef STANDALONE
	gettimeofday(&tv, NULL);
#endif
#ifdef MFS
	if (mfs && !Nflag) {
		if ((membase = mkfs_malloc(fssize * sectorsize)) == NULL)
			exit(12);
	}
#endif
	if ((fsun = calloc(1, sizeof(*fsun))) == NULL)
		exit(12);
	if ((cgun = calloc(1, sizeof(*cgun))) == NULL)
		exit(12);

	fsi = fi;
	fso = fo;
	if (Oflag == 0) {
		sblock.fs_old_inodefmt = FS_42INODEFMT;
		sblock.fs_maxsymlinklen = 0;
		sblock.fs_old_flags = 0;
	} else {
		sblock.fs_old_inodefmt = FS_44INODEFMT;
		sblock.fs_maxsymlinklen = (Oflag == 1 ? UFS1_MAXSYMLINKLEN :
		    UFS2_MAXSYMLINKLEN);
		sblock.fs_old_flags = FS_FLAGS_UPDATED;
		if (isappleufs)
			sblock.fs_old_flags = 0;
		sblock.fs_flags = 0;
	}

	/*
	 * collect and verify the filesystem density info
	 */
	sblock.fs_avgfilesize = avgfilesize;
	sblock.fs_avgfpdir = avgfpdir;
	if (sblock.fs_avgfilesize <= 0) {
		printf("illegal expected average file size %d\n",
		    sblock.fs_avgfilesize);
		fserr(14);
	}
	if (sblock.fs_avgfpdir <= 0) {
		printf("illegal expected number of files per directory %d\n",
		    sblock.fs_avgfpdir);
		fserr(15);
	}
	/*
	 * collect and verify the block and fragment sizes
	 */
	sblock.fs_bsize = bsize;
	sblock.fs_fsize = fsize;
	if (!powerof2(sblock.fs_bsize)) {
		printf("block size must be a power of 2, not %d\n",
		    sblock.fs_bsize);
		fserr(16);
	}
	if (!powerof2(sblock.fs_fsize)) {
		printf("fragment size must be a power of 2, not %d\n",
		    sblock.fs_fsize);
		fserr(17);
	}
	if (sblock.fs_fsize < sectorsize) {
		printf("fragment size %d is too small, minimum is %d\n",
		    sblock.fs_fsize, sectorsize);
		fserr(18);
	}
	if (sblock.fs_bsize < MINBSIZE) {
		printf("block size %d is too small, minimum is %d\n",
		    sblock.fs_bsize, MINBSIZE);
		fserr(19);
	}
	if (sblock.fs_bsize > MAXBSIZE) {
		printf("block size %d is too large, maximum is %d\n",
		    sblock.fs_bsize, MAXBSIZE);
		fserr(19);
	}
	if (sblock.fs_bsize < sblock.fs_fsize) {
		printf("block size (%d) cannot be smaller than fragment size (%d)\n",
		    sblock.fs_bsize, sblock.fs_fsize);
		fserr(20);
	}

	if (maxbsize < bsize || !powerof2(maxbsize)) {
		sblock.fs_maxbsize = sblock.fs_bsize;
	} else if (sblock.fs_maxbsize > FS_MAXCONTIG * sblock.fs_bsize) {
		sblock.fs_maxbsize = FS_MAXCONTIG * sblock.fs_bsize;
	} else {
		sblock.fs_maxbsize = maxbsize;
	}
	sblock.fs_maxcontig = maxcontig;
	if (sblock.fs_maxcontig < sblock.fs_maxbsize / sblock.fs_bsize) {
		sblock.fs_maxcontig = sblock.fs_maxbsize / sblock.fs_bsize;
		if (verbosity > 0)
			printf("Maxcontig raised to %d\n", sblock.fs_maxbsize);
	}
	if (sblock.fs_maxcontig > 1)
		sblock.fs_contigsumsize = MIN(sblock.fs_maxcontig,FS_MAXCONTIG);

	sblock.fs_bmask = ~(sblock.fs_bsize - 1);
	sblock.fs_fmask = ~(sblock.fs_fsize - 1);
	sblock.fs_qbmask = ~sblock.fs_bmask;
	sblock.fs_qfmask = ~sblock.fs_fmask;
	for (sblock.fs_bshift = 0, i = sblock.fs_bsize; i > 1; i >>= 1)
		sblock.fs_bshift++;
	for (sblock.fs_fshift = 0, i = sblock.fs_fsize; i > 1; i >>= 1)
		sblock.fs_fshift++;
	sblock.fs_frag = ffs_numfrags(&sblock, sblock.fs_bsize);
	for (sblock.fs_fragshift = 0, i = sblock.fs_frag; i > 1; i >>= 1)
		sblock.fs_fragshift++;
	if (sblock.fs_frag > MAXFRAG) {
		printf("fragment size %d is too small, "
			"minimum with block size %d is %d\n",
		    sblock.fs_fsize, sblock.fs_bsize,
		    sblock.fs_bsize / MAXFRAG);
		fserr(21);
	}
	sblock.fs_fsbtodb = ilog2(sblock.fs_fsize / sectorsize);
	sblock.fs_size = FFS_DBTOFSB(&sblock, fssize);
	if (Oflag <= 1) {
		if ((uint64_t)sblock.fs_size >= 1ull << 31) {
			printf("Too many fragments (0x%" PRIx64
			    ") for a FFSv1 filesystem\n", sblock.fs_size);
			fserr(22);
		}
		sblock.fs_magic = FS_UFS1_MAGIC;
		sblock.fs_sblockloc = SBLOCK_UFS1;
		sblock.fs_nindir = sblock.fs_bsize / sizeof(int32_t);
		sblock.fs_inopb = sblock.fs_bsize / sizeof(struct ufs1_dinode);
		sblock.fs_old_cgoffset = 0;
		sblock.fs_old_cgmask = 0xffffffff;
		sblock.fs_old_size = sblock.fs_size;
		sblock.fs_old_rotdelay = 0;
		sblock.fs_old_rps = 60;
		sblock.fs_old_nspf = sblock.fs_fsize / sectorsize;
		sblock.fs_old_cpg = 1;
		sblock.fs_old_interleave = 1;
		sblock.fs_old_trackskew = 0;
		sblock.fs_old_cpc = 0;
		sblock.fs_old_postblformat = FS_DYNAMICPOSTBLFMT;
		sblock.fs_old_nrpos = 1;
	} else {
		sblock.fs_magic = FS_UFS2_MAGIC;
		sblock.fs_sblockloc = SBLOCK_UFS2;
		sblock.fs_nindir = sblock.fs_bsize / sizeof(int64_t);
		sblock.fs_inopb = sblock.fs_bsize / sizeof(struct ufs2_dinode);
	}

	sblock.fs_sblkno =
	    roundup(howmany(sblock.fs_sblockloc + SBLOCKSIZE, sblock.fs_fsize),
		sblock.fs_frag);
	sblock.fs_cblkno = (daddr_t)(sblock.fs_sblkno +
	    roundup(howmany(SBLOCKSIZE, sblock.fs_fsize), sblock.fs_frag));
	sblock.fs_iblkno = sblock.fs_cblkno + sblock.fs_frag;
	sblock.fs_maxfilesize = sblock.fs_bsize * UFS_NDADDR - 1;
	for (sizepb = sblock.fs_bsize, i = 0; i < UFS_NIADDR; i++) {
		sizepb *= FFS_NINDIR(&sblock);
		sblock.fs_maxfilesize += sizepb;
	}

	/*
	 * Calculate the number of blocks to put into each cylinder group.
	 *
	 * The cylinder group size is limited because the data structure
	 * must fit into a single block.
	 * We try to have as few cylinder groups as possible, with a proviso
	 * that we create at least MINCYLGRPS (==4) except for small
	 * filesystems.
	 *
	 * This algorithm works out how many blocks of inodes would be
	 * needed to fill the entire volume at the specified density.
	 * It then looks at how big the 'cylinder block' would have to
	 * be and, assuming that it is linearly related to the number
	 * of inodes and blocks how many cylinder groups are needed to
	 * keep the cylinder block below the filesystem block size.
	 *
	 * The cylinder groups are then all created with the average size.
	 *
	 * Space taken by the red tape on cylinder groups other than the
	 * first is ignored.
	 */

	/* There must be space for 1 inode block and 2 data blocks */
	if (sblock.fs_size < sblock.fs_iblkno + 3 * sblock.fs_frag) {
		printf("Filesystem size %lld < minimum size of %d\n",
		    (long long)sblock.fs_size, sblock.fs_iblkno + 3 * sblock.fs_frag);
		fserr(23);
	}
	if (num_inodes != 0)
		inodeblks = howmany(num_inodes, FFS_INOPB(&sblock));
	else {
		/*
		 * Calculate 'per inode block' so we can allocate less than
		 * 1 fragment per inode - useful for /dev.
		 */
		fragsperinodeblk = MAX(ffs_numfrags(&sblock,
					(uint64_t)density * FFS_INOPB(&sblock)), 1);
		inodeblks = (sblock.fs_size - sblock.fs_iblkno) /	
			(sblock.fs_frag + fragsperinodeblk);
	}
	if (inodeblks == 0)
		inodeblks = 1;
	/* Ensure that there are at least 2 data blocks (or we fail below) */
	if (inodeblks > (uint64_t)(sblock.fs_size - sblock.fs_iblkno)/sblock.fs_frag - 2)
		inodeblks = (sblock.fs_size-sblock.fs_iblkno)/sblock.fs_frag-2;
	/* Even UFS2 limits number of inodes to 2^31 (fs_ipg is int32_t) */
	if (inodeblks * FFS_INOPB(&sblock) >= 1ull << 31)
		inodeblks = ((1ull << 31) - NBBY) / FFS_INOPB(&sblock);
	/*
	 * See what would happen if we tried to use 1 cylinder group.
	 * Assume space linear, so work out number of cylinder groups needed.
	 */
	cgzero = CGSIZE_IF(&sblock, 0, 0);
	cgall = CGSIZE_IF(&sblock, inodeblks * FFS_INOPB(&sblock), sblock.fs_size);
	ncg = howmany(cgall - cgzero, sblock.fs_bsize - cgzero);
	if (ncg < MINCYLGRPS) {
		/*
		 * We would like to allocate MINCLYGRPS cylinder groups,
		 * but for small file sytems (especially ones with a lot
		 * of inodes) this is not desirable (or possible).
		 */
		u = sblock.fs_size / 2 / (sblock.fs_iblkno +
						inodeblks * sblock.fs_frag);
		if (u > ncg)
			ncg = u;
		if (ncg > MINCYLGRPS)
			ncg = MINCYLGRPS;
		if (ncg > inodeblks)
			ncg = inodeblks;
	}
	/*
	 * Put an equal number of blocks in each cylinder group.
	 * Round up so we don't have more fragments in the last CG than
	 * the earlier ones (does that matter?), but kill a block if the
	 * CGSIZE becomes too big (only happens if there are a lot of CGs).
	 */
	sblock.fs_fpg = roundup(howmany(sblock.fs_size, ncg), sblock.fs_frag);
	/* Round up the fragments/group so the bitmap bytes are full */
	sblock.fs_fpg = roundup(sblock.fs_fpg, NBBY);
	inodes_per_cg = ((inodeblks - 1) / ncg + 1) * FFS_INOPB(&sblock);

	i = CGSIZE_IF(&sblock, inodes_per_cg, sblock.fs_fpg);
	if (i > sblock.fs_bsize) {
		sblock.fs_fpg -= (i - sblock.fs_bsize) * NBBY;
		/* ... and recalculate how many cylinder groups we now need */
		ncg = howmany(sblock.fs_size, sblock.fs_fpg);
		inodes_per_cg = ((inodeblks - 1) / ncg + 1) * FFS_INOPB(&sblock);
	}
	sblock.fs_ipg = inodes_per_cg;
	/* Sanity check on our sums... */
	if ((int)CGSIZE(&sblock) > sblock.fs_bsize) {
		printf("CGSIZE miscalculated %d > %d\n",
		    (int)CGSIZE(&sblock), sblock.fs_bsize);
		fserr(24);
	}

	sblock.fs_dblkno = sblock.fs_iblkno + sblock.fs_ipg / FFS_INOPF(&sblock);
	/* Check that the last cylinder group has enough space for the inodes */
	i = sblock.fs_size - sblock.fs_fpg * (ncg - 1ull);
	if (i < sblock.fs_dblkno) {
		/*
		 * Since we make all the cylinder groups the same size, the
		 * last will only be small if there are a large number of
		 * cylinder groups. If we pull even a fragment from each
		 * of the other groups then the last CG will be overfull.
		 * So we just kill the last CG.
		 */
		ncg--;
		sblock.fs_size -= i;
	}
	sblock.fs_ncg = ncg;

	sblock.fs_cgsize = ffs_fragroundup(&sblock, CGSIZE(&sblock));
	if (Oflag <= 1) {
		sblock.fs_old_spc = sblock.fs_fpg * sblock.fs_old_nspf;
		sblock.fs_old_nsect = sblock.fs_old_spc;
		sblock.fs_old_npsect = sblock.fs_old_spc;
		sblock.fs_old_ncyl = sblock.fs_ncg;
	}

	/*
	 * Cylinder group summary information for each cylinder is written
	 * into the first cylinder group.
	 * Write this fragment by fragment, but doing the first CG last
	 * (after we've taken stuff off for the structure itself and the
	 * root directory.
	 */
	sblock.fs_csaddr = cgdmin(&sblock, 0);
	sblock.fs_cssize =
	    ffs_fragroundup(&sblock, sblock.fs_ncg * sizeof(struct csum));
	if (512 % sizeof *fscs_0)
		errx(1, "cylinder group summary doesn't fit in sectors");
	fscs_0 = mmap(0, 2 * sblock.fs_fsize, PROT_READ|PROT_WRITE,
			MAP_ANON|MAP_PRIVATE, -1, 0);
	if (fscs_0 == MAP_FAILED)
		exit(39);
	memset(fscs_0, 0, 2 * sblock.fs_fsize);
	fs_csaddr = sblock.fs_csaddr;
	fscs_next = fscs_0;
	fscs_end = (void *)((char *)fscs_0 + 2 * sblock.fs_fsize);
	fscs_reset = (void *)((char *)fscs_0 + sblock.fs_fsize);
	/*
	 * fill in remaining fields of the super block
	 */
	sblock.fs_sbsize = ffs_fragroundup(&sblock, sizeof(struct fs));
	if (sblock.fs_sbsize > SBLOCKSIZE)
		sblock.fs_sbsize = SBLOCKSIZE;
	sblock.fs_minfree = minfree;
	sblock.fs_maxcontig = maxcontig;
	sblock.fs_maxbpg = maxbpg;
	sblock.fs_optim = opt;
	sblock.fs_cgrotor = 0;
	sblock.fs_pendingblocks = 0;
	sblock.fs_pendinginodes = 0;
	sblock.fs_cstotal.cs_ndir = 0;
	sblock.fs_cstotal.cs_nbfree = 0;
	sblock.fs_cstotal.cs_nifree = 0;
	sblock.fs_cstotal.cs_nffree = 0;
	sblock.fs_fmod = 0;
	sblock.fs_ronly = 0;
	sblock.fs_state = 0;
	sblock.fs_clean = FS_ISCLEAN;
	sblock.fs_ronly = 0;
	sblock.fs_id[0] = (long)tv.tv_sec;	/* XXXfvdl huh? */
	sblock.fs_id[1] = arc4random() & INT32_MAX;
	sblock.fs_fsmnt[0] = '\0';
	csfrags = howmany(sblock.fs_cssize, sblock.fs_fsize);
	sblock.fs_dsize = sblock.fs_size - sblock.fs_sblkno -
	    sblock.fs_ncg * (sblock.fs_dblkno - sblock.fs_sblkno);
	sblock.fs_cstotal.cs_nbfree =
	    ffs_fragstoblks(&sblock, sblock.fs_dsize) -
	    howmany(csfrags, sblock.fs_frag);
	sblock.fs_cstotal.cs_nffree =
	    ffs_fragnum(&sblock, sblock.fs_size) +
	    (ffs_fragnum(&sblock, csfrags) > 0 ?
	    sblock.fs_frag - ffs_fragnum(&sblock, csfrags) : 0);
	sblock.fs_cstotal.cs_nifree = sblock.fs_ncg * sblock.fs_ipg - UFS_ROOTINO;
	sblock.fs_cstotal.cs_ndir = 0;
	sblock.fs_dsize -= csfrags;
	sblock.fs_time = tv.tv_sec;
	if (Oflag <= 1) {
		sblock.fs_old_time = tv.tv_sec;
		sblock.fs_old_dsize = sblock.fs_dsize;
		sblock.fs_old_csaddr = sblock.fs_csaddr;
		sblock.fs_old_cstotal.cs_ndir = sblock.fs_cstotal.cs_ndir;
		sblock.fs_old_cstotal.cs_nbfree = sblock.fs_cstotal.cs_nbfree;
		sblock.fs_old_cstotal.cs_nifree = sblock.fs_cstotal.cs_nifree;
		sblock.fs_old_cstotal.cs_nffree = sblock.fs_cstotal.cs_nffree;
	}
	/* add quota data in superblock */
	if (quotas) {
		sblock.fs_flags |= FS_DOQUOTA2;
		sblock.fs_quota_magic = Q2_HEAD_MAGIC;
		sblock.fs_quota_flags = quotas;
	}
	/*
	 * Dump out summary information about file system.
	 */
	if (verbosity > 0) {
#define	B2MBFACTOR (1 / (1024.0 * 1024.0))
		printf("%s: %.1fMB (%lld sectors) block size %d, "
		       "fragment size %d\n",
		    fsys, (float)sblock.fs_size * sblock.fs_fsize * B2MBFACTOR,
		    (long long)FFS_FSBTODB(&sblock, sblock.fs_size),
		    sblock.fs_bsize, sblock.fs_fsize);
		printf("\tusing %d cylinder groups of %.2fMB, %d blks, "
		       "%d inodes.\n",
		    sblock.fs_ncg,
		    (float)sblock.fs_fpg * sblock.fs_fsize * B2MBFACTOR,
		    sblock.fs_fpg / sblock.fs_frag, sblock.fs_ipg);
#undef B2MBFACTOR
	}

	/*
	 * allocate space for superblock, cylinder group map, and
	 * two sets of inode blocks.
	 */
	if (sblock.fs_bsize < SBLOCKSIZE)
		iobufsize = SBLOCKSIZE + 3 * sblock.fs_bsize;
	else
		iobufsize = 4 * sblock.fs_bsize;
	iobuf_memsize = iobufsize;
	if (!mfs && sblock.fs_magic == FS_UFS1_MAGIC) {
		/* A larger buffer so we can write multiple inode blks */
		iobuf_memsize += 14 * sblock.fs_bsize;
	}
	for (;;) {
		iobuf = mmap(0, iobuf_memsize, PROT_READ|PROT_WRITE,
				MAP_ANON|MAP_PRIVATE, -1, 0);
		if (iobuf != MAP_FAILED)
			break;
		if (iobuf_memsize != iobufsize) {
			/* Try again with the smaller size */
			iobuf_memsize = iobufsize;
			continue;
		}
		printf("Cannot allocate I/O buffer\n");
		exit(38);
	}
	memset(iobuf, 0, iobuf_memsize);

	/*
	 * We now start writing to the filesystem
	 */

	if (!Nflag) {
		/*
		 * Validate the given file system size.
		 * Verify that its last block can actually be accessed.
		 * Convert to file system fragment sized units.
		 */
		if (fssize <= 0) {
			printf("preposterous size %lld\n", (long long)fssize);
			fserr(13);
		}
		wtfs(fssize - 1, sectorsize, iobuf);

		/*
		 * Ensure there is nothing that looks like a filesystem
		 * superbock anywhere other than where ours will be.
		 * If fsck finds the wrong one all hell breaks loose!
		 */
		for (i = 0; ; i++) {
			static const int sblocklist[] = SBLOCKSEARCH;
			int sblkoff = sblocklist[i];
			int sz;
			if (sblkoff == -1)
				break;
			/* Remove main superblock */
			zap_old_sblock(sblkoff);
			/* and all possible locations for the first alternate */
			sblkoff += SBLOCKSIZE;
			for (sz = SBLOCKSIZE; sz <= 0x10000; sz <<= 1)
				zap_old_sblock(roundup(sblkoff, sz));
		}
		/*
		 * Also zap possible Ext2fs magic leftover to prevent
		 * kernel vfs_mountroot() and bootloaders from mis-recognizing
		 * this file system as Ext2fs.
		 */
		zap_old_sblock(EXT2FS_SBOFF);

#ifndef NO_APPLE_UFS
		if (isappleufs) {
			struct appleufslabel appleufs;
			ffs_appleufs_set(&appleufs, appleufs_volname,
			    tv.tv_sec, 0);
			wtfs(APPLEUFS_LABEL_OFFSET/sectorsize,
			    APPLEUFS_LABEL_SIZE, &appleufs);
		} else if (APPLEUFS_LABEL_SIZE % sectorsize == 0) {
			struct appleufslabel appleufs;
			/* Look for & zap any existing valid apple ufs labels */
			rdfs(APPLEUFS_LABEL_OFFSET/sectorsize,
			    APPLEUFS_LABEL_SIZE, &appleufs);
			if (ffs_appleufs_validate(fsys, &appleufs, NULL) == 0) {
				memset(&appleufs, 0, sizeof(appleufs));
				wtfs(APPLEUFS_LABEL_OFFSET/sectorsize,
				    APPLEUFS_LABEL_SIZE, &appleufs);
			}
		}
#endif
	}

	/*
	 * Make a copy of the superblock into the buffer that we will be
	 * writing out in each cylinder group.
	 */
	memcpy(iobuf, &sblock, sizeof sblock);
	if (needswap)
		ffs_sb_swap(&sblock, (struct fs *)iobuf);
	if ((sblock.fs_old_flags & FS_FLAGS_UPDATED) == 0)
		memset(iobuf + offsetof(struct fs, fs_old_postbl_start),
		    0xff, 256);

	if (verbosity >= 3)
		printf("super-block backups (for fsck_ffs -b #) at:\n");
	/* If we are printing more than one line of numbers, line up columns */
	fld_width = verbosity < 4 ? 1 : snprintf(NULL, 0, "%" PRIu64, 
		(uint64_t)FFS_FSBTODB(&sblock, cgsblock(&sblock, sblock.fs_ncg-1)));
	/* Get terminal width */
	if (ioctl(fileno(stdout), TIOCGWINSZ, &winsize) == 0)
		max_cols = winsize.ws_col;
	else
		max_cols = 80;
	if (Nflag && verbosity == 3)
		/* Leave space to add " ..." after one row of numbers */
		max_cols -= 4;
#define BASE 0x10000	/* For some fixed-point maths */
	col = 0;
	delta = verbosity > 2 ? 0 : max_cols * BASE / sblock.fs_ncg;
	for (cylno = 0; cylno < sblock.fs_ncg; cylno++) {
		fflush(stdout);
		initcg(cylno, &tv);
		if (verbosity < 2)
			continue;
		if (delta > 0) {
			if (Nflag)
				/* No point doing dots for -N */
				break;
			/* Print dots scaled to end near RH margin */
			for (col += delta; col > BASE; col -= BASE)
				printf(".");
			continue;
		}
		/* Print superblock numbers */
		len = printf("%s%*" PRIu64 ",", col ? " " : "", fld_width,
		    (uint64_t)FFS_FSBTODB(&sblock, cgsblock(&sblock, cylno)));
		col += len;
		if (col + len < max_cols)
			/* Next number fits */
			continue;
		/* Next number won't fit, need a newline */
		if (verbosity <= 3) {
			/* Print dots for subsequent cylinder groups */
			delta = sblock.fs_ncg - cylno - 1;
			if (delta != 0) {
				if (Nflag) {
					printf(" ...");
					break;
				}
				delta = max_cols * BASE / delta;
			}
		}
		col = 0;
		printf("\n");
	}
#undef BASE
	if (col > 0)
		printf("\n");
	if (Nflag)
		exit(0);

	/*
	 * Now construct the initial file system,
	 */
	if (fsinit(&tv, mfsmode, mfsuid, mfsgid) == 0 && mfs)
		errx(1, "Error making filesystem");
	sblock.fs_time = tv.tv_sec;
	if (Oflag <= 1) {
		sblock.fs_old_cstotal.cs_ndir = sblock.fs_cstotal.cs_ndir;
		sblock.fs_old_cstotal.cs_nbfree = sblock.fs_cstotal.cs_nbfree;
		sblock.fs_old_cstotal.cs_nifree = sblock.fs_cstotal.cs_nifree;
		sblock.fs_old_cstotal.cs_nffree = sblock.fs_cstotal.cs_nffree;
	}
	/*
	 * Write out the super-block and zeros until the first cg info
	 */
	i = cgsblock(&sblock, 0) * sblock.fs_fsize - sblock.fs_sblockloc;
	if ((size_t)i < sizeof(sblock))
		errx(1, "No space for superblock");
	memcpy(iobuf, &sblock, sizeof(sblock));
	memset(iobuf + sizeof(sblock), 0, i - sizeof(sblock));
	if (needswap)
		ffs_sb_swap(&sblock, (struct fs *)iobuf);
	if ((sblock.fs_old_flags & FS_FLAGS_UPDATED) == 0)
		memset(iobuf + offsetof(struct fs, fs_old_postbl_start),
		    0xff, 256);
	wtfs(sblock.fs_sblockloc / sectorsize, i, iobuf);

	/* Write out first and last cylinder summary sectors */
	if (needswap)
		ffs_csum_swap(fscs_0, fscs_0, sblock.fs_fsize);
	wtfs(FFS_FSBTODB(&sblock, sblock.fs_csaddr), sblock.fs_fsize, fscs_0);

	if (fscs_next > fscs_reset) {
		if (needswap)
			ffs_csum_swap(fscs_reset, fscs_reset, sblock.fs_fsize);
		fs_csaddr++;
		wtfs(FFS_FSBTODB(&sblock, fs_csaddr), sblock.fs_fsize, fscs_reset);
	}

	/* mfs doesn't need these permanently allocated */
	munmap(iobuf, iobuf_memsize);
	munmap(fscs_0, 2 * sblock.fs_fsize);
}

/*
 * Initialize a cylinder group.
 */
void
initcg(int cylno, const struct timeval *tv)
{
	daddr_t cbase, dmax;
	int32_t i, d, dlower, dupper, blkno;
	uint32_t u;
	struct ufs1_dinode *dp1;
	struct ufs2_dinode *dp2;
	int start;

	/*
	 * Determine block bounds for cylinder group.
	 * Allow space for super block summary information in first
	 * cylinder group.
	 */
	cbase = cgbase(&sblock, cylno);
	dmax = cbase + sblock.fs_fpg;
	if (dmax > sblock.fs_size)
		dmax = sblock.fs_size;
	dlower = cgsblock(&sblock, cylno) - cbase;
	dupper = cgdmin(&sblock, cylno) - cbase;
	if (cylno == 0) {
		dupper += howmany(sblock.fs_cssize, sblock.fs_fsize);
		if (dupper >= cgstart(&sblock, cylno + 1)) {
			printf("\rToo many cylinder groups to fit summary "
				"information into first cylinder group\n");
			fserr(40);
		}
	}
	memset(&acg, 0, sblock.fs_cgsize);
	acg.cg_magic = CG_MAGIC;
	acg.cg_cgx = cylno;
	acg.cg_ndblk = dmax - cbase;
	if (sblock.fs_contigsumsize > 0)
		acg.cg_nclusterblks = acg.cg_ndblk >> sblock.fs_fragshift;
	start = &acg.cg_space[0] - (u_char *)(&acg.cg_firstfield);
	if (Oflag == 2) {
		acg.cg_time = tv->tv_sec;
		acg.cg_niblk = sblock.fs_ipg;
		acg.cg_initediblk = sblock.fs_ipg < 2 * FFS_INOPB(&sblock) ?
		    sblock.fs_ipg : 2 * FFS_INOPB(&sblock);
		acg.cg_iusedoff = start;
	} else {
		acg.cg_old_ncyl = sblock.fs_old_cpg;
		if ((sblock.fs_old_flags & FS_FLAGS_UPDATED) == 0 &&
		    (cylno == sblock.fs_ncg - 1))
			acg.cg_old_ncyl = 
			    sblock.fs_old_ncyl % sblock.fs_old_cpg;
		acg.cg_old_time = tv->tv_sec;
		acg.cg_old_niblk = sblock.fs_ipg;
		acg.cg_old_btotoff = start;
		acg.cg_old_boff = acg.cg_old_btotoff +
		    sblock.fs_old_cpg * sizeof(int32_t);
		acg.cg_iusedoff = acg.cg_old_boff +
		    sblock.fs_old_cpg * sizeof(u_int16_t);
	}
	acg.cg_freeoff = acg.cg_iusedoff + howmany(sblock.fs_ipg, CHAR_BIT);
	if (sblock.fs_contigsumsize <= 0) {
		acg.cg_nextfreeoff = acg.cg_freeoff +
		   howmany(sblock.fs_fpg, CHAR_BIT);
	} else {
		acg.cg_clustersumoff = acg.cg_freeoff +
		    howmany(sblock.fs_fpg, CHAR_BIT) - sizeof(int32_t);
		if (isappleufs) {
			/* Apple PR2216969 gives rationale for this change.
			 * I believe they were mistaken, but we need to
			 * duplicate it for compatibility.  -- dbj@NetBSD.org
			 */
			acg.cg_clustersumoff += sizeof(int32_t);
		}
		acg.cg_clustersumoff =
		    roundup(acg.cg_clustersumoff, sizeof(int32_t));
		acg.cg_clusteroff = acg.cg_clustersumoff +
		    (sblock.fs_contigsumsize + 1) * sizeof(int32_t);
		acg.cg_nextfreeoff = acg.cg_clusteroff +
		    howmany(ffs_fragstoblks(&sblock, sblock.fs_fpg), CHAR_BIT);
	}
	if (acg.cg_nextfreeoff > sblock.fs_cgsize) {
		printf("Panic: cylinder group too big\n");
		fserr(37);
	}
	acg.cg_cs.cs_nifree += sblock.fs_ipg;
	if (cylno == 0)
		for (u = 0; u < UFS_ROOTINO; u++) {
			setbit(cg_inosused(&acg, 0), u);
			acg.cg_cs.cs_nifree--;
		}
	if (cylno > 0) {
		/*
		 * In cylno 0, beginning space is reserved
		 * for boot and super blocks.
		 */
		for (d = 0, blkno = 0; d < dlower;) {
			setblock(&sblock, cg_blksfree(&acg, 0), blkno);
			if (sblock.fs_contigsumsize > 0)
				setbit(cg_clustersfree(&acg, 0), blkno);
			acg.cg_cs.cs_nbfree++;
			if (Oflag <= 1) {
				int cn = old_cbtocylno(&sblock, d);
				old_cg_blktot(&acg, 0)[cn]++;
				old_cg_blks(&sblock, &acg,
				    cn, 0)[old_cbtorpos(&sblock, d)]++;
			}
			d += sblock.fs_frag;
			blkno++;
		}
	}
	if ((i = (dupper & (sblock.fs_frag - 1))) != 0) {
		acg.cg_frsum[sblock.fs_frag - i]++;
		for (d = dupper + sblock.fs_frag - i; dupper < d; dupper++) {
			setbit(cg_blksfree(&acg, 0), dupper);
			acg.cg_cs.cs_nffree++;
		}
	}
	for (d = dupper, blkno = dupper >> sblock.fs_fragshift;
	     d + sblock.fs_frag <= acg.cg_ndblk; ) {
		setblock(&sblock, cg_blksfree(&acg, 0), blkno);
		if (sblock.fs_contigsumsize > 0)
			setbit(cg_clustersfree(&acg, 0), blkno);
		acg.cg_cs.cs_nbfree++;
		if (Oflag <= 1) {
			int cn = old_cbtocylno(&sblock, d);
			old_cg_blktot(&acg, 0)[cn]++;
			old_cg_blks(&sblock, &acg,
			    cn, 0)[old_cbtorpos(&sblock, d)]++;
		}
		d += sblock.fs_frag;
		blkno++;
	}
	if (d < acg.cg_ndblk) {
		acg.cg_frsum[acg.cg_ndblk - d]++;
		for (; d < acg.cg_ndblk; d++) {
			setbit(cg_blksfree(&acg, 0), d);
			acg.cg_cs.cs_nffree++;
		}
	}
	if (sblock.fs_contigsumsize > 0) {
		int32_t *sump = cg_clustersum(&acg, 0);
		u_char *mapp = cg_clustersfree(&acg, 0);
		int map = *mapp++;
		int bit = 1;
		int run = 0;

		for (i = 0; i < acg.cg_nclusterblks; i++) {
			if ((map & bit) != 0) {
				run++;
			} else if (run != 0) {
				if (run > sblock.fs_contigsumsize)
					run = sblock.fs_contigsumsize;
				sump[run]++;
				run = 0;
			}
			if ((i & (CHAR_BIT - 1)) != (CHAR_BIT - 1)) {
				bit <<= 1;
			} else {
				map = *mapp++;
				bit = 1;
			}
		}
		if (run != 0) {
			if (run > sblock.fs_contigsumsize)
				run = sblock.fs_contigsumsize;
			sump[run]++;
		}
	}
	*fscs_next++ = acg.cg_cs;
	if (fscs_next == fscs_end) {
		/* write block of cylinder group summary info into cyl 0 */
		if (needswap)
			ffs_csum_swap(fscs_reset, fscs_reset, sblock.fs_fsize);
		fs_csaddr++;
		wtfs(FFS_FSBTODB(&sblock, fs_csaddr), sblock.fs_fsize, fscs_reset);
		fscs_next = fscs_reset;
		memset(fscs_next, 0, sblock.fs_fsize);
	}
	/*
	 * Write out the duplicate super block, the cylinder group map
	 * and two blocks worth of inodes in a single write.
	 */
	start = sblock.fs_bsize > SBLOCKSIZE ? sblock.fs_bsize : SBLOCKSIZE;
	memcpy(&iobuf[start], &acg, sblock.fs_cgsize);
	if (needswap)
		ffs_cg_swap(&acg, (struct cg*)&iobuf[start], &sblock);
	start += sblock.fs_bsize;
	dp1 = (struct ufs1_dinode *)(&iobuf[start]);
	dp2 = (struct ufs2_dinode *)(&iobuf[start]);
	for (i = MIN(sblock.fs_ipg, 2) * FFS_INOPB(&sblock); i != 0; i--) {
		if (sblock.fs_magic == FS_UFS1_MAGIC) {
			/* No need to swap, it'll stay random */
			dp1->di_gen = arc4random() & INT32_MAX;
			dp1++;
		} else {
			dp2->di_gen = arc4random() & INT32_MAX;
			dp2++;
		}
	}
	wtfs(FFS_FSBTODB(&sblock, cgsblock(&sblock, cylno)), iobufsize, iobuf);
	/*
	 * For the old file system, we have to initialize all the inodes.
	 */
	if (sblock.fs_magic != FS_UFS1_MAGIC)
		return;

	/* Write 'd' (usually 16 * fs_frag) file-system fragments at once */
	d = (iobuf_memsize - start) / sblock.fs_bsize * sblock.fs_frag;
	dupper = sblock.fs_ipg / FFS_INOPF(&sblock);
	for (i = 2 * sblock.fs_frag; i < dupper; i += d) {
		if (d > dupper - i)
			d = dupper - i;
		dp1 = (struct ufs1_dinode *)(&iobuf[start]);
		do
			dp1->di_gen = arc4random() & INT32_MAX;
		while ((char *)++dp1 < &iobuf[iobuf_memsize]);
		wtfs(FFS_FSBTODB(&sblock, cgimin(&sblock, cylno) + i),
		    d * sblock.fs_bsize / sblock.fs_frag, &iobuf[start]);
	}
}

/*
 * initialize the file system
 */

#ifdef LOSTDIR
#define	PREDEFDIR 3
#else
#define	PREDEFDIR 2
#endif

struct direct root_dir[] = {
	{ UFS_ROOTINO, sizeof(struct direct), DT_DIR, 1, "." },
	{ UFS_ROOTINO, sizeof(struct direct), DT_DIR, 2, ".." },
#ifdef LOSTDIR
	{ LOSTFOUNDINO, sizeof(struct direct), DT_DIR, 10, "lost+found" },
#endif
};
struct odirect {
	u_int32_t d_ino;
	u_int16_t d_reclen;
	u_int16_t d_namlen;
	u_char	d_name[FFS_MAXNAMLEN + 1];
} oroot_dir[] = {
	{ UFS_ROOTINO, sizeof(struct direct), 1, "." },
	{ UFS_ROOTINO, sizeof(struct direct), 2, ".." },
#ifdef LOSTDIR
	{ LOSTFOUNDINO, sizeof(struct direct), 10, "lost+found" },
#endif
};
#ifdef LOSTDIR
struct direct lost_found_dir[] = {
	{ LOSTFOUNDINO, sizeof(struct direct), DT_DIR, 1, "." },
	{ UFS_ROOTINO, sizeof(struct direct), DT_DIR, 2, ".." },
	{ 0, DIRBLKSIZ, 0, 0, 0 },
};
struct odirect olost_found_dir[] = {
	{ LOSTFOUNDINO, sizeof(struct direct), 1, "." },
	{ UFS_ROOTINO, sizeof(struct direct), 2, ".." },
	{ 0, DIRBLKSIZ, 0, 0 },
};
#endif

static void copy_dir(struct direct *, struct direct *);

int
fsinit(const struct timeval *tv, mode_t mfsmode, uid_t mfsuid, gid_t mfsgid)
{
	union dinode node;
	union Buffer buf;
	int i;
	int qblocks = 0;
	int qinos = 0;
	uint8_t q2h_hash_shift;
	uint16_t q2h_hash_mask;
#ifdef LOSTDIR
	int dirblksiz = DIRBLKSIZ;
	if (isappleufs)
		dirblksiz = APPLEUFS_DIRBLKSIZ;
	int nextino = LOSTFOUNDINO+1;
#else
	int nextino = UFS_ROOTINO+1;
#endif

	/*
	 * initialize the node
	 */

#ifdef LOSTDIR
	/*
	 * create the lost+found directory
	 */
	memset(&node, 0, sizeof(node));
	if (Oflag == 0) {
		(void)makedir(&buf, (struct direct *)olost_found_dir, 2);
		for (i = dirblksiz; i < sblock.fs_bsize; i += dirblksiz)
			copy_dir((struct direct*)&olost_found_dir[2],
				(struct direct*)&buf[i]);
	} else {
		(void)makedir(&buf, lost_found_dir, 2);
		for (i = dirblksiz; i < sblock.fs_bsize; i += dirblksiz)
			copy_dir(&lost_found_dir[2], (struct direct*)&buf[i]);
	}
	if (sblock.fs_magic == FS_UFS1_MAGIC) {
		node.dp1.di_atime = tv->tv_sec;
		node.dp1.di_atimensec = tv->tv_usec * 1000;
		node.dp1.di_mtime = tv->tv_sec;
		node.dp1.di_mtimensec = tv->tv_usec * 1000;
		node.dp1.di_ctime = tv->tv_sec;
		node.dp1.di_ctimensec = tv->tv_usec * 1000;
		node.dp1.di_mode = IFDIR | UMASK;
		node.dp1.di_nlink = 2;
		node.dp1.di_size = sblock.fs_bsize;
		node.dp1.di_db[0] = alloc(node.dp1.di_size, node.dp1.di_mode);
		if (node.dp1.di_db[0] == 0)
			return (0);
		node.dp1.di_blocks = btodb(ffs_fragroundup(&sblock,
		    node.dp1.di_size));
		qblocks += node.dp1.di_blocks;
		node.dp1.di_uid = geteuid();
		node.dp1.di_gid = getegid();
		wtfs(FFS_FSBTODB(&sblock, node.dp1.di_db[0]), node.dp1.di_size,
		    buf);
	} else {
		node.dp2.di_atime = tv->tv_sec;
		node.dp2.di_atimensec = tv->tv_usec * 1000;
		node.dp2.di_mtime = tv->tv_sec;
		node.dp2.di_mtimensec = tv->tv_usec * 1000;
		node.dp2.di_ctime = tv->tv_sec;
		node.dp2.di_ctimensec = tv->tv_usec * 1000;
		node.dp2.di_birthtime = tv->tv_sec;
		node.dp2.di_birthnsec = tv->tv_usec * 1000;
		node.dp2.di_mode = IFDIR | UMASK;
		node.dp2.di_nlink = 2;
		node.dp2.di_size = sblock.fs_bsize;
		node.dp2.di_db[0] = alloc(node.dp2.di_size, node.dp2.di_mode);
		if (node.dp2.di_db[0] == 0)
			return (0);
		node.dp2.di_blocks = btodb(ffs_fragroundup(&sblock,
		    node.dp2.di_size));
		qblocks += node.dp2.di_blocks;
		node.dp2.di_uid = geteuid();
		node.dp2.di_gid = getegid();
		wtfs(FFS_FSBTODB(&sblock, node.dp2.di_db[0]), node.dp2.di_size,
		    buf);
	}
	qinos++;
	iput(&node, LOSTFOUNDINO);
#endif
	/*
	 * create the root directory
	 */
	memset(&node, 0, sizeof(node));
	if (Oflag <= 1) {
		if (mfs) {
			node.dp1.di_mode = IFDIR | mfsmode;
			node.dp1.di_uid = mfsuid;
			node.dp1.di_gid = mfsgid;
		} else {
			node.dp1.di_mode = IFDIR | UMASK;
			node.dp1.di_uid = geteuid();
			node.dp1.di_gid = getegid();
		}
		node.dp1.di_nlink = PREDEFDIR;
		if (Oflag == 0)
			node.dp1.di_size = makedir(&buf, 
			    (struct direct *)oroot_dir, PREDEFDIR);
		else
			node.dp1.di_size = makedir(&buf, root_dir, PREDEFDIR);
		node.dp1.di_db[0] = alloc(sblock.fs_fsize, node.dp1.di_mode);
		if (node.dp1.di_db[0] == 0)
			return (0);
		node.dp1.di_blocks = btodb(ffs_fragroundup(&sblock,
		    node.dp1.di_size));
		qblocks += node.dp1.di_blocks;
		wtfs(FFS_FSBTODB(&sblock, node.dp1.di_db[0]), sblock.fs_fsize, &buf);
	} else {
		if (mfs) {
			node.dp2.di_mode = IFDIR | mfsmode;
			node.dp2.di_uid = mfsuid;
			node.dp2.di_gid = mfsgid;
		} else {
			node.dp2.di_mode = IFDIR | UMASK;
			node.dp2.di_uid = geteuid();
			node.dp2.di_gid = getegid();
		}
		node.dp2.di_atime = tv->tv_sec;
		node.dp2.di_atimensec = tv->tv_usec * 1000;
		node.dp2.di_mtime = tv->tv_sec;
		node.dp2.di_mtimensec = tv->tv_usec * 1000;
		node.dp2.di_ctime = tv->tv_sec;
		node.dp2.di_ctimensec = tv->tv_usec * 1000;
		node.dp2.di_birthtime = tv->tv_sec;
		node.dp2.di_birthnsec = tv->tv_usec * 1000;
		node.dp2.di_nlink = PREDEFDIR;
		node.dp2.di_size = makedir(&buf, root_dir, PREDEFDIR);
		node.dp2.di_db[0] = alloc(sblock.fs_fsize, node.dp2.di_mode);
		if (node.dp2.di_db[0] == 0)
			return (0);
		node.dp2.di_blocks = btodb(ffs_fragroundup(&sblock,
		    node.dp2.di_size));
		qblocks += node.dp2.di_blocks;
		wtfs(FFS_FSBTODB(&sblock, node.dp2.di_db[0]), sblock.fs_fsize, &buf);
	}
	qinos++;
	iput(&node, UFS_ROOTINO);
	/*
	 * compute the size of the hash table
	 * We know the smallest block size is 4k, so we can use 2k
	 * for the hash table; as an entry is 8 bytes we can store
	 * 256 entries. So let start q2h_hash_shift at 8
	 */
	for (q2h_hash_shift = 8;
	    q2h_hash_shift < 15;
	    q2h_hash_shift++) {
		if ((sizeof(uint64_t) << (q2h_hash_shift + 1)) +
		    sizeof(struct quota2_header) > (u_int)sblock.fs_bsize)
			break;
	}
	q2h_hash_mask = (1 << q2h_hash_shift) - 1;
	for (i = 0; i < MAXQUOTAS; i++) {
		struct quota2_header *q2h;
		struct quota2_entry *q2e;
		uint64_t offset;
		uid_t uid = (i == USRQUOTA ? geteuid() : getegid());

		if ((quotas & FS_Q2_DO_TYPE(i)) == 0)
			continue;
		quota2_create_blk0(sblock.fs_bsize, &buf, q2h_hash_shift,
		    i, needswap);
		/* grab an entry from header for root dir */
		q2h = &buf.q2h;
		offset = ufs_rw64(q2h->q2h_free, needswap);
		q2e = (void *)((char *)&buf + offset);
		q2h->q2h_free = q2e->q2e_next;
		memcpy(q2e, &q2h->q2h_defentry, sizeof(*q2e));
		q2e->q2e_uid = ufs_rw32(uid, needswap);
		q2e->q2e_val[QL_BLOCK].q2v_cur = ufs_rw64(qblocks, needswap);
		q2e->q2e_val[QL_FILE].q2v_cur = ufs_rw64(qinos, needswap);
		/* add to the hash entry */
		q2e->q2e_next = q2h->q2h_entries[uid & q2h_hash_mask];
		q2h->q2h_entries[uid & q2h_hash_mask] =
		    ufs_rw64(offset, needswap);

		memset(&node, 0, sizeof(node));
		if (sblock.fs_magic == FS_UFS1_MAGIC) {
			node.dp1.di_atime = tv->tv_sec;
			node.dp1.di_atimensec = tv->tv_usec * 1000;
			node.dp1.di_mtime = tv->tv_sec;
			node.dp1.di_mtimensec = tv->tv_usec * 1000;
			node.dp1.di_ctime = tv->tv_sec;
			node.dp1.di_ctimensec = tv->tv_usec * 1000;
			node.dp1.di_mode = IFREG;
			node.dp1.di_nlink = 1;
			node.dp1.di_size = sblock.fs_bsize;
			node.dp1.di_db[0] =
			    alloc(node.dp1.di_size, node.dp1.di_mode);
			if (node.dp1.di_db[0] == 0)
				return (0);
			node.dp1.di_blocks = btodb(ffs_fragroundup(&sblock,
			    node.dp1.di_size));
			node.dp1.di_uid = geteuid();
			node.dp1.di_gid = getegid();
			wtfs(FFS_FSBTODB(&sblock, node.dp1.di_db[0]),
			     node.dp1.di_size, &buf);
		} else {
			node.dp2.di_atime = tv->tv_sec;
			node.dp2.di_atimensec = tv->tv_usec * 1000;
			node.dp2.di_mtime = tv->tv_sec;
			node.dp2.di_mtimensec = tv->tv_usec * 1000;
			node.dp2.di_ctime = tv->tv_sec;
			node.dp2.di_ctimensec = tv->tv_usec * 1000;
			node.dp2.di_birthtime = tv->tv_sec;
			node.dp2.di_birthnsec = tv->tv_usec * 1000;
			node.dp2.di_mode = IFREG;
			node.dp2.di_nlink = 1;
			node.dp2.di_size = sblock.fs_bsize;
			node.dp2.di_db[0] =
			    alloc(node.dp2.di_size, node.dp2.di_mode);
			if (node.dp2.di_db[0] == 0)
				return (0);
			node.dp2.di_blocks = btodb(ffs_fragroundup(&sblock,
			    node.dp2.di_size));
			node.dp2.di_uid = geteuid();
			node.dp2.di_gid = getegid();
			wtfs(FFS_FSBTODB(&sblock, node.dp2.di_db[0]),
			    node.dp2.di_size, &buf);
		}
		iput(&node, nextino);
		sblock.fs_quotafile[i] = nextino;
		nextino++;
	}
	return (1);
}

/*
 * construct a set of directory entries in "buf".
 * return size of directory.
 */
int
makedir(union Buffer *buf, struct direct *protodir, int entries)
{
	char *cp;
	int i, spcleft;
	int dirblksiz = UFS_DIRBLKSIZ;
	if (isappleufs)
		dirblksiz = APPLEUFS_DIRBLKSIZ;

	memset(buf, 0, dirblksiz);
	spcleft = dirblksiz;
	for (cp = buf->data, i = 0; i < entries - 1; i++) {
		protodir[i].d_reclen = UFS_DIRSIZ(Oflag == 0, &protodir[i], 0);
		copy_dir(&protodir[i], (struct direct*)cp);
		cp += protodir[i].d_reclen;
		spcleft -= protodir[i].d_reclen;
	}
	protodir[i].d_reclen = spcleft;
	copy_dir(&protodir[i], (struct direct*)cp);
	return (dirblksiz);
}

/*
 * allocate a block or frag
 */
daddr_t
alloc(int size, int mode)
{
	int i, frag;
	daddr_t d, blkno;

	rdfs(FFS_FSBTODB(&sblock, cgtod(&sblock, 0)), sblock.fs_cgsize, &acg);
	/* fs -> host byte order */
	if (needswap)
		ffs_cg_swap(&acg, &acg, &sblock);
	if (acg.cg_magic != CG_MAGIC) {
		printf("cg 0: bad magic number\n");
		return (0);
	}
	if (acg.cg_cs.cs_nbfree == 0) {
		printf("first cylinder group ran out of space\n");
		return (0);
	}
	for (d = 0; d < acg.cg_ndblk; d += sblock.fs_frag)
		if (isblock(&sblock, cg_blksfree(&acg, 0),
		    d >> sblock.fs_fragshift))
			goto goth;
	printf("internal error: can't find block in cyl 0\n");
	return (0);
goth:
	blkno = ffs_fragstoblks(&sblock, d);
	clrblock(&sblock, cg_blksfree(&acg, 0), blkno);
	if (sblock.fs_contigsumsize > 0)
		clrbit(cg_clustersfree(&acg, 0), blkno);
	acg.cg_cs.cs_nbfree--;
	sblock.fs_cstotal.cs_nbfree--;
	fscs_0->cs_nbfree--;
	if (mode & IFDIR) {
		acg.cg_cs.cs_ndir++;
		sblock.fs_cstotal.cs_ndir++;
		fscs_0->cs_ndir++;
	}
	if (Oflag <= 1) {
		int cn = old_cbtocylno(&sblock, d);
		old_cg_blktot(&acg, 0)[cn]--;
		old_cg_blks(&sblock, &acg,
		    cn, 0)[old_cbtorpos(&sblock, d)]--;
	}
	if (size != sblock.fs_bsize) {
		frag = howmany(size, sblock.fs_fsize);
		fscs_0->cs_nffree += sblock.fs_frag - frag;
		sblock.fs_cstotal.cs_nffree += sblock.fs_frag - frag;
		acg.cg_cs.cs_nffree += sblock.fs_frag - frag;
		acg.cg_frsum[sblock.fs_frag - frag]++;
		for (i = frag; i < sblock.fs_frag; i++)
			setbit(cg_blksfree(&acg, 0), d + i);
	}
	/* host -> fs byte order */
	if (needswap)
		ffs_cg_swap(&acg, &acg, &sblock);
	wtfs(FFS_FSBTODB(&sblock, cgtod(&sblock, 0)), sblock.fs_cgsize, &acg);
	return (d);
}

/*
 * Allocate an inode on the disk
 */
static void
iput(union dinode *ip, ino_t ino)
{
	daddr_t d;
	int i;
	struct ufs1_dinode *dp1;
	struct ufs2_dinode *dp2;

	rdfs(FFS_FSBTODB(&sblock, cgtod(&sblock, 0)), sblock.fs_cgsize, &acg);
	/* fs -> host byte order */
	if (needswap)
		ffs_cg_swap(&acg, &acg, &sblock);
	if (acg.cg_magic != CG_MAGIC) {
		printf("cg 0: bad magic number\n");
		fserr(31);
	}
	acg.cg_cs.cs_nifree--;
	setbit(cg_inosused(&acg, 0), ino);
	/* host -> fs byte order */
	if (needswap)
		ffs_cg_swap(&acg, &acg, &sblock);
	wtfs(FFS_FSBTODB(&sblock, cgtod(&sblock, 0)), sblock.fs_cgsize, &acg);
	sblock.fs_cstotal.cs_nifree--;
	fscs_0->cs_nifree--;
	if (ino >= (ino_t)(sblock.fs_ipg * sblock.fs_ncg)) {
		printf("fsinit: inode value out of range (%llu).\n",
		    (unsigned long long)ino);
		fserr(32);
	}
	d = FFS_FSBTODB(&sblock, ino_to_fsba(&sblock, ino));
	rdfs(d, sblock.fs_bsize, (char *)iobuf);
	if (sblock.fs_magic == FS_UFS1_MAGIC) {
		dp1 = (struct ufs1_dinode *)iobuf;
		dp1 += ino_to_fsbo(&sblock, ino);
		if (needswap) {
			ffs_dinode1_swap(&ip->dp1, dp1);
			/* ffs_dinode1_swap() doesn't swap blocks addrs */
			for (i=0; i<UFS_NDADDR; i++)
			    dp1->di_db[i] = bswap32(ip->dp1.di_db[i]);
			for (i=0; i<UFS_NIADDR; i++)
			    dp1->di_ib[i] = bswap32(ip->dp1.di_ib[i]);
		} else
			*dp1 = ip->dp1;
		dp1->di_gen = arc4random() & INT32_MAX;
	} else {
		dp2 = (struct ufs2_dinode *)iobuf;
		dp2 += ino_to_fsbo(&sblock, ino);
		if (needswap) {
			ffs_dinode2_swap(&ip->dp2, dp2);
			for (i=0; i<UFS_NDADDR; i++)
			    dp2->di_db[i] = bswap64(ip->dp2.di_db[i]);
			for (i=0; i<UFS_NIADDR; i++)
			    dp2->di_ib[i] = bswap64(ip->dp2.di_ib[i]);
		} else
			*dp2 = ip->dp2;
		dp2->di_gen = arc4random() & INT32_MAX;
	}
	wtfs(d, sblock.fs_bsize, iobuf);
}

/*
 * read a block from the file system
 */
void
rdfs(daddr_t bno, int size, void *bf)
{
	int n;
	off_t offset;

#ifdef MFS
	if (mfs) {
		if (Nflag)
			memset(bf, 0, size);
		else
			memmove(bf, membase + bno * sectorsize, size);
		return;
	}
#endif
	offset = bno;
	n = pread(fsi, bf, size, offset * sectorsize);
	if (n != size) {
		printf("rdfs: read error for sector %lld: %s\n",
		    (long long)bno, strerror(errno));
		exit(34);
	}
}

/*
 * write a block to the file system
 */
void
wtfs(daddr_t bno, int size, void *bf)
{
	int n;
	off_t offset;

	if (Nflag)
		return;
#ifdef MFS
	if (mfs) {
		memmove(membase + bno * sectorsize, bf, size);
		return;
	}
#endif
	offset = bno;
	n = pwrite(fso, bf, size, offset * sectorsize);
	if (n != size) {
		printf("wtfs: write error for sector %lld: %s\n",
		    (long long)bno, strerror(errno));
		exit(36);
	}
}

/*
 * check if a block is available
 */
int
isblock(struct fs *fs, unsigned char *cp, int h)
{
	unsigned char mask;

	switch (fs->fs_fragshift) {
	case 3:
		return (cp[h] == 0xff);
	case 2:
		mask = 0x0f << ((h & 0x1) << 2);
		return ((cp[h >> 1] & mask) == mask);
	case 1:
		mask = 0x03 << ((h & 0x3) << 1);
		return ((cp[h >> 2] & mask) == mask);
	case 0:
		mask = 0x01 << (h & 0x7);
		return ((cp[h >> 3] & mask) == mask);
	default:
#ifdef STANDALONE
		printf("isblock bad fs_fragshift %d\n", fs->fs_fragshift);
#else
		fprintf(stderr, "isblock bad fs_fragshift %d\n",
		    fs->fs_fragshift);
#endif
		return (0);
	}
}

/*
 * take a block out of the map
 */
void
clrblock(struct fs *fs, unsigned char *cp, int h)
{
	switch ((fs)->fs_fragshift) {
	case 3:
		cp[h] = 0;
		return;
	case 2:
		cp[h >> 1] &= ~(0x0f << ((h & 0x1) << 2));
		return;
	case 1:
		cp[h >> 2] &= ~(0x03 << ((h & 0x3) << 1));
		return;
	case 0:
		cp[h >> 3] &= ~(0x01 << (h & 0x7));
		return;
	default:
#ifdef STANDALONE
		printf("clrblock bad fs_fragshift %d\n", fs->fs_fragshift);
#else
		fprintf(stderr, "clrblock bad fs_fragshift %d\n",
		    fs->fs_fragshift);
#endif
		return;
	}
}

/*
 * put a block into the map
 */
void
setblock(struct fs *fs, unsigned char *cp, int h)
{
	switch (fs->fs_fragshift) {
	case 3:
		cp[h] = 0xff;
		return;
	case 2:
		cp[h >> 1] |= (0x0f << ((h & 0x1) << 2));
		return;
	case 1:
		cp[h >> 2] |= (0x03 << ((h & 0x3) << 1));
		return;
	case 0:
		cp[h >> 3] |= (0x01 << (h & 0x7));
		return;
	default:
#ifdef STANDALONE
		printf("setblock bad fs_frag %d\n", fs->fs_fragshift);
#else
		fprintf(stderr, "setblock bad fs_fragshift %d\n",
		    fs->fs_fragshift);
#endif
		return;
	}
}

/* copy a direntry to a buffer, in fs byte order */
static void
copy_dir(struct direct *dir, struct direct *dbuf)
{
	memcpy(dbuf, dir, UFS_DIRSIZ(Oflag == 0, dir, 0));
	if (needswap) {
		dbuf->d_ino = bswap32(dir->d_ino);
		dbuf->d_reclen = bswap16(dir->d_reclen);
		if (Oflag == 0)
			((struct odirect*)dbuf)->d_namlen =
				bswap16(((struct odirect*)dir)->d_namlen);
	}
}

static int
ilog2(int val)
{
	u_int n;

	for (n = 0; n < sizeof(n) * CHAR_BIT; n++)
		if (1 << n == val)
			return (n);
	errx(1, "ilog2: %d is not a power of 2", val);
}

static void
zap_old_sblock(int sblkoff)
{
	static int cg0_data;
	uint32_t oldfs[SBLOCKSIZE / 4];
	static const struct fsm {
		uint32_t	offset;
		uint32_t	magic;
		uint32_t	mask;
	} fs_magics[] = {
		{offsetof(struct fs, fs_magic)/4, FS_UFS1_MAGIC, ~0u},
		{offsetof(struct fs, fs_magic)/4, FS_UFS2_MAGIC, ~0u},
		{0, 0x70162, ~0u},		/* LFS_MAGIC */
		{14, 0xef53, 0xffff},		/* EXT2FS (little) */
		{14, 0xef530000, 0xffff0000},	/* EXT2FS (big) */
		{.offset = ~0u},
	};
	const struct fsm *fsm;

	if (Nflag)
		return;

	if (sblkoff == 0)	/* Why did UFS2 add support for this?  sigh. */
		return;

	if (cg0_data == 0)
		/* For FFSv1 this could include all the inodes. */
		cg0_data = cgsblock(&sblock, 0) * sblock.fs_fsize + iobufsize;

	/* Ignore anything that is beyond our filesystem */
	if ((sblkoff + SBLOCKSIZE)/sectorsize >= fssize)
		return;
	/* Zero anything inside our filesystem... */
	if (sblkoff >= sblock.fs_sblockloc) {
		/* ...unless we will write that area anyway */
		if (sblkoff >= cg0_data)
			wtfs(sblkoff / sectorsize,
			    roundup(sizeof sblock, sectorsize), iobuf);
		return;
	}

	/* The sector might contain boot code, so we must validate it */
	rdfs(sblkoff/sectorsize, sizeof oldfs, &oldfs);
	for (fsm = fs_magics; ; fsm++) {
		uint32_t v;
		if (fsm->mask == 0)
			return;
		v = oldfs[fsm->offset];
		if ((v & fsm->mask) == fsm->magic ||
		    (bswap32(v) & fsm->mask) == fsm->magic)
			break;
	}

	/* Just zap the magic number */
	oldfs[fsm->offset] = 0;
	wtfs(sblkoff/sectorsize, sizeof oldfs, &oldfs);
}


#ifdef MFS
/*
 * Internal version of malloc that trims the requested size if not enough
 * memory is available.
 */
static void *
mkfs_malloc(size_t size)
{
	u_long pgsz;
	caddr_t *memory, *extra;
	size_t exsize = 128 * 1024;

	if (size == 0)
		return (NULL);

	pgsz = getpagesize() - 1;
	size = (size + pgsz) &~ pgsz;

	/* try to map requested size */
	memory = mmap(0, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE,
	    -1, 0);
	if (memory == MAP_FAILED)
		return NULL;

	/* try to map something extra */
	extra = mmap(0, exsize, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE,
	    -1, 0);
	munmap(extra, exsize);

	/* if extra memory couldn't be mapped, reduce original request accordingly */
	if (extra == MAP_FAILED) {
		munmap(memory, size);
		size -= exsize;
		memory = mmap(0, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE,
		    -1, 0);
		if (memory == MAP_FAILED)
			return NULL;
	}

	return memory;
}
#endif	/* MFS */
