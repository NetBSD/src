/*	$NetBSD: svr4_32_misc.c,v 1.28 2004/04/22 14:32:09 hannken Exp $	 */

/*-
 * Copyright (c) 1994 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * SVR4 compatibility module.
 *
 * SVR4 system calls that are implemented differently in BSD are
 * handled here.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: svr4_32_misc.c,v 1.28 2004/04/22 14:32:09 hannken Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/dirent.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/mbuf.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/socket.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sys/unistd.h>
#include <sys/times.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/ptrace.h>
#include <sys/signalvar.h>

#include <netinet/in.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <miscfs/specfs/specdev.h>

#include <compat/svr4_32/svr4_32_types.h>
#include <compat/svr4_32/svr4_32_signal.h>
#include <compat/svr4_32/svr4_32_lwp.h>
#include <compat/svr4_32/svr4_32_ucontext.h>
#include <compat/svr4_32/svr4_32_syscallargs.h>
#include <compat/svr4_32/svr4_32_util.h>
#include <compat/svr4_32/svr4_32_time.h>
#include <compat/svr4_32/svr4_32_dirent.h>
#include <compat/svr4/svr4_ulimit.h>
#include <compat/svr4_32/svr4_32_hrt.h>
#include <compat/svr4/svr4_wait.h>
#include <compat/svr4_32/svr4_32_statvfs.h>
#include <compat/svr4/svr4_sysconfig.h>
#include <compat/svr4_32/svr4_32_acl.h>
#include <compat/svr4/svr4_mman.h>

#include <machine/cpu.h>

#include <uvm/uvm_extern.h>

static int svr4_to_bsd_mmap_flags __P((int));

static __inline clock_t timeval_to_clock_t __P((struct timeval *));
static int svr4_32_setinfo	__P((struct proc *, int, svr4_32_siginfo_tp));

struct svr4_32_hrtcntl_args;
static int svr4_32_hrtcntl	__P((struct proc *, struct svr4_32_hrtcntl_args *,
    register_t *));
static void bsd_statvfs_to_svr4_32_statvfs __P((const struct statvfs *,
    struct svr4_32_statvfs *));
static void bsd_statvfs_to_svr4_32_statvfs64 __P((const struct statvfs *,
    struct svr4_32_statvfs64 *));
#define svr4_32_pfind(pid) p_find((pid), PFIND_UNLOCK | PFIND_ZOMBIE)

static int svr4_32_mknod __P((struct lwp *, register_t *, const char *,
    svr4_32_mode_t, svr4_32_dev_t));

int
svr4_32_sys_wait(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_wait_args *uap = v;
	struct proc *p = l->l_proc;
	struct sys_wait4_args w4;
	int error;
	size_t sz = sizeof(*SCARG(&w4, status));
	int st, sig;

	SCARG(&w4, rusage) = NULL;
	SCARG(&w4, options) = 0;

	if (SCARG(uap, status) == 0) {
		caddr_t sg = stackgap_init(p, 0);

		SCARG(&w4, status) = stackgap_alloc(p, &sg, sz);
	}
	else
		SCARG(&w4, status) = (int *)(u_long)SCARG(uap, status);

	SCARG(&w4, pid) = WAIT_ANY;

	if ((error = sys_wait4(l, &w4, retval)) != 0)
		return error;
	
	if ((error = copyin(SCARG(&w4, status), &st, sizeof(st))) != 0)
		return error;

	if (WIFSIGNALED(st)) {
		sig = WTERMSIG(st);
		if (sig >= 0 && sig < NSIG)
			st = (st & ~0177) | native_to_svr4_signo[sig];
	} else if (WIFSTOPPED(st)) {
		sig = WSTOPSIG(st);
		if (sig >= 0 && sig < NSIG)
			st = (st & ~0xff00) | (native_to_svr4_signo[sig] << 8);
	}

	/*
	 * It looks like wait(2) on svr4/solaris/2.4 returns
	 * the status in retval[1], and the pid on retval[0].
	 */
	retval[1] = st;

	if (SCARG(uap, status))
		if ((error = copyout(&st, (caddr_t)(u_long)SCARG(uap, status), 
				     sizeof(st))) != 0)
			return error;

	return 0;
}


int
svr4_32_sys_execv(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_execv_args /* {
		syscallarg(char *) path;
		syscallarg(char **) argv;
	} */ *uap = v;
	struct netbsd32_execve_args_noconst {
		syscallarg(netbsd32_charp) path;
		syscallarg(netbsd32_charpp) argp;
		syscallarg(netbsd32_charpp) envp;
	} ap;

	SCARG(&ap, path) = SCARG(uap, path);
	SCARG(&ap, argp) = SCARG(uap, argp);
	SCARG(&ap, envp) = 0;

	return netbsd32_execve(l, &ap, retval);
}

#if 0
int
svr4_32_sys_execve(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_execve_args /* {
		syscallarg(const char *) path;
		syscallarg(char **) argv;
		syscallarg(char **) envp;
	} */ *uap = v;
	struct sys_execve_args ap;
	caddr_t sg;

	sg = stackgap_init(p, 0);

	SCARG(&ap, path) = (const char *)(u_long)SCARG(uap, path);
	CHECK_ALT_EXIST(p, &sg, SCARG(&ap, path));
	SCARG(&ap, argp) = (char **)(u_long)SCARG(uap, argp);
	SCARG(&ap, envp) = (char **)(u_long)SCARG(uap, envp);

	return netbsd32_execve(p, &ap, retval);
}
#endif

int
svr4_32_sys_time(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_time_args *uap = v;
	int error = 0;
	struct timeval tv;
	struct netbsd32_timeval ntv;

	microtime(&tv);
	ntv.tv_sec = tv.tv_sec;
	ntv.tv_usec = tv.tv_usec;
	if (SCARG(uap, t))
		error = copyout(&ntv.tv_sec, (caddr_t)(u_long)SCARG(uap, t),
				sizeof(ntv.tv_sec));
	*retval = (int) ntv.tv_sec;

	return error;
}


/*
 * Read SVR4-style directory entries.  We suck them into kernel space so
 * that they can be massaged before being copied out to user code.  Like
 * SunOS, we squish out `empty' entries.
 *
 * This is quite ugly, but what do you expect from compatibility code?
 */
int
svr4_32_sys_getdents64(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_getdents64_args *uap = v;
	struct proc *p = l->l_proc;
	struct dirent *bdp;
	struct vnode *vp;
	caddr_t inp, buf;	/* BSD-format */
	int len, reclen;	/* BSD-format */
	caddr_t outp;		/* SVR4-format */
	int resid, svr4_32_reclen;	/* SVR4-format */
	struct file *fp;
	struct uio auio;
	struct iovec aiov;
	struct svr4_32_dirent64 idb;
	off_t off;		/* true file offset */
	int buflen, error, eofflag;
	off_t *cookiebuf = NULL, *cookie;
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
	outp = (char *)(u_long) SCARG(uap, dp);
	resid = SCARG(uap, nbytes);
	if ((len = buflen - auio.uio_resid) == 0)
		goto eof;

	for (cookie = cookiebuf; len > 0; len -= reclen) {
		bdp = (struct dirent *)inp;
		reclen = bdp->d_reclen;
		if (reclen & 3)
			panic("svr4_32_getdents64: bad reclen");
		if (bdp->d_fileno == 0) {
			inp += reclen;	/* it is a hole; squish it out */
			off = *cookie++;
			continue;
		}
		svr4_32_reclen = SVR4_RECLEN(&idb, bdp->d_namlen);
		if (reclen > len || resid < svr4_32_reclen) {
			/* entry too big for buffer, so just stop */
			outp++;
			break;
		}
		off = *cookie++;	/* each entry points to the next */
		/*
		 * Massage in place to make a SVR4-shaped dirent (otherwise
		 * we have to worry about touching user memory outside of
		 * the copyout() call).
		 */
		idb.d_ino = (svr4_32_ino64_t)bdp->d_fileno;
		idb.d_off = (svr4_32_off64_t)off;
		idb.d_reclen = (u_short)svr4_32_reclen;
		strlcpy(idb.d_name, bdp->d_name, sizeof(idb.d_name));
		if ((error = copyout((caddr_t)&idb, outp, svr4_32_reclen)))
			goto out;
		/* advance past this real entry */
		inp += reclen;
		/* advance output past SVR4-shaped entry */
		outp += svr4_32_reclen;
		resid -= svr4_32_reclen;
	}

	/* if we squished out the whole block, try again */
	if (outp == (char *)(u_long) SCARG(uap, dp))
		goto again;
	fp->f_offset = off;	/* update the vnode offset */

eof:
	*retval = SCARG(uap, nbytes) - resid;
out:
	VOP_UNLOCK(vp, 0);
	if (cookiebuf)
		free(cookiebuf, M_TEMP);
	free(buf, M_TEMP);
 out1:
	FILE_UNUSE(fp, p);
	return error;
}


int
svr4_32_sys_getdents(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_getdents_args *uap = v;
	struct proc *p = l->l_proc;
	struct dirent *bdp;
	struct vnode *vp;
	caddr_t inp, buf;	/* BSD-format */
	int len, reclen;	/* BSD-format */
	caddr_t outp;		/* SVR4-format */
	int resid, svr4_reclen;	/* SVR4-format */
	struct file *fp;
	struct uio auio;
	struct iovec aiov;
	struct svr4_32_dirent idb;
	off_t off;		/* true file offset */
	int buflen, error, eofflag;
	off_t *cookiebuf = NULL, *cookie;
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
			panic("svr4_32_getdents: bad reclen");
		off = *cookie++;	/* each entry points to the next */
		if ((off >> 32) != 0) {
			compat_offseterr(vp, "svr4_32_getdents");
			error = EINVAL;
			goto out;
		}
		if (bdp->d_fileno == 0) {
			inp += reclen;	/* it is a hole; squish it out */
			continue;
		}
		svr4_reclen = SVR4_RECLEN(&idb, bdp->d_namlen);
		if (reclen > len || resid < svr4_reclen) {
			/* entry too big for buffer, so just stop */
			outp++;
			break;
		}
		/*
		 * Massage in place to make a SVR4-shaped dirent (otherwise
		 * we have to worry about touching user memory outside of
		 * the copyout() call).
		 */
		idb.d_ino = (svr4_32_ino_t)bdp->d_fileno;
		idb.d_off = (svr4_32_off_t)off;
		idb.d_reclen = (u_short)svr4_reclen;
		strlcpy(idb.d_name, bdp->d_name, sizeof(idb.d_name));
		if ((error = copyout((caddr_t)&idb, outp, svr4_reclen)))
			goto out;
		/* advance past this real entry */
		inp += reclen;
		/* advance output past SVR4-shaped entry */
		outp += svr4_reclen;
		resid -= svr4_reclen;
	}

	/* if we squished out the whole block, try again */
	if (outp == (caddr_t)(u_long)SCARG(uap, buf))
		goto again;
	fp->f_offset = off;	/* update the vnode offset */

eof:
	*retval = SCARG(uap, nbytes) - resid;
out:
	VOP_UNLOCK(vp, 0);
	if (cookiebuf)
		free(cookiebuf, M_TEMP);
	free(buf, M_TEMP);
 out1:
	FILE_UNUSE(fp, p);
	return error;
}


static int
svr4_to_bsd_mmap_flags(f)
	int f;
{
	int type = f & SVR4_MAP_TYPE;
	int nf;

	if (type != MAP_PRIVATE && type != MAP_SHARED)
		return -1;

	nf = f & SVR4_MAP_COPYFLAGS;
	if (f & SVR4_MAP_ANON)
	nf |= MAP_ANON;

	return nf;
}


int
svr4_32_sys_mmap(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_mmap_args	*uap = v;
	struct sys_mmap_args		 mm;
	int				 error;
	/*
         * Verify the arguments.
         */
	if (SCARG(uap, prot) & ~(PROT_READ | PROT_WRITE | PROT_EXEC))
		return EINVAL;	/* XXX still needed? */

	if (SCARG(uap, len) == 0)
		return EINVAL;

	if ((SCARG(&mm, flags) = svr4_to_bsd_mmap_flags(SCARG(uap, flags))) == -1)
		return EINVAL;

	SCARG(&mm, prot) = SCARG(uap, prot);
	SCARG(&mm, len) = SCARG(uap, len);
	SCARG(&mm, fd) = SCARG(uap, fd);
	SCARG(&mm, addr) = (void *)(u_long)SCARG(uap, addr);
	SCARG(&mm, pos) = SCARG(uap, pos);

	error = sys_mmap(l, &mm, retval);
	if ((u_long)*retval > (u_long)UINT_MAX) {
		printf("svr4_32_mmap: retval out of range: 0x%lx",
		       (u_long)*retval);
		/* Should try to recover and return an error here. */
	}
	return (error);
}


int
svr4_32_sys_mmap64(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_mmap64_args	*uap = v;
	struct sys_mmap_args		 mm;
	int				 error;
	/*
         * Verify the arguments.
         */
	if (SCARG(uap, prot) & ~(PROT_READ | PROT_WRITE | PROT_EXEC))
		return EINVAL;	/* XXX still needed? */

	if (SCARG(uap, len) == 0)
		return EINVAL;

	if ((SCARG(&mm, flags) = svr4_to_bsd_mmap_flags(SCARG(uap, flags))) == -1)
		return EINVAL;

	SCARG(&mm, prot) = SCARG(uap, prot);
	SCARG(&mm, len) = SCARG(uap, len);
	SCARG(&mm, fd) = SCARG(uap, fd);
	SCARG(&mm, addr) = (void *)(u_long)SCARG(uap, addr);
	SCARG(&mm, pos) = SCARG(uap, pos);

	error = sys_mmap(l, &mm, retval);
	if ((u_long)*retval > (u_long)UINT_MAX) {
		printf("svr4_32_mmap64: retval out of range: 0x%lx",
		       (u_long)*retval);
		/* Should try to recover and return an error here. */
	}
	return (error);
}


static int
svr4_32_mknod(l, retval, path, mode, dev)
	struct lwp *l;
	register_t *retval;
	const char *path;
	svr4_32_mode_t mode;
	svr4_32_dev_t dev;
{
	caddr_t sg = stackgap_init(l->l_proc, 0);

	CHECK_ALT_CREAT(l->l_proc, &sg, path);

	if (S_ISFIFO(mode)) {
		struct sys_mkfifo_args ap;
		SCARG(&ap, path) = path;
		SCARG(&ap, mode) = mode;
		return sys_mkfifo(l, &ap, retval);
	} else {
		struct sys_mknod_args ap;
		SCARG(&ap, path) = path;
		SCARG(&ap, mode) = mode;
		SCARG(&ap, dev) = dev;
		return sys_mknod(l, &ap, retval);
	}
}


int
svr4_32_sys_mknod(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_mknod_args *uap = v;
	return svr4_32_mknod(l, retval,
			  (caddr_t)(u_long)SCARG(uap, path), SCARG(uap, mode),
			  svr4_32_to_bsd_odev_t(SCARG(uap, dev)));
}


int
svr4_32_sys_xmknod(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_xmknod_args *uap = v;
	return svr4_32_mknod(l, retval,
			  (caddr_t)(u_long)SCARG(uap, path), SCARG(uap, mode),
			  svr4_32_to_bsd_dev_t(SCARG(uap, dev)));
}


int
svr4_32_sys_vhangup(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	return 0;
}


int
svr4_32_sys_sysconfig(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_sysconfig_args *uap = v;
	extern int	maxfiles;

	switch (SCARG(uap, name)) {
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
		*retval = PAGE_SIZE;
		break;
	case SVR4_CONFIG_CLK_TCK:
		*retval = 60;	/* should this be `hz', ie. 100? */
		break;
	case SVR4_CONFIG_XOPEN_VER:
		*retval = 2;	/* XXX: What should that be? */
		break;
	case SVR4_CONFIG_PROF_TCK:
		*retval = 60;	/* XXX: What should that be? */
		break;
	case SVR4_CONFIG_NPROC_CONF:
		*retval = 1;	/* Only one processor for now */
		break;
	case SVR4_CONFIG_NPROC_ONLN:
		*retval = 1;	/* And it better be online */
		break;
	case SVR4_CONFIG_AIO_LISTIO_MAX:
	case SVR4_CONFIG_AIO_MAX:
	case SVR4_CONFIG_AIO_PRIO_DELTA_MAX:
		*retval = 0;	/* No aio support */
		break;
	case SVR4_CONFIG_DELAYTIMER_MAX:
		*retval = 0;	/* No delaytimer support */
		break;
	case SVR4_CONFIG_MQ_OPEN_MAX:
#ifdef SYSVMSG
		*retval = msginfo.msgmni;
#else
		*retval = 0;
#endif
		break;
	case SVR4_CONFIG_MQ_PRIO_MAX:
		*retval = 0;	/* XXX: Don't know */
		break;
	case SVR4_CONFIG_RTSIG_MAX:
		*retval = 0;
		break;
	case SVR4_CONFIG_SEM_NSEMS_MAX:
#ifdef SYSVSEM
		*retval = seminfo.semmni;
#else
		*retval = 0;
#endif
		break;
	case SVR4_CONFIG_SEM_VALUE_MAX:
#ifdef SYSVSEM
		*retval = seminfo.semvmx;
#else
		*retval = 0;
#endif
		break;
	case SVR4_CONFIG_SIGQUEUE_MAX:
		*retval = 0;	/* XXX: Don't know */
		break;
	case SVR4_CONFIG_SIGRT_MIN:
	case SVR4_CONFIG_SIGRT_MAX:
		*retval = 0;	/* No real time signals */
		break;
	case SVR4_CONFIG_TIMER_MAX:
		*retval = 3;	/* XXX: real, virtual, profiling */
		break;
	case SVR4_CONFIG_PHYS_PAGES:
		*retval = uvmexp.free;	/* XXX: free instead of total */
		break;
	case SVR4_CONFIG_AVPHYS_PAGES:
		*retval = uvmexp.active;	/* XXX: active instead of avg */
		break;
	case SVR4_CONFIG_COHERENCY:
		*retval = 0;	/* XXX */
		break;
	case SVR4_CONFIG_SPLIT_CACHE:
		*retval = 0;	/* XXX */
		break;
	case SVR4_CONFIG_ICACHESZ:
		*retval = 256;	/* XXX */
		break;
	case SVR4_CONFIG_DCACHESZ:
		*retval = 256;	/* XXX */
		break;
	case SVR4_CONFIG_ICACHELINESZ:
		*retval = 64;	/* XXX */
		break;
	case SVR4_CONFIG_DCACHELINESZ:
		*retval = 64;	/* XXX */
		break;
	case SVR4_CONFIG_ICACHEBLKSZ:
		*retval = 64;	/* XXX */
		break;
	case SVR4_CONFIG_DCACHEBLKSZ:
		*retval = 64;	/* XXX */
		break;
	case SVR4_CONFIG_DCACHETBLKSZ:
		*retval = 64;	/* XXX */
		break;
	case SVR4_CONFIG_ICACHE_ASSOC:
		*retval = 1;	/* XXX */
		break;
	case SVR4_CONFIG_DCACHE_ASSOC:
		*retval = 1;	/* XXX */
		break;
	case SVR4_CONFIG_MAXPID:
		*retval = PID_MAX;
		break;
	case SVR4_CONFIG_STACK_PROT:
		*retval = PROT_READ|PROT_WRITE|PROT_EXEC;
		break;
	default:
		return EINVAL;
	}
	return 0;
}


/* ARGSUSED */
int
svr4_32_sys_break(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_break_args *uap = v;
	struct proc *p = l->l_proc;
	struct vmspace *vm = p->p_vmspace;
	vaddr_t new, old;
	int error;

	old = (vaddr_t) vm->vm_daddr;
	new = round_page((vaddr_t)SCARG(uap, nsize));

	if (new - old > p->p_rlimit[RLIMIT_DATA].rlim_cur && new > old)
		return ENOMEM;

	old = round_page(old + ctob(vm->vm_dsize));
	DPRINTF(("break(2): dsize = %x ctob %x\n",
		 vm->vm_dsize, ctob(vm->vm_dsize)));

	if (new > old) {
		error = uvm_map(&vm->vm_map, &old, new - old, NULL,
			UVM_UNKNOWN_OFFSET, 0,
           		UVM_MAPFLAG(UVM_PROT_ALL, UVM_PROT_ALL, UVM_INH_COPY, 
			UVM_ADV_NORMAL, 
			UVM_FLAG_AMAPPAD|UVM_FLAG_FIXED|
			UVM_FLAG_OVERLAY|UVM_FLAG_COPYONW)); 
		if (error) {
			uprintf("sbrk: grow failed, return = %d\n", error);
			return error;
		}
		vm->vm_dsize += btoc(new - old);
	} else if (new < old) {
		uvm_deallocate(&vm->vm_map, new, old - new);
		vm->vm_dsize -= btoc(old - new);
	}
	return 0;
}


static __inline clock_t
timeval_to_clock_t(tv)
	struct timeval *tv;
{
	return tv->tv_sec * hz + tv->tv_usec / (1000000 / hz);
}


int
svr4_32_sys_times(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_times_args *uap = v;
	struct proc 		*p = l->l_proc;
	int			 error;
	struct tms		 tms;
	struct timeval		 t;
	struct rusage		*ru;
	struct rusage		 r;
	struct sys_getrusage_args 	 ga;

	caddr_t sg = stackgap_init(p, 0);
	ru = stackgap_alloc(p, &sg, sizeof(struct rusage));

	SCARG(&ga, who) = RUSAGE_SELF;
	SCARG(&ga, rusage) = ru;

	error = sys_getrusage(l, &ga, retval);
	if (error)
		return error;

	if ((error = copyin(ru, &r, sizeof r)) != 0)
		return error;

	tms.tms_utime = timeval_to_clock_t(&r.ru_utime);
	tms.tms_stime = timeval_to_clock_t(&r.ru_stime);

	SCARG(&ga, who) = RUSAGE_CHILDREN;
	error = sys_getrusage(l, &ga, retval);
	if (error)
		return error;

	if ((error = copyin(ru, &r, sizeof r)) != 0)
		return error;

	tms.tms_cutime = timeval_to_clock_t(&r.ru_utime);
	tms.tms_cstime = timeval_to_clock_t(&r.ru_stime);

	microtime(&t);
	*retval = timeval_to_clock_t(&t);

	return copyout(&tms, (caddr_t)(u_long)SCARG(uap, tp), sizeof(tms));
}


int
svr4_32_sys_ulimit(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_ulimit_args *uap = v;
	struct proc *p = l->l_proc;

	switch (SCARG(uap, cmd)) {
	case SVR4_GFILLIM:
		*retval = p->p_rlimit[RLIMIT_FSIZE].rlim_cur / 512;
		if (*retval == -1)
			*retval = 0x7fffffff;
		return 0;

	case SVR4_SFILLIM:
		{
			int error;
			struct sys_setrlimit_args srl;
			struct rlimit krl;
			caddr_t sg = stackgap_init(p, 0);
			struct rlimit *url = (struct rlimit *) 
				stackgap_alloc(p, &sg, sizeof *url);

			krl.rlim_cur = SCARG(uap, newlimit) * 512;
			krl.rlim_max = p->p_rlimit[RLIMIT_FSIZE].rlim_max;

			error = copyout(&krl, url, sizeof(*url));
			if (error)
				return error;

			SCARG(&srl, which) = RLIMIT_FSIZE;
			SCARG(&srl, rlp) = url;

			error = sys_setrlimit(l, &srl, retval);
			if (error)
				return error;

			*retval = p->p_rlimit[RLIMIT_FSIZE].rlim_cur;
			if (*retval == -1)
				*retval = 0x7fffffff;
			return 0;
		}

	case SVR4_GMEMLIM:
		{
			struct vmspace *vm = p->p_vmspace;
			register_t r = p->p_rlimit[RLIMIT_DATA].rlim_cur;

			if (r == -1)
				r = 0x7fffffff;
			r += (long) vm->vm_daddr;
			if (r < 0)
				r = 0x7fffffff;
			*retval = r;
			return 0;
		}

	case SVR4_GDESLIM:
		*retval = p->p_rlimit[RLIMIT_NOFILE].rlim_cur;
		if (*retval == -1)
			*retval = 0x7fffffff;
		return 0;

	default:
		return EINVAL;
	}
}


int
svr4_32_sys_pgrpsys(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_pgrpsys_args *uap = v;
	struct proc *p = l->l_proc;

	switch (SCARG(uap, cmd)) {
	case 1:			/* setpgrp() */
		/*
		 * SVR4 setpgrp() (which takes no arguments) has the
		 * semantics that the session ID is also created anew, so
		 * in almost every sense, setpgrp() is identical to
		 * setsid() for SVR4.  (Under BSD, the difference is that
		 * a setpgid(0,0) will not create a new session.)
		 */
		sys_setsid(l, NULL, retval);
		/*FALLTHROUGH*/

	case 0:			/* getpgrp() */
		*retval = p->p_pgrp->pg_id;
		return 0;

	case 2:			/* getsid(pid) */
		if (SCARG(uap, pid) != 0 &&
		    (p = svr4_32_pfind(SCARG(uap, pid))) == NULL)
			return ESRCH;
		/*
		 * This has already been initialized to the pid of
		 * the session leader.
		 */
		*retval = (register_t) p->p_session->s_sid;
		return 0;

	case 3:			/* setsid() */
		return sys_setsid(l, NULL, retval);

	case 4:			/* getpgid(pid) */

		if (SCARG(uap, pid) != 0 &&
		    (p = svr4_32_pfind(SCARG(uap, pid))) == NULL)
			return ESRCH;

		*retval = (int) p->p_pgrp->pg_id;
		return 0;

	case 5:			/* setpgid(pid, pgid); */
		{
			struct sys_setpgid_args sa;

			SCARG(&sa, pid) = SCARG(uap, pid);
			SCARG(&sa, pgid) = SCARG(uap, pgid);
			return sys_setpgid(l, &sa, retval);
		}

	default:
		return EINVAL;
	}
}

struct svr4_32_hrtcntl_args {
	syscallarg(int) 			cmd;
	syscallarg(int) 			fun;
	syscallarg(int) 			clk;
	syscallarg(svr4_32_hrt_interval_tp)	iv;
	syscallarg(svr4_32_hrt_time_tp)		ti;
};


static int
svr4_32_hrtcntl(p, uap, retval)
	struct proc *p;
	struct svr4_32_hrtcntl_args *uap;
	register_t *retval;
{
	switch (SCARG(uap, fun)) {
	case SVR4_HRT_CNTL_RES:
		DPRINTF(("htrcntl(RES)\n"));
		*retval = SVR4_HRT_USEC;
		return 0;

	case SVR4_HRT_CNTL_TOFD:
		DPRINTF(("htrcntl(TOFD)\n"));
		{
			struct timeval tv;
			svr4_hrt_time_t t;
			if (SCARG(uap, clk) != SVR4_HRT_CLK_STD) {
				DPRINTF(("clk == %d\n", SCARG(uap, clk)));
				return EINVAL;
			}
			if (SCARG(uap, ti) == 0) {
				DPRINTF(("ti NULL\n"));
				return EINVAL;
			}
			microtime(&tv);
			t.h_sec = tv.tv_sec;
			t.h_rem = tv.tv_usec;
			t.h_res = SVR4_HRT_USEC;
			return copyout(&t, (caddr_t)(u_long)SCARG(uap, ti), 
				       sizeof(t));
		}

	case SVR4_HRT_CNTL_START:
		DPRINTF(("htrcntl(START)\n"));
		return ENOSYS;

	case SVR4_HRT_CNTL_GET:
		DPRINTF(("htrcntl(GET)\n"));
		return ENOSYS;
	default:
		DPRINTF(("Bad htrcntl command %d\n", SCARG(uap, fun)));
		return ENOSYS;
	}
}


int
svr4_32_sys_hrtsys(l, v, retval) 
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_hrtsys_args *uap = v;

	switch (SCARG(uap, cmd)) {
	case SVR4_HRT_CNTL:
		return svr4_32_hrtcntl(l->l_proc, (struct svr4_32_hrtcntl_args *) uap,
				    retval);

	case SVR4_HRT_ALRM:
		DPRINTF(("hrtalarm\n"));
		return ENOSYS;

	case SVR4_HRT_SLP:
		DPRINTF(("hrtsleep\n"));
		return ENOSYS;

	case SVR4_HRT_CAN:
		DPRINTF(("hrtcancel\n"));
		return ENOSYS;

	default:
		DPRINTF(("Bad hrtsys command %d\n", SCARG(uap, cmd)));
		return EINVAL;
	}
}


static int
svr4_32_setinfo(p, st, si)
	struct proc *p;
	int st;
	svr4_32_siginfo_tp si;
{
	svr4_32_siginfo_t *s = (svr4_32_siginfo_t *)(u_long)si;
	svr4_32_siginfo_t i;
	int sig;

	memset(&i, 0, sizeof(i));

	i.si_signo = SVR4_SIGCHLD;
	i.si_errno = 0;	/* XXX? */

	if (p) {
		i.si_pid = p->p_pid;
		if (p->p_stat == SZOMB) {
			i.si_stime = p->p_ru->ru_stime.tv_sec;
			i.si_utime = p->p_ru->ru_utime.tv_sec;
		}
		else {
			i.si_stime = p->p_stats->p_ru.ru_stime.tv_sec;
			i.si_utime = p->p_stats->p_ru.ru_utime.tv_sec;
		}
	}

	if (WIFEXITED(st)) {
		i.si_status = WEXITSTATUS(st);
		i.si_code = SVR4_CLD_EXITED;
	} else if (WIFSTOPPED(st)) {
		sig = WSTOPSIG(st);
		if (sig >= 0 && sig < NSIG)
			i.si_status = native_to_svr4_signo[sig];

		if (i.si_status == SVR4_SIGCONT)
			i.si_code = SVR4_CLD_CONTINUED;
		else
			i.si_code = SVR4_CLD_STOPPED;
	} else {
		sig = WTERMSIG(st);
		if (sig >= 0 && sig < NSIG)
			i.si_status = native_to_svr4_signo[sig];

		if (WCOREDUMP(st))
			i.si_code = SVR4_CLD_DUMPED;
		else
			i.si_code = SVR4_CLD_KILLED;
	}

	DPRINTF(("siginfo [pid %ld signo %d code %d errno %d status %d]\n",
		 i.si_pid, i.si_signo, i.si_code, i.si_errno, i.si_status));

	return copyout(&i, s, sizeof(i));
}


int
svr4_32_sys_waitsys(l, v, retval) 
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_waitsys_args *uap = v;
	struct proc *parent = l->l_proc;
	int options, error;
	struct proc *child;

	switch (SCARG(uap, grp)) {
	case SVR4_P_PID:	
		break;

	case SVR4_P_PGID:
		SCARG(uap, id) = -parent->p_pgid;
		break;

	case SVR4_P_ALL:
		SCARG(uap, id) = WAIT_ANY;
		break;

	default:
		return EINVAL;
	}

	DPRINTF(("waitsys(%d, %d, %p, %x)\n", 
	         SCARG(uap, grp), SCARG(uap, id),
		 SCARG(uap, info), SCARG(uap, options)));

	/* Translate options */
	options = 0;
	if (SCARG(uap, options) & SVR4_WNOWAIT)
		options |= WNOWAIT;
	if (SCARG(uap, options) & SVR4_WNOHANG)
		options |= WNOHANG;
	if ((SCARG(uap, options) & (SVR4_WEXITED|SVR4_WTRAPPED)) == 0)
		options |= WNOZOMBIE;
	if (SCARG(uap, options) & (SVR4_WSTOPPED|SVR4_WCONTINUED))
		options |= WUNTRACED;

	error = find_stopped_child(parent, SCARG(uap, id), options, &child);
	if (error != 0)
		return error;
	*retval = 0;
	if (child == NULL)
		return svr4_32_setinfo(NULL, 0, SCARG(uap, info));

	if (child->p_stat == SZOMB) {
		DPRINTF(("found %d\n", child->p_pid));
		error = svr4_32_setinfo(child, child->p_xstat, SCARG(uap,info));
		if (error)
			return error;

		if ((SCARG(uap, options) & SVR4_WNOWAIT)) {
			DPRINTF(("Don't wait\n"));
			return 0;
		}

		proc_free(child);
		return 0;
	}

	DPRINTF(("jobcontrol %d\n", child->p_pid));
	return svr4_32_setinfo(child, W_STOPCODE(child->p_xstat),
					       SCARG(uap, info));
}


static void
bsd_statvfs_to_svr4_32_statvfs(bfs, sfs)
	const struct statvfs *bfs;
	struct svr4_32_statvfs *sfs;
{
	sfs->f_bsize = bfs->f_iosize; /* XXX */
	sfs->f_frsize = bfs->f_bsize;
	sfs->f_blocks = bfs->f_blocks;
	sfs->f_bfree = bfs->f_bfree;
	sfs->f_bavail = bfs->f_bavail;
	sfs->f_files = bfs->f_files;
	sfs->f_ffree = bfs->f_ffree;
	sfs->f_favail = bfs->f_ffree;
	sfs->f_fsid = bfs->f_fsidx.__fsid_val[0];
	memcpy(sfs->f_basetype, bfs->f_fstypename, sizeof(sfs->f_basetype));
	sfs->f_flag = 0;
	if (bfs->f_flag & MNT_RDONLY)
		sfs->f_flag |= SVR4_ST_RDONLY;
	if (bfs->f_flag & MNT_NOSUID)
		sfs->f_flag |= SVR4_ST_NOSUID;
	sfs->f_namemax = MAXNAMLEN;
	memcpy(sfs->f_fstr, bfs->f_fstypename, sizeof(sfs->f_fstr)); /* XXX */
	memset(sfs->f_filler, 0, sizeof(sfs->f_filler));
}


static void
bsd_statvfs_to_svr4_32_statvfs64(bfs, sfs)
	const struct statvfs *bfs;
	struct svr4_32_statvfs64 *sfs;
{
	sfs->f_bsize = bfs->f_iosize; /* XXX */
	sfs->f_frsize = bfs->f_bsize;
	sfs->f_blocks = bfs->f_blocks;
	sfs->f_bfree = bfs->f_bfree;
	sfs->f_bavail = bfs->f_bavail;
	sfs->f_files = bfs->f_files;
	sfs->f_ffree = bfs->f_ffree;
	sfs->f_favail = bfs->f_ffree;
	sfs->f_fsid = bfs->f_fsidx.__fsid_val[0];
	memcpy(sfs->f_basetype, bfs->f_fstypename, sizeof(sfs->f_basetype));
	sfs->f_flag = 0;
	if (bfs->f_flag & MNT_RDONLY)
		sfs->f_flag |= SVR4_ST_RDONLY;
	if (bfs->f_flag & MNT_NOSUID)
		sfs->f_flag |= SVR4_ST_NOSUID;
	sfs->f_namemax = MAXNAMLEN;
	memcpy(sfs->f_fstr, bfs->f_fstypename, sizeof(sfs->f_fstr)); /* XXX */
	memset(sfs->f_filler, 0, sizeof(sfs->f_filler));
}


int
svr4_32_sys_statvfs(l, v, retval) 
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_statvfs_args *uap = v;
	struct proc *p = l->l_proc;
	struct sys_statvfs1_args	fs_args;
	caddr_t sg = stackgap_init(p, 0);
	struct statvfs *fs = stackgap_alloc(p, &sg, sizeof(struct statvfs));
	struct statvfs bfs;
	struct svr4_32_statvfs sfs;
	int error;

	SCARG(&fs_args, path) = (caddr_t)(u_long)SCARG(uap, path);
	CHECK_ALT_EXIST(p, &sg, SCARG(&fs_args, path));
	SCARG(&fs_args, buf) = fs;
	SCARG(&fs_args, flags) = ST_WAIT;

	if ((error = sys_statvfs1(l, &fs_args, retval)) != 0)
		return error;

	if ((error = copyin(fs, &bfs, sizeof(bfs))) != 0)
		return error;

	bsd_statvfs_to_svr4_32_statvfs(&bfs, &sfs);

	return copyout(&sfs, (caddr_t)(u_long)SCARG(uap, fs), sizeof(sfs));
}


int
svr4_32_sys_fstatvfs(l, v, retval) 
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_fstatvfs_args *uap = v;
	struct proc *p = l->l_proc;
	struct sys_fstatvfs1_args	fs_args;
	caddr_t sg = stackgap_init(p, 0);
	struct statvfs *fs = stackgap_alloc(p, &sg, sizeof(struct statvfs));
	struct statvfs bfs;
	struct svr4_32_statvfs sfs;
	int error;

	SCARG(&fs_args, fd) = SCARG(uap, fd);
	SCARG(&fs_args, buf) = fs;
	SCARG(&fs_args, flags) = ST_WAIT;

	if ((error = sys_fstatvfs1(l, &fs_args, retval)) != 0)
		return error;

	if ((error = copyin(fs, &bfs, sizeof(bfs))) != 0)
		return error;

	bsd_statvfs_to_svr4_32_statvfs(&bfs, &sfs);

	return copyout(&sfs, (caddr_t)(u_long)SCARG(uap, fs), sizeof(sfs));
}


int
svr4_32_sys_statvfs64(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_statvfs64_args *uap = v;
	struct sys_statvfs1_args	fs_args;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);
	struct statvfs *fs = stackgap_alloc(p, &sg, sizeof(struct statvfs));
	struct statvfs bfs;
	struct svr4_32_statvfs64 sfs;
	int error;

	SCARG(&fs_args, path) = (caddr_t)(u_long)SCARG(uap, path);
	CHECK_ALT_EXIST(p, &sg, SCARG(&fs_args, path));
	SCARG(&fs_args, buf) = fs;
	SCARG(&fs_args, flags) = ST_WAIT;

	if ((error = sys_statvfs1(l, &fs_args, retval)) != 0)
		return error;

	if ((error = copyin(fs, &bfs, sizeof(bfs))) != 0)
		return error;

	bsd_statvfs_to_svr4_32_statvfs64(&bfs, &sfs);

	return copyout(&sfs, (caddr_t)(u_long)SCARG(uap, fs), sizeof(sfs));
}


int
svr4_32_sys_fstatvfs64(l, v, retval) 
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_fstatvfs64_args *uap = v;
	struct proc *p = l->l_proc;
	struct sys_fstatvfs1_args	fs_args;
	caddr_t sg = stackgap_init(p, 0);
	struct statvfs *fs = stackgap_alloc(p, &sg, sizeof(struct statvfs));
	struct statvfs bfs;
	struct svr4_32_statvfs64 sfs;
	int error;

	SCARG(&fs_args, fd) = SCARG(uap, fd);
	SCARG(&fs_args, buf) = fs;
	SCARG(&fs_args, flags) = ST_WAIT;

	if ((error = sys_fstatvfs1(l, &fs_args, retval)) != 0)
		return error;

	if ((error = copyin(fs, &bfs, sizeof(bfs))) != 0)
		return error;

	bsd_statvfs_to_svr4_32_statvfs64(&bfs, &sfs);

	return copyout(&sfs, (caddr_t)(u_long)SCARG(uap, fs), sizeof(sfs));
}


int
svr4_32_sys_alarm(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_alarm_args *uap = v;
	struct proc *p = l->l_proc;
	int error;
        struct itimerval *ntp, *otp, tp;
	struct sys_setitimer_args sa;
	caddr_t sg = stackgap_init(p, 0);

        ntp = stackgap_alloc(p, &sg, sizeof(struct itimerval));
        otp = stackgap_alloc(p, &sg, sizeof(struct itimerval));

        timerclear(&tp.it_interval);
        tp.it_value.tv_sec = SCARG(uap, sec);
        tp.it_value.tv_usec = 0;

	if ((error = copyout(&tp, ntp, sizeof(tp))) != 0)
		return error;

	SCARG(&sa, which) = ITIMER_REAL;
	SCARG(&sa, itv) = ntp;
	SCARG(&sa, oitv) = otp;

        if ((error = sys_setitimer(l, &sa, retval)) != 0)
		return error;

	if ((error = copyin(otp, &tp, sizeof(tp))) != 0)
		return error;

        if (tp.it_value.tv_usec)
                tp.it_value.tv_sec++;

        *retval = (register_t) tp.it_value.tv_sec;

        return 0;
}


int
svr4_32_sys_gettimeofday(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_gettimeofday_args *uap = v;

	if (SCARG(uap, tp)) {
		struct timeval atv;

		microtime(&atv);
		return copyout(&atv, (caddr_t)(u_long)SCARG(uap, tp), sizeof (atv));
	}

	return 0;
}


int
svr4_32_sys_facl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_facl_args *uap = v;

	*retval = 0;

	switch (SCARG(uap, cmd)) {
	case SVR4_SYS_SETACL:
		/* We don't support acls on any filesystem */
		return ENOSYS;

	case SVR4_SYS_GETACL:
		return copyout(retval, &SCARG(uap, num),
		    sizeof(SCARG(uap, num)));

	case SVR4_SYS_GETACLCNT:
		return 0;

	default:
		return EINVAL;
	}
}


int
svr4_32_sys_acl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	return svr4_32_sys_facl(l, v, retval);	/* XXX: for now the same */
}


int
svr4_32_sys_auditsys(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	/*
	 * XXX: Big brother is *not* watching.
	 */
	return 0;
}


int
svr4_32_sys_memcntl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_memcntl_args *uap = v;
	switch (SCARG(uap, cmd)) {
	case SVR4_MC_SYNC:
		{
			struct sys___msync13_args msa;

			SCARG(&msa, addr) = (void *)(u_long)SCARG(uap, addr);
			SCARG(&msa, len) = SCARG(uap, len);
			SCARG(&msa, flags) = (int)(u_long)SCARG(uap, arg);

			return sys___msync13(l, &msa, retval);
		}
	case SVR4_MC_ADVISE:
		{
			struct sys_madvise_args maa;

			SCARG(&maa, addr) = (void *)(u_long)SCARG(uap, addr);
			SCARG(&maa, len) = SCARG(uap, len);
			SCARG(&maa, behav) = (int)(u_long)SCARG(uap, arg);

			return sys_madvise(l, &maa, retval);
		}
	case SVR4_MC_LOCK:
	case SVR4_MC_UNLOCK:
	case SVR4_MC_LOCKAS:
	case SVR4_MC_UNLOCKAS:
		return EOPNOTSUPP;
	default:
		return ENOSYS;
	}
}


int
svr4_32_sys_nice(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_nice_args *uap = v;
	struct sys_setpriority_args ap;
	int error;

	SCARG(&ap, which) = PRIO_PROCESS;
	SCARG(&ap, who) = 0;
	SCARG(&ap, prio) = SCARG(uap, prio);

	if ((error = sys_setpriority(l, &ap, retval)) != 0)
		return error;

	if ((error = sys_getpriority(l, &ap, retval)) != 0)
		return error;

	return 0;
}


int
svr4_32_sys_resolvepath(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_resolvepath_args *uap = v;
	struct proc *p = l->l_proc;
	struct nameidata nd;
	int error;
	size_t len;

	NDINIT(&nd, LOOKUP, NOFOLLOW | SAVENAME, UIO_USERSPACE,
	    (const char *)(u_long)SCARG(uap, path), p);

	if ((error = namei(&nd)) != 0)
		return error;

	if ((error = copyoutstr(nd.ni_cnd.cn_pnbuf,
	    (caddr_t)(u_long)SCARG(uap, buf),
	    SCARG(uap, bufsiz), &len)) != 0)
		goto bad;

	*retval = len;
bad:
	vrele(nd.ni_vp);
	PNBUF_PUT(nd.ni_cnd.cn_pnbuf);
	return error;
}
