/*	$NetBSD: irix_stat.c,v 1.3.2.3 2002/04/01 07:43:59 nathanw Exp $ */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: irix_stat.c,v 1.3.2.3 2002/04/01 07:43:59 nathanw Exp $");

#include <sys/errno.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/stdint.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/syscallargs.h>

#include <compat/common/compat_util.h>

#include <compat/svr4/svr4_types.h>

#include <compat/irix/irix_types.h>
#include <compat/irix/irix_signal.h>
#include <compat/irix/irix_syscall.h>
#include <compat/irix/irix_syscallargs.h>

static void bsd_to_irix_stat __P((struct stat *, struct irix_stat *));
static void bsd_to_irix_stat64 __P((struct stat *, struct irix_stat64 *));

static void
bsd_to_irix_stat(bsp, isp)
	struct stat *bsp;
	struct irix_stat *isp;
{
	memset(isp, 0, sizeof(*isp));
	isp->ist_dev = (irix_dev_t)bsd_to_svr4_dev_t(bsp->st_dev);
	isp->ist_ino = bsp->st_ino;
	isp->ist_mode = bsp->st_mode;	/* XXX translate it */
	isp->ist_nlink = bsp->st_nlink;
	isp->ist_uid = bsp->st_uid;
	isp->ist_gid = bsp->st_gid;
	if ((bsp->st_mode & S_IFMT) == S_IFBLK || 
	    (bsp->st_mode & S_IFMT) == S_IFCHR)
		isp->ist_rdev = (irix_dev_t)bsd_to_svr4_dev_t(bsp->st_rdev);
	else
		isp->ist_rdev = 0;
	isp->ist_size = bsp->st_size;
	isp->ist_atim.tv_sec = bsp->st_atimespec.tv_sec;
	isp->ist_atim.tv_nsec = bsp->st_atimespec.tv_nsec;
	isp->ist_mtim.tv_sec = bsp->st_mtimespec.tv_sec;
	isp->ist_mtim.tv_nsec = bsp->st_mtimespec.tv_nsec;
	isp->ist_ctim.tv_sec = bsp->st_ctimespec.tv_sec;
	isp->ist_ctim.tv_nsec = bsp->st_ctimespec.tv_nsec;
	isp->ist_size = bsp->st_size;
	isp->ist_blocks = bsp->st_blocks;
	isp->ist_blksize = bsp->st_blksize;
	strcpy(isp->ist_fstype, "unknown");

	return;
}

static void
bsd_to_irix_stat64(bsp, isp)
	struct stat *bsp;
	struct irix_stat64 *isp;
{
	memset(isp, 0, sizeof(*isp));
	isp->ist_dev = (irix_dev_t)bsd_to_svr4_dev_t(bsp->st_dev);
	isp->ist_ino = bsp->st_ino;
	isp->ist_mode = bsp->st_mode;	/* XXX translate it */
	isp->ist_nlink = bsp->st_nlink;
	isp->ist_uid = bsp->st_uid;
	isp->ist_gid = bsp->st_gid;
	if ((bsp->st_mode & S_IFMT) == S_IFBLK || 
	    (bsp->st_mode & S_IFMT) == S_IFCHR)
		isp->ist_rdev = (irix_dev_t)bsd_to_svr4_dev_t(bsp->st_rdev);
	else
		isp->ist_rdev = 0;
	isp->ist_size = bsp->st_size;
	isp->ist_atim.tv_sec = bsp->st_atimespec.tv_sec;
	isp->ist_atim.tv_nsec = bsp->st_atimespec.tv_nsec;
	isp->ist_mtim.tv_sec = bsp->st_mtimespec.tv_sec;
	isp->ist_mtim.tv_nsec = bsp->st_mtimespec.tv_nsec;
	isp->ist_ctim.tv_sec = bsp->st_ctimespec.tv_sec;
	isp->ist_ctim.tv_nsec = bsp->st_ctimespec.tv_nsec;
	isp->ist_size = bsp->st_size;
	isp->ist_blocks = bsp->st_blocks;
	isp->ist_blksize = bsp->st_blksize;
	strcpy(isp->ist_fstype, "unknown");

	return;
}

int
irix_sys_xstat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct irix_sys_xstat_args /* {
		syscallarg(const int) version;
		syscallarg(const char *) path;
		syscallarg(struct stat *) buf;
	} */ *uap = v;
	struct sys___stat13_args cup;
	struct stat st;
	caddr_t sg = stackgap_init(p, 0);
	int error;

	SCARG(&cup, ub) = stackgap_alloc(p, &sg, sizeof(struct stat));
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	SCARG(&cup, path) = SCARG(uap, path);

	if ((error = sys___stat13(p, &cup, retval)) != 0)
		return error;

	if ((error = copyin(SCARG(&cup, ub), &st, sizeof st)) != 0)
		return error;

	
	switch (SCARG(uap, version)) {
	case IRIX__STAT_VER: {
		struct irix_stat ist;

		bsd_to_irix_stat(&st, &ist);
		if ((error = copyout(&ist, SCARG(uap, buf), 
		    sizeof (struct irix_stat))) != 0)
			return error;
		break;
	}
	case IRIX__STAT64_VER: {
		struct irix_stat64 ist;

		bsd_to_irix_stat64(&st, &ist);
		if ((error = copyout(&ist, SCARG(uap, buf), 
		    sizeof (struct irix_stat64))) != 0)
			return error;
		break;
	}
	case IRIX__R3_STAT_VER:
	default:
		printf("Warning: unimplemented irix_sys_xstat() version %d\n",
		    SCARG(uap, version));
		return EINVAL;
		break;
	}

	return 0;
}

int
irix_sys_lxstat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct irix_sys_lxstat_args /* {
		syscallarg(const int) version;
		syscallarg(const char *) path;
		syscallarg(struct stat *) buf;
	} */ *uap = v;
	struct sys___lstat13_args cup;
	struct stat st;
	caddr_t sg = stackgap_init(p, 0);
	int error;

	SCARG(&cup, ub) = stackgap_alloc(p, &sg, sizeof(struct stat));
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	SCARG(&cup, path) = SCARG(uap, path);

	if ((error = sys___lstat13(p, &cup, retval)) != 0)
		return error;

	if ((error = copyin(SCARG(&cup, ub), &st, sizeof st)) != 0)
		return error;

	
	switch (SCARG(uap, version)) {
	case IRIX__STAT_VER: {
		struct irix_stat ist;

		bsd_to_irix_stat(&st, &ist);
		if ((error = copyout(&ist, SCARG(uap, buf), 
		    sizeof (struct irix_stat))) != 0)
			return error;
		break;
	}
	case IRIX__STAT64_VER: {
		struct irix_stat64 ist;

		bsd_to_irix_stat64(&st, &ist);
		if ((error = copyout(&ist, SCARG(uap, buf), 
		    sizeof (struct irix_stat64))) != 0)
			return error;
		break;
	}
	case IRIX__R3_STAT_VER:
	default:
		printf("Warning: unimplemented irix_sys_lxstat() version %d\n",
		    SCARG(uap, version));
		return EINVAL;
		break;
	}

	return 0;
}

int
irix_sys_fxstat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct irix_sys_fxstat_args /* {
		syscallarg(const int) version;
		syscallarg(const int) fd;
		syscallarg(struct stat *) buf;
	} */ *uap = v;
	struct sys___fstat13_args cup;
	struct stat st;
	int error;
	caddr_t sg = stackgap_init(p, 0);

	SCARG(&cup, sb) = stackgap_alloc(p, &sg, sizeof(struct stat));
	SCARG(&cup, fd) = SCARG(uap, fd);

	if ((error = sys___fstat13(p, &cup, retval)) != 0)
		return error;

	if ((error = copyin(SCARG(&cup, sb), &st, sizeof st)) != 0)
		return error;

	
	switch (SCARG(uap, version)) {
	case IRIX__STAT_VER: {
		struct irix_stat ist;

		bsd_to_irix_stat(&st, &ist);
		if ((error = copyout(&ist, SCARG(uap, buf), 
		    sizeof (struct irix_stat))) != 0)
			return error;
		break;
	}
	case IRIX__STAT64_VER: {
		struct irix_stat64 ist;

		bsd_to_irix_stat64(&st, &ist);
		if ((error = copyout(&ist, SCARG(uap, buf), 
		    sizeof (struct irix_stat64))) != 0)
			return error;
		break;
	}
	case IRIX__R3_STAT_VER:
	default:
		printf("Warning: unimplemented irix_sys_fxstat() version %d\n",
		    SCARG(uap, version));
		return EINVAL;
		break;
	}

	return 0;
}
