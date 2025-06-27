/*	$NetBSD: ext2fs_alloc.c,v 1.59 2025/06/27 19:55:38 andvar Exp $	*/

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
 *	@(#)ffs_alloc.c	8.11 (Berkeley) 10/27/94
 *  Modified for ext2fs by Manuel Bouyer.
 */

/*
 * Copyright (c) 1997 Manuel Bouyer.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	@(#)ffs_alloc.c	8.11 (Berkeley) 10/27/94
 *  Modified for ext2fs by Manuel Bouyer.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ext2fs_alloc.c,v 1.59 2025/06/27 19:55:38 andvar Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/kauth.h>

#include <lib/libkern/crc16.h>

#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufs_extern.h>
#include <ufs/ufs/ufsmount.h>

#include <ufs/ext2fs/ext2fs.h>
#include <ufs/ext2fs/ext2fs_extern.h>

u_long ext2gennumber;

static daddr_t	ext2fs_alloccg(struct inode *, int, daddr_t, int);
static u_long	ext2fs_dirpref(struct m_ext2fs *);
static void	ext2fs_fserr(struct m_ext2fs *, u_int, const char *);
static u_long	ext2fs_hashalloc(struct inode *, int, long, int,
		    daddr_t (*)(struct inode *, int, daddr_t, int));
static daddr_t	ext2fs_nodealloccg(struct inode *, int, daddr_t, int);
static daddr_t	ext2fs_mapsearch(struct m_ext2fs *, char *, daddr_t);
static __inline void	ext2fs_cg_update(struct m_ext2fs *, int,
    struct ext2_gd *, int, int, int, daddr_t);
static uint16_t ext2fs_cg_get_csum(struct m_ext2fs *, int, struct ext2_gd *);
static void	ext2fs_init_bb(struct m_ext2fs *, int, struct ext2_gd *,
    char *);

/*
 * Allocate a block in the file system.
 *
 * A preference may be optionally specified. If a preference is given
 * the following hierarchy is used to allocate a block:
 *   1) allocate the requested block.
 *   2) allocate a rotationally optimal block in the same cylinder.
 *   3) allocate a block in the same cylinder group.
 *   4) quadratically rehash into other cylinder groups, until an
 *	  available block is located.
 * If no block preference is given the following hierarchy is used
 * to allocate a block:
 *   1) allocate a block in the cylinder group that contains the
 *	  inode for the file.
 *   2) quadratically rehash into other cylinder groups, until an
 *	  available block is located.
 */
int
ext2fs_alloc(struct inode *ip, daddr_t lbn, daddr_t bpref,
    kauth_cred_t cred, daddr_t *bnp)
{
	struct m_ext2fs *fs;
	daddr_t bno;
	int cg;

	*bnp = 0;
	fs = ip->i_e2fs;
#ifdef DIAGNOSTIC
	if (cred == NOCRED)
		panic("ext2fs_alloc: missing credential");
#endif /* DIAGNOSTIC */
	if (fs->e2fs.e2fs_fbcount == 0)
		goto nospace;
	if (kauth_authorize_system(cred, KAUTH_SYSTEM_FS_RESERVEDSPACE, 0, NULL,
	    NULL, NULL) != 0 &&
	    freespace(fs) <= 0)
		goto nospace;
	if (bpref >= fs->e2fs.e2fs_bcount)
		bpref = 0;
	if (bpref == 0)
		cg = ino_to_cg(fs, ip->i_number);
	else
		cg = dtog(fs, bpref);
	bno = (daddr_t)ext2fs_hashalloc(ip, cg, bpref, fs->e2fs_bsize,
	    ext2fs_alloccg);
	if (bno > 0) {
		ext2fs_setnblock(ip, ext2fs_nblock(ip) + btodb(fs->e2fs_bsize));
		ip->i_flag |= IN_CHANGE | IN_UPDATE;
		*bnp = bno;
		return 0;
	}
nospace:
	ext2fs_fserr(fs, kauth_cred_geteuid(cred), "file system full");
	uprintf("\n%s: write failed, file system is full\n", fs->e2fs_fsmnt);
	return ENOSPC;
}

/*
 * Allocate an inode in the file system.
 *
 * If allocating a directory, use ext2fs_dirpref to select the inode.
 * If allocating in a directory, the following hierarchy is followed:
 *   1) allocate the preferred inode.
 *   2) allocate an inode in the same cylinder group.
 *   3) quadratically rehash into other cylinder groups, until an
 *	  available inode is located.
 * If no inode preference is given the following hierarchy is used
 * to allocate an inode:
 *   1) allocate an inode in cylinder group 0.
 *   2) quadratically rehash into other cylinder groups, until an
 *	  available inode is located.
 */
int
ext2fs_valloc(struct vnode *pvp, int mode, kauth_cred_t cred, ino_t *inop)
{
	struct inode *pip;
	struct m_ext2fs *fs;
	ino_t ino, ipref;
	int cg;

	pip = VTOI(pvp);
	fs = pip->i_e2fs;
	if (fs->e2fs.e2fs_ficount == 0)
		goto noinodes;

	if ((mode & IFMT) == IFDIR)
		cg = ext2fs_dirpref(fs);
	else
		cg = ino_to_cg(fs, pip->i_number);
	ipref = cg * fs->e2fs.e2fs_ipg + 1;
	ino = (ino_t)ext2fs_hashalloc(pip, cg, (long)ipref, mode, ext2fs_nodealloccg);
	if (ino == 0)
		goto noinodes;

	*inop = ino;
	return 0;

noinodes:
	ext2fs_fserr(fs, kauth_cred_geteuid(cred), "out of inodes");
	uprintf("\n%s: create/symlink failed, no inodes free\n", fs->e2fs_fsmnt);
	return ENOSPC;
}

/*
 * Find a cylinder to place a directory.
 *
 * The policy implemented by this algorithm is to select from
 * among those cylinder groups with above the average number of
 * free inodes, the one with the smallest number of directories.
 */
static u_long
ext2fs_dirpref(struct m_ext2fs *fs)
{
	int cg, maxspace, mincg, avgifree;

	avgifree = fs->e2fs.e2fs_ficount / fs->e2fs_ncg;
	maxspace = 0;
	mincg = -1;
	for (cg = 0; cg < fs->e2fs_ncg; cg++) {
		uint32_t nifree =
		    (fs2h16(fs->e2fs_gd[cg].ext2bgd_nifree_hi) << 16)
		    | fs2h16(fs->e2fs_gd[cg].ext2bgd_nifree);
		if (nifree < avgifree) {
			continue;
		}
		uint32_t nbfree =
		    (fs2h16(fs->e2fs_gd[cg].ext2bgd_nbfree_hi) << 16)
		    | fs2h16(fs->e2fs_gd[cg].ext2bgd_nbfree);
		if (mincg == -1 || nbfree > maxspace) {
			mincg = cg;
			maxspace = nbfree;
		}
	}
	return mincg;
}

/*
 * Select the desired position for the next block in a file.  The file is
 * logically divided into sections. The first section is composed of the
 * direct blocks. Each additional section contains fs_maxbpg blocks.
 *
 * If no blocks have been allocated in the first section, the policy is to
 * request a block in the same cylinder group as the inode that describes
 * the file. Otherwise, the policy is to try to allocate the blocks
 * contiguously. The two fields of the ext2 inode extension (see
 * ufs/ufs/inode.h) help this.
 */
daddr_t
ext2fs_blkpref(struct inode *ip, daddr_t lbn, int indx,
		int32_t *bap /* XXX ondisk32 */)
{
	struct m_ext2fs *fs;
	int cg, i;

	fs = ip->i_e2fs;
	/*
	 * if we are doing contiguous lbn allocation, try to alloc blocks
	 * contiguously on disk
	 */

	if ( ip->i_e2fs_last_blk && lbn == ip->i_e2fs_last_lblk + 1) {
		return ip->i_e2fs_last_blk + 1;
	}

	/*
	 * bap, if provided, gives us a list of blocks to which we want to
	 * stay close
	 */

	if (bap) {
		for (i = indx; i >= 0 ; i--) {
			if (bap[i]) {
				return fs2h32(bap[i]) + 1;
			}
		}
	}

	/* fall back to the first block of the cylinder containing the inode */

	cg = ino_to_cg(fs, ip->i_number);
	return fs->e2fs.e2fs_bpg * cg + fs->e2fs.e2fs_first_dblock + 1;
}

/*
 * Implement the cylinder overflow algorithm.
 *
 * The policy implemented by this algorithm is:
 *   1) allocate the block in its requested cylinder group.
 *   2) quadratically rehash on the cylinder group number.
 *   3) brute force search for a free block.
 */
static u_long
ext2fs_hashalloc(struct inode *ip, int cg, long pref, int size,
		daddr_t (*allocator)(struct inode *, int, daddr_t, int))
{
	struct m_ext2fs *fs;
	long result;
	int i, icg = cg;

	fs = ip->i_e2fs;
	/*
	 * 1: preferred cylinder group
	 */
	result = (*allocator)(ip, cg, pref, size);
	if (result)
		return result;
	/*
	 * 2: quadratic rehash
	 */
	for (i = 1; i < fs->e2fs_ncg; i *= 2) {
		cg += i;
		if (cg >= fs->e2fs_ncg)
			cg -= fs->e2fs_ncg;
		result = (*allocator)(ip, cg, 0, size);
		if (result)
			return result;
	}
	/*
	 * 3: brute force search
	 * Note that we start at i == 2, since 0 was checked initially,
	 * and 1 is always checked in the quadratic rehash.
	 */
	cg = (icg + 2) % fs->e2fs_ncg;
	for (i = 2; i < fs->e2fs_ncg; i++) {
		result = (*allocator)(ip, cg, 0, size);
		if (result)
			return result;
		cg++;
		if (cg == fs->e2fs_ncg)
			cg = 0;
	}
	return 0;
}

/*
 * Determine whether a block can be allocated.
 *
 * Check to see if a block of the appropriate size is available,
 * and if it is, allocate it.
 */

static daddr_t
ext2fs_alloccg(struct inode *ip, int cg, daddr_t bpref, int size)
{
	struct m_ext2fs *fs;
	char *bbp;
	struct buf *bp;
	int error, bno, start, end, loc;

	fs = ip->i_e2fs;
	if (fs->e2fs_gd[cg].ext2bgd_nbfree == 0 &&
	    fs->e2fs_gd[cg].ext2bgd_nbfree_hi == 0)
		return 0;
	error = bread(ip->i_devvp, EXT2_FSBTODB64(fs,
		fs2h32(fs->e2fs_gd[cg].ext2bgd_b_bitmap),
		fs2h32(fs->e2fs_gd[cg].ext2bgd_b_bitmap_hi)),
		(int)fs->e2fs_bsize, B_MODIFY, &bp);
	if (error) {
		return 0;
	}
	bbp = (char *)bp->b_data;

	if (dtog(fs, bpref) != cg)
		bpref = 0;

	/* initialize block bitmap now if uninit */
	if (__predict_false(E2FS_HAS_GD_CSUM(fs) &&
	    (fs->e2fs_gd[cg].ext2bgd_flags & h2fs16(E2FS_BG_BLOCK_UNINIT)))) {
		ext2fs_init_bb(fs, cg, &fs->e2fs_gd[cg], bbp);
		fs->e2fs_gd[cg].ext2bgd_flags &= h2fs16(~E2FS_BG_BLOCK_UNINIT);
	}

	if (bpref != 0) {
		bpref = dtogd(fs, bpref);
		/*
		 * if the requested block is available, use it
		 */
		if (isclr(bbp, bpref)) {
			bno = bpref;
			goto gotit;
		}
	}
	/*
	 * no blocks in the requested cylinder, so take next
	 * available one in this cylinder group.
	 * first try to get 8 contiguous blocks, then fall back to a single
	 * block.
	 */
	if (bpref)
		start = dtogd(fs, bpref) / NBBY;
	else
		start = 0;
	end = howmany(fs->e2fs.e2fs_fpg, NBBY) - start;
	for (loc = start; loc < end; loc++) {
		if (bbp[loc] == 0) {
			bno = loc * NBBY;
			goto gotit;
		}
	}
	for (loc = 0; loc < start; loc++) {
		if (bbp[loc] == 0) {
			bno = loc * NBBY;
			goto gotit;
		}
	}

	bno = ext2fs_mapsearch(fs, bbp, bpref);
#if 0
	/*
	 * XXX jdolecek mapsearch actually never fails, it panics instead.
	 * If re-enabling, make sure to brele() before returning.
	 */
	if (bno < 0)
		return 0;
#endif
gotit:
#ifdef DIAGNOSTIC
	if (isset(bbp, (daddr_t)bno)) {
		printf("%s: cg=%d bno=%d fs=%s\n", __func__,
		    cg, bno, fs->e2fs_fsmnt);
		panic("ext2fs_alloccg: dup alloc");
	}
#endif
	setbit(bbp, (daddr_t)bno);
	fs->e2fs.e2fs_fbcount--;
	ext2fs_cg_update(fs, cg, &fs->e2fs_gd[cg], -1, 0, 0, 0);
	fs->e2fs_fmod = 1;
	bdwrite(bp);
	return cg * fs->e2fs.e2fs_fpg + fs->e2fs.e2fs_first_dblock + bno;
}

/*
 * Determine whether an inode can be allocated.
 *
 * Check to see if an inode is available, and if it is,
 * allocate it using the following policy:
 *   1) allocate the requested inode.
 *   2) allocate the next available inode after the requested
 *	  inode in the specified cylinder group.
 */
static daddr_t
ext2fs_nodealloccg(struct inode *ip, int cg, daddr_t ipref, int mode)
{
	struct m_ext2fs *fs;
	char *ibp;
	struct buf *bp;
	int error, start, len, loc, map, i;

	ipref--; /* to avoid a lot of (ipref -1) */
	if (ipref == -1)
		ipref = 0;
	fs = ip->i_e2fs;
	if (fs->e2fs_gd[cg].ext2bgd_nifree == 0 &&
	    fs->e2fs_gd[cg].ext2bgd_nifree_hi == 0)
		return 0;
	error = bread(ip->i_devvp, EXT2_FSBTODB64(fs,
		fs2h32(fs->e2fs_gd[cg].ext2bgd_i_bitmap),
		fs2h32(fs->e2fs_gd[cg].ext2bgd_i_bitmap_hi)),
		(int)fs->e2fs_bsize, B_MODIFY, &bp);
	if (error) {
		return 0;
	}
	ibp = (char *)bp->b_data;

	KASSERT(!E2FS_HAS_GD_CSUM(fs) || (fs->e2fs_gd[cg].ext2bgd_flags & h2fs16(E2FS_BG_INODE_ZEROED)) != 0);

	/* initialize inode bitmap now if uninit */
	if (__predict_false(E2FS_HAS_GD_CSUM(fs) &&
	    (fs->e2fs_gd[cg].ext2bgd_flags & h2fs16(E2FS_BG_INODE_UNINIT)))) {
		KASSERT(fs2h16(fs->e2fs_gd[cg].ext2bgd_nifree) == fs->e2fs.e2fs_ipg);
		memset(ibp, 0, fs->e2fs_bsize);
		fs->e2fs_gd[cg].ext2bgd_flags &= h2fs16(~E2FS_BG_INODE_UNINIT);
	}

	if (ipref) {
		ipref %= fs->e2fs.e2fs_ipg;
		if (isclr(ibp, ipref))
			goto gotit;
	}
	start = ipref / NBBY;
	len = howmany(fs->e2fs.e2fs_ipg - ipref, NBBY);
	loc = skpc(0xff, len, &ibp[start]);
	if (loc == 0) {
		len = start + 1;
		start = 0;
		loc = skpc(0xff, len, &ibp[0]);
		if (loc == 0) {
			printf("%s: cg = %d, ipref = %lld, fs = %s\n", __func__,
			    cg, (long long)ipref, fs->e2fs_fsmnt);
			panic("%s: map corrupted", __func__);
			/* NOTREACHED */
		}
	}
	i = start + len - loc;
	map = ibp[i] ^ 0xff;
	if (map == 0) {
		printf("%s: fs = %s\n", __func__, fs->e2fs_fsmnt);
		panic("%s: inode not in map", __func__);
	}
	ipref = i * NBBY + ffs(map) - 1;
gotit:
	setbit(ibp, ipref);
	fs->e2fs.e2fs_ficount--;
	ext2fs_cg_update(fs, cg, &fs->e2fs_gd[cg],
		0, -1, ((mode & IFMT) == IFDIR) ? 1 : 0, ipref);
	fs->e2fs_fmod = 1;
	bdwrite(bp);
	return cg * fs->e2fs.e2fs_ipg + ipref + 1;
}

/*
 * Free a block.
 *
 * The specified block is placed back in the
 * free map.
 */
void
ext2fs_blkfree(struct inode *ip, daddr_t bno)
{
	struct m_ext2fs *fs;
	char *bbp;
	struct buf *bp;
	int error, cg;

	fs = ip->i_e2fs;
	cg = dtog(fs, bno);

	KASSERT(!E2FS_HAS_GD_CSUM(fs) ||
	    (fs->e2fs_gd[cg].ext2bgd_flags & h2fs16(E2FS_BG_BLOCK_UNINIT)) == 0);

	if ((u_int)bno >= fs->e2fs.e2fs_bcount) {
		printf("%s: bad block %jd, ino %ju\n",
		    __func__, (intmax_t)bno, (uintmax_t)ip->i_number);
		ext2fs_fserr(fs, ip->i_uid, "bad block");
		return;
	}
	error = bread(ip->i_devvp, EXT2_FSBTODB64(fs,
	    fs2h32(fs->e2fs_gd[cg].ext2bgd_b_bitmap),
	    fs2h32(fs->e2fs_gd[cg].ext2bgd_b_bitmap_hi)),
	    (int)fs->e2fs_bsize, B_MODIFY, &bp);
	if (error) {
		return;
	}
	bbp = (char *)bp->b_data;
	bno = dtogd(fs, bno);
	if (isclr(bbp, bno)) {
		printf("%s: dev = %#jx, block = %jd, fs = %s\n", __func__,
		    (uintmax_t)ip->i_dev, (intmax_t)bno,
		    fs->e2fs_fsmnt);
		panic("%s: freeing free block", __func__);
	}
	clrbit(bbp, bno);
	fs->e2fs.e2fs_fbcount++;
	ext2fs_cg_update(fs, cg, &fs->e2fs_gd[cg], 1, 0, 0, 0);
	fs->e2fs_fmod = 1;
	bdwrite(bp);
}

/*
 * Free an inode.
 *
 * The specified inode is placed back in the free map.
 */
int
ext2fs_vfree(struct vnode *pvp, ino_t ino, int mode)
{
	struct m_ext2fs *fs;
	char *ibp;
	struct inode *pip;
	struct buf *bp;
	int error, cg;

	pip = VTOI(pvp);
	fs = pip->i_e2fs;

	if ((u_int)ino > fs->e2fs.e2fs_icount || (u_int)ino < EXT2_FIRSTINO)
		panic("%s: range: dev = %#jx, ino = %ju, fs = %s",
		    __func__, (uintmax_t)pip->i_dev, (uintmax_t)ino,
		    fs->e2fs_fsmnt);

	cg = ino_to_cg(fs, ino);

	KASSERT(!E2FS_HAS_GD_CSUM(fs) ||
	    (fs->e2fs_gd[cg].ext2bgd_flags & h2fs16(E2FS_BG_INODE_UNINIT)) == 0);

	error = bread(pip->i_devvp, EXT2_FSBTODB64(fs,
	    fs2h32(fs->e2fs_gd[cg].ext2bgd_i_bitmap),
	    fs2h32(fs->e2fs_gd[cg].ext2bgd_i_bitmap_hi)),
	    (int)fs->e2fs_bsize, B_MODIFY, &bp);
	if (error) {
		return 0;
	}
	ibp = (char *)bp->b_data;
	ino = (ino - 1) % fs->e2fs.e2fs_ipg;
	if (isclr(ibp, ino)) {
		printf("%s: dev = %#jx, ino = %ju, fs = %s\n", __func__,
		    (uintmax_t)pip->i_dev,
		    (uintmax_t)ino, fs->e2fs_fsmnt);
		if (fs->e2fs_ronly == 0)
			panic("%s: freeing free inode", __func__);
	}
	clrbit(ibp, ino);
	fs->e2fs.e2fs_ficount++;
	ext2fs_cg_update(fs, cg, &fs->e2fs_gd[cg],
		0, 1, ((mode & IFMT) == IFDIR) ? -1 : 0, 0);
	fs->e2fs_fmod = 1;
	bdwrite(bp);
	return 0;
}

/*
 * Find a block in the specified cylinder group.
 *
 * It is a panic if a request is made to find a block if none are
 * available.
 */

static daddr_t
ext2fs_mapsearch(struct m_ext2fs *fs, char *bbp, daddr_t bpref)
{
	int start, len, loc, i, map;

	/*
	 * find the fragment by searching through the free block
	 * map for an appropriate bit pattern
	 */
	if (bpref)
		start = dtogd(fs, bpref) / NBBY;
	else
		start = 0;
	len = howmany(fs->e2fs.e2fs_fpg, NBBY) - start;
	loc = skpc(0xff, len, &bbp[start]);
	if (loc == 0) {
		len = start + 1;
		start = 0;
		loc = skpc(0xff, len, &bbp[start]);
		if (loc == 0) {
			printf("%s: start = %d, len = %d, fs = %s\n",
			    __func__, start, len, fs->e2fs_fsmnt);
			panic("%s: map corrupted", __func__);
			/* NOTREACHED */
		}
	}
	i = start + len - loc;
	map = bbp[i] ^ 0xff;
	if (map == 0) {
		printf("%s: fs = %s\n", __func__, fs->e2fs_fsmnt);
		panic("%s: block not in map", __func__);
	}
	return i * NBBY + ffs(map) - 1;
}

/*
 * Fserr prints the name of a file system with an error diagnostic.
 *
 * The form of the error message is:
 *	fs: error message
 */
static void
ext2fs_fserr(struct m_ext2fs *fs, u_int uid, const char *cp)
{

	log(LOG_ERR, "uid %d on %s: %s\n", uid, fs->e2fs_fsmnt, cp);
}

static __inline void
ext2fs_cg_update(struct m_ext2fs *fs, int cg, struct ext2_gd *gd, int nbfree, int nifree, int ndirs, daddr_t ioff)
{
	if (nifree) {
		uint32_t ext2bgd_nifree = fs2h16(gd->ext2bgd_nifree) |
		    (fs2h16(gd->ext2bgd_nifree_hi) << 16);
		ext2bgd_nifree += nifree;
		gd->ext2bgd_nifree = h2fs16(ext2bgd_nifree);
		gd->ext2bgd_nifree_hi = h2fs16(ext2bgd_nifree >> 16);
		/*
		 * If we allocated inode on bigger offset than what was
		 * ever used before, bump the itable_unused count. This
		 * member only ever grows, and is used only for initialization
		 * !INODE_ZEROED groups with used inodes. Of course, by the
		 * time we get here the itables are already zeroed, but
		 * e2fstools fsck.ext4 still checks this.
		 */
		if (E2FS_HAS_GD_CSUM(fs) && nifree < 0 &&
		    (ioff + 1) >= (fs->e2fs.e2fs_ipg -
		    fs2h16(gd->ext2bgd_itable_unused_lo))) {
			gd->ext2bgd_itable_unused_lo =
			    h2fs16(fs->e2fs.e2fs_ipg - (ioff + 1));
		}

		KASSERT(!E2FS_HAS_GD_CSUM(fs) ||
		    gd->ext2bgd_itable_unused_lo <= ext2bgd_nifree);
	}


	if (nbfree) {
		uint32_t ext2bgd_nbfree = fs2h16(gd->ext2bgd_nbfree)
		    | (fs2h16(gd->ext2bgd_nbfree_hi) << 16);
		ext2bgd_nbfree += nbfree;
		gd->ext2bgd_nbfree = h2fs16(ext2bgd_nbfree);
		gd->ext2bgd_nbfree_hi = h2fs16(ext2bgd_nbfree >> 16);
	}

	if (ndirs) {
		uint32_t ext2bgd_ndirs = fs2h16(gd->ext2bgd_ndirs)
		    | (fs2h16(gd->ext2bgd_ndirs_hi) << 16);
		ext2bgd_ndirs += ndirs;
		gd->ext2bgd_ndirs = h2fs16(ext2bgd_ndirs);
		gd->ext2bgd_ndirs_hi = h2fs16(ext2bgd_ndirs >> 16);
	}

	if (E2FS_HAS_GD_CSUM(fs))
		gd->ext2bgd_checksum = ext2fs_cg_get_csum(fs, cg, gd);
}

static const uint32_t ext2fs_crc32c_table[256] = {
	0x00000000, 0xf26b8303, 0xe13b70f7, 0x1350f3f4,
	0xc79a971f, 0x35f1141c, 0x26a1e7e8, 0xd4ca64eb,
	0x8ad958cf, 0x78b2dbcc, 0x6be22838, 0x9989ab3b,
	0x4d43cfd0, 0xbf284cd3, 0xac78bf27, 0x5e133c24,
	0x105ec76f, 0xe235446c, 0xf165b798, 0x030e349b,
	0xd7c45070, 0x25afd373, 0x36ff2087, 0xc494a384,
	0x9a879fa0, 0x68ec1ca3, 0x7bbcef57, 0x89d76c54,
	0x5d1d08bf, 0xaf768bbc, 0xbc267848, 0x4e4dfb4b,
	0x20bd8ede, 0xd2d60ddd, 0xc186fe29, 0x33ed7d2a,
	0xe72719c1, 0x154c9ac2, 0x061c6936, 0xf477ea35,
	0xaa64d611, 0x580f5512, 0x4b5fa6e6, 0xb93425e5,
	0x6dfe410e, 0x9f95c20d, 0x8cc531f9, 0x7eaeb2fa,
	0x30e349b1, 0xc288cab2, 0xd1d83946, 0x23b3ba45,
	0xf779deae, 0x05125dad, 0x1642ae59, 0xe4292d5a,
	0xba3a117e, 0x4851927d, 0x5b016189, 0xa96ae28a,
	0x7da08661, 0x8fcb0562, 0x9c9bf696, 0x6ef07595,
	0x417b1dbc, 0xb3109ebf, 0xa0406d4b, 0x522bee48,
	0x86e18aa3, 0x748a09a0, 0x67dafa54, 0x95b17957,
	0xcba24573, 0x39c9c670, 0x2a993584, 0xd8f2b687,
	0x0c38d26c, 0xfe53516f, 0xed03a29b, 0x1f682198,
	0x5125dad3, 0xa34e59d0, 0xb01eaa24, 0x42752927,
	0x96bf4dcc, 0x64d4cecf, 0x77843d3b, 0x85efbe38,
	0xdbfc821c, 0x2997011f, 0x3ac7f2eb, 0xc8ac71e8,
	0x1c661503, 0xee0d9600, 0xfd5d65f4, 0x0f36e6f7,
	0x61c69362, 0x93ad1061, 0x80fde395, 0x72966096,
	0xa65c047d, 0x5437877e, 0x4767748a, 0xb50cf789,
	0xeb1fcbad, 0x197448ae, 0x0a24bb5a, 0xf84f3859,
	0x2c855cb2, 0xdeeedfb1, 0xcdbe2c45, 0x3fd5af46,
	0x7198540d, 0x83f3d70e, 0x90a324fa, 0x62c8a7f9,
	0xb602c312, 0x44694011, 0x5739b3e5, 0xa55230e6,
	0xfb410cc2, 0x092a8fc1, 0x1a7a7c35, 0xe811ff36,
	0x3cdb9bdd, 0xceb018de, 0xdde0eb2a, 0x2f8b6829,
	0x82f63b78, 0x709db87b, 0x63cd4b8f, 0x91a6c88c,
	0x456cac67, 0xb7072f64, 0xa457dc90, 0x563c5f93,
	0x082f63b7, 0xfa44e0b4, 0xe9141340, 0x1b7f9043,
	0xcfb5f4a8, 0x3dde77ab, 0x2e8e845f, 0xdce5075c,
	0x92a8fc17, 0x60c37f14, 0x73938ce0, 0x81f80fe3,
	0x55326b08, 0xa759e80b, 0xb4091bff, 0x466298fc,
	0x1871a4d8, 0xea1a27db, 0xf94ad42f, 0x0b21572c,
	0xdfeb33c7, 0x2d80b0c4, 0x3ed04330, 0xccbbc033,
	0xa24bb5a6, 0x502036a5, 0x4370c551, 0xb11b4652,
	0x65d122b9, 0x97baa1ba, 0x84ea524e, 0x7681d14d,
	0x2892ed69, 0xdaf96e6a, 0xc9a99d9e, 0x3bc21e9d,
	0xef087a76, 0x1d63f975, 0x0e330a81, 0xfc588982,
	0xb21572c9, 0x407ef1ca, 0x532e023e, 0xa145813d,
	0x758fe5d6, 0x87e466d5, 0x94b49521, 0x66df1622,
	0x38cc2a06, 0xcaa7a905, 0xd9f75af1, 0x2b9cd9f2,
	0xff56bd19, 0x0d3d3e1a, 0x1e6dcdee, 0xec064eed,
	0xc38d26c4, 0x31e6a5c7, 0x22b65633, 0xd0ddd530,
	0x0417b1db, 0xf67c32d8, 0xe52cc12c, 0x1747422f,
	0x49547e0b, 0xbb3ffd08, 0xa86f0efc, 0x5a048dff,
	0x8ecee914, 0x7ca56a17, 0x6ff599e3, 0x9d9e1ae0,
	0xd3d3e1ab, 0x21b862a8, 0x32e8915c, 0xc083125f,
	0x144976b4, 0xe622f5b7, 0xf5720643, 0x07198540,
	0x590ab964, 0xab613a67, 0xb831c993, 0x4a5a4a90,
	0x9e902e7b, 0x6cfbad78, 0x7fab5e8c, 0x8dc0dd8f,
	0xe330a81a, 0x115b2b19, 0x020bd8ed, 0xf0605bee,
	0x24aa3f05, 0xd6c1bc06, 0xc5914ff2, 0x37faccf1,
	0x69e9f0d5, 0x9b8273d6, 0x88d28022, 0x7ab90321,
	0xae7367ca, 0x5c18e4c9, 0x4f48173d, 0xbd23943e,
	0xf36e6f75, 0x0105ec76, 0x12551f82, 0xe03e9c81,
	0x34f4f86a, 0xc69f7b69, 0xd5cf889d, 0x27a40b9e,
	0x79b737ba, 0x8bdcb4b9, 0x988c474d, 0x6ae7c44e,
	0xbe2da0a5, 0x4c4623a6, 0x5f16d052, 0xad7d5351,
};

static uint32_t
ext2fs_crc32c(uint32_t last, const void *vbuf, size_t len)
{
	uint32_t crc = last;
	const uint8_t *buf = vbuf;

	while (len--)
		crc = ext2fs_crc32c_table[(crc ^ *buf++) & 0xff] ^ (crc >> 8);

	return crc;
}

/*
 * Compute group description csum. Structure data must be LE (not host).
 * Returned as LE (disk encoding).
 */
static uint16_t
ext2fs_cg_get_csum(struct m_ext2fs *fs, int cg, struct ext2_gd *gd)
{
	uint16_t crc;
	size_t cgsize = 1 << fs->e2fs_group_desc_shift;
	uint32_t cg_bswapped = h2fs32((uint32_t)cg);
	size_t off;

	if (EXT2F_HAS_ROCOMPAT_FEATURE(fs, EXT2F_ROCOMPAT_METADATA_CKSUM)) {
		uint32_t crc32;
		uint8_t dummy[2] = {0, 0};

		off = offsetof(struct ext2_gd, ext2bgd_checksum);

		crc32 = ext2fs_crc32c(~0, fs->e2fs.e2fs_uuid,
		    sizeof(fs->e2fs.e2fs_uuid));
		crc32 = ext2fs_crc32c(crc32, &cg_bswapped, sizeof(cg_bswapped));
		crc32 = ext2fs_crc32c(crc32, gd, off);
		crc32 = ext2fs_crc32c(crc32, dummy, 2);
		crc32 = ext2fs_crc32c(crc32, gd + off + 2, cgsize - (off + 2));

		return h2fs16(crc32 & 0xffff);
	}

	if (!EXT2F_HAS_ROCOMPAT_FEATURE(fs, EXT2F_ROCOMPAT_GDT_CSUM))
		return 0;

	off = offsetof(struct ext2_gd, ext2bgd_checksum);

	crc = crc16(~0, (uint8_t *)fs->e2fs.e2fs_uuid,
	    sizeof(fs->e2fs.e2fs_uuid));
	crc = crc16(crc, (uint8_t *)&cg_bswapped, sizeof(cg_bswapped));
	crc = crc16(crc, (uint8_t *)gd, off);
	crc = crc16(crc, (uint8_t *)gd + off + 2, cgsize - (off + 2));

	return h2fs16(crc);
}

static void
ext2fs_init_bb(struct m_ext2fs *fs, int cg, struct ext2_gd *gd, char *bbp)
{
	int i;

	memset(bbp, 0, fs->e2fs_bsize);

	/*
	 * No block was ever allocated on this cg before, so the only used
	 * blocks are metadata blocks on start of the group. We could optimize
	 * this to set by bytes, but since this is done once per the group
	 * in lifetime of filesystem, it really is not worth it.
	 */
	for(i=0; i < fs->e2fs.e2fs_bpg - fs2h16(gd->ext2bgd_nbfree); i++)
		setbit(bbp, i);
}

/*
 * Verify csum and initialize itable if not done already
 */
int
ext2fs_cg_verify_and_initialize(struct vnode *devvp, struct m_ext2fs *fs, int ronly)
{
	struct ext2_gd *gd;
	ino_t ioff;
	size_t boff;
	struct buf *bp;
	int cg, i, error;

	if (!E2FS_HAS_GD_CSUM(fs))
		return 0;

	for(cg = 0; cg < fs->e2fs_ncg; cg++) {
		gd = &fs->e2fs_gd[cg];

		/* Verify checksum */
		uint16_t csum = ext2fs_cg_get_csum(fs, cg, gd);
		if (gd->ext2bgd_checksum != csum) {
			printf("%s: group %d invalid csum (%#x != %#x)\n",
			    __func__, cg, gd->ext2bgd_checksum, csum);
			return EINVAL;
		}

		/* if mounting read-write, zero itable if not already done */
		if (ronly ||
		    (gd->ext2bgd_flags & h2fs16(E2FS_BG_INODE_ZEROED)) != 0)
			continue;

		/*
		 * We are skipping already used inodes, zero rest of itable
		 * blocks. First block to zero could be only partial wipe, all
		 * others are wiped completely. This might take a while,
		 * there could be many inode table blocks. We use
		 * delayed writes, so this shouldn't block for very
		 * long.
		 */
		ioff = fs->e2fs.e2fs_ipg - fs2h16(gd->ext2bgd_itable_unused_lo);
		boff = (ioff % fs->e2fs_ipb) * EXT2_DINODE_SIZE(fs);

		for(i = ioff / fs->e2fs_ipb; i < fs->e2fs_itpg; i++) {
			if (boff) {
				/* partial wipe, must read old data */
				error = bread(devvp, EXT2_FSBTODB64OFF(fs,
				    fs2h32(gd->ext2bgd_i_tables),
				    fs2h32(gd->ext2bgd_i_tables_hi), i),
				    (int)fs->e2fs_bsize, B_MODIFY, &bp);
				if (error) {
					printf("%s: can't read itable block",
					    __func__);
					return error;
				}
				memset((char *)bp->b_data + boff, 0,
				    fs->e2fs_bsize - boff);
				boff = 0;
			} else {
				/*
				 * Complete wipe, don't need to read data. This
				 * assumes nothing else is changing the data.
				 */
				bp = getblk(devvp, EXT2_FSBTODB64OFF(fs,
				    fs2h32(gd->ext2bgd_i_tables),
				    fs2h32(gd->ext2bgd_i_tables_hi), i),
				    (int)fs->e2fs_bsize, 0, 0);
				clrbuf(bp);
			}

			bdwrite(bp);
		}

		gd->ext2bgd_flags |= h2fs16(E2FS_BG_INODE_ZEROED);
		gd->ext2bgd_checksum = ext2fs_cg_get_csum(fs, cg, gd);
		fs->e2fs_fmod = 1;
	}

	return 0;
}
