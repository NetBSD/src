/*	$NetBSD: linux_file.c,v 1.77.2.4 2007/05/17 13:41:13 yamt Exp $	*/

/*-
 * Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Functions in multiarch:
 *	linux_sys_llseek	: linux_llseek.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_file.c,v 1.77.2.4 2007/05/17 13:41:13 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/malloc.h>
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

#include <compat/linux/linux_syscallargs.h>

static int linux_to_bsd_ioflags __P((int));
static int bsd_to_linux_ioflags __P((int));
static void bsd_to_linux_flock __P((struct flock *, struct linux_flock *));
static void linux_to_bsd_flock __P((struct linux_flock *, struct flock *));
#ifndef __amd64__
static void bsd_to_linux_stat __P((struct stat *, struct linux_stat *));
static int linux_stat1 __P((struct lwp *, void *, register_t *, int));
#endif

/*
 * Some file-related calls are handled here. The usual flag conversion
 * an structure conversion is done, and alternate emul path searching.
 */

/*
 * The next two functions convert between the Linux and NetBSD values
 * of the flags used in open(2) and fcntl(2).
 */
static int
linux_to_bsd_ioflags(lflags)
	int lflags;
{
	int res = 0;

	res |= cvtto_bsd_mask(lflags, LINUX_O_WRONLY, O_WRONLY);
	res |= cvtto_bsd_mask(lflags, LINUX_O_RDONLY, O_RDONLY);
	res |= cvtto_bsd_mask(lflags, LINUX_O_RDWR, O_RDWR);
	res |= cvtto_bsd_mask(lflags, LINUX_O_CREAT, O_CREAT);
	res |= cvtto_bsd_mask(lflags, LINUX_O_EXCL, O_EXCL);
	res |= cvtto_bsd_mask(lflags, LINUX_O_NOCTTY, O_NOCTTY);
	res |= cvtto_bsd_mask(lflags, LINUX_O_TRUNC, O_TRUNC);
	res |= cvtto_bsd_mask(lflags, LINUX_O_NDELAY, O_NDELAY);
	res |= cvtto_bsd_mask(lflags, LINUX_O_SYNC, O_FSYNC);
	res |= cvtto_bsd_mask(lflags, LINUX_FASYNC, O_ASYNC);
	res |= cvtto_bsd_mask(lflags, LINUX_O_APPEND, O_APPEND);

	return res;
}

static int
bsd_to_linux_ioflags(bflags)
	int bflags;
{
	int res = 0;

	res |= cvtto_linux_mask(bflags, O_WRONLY, LINUX_O_WRONLY);
	res |= cvtto_linux_mask(bflags, O_RDONLY, LINUX_O_RDONLY);
	res |= cvtto_linux_mask(bflags, O_RDWR, LINUX_O_RDWR);
	res |= cvtto_linux_mask(bflags, O_CREAT, LINUX_O_CREAT);
	res |= cvtto_linux_mask(bflags, O_EXCL, LINUX_O_EXCL);
	res |= cvtto_linux_mask(bflags, O_NOCTTY, LINUX_O_NOCTTY);
	res |= cvtto_linux_mask(bflags, O_TRUNC, LINUX_O_TRUNC);
	res |= cvtto_linux_mask(bflags, O_NDELAY, LINUX_O_NDELAY);
	res |= cvtto_linux_mask(bflags, O_FSYNC, LINUX_O_SYNC);
	res |= cvtto_linux_mask(bflags, O_ASYNC, LINUX_FASYNC);
	res |= cvtto_linux_mask(bflags, O_APPEND, LINUX_O_APPEND);

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
linux_sys_creat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_creat_args /* {
		syscallarg(const char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	struct sys_open_args oa;

	SCARG(&oa, path) = SCARG(uap, path);
	SCARG(&oa, flags) = O_CREAT | O_TRUNC | O_WRONLY;
	SCARG(&oa, mode) = SCARG(uap, mode);

	return sys_open(l, &oa, retval);
}

/*
 * open(2). Take care of the different flag values, and let the
 * NetBSD syscall do the real work. See if this operation
 * gives the current process a controlling terminal.
 * (XXX is this necessary?)
 */
int
linux_sys_open(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_open_args /* {
		syscallarg(const char *) path;
		syscallarg(int) flags;
		syscallarg(int) mode;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int error, fl;
	struct sys_open_args boa;

	fl = linux_to_bsd_ioflags(SCARG(uap, flags));

	SCARG(&boa, path) = SCARG(uap, path);
	SCARG(&boa, flags) = fl;
	SCARG(&boa, mode) = SCARG(uap, mode);

	if ((error = sys_open(l, &boa, retval)))
		return error;

	/*
	 * this bit from sunos_misc.c (and svr4_fcntl.c).
	 * If we are a session leader, and we don't have a controlling
	 * terminal yet, and the O_NOCTTY flag is not set, try to make
	 * this the controlling terminal.
	 */
        if (!(fl & O_NOCTTY) && SESS_LEADER(p) && !(p->p_lflag & PL_CONTROLT)) {
                struct filedesc *fdp = p->p_fd;
                struct file     *fp;

		fp = fd_getfile(fdp, *retval);

                /* ignore any error, just give it a try */
                if (fp != NULL) {
			FILE_USE(fp);
			if (fp->f_type == DTYPE_VNODE) {
				(fp->f_ops->fo_ioctl) (fp, TIOCSCTTY,
				    (void *) 0, l);
			}
			FILE_UNUSE(fp, l);
		}
        }
	return 0;
}

/*
 * The next two functions take care of converting the flock
 * structure back and forth between Linux and NetBSD format.
 * The only difference in the structures is the order of
 * the fields, and the 'whence' value.
 */
static void
bsd_to_linux_flock(bfp, lfp)
	struct flock *bfp;
	struct linux_flock *lfp;
{

	lfp->l_start = bfp->l_start;
	lfp->l_len = bfp->l_len;
	lfp->l_pid = bfp->l_pid;
	lfp->l_whence = bfp->l_whence;
	switch (bfp->l_type) {
	case F_RDLCK:
		lfp->l_type = LINUX_F_RDLCK;
		break;
	case F_UNLCK:
		lfp->l_type = LINUX_F_UNLCK;
		break;
	case F_WRLCK:
		lfp->l_type = LINUX_F_WRLCK;
		break;
	}
}

static void
linux_to_bsd_flock(lfp, bfp)
	struct linux_flock *lfp;
	struct flock *bfp;
{

	bfp->l_start = lfp->l_start;
	bfp->l_len = lfp->l_len;
	bfp->l_pid = lfp->l_pid;
	bfp->l_whence = lfp->l_whence;
	switch (lfp->l_type) {
	case LINUX_F_RDLCK:
		bfp->l_type = F_RDLCK;
		break;
	case LINUX_F_UNLCK:
		bfp->l_type = F_UNLCK;
		break;
	case LINUX_F_WRLCK:
		bfp->l_type = F_WRLCK;
		break;
	}
}

/*
 * Most actions in the fcntl() call are straightforward; simply
 * pass control to the NetBSD system call. A few commands need
 * conversions after the actual system call has done its work,
 * because the flag values and lock structure are different.
 */
int
linux_sys_fcntl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_fcntl_args /* {
		syscallarg(int) fd;
		syscallarg(int) cmd;
		syscallarg(void *) arg;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int fd, cmd, error;
	u_long val;
	void *arg;
	struct linux_flock lfl;
	struct flock bfl;
	struct sys_fcntl_args fca;
	struct filedesc *fdp;
	struct file *fp;
	struct vnode *vp;
	struct vattr va;
	const struct cdevsw *cdev;
	long pgid;
	struct pgrp *pgrp;
	struct tty *tp, *(*d_tty) __P((dev_t));

	fd = SCARG(uap, fd);
	cmd = SCARG(uap, cmd);
	arg = (void *) SCARG(uap, arg);

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
		struct file	*fp1 = NULL;

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
			if (((fp1 = fd_getfile(p->p_fd, fd)) == NULL))
			    return (EBADF);

			FILE_USE(fp1);

			if (((fp1->f_type == DTYPE_SOCKET) && fp1->f_data
			      && ((struct socket *)fp1->f_data)->so_state & SS_ISAPIPE)
			    || (fp1->f_type == DTYPE_PIPE))
				val &= ~O_ASYNC;
			else {
				/* not a pipe, do not modify anything */
				FILE_UNUSE(fp1, l);
				fp1 = NULL;
			}
		}

		SCARG(&fca, fd) = fd;
		SCARG(&fca, cmd) = F_SETFL;
		SCARG(&fca, arg) = (void *) val;

		error = sys_fcntl(l, &fca, retval);

		/* Now set the FASYNC flag for pipes */
		if (fp1) {
			if (!error)
				fp1->f_flag |= FASYNC;
			FILE_UNUSE(fp1, l);
		}

		return (error);
	    }
	case LINUX_F_GETLK:
		if ((error = copyin(arg, &lfl, sizeof lfl)))
			return error;
		linux_to_bsd_flock(&lfl, &bfl);
		error = do_fcntl_lock(l, fd, F_GETLK, &bfl);
		if (error)
			return error;
		bsd_to_linux_flock(&bfl, &lfl);
		return copyout(&lfl, arg, sizeof lfl);

	case LINUX_F_SETLK:
	case LINUX_F_SETLKW:
		cmd = (cmd == LINUX_F_SETLK ? F_SETLK : F_SETLKW);
		if ((error = copyin(arg, &lfl, sizeof lfl)))
			return error;
		linux_to_bsd_flock(&lfl, &bfl);
		return do_fcntl_lock(l, fd, cmd, &bfl);

	case LINUX_F_SETOWN:
	case LINUX_F_GETOWN:
		/*
		 * We need to route fcntl() for tty descriptors around normal
		 * fcntl(), since NetBSD tty TIOC{G,S}PGRP semantics is too
		 * restrictive for Linux F_{G,S}ETOWN. For non-tty descriptors,
		 * this is not a problem.
		 */
		fdp = p->p_fd;
		if ((fp = fd_getfile(fdp, fd)) == NULL)
			return EBADF;
		FILE_USE(fp);

		/* Check it's a character device vnode */
		if (fp->f_type != DTYPE_VNODE
		    || (vp = (struct vnode *)fp->f_data) == NULL
		    || vp->v_type != VCHR) {
			FILE_UNUSE(fp, l);

	    not_tty:
			/* Not a tty, proceed with common fcntl() */
			cmd = cmd == LINUX_F_SETOWN ? F_SETOWN : F_GETOWN;
			break;
		}

		error = VOP_GETATTR(vp, &va, l->l_cred, l);

		FILE_UNUSE(fp, l);

		if (error)
			return error;

		cdev = cdevsw_lookup(va.va_rdev);
		if (cdev == NULL)
			return (ENXIO);
		d_tty = cdev->d_tty;
		if (!d_tty || (!(tp = (*d_tty)(va.va_rdev))))
			goto not_tty;

		/* set tty pg_id appropriately */
		if (cmd == LINUX_F_GETOWN) {
			retval[0] = tp->t_pgrp ? tp->t_pgrp->pg_id : NO_PGID;
			return 0;
		}
		mutex_enter(&proclist_lock);
		if ((long)arg <= 0) {
			pgid = -(long)arg;
		} else {
			struct proc *p1 = p_find((long)arg, PFIND_LOCKED | PFIND_UNLOCK_FAIL);
			if (p1 == NULL)
				return (ESRCH);
			pgid = (long)p1->p_pgrp->pg_id;
		}
		pgrp = pg_find(pgid, PFIND_LOCKED);
		if (pgrp == NULL || pgrp->pg_session != p->p_session) {
			mutex_exit(&proclist_lock);
			return EPERM;
		}
		tp->t_pgrp = pgrp;
		mutex_exit(&proclist_lock);
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
bsd_to_linux_stat(bsp, lsp)
	struct stat *bsp;
	struct linux_stat *lsp;
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
linux_sys_fstat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_fstat_args /* {
		syscallarg(int) fd;
		syscallarg(linux_stat *) sp;
	} */ *uap = v;
	struct linux_stat tmplst;
	struct stat tmpst;
	int error;

	error = do_sys_fstat(l, SCARG(uap, fd), &tmpst);
	if (error != 0)
		return error;
	bsd_to_linux_stat(&tmpst, &tmplst);

	return copyout(&tmplst, SCARG(uap, sp), sizeof tmplst);
}

static int
linux_stat1(l, v, retval, flags)
	struct lwp *l;
	void *v;
	register_t *retval;
	int flags;
{
	struct linux_stat tmplst;
	struct stat tmpst;
	int error;
	struct linux_sys_stat_args *uap = v;

	error = do_sys_stat(l, SCARG(uap, path), flags, &tmpst);
	if (error != 0)
		return error;

	bsd_to_linux_stat(&tmpst, &tmplst);

	return copyout(&tmplst, SCARG(uap, sp), sizeof tmplst);
}

int
linux_sys_stat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_stat_args /* {
		syscallarg(const char *) path;
		syscallarg(struct linux_stat *) sp;
	} */ *uap = v;

	return linux_stat1(l, uap, retval, FOLLOW);
}

/* Note: this is "newlstat" in the Linux sources */
/*	(we don't bother with the old lstat currently) */
int
linux_sys_lstat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_lstat_args /* {
		syscallarg(const char *) path;
		syscallarg(struct linux_stat *) sp;
	} */ *uap = v;

	return linux_stat1(l, uap, retval, NOFOLLOW);
}
#endif /* !__amd64__ */

/*
 * The following syscalls are mostly here because of the alternate path check.
 */
int
linux_sys_access(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_access_args /* {
		syscallarg(const char *) path;
		syscallarg(int) flags;
	} */ *uap = v;

	return sys_access(l, uap, retval);
}

int
linux_sys_unlink(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;

{
	struct linux_sys_unlink_args /* {
		syscallarg(const char *) path;
	} */ *uap = v;
	int error;
	struct nameidata nd;

	error = sys_unlink(l, uap, retval);
	if (error != EPERM)
		return (error);

	/*
	 * Linux returns EISDIR if unlink(2) is called on a directory.
	 * We return EPERM in such cases. To emulate correct behaviour,
	 * check if the path points to directory and return EISDIR if this
	 * is the case.
	 */
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF | TRYEMULROOT, UIO_USERSPACE,
	    SCARG(uap, path), l);
	if (namei(&nd) == 0) {
		struct stat sb;

		if (vn_stat(nd.ni_vp, &sb, l) == 0
		    && S_ISDIR(sb.st_mode))
			error = EISDIR;

		vput(nd.ni_vp);
	}

	return (error);
}

int
linux_sys_chdir(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_chdir_args /* {
		syscallarg(const char *) path;
	} */ *uap = v;

	return sys_chdir(l, uap, retval);
}

int
linux_sys_mknod(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_mknod_args /* {
		syscallarg(const char *) path;
		syscallarg(int) mode;
		syscallarg(int) dev;
	} */ *uap = v;

	/*
	 * BSD handles FIFOs separately
	 */
	if (S_ISFIFO(SCARG(uap, mode))) {
		struct sys_mkfifo_args bma;

		SCARG(&bma, path) = SCARG(uap, path);
		SCARG(&bma, mode) = SCARG(uap, mode);
		return sys_mkfifo(l, &bma, retval);
	} else {
		struct sys_mknod_args bma;

		SCARG(&bma, path) = SCARG(uap, path);
		SCARG(&bma, mode) = SCARG(uap, mode);
		/*
		 * Linux device numbers uses 8 bits for minor and 8 bits
		 * for major. Due to how we map our major and minor,
		 * this just fits into our dev_t. Just mask off the
		 * upper 16bit to remove any random junk.
		 */
		SCARG(&bma, dev) = SCARG(uap, dev) & 0xffff;
		return sys_mknod(l, &bma, retval);
	}
}

int
linux_sys_chmod(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_chmod_args /* {
		syscallarg(const char *) path;
		syscallarg(int) mode;
	} */ *uap = v;

	return sys_chmod(l, uap, retval);
}

#if defined(__i386__) || defined(__m68k__) || \
    defined(__arm__)
int
linux_sys_chown16(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_chown16_args /* {
		syscallarg(const char *) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */ *uap = v;
	struct sys___posix_chown_args bca;

	SCARG(&bca, path) = SCARG(uap, path);
	SCARG(&bca, uid) = ((linux_uid_t)SCARG(uap, uid) == (linux_uid_t)-1) ?
		(uid_t)-1 : SCARG(uap, uid);
	SCARG(&bca, gid) = ((linux_gid_t)SCARG(uap, gid) == (linux_gid_t)-1) ?
		(gid_t)-1 : SCARG(uap, gid);

	return sys___posix_chown(l, &bca, retval);
}

int
linux_sys_fchown16(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_fchown16_args /* {
		syscallarg(int) fd;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */ *uap = v;
	struct sys___posix_fchown_args bfa;

	SCARG(&bfa, fd) = SCARG(uap, fd);
	SCARG(&bfa, uid) = ((linux_uid_t)SCARG(uap, uid) == (linux_uid_t)-1) ?
		(uid_t)-1 : SCARG(uap, uid);
	SCARG(&bfa, gid) = ((linux_gid_t)SCARG(uap, gid) == (linux_gid_t)-1) ?
		(gid_t)-1 : SCARG(uap, gid);

	return sys___posix_fchown(l, &bfa, retval);
}

int
linux_sys_lchown16(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_lchown16_args /* {
		syscallarg(char *) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */ *uap = v;
	struct sys___posix_lchown_args bla;

	SCARG(&bla, path) = SCARG(uap, path);
	SCARG(&bla, uid) = ((linux_uid_t)SCARG(uap, uid) == (linux_uid_t)-1) ?
		(uid_t)-1 : SCARG(uap, uid);
	SCARG(&bla, gid) = ((linux_gid_t)SCARG(uap, gid) == (linux_gid_t)-1) ?
		(gid_t)-1 : SCARG(uap, gid);

	return sys___posix_lchown(l, &bla, retval);
}
#endif /* __i386__ || __m68k__ || __arm__ || __amd64__ */
#if defined (__i386__) || defined (__m68k__) || defined(__amd64__) || \
    defined (__powerpc__) || defined (__mips__) || defined (__arm__)
int
linux_sys_chown(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_chown_args /* {
		syscallarg(char *) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */ *uap = v;

	return sys___posix_chown(l, uap, retval);
}

int
linux_sys_lchown(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_lchown_args /* {
		syscallarg(char *) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */ *uap = v;

	return sys___posix_lchown(l, uap, retval);
}
#endif /* __i386__||__m68k__||__powerpc__||__mips__||__arm__ ||__amd64__ */

int
linux_sys_rename(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_rename_args /* {
		syscallarg(const char *) from;
		syscallarg(const char *) to;
	} */ *uap = v;

	return sys___posix_rename(l, uap, retval);
}

int
linux_sys_mkdir(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_mkdir_args /* {
		syscallarg(const char *) path;
		syscallarg(int) mode;
	} */ *uap = v;

	return sys_mkdir(l, uap, retval);
}

int
linux_sys_rmdir(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_rmdir_args /* {
		syscallarg(const char *) path;
	} */ *uap = v;

	return sys_rmdir(l, uap, retval);
}

int
linux_sys_symlink(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_symlink_args /* {
		syscallarg(const char *) path;
		syscallarg(const char *) to;
	} */ *uap = v;

	return sys_symlink(l, uap, retval);
}

int
linux_sys_link(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_link_args /* {
		syscallarg(const char *) path;
		syscallarg(const char *) link;
	} */ *uap = v;

	return sys_link(l, uap, retval);
}

int
linux_sys_readlink(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_readlink_args /* {
		syscallarg(const char *) name;
		syscallarg(char *) buf;
		syscallarg(int) count;
	} */ *uap = v;

	return sys_readlink(l, uap, retval);
}

#if !defined(__amd64__)
int
linux_sys_truncate(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_truncate_args /* {
		syscallarg(const char *) path;
		syscallarg(long) length;
	} */ *uap = v;

	return compat_43_sys_truncate(l, uap, retval);
}
#endif /* !__amd64__ */

/*
 * This is just fsync() for now (just as it is in the Linux kernel)
 * Note: this is not implemented under Linux on Alpha and Arm
 *	but should still be defined in our syscalls.master.
 *	(syscall #148 on the arm)
 */
int
linux_sys_fdatasync(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
#ifdef notdef
	struct linux_sys_fdatasync_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
#endif
	return sys_fsync(l, v, retval);
}

/*
 * pread(2).
 */
int
linux_sys_pread(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_pread_args /* {
		syscallarg(int) fd;
		syscallarg(void *) buf;
		syscallarg(size_t) nbyte;
		syscallarg(linux_off_t) offset;
	} */ *uap = v;
	struct sys_pread_args pra;

	SCARG(&pra, fd) = SCARG(uap, fd);
	SCARG(&pra, buf) = SCARG(uap, buf);
	SCARG(&pra, nbyte) = SCARG(uap, nbyte);
	SCARG(&pra, offset) = SCARG(uap, offset);

	return sys_pread(l, &pra, retval);
}

/*
 * pwrite(2).
 */
int
linux_sys_pwrite(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_pwrite_args /* {
		syscallarg(int) fd;
		syscallarg(void *) buf;
		syscallarg(size_t) nbyte;
		syscallarg(linux_off_t) offset;
	} */ *uap = v;
	struct sys_pwrite_args pra;

	SCARG(&pra, fd) = SCARG(uap, fd);
	SCARG(&pra, buf) = SCARG(uap, buf);
	SCARG(&pra, nbyte) = SCARG(uap, nbyte);
	SCARG(&pra, offset) = SCARG(uap, offset);

	return sys_pwrite(l, &pra, retval);
}

#define LINUX_NOT_SUPPORTED(fun) \
int \
fun(struct lwp *l, void *v, register_t *retval) \
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
