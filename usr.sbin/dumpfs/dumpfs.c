/*	$NetBSD: dumpfs.c,v 1.39 2003/09/26 07:02:43 dsl Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1983, 1992, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)dumpfs.c	8.5 (Berkeley) 4/29/95";
#else
__RCSID("$NetBSD: dumpfs.c,v 1.39 2003/09/26 07:02:43 dsl Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/ufs_bswap.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fstab.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

union fsun {
	struct fs fs;
	char pad[MAXBSIZE];
} fsun;
#define	afs	fsun.fs

union {
	struct cg cg;
	char pad[MAXBSIZE];
} cgun;
#define	acg	cgun.cg

#define OPT_FLAG(ch)	(1 << ((ch) & 31))
#define ISOPT(opt)	(opt_flags & (opt))
#define opt_alt_super	OPT_FLAG('a')
#define opt_superblock	OPT_FLAG('s')
#define opt_cg_summary	OPT_FLAG('m')
#define opt_cg_info	OPT_FLAG('c')
#define opt_inodes	OPT_FLAG('i')
#define opt_verbose	OPT_FLAG('v')
#define DFLT_OPTS	(opt_superblock | opt_cg_summary | opt_cg_info)

long	dev_bsize = 512;
int	needswap, printold, is_ufs2;
int	Fflag;

uint	opt_flags;

int	dumpfs(const char *);
int	print_superblock(struct fs *, const char *, int, off_t);
int	print_cgsum(const char *, int);
int	print_cginfo(const char *, int);
int	print_inodes(const char *, int);
int	print_alt_super(const char *, int);
int	dumpcg(const char *, int, int);
int	main(int, char **);
int	openpartition(const char *, int, char *, size_t);
void	pbits(int, void *, int);
void	usage(void);
void	swap_cg(struct cg *);
void	print_ufs1_inode(int, int, void *);
void	print_ufs2_inode(int, int, void *);
void	fix_superblock(struct fs *);

int
main(int argc, char *argv[])
{
	int ch, eval;

	while ((ch = getopt(argc, argv, "Facimsv")) != -1)
		switch(ch) {
		case 'a':	/* alternate suberblocks */
		case 'c':	/* cylinder group info */
		case 'i':	/* actual inodes */
		case 'm':	/* cylinder group summary */
		case 's':	/* superblock */
		case 'v':	/* more verbose */
			opt_flags |= OPT_FLAG(ch);
			break;
		case 'F':	/* File (not device) */
			Fflag = 1;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (opt_flags == 0)
		opt_flags = DFLT_OPTS;

	if (argc < 1)
		usage();

	for (eval = 0; *argv; ++argv) {
		eval |= dumpfs(*argv);
		printf("\n");
	}

	exit(eval);
}


int
dumpfs(const char *name)
{
	const static off_t sblock_try[] = SBLOCKSEARCH;
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
		break;
	}

	fix_superblock(&afs);

	dev_bsize = afs.fs_fsize / fsbtodb(&afs, 1);

	rval = 0;
	printf("file system: %s\n", name);

	if (ISOPT(opt_superblock))
		rval = print_superblock(&afs, name, fd, sblock_try[i]);
	if (rval == 0 && ISOPT(opt_alt_super))
		rval = print_alt_super(name, fd);
	if (rval == 0 && ISOPT(opt_cg_summary))
		rval = print_cgsum(name, fd);
	if (rval == 0 && ISOPT(opt_cg_info))
		rval = print_cginfo(name, fd);
	if (rval == 0 && ISOPT(opt_inodes))
		rval = print_inodes(name, fd);

    err:
	if (fd != -1)
		(void)close(fd);
	if (rval)
		warn("%s", name);
	return rval;
}

void
fix_superblock(struct fs *fs)
{

	if (needswap)
		ffs_sb_swap(fs, fs);

	printold = (fs->fs_magic == FS_UFS1_MAGIC &&
		    (fs->fs_old_flags & FS_FLAGS_UPDATED) == 0);
	if (printold) {
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

int
print_superblock(struct fs *fs, const char *name, int fd, off_t sblock)
{
	int i, size;
	time_t t;

#if BYTE_ORDER == LITTLE_ENDIAN
	if (needswap)
#else
	if (!needswap)
#endif
		printf("endian\tbig-endian\n");
	else
		printf("endian\tlittle-endian\n");
	t = fs->fs_time;
	printf("location%lld\tmagic\t%x\ttime\t%s", (long long)sblock,
	    fs->fs_magic, ctime(&t));

	if (is_ufs2)
		i = 4;
	else {
		i = 0;
		if (!printold && fs->fs_old_postblformat != FS_42POSTBLFMT) {
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
		}
	}
	printf("id\t[ %x %x ]\n", fs->fs_id[0], fs->fs_id[1]);
	printf("cylgrp\t%s\tinodes\t%s\tfslevel %d\tsoftdep %sabled\n",
	    i < 1 ? "static" : "dynamic",
	    i < 2 ? "4.2/4.3BSD" : i < 4 ? "4.4BSD" : "FFSv2", i,
	    (fs->fs_flags & FS_DOSOFTDEP) ? "en" : "dis");
	printf("nbfree\t%lld\tndir\t%lld\tnifree\t%lld\tnffree\t%lld\n",
	    (long long)fs->fs_cstotal.cs_nbfree,
	    (long long)fs->fs_cstotal.cs_ndir,
	    (long long)fs->fs_cstotal.cs_nifree,
	    (long long)fs->fs_cstotal.cs_nffree);
	printf("ncg\t%d\tsize\t%lld\tblocks\t%lld\n",
	    fs->fs_ncg, (long long)fs->fs_size, (long long)fs->fs_dsize);
	if (printold)
		printf("ncyl\t%d\n", fs->fs_old_ncyl);
	printf("bsize\t%d\tshift\t%d\tmask\t0x%08x\n",
	    fs->fs_bsize, fs->fs_bshift, fs->fs_bmask);
	printf("fsize\t%d\tshift\t%d\tmask\t0x%08x\n",
	    fs->fs_fsize, fs->fs_fshift, fs->fs_fmask);
	printf("frag\t%d\tshift\t%d\tfsbtodb\t%d\n",
	    fs->fs_frag, fs->fs_fragshift, fs->fs_fsbtodb);
	printf("bpg\t%d\tfpg\t%d\tipg\t%d\n",
	    fs->fs_fpg / fs->fs_frag, fs->fs_fpg, fs->fs_ipg);
	printf("minfree\t%d%%\toptim\t%s\tmaxcontig %d\tmaxbpg\t%d\n",
	    fs->fs_minfree, fs->fs_optim == FS_OPTSPACE ? "space" : "time",
	    fs->fs_maxcontig, fs->fs_maxbpg);
	if (printold) {
		printf("cpg\t%d\trotdelay %dms\trps\t%d\n",
		    fs->fs_old_cpg, fs->fs_old_rotdelay, fs->fs_old_rps);
		printf("ntrak\t%d\tnsect\t%d\tnpsect\t%d\tspc\t%d\n",
		    fs->fs_spare2, fs->fs_old_nsect, fs->fs_old_npsect,
		    fs->fs_old_spc);
		printf("trackskew %d\tinterleave %d\n",
		    fs->fs_old_trackskew, fs->fs_old_interleave);
	}
	printf("symlinklen %d\tcontigsumsize %d\n",
	    fs->fs_maxsymlinklen, fs->fs_contigsumsize);
	printf("maxfilesize 0x%016llx\n",
	    (unsigned long long)fs->fs_maxfilesize);
	printf("nindir\t%d\tinopb\t%d\n", fs->fs_nindir, fs->fs_inopb);
	if (printold)
		printf("nspf\t%d\n", fs->fs_old_nspf);
	printf("avgfilesize %d\tavgfpdir %d\n",
	    fs->fs_avgfilesize, fs->fs_avgfpdir);
	printf("sblkno\t%d\tcblkno\t%d\tiblkno\t%d\tdblkno\t%d\n",
	    fs->fs_sblkno, fs->fs_cblkno, fs->fs_iblkno, fs->fs_dblkno);
	printf("sbsize\t%d\tcgsize\t%d\n", fs->fs_sbsize, fs->fs_cgsize);
	if (printold)
		printf("offset\t%d\tmask\t0x%08x\n",
		    fs->fs_old_cgoffset, fs->fs_old_cgmask);
	printf("csaddr\t%lld\tcssize %d\n",
	    (long long)fs->fs_csaddr, fs->fs_cssize);
	if (printold)
		printf("shift\t%d\tmask\t0x%08x\n",
		    fs->fs_spare1[0], fs->fs_spare1[1]);
	printf("cgrotor\t%d\tfmod\t%d\tronly\t%d\tclean\t0x%02x\n",
	    fs->fs_cgrotor, fs->fs_fmod, fs->fs_ronly, fs->fs_clean);
	if (printold) {
		if (fs->fs_old_cpc != 0)
			printf("blocks available in each of %d rotational "
			       "positions", fs->fs_old_nrpos);
		else
			printf("(no rotational position table)\n");
	}

	return 0;
}

int
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
		    (off_t)(fsbtodb(&afs, (afs.fs_csaddr + j * afs.fs_frag))) *
		    dev_bsize, SEEK_SET) == (off_t)-1)
			return 1;
		if (read(fd, ccsp, size) != size)
			return 1;
		if (needswap)
			ffs_csum_swap(ccsp, ccsp, size);
	}

	printf("\ncs[].cs_(nbfree,ndir,nifree,nffree):\n\t");
	for (i = 0; i < afs.fs_ncg; i++) {
		struct csum *cs = &afs.fs_cs(&afs, i);
		if (i && i % 4 == 0)
			printf("\n\t");
		printf("(%d,%d,%d,%d) ",
		    cs->cs_nbfree, cs->cs_ndir, cs->cs_nifree, cs->cs_nffree);
	}
	if (i % 4 == 0)
		printf("\n");

	if (printold && (afs.fs_old_ncyl % afs.fs_old_cpg)) {
		printf("cylinders in last group %d\n",
		    i = afs.fs_old_ncyl % afs.fs_old_cpg);
		printf("blocks in last group %d\n",
		    i * afs.fs_old_spc / (afs.fs_old_nspf << afs.fs_fragshift));
	}
	printf("\n");
	free(afs.fs_csp);
	afs.fs_csp = NULL;

	return 0;
}

int
print_alt_super(const char *name, int fd)
{
	union fsun alt;
	int i;
	off_t loc;

	for (i = 0; i < afs.fs_ncg; i++) {
		printf("\nalternate %d:\n", i);
		loc = fsbtodb(&afs, cgtod(&afs, i)) * dev_bsize - afs.fs_bsize;
		if (pread(fd, &alt, sizeof alt, loc) != sizeof alt) {
			warnx("%s: error reading alt %d", name, i);
			return (1);
		}
		if (print_superblock(&alt.fs, name, fd, loc))
			return 1;
	}
	return 0;
}

int
print_cginfo(const char *name, int fd)
{
	int i;

	for (i = 0; i < afs.fs_ncg; i++)
		if (dumpcg(name, fd, i))
			return 1;
	return 0;
}

int
print_inodes(const char *name, int fd)
{
	void *ino_buf = malloc(afs.fs_bsize);
	void (*print_inode)(int, int, void *);
	int i, inum;

	if (ino_buf == 0)
		return 1;

	print_inode = is_ufs2 ? print_ufs2_inode : print_ufs1_inode;

	for (inum = 0; inum < afs.fs_ncg * afs.fs_ipg; inum += afs.fs_inopb) {
		if (pread(fd, ino_buf, afs.fs_bsize,
		    ino_to_fsba(&afs, inum) * afs.fs_fsize) != afs.fs_bsize)
			return 1;
		for (i = 0; i < afs.fs_inopb; i++)
			print_inode(inum + i, i, ino_buf);
	}

	free(ino_buf);
	return 0;
}

int
dumpcg(const char *name, int fd, int c)
{
	off_t cur;
	int i, j;
	time_t t;

	printf("\ncg %d:\n", c);
	if ((cur = lseek(fd, (off_t)(fsbtodb(&afs, cgtod(&afs, c))) * dev_bsize,
	    SEEK_SET)) == (off_t)-1)
		return (1);
	if (read(fd, &acg, afs.fs_bsize) != afs.fs_bsize) {
		warnx("%s: error reading cg", name);
		return (1);
	}
	if (needswap)
		ffs_cg_swap(&acg, &acg, &afs);
	t = acg.cg_time;
	printf("magic\t%x\ttell\t%llx\ttime\t%s",
	    afs.fs_old_postblformat == FS_42POSTBLFMT ?
	    ((struct ocg *)&acg)->cg_magic : acg.cg_magic,
	    (long long)cur, ctime(&t));
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
	pbits(c * afs.fs_ipg, cg_inosused(&acg, 0), afs.fs_ipg);
	printf("free:\t");
	pbits(0, cg_blksfree(&acg, 0), afs.fs_fpg);
	return (0);
}

void
print_ufs1_inode(int inum, int i_off, void *ibuf)
{
	struct ufs1_dinode *i = ibuf;

	i += i_off;

	if (inum == 0)
		printf("\n   inode:   mode  nlink                 size"
		    "      ctime.nsec         flags     blocks"
		    " generation      uid        gid\n");
	if (i->di_mode == 0 && i->di_nlink == 0 && !ISOPT(opt_verbose))
		return;
	printf("%8u: %6o %6d %20" PRIu64 " %10u.%09u %8x %10u %8x %10u %10u\n",
		inum, i->di_mode, i->di_nlink, i->di_size,
		i->di_ctime, i->di_ctimensec, i->di_flags, i->di_blocks,
		i->di_gen, i->di_uid, i->di_gid);
}

void
print_ufs2_inode(int inum, int i_off, void *ibuf)
{
	struct ufs2_dinode *i = ibuf;

	i += i_off;

	if (inum == 0)
		printf("\n   inode:   mode  nlink                 size"
		    "      ctime.nsec         flags     blocks"
		    " generation      uid        gid\n");

	if (i->di_mode == 0 && i->di_nlink == 0 && !ISOPT(opt_verbose))
		return;

	printf("%8u: %6o %6d %20" PRIu64 " %10" PRIu64 ".%09u %8x %10" PRIu64 " %8x %10u %10u\n",
		inum, i->di_mode, i->di_nlink, i->di_size,
		i->di_ctime, i->di_ctimensec, i->di_flags, i->di_blocks,
		i->di_gen, i->di_uid, i->di_gid);
}


void
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

void
usage(void)
{

	(void)fprintf(stderr, "usage: dumpfs [-acFimsv] filesys | device [...]\n");
	exit(1);
}

int
openpartition(const char *name, int flags, char *device, size_t devicelen)
{
	char		rawspec[MAXPATHLEN], *p;
	struct fstab	*fs;
	int		fd, oerrno;

	fs = getfsfile(name);
	if (fs) {
		if ((p = strrchr(fs->fs_spec, '/')) != NULL) {
			snprintf(rawspec, sizeof(rawspec), "%.*s/r%s",
			    (int)(p - fs->fs_spec), fs->fs_spec, p + 1);
			name = rawspec;
		} else
			name = fs->fs_spec;
	}
	fd = opendisk(name, flags, device, devicelen, 0);
	if (fd == -1 && errno == ENOENT) {
		oerrno = errno;
		strlcpy(device, name, devicelen);
		errno = oerrno;
	}
	return (fd);
}
