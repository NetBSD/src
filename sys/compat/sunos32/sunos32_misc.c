/*	$NetBSD: sunos32_misc.c,v 1.7.4.6 2002/05/29 21:32:58 nathanw Exp $	*/
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: sunos32_misc.c,v 1.7.4.6 2002/05/29 21:32:58 nathanw Exp $");

#define COMPAT_SUNOS 1

#if defined(_KERNEL_OPT)
#include "opt_nfsserver.h"
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
#include <sys/sa.h>
#include <sys/syscallargs.h>
#include <sys/conf.h>
#include <sys/socketvar.h>
#include <sys/exec.h>
#include <sys/swap.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/sunos32/sunos32.h>
#include <compat/sunos32/sunos32_dirent.h>
#include <compat/sunos32/sunos32_syscallargs.h>
#include <compat/common/compat_util.h>

#include <netinet/in.h>

#include <miscfs/specfs/specdev.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsmount.h>

static void sunos32_sigvec_to_sigaction(const struct netbsd32_sigvec *, struct sigaction *);
static void sunos32_sigvec_from_sigaction(struct netbsd32_sigvec *, const struct sigaction *);

static int sunstatfs __P((struct statfs *, caddr_t));

static void
sunos32_sigvec_to_sigaction(sv, sa)
	const struct netbsd32_sigvec *sv;
	struct sigaction *sa;
{
/*XXX*/ extern void compat_43_sigmask_to_sigset __P((const int *, sigset_t *));
	
	sa->sa_handler = (void *)(u_long)sv->sv_handler;
	compat_43_sigmask_to_sigset(&sv->sv_mask, &sa->sa_mask);
	sa->sa_flags = sv->sv_flags ^ SA_RESTART;
}

static 
void sunos32_sigvec_from_sigaction(sv, sa)
	struct netbsd32_sigvec *sv;
	const struct sigaction *sa;
{
/*XXX*/ extern void compat_43_sigset_to_sigmask __P((const sigset_t *, int *));

	sv->sv_handler = (netbsd32_voidp)(u_long)sa->sa_handler;
	compat_43_sigset_to_sigmask(&sa->sa_mask, &sv->sv_mask);
	sv->sv_flags = sa->sa_flags ^ SA_RESTART;
}

int
sunos32_sys_stime(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_stime_args /* {
		syscallarg(sunos32_time_tp) tp;
	} */ *uap = v;
	struct sys_settimeofday_args ap;
	caddr_t sg = stackgap_init(p, 0);
	struct netbsd32_timeval ntv;
	struct timeval tv, *sgtvp;
	int error;

	error = copyin((caddr_t)(u_long)SCARG(uap, tp), &ntv.tv_sec, sizeof(ntv.tv_sec));
	if (error)
		return error;
	tv.tv_sec = ntv.tv_sec;
	tv.tv_usec = 0;

	SCARG(&ap, tv) = sgtvp = stackgap_alloc(p, &sg, sizeof(struct timeval));
	SCARG(&ap, tzp) = NULL;

	error = copyout(&tv, sgtvp, sizeof(struct timeval));
	if (error)
		return error;

	return sys_settimeofday(p, &ap, retval);
}

int
sunos32_sys_wait4(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_wait4_args /* {
		syscallarg(int) pid;
		syscallarg(netbsd32_intp) status;
		syscallarg(int) options;
		syscallarg(netbsd32_rusagep_t) rusage;
	} */ *uap = v;

	if (SCARG(uap, pid) == 0)
		SCARG(uap, pid) = WAIT_ANY;
	return (netbsd32_wait4(p, uap, retval));
}

int
sunos32_sys_creat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_creat_args /* { 
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) mode;
	} */ *uap = v;
	struct sys_open_args ua;
	caddr_t sg = stackgap_init(p, 0);

	SUNOS32TOP_UAP(path, const char);
	SCARG(&ua, flags) = O_WRONLY | O_CREAT | O_TRUNC;
	SUNOS32TO64_UAP(mode);

	SUNOS32_CHECK_ALT_CREAT(p, &sg, SCARG(&ua, path));

	return (sys_open(p, &ua, retval));
}

int
sunos32_sys_access(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_access_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys_access_args ua;
	caddr_t sg = stackgap_init(p, 0);

	SUNOS32TOP_UAP(path, const char);
	SUNOS32TO64_UAP(flags);
	SUNOS32_CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));

	return (sys_access(p, &ua, retval));
}

static __inline void sunos32_from___stat13 __P((struct stat *, struct netbsd32_stat43 *));

static __inline void
sunos32_from___stat13(sbp, sb32p)
	struct stat *sbp;
	struct netbsd32_stat43 *sb32p;
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
sunos32_sys_stat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_stat_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_stat43p_t) ub;
	} */ *uap = v;
	struct netbsd32_stat43 sb32;
	struct stat sb;
	struct nameidata nd;
	caddr_t sg;
	const char *path;
	int error;

	path = (char *)(u_long)SCARG(uap, path);
	sg = stackgap_init(p, 0);
	SUNOS32_CHECK_ALT_EXIST(p, &sg, path);

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE, path, p);
	if ((error = namei(&nd)) != 0)
		return (error);
	error = vn_stat(nd.ni_vp, &sb, p);
	vput(nd.ni_vp);
	if (error)
		return (error);
	sunos32_from___stat13(&sb, &sb32);
	error = copyout((caddr_t)&sb32, (caddr_t)(u_long)SCARG(uap, ub), sizeof (sb32));
	return (error);
}

int
sunos32_sys_lstat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_lstat_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_stat43p_t) ub;
	} */ *uap = v;
	struct vnode *vp, *dvp;
	struct stat sb, sb1;
	struct netbsd32_stat43 sb32;
	int error;
	struct nameidata nd;
	int ndflags;
	const char *path;
	caddr_t sg;

	path = (char *)(u_long)SCARG(uap, path);
	sg = stackgap_init(p, 0);
	SUNOS32_CHECK_ALT_EXIST(p, &sg, path);

	ndflags = NOFOLLOW | LOCKLEAF | LOCKPARENT;
again:
	NDINIT(&nd, LOOKUP, ndflags, UIO_USERSPACE, path, p);
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
		error = vn_stat(vp, &sb, p);
		vput(vp);
		if (error)
			return (error);
	} else {
		error = vn_stat(dvp, &sb, p);
		vput(dvp);
		if (error) {
			vput(vp);
			return (error);
		}
		error = vn_stat(vp, &sb1, p);
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
	error = copyout((caddr_t)&sb32, (caddr_t)(u_long)SCARG(uap, ub), sizeof (sb32));
	return (error);
}

int
sunos32_sys_execv(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_execv_args /* {
		syscallarg(const char *) path;
		syscallarg(char **) argv;
	} */ *uap = v;
	struct sys_execve_args ua;
	caddr_t sg;

	SUNOS32TOP_UAP(path, const char);
	SUNOS32TOP_UAP(argp, char *);
	SCARG(&ua, envp) = NULL;
	sg = stackgap_init(p, 0);
	SUNOS32_CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));

	return netbsd32_execve2(p, &ua, retval);
}

int
sunos32_sys_execve(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_execve_args /* {
		syscallarg(const char *) path;
		syscallarg(char **) argv;
		syscallarg(char **) envp;
	} */ *uap = v;
	struct sys_execve_args ua;
	caddr_t sg;

	SUNOS32TOP_UAP(path, const char);
	SUNOS32TOP_UAP(argp, char *);
	SUNOS32TOP_UAP(envp, char *);
	sg = stackgap_init(p, 0);
	SUNOS32_CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return netbsd32_execve2(p, &ua, retval);
}

int
sunos32_sys_omsync(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_omsync_args /* {
		syscallarg(netbsd32_caddr_t) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) flags;
	} */ *uap = v;
	struct netbsd32___msync13_args ouap;

	SCARG(&ouap, addr) = SCARG(uap, addr);
	SCARG(&ouap, len) = SCARG(uap, len);
	SCARG(&ouap, flags) = SCARG(uap, flags);

	return (netbsd32___msync13(p, &ouap, retval));
}

int
sunos32_sys_unmount(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_unmount_args /* {
		syscallarg(netbsd32_charp) path;
	} */ *uap = v;
	struct sys_unmount_args ua;

	SUNOS32TOP_UAP(path, const char);
	SCARG(&ua, flags) = 0;

	return (sys_unmount(p, &ua, retval));
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
sunos32_sys_mount(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_mount_args /* {
		syscallarg(netbsd32_charp) type;
		syscallarg(netbsd32_charp) path;
		syscallarg(int) flags;
		syscallarg(netbsd32_caddr_t) data;
	} */ *uap = v;
	struct sys_mount_args ua;
	int oflags = SCARG(uap, flags), nflags, error;
	char fsname[MFSNAMELEN];
	caddr_t sg = stackgap_init(p, 0);

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
	SCARG(uap, flags) = nflags;

	error = copyinstr((caddr_t)(u_long)SCARG(uap, type), fsname,
	    sizeof fsname, (size_t *)0);
	if (error)
		return (error);

	if (strncmp(fsname, "4.2", sizeof fsname) == 0) {
		SCARG(uap, type) = (netbsd32_charp)(u_long)stackgap_alloc(p, &sg, sizeof("ffs"));
		error = copyout("ffs", (caddr_t)(u_long)SCARG(uap, type), sizeof("ffs"));
		if (error)
			return (error);
	} else if (strncmp(fsname, "nfs", sizeof fsname) == 0) {
		struct sunos_nfs_args sna;
		struct sockaddr_in sain;
		struct nfs_args na;	/* XXX */
		struct sockaddr sa;
		int n;

		error = copyin((caddr_t)(u_long)SCARG(uap, data), &sna, sizeof sna);
		if (error)
			return (error);
		error = copyin(sna.addr, &sain, sizeof sain);
		if (error)
			return (error);
		memcpy(&sa, &sain, sizeof sa);
		sa.sa_len = sizeof(sain);
		SCARG(uap, data) = (netbsd32_charp)(u_long)stackgap_alloc(p, &sg, sizeof(na));
		na.version = NFS_ARGSVERSION;
		na.addr = stackgap_alloc(p, &sg, sizeof(struct sockaddr));
		na.addrlen = sizeof(struct sockaddr);
		na.sotype = SOCK_DGRAM;
		na.proto = IPPROTO_UDP;
		na.fh = (void *)sna.fh;
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

		error = copyout(&sa, na.addr, sizeof sa);
		if (error)
			return (error);
		error = copyout(&na, (caddr_t)(u_long)SCARG(uap, data), sizeof na);
		if (error)
			return (error);
	}
	SUNOS32TOP_UAP(type, const char);
	SUNOS32TOP_UAP(path, const char);
	SUNOS32TO64_UAP(flags);
	SUNOS32TOP_UAP(data, void);
	return (sys_mount(p, &ua, retval));
}

#if defined(NFS)
int
async_daemon(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_nfssvc_args ouap;

	SCARG(&ouap, flag) = NFSSVC_BIOD;
	SCARG(&ouap, argp) = NULL;

	return (netbsd32_nfssvc(p, &ouap, retval));
}
#endif /* NFS */

void	native_to_sunos_sigset __P((const sigset_t *, int *));
void	sunos_to_native_sigset __P((const int, sigset_t *));

__inline__ void
native_to_sunos_sigset(ss, mask)
	const sigset_t *ss;
	int *mask;
{
	*mask = ss->__bits[0];
}

__inline__ void
sunos_to_native_sigset(mask, ss)
	const int mask;
	sigset_t *ss;
{

	ss->__bits[0] = mask;
	ss->__bits[1] = 0;
	ss->__bits[2] = 0;
	ss->__bits[3] = 0;
}

int
sunos32_sys_sigpending(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_sigpending_args /* {
		syscallarg(netbsd32_intp) mask;
	} */ *uap = v;
	sigset_t ss;
	int mask;

	sigpending1(p, &ss);
	native_to_sunos_sigset(&ss, &mask);

	return (copyout((caddr_t)(u_long)&mask, (caddr_t)(u_long)SCARG(uap, mask), sizeof(int)));
}

int     
sunos32_sys_sigsuspend(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_sigsuspend_args /* {
		syscallarg(int) mask;
	} */ *uap = v;
	int mask;
	sigset_t ss;
        
	mask = SCARG(uap, mask);
	sunos_to_native_sigset(mask, &ss);
	return (sigsuspend1(p, &ss));
}

/*
 * Read Sun-style directory entries.  We suck them into kernel space so
 * that they can be massaged before being copied out to user code.  Like
 * SunOS, we squish out `empty' entries.
 *
 * This is quite ugly, but what do you expect from compatibility code?
 */
int
sunos32_sys_getdents(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_getdents_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) buf;
		syscallarg(int) nbytes;
	} */ *uap = v;
	struct dirent *bdp;
	struct vnode *vp;
	caddr_t inp, buf;	/* BSD-format */
	int len, reclen;	/* BSD-format */
	caddr_t outp;		/* Sun-format */
	int resid, sunos_reclen;/* Sun-format */
	struct file *fp;
	struct uio auio;
	struct iovec aiov;
	struct sunos32_dirent idb;
	off_t off;			/* true file offset */
	int buflen, error, eofflag;
	off_t *cookiebuf, *cookie;
	int ncookies;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);

	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto out1;
	}

	vp = (struct vnode *)fp->f_data;
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
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_procp = p;
	auio.uio_resid = buflen;
	auio.uio_offset = off;
	/*
	 * First we read into the malloc'ed buffer, then
	 * we massage it into user space, one record at a time.
	 */
	error = VOP_READDIR(vp, &auio, fp->f_cred, &eofflag, &cookiebuf,
	    &ncookies);
	if (error)
		goto out;

	inp = buf;
	outp = (caddr_t)(u_long)SCARG(uap, buf);
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
			off = *cookie++;
			continue;
		}
		sunos_reclen = SUNOS32_RECLEN(&idb, bdp->d_namlen);
		if (reclen > len || resid < sunos_reclen) {
			/* entry too big for buffer, so just stop */
			outp++;
			break;
		}
		off = *cookie++;	/* each entry points to next */
		/*
		 * Massage in place to make a Sun-shaped dirent (otherwise
		 * we have to worry about touching user memory outside of
		 * the copyout() call).
		 */
		idb.d_fileno = bdp->d_fileno;
		idb.d_off = off;
		idb.d_reclen = sunos_reclen;
		idb.d_namlen = bdp->d_namlen;
		strcpy(idb.d_name, bdp->d_name);
		if ((error = copyout((caddr_t)&idb, outp, sunos_reclen)) != 0)
			goto out;
		/* advance past this real entry */
		inp += reclen;
		/* advance output past Sun-shaped entry */
		outp += sunos_reclen;
		resid -= sunos_reclen;
	}

	/* if we squished out the whole block, try again */
	if (outp == (caddr_t)(u_long)SCARG(uap, buf))
		goto again;
	fp->f_offset = off;		/* update the vnode offset */

eof:
	*retval = SCARG(uap, nbytes) - resid;
out:
	VOP_UNLOCK(vp, 0);
	free(cookiebuf, M_TEMP);
	free(buf, M_TEMP);
 out1:
	FILE_UNUSE(fp, p);
	return (error);
}

#define	SUNOS32__MAP_NEW	0x80000000	/* if not, old mmap & cannot handle */

int
sunos32_sys_mmap(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_mmap_args /* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) prot;
		syscallarg(int) flags;
		syscallarg(int) fd;
		syscallarg(netbsd32_long) pos;
	} */ *uap = v;
	struct sys_mmap_args ua;
	struct filedesc *fdp;
	struct file *fp;
	struct vnode *vp;
	void *rt;
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
	SCARG(&ua, pad) = 0;
	SUNOS32TOX_UAP(pos, off_t);

	if ((SCARG(&ua, flags) & MAP_FIXED) == 0 &&
	    SCARG(&ua, addr) != 0 &&
	    SCARG(&ua, addr) < (void *)round_page((vaddr_t)p->p_vmspace->vm_daddr+MAXDSIZ))
		SCARG(&ua, addr) = (void *)round_page((vaddr_t)p->p_vmspace->vm_daddr+MAXDSIZ);

	/*
	 * Special case: if fd refers to /dev/zero, map as MAP_ANON.  (XXX)
	 */
	fdp = p->p_fd;
	if ((fp = fd_getfile(fdp, SCARG(&ua, fd))) != NULL &&		/*XXX*/
	    fp->f_type == DTYPE_VNODE &&				/*XXX*/
	    (vp = (struct vnode *)fp->f_data)->v_type == VCHR &&	/*XXX*/
	    iszerodev(vp->v_rdev)) {					/*XXX*/
		SCARG(&ua, flags) |= MAP_ANON;
		SCARG(&ua, fd) = -1;
	}

	error = sys_mmap(p, &ua, (register_t *)&rt);
	if ((long)rt > (long)UINT_MAX)
		printf("sunos32_mmap: retval out of range: %p", rt);
	*retval = (netbsd32_voidp)(u_long)rt;
	return (error);
}

#define	MC_SYNC		1
#define	MC_LOCK		2
#define	MC_UNLOCK	3
#define	MC_ADVISE	4
#define	MC_LOCKAS	5
#define	MC_UNLOCKAS	6

int
sunos32_sys_mctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_mctl_args /* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(int) len;
		syscallarg(int) func;
		syscallarg(netbsd32_voidp) arg;
	} */ *uap = v;

	switch (SCARG(uap, func)) {
	case MC_ADVISE:		/* ignore for now */
		return (0);
	case MC_SYNC:		/* translate to msync */
		return (netbsd32___msync13(p, uap, retval));
	default:
		return (EINVAL);
	}
}

int
sunos32_sys_setsockopt(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_setsockopt_args /* {
		syscallarg(int) s;
		syscallarg(int) level;
		syscallarg(int) name;
		syscallarg(netbsd32_caddr_t) val;
		syscallarg(int) valsize;
	} */ *uap = v;
	struct file *fp;
	struct mbuf *m = NULL;
	int error;

	/* getsock() will use the descriptor for us */
	if ((error = getsock(p->p_fd, SCARG(uap, s), &fp)) != 0)
		return (error);
#define	SO_DONTLINGER (~SO_LINGER)
	if (SCARG(uap, name) == SO_DONTLINGER) {
		m = m_get(M_WAIT, MT_SOOPTS);
		mtod(m, struct linger *)->l_onoff = 0;
		m->m_len = sizeof(struct linger);
		error = sosetopt((struct socket *)fp->f_data, SCARG(uap, level),
		    SO_LINGER, m);
		goto out;
	}
	if (SCARG(uap, level) == IPPROTO_IP) {
#define		SUNOS_IP_MULTICAST_IF		2
#define		SUNOS_IP_MULTICAST_TTL		3
#define		SUNOS_IP_MULTICAST_LOOP		4
#define		SUNOS_IP_ADD_MEMBERSHIP		5
#define		SUNOS_IP_DROP_MEMBERSHIP	6
		static int ipoptxlat[] = {
			IP_MULTICAST_IF,
			IP_MULTICAST_TTL,
			IP_MULTICAST_LOOP,
			IP_ADD_MEMBERSHIP,
			IP_DROP_MEMBERSHIP
		};
		if (SCARG(uap, name) >= SUNOS_IP_MULTICAST_IF &&
		    SCARG(uap, name) <= SUNOS_IP_DROP_MEMBERSHIP) {
			SCARG(uap, name) =
			    ipoptxlat[SCARG(uap, name) - SUNOS_IP_MULTICAST_IF];
		}
	}
	if (SCARG(uap, valsize) > MLEN) {
		error = EINVAL;
		goto out;
	}
	if (SCARG(uap, val)) {
		m = m_get(M_WAIT, MT_SOOPTS);
		error = copyin((caddr_t)(u_long)SCARG(uap, val), mtod(m, caddr_t),
		    (u_int)SCARG(uap, valsize));
		if (error) {
			(void) m_free(m);
			goto out;
		}
		m->m_len = SCARG(uap, valsize);
	}
	error = sosetopt((struct socket *)fp->f_data, SCARG(uap, level),
	    SCARG(uap, name), m);
 out:
	FILE_UNUSE(fp, p);
	return (error);
}

static __inline__ int sunos32_sys_socket_common(struct proc *, register_t *,
					      int type);
static __inline__ int
sunos32_sys_socket_common(p, retval, type)
	struct proc *p;
	register_t *retval;
	int type;
{
	struct socket *so;
	struct file *fp;
	int error, fd;

	/* getsock() will use the descriptor for us */
	fd = (int)*retval;
	if ((error = getsock(p->p_fd, fd, &fp)) == 0) {
		so = (struct socket *)fp->f_data;
		if (type == SOCK_DGRAM)
			so->so_options |= SO_BROADCAST;
	}
	FILE_UNUSE(fp, p);
	return (error);
}

int
sunos32_sys_socket(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_socket_args /* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
	} */ *uap = v;
	int error;

	error = netbsd32_socket(p, v, retval);
	if (error)
		return (error);
	return sunos32_sys_socket_common(p, retval, SCARG(uap, type));
}

int
sunos32_sys_socketpair(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_socketpair_args /* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
		syscallarg(int *) rsv;
	} */ *uap = v;
	int error;

	error = netbsd32_socketpair(p, v, retval);
	if (error)
		return (error);
	return sunos32_sys_socket_common(p, retval, SCARG(uap, type));
}


/*
 * XXX: This needs cleaning up.
 */
int
sunos32_sys_auditsys(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	return 0;
}

int
sunos32_sys_uname(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_uname_args /* {
		syscallarg(sunos32_utsnamep_t) name;
	} */ *uap = v;
	struct sunos_utsname sut;

	memset(&sut, 0, sizeof(sut));

	memcpy(sut.sysname, ostype, sizeof(sut.sysname) - 1);
	memcpy(sut.nodename, hostname, sizeof(sut.nodename));
	sut.nodename[sizeof(sut.nodename)-1] = '\0';
	memcpy(sut.release, osrelease, sizeof(sut.release) - 1);
	memcpy(sut.version, "1", sizeof(sut.version) - 1);
	memcpy(sut.machine, machine, sizeof(sut.machine) - 1);

	return copyout((caddr_t)&sut, (caddr_t)(u_long)SCARG(uap, name),
	    sizeof(struct sunos_utsname));
}

int
sunos32_sys_setpgrp(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_setpgrp_args /* {
		syscallarg(int) pid;
		syscallarg(int) pgid;
	} */ *uap = v;

	/*
	 * difference to our setpgid call is to include backwards
	 * compatibility to pre-setsid() binaries. Do setsid()
	 * instead of setpgid() in those cases where the process
	 * tries to create a new session the old way.
	 */
	if (!SCARG(uap, pgid) &&
	    (!SCARG(uap, pid) || SCARG(uap, pid) == p->p_pid))
		return sys_setsid(p, uap, retval);
	else
		return netbsd32_setpgid(p, uap, retval);
}

int
sunos32_sys_open(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_open_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) flags;
		syscallarg(int) mode;
	} */ *uap = v;
	struct sys_open_args ua;
	int l, r;
	int noctty;
	int ret;
	caddr_t sg = stackgap_init(p, 0);

	/* convert mode into NetBSD mode */
	l = SCARG(uap, flags);
	noctty = l & 0x8000;
	r =	(l & (0x0001 | 0x0002 | 0x0008 | 0x0040 | 0x0200 | 0x0400 | 0x0800));
	r |=	((l & (0x0004 | 0x1000 | 0x4000)) ? O_NONBLOCK : 0);
	r |=	((l & 0x0080) ? O_SHLOCK : 0);
	r |=	((l & 0x0100) ? O_EXLOCK : 0);
	r |=	((l & 0x2000) ? O_FSYNC : 0);

	SUNOS32TOP_UAP(path, const char);
	SCARG(&ua, flags) = r;
	SUNOS32TO64_UAP(mode);

	if (r & O_CREAT)
		SUNOS32_CHECK_ALT_CREAT(p, &sg, SCARG(&ua, path));
	else
		SUNOS32_CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));

	ret = sys_open(p, &ua, retval);

	if (!ret && !noctty && SESS_LEADER(p) && !(p->p_flag & P_CONTROLT)) {
		struct filedesc *fdp = p->p_fd;
		struct file *fp;

		fp = fd_getfile(fdp, *retval);

		/* ignore any error, just give it a try */
		if (fp != NULL && fp->f_type == DTYPE_VNODE)
			(fp->f_ops->fo_ioctl)(fp, TIOCSCTTY, (caddr_t)0, p);
	}
	return ret;
}

#if defined (NFSSERVER)
int
sunos32_sys_nfssvc(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#if 0
	struct sunos32_sys_nfssvc_args *uap = v;
	struct emul *e = p->p_emul;
	struct sys_nfssvc_args outuap;
	struct sockaddr sa;
	int error;
	caddr_t sg = stackgap_init(p, 0);

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

	return nfssvc(p, &outuap, retval);
#else
	return (ENOSYS);
#endif
}
#endif /* NFSSERVER */

int
sunos32_sys_ustat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_ustat_args /* {
		syscallarg(int) dev;
		syscallarg(sunos32_ustatp_t) buf;
	} */ *uap = v;
	struct sunos_ustat us;
	int error;

	memset(&us, 0, sizeof us);

	/*
	 * XXX: should set f_tfree and f_tinode at least
	 * How do we translate dev -> fstat? (and then to sunos_ustat)
	 */

	if ((error = copyout(&us, (caddr_t)(u_long)SCARG(uap, buf), sizeof us)) != 0)
		return (error);
	return 0;
}

int
sunos32_sys_quotactl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{

	return EINVAL;
}

int
sunos32_sys_vhangup(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
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
sunstatfs(sp, buf)
	struct statfs *sp;
	caddr_t buf;
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
	ssfs.f_fsid = sp->f_fsid;
	return copyout((caddr_t)&ssfs, buf, sizeof ssfs);
}	

int
sunos32_sys_statfs(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_statfs_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(sunos32_statfsp_t) buf;
	} */ *uap = v;
	struct mount *mp;
	struct statfs *sp;
	int error;
	struct nameidata nd;

	caddr_t sg = stackgap_init(p, 0);
	SUNOS32_CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, (caddr_t)(u_long)SCARG(uap, path), p);
	if ((error = namei(&nd)) != 0)
		return (error);
	mp = nd.ni_vp->v_mount;
	sp = &mp->mnt_stat;
	vrele(nd.ni_vp);
	if ((error = VFS_STATFS(mp, sp, p)) != 0)
		return (error);
	sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
	return sunstatfs(sp, (caddr_t)(u_long)SCARG(uap, buf));
}

int
sunos32_sys_fstatfs(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_fstatfs_args /* {
		syscallarg(int) fd;
		syscallarg(sunos32_statfsp_t) buf;
	} */ *uap = v;
	struct file *fp;
	struct mount *mp;
	struct statfs *sp;
	int error;

	/* getvnode() will use the descriptor for us */
	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);
	mp = ((struct vnode *)fp->f_data)->v_mount;
	sp = &mp->mnt_stat;
	if ((error = VFS_STATFS(mp, sp, p)) != 0)
		goto out;
	sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
	error = sunstatfs(sp, (caddr_t)(u_long)SCARG(uap, buf));
 out:
	FILE_UNUSE(fp, p);
	return (error);
}

int
sunos32_sys_exportfs(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	/*
	 * XXX: should perhaps translate into a mount(2)
	 * with MOUNT_EXPORT?
	 */
	return 0;
}

int
sunos32_sys_mknod(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_mknod_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) mode;
		syscallarg(int) dev;
	} */ *uap = v;

	caddr_t sg = stackgap_init(p, 0);
	SUNOS32_CHECK_ALT_CREAT(p, &sg, SCARG(uap, path));

	/* netbsd32_mkfifo/mknod to not do alt checking */
	if (S_ISFIFO(SCARG(uap, mode)))
		return netbsd32_mkfifo(p, (struct netbsd32_mkfifo_args *)uap, retval);

	return netbsd32_mknod(p, (struct netbsd32_mknod_args *)uap, retval);
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
sunos32_sys_sysconf(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_sysconf_args /* {
		syscallarg(int) name;
	} */ *uap = v;
	extern int maxfiles;

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
sunos32_sys_getrlimit(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_getrlimit_args /* {
		syscallarg(u_int) which;
		syscallarg(netbsd32_orlimitp_t) rlp;
	} */ *uap = v;

	if (SCARG(uap, which) >= SUNOS_RLIM_NLIMITS)
		return EINVAL;

	if (SCARG(uap, which) == SUNOS_RLIMIT_NOFILE)
		SCARG(uap, which) = RLIMIT_NOFILE;

	return compat_43_netbsd32_ogetrlimit(p, uap, retval);
}

int
sunos32_sys_setrlimit(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_setrlimit_args /* {
		syscallarg(u_int) which;
		syscallarg(netbsd32_orlimitp_t) rlp;
	} */ *uap = v;

	if (SCARG(uap, which) >= SUNOS_RLIM_NLIMITS)
		return EINVAL;

	if (SCARG(uap, which) == SUNOS_RLIMIT_NOFILE)
		SCARG(uap, which) = RLIMIT_NOFILE;

	return compat_43_netbsd32_osetrlimit(p, uap, retval);
}

/* for the m68k machines */
#ifndef PT_GETFPREGS
#define PT_GETFPREGS -1
#endif
#ifndef PT_SETFPREGS
#define PT_SETFPREGS -1
#endif

static int sreq2breq[] = {
	PT_TRACE_ME,    PT_READ_I,      PT_READ_D,      -1,
	PT_WRITE_I,     PT_WRITE_D,     -1,             PT_CONTINUE,
	PT_KILL,        -1,             PT_ATTACH,      PT_DETACH,
	PT_GETREGS,     PT_SETREGS,     PT_GETFPREGS,   PT_SETFPREGS
};
static int nreqs = sizeof(sreq2breq) / sizeof(sreq2breq[0]);

int
sunos32_sys_ptrace(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_ptrace_args /* {
		syscallarg(int) req;
		syscallarg(pid_t) pid;
		syscallarg(netbsd32_caddr_t) addr;
		syscallarg(int) data;
		syscallarg(netbsd32_charp) addr2;
	} */ *uap = v;
	struct netbsd32_ptrace_args pa;
	int req;

	req = SCARG(uap, req);

	if (req < 0 || req >= nreqs)
		return (EINVAL);

	req = sreq2breq[req];
	if (req == -1)
		return (EINVAL);

	SCARG(&pa, req) = req;
	SCARG(&pa, pid) = (pid_t)SCARG(uap, pid);
	SCARG(&pa, addr) = SCARG(uap, addr);
	SCARG(&pa, data) = SCARG(uap, data);

	return netbsd32_ptrace(p, &pa, retval);
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
sunos32_sys_reboot(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_reboot_args /* {
		syscallarg(int) howto;
		syscallarg(netbsd32_charp) bootstr;
	} */ *uap = v;
	struct sys_reboot_args ua;
	struct sunos_howto_conv *convp;
	int error, bsd_howto, sun_howto;
	char *bootstr;

	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
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
		bootstr = (char *)(u_long)SCARG(uap, bootstr);
	else
		bootstr = NULL;

	SCARG(&ua, opt) = bsd_howto;
	SCARG(&ua, bootstr) = bootstr;
	return (sys_reboot(p, &ua, retval));
}

/*
 * Generalized interface signal handler, 4.3-compatible.
 */
/* ARGSUSED */
int
sunos32_sys_sigvec(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sunos32_sys_sigvec_args /* {
		syscallarg(int) signum;
		syscallarg(struct sigvec *) nsv;
		syscallarg(struct sigvec *) osv;
	} */ *uap = v;
	struct netbsd32_sigvec sv;
	struct sigaction nsa, osa;
	int error;

	if (SCARG(uap, nsv)) {
		error = copyin((caddr_t)(u_long)SCARG(uap, nsv), &sv, sizeof(sv));
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
	error = sigaction1(p, SCARG(uap, signum),
			   SCARG(uap, nsv) ? &nsa : 0,
			   SCARG(uap, osv) ? &osa : 0);
	if (error != 0)
		return (error);

	if (SCARG(uap, osv)) {
		sunos32_sigvec_from_sigaction(&sv, &osa);
		error = copyout(&sv, (caddr_t)(u_long)SCARG(uap, osv), sizeof(sv));
		if (error != 0)
			return (error);
	}

	return (0);
}
