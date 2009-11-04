/*	$NetBSD: svr4_misc.c,v 1.147 2009/11/04 21:23:03 rmind Exp $	 */

/*-
 * Copyright (c) 1994, 2008 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: svr4_misc.c,v 1.147 2009/11/04 21:23:03 rmind Exp $");

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
#include <sys/vfs_syscalls.h>
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

#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

static int svr4_to_bsd_mmap_flags(int);

static inline clock_t timeval_to_clock_t(struct timeval *);
static void svr4_setinfo(int, struct rusage *, int, svr4_siginfo_t *);

struct svr4_hrtcntl_args;
static int svr4_hrtcntl(struct lwp *, const struct svr4_hrtcntl_args *,
    register_t *);
#define svr4_pfind(pid) p_find((pid), PFIND_UNLOCK | PFIND_ZOMBIE)

static int svr4_mknod(struct lwp *, register_t *, const char *,
    svr4_mode_t, svr4_dev_t);

int
svr4_sys_wait(struct lwp *l, const struct svr4_sys_wait_args *uap,
    register_t *retval)
{
	int error, st, sig, pid = WAIT_ANY;

	error = do_sys_wait(&pid, &st, 0, NULL);

	retval[0] = pid;
	if (pid == 0)
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
		error = copyout(&st, SCARG(uap, status), sizeof(st));

	return error;
}


int
svr4_sys_execv(struct lwp *l, const struct svr4_sys_execv_args *uap, register_t *retval)
{
	/* {
		syscallarg(char *) path;
		syscallarg(char **) argv;
	} */
	struct sys_execve_args ap;

	SCARG(&ap, path) = SCARG(uap, path);
	SCARG(&ap, argp) = SCARG(uap, argp);
	SCARG(&ap, envp) = NULL;

	return sys_execve(l, &ap, retval);
}


int
svr4_sys_execve(struct lwp *l, const struct svr4_sys_execve_args *uap, register_t *retval)
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

	return sys_execve(l, &ap, retval);
}


int
svr4_sys_time(struct lwp *l, const struct svr4_sys_time_args *uap, register_t *retval)
{
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
svr4_sys_getdents64(struct lwp *l, const struct svr4_sys_getdents64_args *uap, register_t *retval)
{
	struct dirent *bdp;
	struct vnode *vp;
	char *inp, *tbuf;	/* BSD-format */
	int len, reclen;	/* BSD-format */
	char *outp;		/* SVR4-format */
	int resid, svr4_reclen;	/* SVR4-format */
	struct file *fp;
	struct uio auio;
	struct iovec aiov;
	struct svr4_dirent64 idb;
	off_t off;		/* true file offset */
	int buflen, error, eofflag;
	off_t *cookiebuf = NULL, *cookie;
	int ncookies;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
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
	tbuf = malloc(buflen, M_TEMP, M_WAITOK);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	off = fp->f_offset;
again:
	aiov.iov_base = tbuf;
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

	inp = tbuf;
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
			if (cookie)
				off = *cookie++;
			else
				off += reclen;
			continue;
		}
		svr4_reclen = SVR4_RECLEN(&idb, bdp->d_namlen);
		if (reclen > len || resid < svr4_reclen) {
			/* entry too big for buffer, so just stop */
			outp++;
			break;
		}
		if (cookie)
			off = *cookie++; /* each entry points to the next */
		else
			off += reclen;
		/*
		 * Massage in place to make a SVR4-shaped dirent (otherwise
		 * we have to worry about touching user memory outside of
		 * the copyout() call).
		 */
		idb.d_ino = (svr4_ino64_t)bdp->d_fileno;
		idb.d_off = (svr4_off64_t)off;
		idb.d_reclen = (u_short)svr4_reclen;
		strlcpy(idb.d_name, bdp->d_name, sizeof(idb.d_name));
		if ((error = copyout((void *)&idb, outp, svr4_reclen)))
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
	free(tbuf, M_TEMP);
 out1:
 	fd_putfile(SCARG(uap, fd));
	return error;
}


int
svr4_sys_getdents(struct lwp *l, const struct svr4_sys_getdents_args *uap, register_t *retval)
{
	struct dirent *bdp;
	struct vnode *vp;
	char *inp, *tbuf;	/* BSD-format */
	int len, reclen;	/* BSD-format */
	char *outp;		/* SVR4-format */
	int resid, svr4_reclen;	/* SVR4-format */
	struct file *fp;
	struct uio auio;
	struct iovec aiov;
	struct svr4_dirent idb;
	off_t off;		/* true file offset */
	int buflen, error, eofflag;
	off_t *cookiebuf = NULL, *cookie;
	int ncookies;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
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
	tbuf = malloc(buflen, M_TEMP, M_WAITOK);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	off = fp->f_offset;
again:
	aiov.iov_base = tbuf;
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

	inp = tbuf;
	outp = SCARG(uap, buf);
	resid = SCARG(uap, nbytes);
	if ((len = buflen - auio.uio_resid) == 0)
		goto eof;

	for (cookie = cookiebuf; len > 0; len -= reclen) {
		bdp = (struct dirent *)inp;
		reclen = bdp->d_reclen;
		if (reclen & 3)
			panic("svr4_getdents: bad reclen");
		if (cookie)
			off = *cookie++; /* each entry points to the next */
		else
			off += reclen;
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
		strlcpy(idb.d_name, bdp->d_name, sizeof(idb.d_name));
		if ((error = copyout((void *)&idb, outp, svr4_reclen)))
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
	free(tbuf, M_TEMP);
 out1:
	fd_putfile(SCARG(uap, fd));
	return error;
}


static int
svr4_to_bsd_mmap_flags(int f)
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
svr4_sys_mmap(struct lwp *l, const struct svr4_sys_mmap_args *uap, register_t *retval)
{
	struct sys_mmap_args	 mm;
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

	return sys_mmap(l, &mm, retval);
}


int
svr4_sys_mmap64(struct lwp *l, const struct svr4_sys_mmap64_args *uap, register_t *retval)
{
	struct sys_mmap_args	 mm;
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

	return sys_mmap(l, &mm, retval);
}


static int
svr4_mknod(struct lwp *l, register_t *retval, const char *path, svr4_mode_t mode, svr4_dev_t dev)
{
	if (S_ISFIFO(mode)) {
		struct sys_mkfifo_args ap;
		SCARG(&ap, path) = path;
		SCARG(&ap, mode) = mode;
		return sys_mkfifo(l, &ap, retval);
	} else {
		return do_sys_mknod(l, path, mode, dev, retval, UIO_USERSPACE);
	}
}


int
svr4_sys_mknod(struct lwp *l, const struct svr4_sys_mknod_args *uap, register_t *retval)
{
	return svr4_mknod(l, retval,
			  SCARG(uap, path), SCARG(uap, mode),
			  svr4_to_bsd_odev_t(SCARG(uap, dev)));
}


int
svr4_sys_xmknod(struct lwp *l, const struct svr4_sys_xmknod_args *uap, register_t *retval)
{
	return svr4_mknod(l, retval,
			  SCARG(uap, path), SCARG(uap, mode),
			  svr4_to_bsd_dev_t(SCARG(uap, dev)));
}


int
svr4_sys_vhangup(struct lwp *l, const void *v, register_t *retval)
{
	return 0;
}


int
svr4_sys_sysconfig(struct lwp *l, const struct svr4_sys_sysconfig_args *uap, register_t *retval)
{
	int active;

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
		uvm_estimatepageable(&active, NULL);
		*retval = active;	/* XXX: active instead of avg */
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
svr4_sys_break(struct lwp *l, const struct svr4_sys_break_args *uap, register_t *retval)
{
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


static inline clock_t
timeval_to_clock_t(struct timeval *tv)
{
	return tv->tv_sec * hz + tv->tv_usec / (1000000 / hz);
}


int
svr4_sys_times(struct lwp *l, const struct svr4_sys_times_args *uap, register_t *retval)
{
	struct tms		 tms;
	struct timeval		 t;
	struct rusage		 ru, *rup;
	struct proc		 *p = l->l_proc;

	ru = p->p_stats->p_ru;
	mutex_enter(p->p_lock);
	calcru(p, &ru.ru_utime, &ru.ru_stime, NULL, NULL);
	rulwps(p, &ru);
	mutex_exit(p->p_lock);

	tms.tms_utime = timeval_to_clock_t(&ru.ru_utime);
	tms.tms_stime = timeval_to_clock_t(&ru.ru_stime);

	rup = &p->p_stats->p_cru;
	tms.tms_cutime = timeval_to_clock_t(&rup->ru_utime);
	tms.tms_cstime = timeval_to_clock_t(&rup->ru_stime);

	microtime(&t);
	*retval = timeval_to_clock_t(&t);

	return copyout(&tms, SCARG(uap, tp), sizeof(tms));
}


int
svr4_sys_ulimit(struct lwp *l, const struct svr4_sys_ulimit_args *uap, register_t *retval)
{
	struct proc *p = l->l_proc;
	int error;
	struct rlimit krl;
	register_t r;

	switch (SCARG(uap, cmd)) {
	case SVR4_GFILLIM:
		r = p->p_rlimit[RLIMIT_FSIZE].rlim_cur / 512;
		break;

	case SVR4_SFILLIM:
		krl.rlim_cur = SCARG(uap, newlimit) * 512;
		krl.rlim_max = p->p_rlimit[RLIMIT_FSIZE].rlim_max;

		error = dosetrlimit(l, l->l_proc, RLIMIT_FSIZE, &krl);
		if (error)
			return error;

		r = p->p_rlimit[RLIMIT_FSIZE].rlim_cur;
		break;

	case SVR4_GMEMLIM:
		r = p->p_rlimit[RLIMIT_DATA].rlim_cur;
		if (r > 0x7fffffff)
			r = 0x7fffffff;
		r += (long)p->p_vmspace->vm_daddr;
		break;

	case SVR4_GDESLIM:
		r = p->p_rlimit[RLIMIT_NOFILE].rlim_cur;
		break;

	default:
		return EINVAL;
	}

	*retval = r > 0x7fffffff ? 0x7fffffff : r;
	return 0;
}


int
svr4_sys_pgrpsys(struct lwp *l, const struct svr4_sys_pgrpsys_args *uap, register_t *retval)
{
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
		mutex_enter(proc_lock);
		*retval = p->p_pgrp->pg_id;
		mutex_exit(proc_lock);
		return 0;

	case 2:			/* getsid(pid) */
		mutex_enter(proc_lock);
		if (SCARG(uap, pid) != 0 &&
		    (p = p_find(SCARG(uap, pid), PFIND_LOCKED | PFIND_ZOMBIE)) == NULL) {
			mutex_exit(proc_lock);
			return ESRCH;
		}
		/*
		 * This has already been initialized to the pid of
		 * the session leader.
		 */
		*retval = (register_t) p->p_session->s_sid;
		mutex_exit(proc_lock);
		return 0;

	case 3:			/* setsid() */
		return sys_setsid(l, NULL, retval);

	case 4:			/* getpgid(pid) */
		mutex_enter(proc_lock);
		if (SCARG(uap, pid) != 0 &&
		    (p = p_find(SCARG(uap, pid), PFIND_LOCKED | PFIND_ZOMBIE)) == NULL) {
			mutex_exit(proc_lock);
			return ESRCH;
		}

		*retval = (int) p->p_pgrp->pg_id;
		mutex_exit(proc_lock);
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

struct svr4_hrtcntl_args {
	syscallarg(int) 			cmd;
	syscallarg(int) 			fun;
	syscallarg(int) 			clk;
	syscallarg(svr4_hrt_interval_t *)	iv;
	syscallarg(svr4_hrt_time_t *)		ti;
};


static int
svr4_hrtcntl(struct lwp *l, const struct svr4_hrtcntl_args *uap,
    register_t *retval)
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
svr4_sys_hrtsys(struct lwp *l, const struct svr4_sys_hrtsys_args *uap, register_t *retval)
{

	switch (SCARG(uap, cmd)) {
	case SVR4_HRT_CNTL:
		return svr4_hrtcntl(l, (const struct svr4_hrtcntl_args *) uap,
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


static void
svr4_setinfo(int pid, struct rusage *ru, int st, svr4_siginfo_t *s)
{
	int sig;

	memset(s, 0, sizeof(*s));

	s->si_signo = SVR4_SIGCHLD;
	s->si_errno = 0;	/* XXX? */

	if (pid != 0) {
		s->si_pid = pid;
		s->si_stime = ru->ru_stime.tv_sec;
		s->si_utime = ru->ru_utime.tv_sec;
	}

	if (WIFEXITED(st)) {
		s->si_status = WEXITSTATUS(st);
		s->si_code = SVR4_CLD_EXITED;
	} else if (WIFSTOPPED(st)) {
		sig = WSTOPSIG(st);
		if (sig >= 0 && sig < NSIG)
			s->si_status = native_to_svr4_signo[sig];

		if (s->si_status == SVR4_SIGCONT)
			s->si_code = SVR4_CLD_CONTINUED;
		else
			s->si_code = SVR4_CLD_STOPPED;
	} else {
		sig = WTERMSIG(st);
		if (sig >= 0 && sig < NSIG)
			s->si_status = native_to_svr4_signo[sig];

		if (WCOREDUMP(st))
			s->si_code = SVR4_CLD_DUMPED;
		else
			s->si_code = SVR4_CLD_KILLED;
	}

	DPRINTF(("siginfo [pid %ld signo %d code %d errno %d status %d]\n",
		(long) s->si_pid,
		s->si_signo, s->si_code, s->si_errno, s->si_status));
}


int
svr4_sys_waitsys(struct lwp *l, const struct svr4_sys_waitsys_args *uap, register_t *retval)
{
	int options, status;
	int error;
	int was_zombie;
	struct rusage ru;
	svr4_siginfo_t i;
	int id = SCARG(uap, id);

	switch (SCARG(uap, grp)) {
	case SVR4_P_PID:
		break;

	case SVR4_P_PGID:
		mutex_enter(proc_lock);
		id = -l->l_proc->p_pgid;
		mutex_exit(proc_lock);
		break;

	case SVR4_P_ALL:
		id = WAIT_ANY;
		break;

	default:
		return EINVAL;
	}

	/* Translate options */
	options = WOPTSCHECKED;
	if (SCARG(uap, options) & SVR4_WNOWAIT)
		options |= WNOWAIT;
	if (SCARG(uap, options) & SVR4_WNOHANG)
		options |= WNOHANG;
	if ((SCARG(uap, options) & (SVR4_WEXITED|SVR4_WTRAPPED)) == 0)
		options |= WNOZOMBIE;
	if (SCARG(uap, options) & (SVR4_WSTOPPED|SVR4_WCONTINUED))
		options |= WUNTRACED;

	DPRINTF(("waitsys(%d, %d, %p, %x)\n",
	         SCARG(uap, grp), id,
		 SCARG(uap, info), SCARG(uap, options)));

	error = do_sys_wait(l, &id, &status, options, &ru,
	    &was_zombie);

	retval[0] = id;
	if (error != 0)
		return error;

	svr4_setinfo(id, &ru, status, &i);
	return copyout(&i, SCARG(uap, info), sizeof(i));
}


static int
svr4_copyout_statvfs(const struct statvfs *bfs, struct svr4_statvfs *sufs)
{
	struct svr4_statvfs *sfs = malloc(sizeof(*sfs), M_TEMP, M_WAITOK);
	int error;

	sfs->f_bsize = bfs->f_bsize;
	sfs->f_frsize = bfs->f_frsize;
	sfs->f_blocks = bfs->f_blocks;
	sfs->f_bfree = bfs->f_bfree;
	sfs->f_bavail = bfs->f_bavail;
	sfs->f_files = bfs->f_files;
	sfs->f_ffree = bfs->f_ffree;
	sfs->f_favail = bfs->f_favail;
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

	error = copyout(sfs, sufs, sizeof(*sfs));

	free(sfs, M_TEMP);
	return error;
}


static int
svr4_copyout_statvfs64(const struct statvfs *bfs, struct svr4_statvfs64 *sufs)
{
	struct svr4_statvfs64 *sfs = malloc(sizeof(*sfs), M_TEMP, M_WAITOK);
	int error;

	sfs->f_bsize = bfs->f_bsize;
	sfs->f_frsize = bfs->f_frsize;
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

	error = copyout(sfs, sufs, sizeof(*sfs));

	free(sfs, M_TEMP);
	return error;
}


int
svr4_sys_statvfs(struct lwp *l, const struct svr4_sys_statvfs_args *uap, register_t *retval)
{
	struct statvfs *sb;
	int error;

	sb =  STATVFSBUF_GET();
	error = do_sys_pstatvfs(l, SCARG(uap, path), ST_WAIT, sb);
	if (error == 0)
		error = svr4_copyout_statvfs(sb, SCARG(uap, fs));
	STATVFSBUF_PUT(sb);
	return error;
}


int
svr4_sys_fstatvfs(struct lwp *l, const struct svr4_sys_fstatvfs_args *uap, register_t *retval)
{
	struct statvfs *sb;
	int error;

	sb =  STATVFSBUF_GET();
	error = do_sys_fstatvfs(l, SCARG(uap, fd), ST_WAIT, sb);
	if (error == 0)
		error = svr4_copyout_statvfs(sb, SCARG(uap, fs));
	STATVFSBUF_PUT(sb);
	return error;
}


int
svr4_sys_statvfs64(struct lwp *l, const struct svr4_sys_statvfs64_args *uap, register_t *retval)
{
	struct statvfs *sb;
	int error;

	sb =  STATVFSBUF_GET();
	error = do_sys_pstatvfs(l, SCARG(uap, path), ST_WAIT, sb);
	if (error == 0)
		error = svr4_copyout_statvfs64(sb, SCARG(uap, fs));
	STATVFSBUF_PUT(sb);
	return error;
}


int
svr4_sys_fstatvfs64(struct lwp *l, const struct svr4_sys_fstatvfs64_args *uap, register_t *retval)
{
	struct statvfs *sb;
	int error;

	sb =  STATVFSBUF_GET();
	error = do_sys_fstatvfs(l, SCARG(uap, fd), ST_WAIT, sb);
	if (error == 0)
		error = svr4_copyout_statvfs64(sb, SCARG(uap, fs));
	STATVFSBUF_PUT(sb);
	return error;
}

int
svr4_sys_alarm(struct lwp *l, const struct svr4_sys_alarm_args *uap, register_t *retval)
{
        struct itimerval tp;

	dogetitimer(l->l_proc, ITIMER_REAL, &tp);
        if (tp.it_value.tv_usec)
                tp.it_value.tv_sec++;
        *retval = (register_t)tp.it_value.tv_sec;

        timerclear(&tp.it_interval);
        tp.it_value.tv_sec = SCARG(uap, sec);
        tp.it_value.tv_usec = 0;

        return dosetitimer(l->l_proc, ITIMER_REAL, &tp);
}


int
svr4_sys_gettimeofday(struct lwp *l, const struct svr4_sys_gettimeofday_args *uap, register_t *retval)
{

	if (SCARG(uap, tp)) {
		struct timeval atv;

		microtime(&atv);
		return copyout(&atv, SCARG(uap, tp), sizeof (atv));
	}

	return 0;
}


int
svr4_sys_facl(struct lwp *l, const struct svr4_sys_facl_args *uap, register_t *retval)
{

	*retval = 0;

	switch (SCARG(uap, cmd)) {
	case SVR4_SYS_SETACL:
		/* We don't support acls on any filesystem */
		return ENOSYS;

	case SVR4_SYS_GETACL:
#if 1
		return ENOSYS;
#else
		/*
		 * XXX This code is completly borked, no idea what it was
		 * trying to do.
		 */
		return copyout(retval, &SCARG(uap, num),
		    sizeof(SCARG(uap, num)));
#endif

	case SVR4_SYS_GETACLCNT:
		return 0;

	default:
		return EINVAL;
	}
}


int
svr4_sys_acl(struct lwp *l, const struct svr4_sys_acl_args *uap, register_t *retval)
{
	return svr4_sys_facl(l, (const void *)uap, retval);	/* XXX: for now the same */
}


int
svr4_sys_auditsys(struct lwp *l, const struct svr4_sys_auditsys_args *uap, register_t *retval)
{
	/*
	 * XXX: Big brother is *not* watching.
	 */
	return 0;
}


int
svr4_sys_memcntl(struct lwp *l, const struct svr4_sys_memcntl_args *uap, register_t *retval)
{
	switch (SCARG(uap, cmd)) {
	case SVR4_MC_SYNC:
		{
			struct sys___msync13_args msa;

			SCARG(&msa, addr) = SCARG(uap, addr);
			SCARG(&msa, len) = SCARG(uap, len);
			SCARG(&msa, flags) = (int)(u_long)SCARG(uap, arg);

			return sys___msync13(l, &msa, retval);
		}
	case SVR4_MC_ADVISE:
		{
			struct sys_madvise_args maa;

			SCARG(&maa, addr) = SCARG(uap, addr);
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
svr4_sys_nice(struct lwp *l, const struct svr4_sys_nice_args *uap, register_t *retval)
{
	struct sys_setpriority_args ap;
	int error;

	SCARG(&ap, which) = PRIO_PROCESS;
	SCARG(&ap, who) = 0;
	SCARG(&ap, prio) = SCARG(uap, prio);

	if ((error = sys_setpriority(l, &ap, retval)) != 0)
		return error;

	if ((error = sys_getpriority(l, (const void *)&ap, retval)) != 0)
		return error;

	return 0;
}


int
svr4_sys_resolvepath(struct lwp *l, const struct svr4_sys_resolvepath_args *uap, register_t *retval)
{
	struct nameidata nd;
	int error;
	size_t len;

	NDINIT(&nd, LOOKUP, NOFOLLOW | SAVENAME | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path));

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
