/*	$NetBSD: dumpfs.c,v 1.27 2001/08/17 02:18:49 lukem Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1983, 1992, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)dumpfs.c	8.5 (Berkeley) 4/29/95";
#else
__RCSID("$NetBSD: dumpfs.c,v 1.27 2001/08/17 02:18:49 lukem Exp $");
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

union {
	struct fs fs;
	char pad[MAXBSIZE];
} fsun;
#define	afs	fsun.fs

union {
	struct cg cg;
	char pad[MAXBSIZE];
} cgun;
#define	acg	cgun.cg

long	dev_bsize = 1;
int	needswap = 0;

int	dumpfs(const char *);
int	dumpcg(const char *, int, int);
int	main(int, char **);
void	pbits(void *, int);
void	usage(void);
void	swap_cg(struct cg *);

int
main(int argc, char *argv[])
{
	struct fstab *fs;
	int ch, eval;
	int Fflag;

	Fflag = 0;
	while ((ch = getopt(argc, argv, "F")) != -1)
		switch(ch) {
		case 'F':
			Fflag = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage();

	for (eval = 0; *argv; ++argv)
		if (Fflag || ((fs = getfsfile(*argv)) == NULL))
			eval |= dumpfs(*argv);
		else
			eval |= dumpfs(fs->fs_spec);
	exit(eval);
}

int
dumpfs(const char *name)
{
	int fd, c, i, j, k, size;
	time_t t;

	if ((fd = open(name, O_RDONLY, 0)) < 0)
		goto err;
	if (lseek(fd, (off_t)SBOFF, SEEK_SET) == (off_t)-1)
		goto err;
	if (read(fd, &afs, SBSIZE) != SBSIZE)
		goto err;

 	if (afs.fs_magic != FS_MAGIC) {
		if (afs.fs_magic == bswap32(FS_MAGIC)) {
			ffs_sb_swap(&afs, &afs);
			needswap = 1;
		} else {
			warnx("%s: superblock has bad magic number, skipped",
			    name);
			(void)close(fd);
 			return (1);
 		}
 	}
#if BYTE_ORDER == LITTLE_ENDIAN
	if (needswap)
#else
	if (!needswap)
#endif
		printf("endian\tbig-endian\n");
	else
		printf("endian\tlittle-endian\n");
	if (afs.fs_postblformat == FS_42POSTBLFMT)
		afs.fs_nrpos = 8;
	dev_bsize = afs.fs_fsize / fsbtodb(&afs, 1);
	t = afs.fs_time;
	printf("magic\t%x\ttime\t%s", afs.fs_magic,
	    ctime(&t));
	i = 0;
	if (afs.fs_postblformat != FS_42POSTBLFMT) {
		i++;
		if (afs.fs_inodefmt >= FS_44INODEFMT) {
			int max;

			i++;
			max = afs.fs_maxcontig;
			size = afs.fs_contigsumsize;
			if ((max < 2 && size == 0)
			    || (max > 1 && size >= MIN(max, FS_MAXCONTIG)))
				i++;
		}
	}
	printf("cylgrp\t%s\tinodes\t%s\tfslevel %d\tsoftdep %sabled\n",
	    i < 1 ? "static" : "dynamic", i < 2 ? "4.2/4.3BSD" : "4.4BSD", i,
	    (afs.fs_flags & FS_DOSOFTDEP) ? "en" : "dis");
	printf("nbfree\t%d\tndir\t%d\tnifree\t%d\tnffree\t%d\n",
	    afs.fs_cstotal.cs_nbfree, afs.fs_cstotal.cs_ndir,
	    afs.fs_cstotal.cs_nifree, afs.fs_cstotal.cs_nffree);
	printf("ncg\t%d\tncyl\t%d\tsize\t%d\tblocks\t%d\n",
	    afs.fs_ncg, afs.fs_ncyl, afs.fs_size, afs.fs_dsize);
	printf("bsize\t%d\tshift\t%d\tmask\t0x%08x\n",
	    afs.fs_bsize, afs.fs_bshift, afs.fs_bmask);
	printf("fsize\t%d\tshift\t%d\tmask\t0x%08x\n",
	    afs.fs_fsize, afs.fs_fshift, afs.fs_fmask);
	printf("frag\t%d\tshift\t%d\tfsbtodb\t%d\n",
	    afs.fs_frag, afs.fs_fragshift, afs.fs_fsbtodb);
	printf("cpg\t%d\tbpg\t%d\tfpg\t%d\tipg\t%d\n",
	    afs.fs_cpg, afs.fs_fpg / afs.fs_frag, afs.fs_fpg, afs.fs_ipg);
	printf("minfree\t%d%%\toptim\t%s\tmaxcontig %d\tmaxbpg\t%d\n",
	    afs.fs_minfree, afs.fs_optim == FS_OPTSPACE ? "space" : "time",
	    afs.fs_maxcontig, afs.fs_maxbpg);
	printf("rotdelay %dms\theadswitch %dus\ttrackseek %dus\trps\t%d\n",
	    afs.fs_rotdelay, afs.fs_headswitch, afs.fs_trkseek, afs.fs_rps);
	printf("ntrak\t%d\tnsect\t%d\tnpsect\t%d\tspc\t%d\n",
	    afs.fs_ntrak, afs.fs_nsect, afs.fs_npsect, afs.fs_spc);
	printf("symlinklen %d\ttrackskew %d\tinterleave %d\tcontigsumsize %d\n",
	    afs.fs_maxsymlinklen, afs.fs_trackskew, afs.fs_interleave,
	    afs.fs_contigsumsize);
	printf("maxfilesize 0x%016llx\n",
	    (unsigned long long)afs.fs_maxfilesize);
	printf("nindir\t%d\tinopb\t%d\tnspf\t%d\n",
	    afs.fs_nindir, afs.fs_inopb, afs.fs_nspf);
	printf("sblkno\t%d\tcblkno\t%d\tiblkno\t%d\tdblkno\t%d\n",
	    afs.fs_sblkno, afs.fs_cblkno, afs.fs_iblkno, afs.fs_dblkno);
	printf("sbsize\t%d\tcgsize\t%d\toffset\t%d\tmask\t0x%08x\n",
	    afs.fs_sbsize, afs.fs_cgsize, afs.fs_cgoffset, afs.fs_cgmask);
	printf("csaddr\t%d\tcssize\t%d\tshift\t%d\tmask\t0x%08x\n",
	    afs.fs_csaddr, afs.fs_cssize, afs.fs_csshift, afs.fs_csmask);
	printf("cgrotor\t%d\tfmod\t%d\tronly\t%d\tclean\t0x%02x\n",
	    afs.fs_cgrotor, afs.fs_fmod, afs.fs_ronly, afs.fs_clean);
	if (afs.fs_cpc != 0)
		printf("blocks available in each of %d rotational positions",
		     afs.fs_nrpos);
	else
		printf("insufficient space to maintain rotational tables\n");
	for (c = 0; c < afs.fs_cpc; c++) {
		printf("\ncylinder number %d:", c);
		for (i = 0; i < afs.fs_nrpos; i++) {
			if (fs_postbl(&afs, c)[i] == -1)
				continue;
			printf("\n   position %d:\t", i);
			for (j = fs_postbl(&afs, c)[i], k = 1; ;
			     j += fs_rotbl(&afs)[j], k++) {
				printf("%5d", j);
				if (k % 12 == 0)
					printf("\n\t\t");
				if (fs_rotbl(&afs)[j] == 0)
					break;
			}
		}
	}
	printf("\ncs[].cs_(nbfree,ndir,nifree,nffree):\n\t");
	for (i = 0, j = 0; i < afs.fs_cssize; i += afs.fs_bsize, j++) {
		size = afs.fs_cssize - i < afs.fs_bsize ?
		    afs.fs_cssize - i : afs.fs_bsize;
		afs.fs_csp[j] = calloc(1, size);
		if (lseek(fd,
		    (off_t)(fsbtodb(&afs, (afs.fs_csaddr + j * afs.fs_frag))) *
		    dev_bsize, SEEK_SET) == (off_t)-1)
			goto err;
		if (read(fd, afs.fs_csp[j], size) != size)
			goto err;
		if (needswap)
			ffs_csum_swap(afs.fs_csp[j], afs.fs_csp[j], size);
	}
	for (i = 0; i < afs.fs_ncg; i++) {
		struct csum *cs = &afs.fs_cs(&afs, i);
		if (i && i % 4 == 0)
			printf("\n\t");
		printf("(%d,%d,%d,%d) ",
		    cs->cs_nbfree, cs->cs_ndir, cs->cs_nifree, cs->cs_nffree);
	}
	printf("\n");
	if (afs.fs_ncyl % afs.fs_cpg) {
		printf("cylinders in last group %d\n",
		    i = afs.fs_ncyl % afs.fs_cpg);
		printf("blocks in last group %d\n",
		    i * afs.fs_spc / NSPB(&afs));
	}
	printf("\n");
	for (i = 0; i < afs.fs_ncg; i++)
		if (dumpcg(name, fd, i))
			goto err;
	(void)close(fd);
	return (0);

err:	if (fd != -1)
		(void)close(fd);
	warn("%s", name);
	return (1);
};

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
		swap_cg(&acg);
	t = acg.cg_time;
	printf("magic\t%x\ttell\t%llx\ttime\t%s",
	    afs.fs_postblformat == FS_42POSTBLFMT ?
	    ((struct ocg *)&acg)->cg_magic : acg.cg_magic,
	    (long long)cur, ctime(&t));
	printf("cgx\t%d\tncyl\t%d\tniblk\t%d\tndblk\t%d\n",
	    acg.cg_cgx, acg.cg_ncyl, acg.cg_niblk, acg.cg_ndblk);
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
		pbits(cg_clustersfree(&acg, 0), acg.cg_nclusterblks);
	} else
		printf("\n");
	printf("iused:\t");
	pbits(cg_inosused(&acg, 0), afs.fs_ipg);
	printf("free:\t");
	pbits(cg_blksfree(&acg, 0), afs.fs_fpg);
	printf("b:\n");
	for (i = 0; i < afs.fs_cpg; i++) {
		if (cg_blktot(&acg, 0)[i] == 0)
			continue;
		printf("   c%d:\t(%d)\t", i, cg_blktot(&acg, 0)[i]);
		for (j = 0; j < afs.fs_nrpos; j++) {
			if (afs.fs_cpc > 0 &&
			    fs_postbl(&afs, i % afs.fs_cpc)[j] == -1)
				continue;
			printf(" %d", cg_blks(&afs, &acg, i, 0)[j]);
		}
		printf("\n");
	}
	return (0);
};

void
pbits(void *vp, int max)
{
	int i;
	char *p;
	int count, j;

	for (count = i = 0, p = vp; i < max; i++)
		if (isset(p, i)) {
			if (count)
				printf(",%s", count % 6 ? " " : "\n\t");
			count++;
			printf("%d", i);
			j = i;
			while ((i+1)<max && isset(p, i+1))
				i++;
			if (i != j)
				printf("-%d", i);
		}
	printf("\n");
}

void
usage(void)
{

	(void)fprintf(stderr, "usage: dumpfs [-F] filesys | device [...]\n");
	exit(1);
}

void
swap_cg(struct cg *cg)
{
	int i;
	u_int32_t *n32;
	u_int16_t *n16;

	cg->cg_firstfield = bswap32(cg->cg_firstfield);
	cg->cg_magic = bswap32(cg->cg_magic);
	cg->cg_time = bswap32(cg->cg_time);
	cg->cg_cgx = bswap32(cg->cg_cgx);
	cg->cg_ncyl = bswap16(cg->cg_ncyl);
	cg->cg_niblk = bswap16(cg->cg_niblk);
	cg->cg_ndblk = bswap32(cg->cg_ndblk);
	cg->cg_cs.cs_ndir = bswap32(cg->cg_cs.cs_ndir);
	cg->cg_cs.cs_nbfree = bswap32(cg->cg_cs.cs_nbfree);
	cg->cg_cs.cs_nifree = bswap32(cg->cg_cs.cs_nifree);
	cg->cg_cs.cs_nffree = bswap32(cg->cg_cs.cs_nffree);
	cg->cg_rotor = bswap32(cg->cg_rotor);
	cg->cg_frotor = bswap32(cg->cg_frotor);
	cg->cg_irotor = bswap32(cg->cg_irotor);
	cg->cg_btotoff = bswap32(cg->cg_btotoff);
	cg->cg_boff = bswap32(cg->cg_boff);
	cg->cg_iusedoff = bswap32(cg->cg_iusedoff);
	cg->cg_freeoff = bswap32(cg->cg_freeoff);
	cg->cg_nextfreeoff = bswap32(cg->cg_nextfreeoff);
	cg->cg_clustersumoff = bswap32(cg->cg_clustersumoff);
	cg->cg_clusteroff = bswap32(cg->cg_clusteroff);
	cg->cg_nclusterblks = bswap32(cg->cg_nclusterblks);
	for (i=0; i < MAXFRAG; i++)
		cg->cg_frsum[i] = bswap32(cg->cg_frsum[i]);

	if (afs.fs_postblformat == FS_42POSTBLFMT) { /* old format */
		struct ocg *ocg;
		int j;
		ocg = (struct ocg *)cg;
		for(i = 0; i < 8; i++) {
			ocg->cg_frsum[i] = bswap32(ocg->cg_frsum[i]);
		}
		for(i = 0; i < 32; i++) {
			ocg->cg_btot[i] = bswap32(ocg->cg_btot[i]);
			for (j = 0; j < 8; j++)
				ocg->cg_b[i][j] = bswap16(ocg->cg_b[i][j]);
		}
		ocg->cg_magic = bswap32(ocg->cg_magic);
	} else {  /* new format */
		if (cg->cg_magic == CG_MAGIC) {
			n32 = (u_int32_t*)((u_int8_t*)cg + cg->cg_btotoff);
			n16 = (u_int16_t*)((u_int8_t*)cg + cg->cg_boff);
		} else {
			n32 = (u_int32_t*)((u_int8_t*)cg + bswap32(cg->cg_btotoff));
			n16 = (u_int16_t*)((u_int8_t*)cg + bswap32(cg->cg_boff));
		}
		for (i=0; i< afs.fs_cpg; i++)
			n32[i] = bswap32(n32[i]);
		
		for (i=0; i < afs.fs_cpg * afs.fs_nrpos; i++)
			n16[i] = bswap16(n16[i]);

		if (cg->cg_magic == CG_MAGIC) {
			n32 = (u_int32_t*)((u_int8_t*)cg + cg->cg_clustersumoff);
		} else {
			n32 = (u_int32_t*)((u_int8_t*)cg + cg->cg_clustersumoff);
		}
		for (i = 0; i < afs.fs_contigsumsize + 1; i++)
			n32[i] = bswap32(n32[i]);
	}
}
