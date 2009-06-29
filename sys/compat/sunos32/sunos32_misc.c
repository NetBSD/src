/*	$NetBSD: sunos32_misc.c,v 1.67 2009/06/29 05:08:16 dholland Exp $	*/
/* from :NetBSD: sunos_misc.c,v 1.107 2000/12/01 19:25:10 jdolecek Exp	*/

/*
 * Copyright (c) 2001 Matthew R. Green
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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
 * SunOS compatibility module, 64-bit kernel version
 *
 * SunOS system calls that are implemented differently in BSD are
 * handled here.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunos32_misc.c,v 1.67 2009/06/29 05:08:16 dholland Exp $");

#define COMPAT_SUNOS 1

#if defined(_KERNEL_OPT)
#include "opt_compat_43.h"
#include "opt_compat_netbsd.h"
#include "fs_nfs.h"
#endif

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
#include <sys/vfs_syscalls.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/sunos32/sunos32.h>
#include <compat/sunos32/sunos32_dirent.h>
#include <compat/sunos32/sunos32_syscallargs.h>
#include <compat/common/compat_util.h>
#include <compat/sys/mount.h>

#include <netinet/in.h>

#include <miscfs/specfs/specdev.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsmount.h>

static void sunos32_sigvec_to_sigaction(const struct netbsd32_sigvec *, struct sigaction *);
static void sunos32_sigvec_from_sigaction(struct netbsd32_sigvec *, const struct sigaction *);

static int sunstatfs(struct statvfs *, void *);

static void
sunos32_sigvec_to_sigaction(const struct netbsd32_sigvec *sv, struct sigaction *sa)
{
/*XXX*/ extern void compat_43_sigmask_to_sigset(const int *, sigset_t *);

	sa->sa_handler = NETBSD32PTR64(sv->sv_handler);
	compat_43_sigmask_to_sigset(&sv->sv_mask, &sa->sa_mask);
	sa->sa_flags = sv->sv_flags ^ SA_RESTART;
}

static
void sunos32_sigvec_from_sigaction(sv, sa)
	struct netbsd32_sigvec *sv;
	const struct sigaction *sa;
{
/*XXX*/ extern void compat_43_sigset_to_sigmask(const sigset_t *, int *);

	NETBSD32PTR32(sv->sv_handler, sa->sa_handler);
	compat_43_sigset_to_sigmask(&sa->sa_mask, &sv->sv_mask);
	sv->sv_flags = sa->sa_flags ^ SA_RESTART;
}

int
sunos32_sys_stime(struct lwp *l, const struct sunos32_sys_stime_args *uap, register_t *retval)
{
	/* {
		syscallarg(sunos32_time_tp) tp;
	} */
	struct netbsd32_timeval ntv;
	struct timeval tv;
	int error;

	error = copyin(SCARG_P32(uap, tp), &ntv.tv_sec, sizeof(ntv.tv_sec));
	if (error)
		return error;
	tv.tv_sec = ntv.tv_sec;
	tv.tv_usec = 0;

	return settimeofday1(&tv, false, NULL, l, true);
}

int
sunos32_sys_wait4(struct lwp *l, const struct sunos32_sys_wait4_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) pid;
		syscallarg(netbsd32_intp) status;
		syscallarg(int) options;
		syscallarg(netbsd32_rusagep_t) rusage;
	} */

	struct compat_50_netbsd32_wait4_args bsd_ua;

	SCARG(&bsd_ua, pid) = SCARG(uap, pid) == 0 ? WAIT_ANY : SCARG(uap, pid);
	SCARG(&bsd_ua, status) = SCARG(uap, status);
	SCARG(&bsd_ua, options) = SCARG(uap, options);
	SCARG(&bsd_ua, rusage) = SCARG(uap, rusage);

	return compat_50_netbsd32_wait4(l, &bsd_ua, retval);
}

int
sunos32_sys_creat(struct lwp *l, const struct sunos32_sys_creat_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) mode;
	} */
	struct sys_open_args ua;

	SUNOS32TOP_UAP(path, const char);
	SCARG(&ua, flags) = O_WRONLY | O_CREAT | O_TRUNC;
	SUNOS32TO64_UAP(mode);

	return (sys_open(l, &ua, retval));
}

int
sunos32_sys_access(struct lwp *l, const struct sunos32_sys_access_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) flags;
	} */
	struct sys_access_args ua;

	SUNOS32TOP_UAP(path, const char);
	SUNOS32TO64_UAP(flags);

	return (sys_access(l, &ua, retval));
}

static inline void sunos32_from___stat13(struct stat *, struct netbsd32_stat43 *);

static inline void
sunos32_from___stat13(struct stat *sbp, struct netbsd32_stat43 *sb32p)
{
	sb32p->st_dev = sbp->st_dev;
	sb32p->st_ino = sbp->st_ino;
	sb32p->st_mode = sbp->st_mode;
	sb32p->st_nlink = sbp->st_nlink;
	sb32p->st_uid = sbp->st_uid;
	sb32p->st_gid = sbp->st_gid;
	sb32p->st_rdev = sbp->st_rdev;
	if (sbp->st_size < (quad_t)1 << 32)
		sb32p->st_size = sbp->st_size;
	else
		sb32p->st_size = -2;
	sb32p->st_atimespec.tv_sec = (netbsd32_time_t)sbp->st_atimespec.tv_sec;
	sb32p->st_atimespec.tv_nsec = (netbsd32_long)sbp->st_atimespec.tv_nsec;
	sb32p->st_mtimespec.tv_sec = (netbsd32_time_t)sbp->st_mtimespec.tv_sec;
	sb32p->st_mtimespec.tv_nsec = (netbsd32_long)sbp->st_mtimespec.tv_nsec;
	sb32p->st_ctimespec.tv_sec = (netbsd32_time_t)sbp->st_ctimespec.tv_sec;
	sb32p->st_ctimespec.tv_nsec = (netbsd32_long)sbp->st_ctimespec.tv_nsec;
	sb32p->st_blksize = sbp->st_blksize;
	sb32p->st_blocks = sbp->st_blocks;
	sb32p->st_flags = sbp->st_flags;
	sb32p->st_gen = sbp->st_gen;
}


int
sunos32_sys_stat(struct lwp *l, const struct sunos32_sys_stat_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_stat43p_t) ub;
	} */
	struct netbsd32_stat43 sb32;
	struct stat sb;
	const char *path;
	int error;

	path = SCARG_P32(uap, path);

	error = do_sys_stat(path, FOLLOW, &sb);
	if (error)
		return (error);
	sunos32_from___stat13(&sb, &sb32);
	error = copyout(&sb32, SCARG_P32(uap, ub), sizeof (sb32));
	return (error);
}

int
sunos32_sys_lstat(struct lwp *l, const struct sunos32_sys_lstat_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_stat43p_t) ub;
	} */
	struct vnode *vp, *dvp;
	struct stat sb, sb1;
	struct netbsd32_stat43 sb32;
	int error;
	struct nameidata nd;
	int ndflags;
	const char *path;

	path = SCARG_P32(uap, path);

	ndflags = NOFOLLOW | LOCKLEAF | LOCKPARENT | TRYEMULROOT;
again:
	NDINIT(&nd, LOOKUP, ndflags, UIO_USERSPACE, path);
	if ((error = namei(&nd))) {
		if (error == EISDIR && (ndflags & LOCKPARENT) != 0) {
			/*
			 * Should only happen on '/'. Retry without LOCKPARENT;
			 * this is safe since the vnode won't be a VLNK.
			 */
			ndflags &= ~LOCKPARENT;
			goto again;
		}
		return (error);
	}
	/*
	 * For symbolic links, always return the attributes of its
	 * containing directory, except for mode, size, and links.
	 */
	vp = nd.ni_vp;
	dvp = nd.ni_dvp;
	if (vp->v_type != VLNK) {
		if ((ndflags & LOCKPARENT) != 0) {
			if (dvp == vp)
				vrele(dvp);
			else
				vput(dvp);
		}
		error = vn_stat(vp, &sb);
		vput(vp);
		if (error)
			return (error);
	} else {
		error = vn_stat(dvp, &sb);
		vput(dvp);
		if (error) {
			vput(vp);
			return (error);
		}
		error = vn_stat(vp, &sb1);
		vput(vp);
		if (error)
			return (error);
		sb.st_mode &= ~S_IFDIR;
		sb.st_mode |= S_IFLNK;
		sb.st_nlink = sb1.st_nlink;
		sb.st_size = sb1.st_size;
		sb.st_blocks = sb1.st_blocks;
	}
	sunos32_from___stat13(&sb, &sb32);
	error = copyout((void *)&sb32, SCARG_P32(uap, ub), sizeof (sb32));
	return (error);
}

static int
sunos32_execve_fetch_element(char * const *array, size_t index, char **value)
{
	int error;
	netbsd32_charp const *a32 = (void const *)array;
	netbsd32_charp e;

	error = copyin(a32 + index, &e, sizeof(e));
	if (error)
		return error;
	*value = NETBSD32PTR64(e);
	return 0;
}

int
sunos32_sys_execv(struct lwp *l, const struct sunos32_sys_execv_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(char **) argv;
	} */
	const char *path = SCARG_P32(uap, path);

	return execve1(l, path, SCARG_P32(uap, argp), NULL,
	    sunos32_execve_fetch_element);
}

int
sunos32_sys_execve(struct lwp *l, const struct sunos32_sys_execve_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(char **) argv;
		syscallarg(char **) envp;
	} */
	const char *path = SCARG_P32(uap, path);

	return execve1(l, path, SCARG_P32(uap, argp),
	    SCARG_P32(uap, envp),
	    sunos32_execve_fetch_element);
}

int
sunos32_sys_omsync(struct lwp *l, const struct sunos32_sys_omsync_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_caddr_t) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) flags;
	} */
	struct netbsd32___msync13_args ouap;

	SCARG(&ouap, addr) = SCARG(uap, addr);
	SCARG(&ouap, len) = SCARG(uap, len);
	SCARG(&ouap, flags) = SCARG(uap, flags);

	return (netbsd32___msync13(l, &ouap, retval));
}

int
sunos32_sys_unmount(struct lwp *l, const struct sunos32_sys_unmount_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_charp) path;
	} */
	struct sys_unmount_args ua;

	SUNOS32TOP_UAP(path, const char);
	SCARG(&ua, flags) = 0;

	return (sys_unmount(l, &ua, retval));
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
sunos32_sys_mount(struct lwp *l, const struct sunos32_sys_mount_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_charp) type;
		syscallarg(netbsd32_charp) path;
		syscallarg(int) flags;
		syscallarg(netbsd32_caddr_t) data;
	} */
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

	error = copyinstr(SCARG_P32(uap, type), fsname, sizeof fsname, NULL);
	if (error)
		return (error);

	if (strncmp(fsname, "nfs", sizeof fsname) == 0) {
		struct sunos_nfs_args sna;
		struct nfs_args na;	/* XXX */
		int n;

		error = copyin(SCARG_P32(uap, data), &sna, sizeof sna);
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
		na.hostname = (char *)(u_long)sna.hostname;

		return do_sys_mount(l, vfs_getopsbyname("nfs"), NULL,
		    SCARG_P32(uap, path), nflags, &na, UIO_SYSSPACE, sizeof na,
		    &dummy);
	}

	if (strcmp(fsname, "4.2") == 0)
		strcpy(fsname, "ffs");

	return do_sys_mount(l, vfs_getopsbyname(fsname), NULL,
	    SCARG_P32(uap, path), nflags, SCARG_P32(uap, data), UIO_USERSPACE,
	    0, &dummy);
}

#if defined(NFS)
int
async_daemon(struct lwp *l, const void *v, register_t *retval)
{
	struct netbsd32_nfssvc_args ouap;

	SCARG(&ouap, flag) = NFSSVC_BIOD;
	NETBSD32PTR32(SCARG(&ouap, argp), 0);

	return (netbsd32_nfssvc(l, &ouap, retval));
}
#endif /* NFS */

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
sunos32_sys_sigpending(struct lwp *l, const struct sunos32_sys_sigpending_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_intp) mask;
	} */
	sigset_t ss;
	int mask;

	sigpending1(l, &ss);
	native_to_sunos_sigset(&ss, &mask);

	return (copyout((void *)(u_long)&mask, SCARG_P32(uap, mask), sizeof(int)));
}

int
sunos32_sys_sigsuspend(struct lwp *l, const struct sunos32_sys_sigsuspend_args *uap, register_t *retval)
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
sunos32_sys_getdents(struct lwp *l, const struct sunos32_sys_getdents_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) buf;
		syscallarg(int) nbytes;
	} */
	struct dirent *bdp;
	struct vnode *vp;
	char *inp, *sbuf;	/* BSD-format */
	int len, reclen;	/* BSD-format */
	char *outp;		/* Sun-format */
	int resid, sunos_reclen;/* Sun-format */
	struct file *fp;
	struct uio auio;
	struct iovec aiov;
	struct sunos32_dirent idb;
	off_t off;			/* true file offset */
	int buflen, error, eofflag;
	off_t *cookiebuf, *cookie;
	int ncookies;

	/* fd_getvnode() will use the descriptor for us */
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
	sbuf = malloc(buflen, M_TEMP, M_WAITOK);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	off = fp->f_offset;
again:
	aiov.iov_base = sbuf;
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

	inp = sbuf;
	outp = SCARG_P32(uap, buf);
	resid = SCARG(uap, nbytes);
	if ((len = buflen - auio.uio_resid) == 0)
		goto eof;

	for (cookie = cookiebuf; len > 0; len -= reclen) {
		bdp = (struct dirent *)inp;
		reclen = bdp->d_reclen;
		if (reclen & 3)
			panic("sunos_getdents");
		if (cookie && (*cookie >> 32) != 0) {
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
		sunos_reclen = SUNOS32_RECLEN(&idb, bdp->d_namlen);
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
	if (outp == SCARG_P32(uap, buf))
		goto again;
	fp->f_offset = off;		/* update the vnode offset */

eof:
	*retval = SCARG(uap, nbytes) - resid;
out:
	VOP_UNLOCK(vp, 0);
	free(cookiebuf, M_TEMP);
	free(sbuf, M_TEMP);
 out1:
 	fd_putfile(SCARG(uap, fd));
	return (error);
}

#define	SUNOS32__MAP_NEW	0x80000000	/* if not, old mmap & cannot handle */

int
sunos32_sys_mmap(struct lwp *l, const struct sunos32_sys_mmap_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) prot;
		syscallarg(int) flags;
		syscallarg(int) fd;
		syscallarg(netbsd32_long) pos;
	} */
	struct sys_mmap_args ua;
	int error;

	/*
	 * Verify the arguments.
	 */
	if (SCARG(uap, prot) & ~(PROT_READ|PROT_WRITE|PROT_EXEC))
		return (EINVAL);			/* XXX still needed? */

	if ((SCARG(uap, flags) & SUNOS32__MAP_NEW) == 0)
		return (EINVAL);

	SUNOS32TOP_UAP(addr, void);
	SUNOS32TOX_UAP(len, size_t);
	SUNOS32TO64_UAP(prot);
	SCARG(&ua, flags) = SCARG(uap, flags) & ~SUNOS32__MAP_NEW;
	SUNOS32TO64_UAP(fd);
	SCARG(&ua, PAD) = 0;
	SUNOS32TOX_UAP(pos, off_t);

	error = sys_mmap(l, &ua, retval);
	if ((u_long)*retval > (u_long)UINT_MAX) {
		printf("sunos32_mmap: retval out of range: 0x%lx",
		       (u_long)*retval);
		/* Should try to recover and return an error here. */
	}
	return (error);
}

#define	MC_SYNC		1
#define	MC_LOCK		2
#define	MC_UNLOCK	3
#define	MC_ADVISE	4
#define	MC_LOCKAS	5
#define	MC_UNLOCKAS	6

int
sunos32_sys_mctl(struct lwp *l, const struct sunos32_sys_mctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(int) len;
		syscallarg(int) func;
		syscallarg(netbsd32_voidp) arg;
	} */

	switch (SCARG(uap, func)) {
	case MC_ADVISE:		/* ignore for now */
		return (0);
	case MC_SYNC:		/* translate to msync */
		return (netbsd32___msync13(l, (const void *)uap, retval));
	default:
		return (EINVAL);
	}
}

int
sunos32_sys_setsockopt(struct lwp *l, const struct sunos32_sys_setsockopt_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(int) level;
		syscallarg(int) name;
		syscallarg(netbsd32_caddr_t) val;
		syscallarg(int) valsize;
	} */
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
	if (SCARG_P32(uap, val)) {
		error = copyin(SCARG_P32(uap, val), sopt.sopt_data,
		    (u_int)SCARG(uap, valsize));
	}
	if (error == 0)
		error = sosetopt(so, &sopt);
	sockopt_destroy(&sopt);
 out:
 	fd_putfile(SCARG(uap, s));
	return (error);
}

static inline int sunos32_sys_socket_common(struct lwp *, register_t *,
					      int type);
static inline int
sunos32_sys_socket_common(struct lwp *l, register_t *retval, int type)
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
sunos32_sys_socket(struct lwp *l, const struct sunos32_sys_socket_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
	} */
	int error;

	error = netbsd32___socket30(l, (const void *)uap, retval);
	if (error)
		return (error);
	return sunos32_sys_socket_common(l, retval, SCARG(uap, type));
}

int
sunos32_sys_socketpair(struct lwp *l, const struct sunos32_sys_socketpair_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
		syscallarg(int *) rsv;
	} */
	int error;

	error = netbsd32_socketpair(l, (const void *)uap, retval);
	if (error)
		return (error);
	return sunos32_sys_socket_common(l, retval, SCARG(uap, type));
}


/*
 * XXX: This needs cleaning up.
 */
int
sunos32_sys_auditsys(struct lwp *l, const struct sunos32_sys_auditsys_args *uap, register_t *retval)
{
	return 0;
}

int
sunos32_sys_uname(struct lwp *l, const struct sunos32_sys_uname_args *uap, register_t *retval)
{
	/* {
		syscallarg(sunos32_utsnamep_t) name;
	} */
	struct sunos_utsname sut;

	memset(&sut, 0, sizeof(sut));

	memcpy(sut.sysname, ostype, sizeof(sut.sysname) - 1);
	memcpy(sut.nodename, hostname, sizeof(sut.nodename));
	sut.nodename[sizeof(sut.nodename)-1] = '\0';
	memcpy(sut.release, osrelease, sizeof(sut.release) - 1);
	memcpy(sut.version, "1", sizeof(sut.version) - 1);
	memcpy(sut.machine, machine, sizeof(sut.machine) - 1);

	return copyout((void *)&sut, SCARG_P32(uap, name),
	    sizeof(struct sunos_utsname));
}

int
sunos32_sys_setpgrp(struct lwp *l, const struct sunos32_sys_setpgrp_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) pid;
		syscallarg(int) pgid;
	} */
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
		return netbsd32_setpgid(l, (const void *)uap, retval);
}

int
sunos32_sys_open(struct lwp *l, const struct sunos32_sys_open_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) flags;
		syscallarg(int) mode;
	} */
	struct proc *p = l->l_proc;
	struct sys_open_args ua;
	int lf, r;
	int noctty;
	int ret;

	/* convert mode into NetBSD mode */
	lf = SCARG(uap, flags);
	noctty = lf & 0x8000;
	r =	(lf & (0x0001 | 0x0002 | 0x0008 | 0x0040 | 0x0200 | 0x0400 | 0x0800));
	r |=	((lf & (0x0004 | 0x1000 | 0x4000)) ? O_NONBLOCK : 0);
	r |=	((lf & 0x0080) ? O_SHLOCK : 0);
	r |=	((lf & 0x0100) ? O_EXLOCK : 0);
	r |=	((lf & 0x2000) ? O_FSYNC : 0);

	SUNOS32TOP_UAP(path, const char);
	SCARG(&ua, flags) = r;
	SUNOS32TO64_UAP(mode);

	ret = sys_open(l, &ua, retval);

	/* XXXSMP unlocked */
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
sunos32_sys_nfssvc(struct lwp *l, const struct sunos32_sys_nfssvc_args *uap, register_t *retval)
{
#if 0
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
sunos32_sys_ustat(struct lwp *l, const struct sunos32_sys_ustat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) dev;
		syscallarg(sunos32_ustatp_t) buf;
	} */
	struct sunos_ustat us;
	int error;

	memset(&us, 0, sizeof us);

	/*
	 * XXX: should set f_tfree and f_tinode at least
	 * How do we translate dev -> fstat? (and then to sunos_ustat)
	 */

	if ((error = copyout(&us, SCARG_P32(uap, buf), sizeof us)) != 0)
		return (error);
	return 0;
}

int
sunos32_sys_quotactl(struct lwp *l, const struct sunos32_sys_quotactl_args *uap, register_t *retval)
{

	return EINVAL;
}

int
sunos32_sys_vhangup(struct lwp *l, const void *v, register_t *retval)
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
sunstatfs(struct statvfs *sp, void *sbuf)
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
	return copyout((void *)&ssfs, sbuf, sizeof ssfs);
}

int
sunos32_sys_statfs(struct lwp *l, const struct sunos32_sys_statfs_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(sunos32_statfsp_t) buf;
	} */
	struct mount *mp;
	struct statvfs *sp;
	int error;
	struct vnode *vp;
	struct sys_statvfs1_args ua;

	SUNOS32TOP_UAP(path, const char);

	error = namei_simple_user(SCARG(&ua, path),
				NSM_FOLLOW_TRYEMULROOT, &vp);
	if (error != 0)
		return (error);
	mp = vp->v_mount;
	sp = &mp->mnt_stat;
	vrele(vp);
	if ((error = VFS_STATVFS(mp, sp)) != 0)
		return (error);
	sp->f_flag = mp->mnt_flag & MNT_VISFLAGMASK;
	return sunstatfs(sp, SCARG_P32(uap, buf));
}

int
sunos32_sys_fstatfs(struct lwp *l, const struct sunos32_sys_fstatfs_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(sunos32_statfsp_t) buf;
	} */
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
	error = sunstatfs(sp, SCARG_P32(uap, buf));
 out:
 	fd_putfile(SCARG(uap, fd));
	return (error);
}

int
sunos32_sys_exportfs(struct lwp *l, const struct sunos32_sys_exportfs_args *uap, register_t *retval)
{
	/*
	 * XXX: should perhaps translate into a mount(2)
	 * with MOUNT_EXPORT?
	 */
	return 0;
}

int
sunos32_sys_mknod(struct lwp *l, const struct sunos32_sys_mknod_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) mode;
		syscallarg(int) dev;
	} */

	if (S_ISFIFO(SCARG(uap, mode)))
		return netbsd32_mkfifo(l, (const struct netbsd32_mkfifo_args *)uap, retval);

	return compat_50_netbsd32_mknod(l, (const struct compat_50_netbsd32_mknod_args *)uap, retval);
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
sunos32_sys_sysconf(struct lwp *l, const struct sunos32_sys_sysconf_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) name;
	} */
	extern u_int maxfiles;

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
sunos32_sys_getrlimit(struct lwp *l, const struct sunos32_sys_getrlimit_args *uap, register_t *retval)
{
	/* {
		syscallarg(u_int) which;
		syscallarg(netbsd32_orlimitp_t) rlp;
	} */
	struct compat_43_netbsd32_ogetrlimit_args ua_43;

	if (SCARG(uap, which) >= SUNOS_RLIM_NLIMITS)
		return EINVAL;

	SCARG(&ua_43, which) = SCARG(uap, which) == SUNOS_RLIMIT_NOFILE ? RLIMIT_NOFILE : SCARG(uap, which);
	SCARG(&ua_43, rlp) = SCARG(uap, rlp);

	return compat_43_netbsd32_ogetrlimit(l, &ua_43, retval);
}

int
sunos32_sys_setrlimit(struct lwp *l, const struct sunos32_sys_setrlimit_args *uap, register_t *retval)
{
	/* {
		syscallarg(u_int) which;
		syscallarg(netbsd32_orlimitp_t) rlp;
	} */
	struct compat_43_netbsd32_osetrlimit_args ua_43;

	if (SCARG(uap, which) >= SUNOS_RLIM_NLIMITS)
		return EINVAL;

	SCARG(&ua_43, which) = SCARG(uap, which) == SUNOS_RLIMIT_NOFILE ? RLIMIT_NOFILE : SCARG(uap, which);
	SCARG(&ua_43, rlp) = SCARG(uap, rlp);

	return compat_43_netbsd32_osetrlimit(l, &ua_43, retval);
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
sunos32_sys_ptrace(struct lwp *l, const struct sunos32_sys_ptrace_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) req;
		syscallarg(pid_t) pid;
		syscallarg(netbsd32_caddr_t) addr;
		syscallarg(int) data;
		syscallarg(netbsd32_charp) addr2;
	} */
	struct netbsd32_ptrace_args pa;
	int req;

#define sys_ptrace sysent[SYS_ptrace].sy_call
	if (sys_ptrace == sys_nosys)
		return ENOSYS;

	req = SCARG(uap, req);
	if ((unsigned int)req >= nreqs)
		return (EINVAL);

	req = sreq2breq[req];
	if (req == -1)
		return (EINVAL);

	SCARG(&pa, req) = req;
	SCARG(&pa, pid) = (pid_t)SCARG(uap, pid);
	SCARG(&pa, addr) = SCARG(uap, addr);
	SCARG(&pa, data) = SCARG(uap, data);

	return netbsd32_ptrace(l, &pa, retval);
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
sunos32_sys_reboot(struct lwp *l, const struct sunos32_sys_reboot_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) howto;
		syscallarg(netbsd32_charp) bootstr;
	} */
	struct sys_reboot_args ua;
	struct sunos_howto_conv *convp;
	int error, bsd_howto, sun_howto;
	char *bootstr;

	if ((error = kauth_authorize_system(l->l_cred,
	    KAUTH_SYSTEM_REBOOT, 0, NULL, NULL, NULL)) != 0)
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
	if (sun_howto & SUNOS_RB_STRING)
		bootstr = SCARG_P32(uap, bootstr);
	else
		bootstr = NULL;

	SCARG(&ua, opt) = bsd_howto;
	SCARG(&ua, bootstr) = bootstr;
	return (sys_reboot(l, &ua, retval));
}

/*
 * Generalized interface signal handler, 4.3-compatible.
 */
/* ARGSUSED */
int
sunos32_sys_sigvec(struct lwp *l, const struct sunos32_sys_sigvec_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) signum;
		syscallarg(struct sigvec *) nsv;
		syscallarg(struct sigvec *) osv;
	} */
	struct netbsd32_sigvec sv;
	struct sigaction nsa, osa;
	int error;

	if (SCARG_P32(uap, nsv)) {
		error = copyin(SCARG_P32(uap, nsv), &sv, sizeof(sv));
		if (error != 0)
			return (error);

		/*
		 * SunOS uses the mask 0x0004 as SV_RESETHAND
		 * meaning: `reset to SIG_DFL on delivery'.
		 * We support only the bits in: 0xF
		 * (those bits are the same as ours)
		 */
		if (sv.sv_flags & ~0xF)
			return (EINVAL);

		sunos32_sigvec_to_sigaction(&sv, &nsa);
	}
	error = sigaction1(l, SCARG(uap, signum),
			   SCARG_P32(uap, nsv) ? &nsa : 0,
			   SCARG_P32(uap, osv) ? &osa : 0,
			   NULL, 0);
	if (error != 0)
		return (error);

	if (SCARG_P32(uap, osv)) {
		sunos32_sigvec_from_sigaction(&sv, &osa);
		error = copyout(&sv, SCARG_P32(uap, osv), sizeof(sv));
		if (error != 0)
			return (error);
	}

	return (0);
}
