/*	$NetBSD: ffs_subr.c,v 1.49.16.1 2018/07/28 04:38:12 pgoyette Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *
 *	@(#)ffs_subr.c	8.5 (Berkeley) 3/21/95
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ffs_subr.c,v 1.49.16.1 2018/07/28 04:38:12 pgoyette Exp $");

#include <sys/param.h>

/* in ffs_tables.c */
extern const int inside[], around[];
extern const u_char * const fragtbl[];

#ifndef _KERNEL
#define FFS_EI /* always include byteswapped filesystems support */
#endif
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>
#include <ufs/ufs/ufs_bswap.h>

#ifndef _KERNEL
#include <ufs/ufs/dinode.h>
void    panic(const char *, ...)
    __attribute__((__noreturn__,__format__(__printf__,1,2)));

#else	/* _KERNEL */
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/inttypes.h>
#include <sys/pool.h>
#include <sys/fstrans.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

/*
 * Load up the contents of an inode and copy the appropriate pieces
 * to the incore copy.
 */
void
ffs_load_inode(struct buf *bp, struct inode *ip, struct fs *fs, ino_t ino)
{
	struct ufs1_dinode *dp1;
	struct ufs2_dinode *dp2;

	if (ip->i_ump->um_fstype == UFS1) {
		dp1 = (struct ufs1_dinode *)bp->b_data + ino_to_fsbo(fs, ino);
#ifdef FFS_EI
		if (UFS_FSNEEDSWAP(fs))
			ffs_dinode1_swap(dp1, ip->i_din.ffs1_din);
		else
#endif
		*ip->i_din.ffs1_din = *dp1;

		ip->i_mode = ip->i_ffs1_mode;
		ip->i_nlink = ip->i_ffs1_nlink;
		ip->i_size = ip->i_ffs1_size;
		ip->i_flags = ip->i_ffs1_flags;
		ip->i_gen = ip->i_ffs1_gen;
		ip->i_uid = ip->i_ffs1_uid;
		ip->i_gid = ip->i_ffs1_gid;
	} else {
		dp2 = (struct ufs2_dinode *)bp->b_data + ino_to_fsbo(fs, ino);
#ifdef FFS_EI
		if (UFS_FSNEEDSWAP(fs))
			ffs_dinode2_swap(dp2, ip->i_din.ffs2_din);
		else
#endif
		*ip->i_din.ffs2_din = *dp2;

		ip->i_mode = ip->i_ffs2_mode;
		ip->i_nlink = ip->i_ffs2_nlink;
		ip->i_size = ip->i_ffs2_size;
		ip->i_flags = ip->i_ffs2_flags;
		ip->i_gen = ip->i_ffs2_gen;
		ip->i_uid = ip->i_ffs2_uid;
		ip->i_gid = ip->i_ffs2_gid;
	}
}

int
ffs_getblk(struct vnode *vp, daddr_t lblkno, daddr_t blkno, int size,
    bool clearbuf, buf_t **bpp)
{
	int error = 0;

	KASSERT(blkno >= 0 || blkno == FFS_NOBLK);

	if ((*bpp = getblk(vp, lblkno, size, 0, 0)) == NULL)
		return ENOMEM;
	if (blkno != FFS_NOBLK)
		(*bpp)->b_blkno = blkno;
	if (clearbuf)
		clrbuf(*bpp);
	if ((*bpp)->b_blkno >= 0 && (error = fscow_run(*bpp, false)) != 0) {
		brelse(*bpp, BC_INVAL);
		*bpp = NULL;
	}
	return error;
}

#endif	/* _KERNEL */

/*
 * Update the frsum fields to reflect addition or deletion
 * of some frags.
 */
void
ffs_fragacct(struct fs *fs, int fragmap, int32_t fraglist[], int cnt,
    int needswap)
{
	int inblk;
	int field, subfield;
	int siz, pos;

	inblk = (int)(fragtbl[fs->fs_frag][fragmap]) << 1;
	fragmap <<= 1;
	for (siz = 1; siz < fs->fs_frag; siz++) {
		if ((inblk & (1 << (siz + (fs->fs_frag & (NBBY - 1))))) == 0)
			continue;
		field = around[siz];
		subfield = inside[siz];
		for (pos = siz; pos <= fs->fs_frag; pos++) {
			if ((fragmap & field) == subfield) {
				fraglist[siz] = ufs_rw32(
				    ufs_rw32(fraglist[siz], needswap) + cnt,
				    needswap);
				pos += siz;
				field <<= siz;
				subfield <<= siz;
			}
			field <<= 1;
			subfield <<= 1;
		}
	}
}

/*
 * block operations
 *
 * check if a block is available
 *  returns true if all the correponding bits in the free map are 1
 *  returns false if any corresponding bit in the free map is 0
 */
int
ffs_isblock(struct fs *fs, u_char *cp, int32_t h)
{
	u_char mask;

	switch ((int)fs->fs_fragshift) {
	case 3:
		return (cp[h] == 0xff);
	case 2:
		mask = 0x0f << ((h & 0x1) << 2);
		return ((cp[h >> 1] & mask) == mask);
	case 1:
		mask = 0x03 << ((h & 0x3) << 1);
		return ((cp[h >> 2] & mask) == mask);
	case 0:
		mask = 0x01 << (h & 0x7);
		return ((cp[h >> 3] & mask) == mask);
	default:
		panic("ffs_isblock: unknown fs_fragshift %d",
		    (int)fs->fs_fragshift);
	}
}

/*
 * check if a block is completely allocated
 *  returns true if all the corresponding bits in the free map are 0
 *  returns false if any corresponding bit in the free map is 1
 */
int
ffs_isfreeblock(struct fs *fs, u_char *cp, int32_t h)
{

	switch ((int)fs->fs_fragshift) {
	case 3:
		return (cp[h] == 0);
	case 2:
		return ((cp[h >> 1] & (0x0f << ((h & 0x1) << 2))) == 0);
	case 1:
		return ((cp[h >> 2] & (0x03 << ((h & 0x3) << 1))) == 0);
	case 0:
		return ((cp[h >> 3] & (0x01 << (h & 0x7))) == 0);
	default:
		panic("ffs_isfreeblock: unknown fs_fragshift %d",
		    (int)fs->fs_fragshift);
	}
}

/*
 * take a block out of the map
 */
void
ffs_clrblock(struct fs *fs, u_char *cp, int32_t h)
{

	switch ((int)fs->fs_fragshift) {
	case 3:
		cp[h] = 0;
		return;
	case 2:
		cp[h >> 1] &= ~(0x0f << ((h & 0x1) << 2));
		return;
	case 1:
		cp[h >> 2] &= ~(0x03 << ((h & 0x3) << 1));
		return;
	case 0:
		cp[h >> 3] &= ~(0x01 << (h & 0x7));
		return;
	default:
		panic("ffs_clrblock: unknown fs_fragshift %d",
		    (int)fs->fs_fragshift);
	}
}

/*
 * put a block into the map
 */
void
ffs_setblock(struct fs *fs, u_char *cp, int32_t h)
{

	switch ((int)fs->fs_fragshift) {
	case 3:
		cp[h] = 0xff;
		return;
	case 2:
		cp[h >> 1] |= (0x0f << ((h & 0x1) << 2));
		return;
	case 1:
		cp[h >> 2] |= (0x03 << ((h & 0x3) << 1));
		return;
	case 0:
		cp[h >> 3] |= (0x01 << (h & 0x7));
		return;
	default:
		panic("ffs_setblock: unknown fs_fragshift %d",
		    (int)fs->fs_fragshift);
	}
}

/*
 * Update the cluster map because of an allocation or free.
 *
 * Cnt == 1 means free; cnt == -1 means allocating.
 */
void
ffs_clusteracct(struct fs *fs, struct cg *cgp, int32_t blkno, int cnt)
{
	int32_t *sump;
	int32_t *lp;
	u_char *freemapp, *mapp;
	int i, start, end, forw, back, map;
	unsigned int bit;
	const int needswap = UFS_FSNEEDSWAP(fs);

	/* KASSERT(mutex_owned(&ump->um_lock)); */

	if (fs->fs_contigsumsize <= 0)
		return;
	freemapp = cg_clustersfree(cgp, needswap);
	sump = cg_clustersum(cgp, needswap);
	/*
	 * Allocate or clear the actual block.
	 */
	if (cnt > 0)
		setbit(freemapp, blkno);
	else
		clrbit(freemapp, blkno);
	/*
	 * Find the size of the cluster going forward.
	 */
	start = blkno + 1;
	end = start + fs->fs_contigsumsize;
	if ((uint32_t)end >= ufs_rw32(cgp->cg_nclusterblks, needswap))
		end = ufs_rw32(cgp->cg_nclusterblks, needswap);
	mapp = &freemapp[start / NBBY];
	map = *mapp++;
	bit = 1U << (start % NBBY);
	for (i = start; i < end; i++) {
		if ((map & bit) == 0)
			break;
		if ((i & (NBBY - 1)) != (NBBY - 1)) {
			bit <<= 1;
		} else {
			map = *mapp++;
			bit = 1;
		}
	}
	forw = i - start;
	/*
	 * Find the size of the cluster going backward.
	 */
	start = blkno - 1;
	end = start - fs->fs_contigsumsize;
	if (end < 0)
		end = -1;
	mapp = &freemapp[start / NBBY];
	map = *mapp--;
	bit = 1U << (start % NBBY);
	for (i = start; i > end; i--) {
		if ((map & bit) == 0)
			break;
		if ((i & (NBBY - 1)) != 0) {
			bit >>= 1;
		} else {
			map = *mapp--;
			bit = 1U << (NBBY - 1);
		}
	}
	back = start - i;
	/*
	 * Account for old cluster and the possibly new forward and
	 * back clusters.
	 */
	i = back + forw + 1;
	if (i > fs->fs_contigsumsize)
		i = fs->fs_contigsumsize;
	ufs_add32(sump[i], cnt, needswap);
	if (back > 0)
		ufs_add32(sump[back], -cnt, needswap);
	if (forw > 0)
		ufs_add32(sump[forw], -cnt, needswap);

	/*
	 * Update cluster summary information.
	 */
	lp = &sump[fs->fs_contigsumsize];
	for (i = fs->fs_contigsumsize; i > 0; i--)
		if (ufs_rw32(*lp--, needswap) > 0)
			break;
#if defined(_KERNEL)
	fs->fs_maxcluster[ufs_rw32(cgp->cg_cgx, needswap)] = i;
#endif
}
