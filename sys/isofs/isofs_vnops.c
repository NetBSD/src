/*
 *	$Id: isofs_vnops.c,v 1.12 1993/12/18 04:32:04 mycroft Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/resourcevar.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/dir.h>

#include <miscfs/specfs/specdev.h> /* XXX */
#include <miscfs/fifofs/fifo.h> /* XXX */

#include <isofs/iso.h>
#include <isofs/isofs_node.h>
#include <isofs/iso_rrip.h>

/*
 * Mknod vnode call
 *  Actually remap the device number
 */
/* ARGSUSED */
isofs_mknod(ndp, vap, cred, p)
	struct nameidata *ndp;
	struct ucred *cred;
	struct vattr *vap;
	struct proc *p;
{
#ifndef	ISODEVMAP
	free(ndp->ni_pnbuf, M_NAMEI);
	vput(ndp->ni_dvp);
	vput(ndp->ni_vp);
	return EINVAL;
#else
	register struct vnode *vp;
	struct iso_node *ip;
	struct iso_dnode *dp;
	int error;
	
	vp = ndp->ni_vp;
	ip = VTOI(vp);
	
	if (ip->i_mnt->iso_ftype != ISO_FTYPE_RRIP
	    || vap->va_type != vp->v_type
	    || (vap->va_type != VCHR && vap->va_type != VBLK)) {
		free(ndp->ni_pnbuf, M_NAMEI);
		vput(ndp->ni_dvp);
		vput(ndp->ni_vp);
		return EINVAL;
	}
	
	dp = iso_dmap(ip->i_dev,ip->i_number,1);
	if (ip->inode.iso_rdev == vap->va_rdev || vap->va_rdev == VNOVAL) {
		/* same as the unmapped one, delete the mapping */
		remque(dp);
		FREE(dp,M_CACHE);
	} else
		/* enter new mapping */
		dp->d_dev = vap->va_rdev;
	
	/*
	 * Remove inode so that it will be reloaded by iget and
	 * checked to see if it is an alias of an existing entry
	 * in the inode cache.
	 */
	vput(vp);
	vp->v_type = VNON;
	vgone(vp);
	return (0);
#endif
}

/*
 * Open called.
 *
 * Nothing to do.
 */
/* ARGSUSED */
isofs_open(vp, mode, cred, p)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
	struct proc *p;
{
	return (0);
}

/*
 * Close called
 *
 * Update the times on the inode on writeable file systems.
 */
/* ARGSUSED */
isofs_close(vp, fflag, cred, p)
	struct vnode *vp;
	int fflag;
	struct ucred *cred;
	struct proc *p;
{
	return (0);
}

/*
 * Check mode permission on inode pointer. Mode is READ, WRITE or EXEC.
 * The mode is shifted to select the owner/group/other fields. The
 * super user is granted all permissions.
 */
isofs_access(vp, mode, cred, p)
	struct vnode *vp;
	register int mode;
	struct ucred *cred;
	struct proc *p;
{
	return (0);
}

/* ARGSUSED */
isofs_getattr(vp, vap, cred, p)
	struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
	struct proc *p;
{
	register struct iso_node *ip = VTOI(vp);
	int i;

	vap->va_fsid	= ip->i_dev;
	vap->va_fileid	= ip->i_number;

	vap->va_mode	= ip->inode.iso_mode;
	vap->va_nlink	= ip->inode.iso_links;
	vap->va_uid	= ip->inode.iso_uid;
	vap->va_gid	= ip->inode.iso_gid;
	vap->va_atime	= ip->inode.iso_atime;
	vap->va_mtime	= ip->inode.iso_mtime;
	vap->va_ctime	= ip->inode.iso_ctime;
	vap->va_rdev	= ip->inode.iso_rdev;

	vap->va_size	= ip->i_size;
	vap->va_size_rsv = 0;
	vap->va_flags	= 0;
	vap->va_gen = 1;
	vap->va_blocksize = ip->i_mnt->logical_block_size;
	vap->va_bytes	= ip->i_size;
	vap->va_bytes_rsv = 0;
	vap->va_type	= vp->v_type;
	return (0);
}

/*
 * Vnode op for reading.
 */
/* ARGSUSED */
isofs_read(vp, uio, ioflag, cred)
	struct vnode *vp;
	register struct uio *uio;
	int ioflag;
	struct ucred *cred;
{
	register struct iso_node *ip = VTOI(vp);
	register struct iso_mnt *imp;
	struct buf *bp;
	daddr_t lbn, bn, rablock;
	int size, diff, error = 0;
	long n, on, type;
	
	if (uio->uio_resid == 0)
		return (0);
	if (uio->uio_offset < 0)
		return (EINVAL);
	ip->i_flag |= IACC;
	imp = ip->i_mnt;
	do {
		lbn = iso_lblkno(imp, uio->uio_offset);
		on = iso_blkoff(imp, uio->uio_offset);
		n = MIN((unsigned)(imp->logical_block_size - on), uio->uio_resid);
		diff = ip->i_size - uio->uio_offset;
		if (diff <= 0)
			return (0);
		if (diff < n)
			n = diff;
		size = iso_blksize(imp, ip, lbn);
		rablock = lbn + 1;
		if (vp->v_lastr + 1 == lbn &&
		    iso_lblktosize(imp, rablock) < ip->i_size)
			error = breada(ITOV(ip), lbn, size, rablock,
				iso_blksize(imp, ip, rablock), NOCRED, &bp);
		else
			error = bread(ITOV(ip), lbn, size, NOCRED, &bp);
		vp->v_lastr = lbn;
		n = MIN(n, size - bp->b_resid);
		if (error) {
			brelse(bp);
			return (error);
		}

		error = uiomove(bp->b_un.b_addr + on, (int)n, uio);
		if (n + on == imp->logical_block_size || uio->uio_offset == ip->i_size)
			bp->b_flags |= B_AGE;
		brelse(bp);
	} while (error == 0 && uio->uio_resid > 0 && n != 0);
	return (error);
}

/* ARGSUSED */
isofs_ioctl(vp, com, data, fflag, cred, p)
	struct vnode *vp;
	int com;
	caddr_t data;
	int fflag;
	struct ucred *cred;
	struct proc *p;
{
	printf("You did ioctl for isofs !!\n");
	return (ENOTTY);
}

/* ARGSUSED */
isofs_select(vp, which, fflags, cred, p)
	struct vnode *vp;
	int which, fflags;
	struct ucred *cred;
	struct proc *p;
{

	/*
	 * We should really check to see if I/O is possible.
	 */
	return (1);
}

/*
 * Mmap a file
 *
 * NB Currently unsupported.
 */
/* ARGSUSED */
isofs_mmap(vp, fflags, cred, p)
	struct vnode *vp;
	int fflags;
	struct ucred *cred;
	struct proc *p;
{

	return (EINVAL);
}

/*
 * Seek on a file
 *
 * Nothing to do, so just return.
 */
/* ARGSUSED */
isofs_seek(vp, oldoff, newoff, cred)
	struct vnode *vp;
	off_t oldoff, newoff;
	struct ucred *cred;
{

	return (0);
}

/*
 * Structure for reading directories
 */
struct isoreaddir {
	struct dirent saveent;
	struct dirent assocent;
	struct dirent current;
	off_t saveoff;
	off_t assocoff;
	off_t curroff;
	struct uio *uio;
	off_t uio_off;
	u_int *cookiep;
	int ncookies;
	int eof;
};

static int
iso_uiodir(idp,dp,off)
	struct isoreaddir *idp;
	struct dirent *dp;
	off_t off;
{
	int error;
	
	dp->d_name[dp->d_namlen] = 0;
	dp->d_reclen = DIRSIZ(dp);
	
	if (idp->uio->uio_resid < dp->d_reclen) {
		idp->eof = 0;
		return -1;
	}
	
	if (idp->cookiep) {
		if (idp->ncookies <= 0) {
			idp->eof = 0;
			return -1;
		}
		
		*idp->cookiep++ = off;
		--idp->ncookies;
	}
	
	if (error = uiomove(dp,dp->d_reclen,idp->uio))
		return error;
	idp->uio_off = off;
	return 0;
}

static int
iso_shipdir(idp)
	struct isoreaddir *idp;
{
	struct dirent *dp;
	int cl, sl, assoc;
	int error;
	char *cname, *sname;
	
	cl = idp->current.d_namlen;
	cname = idp->current.d_name;
	if (assoc = cl > 1 && *cname == ASSOCCHAR) {
		cl--;
		cname++;
	}
	
	dp = &idp->saveent;
	sname = dp->d_name;
	if (!(sl = dp->d_namlen)) {
		dp = &idp->assocent;
		sname = dp->d_name + 1;
		sl = dp->d_namlen - 1;
	}
	if (sl > 0) {
		if (sl != cl
		    || bcmp(sname,cname,sl)) {
			if (idp->assocent.d_namlen) {
				if (error = iso_uiodir(idp,&idp->assocent,idp->assocoff))
					return error;
				idp->assocent.d_namlen = 0;
			}
			if (idp->saveent.d_namlen) {
				if (error = iso_uiodir(idp,&idp->saveent,idp->saveoff))
					return error;
				idp->saveent.d_namlen = 0;
			}
		}
	}
	idp->current.d_reclen = DIRSIZ(&idp->current);
	if (assoc) {
		idp->assocoff = idp->curroff;
		bcopy(&idp->current,&idp->assocent,idp->current.d_reclen);
	} else {
		idp->saveoff = idp->curroff;
		bcopy(&idp->current,&idp->saveent,idp->current.d_reclen);
	}
	return 0;
}

/*
 * Vnode op for readdir
 */
isofs_readdir(vp, uio, cred, eofflagp, cookies, ncookies)
	struct vnode *vp;
	register struct uio *uio;
	struct ucred *cred;
	int *eofflagp;
	u_int *cookies;
	int ncookies;
{
	struct isoreaddir *idp;
	int entryoffsetinblock;
	int error = 0;
	int endsearch;
	struct iso_directory_record *ep;
	int reclen;
	struct iso_mnt *imp;
	struct iso_node *ip;
	struct buf *bp = NULL;
	
	ip = VTOI (vp);
	imp = ip->i_mnt;
	
	MALLOC(idp,struct isoreaddir *,sizeof(*idp),M_TEMP,M_WAITOK);
	idp->saveent.d_namlen = 0;
	idp->assocent.d_namlen = 0;
	idp->uio = uio;
	idp->cookiep = cookies;
	idp->ncookies = ncookies;
	idp->curroff = uio->uio_offset;
	idp->eof = 1;
	
	entryoffsetinblock = iso_blkoff(imp, idp->curroff);
	if (entryoffsetinblock != 0) {
		if (error = iso_blkatoff(ip, idp->curroff, &bp)) {
			FREE(idp,M_TEMP);
			return (error);
		}
	}
	
	endsearch = ip->i_size;
	
	while (idp->curroff < endsearch) {
		/*
		 * If offset is on a block boundary,
		 * read the next directory block.
		 * Release previous if it exists.
		 */
		
		if (iso_blkoff(imp, idp->curroff) == 0) {
			if (bp != NULL)
				brelse(bp);
			if (error = iso_blkatoff(ip, idp->curroff, &bp))
				break;
			entryoffsetinblock = 0;
		}
		/*
		 * Get pointer to next entry.
		 */
		
		ep = (struct iso_directory_record *)
			(bp->b_un.b_addr + entryoffsetinblock);
		
		reclen = isonum_711 (ep->length);
		if (reclen == 0) {
			/* skip to next block, if any */
			idp->curroff = roundup (idp->curroff,
						imp->logical_block_size);
			continue;
		}
		
		if (reclen < ISO_DIRECTORY_RECORD_SIZE) {
			error = EINVAL;
			/* illegal entry, stop */
			break;
		}
		
		if (entryoffsetinblock + reclen >= imp->logical_block_size) {
			error = EINVAL;
			/* illegal directory, so stop looking */
			break;
		}
		
		idp->current.d_namlen = isonum_711 (ep->name_len);
		if (isonum_711(ep->flags)&2)
			isodirino(&idp->current.d_fileno,ep,imp);
		else
			idp->current.d_fileno = (bp->b_blkno << DEV_BSHIFT) + idp->curroff;
		
		if (reclen < ISO_DIRECTORY_RECORD_SIZE + idp->current.d_namlen) {
			error = EINVAL;
			/* illegal entry, stop */
			break;
		}
		
		idp->curroff += reclen;
		/*
		 *
		 */
		switch (imp->iso_ftype) {
		case ISO_FTYPE_RRIP:
			isofs_rrip_getname(ep,idp->current.d_name,
					   &idp->current.d_namlen,
					   &idp->current.d_fileno,imp);
			if (idp->current.d_namlen)
				error = iso_uiodir(idp,&idp->current,idp->curroff);
			break;
		default:	/* ISO_FTYPE_DEFAULT || ISO_FTYPE_9660 */
			strcpy(idp->current.d_name,"..");
			switch (ep->name[0]) {
			case 0:
				idp->current.d_namlen = 1;
				error = iso_uiodir(idp,&idp->current,idp->curroff);
				break;
			case 1:
				idp->current.d_namlen = 2;
				error = iso_uiodir(idp,&idp->current,idp->curroff);
				break;
			default:
				isofntrans(ep->name,idp->current.d_namlen,
					   idp->current.d_name,&idp->current.d_namlen,
					   imp->iso_ftype == ISO_FTYPE_9660,
					   isonum_711(ep->flags)&4);
				if (imp->iso_ftype == ISO_FTYPE_DEFAULT)
					error = iso_shipdir(idp);
				else
					error = iso_uiodir(idp,&idp->current,idp->curroff);
				break;
			}
		}
		if (error)
			break;
		
		entryoffsetinblock += reclen;
	}
	
	if (!error && imp->iso_ftype == ISO_FTYPE_DEFAULT) {
		idp->current.d_namlen = 0;
		error = iso_shipdir(idp);
	}
	if (error < 0)
		error = 0;
	
	if (bp)
		brelse (bp);

	uio->uio_offset = idp->uio_off;
	*eofflagp = idp->eof;
	
	FREE(idp,M_TEMP);
	
	return (error);
}

/*
 * Return target name of a symbolic link
 * Shouldn't we get the parent vnode and read the data from there?
 * This could eventually result in deadlocks in isofs_lookup.
 * But otherwise the block read here is in the block buffer two times.
 */
typedef struct iso_directory_record ISODIR;
typedef struct iso_node             ISONODE;
typedef struct iso_mnt              ISOMNT;
int isofs_readlink(vp, uio, cred)
	struct vnode *vp;
	struct uio   *uio;
	struct ucred *cred;
{
	ISONODE	*ip;
	ISODIR	*dirp;                   
	ISOMNT	*imp;
	struct	buf *bp;
	u_short	symlen;
	int	error;
	char	*symname;
	ino_t	ino;
	
	ip  = VTOI(vp);
	imp = ip->i_mnt;
	
	if (imp->iso_ftype != ISO_FTYPE_RRIP)
		return EINVAL;
	
	/*
	 * Get parents directory record block that this inode included.
	 */
	error = bread(imp->im_devvp,
		      (daddr_t)(ip->i_number / DEV_BSIZE),
		      imp->logical_block_size,
		      NOCRED,
		      &bp);
	if (error) {
		brelse(bp);
		return EINVAL;
	}

	/*
	 * Setup the directory pointer for this inode
	 */
	dirp = (ISODIR *)(bp->b_un.b_addr + (ip->i_number & imp->im_bmask));
#ifdef DEBUG
	printf("lbn=%d,off=%d,bsize=%d,DEV_BSIZE=%d, dirp= %08x, b_addr=%08x, offset=%08x(%08x)\n",
	       (daddr_t)(ip->i_number >> imp->im_bshift),
	       ip->i_number & imp->im_bmask,
	       imp->logical_block_size,
	       DEV_BSIZE,
	       dirp,
	       bp->b_un.b_addr,
	       ip->i_number,
	       ip->i_number & imp->im_bmask );
#endif
	
	/*
	 * Just make sure, we have a right one....
	 *   1: Check not cross boundary on block
	 */
	if ((ip->i_number & imp->im_bmask) + isonum_711(dirp->length)
	    >= imp->logical_block_size) {
		brelse(bp);
		return EINVAL;
	}
	
	/*
	 * Now get a buffer
	 * Abuse a namei buffer for now.
	 */
	MALLOC(symname,char *,MAXPATHLEN,M_NAMEI,M_WAITOK);
	
	/*
	 * Ok, we just gathering a symbolic name in SL record.
	 */
	if (isofs_rrip_getsymname(dirp,symname,&symlen,imp) == 0) {
		FREE(symname,M_NAMEI);
		brelse(bp);
		return EINVAL;
	}
	/*
	 * Don't forget before you leave from home ;-)
	 */
	brelse(bp);
	
	/*
	 * return with the symbolic name to caller's.
	 */
	error = uiomove(symname,symlen,uio);
	
	FREE(symname,M_NAMEI);
	
	return error;
}

/*
 * Ufs abort op, called after namei() when a CREATE/DELETE isn't actually
 * done. If a buffer has been saved in anticipation of a CREATE, delete it.
 */
/* ARGSUSED */
isofs_abortop(ndp)
	struct nameidata *ndp;
{

	if ((ndp->ni_nameiop & (HASBUF | SAVESTART)) == HASBUF)
		FREE(ndp->ni_pnbuf, M_NAMEI);
	return 0;
}

/*
 * Lock an inode.
 */
isofs_lock(vp)
	struct vnode *vp;
{
	register struct iso_node *ip = VTOI(vp);

	ISO_ILOCK(ip);
	return 0;
}

/*
 * Unlock an inode.
 */
isofs_unlock(vp)
	struct vnode *vp;
{
	register struct iso_node *ip = VTOI(vp);

	if (!(ip->i_flag & ILOCKED))
		panic("isofs_unlock NOT LOCKED");
	ISO_IUNLOCK(ip);
	return 0;
}

/*
 * Check for a locked inode.
 */
isofs_islocked(vp)
	struct vnode *vp;
{

	if (VTOI(vp)->i_flag & ILOCKED)
		return 1;
	return 0;
}

/*
 * Calculate the logical to physical mapping if not done already,
 * then call the device strategy routine.
 */

isofs_strategy(bp)
	register struct buf *bp;
{
	register struct iso_node *ip = VTOI(bp->b_vp);
	struct vnode *vp;
	int error;

	if (bp->b_vp->v_type == VBLK || bp->b_vp->v_type == VCHR)
		panic("isofs_strategy: spec");
	if (bp->b_blkno == bp->b_lblkno) {
		if (error = iso_bmap(ip, bp->b_lblkno, &bp->b_blkno))
			return error;
		if ((long)bp->b_blkno == -1)
			clrbuf(bp);
	}
	if ((long)bp->b_blkno == -1) {
		biodone(bp);
		return 0;
	}
	vp = ip->i_devvp;
	bp->b_dev = vp->v_rdev;
	(*(vp->v_op->vop_strategy))(bp);
	return 0;
}

/*
 * Print out the contents of an inode.
 */
void
isofs_print(vp)
	struct vnode *vp;
{
	printf("tag VT_ISOFS, isofs vnode\n");
}

extern int enodev ();

/*
 * Global vfs data structures for isofs
 */
struct vnodeops isofs_vnodeops = {
	isofs_lookup,		/* lookup */
	(void *)enodev,		/* create */
	(void *)enodev,		/* mknod */
	isofs_open,		/* open */
	isofs_close,		/* close */
	isofs_access,		/* access */
	isofs_getattr,		/* getattr */
	(void *)enodev,		/* setattr */
	isofs_read,		/* read */
	(void *)enodev,		/* write */
	isofs_ioctl,		/* ioctl */
	isofs_select,		/* select */
	isofs_mmap,		/* mmap */
	(void *)nullop,		/* fsync */
	isofs_seek,		/* seek */
	(void *)enodev,		/* remove */
	(void *)enodev,		/* link */
	(void *)enodev,		/* rename */
	(void *)enodev,		/* mkdir */
	(void *)enodev,		/* rmdir */
	(void *)enodev,		/* symlink */
	isofs_readdir,		/* readdir */
	isofs_readlink,		/* readlink */
	isofs_abortop,		/* abortop */
	isofs_inactive,		/* inactive */
	isofs_reclaim,		/* reclaim */
	isofs_lock,		/* lock */
	isofs_unlock,		/* unlock */
	(void *)enodev,		/* bmap */
	isofs_strategy,		/* strategy */
	isofs_print,		/* print */
	isofs_islocked,		/* islocked */
	(void *)enodev,		/* advlock */
};

struct vnodeops isofs_spec_inodeops = {
	spec_lookup,		/* lookup */
	(void *)enodev,		/* create */
	(void *)enodev,		/* mknod */
	spec_open,		/* open */
	spec_close,		/* close */
	isofs_access,		/* access */
	isofs_getattr,		/* getattr */
	(void *)enodev,		/* setattr */
	spec_read,		/* read */
	spec_write,		/* write */
	spec_ioctl,		/* ioctl */
	spec_select,		/* select */
	spec_mmap,		/* mmap */
	spec_fsync,		/* fsync */
	spec_seek,		/* seek */
	(void *)enodev,		/* remove */
	(void *)enodev,		/* link */
	(void *)enodev,		/* rename */
	(void *)enodev,		/* mkdir */
	(void *)enodev,		/* rmdir */
	(void *)enodev,		/* symlink */
	spec_readdir,		/* readdir */
	spec_readlink,		/* readlink */
	spec_abortop,		/* abortop */
	isofs_inactive,		/* inactive */
	isofs_reclaim,		/* reclaim */
	isofs_lock,		/* lock */
	isofs_unlock,		/* unlock */
	(void *)enodev,		/* bmap */
	spec_strategy,		/* strategy */
	isofs_print,		/* print */
	isofs_islocked,		/* islocked */
	spec_advlock,		/* advlock */
};

#ifdef	FIFO
struct vnodeops isofs_fifo_inodeops = {
	fifo_lookup,		/* lookup */
	(void *)enodev,		/* create */
	(void *)enodev,		/* mknod */
	fifo_open,		/* open */
	fifo_close,		/* close */
	isofs_access,		/* access */
	isofs_getattr,		/* getattr */
	(void *)enodev,		/* setattr */
	fifo_read,		/* read */
	fifo_write,		/* write */
	fifo_ioctl,		/* ioctl */
	fifo_select,		/* select */
	fifo_mmap,		/* mmap */
	fifo_fsync,		/* fsync */
	fifo_seek,		/* seek */
	(void *)enodev,		/* remove */
	(void *)enodev,		/* link */
	(void *)enodev,		/* rename */
	(void *)enodev,		/* mkdir */
	(void *)enodev,		/* rmdir */
	(void *)enodev,		/* symlink */
	fifo_readdir,		/* readdir */
	fifo_readlink,		/* readlink */
	fifo_abortop,		/* abortop */
	isofs_inactive,		/* inactive */
	isofs_reclaim,		/* reclaim */
	isofs_lock,		/* lock */
	isofs_unlock,		/* unlock */
	(void *)enodev,		/* bmap */
	fifo_strategy,		/* strategy */
	isofs_print,		/* print */
	isofs_islocked,		/* islocked */
	fifo_advlock,		/* advlock */
};
#endif	/* FIFO */
