/*	$NetBSD: lfs_alloc.c,v 1.32.2.1 2000/06/22 17:10:36 minoura Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Konrad E. Schroder <perseant@hhhh.org>.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
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
 *
 *	@(#)lfs_alloc.c	8.4 (Berkeley) 1/4/94
 */

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_quota.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/syslog.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/pool.h>

#include <vm/vm.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

extern struct lock ufs_hashlock;

/* Allocate a new inode. */
/* ARGSUSED */
int
lfs_valloc(v)
	void *v;
{
	struct vop_valloc_args /* {
				  struct vnode *a_pvp;
				  int a_mode;
				  struct ucred *a_cred;
				  struct vnode **a_vpp;
				  } */ *ap = v;
	struct lfs *fs;
	struct buf *bp;
	struct ifile *ifp;
	struct inode *ip;
	struct vnode *vp;
	ufs_daddr_t blkno;
	ino_t new_ino;
	u_long i, max;
	int error;
	extern int lfs_dirvcount;

	fs = VTOI(ap->a_pvp)->i_lfs;
	
	/*
	 * Use lfs_seglock here, instead of fs->lfs_freelock, to ensure that
	 * the free list is not changed in between the time that the ifile
	 * blocks are written to disk and the time that the superblock is
	 * written to disk.
	 *
	 * XXX this sucks.  We should instead encode the head of the free
	 * list into the CLEANERINFO block of the Ifile.
	 */
	lfs_seglock(fs, SEGM_PROT);

	/* Get the head of the freelist. */
	new_ino = fs->lfs_free;
#ifdef DIAGNOSTIC
	if(new_ino == LFS_UNUSED_INUM) {
#ifdef DEBUG
		lfs_dump_super(fs);
#endif /* DEBUG */
		panic("inode 0 allocated [1]");
	}
#endif /* DIAGNOSTIC */
#ifdef ALLOCPRINT
	printf("lfs_ialloc: allocate inode %d\n", new_ino);
#endif
	
	/*
	 * Remove the inode from the free list and write the new start
	 * of the free list into the superblock.
	 */
	LFS_IENTRY(ifp, fs, new_ino, bp);
	if (ifp->if_daddr != LFS_UNUSED_DADDR)
		panic("lfs_ialloc: inuse inode %d on the free list", new_ino);
	fs->lfs_free = ifp->if_nextfree;
#ifdef LFS_DEBUG_NEXTFREE
	ifp->if_nextfree = 0;
	VOP_BWRITE(bp);
#else
	brelse(bp);
#endif

	/* Extend IFILE so that the next lfs_valloc will succeed. */
	if (fs->lfs_free == LFS_UNUSED_INUM) {
		vp = fs->lfs_ivnode;
		VOP_LOCK(vp,LK_EXCLUSIVE);
		ip = VTOI(vp);
		blkno = lblkno(fs, ip->i_ffs_size);
		if ((error = VOP_BALLOC(vp, ip->i_ffs_size, fs->lfs_bsize,
		    NULL, 0, &bp)) != 0) {
			lfs_segunlock(fs);
			return (error);
		}
		ip->i_ffs_size += fs->lfs_bsize;
		uvm_vnp_setsize(vp, ip->i_ffs_size);
		(void)uvm_vnp_uncache(vp);

		i = (blkno - fs->lfs_segtabsz - fs->lfs_cleansz) *
			fs->lfs_ifpb;
		fs->lfs_free = i;
#ifdef DIAGNOSTIC
		if(fs->lfs_free == LFS_UNUSED_INUM)
			panic("inode 0 allocated [2]");
#endif /* DIAGNOSTIC */
		max = i + fs->lfs_ifpb;
		for (ifp = (struct ifile *)bp->b_data; i < max; ++ifp) {
			ifp->if_version = 1;
			ifp->if_daddr = LFS_UNUSED_DADDR;
			ifp->if_nextfree = ++i;
		}
		ifp--;
		ifp->if_nextfree = LFS_UNUSED_INUM;
		VOP_UNLOCK(vp,0);
		if ((error = VOP_BWRITE(bp)) != 0) {
			lfs_segunlock(fs);
			return (error);
		}
	}
#ifdef DIAGNOSTIC
	if(fs->lfs_free == LFS_UNUSED_INUM)
		panic("inode 0 allocated [3]");
#endif /* DIAGNOSTIC */
	
	lfs_segunlock(fs);

	lockmgr(&ufs_hashlock, LK_EXCLUSIVE, 0);
	/* Create a vnode to associate with the inode. */
	if ((error = lfs_vcreate(ap->a_pvp->v_mount, new_ino, &vp)) != 0) {
		lockmgr(&ufs_hashlock, LK_RELEASE, 0);
		return (error);
	}
	
	ip = VTOI(vp);
	/* Zero out the direct and indirect block addresses. */
	bzero(&ip->i_din, sizeof(ip->i_din));
	ip->i_din.ffs_din.di_inumber = new_ino;
	
	/* Set a new generation number for this inode. */
	ip->i_ffs_gen++;
	
	/* Insert into the inode hash table. */
	ufs_ihashins(ip);
	lockmgr(&ufs_hashlock, LK_RELEASE, 0);
	
	error = ufs_vinit(vp->v_mount, lfs_specop_p, lfs_fifoop_p, &vp);
	if (error) {
		vput(vp);
		*ap->a_vpp = NULL;
		return (error);
	}
	
	*ap->a_vpp = vp;
	if(!(vp->v_flag & VDIROP)) {
		lfs_vref(vp);
		++lfs_dirvcount;
	}
	vp->v_flag |= VDIROP;
	VREF(ip->i_devvp);
	
	if(!(ip->i_flag & IN_ADIROP))
		++fs->lfs_nadirop;
	ip->i_flag |= IN_ADIROP;

	/* Set superblock modified bit and increment file count. */
	fs->lfs_fmod = 1;
	++fs->lfs_nfiles;
	return (0);
}

/* Create a new vnode/inode pair and initialize what fields we can. */
int
lfs_vcreate(mp, ino, vpp)
	struct mount *mp;
	ino_t ino;
	struct vnode **vpp;
{
	extern int (**lfs_vnodeop_p) __P((void *));
	struct inode *ip;
	struct ufsmount *ump;
	int error;
#ifdef QUOTA
	int i;
#endif
	
	/* Create the vnode. */
	if ((error = getnewvnode(VT_LFS, mp, lfs_vnodeop_p, vpp)) != 0) {
		*vpp = NULL;
		return (error);
	}
	
	/* Get a pointer to the private mount structure. */
	ump = VFSTOUFS(mp);
	
	/* Initialize the inode. */
	ip = pool_get(&lfs_inode_pool, PR_WAITOK);
	(*vpp)->v_data = ip;
	ip->i_vnode = *vpp;
	ip->i_devvp = ump->um_devvp;
	ip->i_flag = IN_MODIFIED;
	ip->i_dev = ump->um_dev;
	ip->i_number = ip->i_din.ffs_din.di_inumber = ino;
	ip->i_lfs = ump->um_lfs;
#ifdef QUOTA
	for (i = 0; i < MAXQUOTAS; i++)
		ip->i_dquot[i] = NODQUOT;
#endif
	ip->i_lockf = 0;
	ip->i_diroff = 0;
	ip->i_ffs_mode = 0;
	ip->i_ffs_size = 0;
	ip->i_ffs_blocks = 0;
	++ump->um_lfs->lfs_uinodes;
	return (0);
}

/* Free an inode. */
/* ARGUSED */
int
lfs_vfree(v)
	void *v;
{
	struct vop_vfree_args /* {
				 struct vnode *a_pvp;
				 ino_t a_ino;
				 int a_mode;
				 } */ *ap = v;
	SEGUSE *sup;
	struct buf *bp;
	struct ifile *ifp;
	struct inode *ip;
	struct vnode *vp;
	struct lfs *fs;
	ufs_daddr_t old_iaddr;
	ino_t ino;
	extern int lfs_dirvcount;
	
	/* Get the inode number and file system. */
	vp = ap->a_pvp;
	ip = VTOI(vp);
	fs = ip->i_lfs;
	ino = ip->i_number;

#if 0
	/*
	 * Right now this is unnecessary since we take the seglock.
	 * But if the seglock is no longer necessary (e.g. we put the
	 * head of the free list into the Ifile) we will need to drain
	 * this vnode of any pending writes.
	 */
	if (WRITEINPROG(vp))
		tsleep(vp, (PRIBIO+1), "lfs_vfree", 0);
#endif
	lfs_seglock(fs, SEGM_PROT);
	
	if(vp->v_flag & VDIROP) {
		--lfs_dirvcount;
		vp->v_flag &= ~VDIROP;
		wakeup(&lfs_dirvcount);
		lfs_vunref(vp);
	}

	if (ip->i_flag & IN_CLEANING) {
		--fs->lfs_uinodes;
	}
	if (ip->i_flag & (IN_MODIFIED | IN_ACCESSED)) {
		--fs->lfs_uinodes;
	}
	ip->i_flag &= ~IN_ALLMOD;
#ifdef DEBUG_LFS	
	if((int32_t)fs->lfs_uinodes<0) {
		printf("U1");
		fs->lfs_uinodes=0;
	}
#endif
	/*
	 * Set the ifile's inode entry to unused, increment its version number
	 * and link it into the free chain.
	 */
	LFS_IENTRY(ifp, fs, ino, bp);
	old_iaddr = ifp->if_daddr;
	ifp->if_daddr = LFS_UNUSED_DADDR;
	++ifp->if_version;
	ifp->if_nextfree = fs->lfs_free;
	fs->lfs_free = ino;
	(void) VOP_BWRITE(bp);
#ifdef DIAGNOSTIC
	if(fs->lfs_free == LFS_UNUSED_INUM) {
		panic("inode 0 freed");
	}
#endif /* DIAGNOSTIC */
	if (old_iaddr != LFS_UNUSED_DADDR) {
		LFS_SEGENTRY(sup, fs, datosn(fs, old_iaddr), bp);
#ifdef DIAGNOSTIC
		if (sup->su_nbytes < DINODE_SIZE) {
			printf("lfs_vfree: negative byte count (segment %d short by %d)\n", datosn(fs, old_iaddr), (int)DINODE_SIZE - sup->su_nbytes);
			panic("lfs_vfree: negative byte count");
			sup->su_nbytes = DINODE_SIZE;
		}
#endif
		sup->su_nbytes -= DINODE_SIZE;
		(void) VOP_BWRITE(bp);
	}
	
	/* Set superblock modified bit and decrement file count. */
	fs->lfs_fmod = 1;
	--fs->lfs_nfiles;
	
	lfs_segunlock(fs);
	return (0);
}
