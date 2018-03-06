/*	$NetBSD: dumpfs.c,v 1.64 2018/03/06 07:45:38 mlelstv Exp $	*/

/*
 * Copyright (c) 1983, 1992, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1992, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)dumpfs.c	8.5 (Berkeley) 4/29/95";
#else
__RCSID("$NetBSD: dumpfs.c,v 1.64 2018/03/06 07:45:38 mlelstv Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>

#include <sys/wapbl.h>
#include <sys/wapbl_replay.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/ufs_bswap.h>
#include <ufs/ufs/ufs_wapbl.h>
#include <ufs/ufs/quota.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fstab.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <util.h>

static union fsun {
	struct fs fs;
	char pad[MAXBSIZE];
} fsun;
#define	afs	fsun.fs

static uint16_t opostblsave[32*8];

static union {
	struct cg cg;
	char pad[MAXBSIZE];
} cgun;
#define	acg	cgun.cg

static union {
	struct wapbl_wc_header wh;
	struct wapbl_wc_null wn;
	char pad[MAXBSIZE];
} jbuf;
#define awh 	jbuf.wh
#define awn 	jbuf.wn

#define OPT_FLAG(ch)	(1 << ((ch) & 31))
#define ISOPT(opt)	(opt_flags & (opt))
#define opt_alt_super	OPT_FLAG('a')
#define opt_superblock	OPT_FLAG('s')
#define opt_cg_summary	OPT_FLAG('m')
#define opt_cg_info	OPT_FLAG('c')
#define opt_inodes	OPT_FLAG('i')
#define opt_journal	OPT_FLAG('j')
#define opt_verbose	OPT_FLAG('v')
#define DFLT_CHECK (opt_alt_super | opt_cg_info | opt_inodes | \
    opt_cg_summary | opt_superblock | opt_journal )
#define DFLT_OPTS	(opt_superblock | opt_cg_summary | opt_cg_info | opt_verbose)

static long	dev_bsize = 512;
static int	needswap, printold, is_ufs2;
static int	Fflag;

static uint	opt_flags;

static int	dumpfs(const char *);
static int	print_superblock(struct fs *, uint16_t *, const char *, int, off_t);
static int	print_cgsum(const char *, int);
static int	print_cginfo(const char *, int);
static int	print_inodes(const char *, int, int, int);
static int	print_alt_super(const char *, int);
static int	print_journal(const char *, int);
static void	print_journal_header(const char *);
static off_t	print_journal_entries(const char *, size_t);
static int	dumpcg(const char *, int, int);
static int	openpartition(const char *, int, char *, size_t);
static void	pbits(int, void *, int);
__dead static void	usage(void);
static void	print_ufs1_inode(int, int, void *);
static void	print_ufs2_inode(int, int, void *);
static void	fix_superblock(struct fs *, uint16_t *);

int
main(int argc, char *argv[])
{
	int ch, eval;

	while ((ch = getopt(argc, argv, "acijmsvF")) != -1)
		switch(ch) {
		case 'a':	/* alternate superblocks */
		case 'c':	/* cylinder group info */
		case 'i':	/* actual inodes */
		case 'j':	/* journal */
		case 'm':	/* cylinder group summary */
		case 's':	/* superblock */
		case 'v':	/* more verbose */
			opt_flags |= OPT_FLAG(ch);
			break;
		case 'F':	/* File (not device) */
			Fflag = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if ((opt_flags & DFLT_CHECK) == 0)
		opt_flags |= DFLT_OPTS;

	if (argc < 1)
		usage();

	for (eval = 0; *argv; ++argv) {
		eval |= dumpfs(*argv);
	}

	exit(eval);
}


static int
dumpfs(const char *name)
{
	static const off_t sblock_try[] = SBLOCKSEARCH;
	char device[MAXPATHLEN];
	int fd, i;
	int rval = 1;

	if (Fflag)
		fd = open(name, O_RDONLY);
	else {
		fd = openpartition(name, O_RDONLY, device, sizeof(device));
		name = device;
	}
	if (fd == -1)
		goto err;

	for (i = 0; ; i++) {
		if (sblock_try[i] == -1) {
			warnx("%s: could not find superblock, skipped", name);
			return 1;
		}
		if (lseek(fd, sblock_try[i], SEEK_SET) == (off_t)-1)
			continue;
		if (read(fd, &afs, SBLOCKSIZE) != SBLOCKSIZE)
			continue;
		switch(afs.fs_magic) {
		case FS_UFS2_MAGIC:
			is_ufs2 = 1;
			break;
		case FS_UFS1_MAGIC:
			break;
		case FS_UFS2_MAGIC_SWAPPED:
			is_ufs2 = 1;
			needswap = 1;
			break;
		case FS_UFS1_MAGIC_SWAPPED:
			needswap = 1;
			break;
		default:
			continue;
		}
		fix_superblock(&afs, opostblsave);
		if (printold) {
			if (sblock_try[i] == SBLOCK_UFS2)
				/* This might be an alternate superblock */
				continue;
		} else {
			if (sblock_try[i] != afs.fs_sblockloc)
				/* This must be an alternate superblock */
				continue;
		}
		break;
	}


	dev_bsize = afs.fs_fsize / FFS_FSBTODB(&afs, 1);

	rval = 0;
	printf("file system: %s\n", name);

	if (ISOPT(opt_superblock))
		rval = print_superblock(&afs, opostblsave, name, fd, sblock_try[i]);
	if (rval == 0 && ISOPT(opt_cg_summary))
		rval = print_cgsum(name, fd);
	if (rval == 0 && ISOPT(opt_alt_super))
		rval = print_alt_super(name, fd);
	if (rval == 0 && ISOPT(opt_journal))
		rval = print_journal(name, fd);
	if (rval == 0 && ISOPT(opt_cg_info))
		rval = print_cginfo(name, fd);
	else if (rval == 0 && ISOPT(opt_inodes))
		rval = print_inodes(name, fd, 0, afs.fs_ncg);

    err:
	if (fd != -1)
		(void)close(fd);
	if (rval)
		warn("%s", name);
	return rval;
}

static void
fix_superblock(struct fs *fs, uint16_t *opostbl)
{
	if (needswap &&
	    (((int32_t)bswap32(fs->fs_old_postblformat) == FS_42POSTBLFMT) ||
	     (bswap32(fs->fs_old_postbloff) == offsetof(struct fs, fs_old_postbl_start)))) {
		int i;
		memcpy(opostbl, &fs->fs_old_postbl_start, 512);
		for(i=0;i<256;i++)
			opostbl[i] = bswap16(opostbl[i]);
	} else if (!needswap &&
	   ((fs->fs_old_postblformat == FS_42POSTBLFMT) ||
	    (fs->fs_old_postbloff == offsetof(struct fs, fs_old_postbl_start)))) {
		memcpy(opostbl, &fs->fs_old_postbl_start, 512);
	}

	if (needswap)
		ffs_sb_swap(fs, fs);

	printold = (fs->fs_magic == FS_UFS1_MAGIC &&
		    (fs->fs_old_flags & FS_FLAGS_UPDATED) == 0);
	if (printold) {
		fs->fs_sblockloc = SBLOCK_UFS1;
		fs->fs_flags = fs->fs_old_flags;
		fs->fs_maxbsize = fs->fs_bsize;
		fs->fs_time = fs->fs_old_time;
		fs->fs_size = fs->fs_old_size;
		fs->fs_dsize = fs->fs_old_dsize;
		fs->fs_csaddr = fs->fs_old_csaddr;
		fs->fs_cstotal.cs_ndir = fs->fs_old_cstotal.cs_ndir;
		fs->fs_cstotal.cs_nbfree = fs->fs_old_cstotal.cs_nbfree;
		fs->fs_cstotal.cs_nifree = fs->fs_old_cstotal.cs_nifree;
		fs->fs_cstotal.cs_nffree = fs->fs_old_cstotal.cs_nffree;
	}

	if (printold && fs->fs_old_postblformat == FS_42POSTBLFMT)
		fs->fs_old_nrpos = 8;
}

static int
print_superblock(struct fs *fs, uint16_t *opostbl,
    const char *name, int fd, off_t sblock)
{
	int i, size;
	time_t t;
	int32_t fsflags;

	printf("format\tFFSv%d\n", is_ufs2+1);
#if BYTE_ORDER == LITTLE_ENDIAN
	if (needswap)
#else
	if (!needswap)
#endif
		printf("endian\tbig-endian\n");
	else
		printf("endian\tlittle-endian\n");
	t = fs->fs_time;
	if ((sblock != SBLOCK_UFS1) || ISOPT(opt_alt_super))
		printf("location %lld\t(-b %lld)\n",
		    (long long)sblock, (long long)(sblock/dev_bsize));
	printf("magic\t%-8x\ttime\t%s",
	    fs->fs_magic, ctime(&t));

	if (is_ufs2)
		i = 5;
	else {
		i = 0;
		if (fs->fs_old_postblformat != FS_42POSTBLFMT) {
			i++;
			if (fs->fs_old_inodefmt >= FS_44INODEFMT) {
				int max;

				i++;
				max = fs->fs_maxcontig;
				size = fs->fs_contigsumsize;
				if ((max < 2 && size == 0)
				    || (max > 1 && size >= MIN(max, FS_MAXCONTIG)))
					i++;
			}
			if (fs->fs_old_flags & FS_FLAGS_UPDATED) {
				i = 4;
			}
		}
	}
	if (!printold || fs->fs_sblockloc != SBLOCK_UFS1 ||
	    fs->fs_id[0] || fs->fs_id[1])
		printf("superblock location\t%jd\tid\t[ %x %x ]\n",
		    (intmax_t)fs->fs_sblockloc, fs->fs_id[0], fs->fs_id[1]);
	printf("cylgrp\t%s\tinodes\t%s\tsblock\t%s\tfslevel %d\n",
	    i < 1 ? "static" : "dynamic",
	    i < 2 ? "4.2/4.3BSD" : i < 5 ? "4.4BSD" : "FFSv2",
	    i < 4 ? "FFSv1" : "FFSv2", i);
	printf("nbfree\t%lld\tndir\t%lld\tnifree\t%lld\tnffree\t%lld\n",
	    (long long)fs->fs_cstotal.cs_nbfree,
	    (long long)fs->fs_cstotal.cs_ndir,
	    (long long)fs->fs_cstotal.cs_nifree,
	    (long long)fs->fs_cstotal.cs_nffree);
	if (printold)
		printf("ncg\t%d\tncyl\t%d\tsize\t%lld\tblocks\t%lld\n",
		    fs->fs_ncg, fs->fs_old_ncyl, (long long)fs->fs_size, (long long)fs->fs_dsize);
	else
		printf("ncg\t%d\tsize\t%lld\tblocks\t%lld\n",
		    fs->fs_ncg, (long long)fs->fs_size, (long long)fs->fs_dsize);
	printf("bsize\t%d\tshift\t%d\tmask\t0x%08x\n",
	    fs->fs_bsize, fs->fs_bshift, fs->fs_bmask);
	printf("fsize\t%d\tshift\t%d\tmask\t0x%08x\n",
	    fs->fs_fsize, fs->fs_fshift, fs->fs_fmask);
	printf("frag\t%d\tshift\t%d\tfsbtodb\t%d\n",
	    fs->fs_frag, fs->fs_fragshift, fs->fs_fsbtodb);
	if (printold)
		printf("cpg\t%d\t", fs->fs_old_cpg);
	printf("bpg\t%d\tfpg\t%d\tipg\t%d\n",
	    fs->fs_fpg / fs->fs_frag, fs->fs_fpg, fs->fs_ipg);
	printf("minfree\t%d%%\toptim\t%s\tmaxcontig %d\tmaxbpg\t%d\n",
	    fs->fs_minfree, fs->fs_optim == FS_OPTSPACE ? "space" : "time",
	    fs->fs_maxcontig, fs->fs_maxbpg);
	if (printold) {
		printf("rotdelay %dms\theadswitch %dus\ttrackseek %dus\trps\t%d\n",
		    fs->fs_old_rotdelay, fs->fs_old_headswitch,
		    fs->fs_old_trkseek, fs->fs_old_rps);
		printf("ntrak\t%d\tnsect\t%d\tnpsect\t%d\tspc\t%d\n",
		    fs->fs_spare2, fs->fs_old_nsect, fs->fs_old_npsect,
		    fs->fs_old_spc);
	}
	printf("symlinklen %d\t", fs->fs_maxsymlinklen);
	if (printold)
		printf("trackskew %d\tinterleave %d\t",
		    fs->fs_old_trackskew, fs->fs_old_interleave);
	printf("contigsumsize %d\n", fs->fs_contigsumsize);
	printf("maxfilesize 0x%016llx\n",
	    (unsigned long long)fs->fs_maxfilesize);
	if (printold)
		printf("nindir\t%d\tinopb\t%d\tnspf\t%d\n", fs->fs_nindir,
		    fs->fs_inopb, fs->fs_old_nspf);
	else 
		printf("nindir\t%d\tinopb\t%d\n", fs->fs_nindir, fs->fs_inopb);
	if (!printold || (fs->fs_avgfilesize > 0) || (fs->fs_avgfpdir > 0))
		printf("avgfilesize %d\tavgfpdir %d\n",
		    fs->fs_avgfilesize, fs->fs_avgfpdir);
	printf("sblkno\t%d\tcblkno\t%d\tiblkno\t%d\tdblkno\t%d\n",
	    fs->fs_sblkno, fs->fs_cblkno, fs->fs_iblkno, fs->fs_dblkno);
	printf("sbsize\t%d\tcgsize\t%d", fs->fs_sbsize, fs->fs_cgsize);
	if (printold)
		printf("\toffset\t%d\tmask\t0x%08x",
		    fs->fs_old_cgoffset, fs->fs_old_cgmask);
	printf("\ncsaddr\t%lld\tcssize\t%d",
	    (long long)fs->fs_csaddr, fs->fs_cssize);
	if (printold)
		printf("\tshift\t%d\tmask\t0x%08x",
		    fs->fs_old_csshift, fs->fs_old_csmask);
	printf("\ncgrotor\t%d\tfmod\t%d\tronly\t%d\tclean\t0x%02x\n",
	    fs->fs_cgrotor, fs->fs_fmod, fs->fs_ronly, fs->fs_clean);
	printf("wapbl version 0x%x\tlocation %u\tflags 0x%x\n",
	    fs->fs_journal_version, fs->fs_journal_location,
	    fs->fs_journal_flags);
	printf("wapbl loc0 %" PRIu64 "\tloc1 %" PRIu64,
	    fs->fs_journallocs[0], fs->fs_journallocs[1]);
	printf("\tloc2 %" PRIu64 "\tloc3 %" PRIu64 "\n",
	    fs->fs_journallocs[2], fs->fs_journallocs[3]);
	printf("usrquota %" PRIu64 "\tgrpquota %" PRIu64 "\n",
		fs->fs_quotafile[USRQUOTA], fs->fs_quotafile[GRPQUOTA]);
	printf("flags\t");
	if (fs->fs_flags == 0)
		printf("none");
	if (fs->fs_flags & FS_UNCLEAN)
		printf("unclean ");
	if (fs->fs_flags & FS_DOSOFTDEP)
		printf("soft-updates ");
	if (fs->fs_flags & FS_NEEDSFSCK)
		printf("needs fsck run ");
	if (fs->fs_flags & FS_SUJ)
		printf("journaled soft-updates ");
	if (fs->fs_flags & FS_ACLS)
		printf("acls ");
	if (fs->fs_flags & FS_MULTILABEL)
		printf("multilabel ");
	if (fs->fs_flags & FS_GJOURNAL)
		printf("gjournal ");
	if (fs->fs_flags & FS_FLAGS_UPDATED)
		printf("fs_flags expanded ");
	if (fs->fs_flags & FS_DOWAPBL)
		printf("wapbl ");
	if (fs->fs_flags & FS_DOQUOTA2)
		printf("quotas ");
	if (fs->fs_flags & FS_TRIM)
		printf("trim ");
	fsflags = fs->fs_flags & ~(FS_UNCLEAN | FS_DOSOFTDEP | FS_NEEDSFSCK |
			FS_SUJ | FS_ACLS | FS_MULTILABEL | FS_GJOURNAL |
			FS_FLAGS_UPDATED | FS_DOWAPBL | FS_DOQUOTA2 | FS_TRIM);
#ifdef FS_INDEXDIRS
	if (fs->fs_flags & FS_INDEXDIRS)
		printf("indexed directories ");
	fsflags &= ~FS_INDEXDIRS
#endif
	if (fsflags != 0)
		printf("unknown flags (%#x)", fsflags);
	printf("\nfsmnt\t%s\n", fs->fs_fsmnt);
	if (!printold)
		printf("volname\t%s\tswuid\t%ju\n",
		    fs->fs_volname, (uintmax_t)fs->fs_swuid);
	if (printold) {
		if (fs->fs_old_cpc != 0)
			printf("blocks available in each of %d rotational "
			       "positions\n", fs->fs_old_nrpos);
		else
			printf("(no rotational position table)\n\n");
		if (ISOPT(opt_verbose)) {
			int c, j, k;
			for (c = 0; c < fs->fs_old_cpc; c++) {
				printf("cylinder number %d:", c);
				for (i = 0; i < fs->fs_old_nrpos; i++) {
					if (old_fs_postbl(&afs, c, opostbl)[i] == -1)
						continue;
					printf("\n   position %d:\t", i);
					for (j = old_fs_postbl(&afs, c, opostbl)[i], k = 1; ;
							 j += old_fs_rotbl(&afs)[j], k++) {
						printf("%5d", j);
						if (k % 12 == 0)
							printf("\n\t\t");
						if (old_fs_rotbl(&afs)[j] == 0)
							break;
					}
				}
				printf("\n");
			}
		}
	}

	return 0;
}

static int
print_cgsum(const char *name, int fd)
{
	struct csum *ccsp;
	int i, j, size;

	afs.fs_csp = calloc(1, afs.fs_cssize);
	for (i = 0, j = 0; i < afs.fs_cssize; i += afs.fs_bsize, j++) {
		size = afs.fs_cssize - i < afs.fs_bsize ?
		    afs.fs_cssize - i : afs.fs_bsize;
		ccsp = (struct csum *)((char *)afs.fs_csp + i);
		if (lseek(fd,
		    (off_t)(FFS_FSBTODB(&afs, (afs.fs_csaddr + j * afs.fs_frag))) *
		    dev_bsize, SEEK_SET) == (off_t)-1)
			return 1;
		if (read(fd, ccsp, size) != size)
			return 1;
		if (needswap)
			ffs_csum_swap(ccsp, ccsp, size);
	}

	printf("cs[].cs_(nbfree,ndir,nifree,nffree):\n\t");
	for (i = 0; i < afs.fs_ncg; i++) {
		struct csum *cs = &afs.fs_cs(&afs, i);
		if (i && i % 4 == 0)
			printf("\n\t");
		printf("(%d,%d,%d,%d) ",
		    cs->cs_nbfree, cs->cs_ndir, cs->cs_nifree, cs->cs_nffree);
	}
	printf("\n");
	if (printold && (afs.fs_old_ncyl % afs.fs_old_cpg)) {
		printf("cylinders in last group %d\n",
		    i = afs.fs_old_ncyl % afs.fs_old_cpg);
		printf("blocks in last group %d\n",
		    i * afs.fs_old_spc / (afs.fs_old_nspf << afs.fs_fragshift));
	}
	free(afs.fs_csp);
	afs.fs_csp = NULL;

	return 0;
}

static int
print_alt_super(const char *name, int fd)
{
	union fsun alt;
	int i;
	off_t loc;
	uint16_t alt_opostblsave[32*8];
	int save_printold;

	for (i = 0; i < afs.fs_ncg; i++) {
		loc = (off_t)(FFS_FSBTODB(&afs, cgsblock(&afs, i))) * dev_bsize;
		printf("\nalternate %d\n", i);
		if (pread(fd, &alt, sizeof alt, loc) != sizeof alt) {
			warnx("%s: error reading alt %d", name, i);
			return (1);
		}
		save_printold = printold;
		fix_superblock(&alt.fs, alt_opostblsave);
		if (print_superblock(&alt.fs, alt_opostblsave, name, fd, loc)) {
			printold = save_printold;
			return 1;
		}
		printold = save_printold;
	}
	return 0;
}

static int
print_cginfo(const char *name, int fd)
{
	int i;

	printf("\n");
	for (i = 0; i < afs.fs_ncg; i++) {
		printf("\n");
		if (dumpcg(name, fd, i))
			return 1;
		if (ISOPT(opt_inodes) && print_inodes(name, fd, i, 1))
			return 1;
	}
	return 0;
}

static int
print_inodes(const char *name, int fd, int c, int n)
{
	void *ino_buf = malloc(afs.fs_bsize);
	void (*print_inode)(int, int, void *);
	int i, inum;

	if (ino_buf == 0)
		return 1;

	print_inode = is_ufs2 ? print_ufs2_inode : print_ufs1_inode;

	for (inum = c * afs.fs_ipg ; inum < (c+n) * afs.fs_ipg; inum += afs.fs_inopb) {
		if (pread(fd, ino_buf, afs.fs_bsize,
		    ino_to_fsba(&afs, inum) * afs.fs_fsize) != afs.fs_bsize) {
			free(ino_buf);
			return 1;
		}
		for (i = 0; i < afs.fs_inopb; i++)
			print_inode(inum + i, i, ino_buf);
	}

	free(ino_buf);
	return 0;
}

static int
dumpcg(const char *name, int fd, int c)
{
	off_t cur;
	int i, j;
	time_t t;

	printf("cg %d:\n", c);
	if ((cur = lseek(fd, (off_t)(FFS_FSBTODB(&afs, cgtod(&afs, c))) * dev_bsize,
	    SEEK_SET)) == (off_t)-1)
		return (1);
	if (read(fd, &acg, afs.fs_bsize) != afs.fs_bsize) {
		warnx("%s: error reading cg", name);
		return (1);
	}
	if (needswap)
		ffs_cg_swap(&acg, &acg, &afs);
	if (printold)
		t = acg.cg_old_time;
	else
		t = acg.cg_time;
	printf("magic\t%x\ttell\t%llx\ttime\t%s",
	    afs.fs_old_postblformat == FS_42POSTBLFMT ?
	    ((struct ocg *)&acg)->cg_magic : acg.cg_magic,
	    (long long)cur, ctime(&t));
	if (printold)
		printf("cgx\t%d\tncyl\t%d\tniblk\t%d\tndblk\t%d\n",
		    acg.cg_cgx, acg.cg_old_ncyl, acg.cg_old_niblk,
		    acg.cg_ndblk);
	else
		printf("cgx\t%d\tniblk\t%d\tndblk\t%d\n",
		    acg.cg_cgx, acg.cg_niblk, acg.cg_ndblk);
	printf("nbfree\t%d\tndir\t%d\tnifree\t%d\tnffree\t%d\n",
	    acg.cg_cs.cs_nbfree, acg.cg_cs.cs_ndir,
	    acg.cg_cs.cs_nifree, acg.cg_cs.cs_nffree);
	printf("rotor\t%d\tirotor\t%d\tfrotor\t%d\nfrsum",
	    acg.cg_rotor, acg.cg_irotor, acg.cg_frotor);
	for (i = 1, j = 0; i < afs.fs_frag; i++) {
		printf("\t%d", acg.cg_frsum[i]);
		j += i * acg.cg_frsum[i];
	}
	printf("\nsum of frsum: %d", j);
	if (afs.fs_contigsumsize > 0) {
		for (i = 1; i < afs.fs_contigsumsize; i++) {
			if ((i - 1) % 8 == 0)
				printf("\nclusters %d-%d:", i,
				    afs.fs_contigsumsize - 1 < i + 7 ?
				    afs.fs_contigsumsize - 1 : i + 7);
			printf("\t%d", cg_clustersum(&acg, 0)[i]);
		}
		printf("\nclusters size %d and over: %d\n",
		    afs.fs_contigsumsize,
		    cg_clustersum(&acg, 0)[afs.fs_contigsumsize]);
		printf("clusters free:\t");
		pbits(0, cg_clustersfree(&acg, 0), acg.cg_nclusterblks);
	} else
		printf("\n");
	printf("iused:\t");
	pbits(0 * c * afs.fs_ipg, cg_inosused(&acg, 0), afs.fs_ipg);
	printf("free:\t");
	pbits(0, cg_blksfree(&acg, 0), afs.fs_fpg);
	if (printold && ISOPT(opt_verbose)) {
		printf("b:\n");
		for (i = 0; i < afs.fs_old_cpg; i++) {
			if (old_cg_blktot(&acg, 0)[i] == 0)
				continue;
			printf("   c%d:\t(%d)\t", i, old_cg_blktot(&acg, 0)[i]);
			for (j = 0; j < afs.fs_old_nrpos; j++) {
				if (afs.fs_old_cpc > 0 &&
						old_fs_postbl(&afs, i % afs.fs_old_cpc, opostblsave)[j] == -1)
					continue;
				printf(" %d", old_cg_blks(&afs, &acg, i, 0)[j]);
			}
			printf("\n");
		}
	}
	return (0);
}

static void
print_ufs1_inode(int inum, int i_off, void *ibuf)
{
	struct ufs1_dinode *i = ibuf;

	i += i_off;

	if (needswap)
		ffs_dinode1_swap(i,i);

	if (afs.fs_old_inodefmt < FS_44INODEFMT) {
		i->di_uid = i->di_ouid;
		i->di_gid = i->di_ogid;
	}

	if (inum % afs.fs_ipg == 0)
		printf("   inode:   mode  nlink                 size"
		    "      ctime.nsec         flags     blocks"
		    " generation      uid        gid\n");
	if (i->di_mode == 0 && i->di_nlink == 0 && !ISOPT(opt_verbose))
		return;
	printf("%8u: %6o %6d %20" PRIu64 " %10u.%09u %8x %10u %8x %10u %10u\n",
		inum, i->di_mode, i->di_nlink, i->di_size,
		i->di_ctime, i->di_ctimensec, i->di_flags, i->di_blocks,
		i->di_gen, i->di_uid, i->di_gid);
}

static void
print_ufs2_inode(int inum, int i_off, void *ibuf)
{
	struct ufs2_dinode *i = ibuf;

	i += i_off;

	if (needswap)
		ffs_dinode2_swap(i,i);

	if (inum % afs.fs_ipg == 0)
		printf("   inode:   mode  nlink                 size"
		    "      ctime.nsec         flags     blocks"
		    " generation      uid        gid\n");

	if (i->di_mode == 0 && i->di_nlink == 0 && !ISOPT(opt_verbose))
		return;

	printf("%8u: %6o %6d %20" PRIu64 " %10" PRIu64 ".%09u %8x %10" PRIu64 " %8x %10u %10u\n",
		inum, i->di_mode, i->di_nlink, i->di_size,
		i->di_ctime, i->di_ctimensec, i->di_flags, i->di_blocks,
		i->di_gen, i->di_uid, i->di_gid);
}

static void
pbits(int offset, void *vp, int max)
{
	int i;
	char *p;
	int count, j;

	for (count = i = 0, p = vp; i < max; i++)
		if (isset(p, i)) {
			if (count)
				printf(",%s", count % 6 ? " " : "\n\t");
			count++;
			printf("%d", offset + i);
			j = i;
			while ((i+1)<max && isset(p, i+1))
				i++;
			if (i != j)
				printf("-%d", offset + i);
		}
	printf("\n");
}

static int
print_journal(const char *name, int fd)
{
	daddr_t off;
	size_t count, blklen, bno, skip;
	off_t boff, head, tail, len;
	uint32_t generation;

	if (afs.fs_journal_version != UFS_WAPBL_VERSION)
		return 0;

	generation = 0;
	head = tail = 0;

	switch (afs.fs_journal_location) {
	case UFS_WAPBL_JOURNALLOC_END_PARTITION:
	case UFS_WAPBL_JOURNALLOC_IN_FILESYSTEM:

		off    = afs.fs_journallocs[0];
		count  = afs.fs_journallocs[1];
		blklen = afs.fs_journallocs[2];

		for (bno=0; bno<count; bno += skip / blklen) {

			skip = blklen;

			boff = bno * blklen;
			if (bno * blklen >= 2 * blklen &&
			  ((head >= tail && (boff < tail || boff >= head)) ||
			  (head < tail && (boff >= head && boff < tail))))
				continue;

			printf("journal block %lu offset %lld\n",
				(unsigned long)bno, (long long) boff);

			if (lseek(fd, (off_t)(off*blklen) + boff, SEEK_SET)
			    == (off_t)-1)
				return (1);
			if (read(fd, &jbuf, blklen) != (ssize_t)blklen) {
				warnx("%s: error reading journal", name);
				return 1;
			}

			switch (awh.wc_type) {
			case 0:
				break;
			case WAPBL_WC_HEADER:
				print_journal_header(name);
				if (awh.wc_generation > generation) {
					head = awh.wc_head;
					tail = awh.wc_tail;
				}
				generation = awh.wc_generation;
				skip = awh.wc_len;
				break;
			default:
				len = print_journal_entries(name, blklen);
				skip = awh.wc_len;
				if (len != (off_t)skip)
					printf("  CORRUPTED RECORD\n");
				break;
			}

			if (blklen == 0)
				break;

			skip = (skip + blklen - 1) / blklen * blklen;
			if (skip == 0)
				break;

		}
		break;
	}

	return 0;
}

static const char *
wapbl_type_string(unsigned t)
{
	static char buf[12];

	switch (t) {
	case WAPBL_WC_BLOCKS:
		return "blocks";
	case WAPBL_WC_REVOCATIONS:
		return "revocations";
	case WAPBL_WC_INODES:
		return "inodes";
	case WAPBL_WC_HEADER:
		return "header";
	}

	snprintf(buf,sizeof(buf),"%08x",t);
	return buf;
}

static void
print_journal_header(const char *name)
{
	printf("  type %s len %d  version %u\n",
		wapbl_type_string(awh.wc_type), awh.wc_len,
		awh.wc_version);
	printf("  checksum      %08x  generation %9u\n",
		awh.wc_checksum, awh.wc_generation);
	printf("  fsid %08x.%08x  time %llu nsec %u\n",
		awh.wc_fsid[0], awh.wc_fsid[1],
		(unsigned long long)awh.wc_time, awh.wc_timensec);
	printf("  log_bshift  %10u  fs_bshift %10u\n",
		awh.wc_log_dev_bshift, awh.wc_fs_dev_bshift);
	printf("  head        %10lld  tail      %10lld\n",
		(long long)awh.wc_head, (long long)awh.wc_tail);
	printf("  circ_off    %10lld  circ_size %10lld\n",
		(long long)awh.wc_circ_off, (long long)awh.wc_circ_size);
}

static off_t
print_journal_entries(const char *name, size_t blklen)
{
	int i, n;
	struct wapbl_wc_blocklist *wcb;
	struct wapbl_wc_inodelist *wci;
	off_t len = 0;
	int ph;

	printf("  type %s len %d",
		wapbl_type_string(awn.wc_type), awn.wc_len);

	switch (awn.wc_type) {
	case WAPBL_WC_BLOCKS:
	case WAPBL_WC_REVOCATIONS:
		wcb = (struct wapbl_wc_blocklist *)&awn;
		printf("  blkcount %u\n", wcb->wc_blkcount);
		ph = (blklen - offsetof(struct wapbl_wc_blocklist, wc_blocks))
			/ sizeof(wcb->wc_blocks[0]);
		n = MIN(wcb->wc_blkcount, ph);
		for (i=0; i<n; i++) {
			if (ISOPT(opt_verbose)) {
				printf("  %3d: daddr %14llu  dlen %d\n", i,
				 (unsigned long long)wcb->wc_blocks[i].wc_daddr,
				 wcb->wc_blocks[i].wc_dlen);
			}
			len += wcb->wc_blocks[i].wc_dlen;
		}
		if (awn.wc_type == WAPBL_WC_BLOCKS) {
			if (len % blklen)
				len += blklen - len % blklen;
		} else
			len = 0;
		break;
	case WAPBL_WC_INODES:
		wci = (struct wapbl_wc_inodelist *)&awn;
		printf("  count %u clear %u\n",
			wci->wc_inocnt, wci->wc_clear);
		ph = (blklen - offsetof(struct wapbl_wc_inodelist, wc_inodes))
			/ sizeof(wci->wc_inodes[0]);
		n = MIN(wci->wc_inocnt, ph);
		for (i=0; i<n; ++i) {
			if (ISOPT(opt_verbose)) {
				printf("  %3d: inumber %10u  imode %08x\n", i,
					wci->wc_inodes[i].wc_inumber,
					wci->wc_inodes[i].wc_imode);
			}
		}
		break;
	default:
		printf("\n");
		break;
	}

	return len + blklen;
}

static void
usage(void)
{

	(void)fprintf(stderr, "usage: dumpfs [-acFijmsv] filesys | device [...]\n");
	exit(1);
}

static int
openpartition(const char *name, int flags, char *device, size_t devicelen)
{
	char		rawspec[MAXPATHLEN], xbuf[MAXPATHLEN], *p;
	struct fstab	*fs;
	int		fd, oerrno;

	fs = getfsfile(name);
	if (fs) {
		const char *fsspec;
		fsspec = getfsspecname(xbuf, sizeof(xbuf), fs->fs_spec);
		if (fsspec == NULL) {
			warn("%s", xbuf);
			return -1;
		}
		if ((p = strrchr(fsspec, '/')) != NULL) {
			snprintf(rawspec, sizeof(rawspec), "%.*s/r%s",
			    (int)(p - fsspec), fsspec, p + 1);
			name = rawspec;
		} else
			name = fsspec;
	}
	fd = opendisk(name, flags, device, devicelen, 0);
	if (fd == -1 && errno == ENOENT) {
		oerrno = errno;
		strlcpy(device, name, devicelen);
		errno = oerrno;
	}
	return (fd);
}
