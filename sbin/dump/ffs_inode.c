/*	$NetBSD: ffs_inode.c,v 1.22.20.1 2019/03/29 19:43:28 martin Exp $ */

/*-
 * Copyright (c) 1980, 1991, 1993, 1994
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1991, 1993, 1994\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
__RCSID("$NetBSD: ffs_inode.c,v 1.22.20.1 2019/03/29 19:43:28 martin Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>
#include <ufs/ufs/ufs_bswap.h>

#include <protocols/dumprestore.h>

#include <ctype.h>
#include <errno.h>
#include <fts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dump.h"

static struct fs *sblock;

static off_t sblock_try[] = SBLOCKSEARCH;

int is_ufs2;

/*
 * Read the superblock from disk, and check its magic number.
 * Determine whether byte-swapping needs to be done on this file system.
 */
int
fs_read_sblock(char *superblock)
{
	int i;
	int ns = 0;

	sblock = (struct fs *)superblock;
	for (i = 0; ; i++) {
		if (sblock_try[i] == -1)
			quit("can't find superblock");
		rawread(sblock_try[i], (char *)superblock, MAXBSIZE);

		switch(sblock->fs_magic) {
		case FS_UFS2_MAGIC:
			is_ufs2 = 1;
			/*FALLTHROUGH*/
		case FS_UFS1_MAGIC:
			break;
		case FS_UFS2_MAGIC_SWAPPED:
			is_ufs2 = 1;
			/*FALLTHROUGH*/
		case FS_UFS1_MAGIC_SWAPPED:
			ns = 1;
			ffs_sb_swap(sblock, sblock);
			break;
                default:
                        continue;
                }
		if (!is_ufs2 && sblock_try[i] == SBLOCK_UFS2)
			continue;
		if ((is_ufs2 || sblock->fs_old_flags & FS_FLAGS_UPDATED)
		    && sblock_try[i] != sblock->fs_sblockloc)
			continue;
		break;
        }
	return ns;
}

/*
 * Fill in the ufsi struct, as well as the maxino and dev_bsize global
 * variables.
 */
struct ufsi *
fs_parametrize(void)
{
	static struct ufsi ufsi;

#ifdef FS_44INODEFMT
	if (is_ufs2 || sblock->fs_old_inodefmt >= FS_44INODEFMT) {
		spcl.c_flags = iswap32(iswap32(spcl.c_flags) | DR_NEWINODEFMT);
	} else {
		/*
		 * Determine parameters for older file systems. From
		 *	/sys/ufs/ffs/ffs_vfsops.c::ffs_oldfscompat()
		 *
		 * XXX: not sure if other variables (fs_npsect, fs_interleave,
		 * fs_nrpos, fs_maxfilesize) need to be fudged too.
		 */
		sblock->fs_qbmask = ~sblock->fs_bmask;
		sblock->fs_qfmask = ~sblock->fs_fmask;
	}
#endif

	/* Fill out ufsi struct */
	ufsi.ufs_dsize = FFS_FSBTODB(sblock,sblock->fs_size);
	ufsi.ufs_bsize = sblock->fs_bsize;
	ufsi.ufs_bshift = sblock->fs_bshift;
	ufsi.ufs_fsize = sblock->fs_fsize;
	ufsi.ufs_frag = sblock->fs_frag;
	ufsi.ufs_fsatoda = sblock->fs_fsbtodb;
	ufsi.ufs_nindir = sblock->fs_nindir;
	ufsi.ufs_inopb = sblock->fs_inopb;
	ufsi.ufs_maxsymlinklen = sblock->fs_maxsymlinklen;
	ufsi.ufs_bmask = sblock->fs_bmask;
	ufsi.ufs_fmask = sblock->fs_fmask;
	ufsi.ufs_qbmask = sblock->fs_qbmask;
	ufsi.ufs_qfmask = sblock->fs_qfmask;

	dev_bsize = sblock->fs_fsize / FFS_FSBTODB(sblock, 1);

	return &ufsi;
}

ino_t
fs_maxino(void)
{

	return sblock->fs_ipg * sblock->fs_ncg;
}

void
fs_mapinodes(ino_t maxino __unused, u_int64_t *tape_size, int *anydirskipped)
{
	int i, cg, inosused;
	struct cg *cgp;
	ino_t ino;
	char *cp;

	if ((cgp = malloc(sblock->fs_cgsize)) == NULL)
		quite(errno, "fs_mapinodes: cannot allocate memory.");

	for (cg = 0; cg < sblock->fs_ncg; cg++) {
		ino = cg * sblock->fs_ipg;
		bread(FFS_FSBTODB(sblock, cgtod(sblock, cg)), (char *)cgp,
		    sblock->fs_cgsize);
		if (needswap)
			ffs_cg_swap(cgp, cgp, sblock);
		if (sblock->fs_magic == FS_UFS2_MAGIC)
			inosused = cgp->cg_initediblk;
		else
			inosused = sblock->fs_ipg;
		/*
		 * If we are using soft updates, then we can trust the
		 * cylinder group inode allocation maps to tell us which
		 * inodes are allocated. We will scan the used inode map
		 * to find the inodes that are really in use, and then
		 * read only those inodes in from disk.
		 */
		if (sblock->fs_flags & FS_DOSOFTDEP) {
			if (!cg_chkmagic(cgp, 0))
				quit("%s: cg %d: bad magic number\n",
				    __func__, cg);
			cp = &cg_inosused(cgp, 0)[(inosused - 1) / CHAR_BIT];
			for ( ; inosused > 0; inosused -= CHAR_BIT, cp--) {
				if (*cp == 0)
					continue;
				for (i = 1 << (CHAR_BIT - 1); i > 0; i >>= 1) {
					if (*cp & i)
						break;
					inosused--;
				}
				break;
			}
			if (inosused <= 0)
				continue;
		}
		for (i = 0; i < inosused; i++, ino++) {
			if (ino < UFS_ROOTINO)
				continue;
			mapfileino(ino, tape_size, anydirskipped);
		}
	}

	free(cgp);
}

union dinode *
getino(ino_t inum)
{
	static ino_t minino, maxino;
	static caddr_t inoblock;
	int ntoswap, i;
	struct ufs1_dinode *dp1;
	struct ufs2_dinode *dp2;

	if (inoblock == NULL && (inoblock = malloc(ufsib->ufs_bsize)) == NULL)
		quite(errno, "cannot allocate inode memory.");
	curino = inum;
	if (inum >= minino && inum < maxino)
		goto gotit;
	bread(fsatoda(ufsib, ino_to_fsba(sblock, inum)), (char *)inoblock,
	    (int)ufsib->ufs_bsize);
	minino = inum - (inum % FFS_INOPB(sblock));
	maxino = minino + FFS_INOPB(sblock);
	if (needswap) {
		if (is_ufs2) {
			dp2 = (struct ufs2_dinode *)inoblock;
			ntoswap = ufsib->ufs_bsize / sizeof(struct ufs2_dinode);
			for (i = 0; i < ntoswap; i++)
				ffs_dinode2_swap(&dp2[i], &dp2[i]);
		} else {
			dp1 = (struct ufs1_dinode *)inoblock;
			ntoswap = ufsib->ufs_bsize / sizeof(struct ufs1_dinode);
			for (i = 0; i < ntoswap; i++)
				ffs_dinode1_swap(&dp1[i], &dp1[i]);
		}
	}
gotit:
	if (is_ufs2) {
		dp2 = &((struct ufs2_dinode *)inoblock)[inum - minino];
		return (union dinode *)dp2;
	}
	dp1 = &((struct ufs1_dinode *)inoblock)[inum - minino];
	return ((union dinode *)dp1);
}
