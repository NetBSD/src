/*
 * Copyright (c) 1994 Christian E. Hopps
 * All rights reserved.
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
 *      This product includes software developed by Christian E. Hopps.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 *	$Id: advnops.c,v 1.4 1994/05/25 11:34:15 chopps Exp $
 */
#include <sys/param.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <sys/namei.h>
#include <sys/buf.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <miscfs/specfs/specdev.h>
#include <adosfs/adosfs.h>

extern struct vnodeops adosfs_vnodeops;

int
adosfs_open(vp, mod, ucp, p)
	struct vnode *vp;
	int mod;
	struct ucred *ucp;
	struct proc *p;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("(open [0x%x %d 0x%x 0x%x] 0)", vp, mod, ucp, p);
#endif
	return(0);
}

int
adosfs_close(vp, fflag, ucp, p)
	struct vnode *vp;
	int fflag;
	struct ucred *ucp;
	struct proc *p;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("(close [0x%x %d 0x%x 0x%x] 0)", vp, fflag, ucp, p);
#endif
	return(0);
}

int 
adosfs_create(ndp, vap, p)
	struct nameidata *ndp;
	struct vattr *vap;
	struct proc *p;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("(create EOPNOTSUPP)");
#endif
	return(EOPNOTSUPP);
}

int 
adosfs_remove(ndp, p)
	struct nameidata *ndp;
	struct proc *p;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("(remove EOPNOTSUPP)");
#endif
	return(EOPNOTSUPP);
}

int
adosfs_getattr(vp, vap, ucp, p)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *ucp;
	struct proc *p;
{
	struct amount *amp;
	struct anode *ap;
	u_long fblks;

#ifdef ADOSFS_DIAGNOSTIC
	printf("(getattr [0x%x 0x%x 0x%x 0x%x] ", vp, vap, ucp, p);
#endif
	ap = VTOA(vp);
	amp = ap->amp;
	vattr_null(vap);
	vap->va_uid = amp->uid;
	vap->va_gid = amp->gid;
	vap->va_fsid = vp->v_mount->mnt_stat.f_fsid.val[0];
	microtime(&vap->va_atime);
	vap->va_mtime = vap->va_atime;
	vap->va_ctime = vap->va_ctime;
	vap->va_gen = 0;
	vap->va_flags = 0;
	vap->va_rdev = NODEV;
	vap->va_fileid = ap->block;
	vap->va_type = vp->v_type;
	vap->va_mode = amp->mask & adunixprot(ap->adprot);
	if (vp->v_type == VDIR) {
		vap->va_nlink = 1;	/* XXX bogus, oh well */
		vap->va_bytes = amp->bsize;
		vap->va_size = amp->bsize;
	} else {
		/* 
		 * XXX actually we can track this if we were to walk the list
		 * of links if it exists.
		 */
		vap->va_nlink = 1;
		/*
		 * round up to nearest blocks add number of file list 
		 * blocks needed and mutiply by number of bytes per block.
		 */
		fblks = howmany(ap->fsize, amp->bsize);
		fblks += howmany(fblks, ANODENDATBLKENT(ap));
		vap->va_bytes = fblks * amp->bsize;
		vap->va_size = ap->fsize;
	}
#ifdef ADOSFS_DIAGNOSTIC
	printf(" 0)");
#endif
	return(0);
}
/*
 * are things locked??? they need to be to avoid this being 
 * deleted or changed (data block pointer blocks moving about.)
 */
int
adosfs_read(vp, uio, ioflag, ucp)
	struct vnode *vp;
	struct uio *uio;
	int ioflag;
	struct ucred *ucp;
{
	struct anode *ap;
	struct amount *amp;
	struct fs *fs;
	struct buf *bp;
	daddr_t lbn, bn;
	int size, diff, error;
	long n, on;

#ifdef ADOSFS_DIAGNOSTIC
	printf("(read [0x%x 0x%x %d 0x %x]", vp, uio, ioflag, ucp);
#endif
	error = 0;
	ap = VTOA(vp);
	amp = ap->amp;	
	/*
	 * Return EOF for character devices, EIO for others
	 */
	if (vp->v_type != VREG) {
		error = EIO;
		goto reterr;
	}
	if (uio->uio_resid == 0)
		goto reterr;
	if (uio->uio_offset < 0) {
		error = EINVAL;
		goto reterr;
	}

	/*
	 * to expensive to let general algorithm figure out that 
	 * we are beyond the file.  Do it now.
	 */
	if (uio->uio_offset >= ap->fsize)
		goto reterr;

	/*
	 * taken from ufs_read()
	 */
	do {
		/*
		 * we are only supporting ADosFFS currently
		 * (which have data blocks of 512 bytes)
		 */
		size = amp->bsize;
		lbn = uio->uio_offset / size;
		on = uio->uio_offset % size;
		n = min((u_int)(size - on), uio->uio_resid);
		diff = ap->fsize - uio->uio_offset;
		/* 
		 * check for EOF
		 */
		if (diff <= 0)
			return(0);
		if (diff < n)
			n = diff;
		/*
		 * read ahead could possibly be worth something
		 * but not much as ados makes little attempt to 
		 * make things contigous
		 */
		error = bread(vp, lbn, size, NOCRED, &bp);
		vp->v_lastr = lbn;
		n = min(n, (u_int)size - bp->b_resid);
		if (error) {
			brelse(bp);
			goto reterr;
		}
#ifdef ADOSFS_DIAGNOSTIC
	printf(" %d+%d-%d+%d", lbn, on, lbn, n);
#endif
		error = uiomove(bp->b_un.b_addr + on, (int)n, uio);
		brelse(bp);
	} while (error == 0 && uio->uio_resid > 0 && n != 0);
reterr:
#ifdef ADOSFS_DIAGNOSTIC
	printf(" %d)", error);
#endif
	return(error);
}

int
adosfs_write(vp, uio, ioflag, ucp)
	register struct vnode *vp;
	struct uio *uio;
	int ioflag;
	struct ucred *ucp;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("(write EOPNOTSUPP)");
#endif
	return(EOPNOTSUPP);
}

/*
 * Device ioctl operation.
 */
/* ARGSUSED */
int
adosfs_ioctl(vp, com, data, fflag, ucp, p)
	struct vnode *vp;
	register int com;
	caddr_t data;
	int fflag;
	struct ucred *ucp;
	struct proc *p;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("(ioctl ENOTTY)");
#endif
	return(ENOTTY);
}

/* ARGSUSED */
int
adosfs_select(vp, which, fflags, ucp, p)
	struct vnode *vp;
	int which, fflags;
	struct ucred *ucp;
	struct proc *p;
{
	/*
	 * sure there's something to read...
	 */
#ifdef ADOSFS_DIAGNOSTIC
	printf("(select [0x%x %d 0x%x 0x%x 0x%x] 1)", vp, which, fflags, ucp
	    p);
#endif
	return(1);
}

/*
 * Just call the device strategy routine
 */
int
adosfs_strategy(bp)
	struct buf *bp;
{
	struct anode *ap;
	struct vnode *vp;
	int error;

#ifdef ADOSFS_DIAGNOSTIC
	printf("(strategy [0x%x]", bp);
#endif
	error = 0;
	if (bp->b_vp == NULL) {
		bp->b_flags |= B_ERROR;
		biodone(bp);
		error = EIO;
		goto reterr;
	}
	if ((long)bp->b_blkno == -1) {
		biodone(bp);
		goto reterr;
	}
	vp = bp->b_vp;
	ap = VTOA(vp);
	if (bp->b_blkno == bp->b_lblkno)
		if (error = adosfs_bmap(vp, bp->b_lblkno,NULL, &bp->b_blkno)) {
			bp->b_flags |= B_ERROR;
			biodone(bp);
			goto reterr;
		}
	vp = ap->amp->devvp;
	bp->b_dev = vp->v_rdev;
	(*(vp->v_op->vop_strategy))(bp);
reterr:
#ifdef ADOSFS_DIAGNOSTIC
	printf(" %d)", error);
#endif
	return(error);
}

/*
 * lock the vnode 
 */
int
adosfs_lock(vp)
	struct vnode *vp;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("(lock [0x%x]", vp);
#endif
	ALOCK(VTOA(vp));

#ifdef ADOSFS_DIAGNOSTIC
	printf(" 0)");
#endif
	return(0);
}

int
adosfs_unlock(vp)
	struct vnode *vp;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("(unlock [0x%x]", vp);
#endif
	AUNLOCK(VTOA(vp));

#ifdef ADOSFS_DIAGNOSTIC
	printf(" 0)");
#endif
	return(0);
}


/*
 * Wait until the vnode has finished changing state.
 */
int
adosfs_bmap(vp, bn, vpp, bnp)
	struct vnode *vp;
	daddr_t bn;
	struct vnode **vpp;
	daddr_t *bnp;
{
	struct anode *ap;
	struct buf *flbp;
	long nb, flblk, flblkoff;
	int error; 

#ifdef ADOSFS_DIAGNOSTIC
	printf("(bmap [0x%x %d 0x%x 0x%x]", vp, bn, vpp, bnp);
#endif

	error = 0;
	ap = VTOA(vp);

	if (bn < 0) {
		error = EFBIG;
		goto reterr;
	}
	if (vpp != NULL)
		*vpp = ap->amp->devvp;
	if (bnp == NULL)
		goto reterr;
	if (vp->v_type != VREG) {
		error = EINVAL;
		goto reterr;
	}

	/*
	 * walk the chain of file list blocks until we find
	 * the one that will yield the block pointer we need.
	 */
	if (ap->type == AFILE)
		nb = ap->block;			/* pointer to ourself */
	else if (ap->type == ALFILE)
		nb = ap->linkto;		/* pointer to real file */
	else {
		error = EINVAL;
		goto reterr;
	}

	flblk = bn / ANODENDATBLKENT(ap);
	flbp = NULL;
	while (flblk >= 0) {
		if (flbp)
			brelse(flbp);
		if (nb == 0) {
#ifdef DIAGNOSTIC
			printf("adosfs: bad file list chain.\n");
#endif
			error = EINVAL;
			goto reterr;
		}
		if (error = bread(ap->amp->devvp, nb, ap->amp->bsize, 
		    NOCRED, &flbp))
			goto reterr;
		if (adoscksum(flbp, ap->nwords)) {
#ifdef DIAGNOSTIC
			printf("adosfs: blk %d failed cksum.\n", nb);
#endif
			brelse(flbp);
			error = EINVAL;
			goto reterr;
		}
		nb = adoswordn(flbp, ap->nwords - 2);
		flblk--;
	}
	/* 
	 * calculate offset of block number in table.  The table starts
	 * at nwords - 51 and goes to offset 6 or less if indicated by the
	 * valid table entries stored at offset ADBI_NBLKTABENT.
	 */
	flblkoff = bn % ANODENDATBLKENT(ap);
	if (flblkoff < adoswordn(flbp, 2 /* ADBI_NBLKTABENT */)) {
		flblkoff = (ap->nwords - 51) - flblkoff;
		*bnp = adoswordn(flbp, flblkoff);
	} else {
#ifdef DIAGNOSTIC
		printf("flblk offset %d too large in lblk %d blk %d\n", 
		    flblkoff, bn, flbp->b_blkno);
#endif
		error = EINVAL;
	}
	brelse(flbp);
reterr:
#ifdef ADOSFS_DIAGNOSTIC
	if (error == 0 && bnp)
		printf(" %d => %d", bn, *bnp);
	printf(" %d)", error);
#endif
	return(error);
}

/*
 * Print out the contents of a adosfs vnode.
 */
/* ARGSUSED */
int
adosfs_print(vp)
	struct vnode *vp;
{
	printf("print 0x%x\n", vp);
	return(0);
}

struct adirent {
	u_long  fileno;
	u_short reclen;
	u_short namlen;
	char    name[32];	/* maxlen of 30 plus 2 NUL's */
};
	
int 
adosfs_readdir(pvp, uio, ucp, eofp, cookies, ncookies)
	struct vnode *pvp;
	struct uio *uio;
	struct ucred *ucp;
	int *eofp, ncookies;
	u_int *cookies;
{
	int error, useri, chainc, hashi, scanned, uavail;
	struct adirent ad, *adp;
	struct anode *pap, *ap;
	struct amount *amp;
	u_long nextbn, resid;
	off_t uoff;

#ifdef ADOSFS_DIAGNOSTIC
	printf("(readdir [0x%x 0x%x 0x%x 0x%x 0x%x %d]", pvp, uio, ucp, eofp,
	    cookies, ncookies);
#endif
	if (pvp->v_type != VDIR) {
		error = ENOTDIR;
		goto reterr;
	}

	uoff = uio->uio_offset;
	if (uoff < 0) {
		error = EINVAL;
		goto reterr;
	}

	pap = VTOA(pvp);
	amp = pap->amp;
	adp = &ad;
	error = nextbn = hashi = chainc = scanned = 0;
	uavail = uio->uio_resid / sizeof(ad);
	useri = uoff / sizeof(ad);

	/*
	 * if no slots available or offset requested is not on a slot boundry
	 */
	if (uavail < 1 || uoff % sizeof(ad)) {
		error = EINVAL;
		goto reterr;
	}

	while (uavail && (cookies == NULL || ncookies > 0)) {
		if (hashi == pap->ntabent) {
			*eofp = 1;
			break;
		}
		if (pap->tab[hashi] == 0) {
			hashi++;
			continue;
		}
		if (nextbn == 0)
			nextbn = pap->tab[hashi];

		/*
		 * first determine if we can skip this chain
		 */
		if (chainc == 0) {
			int skip;

			skip = useri - scanned;
			if (pap->tabi[hashi] > 0 && pap->tabi[hashi] <= skip) {
				scanned += pap->tabi[hashi];
				hashi++;
				nextbn = 0;
				continue;
			}
		}

		/*
		 * now [continue to] walk the chain
		 */
		ap = NULL;
		do {
			if (error = aget(amp->mp, nextbn, &ap))
				goto reterr;
			scanned++;
			chainc++;
			nextbn = ap->hashf;

			/*
			 * check for end of chain.
			 */
			if (nextbn == 0) {
				pap->tabi[hashi] = chainc;
				hashi++;
				chainc = 0;
			} else if (pap->tabi[hashi] <= 0 &&
			    -chainc < pap->tabi[hashi])
				pap->tabi[hashi] = -chainc;

			if (useri >= scanned) {
				aput(ap);
				ap = NULL;
			}
		} while (ap == NULL && nextbn != 0);

		/*
		 * we left the loop but without a result so do main over.
		 */
		if (ap == NULL)
			continue;
		/*
		 * Fill in dirent record
		 */
		bzero(adp, sizeof(struct adirent));
		adp->fileno = ap->block;
		adp->reclen = sizeof(struct adirent);
		adp->namlen = strlen(ap->name);
		bcopy(ap->name, adp->name, adp->namlen);
		aput(ap);

		error = uiomove(adp, sizeof(struct adirent), uio);
		if (error)
			break;
		if (cookies) {
			*cookies++ = uoff;
			ncookies--;
		}
		uoff += sizeof(struct adirent);
		useri++;
		uavail--;
	}
#if doesnt_uiomove_handle_this
	uio->uio_offset = uoff;
#endif
reterr:
#ifdef ADOSFS_DIAGNOSTIC
	printf(" %d)", error);
#endif
	return(error);
}

/*ARGSUSED*/
int 
adosfs_mknod(ndp, vap, ucp, p)
	struct nameidata *ndp;
	struct vattr *vap;
	struct ucred *ucp; 
	struct proc *p;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("(mknod EOPNOTSUPP)");
#endif
	return(EOPNOTSUPP);
}

/*ARGSUSED*/
int
adosfs_access(vp, mode, ucp, p)
	struct vnode *vp;
	int mode;
	struct ucred *ucp;
	struct proc *p;
{
	struct anode *ap;
	gid_t *gp;
	int i, error;

#ifdef ADOSFS_DIAGNOSTIC
	printf("(access [0x%x %d 0x%x 0x%x]", vp, mode, ucp, p);
#endif

	error = 0;
	ap = VTOA(vp);
#ifdef DIAGNOSTIC
	if (!VOP_ISLOCKED(vp)) {
		vprint("adosfs_access: not locked", vp);
		panic("adosfs_access: not locked");
	}
#endif
#ifdef QUOTA
#endif
	/*
	 * super-user always gets a go ahead (suser()?)
	 */
	if (suser(ucp, NULL) == 0)
		return(0);

	/*
	 * not the owner of the mount?
	 */
	if (ucp->cr_uid != ap->amp->uid) {
		mode >>= 3;
		gp = ucp->cr_groups;
		for (i = 0; i < ucp->cr_ngroups; i++, gp++)
			if (ap->amp->gid == *gp)
				goto found;
		/*
		 * check other status
		 */
		mode >>= 3;
	}
found:
	if ((adunixprot(ap->adprot) & mode) != mode)
		error = EACCES;
#ifdef ADOSFS_DIAGNOSTIC
	printf(" %d)", error);
#endif
	return(error);
}

/*ARGSUSED*/
int
adosfs_setattr(vp, vap, ucp, p)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *ucp;
	struct proc *p;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("(setattr EOPNOTSUPP)");
#endif
	return(EOPNOTSUPP);
}

/*ARGSUSED*/
int
adosfs_mmap(vp, fflags, ucp, p)
	struct vnode *vp;
	int fflags;
	struct ucred *ucp;
	struct proc *p;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("(mmap EINVAL)");
#endif
	return(EINVAL);
}

/*ARGSUSED*/
int
adosfs_fsync(vp, fflags, ucp, waitfor, p)
	struct vnode *vp;
	int fflags, waitfor;
	struct ucred *ucp;
	struct proc *p;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("(fsync 0)");
#endif
	return(0);
}

/*ARGSUSED*/
int
adosfs_seek(vp, ooff, noff, ucp)
	struct vnode *vp;
	off_t ooff, noff;
	struct ucred *ucp;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("(seek 0)");
#endif
	return(0);
}

/*ARGSUSED*/
int
adosfs_link(vp, ndp, p)
	struct vnode *vp;
	struct nameidata *ndp;
	struct proc *p;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("(link EOPNOTSUPP)");
#endif
	return(EOPNOTSUPP);
}

/*ARGSUSED*/
int
adosfs_rename(ondp, nndp, p)
	struct nameidata *ondp;
	struct nameidata *nndp;
	struct proc *p;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("(rename EOPNOTSUPP)");
#endif
	return(EOPNOTSUPP);
}

/*ARGSUSED*/
int
adosfs_mkdir(ndp, vap, p)
	struct nameidata *ndp;
	struct vattr *vap;
	struct proc *p;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("(mkdir EOPNOTSUPP)");
#endif
	return(EOPNOTSUPP);
}

/*ARGSUSED*/
int
adosfs_rmdir(ndp, p)
	struct nameidata *ndp;
	struct proc *p;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("(rmdir EOPNOTSUPP)");
#endif
	return(EOPNOTSUPP);
}

/*ARGSUSED*/
int
adosfs_symlink(ndp, vap, targ, p)
	struct nameidata *ndp;
	struct vattr *vap;
	char *targ;
	struct proc *p;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("(symlink EOPNOTSUPP)");
#endif
	return(EOPNOTSUPP);
}

/*ARGSUSED*/
int
adosfs_readlink(vp, uio, ucp)
	struct vnode *vp;
	struct uio *uio;
	struct ucred *ucp;
{
	struct anode *ap;
	int error;

#ifdef ADOSFS_DIAGNOSTIC
	printf("(readlink [0x%x 0x%x 0x%x]", vp, uio, ucp);
#endif
	error = 0;
	ap = VTOA(vp);
	if (ap->type != ASLINK)
		error = EBADF;
	/*
	 * XXX Should this be NULL terminated?
	 */
	if (error == 0)
		error = uiomove(ap->slinkto, strlen(ap->slinkto) + 1, uio);
#ifdef ADOSFS_DIAGNOSTIC
	printf(" %d)", error);
#endif
	return(error);
}

/*ARGSUSED*/
int
adosfs_abortop(ndp)
	struct nameidata *ndp;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("(abortop EOPNOTSUPP)");
#endif
	return(EOPNOTSUPP);
}

/*ARGSUSED*/
int
adosfs_inactive(vp, p)
	struct vnode *vp;
	struct proc *p;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("(inactive ");
#endif
	if (vp->v_usecount == 0 && 0 /* check for file gone? */)
		vgone(vp);

#ifdef ADOSFS_DIAGNOSTIC
	printf(" 0)");
#endif
	return(0);
}

/*ARGSUSED*/
int
adosfs_advlock(vp, id, op, flp, flags)
	struct vnode *vp;
	caddr_t id;
	int op;
	struct flock *flp;
	int flags;
{
#ifdef ADOSFS_DIAGNOSTIC
	printf("(advlock EOPNOTSUPP)");
#endif
	return(EOPNOTSUPP);
}

/*ARGSUSED*/
int
adosfs_islocked(vp)
	struct vnode *vp;
{
	int locked;
#ifdef ADOSFS_DIAGNOSTIC
	printf("(islocked [0x%x]", vp);
#endif
	locked = (VTOA(vp)->flags & ALOCKED) == ALOCKED;

#ifdef ADOSFS_DIAGNOSTIC
	printf(" %d)", locked);
#endif
	return(locked);
}

int adosfs_lookup __P((struct vnode *, struct nameidata *, struct proc *));
int adosfs_reclaim __P((struct vnode *));

/*
 * vnode indirect function call structure.
 */
struct vnodeops adosfs_vnodeops = {
	adosfs_lookup,	/* lookup */
	adosfs_create,	/* create */
	adosfs_mknod,	/* mknod */
	adosfs_open,	/* open */
	adosfs_close,	/* close */
	adosfs_access,	/* access */
	adosfs_getattr,	/* getattr */
	adosfs_setattr,	/* setattr */
	adosfs_read,	/* read */
	adosfs_write,	/* write */
	adosfs_ioctl,	/* ioctl */
	adosfs_select,	/* select */
	adosfs_mmap,	/* mmap */
	adosfs_fsync,	/* fsync */
	adosfs_seek,	/* seek */
	adosfs_remove,	/* remove */
	adosfs_link,	/* link */
	adosfs_rename,	/* rename */
	adosfs_mkdir,	/* mkdir */
	adosfs_rmdir,	/* rmdir */
	adosfs_symlink,	/* symlink */
	adosfs_readdir,	/* readdir */
	adosfs_readlink,/* readlink */
	adosfs_abortop,	/* abortop */
	adosfs_inactive,/* inactive */
	adosfs_reclaim,	/* reclaim */
	adosfs_lock,	/* lock */
	adosfs_unlock,	/* unlock */
	adosfs_bmap,	/* bmap */
	adosfs_strategy,/* strategy */
	adosfs_print,	/* print */
	adosfs_islocked,/* islocked */
	adosfs_advlock,	/* advlock */
};
