/*      $NetBSD: ffs_inode.c,v 1.3 1999/10/01 04:35:22 perseant Exp $ */

/*-
 * Copyright (c) 1980, 1991, 1993, 1994
 *      The Regents of the University of California.  All rights reserved.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1991, 1993, 1994\n\
        The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)main.c      8.6 (Berkeley) 5/1/95";
#else
__RCSID("$NetBSD: ffs_inode.c,v 1.3 1999/10/01 04:35:22 perseant Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>
#ifdef sunos
#include <sys/vnode.h>

#include <ufs/fs.h>
#include <ufs/fsdir.h>
#include <ufs/inode.h>
#else
#include <ufs/ufs/dir.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>
#endif

#include <protocols/dumprestore.h>

#include <ctype.h>
#include <errno.h>
#include <fts.h>
#include <stdio.h>
#ifdef __STDC__
#include <string.h>
#include <unistd.h>
#endif

#include "dump.h"

#ifndef SBOFF
#define SBOFF (SBLOCK * DEV_BSIZE)
#endif

struct fs *sblock;

/*
 * Read the superblock from disk, and check its magic number.
 * Determine whether byte-swapping needs to be done on this filesystem.
 */
int
fs_read_sblock(char *sblock_buf)
{
	int needswap = 0;

	sblock = (struct fs *)sblock_buf;
	rawread(SBOFF, (char *) sblock, SBSIZE);
	if (sblock->fs_magic != FS_MAGIC) {
		if (sblock->fs_magic == bswap32(FS_MAGIC)) {
			ffs_sb_swap(sblock, sblock, 0);
			needswap = 1;
		} else
			quit("bad sblock magic number\n");
	}
	return needswap;
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
	if (sblock->fs_inodefmt >= FS_44INODEFMT) {
		spcl.c_flags = iswap32(iswap32(spcl.c_flags) | DR_NEWINODEFMT);
	} else {
		/*
		 * Determine parameters for older filesystems. From
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
	ufsi.ufs_dsize = fsbtodb(sblock,sblock->fs_size);
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

	dev_bsize = sblock->fs_fsize / fsbtodb(sblock, 1);

	return &ufsi;
}

ino_t
fs_maxino(void)
{
	return sblock->fs_ipg * sblock->fs_ncg;
}

struct dinode *
getino(inum)
	ino_t inum;
{
	static daddr_t minino, maxino;
	static struct dinode inoblock[MAXINOPB];
	int i;

	curino = inum;
	if (inum >= minino && inum < maxino)
		return (&inoblock[inum - minino]);
	bread(fsatoda(ufsib, ino_to_fsba(sblock, inum)), (char *)inoblock,
	    (int)ufsib->ufs_bsize);
	if (needswap)
		for (i = 0; i < MAXINOPB; i++)
			ffs_dinode_swap(&inoblock[i], &inoblock[i]);
	minino = inum - (inum % INOPB(sblock));
	maxino = minino + INOPB(sblock);
	return (&inoblock[inum - minino]);
}
