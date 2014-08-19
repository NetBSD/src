/*	$NetBSD: pass5.c,v 1.50.2.2 2014/08/20 00:02:24 tls Exp $	*/

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
static char sccsid[] = "@(#)pass5.c	8.9 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: pass5.c,v 1.50.2.2 2014/08/20 00:02:24 tls Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>
#include <ufs/ufs/ufs_bswap.h>

#include <err.h>
#include <string.h>
#include <stdlib.h>

#include "fsutil.h"
#include "fsck.h"
#include "extern.h"

void print_bmap(u_char *,u_int32_t);

void
pass5(void)
{
	int c, blk, frags, basesize, sumsize, mapsize, cssize;
	int inomapsize, blkmapsize;
	struct fs *fs = sblock;
	daddr_t dbase, dmax;
	daddr_t d;
	long i, j, k;
	struct csum *cs;
	struct csum_total cstotal;
	struct inodesc idesc[4];
	char buf[MAXBSIZE];
	struct cg *newcg = (struct cg *)buf;
	struct ocg *ocg = (struct ocg *)buf;
	struct cg *cg = cgrp, *ncg;
	struct inostat *info;
	u_int32_t ncgsize;

	inoinfo(UFS_WINO)->ino_state = USTATE;
	memset(newcg, 0, (size_t)fs->fs_cgsize);
	newcg->cg_niblk = fs->fs_ipg;
	if (cvtlevel >= 3) {
		if (fs->fs_maxcontig < 2 && fs->fs_contigsumsize > 0) {
			if (preen)
				pwarn("DELETING CLUSTERING MAPS\n");
			if (preen || reply("DELETE CLUSTERING MAPS")) {
				fs->fs_contigsumsize = 0;
				doinglevel1 = 1;
				sbdirty();
			}
		}
		if (fs->fs_maxcontig > 1) {
			const char *doit = NULL;

			if (fs->fs_contigsumsize < 1) {
				doit = "CREAT";
			} else if (fs->fs_contigsumsize < fs->fs_maxcontig &&
				   fs->fs_contigsumsize < FS_MAXCONTIG) {
				doit = "EXPAND";
			}
			if (doit) {
				i = fs->fs_contigsumsize;
				fs->fs_contigsumsize =
				    MIN(fs->fs_maxcontig, FS_MAXCONTIG);
				if (CGSIZE(fs) > (uint32_t)fs->fs_bsize) {
					pwarn("CANNOT %s CLUSTER MAPS\n", doit);
					fs->fs_contigsumsize = i;
				} else if (preen ||
				    reply("CREATE CLUSTER MAPS")) {
					if (preen)
						pwarn("%sING CLUSTER MAPS\n",
						    doit);
					ncgsize = ffs_fragroundup(fs, CGSIZE(fs));
					ncg = realloc(cgrp, ncgsize);
					if (ncg == NULL)
						errexit(
						"cannot reallocate cg space");
					cg = cgrp = ncg;
					fs->fs_cgsize = ncgsize;
					doinglevel1 = 1;
					sbdirty();
				}
			}
		}
	}
	basesize = &newcg->cg_space[0] - (u_char *)(&newcg->cg_firstfield);
	cssize = (u_char *)&cstotal.cs_spare[0] - (u_char *)&cstotal.cs_ndir;
	sumsize = 0;
	if (is_ufs2) {
		newcg->cg_iusedoff = basesize;
	} else {
		/*
		 * We reserve the space for the old rotation summary
		 * tables for the benefit of old kernels, but do not
		 * maintain them in modern kernels. In time, they can
		 * go away.
		 */
		newcg->cg_old_btotoff = basesize;
		newcg->cg_old_boff = newcg->cg_old_btotoff +
		    fs->fs_old_cpg * sizeof(int32_t);
		newcg->cg_iusedoff = newcg->cg_old_boff +
		    fs->fs_old_cpg * fs->fs_old_nrpos * sizeof(u_int16_t);
		memset(&newcg->cg_space[0], 0, newcg->cg_iusedoff - basesize);
	}
	inomapsize = howmany(fs->fs_ipg, CHAR_BIT);
	newcg->cg_freeoff = newcg->cg_iusedoff + inomapsize;
	blkmapsize = howmany(fs->fs_fpg, CHAR_BIT);
	newcg->cg_nextfreeoff = newcg->cg_freeoff + blkmapsize;
	if (fs->fs_contigsumsize > 0) {
		newcg->cg_clustersumoff = newcg->cg_nextfreeoff -
		    sizeof(u_int32_t);
		if (isappleufs) {
			/* Apple PR2216969 gives rationale for this change.
			 * I believe they were mistaken, but we need to
			 * duplicate it for compatibility.  -- dbj@NetBSD.org
			 */
			newcg->cg_clustersumoff += sizeof(u_int32_t);
		}
		newcg->cg_clustersumoff =
		    roundup(newcg->cg_clustersumoff, sizeof(u_int32_t));
		newcg->cg_clusteroff = newcg->cg_clustersumoff +
		    (fs->fs_contigsumsize + 1) * sizeof(u_int32_t);
		newcg->cg_nextfreeoff = newcg->cg_clusteroff +
		    howmany(ffs_fragstoblks(fs, fs->fs_fpg), CHAR_BIT);
	}
	newcg->cg_magic = CG_MAGIC;
	mapsize = newcg->cg_nextfreeoff - newcg->cg_iusedoff;
	if (!is_ufs2 && ((fs->fs_old_flags & FS_FLAGS_UPDATED) == 0)) {
		switch ((int)fs->fs_old_postblformat) {

		case FS_42POSTBLFMT:
			basesize = (char *)(&ocg->cg_btot[0]) -
			    (char *)(&ocg->cg_firstfield);
			sumsize = &ocg->cg_iused[0] - (u_int8_t *)(&ocg->cg_btot[0]);
			mapsize = &ocg->cg_free[howmany(fs->fs_fpg, NBBY)] -
			    (u_char *)&ocg->cg_iused[0];
			blkmapsize = howmany(fs->fs_fpg, NBBY);
			inomapsize = &ocg->cg_free[0] - (u_char *)&ocg->cg_iused[0];
			ocg->cg_magic = CG_MAGIC;
			newcg->cg_magic = 0;
			break;

		case FS_DYNAMICPOSTBLFMT:
			sumsize = newcg->cg_iusedoff - newcg->cg_old_btotoff;
			break;

		default:
			errexit("UNKNOWN ROTATIONAL TABLE FORMAT %d",
			    fs->fs_old_postblformat);
		}
	}
	memset(&idesc[0], 0, sizeof idesc);
	for (i = 0; i < 4; i++) {
		idesc[i].id_type = ADDR;
		if (!is_ufs2 && doinglevel2)
			idesc[i].id_fix = FIX;
	}
	memset(&cstotal, 0, sizeof(struct csum_total));
	dmax = ffs_blknum(fs, fs->fs_size + fs->fs_frag - 1);
	for (d = fs->fs_size; d < dmax; d++)
		setbmap(d);
	for (c = 0; c < fs->fs_ncg; c++) {
		if (got_siginfo) {
			fprintf(stderr,
			    "%s: phase 5: cyl group %d of %d (%d%%)\n",
			    cdevname(), c, fs->fs_ncg,
			    c * 100 / fs->fs_ncg);
			got_siginfo = 0;
		}
#ifdef PROGRESS
		progress_bar(cdevname(), preen ? NULL : "phase 5",
			    c, fs->fs_ncg);
#endif /* PROGRESS */
		getblk(&cgblk, cgtod(fs, c), fs->fs_cgsize);
		memcpy(cg, cgblk.b_un.b_cg, fs->fs_cgsize);
		if((doswap && !needswap) || (!doswap && needswap))
			ffs_cg_swap(cgblk.b_un.b_cg, cg, sblock);
		if (!doinglevel1 && !cg_chkmagic(cg, 0))
			pfatal("CG %d: PASS5: BAD MAGIC NUMBER\n", c);
		if(doswap)
			cgdirty();
		/*
		 * While we have the disk head where we want it,
		 * write back the superblock to the spare at this
		 * cylinder group.
		 */
		if ((cvtlevel && sblk.b_dirty) || doswap) {
			bwrite(fswritefd, sblk.b_un.b_buf,
			    FFS_FSBTODB(sblock, cgsblock(sblock, c)),
			    sblock->fs_sbsize);
		} else {
			/*
			 * Read in the current alternate superblock,
			 * and compare it to the master.  If it's
			 * wrong, fix it up.
			 */
			getblk(&asblk, cgsblock(sblock, c), sblock->fs_sbsize);
			if (asblk.b_errs)
				pfatal("CG %d: UNABLE TO READ ALTERNATE "
				    "SUPERBLK\n", c);
			else {
				memmove(altsblock, asblk.b_un.b_fs,
				    sblock->fs_sbsize);
				if (needswap)
					ffs_sb_swap(asblk.b_un.b_fs, altsblock);
			}
			sb_oldfscompat_write(sblock, sblocksave);
			if ((asblk.b_errs || cmpsblks(sblock, altsblock)) &&
			     dofix(&idesc[3],
				   "ALTERNATE SUPERBLK(S) ARE INCORRECT")) {
				bwrite(fswritefd, sblk.b_un.b_buf,
				    FFS_FSBTODB(sblock, cgsblock(sblock, c)),
				    sblock->fs_sbsize);
			}
			sb_oldfscompat_read(sblock, 0);
		}
		dbase = cgbase(fs, c);
		dmax = dbase + fs->fs_fpg;
		if (dmax > fs->fs_size)
			dmax = fs->fs_size;
		if (is_ufs2 || (fs->fs_old_flags & FS_FLAGS_UPDATED))
			newcg->cg_time = cg->cg_time;
		newcg->cg_old_time = cg->cg_old_time;
		newcg->cg_cgx = c;
		newcg->cg_ndblk = dmax - dbase;
		if (!is_ufs2) {
			if (c == fs->fs_ncg - 1) {
				/* Avoid fighting old fsck for this value.  Its never used
				 * outside of this check anyway.
				 */
				if ((fs->fs_old_flags & FS_FLAGS_UPDATED) == 0)
					newcg->cg_old_ncyl = fs->fs_old_ncyl % fs->fs_old_cpg;
				else
					newcg->cg_old_ncyl = howmany(newcg->cg_ndblk,
					    fs->fs_fpg / fs->fs_old_cpg);
			} else
				newcg->cg_old_ncyl = fs->fs_old_cpg;
			newcg->cg_old_niblk = fs->fs_ipg;
			newcg->cg_niblk = 0;
		}
		if (fs->fs_contigsumsize > 0)
			newcg->cg_nclusterblks = newcg->cg_ndblk / fs->fs_frag;
		newcg->cg_cs.cs_ndir = 0;
		newcg->cg_cs.cs_nffree = 0;
		newcg->cg_cs.cs_nbfree = 0;
		newcg->cg_cs.cs_nifree = fs->fs_ipg;
		if (cg->cg_rotor >= 0 && cg->cg_rotor < newcg->cg_ndblk)
			newcg->cg_rotor = cg->cg_rotor;
		else
			newcg->cg_rotor = 0;
		if (cg->cg_frotor >= 0 && cg->cg_frotor < newcg->cg_ndblk)
			newcg->cg_frotor = cg->cg_frotor;
		else
			newcg->cg_frotor = 0;
		if (cg->cg_irotor >= 0 && cg->cg_irotor < fs->fs_ipg)
			newcg->cg_irotor = cg->cg_irotor;
		else
			newcg->cg_irotor = 0;
		if (!is_ufs2) {
			newcg->cg_initediblk = 0;
		} else {
			if ((unsigned)cg->cg_initediblk > (unsigned)fs->fs_ipg)
				newcg->cg_initediblk = fs->fs_ipg;
			else
				newcg->cg_initediblk = cg->cg_initediblk;
		}
		memset(&newcg->cg_frsum[0], 0, sizeof newcg->cg_frsum);
		memset(&old_cg_blktot(newcg, 0)[0], 0, (size_t)(sumsize));
		memset(cg_inosused(newcg, 0), 0, (size_t)(mapsize));
		if (!is_ufs2 && ((fs->fs_old_flags & FS_FLAGS_UPDATED) == 0) &&
		    fs->fs_old_postblformat == FS_42POSTBLFMT)
			ocg->cg_magic = CG_MAGIC;
		j = fs->fs_ipg * c;
		for (i = 0; i < fs->fs_ipg; j++, i++) {
			info = inoinfo(j);
			switch (info->ino_state) {

			case USTATE:
				break;

			case DSTATE:
			case DCLEAR:
			case DFOUND:
				newcg->cg_cs.cs_ndir++;
				/* fall through */

			case FSTATE:
			case FCLEAR:
				newcg->cg_cs.cs_nifree--;
				setbit(cg_inosused(newcg, 0), i);
				break;

			default:
				if ((ino_t)j < UFS_ROOTINO)
					break;
				errexit("BAD STATE %d FOR INODE I=%ld",
				    info->ino_state, (long)j);
			}
		}
		if (c == 0)
			for (i = 0; i < (long)UFS_ROOTINO; i++) {
				setbit(cg_inosused(newcg, 0), i);
				newcg->cg_cs.cs_nifree--;
			}
		for (i = 0, d = dbase;
		     d < dmax;
		     d += fs->fs_frag, i += fs->fs_frag) {
			frags = 0;
			for (j = 0; j < fs->fs_frag; j++) {
				if (testbmap(d + j))
					continue;
				setbit(cg_blksfree(newcg, 0), i + j);
				frags++;
			}
			if (frags == fs->fs_frag) {
				newcg->cg_cs.cs_nbfree++;
				if (sumsize) {
					j = old_cbtocylno(fs, i);
					old_cg_blktot(newcg, 0)[j]++;
					old_cg_blks(fs, newcg, j, 0)[old_cbtorpos(fs, i)]++;
				}
				if (fs->fs_contigsumsize > 0)
					setbit(cg_clustersfree(newcg, 0),
					    ffs_fragstoblks(fs, i));
			} else if (frags > 0) {
				newcg->cg_cs.cs_nffree += frags;
				blk = blkmap(fs, cg_blksfree(newcg, 0), i);
				ffs_fragacct(fs, blk, newcg->cg_frsum, 1, 0);
			}
		}
		if (fs->fs_contigsumsize > 0) {
			int32_t *sump = cg_clustersum(newcg, 0);
			u_char *mapp = cg_clustersfree(newcg, 0);
			int map = *mapp++;
			int bit = 1;
			int run = 0;

			for (i = 0; i < newcg->cg_nclusterblks; i++) {
				if ((map & bit) != 0) {
					run++;
				} else if (run != 0) {
					if (run > fs->fs_contigsumsize)
						run = fs->fs_contigsumsize;
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
				if (run > fs->fs_contigsumsize)
					run = fs->fs_contigsumsize;
				sump[run]++;
			}
		}
		cstotal.cs_nffree += newcg->cg_cs.cs_nffree;
		cstotal.cs_nbfree += newcg->cg_cs.cs_nbfree;
		cstotal.cs_nifree += newcg->cg_cs.cs_nifree;
		cstotal.cs_ndir += newcg->cg_cs.cs_ndir;
		cs = &fs->fs_cs(fs, c);
		if (memcmp(&newcg->cg_cs, cs, sizeof *cs) != 0) {
			if (debug) {
				printf("cg %d: nffree: %d/%d nbfree %d/%d"
					" nifree %d/%d ndir %d/%d\n",
					c, cs->cs_nffree,newcg->cg_cs.cs_nffree,
					cs->cs_nbfree,newcg->cg_cs.cs_nbfree,
					cs->cs_nifree,newcg->cg_cs.cs_nifree,
					cs->cs_ndir,newcg->cg_cs.cs_ndir);
			}
			if (dofix(&idesc[0], "FREE BLK COUNT(S) WRONG IN SUPERBLK")) {
				memmove(cs, &newcg->cg_cs, sizeof *cs);
				sbdirty();
			} else
				markclean = 0;
		}
		if (doinglevel1) {
			memmove(cg, newcg, (size_t)fs->fs_cgsize);
			cgdirty();
			continue;
		}
		if ((memcmp(newcg, cg, basesize) != 0) ||
		    (memcmp(&old_cg_blktot(newcg, 0)[0],
		        &old_cg_blktot(cg, 0)[0], sumsize) != 0)) {
		 	if (dofix(&idesc[2], "SUMMARY INFORMATION BAD")) {
				memmove(cg, newcg, (size_t)basesize);
				memmove(&old_cg_blktot(cg, 0)[0],
			       &old_cg_blktot(newcg, 0)[0], (size_t)sumsize);
				cgdirty();
			} else
				markclean = 0;
		}
		if (usedsoftdep) {
			for (i = 0; i < inomapsize; i++) {
				j = cg_inosused(newcg, 0)[i];
				if ((cg_inosused(cg, 0)[i] & j) == j)
					continue;
				for (k = 0; k < NBBY; k++) {
					if ((j & (1 << k)) == 0)
						continue;
					if (cg_inosused(cg, 0)[i] & (1 << k))
						continue;
					pwarn("ALLOCATED INODE %ld "
					    "MARKED FREE\n",
					    c * fs->fs_ipg + i * 8 + k);
				}
			}
			for (i = 0; i < blkmapsize; i++) {
				j = cg_blksfree(cg, 0)[i];
				if ((cg_blksfree(newcg, 0)[i] & j) == j)
					continue;
				for (k = 0; k < NBBY; k++) {
					if ((j & (1 << k)) == 0)
						continue;
					if (cg_inosused(cg, 0)[i] & (1 << k))
						continue;
					pwarn("ALLOCATED FRAG %ld "
					    "MARKED FREE\n",
					    c * fs->fs_fpg + i * 8 + k);
				}
			}
		}
		if (memcmp(cg_inosused(newcg, 0), cg_inosused(cg, 0), mapsize)
		    != 0 && dofix(&idesc[1], "BLK(S) MISSING IN BIT MAPS")) {
			memmove(cg_inosused(cg, 0), cg_inosused(newcg, 0),
			    (size_t)mapsize);
                        cgdirty();
                }
	}
	if (memcmp(&cstotal, &fs->fs_cstotal, cssize) != 0) {
		if (debug) {
			printf("total: nffree: %lld/%lld nbfree %lld/%lld"
				" nifree %lld/%lld ndir %lld/%lld\n",
				(long long int)fs->fs_cstotal.cs_nffree,
				(long long int)cstotal.cs_nffree,
				(long long int)fs->fs_cstotal.cs_nbfree,
				(long long int)cstotal.cs_nbfree,
				(long long int)fs->fs_cstotal.cs_nifree,
				(long long int)cstotal.cs_nifree,
				(long long int)fs->fs_cstotal.cs_ndir,
				(long long int)cstotal.cs_ndir);
		}
		if (dofix(&idesc[0], "FREE BLK COUNT(S) WRONG IN SUPERBLK")) {
			memmove(&fs->fs_cstotal, &cstotal, sizeof cstotal);
			fs->fs_ronly = 0;
			fs->fs_fmod = 0;
			sbdirty();
		} else
			markclean = 0;
	}
#ifdef PROGRESS
	if (!preen)
		progress_done();
#endif /* PROGRESS */
}

void 
print_bmap(u_char *map, uint32_t size)
{
	uint32_t i, j;

	i = 0;
	while (i < size) {
		printf("%u: ",i);
		for (j = 0; j < 16; j++, i++)
			printf("%2x ", (u_int)map[i] & 0xff);
		printf("\n");
	}
}
