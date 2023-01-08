/*	$NetBSD: setup.c,v 1.106 2023/01/08 05:25:24 chs Exp $	*/

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
static char sccsid[] = "@(#)setup.c	8.10 (Berkeley) 5/9/95";
#else
__RCSID("$NetBSD: setup.c,v 1.106 2023/01/08 05:25:24 chs Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/disk.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/ufs_bswap.h>
#include <ufs/ufs/quota2.h>
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
#include "partutil.h"
#include "exitvalues.h"

#define POWEROF2(num)	(((num) & ((num) - 1)) == 0)

static void badsb(int, const char *);
static int calcsb(const char *, int, struct fs *);
static int readsb(int);
#ifndef NO_APPLE_UFS
static int readappleufs(void);
#endif
static int check_snapinum(void);

int16_t sblkpostbl[256];

/*
 * Read in a superblock finding an alternate if necessary.
 * Return 1 if successful, 0 if unsuccessful, -1 if filesystem
 * is already clean (preen mode only).
 */
int
setup(const char *dev, const char *origdev)
{
	uint32_t cg;
	long size, asked, i, j;
	long bmapsize;
	struct disk_geom geo;
	struct dkwedge_info dkw;
	off_t sizepb;
	struct stat statb;
	struct fs proto;
	int doskipclean;
	u_int64_t maxfilesize;
	struct csum *ccsp;
	int fd;

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
	if (nflag || (fswritefd = open(dev, O_WRONLY)) < 0) {
		fswritefd = -1;
		if (preen)
			pfatal("NO WRITE ACCESS");
		printf("** %s (NO WRITE)\n", dev);
		quiet = 0;
	} else
		if (!preen && !quiet)
			printf("** %s\n", dev);
	fsmodified = 0;
	lfdir = 0;
	initbarea(&sblk);
	initbarea(&asblk);
	sblk.b_un.b_buf = aligned_alloc(DEV_BSIZE, SBLOCKSIZE);
	sblock = aligned_alloc(DEV_BSIZE, SBLOCKSIZE);
	asblk.b_un.b_buf = aligned_alloc(DEV_BSIZE, SBLOCKSIZE);
	altsblock = aligned_alloc(DEV_BSIZE, SBLOCKSIZE);
	if (sblk.b_un.b_buf == NULL || asblk.b_un.b_buf == NULL ||
		sblock == NULL || altsblock == NULL)
		errexit("Cannot allocate space for superblock");
	if (strcmp(dev, origdev) && !forceimage) {
		/*
		 * dev isn't the original fs (for example it's a snapshot)
		 * do getdiskinfo on the original device
		 */
		 fd = open(origdev, O_RDONLY);
		 if (fd < 0) {
			warn("Can't open %s", origdev);
			return (0);
		}
	} else {
		fd = fsreadfd;
	}
	if (!forceimage && getdiskinfo(origdev, fd, NULL, &geo, &dkw) != -1)
		dev_bsize = secsize = geo.dg_secsize;
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
			bflag = FFS_FSBTODB(&proto, cgsblock(&proto, cg));
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

	if (!quota2_check_doquota())
		doskipclean = 0;
		
	/* ffs_superblock_layout() == 2 */
	if (sblock->fs_magic != FS_UFS1_MAGIC ||
	    (sblock->fs_old_flags & FS_FLAGS_UPDATED) != 0) {
		/* can have WAPBL */
		if (check_wapbl() != 0) {
			doskipclean = 0;
		}
		if (sblock->fs_flags & FS_DOWAPBL) {
			if (preen && doskipclean) {
				if (!quiet)
					pwarn("file system is journaled; "
					    "not checking\n");
				return (-1);
			}
			if (!quiet)
				pwarn("** File system is journaled; "
				    "replaying journal\n");
			replay_wapbl();
			doskipclean = 0;
			sblock->fs_flags &= ~FS_DOWAPBL;
			sbdirty();
			/* Although we may have updated the superblock from
			 * the journal, we are still going to do a full check,
			 * so we don't bother to re-read the superblock from
			 * the journal.
			 * XXX, instead we could re-read the superblock and
			 * then not force doskipclean = 0 
			 */
		}
	}
	if (debug)
		printf("clean = %d\n", sblock->fs_clean);

	if (doswap)
		doskipclean = 0;

	if (sblock->fs_clean & FS_ISCLEAN) {
		if (doskipclean) {
			if (!quiet)
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
	maxfilesize = sblock->fs_bsize * UFS_NDADDR - 1;
	for (i = 0; i < UFS_NIADDR; i++) {
		sizepb *= FFS_NINDIR(sblock);
		maxfilesize += sizepb;
	}
	if ((!is_ufs2 && cvtlevel >= 4) &&
			(sblock->fs_old_flags & FS_FLAGS_UPDATED) == 0) {
		if (preen)
			pwarn("CONVERTING TO NEW SUPERBLOCK LAYOUT\n");
		else if (!reply("CONVERT TO NEW SUPERBLOCK LAYOUT"))
			return(0);
		sblock->fs_old_flags |= FS_FLAGS_UPDATED;
		/* Disable the postbl tables */
		sblock->fs_old_cpc = 0;
		sblock->fs_old_nrpos = 1;
		sblock->fs_old_trackskew = 0;
		/* The other fields have already been updated by
		 * sb_oldfscompat_read
		 */
		sbdirty();
	}
	if (!is_ufs2 && cvtlevel == 3 &&
	    (sblock->fs_old_flags & FS_FLAGS_UPDATED)) {
		if (preen)
			pwarn("DOWNGRADING TO OLD SUPERBLOCK LAYOUT\n");
		else if (!reply("DOWNGRADE TO OLD SUPERBLOCK LAYOUT"))
			return(0);
		sblock->fs_old_flags &= ~FS_FLAGS_UPDATED;
		sb_oldfscompat_write(sblock, sblock);
		sblock->fs_old_flags &= ~FS_FLAGS_UPDATED; /* just in case */
		/* Leave postbl tables disabled, but blank its superblock region anyway */
		sblock->fs_old_postblformat = FS_DYNAMICPOSTBLFMT;
		sblock->fs_old_cpc = 0;
		sblock->fs_old_nrpos = 1;
		sblock->fs_old_trackskew = 0;
		memset(&sblock->fs_old_postbl_start, 0xff, 256);
		sb_oldfscompat_read(sblock, &sblocksave);
		sbdirty();
	}
	/*
	 * Check and potentially fix certain fields in the super block.
	 */
	if (sblock->fs_flags & ~(FS_KNOWN_FLAGS)) {
		pfatal("UNKNOWN FLAGS=0x%08x IN SUPERBLOCK", sblock->fs_flags);
		if (reply("CLEAR") == 1) {
			sblock->fs_flags &= FS_KNOWN_FLAGS;
			sbdirty();
		}
	}
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
	if (check_snapinum()) {
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("FIX") == 1) {
			sbdirty();
			dirty(&asblk);
		}
	}
	if (is_ufs2 || sblock->fs_old_inodefmt >= FS_44INODEFMT) {
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
		if ((is_ufs2 && sblock->fs_maxsymlinklen != UFS2_MAXSYMLINKLEN)
		    ||
		   (!is_ufs2 && sblock->fs_maxsymlinklen != UFS1_MAXSYMLINKLEN))
		    {
			pwarn("INCORRECT MAXSYMLINKLEN=%d IN SUPERBLOCK",
				sblock->fs_maxsymlinklen);
			sblock->fs_maxsymlinklen = is_ufs2 ?
			    UFS2_MAXSYMLINKLEN : UFS1_MAXSYMLINKLEN;
			if (preen)
				printf(" (FIXED)\n");
			if (preen || reply("FIX") == 1) {
				sbdirty();
				dirty(&asblk);
			}
		}
		if (sblock->fs_qbmask != ~sblock->fs_bmask) {
			pwarn("INCORRECT QBMASK=%#llx IN SUPERBLOCK",
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
			pwarn("INCORRECT QFMASK=%#llx IN SUPERBLOCK",
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
		sblock->fs_maxsymlinklen = UFS1_MAXSYMLINKLEN;
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
		sblock->fs_old_nrpos = 8;
		sblock->fs_old_postbloff =
		    (char *)(&sblock->fs_old_postbl_start) -
		    (char *)(&sblock->fs_firstfield);
		sblock->fs_old_rotbloff =
				(char *)(&sblock->fs_magic+1) -
				(char *)(&sblock->fs_firstfield);
		sblock->fs_cgsize =
			ffs_fragroundup(sblock, CGSIZE(sblock));
		sbdirty();
		dirty(&asblk);
	}
	if (asblk.b_dirty && !bflag) {
		memmove(sblk.b_un.b_fs, sblock, SBLOCKSIZE);
		sb_oldfscompat_write(sblk.b_un.b_fs, sblocksave);
		if (needswap)
			ffs_sb_swap(sblk.b_un.b_fs, sblk.b_un.b_fs);
		memmove(asblk.b_un.b_fs, sblk.b_un.b_fs, (size_t)sblock->fs_sbsize);
		flush(fswritefd, &asblk);
	}
	/*
	 * read in the summary info.
	 */
	asked = 0;
	sblock->fs_csp = (struct csum *)aligned_alloc(DEV_BSIZE,
	    sblock->fs_cssize);
	if (sblock->fs_csp == NULL) {
		pwarn("cannot alloc %u bytes for summary info\n",
		    sblock->fs_cssize);	
		goto badsblabel;
	}
	memset(sblock->fs_csp, 0, sblock->fs_cssize);
	for (i = 0, j = 0; i < sblock->fs_cssize; i += sblock->fs_bsize, j++) {
		size = sblock->fs_cssize - i < sblock->fs_bsize ?
		    sblock->fs_cssize - i : sblock->fs_bsize;
		ccsp = (struct csum *)((char *)sblock->fs_csp + i);
		if (bread(fsreadfd, (char *)ccsp,
		    FFS_FSBTODB(sblock, sblock->fs_csaddr + j * sblock->fs_frag),
		    size) != 0 && !asked) {
			pfatal("BAD SUMMARY INFORMATION");
			if (reply("CONTINUE") == 0) {
				markclean = 0;
				exit(FSCK_EXIT_CHECK_FAILED);
			}
			asked++;
		}
		if (doswap) {
			ffs_csum_swap(ccsp, ccsp, size);
			bwrite(fswritefd, (char *)ccsp,
			    FFS_FSBTODB(sblock,
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
	blockmap = aligned_alloc(DEV_BSIZE, (unsigned)bmapsize);
	if (blockmap == NULL) {
		pwarn("cannot alloc %u bytes for blockmap\n",
		    (unsigned)bmapsize);
		goto badsblabel;
	}
	memset(blockmap, 0, bmapsize);
	inostathead = calloc((unsigned)(sblock->fs_ncg),
	    sizeof(struct inostatlist));
	if (inostathead == NULL) {
		pwarn("cannot alloc %u bytes for inostathead\n",
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
	if ((ino_t)numdirs > maxino + 1)
		numdirs = maxino + 1;
	dirhash = numdirs;
	inplast = 0;
	listmax = numdirs + 10;
	inpsort = calloc((unsigned)listmax, sizeof(*inpsort));
	inphead = calloc((unsigned)numdirs, sizeof(*inphead));
	if (inpsort == NULL || inphead == NULL) {
		pwarn("cannot alloc %u bytes for inphead\n", 
		    (unsigned)(numdirs * sizeof(struct inoinfo *)));
		goto badsblabel;
	}
	cgrp = aligned_alloc(DEV_BSIZE, sblock->fs_cgsize);
	if (cgrp == NULL) {
		pwarn("cannot alloc %u bytes for cylinder group\n",
		    sblock->fs_cgsize);
		goto badsblabel;
	}
	bufinit();
	if (sblock->fs_flags & FS_DOSOFTDEP)
		usedsoftdep = 1;
	else
		usedsoftdep = 0;

#ifndef NO_APPLE_UFS
	if (!forceimage && dkw.dkw_parent[0]) 
		if (strcmp(dkw.dkw_ptype, DKW_PTYPE_APPLEUFS) == 0)
			isappleufs = 1;

	if (readappleufs())
		isappleufs = 1;
#endif

	if (isappleufs)
		dirblksiz = APPLEUFS_DIRBLKSIZ;
	else
		dirblksiz = UFS_DIRBLKSIZ;

	if (debug)
#ifndef NO_APPLE_UFS
		printf("isappleufs = %d, dirblksiz = %d\n", isappleufs, dirblksiz);
#else
		printf("dirblksiz = %d\n", dirblksiz);
#endif

	if (sblock->fs_flags & FS_DOQUOTA2) {
		/* allocate the quota hash table */
		/*
		 * first compute the size of the hash table
		 * We know the smallest block size is 4k, so we can use 2k
		 * for the hash table; as an entry is 8 bytes we can store
		 * 256 entries. So let start q2h_hash_shift at 8
		 */
		for (q2h_hash_shift = 8;
		    q2h_hash_shift < 15;
		    q2h_hash_shift++) {
			if ((sizeof(uint64_t) << (q2h_hash_shift + 1)) +
			    sizeof(struct quota2_header) >
			    (size_t)sblock->fs_bsize)
				break;
		}
		q2h_hash_mask = (1 << q2h_hash_shift) - 1;
		if (debug) {
			printf("quota hash shift %d, %d entries, mask 0x%x\n",
			    q2h_hash_shift, (1 << q2h_hash_shift),
			    q2h_hash_mask);
		}
		uquot_user_hash =
		    calloc((1 << q2h_hash_shift), sizeof(struct uquot_hash));
		uquot_group_hash =
		    calloc((1 << q2h_hash_shift), sizeof(struct uquot_hash));
		if (uquot_user_hash == NULL || uquot_group_hash == NULL)
			errexit("Cannot allocate space for quotas hash\n");
	} else {
		uquot_user_hash = uquot_group_hash = NULL;
		q2h_hash_shift = q2h_hash_mask = 0;
	}
	return (1);
badsblabel:
	markclean=0;
	ckfini(1);
	return (0);
}

#ifndef NO_APPLE_UFS
static int
readappleufs(void)
{
	daddr_t label = APPLEUFS_LABEL_OFFSET / dev_bsize;
	struct appleufslabel *appleufs;
	int i;

	/* XXX do we have to deal with APPLEUFS_LABEL_OFFSET not
	 * being block aligned (CD's?)
	 */
	if (APPLEUFS_LABEL_SIZE % dev_bsize != 0)
		return 0;
	if (bread(fsreadfd, (char *)appleufsblk.b_un.b_fs, label,
	    (long)APPLEUFS_LABEL_SIZE) != 0)
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
			ffs_appleufs_set(appleufs, NULL, -1, 0);
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
			ffs_appleufs_set(appleufs, NULL, -1, 0);
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
#endif /* !NO_APPLE_UFS */

/*
 * Detect byte order. Return 0 if valid magic found, -1 otherwise.
 */
static int
detect_byteorder(struct fs *fs, int sblockoff)
{
	if (sblockoff == SBLOCK_UFS2 && (fs->fs_magic == FS_UFS1_MAGIC ||
	    fs->fs_magic == FS_UFS1_MAGIC_SWAPPED))
		/* Likely to be the first alternate of a fs with 64k blocks */
		return -1;
	if (fs->fs_magic == FS_UFS1_MAGIC || fs->fs_magic == FS_UFS2_MAGIC ||
	    fs->fs_magic == FS_UFS2EA_MAGIC) {
#ifndef NO_FFS_EI
		if (endian == 0 || BYTE_ORDER == endian) {
			needswap = 0;
			doswap = do_blkswap = do_dirswap = 0;
		} else {
			needswap = 1;
			doswap = do_blkswap = do_dirswap = 1;
		}
#endif
		return 0;
	}
#ifndef NO_FFS_EI
	else if (fs->fs_magic == FS_UFS1_MAGIC_SWAPPED ||
		 fs->fs_magic == FS_UFS2_MAGIC_SWAPPED ||
		 fs->fs_magic == FS_UFS2EA_MAGIC_SWAPPED) {
		if (endian == 0 || BYTE_ORDER != endian) {
			needswap = 1;
			doswap = do_blkswap = do_dirswap = 0;
		} else {
			needswap = 0;
			doswap = do_blkswap = do_dirswap = 1;
		}
		return 0;
	}
#endif
	return -1;
}

/* Update on-disk fs->fs_magic if we are converting */
void
cvt_magic(struct fs *fs)
{

	if (is_ufs2ea || doing2ea) {
		if (fs->fs_magic == FS_UFS2_MAGIC) {
			fs->fs_magic = FS_UFS2EA_MAGIC;
		}
		if (fs->fs_magic == FS_UFS2_MAGIC_SWAPPED) {
			fs->fs_magic = FS_UFS2EA_MAGIC_SWAPPED;
		}
	}
	if (doing2noea) {
		if (fs->fs_magic == FS_UFS2EA_MAGIC) {
			fs->fs_magic = FS_UFS2_MAGIC;
		}
		if (fs->fs_magic == FS_UFS2EA_MAGIC_SWAPPED) {
			fs->fs_magic = FS_UFS2_MAGIC_SWAPPED;
		}
	}
}

/*
 * Possible superblock locations ordered from most to least likely.
 */
static off_t sblock_try[] = SBLOCKSEARCH;

/*
 * Read in the super block and its summary info.
 */
static int
readsb(int listerr)
{
	daddr_t super = 0;
	struct fs *fs;
	int i;

	if (bflag) {
		super = bflag;
		if (bread(fsreadfd, (char *)sblk.b_un.b_fs, super,
		    (long)SBLOCKSIZE) != 0)
			return (0);
		fs = sblk.b_un.b_fs;
		if (detect_byteorder(fs, -1) < 0) {
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
			if (detect_byteorder(fs, sblock_try[i]) == 0)
				break;
		}
		if (sblock_try[i] == -1) {
			badsb(listerr, "CAN'T FIND SUPERBLOCK");
			return (0);
		}
	}
	if (doswap) {
		if (preen)
			errx(FSCK_EXIT_USAGE,
			    "Incompatible options -B and -p");
		if (nflag)
			errx(FSCK_EXIT_USAGE,
			    "Incompatible options -B and -n");
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
		pwarn("** Swapped byte order\n");
	/* swap SB byte order if asked */
	if (doswap)
		ffs_sb_swap(sblk.b_un.b_fs, sblk.b_un.b_fs);

	memmove(sblock, sblk.b_un.b_fs, SBLOCKSIZE);
	if (needswap)
		ffs_sb_swap(sblk.b_un.b_fs, sblock);
	if (sblock->fs_magic == FS_UFS2EA_MAGIC) {
		is_ufs2ea = 1;
		sblock->fs_magic = FS_UFS2_MAGIC;
	}
	is_ufs2 = sblock->fs_magic == FS_UFS2_MAGIC;

	/* change on-disk magic if asked */
	cvt_magic(fs);

	/*
	 * run a few consistency checks of the super block
	 */
	if (sblock->fs_sbsize > SBLOCKSIZE)
		{ badsb(listerr, "SIZE PREPOSTEROUSLY LARGE"); return (0); }
	/*
	 * Compute block size that the filesystem is based on,
	 * according to FFS_FSBTODB, and adjust superblock block number
	 * so we can tell if this is an alternate later.
	 */
	super *= dev_bsize;
	dev_bsize = sblock->fs_fsize / FFS_FSBTODB(sblock, 1);
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
	if (altsblock->fs_magic == FS_UFS2EA_MAGIC) {
		altsblock->fs_magic = FS_UFS2_MAGIC;
	}
	/* change on-disk magic if asked */
	cvt_magic(asblk.b_un.b_fs);
	if (cmpsblks(sblock, altsblock)) {
		if (debug) {
			uint32_t *nlp, *olp, *endlp;

			printf("superblock mismatches\n");
			nlp = (uint32_t *)altsblock;
			olp = (uint32_t *)sblock;
			endlp = olp + (sblock->fs_sbsize / sizeof *olp);
			for ( ; olp < endlp; olp++, nlp++) {
				if (*olp == *nlp)
					continue;
				printf("offset %#x, original 0x%08x, alternate "
				       "0x%08x\n",
				    (int)((uint8_t *)olp-(uint8_t *)sblock),
				    *olp, *nlp);
			}
		}
		badsb(listerr,
		"VALUES IN SUPER BLOCK DISAGREE WITH THOSE IN FIRST ALTERNATE");
/*
		return (0);
*/
	}
out:

	sb_oldfscompat_read(sblock, &sblocksave);

	/* Now we know the SB is valid, we can write it back if needed */
	if (doswap || doing2ea || doing2noea) {
		sbdirty();
		dirty(&asblk);
	}
	havesb = 1;
	return (1);
}

int
cmpsblks(const struct fs *sb, struct fs *asb)
{
	if (!is_ufs2 && ((sb->fs_old_flags & FS_FLAGS_UPDATED) == 0)) {
		if (sb->fs_old_postblformat < FS_DYNAMICPOSTBLFMT)
			return cmpsblks42(sb, asb);
		else
			return cmpsblks44(sb, asb);
	}
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

/* BSD 4.2 performed the following superblock comparison
 * It should correspond to FS_42POSTBLFMT
 * (although note that in 4.2, the fs_old_postblformat
 * field didn't exist and the corresponding bits are
 * located near the end of the postbl itself, where they
 * are not likely to be used.)
 */
int
cmpsblks42(const struct fs *sb, struct fs *asb)
{
	asb->fs_firstfield = sb->fs_firstfield; /* fs_link */
	asb->fs_unused_1 = sb->fs_unused_1; /* fs_rlink */
	asb->fs_old_time = sb->fs_old_time; /* fs_time */
	asb->fs_old_cstotal = sb->fs_old_cstotal; /* fs_cstotal */
	asb->fs_cgrotor = sb->fs_cgrotor;
	asb->fs_fmod = sb->fs_fmod;
	asb->fs_clean = sb->fs_clean;
	asb->fs_ronly = sb->fs_ronly;
	asb->fs_old_flags = sb->fs_old_flags;
	asb->fs_maxcontig = sb->fs_maxcontig;
	asb->fs_minfree = sb->fs_minfree;
	asb->fs_old_rotdelay = sb->fs_old_rotdelay;
	asb->fs_maxbpg = sb->fs_maxbpg;

	/* The former fs_csp, totaling 128 bytes  */
	memmove(asb->fs_ocsp, sb->fs_ocsp, sizeof sb->fs_ocsp);
	asb->fs_contigdirs = sb->fs_contigdirs;
	asb->fs_csp = sb->fs_csp;
	asb->fs_maxcluster = sb->fs_maxcluster;
	asb->fs_active = sb->fs_active;

	/* The former fs_fsmnt, totaling 512 bytes */
	memmove(asb->fs_fsmnt, sb->fs_fsmnt, sizeof sb->fs_fsmnt);
	memmove(asb->fs_volname, sb->fs_volname, sizeof sb->fs_volname);

	return memcmp(sb, asb, sb->fs_sbsize);
}

/* BSD 4.4 performed the following superblock comparison
 * This was used in NetBSD through 1.6.1
 *
 * Note that this implementation is destructive to asb.
 */
int
cmpsblks44(const struct fs *sb, struct fs *asb)
{
	/*
	 * "Copy fields which we don't care if they're different in the
	 * alternate superblocks, as they're either likely to be
	 * different because they're per-cylinder-group specific, or
	 * because they're transient details which are only maintained
	 * in the primary superblock."
	 */
	asb->fs_firstfield = sb->fs_firstfield;
	asb->fs_unused_1 = sb->fs_unused_1;
	asb->fs_old_time = sb->fs_old_time;
	asb->fs_old_cstotal = sb->fs_old_cstotal;
	asb->fs_cgrotor = sb->fs_cgrotor;
	asb->fs_fmod = sb->fs_fmod;
	asb->fs_clean = sb->fs_clean;
	asb->fs_ronly = sb->fs_ronly;
	asb->fs_old_flags = sb->fs_old_flags;
	asb->fs_maxcontig = sb->fs_maxcontig;
	asb->fs_minfree = sb->fs_minfree;
	asb->fs_optim = sb->fs_optim;
	asb->fs_old_rotdelay = sb->fs_old_rotdelay;
	asb->fs_maxbpg = sb->fs_maxbpg;

	/* The former fs_csp and fs_maxcluster, totaling 128 bytes */
	memmove(asb->fs_ocsp, sb->fs_ocsp, sizeof sb->fs_ocsp);
	asb->fs_contigdirs = sb->fs_contigdirs;
	asb->fs_csp = sb->fs_csp;
	asb->fs_maxcluster = sb->fs_maxcluster;
	asb->fs_active = sb->fs_active;

	/* The former fs_fsmnt, totaling 512 bytes */
	memmove(asb->fs_fsmnt, sb->fs_fsmnt, sizeof sb->fs_fsmnt);
	memmove(asb->fs_volname, sb->fs_volname, sizeof sb->fs_volname);

	/* The former fs_sparecon, totaling 200 bytes */
	memmove(asb->fs_snapinum,
		sb->fs_snapinum, sizeof sb->fs_snapinum);
	asb->fs_avgfilesize = sb->fs_avgfilesize;
	asb->fs_avgfpdir = sb->fs_avgfpdir;
	asb->fs_save_cgsize = sb->fs_save_cgsize;
	memmove(asb->fs_sparecon32,
		sb->fs_sparecon32, sizeof sb->fs_sparecon32);
	asb->fs_flags = sb->fs_flags;

	/* Original comment:
	 * "The following should not have to be copied, but need to be."
	 */
	asb->fs_fsbtodb = sb->fs_fsbtodb;
	asb->fs_old_interleave = sb->fs_old_interleave;
	asb->fs_old_npsect = sb->fs_old_npsect;
	asb->fs_old_nrpos = sb->fs_old_nrpos;
	asb->fs_state = sb->fs_state;
	asb->fs_qbmask = sb->fs_qbmask;
	asb->fs_qfmask = sb->fs_qfmask;
	asb->fs_state = sb->fs_state;
	asb->fs_maxfilesize = sb->fs_maxfilesize;

	/*
	 * "Compare the superblocks, effectively checking every other
	 * field to see if they differ."
	 */
	return memcmp(sb, asb, sb->fs_sbsize);
}


static void
badsb(int listerr, const char *s)
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
calcsb(const char *dev, int devfd, struct fs *fs)
{
	struct dkwedge_info dkw;
	struct disk_geom geo;
	int i, nspf;

	if (getdiskinfo(dev, fsreadfd, NULL, &geo, &dkw) == -1)
		pfatal("%s: CANNOT FIGURE OUT FILE SYSTEM PARTITION\n", dev);
	if (dkw.dkw_parent[0] == '\0') {
		pfatal("%s: CANNOT FIGURE OUT FILE SYSTEM PARTITION\n", dev);
		return (0);
	}
	if (strcmp(dkw.dkw_ptype, DKW_PTYPE_FFS)
#ifndef NO_APPLE_UFS
	 && strcmp(dkw.dkw_ptype, DKW_PTYPE_APPLEUFS)
#endif
	) {
		pfatal("%s: NOT LABELED AS A BSD FILE SYSTEM (%s)\n",
		    dev, dkw.dkw_ptype);
		return (0);
	}
	if (geo.dg_secsize == 0) {
		pfatal("%s: CANNOT FIGURE OUT SECTOR SIZE\n", dev);
		return 0;
	}
	if (geo.dg_secpercyl == 0) {
		pfatal("%s: CANNOT FIGURE OUT SECTORS PER CYLINDER\n", dev);
		return 0;
	}
	if (sblk.b_un.b_fs->fs_fsize == 0) {
		pfatal("%s: CANNOT FIGURE OUT FRAG BLOCK SIZE\n", dev);
		return 0;
	}
	if (sblk.b_un.b_fs->fs_fpg == 0) {
		pfatal("%s: CANNOT FIGURE OUT FRAGS PER GROUP\n", dev);
		return 0;
	}
	if (sblk.b_un.b_fs->fs_old_cpg == 0) {
		pfatal("%s: CANNOT FIGURE OUT OLD CYLINDERS PER GROUP\n", dev);
		return 0;
	}
	memcpy(fs, sblk.b_un.b_fs, sizeof(struct fs));
	nspf = fs->fs_fsize / geo.dg_secsize;
	fs->fs_old_nspf = nspf;
	for (fs->fs_fsbtodb = 0, i = nspf; i > 1; i >>= 1)
		fs->fs_fsbtodb++;
	dev_bsize = geo.dg_secsize;
	if (fs->fs_magic == FS_UFS2_MAGIC) {
		fs->fs_ncg = howmany(fs->fs_size, fs->fs_fpg);
	} else /* if (fs->fs_magic == FS_UFS1_MAGIC) */ {
		fs->fs_old_cgmask = 0xffffffff;
		for (i = geo.dg_ntracks; i > 1; i >>= 1)
			fs->fs_old_cgmask <<= 1;
		if (!POWEROF2(geo.dg_ntracks))
			fs->fs_old_cgmask <<= 1;
		fs->fs_old_cgoffset = roundup(
			howmany(geo.dg_nsectors, nspf), fs->fs_frag);
		fs->fs_fpg = (fs->fs_old_cpg * geo.dg_secpercyl) / nspf;
		fs->fs_ncg = howmany(fs->fs_size / geo.dg_secpercyl,
		    fs->fs_old_cpg);
	}
	return (1);
}

/*
 * Test the list of snapshot inode numbers for duplicates and repair.
 */
static int
check_snapinum(void)
{
	int loc, loc2, res;
	uint32_t *snapinum = &sblock->fs_snapinum[0];

	res = 0;
 
	if (isappleufs)
		return 0;

	for (loc = 0; loc < FSMAXSNAP; loc++) {
		if (snapinum[loc] == 0)
			break;
		for (loc2 = loc + 1; loc2 < FSMAXSNAP; loc2++) {
			if (snapinum[loc2] == 0 ||
			    snapinum[loc2] == snapinum[loc])
				break;
		}
		if (loc2 >= FSMAXSNAP || snapinum[loc2] == 0)
			continue;
		pwarn("SNAPSHOT INODE %u ALREADY ON LIST%s", snapinum[loc2],
		    (res ? "" : "\n"));
		res = 1;
		for (loc2 = loc + 1; loc2 < FSMAXSNAP; loc2++) {
			if (snapinum[loc2] == 0)
				break;
			snapinum[loc2 - 1] = snapinum[loc2];
		}
		snapinum[loc2 - 1] = 0;
		loc--;
	}

	return res;
}
