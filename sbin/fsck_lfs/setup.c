/* $NetBSD: setup.c,v 1.5 2000/05/23 01:48:55 perseant Exp $	 */

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

/* #define DKTYPENAMES */
#define FSTYPENAMES
#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/file.h>

#include <ufs/ufs/dinode.h>
#include <sys/mount.h>		/* XXX ufs/lfs/lfs.h should include this for
				 * us */
#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fsck.h"
#include "extern.h"
#include "fsutil.h"

struct bufarea  asblk;
daddr_t        *din_table;
SEGUSE         *seg_table;
#define altsblock (*asblk.b_un.b_fs)
#define POWEROF2(num)	(((num) & ((num) - 1)) == 0)

void            badsb(int, char *);
int             calcsb(const char *, int, struct lfs *);
static struct disklabel *getdisklabel(const char *, int);
static int      readsb(int);
int             lfs_maxino(void);

static daddr_t  try_verify(struct lfs *, struct lfs *);

#ifdef DKTYPENAMES
int             useless(void);

int
useless(void)
{
	char          **foo = (char **)dktypenames;
	char          **bar = (char **)fscknames;

	return foo - bar;
}
#endif

static          daddr_t
try_verify(struct lfs * osb, struct lfs * nsb)
{
	daddr_t         daddr;
	SEGSUM         *sp;
	char            summary[LFS_SUMMARY_SIZE];
	int             bc, flag;

	daddr = osb->lfs_offset;
	while (daddr != nsb->lfs_offset) {
		flag = 0;
oncemore:
		/* Read in summary block */
		bread(fsreadfd, summary, daddr, LFS_SUMMARY_SIZE);
		sp = (SEGSUM *)summary;

		/*
		 * Could be a superblock instead of a segment summary.
		 * XXX should use gseguse, but right now we need to do more
		 * setup before we can...fix this
		 */
		if (sp->ss_magic != SS_MAGIC ||
		  sp->ss_sumsum != cksum(&sp->ss_datasum, LFS_SUMMARY_SIZE -
					 sizeof(sp->ss_sumsum))) {
			if (flag == 0) {
				daddr += LFS_SBPAD / dev_bsize;
				goto oncemore;
			}
			return 0x0;
		}
		bc = check_summary(osb, sp, daddr);
		if (bc == 0)
			break;
		daddr += (LFS_SUMMARY_SIZE + bc) / dev_bsize;
		if (datosn(osb, daddr) != datosn(osb, daddr + (LFS_SUMMARY_SIZE + (1 << osb->lfs_bshift)) / dev_bsize)) {
			daddr = ((SEGSUM *)summary)->ss_next;
		}
	}
	return daddr;
}

int
setup(const char *dev)
{
	long            bmapsize;
	struct disklabel *lp;
#if 0
	long            i;
	off_t           sizepb;
#endif
	struct stat     statb;
	daddr_t         daddr;
	struct lfs      proto;
	int             doskipclean;
	u_int64_t       maxfilesize;
	struct lfs     *sb0, *sb1, *osb, *nsb;
	struct dinode  *idinode;

	havesb = 0;
	fswritefd = -1;
	doskipclean = skipclean;
	if (stat(dev, &statb) < 0) {
		printf("Can't stat %s: %s\n", dev, strerror(errno));
		return (0);
	}
	if (!S_ISCHR(statb.st_mode)) {
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
	sblk.b_un.b_buf = malloc(LFS_SBPAD);
	asblk.b_un.b_buf = malloc(LFS_SBPAD);
	if (sblk.b_un.b_buf == NULL || asblk.b_un.b_buf == NULL)
		errexit("cannot allocate space for superblock\n");
	if ((lp = getdisklabel((char *) NULL, fsreadfd)) != NULL)
		dev_bsize = secsize = lp->d_secsize;
	else
		dev_bsize = secsize = DEV_BSIZE;

	/*
	 * Read in the superblock, looking for alternates if necessary
	 */
	if (readsb(1) == 0) {
		if (bflag || preen || calcsb(dev, fsreadfd, &proto) == 0)
			return (0);
		if (reply("LOOK FOR ALTERNATE SUPERBLOCKS") == 0)
			return (0);
#if 0				/* XXX find the LFS way to do this */
		for (cg = 0; cg < proto.lfs_ncg; cg++) {
			bflag = fsbtodb(&proto, cgsblock(&proto, cg));
			if (readsb(0) != 0)
				break;
		}
		if (cg >= proto.lfs_ncg) {
			printf("%s %s\n%s %s\n%s %s\n",
			       "SEARCH FOR ALTERNATE SUPER-BLOCK",
			       "FAILED. YOU MUST USE THE",
			       "-b OPTION TO FSCK_FFS TO SPECIFY THE",
			       "LOCATION OF AN ALTERNATE",
			       "SUPER-BLOCK TO SUPPLY NEEDED",
			       "INFORMATION; SEE fsck_ffs(8).");
			return (0);
		}
#else
		pwarn("XXX Can't look for alternate superblocks yet\n");
		return (0);
#endif
		doskipclean = 0;
		pwarn("USING ALTERNATE SUPERBLOCK AT %d\n", bflag);
	}
	bufinit();

	if (bflag == 0) {
		/*
		 * Even if that superblock read in properly, it may not
		 * be guaranteed to point to a complete checkpoint.
		 * Read in the second superblock too, and take whichever
		 * of the two is *less* recent. --ks
		 */
		sb0 = malloc(sizeof(*sb0));
		sb1 = malloc(sizeof(*sb1));
		memcpy(sb0, &sblock, sizeof(*sb0));
		bflag = sblock.lfs_sboffs[1];
		if (readsb(1) == 0) {
			pwarn("COULDN'T READ ALT SUPERBLOCK AT BLK %d", bflag);
			if (reply("ASSUME PRIMARY SUPERBLOCK IS GOOD") == 0) {
				return (0);
			} else {/* use primary as good */
				memcpy(&sblock, sb0, sizeof(*sb0));	/* XXX cheating? */
			}
		} else {
			memcpy(sb1, &sblock, sizeof(*sb1));
			if (debug)
				pwarn("sb0 %d, sb1 %d\n", sb0->lfs_tstamp, sblock.lfs_tstamp);
			/*
			 * Verify the checkpoint of the newer superblock,
			 * if the timestamp of the two superblocks is
			 * different.  XXX use lfs_offset instead, discover
			 * how to quickly discover "newness" based on that.
			 */
			if (sb0->lfs_tstamp != sb1->lfs_tstamp) {
				if (sb0->lfs_tstamp > sb1->lfs_tstamp) {
					osb = sb1;
					nsb = sb0;
				} else {
					osb = sb0;
					nsb = sb1;
				}
				daddr = try_verify(osb, nsb);

				if (debug)
					printf("done.\n");
				if (daddr == nsb->lfs_offset) {
					pwarn("Checkpoint verified, recovered %d seconds of data\n", nsb->lfs_tstamp - osb->lfs_tstamp);
					memcpy(&sblock, nsb, sizeof(*nsb));
					sbdirty();
				} else {
					pwarn("Checkpoint invalid, lost %d seconds of data\n", nsb->lfs_tstamp - osb->lfs_tstamp);
					memcpy(&sblock, osb, sizeof(*osb));
				}
			}
		}
		free(sb0);
		free(sb1);
	}
	if (debug) {
		printf("dev_bsize = %lu\n", dev_bsize);
		printf("lfs_bsize = %lu\n", (unsigned long)sblock.lfs_bsize);
		printf("lfs_fsize = %lu\n", (unsigned long)sblock.lfs_fsize);
		printf("lfs_frag  = %lu\n", (unsigned long)sblock.lfs_frag);
		printf("INOPB(fs) = %lu\n", (unsigned long) INOPB(&sblock));
		/* printf("fsbtodb(fs,1) = %lu\n",fsbtodb(&sblock,1)); */
	}
#if 0				/* FFS-specific fs-clean check */
	if (debug)
		printf("clean = %d\n", sblock.lfs_clean);
	if (sblock.lfs_clean & FS_ISCLEAN) {
		if (doskipclean) {
			pwarn("%sile system is clean; not checking\n",
			      preen ? "f" : "** F");
			return (-1);
		}
		if (!preen)
			pwarn("** File system is already clean\n");
	}
	maxino = sblock.lfs_ncg * sblock.lfs_ipg;
#else
#if 0
	/* XXX - count the number of inodes here */
	maxino = sblock.lfs_nfiles + 2;
#else
	initbarea(&iblk);
	iblk.b_un.b_buf = malloc(sblock.lfs_bsize);
	if (bread(fsreadfd, (char *)iblk.b_un.b_buf,
		  sblock.lfs_idaddr,
		  (long)sblock.lfs_bsize) != 0) {
		printf("Couldn't read disk block %d\n", sblock.lfs_idaddr);
		exit(1);
	}
	idinode = lfs_difind(&sblock, sblock.lfs_ifile, &ifblock);
	maxino = ((idinode->di_size
	    - (sblock.lfs_cleansz + sblock.lfs_segtabsz) * sblock.lfs_bsize)
		  / sblock.lfs_bsize) * sblock.lfs_ifpb - 1;
#endif
	if (debug)
		printf("maxino=%d\n", maxino);
	din_table = (daddr_t *)malloc(maxino * sizeof(*din_table));
	memset(din_table, 0, maxino * sizeof(*din_table));
	seg_table = (SEGUSE *)malloc(sblock.lfs_nseg * sizeof(SEGUSE));
	memset(seg_table, 0, sblock.lfs_nseg * sizeof(SEGUSE));
#endif
	maxfsblock = sblock.lfs_size * (sblock.lfs_bsize / dev_bsize);
#if 0
	sizepb = sblock.lfs_bsize;
	maxfilesize = sblock.lfs_bsize * NDADDR - 1;
	for (i = 0; i < NIADDR; i++) {
		sizepb *= NINDIR(&sblock);
		maxfilesize += sizepb;
	}
	maxfilesize++;		/* XXX */
#else				/* LFS way */
	{
		u_quad_t        maxtable[] = {
			 /* 1 */ -1,
			 /* 2 */ -1,
			 /* 4 */ -1,
			 /* 8 */ -1,
			 /* 16 */ -1,
			 /* 32 */ -1,
			 /* 64 */ -1,
			 /* 128 */ -1,
			 /* 256 */ -1,
			 /* 512 */ NDADDR + 128 + 128 * 128 + 128 * 128 * 128,
			 /* 1024 */ NDADDR + 256 + 256 * 256 + 256 * 256 * 256,
			 /* 2048 */ NDADDR + 512 + 512 * 512 + 512 * 512 * 512,
			 /* 4096 */ NDADDR + 1024 + 1024 * 1024 + 1024 * 1024 * 1024,
			 /* 8192 */ 1 << 31,
			 /* 16 K */ 1 << 31,
			 /* 32 K */ 1 << 31,
		};
		maxfilesize = maxtable[sblock.lfs_bshift] << sblock.lfs_bshift;
	}
#endif
	if ((sblock.lfs_minfree < 0 || sblock.lfs_minfree > 99)) {
		pfatal("IMPOSSIBLE MINFREE=%d IN SUPERBLOCK",
		       sblock.lfs_minfree);
		if (reply("SET TO DEFAULT") == 1) {
			sblock.lfs_minfree = 10;
			sbdirty();
		}
	}
	/* XXX used to be ~(sblock.lfs_bsize - 1) */
	if (sblock.lfs_bmask != sblock.lfs_bsize - 1) {
		pwarn("INCORRECT BMASK=%x IN SUPERBLOCK (should be %x)",
		      (unsigned int)sblock.lfs_bmask,
		      (unsigned int)sblock.lfs_bsize - 1);
		sblock.lfs_bmask = sblock.lfs_bsize - 1;
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("FIX") == 1) {
			sbdirty();
			dirty(&asblk);
		}
	}
#if 0				/* FFS-specific checks */
	if (sblock.lfs_fmask != ~(sblock.lfs_fsize - 1)) {
		pwarn("INCORRECT FMASK=%x IN SUPERBLOCK",
		      sblock.lfs_fmask);
		sblock.lfs_fmask = ~(sblock.lfs_fsize - 1);
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("FIX") == 1) {
			sbdirty();
			dirty(&asblk);
		}
	}
#endif
	if (sblock.lfs_maxfilesize != maxfilesize) {
		pwarn("INCORRECT MAXFILESIZE=%qu IN SUPERBLOCK (should be %qu)",
		      (unsigned long long)sblock.lfs_maxfilesize,
		      (unsigned long long)maxfilesize);
		sblock.lfs_maxfilesize = maxfilesize;
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("FIX") == 1) {
			sbdirty();
			dirty(&asblk);
		}
	}
	if (sblock.lfs_maxsymlinklen != MAXSYMLINKLEN) {
		pwarn("INCORRECT MAXSYMLINKLEN=%d IN SUPERBLOCK",
		      sblock.lfs_maxsymlinklen);
		sblock.lfs_maxsymlinklen = MAXSYMLINKLEN;
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("FIX") == 1) {
			sbdirty();
			dirty(&asblk);
		}
	}
	newinofmt = 1;
	/*
	 * allocate and initialize the necessary maps
	 */
#ifndef VERBOSE_BLOCKMAP
	bmapsize = roundup(howmany(maxfsblock, NBBY), sizeof(int16_t));
	blockmap = malloc((unsigned)bmapsize * sizeof(char));
	bzero(blockmap, bmapsize * sizeof(char));
#else
	bmapsize = maxfsblock * sizeof(ino_t);
	blockmap = (ino_t *)malloc(maxfsblock * sizeof(ino_t));
	bzero(blockmap, maxfsblock * sizeof(ino_t));
#endif
	if (blockmap == NULL) {
		printf("cannot alloc %u bytes for blockmap\n",
		       (unsigned)bmapsize);
		goto badsblabel;
	}
	statemap = calloc((unsigned)(maxino + 1), sizeof(char));
	if (statemap == NULL) {
		printf("cannot alloc %u bytes for statemap\n",
		       (unsigned)(maxino + 1));
		goto badsblabel;
	}
	typemap = calloc((unsigned)(maxino + 1), sizeof(char));
	if (typemap == NULL) {
		printf("cannot alloc %u bytes for typemap\n",
		       (unsigned)(maxino + 1));
		goto badsblabel;
	}
	lncntp = (int16_t *)calloc((unsigned)(maxino + 1), sizeof(int16_t));
	if (lncntp == NULL) {
		printf("cannot alloc %lu bytes for lncntp\n",
		       (unsigned long)(maxino + 1) * sizeof(int16_t));
		goto badsblabel;
	}
	return (1);

badsblabel:
	ckfini(0);
	return (0);
}

/*
 * Read in the LFS super block and its summary info.
 */
static int
readsb(int listerr)
{
	daddr_t         super = bflag ? bflag : LFS_LABELPAD / dev_bsize;
	u_int32_t       checksum;

	if (bread(fsreadfd, (char *) &sblock, super, (long) LFS_SBPAD) != 0)
		return (0);

	sblk.b_bno = super;
	sblk.b_size = LFS_SBPAD;
	/*
	 * run a few consistency checks of the super block
	 */
	if (sblock.lfs_magic != LFS_MAGIC) {
		badsb(listerr, "MAGIC NUMBER WRONG");
		return (0);
	}
	/* checksum */
	checksum = lfs_sb_cksum(&(sblock.lfs_dlfs));
	if (sblock.lfs_cksum != checksum) {
		printf("Superblock checksum (%lu)does not match computed checksum %lu\n",
		(unsigned long)sblock.lfs_cksum, (unsigned long) checksum);
	}
#if 0				/* XXX - replace these checks with
				 * appropriate LFS sanity checks */
	if (sblock.lfs_ncg < 1) {
		badsb(listerr, "NCG OUT OF RANGE");
		return (0);
	}
	if (sblock.lfs_cpg < 1) {
		badsb(listerr, "CPG OUT OF RANGE");
		return (0);
	}
	if (sblock.lfs_ncg * sblock.lfs_cpg < sblock.lfs_ncyl ||
	    (sblock.lfs_ncg - 1) * sblock.lfs_cpg >= sblock.lfs_ncyl) {
		badsb(listerr, "NCYL LESS THAN NCG*CPG");
		return (0);
	}
	if (sblock.lfs_sbsize > SBSIZE) {
		badsb(listerr, "SIZE PREPOSTEROUSLY LARGE");
		return (0);
	}
#endif
	/*
	 * Compute block size that the filesystem is based on,
	 * according to fsbtodb, and adjust superblock block number
	 * so we can tell if this is an alternate later.
	 */
	super *= dev_bsize;
#if 0
	dev_bsize = sblock.lfs_bsize / fsbtodb(&sblock, 1);
#endif
	sblk.b_bno = super / dev_bsize;
	if (bflag) {
		havesb = 1;
		return (1);
	}
#if 0				/* XXX - for now skip the alt. superblock
				 * test as well */
	/*
	 * Set all possible fields that could differ, then do check
	 * of whole super block against an alternate super block.
	 * When an alternate super-block is specified this check is skipped.
	 */
	getblk(&asblk, cgsblock(&sblock, sblock.lfs_ncg - 1), sblock.lfs_sbsize);
	if (asblk.b_errs)
		return (0);
	altsblock.lfs_firstfield = sblock.lfs_firstfield;
	altsblock.lfs_fscktime = sblock.lfs_fscktime;
	altsblock.lfs_time = sblock.lfs_time;
	altsblock.lfs_cstotal = sblock.lfs_cstotal;
	altsblock.lfs_cgrotor = sblock.lfs_cgrotor;
	altsblock.lfs_fmod = sblock.lfs_fmod;
	altsblock.lfs_clean = sblock.lfs_clean;
	altsblock.lfs_ronly = sblock.lfs_ronly;
	altsblock.lfs_flags = sblock.lfs_flags;
	altsblock.lfs_maxcontig = sblock.lfs_maxcontig;
	altsblock.lfs_minfree = sblock.lfs_minfree;
	altsblock.lfs_optim = sblock.lfs_optim;
	altsblock.lfs_rotdelay = sblock.lfs_rotdelay;
	altsblock.lfs_maxbpg = sblock.lfs_maxbpg;
	memcpy(altsblock.lfs_csp, sblock.lfs_csp,
	       sizeof sblock.lfs_csp);
	altsblock.lfs_maxcluster = sblock.lfs_maxcluster;
	memcpy(altsblock.lfs_fsmnt, sblock.lfs_fsmnt,
	       sizeof sblock.lfs_fsmnt);
	memcpy(altsblock.lfs_sparecon, sblock.lfs_sparecon,
	       sizeof sblock.lfs_sparecon);
	/*
	 * The following should not have to be copied.
	 */
	altsblock.lfs_fsbtodb = sblock.lfs_fsbtodb;
	altsblock.lfs_interleave = sblock.lfs_interleave;
	altsblock.lfs_npsect = sblock.lfs_npsect;
	altsblock.lfs_nrpos = sblock.lfs_nrpos;
	altsblock.lfs_state = sblock.lfs_state;
	altsblock.lfs_qbmask = sblock.lfs_qbmask;
	altsblock.lfs_qfmask = sblock.lfs_qfmask;
	altsblock.lfs_state = sblock.lfs_state;
	altsblock.lfs_maxfilesize = sblock.lfs_maxfilesize;
	if (memcmp(&sblock, &altsblock, (int)sblock.lfs_sbsize)) {
		if (debug) {
			long           *nlp, *olp, *endlp;

			printf("superblock mismatches\n");
			nlp = (long *) &altsblock;
			olp = (long *) &sblock;
			endlp = olp + (sblock.lfs_sbsize / sizeof *olp);
			for (; olp < endlp; olp++, nlp++) {
				if (*olp == *nlp)
					continue;
				printf("offset %d, original %ld, alternate %ld\n",
				       olp - (long *) &sblock, *olp, *nlp);
			}
		}
		badsb(listerr,
		      "VALUES IN SUPER BLOCK DISAGREE WITH THOSE IN FIRST ALTERNATE");
		return (0);
	}
#endif
	havesb = 1;
	return (1);
}

void
badsb(int listerr, char *s)
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
int
calcsb(const char *dev, int devfd, struct lfs * fs)
{
	register struct disklabel *lp;
	register struct partition *pp;
	register char  *cp;
	int             i;

	cp = strchr(dev, '\0') - 1;
	if ((cp == (char *) -1 || (*cp < 'a' || *cp > 'h')) && !isdigit(*cp)) {
		pfatal("%s: CANNOT FIGURE OUT FILE SYSTEM PARTITION\n", dev);
		return (0);
	}
	lp = getdisklabel(dev, devfd);
	if (lp == NULL) {
		dev_bsize = DEV_BSIZE;
	} else {
		if (isdigit(*cp))
			pp = &lp->d_partitions[0];
		else
			pp = &lp->d_partitions[*cp - 'a'];
		if (pp->p_fstype != FS_BSDLFS) {
			pfatal("%s: NOT LABELED AS AN LFS FILE SYSTEM (%s)\n",
			       dev, pp->p_fstype < FSMAXTYPES ?
			       fstypenames[pp->p_fstype] : "unknown");
			return (0);
		}
		memset(fs, 0, sizeof(struct lfs));
		fs->lfs_fsize = pp->p_fsize;
		fs->lfs_frag = pp->p_frag;
		fs->lfs_size = pp->p_size;
		fs->lfs_nspf = fs->lfs_fsize / lp->d_secsize;
		dev_bsize = lp->d_secsize;
		for (fs->lfs_fsbtodb = 0, i = fs->lfs_nspf; i > 1; i >>= 1)
			fs->lfs_fsbtodb++;
	}
	return (1);
}

static struct disklabel *
getdisklabel(const char *s, int fd)
{
	static struct disklabel lab;

	if (ioctl(fd, DIOCGDINFO, (char *) &lab) < 0) {
		if (s == NULL)
			return ((struct disklabel *) NULL);
		pwarn("ioctl (GCINFO): %s\n", strerror(errno));
#if 0
		errexit("%s: can't read disk label\n", s);
#else
		return NULL;
#endif
	}
	return (&lab);
}
