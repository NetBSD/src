/*	$NetBSD: lfs_syscalls.c,v 1.58 2001/08/03 06:02:42 jdolecek Exp $	*/

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
/*-
 * Copyright (c) 1991, 1993, 1994
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
 *	@(#)lfs_syscalls.c	8.10 (Berkeley) 5/14/95
 */

#define LFS		/* for prototypes in syscallargs.h */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <sys/syscallargs.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

/* Flags for return from lfs_fastvget */
#define FVG_UNLOCK 0x01	  /* Needs to be unlocked */
#define FVG_PUT	   0x02	  /* Needs to be vput() */

/* Max block count for lfs_markv() */
#define MARKV_MAXBLKCNT		65536

struct buf *lfs_fakebuf(struct vnode *, int, size_t, caddr_t);
int lfs_fasthashget(dev_t, ino_t, int *, struct vnode **);

int debug_cleaner = 0; 
int clean_vnlocked = 0;
int clean_inlocked = 0;
int verbose_debug = 0;
    
pid_t lfs_cleaner_pid = 0;

/*
 * Definitions for the buffer free lists.
 */
#define BQUEUES		4		/* number of free buffer queues */
 
#define BQ_LOCKED	0		/* super-blocks &c */
#define BQ_LRU		1		/* lru, useful buffers */
#define BQ_AGE		2		/* rubbish */ 
#define BQ_EMPTY	3		/* buffer headers with no memory */
 
extern TAILQ_HEAD(bqueues, buf) bufqueues[BQUEUES];

#define LFS_FORCE_WRITE UNASSIGNED

#define LFS_VREF_THRESHOLD 128

static int lfs_bmapv(struct proc *, fsid_t *, BLOCK_INFO *, int);
static int lfs_markv(struct proc *, fsid_t *, BLOCK_INFO *, int);

/*
 * sys_lfs_markv:
 *
 * This will mark inodes and blocks dirty, so they are written into the log.
 * It will block until all the blocks have been written.  The segment create
 * time passed in the block_info and inode_info structures is used to decide
 * if the data is valid for each block (in case some process dirtied a block
 * or inode that is being cleaned between the determination that a block is
 * live and the lfs_markv call).
 *
 *  0 on success
 * -1/errno is return on error.
 */
#ifdef USE_64BIT_SYSCALLS
int
sys_lfs_markv(struct proc *p, void *v, register_t *retval)
{
	struct sys_lfs_markv_args /* {
		syscallarg(fsid_t *) fsidp;
		syscallarg(struct block_info *) blkiov;
		syscallarg(int) blkcnt;
	} */ *uap = v;
	BLOCK_INFO *blkiov;
	int blkcnt, error;
	fsid_t fsid;

	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
		return (error);
	
	if ((error = copyin(SCARG(uap, fsidp), &fsid, sizeof(fsid_t))) != 0)
		return (error);

	blkcnt = SCARG(uap, blkcnt);
	if ((u_int) blkcnt > MARKV_MAXBLKCNT)
		return (EINVAL);

	blkiov = malloc(blkcnt * sizeof(BLOCK_INFO), M_SEGMENT, M_WAITOK);
	if ((error = copyin(SCARG(uap, blkiov), blkiov,
			    blkcnt * sizeof(BLOCK_INFO))) != 0)
		goto out;

	if ((error = lfs_markv(p, &fsid, blkiov, blkcnt)) == 0)
		copyout(blkiov, SCARG(uap, blkiov),
			blkcnt * sizeof(BLOCK_INFO));
    out:
	free(blkiov, M_SEGMENT);
	return error;
}
#else
int
sys_lfs_markv(struct proc *p, void *v, register_t *retval)
{
	struct sys_lfs_markv_args /* {
		syscallarg(fsid_t *) fsidp;
		syscallarg(struct block_info *) blkiov;
		syscallarg(int) blkcnt;
	} */ *uap = v;
	BLOCK_INFO *blkiov;
	BLOCK_INFO_15 *blkiov15;
	int i, blkcnt, error;
	fsid_t fsid;

	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
		return (error);
	
	if ((error = copyin(SCARG(uap, fsidp), &fsid, sizeof(fsid_t))) != 0)
		return (error);

	blkcnt = SCARG(uap, blkcnt);
	if ((u_int) blkcnt > MARKV_MAXBLKCNT)
		return (EINVAL);

	blkiov = malloc(blkcnt * sizeof(BLOCK_INFO), M_SEGMENT, M_WAITOK);
	blkiov15 = malloc(blkcnt * sizeof(BLOCK_INFO_15), M_SEGMENT, M_WAITOK);
	if ((error = copyin(SCARG(uap, blkiov), blkiov15,
			    blkcnt * sizeof(BLOCK_INFO_15))) != 0)
		goto out;

	for (i = 0; i < blkcnt; i++) {
		blkiov[i].bi_inode     = blkiov15[i].bi_inode;
		blkiov[i].bi_lbn       = blkiov15[i].bi_lbn;
		blkiov[i].bi_daddr     = blkiov15[i].bi_daddr;
		blkiov[i].bi_segcreate = blkiov15[i].bi_segcreate;
		blkiov[i].bi_version   = blkiov15[i].bi_version;
		blkiov[i].bi_bp        = blkiov15[i].bi_bp;
		blkiov[i].bi_size      = blkiov15[i].bi_size;
	}

	if ((error = lfs_markv(p, &fsid, blkiov, blkcnt)) == 0) {
		for (i = 0; i < blkcnt; i++) {
			blkiov15[i].bi_inode     = blkiov[i].bi_inode;
			blkiov15[i].bi_lbn       = blkiov[i].bi_lbn;
			blkiov15[i].bi_daddr     = blkiov[i].bi_daddr;
			blkiov15[i].bi_segcreate = blkiov[i].bi_segcreate;
			blkiov15[i].bi_version   = blkiov[i].bi_version;
			blkiov15[i].bi_bp        = blkiov[i].bi_bp;
			blkiov15[i].bi_size      = blkiov[i].bi_size;
		}
		copyout(blkiov15, SCARG(uap, blkiov),
			blkcnt * sizeof(BLOCK_INFO_15));
	}
    out:
	free(blkiov, M_SEGMENT);
	free(blkiov15, M_SEGMENT);
	return error;
}
#endif

static int
lfs_markv(struct proc *p, fsid_t *fsidp, BLOCK_INFO *blkiov, int blkcnt)
{
	BLOCK_INFO *blkp;
	IFILE *ifp;
	struct buf *bp, *nbp;
	struct inode *ip = NULL;
	struct lfs *fs;
	struct mount *mntp;
	struct vnode *vp;
#ifdef DEBUG_LFS
	int vputc=0, iwritten=0;
#endif
	ino_t lastino;
	ufs_daddr_t b_daddr, v_daddr;
	int cnt, error, lfs_fastvget_unlock;
	int do_again=0;
	int s;
#ifdef CHECK_COPYIN
	int i;
#endif /* CHECK_COPYIN */
#ifdef LFS_TRACK_IOS
	int j;
#endif
	int numlocked=0, numrefed=0;
	ino_t maxino;

	if ((mntp = vfs_getvfs(fsidp)) == NULL)
		return (ENOENT);

	fs = VFSTOUFS(mntp)->um_lfs;
	maxino = (fragstoblks(fs, fsbtofrags(fs, VTOI(fs->lfs_ivnode)->i_ffs_blocks)) -
		      fs->lfs_cleansz - fs->lfs_segtabsz) * fs->lfs_ifpb;

	cnt = blkcnt;
	
	if ((error = vfs_busy(mntp, LK_NOWAIT, NULL)) != 0)
		return (error);

	/*
	 * This seglock is just to prevent the fact that we might have to sleep
	 * from allowing the possibility that our blocks might become
	 * invalid.
	 *
	 * It is also important to note here that unless we specify SEGM_CKP,
	 * any Ifile blocks that we might be asked to clean will never get
	 * to the disk.
	 */
	lfs_seglock(fs, SEGM_SYNC|SEGM_CLEAN|SEGM_CKP);
	
	/* Mark blocks/inodes dirty.  */
	error = 0;

#ifdef DEBUG_LFS
	/* Run through and count the inodes */
	lastino = LFS_UNUSED_INUM;
	for(blkp = blkiov; cnt--; ++blkp) {
		if(lastino != blkp->bi_inode) {
			lastino = blkp->bi_inode;
			vputc++;
		}
	}
	cnt = blkcnt;
	printf("[%d/",vputc);
	iwritten=0;
#endif /* DEBUG_LFS */
	/* these were inside the initialization for the for loop */
	v_daddr = LFS_UNUSED_DADDR;
	lastino = LFS_UNUSED_INUM;
	for (blkp = blkiov; cnt--; ++blkp)
	{
		if(blkp->bi_daddr == LFS_FORCE_WRITE)
			printf("lfs_markv: warning: force-writing ino %d lbn %d\n",
			       blkp->bi_inode, blkp->bi_lbn);
#ifdef LFS_TRACK_IOS
		/*
		 * If there is I/O on this segment that is not yet complete,
		 * the cleaner probably does not have the right information.
		 * Send it packing.
		 */
		for(j=0;j<LFS_THROTTLE;j++) {
			if(fs->lfs_pending[j] != LFS_UNUSED_DADDR
			   && dtosn(fs,fs->lfs_pending[j])==dtosn(fs,blkp->bi_daddr)
			   && blkp->bi_daddr != LFS_FORCE_WRITE)
			{
				printf("lfs_markv: attempt to clean pending segment? (#%d)\n",
				       dtosn(fs, fs->lfs_pending[j]));
				/* return (EBUSY); */
			}
		}
#endif /* LFS_TRACK_IOS */
		/* Bounds-check incoming data, avoid panic for failed VGET */
		if (blkp->bi_inode <= 0 || blkp->bi_inode >= maxino) {
			error = EINVAL;
			goto again;
		}
		/*
		 * Get the IFILE entry (only once) and see if the file still
		 * exists.
		 */
		if (lastino != blkp->bi_inode) {
			/*
			 * Finish the old file, if there was one.  The presence
			 * of a usable vnode in vp is signaled by a valid v_daddr.
			 */
			if(v_daddr != LFS_UNUSED_DADDR) {
#ifdef DEBUG_LFS
				if(ip->i_flag & (IN_MODIFIED|IN_CLEANING))
					iwritten++;
#endif
				if(lfs_fastvget_unlock) {
					VOP_UNLOCK(vp, 0);
					numlocked--;
				}
				lfs_vunref(vp);
				numrefed--;
			}

			/*
			 * Start a new file
			 */
			lastino = blkp->bi_inode;
			if (blkp->bi_inode == LFS_IFILE_INUM)
				v_daddr = fs->lfs_idaddr;
			else {
				LFS_IENTRY(ifp, fs, blkp->bi_inode, bp);
				/* XXX fix for force write */
				v_daddr = ifp->if_daddr;
				brelse(bp);
			}
			/* Don't force-write the ifile */
			if (blkp->bi_inode == LFS_IFILE_INUM
			    && blkp->bi_daddr == LFS_FORCE_WRITE)
			{
				continue;
			}
			if (v_daddr == LFS_UNUSED_DADDR
			    && blkp->bi_daddr != LFS_FORCE_WRITE)
			{
				continue;
			}

			/* Get the vnode/inode. */
			error=lfs_fastvget(mntp, blkp->bi_inode, v_daddr, 
					   &vp,
					   (blkp->bi_lbn==LFS_UNUSED_LBN
					    ? blkp->bi_bp
					    : NULL),
					   &lfs_fastvget_unlock);
			if(lfs_fastvget_unlock)
				numlocked++;

			if(!error) {
				numrefed++;
			}
			if(error) {
#ifdef DEBUG_LFS
				printf("lfs_markv: lfs_fastvget failed with %d (ino %d, segment %d)\n", 
				       error, blkp->bi_inode,
				       dtosn(fs, blkp->bi_daddr));
#endif /* DEBUG_LFS */
				/*
				 * If we got EAGAIN, that means that the
				 * Inode was locked.  This is
				 * recoverable: just clean the rest of
				 * this segment, and let the cleaner try
				 * again with another.  (When the
				 * cleaner runs again, this segment will
				 * sort high on the list, since it is
				 * now almost entirely empty.) But, we
				 * still set v_daddr = LFS_UNUSED_ADDR
				 * so as not to test this over and over
				 * again.
				 */
				if(error == EAGAIN) {
					error = 0;
					do_again++;
				}
#ifdef DIAGNOSTIC
				else if(error != ENOENT)
					panic("lfs_markv VFS_VGET FAILED");
#endif
				/* lastino = LFS_UNUSED_INUM; */
				v_daddr = LFS_UNUSED_DADDR;
				vp = NULL;
				ip = NULL;
				continue;
			}
			ip = VTOI(vp);
		} else if (v_daddr == LFS_UNUSED_DADDR) {
			/*
			 * This can only happen if the vnode is dead (or
			 * in any case we can't get it...e.g., it is
			 * inlocked).  Keep going.
			 */
			continue;
		}

		/* Past this point we are guaranteed that vp, ip are valid. */

		/* If this BLOCK_INFO didn't contain a block, keep going. */
		if (blkp->bi_lbn == LFS_UNUSED_LBN) {
			/* XXX need to make sure that the inode gets written in this case */
			/* XXX but only write the inode if it's the right one */
			if (blkp->bi_inode != LFS_IFILE_INUM) {
				LFS_IENTRY(ifp, fs, blkp->bi_inode, bp);
				if(ifp->if_daddr == blkp->bi_daddr
				   || blkp->bi_daddr == LFS_FORCE_WRITE)
				{
					LFS_SET_UINO(ip, IN_CLEANING);
				}
				brelse(bp);
			}
			continue;
		}

		b_daddr = 0;
		if(blkp->bi_daddr != LFS_FORCE_WRITE) {
			if (VOP_BMAP(vp, blkp->bi_lbn, NULL, &b_daddr, NULL) ||
			    dbtofsb(fs, b_daddr) != blkp->bi_daddr)
			{
				if(dtosn(fs,dbtofsb(fs, b_daddr))
				   == dtosn(fs,blkp->bi_daddr))
				{
					printf("lfs_markv: wrong da same seg: %x vs %x\n",
					       blkp->bi_daddr, dbtofsb(fs, b_daddr));
				}
				continue;
			}
		}
		/*
		 * If we got to here, then we are keeping the block.  If
		 * it is an indirect block, we want to actually put it
		 * in the buffer cache so that it can be updated in the
		 * finish_meta section.  If it's not, we need to
		 * allocate a fake buffer so that writeseg can perform
		 * the copyin and write the buffer.
		 */
		/*
		 * XXX - if the block we are reading has been *extended* since
		 * it was written to disk, then we risk throwing away
		 * the extension in bread()/getblk().  Check the size
		 * here.
		 */
		if(blkp->bi_size < fs->lfs_bsize) {
			s = splbio();
			bp = incore(vp, blkp->bi_lbn);
			if(bp && bp->b_bcount > blkp->bi_size) {
				printf("lfs_markv: %ld > %d (fixed)\n",
				       bp->b_bcount, blkp->bi_size);
				blkp->bi_size = bp->b_bcount;
			}
			splx(s);
		}
		if (ip->i_number != LFS_IFILE_INUM && blkp->bi_lbn >= 0) {
			/* Data Block */
			bp = lfs_fakebuf(vp, blkp->bi_lbn,
					 blkp->bi_size, blkp->bi_bp);
			/* Pretend we used bread() to get it */
			bp->b_blkno = fsbtodb(fs, blkp->bi_daddr);
		} else {
			/* Indirect block */
			bp = getblk(vp, blkp->bi_lbn, blkp->bi_size, 0, 0);
			if (!(bp->b_flags & (B_DONE|B_DELWRI))) { /* B_CACHE */
				/*
				 * The block in question was not found
				 * in the cache; i.e., the block that
				 * getblk() returned is empty.  So, we
				 * can (and should) copy in the
				 * contents, because we've already
				 * determined that this was the right
				 * version of this block on disk.
				 *
				 * And, it can't have changed underneath
				 * us, because we have the segment lock.
				 */
				error = copyin(blkp->bi_bp, bp->b_data, blkp->bi_size);
				if(error)
					goto err2;
			}
		}
		if ((error = lfs_bwrite_ext(bp,BW_CLEAN)) != 0)
			goto err2;
	}
	
	/*
	 * Finish the old file, if there was one
	 */
	if(v_daddr != LFS_UNUSED_DADDR) {
#ifdef DEBUG_LFS
		if(ip->i_flag & (IN_MODIFIED|IN_CLEANING))
			iwritten++;
#endif
		if(lfs_fastvget_unlock) {
			VOP_UNLOCK(vp, 0);
			numlocked--;
		}
		lfs_vunref(vp);
		numrefed--;
	}
	
	/*
	 * The last write has to be SEGM_SYNC, because of calling semantics.
	 * It also has to be SEGM_CKP, because otherwise we could write
	 * over the newly cleaned data contained in a checkpoint, and then
	 * we'd be unhappy at recovery time.
	 */
	lfs_segwrite(mntp, SEGM_SYNC|SEGM_CLEAN|SEGM_CKP);
	
	lfs_segunlock(fs);

#ifdef DEBUG_LFS
	printf("%d]",iwritten);
	if(numlocked != 0 || numrefed != 0) {
		panic("lfs_markv: numlocked=%d numrefed=%d", numlocked, numrefed);
	}
#endif
	
	vfs_unbusy(mntp);
	if(error)
		return (error);
	else if(do_again)
		return EAGAIN;

	return 0;
	
 err2:
	printf("lfs_markv err2\n");
	if(lfs_fastvget_unlock) {
		VOP_UNLOCK(vp, 0);
		--numlocked;
	}
	lfs_vunref(vp);
	--numrefed;

	/* Free up fakebuffers -- have to take these from the LOCKED list */
 again:
	s = splbio();
	for(bp = bufqueues[BQ_LOCKED].tqh_first; bp; bp=nbp) {
		nbp = bp->b_freelist.tqe_next;
		if(bp->b_flags & B_CALL) {
			if(bp->b_flags & B_BUSY) { /* not bloody likely */
				bp->b_flags |= B_WANTED;
				tsleep(bp, PRIBIO+1, "markv", 0);
				splx(s);
				goto again;
			}
			if(bp->b_flags & B_DELWRI) 
				fs->lfs_avail += btofsb(fs, bp->b_bcount);
			bremfree(bp);
			splx(s);
			brelse(bp);
			s = splbio();
		}
	}
	splx(s);
	lfs_segunlock(fs);
	vfs_unbusy(mntp);
#ifdef DEBUG_LFS
	if(numlocked != 0 || numrefed != 0) {
		panic("lfs_markv: numlocked=%d numrefed=%d", numlocked, numrefed);
	}
#endif

	return (error);
}

/*
 * sys_lfs_bmapv:
 *
 * This will fill in the current disk address for arrays of blocks.
 *
 *  0 on success
 * -1/errno is return on error.
 */
#ifdef USE_64BIT_SYSCALLS
int
sys_lfs_bmapv(struct proc *p, void *v, register_t *retval)
{
	struct sys_lfs_bmapv_args /* {
		syscallarg(fsid_t *) fsidp;
		syscallarg(struct block_info *) blkiov;
		syscallarg(int) blkcnt;
	} */ *uap = v;
	BLOCK_INFO *blkiov;
	int blkcnt, error;
	fsid_t fsid;

	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
		return (error);
	
	if ((error = copyin(SCARG(uap, fsidp), &fsid, sizeof(fsid_t))) != 0)
		return (error);

	blkcnt = SCARG(uap, blkcnt);
	blkiov = malloc(blkcnt * sizeof(BLOCK_INFO), M_SEGMENT, M_WAITOK);
	if ((error = copyin(SCARG(uap, blkiov), blkiov,
			    blkcnt * sizeof(BLOCK_INFO))) != 0)
		goto out;

	if ((error = lfs_bmapv(p, &fsid, blkiov, blkcnt)) == 0)
		copyout(blkiov, SCARG(uap, blkiov),
			blkcnt * sizeof(BLOCK_INFO));
    out:
	free(blkiov, M_SEGMENT);
	return error;
}
#else
int
sys_lfs_bmapv(struct proc *p, void *v, register_t *retval)
{
	struct sys_lfs_bmapv_args /* {
		syscallarg(fsid_t *) fsidp;
		syscallarg(struct block_info *) blkiov;
		syscallarg(int) blkcnt;
	} */ *uap = v;
	BLOCK_INFO *blkiov;
	BLOCK_INFO_15 *blkiov15;
	int i, blkcnt, error;
	fsid_t fsid;

	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
		return (error);
	
	if ((error = copyin(SCARG(uap, fsidp), &fsid, sizeof(fsid_t))) != 0)
		return (error);

	blkcnt = SCARG(uap, blkcnt);
	blkiov = malloc(blkcnt * sizeof(BLOCK_INFO), M_SEGMENT, M_WAITOK);
	blkiov15 = malloc(blkcnt * sizeof(BLOCK_INFO_15), M_SEGMENT, M_WAITOK);
	if ((error = copyin(SCARG(uap, blkiov), blkiov15,
			    blkcnt * sizeof(BLOCK_INFO_15))) != 0)
		goto out;

	for (i = 0; i < blkcnt; i++) {
		blkiov[i].bi_inode     = blkiov15[i].bi_inode;
		blkiov[i].bi_lbn       = blkiov15[i].bi_lbn;
		blkiov[i].bi_daddr     = blkiov15[i].bi_daddr;
		blkiov[i].bi_segcreate = blkiov15[i].bi_segcreate;
		blkiov[i].bi_version   = blkiov15[i].bi_version;
		blkiov[i].bi_bp        = blkiov15[i].bi_bp;
		blkiov[i].bi_size      = blkiov15[i].bi_size;
	}

	if ((error = lfs_bmapv(p, &fsid, blkiov, blkcnt)) == 0) {
		for (i = 0; i < blkcnt; i++) {
			blkiov15[i].bi_inode     = blkiov[i].bi_inode;
			blkiov15[i].bi_lbn       = blkiov[i].bi_lbn;
			blkiov15[i].bi_daddr     = blkiov[i].bi_daddr;
			blkiov15[i].bi_segcreate = blkiov[i].bi_segcreate;
			blkiov15[i].bi_version   = blkiov[i].bi_version;
			blkiov15[i].bi_bp        = blkiov[i].bi_bp;
			blkiov15[i].bi_size      = blkiov[i].bi_size;
		}
		copyout(blkiov15, SCARG(uap, blkiov),
			blkcnt * sizeof(BLOCK_INFO_15));
	}
    out:
	free(blkiov, M_SEGMENT);
	free(blkiov15, M_SEGMENT);
	return error;
}
#endif

static int
lfs_bmapv(struct proc *p, fsid_t *fsidp, BLOCK_INFO *blkiov, int blkcnt)
{
	BLOCK_INFO *blkp;
	IFILE *ifp;
	struct buf *bp;
	struct inode *ip = NULL;
	struct lfs *fs;
	struct mount *mntp;
	struct ufsmount *ump;
	struct vnode *vp;
	ino_t lastino;
	ufs_daddr_t v_daddr;
	int cnt, error, need_unlock=0;
	int numlocked=0, numrefed=0;
#ifdef LFS_TRACK_IOS
	int j;
#endif

	lfs_cleaner_pid = p->p_pid;
	
	if ((mntp = vfs_getvfs(fsidp)) == NULL)
		return (ENOENT);
	
	ump = VFSTOUFS(mntp);
	if ((error = vfs_busy(mntp, LK_NOWAIT, NULL)) != 0)
		return (error);
	
	cnt = blkcnt;
	
	fs = VFSTOUFS(mntp)->um_lfs;
	
	error = 0;
	
	/* these were inside the initialization for the for loop */
	v_daddr = LFS_UNUSED_DADDR;
	lastino = LFS_UNUSED_INUM;
	for (blkp = blkiov; cnt--; ++blkp)
	{
#ifdef DEBUG
		if (dtosn(fs, fs->lfs_curseg) == dtosn(fs, blkp->bi_daddr)) {
			printf("lfs_bmapv: attempt to clean current segment? (#%d)\n",
			       dtosn(fs, fs->lfs_curseg));
			vfs_unbusy(mntp);
			return (EBUSY);
		}
#endif /* DEBUG */
#ifdef LFS_TRACK_IOS
		/*
		 * If there is I/O on this segment that is not yet complete,
		 * the cleaner probably does not have the right information.
		 * Send it packing.
		 */
		for(j=0;j<LFS_THROTTLE;j++) {
			if(fs->lfs_pending[j] != LFS_UNUSED_DADDR
			   && dtosn(fs,fs->lfs_pending[j])==dtosn(fs,blkp->bi_daddr))
			{
				printf("lfs_bmapv: attempt to clean pending segment? (#%d)\n",
				       dtosn(fs, fs->lfs_pending[j]));
				vfs_unbusy(mntp);
				return (EBUSY);
			}
		}

#endif /* LFS_TRACK_IOS */
		/*
		 * Get the IFILE entry (only once) and see if the file still
		 * exists.
		 */
		if (lastino != blkp->bi_inode) {
			/*
			 * Finish the old file, if there was one.  The presence
			 * of a usable vnode in vp is signaled by a valid
			 * v_daddr.
			 */
			if(v_daddr != LFS_UNUSED_DADDR) {
				if(need_unlock) {
					VOP_UNLOCK(vp, 0);
					numlocked--;
				}
				lfs_vunref(vp);
				numrefed--;
			}

			/*
			 * Start a new file
			 */
			lastino = blkp->bi_inode;
			if (blkp->bi_inode == LFS_IFILE_INUM)
				v_daddr = fs->lfs_idaddr;
			else {
				LFS_IENTRY(ifp, fs, blkp->bi_inode, bp);
				v_daddr = ifp->if_daddr;
				brelse(bp);
			}
			if (v_daddr == LFS_UNUSED_DADDR) {
				blkp->bi_daddr = LFS_UNUSED_DADDR;
				continue;
			}
			/*
			 * A regular call to VFS_VGET could deadlock
			 * here.  Instead, we try an unlocked access.
			 */
			vp = ufs_ihashlookup(ump->um_dev, blkp->bi_inode);
			if (vp != NULL && !(vp->v_flag & VXLOCK)) {
				ip = VTOI(vp);
				if (lfs_vref(vp)) {
					v_daddr = LFS_UNUSED_DADDR;
					need_unlock = 0;
					continue;
				}
				numrefed++;
				if(VOP_ISLOCKED(vp)) {
#ifdef DEBUG_LFS
					printf("lfs_bmapv: inode %d inlocked\n",ip->i_number);
#endif
					v_daddr = LFS_UNUSED_DADDR;
					need_unlock = 0;
					lfs_vunref(vp);
					--numrefed;
					continue;
				} else {
					vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
					need_unlock = FVG_UNLOCK;
					numlocked++;
				}
			} else {
				error = VFS_VGET(mntp, blkp->bi_inode, &vp);
				if(error) {
#ifdef DEBUG_LFS
					printf("lfs_bmapv: vget of ino %d failed with %d",blkp->bi_inode,error);
#endif
					v_daddr = LFS_UNUSED_DADDR;
					need_unlock = 0;
					continue;
				} else {
					need_unlock = FVG_PUT;
					numlocked++;
					numrefed++;
				}
			}
			ip = VTOI(vp);
		} else if (v_daddr == LFS_UNUSED_DADDR) {
			/*
			 * This can only happen if the vnode is dead.
			 * Keep going.  Note that we DO NOT set the
			 * bi_addr to anything -- if we failed to get
			 * the vnode, for example, we want to assume
			 * conservatively that all of its blocks *are*
			 * located in the segment in question.
			 * lfs_markv will throw them out if we are
			 * wrong.
			 */
			/* blkp->bi_daddr = LFS_UNUSED_DADDR; */
			continue;
		}

		/* Past this point we are guaranteed that vp, ip are valid. */

		if(blkp->bi_lbn == LFS_UNUSED_LBN) {
			/*
			 * We just want the inode address, which is
			 * conveniently in v_daddr.
			 */
			blkp->bi_daddr = v_daddr;
		} else {
			error = VOP_BMAP(vp, blkp->bi_lbn, NULL,
					 &(blkp->bi_daddr), NULL);
			if(error)
			{
				blkp->bi_daddr = LFS_UNUSED_DADDR;
				continue;
			}
			blkp->bi_daddr = dbtofsb(fs, blkp->bi_daddr);
		}
	}
	
	/*
	 * Finish the old file, if there was one.  The presence
	 * of a usable vnode in vp is signaled by a valid v_daddr.
	 */
	if(v_daddr != LFS_UNUSED_DADDR) {
		if(need_unlock) {
			VOP_UNLOCK(vp, 0);
			numlocked--;
		}
		lfs_vunref(vp);
		numrefed--;
	}
	
	if(numlocked != 0 || numrefed != 0) {
		panic("lfs_bmapv: numlocked=%d numrefed=%d", numlocked,
		      numrefed);
	}
	
	vfs_unbusy(mntp);
	
	return 0;
}

/*
 * sys_lfs_segclean:
 *
 * Mark the segment clean.
 *
 *  0 on success
 * -1/errno is return on error.
 */
int
sys_lfs_segclean(struct proc *p, void *v, register_t *retval)
{
	struct sys_lfs_segclean_args /* {
		syscallarg(fsid_t *) fsidp;
		syscallarg(u_long) segment;
	} */ *uap = v;
	CLEANERINFO *cip;
	SEGUSE *sup;
	struct buf *bp;
	struct mount *mntp;
	struct lfs *fs;
	fsid_t fsid;
	int error;
	
	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
		return (error);
	
	if ((error = copyin(SCARG(uap, fsidp), &fsid, sizeof(fsid_t))) != 0)
		return (error);
	if ((mntp = vfs_getvfs(&fsid)) == NULL)
		return (ENOENT);
	
	fs = VFSTOUFS(mntp)->um_lfs;
	
	if (dtosn(fs, fs->lfs_curseg) == SCARG(uap, segment))
		return (EBUSY);
	
	if ((error = vfs_busy(mntp, LK_NOWAIT, NULL)) != 0) 
		return (error);
	LFS_SEGENTRY(sup, fs, SCARG(uap, segment), bp);
	if (sup->su_flags & SEGUSE_ACTIVE) {
		brelse(bp);
		vfs_unbusy(mntp);
		return (EBUSY);
	}
	if (!(sup->su_flags & SEGUSE_DIRTY)) {
		brelse(bp);
		vfs_unbusy(mntp);
		return (EALREADY);
	}
	
	fs->lfs_avail += segtod(fs, 1);
	if (sup->su_flags & SEGUSE_SUPERBLOCK)
		fs->lfs_avail -= btofsb(fs, LFS_SBPAD);
	if (fs->lfs_version > 1 && SCARG(uap, segment) == 0 &&
	    fs->lfs_start < btofsb(fs, LFS_LABELPAD))
		fs->lfs_avail -= btofsb(fs, LFS_LABELPAD) - fs->lfs_start;
	fs->lfs_bfree += sup->su_nsums * btofsb(fs, fs->lfs_sumsize) +
		btofsb(fs, sup->su_ninos * fs->lfs_ibsize);
	fs->lfs_dmeta -= sup->su_nsums * btofsb(fs, fs->lfs_sumsize) +
		btofsb(fs, sup->su_ninos * fs->lfs_ibsize);
	if (fs->lfs_dmeta < 0)
		fs->lfs_dmeta = 0;
	sup->su_flags &= ~SEGUSE_DIRTY;
	(void) VOP_BWRITE(bp);
	
	LFS_CLEANERINFO(cip, fs, bp);
	++cip->clean;
	--cip->dirty;
	fs->lfs_nclean = cip->clean;
	cip->bfree = fs->lfs_bfree;
	cip->avail = fs->lfs_avail - fs->lfs_ravail;
	(void) VOP_BWRITE(bp);
	wakeup(&fs->lfs_avail);
	vfs_unbusy(mntp);

	return (0);
}

/*
 * sys_lfs_segwait:
 *
 * This will block until a segment in file system fsid is written.  A timeout
 * in milliseconds may be specified which will awake the cleaner automatically.
 * An fsid of -1 means any file system, and a timeout of 0 means forever.
 *
 *  0 on success
 *  1 on timeout
 * -1/errno is return on error.
 */
int
sys_lfs_segwait(struct proc *p, void *v, register_t *retval)
{
	struct sys_lfs_segwait_args /* {
		syscallarg(fsid_t *) fsidp;
		syscallarg(struct timeval *) tv;
	} */ *uap = v;
	extern int lfs_allclean_wakeup;
	struct mount *mntp;
	struct timeval atv;
	fsid_t fsid;
	void *addr;
	u_long timeout;
	int error, s;
	
	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0) {
		return (error);
	}
	if ((error = copyin(SCARG(uap, fsidp), &fsid, sizeof(fsid_t))) != 0)
		return (error);
	if ((mntp = vfs_getvfs(&fsid)) == NULL)
		addr = &lfs_allclean_wakeup;
	else
		addr = &VFSTOUFS(mntp)->um_lfs->lfs_nextseg;
	
	if (SCARG(uap, tv)) {
		error = copyin(SCARG(uap, tv), &atv, sizeof(struct timeval));
		if (error)
			return (error);
		if (itimerfix(&atv))
			return (EINVAL);
		/*
		 * XXX THIS COULD SLEEP FOREVER IF TIMEOUT IS {0,0}!
		 * XXX IS THAT WHAT IS INTENDED?
		 */
		s = splclock();
		timeradd(&atv, &time, &atv);
		timeout = hzto(&atv);
		splx(s);
	} else
		timeout = 0;
	
	error = tsleep(addr, PCATCH | PUSER, "segment", timeout);
	return (error == ERESTART ? EINTR : 0);
}

/*
 * VFS_VGET call specialized for the cleaner.  The cleaner already knows the
 * daddr from the ifile, so don't look it up again.  If the cleaner is
 * processing IINFO structures, it may have the ondisk inode already, so
 * don't go retrieving it again.
 *
 * If we find the vnode on the hash chain, then it may be locked by another
 * process; so we set (*need_unlock) to zero.
 *
 * If we don't, we call ufs_ihashins, which locks the inode, and we set
 * (*need_unlock) to non-zero.
 *
 * In either case we lfs_vref, and it is the caller's responsibility to
 * lfs_vunref and VOP_UNLOCK (if necessary) when finished.
 */
extern struct lock ufs_hashlock;

int
lfs_fasthashget(dev_t dev, ino_t ino, int *need_unlock, struct vnode **vpp)
{
	struct inode *ip;

	/*
	 * This is playing fast and loose.  Someone may have the inode
	 * locked, in which case they are going to be distinctly unhappy
	 * if we trash something.
	 */
	if ((*vpp = ufs_ihashlookup(dev, ino)) != NULL) {
		if ((*vpp)->v_flag & VXLOCK) {
			printf("lfs_fastvget: vnode VXLOCKed for ino %d\n",
			       ino);
			clean_vnlocked++;
#ifdef LFS_EAGAIN_FAIL
			return EAGAIN;
#endif
		}
		ip = VTOI(*vpp);
		if (lfs_vref(*vpp)) {
			clean_inlocked++;
			return EAGAIN;
		}
		if (VOP_ISLOCKED(*vpp)) {
#ifdef DEBUG_LFS
			printf("lfs_fastvget: ino %d inlocked by pid %d\n",
			       ip->i_number, (*vpp)->v_lock.lk_lockholder);
#endif
			clean_inlocked++;
#ifdef LFS_EAGAIN_FAIL
			lfs_vunref(*vpp);
			return EAGAIN;
#endif /* LFS_EAGAIN_FAIL */
		} else {
			vn_lock(*vpp, LK_EXCLUSIVE | LK_RETRY);
			*need_unlock |= FVG_UNLOCK;
		}
	} else
		*vpp = NULL;

	return (0);
}

int
lfs_fastvget(struct mount *mp, ino_t ino, ufs_daddr_t daddr, struct vnode **vpp, struct dinode *dinp, int *need_unlock)
{
	struct inode *ip;
	struct vnode *vp;
	struct ufsmount *ump;
	dev_t dev;
	int error;
	struct buf *bp;
	struct lfs *fs;
	
	ump = VFSTOUFS(mp);
	dev = ump->um_dev;
	fs = ump->um_lfs;
	*need_unlock = 0;

	/*
	 * Wait until the filesystem is fully mounted before allowing vget
	 * to complete.  This prevents possible problems with roll-forward.
	 */
	while(fs->lfs_flags & LFS_NOTYET) {
		tsleep(&fs->lfs_flags, PRIBIO+1, "lfs_fnotyet", 0);
	}
	/*
	 * This is playing fast and loose.  Someone may have the inode
	 * locked, in which case they are going to be distinctly unhappy
	 * if we trash something.
	 */

	error = lfs_fasthashget(dev, ino, need_unlock, vpp);
	if (error != 0 || *vpp != NULL)
		return (error);

	if ((error = getnewvnode(VT_LFS, mp, lfs_vnodeop_p, &vp)) != 0) {
		*vpp = NULL;
		return (error);
	}

	do {
		error = lfs_fasthashget(dev, ino, need_unlock, vpp);
		if (error != 0 || *vpp != NULL) {
			ungetnewvnode(vp);
			return (error);
		}
	} while (lockmgr(&ufs_hashlock, LK_EXCLUSIVE|LK_SLEEPFAIL, 0));

	/* Allocate new vnode/inode. */
	lfs_vcreate(mp, ino, vp);

	/*
	 * Put it onto its hash chain and lock it so that other requests for
	 * this inode will block if they arrive while we are sleeping waiting
	 * for old data structures to be purged or for the contents of the
	 * disk portion of this inode to be read.
	 */
	ip = VTOI(vp);
	ufs_ihashins(ip);
	lockmgr(&ufs_hashlock, LK_RELEASE, 0);
	
	/*
	 * XXX
	 * This may not need to be here, logically it should go down with
	 * the i_devvp initialization.
	 * Ask Kirk.
	 */
	ip->i_lfs = fs;

	/* Read in the disk contents for the inode, copy into the inode. */
	if (dinp) {
		error = copyin(dinp, &ip->i_din.ffs_din, DINODE_SIZE);
		if (error) {
			printf("lfs_fastvget: dinode copyin failed for ino %d\n", ino);
			ufs_ihashrem(ip);

			/* Unlock and discard unneeded inode. */
			lockmgr(&vp->v_lock, LK_RELEASE, &vp->v_interlock);
			lfs_vunref(vp);
			*vpp = NULL;
			return (error);
		}
		if(ip->i_number != ino)
			panic("lfs_fastvget: I was fed the wrong inode!");
	} else {
		error = bread(ump->um_devvp, fsbtodb(fs, daddr), fs->lfs_ibsize,
			      NOCRED, &bp);
		if (error) {
			printf("lfs_fastvget: bread failed with %d\n",error);
			/*
			 * The inode does not contain anything useful, so it
			 * would be misleading to leave it on its hash chain.
			 * Iput() will return it to the free list.
			 */
			ufs_ihashrem(ip);
			
			/* Unlock and discard unneeded inode. */
			lockmgr(&vp->v_lock, LK_RELEASE, &vp->v_interlock);
			lfs_vunref(vp);
			brelse(bp);
			*vpp = NULL;
			return (error);
		}
		ip->i_din.ffs_din = *lfs_ifind(fs, ino, bp);
		brelse(bp);
	}
	ip->i_ffs_effnlink = ip->i_ffs_nlink;
	ip->i_lfs_effnblks = ip->i_ffs_blocks;

	/*
	 * Initialize the vnode from the inode, check for aliases.  In all
	 * cases re-init ip, the underlying vnode/inode may have changed.
	 */
	error = ufs_vinit(mp, lfs_specop_p, lfs_fifoop_p, &vp);
	if (error) {
		/* This CANNOT happen (see ufs_vinit) */
		printf("lfs_fastvget: ufs_vinit returned %d for ino %d\n", error, ino);
		lockmgr(&vp->v_lock, LK_RELEASE, &vp->v_interlock);
		lfs_vunref(vp);
		*vpp = NULL;
		return (error);
	}
#ifdef DEBUG_LFS
	if(vp->v_type == VNON) {
		printf("lfs_fastvget: ino %d is type VNON! (ifmt=%o, dinp=%p)\n",
		       ip->i_number, (ip->i_ffs_mode & IFMT)>>12, dinp);
		lfs_dump_dinode(&ip->i_din.ffs_din);
#ifdef DDB
		Debugger();
#endif
	}
#endif /* DEBUG_LFS */
	/*
	 * Finish inode initialization now that aliasing has been resolved.
	 */
	ip->i_devvp = ump->um_devvp;
	VREF(ip->i_devvp);
	*vpp = vp;
	*need_unlock |= FVG_PUT;

	uvm_vnp_setsize(vp, ip->i_ffs_size);

	return (0);
}

struct buf *
lfs_fakebuf(struct vnode *vp, int lbn, size_t size, caddr_t uaddr)
{
	struct buf *bp;
	int error;
	
#ifndef ALLOW_VFLUSH_CORRUPTION
	bp = lfs_newbuf(VTOI(vp)->i_lfs, vp, lbn, size);
	error = copyin(uaddr, bp->b_data, size);
	if(error) {
		lfs_freebuf(bp);
		return NULL;
	}
#else
	bp = lfs_newbuf(VTOI(vp)->i_lfs, vp, lbn, 0);
	bp->b_flags |= B_INVAL;
	bp->b_saveaddr = uaddr;
#endif

	bp->b_bufsize = size;
	bp->b_bcount = size;
	return (bp);
}
