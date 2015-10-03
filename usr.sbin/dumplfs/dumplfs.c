/*	$NetBSD: dumplfs.c,v 1.58 2015/10/03 08:28:46 dholland Exp $	*/

/*-
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
__COPYRIGHT("@(#) Copyright (c) 1991, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)dumplfs.c	8.5 (Berkeley) 5/24/95";
#else
__RCSID("$NetBSD: dumplfs.c,v 1.58 2015/10/03 08:28:46 dholland Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#include <sys/time.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_accessors.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fstab.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "extern.h"

static void	addseg(char *);
static void	dump_cleaner_info(struct lfs *, void *);
static void	dump_dinode(struct lfs *, union lfs_dinode *);
static void	dump_ifile(int, struct lfs *, int, int, daddr_t);
static int	dump_ipage_ifile(struct lfs *, int, char *, int);
static int	dump_ipage_segusage(struct lfs *, int, char *, int);
static void	dump_segment(int, int, daddr_t, struct lfs *, int);
static int	dump_sum(int, struct lfs *, SEGSUM *, int, daddr_t);
static void	dump_super(struct lfs *);
static void	usage(void);

extern uint32_t	cksum(void *, size_t);

typedef struct seglist SEGLIST;
struct seglist {
        SEGLIST *next;
	int num;
};
SEGLIST	*seglist;

char *special;

/* Segment Usage formats */
#define print_suheader \
	(void)printf("segnum\tflags\tnbytes\tninos\tnsums\tlastmod\n")

static inline void
print_suentry(int i, SEGUSE *sp, struct lfs *fs)
{
	time_t t;
	char flags[4] = "   ";

	if (sp->su_flags & SEGUSE_ACTIVE)
		flags[0] = 'A';
	if (sp->su_flags & SEGUSE_DIRTY)
		flags[1] = 'D';
	else
		flags[1] = 'C';
	if (sp->su_flags & SEGUSE_SUPERBLOCK)
		flags[2] = 'S';

	t = (lfs_sb_getversion(fs) == 1 ? sp->su_olastmod : sp->su_lastmod);

	printf("%d\t%s\t%d\t%d\t%d\t%s", i, flags,
		sp->su_nbytes, sp->su_ninos, sp->su_nsums,
		ctime(&t));
}

/* Ifile formats */
#define print_iheader \
	(void)printf("inum\tstatus\tversion\tdaddr\t\tfreeptr\n")

static inline void
print_ientry(int i, struct lfs *lfsp, IFILE *ip)
{
	uint32_t version;
	daddr_t daddr;
	ino_t nextfree;

	version = lfs_if_getversion(lfsp, ip);
	daddr = lfs_if_getdaddr(lfsp, ip);
	nextfree = lfs_if_getnextfree(lfsp, ip);

	if (daddr == LFS_UNUSED_DADDR)
		printf("%d\tFREE\t%u\t \t\t%ju\n", i, version,
		    (uintmax_t)nextfree);
	else
		printf("%d\tINUSE\t%u\t%8jX\t%s\n",
		    i, version, (intmax_t)daddr,
		    nextfree == LFS_ORPHAN_NEXTFREE ? "FFFFFFFF" : "-");
}

#define fsbtobyte(fs, b)	lfs_fsbtob((fs), (off_t)((b)))

int datasum_check = 0;

int
main(int argc, char **argv)
{
	struct lfs lfs_sb1, lfs_sb2, *lfs_master;
	daddr_t seg_addr, idaddr, sbdaddr;
	int ch, do_allsb, do_ientries, do_segentries, fd, segnum;
	void *sbuf;

	do_allsb = 0;
	do_ientries = 0;
	do_segentries = 0;
	idaddr = 0x0;
	sbdaddr = 0x0;
	while ((ch = getopt(argc, argv, "ab:diI:Ss:")) != -1)
		switch(ch) {
		case 'a':		/* Dump all superblocks */
			do_allsb = 1;
			break;
		case 'b':		/* Use this superblock */
			sbdaddr = strtol(optarg, NULL, 0);
			break;
		case 'd':
			datasum_check = 1;
			break;
		case 'i':		/* Dump ifile entries */
			do_ientries = !do_ientries;
			break;
		case 'I':		/* Use this ifile inode */
			idaddr = strtol(optarg, NULL, 0);
			break;
		case 'S':
			do_segentries = !do_segentries;
			break;
		case 's':		/* Dump out these segments */
			addseg(optarg);
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	special = argv[0];
	if ((fd = open(special, O_RDONLY, 0)) < 0)
		err(1, "%s", special);

	sbuf = malloc(LFS_SBPAD);
	if (sbuf == NULL)
		err(1, "malloc");

	if (sbdaddr == 0x0) {
		/* Read the proto-superblock */
		__CTASSERT(sizeof(struct dlfs) == sizeof(struct dlfs64));
		get(fd, LFS_LABELPAD, sbuf, LFS_SBPAD);
		memcpy(&lfs_sb1.lfs_dlfs_u, sbuf, sizeof(struct dlfs));

		/* If that wasn't the real first sb, get the real first sb */
		if (lfs_sb_getversion(&lfs_sb1) > 1 &&
		    lfs_sb_getsboff(&lfs_sb1, 0) > lfs_btofsb(&lfs_sb1, LFS_LABELPAD))
			get(fd, lfs_fsbtob(&lfs_sb1, lfs_sb_getsboff(&lfs_sb1, 0)),
			    &lfs_sb1.lfs_dlfs_u, sizeof(struct dlfs));
	
		/*
	 	* Read the second superblock and figure out which check point is
	 	* most up to date.
	 	*/
		get(fd,
		    fsbtobyte(&lfs_sb1, lfs_sb_getsboff(&lfs_sb1, 1)),
		    sbuf, LFS_SBPAD);
		memcpy(&lfs_sb2.lfs_dlfs_u, sbuf, sizeof(struct dlfs));
	
		lfs_master = &lfs_sb1;
		if (lfs_sb_getversion(&lfs_sb1) > 1) {
			if (lfs_sb_getserial(&lfs_sb1) > lfs_sb_getserial(&lfs_sb2)) {
				lfs_master = &lfs_sb2;
				sbdaddr = lfs_sb_getsboff(&lfs_sb1, 1);
			} else
				sbdaddr = lfs_sb_getsboff(&lfs_sb1, 0);
		} else {
			if (lfs_sb_getotstamp(&lfs_sb1) > lfs_sb_getotstamp(&lfs_sb2)) {
				lfs_master = &lfs_sb2;
				sbdaddr = lfs_sb_getsboff(&lfs_sb1, 1);
			} else
				sbdaddr = lfs_sb_getsboff(&lfs_sb1, 0);
		}
	} else {
		/* Read the first superblock */
		get(fd, dbtob((off_t)sbdaddr), sbuf, LFS_SBPAD);
		memcpy(&lfs_sb1.lfs_dlfs_u, sbuf, sizeof(struct dlfs));
		lfs_master = &lfs_sb1;
	}

	free(sbuf);

	/* Compatibility */
	if (lfs_sb_getversion(lfs_master) == 1) {
		lfs_sb_setsumsize(lfs_master, LFS_V1_SUMMARY_SIZE);
		lfs_sb_setibsize(lfs_master, lfs_sb_getbsize(lfs_master));
		lfs_sb_sets0addr(lfs_master, lfs_sb_getsboff(lfs_master, 0));
		lfs_sb_settstamp(lfs_master, lfs_sb_getotstamp(lfs_master));
		lfs_sb_setfsbtodb(lfs_master, 0);
	}

	(void)printf("Master Superblock at 0x%llx:\n", (long long)sbdaddr);
	dump_super(lfs_master);

	dump_ifile(fd, lfs_master, do_ientries, do_segentries, idaddr);

	if (seglist != NULL)
		for (; seglist != NULL; seglist = seglist->next) {
			seg_addr = lfs_sntod(lfs_master, seglist->num);
			dump_segment(fd, seglist->num, seg_addr, lfs_master,
				     do_allsb);
		}
	else
		for (segnum = 0, seg_addr = lfs_sntod(lfs_master, 0);
		     segnum < lfs_sb_getnseg(lfs_master);
		     segnum++, seg_addr = lfs_sntod(lfs_master, segnum))
			dump_segment(fd, segnum, seg_addr, lfs_master,
				     do_allsb);

	(void)close(fd);
	exit(0);
}

/*
 * We are reading all the blocks of an inode and dumping out the ifile table.
 * This code could be tighter, but this is a first pass at getting the stuff
 * printed out rather than making this code incredibly efficient.
 */
static void
dump_ifile(int fd, struct lfs *lfsp, int do_ientries, int do_segentries, daddr_t addr)
{
	char *ipage;
	char *dpage;
	union lfs_dinode *dip = NULL;
	/* XXX ondisk32 */
	int32_t *addrp, *dindir, *iaddrp, *indir;
	daddr_t pdb;
	int block_limit, i, inum, j, nblocks, psize;

	psize = lfs_sb_getbsize(lfsp);
	if (!addr)
		addr = lfs_sb_getidaddr(lfsp);

	if (!(dpage = malloc(psize)))
		err(1, "malloc");
	get(fd, fsbtobyte(lfsp, addr), dpage, psize);

	for (i = LFS_INOPB(lfsp); i-- > 0; ) {
		dip = DINO_IN_BLOCK(lfsp, dpage, i);
		if (lfs_dino_getinumber(lfsp, dip) == LFS_IFILE_INUM)
			break;
	}

	if (lfs_dino_getinumber(lfsp, dip) != LFS_IFILE_INUM) {
		warnx("unable to locate ifile inode at disk address 0x%jx",
		     (uintmax_t)addr);
		return;
	}

	(void)printf("\nIFILE inode\n");
	dump_dinode(lfsp, dip);

	(void)printf("\nIFILE contents\n");
	nblocks = lfs_dino_getsize(lfsp, dip) >> lfs_sb_getbshift(lfsp);
	block_limit = MIN(nblocks, ULFS_NDADDR);

	/* Get the direct block */
	if ((ipage = malloc(psize)) == NULL)
		err(1, "malloc");
	for (inum = 0, i = 0; i < block_limit; i++) {
		pdb = lfs_dino_getdb(lfsp, dip, i);
		get(fd, fsbtobyte(lfsp, pdb), ipage, psize);
		if (i < lfs_sb_getcleansz(lfsp)) {
			dump_cleaner_info(lfsp, ipage);
			if (do_segentries)
				print_suheader;
			continue;
		} 

		if (i < (lfs_sb_getsegtabsz(lfsp) + lfs_sb_getcleansz(lfsp))) {
			if (do_segentries)
				inum = dump_ipage_segusage(lfsp, inum, ipage, 
							   lfs_sb_getsepb(lfsp));
			else
				inum = (i < lfs_sb_getsegtabsz(lfsp) + lfs_sb_getcleansz(lfsp) - 1);
			if (!inum) {
				if(!do_ientries)
					goto e0;
				else
					print_iheader;
			}
		} else
			inum = dump_ipage_ifile(lfsp, inum, ipage, lfs_sb_getifpb(lfsp));
	}

	if (nblocks <= ULFS_NDADDR)
		goto e0;

	/* Dump out blocks off of single indirect block */
	if (!(indir = malloc(psize)))
		err(1, "malloc");
	get(fd, fsbtobyte(lfsp, lfs_dino_getib(lfsp, dip, 0)), indir, psize);
	block_limit = MIN(i + lfs_sb_getnindir(lfsp), nblocks);
	for (addrp = indir; i < block_limit; i++, addrp++) {
		if (*addrp == LFS_UNUSED_DADDR)
			break;
		get(fd, fsbtobyte(lfsp, *addrp), ipage, psize);
		if (i < lfs_sb_getcleansz(lfsp)) {
			dump_cleaner_info(lfsp, ipage);
			continue;
		}

		if (i < lfs_sb_getsegtabsz(lfsp) + lfs_sb_getcleansz(lfsp)) {
			if (do_segentries)
				inum = dump_ipage_segusage(lfsp, inum, ipage, 
							   lfs_sb_getsepb(lfsp));
			else
				inum = (i < lfs_sb_getsegtabsz(lfsp) + lfs_sb_getcleansz(lfsp) - 1);
			if (!inum) {
				if(!do_ientries)
					goto e1;
				else
					print_iheader;
			}
		} else
			inum = dump_ipage_ifile(lfsp, inum, ipage, lfs_sb_getifpb(lfsp));
	}

	if (nblocks <= ULFS_NDADDR + lfs_sb_getnindir(lfsp))
		goto e1;

	/* Get the double indirect block */
	if (!(dindir = malloc(psize)))
		err(1, "malloc");
	get(fd, fsbtobyte(lfsp, lfs_dino_getib(lfsp, dip, 1)), dindir, psize);
	for (iaddrp = dindir, j = 0; j < lfs_sb_getnindir(lfsp); j++, iaddrp++) {
		if (*iaddrp == LFS_UNUSED_DADDR)
			break;
		get(fd, fsbtobyte(lfsp, *iaddrp), indir, psize);
		block_limit = MIN(i + lfs_sb_getnindir(lfsp), nblocks);
		for (addrp = indir; i < block_limit; i++, addrp++) {
			if (*addrp == LFS_UNUSED_DADDR)
				break;
			get(fd, fsbtobyte(lfsp, *addrp), ipage, psize);
			if (i < lfs_sb_getcleansz(lfsp)) {
				dump_cleaner_info(lfsp, ipage);
				continue;
			}

			if (i < lfs_sb_getsegtabsz(lfsp) + lfs_sb_getcleansz(lfsp)) {
				if (do_segentries)
					inum = dump_ipage_segusage(lfsp,
						 inum, ipage, lfs_sb_getsepb(lfsp));
				else
					inum = (i < lfs_sb_getsegtabsz(lfsp) +
						lfs_sb_getcleansz(lfsp) - 1);
				if (!inum) {
					if(!do_ientries)
						goto e2;
					else
						print_iheader;
				}
			} else
				inum = dump_ipage_ifile(lfsp, inum,
				    ipage, lfs_sb_getifpb(lfsp));
		}
	}
e2:	free(dindir);
e1:	free(indir);
e0:	free(dpage);
	free(ipage);
}

static int
dump_ipage_ifile(struct lfs *lfsp, int i, char *pp, int tot)
{
	char *ip;
	int cnt, max, entsize;

	if (lfsp->lfs_is64)
		entsize = sizeof(IFILE64);
	if (lfs_sb_getversion(lfsp) > 1) 
		entsize = sizeof(IFILE32);
	else 
		entsize = sizeof(IFILE_V1);
	max = i + tot;

	for (ip = pp, cnt = i; cnt < max; cnt++, ip += entsize)
		print_ientry(cnt, lfsp, (IFILE *)ip);
	return (max);
}

static int
dump_ipage_segusage(struct lfs *lfsp, int i, char *pp, int tot)
{
	SEGUSE *sp;
	int cnt, max;
	struct seglist *slp;

	max = i + tot;
	for (sp = (SEGUSE *)pp, cnt = i;
	     cnt < lfs_sb_getnseg(lfsp) && cnt < max; cnt++) {
		if (seglist == NULL)
			print_suentry(cnt, sp, lfsp);
		else {
			for (slp = seglist; slp != NULL; slp = slp->next)
				if (cnt == slp->num) {
					print_suentry(cnt, sp, lfsp);
					break;
				}
		}
		if (lfs_sb_getversion(lfsp) > 1)
			++sp;
		else
			sp = (SEGUSE *)((SEGUSE_V1 *)sp + 1);
	}
	if (max >= lfs_sb_getnseg(lfsp))
		return (0);
	else
		return (max);
}

static void
dump_dinode(struct lfs *fs, union lfs_dinode *dip)
{
	int i;
	time_t at, mt, ct;

	at = lfs_dino_getatime(fs, dip);
	mt = lfs_dino_getmtime(fs, dip);
	ct = lfs_dino_getctime(fs, dip);

	(void)printf("    %so%o\t%s%d\t%s%d\t%s%d\t%s%ju\n",
		"mode  ", lfs_dino_getmode(fs, dip),
		"nlink ", lfs_dino_getnlink(fs, dip),
		"uid   ", lfs_dino_getuid(fs, dip),
		"gid   ", lfs_dino_getgid(fs, dip),
		"size  ", (uintmax_t)lfs_dino_getsize(fs, dip));
	(void)printf("    %s%s", "atime ", ctime(&at));
	(void)printf("    %s%s", "mtime ", ctime(&mt));
	(void)printf("    %s%s", "ctime ", ctime(&ct));
	(void)printf("    inum  %ju\n",
		(uintmax_t)lfs_dino_getinumber(fs, dip));
	(void)printf("    Direct Addresses\n");
	for (i = 0; i < ULFS_NDADDR; i++) {
		(void)printf("\t0x%jx", (intmax_t)lfs_dino_getdb(fs, dip, i));
		if ((i % 6) == 5)
			(void)printf("\n");
	}
	(void)printf("    Indirect Addresses\n");
	for (i = 0; i < ULFS_NIADDR; i++)
		(void)printf("\t0x%jx", (intmax_t)lfs_dino_getib(fs, dip, i));
	(void)printf("\n");
}

static int
dump_sum(int fd, struct lfs *lfsp, SEGSUM *sp, int segnum, daddr_t addr)
{
	FINFO *fp;
	IINFO *iip, *iip2;
	union lfs_blocks fipblocks;
	int i, j, acc;
	int ck;
	int numbytes, numblocks;
	char *datap;
	char *diblock;
	union lfs_dinode *dip;
	size_t el_size;
	u_int32_t datasum;
	u_int32_t ssflags;
	time_t t;
	char *buf;
	size_t sumstart;

	sumstart = lfs_ss_getsumstart(lfsp);
	if (lfs_ss_getmagic(lfsp, sp) != SS_MAGIC || 
	    lfs_ss_getsumsum(lfsp, sp) != (ck = cksum((char *)sp + sumstart,
	    lfs_sb_getsumsize(lfsp) - sumstart))) {
		/* Don't print "corrupt" if we're just too close to the edge */
		if (lfs_dtosn(lfsp, addr + LFS_FSBTODB(lfsp, 1)) ==
		    lfs_dtosn(lfsp, addr))
			(void)printf("dumplfs: %s %d address 0x%llx\n",
		                     "corrupt summary block; segment", segnum,
				     (long long)addr);
		return -1;
	}
	if (lfs_sb_getversion(lfsp) > 1 && lfs_ss_getident(lfsp, sp) != lfs_sb_getident(lfsp)) {
		(void)printf("dumplfs: %s %d address 0x%llx\n",
	                     "summary from a former life; segment", segnum,
			     (long long)addr);
		return -1;
	}

	(void)printf("Segment Summary Info at 0x%llx\n", (long long)addr);
	ssflags = lfs_ss_getflags(lfsp, sp);
	(void)printf("    %s0x%jx\t%s%d\t%s%d\t%s%c%c%c%c\n    %s0x%x\t%s0x%x",
		"next     ", (intmax_t)lfs_ss_getnext(lfsp, sp),
		"nfinfo   ", lfs_ss_getnfinfo(lfsp, sp),
		"ninos    ", lfs_ss_getninos(lfsp, sp),
		"flags    ", (ssflags & SS_DIROP) ? 'D' : '-',
			     (ssflags & SS_CONT)  ? 'C' : '-',
			     (ssflags & SS_CLEAN)  ? 'L' : '-',
			     (ssflags & SS_RFW)  ? 'R' : '-',
		"sumsum   ", lfs_ss_getsumsum(lfsp, sp),
		"datasum  ", lfs_ss_getdatasum(lfsp, sp));
	if (lfs_sb_getversion(lfsp) == 1) {
		t = lfs_ss_getocreate(lfsp, sp);
		(void)printf("\tcreate   %s\n", ctime(&t));
	} else {
		t = lfs_ss_getcreate(lfsp, sp);
		(void)printf("\tcreate   %s", ctime(&t));
		(void)printf("    roll_id  %-8x", lfs_ss_getident(lfsp, sp));
		(void)printf("   serial   %lld\n",
			     (long long)lfs_ss_getserial(lfsp, sp));
	}

	/* Dump out inode disk addresses */
	iip = SEGSUM_IINFOSTART(lfsp, sp);
	diblock = malloc(lfs_sb_getbsize(lfsp));
	printf("    Inode addresses:");
	numbytes = 0;
	numblocks = 0;
	for (i = 0; i < lfs_ss_getninos(lfsp, sp); iip = NEXTLOWER_IINFO(lfsp, iip)) {
		++numblocks;
		numbytes += lfs_sb_getibsize(lfsp);	/* add bytes for inode block */
		printf("\t0x%jx {", (intmax_t)lfs_ii_getblock(lfsp, iip));
		get(fd, fsbtobyte(lfsp, lfs_ii_getblock(lfsp, iip)), diblock, lfs_sb_getibsize(lfsp));
		for (j = 0; i < lfs_ss_getninos(lfsp, sp) && j < LFS_INOPB(lfsp); j++, i++) {
			if (j > 0) 
				(void)printf(", ");
			dip = DINO_IN_BLOCK(lfsp, diblock, j);
			(void)printf("%juv%d", lfs_dino_getinumber(lfsp, dip),
				     lfs_dino_getgen(lfsp, dip));
		}
		(void)printf("}");
		if (((i/LFS_INOPB(lfsp)) % 4) == 3)
			(void)printf("\n");
	}
	free(diblock);

	printf("\n");

	fp = SEGSUM_FINFOBASE(lfsp, sp);
	for (i = 0; i < lfs_ss_getnfinfo(lfsp, sp); i++) {
		(void)printf("    FINFO for inode: %ju version %u nblocks %u lastlength %u\n",
		    (uintmax_t)lfs_fi_getino(lfsp, fp),
		    lfs_fi_getversion(lfsp, fp),
		    lfs_fi_getnblocks(lfsp, fp),
		    lfs_fi_getlastlength(lfsp, fp));
		lfs_blocks_fromfinfo(lfsp, &fipblocks, fp);
		numblocks += lfs_fi_getnblocks(lfsp, fp);
		for (j = 0; j < lfs_fi_getnblocks(lfsp, fp); j++) {
			(void)printf("\t%jd",
			    (intmax_t)lfs_blocks_get(lfsp, &fipblocks, j));
			if ((j % 8) == 7)
				(void)printf("\n");
			if (j == lfs_fi_getnblocks(lfsp, fp) - 1)
				numbytes += lfs_fi_getlastlength(lfsp, fp);
			else
				numbytes += lfs_sb_getbsize(lfsp);
		}
		if ((j % 8) != 0)
			(void)printf("\n");
		fp = NEXT_FINFO(lfsp, fp);
	}

	if (datasum_check == 0)
		return (numbytes);

	/*
	 * Now that we know the number of blocks, run back through and
	 * compute the data checksum.  (A bad data checksum is not enough
	 * to prevent us from continuing, but it odes merit a warning.)
	 */
	iip2 = SEGSUM_IINFOSTART(lfsp, sp);
	if (lfs_sb_getversion(lfsp) == 1) {
		fp = (FINFO *)((SEGSUM_V1 *)sp + 1);
		el_size = sizeof(unsigned long);
	} else {
		fp = (FINFO *)(sp + 1);
		el_size = sizeof(u_int32_t);
	}
	datap = (char *)malloc(el_size * numblocks);
	memset(datap, 0, el_size * numblocks);
	acc = 0;
	addr += lfs_btofsb(lfsp, lfs_sb_getsumsize(lfsp));
	buf = malloc(lfs_sb_getbsize(lfsp));
	for (i = 0; i < lfs_ss_getnfinfo(lfsp, sp); i++) {
		while (addr == lfs_ii_getblock(lfsp, iip2)) {
			get(fd, fsbtobyte(lfsp, addr), buf, lfs_sb_getibsize(lfsp));
			memcpy(datap + acc * el_size, buf, el_size);
			addr += lfs_btofsb(lfsp, lfs_sb_getibsize(lfsp));
			iip2 = NEXTLOWER_IINFO(lfsp, iip2);
			++acc;
		}
		for (j = 0; j < lfs_fi_getnblocks(lfsp, fp); j++) {
			get(fd, fsbtobyte(lfsp, addr), buf, lfs_sb_getfsize(lfsp));
			memcpy(datap + acc * el_size, buf, el_size);
			if (j == lfs_fi_getnblocks(lfsp, fp) - 1)
				addr += lfs_btofsb(lfsp, lfs_fi_getlastlength(lfsp, fp));
			else
				addr += lfs_btofsb(lfsp, lfs_sb_getbsize(lfsp));
			++acc;
		}
		fp = NEXT_FINFO(lfsp, fp);
	}
	while (addr == lfs_ii_getblock(lfsp, iip2)) {
		get(fd, fsbtobyte(lfsp, addr), buf, lfs_sb_getibsize(lfsp));
		memcpy(datap + acc * el_size, buf, el_size);
		addr += lfs_btofsb(lfsp, lfs_sb_getibsize(lfsp));
		iip2 = NEXTLOWER_IINFO(lfsp, iip2);
		++acc;
	}
	free(buf);
	if (acc != numblocks)
		printf("** counted %d blocks but should have been %d\n",
		     acc, numblocks);
	datasum = cksum(datap, numblocks * el_size);
	if (datasum != lfs_ss_getdatasum(lfsp, sp))
		printf("** computed datasum 0x%lx does not match given datasum 0x%lx\n", (unsigned long)datasum, (unsigned long)lfs_ss_getdatasum(lfsp, sp));
	free(datap);

	return (numbytes);
}

static void
dump_segment(int fd, int segnum, daddr_t addr, struct lfs *lfsp, int dump_sb)
{
	struct lfs lfs_sb, *sbp;
	SEGSUM *sump;
	size_t sumstart;
	char *sumblock;
	int did_one, nbytes, sb;
	off_t sum_offset;
	daddr_t new_addr;

	(void)printf("\nSEGMENT %lld (Disk Address 0x%llx)\n",
		     (long long)lfs_dtosn(lfsp, addr), (long long)addr);
	sum_offset = fsbtobyte(lfsp, addr);
	sumblock = malloc(lfs_sb_getsumsize(lfsp));

	if (lfs_sb_getversion(lfsp) > 1 && segnum == 0) {
		if (lfs_fsbtob(lfsp, lfs_sb_gets0addr(lfsp)) < LFS_LABELPAD) {
			/* First segment eats the disklabel */
			sum_offset += lfs_fragroundup(lfsp, LFS_LABELPAD) -
				      lfs_fsbtob(lfsp, lfs_sb_gets0addr(lfsp));
			addr += lfs_btofsb(lfsp, lfs_fragroundup(lfsp, LFS_LABELPAD)) -
				lfs_sb_gets0addr(lfsp);
			printf("Disklabel at 0x0\n");
		}
	}

	sb = 0;
	did_one = 0;
	do {
		get(fd, sum_offset, sumblock, lfs_sb_getsumsize(lfsp));
		sump = (SEGSUM *)sumblock;
		sumstart = lfs_ss_getsumstart(lfsp);
		if ((lfs_sb_getversion(lfsp) > 1 &&
		     lfs_ss_getident(lfsp, sump) != lfs_sb_getident(lfsp)) ||
		    lfs_ss_getsumsum(lfsp, sump) !=
		      cksum((char *)sump + sumstart,
			    lfs_sb_getsumsize(lfsp) - sumstart)) {
			sbp = (struct lfs *)sump;
			if ((sb = (sbp->lfs_dlfs_u.u_32.dlfs_magic == LFS_MAGIC))) {
				printf("Superblock at 0x%x\n",
				       (unsigned)lfs_btofsb(lfsp, sum_offset));
				if (dump_sb)  {
					__CTASSERT(sizeof(struct dlfs) ==
						   sizeof(struct dlfs64));
					get(fd, sum_offset, &(lfs_sb.lfs_dlfs_u),
					    sizeof(struct dlfs));
					dump_super(&lfs_sb);
				}
				if (lfs_sb_getversion(lfsp) > 1)
					sum_offset += lfs_fragroundup(lfsp, LFS_SBPAD);
				else
					sum_offset += LFS_SBPAD;
			} else if (did_one)
				break;
			else {
				printf("Segment at 0x%llx empty or corrupt\n",
                                       (long long)addr);
				break;
			}
		} else {
			nbytes = dump_sum(fd, lfsp, sump, segnum, 
				lfs_btofsb(lfsp, sum_offset));
			if (nbytes >= 0)
				sum_offset += lfs_sb_getsumsize(lfsp) + nbytes;
			else
				sum_offset = 0;
			did_one = 1;
		}
		/* If the segment ends right on a boundary, it still ends */
		new_addr = lfs_btofsb(lfsp, sum_offset);
		/* printf("end daddr = 0x%lx\n", (long)new_addr); */
		if (lfs_dtosn(lfsp, new_addr) != lfs_dtosn(lfsp, addr))
			break;
	} while (sum_offset);

	free(sumblock);
}

static void
dump_super(struct lfs *lfsp)
{
	time_t stamp;
	int i;

 	(void)printf("    %s0x%-8x  %s0x%-8x  %s%-10ju\n",
 		     "magic    ", lfsp->lfs_dlfs_u.u_32.dlfs_magic,
 		     "version  ", lfs_sb_getversion(lfsp),
 		     "size     ", (uintmax_t)lfs_sb_getsize(lfsp));
 	(void)printf("    %s%-10d  %s%-10ju  %s%-10d\n",
 		     "ssize    ", lfs_sb_getssize(lfsp),
 		     "dsize    ", (uintmax_t)lfs_sb_getdsize(lfsp),
 		     "bsize    ", lfs_sb_getbsize(lfsp));
 	(void)printf("    %s%-10d  %s%-10d  %s%-10d\n",
 		     "fsize    ", lfs_sb_getfsize(lfsp),
 		     "frag     ", lfs_sb_getfrag(lfsp),
 		     "minfree  ", lfs_sb_getminfree(lfsp));
 	(void)printf("    %s%-10d  %s%-10d  %s%-10d\n",
 		     "inopb    ", lfs_sb_getinopb(lfsp),
 		     "ifpb     ", lfs_sb_getifpb(lfsp),
 		     "nindir   ", lfs_sb_getnindir(lfsp));
 	(void)printf("    %s%-10d  %s%-10d  %s%-10d\n",
 		     "nseg     ", lfs_sb_getnseg(lfsp),
 		     "sepb     ", lfs_sb_getsepb(lfsp),
 		     "cleansz  ", lfs_sb_getcleansz(lfsp));
 	(void)printf("    %s%-10d  %s0x%-8x  %s%-10d\n",
 		     "segtabsz ", lfs_sb_getsegtabsz(lfsp),
 		     "segmask  ", lfs_sb_getsegmask(lfsp),
 		     "segshift ", lfs_sb_getsegshift(lfsp));
 	(void)printf("    %s0x%-8jx  %s%-10d  %s0x%-8jX\n",
 		     "bmask    ", (uintmax_t)lfs_sb_getbmask(lfsp),
 		     "bshift   ", lfs_sb_getbshift(lfsp),
 		     "ffmask   ", (uintmax_t)lfs_sb_getffmask(lfsp));
 	(void)printf("    %s%-10d  %s0x%-8jx  %s%u\n",
 		     "ffshift  ", lfs_sb_getffshift(lfsp),
 		     "fbmask   ", (uintmax_t)lfs_sb_getfbmask(lfsp),
 		     "fbshift  ", lfs_sb_getfbshift(lfsp));
 	
 	(void)printf("    %s%-10d  %s%-10d  %s0x%-8x\n",
 		     "sushift  ", lfs_sb_getsushift(lfsp),
 		     "fsbtodb  ", lfs_sb_getfsbtodb(lfsp),
 		     "cksum    ", lfs_sb_getcksum(lfsp));
 	(void)printf("    %s%-10d  %s%-10d  %s%-10d\n",
 		     "nclean   ", lfs_sb_getnclean(lfsp),
 		     "dmeta    ", lfs_sb_getdmeta(lfsp),
 		     "minfreeseg ", lfs_sb_getminfreeseg(lfsp));
 	(void)printf("    %s0x%-8x  %s%-9d %s%-10d\n",
 		     "roll_id  ", lfs_sb_getident(lfsp),
 		     "interleave ", lfs_sb_getinterleave(lfsp),
 		     "sumsize  ", lfs_sb_getsumsize(lfsp));
 	(void)printf("    %s%-10jd  %s0x%-8jx\n",
		     "seg0addr ", (intmax_t)lfs_sb_gets0addr(lfsp),
 		     "maxfilesize  ", (uintmax_t)lfs_sb_getmaxfilesize(lfsp));
 	
 	
 	(void)printf("  Superblock disk addresses:\n    ");
  	for (i = 0; i < LFS_MAXNUMSB; i++) {
 		(void)printf(" 0x%-8jx", (intmax_t)lfs_sb_getsboff(lfsp, i));
 		if (i == (LFS_MAXNUMSB >> 1))
 			(void)printf("\n    ");
  	}
  	(void)printf("\n");
 	
 	(void)printf("  Checkpoint Info\n");
 	(void)printf("    %s%-10ju  %s0x%-8jx\n",
 		     "freehd   ", (uintmax_t)lfs_sb_getfreehd(lfsp),
 		     "idaddr   ", (intmax_t)lfs_sb_getidaddr(lfsp));
 	(void)printf("    %s%-10d  %s%-10jd  %s%-10jd\n",
 		     "uinodes  ", lfs_sb_getuinodes(lfsp),
 		     "bfree    ", (intmax_t)lfs_sb_getbfree(lfsp),
 		     "avail    ", (intmax_t)lfs_sb_getavail(lfsp));
 	(void)printf("    %s%-10ju  %s0x%-8jx  %s0x%-8jx\n",
 		     "nfiles   ", (uintmax_t)lfs_sb_getnfiles(lfsp),
 		     "lastseg  ", (uintmax_t)lfs_sb_getlastseg(lfsp),
 		     "nextseg  ", (uintmax_t)lfs_sb_getnextseg(lfsp));
 	(void)printf("    %s0x%-8jx  %s0x%-8jx  %s%-10ju\n",
 		     "curseg   ", (uintmax_t)lfs_sb_getcurseg(lfsp),
 		     "offset   ", (uintmax_t)lfs_sb_getoffset(lfsp),
		     "serial   ", (uintmax_t)lfs_sb_getserial(lfsp));
	stamp = lfs_sb_gettstamp(lfsp);
 	(void)printf("    tstamp   %s", ctime(&stamp));

	if (!lfsp->lfs_is64) {
		(void)printf("  32-bit only derived or constant fields\n");
		(void)printf("    %s%-10u\n",
 		     "ifile    ", lfs_sb_getifile(lfsp));
	}
}

static void
addseg(char *arg)
{
	SEGLIST *p;

	if ((p = malloc(sizeof(SEGLIST))) == NULL)
		err(1, "malloc");
	p->next = seglist;
	p->num = atoi(arg);
	seglist = p;
}

static void
dump_cleaner_info(struct lfs *lfsp, void *ipage)
{
	CLEANERINFO *cip;

	cip = (CLEANERINFO *)ipage;
	if (lfs_sb_getversion(lfsp) > 1) {
		(void)printf("free_head %ju\n",
			     (uintmax_t)lfs_ci_getfree_head(lfsp, cip));
		(void)printf("free_tail %ju\n",
			     (uintmax_t)lfs_ci_getfree_tail(lfsp, cip));
	}
	(void)printf("clean\t%u\tdirty\t%u\n",
		     lfs_ci_getclean(lfsp, cip), lfs_ci_getdirty(lfsp, cip));
	(void)printf("bfree\t%jd\tavail\t%jd\n\n",
		     (intmax_t)lfs_ci_getbfree(lfsp, cip),
		     (intmax_t)lfs_ci_getavail(lfsp, cip));
}

static void
usage(void)
{
	(void)fprintf(stderr, "usage: dumplfs [-adiS] [-b blkno] [-I blkno] [-s segno] filesys|device\n");
	exit(1);
}
