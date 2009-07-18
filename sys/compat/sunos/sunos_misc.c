/*	$NetBSD: sunos_misc.c,v 1.159.4.2 2009/07/18 14:52:58 yamt Exp $	*/

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
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)sunos_misc.c	8.1 (Berkeley) 6/18/93
 *
 *	Header: sunos_misc.c,v 1.16 93/04/07 02:46:27 torek Exp
 */

/*
 * SunOS compatibility module.
 *
 * SunOS system calls that are implemented differently in BSD are
 * handled here.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunos_misc.c,v 1.159.4.2 2009/07/18 14:52:58 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/dirent.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/socket.h>
#include <sys/tty.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sys/unistd.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>
#include <sys/conf.h>
#include <sys/socketvar.h>
#include <sys/exec.h>
#include <sys/swap.h>
#include <sys/kauth.h>

#include <compat/sys/signal.h>

#include <compat/sunos/sunos.h>
#include <compat/sunos/sunos_syscallargs.h>
#include <compat/common/compat_util.h>
#include <compat/sunos/sunos_dirent.h>
#include <compat/sys/mount.h>

#include <netinet/in.h>

#include <miscfs/specfs/specdev.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsmount.h>

static int sunstatfs(struct statvfs *, void *);

int
sunos_sys_stime(struct lwp *l, const struct sunos_sys_stime_args *uap, register_t *retval)
{
	struct timeval tv;
	int error;

	error = copyin(SCARG(uap, tp), &tv.tv_sec, sizeof(tv.tv_sec));
	if (error)
		return error;
	tv.tv_usec = 0;

	return settimeofday1(&tv, false, NULL, l, true);
}

int
sunos_sys_wait4(struct lwp *l, const struct sunos_sys_wait4_args *uap, register_t *retval)
{
	struct compat_50_sys_wait4_args bsd_ua;

	SCARG(&bsd_ua, pid) = SCARG(uap, pid) == 0 ? WAIT_ANY : SCARG(uap, pid);
	SCARG(&bsd_ua, status) = SCARG(uap, status);
	SCARG(&bsd_ua, options) = SCARG(uap, options);
	SCARG(&bsd_ua, rusage) = SCARG(uap, rusage);

	return (compat_50_sys_wait4(l, &bsd_ua, retval));
}

int
sunos_sys_creat(struct lwp *l, const struct sunos_sys_creat_args *uap, register_t *retval)
{
	struct sys_open_args ouap;

	SCARG(&ouap, path) = SCARG(uap, path);
	SCARG(&ouap, flags) = O_WRONLY | O_CREAT | O_TRUNC;
	SCARG(&ouap, mode) = SCARG(uap, mode);

	return (sys_open(l, &ouap, retval));
}

int
sunos_sys_execv(struct lwp *l, const struct sunos_sys_execv_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(char **) argv;
	} */
	struct sys_execve_args ap;

	SCARG(&ap, path) = SCARG(uap, path);
	SCARG(&ap, argp) = SCARG(uap, argp);
	SCARG(&ap, envp) = NULL;

	return (sys_execve(l, &ap, retval));
}

int
sunos_sys_execve(struct lwp *l, const struct sunos_sys_execve_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(char **) argv;
		syscallarg(char **) envp;
	} */
	struct sys_execve_args ap;

	SCARG(&ap, path) = SCARG(uap, path);
	SCARG(&ap, argp) = SCARG(uap, argp);
	SCARG(&ap, envp) = SCARG(uap, envp);

	return (sys_execve(l, &ap, retval));
}

int
sunos_sys_omsync(struct lwp *l, const struct sunos_sys_omsync_args *uap, register_t *retval)
{
	struct sys___msync13_args ouap;

	SCARG(&ouap, addr) = SCARG(uap, addr);
	SCARG(&ouap, len) = SCARG(uap, len);
	SCARG(&ouap, flags) = SCARG(uap, flags);

	return (sys___msync13(l, &ouap, retval));
}

int
sunos_sys_unmount(struct lwp *l, const struct sunos_sys_unmount_args *uap, register_t *retval)
{
	struct sys_unmount_args ouap;

	SCARG(&ouap, path) = SCARG(uap, path);
	SCARG(&ouap, flags) = 0;

	return (sys_unmount(l, &ouap, retval));
}

/*
 * Conversion table for SunOS NFS mount flags.
 */
static struct {
	int	sun_flg;
	int	bsd_flg;
} sunnfs_flgtab[] = {
	{ SUNNFS_SOFT,		NFSMNT_SOFT },
	{ SUNNFS_WSIZE,		NFSMNT_WSIZE },
	{ SUNNFS_RSIZE,		NFSMNT_RSIZE },
	{ SUNNFS_TIMEO,		NFSMNT_TIMEO },
	{ SUNNFS_RETRANS,	NFSMNT_RETRANS },
	{ SUNNFS_HOSTNAME,	0 },			/* Ignored */
	{ SUNNFS_INT,		NFSMNT_INT },
	{ SUNNFS_NOAC,		0 },			/* Ignored */
	{ SUNNFS_ACREGMIN,	0 },			/* Ignored */
	{ SUNNFS_ACREGMAX,	0 },			/* Ignored */
	{ SUNNFS_ACDIRMIN,	0 },			/* Ignored */
	{ SUNNFS_ACDIRMAX,	0 },			/* Ignored */
	{ SUNNFS_SECURE,	0 },			/* Ignored */
	{ SUNNFS_NOCTO,		0 },			/* Ignored */
	{ SUNNFS_POSIX,		0 }			/* Ignored */
};

int
sunos_sys_mount(struct lwp *l, const struct sunos_sys_mount_args *uap, register_t *retval)
{
	int oflags = SCARG(uap, flags), nflags, error;
	char fsname[MFSNAMELEN];
	register_t dummy;

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

	error = copyinstr(SCARG(uap, type), fsname, sizeof fsname, NULL);
	if (error)
		return (error);

	if (strcmp(fsname, "nfs") == 0) {
		struct sunos_nfs_args sna;
		struct nfs_args na;
		int n;

		error = copyin(SCARG(uap, data), &sna, sizeof sna);
		if (error)
			return (error);
		/* sa.sa_len = sizeof(sain); */
		na.version = NFS_ARGSVERSION;
		na.addr = (void *)sna.addr;
		na.addrlen = sizeof(struct sockaddr);
		na.sotype = SOCK_DGRAM;
		na.proto = IPPROTO_UDP;
		na.fh = sna.fh;
		na.fhsize = NFSX_V2FH;
		na.flags = 0;
		n = sizeof(sunnfs_flgtab) / sizeof(sunnfs_flgtab[0]);
		while (--n >= 0)
			if (sna.flags & sunnfs_flgtab[n].sun_flg)
				na.flags |= sunnfs_flgtab[n].bsd_flg;
		na.wsize = sna.wsize;
		na.rsize = sna.rsize;
		if (na.flags & NFSMNT_RSIZE) {
			na.flags |= NFSMNT_READDIRSIZE;
			na.readdirsize = na.rsize;
		}
		na.timeo = sna.timeo;
		na.retrans = sna.retrans;
		na.hostname = /* (char *)(u_long) */ sna.hostname;

		return do_sys_mount(l, vfs_getopsbyname("nfs"), NULL,
		    SCARG(uap, dir), nflags, &na,
		    UIO_SYSSPACE, sizeof na, &dummy);
	}

	if (strcmp(fsname, "4.2") == 0)
		strcpy(fsname, "ffs");

	return do_sys_mount(l, vfs_getopsbyname(fsname), NULL,
	    SCARG(uap, dir), nflags, SCARG(uap, data),
	    UIO_USERSPACE, 0, &dummy);
}

int
async_daemon(struct lwp *l, const void *v, register_t *retval)
{

	return kpause("fakeniod", false, 0, NULL);
}

void	native_to_sunos_sigset(const sigset_t *, int *);
void	sunos_to_native_sigset(const int, sigset_t *);

inline void
native_to_sunos_sigset(const sigset_t *ss, int *mask)
{
	*mask = ss->__bits[0];
}

inline void
sunos_to_native_sigset(const int mask, sigset_t *ss)
{

	ss->__bits[0] = mask;
	ss->__bits[1] = 0;
	ss->__bits[2] = 0;
	ss->__bits[3] = 0;
}

int
sunos_sys_sigpending(struct lwp *l, const struct sunos_sys_sigpending_args *uap, register_t *retval)
{
	sigset_t ss;
	int mask;

	sigpending1(l, &ss);
	native_to_sunos_sigset(&ss, &mask);

	return (copyout((void *)&mask, (void *)SCARG(uap, mask), sizeof(int)));
}

int
sunos_sys_sigsuspend(struct lwp *l, const struct sunos_sys_sigsuspend_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) mask;
	} */
	int mask;
	sigset_t ss;

	mask = SCARG(uap, mask);
	sunos_to_native_sigset(mask, &ss);
	return (sigsuspend1(l, &ss));
}

/*
 * Read Sun-style directory entries.  We suck them into kernel space so
 * that they can be massaged before being copied out to user code.  Like
 * SunOS, we squish out `empty' entries.
 *
 * This is quite ugly, but what do you expect from compatibility code?
 */
int
sunos_sys_getdents(struct lwp *l, const struct sunos_sys_getdents_args *uap, register_t *retval)
{
	struct dirent *bdp;
	struct vnode *vp;
	char *inp, *buf;	/* BSD-format */
	int len, reclen;	/* BSD-format */
	char *outp;		/* Sun-format */
	int resid, sunos_reclen;/* Sun-format */
	struct file *fp;
	struct uio auio;
	struct iovec aiov;
	struct sunos_dirent idb;
	off_t off;			/* true file offset */
	int buflen, error, eofflag;
	off_t *cookiebuf, *cookie;
	int ncookies;

	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return (error);

	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto out1;
	}

	vp = fp->f_data;
	if (vp->v_type != VDIR) {
		error = EINVAL;
		goto out1;
	}

	buflen = min(MAXBSIZE, SCARG(uap, nbytes));
	buf = malloc(buflen, M_TEMP, M_WAITOK);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	off = fp->f_offset;
again:
	aiov.iov_base = buf;
	aiov.iov_len = buflen;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_READ;
	auio.uio_resid = buflen;
	auio.uio_offset = off;
	UIO_SETUP_SYSSPACE(&auio);
	/*
	 * First we read into the malloc'ed buffer, then
	 * we massage it into user space, one record at a time.
	 */
	error = VOP_READDIR(vp, &auio, fp->f_cred, &eofflag, &cookiebuf,
	    &ncookies);
	if (error)
		goto out;

	inp = buf;
	outp = SCARG(uap, buf);
	resid = SCARG(uap, nbytes);
	if ((len = buflen - auio.uio_resid) == 0)
		goto eof;

	for (cookie = cookiebuf; len > 0; len -= reclen) {
		bdp = (struct dirent *)inp;
		reclen = bdp->d_reclen;
		if (reclen & 3)
			panic("sunos_getdents");
		if ((*cookie >> 32) != 0) {
			compat_offseterr(vp, "sunos_getdents");
			error = EINVAL;
			goto out;
		}
		if (bdp->d_fileno == 0) {
			inp += reclen;	/* it is a hole; squish it out */
			if (cookie)
				off = *cookie++;
			else
				off += reclen;
			continue;
		}
		sunos_reclen = SUNOS_RECLEN(&idb, bdp->d_namlen);
		if (reclen > len || resid < sunos_reclen) {
			/* entry too big for buffer, so just stop */
			outp++;
			break;
		}
		if (cookie)
			off = *cookie++;	/* each entry points to next */
		else
			off += reclen;
		/*
		 * Massage in place to make a Sun-shaped dirent (otherwise
		 * we have to worry about touching user memory outside of
		 * the copyout() call).
		 */
		idb.d_fileno = bdp->d_fileno;
		idb.d_off = off;
		idb.d_reclen = sunos_reclen;
		idb.d_namlen = bdp->d_namlen;
		strlcpy(idb.d_name, bdp->d_name, sizeof(idb.d_name));
		if ((error = copyout((void *)&idb, outp, sunos_reclen)) != 0)
			goto out;
		/* advance past this real entry */
		inp += reclen;
		/* advance output past Sun-shaped entry */
		outp += sunos_reclen;
		resid -= sunos_reclen;
	}

	/* if we squished out the whole block, try again */
	if (outp == SCARG(uap, buf))
		goto again;
	fp->f_offset = off;		/* update the vnode offset */

eof:
	*retval = SCARG(uap, nbytes) - resid;
out:
	VOP_UNLOCK(vp, 0);
	free(cookiebuf, M_TEMP);
	free(buf, M_TEMP);
 out1:
 	fd_putfile(SCARG(uap, fd));
	return (error);
}

#define	SUNOS__MAP_NEW	0x80000000	/* if not, old mmap & cannot handle */

int
sunos_sys_mmap(struct lwp *l, const struct sunos_sys_mmap_args *uap, register_t *retval)
{
	struct sys_mmap_args ouap;

	/*
	 * Verify the arguments.
	 */
	if (SCARG(uap, prot) & ~(PROT_READ|PROT_WRITE|PROT_EXEC))
		return (EINVAL);			/* XXX still needed? */

	if ((SCARG(uap, flags) & SUNOS__MAP_NEW) == 0)
		return (EINVAL);

	SCARG(&ouap, flags) = SCARG(uap, flags) & ~SUNOS__MAP_NEW;
	SCARG(&ouap, addr) = SCARG(uap, addr);
	SCARG(&ouap, len) = SCARG(uap, len);
	SCARG(&ouap, prot) = SCARG(uap, prot);
	SCARG(&ouap, fd) = SCARG(uap, fd);
	SCARG(&ouap, pos) = SCARG(uap, pos);

	return (sys_mmap(l, &ouap, retval));
}

#define	MC_SYNC		1
#define	MC_LOCK		2
#define	MC_UNLOCK	3
#define	MC_ADVISE	4
#define	MC_LOCKAS	5
#define	MC_UNLOCKAS	6

int
sunos_sys_mctl(struct lwp *l, const struct sunos_sys_mctl_args *uap, register_t *retval)
{

	switch (SCARG(uap, func)) {
	case MC_ADVISE:		/* ignore for now */
		return (0);
	case MC_SYNC:		/* translate to msync */
		return (sys___msync13(l, (const void *)uap, retval));
	default:
		return (EINVAL);
	}
}

int
sunos_sys_setsockopt(struct lwp *l, const struct sunos_sys_setsockopt_args *uap, register_t *retval)
{
	struct sockopt sopt;
	struct socket *so;
	int name = SCARG(uap, name);
	int error;

	/* fd_getsock() will use the descriptor for us */
	if ((error = fd_getsock(SCARG(uap, s), &so)) != 0)
		return (error);
#define	SO_DONTLINGER (~SO_LINGER)
	if (name == SO_DONTLINGER) {
		struct linger lg;

		lg.l_onoff = 0;
		error = so_setsockopt(l, so, SCARG(uap, level), SO_LINGER,
		    &lg, sizeof(lg));
		goto out;
	}
	if (SCARG(uap, level) == IPPROTO_IP) {
#define		SUNOS_IP_MULTICAST_IF		2
#define		SUNOS_IP_MULTICAST_TTL		3
#define		SUNOS_IP_MULTICAST_LOOP		4
#define		SUNOS_IP_ADD_MEMBERSHIP		5
#define		SUNOS_IP_DROP_MEMBERSHIP	6
		static const int ipoptxlat[] = {
			IP_MULTICAST_IF,
			IP_MULTICAST_TTL,
			IP_MULTICAST_LOOP,
			IP_ADD_MEMBERSHIP,
			IP_DROP_MEMBERSHIP
		};
		if (name >= SUNOS_IP_MULTICAST_IF &&
		    name <= SUNOS_IP_DROP_MEMBERSHIP) {
			name = ipoptxlat[name - SUNOS_IP_MULTICAST_IF];
		}
	}
	if (SCARG(uap, valsize) > MLEN) {
		error = EINVAL;
		goto out;
	}
	sockopt_init(&sopt, SCARG(uap, level), name, SCARG(uap, valsize));
	if (SCARG(uap, val)) {
		error = copyin(SCARG(uap, val), sopt.sopt_data,
		    (u_int)SCARG(uap, valsize));
	}
	if (error == 0)
		error = sosetopt(so, &sopt);
	sockopt_destroy(&sopt);
 out:
 	fd_putfile(SCARG(uap, s));
	return (error);
}

static inline int sunos_sys_socket_common(struct lwp *, register_t *,
					      int type);
static inline int
sunos_sys_socket_common(struct lwp *l, register_t *retval, int type)
{
	struct socket *so;
	int error, fd;

	/* fd_getsock() will use the descriptor for us */
	fd = (int)*retval;
	if ((error = fd_getsock(fd, &so)) == 0) {
		if (type == SOCK_DGRAM)
			so->so_options |= SO_BROADCAST;
		fd_putfile(fd);
	}
	return (error);
}

int
sunos_sys_socket(struct lwp *l, const struct sunos_sys_socket_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
	} */
	int error;

	error = compat_30_sys_socket(l, (const void *)uap, retval);
	if (error)
		return (error);
	return sunos_sys_socket_common(l, retval, SCARG(uap, type));
}

int
sunos_sys_socketpair(struct lwp *l, const struct sunos_sys_socketpair_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
		syscallarg(int *) rsv;
	} */
	int error;

	error = sys_socketpair(l, (const void *)uap, retval);
	if (error)
		return (error);
	return sunos_sys_socket_common(l, retval, SCARG(uap, type));
}

/*
 * XXX: This needs cleaning up.
 */
int
sunos_sys_auditsys(struct lwp *l, const struct sunos_sys_auditsys_args *uap, register_t *retval)
{
	return 0;
}

int
sunos_sys_uname(struct lwp *l, const struct sunos_sys_uname_args *uap, register_t *retval)
{
	struct sunos_utsname sut;

	memset(&sut, 0, sizeof(sut));

	memcpy(sut.sysname, ostype, sizeof(sut.sysname) - 1);
	memcpy(sut.nodename, hostname, sizeof(sut.nodename));
	sut.nodename[sizeof(sut.nodename)-1] = '\0';
	memcpy(sut.release, osrelease, sizeof(sut.release) - 1);
	memcpy(sut.version, "1", sizeof(sut.version) - 1);
	memcpy(sut.machine, machine, sizeof(sut.machine) - 1);

	return copyout((void *)&sut, (void *)SCARG(uap, name),
	    sizeof(struct sunos_utsname));
}

int
sunos_sys_setpgrp(struct lwp *l, const struct sunos_sys_setpgrp_args *uap, register_t *retval)
{
	struct proc *p = l->l_proc;

	/*
	 * difference to our setpgid call is to include backwards
	 * compatibility to pre-setsid() binaries. Do setsid()
	 * instead of setpgid() in those cases where the process
	 * tries to create a new session the old way.
	 */
	if (!SCARG(uap, pgid) &&
	    (!SCARG(uap, pid) || SCARG(uap, pid) == p->p_pid))
		return sys_setsid(l, NULL, retval);
	else
		return sys_setpgid(l, (const void *)uap, retval);
}

int
sunos_sys_open(struct lwp *l, const struct sunos_sys_open_args *uap, register_t *retval)
{
	struct proc *p = l->l_proc;
	struct sys_open_args open_ua;
	int smode, nmode;
	int noctty;
	int ret;

	/* convert mode into NetBSD mode */
	smode = SCARG(uap, flags);
	noctty = smode & 0x8000;
	nmode =	smode &
		(0x0001 | 0x0002 | 0x0008 | 0x0040 | 0x0200 | 0x0400 | 0x0800);
	nmode |= ((smode & (0x0004 | 0x1000 | 0x4000)) ? O_NONBLOCK : 0);
	nmode |= ((smode & 0x0080) ? O_SHLOCK : 0);
	nmode |= ((smode & 0x0100) ? O_EXLOCK : 0);
	nmode |= ((smode & 0x2000) ? O_FSYNC : 0);

	SCARG(&open_ua, path) = SCARG(uap, path);
	SCARG(&open_ua, flags) = nmode;
	SCARG(&open_ua, mode) = SCARG(uap, mode);
	ret = sys_open(l, &open_ua, retval);

	/* XXXSMP */
	if (!ret && !noctty && SESS_LEADER(p) && !(p->p_lflag & PL_CONTROLT)) {
		file_t *fp;
		int fd;

		fd = (int)*retval;
		fp = fd_getfile(fd);

		/* ignore any error, just give it a try */
		if (fp != NULL) {
			if (fp->f_type == DTYPE_VNODE)
				(fp->f_ops->fo_ioctl)(fp, TIOCSCTTY, NULL);
			fd_putfile(fd);
		}
	}
	return ret;
}

int
sunos_sys_nfssvc(struct lwp *l, const struct sunos_sys_nfssvc_args *uap, register_t *retval)
{
#if 0
	struct proc *p = l->l_proc;
	struct emul *e = p->p_emul;
	struct sys_nfssvc_args outuap;
	struct sockaddr sa;
	int error;
	void *sg = stackgap_init(p, 0);

	memset(&outuap, 0, sizeof outuap);
	SCARG(&outuap, fd) = SCARG(uap, fd);
	SCARG(&outuap, mskval) = stackgap_alloc(p, &sg, sizeof(sa));
	SCARG(&outuap, msklen) = sizeof(sa);
	SCARG(&outuap, mtchval) = stackgap_alloc(p, &sg, sizeof(sa));
	SCARG(&outuap, mtchlen) = sizeof(sa);

	memset(&sa, 0, sizeof sa);
	if (error = copyout(&sa, SCARG(&outuap, mskval), SCARG(&outuap, msklen)))
		return (error);
	if (error = copyout(&sa, SCARG(&outuap, mtchval), SCARG(&outuap, mtchlen)))
		return (error);

	return nfssvc(l, &outuap, retval);
#else
	return (ENOSYS);
#endif
}

int
sunos_sys_ustat(struct lwp *l, const struct sunos_sys_ustat_args *uap, register_t *retval)
{
	struct sunos_ustat us;
	int error;

	memset(&us, 0, sizeof us);

	/*
	 * XXX: should set f_tfree and f_tinode at least
	 * How do we translate dev -> fstat? (and then to sunos_ustat)
	 */

	if ((error = copyout(&us, SCARG(uap, buf), sizeof us)) != 0)
		return (error);
	return 0;
}

int
sunos_sys_quotactl(struct lwp *l, const struct sunos_sys_quotactl_args *uap, register_t *retval)
{

	return EINVAL;
}

int
sunos_sys_vhangup(struct lwp *l, const void *v, register_t *retval)
{
	struct proc *p = l->l_proc;
	struct session *sp = p->p_session;

	if (sp->s_ttyvp == 0)
		return 0;

	if (sp->s_ttyp && sp->s_ttyp->t_session == sp && sp->s_ttyp->t_pgrp)
		pgsignal(sp->s_ttyp->t_pgrp, SIGHUP, 1);

	(void) ttywait(sp->s_ttyp);
	if (sp->s_ttyvp)
		VOP_REVOKE(sp->s_ttyvp, REVOKEALL);
	if (sp->s_ttyvp)
		vrele(sp->s_ttyvp);
	sp->s_ttyvp = NULL;

	return 0;
}

static int
sunstatfs(struct statvfs *sp, void *buf)
{
	struct sunos_statfs ssfs;

	memset(&ssfs, 0, sizeof ssfs);
	ssfs.f_type = 0;
	ssfs.f_bsize = sp->f_bsize;
	ssfs.f_blocks = sp->f_blocks;
	ssfs.f_bfree = sp->f_bfree;
	ssfs.f_bavail = sp->f_bavail;
	ssfs.f_files = sp->f_files;
	ssfs.f_ffree = sp->f_ffree;
	ssfs.f_fsid = sp->f_fsidx;
	return copyout((void *)&ssfs, buf, sizeof ssfs);
}

int
sunos_sys_statfs(struct lwp *l, const struct sunos_sys_statfs_args *uap, register_t *retval)
{
	struct mount *mp;
	struct statvfs *sp;
	int error;
	struct vnode *vp;

	error = namei_simple_user(SCARG(uap, path),
				NSM_FOLLOW_TRYEMULROOT, &vp);
	if (error != 0)
		return (error);
	mp = vp->v_mount;
	sp = &mp->mnt_stat;
	vrele(vp);
	if ((error = VFS_STATVFS(mp, sp)) != 0)
		return (error);
	sp->f_flag = mp->mnt_flag & MNT_VISFLAGMASK;
	return sunstatfs(sp, (void *)SCARG(uap, buf));
}

int
sunos_sys_fstatfs(struct lwp *l, const struct sunos_sys_fstatfs_args *uap, register_t *retval)
{
	file_t *fp;
	struct mount *mp;
	struct statvfs *sp;
	int error;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return (error);
	mp = ((struct vnode *)fp->f_data)->v_mount;
	sp = &mp->mnt_stat;
	if ((error = VFS_STATVFS(mp, sp)) != 0)
		goto out;
	sp->f_flag = mp->mnt_flag & MNT_VISFLAGMASK;
	error = sunstatfs(sp, (void *)SCARG(uap, buf));
 out:
 	fd_putfile(SCARG(uap, fd));
	return (error);
}

int
sunos_sys_exportfs(struct lwp *l, const struct sunos_sys_exportfs_args *uap, register_t *retval)
{
	/*
	 * XXX: should perhaps translate into a mount(2)
	 * with MOUNT_EXPORT?
	 */
	return 0;
}

int
sunos_sys_mknod(struct lwp *l, const struct sunos_sys_mknod_args *uap, register_t *retval)
{
	struct sys_mkfifo_args fifo_ua;

	if (S_ISFIFO(SCARG(uap, mode))) {
		SCARG(&fifo_ua, path) = SCARG(uap, path);
		SCARG(&fifo_ua, mode) = SCARG(uap, mode);
		return sys_mkfifo(l, &fifo_ua, retval);
	}

	return compat_50_sys_mknod(l,
	    (const struct compat_50_sys_mknod_args *)uap, retval);
}

#define SUNOS_SC_ARG_MAX	1
#define SUNOS_SC_CHILD_MAX	2
#define SUNOS_SC_CLK_TCK	3
#define SUNOS_SC_NGROUPS_MAX	4
#define SUNOS_SC_OPEN_MAX	5
#define SUNOS_SC_JOB_CONTROL	6
#define SUNOS_SC_SAVED_IDS	7
#define SUNOS_SC_VERSION	8

int
sunos_sys_sysconf(struct lwp *l, const struct sunos_sys_sysconf_args *uap, register_t *retval)
{

	switch(SCARG(uap, name)) {
	case SUNOS_SC_ARG_MAX:
		*retval = ARG_MAX;
		break;
	case SUNOS_SC_CHILD_MAX:
		*retval = maxproc;
		break;
	case SUNOS_SC_CLK_TCK:
		*retval = 60;		/* should this be `hz', ie. 100? */
		break;
	case SUNOS_SC_NGROUPS_MAX:
		*retval = NGROUPS_MAX;
		break;
	case SUNOS_SC_OPEN_MAX:
		*retval = maxfiles;
		break;
	case SUNOS_SC_JOB_CONTROL:
		*retval = 1;
		break;
	case SUNOS_SC_SAVED_IDS:
#ifdef _POSIX_SAVED_IDS
		*retval = 1;
#else
		*retval = 0;
#endif
		break;
	case SUNOS_SC_VERSION:
		*retval = 198808;
		break;
	default:
		return EINVAL;
	}
	return 0;
}

#define SUNOS_RLIMIT_NOFILE	6	/* Other RLIMIT_* are the same */
#define SUNOS_RLIM_NLIMITS	7

int
sunos_sys_getrlimit(struct lwp *l, const struct sunos_sys_getrlimit_args *uap, register_t *retval)
{
	struct compat_43_sys_getrlimit_args ua_43;

	SCARG(&ua_43, which) = SCARG(uap, which);
	SCARG(&ua_43, rlp) = SCARG(uap, rlp);

	if (SCARG(uap, which) >= SUNOS_RLIM_NLIMITS)
		return EINVAL;

	if (SCARG(uap, which) == SUNOS_RLIMIT_NOFILE)
		SCARG(&ua_43, which) = RLIMIT_NOFILE;

	return compat_43_sys_getrlimit(l, &ua_43, retval);
}

int
sunos_sys_setrlimit(struct lwp *l, const struct sunos_sys_setrlimit_args *uap, register_t *retval)
{
	struct compat_43_sys_setrlimit_args ua_43;

	SCARG(&ua_43, which) = SCARG(uap, which);
	SCARG(&ua_43, rlp) = SCARG(uap, rlp);

	if (SCARG(uap, which) >= SUNOS_RLIM_NLIMITS)
		return EINVAL;

	if (SCARG(uap, which) == SUNOS_RLIMIT_NOFILE)
		SCARG(&ua_43, which) = RLIMIT_NOFILE;

	return compat_43_sys_setrlimit(l, &ua_43, retval);
}

/* for the m68k machines */
#ifndef PT_GETFPREGS
#define PT_GETFPREGS -1
#endif
#ifndef PT_SETFPREGS
#define PT_SETFPREGS -1
#endif

static const int sreq2breq[] = {
	PT_TRACE_ME,    PT_READ_I,      PT_READ_D,      -1,
	PT_WRITE_I,     PT_WRITE_D,     -1,             PT_CONTINUE,
	PT_KILL,        -1,             PT_ATTACH,      PT_DETACH,
	PT_GETREGS,     PT_SETREGS,     PT_GETFPREGS,   PT_SETFPREGS
};
static const int nreqs = sizeof(sreq2breq) / sizeof(sreq2breq[0]);

int
sunos_sys_ptrace(struct lwp *l, const struct sunos_sys_ptrace_args *uap, register_t *retval)
{
	struct sys_ptrace_args pa;
	int req;

#define	sys_ptrace sysent[SYS_ptrace].sy_call 
	if (sys_ptrace == sys_nosys)
		return ENOSYS;

	req = SCARG(uap, req);

	if (req < 0 || req >= nreqs)
		return (EINVAL);

	req = sreq2breq[req];
	if (req == -1)
		return (EINVAL);

	SCARG(&pa, req) = req;
	SCARG(&pa, pid) = (pid_t)SCARG(uap, pid);
	SCARG(&pa, addr) = (void *)SCARG(uap, addr);
	SCARG(&pa, data) = SCARG(uap, data);

	return sys_ptrace(l, &pa, retval);
}

/*
 * SunOS reboot system call (for compatibility).
 * Sun lets you pass in a boot string which the PROM
 * saves and provides to the next boot program.
 */

#define SUNOS_RB_ASKNAME	0x001
#define SUNOS_RB_SINGLE 	0x002
#define SUNOS_RB_NOSYNC		0x004
#define SUNOS_RB_HALT		0x008
#define SUNOS_RB_DUMP		0x080
#define	SUNOS_RB_STRING		0x200

static struct sunos_howto_conv {
	int sun_howto;
	int bsd_howto;
} sunos_howto_conv[] = {
	{ SUNOS_RB_ASKNAME,	RB_ASKNAME },
	{ SUNOS_RB_SINGLE,	RB_SINGLE },
	{ SUNOS_RB_NOSYNC,	RB_NOSYNC },
	{ SUNOS_RB_HALT,	RB_HALT },
	{ SUNOS_RB_DUMP,	RB_DUMP },
	{ SUNOS_RB_STRING,	RB_STRING },
	{ 0x000,		0 },
};

int
sunos_sys_reboot(struct lwp *l, const struct sunos_sys_reboot_args *uap, register_t *retval)
{
	struct sys_reboot_args ua;
	struct sunos_howto_conv *convp;
	int error, bsd_howto, sun_howto;
	char *bootstr;

	if ((error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_REBOOT,
	    0, NULL, NULL, NULL)) != 0)
		return (error);

	/*
	 * Convert howto bits to BSD format.
	 */
	sun_howto = SCARG(uap, howto);
	bsd_howto = 0;
	convp = sunos_howto_conv;
	while (convp->sun_howto) {
		if (sun_howto & convp->sun_howto)
			bsd_howto |= convp->bsd_howto;
		convp++;
	}

	/*
	 * Sun RB_STRING (Get user supplied bootstring.)
	 * If the machine supports passing a string to the
	 * next booted kernel.
	 */
	if (sun_howto & SUNOS_RB_STRING) {
		char bs[128];

		error = copyinstr(SCARG(uap, bootstr), bs, sizeof(bs), 0);

		if (error)
			bootstr = NULL;
		else
			bootstr = bs;
	} else
		bootstr = NULL;

	SCARG(&ua, opt) = bsd_howto;
	SCARG(&ua, bootstr) = bootstr;
	sys_reboot(l, &ua, retval);
	return(0);
}

/*
 * Generalized interface signal handler, 4.3-compatible.
 */
/* ARGSUSED */
int
sunos_sys_sigvec(struct lwp *l, const struct sunos_sys_sigvec_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) signum;
		syscallarg(struct sigvec *) nsv;
		syscallarg(struct sigvec *) osv;
	} */
	struct sigvec nsv, osv;
	struct sigaction nsa, osa;
	int error;
/*XXX*/extern	void compat_43_sigvec_to_sigaction
(const struct sigvec *, struct sigaction *);
/*XXX*/extern	void compat_43_sigaction_to_sigvec
(const struct sigaction *, struct sigvec *);

	if (SCARG(uap, nsv)) {
		error = copyin(SCARG(uap, nsv), &nsv, sizeof(nsv));
		if (error != 0)
			return (error);

		/*
		 * SunOS uses the mask 0x0004 as SV_RESETHAND
		 * meaning: `reset to SIG_DFL on delivery'.
		 * We support only the bits in: 0xF
		 * (those bits are the same as ours)
		 */
		if (nsv.sv_flags & ~0xF)
			return (EINVAL);

		compat_43_sigvec_to_sigaction(&nsv, &nsa);
	}
	error = sigaction1(l, SCARG(uap, signum),
			   SCARG(uap, nsv) ? &nsa : 0,
			   SCARG(uap, osv) ? &osa : 0,
			   NULL, 0);
	if (error != 0)
		return (error);

	if (SCARG(uap, osv)) {
		compat_43_sigaction_to_sigvec(&osa, &osv);
		error = copyout(&osv, SCARG(uap, osv), sizeof(osv));
		if (error != 0)
			return (error);
	}

	return (0);
}
