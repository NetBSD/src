/*	$NetBSD: linux_file64.c,v 1.2.4.7 2002/04/01 07:44:25 nathanw Exp $	*/

/*-
 * Copyright (c) 1995, 1998, 2000 The NetBSD Foundation, Inc.
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
 * Linux 64bit filesystem calls. Used on 32bit archs, not used on 64bit ones.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_file64.c,v 1.2.4.7 2002/04/01 07:44:25 nathanw Exp $");

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
#include <sys/conf.h>

#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_fcntl.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_machdep.h>

#include <compat/linux/linux_syscallargs.h>

static void bsd_to_linux_stat __P((struct stat *, struct linux_stat64 *));
static int linux_do_stat64 __P((struct lwp *, void *, register_t *, int));

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
	struct linux_stat64 *lsp;
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
	lsp->lst_ino64   = bsp->st_ino;
}

/*
 * The stat functions below are plain sailing. stat and lstat are handled
 * by one function to avoid code duplication.
 */
int
linux_sys_fstat64(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_fstat64_args /* {
		syscallarg(int) fd;
		syscallarg(struct linux_stat64 *) sp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct sys___fstat13_args fsa;
	struct linux_stat64 tmplst;
	struct stat *st,tmpst;
	caddr_t sg;
	int error;

	sg = stackgap_init(p, 0);

	st = stackgap_alloc(p, &sg, sizeof (struct stat));

	SCARG(&fsa, fd) = SCARG(uap, fd);
	SCARG(&fsa, sb) = st;

	if ((error = sys___fstat13(l, &fsa, retval)))
		return error;

	if ((error = copyin(st, &tmpst, sizeof tmpst)))
		return error;

	bsd_to_linux_stat(&tmpst, &tmplst);

	if ((error = copyout(&tmplst, SCARG(uap, sp), sizeof tmplst)))
		return error;

	return 0;
}

static int
linux_do_stat64(l, v, retval, dolstat)
	struct lwp *l;
	void *v;
	register_t *retval;
	int dolstat;
{
	struct proc *p = l->l_proc;
	struct sys___stat13_args sa;
	struct linux_stat64 tmplst;
	struct stat *st, tmpst;
	caddr_t sg;
	int error;
	struct linux_sys_stat64_args *uap = v;

	sg = stackgap_init(p, 0);
	st = stackgap_alloc(p, &sg, sizeof (struct stat));
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	SCARG(&sa, ub) = st;
	SCARG(&sa, path) = SCARG(uap, path);

	if ((error = (dolstat ? sys___lstat13(l, &sa, retval) :
				sys___stat13(l, &sa, retval))))
		return error;

	if ((error = copyin(st, &tmpst, sizeof tmpst)))
		return error;

	bsd_to_linux_stat(&tmpst, &tmplst);

	if ((error = copyout(&tmplst, SCARG(uap, sp), sizeof tmplst)))
		return error;

	return 0;
}

int
linux_sys_stat64(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_stat64_args /* {
		syscallarg(const char *) path;
		syscallarg(struct linux_stat64 *) sp;
	} */ *uap = v;

	return linux_do_stat64(l, uap, retval, 0);
}

int
linux_sys_lstat64(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_lstat64_args /* {
		syscallarg(const char *) path;
		syscallarg(struct linux_stat64 *) sp;
	} */ *uap = v;

	return linux_do_stat64(l, uap, retval, 1);
}

int
linux_sys_truncate64(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_truncate64_args /* {
		syscallarg(const char *) path;
		syscallarg(off_t) length;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return sys_truncate(l, uap, retval);
}

#if defined(__mips__) || defined(__i386__) /* powerpc could use it too */
static void bsd_to_linux_flock64 __P((struct linux_flock64 *,
    const struct flock *));
static void linux_to_bsd_flock64 __P((struct flock *, 
    const struct linux_flock64 *));

static void
bsd_to_linux_flock64(lfp, bfp)
	struct linux_flock64 *lfp;
	const struct flock *bfp;
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
linux_to_bsd_flock64(bfp, lfp)
	struct flock *bfp;
	const struct linux_flock64 *lfp;
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
int
linux_sys_fcntl64(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_fcntl64_args /* {
		syscallarg(int) fd;
		syscallarg(int) cmd;
		syscallarg(void *) arg;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct sys_fcntl_args fca;
	struct linux_flock64 lfl;
	struct flock bfl, *bfp;
	int error;
	caddr_t sg;
	void *arg = SCARG(uap, arg);
	int cmd = SCARG(uap, cmd);
	int fd = SCARG(uap, fd);

	switch (cmd) {
	case LINUX_F_GETLK64:
		sg = stackgap_init(p, 0);
		bfp = (struct flock *) stackgap_alloc(p, &sg, sizeof *bfp);
		if ((error = copyin(arg, &lfl, sizeof lfl)) != 0)
			return error;
		linux_to_bsd_flock64(&bfl, &lfl);
		if ((error = copyout(&bfl, bfp, sizeof bfl)) != 0)
			return error;
		SCARG(&fca, fd) = fd;
		SCARG(&fca, cmd) = F_GETLK;
		SCARG(&fca, arg) = bfp;
		if ((error = sys_fcntl(p, &fca, retval)) != 0)
			return error;
		if ((error = copyin(bfp, &bfl, sizeof bfl)) != 0)
			return error;
		bsd_to_linux_flock64(&lfl, &bfl);
		return copyout(&lfl, arg, sizeof lfl);
	case LINUX_F_SETLK64:
	case LINUX_F_SETLKW64:
		cmd = (cmd == LINUX_F_SETLK64 ? F_SETLK : F_SETLKW);
		if ((error = copyin(arg, &lfl, sizeof lfl)) != 0)
			return error;
		linux_to_bsd_flock64(&bfl, &lfl);
		sg = stackgap_init(p, 0);
		bfp = (struct flock *) stackgap_alloc(p, &sg, sizeof *bfp);
		if ((error = copyout(&bfl, bfp, sizeof bfl)) != 0)
			return error;
		SCARG(&fca, fd) = fd;
		SCARG(&fca, cmd) = cmd;
		SCARG(&fca, arg) = bfp;
		return sys_fcntl(p, &fca, retval);
	default:
		return linux_sys_fcntl(l, v, retval);
	}

	return error;
}
#endif /* __mips__ || __i386__ */
