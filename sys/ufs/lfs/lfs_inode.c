/*	$NetBSD: lfs_inode.c,v 1.28 1999/11/23 23:52:42 fvdl Exp $	*/

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
 * Copyright (c) 1986, 1989, 1991, 1993
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
 *	@(#)lfs_inode.c	8.9 (Berkeley) 5/8/95
 */

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_quota.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <vm/vm.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

static int lfs_vinvalbuf __P((struct vnode *, struct ucred *, struct proc *, ufs_daddr_t));

/* Search a block for a specific dinode. */
struct dinode *
lfs_ifind(fs, ino, dip)
	struct lfs *fs;
	ino_t ino;
	register struct dinode *dip;
{
	register int cnt;
	register struct dinode *ldip;
	
	for (cnt = INOPB(fs), ldip = dip + (cnt - 1); cnt--; --ldip)
		if (ldip->di_inumber == ino)
			return (ldip);
	
	panic("lfs_ifind: dinode %u not found", ino);
	/* NOTREACHED */
}

int
lfs_update(v)
	void *v;
{
	struct vop_update_args /* {
				  struct vnode *a_vp;
				  struct timespec *a_access;
				  struct timespec *a_modify;
				  int a_waitfor;
				  } */ *ap = v;
	struct inode *ip;
	struct vnode *vp = ap->a_vp;
	int mod, oflag;
	struct timespec ts;
	struct lfs *fs = VFSTOUFS(vp->v_mount)->um_lfs;
	
	if (vp->v_mount->mnt_flag & MNT_RDONLY)
		return (0);
	ip = VTOI(vp);

	/*
	 * If we are called from vinvalbuf, and the file's blocks have
	 * already been scheduled for writing, but the writes have not
	 * yet completed, lfs_vflush will not be called, and vinvalbuf
	 * will cause a panic.  So, we must wait until any pending write
	 * for our inode completes, if we are called with LFS_SYNC set.
	 */
	while((ap->a_waitfor & LFS_SYNC) && WRITEINPROG(vp)) {
#ifdef DEBUG_LFS
		printf("lfs_update: sleeping on inode %d (in-progress)\n",ip->i_number);
#endif
		tsleep(vp, (PRIBIO+1), "lfs_update", 0);
	}
	mod = ip->i_flag & IN_MODIFIED;
	oflag = ip->i_flag;
	TIMEVAL_TO_TIMESPEC(&time, &ts);
	LFS_ITIMES(ip,
		   ap->a_access ? ap->a_access : &ts,
		   ap->a_modify ? ap->a_modify : &ts, &ts);
	if (!mod && (ip->i_flag & IN_MODIFIED))
		ip->i_lfs->lfs_uinodes++;
	if ((ip->i_flag & (IN_MODIFIED|IN_CLEANING)) == 0) {
		return (0);
	}
	
	/* If sync, push back the vnode and any dirty blocks it may have. */
	if(ap->a_waitfor & LFS_SYNC) {
		/* Avoid flushing VDIROP. */
		++fs->lfs_diropwait;
		while(vp->v_flag & VDIROP) {
#ifdef DEBUG_LFS
			printf("lfs_update: sleeping on inode %d (dirops)\n",ip->i_number);
#endif
			if(fs->lfs_dirops==0)
				lfs_flush_fs(vp->v_mount,SEGM_SYNC);
			else
				tsleep(&fs->lfs_writer, PRIBIO+1, "lfs_fsync", 0);
			/* XXX KS - by falling out here, are we writing the vn
			twice? */
		}
		--fs->lfs_diropwait;
		return lfs_vflush(vp);
        }
	return 0;
}

/* Update segment usage information when removing a block. */
#define UPDATE_SEGUSE \
	if (lastseg != -1) { \
		LFS_SEGENTRY(sup, fs, lastseg, sup_bp); \
		if (num > sup->su_nbytes) { \
			printf("lfs_truncate: negative bytes: segment %d short by %d\n", \
			      lastseg, num - sup->su_nbytes); \
			panic("lfs_truncate: negative bytes"); \
		      sup->su_nbytes = 0; \
		} else \
		sup->su_nbytes -= num; \
		e1 = VOP_BWRITE(sup_bp); \
		fragsreleased += numfrags(fs, num); \
	}

#define SEGDEC(S) { \
	if (daddr != 0) { \
		if (lastseg != (seg = datosn(fs, daddr))) { \
			UPDATE_SEGUSE; \
			num = (S); \
			lastseg = seg; \
		} else \
			num += (S); \
	} \
}

/*
 * Truncate the inode ip to at most length size.  Update segment usage
 * table information.
 */
/* ARGSUSED */
int
lfs_truncate(v)
	void *v;
{
	struct vop_truncate_args /* {
		struct vnode *a_vp;
		off_t a_length;
		int a_flags;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;
	register struct indir *inp;
	register int i;
	register ufs_daddr_t *daddrp;
	register struct vnode *vp = ap->a_vp;
	off_t length = ap->a_length;
	struct buf *bp, *sup_bp;
	struct ifile *ifp;
	struct inode *ip;
	struct lfs *fs;
	struct indir a[NIADDR + 2], a_end[NIADDR + 2];
	SEGUSE *sup;
	ufs_daddr_t daddr, lastblock, lbn, olastblock;
	ufs_daddr_t oldsize_lastblock, oldsize_newlast, newsize;
	long off, a_released, fragsreleased, i_released;
	int e1, e2, depth, lastseg, num, offset, seg, freesize, s;
	
	ip = VTOI(vp);

	if (vp->v_type == VLNK && vp->v_mount->mnt_maxsymlinklen > 0) {
#ifdef DIAGNOSTIC
		if (length != 0)
			panic("lfs_truncate: partial truncate of symlink");
#endif
		bzero((char *)&ip->i_ffs_shortlink, (u_int)ip->i_ffs_size);
		ip->i_ffs_size = 0;
		ip->i_flag |= IN_CHANGE | IN_UPDATE;
		return (VOP_UPDATE(vp, NULL, NULL, 0));
	}
	uvm_vnp_setsize(vp, length);
	
	fs = ip->i_lfs;
	lfs_imtime(fs);
	
	/* If length is larger than the file, just update the times. */
	if (ip->i_ffs_size <= length) {
		ip->i_flag |= IN_CHANGE | IN_UPDATE;
		return (VOP_UPDATE(vp, NULL, NULL, 0));
	}

	/*
	 * Make sure no writes happen while we're truncating.
	 * Otherwise, blocks which are accounted for on the inode
	 * *and* which have been created for cleaning can coexist,
	 * and cause us to overcount, and panic below.
	 *
	 * XXX KS - too restrictive?  Maybe only when cleaning?
	 */
	while(fs->lfs_seglock) {
		tsleep(&fs->lfs_seglock, (PRIBIO+1), "lfs_truncate", 0);
	}
	
	/*
	 * Calculate index into inode's block list of last direct and indirect
	 * blocks (if any) which we want to keep.  Lastblock is 0 when the
	 * file is truncated to 0.
	 */
	lastblock = lblkno(fs, length + fs->lfs_bsize - 1);
	olastblock = lblkno(fs, ip->i_ffs_size + fs->lfs_bsize - 1) - 1;

	/*
	 * Update the size of the file. If the file is not being truncated to
	 * a block boundry, the contents of the partial block following the end
	 * must be zero'd in case it ever becomes accessible again
	 * because of subsequent file growth.  For this part of the code,
	 * oldsize_newlast refers to the old size of the new last block in the
	 * file.
	 */
	offset = blkoff(fs, length);
	lbn = lblkno(fs, length);
	oldsize_newlast = blksize(fs, ip, lbn);

	/* Now set oldsize to the current size of the current last block */
	oldsize_lastblock = blksize(fs, ip, olastblock);
	if (offset == 0)
		ip->i_ffs_size = length;
	else {
#ifdef QUOTA
		if ((e1 = getinoquota(ip)) != 0)
			return (e1);
#endif	
		if ((e1 = bread(vp, lbn, oldsize_newlast, NOCRED, &bp)) != 0) {
			printf("lfs_truncate: bread: %d\n",e1);
			brelse(bp);
			return (e1);
		}
		ip->i_ffs_size = length;
		(void)uvm_vnp_uncache(vp);
		newsize = blksize(fs, ip, lbn);
		bzero((char *)bp->b_data + offset, (u_int)(newsize - offset));
#ifdef DEBUG
		if(bp->b_flags & B_CALL)
		    panic("Can't allocbuf malloced buffer!");
		else
#endif
			allocbuf(bp, newsize);
		if(oldsize_newlast > newsize)
			ip->i_ffs_blocks -= btodb(oldsize_newlast - newsize);
		if ((e1 = VOP_BWRITE(bp)) != 0) {
			printf("lfs_truncate: bwrite: %d\n",e1);
			return (e1);
		}
	}
	/*
	 * Modify sup->su_nbyte counters for each deleted block; keep track
	 * of number of blocks removed for ip->i_ffs_blocks.
	 */
	fragsreleased = 0;
	num = 0;
	lastseg = -1;

	for (lbn = olastblock; lbn >= lastblock;) {
		/* XXX use run length from bmap array to make this faster */
		ufs_bmaparray(vp, lbn, &daddr, a, &depth, NULL);
		if (lbn == olastblock) {
			for (i = NIADDR + 2; i--;)
				a_end[i] = a[i];
			freesize = oldsize_lastblock;
		} else 
			freesize = fs->lfs_bsize;
		switch (depth) {
		case 0:		/* Direct block. */
			daddr = ip->i_ffs_db[lbn];
			SEGDEC(freesize);
			ip->i_ffs_db[lbn] = 0;
			--lbn;
			break;
#ifdef DIAGNOSTIC
		case 1:		/* An indirect block. */
			panic("lfs_truncate: ufs_bmaparray returned depth 1");
			/* NOTREACHED */
#endif
		default:	/* Chain of indirect blocks. */
			inp = a + --depth;
			if (inp->in_off > 0 && lbn != lastblock) {
				lbn -= inp->in_off < lbn - lastblock ?
					inp->in_off : lbn - lastblock;
				break;
			}
			for (; depth && (inp->in_off == 0 || lbn == lastblock);
			     --inp, --depth) {
				if (bread(vp,
					  inp->in_lbn, fs->lfs_bsize, NOCRED, &bp))
					panic("lfs_truncate: bread bno %d",
					      inp->in_lbn);
				daddrp = (ufs_daddr_t *)bp->b_data + inp->in_off;
				for (i = inp->in_off;
				     i++ <= a_end[depth].in_off;) {
					daddr = *daddrp++;
					SEGDEC(freesize);
				}
				a_end[depth].in_off = NINDIR(fs) - 1;
				if (inp->in_off == 0)
					brelse (bp);
				else {
					bzero((ufs_daddr_t *)bp->b_data +
					      inp->in_off, fs->lfs_bsize - 
					      inp->in_off * sizeof(ufs_daddr_t));
					if ((e1 = VOP_BWRITE(bp)) != 0) {
						printf("lfs_truncate: indir bwrite: %d\n",e1);
						return (e1);
					}
				}
			}
			if (depth == 0 && a[1].in_off == 0) {
				off = a[0].in_off;
				daddr = ip->i_ffs_ib[off];
				SEGDEC(freesize);
				ip->i_ffs_ib[off] = 0;
			}
			if (lbn == lastblock || lbn <= NDADDR)
				--lbn;
			else {
				lbn -= NINDIR(fs);
				if (lbn < lastblock)
					lbn = lastblock;
			}
		}
	}
	UPDATE_SEGUSE;
	
	/* If truncating the file to 0, update the version number. */
	if (length == 0) {
		LFS_IENTRY(ifp, fs, ip->i_number, bp);
		++ifp->if_version;
		(void) VOP_BWRITE(bp);
	}
#ifdef DIAGNOSTIC
	if (ip->i_ffs_blocks < fragstodb(fs, fragsreleased)) {
		panic("lfs_truncate: frag count < 0 (%d<%ld), ino %d\n",
			    ip->i_ffs_blocks, fragstodb(fs, fragsreleased),
			    ip->i_number);
		fragsreleased = dbtofrags(fs, ip->i_ffs_blocks);
	}
#endif
	ip->i_ffs_blocks -= fragstodb(fs, fragsreleased);
	fs->lfs_bfree +=  fragstodb(fs, fragsreleased);
	ip->i_flag |= IN_CHANGE | IN_UPDATE;
	/*
	 * Traverse dirty block list counting number of dirty buffers
	 * that are being deleted out of the cache, so that the lfs_avail
	 * field can be updated.
	 */
	a_released = 0;
	i_released = 0;

	s = splbio();
	for (bp = vp->v_dirtyblkhd.lh_first; bp; bp = bp->b_vnbufs.le_next) {

		/* XXX KS - Don't miscount if we're not truncating to zero. */
		if(length>0 && !(bp->b_lblkno >= 0 && bp->b_lblkno > lastblock)
		   && !(bp->b_lblkno < 0 && bp->b_lblkno < -lastblock-NIADDR))
			continue;

		if (bp->b_flags & B_LOCKED) {
			a_released += numfrags(fs, bp->b_bcount);
			/*
			 * XXX
			 * When buffers are created in the cache, their block
			 * number is set equal to their logical block number.
			 * If that is still true, we are assuming that the
			 * blocks are new (not yet on disk) and weren't
			 * counted above.  However, there is a slight chance
			 * that a block's disk address is equal to its logical
			 * block number in which case, we'll get an overcounting
			 * here.
			 */
			if (bp->b_blkno == bp->b_lblkno) {
				i_released += numfrags(fs, bp->b_bcount);
			}
		}
	}
	splx(s);
	fragsreleased = i_released;
#ifdef DIAGNOSTIC
	if (fragsreleased > dbtofrags(fs, ip->i_ffs_blocks)) {
		printf("lfs_inode: %ld frags released > %d in inode %d\n",
		       fragsreleased, dbtofrags(fs, ip->i_ffs_blocks),
		       ip->i_number);
		fragsreleased = dbtofrags(fs, ip->i_ffs_blocks);
	}
#endif
	fs->lfs_bfree += fragstodb(fs, fragsreleased);
	ip->i_ffs_blocks -= fragstodb(fs, fragsreleased);
#ifdef DIAGNOSTIC
	if (length == 0 && ip->i_ffs_blocks != 0) {
		printf("lfs_inode: trunc to zero, but %d blocks left on inode %d\n",
		       ip->i_ffs_blocks, ip->i_number);
		panic("lfs_inode\n");
	}
#endif
	fs->lfs_avail += fragstodb(fs, a_released);
	if(length>0)
		e1 = lfs_vinvalbuf(vp, ap->a_cred, ap->a_p, lastblock-1);
	else
		e1 = vinvalbuf(vp, 0, ap->a_cred, ap->a_p, 0, 0); 
	e2 = VOP_UPDATE(vp, NULL, NULL, 0);
	if(e1)
		printf("lfs_truncate: vinvalbuf: %d\n",e1);
	if(e2)
		printf("lfs_truncate: update: %d\n",e2);

	return (e1 ? e1 : e2 ? e2 : 0);
}

/*
 * Get rid of blocks a la vinvalbuf; but only blocks that are of a higher
 * lblkno than the file size allows.
 */
int
lfs_vinvalbuf(vp, cred, p, maxblk)
	register struct vnode *vp;
	struct ucred *cred;
	struct proc *p;
	ufs_daddr_t maxblk;
{
	register struct buf *bp;
	struct buf *nbp, *blist;
	int i, s, error, dirty;

      top:
	dirty=0;
	for (i=0;i<2;i++) {
		if(i==0)
			blist = vp->v_cleanblkhd.lh_first;
		else /* i == 1 */
			blist = vp->v_dirtyblkhd.lh_first;

		s = splbio();
		for (bp = blist; bp; bp = nbp) {
			nbp = bp->b_vnbufs.le_next;

			if (bp->b_flags & B_GATHERED) {
				error = tsleep(vp, PRIBIO+1, "lfs_vin2", 0);
				splx(s);
				if(error)
					return error;
				goto top;
			}
			if (bp->b_flags & B_BUSY) {
				bp->b_flags |= B_WANTED;
				error = tsleep((caddr_t)bp,
					(PRIBIO + 1), "lfs_vinval", 0);
				if (error) {
					splx(s);
					return (error);
				}
				goto top;
			}

			bp->b_flags |= B_BUSY;
			if((bp->b_lblkno >= 0 && bp->b_lblkno > maxblk)
			   || (bp->b_lblkno < 0 && bp->b_lblkno < -maxblk-(NIADDR-1)))
			{
				bp->b_flags |= B_INVAL | B_VFLUSH;
				if(bp->b_flags & B_CALL) {
					lfs_freebuf(bp);
				} else {
					brelse(bp);
				}
				++dirty;
			} else {
				/*
				 * This buffer is still on its free list.
				 * So don't brelse, but wake up any sleepers.
				 */
				bp->b_flags &= ~B_BUSY;
				if(bp->b_flags & B_WANTED) {
					bp->b_flags &= ~(B_WANTED|B_AGE);
					wakeup(bp);
				}
			}
		}
		splx(s);
	}
	if(dirty)
		goto top;
	return (0);
}
