/*
 * Copyright (c) 1992 The Regents of the University of California
 * Copyright (c) 1990, 1992, 1993 Jan-Simon Pendry
 * All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
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
 * From:
 *	Id: fdesc_vnops.c,v 4.1 1993/12/17 10:47:45 jsp Rel
 *
 *	$Id: fdesc_vnops.c,v 1.9 1994/01/05 11:07:44 cgd Exp $
 */

/*
 * /dev/fd Filesystem
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/kernel.h>	/* boottime */
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

#define cttyvp(p) ((p)->p_flag&SCTTY ? (p)->p_session->s_ttyvp : NULL)

#define FDL_WANT	0x01
#define FDL_LOCKED	0x02
static int fdescvplock;
static struct vnode *fdescvp[FD_MAX];

#if (FD_STDIN != FD_STDOUT-1) || (FD_STDOUT != FD_STDERR-1)
FD_STDIN, FD_STDOUT, FD_STDERR must be a sequence n, n+1, n+2
#endif

fdesc_allocvp(ftype, ix, mp, vpp)
	fdntype ftype;
	int ix;
	struct mount *mp;
	struct vnode **vpp;
{
	struct vnode **nvpp = 0;
	int error = 0;

loop:
	/* get stashed copy of the vnode */
	if (ix >= 0 && ix < FD_MAX) {
		nvpp = &fdescvp[ix];
		if (*nvpp) {
			if (vget(*nvpp))
				goto loop;
			VOP_UNLOCK(*nvpp);
			*vpp = *nvpp;
			return (error);
		}
	}

	/*
	 * otherwise lock the array while we call getnewvnode
	 * since that can block.
	 */ 
	if (fdescvplock & FDL_LOCKED) {
		fdescvplock |= FDL_WANT;
		sleep((caddr_t) &fdescvplock, PINOD);
		goto loop;
	}
	fdescvplock |= FDL_LOCKED;

	error = getnewvnode(VT_FDESC, mp, &fdesc_vnodeops, vpp);
	if (error)
		goto out;
	if (nvpp)
		*nvpp = *vpp;
	VTOFDESC(*vpp)->fd_type = ftype;
	VTOFDESC(*vpp)->fd_fd = -1;
	VTOFDESC(*vpp)->fd_link = 0;
	VTOFDESC(*vpp)->fd_ix = ix;

out:;
	fdescvplock &= ~FDL_LOCKED;

	if (fdescvplock & FDL_WANT) {
		fdescvplock &= ~FDL_WANT;
		wakeup((caddr_t) &fdescvplock);
	}

	return (error);
}

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
	char *ln;

#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_lookup(%s)\n", pname);
#endif
	if (ndp->ni_namelen == 1 && *pname == '.') {
		ndp->ni_dvp = dvp;
		ndp->ni_vp = dvp;
		VREF(dvp);
		VOP_LOCK(dvp);
		return (0);
	}

	switch (VTOFDESC(dvp)->fd_type) {
	default:
	case Flink:
	case Fdesc:
	case Fctty:
		error = ENOTDIR;
		goto bad;

	case Froot:
		if (ndp->ni_namelen == 2 && bcmp(pname, "fd", 2) == 0) {
			error = fdesc_allocvp(Fdevfd, FD_DEVFD, dvp->v_mount, &fvp);
			if (error)
				goto bad;
			ndp->ni_dvp = dvp;
			ndp->ni_vp = fvp;
			fvp->v_type = VDIR;
			VOP_LOCK(fvp);
#ifdef FDESC_DIAGNOSTIC
			printf("fdesc_lookup: newvp = %x\n", fvp);
#endif
			return (0);
		}

		if (ndp->ni_namelen == 3 && bcmp(pname, "tty", 3) == 0) {
			struct vnode *ttyvp = cttyvp(p);
			if (ttyvp == NULL) {
				error = ENXIO;
				goto bad;
			}
			error = fdesc_allocvp(Fctty, FD_CTTY, dvp->v_mount, &fvp);
			if (error)
				goto bad;
			ndp->ni_dvp = dvp;
			ndp->ni_vp = fvp;
			fvp->v_type = VFIFO;
			VOP_LOCK(fvp);
#ifdef FDESC_DIAGNOSTIC
			printf("fdesc_lookup: ttyvp = %x\n", fvp);
#endif
			return (0);
		}

		ln = 0;
		switch (ndp->ni_namelen) {
		case 5:
			if (bcmp(pname, "stdin", 5) == 0) {
				ln = "fd/0";
				fd = FD_STDIN;
			}
			break;
		case 6:
			if (bcmp(pname, "stdout", 6) == 0) {
				ln = "fd/1";
				fd = FD_STDOUT;
			} else
			if (bcmp(pname, "stderr", 6) == 0) {
				ln = "fd/2";
				fd = FD_STDERR;
			}
			break;
		}

		if (ln) {
#ifdef FDESC_DIAGNOSTIC
			printf("fdesc_lookup: link -> %s\n", ln);
#endif
			error = fdesc_allocvp(Flink, fd, dvp->v_mount, &fvp);
			if (error)
				goto bad;
			VTOFDESC(fvp)->fd_link = ln;
			ndp->ni_dvp = dvp;
			ndp->ni_vp = fvp;
			fvp->v_type = VLNK;
			VOP_LOCK(fvp);
#ifdef FDESC_DIAGNOSTIC
			printf("fdesc_lookup: newvp = %x\n", fvp);
#endif
			return (0);
		} else {
			error = ENOENT;
			goto bad;
		}

		/* fall through */

	case Fdevfd:
		if (ndp->ni_namelen == 2 && bcmp(pname, "..", 2) == 0) {
			ndp->ni_dvp = dvp;
			error = fdesc_root(dvp->v_mount, &ndp->ni_vp);
			return (error);
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

devfd:
		if (fd >= nfiles || p->p_fd->fd_ofiles[fd] == NULL) {
			error = EBADF;
			goto bad;
		}

#ifdef FDESC_DIAGNOSTIC
		printf("fdesc_lookup: allocate new vnode\n");
#endif
		error = fdesc_allocvp(Fdesc, FD_DESC+fd, dvp->v_mount, &fvp);
		if (error)
			goto bad;
		VTOFDESC(fvp)->fd_fd = fd;
		ndp->ni_dvp = dvp;
		ndp->ni_vp = fvp;
#ifdef FDESC_DIAGNOSTIC
		printf("fdesc_lookup: newvp = %x\n", fvp);
#endif
		return (0);
	}

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
	int error = 0;

	switch (VTOFDESC(vp)->fd_type) {
	case Fdesc:
		/*
		 * XXX Kludge: set p->p_dupfd to contain the value of the
		 * the file descriptor being sought for duplication. The error 
		 * return ensures that the vnode for this device will be
		 * released by vn_open. Open will detect this special error and
		 * take the actions in dupfdopen.  Other callers of vn_open or
		 * VOP_OPEN will simply report the error.
		 */
		p->p_dupfd = VTOFDESC(vp)->fd_fd;	/* XXX */
		error = ENODEV;
		break;

	case Fctty:
		error = cttyopen(devctty, mode, 0, p);
		break;
	}

	return (error);
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
	struct stat stb;
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
		if (error == 0 && vap->va_type == VDIR) {
			/*
			 * don't allow directories to show up because
			 * that causes loops in the namespace.
			 */
			vap->va_type = VFIFO;
		}
		break;

	case DTYPE_SOCKET:
		error = soo_stat((struct socket *)fp->f_data, &stb);
		if (error == 0) {
			vattr_null(vap);
			vap->va_type = VSOCK;
			vap->va_mode = stb.st_mode;
			vap->va_nlink = stb.st_nlink;
			vap->va_uid = stb.st_uid;
			vap->va_gid = stb.st_gid;
			vap->va_fsid = stb.st_dev;
			vap->va_fileid = stb.st_ino;
			vap->va_size = stb.st_size;
			vap->va_blocksize = stb.st_blksize;
			vap->va_atime.tv_sec = stb.st_atime;
			vap->va_atime.tv_usec = 0;
			vap->va_mtime.tv_sec = stb.st_mtime;
			vap->va_mtime.tv_usec = 0;
			vap->va_ctime.tv_sec = stb.st_ctime;
			vap->va_ctime.tv_usec = 0;
			vap->va_gen = stb.st_gen;
			vap->va_flags = stb.st_flags;
			vap->va_rdev = stb.st_rdev;
			vap->va_bytes = stb.st_blocks * stb.st_blksize;
		}
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
	int error = 0;

#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_getattr: stat type = %d\n", VTOFDESC(vp)->fd_type);
#endif

	switch (VTOFDESC(vp)->fd_type) {
	case Froot:
	case Fdevfd:
	case Flink:
	case Fctty:
		bzero((caddr_t) vap, sizeof(*vap));
		vattr_null(vap);
		vap->va_fileid = VTOFDESC(vp)->fd_ix;

		switch (VTOFDESC(vp)->fd_type) {
		case Flink:
			vap->va_mode = S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;
			vap->va_type = VLNK;
			vap->va_nlink = 1;
			/* vap->va_qsize = strlen(VTOFDESC(vp)->fd_link); */
			vap->va_size = strlen(VTOFDESC(vp)->fd_link);
			break;

		case Fctty:
			vap->va_mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
			vap->va_type = VFIFO;
			vap->va_nlink = 1;
			/* vap->va_qsize = 0; */
			vap->va_size = 0;
			break;

		default:
			vap->va_mode = S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;
			vap->va_type = VDIR;
			vap->va_nlink = 2;
			/* vap->va_qsize = 0; */
			vap->va_size = DEV_BSIZE;
			break;
		}
		vap->va_uid = 0;
		vap->va_gid = 0;
		vap->va_fsid = vp->v_mount->mnt_stat.f_fsid.val[0];
		vap->va_blocksize = DEV_BSIZE;
		vap->va_atime.tv_sec = boottime.tv_sec;
		vap->va_atime.tv_usec = 0;
		vap->va_mtime = vap->va_atime;
		vap->va_ctime = vap->va_ctime;
		vap->va_gen = 0;
		vap->va_flags = 0;
		vap->va_rdev = 0;
		/* vap->va_qbytes = 0; */
		vap->va_bytes = 0;
		break;

	case Fdesc:
#ifdef FDESC_DIAGNOSTIC
		printf("fdesc_getattr: stat desc #%d\n", VTOFDESC(vp)->fd_fd);
#endif
		fd = VTOFDESC(vp)->fd_fd;
		error = fdesc_attr(fd, vap, cred, p);
		break;

	default:
		panic("fdesc_getattr");
		break;	
	}

	if (error == 0)
		vp->v_type = vap->va_type;

#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_getattr: stat returns 0\n");
#endif
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
	switch (VTOFDESC(vp)->fd_type) {
	case Fdesc:
		break;

	case Fctty:
		return (0);

	default:
		return (EACCES);
	}

	fd = VTOFDESC(vp)->fd_fd;
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
		error = 0; /* EOPNOTSUPP? */
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

#define UIO_MX 16

static struct dirtmp {
	u_long d_ino;
	u_short d_reclen;
	u_short d_namlen;
	char d_name[8];
} rootent[] = {
	{ FD_DEVFD, UIO_MX, 2, "fd" },
	{ FD_STDIN, UIO_MX, 5, "stdin" },
	{ FD_STDOUT, UIO_MX, 6, "stdout" },
	{ FD_STDERR, UIO_MX, 6, "stderr" },
	{ FD_CTTY, UIO_MX, 3, "tty" },
	{ 0 }
};

fdesc_readdir(vp, uio, cred, eofflagp, cookies, ncookies)
	struct vnode *vp;
	struct uio *uio;
	struct ucred *cred;
	int *eofflagp;
	u_int *cookies;
	int ncookies;
{
	struct filedesc *fdp;
	int i;
	int error;

	switch (VTOFDESC(vp)->fd_type) {
	case Fctty:
		return (0);

	case Fdesc:
		return (ENOTDIR);

	default:
		break;
	}

	fdp = uio->uio_procp->p_fd;

	if (VTOFDESC(vp)->fd_type == Froot) {
		struct direct d;
		struct direct *dp = &d;
		struct dirtmp *dt;

		i = uio->uio_offset / UIO_MX;
		error = 0;

		while (uio->uio_resid > 0) {
			dt = &rootent[i];
			if (dt->d_ino == 0) {
				*eofflagp = 1;
				break;
			}
			i++;
			
			switch (dt->d_ino) {
			case FD_CTTY:
				if (cttyvp(uio->uio_procp) == NULL)
					continue;
				break;

			case FD_STDIN:
			case FD_STDOUT:
			case FD_STDERR:
				if ((dt->d_ino-FD_STDIN) >= fdp->fd_nfiles)
					continue;
				if (fdp->fd_ofiles[dt->d_ino-FD_STDIN] == NULL)
					continue;
				break;
			}
			bzero(dp, UIO_MX);
			dp->d_ino = dt->d_ino;
			dp->d_namlen = dt->d_namlen;
			dp->d_reclen = dt->d_reclen;
			bcopy(dt->d_name, dp->d_name, dp->d_namlen+1);
			error = uiomove((caddr_t) dp, UIO_MX, uio);
			if (error)
				break;
		}
		uio->uio_offset = i * UIO_MX;
		return (error);
	}

	i = uio->uio_offset / UIO_MX;
	error = 0;
	while (uio->uio_resid > 0) {
		if (i >= fdp->fd_nfiles) {
			*eofflagp = 1;
			break;
		}
		if (fdp->fd_ofiles[i] != NULL) {
			struct direct d;
			struct direct *dp = &d;
#ifdef FDESC_FILEID
			struct vattr va;
#endif
			bzero((caddr_t) dp, UIO_MX);

			dp->d_namlen = sprintf(dp->d_name, "%d", i);
			/*
			 * Fill in the remaining fields
			 */
			dp->d_reclen = UIO_MX;
			dp->d_ino = i + FD_STDIN;
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
		}
		i++;
	}

	uio->uio_offset = i * UIO_MX;
	return (error);
}

fdesc_readlink(vp, uio, cred)
	struct vnode *vp;
	struct uio *uio;
	struct ucred *cred;
{
	int error;

	if (VTOFDESC(vp)->fd_type == Flink) {
		char *ln = VTOFDESC(vp)->fd_link;
		error = uiomove(ln, strlen(ln), uio);
	} else {
		error = EOPNOTSUPP;
	}

	return (error);
}

fdesc_read(vp, uio, ioflag, cred)
	struct vnode *vp;
	struct uio *uio;
	int ioflag;
	struct ucred *cred;
{
	int error = EOPNOTSUPP;

	switch (VTOFDESC(vp)->fd_type) {
	case Fctty:
		error = cttyread(devctty, uio, ioflag);
		break;

	default:
		error = EOPNOTSUPP;
		break;
	}
	
	return (error);
}

fdesc_write(vp, uio, ioflag, cred)
	struct vnode *vp;
	struct uio *uio;
	int ioflag;
	struct ucred *cred;
{
	int error = EOPNOTSUPP;

	switch (VTOFDESC(vp)->fd_type) {
	case Fctty:
		error = cttywrite(devctty, uio, ioflag);
		break;

	default:
		error = EOPNOTSUPP;
		break;
	}
	
	return (error);
}

fdesc_ioctl(vp, command, data, fflag, cred, p)
	struct vnode *vp;
	int command;
	caddr_t data;
	int fflag;
	struct ucred *cred;
	struct proc *p;
{
	int error = EOPNOTSUPP;

#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_ioctl: type = %d, command = %x\n",
			VTOFDESC(vp)->fd_type, command);
#endif
	switch (VTOFDESC(vp)->fd_type) {
	case Fctty:
		error = cttyioctl(devctty, command, data, fflag, p);
		break;

	default:
		error = EOPNOTSUPP;
		break;
	}
	
	return (error);
}

fdesc_select(vp, which, fflags, cred, p)
	struct vnode *vp;
	int which;
	int fflags;
	struct ucred *cred;
	struct proc *p;
{
	int error = EOPNOTSUPP;

	switch (VTOFDESC(vp)->fd_type) {
	case Fctty:
		error = cttyselect(devctty, fflags, p);
		break;

	default:
		error = EOPNOTSUPP;
		break;
	}
	
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

int fdesc_reclaim(vp)
struct vnode *vp;
{
	int ix;

#ifdef FDESC_DIAGNOSTIC
	printf("fdesc_reclaim(%x)\n", vp);
#endif

	ix = VTOFDESC(vp)->fd_ix;
	if (ix >= 0 && ix < FD_MAX) {
		if (fdescvp[ix] != vp)
			panic("fdesc_reclaim");
		fdescvp[ix] = 0;
	}
	return (0);
}

/*
 * Print out the contents of a /dev/fd vnode.
 */
/* ARGSUSED */
fdesc_print(vp)
	struct vnode *vp;
{
	printf("tag VT_NON, fdesc vnode\n");
}

/*
 * /dev/fd vnode unsupported operation
 */
fdesc_enotsupp()
{
	return (EOPNOTSUPP);
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
#define fdesc_abortop ((int (*) __P(( \
		struct nameidata *ndp))) nullop)
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
