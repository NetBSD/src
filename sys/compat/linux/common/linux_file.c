/*	$NetBSD: linux_file.c,v 1.47 2002/03/22 14:53:26 christos Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_file.c,v 1.47 2002/03/22 14:53:26 christos Exp $");

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
#include <sys/vnode.h>
#include <sys/tty.h>
#include <sys/socketvar.h>
#include <sys/conf.h>
#include <sys/pipe.h>

#include <sys/syscallargs.h>

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
static void bsd_to_linux_stat __P((struct stat *, struct linux_stat *));
static int linux_stat1 __P((struct proc *, void *, register_t *, int));

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
linux_sys_creat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_creat_args /* {
		syscallarg(const char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	struct sys_open_args oa;
	caddr_t sg;

	sg = stackgap_init(p, 0);
	CHECK_ALT_CREAT(p, &sg, SCARG(uap, path));

	SCARG(&oa, path) = SCARG(uap, path);
	SCARG(&oa, flags) = O_CREAT | O_TRUNC | O_WRONLY;
	SCARG(&oa, mode) = SCARG(uap, mode);

	return sys_open(p, &oa, retval);
}

/*
 * open(2). Take care of the different flag values, and let the
 * NetBSD syscall do the real work. See if this operation
 * gives the current process a controlling terminal.
 * (XXX is this necessary?)
 */
int
linux_sys_open(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_open_args /* {
		syscallarg(const char *) path;
		syscallarg(int) flags;
		syscallarg(int) mode;
	} */ *uap = v;
	int error, fl;
	struct sys_open_args boa;
	caddr_t sg;

	sg = stackgap_init(p, 0);

	fl = linux_to_bsd_ioflags(SCARG(uap, flags));

	if (fl & O_CREAT)
		CHECK_ALT_CREAT(p, &sg, SCARG(uap, path));
	else
		CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	SCARG(&boa, path) = SCARG(uap, path);
	SCARG(&boa, flags) = fl;
	SCARG(&boa, mode) = SCARG(uap, mode);

	if ((error = sys_open(p, &boa, retval)))
		return error;

	/*
	 * this bit from sunos_misc.c (and svr4_fcntl.c).
	 * If we are a session leader, and we don't have a controlling
	 * terminal yet, and the O_NOCTTY flag is not set, try to make
	 * this the controlling terminal.
	 */ 
        if (!(fl & O_NOCTTY) && SESS_LEADER(p) && !(p->p_flag & P_CONTROLT)) {
                struct filedesc *fdp = p->p_fd;
                struct file     *fp;

		fp = fd_getfile(fdp, *retval);

                /* ignore any error, just give it a try */
                if (fp != NULL && fp->f_type == DTYPE_VNODE)
                        (fp->f_ops->fo_ioctl) (fp, TIOCSCTTY, (caddr_t) 0, p);
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
linux_sys_fcntl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_fcntl_args /* {
		syscallarg(int) fd;
		syscallarg(int) cmd;
		syscallarg(void *) arg;
	} */ *uap = v;
	int fd, cmd, error;
	u_long val;
	caddr_t arg, sg;
	struct linux_flock lfl;
	struct flock *bfp, bfl;
	struct sys_fcntl_args fca;
	struct filedesc *fdp;
	struct file *fp;
	struct vnode *vp;
	struct vattr va;
	long pgid;
	struct pgrp *pgrp;
	struct tty *tp, *(*d_tty) __P((dev_t));

	fd = SCARG(uap, fd);
	cmd = SCARG(uap, cmd);
	arg = (caddr_t) SCARG(uap, arg);

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
		if ((error = sys_fcntl(p, &fca, retval)))
			return error;
		retval[0] = bsd_to_linux_ioflags(retval[0]);
		return 0;
	case LINUX_F_SETFL: {
		struct file	*fp = NULL;

		val = linux_to_bsd_ioflags((unsigned long)SCARG(uap, arg));

		/*
		 * Linux seems to have same semantics for sending SIGIO to the
		 * read side of socket, but slighly different semantics
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
			if (((fp = fd_getfile(p->p_fd, fd)) == NULL))
			    return (EBADF);

			FILE_USE(fp);

			if (((fp->f_type == DTYPE_SOCKET) && fp->f_data
			      && ((struct socket *)fp->f_data)->so_state & SS_ISAPIPE)
			    || (fp->f_type == DTYPE_PIPE))
				val &= ~O_ASYNC;
			else {
				/* not a pipe, do not modify anything */
				FILE_UNUSE(fp, p);
				fp = NULL;
			}
		}

		SCARG(&fca, fd) = fd;
		SCARG(&fca, cmd) = F_SETFL;
		SCARG(&fca, arg) = (caddr_t) val;

		error = sys_fcntl(p, &fca, retval);

		/* Now set the FASYNC flag for pipes */
		if (fp) {
			if (!error)
				fp->f_flag |= FASYNC;
			FILE_UNUSE(fp, p);
		}

		return (error);
	    }
	case LINUX_F_GETLK:
		sg = stackgap_init(p, 0);
		bfp = (struct flock *) stackgap_alloc(p, &sg, sizeof *bfp);
		if ((error = copyin(arg, &lfl, sizeof lfl)))
			return error;
		linux_to_bsd_flock(&lfl, &bfl);
		if ((error = copyout(&bfl, bfp, sizeof bfl)))
			return error;
		SCARG(&fca, fd) = fd;
		SCARG(&fca, cmd) = F_GETLK;
		SCARG(&fca, arg) = bfp;
		if ((error = sys_fcntl(p, &fca, retval)))
			return error;
		if ((error = copyin(bfp, &bfl, sizeof bfl)))
			return error;
		bsd_to_linux_flock(&bfl, &lfl);
		return copyout(&lfl, arg, sizeof lfl);
		break;

	case LINUX_F_SETLK:
	case LINUX_F_SETLKW:
		cmd = (cmd == LINUX_F_SETLK ? F_SETLK : F_SETLKW);
		if ((error = copyin(arg, &lfl, sizeof lfl)))
			return error;
		linux_to_bsd_flock(&lfl, &bfl);
		sg = stackgap_init(p, 0);
		bfp = (struct flock *) stackgap_alloc(p, &sg, sizeof *bfp);
		if ((error = copyout(&bfl, bfp, sizeof bfl)))
			return error;
		SCARG(&fca, fd) = fd;
		SCARG(&fca, cmd) = cmd;
		SCARG(&fca, arg) = bfp;
		return sys_fcntl(p, &fca, retval);
		break;

	case LINUX_F_SETOWN:
	case LINUX_F_GETOWN:	
		/*
		 * We need to route around the normal fcntl() for these calls,
		 * since it uses TIOC{G,S}PGRP, which is too restrictive for
		 * Linux F_{G,S}ETOWN semantics. For sockets, this problem
		 * does not exist.
		 */
		fdp = p->p_fd;
		if ((fp = fd_getfile(fdp, fd)) == NULL)
			return EBADF;

		FILE_USE(fp);
		switch (fp->f_type) {
		case DTYPE_SOCKET:
			cmd = cmd == LINUX_F_SETOWN ? F_SETOWN : F_GETOWN;
			goto doit;

		case DTYPE_VNODE:
			vp = (struct vnode *)fp->f_data;
			if (vp->v_type != VCHR) {
				error = EINVAL;
				goto done;
			}
			if ((error = VOP_GETATTR(vp, &va, p->p_ucred, p)) != 0)
				goto done;

			d_tty = cdevsw[major(va.va_rdev)].d_tty;
			if (!d_tty || (tp = (*d_tty)(va.va_rdev)) == NULL) {
				error = EINVAL;
				goto done;
			}
			if (cmd == LINUX_F_GETOWN) {
				retval[0] = tp->t_pgrp ? tp->t_pgrp->pg_id
				    : NO_PID;
				error = 0;
				goto done;
			}
			if ((long)arg <= 0) {
				pgid = -(long)arg;
			} else {
				struct proc *p1 = pfind((long)arg);
				if (p1 == NULL) {
					error = ESRCH;
					goto done;
				}
				pgid = (long)p1->p_pgrp->pg_id;
			}
			pgrp = pgfind(pgid);
			if (pgrp == NULL || pgrp->pg_session != p->p_session) {
				error = EPERM;
				goto done;
			}
			tp->t_pgrp = pgrp;
			error = 0;
			goto done;

		case DTYPE_PIPE:
			error = EINVAL;
			goto done;

		default:
			panic("linux_fcntl: Bad file %d\n", fp->f_type);
		}

done:
		FILE_UNUSE(fp, p);
		return error;
	default:
		return EOPNOTSUPP;
	}

doit:
	FILE_UNUSE(fp, p);
	SCARG(&fca, fd) = fd;
	SCARG(&fca, cmd) = cmd;
	SCARG(&fca, arg) = arg;

	return sys_fcntl(p, &fca, retval);
}

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
}

/*
 * The stat functions below are plain sailing. stat and lstat are handled
 * by one function to avoid code duplication.
 */
int
linux_sys_fstat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_fstat_args /* {
		syscallarg(int) fd;
		syscallarg(linux_stat *) sp;
	} */ *uap = v;
	struct sys___fstat13_args fsa;
	struct linux_stat tmplst;
	struct stat *st,tmpst;
	caddr_t sg;
	int error;

	sg = stackgap_init(p, 0);

	st = stackgap_alloc(p, &sg, sizeof (struct stat));

	SCARG(&fsa, fd) = SCARG(uap, fd);
	SCARG(&fsa, sb) = st;

	if ((error = sys___fstat13(p, &fsa, retval)))
		return error;

	if ((error = copyin(st, &tmpst, sizeof tmpst)))
		return error;

	bsd_to_linux_stat(&tmpst, &tmplst);

	if ((error = copyout(&tmplst, SCARG(uap, sp), sizeof tmplst)))
		return error;

	return 0;
}

static int
linux_stat1(p, v, retval, dolstat)
	struct proc *p;
	void *v;
	register_t *retval;
	int dolstat;
{
	struct sys___stat13_args sa;
	struct linux_stat tmplst;
	struct stat *st, tmpst;
	caddr_t sg;
	int error;
	struct linux_sys_stat_args *uap = v;

	sg = stackgap_init(p, 0);
	st = stackgap_alloc(p, &sg, sizeof (struct stat));
	if (dolstat)
		CHECK_ALT_SYMLINK(p, &sg, SCARG(uap, path));
	else
		CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	SCARG(&sa, ub) = st;
	SCARG(&sa, path) = SCARG(uap, path);

	if ((error = (dolstat ? sys___lstat13(p, &sa, retval) :
				sys___stat13(p, &sa, retval))))
		return error;

	if ((error = copyin(st, &tmpst, sizeof tmpst)))
		return error;

	bsd_to_linux_stat(&tmpst, &tmplst);

	if ((error = copyout(&tmplst, SCARG(uap, sp), sizeof tmplst)))
		return error;

	return 0;
}

int
linux_sys_stat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_stat_args /* {
		syscallarg(const char *) path;
		syscallarg(struct linux_stat *) sp;
	} */ *uap = v;

	return linux_stat1(p, uap, retval, 0);
}

/* Note: this is "newlstat" in the Linux sources */
/*	(we don't bother with the old lstat currently) */
int
linux_sys_lstat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_lstat_args /* {
		syscallarg(const char *) path;
		syscallarg(struct linux_stat *) sp;
	} */ *uap = v;

	return linux_stat1(p, uap, retval, 1);
}

/*
 * The following syscalls are mostly here because of the alternate path check.
 */
int
linux_sys_access(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_access_args /* {
		syscallarg(const char *) path;
		syscallarg(int) flags;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_access(p, uap, retval);
}

int
linux_sys_unlink(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;

{
	struct linux_sys_unlink_args /* {
		syscallarg(const char *) path;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_unlink(p, uap, retval);
}

int
linux_sys_chdir(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_chdir_args /* {
		syscallarg(const char *) path;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_chdir(p, uap, retval);
}

int
linux_sys_mknod(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_mknod_args /* {
		syscallarg(const char *) path;
		syscallarg(int) mode;
		syscallarg(int) dev;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);
	struct sys_mkfifo_args bma;

	CHECK_ALT_CREAT(p, &sg, SCARG(uap, path));

	/*
	 * BSD handles FIFOs separately
	 */
	if (SCARG(uap, mode) & S_IFIFO) {
		SCARG(&bma, path) = SCARG(uap, path);
		SCARG(&bma, mode) = SCARG(uap, mode);
		return sys_mkfifo(p, uap, retval);
	} else
		return sys_mknod(p, uap, retval);
}

int
linux_sys_chmod(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_chmod_args /* {
		syscallarg(const char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_chmod(p, uap, retval);
}

#if defined(__i386__) || defined(__m68k__) || defined(__arm__)
int
linux_sys_chown16(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_chown16_args /* {
		syscallarg(const char *) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */ *uap = v;
	struct sys___posix_chown_args bca;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	SCARG(&bca, path) = SCARG(uap, path);
	SCARG(&bca, uid) = ((linux_uid_t)SCARG(uap, uid) == (linux_uid_t)-1) ?
		(uid_t)-1 : SCARG(uap, uid);
	SCARG(&bca, gid) = ((linux_gid_t)SCARG(uap, gid) == (linux_gid_t)-1) ?
		(gid_t)-1 : SCARG(uap, gid);
	
	return sys___posix_chown(p, &bca, retval);
}

int
linux_sys_fchown16(p, v, retval)
	struct proc *p;
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
	
	return sys___posix_fchown(p, &bfa, retval);
}

int
linux_sys_lchown16(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_lchown16_args /* {
		syscallarg(char *) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */ *uap = v;
	struct sys___posix_lchown_args bla;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_SYMLINK(p, &sg, SCARG(uap, path));

	SCARG(&bla, path) = SCARG(uap, path);
	SCARG(&bla, uid) = ((linux_uid_t)SCARG(uap, uid) == (linux_uid_t)-1) ?
		(uid_t)-1 : SCARG(uap, uid);
	SCARG(&bla, gid) = ((linux_gid_t)SCARG(uap, gid) == (linux_gid_t)-1) ?
		(gid_t)-1 : SCARG(uap, gid);

	return sys___posix_lchown(p, &bla, retval);
}
#endif /* __i386__ || __m68k__ || __arm__ */
#if defined (__i386__) || defined (__m68k__) || \
    defined (__powerpc__) || defined (__mips__) || defined(__arm__)
int
linux_sys_chown(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_chown_args /* {
		syscallarg(char *) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys___posix_chown(p, uap, retval);
}

int
linux_sys_lchown(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_lchown_args /* {
		syscallarg(char *) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_SYMLINK(p, &sg, SCARG(uap, path));

	return sys___posix_lchown(p, uap, retval);
}
#endif /* __i386__ || __m68k__ || __powerpc__ || __mips__ || __arm__ */

int
linux_sys_rename(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_rename_args /* {
		syscallarg(const char *) from;
		syscallarg(const char *) to;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, from));
	CHECK_ALT_CREAT(p, &sg, SCARG(uap, to));

	return sys___posix_rename(p, uap, retval);
}

int
linux_sys_mkdir(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_mkdir_args /* {
		syscallarg(const char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_CREAT(p, &sg, SCARG(uap, path));

	return sys_mkdir(p, uap, retval);
}

int
linux_sys_rmdir(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_rmdir_args /* {
		syscallarg(const char *) path;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_rmdir(p, uap, retval);
}

int
linux_sys_symlink(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_symlink_args /* {
		syscallarg(const char *) path;
		syscallarg(const char *) to;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	CHECK_ALT_CREAT(p, &sg, SCARG(uap, to));

	return sys_symlink(p, uap, retval);
}

int
linux_sys_link(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_link_args /* {
		syscallarg(const char *) path;
		syscallarg(const char *) link;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	CHECK_ALT_CREAT(p, &sg, SCARG(uap, link));

	return sys_link(p, uap, retval);
}

int
linux_sys_readlink(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_readlink_args /* {
		syscallarg(const char *) name;
		syscallarg(char *) buf;
		syscallarg(int) count;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_SYMLINK(p, &sg, SCARG(uap, name));

	return sys_readlink(p, uap, retval);
}

int
linux_sys_truncate(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_truncate_args /* {
		syscallarg(const char *) path;
		syscallarg(long) length;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return compat_43_sys_truncate(p, uap, retval);
}

/*
 * This is just fsync() for now (just as it is in the Linux kernel)
 * Note: this is not implemented under Linux on Alpha and Arm
 *	but should still be defined in our syscalls.master.
 *	(syscall #148 on the arm)
 */
int
linux_sys_fdatasync(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
#ifdef notdef
	struct linux_sys_fdatasync_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
#endif
	return sys_fsync(p, v, retval);
}

/*
 * pread(2).
 */
int
linux_sys_pread(p, v, retval)
	struct proc *p;
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

	return sys_read(p, &pra, retval);
}

/*
 * pwrite(2).
 */
int
linux_sys_pwrite(p, v, retval)
	struct proc *p;
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

	return sys_write(p, &pra, retval);
}
