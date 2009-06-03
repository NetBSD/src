/*	$NetBSD: linux32_stat.c,v 1.14 2009/06/03 14:17:18 njoly Exp $ */

/*-
 * Copyright (c) 2006 Emmanuel Dreyfus, all rights reserved.
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
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: linux32_stat.c,v 1.14 2009/06/03 14:17:18 njoly Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/fstypes.h>
#include <sys/signal.h>
#include <sys/dirent.h>
#include <sys/kernel.h>
#include <sys/namei.h>
#include <sys/fcntl.h>
#include <sys/filedesc.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/ucred.h>
#include <sys/swap.h>
#include <sys/vfs_syscalls.h>

#include <machine/types.h>

#include <sys/syscallargs.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_conv.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/common/linux_misc.h>
#include <compat/linux/common/linux_oldolduname.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>
#include <compat/linux/linux_syscallargs.h>

#include <compat/linux32/common/linux32_types.h>
#include <compat/linux32/common/linux32_signal.h>
#include <compat/linux32/common/linux32_machdep.h>
#include <compat/linux32/common/linux32_sysctl.h>
#include <compat/linux32/common/linux32_socketcall.h>
#include <compat/linux32/linux32_syscallargs.h>

static inline void linux32_from_stat(struct stat *, struct linux32_stat64 *);

#define linux_fakedev(x,y) (x)
static inline void
linux32_from_stat(struct stat *st, struct linux32_stat64 *st32)
{
	memset(st32, 0, sizeof(*st32));
	st32->lst_dev = linux_fakedev(st->st_dev, 0);
	st32->lst_ino = st->st_ino;
	st32->lst_mode = st->st_mode;
	if (st->st_nlink >= (1 << 15))
		st32->lst_nlink = (1 << 15) - 1;
	else
		st32->lst_nlink = st->st_nlink;
	st32->lst_uid = st->st_uid;
	st32->lst_gid = st->st_gid;
	st32->lst_rdev = linux_fakedev(st->st_rdev, 0);
	st32->lst_size = st->st_size;
	st32->lst_blksize = st->st_blksize;
	st32->lst_blocks = st->st_blocks;
	st32->lst_atime = st->st_atime;
	st32->lst_mtime = st->st_mtime;
	st32->lst_ctime = st->st_ctime;
#ifdef LINUX32_STAT64_HAS_NSEC
	st32->lst_atime_nsec = st->st_atimensec;
	st32->lst_mtime_nsec = st->st_mtimensec;
	st32->lst_ctime_nsec = st->st_ctimensec;
#endif
#ifdef LINUX32_STAT64_HAS_BROKEN_ST_INO
	st32->__lst_ino = st->st_ino;
#endif

	return;
}

int
linux32_sys_stat64(struct lwp *l, const struct linux32_sys_stat64_args *uap, register_t *retval)
{
	/* {
	        syscallarg(netbsd32_charp) path;
	        syscallarg(linux32_statp) sp;
	} */
	int error;
	struct stat st;
	struct linux32_stat64 st32;
	struct linux32_stat64 *st32p;
	const char *path = SCARG_P32(uap, path);
	
	error = do_sys_stat(path, FOLLOW, &st);
	if (error != 0)
		return error;

	linux32_from_stat(&st, &st32);

	st32p = SCARG_P32(uap, sp);

	return copyout(&st32, st32p, sizeof(*st32p));
}

int
linux32_sys_lstat64(struct lwp *l, const struct linux32_sys_lstat64_args *uap, register_t *retval)
{
	/* {
	        syscallarg(netbsd32_charp) path;
	        syscallarg(linux32_stat64p) sp;
	} */
	int error;
	struct stat st;
	struct linux32_stat64 st32;
	struct linux32_stat64 *st32p;
	const char *path = SCARG_P32(uap, path);
	
	error = do_sys_stat(path, NOFOLLOW, &st);
	if (error != 0)
		return error;

	linux32_from_stat(&st, &st32);

	st32p = SCARG_P32(uap, sp);

	return copyout(&st32, st32p, sizeof(*st32p));
}

int
linux32_sys_fstat64(struct lwp *l, const struct linux32_sys_fstat64_args *uap, register_t *retval)
{
	/* {
	        syscallarg(int) fd;
	        syscallarg(linux32_stat64p) sp;
	} */
	int error;
	struct stat st;
	struct linux32_stat64 st32;
	struct linux32_stat64 *st32p;

	error = do_sys_fstat(SCARG(uap, fd), &st);
	if (error != 0)
		return error;

	linux32_from_stat(&st, &st32);

	st32p = SCARG_P32(uap, sp);

	return copyout(&st32, st32p, sizeof(*st32p));
}
