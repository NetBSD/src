/*	$NetBSD: svr4_misc.c,v 1.91.2.3 2002/06/23 17:44:46 jdolecek Exp $	 */

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
__KERNEL_RCSID(0, "$NetBSD: svr4_misc.c,v 1.91.2.3 2002/06/23 17:44:46 jdolecek Exp $");

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
#include <sys/syscallargs.h>

#include <miscfs/specfs/specdev.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_signal.h>
#include <compat/svr4/svr4_lwp.h>
#include <compat/svr4/svr4_ucontext.h>
#include <compat/svr4/svr4_syscallargs.h>
#include <compat/svr4/svr4_util.h>
#include <compat/svr4/svr4_time.h>
#include <compat/svr4/svr4_dirent.h>
#include <compat/svr4/svr4_ulimit.h>
#include <compat/svr4/svr4_hrt.h>
#include <compat/svr4/svr4_wait.h>
#include <compat/svr4/svr4_statvfs.h>
#include <compat/svr4/svr4_sysconfig.h>
#include <compat/svr4/svr4_acl.h>
#include <compat/svr4/svr4_mman.h>

#include <machine/cpu.h>

#include <uvm/uvm_extern.h>

static int svr4_to_bsd_mmap_flags __P((int));

static __inline clock_t timeval_to_clock_t __P((struct timeval *));
static int svr4_setinfo	__P((struct proc *, int, svr4_siginfo_t *));

struct svr4_hrtcntl_args;
static int svr4_hrtcntl	__P((struct proc *, struct svr4_hrtcntl_args *,
    register_t *));
static void bsd_statfs_to_svr4_statvfs __P((const struct statfs *,
    struct svr4_statvfs *));
static void bsd_statfs_to_svr4_statvfs64 __P((const struct statfs *,
    struct svr4_statvfs64 *));
static struct proc *svr4_pfind __P((pid_t pid));

static int svr4_mknod __P((struct proc *, register_t *, const char *,
    svr4_mode_t, svr4_dev_t));

int
svr4_sys_wait(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_wait_args *uap = v;
	struct sys_wait4_args w4;
	int error;
	size_t sz = sizeof(*SCARG(&w4, status));
	int st, sig;

	SCARG(&w4, rusage) = NULL;
	SCARG(&w4, options) = 0;

	if (SCARG(uap, status) == NULL) {
		caddr_t sg = stackgap_init(p, 0);

		SCARG(&w4, status) = stackgap_alloc(p, &sg, sz);
	}
	else
		SCARG(&w4, status) = SCARG(uap, status);

	SCARG(&w4, pid) = WAIT_ANY;

	if ((error = sys_wait4(p, &w4, retval)) != 0)
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
		if ((error = copyout(&st, SCARG(uap, status), sizeof(st))) != 0)
			return error;

	return 0;
}


int
svr4_sys_execv(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_execv_args /* {
		syscallarg(char *) path;
		syscallarg(char **) argv;
	} */ *uap = v;
	struct sys_execve_args ap;
	caddr_t sg;

	sg = stackgap_init(p, 0);
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	SCARG(&ap, path) = SCARG(uap, path);
	SCARG(&ap, argp) = SCARG(uap, argp);
	SCARG(&ap, envp) = NULL;

	return sys_execve(p, &ap, retval);
}


int
svr4_sys_execve(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_execve_args /* {
		syscallarg(const char *) path;
		syscallarg(char **) argv;
		syscallarg(char **) envp;
	} */ *uap = v;
	struct sys_execve_args ap;
	caddr_t sg;

	sg = stackgap_init(p, 0);
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	SCARG(&ap, path) = SCARG(uap, path);
	SCARG(&ap, argp) = SCARG(uap, argp);
	SCARG(&ap, envp) = SCARG(uap, envp);

	return sys_execve(p, &ap, retval);
}


int
svr4_sys_time(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_time_args *uap = v;
	int error = 0;
	struct timeval tv;

	microtime(&tv);
	if (SCARG(uap, t))
		error = copyout(&tv.tv_sec, SCARG(uap, t),
				sizeof(*(SCARG(uap, t))));
	*retval = (int) tv.tv_sec;

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
svr4_sys_getdents64(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_getdents64_args *uap = v;
	struct dirent *bdp;
	struct vnode *vp;
	caddr_t inp, buf;	/* BSD-format */
	int len, reclen;	/* BSD-format */
	caddr_t outp;		/* SVR4-format */
	int resid, svr4_reclen;	/* SVR4-format */
	struct file *fp;
	struct uio auio;
	struct iovec aiov;
	struct svr4_dirent64 idb;
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
	outp = (char *) SCARG(uap, dp);
	resid = SCARG(uap, nbytes);
	if ((len = buflen - auio.uio_resid) == 0)
		goto eof;

	for (cookie = cookiebuf; len > 0; len -= reclen) {
		bdp = (struct dirent *)inp;
		reclen = bdp->d_reclen;
		if (reclen & 3)
			panic("svr4_getdents64: bad reclen");
		if (bdp->d_fileno == 0) {
			inp += reclen;	/* it is a hole; squish it out */
			off = *cookie++;
			continue;
		}
		svr4_reclen = SVR4_RECLEN(&idb, bdp->d_namlen);
		if (reclen > len || resid < svr4_reclen) {
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
		idb.d_ino = (svr4_ino64_t)bdp->d_fileno;
		idb.d_off = (svr4_off64_t)off;
		idb.d_reclen = (u_short)svr4_reclen;
		strcpy(idb.d_name, bdp->d_name);
		if ((error = copyout((caddr_t)&idb, outp, svr4_reclen)))
			goto out;
		/* advance past this real entry */
		inp += reclen;
		/* advance output past SVR4-shaped entry */
		outp += svr4_reclen;
		resid -= svr4_reclen;
	}

	/* if we squished out the whole block, try again */
	if (outp == (char *) SCARG(uap, dp))
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
svr4_sys_getdents(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_getdents_args *uap = v;
	struct dirent *bdp;
	struct vnode *vp;
	caddr_t inp, buf;	/* BSD-format */
	int len, reclen;	/* BSD-format */
	caddr_t outp;		/* SVR4-format */
	int resid, svr4_reclen;	/* SVR4-format */
	struct file *fp;
	struct uio auio;
	struct iovec aiov;
	struct svr4_dirent idb;
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
	outp = SCARG(uap, buf);
	resid = SCARG(uap, nbytes);
	if ((len = buflen - auio.uio_resid) == 0)
		goto eof;

	for (cookie = cookiebuf; len > 0; len -= reclen) {
		bdp = (struct dirent *)inp;
		reclen = bdp->d_reclen;
		if (reclen & 3)
			panic("svr4_getdents: bad reclen");
		off = *cookie++;	/* each entry points to the next */
		if ((off >> 32) != 0) {
			compat_offseterr(vp, "svr4_getdents");
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
		idb.d_ino = (svr4_ino_t)bdp->d_fileno;
		idb.d_off = (svr4_off_t)off;
		idb.d_reclen = (u_short)svr4_reclen;
		strcpy(idb.d_name, bdp->d_name);
		if ((error = copyout((caddr_t)&idb, outp, svr4_reclen)))
			goto out;
		/* advance past this real entry */
		inp += reclen;
		/* advance output past SVR4-shaped entry */
		outp += svr4_reclen;
		resid -= svr4_reclen;
	}

	/* if we squished out the whole block, try again */
	if (outp == SCARG(uap, buf))
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
svr4_sys_mmap(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_mmap_args	*uap = v;
	struct sys_mmap_args	 mm;
	void		*rp;
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
	SCARG(&mm, addr) = SCARG(uap, addr);
	SCARG(&mm, pos) = SCARG(uap, pos);

	rp = (void *) round_page((vaddr_t)p->p_vmspace->vm_daddr + MAXDSIZ);
	if ((SCARG(&mm, flags) & MAP_FIXED) == 0 &&
	    SCARG(&mm, addr) != 0 && SCARG(&mm, addr) < rp)
		SCARG(&mm, addr) = rp;

	return sys_mmap(p, &mm, retval);
}


int
svr4_sys_mmap64(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_mmap64_args	*uap = v;
	struct sys_mmap_args	 mm;
	void		*rp;
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
	SCARG(&mm, addr) = SCARG(uap, addr);
	SCARG(&mm, pos) = SCARG(uap, pos);

	rp = (void *) round_page((vaddr_t)p->p_vmspace->vm_daddr + MAXDSIZ);
	if ((SCARG(&mm, flags) & MAP_FIXED) == 0 &&
	    SCARG(&mm, addr) != 0 && SCARG(&mm, addr) < rp)
		SCARG(&mm, addr) = rp;

	return sys_mmap(p, &mm, retval);
}


static int
svr4_mknod(p, retval, path, mode, dev)
	struct proc *p;
	register_t *retval;
	const char *path;
	svr4_mode_t mode;
	svr4_dev_t dev;
{
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_CREAT(p, &sg, path);

	if (S_ISFIFO(mode)) {
		struct sys_mkfifo_args ap;
		SCARG(&ap, path) = path;
		SCARG(&ap, mode) = mode;
		return sys_mkfifo(p, &ap, retval);
	} else {
		struct sys_mknod_args ap;
		SCARG(&ap, path) = path;
		SCARG(&ap, mode) = mode;
		SCARG(&ap, dev) = dev;
		return sys_mknod(p, &ap, retval);
	}
}


int
svr4_sys_mknod(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_mknod_args *uap = v;
	return svr4_mknod(p, retval,
			  SCARG(uap, path), SCARG(uap, mode),
			  svr4_to_bsd_odev_t(SCARG(uap, dev)));
}


int
svr4_sys_xmknod(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_xmknod_args *uap = v;
	return svr4_mknod(p, retval,
			  SCARG(uap, path), SCARG(uap, mode),
			  svr4_to_bsd_dev_t(SCARG(uap, dev)));
}


int
svr4_sys_vhangup(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	return 0;
}


int
svr4_sys_sysconfig(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_sysconfig_args *uap = v;
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
		*retval = NBPG;
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
svr4_sys_break(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_break_args *uap = v;
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
			uprintf("sbrk: grow failed, error = %d\n", error);
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
svr4_sys_times(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_times_args *uap = v;
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

	error = sys_getrusage(p, &ga, retval);
	if (error)
		return error;

	if ((error = copyin(ru, &r, sizeof r)) != 0)
		return error;

	tms.tms_utime = timeval_to_clock_t(&r.ru_utime);
	tms.tms_stime = timeval_to_clock_t(&r.ru_stime);

	SCARG(&ga, who) = RUSAGE_CHILDREN;
	error = sys_getrusage(p, &ga, retval);
	if (error)
		return error;

	if ((error = copyin(ru, &r, sizeof r)) != 0)
		return error;

	tms.tms_cutime = timeval_to_clock_t(&r.ru_utime);
	tms.tms_cstime = timeval_to_clock_t(&r.ru_stime);

	microtime(&t);
	*retval = timeval_to_clock_t(&t);

	return copyout(&tms, SCARG(uap, tp), sizeof(tms));
}


int
svr4_sys_ulimit(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_ulimit_args *uap = v;

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

			error = sys_setrlimit(p, &srl, retval);
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


static struct proc *
svr4_pfind(pid)
	pid_t pid;
{
	struct proc *p = NULL;

	proclist_lock_read();

	/* look in the live processes */
	if ((p = pfind(pid)) != NULL)
		goto out;

	/* look in the zombies */
	for (p = zombproc.lh_first; p != 0; p = p->p_list.le_next)
		if (p->p_pid == pid)
			goto out;

 out:
	proclist_unlock_read();
	return p;
}


int
svr4_sys_pgrpsys(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_pgrpsys_args *uap = v;

	switch (SCARG(uap, cmd)) {
	case 1:			/* setpgrp() */
		/*
		 * SVR4 setpgrp() (which takes no arguments) has the
		 * semantics that the session ID is also created anew, so
		 * in almost every sense, setpgrp() is identical to
		 * setsid() for SVR4.  (Under BSD, the difference is that
		 * a setpgid(0,0) will not create a new session.)
		 */
		sys_setsid(p, NULL, retval);
		/*FALLTHROUGH*/

	case 0:			/* getpgrp() */
		*retval = p->p_pgrp->pg_id;
		return 0;

	case 2:			/* getsid(pid) */
		if (SCARG(uap, pid) != 0 &&
		    (p = svr4_pfind(SCARG(uap, pid))) == NULL)
			return ESRCH;
		/*
		 * This has already been initialized to the pid of
		 * the session leader.
		 */
		*retval = (register_t) p->p_session->s_sid;
		return 0;

	case 3:			/* setsid() */
		return sys_setsid(p, NULL, retval);

	case 4:			/* getpgid(pid) */

		if (SCARG(uap, pid) != 0 &&
		    (p = svr4_pfind(SCARG(uap, pid))) == NULL)
			return ESRCH;

		*retval = (int) p->p_pgrp->pg_id;
		return 0;

	case 5:			/* setpgid(pid, pgid); */
		{
			struct sys_setpgid_args sa;

			SCARG(&sa, pid) = SCARG(uap, pid);
			SCARG(&sa, pgid) = SCARG(uap, pgid);
			return sys_setpgid(p, &sa, retval);
		}

	default:
		return EINVAL;
	}
}

struct svr4_hrtcntl_args {
	syscallarg(int) 			cmd;
	syscallarg(int) 			fun;
	syscallarg(int) 			clk;
	syscallarg(svr4_hrt_interval_t *)	iv;
	syscallarg(svr4_hrt_time_t *)		ti;
};


static int
svr4_hrtcntl(p, uap, retval)
	struct proc *p;
	struct svr4_hrtcntl_args *uap;
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
			if (SCARG(uap, ti) == NULL) {
				DPRINTF(("ti NULL\n"));
				return EINVAL;
			}
			microtime(&tv);
			t.h_sec = tv.tv_sec;
			t.h_rem = tv.tv_usec;
			t.h_res = SVR4_HRT_USEC;
			return copyout(&t, SCARG(uap, ti), sizeof(t));
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
svr4_sys_hrtsys(p, v, retval) 
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_hrtsys_args *uap = v;

	switch (SCARG(uap, cmd)) {
	case SVR4_HRT_CNTL:
		return svr4_hrtcntl(p, (struct svr4_hrtcntl_args *) uap,
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
svr4_setinfo(p, st, s)
	struct proc *p;
	int st;
	svr4_siginfo_t *s;
{
	svr4_siginfo_t i;
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
		(long) i.si_pid,
		i.si_signo, i.si_code, i.si_errno, i.si_status));

	return copyout(&i, s, sizeof(i));
}


int
svr4_sys_waitsys(p, v, retval) 
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_waitsys_args *uap = v;
	int nfound, error, s;
	struct proc *q, *t;


	switch (SCARG(uap, grp)) {
	case SVR4_P_PID:	
		break;

	case SVR4_P_PGID:
		SCARG(uap, id) = -p->p_pgid;
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

loop:
	nfound = 0;
	for (q = p->p_children.lh_first; q != 0; q = q->p_sibling.le_next) {
		if (SCARG(uap, id) != WAIT_ANY &&
		    q->p_pid != SCARG(uap, id) &&
		    q->p_pgid != -SCARG(uap, id)) {
			DPRINTF(("pid %d pgid %d != %d\n", q->p_pid,
				 q->p_pgid, SCARG(uap, id)));
			continue;
		}
		nfound++;
		if (q->p_stat == SZOMB && 
		    ((SCARG(uap, options) & (SVR4_WEXITED|SVR4_WTRAPPED)))) {
			*retval = 0;
			DPRINTF(("found %d\n", q->p_pid));
			if ((error = svr4_setinfo(q, q->p_xstat,
						  SCARG(uap, info))) != 0)
				return error;


		        if ((SCARG(uap, options) & SVR4_WNOWAIT)) {
				DPRINTF(("Don't wait\n"));
				return 0;
			}

			/*
			 * If we got the child via ptrace(2) or procfs, and
			 * the parent is different (meaning the process was
			 * attached, rather than run as a child), then we need
			 * to give it back to the old parent, and send the
			 * parent a SIGCHLD.  The rest of the cleanup will be
			 * done when the old parent waits on the child.
			 */
			if ((q->p_flag & P_TRACED) &&
			    q->p_oppid != q->p_pptr->p_pid) {
				t = pfind(q->p_oppid);
				proc_reparent(q, t ? t : initproc);
				q->p_oppid = 0;
				q->p_flag &= ~(P_TRACED|P_WAITED|P_FSTRACE);
				psignal(q->p_pptr, SIGCHLD);
				wakeup((caddr_t)q->p_pptr);
				return (0);
			}
			q->p_xstat = 0;
			ruadd(&p->p_stats->p_cru, q->p_ru);
			pool_put(&rusage_pool, q->p_ru);

			/*
			 * Finally finished with old proc entry.
			 * Unlink it from its process group and free it.
			 */
			leavepgrp(q);

			s = proclist_lock_write();
			LIST_REMOVE(q, p_list);	/* off zombproc */
			proclist_unlock_write(s);

			LIST_REMOVE(q, p_sibling);

			/*
			 * Decrement the count of procs running with this uid.
			 */
			(void)chgproccnt(q->p_cred->p_ruid, -1);

			/*
			 * Free up credentials.
			 */
			if (--q->p_cred->p_refcnt == 0) {
				crfree(q->p_cred->pc_ucred);
				pool_put(&pcred_pool, q->p_cred);
			}

			/*
			 * Release reference to text vnode
			 */
			if (q->p_textvp)
				vrele(q->p_textvp);

			pool_put(&proc_pool, q);
			nprocs--;
			return 0;
		}
		if (q->p_stat == SSTOP && (q->p_flag & P_WAITED) == 0 &&
		    (q->p_flag & P_TRACED ||
		     (SCARG(uap, options) & (SVR4_WSTOPPED|SVR4_WCONTINUED)))) {
			DPRINTF(("jobcontrol %d\n", q->p_pid));
		        if (((SCARG(uap, options) & SVR4_WNOWAIT)) == 0)
				q->p_flag |= P_WAITED;
			*retval = 0;
			return svr4_setinfo(q, W_STOPCODE(q->p_xstat),
					    SCARG(uap, info));
		}
	}

	if (nfound == 0)
		return ECHILD;

	if (SCARG(uap, options) & SVR4_WNOHANG) {
		*retval = 0;
		if ((error = svr4_setinfo(NULL, 0, SCARG(uap, info))) != 0)
			return error;
		return 0;
	}

	if ((error = tsleep((caddr_t)p, PWAIT | PCATCH, "svr4_wait", 0)) != 0)
		return error;
	goto loop;
}


static void
bsd_statfs_to_svr4_statvfs(bfs, sfs)
	const struct statfs *bfs;
	struct svr4_statvfs *sfs;
{
	sfs->f_bsize = bfs->f_iosize; /* XXX */
	sfs->f_frsize = bfs->f_bsize;
	sfs->f_blocks = bfs->f_blocks;
	sfs->f_bfree = bfs->f_bfree;
	sfs->f_bavail = bfs->f_bavail;
	sfs->f_files = bfs->f_files;
	sfs->f_ffree = bfs->f_ffree;
	sfs->f_favail = bfs->f_ffree;
	sfs->f_fsid = bfs->f_fsid.val[0];
	memcpy(sfs->f_basetype, bfs->f_fstypename, sizeof(sfs->f_basetype));
	sfs->f_flag = 0;
	if (bfs->f_flags & MNT_RDONLY)
		sfs->f_flag |= SVR4_ST_RDONLY;
	if (bfs->f_flags & MNT_NOSUID)
		sfs->f_flag |= SVR4_ST_NOSUID;
	sfs->f_namemax = MAXNAMLEN;
	memcpy(sfs->f_fstr, bfs->f_fstypename, sizeof(sfs->f_fstr)); /* XXX */
	memset(sfs->f_filler, 0, sizeof(sfs->f_filler));
}


static void
bsd_statfs_to_svr4_statvfs64(bfs, sfs)
	const struct statfs *bfs;
	struct svr4_statvfs64 *sfs;
{
	sfs->f_bsize = bfs->f_iosize; /* XXX */
	sfs->f_frsize = bfs->f_bsize;
	sfs->f_blocks = bfs->f_blocks;
	sfs->f_bfree = bfs->f_bfree;
	sfs->f_bavail = bfs->f_bavail;
	sfs->f_files = bfs->f_files;
	sfs->f_ffree = bfs->f_ffree;
	sfs->f_favail = bfs->f_ffree;
	sfs->f_fsid = bfs->f_fsid.val[0];
	memcpy(sfs->f_basetype, bfs->f_fstypename, sizeof(sfs->f_basetype));
	sfs->f_flag = 0;
	if (bfs->f_flags & MNT_RDONLY)
		sfs->f_flag |= SVR4_ST_RDONLY;
	if (bfs->f_flags & MNT_NOSUID)
		sfs->f_flag |= SVR4_ST_NOSUID;
	sfs->f_namemax = MAXNAMLEN;
	memcpy(sfs->f_fstr, bfs->f_fstypename, sizeof(sfs->f_fstr)); /* XXX */
	memset(sfs->f_filler, 0, sizeof(sfs->f_filler));
}


int
svr4_sys_statvfs(p, v, retval) 
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_statvfs_args *uap = v;
	struct sys_statfs_args	fs_args;
	caddr_t sg = stackgap_init(p, 0);
	struct statfs *fs = stackgap_alloc(p, &sg, sizeof(struct statfs));
	struct statfs bfs;
	struct svr4_statvfs sfs;
	int error;

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	SCARG(&fs_args, path) = SCARG(uap, path);
	SCARG(&fs_args, buf) = fs;

	if ((error = sys_statfs(p, &fs_args, retval)) != 0)
		return error;

	if ((error = copyin(fs, &bfs, sizeof(bfs))) != 0)
		return error;

	bsd_statfs_to_svr4_statvfs(&bfs, &sfs);

	return copyout(&sfs, SCARG(uap, fs), sizeof(sfs));
}


int
svr4_sys_fstatvfs(p, v, retval) 
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_fstatvfs_args *uap = v;
	struct sys_fstatfs_args	fs_args;
	caddr_t sg = stackgap_init(p, 0);
	struct statfs *fs = stackgap_alloc(p, &sg, sizeof(struct statfs));
	struct statfs bfs;
	struct svr4_statvfs sfs;
	int error;

	SCARG(&fs_args, fd) = SCARG(uap, fd);
	SCARG(&fs_args, buf) = fs;

	if ((error = sys_fstatfs(p, &fs_args, retval)) != 0)
		return error;

	if ((error = copyin(fs, &bfs, sizeof(bfs))) != 0)
		return error;

	bsd_statfs_to_svr4_statvfs(&bfs, &sfs);

	return copyout(&sfs, SCARG(uap, fs), sizeof(sfs));
}


int
svr4_sys_statvfs64(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_statvfs64_args *uap = v;
	struct sys_statfs_args	fs_args;
	caddr_t sg = stackgap_init(p, 0);
	struct statfs *fs = stackgap_alloc(p, &sg, sizeof(struct statfs));
	struct statfs bfs;
	struct svr4_statvfs64 sfs;
	int error;

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	SCARG(&fs_args, path) = SCARG(uap, path);
	SCARG(&fs_args, buf) = fs;

	if ((error = sys_statfs(p, &fs_args, retval)) != 0)
		return error;

	if ((error = copyin(fs, &bfs, sizeof(bfs))) != 0)
		return error;

	bsd_statfs_to_svr4_statvfs64(&bfs, &sfs);

	return copyout(&sfs, SCARG(uap, fs), sizeof(sfs));
}


int
svr4_sys_fstatvfs64(p, v, retval) 
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_fstatvfs64_args *uap = v;
	struct sys_fstatfs_args	fs_args;
	caddr_t sg = stackgap_init(p, 0);
	struct statfs *fs = stackgap_alloc(p, &sg, sizeof(struct statfs));
	struct statfs bfs;
	struct svr4_statvfs64 sfs;
	int error;

	SCARG(&fs_args, fd) = SCARG(uap, fd);
	SCARG(&fs_args, buf) = fs;

	if ((error = sys_fstatfs(p, &fs_args, retval)) != 0)
		return error;

	if ((error = copyin(fs, &bfs, sizeof(bfs))) != 0)
		return error;

	bsd_statfs_to_svr4_statvfs64(&bfs, &sfs);

	return copyout(&sfs, SCARG(uap, fs), sizeof(sfs));
}


int
svr4_sys_alarm(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_alarm_args *uap = v;
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

        if ((error = sys_setitimer(p, &sa, retval)) != 0)
		return error;

	if ((error = copyin(otp, &tp, sizeof(tp))) != 0)
		return error;

        if (tp.it_value.tv_usec)
                tp.it_value.tv_sec++;

        *retval = (register_t) tp.it_value.tv_sec;

        return 0;
}


int
svr4_sys_gettimeofday(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_gettimeofday_args *uap = v;

	if (SCARG(uap, tp)) {
		struct timeval atv;

		microtime(&atv);
		return copyout(&atv, SCARG(uap, tp), sizeof (atv));
	}

	return 0;
}


int
svr4_sys_facl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_facl_args *uap = v;

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
svr4_sys_acl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	return svr4_sys_facl(p, v, retval);	/* XXX: for now the same */
}


int
svr4_sys_auditsys(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	/*
	 * XXX: Big brother is *not* watching.
	 */
	return 0;
}


int
svr4_sys_memcntl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_memcntl_args *uap = v;
	switch (SCARG(uap, cmd)) {
	case SVR4_MC_SYNC:
		{
			struct sys___msync13_args msa;

			SCARG(&msa, addr) = SCARG(uap, addr);
			SCARG(&msa, len) = SCARG(uap, len);
			SCARG(&msa, flags) = (int)(u_long)SCARG(uap, arg);

			return sys___msync13(p, &msa, retval);
		}
	case SVR4_MC_ADVISE:
		{
			struct sys_madvise_args maa;

			SCARG(&maa, addr) = SCARG(uap, addr);
			SCARG(&maa, len) = SCARG(uap, len);
			SCARG(&maa, behav) = (int)(u_long)SCARG(uap, arg);

			return sys_madvise(p, &maa, retval);
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
svr4_sys_nice(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_nice_args *uap = v;
	struct sys_setpriority_args ap;
	int error;

	SCARG(&ap, which) = PRIO_PROCESS;
	SCARG(&ap, who) = 0;
	SCARG(&ap, prio) = SCARG(uap, prio);

	if ((error = sys_setpriority(p, &ap, retval)) != 0)
		return error;

	if ((error = sys_getpriority(p, &ap, retval)) != 0)
		return error;

	return 0;
}


int
svr4_sys_resolvepath(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_sys_resolvepath_args *uap = v;
	struct nameidata nd;
	int error;
	size_t len;

	NDINIT(&nd, LOOKUP, NOFOLLOW | SAVENAME, UIO_USERSPACE,
	    SCARG(uap, path), p);

	if ((error = namei(&nd)) != 0)
		return error;

	if ((error = copyoutstr(nd.ni_cnd.cn_pnbuf, SCARG(uap, buf),
	    SCARG(uap, bufsiz), &len)) != 0)
		goto bad;

	*retval = len;
bad:
	vrele(nd.ni_vp);
	PNBUF_PUT(nd.ni_cnd.cn_pnbuf);
	return error;
}
