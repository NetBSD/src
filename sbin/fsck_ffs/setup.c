/*	$NetBSD: setup.c,v 1.58 2003/04/02 10:39:26 fvdl Exp $	*/

/*
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

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)setup.c	8.10 (Berkeley) 5/9/95";
#else
__RCSID("$NetBSD: setup.c,v 1.58 2003/04/02 10:39:26 fvdl Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#define FSTYPENAMES
#include <sys/disklabel.h>
#include <sys/file.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/ufs_bswap.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fsck.h"
#include "extern.h"
#include "fsutil.h"

#define POWEROF2(num)	(((num) & ((num) - 1)) == 0)

static void badsb __P((int, char *));
static int calcsb __P((const char *, int, struct fs *));
static struct disklabel *getdisklabel __P((const char *, int));
static struct partition *getdisklabelpart __P((const char *, struct disklabel *));
static int readsb __P((int));
static int readappleufs __P((void));

/*
 * Read in a superblock finding an alternate if necessary.
 * Return 1 if successful, 0 if unsuccessful, -1 if filesystem
 * is already clean (preen mode only).
 */
int
setup(dev)
	const char *dev;
{
	long cg, size, asked, i, j;
	long bmapsize;
	struct disklabel *lp;
	off_t sizepb;
	struct stat statb;
	struct fs proto;
	int doskipclean;
	u_int64_t maxfilesize;
	struct csum *ccsp;

	havesb = 0;
	fswritefd = -1;
	doskipclean = skipclean;
	if (stat(dev, &statb) < 0) {
		printf("Can't stat %s: %s\n", dev, strerror(errno));
		return (0);
	}
	if (!forceimage && !S_ISCHR(statb.st_mode)) {
		pfatal("%s is not a character device", dev);
		if (reply("CONTINUE") == 0)
			return (0);
	}
	if ((fsreadfd = open(dev, O_RDONLY)) < 0) {
		printf("Can't open %s: %s\n", dev, strerror(errno));
		return (0);
	}
	if (preen == 0)
		printf("** %s", dev);
	if (nflag || (fswritefd = open(dev, O_WRONLY)) < 0) {
		fswritefd = -1;
		if (preen)
			pfatal("NO WRITE ACCESS");
		printf(" (NO WRITE)");
	}
	if (preen == 0)
		printf("\n");
	fsmodified = 0;
	lfdir = 0;
	initbarea(&sblk);
	initbarea(&asblk);
	sblk.b_un.b_buf = malloc(SBLOCKSIZE);
	sblock = malloc(SBLOCKSIZE);
	asblk.b_un.b_buf = malloc(SBLOCKSIZE);
	altsblock = malloc(SBLOCKSIZE);
	if (sblk.b_un.b_buf == NULL || asblk.b_un.b_buf == NULL ||
		sblock == NULL || altsblock == NULL)
		errx(EEXIT, "cannot allocate space for superblock");
	if (!forceimage && (lp = getdisklabel(NULL, fsreadfd)) != NULL)
		dev_bsize = secsize = lp->d_secsize;
	else
		dev_bsize = secsize = DEV_BSIZE;
	/*
	 * Read in the superblock, looking for alternates if necessary
	 */
	if (readsb(1) == 0) {
		if (bflag || preen || forceimage ||
		    calcsb(dev, fsreadfd, &proto) == 0)
			return(0);
		if (reply("LOOK FOR ALTERNATE SUPERBLOCKS") == 0)
			return (0);
		for (cg = 0; cg < proto.fs_ncg; cg++) {
			bflag = fsbtodb(&proto, cgsblock(&proto, cg));
			if (readsb(0) != 0)
				break;
		}
		if (cg >= proto.fs_ncg) {
			printf("%s %s\n%s %s\n%s %s\n",
				"SEARCH FOR ALTERNATE SUPER-BLOCK",
				"FAILED. YOU MUST USE THE",
				"-b OPTION TO fsck_ffs TO SPECIFY THE",
				"LOCATION OF AN ALTERNATE",
				"SUPER-BLOCK TO SUPPLY NEEDED",
				"INFORMATION; SEE fsck_ffs(8).");
			return(0);
		}
		doskipclean = 0;
		pwarn("USING ALTERNATE SUPERBLOCK AT %d\n", bflag);
	}
	if (debug)
		printf("clean = %d\n", sblock->fs_clean);
	if (doswap)
		doskipclean = 0;
	if (sblock->fs_clean & FS_ISCLEAN) {
		if (doskipclean) {
			pwarn("%sile system is clean; not checking\n",
			    preen ? "f" : "** F");
			return (-1);
		}
		if (!preen && !doswap)
			pwarn("** File system is already clean\n");
	}
	maxfsblock = sblock->fs_size;
	maxino = sblock->fs_ncg * sblock->fs_ipg;
	sizepb = sblock->fs_bsize;
	maxfilesize = sblock->fs_bsize * NDADDR - 1;
	for (i = 0; i < NIADDR; i++) {
		sizepb *= NINDIR(sblock);
		maxfilesize += sizepb;
	}
	/*
	 * Check and potentially fix certain fields in the super block.
	 */
	if (sblock->fs_optim != FS_OPTTIME && sblock->fs_optim != FS_OPTSPACE) {
		pfatal("UNDEFINED OPTIMIZATION IN SUPERBLOCK");
		if (reply("SET TO DEFAULT") == 1) {
			sblock->fs_optim = FS_OPTTIME;
			sbdirty();
		}
	}
	if ((sblock->fs_minfree < 0 || sblock->fs_minfree > 99)) {
		pfatal("IMPOSSIBLE MINFREE=%d IN SUPERBLOCK",
			sblock->fs_minfree);
		if (reply("SET TO DEFAULT") == 1) {
			sblock->fs_minfree = 10;
			sbdirty();
		}
	}
	if (!is_ufs2 && sblock->fs_old_postblformat != FS_42POSTBLFMT &&
	    (sblock->fs_old_interleave < 1 || 
	    sblock->fs_old_interleave > sblock->fs_old_nsect)) {
		pwarn("IMPOSSIBLE INTERLEAVE=%d IN SUPERBLOCK",
			sblock->fs_old_interleave);
		sblock->fs_old_interleave = 1;
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("SET TO DEFAULT") == 1) {
			sbdirty();
			dirty(&asblk);
		}
	}
	if (!is_ufs2 && sblock->fs_old_postblformat != FS_42POSTBLFMT &&
	    (sblock->fs_old_npsect < sblock->fs_old_nsect || 
	    sblock->fs_old_npsect > sblock->fs_old_nsect*2)) {
		pwarn("IMPOSSIBLE NPSECT=%d IN SUPERBLOCK",
			sblock->fs_old_npsect);
		sblock->fs_old_npsect = sblock->fs_old_nsect;
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("SET TO DEFAULT") == 1) {
			sbdirty();
			dirty(&asblk);
		}
	}
	if (sblock->fs_bmask != ~(sblock->fs_bsize - 1)) {
		pwarn("INCORRECT BMASK=0x%x IN SUPERBLOCK",
			sblock->fs_bmask);
		sblock->fs_bmask = ~(sblock->fs_bsize - 1);
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("FIX") == 1) {
			sbdirty();
			dirty(&asblk);
		}
	}
	if (sblock->fs_fmask != ~(sblock->fs_fsize - 1)) {
		pwarn("INCORRECT FMASK=0x%x IN SUPERBLOCK",
			sblock->fs_fmask);
		sblock->fs_fmask = ~(sblock->fs_fsize - 1);
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("FIX") == 1) {
			sbdirty();
			dirty(&asblk);
		}
	}
	if (sblock->fs_old_inodefmt >= FS_44INODEFMT) {
		if (sblock->fs_maxfilesize != maxfilesize) {
			pwarn("INCORRECT MAXFILESIZE=%lld IN SUPERBLOCK",
			    (unsigned long long)sblock->fs_maxfilesize);
			sblock->fs_maxfilesize = maxfilesize;
			if (preen)
				printf(" (FIXED)\n");
			if (preen || reply("FIX") == 1) {
				sbdirty();
				dirty(&asblk);
			}
		}
		if ((is_ufs2 && sblock->fs_maxsymlinklen != MAXSYMLINKLEN_UFS2)
		    ||
		   (!is_ufs2 && sblock->fs_maxsymlinklen != MAXSYMLINKLEN_UFS1))
		    {
			pwarn("INCORRECT MAXSYMLINKLEN=%d IN SUPERBLOCK",
				sblock->fs_maxsymlinklen);
			sblock->fs_maxsymlinklen = is_ufs2 ?
			    MAXSYMLINKLEN_UFS2 : MAXSYMLINKLEN_UFS1;
			if (preen)
				printf(" (FIXED)\n");
			if (preen || reply("FIX") == 1) {
				sbdirty();
				dirty(&asblk);
			}
		}
		if (sblock->fs_qbmask != ~sblock->fs_bmask) {
			pwarn("INCORRECT QBMASK=%llx IN SUPERBLOCK",
			    (unsigned long long)sblock->fs_qbmask);
			sblock->fs_qbmask = ~sblock->fs_bmask;
			if (preen)
				printf(" (FIXED)\n");
			if (preen || reply("FIX") == 1) {
				sbdirty();
				dirty(&asblk);
			}
		}
		if (sblock->fs_qfmask != ~sblock->fs_fmask) {
			pwarn("INCORRECT QFMASK=%llx IN SUPERBLOCK",
			    (unsigned long long)sblock->fs_qfmask);
			sblock->fs_qfmask = ~sblock->fs_fmask;
			if (preen)
				printf(" (FIXED)\n");
			if (preen || reply("FIX") == 1) {
				sbdirty();
				dirty(&asblk);
			}
		}
		newinofmt = 1;
	} else {
		sblock->fs_qbmask = ~sblock->fs_bmask;
		sblock->fs_qfmask = ~sblock->fs_fmask;
		newinofmt = 0;
	}
	/*
	 * Convert to new inode format.
	 */
	if (!is_ufs2 && cvtlevel >= 2 &&
	    sblock->fs_old_inodefmt < FS_44INODEFMT) {
		if (preen)
			pwarn("CONVERTING TO NEW INODE FORMAT\n");
		else if (!reply("CONVERT TO NEW INODE FORMAT"))
			return(0);
		doinglevel2++;
		sblock->fs_old_inodefmt = FS_44INODEFMT;
		sblock->fs_maxfilesize = maxfilesize;
		sblock->fs_maxsymlinklen = MAXSYMLINKLEN_UFS1;
		sblock->fs_qbmask = ~sblock->fs_bmask;
		sblock->fs_qfmask = ~sblock->fs_fmask;
		sbdirty();
		dirty(&asblk);
	}
	/*
	 * Convert to new cylinder group format.
	 */
	if (!is_ufs2 && cvtlevel >= 1 &&
	    sblock->fs_old_postblformat == FS_42POSTBLFMT) {
		if (preen)
			pwarn("CONVERTING TO NEW CYLINDER GROUP FORMAT\n");
		else if (!reply("CONVERT TO NEW CYLINDER GROUP FORMAT"))
			return(0);
		doinglevel1++;
		sblock->fs_old_postblformat = FS_DYNAMICPOSTBLFMT;
		sblock->fs_old_nrpos = 0;
		sbdirty();
		dirty(&asblk);
	}
	if (asblk.b_dirty && !bflag) {
		memmove((struct fs*)sblk.b_un.b_fs, sblock, SBLOCKSIZE);
		if (needswap)
			ffs_sb_swap(sblock, (struct fs*)sblk.b_un.b_fs);
		memmove(asblk.b_un.b_fs, sblk.b_un.b_fs, (size_t)sblock->fs_sbsize);
		flush(fswritefd, &asblk);
	}
	/*
	 * read in the summary info.
	 */
	asked = 0;
	sblock->fs_csp = (struct csum *)calloc(1, sblock->fs_cssize);
	for (i = 0, j = 0; i < sblock->fs_cssize; i += sblock->fs_bsize, j++) {
		size = sblock->fs_cssize - i < sblock->fs_bsize ?
		    sblock->fs_cssize - i : sblock->fs_bsize;
		ccsp = (struct csum *)((char *)sblock->fs_csp + i);
		if (bread(fsreadfd, (char *)ccsp,
		    fsbtodb(sblock, sblock->fs_csaddr + j * sblock->fs_frag),
		    size) != 0 && !asked) {
			pfatal("BAD SUMMARY INFORMATION");
			if (reply("CONTINUE") == 0) {
				markclean = 0;
				exit(EEXIT);
			}
			asked++;
		}
		if (doswap) {
			ffs_csum_swap(ccsp, ccsp, size);
			bwrite(fswritefd, (char *)ccsp,
			    fsbtodb(sblock,
				sblock->fs_csaddr + j * sblock->fs_frag),
			    size);
		}
		if (needswap)
			ffs_csum_swap(ccsp, ccsp, size);
	}
	/*
	 * allocate and initialize the necessary maps
	 */
	bmapsize = roundup(howmany(maxfsblock, NBBY), sizeof(int16_t));
	blockmap = calloc((unsigned)bmapsize, sizeof (char));
	if (blockmap == NULL) {
		printf("cannot alloc %u bytes for blockmap\n",
		    (unsigned)bmapsize);
		goto badsblabel;
	}
	inostathead = calloc((unsigned)(sblock->fs_ncg),
	    sizeof(struct inostatlist));
	if (inostathead == NULL) {
		printf("cannot alloc %u bytes for inostathead\n",
		    (unsigned)(sizeof(struct inostatlist) * (sblock->fs_ncg)));
		goto badsblabel;
	}
	/*
	 * cs_ndir may be inaccurate, particularly if we're using the -b
	 * option, so set a minimum to prevent bogus subdirectory reconnects
	 * and really inefficient directory scans.
	 * Also set a maximum in case the value is too large.
	 */
	numdirs = sblock->fs_cstotal.cs_ndir;
	if (numdirs < 1024)
		numdirs = 1024;
	if (numdirs > maxino + 1)
		numdirs = maxino + 1;
	dirhash = numdirs;
	inplast = 0;
	listmax = numdirs + 10;
	inpsort = (struct inoinfo **)calloc((unsigned)listmax,
	    sizeof(struct inoinfo *));
	inphead = (struct inoinfo **)calloc((unsigned)numdirs,
	    sizeof(struct inoinfo *));
	if (inpsort == NULL || inphead == NULL) {
		printf("cannot alloc %u bytes for inphead\n", 
		    (unsigned)(numdirs * sizeof(struct inoinfo *)));
		goto badsblabel;
	}
	cgrp = malloc(sblock->fs_cgsize);
	if (cgrp == NULL) {
		printf("cannot alloc %u bytes for cylinder group\n",
		    sblock->fs_cgsize);
		goto badsblabel;
	}
	bufinit();
	if (sblock->fs_flags & FS_DOSOFTDEP)
		usedsoftdep = 1;
	else
		usedsoftdep = 0;

	{
		struct partition *pp = 0;
		if (!forceimage && lp) 
			pp = getdisklabelpart(dev,lp);
		if (pp && (pp->p_fstype == FS_APPLEUFS)) {
			isappleufs = 1;
		}
	}
	if (readappleufs()) {
		isappleufs = 1;
	}

	dirblksiz = DIRBLKSIZ;
	if (isappleufs)
		dirblksiz = APPLEUFS_DIRBLKSIZ;

	if (debug)
		printf("isappleufs = %d, dirblksiz = %d\n", isappleufs, dirblksiz);

	return (1);

badsblabel:
	markclean=0;
	ckfini();
	return (0);
}

static int
readappleufs()
{
	daddr_t label = APPLEUFS_LABEL_OFFSET / dev_bsize;
	struct appleufslabel *appleufs;
	int i;

	/* XXX do we have to deal with APPLEUFS_LABEL_OFFSET not
	 * being block aligned (CD's?)
	 */
	if (bread(fsreadfd, (char *)appleufsblk.b_un.b_fs, label, (long)APPLEUFS_LABEL_SIZE) != 0)
		return 0;
	appleufsblk.b_bno = label;
	appleufsblk.b_size = APPLEUFS_LABEL_SIZE;

	appleufs = appleufsblk.b_un.b_appleufs;

	if (ntohl(appleufs->ul_magic) != APPLEUFS_LABEL_MAGIC) {
		if (!isappleufs) {
			return 0;
		} else {
			pfatal("MISSING APPLEUFS VOLUME LABEL\n");
			if (reply("FIX") == 0) {
				return 1;
			}
			ffs_appleufs_set(appleufs,NULL,-1);
			appleufsdirty();
		}
	}

	if (ntohl(appleufs->ul_version) != APPLEUFS_LABEL_VERSION) {
		pwarn("INCORRECT APPLE UFS VERSION NUMBER (%d should be %d)",
			ntohl(appleufs->ul_version),APPLEUFS_LABEL_VERSION);
		if (preen) {
			printf(" (CORRECTED)\n");
		}
		if (preen || reply("CORRECT")) {
			appleufs->ul_version = htonl(APPLEUFS_LABEL_VERSION);
			appleufsdirty();
		}
	}

	if (ntohs(appleufs->ul_namelen) > APPLEUFS_MAX_LABEL_NAME) {
		pwarn("APPLE UFS LABEL NAME TOO LONG");
		if (preen) {
			printf(" (TRUNCATED)\n");
		}
		if (preen || reply("TRUNCATE")) {
			appleufs->ul_namelen = htons(APPLEUFS_MAX_LABEL_NAME);
			appleufsdirty();
		}
	}

	if (ntohs(appleufs->ul_namelen) == 0) {
		pwarn("MISSING APPLE UFS LABEL NAME");
		if (preen) {
			printf(" (FIXED)\n");
		}
		if (preen || reply("FIX")) {
			ffs_appleufs_set(appleufs,NULL,-1);
			appleufsdirty();
		}
	}

	/* Scan name for first illegal character */
	for (i=0;i<ntohs(appleufs->ul_namelen);i++) {
		if ((appleufs->ul_name[i] == '\0') ||
			(appleufs->ul_name[i] == ':') ||
			(appleufs->ul_name[i] == '/')) {
			pwarn("APPLE UFS LABEL NAME CONTAINS ILLEGAL CHARACTER");
			if (preen) {
				printf(" (TRUNCATED)\n");
			}
			if (preen || reply("TRUNCATE")) {
				appleufs->ul_namelen = i+1;
				appleufsdirty();
			}
			break;
		}
	}

	/* Check the checksum last, because if anything else was wrong,
	 * then the checksum gets reset anyway.
	 */
	appleufs->ul_checksum = 0;
	appleufs->ul_checksum = ffs_appleufs_cksum(appleufs);
	if (appleufsblk.b_un.b_appleufs->ul_checksum != appleufs->ul_checksum) {
		pwarn("INVALID APPLE UFS CHECKSUM (%#04x should be %#04x)",
			appleufsblk.b_un.b_appleufs->ul_checksum, appleufs->ul_checksum);
		if (preen) {
			printf(" (CORRECTED)\n");
		}
		if (preen || reply("CORRECT")) {
			appleufsdirty();
		} else {
			/* put the incorrect checksum back in place */
			appleufs->ul_checksum = appleufsblk.b_un.b_appleufs->ul_checksum;
		}
	}
	return 1;
}

/*
 * Detect byte order. Return 0 if valid magic found, -1 otherwise.
 */
static int
detect_byteorder(struct fs *fs)
{
	if (fs->fs_magic == FS_UFS1_MAGIC || fs->fs_magic == FS_UFS2_MAGIC) {
		if (endian == 0 || BYTE_ORDER == endian) {
			needswap = 0;
			doswap = do_blkswap = do_dirswap = 0;
		} else {
			needswap = 1;
			doswap = do_blkswap = do_dirswap = 1;
		}
		return 0;
	} else if (fs->fs_magic == bswap32(FS_UFS2_MAGIC) ||
		   fs->fs_magic == bswap32(FS_UFS2_MAGIC)) {
		if (endian == 0 || BYTE_ORDER != endian) {
			needswap = 1;
			doswap = do_blkswap = do_dirswap = 0;
		} else {
			needswap = 0;
			doswap = do_blkswap = do_dirswap = 1;
		}
		return 0;
	}
	return -1;
}

/*
 * Possible superblock locations ordered from most to least likely.
 */
static off_t sblock_try[] = SBLOCKSEARCH;

/*
 * Read in the super block and its summary info.
 */
static int
readsb(listerr)
	int listerr;
{
	daddr_t super;
	struct fs *fs;
	int i;

	if (bflag) {
		super = bflag;
		if (bread(fsreadfd, (char *)sblk.b_un.b_fs, super,
		    (long)SBLOCKSIZE) != 0)
			return (0);
		fs = sblk.b_un.b_fs;
		if (detect_byteorder(fs) < 0) {
			badsb(listerr, "MAGIC NUMBER WRONG");
			return (0);
		}
	} else {
		for (i = 0; sblock_try[i] != -1; i++) {
			super = sblock_try[i] / dev_bsize;
			if (bread(fsreadfd, (char *)sblk.b_un.b_fs,
			    super, (long)SBLOCKSIZE) != 0)
				continue;
			fs = sblk.b_un.b_fs;
			if (detect_byteorder(fs) == 0)
				break;
		}
		if (sblock_try[i] == -1) {
			badsb(listerr, "CAN'T FIND SUPERBLOCK");
			return (0);
		}
	}
	if (doswap) {
		if (preen)
			errx(EEXIT, "incompatible options -B and -p");
		if (nflag)
			errx(EEXIT, "incompatible options -B and -n");
		if (endian == LITTLE_ENDIAN) {
			if (!reply("CONVERT TO LITTLE ENDIAN"))
				return 0;
		} else if (endian == BIG_ENDIAN) {
			if (!reply("CONVERT TO BIG ENDIAN"))
				return 0;
		} else
			pfatal("INTERNAL ERROR: unknown endian");
	}
	if (needswap)
		printf("** Swapped byte order\n");
	/* swap SB byte order if asked */
	if (doswap)
		ffs_sb_swap(sblk.b_un.b_fs, sblk.b_un.b_fs);

	memmove(sblock, sblk.b_un.b_fs, SBLOCKSIZE);
	if (needswap)
		ffs_sb_swap(sblk.b_un.b_fs, sblock);

	is_ufs2 = sblock->fs_magic == FS_UFS2_MAGIC;

	/*
	 * run a few consistency checks of the super block
	 */
	if (sblock->fs_sbsize > SBLOCKSIZE)
		{ badsb(listerr, "SIZE PREPOSTEROUSLY LARGE"); return (0); }
	/*
	 * Compute block size that the filesystem is based on,
	 * according to fsbtodb, and adjust superblock block number
	 * so we can tell if this is an alternate later.
	 */
	super *= dev_bsize;
	dev_bsize = sblock->fs_fsize / fsbtodb(sblock, 1);
	sblk.b_bno = super / dev_bsize;
	sblk.b_size = SBLOCKSIZE;
	if (bflag)
		goto out;
	/*
	 * Set all possible fields that could differ, then do check
	 * of whole super block against an alternate super block->
	 * When an alternate super-block is specified this check is skipped.
	 */
	getblk(&asblk, cgsblock(sblock, sblock->fs_ncg - 1), sblock->fs_sbsize);
	if (asblk.b_errs)
		return (0);
	/* swap SB byte order if asked */
	if (doswap)
		ffs_sb_swap(asblk.b_un.b_fs, asblk.b_un.b_fs);

	memmove(altsblock, asblk.b_un.b_fs, sblock->fs_sbsize);
	if (needswap)
		ffs_sb_swap(asblk.b_un.b_fs, altsblock);
	if (cmpsblks(sblock, altsblock)) {
		badsb(listerr,
		"VALUES IN SUPER BLOCK DISAGREE WITH THOSE IN FIRST ALTERNATE");
		return (0);
	}
out:
        /*
         * If not yet done, update UFS1 superblock with new wider fields.
         */
        if (sblock->fs_magic == FS_UFS1_MAGIC &&
	    sblock->fs_maxbsize != sblock->fs_bsize) {
		sblock->fs_maxbsize = sblock->fs_bsize;
		sblock->fs_time = sblock->fs_old_time;
		sblock->fs_size = sblock->fs_old_size;
		sblock->fs_dsize = sblock->fs_old_dsize;
		sblock->fs_csaddr = sblock->fs_old_csaddr;
		sblock->fs_cstotal.cs_ndir = sblock->fs_old_cstotal.cs_ndir;
		sblock->fs_cstotal.cs_nbfree = sblock->fs_old_cstotal.cs_nbfree;
		sblock->fs_cstotal.cs_nifree = sblock->fs_old_cstotal.cs_nifree;
		sblock->fs_cstotal.cs_nffree = sblock->fs_old_cstotal.cs_nffree;
	}

	/* Now we know the SB is valid, we can write it back if needed */
	if (doswap) {
		sbdirty();
		dirty(&asblk);
	}
	havesb = 1;
	return (1);
}

int
cmpsblks(const struct fs *sb, struct fs *asb)
{
        if (asb->fs_sblkno != sb->fs_sblkno ||
	    asb->fs_cblkno != sb->fs_cblkno ||
	    asb->fs_iblkno != sb->fs_iblkno ||
	    asb->fs_dblkno != sb->fs_dblkno ||
	    asb->fs_ncg != sb->fs_ncg ||
	    asb->fs_bsize != sb->fs_bsize ||
	    asb->fs_fsize != sb->fs_fsize ||
	    asb->fs_frag != sb->fs_frag ||
	    asb->fs_bmask != sb->fs_bmask ||
	    asb->fs_fmask != sb->fs_fmask ||
	    asb->fs_bshift != sb->fs_bshift ||
	    asb->fs_fshift != sb->fs_fshift ||
	    asb->fs_fragshift != sb->fs_fragshift ||
	    asb->fs_fsbtodb != sb->fs_fsbtodb ||
	    asb->fs_sbsize != sb->fs_sbsize ||
	    asb->fs_nindir != sb->fs_nindir ||
	    asb->fs_inopb != sb->fs_inopb ||
	    asb->fs_cssize != sb->fs_cssize ||
	    asb->fs_ipg != sb->fs_ipg ||
	    asb->fs_fpg != sb->fs_fpg ||
	    asb->fs_magic != sb->fs_magic)
		return 1;
	return 0;
}

static void
badsb(listerr, s)
	int listerr;
	char *s;
{

	if (!listerr)
		return;
	if (preen)
		printf("%s: ", cdevname());
	pfatal("BAD SUPER BLOCK: %s\n", s);
}

/*
 * Calculate a prototype superblock based on information in the disk label.
 * When done the cgsblock macro can be calculated and the fs_ncg field
 * can be used. Do NOT attempt to use other macros without verifying that
 * their needed information is available!
 */
static int
calcsb(dev, devfd, fs)
	const char *dev;
	int devfd;
	struct fs *fs;
{
	struct disklabel *lp;
	struct partition *pp;
	int i, nspf;

	lp = getdisklabel(dev, devfd);
	pp = getdisklabelpart(dev,lp);
	if (pp == 0) {
		pfatal("%s: CANNOT FIGURE OUT FILE SYSTEM PARTITION\n", dev);
		return (0);
	}
	if ((pp->p_fstype != FS_BSDFFS) && (pp->p_fstype != FS_APPLEUFS)) {
		pfatal("%s: NOT LABELED AS A BSD FILE SYSTEM (%s)\n",
			dev, pp->p_fstype < FSMAXTYPES ?
			fstypenames[pp->p_fstype] : "unknown");
		return (0);
	}
	/* avoid divide by 0 */
	if (pp->p_fsize == 0 || pp->p_frag == 0)
		return (0);
	memset(fs, 0, sizeof(struct fs));
	fs->fs_fsize = pp->p_fsize;
	fs->fs_frag = pp->p_frag;
	fs->fs_size = pp->p_size;
	fs->fs_sblkno = roundup(
		howmany(lp->d_bbsize + lp->d_sbsize, fs->fs_fsize),
		fs->fs_frag);
	nspf = fs->fs_fsize / lp->d_secsize;
	fs->fs_old_nspf = nspf;
	for (fs->fs_fsbtodb = 0, i = nspf; i > 1; i >>= 1)
		fs->fs_fsbtodb++;
	dev_bsize = lp->d_secsize;
	if (fs->fs_magic == FS_UFS2_MAGIC) {
		fs->fs_fpg = pp->p_cpg;
		fs->fs_ncg = howmany(fs->fs_size, fs->fs_fpg);
	} else /* if (fs->fs_magic == FS_UFS1_MAGIC) */ {
		fs->fs_old_cpg = pp->p_cpg;
		fs->fs_old_cgmask = 0xffffffff;
		for (i = lp->d_ntracks; i > 1; i >>= 1)
			fs->fs_old_cgmask <<= 1;
		if (!POWEROF2(lp->d_ntracks))
			fs->fs_old_cgmask <<= 1;
		fs->fs_old_cgoffset = roundup(
			howmany(lp->d_nsectors, nspf), fs->fs_frag);
		fs->fs_fpg = (fs->fs_old_cpg * lp->d_secpercyl) / nspf;
		fs->fs_ncg = howmany(fs->fs_size / lp->d_secpercyl,
		    fs->fs_old_cpg);
	}
	return (1);
}

static struct disklabel *
getdisklabel(s, fd)
	const char *s;
	int	fd;
{
	static struct disklabel lab;

	if (ioctl(fd, DIOCGDINFO, (char *)&lab) < 0) {
		if (s == NULL)
			return ((struct disklabel *)NULL);
		pwarn("ioctl (GCINFO): %s\n", strerror(errno));
		errx(EEXIT, "%s: can't read disk label", s);
	}
	return (&lab);
}

static struct partition *
getdisklabelpart(dev, lp)
	const char *dev;
	struct disklabel *lp;
{
	char *cp;

	cp = strchr(dev, '\0') - 1;
	if ((cp == (char *)-1 || (*cp < 'a' || *cp > 'p')) && !isdigit(*cp)) {
		return 0;
	}
	if (isdigit(*cp))
		return &lp->d_partitions[0];
	else
		return &lp->d_partitions[*cp - 'a'];
}
  
