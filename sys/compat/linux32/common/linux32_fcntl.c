/*	$NetBSD: linux32_fcntl.c,v 1.9.14.1 2014/08/20 00:03:33 tls Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: linux32_fcntl.c,v 1.9.14.1 2014/08/20 00:03:33 tls Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <sys/fcntl.h>
#include <sys/filedesc.h>

#include <machine/types.h>

#include <sys/syscallargs.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_fcntl.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/common/linux_misc.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>
#include <compat/linux/linux_syscallargs.h>

#include <compat/linux32/common/linux32_types.h>
#include <compat/linux32/common/linux32_signal.h>
#include <compat/linux32/common/linux32_machdep.h>
#include <compat/linux32/linux32_syscallargs.h>

struct linux32_flock {
	short       l_type;
	short       l_whence;
	int32_t     l_start;
	int32_t     l_len;
	linux_pid_t l_pid;
};

struct linux32_flock64 {
	short           l_type;
	short           l_whence;
	netbsd32_int64	l_start;
	netbsd32_int64	l_len;
	linux_pid_t     l_pid;
};

conv_linux_flock(linux32, flock)
conv_linux_flock(linux32, flock64)

int
linux32_sys_open(struct lwp *l, const struct linux32_sys_open_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) flags;
		syscallarg(int) mode;
	} */
	struct linux_sys_open_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);
	NETBSD32TO64_UAP(mode);

	return linux_sys_open(l, &ua, retval);
}

/*
 * The linux support for 64bit file offsets introduced both an fcntl64()
 * function and LINUX_F_SETLK64 (et al), however the fcntl64() function
 * can still be passed LINUX_F_GETLK (etc).
 * In practice this means that the two functions are identical!
 *
 * We have to intercept both sets of locking primitives because the
 * structure layout of the 64bit version differs from that on amd64 due
 * to extra padding because the alignment constraint for int64_t differs.
 */

int
linux32_sys_fcntl(struct lwp *l, const struct linux32_sys_fcntl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
                syscallarg(int) cmd;
		syscallarg(netbsd32_voidp) arg;
	} */
	struct linux_sys_fcntl_args ua;
	int cmd =  SCARG(uap, cmd);

	switch (cmd) {
	case LINUX_F_GETLK64:
		do_linux_getlk(SCARG(uap, fd), cmd, SCARG_P32(uap, arg),
		    linux32, flock64);
	case LINUX_F_SETLK64:
	case LINUX_F_SETLKW64:
		do_linux_setlk(SCARG(uap, fd), cmd, SCARG_P32(uap, arg),
		    linux32, flock64, LINUX_F_SETLK64);
	case LINUX_F_GETLK:
		do_linux_getlk(SCARG(uap, fd), cmd, SCARG_P32(uap, arg),
		    linux32, flock);
	case LINUX_F_SETLK:
	case LINUX_F_SETLKW:
		do_linux_setlk(SCARG(uap, fd), cmd, SCARG_P32(uap, arg),
		    linux32, flock, LINUX_F_SETLK);
	default:
		break;
	}

	NETBSD32TO64_UAP(fd);
	SCARG(&ua, cmd) = cmd;
	NETBSD32TOP_UAP(arg, void);

	return linux_sys_fcntl(l, &ua, retval);
}

int
linux32_sys_fadvise64(struct lwp *l,
    const struct linux32_sys_fadvise64_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(uint32_t) offlo;
		syscallarg(uint32_t) offhi;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) advice;
	} */
	off_t off = ((off_t)SCARG(uap, offhi) << 32) + SCARG(uap, offlo);

	return do_posix_fadvise(SCARG(uap, fd), off,
	    SCARG(uap, len), linux_to_bsd_posix_fadv(SCARG(uap, advice)));
}

int
linux32_sys_fadvise64_64(struct lwp *l,
    const struct linux32_sys_fadvise64_64_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(uint32_t) offlo;
		syscallarg(uint32_t) offhi;
		syscallarg(uint32_t) lenlo;
		syscallarg(uint32_t) lenhi;
		syscallarg(int) advice;
	} */
	off_t off = ((off_t)SCARG(uap, offhi) << 32) + SCARG(uap, offlo);
	off_t len = ((off_t)SCARG(uap, lenhi) << 32) + SCARG(uap, lenlo);

	return do_posix_fadvise(SCARG(uap, fd), off,
	    len, linux_to_bsd_posix_fadv(SCARG(uap, advice)));
}
