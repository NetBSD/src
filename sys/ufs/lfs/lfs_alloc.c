/*	$NetBSD: lfs_alloc.c,v 1.47 2001/05/30 11:57:18 mrg Exp $	*/

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
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

#if defined(_KERNEL_OPT)
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

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

extern int lfs_dirvcount;
extern struct lock ufs_hashlock;

static int extend_ifile(struct lfs *, struct ucred *);
static int lfs_ialloc(struct lfs *, struct vnode *, ino_t, int, struct vnode **);

/*
 * Allocate a particular inode with a particular version number, freeing
 * any previous versions of this inode that may have gone before.
 * Used by the roll-forward code.
 *
 * XXX this function does not have appropriate locking to be used on a live fs;
 * XXX but something similar could probably be used for an "undelete" call.
 */
int
lfs_rf_valloc(struct lfs *fs, ino_t ino, int version, struct proc *p,
	      struct vnode **vpp)
{
	IFILE *ifp;
	struct buf *bp;
	struct vnode *vp;
	struct inode *ip;
	ino_t tino, oldnext;
	int error;

	/*
	 * First, just try a vget. If the version number is the one we want,
	 * we don't have to do anything else.  If the version number is wrong,
	 * take appropriate action.
	 */
	error = VFS_VGET(fs->lfs_ivnode->v_mount, ino, &vp);
	if (error == 0) {
		/* printf("lfs_rf_valloc[1]: ino %d vp %p\n", ino, vp); */

		*vpp = vp;
		ip = VTOI(vp);
		if (ip->i_ffs_gen == version)
			return 0;
		else if (ip->i_ffs_gen < version) {
			VOP_TRUNCATE(vp, (off_t)0, 0, NOCRED, p);
			ip->i_ffs_gen = version;
			LFS_SET_UINO(ip, IN_CHANGE | IN_MODIFIED | IN_UPDATE);
			return 0;
		} else {
			/* printf("ino %d: asked for version %d but got %d\n",
			       ino, version, ip->i_ffs_gen); */
			vput(vp);
			*vpp = NULLVP;
			return EEXIST;
		}
	}

	/*
	 * The inode is not in use.  Find it on the free list.
	 */
	/* If the Ifile is too short to contain this inum, extend it */
	while (VTOI(fs->lfs_ivnode)->i_ffs_size <=
	       dbtob(fsbtodb(fs, ino / fs->lfs_ifpb + fs->lfs_cleansz +
			     fs->lfs_segtabsz))) {
		extend_ifile(fs, NOCRED);
	}

	LFS_IENTRY(ifp, fs, ino, bp);
	oldnext = ifp->if_nextfree;
	ifp->if_version = version;
	brelse(bp);

	if (ino == fs->lfs_free) {
		fs->lfs_free = oldnext;
	} else {
		tino = fs->lfs_free;
		while(1) {
			LFS_IENTRY(ifp, fs, tino, bp);
			if (ifp->if_nextfree == ino ||
			    ifp->if_nextfree == LFS_UNUSED_INUM)
				break;
			tino = ifp->if_nextfree;
			brelse(bp);
		}
		if (ifp->if_nextfree == LFS_UNUSED_INUM) {
			brelse(bp);
			return ENOENT;
		}
		ifp->if_nextfree = oldnext;
		VOP_BWRITE(bp);
	}

	error = lfs_ialloc(fs, fs->lfs_ivnode, ino, version, &vp);
	if (error == 0) {
		/*
		 * Make it VREG so we can put blocks on it.  We will change
		 * this later if it turns out to be some other kind of file.
		 */
		ip = VTOI(vp);
		ip->i_ffs_mode = IFREG;
		ip->i_ffs_nlink = 1;
		ip->i_ffs_effnlink = 1;
		ufs_vinit(vp->v_mount, lfs_specop_p, lfs_fifoop_p, &vp);
		ip = VTOI(vp);

		/* printf("lfs_rf_valloc: ino %d vp %p\n", ino, vp); */

		/* The dirop-nature of this vnode is past */
		(void)lfs_vunref(vp);
		--lfs_dirvcount;
		vp->v_flag &= ~VDIROP;
		--fs->lfs_nadirop;
		ip->i_flag &= ~IN_ADIROP;
	}
	*vpp = vp;
	return error;
}

static int
extend_ifile(struct lfs *fs, struct ucred *cred)
{
	struct vnode *vp;
	struct inode *ip;
	IFILE *ifp;
	struct buf *bp;
	int error;
	ufs_daddr_t i, blkno, max;
	ino_t oldlast;

	vp = fs->lfs_ivnode;
	(void)lfs_vref(vp);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	ip = VTOI(vp);
	blkno = lblkno(fs, ip->i_ffs_size);
	if ((error = VOP_BALLOC(vp, ip->i_ffs_size, fs->lfs_bsize, cred, 0,
				&bp)) != 0) {
		VOP_UNLOCK(vp, 0);
		lfs_vunref(vp);
		return (error);
	}
	ip->i_ffs_size += fs->lfs_bsize;
	uvm_vnp_setsize(vp, ip->i_ffs_size);
	VOP_UNLOCK(vp, 0);
	
	i = (blkno - fs->lfs_segtabsz - fs->lfs_cleansz) *
		fs->lfs_ifpb;
	oldlast = fs->lfs_free;
	fs->lfs_free = i;
#ifdef DIAGNOSTIC
	if(fs->lfs_free == LFS_UNUSED_INUM)
		panic("inode 0 allocated [2]");
#endif /* DIAGNOSTIC */
	max = i + fs->lfs_ifpb;
	/* printf("extend ifile for ino %d--%d\n", i, max); */
	for (ifp = (struct ifile *)bp->b_data; i < max; ++ifp) {
		ifp->if_version = 1;
		ifp->if_daddr = LFS_UNUSED_DADDR;
		ifp->if_nextfree = ++i;
	}
	ifp--;
	ifp->if_nextfree = oldlast;
	(void) VOP_BWRITE(bp); /* Ifile */
	lfs_vunref(vp);

	return 0;
}

/* Allocate a new inode. */
/* ARGSUSED */
/* VOP_BWRITE 2i times */
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
	ino_t new_ino;
	int error;
	int new_gen;

	fs = VTOI(ap->a_pvp)->i_lfs;
	if (fs->lfs_ronly)
		return EROFS;
	*ap->a_vpp = NULL;
	
	/*
	 * Use lfs_seglock here, instead of fs->lfs_freelock, to ensure that
	 * the free list is not changed in between the time that the ifile
	 * blocks are written to disk and the time that the superblock is
	 * written to disk.
	 *
	 * XXX this sucks.  We should instead encode the head of the free
	 * list into the CLEANERINFO block of the Ifile. [XXX v2]
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
	printf("lfs_valloc: allocate inode %d\n", new_ino);
#endif
	
	/*
	 * Remove the inode from the free list and write the new start
	 * of the free list into the superblock.
	 */
	LFS_IENTRY(ifp, fs, new_ino, bp);
	if (ifp->if_daddr != LFS_UNUSED_DADDR)
		panic("lfs_valloc: inuse inode %d on the free list", new_ino);
	fs->lfs_free = ifp->if_nextfree;
	new_gen = ifp->if_version; /* version was updated by vfree */
	brelse(bp);

	/* Extend IFILE so that the next lfs_valloc will succeed. */
	if (fs->lfs_free == LFS_UNUSED_INUM) {
		if ((error = extend_ifile(fs, ap->a_cred)) != 0) {
			fs->lfs_free = new_ino;
			lfs_segunlock(fs);
			return error;
		}
	}
#ifdef DIAGNOSTIC
	if(fs->lfs_free == LFS_UNUSED_INUM)
		panic("inode 0 allocated [3]");
#endif /* DIAGNOSTIC */
	
	lfs_segunlock(fs);

	return lfs_ialloc(fs, ap->a_pvp, new_ino, new_gen, ap->a_vpp);
}

static int
lfs_ialloc(struct lfs *fs, struct vnode *pvp, ino_t new_ino, int new_gen,
	   struct vnode **vpp)
{
	struct inode *ip;
	struct vnode *vp;
	IFILE *ifp;
	struct buf *bp;
	int error;

	error = getnewvnode(VT_LFS, pvp->v_mount, lfs_vnodeop_p, &vp);
	/* printf("lfs_ialloc: ino %d vp %p error %d\n", new_ino, vp, error);*/
	if (error)
		goto errout;

	lockmgr(&ufs_hashlock, LK_EXCLUSIVE, 0);
	/* Create an inode to associate with the vnode. */
	lfs_vcreate(pvp->v_mount, new_ino, vp);
	
	ip = VTOI(vp);
	/* Zero out the direct and indirect block addresses. */
	bzero(&ip->i_din, sizeof(ip->i_din));
	ip->i_din.ffs_din.di_inumber = new_ino;
	
	/* Set a new generation number for this inode. */
	if (new_gen)
		ip->i_ffs_gen = new_gen;
	
	/* Insert into the inode hash table. */
	ufs_ihashins(ip);
	lockmgr(&ufs_hashlock, LK_RELEASE, 0);

	error = ufs_vinit(vp->v_mount, lfs_specop_p, lfs_fifoop_p, &vp);
	ip = VTOI(vp);
	if (error) {
		vput(vp);
		goto errout;
	}
	/* printf("lfs_ialloc[2]: ino %d vp %p\n", new_ino, vp);*/
	
	*vpp = vp;
#if 1
	if(!(vp->v_flag & VDIROP)) {
		(void)lfs_vref(vp);
		++lfs_dirvcount;
	}
	vp->v_flag |= VDIROP;
	
	if(!(ip->i_flag & IN_ADIROP))
		++fs->lfs_nadirop;
	ip->i_flag |= IN_ADIROP;
#endif
	VREF(ip->i_devvp);
	/* Set superblock modified bit and increment file count. */
	fs->lfs_fmod = 1;
	++fs->lfs_nfiles;
	return (0);

    errout:
	/*
	 * Put the new inum back on the free list.
	 */
	LFS_IENTRY(ifp, fs, new_ino, bp);
	ifp->if_daddr = LFS_UNUSED_DADDR;
	ifp->if_nextfree = fs->lfs_free;
	fs->lfs_free = new_ino;
	(void) VOP_BWRITE(bp); /* Ifile */

	*vpp = NULLVP;
	return (error);
}

/* Create a new vnode/inode pair and initialize what fields we can. */
void
lfs_vcreate(mp, ino, vp)
	struct mount *mp;
	ino_t ino;
	struct vnode *vp;
{
	struct inode *ip;
	struct ufsmount *ump;
#ifdef QUOTA
	int i;
#endif
	
	/* Get a pointer to the private mount structure. */
	ump = VFSTOUFS(mp);
	
	/* Initialize the inode. */
	ip = pool_get(&lfs_inode_pool, PR_WAITOK);
	vp->v_data = ip;
	ip->i_vnode = vp;
	ip->i_devvp = ump->um_devvp;
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
	ip->i_lfs_effnblks = 0;
	ip->i_flag = 0;
	LFS_SET_UINO(ip, IN_CHANGE | IN_MODIFIED);
}

/* Free an inode. */
/* ARGUSED */
/* VOP_BWRITE 2i times */
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
	if (ip->i_flag & IN_ADIROP) {
		--fs->lfs_nadirop;
		ip->i_flag &= ~IN_ADIROP;
	}

	LFS_CLR_UINO(ip, IN_ACCESSED|IN_CLEANING|IN_MODIFIED);
	ip->i_flag &= ~IN_ALLMOD;

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
	(void) VOP_BWRITE(bp); /* Ifile */
#ifdef DIAGNOSTIC
	if(fs->lfs_free == LFS_UNUSED_INUM) {
		panic("inode 0 freed");
	}
#endif /* DIAGNOSTIC */
	if (old_iaddr != LFS_UNUSED_DADDR) {
		LFS_SEGENTRY(sup, fs, datosn(fs, old_iaddr), bp);
#ifdef DIAGNOSTIC
		if (sup->su_nbytes < DINODE_SIZE) {
			printf("lfs_vfree: negative byte count"
			       " (segment %d short by %d)\n",
			       datosn(fs, old_iaddr),
			       (int)DINODE_SIZE - sup->su_nbytes);
			panic("lfs_vfree: negative byte count");
			sup->su_nbytes = DINODE_SIZE;
		}
#endif
		sup->su_nbytes -= DINODE_SIZE;
		(void) VOP_BWRITE(bp); /* Ifile */
	}
	
	/* Set superblock modified bit and decrement file count. */
	fs->lfs_fmod = 1;
	--fs->lfs_nfiles;
	
	lfs_segunlock(fs);
	return (0);
}
