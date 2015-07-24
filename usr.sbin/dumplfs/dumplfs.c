/*	$NetBSD: dumplfs.c,v 1.43 2015/07/24 06:56:42 dholland Exp $	*/

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
__RCSID("$NetBSD: dumplfs.c,v 1.43 2015/07/24 06:56:42 dholland Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#include <sys/time.h>

#include <ufs/lfs/lfs.h>

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
static void	dump_dinode(struct ulfs1_dinode *);
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

	t = (fs->lfs_version == 1 ? sp->su_olastmod : sp->su_lastmod);

	printf("%d\t%s\t%d\t%d\t%d\t%s", i, flags,
		sp->su_nbytes, sp->su_ninos, sp->su_nsums,
		ctime(&t));
}

/* Ifile formats */
#define print_iheader \
	(void)printf("inum\tstatus\tversion\tdaddr\t\tfreeptr\n")

static inline void
print_ientry(int i, IFILE *ip)
{
	if (ip->if_daddr == LFS_UNUSED_DADDR)
		printf("%d\tFREE\t%d\t \t\t%llu\n", i, ip->if_version,
		    (unsigned long long)ip->if_nextfree);
	else
		printf("%d\tINUSE\t%d\t%8X\t%s\n",
		    i, ip->if_version, ip->if_daddr,
		    (ip->if_nextfree == LFS_ORPHAN_NEXTFREE ? "FFFFFFFF" : "-"));
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
		get(fd, LFS_LABELPAD, sbuf, LFS_SBPAD);
		memcpy(&(lfs_sb1.lfs_dlfs), sbuf, sizeof(struct dlfs));

		/* If that wasn't the real first sb, get the real first sb */
		if (lfs_sb1.lfs_version > 1 &&
		    lfs_sb1.lfs_sboffs[0] > lfs_btofsb(&lfs_sb1, LFS_LABELPAD))
			get(fd, lfs_fsbtob(&lfs_sb1, lfs_sb1.lfs_sboffs[0]),
			    &(lfs_sb1.lfs_dlfs), sizeof(struct dlfs));
	
		/*
	 	* Read the second superblock and figure out which check point is
	 	* most up to date.
	 	*/
		get(fd,
		    fsbtobyte(&lfs_sb1, lfs_sb1.lfs_sboffs[1]),
		    sbuf, LFS_SBPAD);
		memcpy(&(lfs_sb2.lfs_dlfs), sbuf, sizeof(struct dlfs));
	
		lfs_master = &lfs_sb1;
		if (lfs_sb1.lfs_version > 1) {
			if (lfs_sb_getserial(&lfs_sb1) > lfs_sb_getserial(&lfs_sb2)) {
				lfs_master = &lfs_sb2;
				sbdaddr = lfs_sb1.lfs_sboffs[1];
			} else
				sbdaddr = lfs_sb1.lfs_sboffs[0];
		} else {
			if (lfs_sb_getotstamp(&lfs_sb1) > lfs_sb_getotstamp(&lfs_sb2)) {
				lfs_master = &lfs_sb2;
				sbdaddr = lfs_sb1.lfs_sboffs[1];
			} else
				sbdaddr = lfs_sb1.lfs_sboffs[0];
		}
	} else {
		/* Read the first superblock */
		get(fd, dbtob((off_t)sbdaddr), sbuf, LFS_SBPAD);
		memcpy(&(lfs_sb1.lfs_dlfs), sbuf, sizeof(struct dlfs));
		lfs_master = &lfs_sb1;
	}

	free(sbuf);

	/* Compatibility */
	if (lfs_master->lfs_version == 1) {
		lfs_master->lfs_sumsize = LFS_V1_SUMMARY_SIZE;
		lfs_master->lfs_ibsize = lfs_sb_getbsize(lfs_master);
		lfs_master->lfs_s0addr = lfs_master->lfs_sboffs[0];
		lfs_sb_settstamp(lfs_master, lfs_sb_getotstamp(lfs_master));
		lfs_master->lfs_fsbtodb = 0;
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
		     segnum < lfs_master->lfs_nseg;
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
	struct ulfs1_dinode *dip, *dpage;
	/* XXX ondisk32 */
	int32_t *addrp, *dindir, *iaddrp, *indir;
	int block_limit, i, inum, j, nblocks, psize;

	psize = lfs_sb_getbsize(lfsp);
	if (!addr)
		addr = lfs_sb_getidaddr(lfsp);

	if (!(dpage = malloc(psize)))
		err(1, "malloc");
	get(fd, fsbtobyte(lfsp, addr), dpage, psize);

	for (dip = dpage + LFS_INOPB(lfsp) - 1; dip >= dpage; --dip)
		if (dip->di_inumber == LFS_IFILE_INUM)
			break;

	if (dip < dpage) {
		warnx("unable to locate ifile inode at disk address 0x%llx",
		     (long long)addr);
		return;
	}

	(void)printf("\nIFILE inode\n");
	dump_dinode(dip);

	(void)printf("\nIFILE contents\n");
	nblocks = dip->di_size >> lfs_sb_getbshift(lfsp);
	block_limit = MIN(nblocks, ULFS_NDADDR);

	/* Get the direct block */
	if ((ipage = malloc(psize)) == NULL)
		err(1, "malloc");
	for (inum = 0, addrp = dip->di_db, i = 0; i < block_limit;
	    i++, addrp++) {
		get(fd, fsbtobyte(lfsp, *addrp), ipage, psize);
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
	get(fd, fsbtobyte(lfsp, dip->di_ib[0]), indir, psize);
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

	if (nblocks <= lfs_sb_getnindir(lfsp) * lfs_sb_getifpb(lfsp))
		goto e1;

	/* Get the double indirect block */
	if (!(dindir = malloc(psize)))
		err(1, "malloc");
	get(fd, fsbtobyte(lfsp, dip->di_ib[1]), dindir, psize);
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

	if (lfsp->lfs_version == 1) 
		entsize = sizeof(IFILE_V1);
	else 
		entsize = sizeof(IFILE);
	max = i + tot;

	for (ip = pp, cnt = i; cnt < max; cnt++, ip += entsize)
		print_ientry(cnt, (IFILE *)ip);
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
		if (lfsp->lfs_version > 1)
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
dump_dinode(struct ulfs1_dinode *dip)
{
	int i;
	time_t at, mt, ct;

	at = dip->di_atime;
	mt = dip->di_mtime;
	ct = dip->di_ctime;

	(void)printf("    %so%o\t%s%d\t%s%d\t%s%d\t%s%llu\n",
		"mode  ", dip->di_mode,
		"nlink ", dip->di_nlink,
		"uid   ", dip->di_uid,
		"gid   ", dip->di_gid,
		"size  ", (long long)dip->di_size);
	(void)printf("    %s%s    %s%s    %s%s",
		"atime ", ctime(&at),
		"mtime ", ctime(&mt),
		"ctime ", ctime(&ct));
	(void)printf("    inum  %d\n", dip->di_inumber);
	(void)printf("    Direct Addresses\n");
	for (i = 0; i < ULFS_NDADDR; i++) {
		(void)printf("\t0x%x", dip->di_db[i]);
		if ((i % 6) == 5)
			(void)printf("\n");
	}
	for (i = 0; i < ULFS_NIADDR; i++)
		(void)printf("\t0x%x", dip->di_ib[i]);
	(void)printf("\n");
}

static int
dump_sum(int fd, struct lfs *lfsp, SEGSUM *sp, int segnum, daddr_t addr)
{
	FINFO *fp;
	int32_t *dp, *idp;
	int i, j, acc;
	int ck;
	int numbytes, numblocks;
	char *datap;
	struct ulfs1_dinode *inop;
	size_t el_size;
	u_int32_t datasum;
	time_t t;
	char *buf;

	if (sp->ss_magic != SS_MAGIC || 
	    sp->ss_sumsum != (ck = cksum(&sp->ss_datasum, 
	    lfs_sb_getsumsize(lfsp) - sizeof(sp->ss_sumsum)))) {
		/* Don't print "corrupt" if we're just too close to the edge */
		if (lfs_dtosn(lfsp, addr + LFS_FSBTODB(lfsp, 1)) ==
		    lfs_dtosn(lfsp, addr))
			(void)printf("dumplfs: %s %d address 0x%llx\n",
		                     "corrupt summary block; segment", segnum,
				     (long long)addr);
		return -1;
	}
	if (lfsp->lfs_version > 1 && sp->ss_ident != lfsp->lfs_ident) {
		(void)printf("dumplfs: %s %d address 0x%llx\n",
	                     "summary from a former life; segment", segnum,
			     (long long)addr);
		return -1;
	}

	(void)printf("Segment Summary Info at 0x%llx\n", (long long)addr);
	(void)printf("    %s0x%x\t%s%d\t%s%d\t%s%c%c%c%c\n    %s0x%x\t%s0x%x",
		"next     ", sp->ss_next,
		"nfinfo   ", sp->ss_nfinfo,
		"ninos    ", sp->ss_ninos,
		"flags    ", (sp->ss_flags & SS_DIROP) ? 'D' : '-',
			     (sp->ss_flags & SS_CONT)  ? 'C' : '-',
			     (sp->ss_flags & SS_CLEAN)  ? 'L' : '-',
			     (sp->ss_flags & SS_RFW)  ? 'R' : '-',
		"sumsum   ", sp->ss_sumsum,
		"datasum  ", sp->ss_datasum );
	if (lfsp->lfs_version == 1) {
		t = sp->ss_ocreate;
		(void)printf("\tcreate   %s\n", ctime(&t));
	} else {
		t = sp->ss_create;
		(void)printf("\tcreate   %s", ctime(&t));
		(void)printf("    roll_id  %-8x", sp->ss_ident);
		(void)printf("   serial   %lld\n", (long long)sp->ss_serial);
	}

	/* Dump out inode disk addresses */
	dp = (int32_t *)sp;
	dp += lfs_sb_getsumsize(lfsp) / sizeof(int32_t);
	inop = malloc(lfs_sb_getbsize(lfsp));
	printf("    Inode addresses:");
	numbytes = 0;
	numblocks = 0;
	for (dp--, i = 0; i < sp->ss_ninos; dp--) {
		++numblocks;
		numbytes += lfs_sb_getibsize(lfsp);	/* add bytes for inode block */
		printf("\t0x%x {", *dp);
		get(fd, fsbtobyte(lfsp, *dp), inop, lfs_sb_getibsize(lfsp));
		for (j = 0; i < sp->ss_ninos && j < LFS_INOPB(lfsp); j++, i++) {
			if (j > 0) 
				(void)printf(", ");
			(void)printf("%dv%d", inop[j].di_inumber, inop[j].di_gen);
		}
		(void)printf("}");
		if (((i/LFS_INOPB(lfsp)) % 4) == 3)
			(void)printf("\n");
	}
	free(inop);

	printf("\n");

	if (lfsp->lfs_version == 1)
		fp = (FINFO *)((SEGSUM_V1 *)sp + 1);
	else
		fp = (FINFO *)(sp + 1);
	for (i = 0; i < sp->ss_nfinfo; i++) {
		(void)printf("    FINFO for inode: %d version %d nblocks %d lastlength %d\n",
		    fp->fi_ino, fp->fi_version, fp->fi_nblocks,
		    fp->fi_lastlength);
		dp = &(fp->fi_blocks[0]);
		numblocks += fp->fi_nblocks;
		for (j = 0; j < fp->fi_nblocks; j++, dp++) {
			(void)printf("\t%d", *dp);
			if ((j % 8) == 7)
				(void)printf("\n");
			if (j == fp->fi_nblocks - 1)
				numbytes += fp->fi_lastlength;
			else
				numbytes += lfs_sb_getbsize(lfsp);
		}
		if ((j % 8) != 0)
			(void)printf("\n");
		fp = (FINFO *)dp;
	}

	if (datasum_check == 0)
		return (numbytes);

	/*
	 * Now that we know the number of blocks, run back through and
	 * compute the data checksum.  (A bad data checksum is not enough
	 * to prevent us from continuing, but it odes merit a warning.)
	 */
	idp = (int32_t *)sp;
	idp += lfs_sb_getsumsize(lfsp) / sizeof(int32_t);
	--idp;
	if (lfsp->lfs_version == 1) {
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
	for (i = 0; i < sp->ss_nfinfo; i++) {
		while (addr == *idp) {
			get(fd, fsbtobyte(lfsp, addr), buf, lfs_sb_getibsize(lfsp));
			memcpy(datap + acc * el_size, buf, el_size);
			addr += lfs_btofsb(lfsp, lfs_sb_getibsize(lfsp));
			--idp;
			++acc;
		}
		for (j = 0; j < fp->fi_nblocks; j++) {
			get(fd, fsbtobyte(lfsp, addr), buf, lfs_sb_getfsize(lfsp));
			memcpy(datap + acc * el_size, buf, el_size);
			if (j == fp->fi_nblocks - 1)
				addr += lfs_btofsb(lfsp, fp->fi_lastlength);
			else
				addr += lfs_btofsb(lfsp, lfs_sb_getbsize(lfsp));
			++acc;
		}
		fp = (FINFO *)&(fp->fi_blocks[fp->fi_nblocks]);
	}
	while (addr == *idp) {
		get(fd, fsbtobyte(lfsp, addr), buf, lfs_sb_getibsize(lfsp));
		memcpy(datap + acc * el_size, buf, el_size);
		addr += lfs_btofsb(lfsp, lfs_sb_getibsize(lfsp));
		--idp;
		++acc;
	}
	free(buf);
	if (acc != numblocks)
		printf("** counted %d blocks but should have been %d\n",
		     acc, numblocks);
	datasum = cksum(datap, numblocks * el_size);
	if (datasum != sp->ss_datasum)
		printf("** computed datasum 0x%lx does not match given datasum 0x%lx\n", (unsigned long)datasum, (unsigned long)sp->ss_datasum);
	free(datap);

	return (numbytes);
}

static void
dump_segment(int fd, int segnum, daddr_t addr, struct lfs *lfsp, int dump_sb)
{
	struct lfs lfs_sb, *sbp;
	SEGSUM *sump;
	char *sumblock;
	int did_one, nbytes, sb;
	off_t sum_offset;
	daddr_t new_addr;

	(void)printf("\nSEGMENT %lld (Disk Address 0x%llx)\n",
		     (long long)lfs_dtosn(lfsp, addr), (long long)addr);
	sum_offset = fsbtobyte(lfsp, addr);
	sumblock = malloc(lfs_sb_getsumsize(lfsp));

	if (lfsp->lfs_version > 1 && segnum == 0) {
		if (lfs_fsbtob(lfsp, lfsp->lfs_s0addr) < LFS_LABELPAD) {
			/* First segment eats the disklabel */
			sum_offset += lfs_fragroundup(lfsp, LFS_LABELPAD) -
				      lfs_fsbtob(lfsp, lfsp->lfs_s0addr);
			addr += lfs_btofsb(lfsp, lfs_fragroundup(lfsp, LFS_LABELPAD)) -
				lfsp->lfs_s0addr;
			printf("Disklabel at 0x0\n");
		}
	}

	sb = 0;
	did_one = 0;
	do {
		get(fd, sum_offset, sumblock, lfs_sb_getsumsize(lfsp));
		sump = (SEGSUM *)sumblock;
		if ((lfsp->lfs_version > 1 &&
		     sump->ss_ident != lfs_sb_getident(lfsp)) ||
		    sump->ss_sumsum != cksum (&sump->ss_datasum, 
			      lfs_sb_getsumsize(lfsp) - sizeof(sump->ss_sumsum))) {
			sbp = (struct lfs *)sump;
			if ((sb = (sbp->lfs_magic == LFS_MAGIC))) {
				printf("Superblock at 0x%x\n",
				       (unsigned)lfs_btofsb(lfsp, sum_offset));
				if (dump_sb)  {
					get(fd, sum_offset, &(lfs_sb.lfs_dlfs),
					    sizeof(struct dlfs));
					dump_super(&lfs_sb);
				}
				if (lfsp->lfs_version > 1)
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

 	(void)printf("    %s0x%-8x  %s0x%-8x  %s%-10d\n",
 		     "magic    ", lfsp->lfs_magic,
 		     "version  ", lfsp->lfs_version,
 		     "size     ", lfs_sb_getsize(lfsp));
 	(void)printf("    %s%-10d  %s%-10d  %s%-10d\n",
 		     "ssize    ", lfs_sb_getssize(lfsp),
 		     "dsize    ", lfs_sb_getdsize(lfsp),
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
 	(void)printf("    %s%-10d  %s0x%-8jx\n",
		     "seg0addr ", lfsp->lfs_s0addr,
 		     "maxfilesize  ", (uintmax_t)lfs_sb_getmaxfilesize(lfsp));
 	
 	
 	(void)printf("  Superblock disk addresses:\n    ");
  	for (i = 0; i < LFS_MAXNUMSB; i++) {
 		(void)printf(" 0x%-8x", lfsp->lfs_sboffs[i]);
 		if (i == (LFS_MAXNUMSB >> 1))
 			(void)printf("\n    ");
  	}
  	(void)printf("\n");
 	
 	(void)printf("  Checkpoint Info\n");
 	(void)printf("    %s%-10d  %s0x%-8x  %s%-10d\n",
 		     "freehd   ", lfs_sb_getfreehd(lfsp),
 		     "idaddr   ", lfs_sb_getidaddr(lfsp),
 		     "ifile    ", lfs_sb_getifile(lfsp));
 	(void)printf("    %s%-10d  %s%-10d  %s%-10d\n",
 		     "uinodes  ", lfs_sb_getuinodes(lfsp),
 		     "bfree    ", lfs_sb_getbfree(lfsp),
 		     "avail    ", lfs_sb_getavail(lfsp));
 	(void)printf("    %s%-10d  %s0x%-8x  %s0x%-8x\n",
 		     "nfiles   ", lfs_sb_getnfiles(lfsp),
 		     "lastseg  ", lfs_sb_getlastseg(lfsp),
 		     "nextseg  ", lfs_sb_getnextseg(lfsp));
 	(void)printf("    %s0x%-8x  %s0x%-8x  %s%-10ju\n",
 		     "curseg   ", lfs_sb_getcurseg(lfsp),
 		     "offset   ", lfs_sb_getoffset(lfsp),
		     "serial   ", (uintmax_t)lfs_sb_getserial(lfsp));
	stamp = lfs_sb_gettstamp(lfsp);
 	(void)printf("    tstamp   %s", ctime(&stamp));
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
	if (lfsp->lfs_version > 1) {
		(void)printf("free_head %d\n", cip->free_head);
		(void)printf("free_tail %d\n", cip->free_tail);
	}
	(void)printf("clean\t%d\tdirty\t%d\n",
		     cip->clean, cip->dirty);
	(void)printf("bfree\t%d\tavail\t%d\n\n",
		     cip->bfree, cip->avail);
}

static void
usage(void)
{
	(void)fprintf(stderr, "usage: dumplfs [-adiS] [-b blkno] [-I blkno] [-s segno] filesys|device\n");
	exit(1);
}
