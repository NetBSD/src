/*
 * Copyright (c) 1992 The Regents of the University of California
 * Copyright (c) 1990, 1992 Jan-Simon Pendry
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
 *	Id: portal_vnops.c,v 1.5 1993/09/22 17:57:20 jsp Exp
 *
 *	$Id: portal_vnops.c,v 1.1 1994/01/05 14:23:29 cgd Exp $
 */

/*
 * Portal Filesystem
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/proc.h>
/*#include <sys/resourcevar.h>*/
#include <sys/filedesc.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/namei.h>
/*#include <sys/buf.h>*/
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/un.h>
#include <sys/unpcb.h>
#include <miscfs/portal/portal.h>

static int portal_fileid = PORTAL_ROOTFILEID+1;

static void
portal_closefd(p, fd)
	struct proc *p;
	int fd;
{
	int error;
	struct {
		int fd;
	} ua;
	int rc;

	ua.fd = fd;
	error = close(p, &ua, &rc);
	/*
	 * We should never get an error, and there isn't anything
	 * we could do if we got one, so just print a message.
	 */
	if (error)
		printf("portal_closefd: error = %d\n", error);
}

/*
 * vp is the current namei directory
 * ndp is the name to locate in that directory...
 */
portal_lookup(dvp, ndp, p)
	struct vnode *dvp;
	struct nameidata *ndp;
	struct proc *p;
{
	char *pname = ndp->ni_ptr;
	struct portalnode *pt;
	int error;
	struct vnode *fvp = 0;
	char *path;
	int size;

#ifdef PORTAL_DIAGNOSTIC
	printf("portal_lookup(%s)\n", pname);
#endif
	if (ndp->ni_namelen == 1 && *pname == '.') {
		ndp->ni_dvp = dvp;
		ndp->ni_vp = dvp;
		VREF(dvp);
		/*VOP_LOCK(dvp);*/
		return (0);
	}


#ifdef PORTAL_DIAGNOSTIC
	printf("portal_lookup: allocate new vnode\n");
#endif
	error = getnewvnode(VT_UFS, dvp->v_mount, &portal_vnodeops, &fvp);
	if (error)
		goto bad;
	fvp->v_type = VREG;

	pt = VTOPORTAL(fvp);
	/*
	 * Save all of the remaining pathname and
	 * advance the namei next pointer to the end
	 * of the string.
	 */
	for (size = 0, path = pname; *path; path++)
		size++;
	ndp->ni_next = path;
	ndp->ni_pathlen -= size - ndp->ni_namelen;
	pt->pt_arg = malloc(size+1, M_TEMP, M_WAITOK);
	pt->pt_size = size+1;
	bcopy(pname, pt->pt_arg, pt->pt_size);
	pt->pt_fileid = portal_fileid++;

	ndp->ni_dvp = dvp;
	ndp->ni_vp = fvp;
	/*VOP_LOCK(fvp);*/
#ifdef PORTAL_DIAGNOSTIC
	printf("portal_lookup: newvp = %x\n", fvp);
#endif
	return (0);

bad:;
	if (fvp) {
#ifdef PORTAL_DIAGNOSTIC
		printf("portal_lookup: vrele(%x)\n", fvp);
#endif
		vrele(fvp);
	}
	ndp->ni_dvp = dvp;
	ndp->ni_vp = NULL;
#ifdef PORTAL_DIAGNOSTIC
	printf("portal_lookup: error = %d\n", error);
#endif
	return (error);
}

static int
portal_connect(so, so2)
	struct socket *so;
	struct socket *so2;
{
	/* from unp_connect, bypassing the namei stuff... */

	struct socket *so3;
	struct unpcb *unp2;
	struct unpcb *unp3;

#ifdef PORTAL_DIAGNOSTIC
	printf("portal_connect\n");
#endif

	if (so2 == 0)
		return (ECONNREFUSED);

	if (so->so_type != so2->so_type)
		return (EPROTOTYPE);

	if ((so2->so_options & SO_ACCEPTCONN) == 0)
		return (ECONNREFUSED);

#ifdef PORTAL_DIAGNOSTIC
	printf("portal_connect: calling sonewconn\n");
#endif

	if ((so3 = sonewconn(so2, 0)) == 0)
		return (ECONNREFUSED);

	unp2 = sotounpcb(so2);
	unp3 = sotounpcb(so3);
	if (unp2->unp_addr)
		unp3->unp_addr = m_copy(unp2->unp_addr, 0, (int)M_COPYALL);

	so2 = so3;

#ifdef PORTAL_DIAGNOSTIC
	printf("portal_connect: calling unp_connect2\n");
#endif

	return (unp_connect2(so, so2));
}

portal_open(vp, mode, cred, p)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
	struct proc *p;
{
	struct socket *so = 0;
	struct portalnode *pt;
	int s;
	struct uio auio;
	struct iovec aiov[2];
	int res;
	struct mbuf *cm = 0;
	struct cmsghdr *cmsg;
	int newfds;
	int *ip;
	int fd;
	int error;
	int len;
	struct portalmount *fmp;
	struct file *fp;
	struct portal_cred pcred;

	/*
	 * Nothing to do when opening the root node.
	 */
	if (vp->v_flag & VROOT)
		return (0);

#ifdef PORTAL_DIAGNOSTIC
	printf("portal_open(%x)\n", vp);
#endif

	/*
	 * Can't be opened unless the caller is set up
	 * to deal with the side effects.  Check for this
	 * by testing whether the p_dupfd has been set.
	 */
	if (p->p_dupfd >= 0)
		return (ENODEV);

	pt = VTOPORTAL(vp);
	fmp = VFSTOPORTAL(vp->v_mount);

	/*
	 * Create a new socket.
	 */
	error = socreate(AF_UNIX, &so, SOCK_STREAM, 0);
	if (error)
		goto bad;

	/*
	 * Reserve some buffer space
	 */
#ifdef PORTAL_DIAGNOSTIC
	printf("portal_open: calling soreserve\n");
#endif
	res = max(512, pt->pt_size + 128);
	error = soreserve(so, res, res);
	if (error)
		goto bad;

	/*
	 * Kick off connection
	 */
#ifdef PORTAL_DIAGNOSTIC
	printf("portal_open: calling portal_connect\n");
#endif
	error = portal_connect(so, (struct socket *)fmp->pm_server->f_data);
	if (error)
		goto bad;

	/*
	 * Wait for connection to complete
	 */
#ifdef PORTAL_DIAGNOSTIC
	printf("portal_open: waiting for connect\n");
#endif
	/*
	 * XXX: Since the mount point is holding a reference on the
	 * underlying server socket, it is not easy to find out whether
	 * the server process is still running.  To handle this problem
	 * we loop waiting for the new socket to be connected (something
	 * which will only happen if the server is still running) or for
	 * the reference count on the server socket to drop to 1, which
	 * will happen if the server dies.  Sleep for 5 second intervals
	 * and keep polling the reference count.   XXX.
	 */
	s = splnet();
	while ((so->so_state & SS_ISCONNECTING) && so->so_error == 0) {
		if (fmp->pm_server->f_count == 1) {
			error = ECONNREFUSED;
			splx(s);
#ifdef PORTAL_DIAGNOSTIC
			printf("portal_open: server process has gone away\n");
#endif
			goto bad;
		}
		(void) tsleep((caddr_t) &so->so_timeo, PSOCK, "portalcon", 5 * hz);
	}
	splx(s);

	if (so->so_error) {
		error = so->so_error;
		goto bad;
	}
		
	/*
	 * Set miscellaneous flags
	 */
	so->so_rcv.sb_timeo = 0;
	so->so_snd.sb_timeo = 0;
	so->so_rcv.sb_flags |= SB_NOINTR;
	so->so_snd.sb_flags |= SB_NOINTR;

#ifdef PORTAL_DIAGNOSTIC
	printf("portal_open: constructing data uio\n");
#endif

	pcred.pcr_uid = cred->cr_uid;
	pcred.pcr_gid = cred->cr_gid;
	aiov[0].iov_base = (caddr_t) &pcred;
	aiov[0].iov_len = sizeof(pcred);
	aiov[1].iov_base = pt->pt_arg;
	aiov[1].iov_len = pt->pt_size;
	auio.uio_iov = aiov;
	auio.uio_iovcnt = 2;
	auio.uio_rw = UIO_WRITE;
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_procp = p;
	auio.uio_offset = 0;
	auio.uio_resid = aiov[0].iov_len + aiov[1].iov_len;

#ifdef PORTAL_DIAGNOSTIC
	printf("portal_open: sending data to server\n");
#endif
	error = sosend(so, (struct mbuf *) 0, &auio,
			(struct mbuf *) 0, (struct mbuf *) 0, 0);
	if (error)
		goto bad;

	len = auio.uio_resid = sizeof(int);
	do {
		struct mbuf *m = 0;
		int flags = MSG_WAITALL;
#ifdef PORTAL_DIAGNOSTIC
		printf("portal_open: receiving data from server\n");
		printf("portal_open: so = %x, cm = %x, resid = %d\n",
				so, cm, auio.uio_resid);
		printf("portal_open, uio=%x, mp0=%x, controlp=%x\n", &auio, &cm);
#endif
		error = soreceive(so, (struct mbuf **) 0, &auio,
					&m, &cm, &flags);
#ifdef PORTAL_DIAGNOSTIC
		printf("portal_open: after receiving data\n");
		printf("portal_open: so = %x, cm = %x, resid = %d\n",
				so, cm, auio.uio_resid);
#endif
		if (error)
			goto bad;

		/*
		 * Grab an error code from the mbuf.
		 */
		if (m) {
			m = m_pullup(m, sizeof(int));	/* Needed? */
			if (m) {
				error = *(mtod(m, int *));
				m_freem(m);
			} else {
				error = EINVAL;
			}
#ifdef PORTAL_DIAGNOSTIC
			printf("portal_open: error returned is %d\n", error);
#endif
		} else {
			if (cm == 0) {
#ifdef PORTAL_DIAGNOSTIC
				printf("portal_open: no rights received\n");
#endif
				error = ECONNRESET;	 /* XXX */
#ifdef notdef
				break;
#endif
			}
		}
	} while (cm == 0 && auio.uio_resid == len && !error);

	if (cm == 0)
		goto bad;

	if (auio.uio_resid) {
#ifdef PORTAL_DIAGNOSTIC
		printf("portal_open: still need another %d bytes\n", auio.uio_resid);
#endif
		error = 0;
#ifdef notdef
		error = EMSGSIZE;
		goto bad;
#endif
	}

	/*
	 * XXX: Break apart the control message, and retrieve the
	 * received file descriptor.  Note that more than one descriptor
	 * may have been received, or that the rights chain may have more
	 * than a single mbuf in it.  What to do?
	 */
#ifdef PORTAL_DIAGNOSTIC
	printf("portal_open: about to break apart control message\n");
#endif
	cmsg = mtod(cm, struct cmsghdr *);
	newfds = (cmsg->cmsg_len - sizeof(*cmsg)) / sizeof (int);
	if (newfds == 0) {
#ifdef PORTAL_DIAGNOSTIC
		printf("portal_open: received no fds\n");
#endif
		error = ECONNREFUSED;
		goto bad;
	}
	/*
	 * At this point the rights message consists of a control message
	 * header, followed by a data region containing a vector of
	 * integer file descriptors.  The fds were allocated by the action
	 * of receiving the control message.
	 */
	ip = (int *) (cmsg + 1);
	fd = *ip++;
	if (newfds > 1) {
		/*
		 * Close extra fds.
		 */
		int i;
		printf("portal_open: %d extra fds\n", newfds - 1);
		for (i = 1; i < newfds; i++) {
			portal_closefd(p, *ip);
			ip++;
		}
	}

	/*
	 * Check that the mode the file is being opened for is a subset 
	 * of the mode of the existing descriptor.
	 */
#ifdef PORTAL_DIAGNOSTIC
	printf("portal_open: checking file flags, fd = %d\n", fd);
#endif
 	fp = p->p_fd->fd_ofiles[fd];
	if (((mode & (FREAD|FWRITE)) | fp->f_flag) != fp->f_flag) {
		portal_closefd(p, fd);
		error = EACCES;
		goto bad;
	}

#ifdef PORTAL_DIAGNOSTIC
	printf("portal_open: got fd = %d\n", fd);
#endif
	/*
	 * Save the dup fd in the proc structure then return the
	 * special error code (ENXIO) which causes magic things to
	 * happen in vn_open.  The whole concept is, well, hmmm.
	 */
	p->p_dupfd = fd;
	error = ENXIO;

bad:;
	/*
	 * And discard the control message.
	 */
	if (cm) { 
#ifdef PORTAL_DIAGNOSTIC
		printf("portal_open: free'ing control message\n");
#endif
		m_freem(cm);
	}

	if (so) {
#ifdef PORTAL_DIAGNOSTIC
		printf("portal_open: calling soshutdown\n");
#endif
		soshutdown(so, 2);
#ifdef PORTAL_DIAGNOSTIC
		printf("portal_open: calling soclose\n");
#endif
		soclose(so);
	}
#ifdef PORTAL_DIAGNOSTIC
	if (error != ENODEV)
		printf("portal_open: error = %d\n", error);
#endif
	return (error);

#if 0
	/*
	 * XXX Kludge: set curproc->p_dupfd to contain the value of the
	 * the file descriptor being sought for duplication. The error 
	 * return ensures that the vnode for this device will be released
	 * by vn_open. Open will detect this special error and take the
	 * actions in dupfdopen.  Other callers of vn_open or VOP_OPEN
	 * will simply report the error.
	 */

	fdp = p->p_fd;
	dfd = VTOPORTAL(vp)->f_fd;

	/*
	 * Check that the file descriptor is valid
	 */
	if (dfd >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[dfd]) == NULL)
	    	return (EBADF);

	/*
	 * Check that the mode the file is being opened for is a subset 
	 * of the mode of the existing descriptor.
	 */
	if (((mode & (FREAD|FWRITE)) | fp->f_flag) != fp->f_flag)
		return (EACCES);

	/*
	 * Go ahead and dup the file descriptor.
	 * The file pointer will be stolen back in
	 * dupfdopen, and the file descriptor freed.
	 */
	if (error = fdalloc(p, 0, &fd))
		return (error);
	fdp->fd_ofiles[fd] = fp;
	fdp->fd_ofileflags[fd] = fdp->fd_ofileflags[dfd] /* &~ UF_EXCLOSE */;
	fp->f_count++;
	if (fd > fdp->fd_lastfile)
		fdp->fd_lastfile = fd;
	p->p_dupfd = fd;		/* XXX */
	return (ENODEV);
#endif
}

portal_getattr(vp, vap, cred, p)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
	struct proc *p;
{
	unsigned fd;
	int error;

	bzero((caddr_t) vap, sizeof(*vap));
	vattr_null(vap);
	vap->va_uid = 0;
	vap->va_gid = 0;
	vap->va_fsid = vp->v_mount->mnt_stat.f_fsid.val[0];
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
	/* vap->va_qsize = 0; */
	if (vp->v_flag & VROOT) {
#ifdef PORTAL_DIAGNOSTIC
		printf("portal_getattr: stat rootdir\n");
#endif
		vap->va_type = VDIR;
		vap->va_mode = S_IRUSR|S_IWUSR|S_IXUSR|
				S_IRGRP|S_IWGRP|S_IXGRP|
				S_IROTH|S_IWOTH|S_IXOTH;
		vap->va_nlink = 2;
		vap->va_fileid = 2;
	} else {
#ifdef PORTAL_DIAGNOSTIC
		printf("portal_getattr: stat portal\n");
#endif
		vap->va_type = VREG;
		vap->va_mode = S_IRUSR|S_IWUSR|
				S_IRGRP|S_IWGRP|
				S_IROTH|S_IWOTH;
		vap->va_nlink = 1;
		vap->va_fileid = VTOPORTAL(vp)->pt_fileid;
	}
	return (0);
}

portal_setattr(vp, vap, cred, p)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
	struct proc *p;
{
	/*
	 * Can't mess with the root vnode
	 */
	if (vp->v_flag & VROOT)
		return (EACCES);

	return (0);
}

/*
 * Fake readdir, just return empty directory.
 * It is hard to deal with '.' and '..' so don't bother.
 */
portal_readdir(vp, uio, cred, eofflagp, cookies, ncookies)
	struct vnode *vp;
	struct uio *uio;
	struct ucred *cred;
	int *eofflagp;
	u_int *cookies;
	int ncookies;
{
	*eofflagp = 1;
	return (0);
}

portal_inactive(vp, p)
	struct vnode *vp;
	struct proc *p;
{
#ifdef PORTAL_DIAGNOSTIC
	if (VTOPORTAL(vp)->pt_arg)
		printf("portal_inactive(%x, %s)\n", vp, VTOPORTAL(vp)->pt_arg);
	else
		printf("portal_inactive(%x)\n", vp);
#endif
	/*vgone(vp);*/
	return (0);
}

portal_reclaim(vp)
	struct vnode *vp;
{
	struct portalnode *pt = VTOPORTAL(vp);
	if (pt->pt_arg) {
		free((caddr_t) pt->pt_arg, M_TEMP);
		pt->pt_arg = 0;
	}
	printf("portal_reclaim(%x)\n", vp);
	return (0);
}

/*
 * Print out the contents of a Portal vnode.
 */
/* ARGSUSED */
portal_print(vp)
	struct vnode *vp;
{
	printf("tag VT_PORTAL, portal vnode\n");
}

/*
 * Portal vnode unsupported operation
 */
portal_enotsupp()
{
	return (EOPNOTSUPP);
}

/*
 * Portal "should never get here" operation
 */
portal_badop()
{
	panic("portal: bad op");
	/* NOTREACHED */
}

/*
 * Portal vnode null operation
 */
portal_nullop()
{
	return (0);
}

#define portal_create ((int (*) __P(( \
		struct nameidata *ndp, \
		struct vattr *vap, \
		struct proc *p))) portal_enotsupp)
#define portal_mknod ((int (*) __P(( \
		struct nameidata *ndp, \
		struct vattr *vap, \
		struct ucred *cred, \
		struct proc *p))) portal_enotsupp)
#define portal_close ((int (*) __P(( \
		struct vnode *vp, \
		int fflag, \
		struct ucred *cred, \
		struct proc *p))) nullop)
#define portal_access ((int (*) __P(( \
		struct vnode *vp, \
		int mode, \
		struct ucred *cred, \
		struct proc *p))) nullop)
#define	portal_read ((int (*) __P(( \
		struct vnode *vp, \
		struct uio *uio, \
		int ioflag, \
		struct ucred *cred))) portal_enotsupp)
#define	portal_write ((int (*) __P(( \
		struct vnode *vp, \
		struct uio *uio, \
		int ioflag, \
		struct ucred *cred))) portal_enotsupp)
#define	portal_ioctl ((int (*) __P(( \
		struct vnode *vp, \
		int command, \
		caddr_t data, \
		int fflag, \
		struct ucred *cred, \
		struct proc *p))) portal_enotsupp)
#define	portal_select ((int (*) __P(( \
		struct vnode *vp, \
		int which, \
		int fflags, \
		struct ucred *cred, \
		struct proc *p))) portal_enotsupp)
#define portal_mmap ((int (*) __P(( \
		struct vnode *vp, \
		int fflags, \
		struct ucred *cred, \
		struct proc *p))) portal_enotsupp)
#define portal_fsync ((int (*) __P(( \
		struct vnode *vp, \
		int fflags, \
		struct ucred *cred, \
		int waitfor, \
		struct proc *p))) nullop)
#define portal_seek ((int (*) __P(( \
		struct vnode *vp, \
		off_t oldoff, \
		off_t newoff, \
		struct ucred *cred))) nullop)
#define portal_remove ((int (*) __P(( \
		struct nameidata *ndp, \
		struct proc *p))) portal_enotsupp)
#define portal_link ((int (*) __P(( \
		struct vnode *vp, \
		struct nameidata *ndp, \
		struct proc *p))) portal_enotsupp)
#define portal_rename ((int (*) __P(( \
		struct nameidata *fndp, \
		struct nameidata *tdnp, \
		struct proc *p))) portal_enotsupp)
#define portal_mkdir ((int (*) __P(( \
		struct nameidata *ndp, \
		struct vattr *vap, \
		struct proc *p))) portal_enotsupp)
#define portal_rmdir ((int (*) __P(( \
		struct nameidata *ndp, \
		struct proc *p))) portal_enotsupp)
#define portal_symlink ((int (*) __P(( \
		struct nameidata *ndp, \
		struct vattr *vap, \
		char *target, \
		struct proc *p))) portal_enotsupp)
#define portal_readlink ((int (*) __P(( \
		struct vnode *vp, \
		struct uio *uio, \
		struct ucred *cred))) portal_enotsupp)
#define portal_abortop ((int (*) __P(( \
		struct nameidata *ndp))) nullop)
#define	portal_lock ((int (*) __P(( \
		struct vnode *vp))) nullop)
#define portal_unlock ((int (*) __P(( \
		struct vnode *vp))) nullop)
#define	portal_bmap ((int (*) __P(( \
		struct vnode *vp, \
		daddr_t bn, \
		struct vnode **vpp, \
		daddr_t *bnp))) portal_badop)
#define	portal_strategy ((int (*) __P(( \
		struct buf *bp))) portal_badop)
#define portal_islocked ((int (*) __P(( \
		struct vnode *vp))) nullop)
#define portal_advlock ((int (*) __P(( \
		struct vnode *vp, \
		caddr_t id, \
		int op, \
		struct flock *fl, \
		int flags))) portal_enotsupp)

struct vnodeops portal_vnodeops = {
	portal_lookup,		/* lookup */
	portal_create,		/* create */
	portal_mknod,		/* mknod */
	portal_open,		/* open */
	portal_close,		/* close */
	portal_access,		/* access */
	portal_getattr,		/* getattr */
	portal_setattr,		/* setattr */
	portal_read,		/* read */
	portal_write,		/* write */
	portal_ioctl,		/* ioctl */
	portal_select,		/* select */
	portal_mmap,		/* mmap */
	portal_fsync,		/* fsync */
	portal_seek,		/* seek */
	portal_remove,		/* remove */
	portal_link,		/* link */
	portal_rename,		/* rename */
	portal_mkdir,		/* mkdir */
	portal_rmdir,		/* rmdir */
	portal_symlink,		/* symlink */
	portal_readdir,		/* readdir */
	portal_readlink,	/* readlink */
	portal_abortop,		/* abortop */
	portal_inactive,	/* inactive */
	portal_reclaim,		/* reclaim */
	portal_lock,		/* lock */
	portal_unlock,		/* unlock */
	portal_bmap,		/* bmap */
	portal_strategy,	/* strategy */
	portal_print,		/* print */
	portal_islocked,	/* islocked */
	portal_advlock,		/* advlock */
};
