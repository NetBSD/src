/*	$NetBSD: linux_fcntl64.c,v 1.1 2008/02/02 19:37:53 dsl Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_fcntl64.c,v 1.1 2008/02/02 19:37:53 dsl Exp $");

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
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vfs_syscalls.h>
#include <sys/vnode.h>
#include <sys/tty.h>
#include <sys/conf.h>

#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_fcntl.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/common/linux_dirent.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>

#include <compat/linux/linux_syscallargs.h>

# if !defined(__m68k__) && (!defined(__amd64__) || defined(COMPAT_LINUX32))
static void bsd_to_linux_flock64(struct linux_flock64 *,
    const struct flock *);
static void linux_to_bsd_flock64(struct flock *,
    const struct linux_flock64 *);

static void
bsd_to_linux_flock64(struct linux_flock64 *lfp, const struct flock *bfp)
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
linux_to_bsd_flock64(struct flock *bfp, const struct linux_flock64 *lfp)
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
linux_sys_fcntl64(struct lwp *l, const struct linux_sys_fcntl64_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) cmd;
		syscallarg(void *) arg;
	} */
	struct linux_flock64 lfl;
	struct flock bfl;
	int error;
	void *arg = SCARG(uap, arg);
	int cmd = SCARG(uap, cmd);
	int fd = SCARG(uap, fd);

	switch (cmd) {
	case LINUX_F_GETLK64:
		if ((error = copyin(arg, &lfl, sizeof lfl)) != 0)
			return error;
		linux_to_bsd_flock64(&bfl, &lfl);
		error = do_fcntl_lock(l, fd, F_GETLK, &bfl);
		if (error != 0)
			return error;
		bsd_to_linux_flock64(&lfl, &bfl);
		return copyout(&lfl, arg, sizeof lfl);
	case LINUX_F_SETLK64:
	case LINUX_F_SETLKW64:
		cmd = (cmd == LINUX_F_SETLK64 ? F_SETLK : F_SETLKW);
		if ((error = copyin(arg, &lfl, sizeof lfl)) != 0)
			return error;
		linux_to_bsd_flock64(&bfl, &lfl);
		return do_fcntl_lock(l, fd, cmd, &bfl);
	default:
		return linux_sys_fcntl(l, (const void *)uap, retval);
	}
}
# endif /* !m69k && !amd64  && !COMPAT_LINUX32 */
