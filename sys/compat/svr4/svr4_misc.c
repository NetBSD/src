/*	$NetBSD: svr4_misc.c,v 1.5 1994/06/29 06:30:35 cgd Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 * from: Header: sun_misc.c,v 1.16 93/04/07 02:46:27 torek Exp 
 *
 *	@(#)sun_misc.c	8.1 (Berkeley) 6/18/93
 */

/*
 * SunOS compatibility module.
 *
 * SunOS system calls that are implemented differently in BSD are
 * handled here.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/dir.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/exec.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/socket.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sys/unistd.h>

#include <netinet/in.h>

#include <miscfs/specfs/specdev.h>

#include <vm/vm.h>

#define	szsigcode	(esigcode - sigcode)

struct svr4_wait4_args {
	int	pid;
	int	*status;
	int	options;
	struct	rusage *rusage;
};
svr4_wait4(p, uap, retval)
	struct proc *p;
	struct svr4_wait4_args *uap;
	int *retval;
{

	if (uap->pid == 0)
		uap->pid = WAIT_ANY;
	return (wait4(p, uap, retval));
}

struct svr4_wait_args {
	int	*status;
};
svr4_wait(p, uap, retval)
	struct proc *p;
	struct svr4_wait_args *uap;
	int *retval;
{
	int err;
	struct svr4_wait4_args w4;
	w4.pid = WAIT_ANY;
	err = wait4(p, &w4, retval);
	if (err != -1)
	    uap->status = w4.status;
	return err;
}

struct svr4_creat_args {
	char	*fname;
	int	fmode;
};
svr4_creat(p, uap, retval)
	struct proc *p;
	struct svr4_creat_args *uap;
	int *retval;
{
	struct args {
		char	*fname;
		int	mode;
		int	crtmode;
	} openuap;

	openuap.fname = uap->fname;
	openuap.crtmode = uap->fmode;
	openuap.mode = O_WRONLY | O_CREAT | O_TRUNC;
	return (open(p, &openuap, retval));
}

struct svr4_execv_args {
	char	*fname;
	char	**argp;
	char	**envp;		/* pseudo */
};
svr4_execv(p, uap, retval)
	struct proc *p;
	struct svr4_execv_args *uap;
	int *retval;
{

	uap->envp = NULL;
	return (execve(p, uap, retval));
}

struct svr4_unmount_args {
	char	*name;
	int	flags;	/* pseudo */
};
svr4_unmount(p, uap, retval)
	struct proc *p;
	struct svr4_unmount_args *uap;
	int *retval;
{

	uap->flags = 0;
	return (unmount(p, uap, retval));
}

#ifdef notyet
#define	SVR4_MS_RDONLY	0x01	/* mount fs read-only */
#define	SVR4_MS_NOSUID	0x02	/* mount fs with setuid disallowed */
#define	SVR4_MS_NEWTYPE	0x04	/* type is string (char *), not int */
#define	SVR4_MS_GRPID	0x08	/* (bsd semantics; ignored) */
#define	SVR4_MS_REMOUNT	0x10	/* update existing mount */
#define	SVR4_MS_NOSUB	0x20	/* prevent submounts (rejected) */
#define	SVR4_MS_MULTI	0x40	/* (ignored) */
#define	SVR4_MS_SYS5	0x80	/* Sys 5-specific semantics (rejected) */

struct	svr4_nfs_args {
	struct	sockaddr_in *addr;	/* file server address */
	caddr_t	fh;			/* file handle to be mounted */
	int	flags;			/* flags */
	int	wsize;			/* write size in bytes */
	int	rsize;			/* read size in bytes */
	int	timeo;			/* initial timeout in .1 secs */
	int	retrans;		/* times to retry send */
	char	*hostname;		/* server's hostname */
	int	acregmin;		/* attr cache file min secs */
	int	acregmax;		/* attr cache file max secs */
	int	acdirmin;		/* attr cache dir min secs */
	int	acdirmax;		/* attr cache dir max secs */
	char	*netname;		/* server's netname */
	struct	pathcnf *pathconf;	/* static pathconf kludge */
};

struct svr4_mount_args {
	char	*type;
	char	*dir;
	int	flags;
	caddr_t	data;
};
svr4_mount(p, uap, retval)
	struct proc *p;
	struct svr4_mount_args *uap;
	int *retval;
{
	int oflags = uap->flags, nflags, error;
	extern char sigcode[], esigcode[];
	char fsname[MFSNAMELEN];

	if (oflags & (SVR4M_NOSUB | SVR4M_SYS5))
		return (EINVAL);
	if ((oflags & SVR4M_NEWTYPE) == 0)
		return (EINVAL);
	nflags = 0;
	if (oflags & SVR4M_RDONLY)
		nflags |= MNT_RDONLY;
	if (oflags & SVR4M_NOSUID)
		nflags |= MNT_NOSUID;
	if (oflags & SVR4M_REMOUNT)
		nflags |= MNT_UPDATE;
	uap->flags = nflags;

	if (error = copyinstr((caddr_t)uap->type, fsname, sizeof fsname, (u_int *)0))
		return (error);

	if (strcmp(fsname, "4.2") == 0) {
		uap->type = (caddr_t)ALIGN(PS_STRINGS - szsigcode - STACKGAPLEN);
		if (error = copyout("ufs", uap->type, sizeof("ufs")))
			return (error);
	} else if (strcmp(fsname, "nfs") == 0) {
		struct svr4_nfs_args sna;
		struct sockaddr_in sain;
		struct nfs_args na;
		struct sockaddr sa;

		if (error = copyin(uap->data, &sna, sizeof sna))
			return (error);
		if (error = copyin(sna.addr, &sain, sizeof sain))
			return (error);
		bcopy(&sain, &sa, sizeof sa);
		sa.sa_len = sizeof(sain);
		uap->data = (caddr_t)ALIGN(PS_STRINGS - szsigcode - STACKGAPLEN);
		na.addr = (struct sockaddr *)((int)uap->data + sizeof na);
		na.sotype = SOCK_DGRAM;
		na.proto = IPPROTO_UDP;
		na.fh = (nfsv2fh_t *)sna.fh;
		na.flags = sna.flags;
		na.wsize = sna.wsize;
		na.rsize = sna.rsize;
		na.timeo = sna.timeo;
		na.retrans = sna.retrans;
		na.hostname = sna.hostname;

		if (error = copyout(&sa, na.addr, sizeof sa))
			return (error);
		if (error = copyout(&na, uap->data, sizeof na))
			return (error);
	}
	return (mount(p, uap, retval));
}
#endif

struct svr4_sigpending_args {
	int	*mask;
};
svr4_sigpending(p, uap, retval)
	struct proc *p;
	struct svr4_sigpending_args *uap;
	int *retval;
{
	int mask = p->p_siglist & p->p_sigmask;

	return (copyout((caddr_t)&mask, (caddr_t)uap->mask, sizeof(int)));
}

#ifdef notyet
struct svr4_dirent {
	u_long	d_ino;		/* file number of entry */
	u_long	d_off;		/* length of string in d_name */
	u_short	d_reclen;	/* length of this record */
	char	d_name[255 + 1];/* name must be no longer than this */
};

/*
 * Read Sun-style directory entries.  We suck them into kernel space so
 * that they can be massaged before being copied out to user code.  Like
 * SunOS, we squish out `empty' entries.
 *
 * This is quite ugly, but what do you expect from compatibility code?
 */
struct svr4_getdents_args {
	int	fd;
	char	*buf;
	int	nbytes;
};
svr4_getdents(p, uap, retval)
	struct proc *p;
	register struct svr4_getdents_args *uap;
	int *retval;
{
	register struct vnode *vp;
	register caddr_t inp, buf;	/* BSD-format */
	register int len, reclen;	/* BSD-format */
	register caddr_t outp;		/* Sun-format */
	register int resid;		/* Sun-format */
	struct file *fp;
	struct uio auio;
	struct iovec aiov;
	off_t off;			/* true file offset */
	long soff;			/* Sun file offset */
	int buflen, error, eofflag;
#define	BSD_DIRENT(cp) ((struct dirent *)(cp))
#define	SVR4_RECLEN(reclen) (reclen + sizeof(long))

	if ((error = getvnode(p->p_fd, uap->fd, &fp)) != 0)
		return (error);
	if ((fp->f_flag & FREAD) == 0)
		return (EBADF);
	vp = (struct vnode *)fp->f_data;
	if (vp->v_type != VDIR)	/* XXX  vnode readdir op should do this */
		return (EINVAL);
	buflen = min(MAXBSIZE, uap->nbytes);
	buf = malloc(buflen, M_TEMP, M_WAITOK);
	VOP_LOCK(vp);
	off = fp->f_offset;
again:
	aiov.iov_base = buf;
	aiov.iov_len = buflen;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_READ;
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_procp = p;
	auio.uio_resid = buflen;
	auio.uio_offset = off;
	/*
	 * First we read into the malloc'ed buffer, then
	 * we massage it into user space, one record at a time.
	 */
	if (error = VOP_READDIR(vp, &auio, fp->f_cred, &eofflag, NULL, 0))
		goto out;
	inp = buf;
	outp = uap->buf;
	resid = uap->nbytes;
	if ((len = buflen - auio.uio_resid) == 0)
		goto eof;
	for (; len > 0; len -= reclen) {
		reclen = ((struct dirent *)inp)->d_reclen;
		if (reclen & 3)
			panic("svr4_getdents");
		off += reclen;		/* each entry points to next */
		if (BSD_DIRENT(inp)->d_fileno == 0) {
			inp += reclen;	/* it is a hole; squish it out */
			continue;
		}
		if (reclen > len || resid < SVR4_RECLEN(reclen)) {
			/* entry too big for buffer, so just stop */
			outp++;
			break;
		}
		/*
		 * Massage in place to make a Sun-shaped dirent (otherwise
		 * we have to worry about touching user memory outside of
		 * the copyout() call).
		 */
		BSD_DIRENT(inp)->d_reclen = SVR4_RECLEN(reclen);
#if notdef
		BSD_DIRENT(inp)->d_type = 0; 	/* 4.4 specific */
#endif
		soff = off;
		if ((error = copyout((caddr_t)&soff, outp, sizeof soff)) != 0 ||
		    (error = copyout(inp, outp + sizeof soff, reclen)) != 0)
			goto out;
		/* advance past this real entry */
		inp += reclen;
		/* advance output past Sun-shaped entry */
		outp += SVR4_RECLEN(reclen);
		resid -= SVR4_RECLEN(reclen);
	}
	/* if we squished out the whole block, try again */
	if (outp == uap->buf)
		goto again;
	fp->f_offset = off;		/* update the vnode offset */
eof:
	*retval = uap->nbytes - resid;
out:
	VOP_UNLOCK(vp);
	free(buf, M_TEMP);
	return (error);
}
#endif

#define DEVZERO makedev(2, 12)

struct svr4_mmap_args {
	caddr_t	addr;
	size_t	len;
	int	prot;
	int	flags;
	int	fd;
	long	off;		/* not off_t! */
	off_t	qoff;		/* created here and fed to mmap() */
};
svr4_mmap(p, uap, retval)
	register struct proc *p;
	register struct svr4_mmap_args *uap;
	int *retval;
{
	register struct filedesc *fdp;
	register struct file *fp;
	register struct vnode *vp;

	/*
	 * Verify the arguments.
	 */
	if (uap->prot & ~(PROT_READ|PROT_WRITE|PROT_EXEC))
		return (EINVAL);			/* XXX still needed? */

	if ((uap->flags & MAP_FIXED) == 0 &&
		uap->addr != 0 &&
		uap->addr < (caddr_t)round_page(p->p_vmspace->vm_daddr+MAXDSIZ))
		uap->addr = (caddr_t)round_page(p->p_vmspace->vm_daddr+MAXDSIZ);

	/*
	 * Special case: if fd refers to /dev/zero, map as MAP_ANON.  (XXX)
	 */
	fdp = p->p_fd;
	if ((unsigned)uap->fd < fdp->fd_nfiles &&			/*XXX*/
	    (fp = fdp->fd_ofiles[uap->fd]) != NULL &&			/*XXX*/
	    fp->f_type == DTYPE_VNODE &&				/*XXX*/
	    (vp = (struct vnode *)fp->f_data)->v_type == VCHR &&	/*XXX*/
	    vp->v_rdev == DEVZERO) {					/*XXX*/
		uap->flags |= MAP_ANON;
		uap->fd = -1;
	}

	uap->qoff = uap->off;
	return (mmap(p, uap, retval));
}

#define	MC_SYNC		1
#define	MC_LOCK		2
#define	MC_UNLOCK	3
#define	MC_ADVISE	4
#define	MC_LOCKAS	5
#define	MC_UNLOCKAS	6

struct svr4_mctl_args {
	caddr_t	addr;
	size_t	len;
	int	func;
	void	*arg;
};
svr4_mctl(p, uap, retval)
	register struct proc *p;
	register struct svr4_mctl_args *uap;
	int *retval;
{

	switch (uap->func) {

	case MC_ADVISE:		/* ignore for now */
		return (0);

	case MC_SYNC:		/* translate to msync */
		return (msync(p, uap, retval));

	default:
		return (EINVAL);
	}
}

struct svr4_fchroot_args {
	int	fdes;
};
svr4_fchroot(p, uap, retval)
	register struct proc *p;
	register struct svr4_fchroot_args *uap;
	int *retval;
{
	register struct filedesc *fdp = p->p_fd;
	register struct vnode *vp;
	struct file *fp;
	int error;

	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
		return (error);
	if ((error = getvnode(fdp, uap->fdes, &fp)) != 0)
		return (error);
	vp = (struct vnode *)fp->f_data;
	VOP_LOCK(vp);
	if (vp->v_type != VDIR)
		error = ENOTDIR;
	else
		error = VOP_ACCESS(vp, VEXEC, p->p_ucred, p);
	VOP_UNLOCK(vp);
	if (error)
		return (error);
	VREF(vp);
	if (fdp->fd_rdir != NULL)
		vrele(fdp->fd_rdir);
	fdp->fd_rdir = vp;
	return (0);
}

/*
 * XXX: This needs cleaning up.
 */
svr4_auditsys(...)
{
	return 0;
}

struct svr4_utsname {
	char    sysname[257];
	char    nodename[257];
	char    release[257];
	char    version[257];
	char    machine[257];
};

struct svr4_uname_args {
	struct svr4_utsname *name;
};
svr4_uname(p, uap, retval)
	struct proc *p;
	struct svr4_uname_args *uap;
	int *retval;
{
	struct svr4_utsname sut;
	extern struct utsname utsname;

	/* first update utsname just as with NetBSD uname() */
	bcopy(hostname, utsname.nodename, sizeof(utsname.nodename));
	utsname.nodename[sizeof(utsname.nodename)-1] = '\0';

	/* then copy it over into SVR4 struct utsname */
	bzero(&sut, sizeof(sut));
	bcopy(utsname.sysname, sut.sysname, sizeof(utsname.sysname));
	bcopy(utsname.nodename, sut.nodename, sizeof(utsname.nodename));
	bcopy(utsname.release, sut.release, sizeof(utsname.release));
	bcopy(utsname.version, sut.version, sizeof(utsname.version));
	bcopy(utsname.machine, sut.machine, sizeof(utsname.machine));

	return copyout((caddr_t)&sut, (caddr_t)uap->name, sizeof(struct svr4_utsname));
}


#define SVR4_O_RDONLY	0
#define SVR4_O_WRONLY	1
#define SVR4_O_RDWR	2
#define SVR4_O_NDELAY	0x04
#define SVR4_O_APPEND	0x08
#define SVR4_O_SYNC	0x10
#define SVR4_O_NONBLOCK	0x80
#define SVR4_O_PRIV	0x1000
#define SVR4_O_CREAT	0x100
#define SVR4_O_TRUNC	0x200
#define SVR4_O_EXCL	0x400
#define SVR4_O_NOCTTY	0x800

struct svr4_open_args {
	char	*fname;
	int	fmode;
	int	crtmode;
};
svr4_open(p, uap, retval)
	struct proc *p;
	struct svr4_open_args *uap;
	int *retval;
{
	int l, r = 0;
	int noctty = uap->fmode & 0x8000;
	int ret;
	
	/* convert mode into NetBSD mode */
	l = uap->fmode;

	r |= (l & SVR4_O_RDONLY) ? O_RDONLY : 0;
	r |= (l & SVR4_O_WRONLY) ? O_WRONLY : 0;
	r |= (l & SVR4_O_RDWR) ? O_RDWR : 0;
	r |= (l & SVR4_O_NDELAY) ? O_NONBLOCK : 0;
	r |= (l & SVR4_O_APPEND) ? O_APPEND : 0;
	r |= (l & SVR4_O_SYNC) ? O_FSYNC : 0;
	r |= (l & SVR4_O_NONBLOCK) ? O_NONBLOCK : 0;
	r |= (l & SVR4_O_PRIV) ? O_EXLOCK : 0;
	r |= (l & SVR4_O_CREAT) ? O_CREAT : 0;
	r |= (l & SVR4_O_TRUNC) ? O_TRUNC : 0;
	r |= (l & SVR4_O_EXCL) ? O_EXCL : 0;
	r |= (l & SVR4_O_NOCTTY) ? O_NOCTTY : 0;

	uap->fmode = r;
	ret = open(p, uap, retval);

	if (!ret && !noctty && SESS_LEADER(p) && !(p->p_flag & P_CONTROLT)) {
		struct filedesc *fdp = p->p_fd;
		struct file *fp = fdp->fd_ofiles[*retval];

		/* ignore any error, just give it a try */
		if (fp->f_type == DTYPE_VNODE)
			(fp->f_ops->fo_ioctl)(fp, TIOCSCTTY, (caddr_t) 0, p);
	}
	return ret;
}

#ifdef notyet
#if defined (NFSSERVER)
struct nfssvc_args {
	int	fd;
	caddr_t mskval;
	int msklen;
	caddr_t mtchval;
	int mtchlen;
};

struct svr4_nfssvc_args {
	int	fd;
};
svr4_nfssvc(p, uap, retval)
	struct proc *p;
	struct svr4_nfssvc_args *uap;
	int *retval;
{
	struct nfssvc_args outuap;
	struct sockaddr sa;
	int error;
	extern char sigcode[], esigcode[];

	bzero(&outuap, sizeof outuap);
	outuap.fd = uap->fd;
	outuap.mskval = (caddr_t)ALIGN(PS_STRINGS - szsigcode - STACKGAPLEN);
	outuap.msklen = sizeof sa;
	outuap.mtchval = outuap.mskval + sizeof sa;
	outuap.mtchlen = sizeof sa;

	bzero(&sa, sizeof sa);
	if (error = copyout(&sa, outuap.mskval, outuap.msklen))
		return (error);
	if (error = copyout(&sa, outuap.mtchval, outuap.mtchlen))
		return (error);

	return nfssvc(p, &outuap, retval);
}
#endif /* NFSSERVER */

struct svr4_ustat {
	daddr_t	f_tfree;	/* total free */
	ino_t	f_tinode;	/* total inodes free */
	char	f_fname[6];	/* filsys name */
	char	f_fpack[6];	/* filsys pack name */
};

struct svr4_ustat_args {
	int	dev;
	struct	svr4_ustat *buf;
};
svr4_ustat(p, uap, retval)
	struct proc *p;
	struct svr4_ustat_args *uap;
	int *retval;
{
	struct svr4_ustat us;
	int error;

	bzero(&us, sizeof us);

	/*
	 * XXX: should set f_tfree and f_tinode at least
	 * How do we translate dev -> fstat? (and then to svr4_ustat)
	 */

	if (error = copyout(&us, uap->buf, sizeof us))
		return (error);
	return 0;
}

struct svr4_quotactl_args {
	int	cmd;
	char	*special;
	int	uid;
	caddr_t	addr;
};
svr4_quotactl(p, uap, retval)
	struct proc *p;
	struct svr4_quotactl_args *uap;
	int *retval;
{
	return EINVAL;
}


struct svr4_statfs {
	long	f_type;		/* type of info, zero for now */
	long	f_bsize;	/* fundamental file system block size */
	long	f_blocks;	/* total blocks in file system */
	long	f_bfree;	/* free blocks */
	long	f_bavail;	/* free blocks available to non-super-user */
	long	f_files;	/* total file nodes in file system */
	long	f_ffree;	/* free file nodes in fs */
	fsid_t	f_fsid;		/* file system id */
	long	f_spare[7];	/* spare for later */
};

static
svr4statfs(sp, buf)
	struct statfs *sp;
	caddr_t buf;
{
	struct svr4_statfs ssfs;

	bzero(&ssfs, sizeof ssfs);
	ssfs.f_type = 0;
	ssfs.f_bsize = sp->f_bsize;
	ssfs.f_blocks = sp->f_blocks;
	ssfs.f_bfree = sp->f_bfree;
	ssfs.f_bavail = sp->f_bavail;
	ssfs.f_files = sp->f_files;
	ssfs.f_ffree = sp->f_ffree;
	ssfs.f_fsid = sp->f_fsid;
	return copyout((caddr_t)&ssfs, buf, sizeof ssfs);
}	

struct svr4_statfs_args {
	char	*path;
	struct	svr4_statfs *buf;
};
svr4_statfs(p, uap, retval)
	struct proc *p;
	struct svr4_statfs_args *uap;
	int *retval;
{
	register struct mount *mp;
	register struct nameidata *ndp;
	register struct statfs *sp;
	int error;
	struct nameidata nd;

	ndp = &nd;
	ndp->ni_nameiop = LOOKUP | FOLLOW;
	ndp->ni_segflg = UIO_USERSPACE;
	ndp->ni_dirp = uap->path;
	if (error = namei(ndp, p))
		return (error);
	mp = ndp->ni_vp->v_mount;
	sp = &mp->mnt_stat;
	vrele(ndp->ni_vp);
	if (error = VFS_STATFS(mp, sp, p))
		return (error);
	sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
	return svr4statfs(sp, (caddr_t)uap->buf);
}

struct svr4_fstatfs_args {
	int	fd;
	struct	svr4_statfs *buf;
};
svr4_fstatfs(p, uap, retval)
	struct proc *p;
	struct svr4_fstatfs_args *uap;
	int *retval;
{
	struct file *fp;
	struct mount *mp;
	register struct statfs *sp;
	int error;

	if (error = getvnode(p->p_fd, uap->fd, &fp))
		return (error);
	mp = ((struct vnode *)fp->f_data)->v_mount;
	sp = &mp->mnt_stat;
	if (error = VFS_STATFS(mp, sp, p))
		return (error);
	sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
	return svr4statfs(sp, (caddr_t)uap->buf);
}

struct svr4_exportfs_args {
	char	*path;
	char	*ex;			/* struct svr4_export * */
};

svr4_exportfs(p, uap, retval)
	struct proc *p;
	struct svr4_exportfs_args *uap;
	int *retval;
{
	/*
	 * XXX: should perhaps translate into a mount(2)
	 * with MOUNT_EXPORT?
	 */
	return 0;
}
#endif

struct svr4_mknod_args {
	char	*fname;
	int	fmode;
	int	dev;
};

svr4_mknod(p, uap, retval)
	struct proc *p;
	struct svr4_mknod_args *uap;
	int *retval;
{
	if (S_ISFIFO(uap->fmode))
		return mkfifo(p, uap, retval);

	return mknod(p, uap, retval);
}

svr4_vhangup(p, uap, retval)
	struct proc *p;
	void *uap;
	int *retval;
{
	return 0;
}

#define SVR4_CONFIG_UNUSED	1
#define SVR4_CONFIG_NGROUPS	2
#define SVR4_CONFIG_CHILD_MAX	3
#define SVR4_CONFIG_OPEN_FILES	4
#define SVR4_CONFIG_POSIX_VER	5
#define SVR4_CONFIG_PAGESIZE	6
#define SVR4_CONFIG_CLK_TCK	7
#define SVR4_CONFIG_XOPEN_VER	8
#define SVR4_CONFIG_PROF_TCK	10

struct svr4_sysconfig_args {
	int	name;
};
svr4_sysconfig(p, uap, retval)
	struct proc *p;
	struct svr4_sysconfig_args *uap;
	int *retval;
{
	extern int maxfiles;

	switch(uap->name) {
	case SVR4_CONFIG_UNUSED:
		*retval = 0;
		break;
	case SVR4_CONFIG_NGROUPS:
		*retval = NGROUPS_MAX;
		break;
	case SVR4_CONFIG_CHILD_MAX:
		*retval = maxproc;
		break;
	case SVR4_CONFIG_OPEN_FILES:
		*retval = maxfiles;
		break;
	case SVR4_CONFIG_POSIX_VER:
		*retval = 198808;
		break;
	case SVR4_CONFIG_PAGESIZE:
		*retval = NBPG;
		break;
	case SVR4_CONFIG_CLK_TCK:
		*retval = 60;		/* should this be `hz', ie. 100? */
		break;
	case SVR4_CONFIG_XOPEN_VER:
		*retval = 2;		/* XXX: What should that be? */
		break;
	case SVR4_CONFIG_PROF_TCK:
		*retval = 60;		/* XXX: What should that be? */
		break;
	default:
		return EINVAL;
	}
	return 0;
}

#define SVR4_RLIMIT_NOFILE	5	/* Other RLIMIT_* are the same */
#define SVR4_RLIMIT_VMEM	6	/* Other RLIMIT_* are the same */
#define SVR4_RLIM_NLIMITS	7

struct svr4_getrlimit_args {
	int	which;
	struct	orlimit *rlp;
};
svr4_getrlimit(p, uap, retval)
	struct proc *p;
	struct svr4_getrlimit_args *uap;
	int *retval;
{
	if (uap->which >= SVR4_RLIM_NLIMITS)
		return EINVAL;

	if (uap->which == SVR4_RLIMIT_NOFILE)
		uap->which = RLIMIT_NOFILE;
	if (uap->which == SVR4_RLIMIT_VMEM)
		uap->which = RLIMIT_RSS;

	return ogetrlimit(p, uap, retval);
}

struct svr4_setrlimit_args {
	int	which;
	struct	orlimit *rlp;
};
svr4_setrlimit(p, uap, retval)
	struct proc *p;
	struct svr4_getrlimit_args *uap;
	int *retval;
{
	if (uap->which >= SVR4_RLIM_NLIMITS)
		return EINVAL;

	if (uap->which == SVR4_RLIMIT_NOFILE)
		uap->which = RLIMIT_NOFILE;

	if (uap->which == SVR4_RLIMIT_VMEM)
		uap->which = RLIMIT_RSS;

	return osetrlimit(p, uap, retval);
}

struct svr4_stat {
        u_long  st_dev;
        long    st_pad1[3];     /* reserved for network id */
        u_long  st_ino;
        u_long  st_mode;
        u_long  st_nlink;
        long    st_uid;
        long    st_gid;
        u_long  st_rdev;
        long    st_pad2[2];
        long    st_size;
        long    st_pad3;
        struct timespec st_atim;
        struct timespec st_mtim;
        struct timespec st_ctim;
        long    st_blksize;
        long    st_blocks;
        char    st_fstype[16];
        long    st_pad4[8];
};

static void
svr4_statcvt(st, st4)
	struct stat *st;
	struct svr4_stat *st4;
{
	bzero(st4, sizeof(*st4));
	st4->st_dev = st->st_dev;
	st4->st_ino = st->st_ino;
	st4->st_mode = st->st_mode;
	st4->st_nlink = st->st_nlink;
	st4->st_uid = st->st_uid;
	st4->st_gid = st->st_gid;
	st4->st_rdev = st->st_rdev;
	st4->st_size = st->st_size;
	st4->st_atim = st->st_atimespec;
	st4->st_mtim = st->st_mtimespec;
	st4->st_ctim = st->st_ctimespec;
	st4->st_blksize = st->st_blksize;
	st4->st_blocks = st->st_blocks;
	strcpy(st4->st_fstype, "unknown");
}


struct stat_args {
	char	*fname;
	struct stat *st;
};

struct svr4_stat_args {
	char	*fname;
	struct svr4_stat *st;
};
svr4_stat(p, uap, retval)
	struct proc *p;
	struct svr4_stat_args *uap;
	int *retval;
{
	extern char sigcode[], esigcode[];
	struct stat st;
	struct svr4_stat svr4_st;
	struct stat_args cup;
	int error;

	cup.fname = uap->fname;
	cup.st = (struct stat *)ALIGN(PS_STRINGS - szsigcode - STACKGAPLEN);
	if (error = stat(p, &cup, retval))
		return (error);
	if (error = copyin(cup.st, &st, sizeof st))
		return (error);
	svr4_statcvt(&st, &svr4_st);
	if (error = copyout(&svr4_st, uap->st, sizeof svr4_st))
		return (error);
	return (0);
}

struct lstat_args {
	char	*fname;
	struct stat *st;
};

struct svr4_lstat_args {
	char	*fname;
	struct svr4_lstat *st;
};
svr4_lstat(p, uap, retval)
	struct proc *p;
	struct svr4_lstat_args *uap;
	int *retval;
{
	extern char sigcode[], esigcode[];
	struct stat st;
	struct svr4_stat svr4_st;
	struct lstat_args cup;
	int error;

	cup.fname = uap->fname;
	cup.st = (struct stat *)ALIGN(PS_STRINGS - szsigcode - STACKGAPLEN);
	if (error = lstat(p, &cup, retval))
		return (error);
	if (error = copyin(cup.st, &st, sizeof st))
		return (error);
	svr4_statcvt(&st, &svr4_st);
	if (error = copyout(&svr4_st, uap->st, sizeof svr4_st))
		return (error);
	return (0);
}

struct fstat_args {
	int fd;
	struct stat *st;
};

struct svr4_fstat_args {
	int fd;
	struct svr4_stat *st;
};
svr4_fstat(p, uap, retval)
	struct proc *p;
	struct svr4_fstat_args *uap;
	int *retval;
{
	extern char sigcode[], esigcode[];
	struct stat st;
	struct svr4_stat svr4_st;
	struct fstat_args cup;
	int error;

	cup.fd = uap->fd;
	cup.st = (struct stat *)ALIGN(PS_STRINGS - szsigcode - STACKGAPLEN);
	if (error = fstat(p, &cup, retval))
		return (error);
	if (error = copyin(cup.st, &st, sizeof st))
		return (error);
	svr4_statcvt(&st, &svr4_st);
	if (error = copyout(&svr4_st, uap->st, sizeof svr4_st))
		return (error);
	return (0);
}


struct svr4_syssun_args {
	int gate;
	/* ... */
};
svr4_syssun(p, uap, retval)
	struct proc *p;
	struct svr4_syssun_args *uap;
	int *retval;
{
#ifdef DEBUG_SVR4
	printf("syssun(%d)\n", uap->gate);
#endif
	return 0;
}
