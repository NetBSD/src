/*	$NetBSD: ufs_readwrite.c,v 1.31 2001/03/26 06:47:34 chs Exp $	*/

/*-
 * Copyright (c) 1993
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
 *	@(#)ufs_readwrite.c	8.11 (Berkeley) 5/8/95
 */

#ifdef LFS_READWRITE
#define	BLKSIZE(a, b, c)	blksize(a, b, c)
#define	FS			struct lfs
#define	I_FS			i_lfs
#define	READ			lfs_read
#define	READ_S			"lfs_read"
#define	WRITE			lfs_write
#define	WRITE_S			"lfs_write"
#define	fs_bsize		lfs_bsize
#define	fs_maxfilesize		lfs_maxfilesize
#else
#define	BLKSIZE(a, b, c)	blksize(a, b, c)
#define	FS			struct fs
#define	I_FS			i_fs
#define	READ			ffs_read
#define	READ_S			"ffs_read"
#define	WRITE			ffs_write
#define	WRITE_S			"ffs_write"
#endif

/*
 * Vnode op for reading.
 */
/* ARGSUSED */
int
READ(void *v)
{
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		struct ucred *a_cred;
	} */ *ap = v;
	struct vnode *vp;
	struct inode *ip;
	struct uio *uio;
	FS *fs;
#ifndef LFS_READWRITE
	void *win;
	vsize_t bytelen;
#endif
	struct buf *bp;
	ufs_daddr_t lbn, nextlbn;
	off_t bytesinfile;
	long size, xfersize, blkoffset;
	int error;

	vp = ap->a_vp;
	ip = VTOI(vp);
	uio = ap->a_uio;
	error = 0;

#ifdef DIAGNOSTIC
	if (uio->uio_rw != UIO_READ)
		panic("%s: mode", READ_S);

	if (vp->v_type == VLNK) {
		if ((int)ip->i_ffs_size < vp->v_mount->mnt_maxsymlinklen ||
		    (vp->v_mount->mnt_maxsymlinklen == 0 &&
		     ip->i_ffs_blocks == 0))
			panic("%s: short symlink", READ_S);
	} else if (vp->v_type != VREG && vp->v_type != VDIR)
		panic("%s: type %d", READ_S, vp->v_type);
#endif
	fs = ip->I_FS;
	if ((u_int64_t)uio->uio_offset > fs->fs_maxfilesize)
		return (EFBIG);
	if (uio->uio_resid == 0)
		return (0);
	if (uio->uio_offset >= ip->i_ffs_size) {
		goto out;
	}

#ifndef LFS_READWRITE
	if (vp->v_type == VREG) {
		while (uio->uio_resid > 0) {
			bytelen = MIN(ip->i_ffs_size - uio->uio_offset,
			    uio->uio_resid);
			if (bytelen == 0)
				break;

			win = ubc_alloc(&vp->v_uvm.u_obj, uio->uio_offset,
					&bytelen, UBC_READ);
			error = uiomove(win, bytelen, uio);
			ubc_release(win, 0);
			if (error)
				break;
		}
		goto out;
	}
#endif

	for (error = 0, bp = NULL; uio->uio_resid > 0; bp = NULL) {
		bytesinfile = ip->i_ffs_size - uio->uio_offset;
		if (bytesinfile <= 0)
			break;
		lbn = lblkno(fs, uio->uio_offset);
		nextlbn = lbn + 1;
		size = BLKSIZE(fs, ip, lbn);
		blkoffset = blkoff(fs, uio->uio_offset);
		xfersize = MIN(MIN(fs->fs_bsize - blkoffset, uio->uio_resid),
		    bytesinfile);

#ifdef LFS_READWRITE
		(void)lfs_check(vp, lbn, 0);
		error = cluster_read(vp, ip->i_ffs_size, lbn, size, NOCRED, &bp);
#else
		if (lblktosize(fs, nextlbn) >= ip->i_ffs_size)
			error = bread(vp, lbn, size, NOCRED, &bp);
		else if (lbn - 1 == vp->v_lastr) {
			int nextsize = BLKSIZE(fs, ip, nextlbn);
			error = breadn(vp, lbn,
			    size, &nextlbn, &nextsize, 1, NOCRED, &bp);
		} else
			error = bread(vp, lbn, size, NOCRED, &bp);
#endif
		if (error)
			break;
		vp->v_lastr = lbn;

		/*
		 * We should only get non-zero b_resid when an I/O error
		 * has occurred, which should cause us to break above.
		 * However, if the short read did not cause an error,
		 * then we want to ensure that we do not uiomove bad
		 * or uninitialized data.
		 */
		size -= bp->b_resid;
		if (size < xfersize) {
			if (size == 0)
				break;
			xfersize = size;
		}
		error = uiomove((char *)bp->b_data + blkoffset, xfersize, uio);
		if (error)
			break;
		brelse(bp);
	}
	if (bp != NULL)
		brelse(bp);

 out:
	if (!(vp->v_mount->mnt_flag & MNT_NOATIME)) {
		ip->i_flag |= IN_ACCESS;
		if ((ap->a_ioflag & IO_SYNC) == IO_SYNC)
			error = VOP_UPDATE(vp, NULL, NULL, UPDATE_WAIT);
	}
	return (error);
}

/*
 * Vnode op for writing.
 */
int
WRITE(void *v)
{
	struct vop_write_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		struct ucred *a_cred;
	} */ *ap = v;
	struct vnode *vp;
	struct uio *uio;
	struct inode *ip;
	FS *fs;
	struct buf *bp;
	struct proc *p;
	ufs_daddr_t lbn;
	off_t osize;
	int blkoffset, error, flags, ioflag, resid, size, xfersize;
#ifndef LFS_READWRITE
	void *win;
	vsize_t bytelen;
	off_t oldoff;
	boolean_t rv;
#endif

	ioflag = ap->a_ioflag;
	uio = ap->a_uio;
	vp = ap->a_vp;
	ip = VTOI(vp);

#ifdef DIAGNOSTIC
	if (uio->uio_rw != UIO_WRITE)
		panic("%s: mode", WRITE_S);
#endif

	switch (vp->v_type) {
	case VREG:
		if (ioflag & IO_APPEND)
			uio->uio_offset = ip->i_ffs_size;
		if ((ip->i_ffs_flags & APPEND) && uio->uio_offset != ip->i_ffs_size)
			return (EPERM);
		/* FALLTHROUGH */
	case VLNK:
		break;
	case VDIR:
		if ((ioflag & IO_SYNC) == 0)
			panic("%s: nonsync dir write", WRITE_S);
		break;
	default:
		panic("%s: type", WRITE_S);
	}

	fs = ip->I_FS;
	if (uio->uio_offset < 0 ||
	    (u_int64_t)uio->uio_offset + uio->uio_resid > fs->fs_maxfilesize)
		return (EFBIG);
#ifdef LFS_READWRITE
	/* Disallow writes to the Ifile, even if noschg flag is removed */
	/* XXX can this go away when the Ifile is no longer in the namespace? */
	if (vp == fs->lfs_ivnode)
		return (EPERM);
#endif

	/*
	 * Maybe this should be above the vnode op call, but so long as
	 * file servers have no limits, I don't think it matters.
	 */
	p = uio->uio_procp;
	if (vp->v_type == VREG && p &&
	    uio->uio_offset + uio->uio_resid >
	    p->p_rlimit[RLIMIT_FSIZE].rlim_cur) {
		psignal(p, SIGXFSZ);
		return (EFBIG);
	}

	resid = uio->uio_resid;
	osize = ip->i_ffs_size;
	error = 0;

#ifndef LFS_READWRITE
	if (vp->v_type != VREG) {
		goto bcache;
	}

	while (uio->uio_resid > 0) {
		oldoff = uio->uio_offset;
		blkoffset = blkoff(fs, uio->uio_offset);
		bytelen = MIN(fs->fs_bsize - blkoffset, uio->uio_resid);

		/*
		 * XXXUBC if file is mapped and this is the last block,
		 * process one page at a time.
		 */

		error = ufs_balloc_range(vp, uio->uio_offset, bytelen,
		    ap->a_cred, ioflag & IO_SYNC ? B_SYNC : 0);
		if (error) {
			return error;
		}

		win = ubc_alloc(&vp->v_uvm.u_obj, uio->uio_offset, &bytelen,
				UBC_WRITE);
		error = uiomove(win, bytelen, uio);
		ubc_release(win, 0);

		/*
		 * flush what we just wrote if necessary.
		 * XXXUBC simplistic async flushing.
		 */

		if (ioflag & IO_SYNC) {
			simple_lock(&vp->v_uvm.u_obj.vmobjlock);
#if 1
			/*
			 * XXX 
			 * flush whole blocks in case there are deps.
			 * otherwise we can dirty and flush part of
			 * a block multiple times and the softdep code
			 * will get confused.  fixing this the right way
			 * is complicated so we'll work around it for now.
			 */

			rv = vp->v_uvm.u_obj.pgops->pgo_flush(
			    &vp->v_uvm.u_obj,
			    oldoff & ~(fs->fs_bsize - 1),
			    (oldoff + bytelen + fs->fs_bsize - 1) &
			    ~(fs->fs_bsize - 1),
			    PGO_CLEANIT|PGO_SYNCIO);
#else
			rv = vp->v_uvm.u_obj.pgops->pgo_flush(
			    &vp->v_uvm.u_obj, oldoff, oldoff + bytelen,
			    PGO_CLEANIT|PGO_SYNCIO);
#endif
			simple_unlock(&vp->v_uvm.u_obj.vmobjlock);
		} else if (oldoff >> 16 != uio->uio_offset >> 16) {
			simple_lock(&vp->v_uvm.u_obj.vmobjlock);
			rv = vp->v_uvm.u_obj.pgops->pgo_flush(
			    &vp->v_uvm.u_obj, (oldoff >> 16) << 16,
			    (uio->uio_offset >> 16) << 16, PGO_CLEANIT);
			simple_unlock(&vp->v_uvm.u_obj.vmobjlock);
		}
		if (error) {
			break;
		}
	}
	goto out;

 bcache:
#endif
	flags = ioflag & IO_SYNC ? B_SYNC : 0;
	while (uio->uio_resid > 0) {
		lbn = lblkno(fs, uio->uio_offset);
		blkoffset = blkoff(fs, uio->uio_offset);
		xfersize = MIN(fs->fs_bsize - blkoffset, uio->uio_resid);
		if (fs->fs_bsize > xfersize)
			flags |= B_CLRBUF;
		else
			flags &= ~B_CLRBUF;

		error = VOP_BALLOC(vp, uio->uio_offset, xfersize,
		    ap->a_cred, flags, &bp);

		if (error)
			break;
		if (uio->uio_offset + xfersize > ip->i_ffs_size) {
			ip->i_ffs_size = uio->uio_offset + xfersize;
			uvm_vnp_setsize(vp, ip->i_ffs_size);
		}
		size = BLKSIZE(fs, ip, lbn) - bp->b_resid;
		if (xfersize > size)
			xfersize = size;

		error = uiomove((char *)bp->b_data + blkoffset, xfersize, uio);

		/*
		 * if we didn't clear the block and the uiomove failed,
		 * the buf will now contain part of some other file,
		 * so we need to invalidate it.
		 */
		if (error && (flags & B_CLRBUF) == 0) {
			bp->b_flags |= B_INVAL;
			brelse(bp);
			break;
		}
#ifdef LFS_READWRITE
		if (!error)
			error = lfs_reserve(fs, vp, fsbtodb(fs, NIADDR + 1));
		(void)VOP_BWRITE(bp);
		if (!error)
			lfs_reserve(fs, vp, fsbtodb(fs, -(NIADDR + 1)));
#else
		if (ioflag & IO_SYNC)
			(void)bwrite(bp);
		else if (xfersize + blkoffset == fs->fs_bsize)
			if (doclusterwrite)
				cluster_write(bp, ip->i_ffs_size);
			else
				bawrite(bp);
		else
			bdwrite(bp);
#endif
		if (error || xfersize == 0)
			break;
	}
	/*
	 * If we successfully wrote any data, and we are not the superuser
	 * we clear the setuid and setgid bits as a precaution against
	 * tampering.
	 */
#ifndef LFS_READWRITE
 out:
#endif
	ip->i_flag |= IN_CHANGE | IN_UPDATE;
	if (resid > uio->uio_resid && ap->a_cred && ap->a_cred->cr_uid != 0)
		ip->i_ffs_mode &= ~(ISUID | ISGID);
	if (error) {
		if (ioflag & IO_UNIT) {
			(void)VOP_TRUNCATE(vp, osize,
			    ioflag & IO_SYNC, ap->a_cred, uio->uio_procp);
			uio->uio_offset -= resid - uio->uio_resid;
			uio->uio_resid = resid;
		}
	} else if (resid > uio->uio_resid && (ioflag & IO_SYNC) == IO_SYNC)
		error = VOP_UPDATE(vp, NULL, NULL, UPDATE_WAIT);
	return (error);
}
