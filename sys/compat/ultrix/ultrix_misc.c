/*	$NetBSD: ultrix_misc.c,v 1.10 1995/03/09 12:06:17 mycroft Exp $	*/

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
 *
 *	@(#)sun_misc.c	8.1 (Berkeley) 6/18/93
 *
 * from: Header: sun_misc.c,v 1.16 93/04/07 02:46:27 torek Exp 
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

#include <nfs/rpcv2.h>
#include <nfs/nfsv2.h>
#include <nfs/nfs.h>

#include <vm/vm.h>

#define GSI_PROG_ENV 1

struct ultrix_getsysinfo_args {
	unsigned	op;
	char		*buffer;
	unsigned	nbytes;
	int		*start;
	char		*arg;
};

ultrix_getsysinfo(p, uap, retval)
	struct proc *p;
	struct ultrix_getsysinfo_args *uap;
	int *retval;
{
	static short progenv = 0;

	switch (uap->op) {
		/* operations implemented: */
	case GSI_PROG_ENV:
		if (uap->nbytes < sizeof(short))
			return EINVAL;
		*retval = 1;
		return(copyout(&progenv, uap->buffer, sizeof(short)));
	default:
		*retval = 0; /* info unavail */
		return 0;
	}
}

struct ultrix_setsysinfo_args {
	unsigned	op;
	char		*buffer;
	unsigned	nbytes;
	unsigned	arg;
	unsigned	flag;
};

ultrix_setsysinfo(p, uap, retval)
	struct proc *p;
	struct ultrix_setsysinfo_args *uap;
	int *retval;
{
	*retval = 0;
	return 0;
}

struct ultrix_waitpid_args {
	int	pid;
	int	*status;
	int	options;
	struct	rusage *rusage;
};

ultrix_waitpid(p, uap, retval)
	struct proc *p;
	struct ultrix_waitpid_args *uap;
	register_t *retval;
{

	uap->rusage = 0;
	return (wait4(p, uap, retval));
}
struct sun_wait4_args {
	int	pid;
	int	*status;
	int	options;
	struct	rusage *rusage;
	int compat;
};
sun_wait4(p, uap, retval)
	struct proc *p;
	struct sun_wait4_args *uap;
	register_t *retval;
{

	if (uap->pid == 0)
		uap->pid = WAIT_ANY;
	return (wait4(p, uap, retval));
}

struct sun_wait3_args {
	int	*status;
	int	options;
	struct	rusage *rusage;
};

sun_wait3(p, uap, retval)
	struct proc *p;
	struct sun_wait3_args *uap;
	register_t *retval;
{
	struct sun_wait4_args ua;

	if (uap == NULL)
		panic("uap == NULL");
	ua.pid = -1;
	ua.status = uap->status;
	ua.options = uap->options;
	ua.rusage = uap->rusage;

	return (wait4(p, &ua, retval));
}

struct sun_creat_args {
	char	*fname;
	int	fmode;
};
sun_creat(p, uap, retval)
	struct proc *p;
	struct sun_creat_args *uap;
	register_t *retval;
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

struct sun_execv_args {
	char	*fname;
	char	**argp;
	char	**envp;		/* pseudo */
};
sun_execv(p, uap, retval)
	struct proc *p;
	struct sun_execv_args *uap;
	register_t *retval;
{

	uap->envp = NULL;
	return (execve(p, uap, retval));
}

struct sun_omsync_args {
	caddr_t	addr;
	int	len;
	int	flags;
};
sun_omsync(p, uap, retval)
	struct proc *p;
	struct sun_omsync_args *uap;
	register_t *retval;
{

	if (uap->flags)
		return (EINVAL);
	return (msync(p, uap, retval));
}

struct sun_unmount_args {
	char	*name;
	int	flags;	/* pseudo */
};
sun_unmount(p, uap, retval)
	struct proc *p;
	struct sun_unmount_args *uap;
	register_t *retval;
{

	uap->flags = 0;
	return (unmount(p, uap, retval));
}

#define	SUNM_RDONLY	0x01	/* mount fs read-only */
#define	SUNM_NOSUID	0x02	/* mount fs with setuid disallowed */
#define	SUNM_NEWTYPE	0x04	/* type is string (char *), not int */
#define	SUNM_GRPID	0x08	/* (bsd semantics; ignored) */
#define	SUNM_REMOUNT	0x10	/* update existing mount */
#define	SUNM_NOSUB	0x20	/* prevent submounts (rejected) */
#define	SUNM_MULTI	0x40	/* (ignored) */
#define	SUNM_SYS5	0x80	/* Sys 5-specific semantics (rejected) */

struct	sun_nfs_args {
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

struct sun_mount_args {
	char	*type;
	char	*dir;
	int	flags;
	caddr_t	data;
};
sun_mount(p, uap, retval)
	struct proc *p;
	struct sun_mount_args *uap;
	register_t *retval;
{
	int oflags = uap->flags, nflags, error;
	extern char sigcode[], esigcode[];
	char fsname[MFSNAMELEN];

#define	szsigcode	(esigcode - sigcode)

	if (oflags & (SUNM_NOSUB | SUNM_SYS5))
		return (EINVAL);
	if ((oflags & SUNM_NEWTYPE) == 0)
		return (EINVAL);
	nflags = 0;
	if (oflags & SUNM_RDONLY)
		nflags |= MNT_RDONLY;
	if (oflags & SUNM_NOSUID)
		nflags |= MNT_NOSUID;
	if (oflags & SUNM_REMOUNT)
		nflags |= MNT_UPDATE;
	uap->flags = nflags;

	if (error = copyinstr((caddr_t)uap->type, fsname, sizeof fsname,
	    (size_t *)0))
		return (error);

	if (strcmp(fsname, "4.2") == 0) {
		uap->type = (caddr_t)ALIGN(PS_STRINGS - szsigcode - STACKGAPLEN);
		if (error = copyout("ufs", uap->type, sizeof("ufs")))
			return (error);
	} else if (strcmp(fsname, "nfs") == 0) {
		struct sun_nfs_args sna;
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

#if defined(NFSCLIENT)
async_daemon(p, uap, retval)
      struct proc *p;
      void *uap;
      register_t *retval;
{
      struct nfssvc_args {
              int flag;
              caddr_t argp;
      } args;

      args.flag = NFSSVC_BIOD;
      return nfssvc(p, &args, retval);
}
#endif /* NFSCLIENT */

struct sun_sigpending_args {
	int	*mask;
};
sun_sigpending(p, uap, retval)
	struct proc *p;
	struct sun_sigpending_args *uap;
	register_t *retval;
{
	int mask = p->p_siglist & p->p_sigmask;

	return (copyout((caddr_t)&mask, (caddr_t)uap->mask, sizeof(int)));
}

#if 0
/* XXX: Temporary until sys/dir.h, include/dirent.h and sys/dirent.h are fixed */
struct dirent {
	u_long	d_fileno;		/* file number of entry */
	u_short	d_reclen;		/* length of this record */
	u_short	d_namlen;		/* length of string in d_name */
	char	d_name[255 + 1];	/* name must be no longer than this */
};
#endif

/*
 * Here is the sun layout.  (Compare the BSD layout in <sys/dirent.h>.)
 * We can assume big-endian, so the BSD d_type field is just the high
 * byte of the SunOS d_namlen field, after adjusting for the extra "long".
 */
struct sun_dirent {
	long	d_off;
	u_long	d_fileno;
	u_short	d_reclen;
	u_short	d_namlen;
	char	d_name[256];
};

/*
 * Read Sun-style directory entries.  We suck them into kernel space so
 * that they can be massaged before being copied out to user code.  Like
 * SunOS, we squish out `empty' entries.
 *
 * This is quite ugly, but what do you expect from compatibility code?
 */
struct sun_getdents_args {
	int	fd;
	char	*buf;
	int	nbytes;
};
sun_getdents(p, uap, retval)
	struct proc *p;
	register struct sun_getdents_args *uap;
	register_t *retval;
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
#define	SUN_RECLEN(reclen) (reclen + sizeof(long))

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
	if (error = VOP_READDIR(vp, &auio, fp->f_cred, &eofflag, (u_long *)0,
	    0))
		goto out;
	inp = buf;
	outp = uap->buf;
	resid = uap->nbytes;
	if ((len = buflen - auio.uio_resid) == 0)
		goto eof;
	for (; len > 0; len -= reclen) {
		reclen = ((struct dirent *)inp)->d_reclen;
		if (reclen & 3)
			panic("sun_getdents");
		off += reclen;		/* each entry points to next */
		if (BSD_DIRENT(inp)->d_fileno == 0) {
			inp += reclen;	/* it is a hole; squish it out */
			continue;
		}
		if (reclen > len || resid < SUN_RECLEN(reclen)) {
			/* entry too big for buffer, so just stop */
			outp++;
			break;
		}
		/*
		 * Massage in place to make a Sun-shaped dirent (otherwise
		 * we have to worry about touching user memory outside of
		 * the copyout() call).
		 */
		BSD_DIRENT(inp)->d_reclen = SUN_RECLEN(reclen);
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
		outp += SUN_RECLEN(reclen);
		resid -= SUN_RECLEN(reclen);
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

#define	SUN__MAP_NEW	0x80000000	/* if not, old mmap & cannot handle */

struct sun_mmap_args {
	caddr_t	addr;
	size_t	len;
	int	prot;
	int	flags;
	int	fd;
	long	off;		/* not off_t! */
	off_t	qoff;		/* created here and fed to mmap() */
};
sun_mmap(p, uap, retval)
	register struct proc *p;
	register struct sun_mmap_args *uap;
	register_t *retval;
{
	register struct filedesc *fdp;
	register struct file *fp;
	register struct vnode *vp;

	/*
	 * Verify the arguments.
	 */
	if (uap->prot & ~(PROT_READ|PROT_WRITE|PROT_EXEC))
		return (EINVAL);			/* XXX still needed? */

	if ((uap->flags & SUN__MAP_NEW) == 0)
		return (EINVAL);
	uap->flags &= ~SUN__MAP_NEW;

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
	    iszerodev(vp->v_rdev)) {					/*XXX*/
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

struct sun_mctl_args {
	caddr_t	addr;
	size_t	len;
	int	func;
	void	*arg;
};
sun_mctl(p, uap, retval)
	register struct proc *p;
	register struct sun_mctl_args *uap;
	register_t *retval;
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

struct sun_setsockopt_args {
	int	s;
	int	level;
	int	name;
	caddr_t	val;
	int	valsize;
};
sun_setsockopt(p, uap, retval)
	struct proc *p;
	register struct sun_setsockopt_args *uap;
	register_t *retval;
{
	struct file *fp;
	struct mbuf *m = NULL;
	int error;

	if (error = getsock(p->p_fd, uap->s, &fp))
		return (error);
#define	SO_DONTLINGER (~SO_LINGER)
	if (uap->name == SO_DONTLINGER) {
		m = m_get(M_WAIT, MT_SOOPTS);
		if (m == NULL)
			return (ENOBUFS);
		mtod(m, struct linger *)->l_onoff = 0;
		m->m_len = sizeof(struct linger);
		return (sosetopt((struct socket *)fp->f_data, uap->level,
		    SO_LINGER, m));
	}
	if (uap->valsize > MLEN)
		return (EINVAL);
	if (uap->val) {
		m = m_get(M_WAIT, MT_SOOPTS);
		if (m == NULL)
			return (ENOBUFS);
		if (error = copyin(uap->val, mtod(m, caddr_t),
		    (u_int)uap->valsize)) {
			(void) m_free(m);
			return (error);
		}
		m->m_len = uap->valsize;
	}
	return (sosetopt((struct socket *)fp->f_data, uap->level,
	    uap->name, m));
}

struct sun_fchroot_args {
	int	fdes;
};
sun_fchroot(p, uap, retval)
	register struct proc *p;
	register struct sun_fchroot_args *uap;
	register_t *retval;
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
sun_auditsys()
{
	return 0;
}

struct sun_utsname {
	char    sysname[9];
	char    nodename[9];
	char    nodeext[65-9];
	char    release[9];
	char    version[9];
	char    machine[9];
};

struct sun_uname_args {
	struct sun_utsname *name;
};
sun_uname(p, uap, retval)
	struct proc *p;
	struct sun_uname_args *uap;
	register_t *retval;
{
	struct sun_utsname sut;
	extern char ostype[], machine[], osrelease[];

	bzero(&sut, sizeof(sut));

	bcopy(ostype, sut.sysname, sizeof(sut.sysname) - 1);
	bcopy(hostname, sut.nodename, sizeof(sut.nodename));
	sut.nodename[sizeof(sut.nodename)-1] = '\0';
	bcopy(osrelease, sut.release, sizeof(sut.release) - 1);
	bcopy("1", sut.version, sizeof(sut.version) - 1);
	bcopy(machine, sut.machine, sizeof(sut.machine) - 1);

	return copyout((caddr_t)&sut, (caddr_t)uap->name, sizeof(struct sun_utsname));
}

struct sun_setpgid_args {
	int	pid;	/* target process id */
	int	pgid;	/* target pgrp id */
};
int
sun_setpgid(p, uap, retval)
	struct proc *p;
	struct sun_setpgid_args *uap;
	register_t *retval;
{
	/*
	 * difference to our setpgid call is to include backwards
	 * compatibility to pre-setsid() binaries. Do setsid()
	 * instead of setpgid() in those cases where the process
	 * tries to create a new session the old way.
	 */
	if (!uap->pgid && (!uap->pid || uap->pid == p->p_pid))
		return setsid(p, uap, retval);
	else
		return setpgid(p, uap, retval);
}

struct sun_open_args {
	char	*fname;
	int	fmode;
	int	crtmode;
};
sun_open(p, uap, retval)
	struct proc *p;
	struct sun_open_args *uap;
	register_t *retval;
{
	int l, r;
	int noctty = uap->fmode & 0x8000;
	int ret;
	
	/* convert mode into NetBSD mode */
	l = uap->fmode;
	r =	(l & (0x0001 | 0x0002 | 0x0008 | 0x0040 | 0x0200 | 0x0400 | 0x0800));
	r |=	((l & (0x0004 | 0x1000 | 0x4000)) ? O_NONBLOCK : 0);
	r |=	((l & 0x0080) ? O_SHLOCK : 0);
	r |=	((l & 0x0100) ? O_EXLOCK : 0);
	r |=	((l & 0x2000) ? O_FSYNC : 0);

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

#if defined (NFSSERVER)
struct nfssvc_args {
	int	fd;
	caddr_t mskval;
	int msklen;
	caddr_t mtchval;
	int mtchlen;
};
struct sun_nfssvc_args {
	int	fd;
};
sun_nfssvc(p, uap, retval)
	struct proc *p;
	struct sun_nfssvc_args *uap;
	register_t *retval;
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

struct sun_ustat {
	daddr_t	f_tfree;	/* total free */
	ino_t	f_tinode;	/* total inodes free */
	char	f_fname[6];	/* filsys name */
	char	f_fpack[6];	/* filsys pack name */
};
struct sun_ustat_args {
	int	dev;
	struct	sun_ustat *buf;
};
sun_ustat(p, uap, retval)
	struct proc *p;
	struct sun_ustat_args *uap;
	register_t *retval;
{
	struct sun_ustat us;
	int error;

	bzero(&us, sizeof us);

	/*
	 * XXX: should set f_tfree and f_tinode at least
	 * How do we translate dev -> fstat? (and then to sun_ustat)
	 */

	if (error = copyout(&us, uap->buf, sizeof us))
		return (error);
	return 0;
}

struct sun_quotactl_args {
	int	cmd;
	char	*special;
	int	uid;
	caddr_t	addr;
};
sun_quotactl(p, uap, retval)
	struct proc *p;
	struct sun_quotactl_args *uap;
	register_t *retval;
{
	return EINVAL;
}

sun_vhangup(p, uap, retval)
	struct proc *p;
	void *uap;
	register_t *retval;
{
	return 0;
}

struct sun_statfs {
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
sunstatfs(sp, buf)
	struct statfs *sp;
	caddr_t buf;
{
	struct sun_statfs ssfs;

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

struct sun_statfs_args {
	char	*path;
	struct	sun_statfs *buf;
};
sun_statfs(p, uap, retval)
	struct proc *p;
	struct sun_statfs_args *uap;
	register_t *retval;
{
	register struct mount *mp;
	register struct statfs *sp;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, uap->path, p);
	if (error = namei(&nd))
		return (error);
	mp = nd.ni_vp->v_mount;
	sp = &mp->mnt_stat;
	vrele(nd.ni_vp);
	if (error = VFS_STATFS(mp, sp, p))
		return (error);
	sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
	return sunstatfs(sp, (caddr_t)uap->buf);
}

struct sun_fstatfs_args {
	int	fd;
	struct	sun_statfs *buf;
};
sun_fstatfs(p, uap, retval)
	struct proc *p;
	struct sun_fstatfs_args *uap;
	register_t *retval;
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
	return sunstatfs(sp, (caddr_t)uap->buf);
}

struct sun_exportfs_args {
	char	*path;
	char	*ex;			/* struct sun_export * */
};
sun_exportfs(p, uap, retval)
	struct proc *p;
	struct sun_exportfs_args *uap;
	register_t *retval;
{
	/*
	 * XXX: should perhaps translate into a mount(2)
	 * with MOUNT_EXPORT?
	 */
	return 0;
}

struct sun_mknod_args {
	char	*fname;
	int	fmode;
	int	dev;
};

sun_mknod(p, uap, retval)
	struct proc *p;
	struct sun_mknod_args *uap;
	register_t *retval;
{
	if (S_ISFIFO(uap->fmode))
		return mkfifo(p, uap, retval);

	return mknod(p, uap, retval);
}

#define SUN_SC_ARG_MAX		1
#define SUN_SC_CHILD_MAX	2
#define SUN_SC_CLK_TCK		3
#define SUN_SC_NGROUPS_MAX	4
#define SUN_SC_OPEN_MAX		5
#define SUN_SC_JOB_CONTROL	6
#define SUN_SC_SAVED_IDS	7
#define SUN_SC_VERSION		8

struct sun_sysconf_args {
	int	name;
};

sun_sysconf(p, uap, retval)
	struct proc *p;
	struct sun_sysconf_args *uap;
	register_t *retval;
{
	extern int maxfiles;

	switch(uap->name) {
	case SUN_SC_ARG_MAX:
		*retval = ARG_MAX;
		break;
	case SUN_SC_CHILD_MAX:
		*retval = maxproc;
		break;
	case SUN_SC_CLK_TCK:
		*retval = 60;		/* should this be `hz', ie. 100? */
		break;
	case SUN_SC_NGROUPS_MAX:
		*retval = NGROUPS_MAX;
		break;
	case SUN_SC_OPEN_MAX:
		*retval = maxfiles;
		break;
	case SUN_SC_JOB_CONTROL:
		*retval = 1;
		break;
	case SUN_SC_SAVED_IDS:
#ifdef _POSIX_SAVED_IDS
		*retval = 1;
#else
		*retval = 0;
#endif
		break;
	case SUN_SC_VERSION:
		*retval = 198808;
		break;
	default:
		return EINVAL;
	}
	return 0;
}
