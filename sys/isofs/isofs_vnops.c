/*
 *	$Id: isofs_vnops.c,v 1.7 1993/09/03 04:37:58 cgd Exp $
 */
#include "param.h"
#include "systm.h"
#include "namei.h"
#include "resourcevar.h"
#include "kernel.h"
#include "file.h"
#include "stat.h"
#include "buf.h"
#include "proc.h"
#include "conf.h"
#include "mount.h"
#include "vnode.h"
#include "specdev.h"
#include "fifo.h"
#include "malloc.h"
#include "dir.h"

#include "iso.h"
#include "isofs_node.h"
#include "iso_rrip.h"

#ifdef ISOFS_DEBUG
#define DPRINTF(a) printf a
#else
#define DPRINTF(a)
#endif

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

	vap->va_fsid = ip->i_dev;
	vap->va_fileid = ip->i_number;
	if (vp->v_type == VDIR)
		vap->va_nlink = 2;
	else
		vap->va_nlink = 1;

	vap->va_mode = ip->inode.iso_mode;
	vap->va_uid  = ip->inode.iso_uid;
	vap->va_gid  = ip->inode.iso_gid;
	vap->va_atime= ip->inode.iso_atime;
	vap->va_mtime= ip->inode.iso_mtime;
	vap->va_ctime= ip->inode.iso_ctime;

	vap->va_rdev = 0;
	vap->va_size = ip->i_size;
	vap->va_size_rsv = 0;
	vap->va_flags = 0;
	vap->va_gen = 1;
	vap->va_blocksize = ip->i_mnt->logical_block_size;
	vap->va_bytes = ip->i_size;
	vap->va_bytes_rsv = 0;
	vap->va_type = vp->v_type;
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

#ifdef DIAGNOSTICx
	if (uio->uio_rw != UIO_READ)
		panic("isofs_read mode");
	type = ip->i_mode & IFMT;
	if (type != IFDIR && type != IFREG && type != IFLNK)
		panic("isofs_read type");
#endif
	if (uio->uio_resid == 0)
		return (0);
	if (uio->uio_offset < 0)
		return (EINVAL);
	ip->i_flag |= IACC;
	imp = ip->i_mnt;
	do {
		lbn = iso_lblkno(imp, uio->uio_offset);
		on = iso_blkoff(imp, uio->uio_offset);
		n = MIN((unsigned)(imp->im_bsize - on), uio->uio_resid);
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
		if (n + on == imp->im_bsize || uio->uio_offset == ip->i_size)
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
 * Vnode op for readdir
 */
isofs_readdir(vp, uio, cred, eofflagp)
	struct vnode *vp;
	register struct uio *uio;
	struct ucred *cred;
	int *eofflagp;
{
	struct dirent dirent;
	int iso_offset;
	int entryoffsetinblock;
	int error = 0;
	int endsearch;
	struct iso_directory_record *ep;
	int reclen;
	struct iso_mnt *imp;
	struct iso_node *ip;
	struct buf *bp = NULL;
	int i;
	int end_flag = 0;
	ISO_RRIP_ANALYZE ana;

	ip = VTOI (vp);
	imp = ip->i_mnt;

	iso_offset = uio->uio_offset;

	entryoffsetinblock = iso_blkoff(imp, iso_offset);
	DPRINTF(("isofs_readdir: iso_offset = %d, entryoff..=%d\n", 
		 iso_offset, entryoffsetinblock));
	if (entryoffsetinblock != 0) {
		if (error = iso_blkatoff(ip, iso_offset, (char **)0, &bp))
			return (error);
	}

	endsearch = ip->i_size;
	DPRINTF(("isofs_readdir: endsearch = %d\n", endsearch));

	while (iso_offset < endsearch && uio->uio_resid > 0) {
	        DPRINTF(("isooff=%d, entryoff=%d\n", iso_offset, entryoffsetinblock));
		/*
		 * If offset is on a block boundary,
		 * read the next directory block.
		 * Release previous if it exists.
		 */

		if (iso_blkoff(imp, iso_offset) == 0) {
			if (bp != NULL)
				brelse(bp);
			if (error = iso_blkatoff(ip, iso_offset,
						 (char **)0, &bp))
				return (error);
			entryoffsetinblock = 0;
			DPRINTF(("new block at iso_offset = %d\n", iso_offset));
		}
		/*
		 * Get pointer to next entry.
		 */

		ep = (struct iso_directory_record *)
			(bp->b_un.b_addr + entryoffsetinblock);

		reclen = isonum_711 (ep->length);
		DPRINTF(("isofs_readdir: reclen = %d\n", reclen));
		if (reclen == 0) {
			/* skip to next block, if any */
			iso_offset = roundup (iso_offset,
					      imp->logical_block_size);
			DPRINTF(("isofs: skip to next block, if any\n"));
			continue;
		}

		if (reclen < ISO_DIRECTORY_RECORD_SIZE) {
		        DPRINTF(("isofs_readdir: reclen too small!\n"));
			error = EINVAL;
			/* illegal entry, stop */
			break;
		}

		if (entryoffsetinblock + reclen -1 >= imp->logical_block_size) {
                        DPRINTF(("isofs_readdir: entryoffsetinblock + reclen - 1 > lbs\n"));
			error = EINVAL;
			/* illegal directory, so stop looking */
			break;
		}

		dirent.d_fileno = isonum_733 (ep->extent);
		dirent.d_namlen = isonum_711 (ep->name_len);
		if (reclen < ISO_DIRECTORY_RECORD_SIZE + dirent.d_namlen) {
		        error = EINVAL;
			/* illegal entry, stop */
			break;
	        }

		/*
		 *
		 */
		switch (ep->name[0]) {
			case 0:
				dirent.d_name[0] = '.';
				dirent.d_namlen = 1;
				break;
			case 1:
				dirent.d_name[0] = '.';
				dirent.d_name[1] = '.';
				dirent.d_namlen = 2;
				break;
			default:
				switch ( imp->iso_ftype ) {
					default:
						DPRINTF(("isofs_readdir: unknown ftype %x\n",
							 imp->iso_ftype));
						/* default to iso */
						/* FALLTHRU */

					case ISO_FTYPE_9660:
						DPRINTF(("isofs_readdir: 9660\n"));
						isofntrans(ep->name, dirent.d_namlen, dirent.d_name, &dirent.d_namlen);
					break;

					case ISO_FTYPE_RRIP:
				                DPRINTF(("isofs_readdir: RRIP\n"));
						isofs_rrip_getname( ep, dirent.d_name, &dirent.d_namlen );
					break;
				}
				break;
		}

		dirent.d_name[dirent.d_namlen] = 0;
		dirent.d_reclen = DIRSIZ (&dirent);

		DPRINTF(("d_fileno = %d, d_namlen = %d, name = %s (0x%08x%08x)\n",
			 dirent.d_fileno, dirent.d_namlen, dirent.d_name,
			 *(unsigned long*)dirent.d_name, 
			 *(unsigned long*)(dirent.d_name+4)));

		if (uio->uio_resid < dirent.d_reclen)
			break;

		if (error = uiomove (&dirent, dirent.d_reclen, uio))
			break;

		iso_offset += reclen;
		entryoffsetinblock += reclen;
	}
			
	DPRINTF(("isofs_readdir: out of directory scan loop\n"));
	if (bp)
		brelse (bp);

	if (end_flag || (VTOI(vp)->i_size - iso_offset) <= 0)
		*eofflagp = 1;
	else
		*eofflagp = 0;

	uio->uio_offset = iso_offset;

	return (error);
}

/*
 * Return target name of a symbolic link
 */
typedef struct iso_directory_record ISODIR;
typedef struct iso_node             ISONODE;
typedef struct iso_mnt              ISOMNT;
int isofs_readlink(vp, uio, cred)
struct vnode *vp;
struct uio   *uio;
struct ucred *cred;
{
        ISONODE *ip;
        ISODIR  *dirp;                   
        ISOMNT  *imp;
        struct  buf *bp;
        int     symlen;
        int     error;
        char    symname[NAME_MAX];

        ip  = VTOI( vp );
        imp = ip->i_mnt;
        /*
         * Get parents directory record block that this inode included.
         */
        error = bread(  imp->im_devvp,
                        (daddr_t)(( ip->iso_parent_ext + (ip->iso_parent >> 11 ) )
                        * imp->im_bsize / DEV_BSIZE ),
                        imp->im_bsize,
                        NOCRED,
                        &bp );
        if ( error ) {
                return( EINVAL );
        }

        /*
         * Setup the directory pointer for this inode
         */

        dirp = (ISODIR *)(bp->b_un.b_addr + ( ip->iso_parent & 0x7ff ) );
#ifdef DEBUG
        printf("lbn=%d[base=%d,off=%d,bsize=%d,DEV_BSIZE=%d], dirp= %08x, b_addr=%08x, offset=%08x(%08x)\n",
                                (daddr_t)(( ip->iso_parent_ext + (ip->iso_parent >> 12 ) ) * imp->im_bsize / DEV_BSIZE ),
                                ip->iso_parent_ext,
                                (ip->iso_parent >> 11 ),
                                imp->im_bsize,
                                DEV_BSIZE,
                                dirp,
                                bp->b_un.b_addr,
                                ip->iso_parent,
                                ip->iso_parent & 0x7ff );
#endif

        /*
         * Just make sure, we have a right one....
         *   1: Check not cross boundary on block
         *   2: Check number of inode
         */
        if ( (ip->iso_parent & 0x7ff) + isonum_711( dirp->length ) >=
                                                imp->im_bsize )         {
                brelse ( bp );
                return( EINVAL );
        }
        if ( isonum_733(dirp->extent) != ip->i_number ) {
                brelse ( bp );
                return( EINVAL );
        }

        /*
         * Ok, we just gathering a Symbolick name in SL record.
         */
        if ( isofs_rrip_getsymname( vp, dirp, symname, &symlen ) == 0 ) {
                brelse ( bp );
                return( EINVAL );
        }
        /*
         * Don't forget before you leave from home ;-)
         */
        brelse( bp );

        /*
         * return with the Symbolick name to caller's.
         */
        return ( uiomove( symname, symlen, uio ) );
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
	return (0);
}

/*
 * Lock an inode.
 */
isofs_lock(vp)
	struct vnode *vp;
{
	register struct iso_node *ip = VTOI(vp);

	ISO_ILOCK(ip);
	return (0);
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
	return (0);
}

/*
 * Check for a locked inode.
 */
isofs_islocked(vp)
	struct vnode *vp;
{

	if (VTOI(vp)->i_flag & ILOCKED)
		return (1);
	return (0);
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
			return (error);
		if ((long)bp->b_blkno == -1)
			clrbuf(bp);
	}
	if ((long)bp->b_blkno == -1) {
		biodone(bp);
		return (0);
	}
	vp = ip->i_devvp;
	bp->b_dev = vp->v_rdev;
	(*(vp->v_op->vop_strategy))(bp);
	return (0);
}

/*
 * Print out the contents of an inode.
 */
void
isofs_print(vp)
	struct vnode *vp;
{
	printf ("tag VT_ISOFS, isofs vnode\n");
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
