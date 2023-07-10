/*	$NetBSD: sys_descrip.c,v 1.48 2023/07/10 02:31:55 christos Exp $	*/

/*-
 * Copyright (c) 2008, 2020 The NetBSD Foundation, Inc.
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
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)kern_descrip.c	8.8 (Berkeley) 2/14/95
 */

/*
 * System calls on descriptors.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sys_descrip.c,v 1.48 2023/07/10 02:31:55 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/namei.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/kmem.h>
#include <sys/pool.h>
#include <sys/syslog.h>
#include <sys/unistd.h>
#include <sys/resourcevar.h>
#include <sys/conf.h>
#include <sys/event.h>
#include <sys/kauth.h>
#include <sys/atomic.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <uvm/uvm_readahead.h>

/*
 * Duplicate a file descriptor.
 */
int
sys_dup(struct lwp *l, const struct sys_dup_args *uap, register_t *retval)
{
	/* {
		syscallarg(int)	fd;
	} */
	int error, newfd, oldfd;
	file_t *fp;

	oldfd = SCARG(uap, fd);

	if ((fp = fd_getfile(oldfd)) == NULL) {
		return EBADF;
	}
	error = fd_dup(fp, 0, &newfd, false);
	fd_putfile(oldfd);
	*retval = newfd;
	return error;
}

/*
 * Duplicate a file descriptor to a particular value.
 */
int
dodup(struct lwp *l, int from, int to, int flags, register_t *retval)
{
	int error;
	file_t *fp;

	if ((fp = fd_getfile(from)) == NULL)
		return EBADF;
	mutex_enter(&fp->f_lock);
	fp->f_count++;
	mutex_exit(&fp->f_lock);
	fd_putfile(from);

	if ((u_int)to >= curproc->p_rlimit[RLIMIT_NOFILE].rlim_cur ||
	    (u_int)to >= maxfiles)
		error = EBADF;
	else if (from == to)
		error = 0;
	else
		error = fd_dup2(fp, to, flags);
	closef(fp);
	*retval = to;

	return error;
}

int
sys_dup3(struct lwp *l, const struct sys_dup3_args *uap, register_t *retval)
{
	/* {
		syscallarg(int)	from;
		syscallarg(int)	to;
		syscallarg(int)	flags;
	} */
	return dodup(l, SCARG(uap, from), SCARG(uap, to), SCARG(uap, flags),
	    retval);
}

int
sys_dup2(struct lwp *l, const struct sys_dup2_args *uap, register_t *retval)
{
	/* {
		syscallarg(int)	from;
		syscallarg(int)	to;
	} */
	return dodup(l, SCARG(uap, from), SCARG(uap, to), 0, retval);
}

/*
 * fcntl call which is being passed to the file's fs.
 */
static int
fcntl_forfs(int fd, file_t *fp, int cmd, void *arg)
{
	int		error;
	u_int		size;
	void		*data, *memp;
#define STK_PARAMS	128
	char		stkbuf[STK_PARAMS];

	if ((fp->f_flag & (FREAD | FWRITE)) == 0)
		return (EBADF);

	/*
	 * Interpret high order word to find amount of data to be
	 * copied to/from the user's address space.
	 */
	size = (size_t)F_PARAM_LEN(cmd);
	if (size > F_PARAM_MAX)
		return (EINVAL);
	memp = NULL;
	if (size > sizeof(stkbuf)) {
		memp = kmem_alloc(size, KM_SLEEP);
		data = memp;
	} else
		data = stkbuf;
	if (cmd & F_FSIN) {
		if (size) {
			error = copyin(arg, data, size);
			if (error) {
				if (memp)
					kmem_free(memp, size);
				return (error);
			}
		} else
			*(void **)data = arg;
	} else if ((cmd & F_FSOUT) != 0 && size != 0) {
		/*
		 * Zero the buffer so the user always
		 * gets back something deterministic.
		 */
		memset(data, 0, size);
	} else if (cmd & F_FSVOID)
		*(void **)data = arg;


	error = (*fp->f_ops->fo_fcntl)(fp, cmd, data);

	/*
	 * Copy any data to user, size was
	 * already set and checked above.
	 */
	if (error == 0 && (cmd & F_FSOUT) && size)
		error = copyout(data, arg, size);
	if (memp)
		kmem_free(memp, size);
	return (error);
}

int
do_fcntl_lock(int fd, int cmd, struct flock *fl)
{
	struct file *fp = NULL;
	proc_t *p;
	int (*fo_advlock)(struct file *, void *, int, struct flock *, int);
	int error, flg;

	if ((fp = fd_getfile(fd)) == NULL) {
		error = EBADF;
		goto out;
	}
	if ((fo_advlock = fp->f_ops->fo_advlock) == NULL) {
		error = EINVAL;
		goto out;
	}

	flg = F_POSIX;
	p = curproc;

	switch (cmd) {
	case F_SETLKW:
		flg |= F_WAIT;
		/* Fall into F_SETLK */

		/* FALLTHROUGH */
	case F_SETLK:
		switch (fl->l_type) {
		case F_RDLCK:
			if ((fp->f_flag & FREAD) == 0) {
				error = EBADF;
				break;
			}
			if ((p->p_flag & PK_ADVLOCK) == 0) {
				mutex_enter(p->p_lock);
				p->p_flag |= PK_ADVLOCK;
				mutex_exit(p->p_lock);
			}
			error = (*fo_advlock)(fp, p, F_SETLK, fl, flg);
			break;

		case F_WRLCK:
			if ((fp->f_flag & FWRITE) == 0) {
				error = EBADF;
				break;
			}
			if ((p->p_flag & PK_ADVLOCK) == 0) {
				mutex_enter(p->p_lock);
				p->p_flag |= PK_ADVLOCK;
				mutex_exit(p->p_lock);
			}
			error = (*fo_advlock)(fp, p, F_SETLK, fl, flg);
			break;

		case F_UNLCK:
			error = (*fo_advlock)(fp, p, F_UNLCK, fl, F_POSIX);
			break;

		default:
			error = EINVAL;
			break;
		}
		break;

	case F_GETLK:
		if (fl->l_type != F_RDLCK &&
		    fl->l_type != F_WRLCK &&
		    fl->l_type != F_UNLCK) {
			error = EINVAL;
			break;
		}
		error = (*fo_advlock)(fp, p, F_GETLK, fl, F_POSIX);
		break;

	default:
		error = EINVAL;
		break;
	}

out:	if (fp)
		fd_putfile(fd);
	return error;
}

/*
 * The file control system call.
 */
int
sys_fcntl(struct lwp *l, const struct sys_fcntl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int)		fd;
		syscallarg(int)		cmd;
		syscallarg(void *)	arg;
	} */
	int fd, i, tmp, error, cmd, newmin;
	filedesc_t *fdp;
	fdtab_t *dt;
	file_t *fp;
	char *kpath;
	struct flock fl;
	bool cloexec = false;

	fd = SCARG(uap, fd);
	cmd = SCARG(uap, cmd);
	fdp = l->l_fd;
	error = 0;

	switch (cmd) {
	case F_CLOSEM:
		if (fd < 0)
			return EBADF;
		while ((i = fdp->fd_lastfile) >= fd) {
			if (fd_getfile(i) == NULL) {
				/* Another thread has updated. */
				continue;
			}
			fd_close(i);
		}
		return 0;

	case F_MAXFD:
		*retval = fdp->fd_lastfile;
		return 0;

	case F_SETLKW:
	case F_SETLK:
	case F_GETLK:
		error = copyin(SCARG(uap, arg), &fl, sizeof(fl));
		if (error)
			return error;
		error = do_fcntl_lock(fd, cmd, &fl);
		if (cmd == F_GETLK && error == 0)
			error = copyout(&fl, SCARG(uap, arg), sizeof(fl));
		return error;

	default:
		/* Handled below */
		break;
	}

	if ((fp = fd_getfile(fd)) == NULL)
		return EBADF;

	if ((cmd & F_FSCTL)) {
		error = fcntl_forfs(fd, fp, cmd, SCARG(uap, arg));
		fd_putfile(fd);
		return error;
	}

	switch (cmd) {
	case F_DUPFD_CLOEXEC:
		cloexec = true;
		/*FALLTHROUGH*/
	case F_DUPFD:
		newmin = (long)SCARG(uap, arg);
		if ((u_int)newmin >=
		    l->l_proc->p_rlimit[RLIMIT_NOFILE].rlim_cur ||
		    (u_int)newmin >= maxfiles) {
			fd_putfile(fd);
			return EINVAL;
		}
		error = fd_dup(fp, newmin, &i, cloexec);
		*retval = i;
		break;

	case F_GETFD:
		dt = atomic_load_consume(&fdp->fd_dt);
		*retval = dt->dt_ff[fd]->ff_exclose;
		break;

	case F_SETFD:
		fd_set_exclose(l, fd,
		    ((long)SCARG(uap, arg) & FD_CLOEXEC) != 0);
		break;

	case F_GETNOSIGPIPE:
		*retval = (fp->f_flag & FNOSIGPIPE) != 0;
		break;

	case F_SETNOSIGPIPE:
		if (SCARG(uap, arg))
			atomic_or_uint(&fp->f_flag, FNOSIGPIPE);
		else
			atomic_and_uint(&fp->f_flag, ~FNOSIGPIPE);
		*retval = 0;
		break;

	case F_GETFL:
		*retval = OFLAGS(fp->f_flag);
		break;

	case F_SETFL:
		/* XXX not guaranteed to be atomic. */
		tmp = FFLAGS((long)SCARG(uap, arg)) & FCNTLFLAGS;
		error = (*fp->f_ops->fo_fcntl)(fp, F_SETFL, &tmp);
		if (error)
			break;
		i = tmp ^ fp->f_flag;
		if (i & FNONBLOCK) {
			int flgs = tmp & FNONBLOCK;
			error = (*fp->f_ops->fo_ioctl)(fp, FIONBIO, &flgs);
			if (error) {
				(*fp->f_ops->fo_fcntl)(fp, F_SETFL,
				    &fp->f_flag);
				break;
			}
		}
		if (i & FASYNC) {
			int flgs = tmp & FASYNC;
			error = (*fp->f_ops->fo_ioctl)(fp, FIOASYNC, &flgs);
			if (error) {
				if (i & FNONBLOCK) {
					tmp = fp->f_flag & FNONBLOCK;
					(void)(*fp->f_ops->fo_ioctl)(fp,
						FIONBIO, &tmp);
				}
				(*fp->f_ops->fo_fcntl)(fp, F_SETFL,
				    &fp->f_flag);
				break;
			}
		}
		fp->f_flag = (fp->f_flag & ~FCNTLFLAGS) | tmp;
		break;

	case F_GETOWN:
		error = (*fp->f_ops->fo_ioctl)(fp, FIOGETOWN, &tmp);
		*retval = tmp;
		break;

	case F_SETOWN:
		tmp = (int)(uintptr_t) SCARG(uap, arg);
		error = (*fp->f_ops->fo_ioctl)(fp, FIOSETOWN, &tmp);
		break;

	case F_GETPATH:
		kpath = PNBUF_GET();

		/* vnodes need extra context, so are handled separately */
		if (fp->f_type == DTYPE_VNODE)
			error = vnode_to_path(kpath, MAXPATHLEN, fp->f_vnode,
			    l, l->l_proc);
		else
			error = (*fp->f_ops->fo_fcntl)(fp, F_GETPATH, kpath);

		if (error == 0)
			error = copyoutstr(kpath, SCARG(uap, arg), MAXPATHLEN,
			    NULL);

		PNBUF_PUT(kpath);
		break;

	case F_ADD_SEALS:
		tmp = (int)(uintptr_t) SCARG(uap, arg);
		error = (*fp->f_ops->fo_fcntl)(fp, F_ADD_SEALS, &tmp);
		break;

	case F_GET_SEALS:
		error = (*fp->f_ops->fo_fcntl)(fp, F_GET_SEALS, &tmp);
		*retval = tmp;
		break;

	default:
		error = EINVAL;
	}

	fd_putfile(fd);
	return (error);
}

/*
 * Close a file descriptor.
 */
int
sys_close(struct lwp *l, const struct sys_close_args *uap, register_t *retval)
{
	/* {
		syscallarg(int)	fd;
	} */
	int error;
	int fd = SCARG(uap, fd);

	if (fd_getfile(fd) == NULL) {
		return EBADF;
	}

	error = fd_close(fd);
	if (error == ERESTART) {
#ifdef DIAGNOSTIC
		printf("%s[%d]: close(%d) returned ERESTART\n",
		    l->l_proc->p_comm, (int)l->l_proc->p_pid, fd);
#endif
		error = EINTR;
	}

	return error;
}

/*
 * Return status information about a file descriptor.
 * Common function for compat code.
 */
int
do_sys_fstat(int fd, struct stat *sb)
{
	file_t *fp;
	int error;

	if ((fp = fd_getfile(fd)) == NULL) {
		return EBADF;
	}
	error = (*fp->f_ops->fo_stat)(fp, sb);
	fd_putfile(fd);

	return error;
}

/*
 * Return status information about a file descriptor.
 */
int
sys___fstat50(struct lwp *l, const struct sys___fstat50_args *uap,
	      register_t *retval)
{
	/* {
		syscallarg(int)			fd;
		syscallarg(struct stat *)	sb;
	} */
	struct stat sb;
	int error;

	error = do_sys_fstat(SCARG(uap, fd), &sb);
	if (error == 0) {
		error = copyout(&sb, SCARG(uap, sb), sizeof(sb));
	}
	return error;
}

/*
 * Return pathconf information about a file descriptor.
 */
int
sys_fpathconf(struct lwp *l, const struct sys_fpathconf_args *uap,
	      register_t *retval)
{
	/* {
		syscallarg(int)	fd;
		syscallarg(int)	name;
	} */
	int fd, name, error;
	file_t *fp;

	fd = SCARG(uap, fd);
	name = SCARG(uap, name);
	error = 0;

	if ((fp = fd_getfile(fd)) == NULL)
		return EBADF;
	if (fp->f_ops->fo_fpathconf == NULL)
		error = EOPNOTSUPP;
	else
		error = (*fp->f_ops->fo_fpathconf)(fp, name, retval);
	fd_putfile(fd);
	return error;
}

/*
 * Apply an advisory lock on a file descriptor.
 *
 * Just attempt to get a record lock of the requested type on
 * the entire file (l_whence = SEEK_SET, l_start = 0, l_len = 0).
 */
/* ARGSUSED */
int
sys_flock(struct lwp *l, const struct sys_flock_args *uap, register_t *retval)
{
	/* {
		syscallarg(int)	fd;
		syscallarg(int)	how;
	} */
	int fd, how, error;
	struct file *fp = NULL;
	int (*fo_advlock)(struct file *, void *, int, struct flock *, int);
	struct flock lf;

	fd = SCARG(uap, fd);
	how = SCARG(uap, how);

	if ((fp = fd_getfile(fd)) == NULL) {
		error = EBADF;
		goto out;
	}
	if ((fo_advlock = fp->f_ops->fo_advlock) == NULL) {
		KASSERT((atomic_load_relaxed(&fp->f_flag) & FHASLOCK) == 0);
		error = EOPNOTSUPP;
		goto out;
	}

	lf.l_whence = SEEK_SET;
	lf.l_start = 0;
	lf.l_len = 0;

	switch (how & ~LOCK_NB) {
	case LOCK_UN:
		lf.l_type = F_UNLCK;
		atomic_and_uint(&fp->f_flag, ~FHASLOCK);
		error = (*fo_advlock)(fp, fp, F_UNLCK, &lf, F_FLOCK);
		goto out;
	case LOCK_EX:
		lf.l_type = F_WRLCK;
		break;
	case LOCK_SH:
		lf.l_type = F_RDLCK;
		break;
	default:
		error = EINVAL;
		goto out;
	}

	atomic_or_uint(&fp->f_flag, FHASLOCK);
	if (how & LOCK_NB) {
		error = (*fo_advlock)(fp, fp, F_SETLK, &lf, F_FLOCK);
	} else {
		error = (*fo_advlock)(fp, fp, F_SETLK, &lf, F_FLOCK|F_WAIT);
	}
out:	if (fp)
		fd_putfile(fd);
	return error;
}

int
do_posix_fadvise(int fd, off_t offset, off_t len, int advice)
{
	file_t *fp;
	int error;

	if ((fp = fd_getfile(fd)) == NULL)
		return EBADF;
	if (fp->f_ops->fo_posix_fadvise == NULL) {
		error = EOPNOTSUPP;
	} else {
		error = (*fp->f_ops->fo_posix_fadvise)(fp, offset, len,
		    advice);
	}
	fd_putfile(fd);
	return error;
}

int
sys___posix_fadvise50(struct lwp *l,
		      const struct sys___posix_fadvise50_args *uap,
		      register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
		syscallarg(off_t) len;
		syscallarg(int) advice;
	} */

	*retval = do_posix_fadvise(SCARG(uap, fd), SCARG(uap, offset),
	    SCARG(uap, len), SCARG(uap, advice));

	return 0;
}

int
sys_pipe(struct lwp *l, const void *v, register_t *retval)
{
	int fd[2], error;

	if ((error = pipe1(l, fd, 0)) != 0)
		return error;

	retval[0] = fd[0];
	retval[1] = fd[1];

	return 0;
}

int
sys_pipe2(struct lwp *l, const struct sys_pipe2_args *uap, register_t *retval)
{
	/* {
		syscallarg(int[2]) fildes;
		syscallarg(int) flags;
	} */
	int fd[2], error;

	if ((error = pipe1(l, fd, SCARG(uap, flags))) != 0)
		return error;

	if ((error = copyout(fd, SCARG(uap, fildes), sizeof(fd))) != 0)
		return error;
	retval[0] = 0;
	return 0;
}
