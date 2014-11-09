/*	$NetBSD: linux_file.c,v 1.114 2014/11/09 17:48:08 maxv Exp $	*/

/*-
 * Copyright (c) 1995, 1998, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz.
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
 * Functions in multiarch:
 *	linux_sys_llseek	: linux_llseek.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_file.c,v 1.114 2014/11/09 17:48:08 maxv Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/tty.h>
#include <sys/socketvar.h>
#include <sys/conf.h>
#include <sys/pipe.h>

#include <sys/syscallargs.h>
#include <sys/vfs_syscalls.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_fcntl.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>

#include <compat/linux/linux_syscallargs.h>

static int bsd_to_linux_ioflags(int);
#ifndef __amd64__
static void bsd_to_linux_stat(struct stat *, struct linux_stat *);
#endif

conv_linux_flock(linux, flock)

/*
 * Some file-related calls are handled here. The usual flag conversion
 * an structure conversion is done, and alternate emul path searching.
 */

/*
 * The next two functions convert between the Linux and NetBSD values
 * of the flags used in open(2) and fcntl(2).
 */
int
linux_to_bsd_ioflags(int lflags)
{
	int res = 0;

	res |= cvtto_bsd_mask(lflags, LINUX_O_WRONLY, O_WRONLY);
	res |= cvtto_bsd_mask(lflags, LINUX_O_RDONLY, O_RDONLY);
	res |= cvtto_bsd_mask(lflags, LINUX_O_RDWR, O_RDWR);

	res |= cvtto_bsd_mask(lflags, LINUX_O_CREAT, O_CREAT);
	res |= cvtto_bsd_mask(lflags, LINUX_O_EXCL, O_EXCL);
	res |= cvtto_bsd_mask(lflags, LINUX_O_NOCTTY, O_NOCTTY);
	res |= cvtto_bsd_mask(lflags, LINUX_O_TRUNC, O_TRUNC);
	res |= cvtto_bsd_mask(lflags, LINUX_O_APPEND, O_APPEND);
	res |= cvtto_bsd_mask(lflags, LINUX_O_NONBLOCK, O_NONBLOCK);
	res |= cvtto_bsd_mask(lflags, LINUX_O_NDELAY, O_NDELAY);
	res |= cvtto_bsd_mask(lflags, LINUX_O_SYNC, O_FSYNC);
	res |= cvtto_bsd_mask(lflags, LINUX_FASYNC, O_ASYNC);
	res |= cvtto_bsd_mask(lflags, LINUX_O_DIRECT, O_DIRECT);
	res |= cvtto_bsd_mask(lflags, LINUX_O_DIRECTORY, O_DIRECTORY);
	res |= cvtto_bsd_mask(lflags, LINUX_O_NOFOLLOW, O_NOFOLLOW);
	res |= cvtto_bsd_mask(lflags, LINUX_O_CLOEXEC, O_CLOEXEC);

	return res;
}

static int
bsd_to_linux_ioflags(int bflags)
{
	int res = 0;

	res |= cvtto_linux_mask(bflags, O_WRONLY, LINUX_O_WRONLY);
	res |= cvtto_linux_mask(bflags, O_RDONLY, LINUX_O_RDONLY);
	res |= cvtto_linux_mask(bflags, O_RDWR, LINUX_O_RDWR);

	res |= cvtto_linux_mask(bflags, O_CREAT, LINUX_O_CREAT);
	res |= cvtto_linux_mask(bflags, O_EXCL, LINUX_O_EXCL);
	res |= cvtto_linux_mask(bflags, O_NOCTTY, LINUX_O_NOCTTY);
	res |= cvtto_linux_mask(bflags, O_TRUNC, LINUX_O_TRUNC);
	res |= cvtto_linux_mask(bflags, O_APPEND, LINUX_O_APPEND);
	res |= cvtto_linux_mask(bflags, O_NONBLOCK, LINUX_O_NONBLOCK);
	res |= cvtto_linux_mask(bflags, O_NDELAY, LINUX_O_NDELAY);
	res |= cvtto_linux_mask(bflags, O_FSYNC, LINUX_O_SYNC);
	res |= cvtto_linux_mask(bflags, O_ASYNC, LINUX_FASYNC);
	res |= cvtto_linux_mask(bflags, O_DIRECT, LINUX_O_DIRECT);
	res |= cvtto_linux_mask(bflags, O_DIRECTORY, LINUX_O_DIRECTORY);
	res |= cvtto_linux_mask(bflags, O_NOFOLLOW, LINUX_O_NOFOLLOW);
	res |= cvtto_linux_mask(bflags, O_CLOEXEC, LINUX_O_CLOEXEC);

	return res;
}

/*
 * creat(2) is an obsolete function, but it's present as a Linux
 * system call, so let's deal with it.
 *
 * Note: On the Alpha this doesn't really exist in Linux, but it's defined
 * in syscalls.master anyway so this doesn't have to be special cased.
 *
 * Just call open(2) with the TRUNC, CREAT and WRONLY flags.
 */
int
linux_sys_creat(struct lwp *l, const struct linux_sys_creat_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(linux_umode_t) mode;
	} */
	struct sys_open_args oa;

	SCARG(&oa, path) = SCARG(uap, path);
	SCARG(&oa, flags) = O_CREAT | O_TRUNC | O_WRONLY;
	SCARG(&oa, mode) = SCARG(uap, mode);

	return sys_open(l, &oa, retval);
}

static void
linux_open_ctty(struct lwp *l, int flags, int fd)
{
	struct proc *p = l->l_proc;

	/*
	 * this bit from sunos_misc.c (and svr4_fcntl.c).
	 * If we are a session leader, and we don't have a controlling
	 * terminal yet, and the O_NOCTTY flag is not set, try to make
	 * this the controlling terminal.
	 */
        if (!(flags & O_NOCTTY) && SESS_LEADER(p) && !(p->p_lflag & PL_CONTROLT)) {
                file_t *fp;

		fp = fd_getfile(fd);

                /* ignore any error, just give it a try */
                if (fp != NULL) {
			if (fp->f_type == DTYPE_VNODE) {
				(fp->f_ops->fo_ioctl) (fp, TIOCSCTTY, NULL);
			}
			fd_putfile(fd);
		}
        }
}

/*
 * open(2). Take care of the different flag values, and let the
 * NetBSD syscall do the real work. See if this operation
 * gives the current process a controlling terminal.
 * (XXX is this necessary?)
 */
int
linux_sys_open(struct lwp *l, const struct linux_sys_open_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(int) flags;
		syscallarg(linux_umode_t) mode;
	} */
	int error, fl;
	struct sys_open_args boa;

	fl = linux_to_bsd_ioflags(SCARG(uap, flags));

	SCARG(&boa, path) = SCARG(uap, path);
	SCARG(&boa, flags) = fl;
	SCARG(&boa, mode) = SCARG(uap, mode);

	if ((error = sys_open(l, &boa, retval)))
		return (error == EFTYPE) ? ELOOP : error;

	linux_open_ctty(l, fl, *retval);
	return 0;
}

int
linux_sys_openat(struct lwp *l, const struct linux_sys_openat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const char *) path;
		syscallarg(int) flags;
		syscallarg(linux_umode_t) mode;
	} */
	int error, fl;
	struct sys_openat_args boa;

	fl = linux_to_bsd_ioflags(SCARG(uap, flags));

	SCARG(&boa, fd) = SCARG(uap, fd);
	SCARG(&boa, path) = SCARG(uap, path);
	SCARG(&boa, oflags) = fl;
	SCARG(&boa, mode) = SCARG(uap, mode);

	if ((error = sys_openat(l, &boa, retval)))
		return (error == EFTYPE) ? ELOOP : error;

	linux_open_ctty(l, fl, *retval);
	return 0;
}

/*
 * Most actions in the fcntl() call are straightforward; simply
 * pass control to the NetBSD system call. A few commands need
 * conversions after the actual system call has done its work,
 * because the flag values and lock structure are different.
 */
int
linux_sys_fcntl(struct lwp *l, const struct linux_sys_fcntl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) cmd;
		syscallarg(void *) arg;
	} */
	struct proc *p = l->l_proc;
	int fd, cmd, error;
	u_long val;
	void *arg;
	struct sys_fcntl_args fca;
	file_t *fp;
	struct vnode *vp;
	struct vattr va;
	long pgid;
	struct pgrp *pgrp;
	struct tty *tp;

	fd = SCARG(uap, fd);
	cmd = SCARG(uap, cmd);
	arg = SCARG(uap, arg);

	switch (cmd) {

	case LINUX_F_DUPFD:
		cmd = F_DUPFD;
		break;

	case LINUX_F_GETFD:
		cmd = F_GETFD;
		break;

	case LINUX_F_SETFD:
		cmd = F_SETFD;
		break;

	case LINUX_F_GETFL:
		SCARG(&fca, fd) = fd;
		SCARG(&fca, cmd) = F_GETFL;
		SCARG(&fca, arg) = arg;
		if ((error = sys_fcntl(l, &fca, retval)))
			return error;
		retval[0] = bsd_to_linux_ioflags(retval[0]);
		return 0;

	case LINUX_F_SETFL: {
		file_t	*fp1 = NULL;

		val = linux_to_bsd_ioflags((unsigned long)SCARG(uap, arg));
		/*
		 * Linux seems to have same semantics for sending SIGIO to the
		 * read side of socket, but slightly different semantics
		 * for SIGIO to the write side.  Rather than sending the SIGIO
		 * every time it's possible to write (directly) more data, it
		 * only sends SIGIO if last write(2) failed due to insufficient
		 * memory to hold the data. This is compatible enough
		 * with NetBSD semantics to not do anything about the
		 * difference.
		 *
		 * Linux does NOT send SIGIO for pipes. Deal with socketpair
		 * ones and DTYPE_PIPE ones. For these, we don't set
		 * the underlying flags (we don't pass O_ASYNC flag down
		 * to sys_fcntl()), but set the FASYNC flag for file descriptor,
		 * so that F_GETFL would report the ASYNC i/o is on.
		 */
		if (val & O_ASYNC) {
			if (((fp1 = fd_getfile(fd)) == NULL))
			    return (EBADF);
			if (((fp1->f_type == DTYPE_SOCKET) && fp1->f_data
			      && ((struct socket *)fp1->f_data)->so_state & SS_ISAPIPE)
			    || (fp1->f_type == DTYPE_PIPE))
				val &= ~O_ASYNC;
			else {
				/* not a pipe, do not modify anything */
				fd_putfile(fd);
				fp1 = NULL;
			}
		}

		SCARG(&fca, fd) = fd;
		SCARG(&fca, cmd) = F_SETFL;
		SCARG(&fca, arg) = (void *) val;

		error = sys_fcntl(l, &fca, retval);

		/* Now set the FASYNC flag for pipes */
		if (fp1) {
			if (!error) {
				mutex_enter(&fp1->f_lock);
				fp1->f_flag |= FASYNC;
				mutex_exit(&fp1->f_lock);
			}
			fd_putfile(fd);
		}

		return (error);
	    }

	case LINUX_F_GETLK:
		do_linux_getlk(fd, cmd, arg, linux, flock);

	case LINUX_F_SETLK:
	case LINUX_F_SETLKW:
		do_linux_setlk(fd, cmd, arg, linux, flock, LINUX_F_SETLK);

	case LINUX_F_SETOWN:
	case LINUX_F_GETOWN:
		/*
		 * We need to route fcntl() for tty descriptors around normal
		 * fcntl(), since NetBSD tty TIOC{G,S}PGRP semantics is too
		 * restrictive for Linux F_{G,S}ETOWN. For non-tty descriptors,
		 * this is not a problem.
		 */
		if ((fp = fd_getfile(fd)) == NULL)
			return EBADF;

		/* Check it's a character device vnode */
		if (fp->f_type != DTYPE_VNODE
		    || (vp = (struct vnode *)fp->f_data) == NULL
		    || vp->v_type != VCHR) {
			fd_putfile(fd);

	    not_tty:
			/* Not a tty, proceed with common fcntl() */
			cmd = cmd == LINUX_F_SETOWN ? F_SETOWN : F_GETOWN;
			break;
		}

		vn_lock(vp, LK_SHARED | LK_RETRY);
		error = VOP_GETATTR(vp, &va, l->l_cred);
		VOP_UNLOCK(vp);

		fd_putfile(fd);

		if (error)
			return error;

		if ((tp = cdev_tty(va.va_rdev)) == NULL)
			goto not_tty;

		/* set tty pg_id appropriately */
		mutex_enter(proc_lock);
		if (cmd == LINUX_F_GETOWN) {
			retval[0] = tp->t_pgrp ? tp->t_pgrp->pg_id : NO_PGID;
			mutex_exit(proc_lock);
			return 0;
		}
		if ((long)arg <= 0) {
			pgid = -(long)arg;
		} else {
			struct proc *p1 = proc_find((long)arg);
			if (p1 == NULL) {
				mutex_exit(proc_lock);
				return (ESRCH);
			}
			pgid = (long)p1->p_pgrp->pg_id;
		}
		pgrp = pgrp_find(pgid);
		if (pgrp == NULL || pgrp->pg_session != p->p_session) {
			mutex_exit(proc_lock);
			return EPERM;
		}
		tp->t_pgrp = pgrp;
		mutex_exit(proc_lock);
		return 0;

	default:
		return EOPNOTSUPP;
	}

	SCARG(&fca, fd) = fd;
	SCARG(&fca, cmd) = cmd;
	SCARG(&fca, arg) = arg;

	return sys_fcntl(l, &fca, retval);
}

#if !defined(__amd64__)
/*
 * Convert a NetBSD stat structure to a Linux stat structure.
 * Only the order of the fields and the padding in the structure
 * is different. linux_fakedev is a machine-dependent function
 * which optionally converts device driver major/minor numbers
 * (XXX horrible, but what can you do against code that compares
 * things against constant major device numbers? sigh)
 */
static void
bsd_to_linux_stat(struct stat *bsp, struct linux_stat *lsp)
{

	lsp->lst_dev     = linux_fakedev(bsp->st_dev, 0);
	lsp->lst_ino     = bsp->st_ino;
	lsp->lst_mode    = (linux_mode_t)bsp->st_mode;
	if (bsp->st_nlink >= (1 << 15))
		lsp->lst_nlink = (1 << 15) - 1;
	else
		lsp->lst_nlink = (linux_nlink_t)bsp->st_nlink;
	lsp->lst_uid     = bsp->st_uid;
	lsp->lst_gid     = bsp->st_gid;
	lsp->lst_rdev    = linux_fakedev(bsp->st_rdev, 1);
	lsp->lst_size    = bsp->st_size;
	lsp->lst_blksize = bsp->st_blksize;
	lsp->lst_blocks  = bsp->st_blocks;
	lsp->lst_atime   = bsp->st_atime;
	lsp->lst_mtime   = bsp->st_mtime;
	lsp->lst_ctime   = bsp->st_ctime;
#ifdef LINUX_STAT_HAS_NSEC
	lsp->lst_atime_nsec   = bsp->st_atimensec;
	lsp->lst_mtime_nsec   = bsp->st_mtimensec;
	lsp->lst_ctime_nsec   = bsp->st_ctimensec;
#endif
}

/*
 * The stat functions below are plain sailing. stat and lstat are handled
 * by one function to avoid code duplication.
 */
int
linux_sys_fstat(struct lwp *l, const struct linux_sys_fstat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(linux_stat *) sp;
	} */
	struct linux_stat tmplst;
	struct stat tmpst;
	int error;

	error = do_sys_fstat(SCARG(uap, fd), &tmpst);
	if (error != 0)
		return error;
	bsd_to_linux_stat(&tmpst, &tmplst);

	return copyout(&tmplst, SCARG(uap, sp), sizeof tmplst);
}

static int
linux_stat1(const struct linux_sys_stat_args *uap, register_t *retval, int flags)
{
	struct linux_stat tmplst;
	struct stat tmpst;
	int error;

	error = do_sys_stat(SCARG(uap, path), flags, &tmpst);
	if (error != 0)
		return error;

	bsd_to_linux_stat(&tmpst, &tmplst);

	return copyout(&tmplst, SCARG(uap, sp), sizeof tmplst);
}

int
linux_sys_stat(struct lwp *l, const struct linux_sys_stat_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct linux_stat *) sp;
	} */

	return linux_stat1(uap, retval, FOLLOW);
}

/* Note: this is "newlstat" in the Linux sources */
/*	(we don't bother with the old lstat currently) */
int
linux_sys_lstat(struct lwp *l, const struct linux_sys_lstat_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct linux_stat *) sp;
	} */

	return linux_stat1((const void *)uap, retval, NOFOLLOW);
}
#endif /* !__amd64__ */

/*
 * The following syscalls are mostly here because of the alternate path check.
 */

int
linux_sys_linkat(struct lwp *l, const struct linux_sys_linkat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd1;
		syscallarg(const char *) name1;
		syscallarg(int) fd2;
		syscallarg(const char *) name2;
		syscallarg(int) flags;
	} */
	int fd1 = SCARG(uap, fd1);
	const char *name1 = SCARG(uap, name1);
	int fd2 = SCARG(uap, fd2);
	const char *name2 = SCARG(uap, name2);
	int follow;

	follow = SCARG(uap, flags) & LINUX_AT_SYMLINK_FOLLOW;

	return do_sys_linkat(l, fd1, name1, fd2, name2, follow, retval);
}

static int
linux_unlink_dircheck(const char *path)
{
	struct nameidata nd;
	struct pathbuf *pb;
	int error;

	/*
	 * Linux returns EISDIR if unlink(2) is called on a directory.
	 * We return EPERM in such cases. To emulate correct behaviour,
	 * check if the path points to directory and return EISDIR if this
	 * is the case.
	 *
	 * XXX this should really not copy in the path buffer twice...
	 */
	error = pathbuf_copyin(path, &pb);
	if (error) {
		return error;
	}
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF | TRYEMULROOT, pb);
	if (namei(&nd) == 0) {
		struct stat sb;

		if (vn_stat(nd.ni_vp, &sb) == 0
		    && S_ISDIR(sb.st_mode))
			error = EISDIR;

		vput(nd.ni_vp);
	}
	pathbuf_destroy(pb);
	return error ? error : EPERM;
}

int
linux_sys_unlink(struct lwp *l, const struct linux_sys_unlink_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
	} */
	int error;

	error = sys_unlink(l, (const void *)uap, retval);
	if (error == EPERM)
		error = linux_unlink_dircheck(SCARG(uap, path));

	return error;
}

int
linux_sys_unlinkat(struct lwp *l, const struct linux_sys_unlinkat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const char *) path;
		syscallarg(int) flag;
	} */
	struct sys_unlinkat_args ua;
	int error;

	SCARG(&ua, fd) = SCARG(uap, fd);
	SCARG(&ua, path) = SCARG(uap, path);
	SCARG(&ua, flag) = linux_to_bsd_atflags(SCARG(uap, flag));

	error = sys_unlinkat(l, &ua, retval);
	if (error == EPERM)
		error = linux_unlink_dircheck(SCARG(uap, path));

	return error;
}

int
linux_sys_mknod(struct lwp *l, const struct linux_sys_mknod_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(linux_umode_t) mode;
		syscallarg(unsigned) dev;
	} */
	struct linux_sys_mknodat_args ua;

	SCARG(&ua, fd) = LINUX_AT_FDCWD;
	SCARG(&ua, path) = SCARG(uap, path);
	SCARG(&ua, mode) = SCARG(uap, mode);
	SCARG(&ua, dev) = SCARG(uap, dev);

	return linux_sys_mknodat(l, &ua, retval);
}

int
linux_sys_mknodat(struct lwp *l, const struct linux_sys_mknodat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const char *) path;
		syscallarg(linux_umode_t) mode;
		syscallarg(unsigned) dev;
	} */

	/*
	 * BSD handles FIFOs separately
	 */
	if (S_ISFIFO(SCARG(uap, mode))) {
		struct sys_mkfifoat_args bma;

		SCARG(&bma, fd) = SCARG(uap, fd);
		SCARG(&bma, path) = SCARG(uap, path);
		SCARG(&bma, mode) = SCARG(uap, mode);
		return sys_mkfifoat(l, &bma, retval);
	} else {

		/*
		 * Linux device numbers uses 8 bits for minor and 8 bits
		 * for major. Due to how we map our major and minor,
		 * this just fits into our dev_t. Just mask off the
		 * upper 16bit to remove any random junk.
		 */

		return do_sys_mknodat(l, SCARG(uap, fd), SCARG(uap, path),
		    SCARG(uap, mode), SCARG(uap, dev) & 0xffff, retval,
		    UIO_USERSPACE);
	}
}

int
linux_sys_fchmodat(struct lwp *l, const struct linux_sys_fchmodat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const char *) path;
		syscallarg(linux_umode_t) mode;
	} */

	return do_sys_chmodat(l, SCARG(uap, fd), SCARG(uap, path),
			      SCARG(uap, mode), AT_SYMLINK_FOLLOW);
}

int
linux_sys_fchownat(struct lwp *l, const struct linux_sys_fchownat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const char *) path;
		syscallarg(uid_t) owner;
		syscallarg(gid_t) group;
		syscallarg(int) flag;
	} */
	int flag;

	flag = linux_to_bsd_atflags(SCARG(uap, flag));
	return do_sys_chownat(l, SCARG(uap, fd), SCARG(uap, path),
			      SCARG(uap, owner), SCARG(uap, group), flag);
}

int
linux_sys_faccessat(struct lwp *l, const struct linux_sys_faccessat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const char *) path;
		syscallarg(int) amode;
	} */

	return do_sys_accessat(l, SCARG(uap, fd), SCARG(uap, path),
	     SCARG(uap, amode), AT_SYMLINK_FOLLOW);
}

/*
 * This is just fsync() for now (just as it is in the Linux kernel)
 * Note: this is not implemented under Linux on Alpha and Arm
 *	but should still be defined in our syscalls.master.
 *	(syscall #148 on the arm)
 */
int
linux_sys_fdatasync(struct lwp *l, const struct linux_sys_fdatasync_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
	} */

	return sys_fsync(l, (const void *)uap, retval);
}

/*
 * pread(2).
 */
int
linux_sys_pread(struct lwp *l, const struct linux_sys_pread_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(void *) buf;
		syscallarg(size_t) nbyte;
		syscallarg(off_t) offset;
	} */
	struct sys_pread_args pra;

	SCARG(&pra, fd) = SCARG(uap, fd);
	SCARG(&pra, buf) = SCARG(uap, buf);
	SCARG(&pra, nbyte) = SCARG(uap, nbyte);
	SCARG(&pra, PAD) = 0;
	SCARG(&pra, offset) = SCARG(uap, offset);

	return sys_pread(l, &pra, retval);
}

/*
 * pwrite(2).
 */
int
linux_sys_pwrite(struct lwp *l, const struct linux_sys_pwrite_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(void *) buf;
		syscallarg(size_t) nbyte;
		syscallarg(off_t) offset;
	} */
	struct sys_pwrite_args pra;

	SCARG(&pra, fd) = SCARG(uap, fd);
	SCARG(&pra, buf) = SCARG(uap, buf);
	SCARG(&pra, nbyte) = SCARG(uap, nbyte);
	SCARG(&pra, PAD) = 0;
	SCARG(&pra, offset) = SCARG(uap, offset);

	return sys_pwrite(l, &pra, retval);
}

int
linux_sys_dup3(struct lwp *l, const struct linux_sys_dup3_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) from;
		syscallarg(int) to;
		syscallarg(int) flags;
	} */
	int flags;

	flags = linux_to_bsd_ioflags(SCARG(uap, flags));
	if ((flags & ~O_CLOEXEC) != 0)
		return EINVAL;

	if (SCARG(uap, from) == SCARG(uap, to))
		return EINVAL;

	return dodup(l, SCARG(uap, from), SCARG(uap, to), flags, retval);
}


int
linux_to_bsd_atflags(int lflags)
{
	int bflags = 0;

	if (lflags & LINUX_AT_SYMLINK_NOFOLLOW)
		bflags |= AT_SYMLINK_NOFOLLOW;
	if (lflags & LINUX_AT_REMOVEDIR)
		bflags |= AT_REMOVEDIR;
	if (lflags & LINUX_AT_SYMLINK_FOLLOW)
		bflags |= AT_SYMLINK_FOLLOW;

	return bflags;
}


#define LINUX_NOT_SUPPORTED(fun) \
int \
fun(struct lwp *l, const struct fun##_args *uap, register_t *retval) \
{ \
	return EOPNOTSUPP; \
}

LINUX_NOT_SUPPORTED(linux_sys_setxattr)
LINUX_NOT_SUPPORTED(linux_sys_lsetxattr)
LINUX_NOT_SUPPORTED(linux_sys_fsetxattr)

LINUX_NOT_SUPPORTED(linux_sys_getxattr)
LINUX_NOT_SUPPORTED(linux_sys_lgetxattr)
LINUX_NOT_SUPPORTED(linux_sys_fgetxattr)

LINUX_NOT_SUPPORTED(linux_sys_listxattr)
LINUX_NOT_SUPPORTED(linux_sys_llistxattr)
LINUX_NOT_SUPPORTED(linux_sys_flistxattr)

LINUX_NOT_SUPPORTED(linux_sys_removexattr)
LINUX_NOT_SUPPORTED(linux_sys_lremovexattr)
LINUX_NOT_SUPPORTED(linux_sys_fremovexattr)
