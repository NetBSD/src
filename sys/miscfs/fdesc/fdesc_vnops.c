/*
 * Copyright (c) 1990, 1992 Jan-Simon Pendry
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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
 *	$Id: fdesc_vnops.c,v 1.6.2.2 1993/12/28 16:35:14 pk Exp $
 */

/*
 * /dev/fd Filesystem
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/filedesc.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/buf.h>
#include <miscfs/fdesc/fdesc.h>

#include <ufs/dir.h>		/* For readdir() XXX */

/*
 * vp is the current namei directory
 * ndp is the name to locate in that directory...
 */
fdesc_lookup(dvp, ndp, p)
	struct vnode *dvp;
	struct nameidata *ndp;
	struct proc *p;
{
	char *pname = ndp->ni_ptr;
	int nfiles = p->p_fd->fd_nfiles;
	unsigned fd;
	int error;
	struct vnode *fvp;

#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_lookup(%s)\n", pname);
#endif
	if (ndp->ni_namelen == 1 && *pname == '.') {
		ndp->ni_dvp = dvp;
		ndp->ni_vp = dvp;
		VREF(dvp);
		/*VOP_LOCK(dvp);*/
		return (0);
	}

	fd = 0;
	while (*pname >= '0' && *pname <= '9') {
		fd = 10 * fd + *pname++ - '0';
		if (fd >= nfiles)
			break;
	}

#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_lookup: fd = %d, *pname = %x\n", fd, *pname);
#endif
	if (*pname == '/') {
		error = ENOTDIR;
		goto bad;
	}

	if (*pname != '\0') {
		error = ENOENT;
		goto bad;
	}

	if (fd >= nfiles || p->p_fd->fd_ofiles[fd] == NULL) {
		error = EBADF;
		goto bad;
	}

#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_lookup: allocate new vnode\n");
#endif
	error = getnewvnode(VT_FDESC, dvp->v_mount, &fdesc_vnodeops, &fvp);
	if (error)
		goto bad;
	VTOFDESC(fvp)->f_fd = fd;
	/*VTOFDESC(fvp)->f_isroot = 0;*/
	ndp->ni_dvp = dvp;
	ndp->ni_vp = fvp;
#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_lookup: newvp = %x\n", fvp);
#endif
	return (0);

bad:;
	ndp->ni_dvp = dvp;
	ndp->ni_vp = NULL;
#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_lookup: error = %d\n", error);
#endif
	return (error);
}

fdesc_open(vp, mode, cred, p)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
	struct proc *p;
{
	int error;
	struct filedesc *fdp;
	struct file *fp;
	int dfd;
	int fd;

	/*
	 * Can always open the root (modulo perms)
	 */
	if (vp->v_flag & VROOT)
		return (0);

	/*
	 * XXX Kludge: set p->p_dupfd to contain the value of the
	 * the file descriptor being sought for duplication. The error 
	 * return ensures that the vnode for this device will be released
	 * by vn_open. Open will detect this special error and take the
	 * actions in dupfdopen.  Other callers of vn_open or VOP_OPEN
	 * will simply report the error.
	 */
	p->p_dupfd = VTOFDESC(vp)->f_fd;	/* XXX */
	return (ENODEV);
}

static int
fdesc_attr(fd, vap, cred, p)
	int fd;
	struct vattr *vap;
	struct ucred *cred;
	struct proc *p;
{
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	int error;

#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_attr: fd = %d, nfiles = %d\n", fd, fdp->fd_nfiles);
#endif
	if (fd >= fdp->fd_nfiles || (fp = fdp->fd_ofiles[fd]) == NULL) {
#ifdef FDESC_DIAGNOSTIC
		printf("fdesc_attr: fp = %x (EBADF)\n", fp);
#endif
		return (EBADF);
	}

	/*
	 * Can stat the underlying vnode, but not sockets because
	 * they don't use struct vattrs.  Well, we could convert from
	 * a struct stat back to a struct vattr, later...
	 */
	switch (fp->f_type) {
	case DTYPE_VNODE:
		error = VOP_GETATTR((struct vnode *) fp->f_data, vap, cred, p);
		break;

	case DTYPE_SOCKET:
		error = EOPNOTSUPP;
		break;

	default:
		panic("fdesc attr");
		break;
	}

#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_attr: returns error %d\n", error);
#endif
	return (error);
}

fdesc_getattr(vp, vap, cred, p)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
	struct proc *p;
{
	unsigned fd;
	int error;

	if (vp->v_flag & VROOT) {
#ifdef FDESC_DIAGNOSTIC
		printf("fdesc_getattr: stat rootdir\n");
#endif
		bzero((caddr_t) vap, sizeof(*vap));
		vattr_null(vap);
		vap->va_type = VDIR;
		vap->va_mode = S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;
		vap->va_nlink = 2;
		vap->va_uid = 0;
		vap->va_gid = 0;
		vap->va_fsid = vp->v_mount->mnt_stat.f_fsid.val[0];
		vap->va_fileid = 2;
		/* vap->va_qsize = 0; */
		vap->va_size = DEV_BSIZE;
		vap->va_blocksize = DEV_BSIZE;
		microtime(&vap->va_atime);
		vap->va_mtime = vap->va_atime;
		vap->va_ctime = vap->va_ctime;
		vap->va_gen = 0;
		vap->va_flags = 0;
		vap->va_rdev = 0;
		/* vap->va_qbytes = 0; */
		vap->va_bytes = 0;
		return (0);
	}

	fd = VTOFDESC(vp)->f_fd;
	error = fdesc_attr(fd, vap, cred, p);
	if (error == 0)
		vp->v_type = vap->va_type;
	return (error);
}

fdesc_setattr(vp, vap, cred, p)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
	struct proc *p;
{
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	unsigned fd;
	int error;

	/*
	 * Can't mess with the root vnode
	 */
	if (vp->v_flag & VROOT)
		return (EACCES);

	fd = VTOFDESC(vp)->f_fd;
#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_setattr: fd = %d, nfiles = %d\n", fd, fdp->fd_nfiles);
#endif
	if (fd >= fdp->fd_nfiles || (fp = fdp->fd_ofiles[fd]) == NULL) {
#ifdef FDESC_DIAGNOSTIC
		printf("fdesc_setattr: fp = %x (EBADF)\n", fp);
#endif
		return (EBADF);
	}

	/*
	 * Can setattr the underlying vnode, but not sockets!
	 */
	switch (fp->f_type) {
	case DTYPE_VNODE:
		error = VOP_SETATTR((struct vnode *) fp->f_data, vap, cred, p);
		break;

	case DTYPE_SOCKET:
		error = EOPNOTSUPP;
		break;

	default:
		panic("fdesc setattr");
		break;
	}

#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_setattr: returns error %d\n", error);
#endif
	return (error);
}

fdesc_readdir(vp, uio, cred, eofflagp, cookies, ncookies)
	struct vnode *vp;
	struct uio *uio;
	struct ucred *cred;
	int *eofflagp;
	u_int *cookies;
	int ncookies;
{
	struct filedesc *fdp;
	int ind, i;
	int error;

#define UIO_MX 16

	fdp = uio->uio_procp->p_fd;
	ind = uio->uio_offset / UIO_MX;
	error = 0;
	while (uio->uio_resid > 0 && (!cookies || ncookies > 0)) {
		struct direct d;
		struct direct *dp = &d;

	        if (ind < 2) { /* . or .. */
		  bzero((caddr_t) dp, UIO_MX);
		  if (ind == 0) {
		    strcpy(dp->d_name, ".");
		    dp->d_namlen = 1;
		  } else if (ind == 1) {
		    strcpy(dp->d_name, "..");
		    dp->d_namlen = 2;
		  } else
		    panic("fdesc: ind is negative!");

		  dp->d_reclen = UIO_MX;
		  dp->d_ino = 2;

		  /*
		   * And ship to userland
		   */
		  error = uiomove((caddr_t) dp, UIO_MX, uio);
		  if (error)
		    break;

		  ind++;
		  if (cookies) {
			  *cookies++ = ind * UIO_MX;
			  ncookies--;
		  }
		  continue;
		}
	        i = ind - 2;
		if (i >= fdp->fd_nfiles) {
			*eofflagp = 1;
			break;
		}
		if (fdp->fd_ofiles[i] != NULL) {
			struct direct d;
			struct direct *dp = &d;
			char *cp = dp->d_name;
#ifdef FDESC_FILEID
			struct vattr va;
#endif
			int j;

			bzero((caddr_t) dp, UIO_MX);

			/*
			 * Generate an ASCII representation of the name.
			 * This can cope with fds in the range 0..99999
			 */
			cp++;
			if (i > 10) cp++;
			if (i > 100) cp++;
			if (i > 1000) cp++;
			if (i > 10000) cp++;
			if (i > 100000) panic("fdesc_readdir");
			dp->d_namlen = cp - dp->d_name;
			*cp = '\0';
			j = i;
			do {
				*--cp = j % 10 + '0';
				j /= 10;
			} while (j > 0);
			/*
			 * Fill in the remaining fields
			 */
			dp->d_reclen = UIO_MX;
			dp->d_ino = i + 3;
#ifdef FDESC_FILEID
			/*
			 * If we want the file ids to match the
			 * we must call getattr on the underlying file.
			 * fdesc_attr may return an error, in which case
			 * we ignore the returned file id.
			 */
			error = fdesc_attr(i, &va, cred, p);
			if (error == 0)
				dp->d_ino = va.va_fileid;
#endif
			/*
			 * And ship to userland
			 */
			error = uiomove((caddr_t) dp, UIO_MX, uio);
			if (error)
				break;
			if (cookies) {
				*cookies++ = (ind + 1) * UIO_MX;
				ncookies--;
			}
		}
		ind++;
	}

	uio->uio_offset = ind * UIO_MX;
	return (error);
}

fdesc_inactive(vp, p)
	struct vnode *vp;
	struct proc *p;
{
	/*
	 * Clear out the v_type field to avoid
	 * nasty things happening in vgone().
	 */
	vp->v_type = VNON;
#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_inactive(%x)\n", vp);
#endif
	return (0);
}

/*
 * Print out the contents of a /dev/fd vnode.
 */
/* ARGSUSED */
void
fdesc_print(vp)
	struct vnode *vp;
{
	printf("tag VT_FDESC, fdesc vnode\n");
}

/*
 * /dev/fd vnode unsupported operation
 */
fdesc_enotsupp()
{
	return (ENODEV);
}

/*
 * /dev/fd "should never get here" operation
 */
fdesc_badop()
{
	panic("fdesc: bad op");
	/* NOTREACHED */
}

/*
 * /dev/fd vnode null operation
 */
fdesc_nullop()
{
	return (0);
}

#define fdesc_create ((int (*) __P(( \
		struct nameidata *ndp, \
		struct vattr *vap, \
		struct proc *p))) fdesc_enotsupp)
#define fdesc_mknod ((int (*) __P(( \
		struct nameidata *ndp, \
		struct vattr *vap, \
		struct ucred *cred, \
		struct proc *p))) fdesc_enotsupp)
#define fdesc_close ((int (*) __P(( \
		struct vnode *vp, \
		int fflag, \
		struct ucred *cred, \
		struct proc *p))) nullop)
#define fdesc_access ((int (*) __P(( \
		struct vnode *vp, \
		int mode, \
		struct ucred *cred, \
		struct proc *p))) nullop)
#define	fdesc_read ((int (*) __P(( \
		struct vnode *vp, \
		struct uio *uio, \
		int ioflag, \
		struct ucred *cred))) fdesc_enotsupp)
#define	fdesc_write ((int (*) __P(( \
		struct vnode *vp, \
		struct uio *uio, \
		int ioflag, \
		struct ucred *cred))) fdesc_enotsupp)
#define	fdesc_ioctl ((int (*) __P(( \
		struct vnode *vp, \
		int command, \
		caddr_t data, \
		int fflag, \
		struct ucred *cred, \
		struct proc *p))) fdesc_enotsupp)
#define	fdesc_select ((int (*) __P(( \
		struct vnode *vp, \
		int which, \
		int fflags, \
		struct ucred *cred, \
		struct proc *p))) fdesc_enotsupp)
#define fdesc_mmap ((int (*) __P(( \
		struct vnode *vp, \
		int fflags, \
		struct ucred *cred, \
		struct proc *p))) fdesc_enotsupp)
#define fdesc_fsync ((int (*) __P(( \
		struct vnode *vp, \
		int fflags, \
		struct ucred *cred, \
		int waitfor, \
		struct proc *p))) nullop)
#define fdesc_seek ((int (*) __P(( \
		struct vnode *vp, \
		off_t oldoff, \
		off_t newoff, \
		struct ucred *cred))) nullop)
#define fdesc_remove ((int (*) __P(( \
		struct nameidata *ndp, \
		struct proc *p))) fdesc_enotsupp)
#define fdesc_link ((int (*) __P(( \
		struct vnode *vp, \
		struct nameidata *ndp, \
		struct proc *p))) fdesc_enotsupp)
#define fdesc_rename ((int (*) __P(( \
		struct nameidata *fndp, \
		struct nameidata *tdnp, \
		struct proc *p))) fdesc_enotsupp)
#define fdesc_mkdir ((int (*) __P(( \
		struct nameidata *ndp, \
		struct vattr *vap, \
		struct proc *p))) fdesc_enotsupp)
#define fdesc_rmdir ((int (*) __P(( \
		struct nameidata *ndp, \
		struct proc *p))) fdesc_enotsupp)
#define fdesc_symlink ((int (*) __P(( \
		struct nameidata *ndp, \
		struct vattr *vap, \
		char *target, \
		struct proc *p))) fdesc_enotsupp)
#define fdesc_readlink ((int (*) __P(( \
		struct vnode *vp, \
		struct uio *uio, \
		struct ucred *cred))) fdesc_enotsupp)
#define fdesc_abortop ((int (*) __P(( \
		struct nameidata *ndp))) nullop)
#ifdef FDESC_DIAGNOSTIC
int fdesc_reclaim(vp)
struct vnode *vp;
{
	printf("fdesc_reclaim(%x)\n", vp);
	return (0);
}
#else
#define fdesc_reclaim ((int (*) __P(( \
		struct vnode *vp))) nullop)
#endif
#define	fdesc_lock ((int (*) __P(( \
		struct vnode *vp))) nullop)
#define fdesc_unlock ((int (*) __P(( \
		struct vnode *vp))) nullop)
#define	fdesc_bmap ((int (*) __P(( \
		struct vnode *vp, \
		daddr_t bn, \
		struct vnode **vpp, \
		daddr_t *bnp))) fdesc_badop)
#define	fdesc_strategy ((int (*) __P(( \
		struct buf *bp))) fdesc_badop)
#define fdesc_islocked ((int (*) __P(( \
		struct vnode *vp))) nullop)
#define fdesc_advlock ((int (*) __P(( \
		struct vnode *vp, \
		caddr_t id, \
		int op, \
		struct flock *fl, \
		int flags))) fdesc_enotsupp)

struct vnodeops fdesc_vnodeops = {
	fdesc_lookup,	/* lookup */
	fdesc_create,	/* create */
	fdesc_mknod,	/* mknod */
	fdesc_open,	/* open */
	fdesc_close,	/* close */
	fdesc_access,	/* access */
	fdesc_getattr,	/* getattr */
	fdesc_setattr,	/* setattr */
	fdesc_read,	/* read */
	fdesc_write,	/* write */
	fdesc_ioctl,	/* ioctl */
	fdesc_select,	/* select */
	fdesc_mmap,	/* mmap */
	fdesc_fsync,	/* fsync */
	fdesc_seek,	/* seek */
	fdesc_remove,	/* remove */
	fdesc_link,	/* link */
	fdesc_rename,	/* rename */
	fdesc_mkdir,	/* mkdir */
	fdesc_rmdir,	/* rmdir */
	fdesc_symlink,	/* symlink */
	fdesc_readdir,	/* readdir */
	fdesc_readlink,	/* readlink */
	fdesc_abortop,	/* abortop */
	fdesc_inactive,	/* inactive */
	fdesc_reclaim,	/* reclaim */
	fdesc_lock,	/* lock */
	fdesc_unlock,	/* unlock */
	fdesc_bmap,	/* bmap */
	fdesc_strategy,	/* strategy */
	fdesc_print,	/* print */
	fdesc_islocked,	/* islocked */
	fdesc_advlock,	/* advlock */
};
