/*	$NetBSD: mkfs.c,v 1.12 2003/01/24 21:55:33 fvdl Exp $	*/
/* From NetBSD: mkfs.c,v 1.59 2001/12/31 07:07:58 lukem Exp $	*/

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
#if defined(__RCSID) && !defined(lint)
#if 0
static char sccsid[] = "@(#)mkfs.c	8.11 (Berkeley) 5/3/95";
#else
__RCSID("$NetBSD: mkfs.c,v 1.12 2003/01/24 21:55:33 fvdl Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "makefs.h"

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/ufs_bswap.h>
#include <ufs/ffs/fs.h>

#include "ffs/ufs_inode.h"
#include "ffs/ffs_extern.h"
#include "ffs/newfs_extern.h"

static void initcg(int, time_t, const fsinfo_t *);
static int32_t calcipg(int32_t, int32_t, off_t *);
static void swap_cg(struct cg *, struct cg *);

static int count_digits(int);

/*
 * make file system for cylinder-group style file systems
 */

/*
 * We limit the size of the inode map to be no more than a
 * third of the cylinder group space, since we must leave at
 * least an equal amount of space for the block map.
 *
 * N.B.: MAXIPG must be a multiple of INOPB(fs).
 */
#define MAXIPG(fs)	roundup((fs)->fs_bsize * NBBY / 3, INOPB(fs))

#define UMASK		0755
#define POWEROF2(num)	(((num) & ((num) - 1)) == 0)

union {
	struct fs fs;
	char pad[SBSIZE];
} fsun;
#define	sblock	fsun.fs

union {
	struct cg cg;
	char pad[MAXBSIZE];
} cgun;
#define	acg	cgun.cg

struct dinode zino[MAXBSIZE / DINODE_SIZE];

char writebuf[MAXBSIZE];

static	int	Oflag;		/* format as an 4.3BSD file system */
static	int	fssize;		/* file system size */
static	int	ntracks;	/* # tracks/cylinder */
static	int	nsectors;	/* # sectors/track */
static	int	nphyssectors;	/* # sectors/track including spares */
static	int	secpercyl;	/* sectors per cylinder */
static	int	sectorsize;	/* bytes/sector */
static	int	rpm;		/* revolutions/minute of drive */
static	int	interleave;	/* hardware sector interleave */
static	int	trackskew;	/* sector 0 skew, per track */
static	int	fsize;		/* fragment size */
static	int	bsize;		/* block size */
static	int	cpg;		/* cylinders/cylinder group */
static	int	cpgflg;		/* cylinders/cylinder group flag was given */
static	int	minfree;	/* free space threshold */
static	int	opt;		/* optimization preference (space or time) */
static	int	density;	/* number of bytes per inode */
static	int	maxcontig;	/* max contiguous blocks to allocate */
static	int	rotdelay;	/* rotational delay between blocks */
static	int	maxbpg;		/* maximum blocks per file in a cyl group */
static	int	nrpos;		/* # of distinguished rotational positions */
static	int	bbsize;		/* boot block size */
static	int	sbsize;		/* superblock size */
static	int	avgfilesize;	/* expected average file size */
static	int	avgfpdir;	/* expected number of files per directory */


struct fs *
ffs_mkfs(const char *fsys, const fsinfo_t *fsopts)
{
	int32_t i, mincpc, mincpg, inospercg;
	int32_t cylno, rpos, blk, j, warned = 0;
	int32_t used, mincpgcnt, bpcg;
	off_t usedb;
	int32_t mapcramped, inodecramped;
	int32_t postblsize, rotblsize, totalsbsize;
	long long sizepb;
	void *space;
	int size, blks;
	int nprintcols, printcolwidth;

	Oflag = 0;
	fssize =	fsopts->size / fsopts->sectorsize;
	ntracks =	fsopts->ntracks;
	nsectors =	fsopts->nsectors;
	nphyssectors =	fsopts->nsectors;	/* XXX: no trackspares */
	secpercyl =	nsectors * ntracks;
	sectorsize =	fsopts->sectorsize;
	rpm =		fsopts->rpm;
	interleave =	1;
	trackskew =	0;
	fsize =		fsopts->fsize;
	bsize =		fsopts->bsize;
	cpg =		fsopts->cpg;
	cpgflg =	fsopts->cpgflg;
	minfree =	fsopts->minfree;
	opt =		fsopts->optimization;
	density =	fsopts->density;
	maxcontig =	fsopts->maxcontig;
	rotdelay =	fsopts->rotdelay;
	maxbpg =	fsopts->maxbpg;
	nrpos =		fsopts->nrpos;
	bbsize =	BBSIZE;
	sbsize =	SBSIZE;
	avgfilesize = 	fsopts->avgfilesize;
	avgfpdir = 	fsopts->avgfpdir;

	if (Oflag) {
		sblock.fs_inodefmt = FS_42INODEFMT;
		sblock.fs_maxsymlinklen = 0;
	} else {
		sblock.fs_inodefmt = FS_44INODEFMT;
		sblock.fs_maxsymlinklen = MAXSYMLINKLEN;
	}
	/*
	 * Validate the given file system size.
	 * Verify that its last block can actually be accessed.
	 */
	if (fssize <= 0)
		printf("preposterous size %d\n", fssize), exit(13);
	ffs_wtfs(fssize - 1, sectorsize, (char *)&sblock, fsopts);

	/*
	 * collect and verify the sector and track info
	 */
	sblock.fs_nsect = nsectors;
	sblock.fs_ntrak = ntracks;
	if (sblock.fs_ntrak <= 0)
		printf("preposterous ntrak %d\n", sblock.fs_ntrak), exit(14);
	if (sblock.fs_nsect <= 0)
		printf("preposterous nsect %d\n", sblock.fs_nsect), exit(15);
	/*
	 * collect and verify the filesystem density info
	 */
	sblock.fs_avgfilesize = avgfilesize;
	sblock.fs_avgfpdir = avgfpdir;
	if (sblock.fs_avgfilesize <= 0)
		printf("illegal expected average file size %d\n",
		    sblock.fs_avgfilesize), exit(14);
	if (sblock.fs_avgfpdir <= 0)
		printf("illegal expected number of files per directory %d\n",
		    sblock.fs_avgfpdir), exit(15);
	/*
	 * collect and verify the block and fragment sizes
	 */
	sblock.fs_bsize = bsize;
	sblock.fs_fsize = fsize;
	if (!POWEROF2(sblock.fs_bsize)) {
		printf("block size must be a power of 2, not %d\n",
		    sblock.fs_bsize);
		exit(16);
	}
	if (!POWEROF2(sblock.fs_fsize)) {
		printf("fragment size must be a power of 2, not %d\n",
		    sblock.fs_fsize);
		exit(17);
	}
	if (sblock.fs_fsize < sectorsize) {
		printf("fragment size %d is too small, minimum is %d\n",
		    sblock.fs_fsize, sectorsize);
		exit(18);
	}
	if (sblock.fs_bsize > MAXBSIZE) {
		printf("block size %d is too large, maximum is %d\n",
		    sblock.fs_bsize, MAXBSIZE);
		exit(19);
	}
	if (sblock.fs_bsize < MINBSIZE) {
		printf("block size %d is too small, minimum is %d\n",
		    sblock.fs_bsize, MINBSIZE);
		exit(19);
	}
	if (sblock.fs_bsize < sblock.fs_fsize) {
		printf("block size (%d) cannot be smaller than fragment size (%d)\n",
		    sblock.fs_bsize, sblock.fs_fsize);
		exit(20);
	}
	sblock.fs_bmask = ~(sblock.fs_bsize - 1);
	sblock.fs_fmask = ~(sblock.fs_fsize - 1);
	sblock.fs_qbmask = ~sblock.fs_bmask;
	sblock.fs_qfmask = ~sblock.fs_fmask;
	for (sblock.fs_bshift = 0, i = sblock.fs_bsize; i > 1; i >>= 1)
		sblock.fs_bshift++;
	for (sblock.fs_fshift = 0, i = sblock.fs_fsize; i > 1; i >>= 1)
		sblock.fs_fshift++;
	sblock.fs_frag = numfrags(&sblock, sblock.fs_bsize);
	for (sblock.fs_fragshift = 0, i = sblock.fs_frag; i > 1; i >>= 1)
		sblock.fs_fragshift++;
	if (sblock.fs_frag > MAXFRAG) {
		printf("fragment size %d is too small, "
			"minimum with block size %d is %d\n",
		    sblock.fs_fsize, sblock.fs_bsize,
		    sblock.fs_bsize / MAXFRAG);
		exit(21);
	}
	sblock.fs_nrpos = nrpos;
	/* XXX ondisk32 */
	sblock.fs_nindir = sblock.fs_bsize / sizeof(int32_t);
	sblock.fs_inopb = sblock.fs_bsize / DINODE_SIZE;
	sblock.fs_nspf = sblock.fs_fsize / sectorsize;
	for (sblock.fs_fsbtodb = 0, i = NSPF(&sblock); i > 1; i >>= 1)
		sblock.fs_fsbtodb++;
	sblock.fs_sblkno =
	    roundup(howmany(bbsize + sbsize, sblock.fs_fsize), sblock.fs_frag);
	/* XXX ondisk32 */
	sblock.fs_cblkno = (daddr_t)(sblock.fs_sblkno +
	    roundup(howmany(sbsize, sblock.fs_fsize), sblock.fs_frag));
	sblock.fs_iblkno = sblock.fs_cblkno + sblock.fs_frag;
	sblock.fs_cgoffset = roundup(
	    howmany(sblock.fs_nsect, NSPF(&sblock)), sblock.fs_frag);
	for (sblock.fs_cgmask = 0xffffffff, i = sblock.fs_ntrak; i > 1; i >>= 1)
		sblock.fs_cgmask <<= 1;
	if (!POWEROF2(sblock.fs_ntrak))
		sblock.fs_cgmask <<= 1;
	sblock.fs_maxfilesize = sblock.fs_bsize * NDADDR - 1;
	for (sizepb = sblock.fs_bsize, i = 0; i < NIADDR; i++) {
		sizepb *= NINDIR(&sblock);
		sblock.fs_maxfilesize += sizepb;
	}
	/*
	 * Validate specified/determined secpercyl
	 * and calculate minimum cylinders per group.
	 */
	sblock.fs_spc = secpercyl;
	for (sblock.fs_cpc = NSPB(&sblock), i = sblock.fs_spc;
	     sblock.fs_cpc > 1 && (i & 1) == 0;
	     sblock.fs_cpc >>= 1, i >>= 1)
		/* void */;
	mincpc = sblock.fs_cpc;
	bpcg = sblock.fs_spc * sectorsize;
	inospercg = roundup(bpcg / DINODE_SIZE, INOPB(&sblock));
	if (inospercg > MAXIPG(&sblock))
		inospercg = MAXIPG(&sblock);
	used = (sblock.fs_iblkno + inospercg / INOPF(&sblock)) * NSPF(&sblock);
	mincpgcnt = howmany(sblock.fs_cgoffset * (~sblock.fs_cgmask) + used,
	    sblock.fs_spc);
	mincpg = roundup(mincpgcnt, mincpc);
	/*
	 * Ensure that cylinder group with mincpg has enough space
	 * for block maps.
	 */
	sblock.fs_cpg = mincpg;
	sblock.fs_ipg = inospercg;
	if (maxcontig > 1)
		sblock.fs_contigsumsize = MIN(maxcontig, FS_MAXCONTIG);
	mapcramped = 0;
	while (CGSIZE(&sblock) > sblock.fs_bsize) {
		mapcramped = 1;
		if (sblock.fs_bsize < MAXBSIZE) {
			sblock.fs_bsize <<= 1;
			if ((i & 1) == 0) {
				i >>= 1;
			} else {
				sblock.fs_cpc <<= 1;
				mincpc <<= 1;
				mincpg = roundup(mincpgcnt, mincpc);
				sblock.fs_cpg = mincpg;
			}
			sblock.fs_frag <<= 1;
			sblock.fs_fragshift += 1;
			if (sblock.fs_frag <= MAXFRAG)
				continue;
		}
		if (sblock.fs_fsize == sblock.fs_bsize) {
			printf("There is no block size that");
			printf(" can support this disk\n");
			exit(22);
		}
		sblock.fs_frag >>= 1;
		sblock.fs_fragshift -= 1;
		sblock.fs_fsize <<= 1;
		sblock.fs_nspf <<= 1;
	}
	/*
	 * Ensure that cylinder group with mincpg has enough space for inodes.
	 */
	inodecramped = 0;
	inospercg = calcipg(mincpg, bpcg, &usedb);
	sblock.fs_ipg = inospercg;
	while (inospercg > MAXIPG(&sblock)) {
		inodecramped = 1;
		if (mincpc == 1 || sblock.fs_frag == 1 ||
		    sblock.fs_bsize == MINBSIZE)
			break;
		printf("With a block size of %d %s %d\n", sblock.fs_bsize,
		       "minimum bytes per inode is",
		       (int)((mincpg * (off_t)bpcg - usedb)
			     / MAXIPG(&sblock) + 1));
		sblock.fs_bsize >>= 1;
		sblock.fs_frag >>= 1;
		sblock.fs_fragshift -= 1;
		mincpc >>= 1;
		sblock.fs_cpg = roundup(mincpgcnt, mincpc);
		if (CGSIZE(&sblock) > sblock.fs_bsize) {
			sblock.fs_bsize <<= 1;
			break;
		}
		mincpg = sblock.fs_cpg;
		inospercg = calcipg(mincpg, bpcg, &usedb);
		sblock.fs_ipg = inospercg;
	}
	if (inodecramped) {
		if (inospercg > MAXIPG(&sblock)) {
			printf("Minimum bytes per inode is %d\n",
			       (int)((mincpg * (off_t)bpcg - usedb)
				     / MAXIPG(&sblock) + 1));
		} else if (!mapcramped) {
			printf("With %d bytes per inode, ", density);
			printf("minimum cylinders per group is %d\n", mincpg);
		}
	}
	if (mapcramped) {
		printf("With %d sectors per cylinder, ", sblock.fs_spc);
		printf("minimum cylinders per group is %d\n", mincpg);
	}
	if (inodecramped || mapcramped) {
		if (sblock.fs_bsize != bsize)
			printf("%s to be changed from %d to %d\n",
			    "This requires the block size",
			    bsize, sblock.fs_bsize);
		if (sblock.fs_fsize != fsize)
			printf("\t%s to be changed from %d to %d\n",
			    "and the fragment size",
			    fsize, sblock.fs_fsize);
		exit(23);
	}
	/* 
	 * Calculate the number of cylinders per group
	 */
	sblock.fs_cpg = cpg;
	if (sblock.fs_cpg % mincpc != 0) {
		printf("%s groups must have a multiple of %d cylinders\n",
			cpgflg ? "Cylinder" : "Warning: cylinder", mincpc);
		sblock.fs_cpg = roundup(sblock.fs_cpg, mincpc);
		if (!cpgflg)
			cpg = sblock.fs_cpg;
	}
	/*
	 * Must ensure there is enough space for inodes.
	 */
	sblock.fs_ipg = calcipg(sblock.fs_cpg, bpcg, &usedb);
	while (sblock.fs_ipg > MAXIPG(&sblock)) {
		inodecramped = 1;
		sblock.fs_cpg -= mincpc;
		sblock.fs_ipg = calcipg(sblock.fs_cpg, bpcg, &usedb);
	}
	/*
	 * Must ensure there is enough space to hold block map.
	 */
	while (CGSIZE(&sblock) > sblock.fs_bsize) {
		mapcramped = 1;
		sblock.fs_cpg -= mincpc;
		sblock.fs_ipg = calcipg(sblock.fs_cpg, bpcg, &usedb);
	}
	sblock.fs_fpg = (sblock.fs_cpg * sblock.fs_spc) / NSPF(&sblock);
	if ((sblock.fs_cpg * sblock.fs_spc) % NSPB(&sblock) != 0) {
		printf("panic (fs_cpg * fs_spc) %% NSPF != 0");
		exit(24);
	}
	if (sblock.fs_cpg < mincpg) {
		printf("cylinder groups must have at least %d cylinders\n",
			mincpg);
		exit(25);
	} else if (sblock.fs_cpg != cpg && cpgflg) {
		if (!mapcramped && !inodecramped)
			exit(26);
		if (mapcramped && inodecramped)
			printf("Block size and bytes per inode restrict");
		else if (mapcramped)
			printf("Block size restricts");
		else
			printf("Bytes per inode restrict");
		printf(" cylinders per group to %d.\n", sblock.fs_cpg);
		exit(27);
	}
	sblock.fs_cgsize = fragroundup(&sblock, CGSIZE(&sblock));
	/*
	 * Now have size for file system and nsect and ntrak.
	 * Determine number of cylinders and blocks in the file system.
	 */
	sblock.fs_size = fssize = dbtofsb(&sblock, fssize);
	sblock.fs_ncyl = fssize * NSPF(&sblock) / sblock.fs_spc;
	if (fssize * NSPF(&sblock) > sblock.fs_ncyl * sblock.fs_spc) {
		sblock.fs_ncyl++;
		warned = 1;
	}
	if (sblock.fs_ncyl < 1) {
		printf("file systems must have at least one cylinder\n");
		exit(28);
	}
	/*
	 * Determine feasability/values of rotational layout tables.
	 *
	 * The size of the rotational layout tables is limited by the
	 * size of the superblock, SBSIZE. The amount of space available
	 * for tables is calculated as (SBSIZE - sizeof (struct fs)).
	 * The size of these tables is inversely proportional to the block
	 * size of the file system. The size increases if sectors per track
	 * are not powers of two, because more cylinders must be described
	 * by the tables before the rotational pattern repeats (fs_cpc).
	 */
	sblock.fs_interleave = interleave;
	sblock.fs_trackskew = trackskew;
	sblock.fs_npsect = nphyssectors;
	sblock.fs_postblformat = FS_DYNAMICPOSTBLFMT;
	sblock.fs_sbsize = fragroundup(&sblock, sizeof(struct fs));
	if (sblock.fs_ntrak == 1) {
		sblock.fs_cpc = 0;
		goto next;
	}
	postblsize = sblock.fs_nrpos * sblock.fs_cpc * sizeof(int16_t);
	rotblsize = sblock.fs_cpc * sblock.fs_spc / NSPB(&sblock);
	totalsbsize = sizeof(struct fs) + rotblsize;
	if (sblock.fs_nrpos == 8 && sblock.fs_cpc <= 16) {
		/* use old static table space */
		sblock.fs_postbloff = (char *)(&sblock.fs_opostbl[0][0]) -
		    (char *)(&sblock.fs_firstfield);
		sblock.fs_rotbloff = &sblock.fs_space[0] -
		    (u_char *)(&sblock.fs_firstfield);
	} else {
		/* use dynamic table space */
		sblock.fs_postbloff = &sblock.fs_space[0] -
		    (u_char *)(&sblock.fs_firstfield);
		sblock.fs_rotbloff = sblock.fs_postbloff + postblsize;
		totalsbsize += postblsize;
	}
	if (totalsbsize > SBSIZE ||
	    sblock.fs_nsect > (1 << NBBY) * NSPB(&sblock)) {
		printf("%s %s %d %s %d.%s",
		    "Warning: insufficient space in super block for\n",
		    "rotational layout tables with nsect", sblock.fs_nsect,
		    "and ntrak", sblock.fs_ntrak,
		    "\nFile system performance may be impaired.\n");
		sblock.fs_cpc = 0;
		goto next;
	}
	sblock.fs_sbsize = fragroundup(&sblock, totalsbsize);
	/*
	 * calculate the available blocks for each rotational position
	 */
	for (cylno = 0; cylno < sblock.fs_cpc; cylno++)
		for (rpos = 0; rpos < sblock.fs_nrpos; rpos++)
			fs_postbl(&sblock, cylno)[rpos] = -1;
	for (i = (rotblsize - 1) * sblock.fs_frag;
	     i >= 0; i -= sblock.fs_frag) {
		cylno = cbtocylno(&sblock, i);
		rpos = cbtorpos(&sblock, i);
		blk = fragstoblks(&sblock, i);
		if (fs_postbl(&sblock, cylno)[rpos] == -1)
			fs_rotbl(&sblock)[blk] = 0;
		else
			fs_rotbl(&sblock)[blk] = fs_postbl(&sblock, cylno)[rpos] - blk;
		fs_postbl(&sblock, cylno)[rpos] = blk;
	}
next:
	/*
	 * Compute/validate number of cylinder groups.
	 */
	sblock.fs_ncg = sblock.fs_ncyl / sblock.fs_cpg;
	if (sblock.fs_ncyl % sblock.fs_cpg)
		sblock.fs_ncg++;
	sblock.fs_dblkno = sblock.fs_iblkno + sblock.fs_ipg / INOPF(&sblock);
	i = MIN(~sblock.fs_cgmask, sblock.fs_ncg - 1);
	if (cgdmin(&sblock, i) - cgbase(&sblock, i) >= sblock.fs_fpg) {
		printf("inode blocks/cyl group (%lld) >= data blocks (%d)\n",
		    (long long)cgdmin(&sblock, i) -
			cgbase(&sblock, i) / sblock.fs_frag,
		    sblock.fs_fpg / sblock.fs_frag);
		printf("number of cylinders per cylinder group (%d) %s.\n",
		    sblock.fs_cpg, "must be increased");
		exit(29);
	}
	j = sblock.fs_ncg - 1;
	if ((i = fssize - j * sblock.fs_fpg) < sblock.fs_fpg &&
	    cgdmin(&sblock, j) - cgbase(&sblock, j) > i) {
		if (j == 0) {
			printf("File system must have at least %lld sectors\n",
			    (long long)NSPF(&sblock) *
			    (cgdmin(&sblock, 0) + 3 * sblock.fs_frag));
			exit(30);
		}
		printf("Warning: inode blocks/cyl group (%lld) >= "
			"data blocks (%d) in last\n",
		    (long long)(cgdmin(&sblock, j) - cgbase(&sblock, j))
			/ sblock.fs_frag,
		    i / sblock.fs_frag);
		printf("    cylinder group. This implies %d sector(s) "
			"cannot be allocated.\n",
		    i * NSPF(&sblock));
		sblock.fs_ncg--;
		sblock.fs_ncyl -= sblock.fs_ncyl % sblock.fs_cpg;
		sblock.fs_size = fssize = sblock.fs_ncyl * sblock.fs_spc /
		    NSPF(&sblock);
		warned = 0;
	}
	if (warned) {
		printf("Warning: %d sector(s) in last cylinder unallocated\n",
		    sblock.fs_spc -
		    (fssize * NSPF(&sblock) - (sblock.fs_ncyl - 1)
		    * sblock.fs_spc));
	}
	/*
	 * fill in remaining fields of the super block
	 */
	sblock.fs_csaddr = cgdmin(&sblock, 0);
	sblock.fs_cssize =
	    fragroundup(&sblock, sblock.fs_ncg * sizeof(struct csum));
	/*
	 * The superblock fields 'fs_csmask' and 'fs_csshift' are no
	 * longer used. However, we still initialise them so that the
	 * filesystem remains compatible with old kernels.
	 */
	i = sblock.fs_bsize / sizeof(struct csum);
	sblock.fs_csmask = ~(i - 1);
	for (sblock.fs_csshift = 0; i > 1; i >>= 1)
		sblock.fs_csshift++;

	/*
	 * Setup memory for temporary in-core cylgroup summaries.
	 * Cribbed from ffs_mountfs().
	 */
	size = sblock.fs_cssize;
	blks = howmany(size, sblock.fs_fsize);
	if (sblock.fs_contigsumsize > 0)
		size += sblock.fs_ncg * sizeof(int32_t);
	if ((space = (char *)calloc(1, size)) == NULL)
		err(1, "memory allocation error for cg summaries");
	sblock.fs_csp = space;
	space = (char *)space + sblock.fs_cssize;
	if (sblock.fs_contigsumsize > 0) {
		int32_t *lp;

		sblock.fs_maxcluster = lp = space;
		for (i = 0; i < sblock.fs_ncg; i++)
			*lp++ = sblock.fs_contigsumsize;
	}

	sblock.fs_magic = FS_MAGIC;
	sblock.fs_rotdelay = rotdelay;
	sblock.fs_minfree = minfree;
	sblock.fs_maxcontig = maxcontig;
	sblock.fs_maxbpg = maxbpg;
	sblock.fs_rps = rpm / 60;
	sblock.fs_optim = opt;
	sblock.fs_cgrotor = 0;
	sblock.fs_cstotal.cs_ndir = 0;
	sblock.fs_cstotal.cs_nbfree = 0;
	sblock.fs_cstotal.cs_nifree = 0;
	sblock.fs_cstotal.cs_nffree = 0;
	sblock.fs_fmod = 0;
	sblock.fs_clean = FS_ISCLEAN;
	sblock.fs_ronly = 0;

	/*
	 * Dump out summary information about file system.
	 */
	printf("%s:\t%d sectors in %d %s of %d tracks, %d sectors\n",
		    fsys, sblock.fs_size * NSPF(&sblock), sblock.fs_ncyl,
		    "cylinders", sblock.fs_ntrak, sblock.fs_nsect);
#define B2MBFACTOR (1 / (1024.0 * 1024.0))
	printf("\t%.1fMB in %d cyl groups (%d c/g, %.2fMB/g, %d i/g)\n",
		    (float)sblock.fs_size * sblock.fs_fsize * B2MBFACTOR,
		    sblock.fs_ncg, sblock.fs_cpg,
		    (float)sblock.fs_fpg * sblock.fs_fsize * B2MBFACTOR,
		    sblock.fs_ipg);
#undef B2MBFACTOR
	/*
	 * Now determine how wide each column will be, and calculate how
	 * many columns will fit in a 76 char line. 76 is the width of the
	 * subwindows in sysinst.
	 */
	printcolwidth = count_digits(
			fsbtodb(&sblock, cgsblock(&sblock, sblock.fs_ncg -1)));
	nprintcols = 76 / (printcolwidth + 2);
	/*
	 * Now build the cylinders group blocks and
	 * then print out indices of cylinder groups.
	 */
		printf("super-block backups (for fsck -b #) at:");
	for (cylno = 0; cylno < sblock.fs_ncg; cylno++) {
		initcg(cylno, start_time.tv_sec, fsopts);
		if (cylno % nprintcols == 0)
			printf("\n");
		printf(" %*lld,", printcolwidth,
			(long long)fsbtodb(&sblock, cgsblock(&sblock, cylno)));
		fflush(stdout);
	}
	printf("\n");

	/*
	 * Now construct the initial file system,
	 * then write out the super-block.
	 */
	sblock.fs_time = start_time.tv_sec;
	if (fsopts->needswap)
		sblock.fs_flags |= FS_SWAPPED;
	ffs_write_superblock(&sblock, fsopts);
	return (&sblock);
}

/*
 * Write out the superblock and its duplicates,
 * and the cylinder group summaries
 */
void
ffs_write_superblock(struct fs *fs, const fsinfo_t *fsopts)
{
	int	cylno, size, blks, i, saveflag;
	void	*space;
	char	*wrbuf;

	saveflag = fs->fs_flags & FS_INTERNAL;
	fs->fs_flags &= ~FS_INTERNAL;

			/* Write out the master super block */
	memcpy(writebuf, fs, sbsize);
	if (fsopts->needswap)
		ffs_sb_swap(fs, (struct fs*)writebuf);
	ffs_wtfs((int)SBOFF / sectorsize, sbsize, writebuf, fsopts);

			/* Write out the duplicate super blocks */
	for (cylno = 0; cylno < sblock.fs_ncg; cylno++)
		ffs_wtfs(fsbtodb(fs, cgsblock(fs, cylno)),
		    sbsize, writebuf, fsopts);

			/* Write out the cylinder group summaries */
	size = fs->fs_cssize;
	blks = howmany(size, fs->fs_fsize);
	space = (void *)fs->fs_csp;
	if ((wrbuf = malloc(size)) == NULL)
		err(1, "ffs_write_superblock: malloc %d", size);
	for (i = 0; i < blks; i+= fs->fs_frag) {
		size = fs->fs_bsize;
		if (i + fs->fs_frag > blks)
			size = (blks - i) * fs->fs_fsize;
		if (fsopts->needswap)
			ffs_csum_swap((struct csum *)space,
			    (struct csum *)wrbuf, size);
		else
			memcpy(wrbuf, space, (u_int)size);
		ffs_wtfs(fsbtodb(fs, fs->fs_csaddr + i), size, wrbuf, fsopts);
		space = (char *)space + size;
	}
	free(wrbuf);
	fs->fs_flags |= saveflag;
}


/*
 * Initialize a cylinder group.
 */
static void
initcg(int cylno, time_t utime, const fsinfo_t *fsopts)
{
	daddr_t cbase, d, dlower, dupper, dmax, blkno;
	int32_t i;

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
	if (cylno == 0)
		dupper += howmany(sblock.fs_cssize, sblock.fs_fsize);
	memset(&acg, 0, sblock.fs_cgsize);
	acg.cg_time = utime;
	acg.cg_magic = CG_MAGIC;
	acg.cg_cgx = cylno;
	if (cylno == sblock.fs_ncg - 1)
		acg.cg_ncyl = sblock.fs_ncyl % sblock.fs_cpg;
	else
		acg.cg_ncyl = sblock.fs_cpg;
	acg.cg_niblk = sblock.fs_ipg;
	acg.cg_ndblk = dmax - cbase;
	if (sblock.fs_contigsumsize > 0)
		acg.cg_nclusterblks = acg.cg_ndblk / sblock.fs_frag;
	acg.cg_btotoff = &acg.cg_space[0] - (u_char *)(&acg.cg_firstfield);
	acg.cg_boff = acg.cg_btotoff + sblock.fs_cpg * sizeof(int32_t);
	acg.cg_iusedoff = acg.cg_boff + 
		sblock.fs_cpg * sblock.fs_nrpos * sizeof(int16_t);
	acg.cg_freeoff = acg.cg_iusedoff + howmany(sblock.fs_ipg, NBBY);
	if (sblock.fs_contigsumsize <= 0) {
		acg.cg_nextfreeoff = acg.cg_freeoff +
		   howmany(sblock.fs_cpg * sblock.fs_spc / NSPF(&sblock), NBBY);
	} else {
		acg.cg_clustersumoff = acg.cg_freeoff + howmany
		    (sblock.fs_cpg * sblock.fs_spc / NSPF(&sblock), NBBY) -
		    sizeof(int32_t);
		acg.cg_clustersumoff =
		    roundup(acg.cg_clustersumoff, sizeof(int32_t));
		acg.cg_clusteroff = acg.cg_clustersumoff +
		    (sblock.fs_contigsumsize + 1) * sizeof(int32_t);
		acg.cg_nextfreeoff = acg.cg_clusteroff + howmany
		    (sblock.fs_cpg * sblock.fs_spc / NSPB(&sblock), NBBY);
	}
	if (acg.cg_nextfreeoff > sblock.fs_cgsize) {
		printf("Panic: cylinder group too big\n");
		exit(37);
	}
	acg.cg_cs.cs_nifree += sblock.fs_ipg;
	if (cylno == 0)
		for (i = 0; i < ROOTINO; i++) {
			setbit(cg_inosused(&acg, 0), i);
			acg.cg_cs.cs_nifree--;
		}
	for (i = 0; i < sblock.fs_ipg / INOPF(&sblock); i += sblock.fs_frag)
		ffs_wtfs(fsbtodb(&sblock, cgimin(&sblock, cylno) + i),
		    sblock.fs_bsize, (char *)zino, fsopts);
	if (cylno > 0) {
		/*
		 * In cylno 0, beginning space is reserved
		 * for boot and super blocks.
		 */
		for (d = 0; d < dlower; d += sblock.fs_frag) {
			blkno = d / sblock.fs_frag;
			ffs_setblock(&sblock, cg_blksfree(&acg, 0), blkno);
			if (sblock.fs_contigsumsize > 0)
				setbit(cg_clustersfree(&acg, 0), blkno);
			acg.cg_cs.cs_nbfree++;
			cg_blktot(&acg, 0)[cbtocylno(&sblock, d)]++;
			cg_blks(&sblock, &acg, cbtocylno(&sblock, d), 0)
			    [cbtorpos(&sblock, d)]++;
		}
		sblock.fs_dsize += dlower;
	}
	sblock.fs_dsize += acg.cg_ndblk - dupper;
	if ((i = (dupper % sblock.fs_frag)) != 0) {
		acg.cg_frsum[sblock.fs_frag - i]++;
		for (d = dupper + sblock.fs_frag - i; dupper < d; dupper++) {
			setbit(cg_blksfree(&acg, 0), dupper);
			acg.cg_cs.cs_nffree++;
		}
	}
	for (d = dupper; d + sblock.fs_frag <= dmax - cbase; ) {
		blkno = d / sblock.fs_frag;
		ffs_setblock(&sblock, cg_blksfree(&acg, 0), blkno);
		if (sblock.fs_contigsumsize > 0)
			setbit(cg_clustersfree(&acg, 0), blkno);
		acg.cg_cs.cs_nbfree++;
		cg_blktot(&acg, 0)[cbtocylno(&sblock, d)]++;
		cg_blks(&sblock, &acg, cbtocylno(&sblock, d), 0)
		    [cbtorpos(&sblock, d)]++;
		d += sblock.fs_frag;
	}
	if (d < dmax - cbase) {
		acg.cg_frsum[dmax - cbase - d]++;
		for (; d < dmax - cbase; d++) {
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
			if ((i & (NBBY - 1)) != (NBBY - 1)) {
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
	sblock.fs_cstotal.cs_ndir += acg.cg_cs.cs_ndir;
	sblock.fs_cstotal.cs_nffree += acg.cg_cs.cs_nffree;
	sblock.fs_cstotal.cs_nbfree += acg.cg_cs.cs_nbfree;
	sblock.fs_cstotal.cs_nifree += acg.cg_cs.cs_nifree;
	sblock.fs_cs(&sblock, cylno) = acg.cg_cs;
	memcpy(writebuf, &acg, sblock.fs_bsize);
	if (fsopts->needswap)
		swap_cg(&acg, (struct cg*)writebuf);
	ffs_wtfs(fsbtodb(&sblock, cgtod(&sblock, cylno)),
		sblock.fs_bsize,
		writebuf, fsopts);
}

/*
 * Calculate number of inodes per group.
 */
static int32_t
calcipg(int32_t cylpg, int32_t bpcg, off_t *usedbp)
{
	int i;
	int32_t ipg, new_ipg, ncg, ncyl;
	off_t usedb;

	/*
	 * Prepare to scale by fssize / (number of sectors in cylinder groups).
	 * Note that fssize is still in sectors, not file system blocks.
	 */
	ncyl = howmany(fssize, secpercyl);
	ncg = howmany(ncyl, cylpg);
	/*
	 * Iterate a few times to allow for ipg depending on itself.
	 */
	ipg = 0;
	for (i = 0; i < 10; i++) {
		usedb = (sblock.fs_iblkno + ipg / INOPF(&sblock))
			* NSPF(&sblock) * (off_t)sectorsize;
		if (cylpg * (long long)bpcg < usedb) {
			warnx("Too many inodes per cyl group!");
			return (MAXIPG(&sblock)+1);
		}
		new_ipg = (cylpg * (long long)bpcg - usedb) /
		    (long long)density * fssize / (ncg * secpercyl * cylpg);
		if (new_ipg <= 0)
			new_ipg = 1;		/* ensure ipg > 0 */
		new_ipg = roundup(new_ipg, INOPB(&sblock));
		if (new_ipg == ipg)
			break;
		ipg = new_ipg;
	}
	*usedbp = usedb;
	return (ipg);
}


/*
 * read a block from the file system
 */
void
ffs_rdfs(daddr_t bno, int size, void *bf, const fsinfo_t *fsopts)
{
	int n;
	off_t offset;

	offset = bno;
	offset *= fsopts->sectorsize;
	if (lseek(fsopts->fd, offset, SEEK_SET) < 0)
		err(1, "ffs_rdfs: seek error: %lld", (long long)bno);
	n = read(fsopts->fd, bf, size);
	if (n == -1)
		err(1, "ffs_rdfs: read error bno %lld size %d", (long long)bno,
		    size);
	else if (n != size)
		errx(1,
		    "ffs_rdfs: read error bno %lld size %d: short read of %d",
		    (long long)bno, size, n);
}

/*
 * write a block to the file system
 */
void
ffs_wtfs(daddr_t bno, int size, void *bf, const fsinfo_t *fsopts)
{
	int n;
	off_t offset;

	offset = bno;
	offset *= fsopts->sectorsize;
	if (lseek(fsopts->fd, offset, SEEK_SET) < 0)
		err(1, "ffs_wtfs: seek error: %lld", (long long)bno);
	n = write(fsopts->fd, bf, size);
	if (n == -1)
		err(1, "ffs_wtfs: write error bno %lld size %d", (long long)bno,		   size);
	else if (n != size)
		errx(1,
		    "ffs_wtfs: write error bno %lld size %d: short write of %d",
		    (long long)bno, size, n);
}

/* swap byte order of cylinder group */
static void
swap_cg(struct cg *o, struct cg *n)
{
	int i, btotsize, fbsize;
	u_int32_t *n32, *o32;
	u_int16_t *n16, *o16;

	n->cg_firstfield = bswap32(o->cg_firstfield);
	n->cg_magic = bswap32(o->cg_magic);
	n->cg_time = bswap32(o->cg_time);
	n->cg_cgx = bswap32(o->cg_cgx);
	n->cg_ncyl = bswap16(o->cg_ncyl);
	n->cg_niblk = bswap16(o->cg_niblk);
	n->cg_ndblk = bswap32(o->cg_ndblk);
	n->cg_cs.cs_ndir = bswap32(o->cg_cs.cs_ndir);
	n->cg_cs.cs_nbfree = bswap32(o->cg_cs.cs_nbfree);
	n->cg_cs.cs_nifree = bswap32(o->cg_cs.cs_nifree);
	n->cg_cs.cs_nffree = bswap32(o->cg_cs.cs_nffree);
	n->cg_rotor = bswap32(o->cg_rotor);
	n->cg_frotor = bswap32(o->cg_frotor);
	n->cg_irotor = bswap32(o->cg_irotor);
	n->cg_btotoff = bswap32(o->cg_btotoff);
	n->cg_boff = bswap32(o->cg_boff);
	n->cg_iusedoff = bswap32(o->cg_iusedoff);
	n->cg_freeoff = bswap32(o->cg_freeoff);
	n->cg_nextfreeoff = bswap32(o->cg_nextfreeoff);
	n->cg_clustersumoff = bswap32(o->cg_clustersumoff);
	n->cg_clusteroff = bswap32(o->cg_clusteroff);
	n->cg_nclusterblks = bswap32(o->cg_nclusterblks);
	for (i=0; i < MAXFRAG; i++)
		n->cg_frsum[i] = bswap32(o->cg_frsum[i]);

	/* alays new format */
	if (n->cg_magic == CG_MAGIC) {
		btotsize = n->cg_boff - n->cg_btotoff;
		fbsize = n->cg_iusedoff - n->cg_boff;
		n32 = (u_int32_t*)((u_int8_t*)n + n->cg_btotoff);
		o32 = (u_int32_t*)((u_int8_t*)o + n->cg_btotoff);
		n16 = (u_int16_t*)((u_int8_t*)n + n->cg_boff);
		o16 = (u_int16_t*)((u_int8_t*)o + n->cg_boff);
	} else {
		btotsize = bswap32(n->cg_boff) - bswap32(n->cg_btotoff);
		fbsize = bswap32(n->cg_iusedoff) - bswap32(n->cg_boff);
		n32 = (u_int32_t*)((u_int8_t*)n + bswap32(n->cg_btotoff));
		o32 = (u_int32_t*)((u_int8_t*)o + bswap32(n->cg_btotoff));
		n16 = (u_int16_t*)((u_int8_t*)n + bswap32(n->cg_boff));
		o16 = (u_int16_t*)((u_int8_t*)o + bswap32(n->cg_boff));
	}
	for (i=0; i < btotsize / sizeof(u_int32_t); i++)
		n32[i] = bswap32(o32[i]);
	
	for (i=0; i < fbsize/sizeof(u_int16_t); i++)
		n16[i] = bswap16(o16[i]);

	if (n->cg_magic == CG_MAGIC) {
		n32 = (u_int32_t*)((u_int8_t*)n + n->cg_clustersumoff);
		o32 = (u_int32_t*)((u_int8_t*)o + n->cg_clustersumoff);
	} else {
		n32 = (u_int32_t*)((u_int8_t*)n + bswap32(n->cg_clustersumoff));
		o32 = (u_int32_t*)((u_int8_t*)o + bswap32(n->cg_clustersumoff));
	}
	for (i = 1; i < sblock.fs_contigsumsize + 1; i++)
		n32[i] = bswap32(o32[i]);
}

/* Determine how many digits are needed to print a given integer */
static int
count_digits(int num)
{
	int ndig;

	for(ndig = 1; num > 9; num /=10, ndig++);

	return (ndig);
}
