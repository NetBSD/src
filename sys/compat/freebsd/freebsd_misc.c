/*	$NetBSD: freebsd_misc.c,v 1.30.10.1 2007/12/26 19:48:53 ad Exp $	*/

/*
 * Copyright (c) 1995 Frank van der Linden
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project
 *      by Frank van der Linden
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * FreeBSD compatibility module. Try to deal with various FreeBSD system calls.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: freebsd_misc.c,v 1.30.10.1 2007/12/26 19:48:53 ad Exp $");

#if defined(_KERNEL_OPT)
#include "opt_ntp.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/ktrace.h>

#include <sys/syscallargs.h>

#include <compat/freebsd/freebsd_syscallargs.h>
#include <compat/common/compat_util.h>
#include <compat/freebsd/freebsd_rtprio.h>
#include <compat/freebsd/freebsd_timex.h>
#include <compat/freebsd/freebsd_signal.h>
#include <compat/freebsd/freebsd_mman.h>

int
freebsd_sys_msync(struct lwp *l, const struct freebsd_sys_msync_args *uap, register_t *retval)
{
	/* {
		syscallarg(void *) addr;
		syscallarg(size_t) len;
		syscallarg(int) flags;
	} */
	struct sys___msync13_args bma;

	/*
	 * FreeBSD-2.0-RELEASE's msync(2) is compatible with NetBSD's.
	 * FreeBSD-2.0.5-RELEASE's msync(2) has addtional argument `flags',
	 * but syscall number is not changed. :-<
	 */
	SCARG(&bma, addr) = SCARG(uap, addr);
	SCARG(&bma, len) = SCARG(uap, len);
	SCARG(&bma, flags) = SCARG(uap, flags);
	return sys___msync13(l, &bma, retval);
}

int
freebsd_sys_mmap(struct lwp *l, const struct freebsd_sys_mmap_args *uap, register_t *retval)
{
	/* {
		syscallarg(void *) addr;
		syscallarg(size_t) len;
		syscallarg(int) prot;
		syscallarg(int) flags;
		syscallarg(int) fd;
		syscallarg(long) pad;
		syscallarg(off_t) pos;
	} */
	struct sys_mmap_args bma;
	int flags, prot, fd;
	off_t pos;

	prot = SCARG(uap, prot);
	flags = SCARG(uap, flags);
	fd = SCARG(uap, fd);
	pos = SCARG(uap, pos);

	/*
	 * If using MAP_STACK on FreeBSD:
	 *
	 * + fd has to be -1
	 * + prot must have read and write
	 * + MAP_STACK implies MAP_ANON
	 * + MAP_STACK implies offset of 0
	 */
	if (flags & FREEBSD_MAP_STACK) {
		if ((fd != -1)
		    ||((prot & (PROT_READ|PROT_WRITE))!=(PROT_READ|PROT_WRITE)))
			return (EINVAL);

		flags |= (MAP_ANON | MAP_FIXED);
		flags &= ~FREEBSD_MAP_STACK;

		pos = 0;
	}

	SCARG(&bma, addr) = SCARG(uap, addr);
	SCARG(&bma, len) = SCARG(uap, len);
	SCARG(&bma, prot) = prot;
	SCARG(&bma, flags) = flags;
	SCARG(&bma, fd) = fd;
	SCARG(&bma, pos) = pos;

	return sys_mmap(l, &bma, retval);
}

/* just a place holder */

int
freebsd_sys_rtprio(struct lwp *l, const struct freebsd_sys_rtprio_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) function;
		syscallarg(pid_t) pid;
		syscallarg(struct freebsd_rtprio *) rtp;
	} */

	return ENOSYS;	/* XXX */
}

#ifdef NTP
int
freebsd_ntp_adjtime(struct lwp *l, const struct freebsd_ntp_adjtime_args *uap, register_t *retval)
{
	/* {
		syscallarg(struct freebsd_timex *) tp;
	} */

	return ENOSYS;	/* XXX */
}
#endif

int
freebsd_sys_sigaction4(struct lwp *l, const struct freebsd_sys_sigaction4_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) signum;
		syscallarg(const struct freebsd_sigaction4 *) nsa;
		syscallarg(struct freebsd_sigaction4 *) osa;
	} */
	struct freebsd_sigaction4 nesa, oesa;
	struct sigaction nbsa, obsa;
	int error;

	if (SCARG(uap, nsa)) {
		error = copyin(SCARG(uap, nsa), &nesa, sizeof(nesa));
		if (error)
			return (error);
		nbsa.sa_handler = nesa.freebsd_sa_handler;
		nbsa.sa_mask    = nesa.freebsd_sa_mask;
		nbsa.sa_flags   = nesa.freebsd_sa_flags;
	}
	error = sigaction1(l, SCARG(uap, signum),
	    SCARG(uap, nsa) ? &nbsa : 0, SCARG(uap, osa) ? &obsa : 0,
	    NULL, 0);
	if (error)
		return (error);
	if (SCARG(uap, osa)) {
		oesa.freebsd_sa_handler = obsa.sa_handler;
		oesa.freebsd_sa_mask    = obsa.sa_mask;
		oesa.freebsd_sa_flags   = obsa.sa_flags;
		error = copyout(&oesa, SCARG(uap, osa), sizeof(oesa));
		if (error)
			return (error);
	}
	return (0);
}

int
freebsd_sys_utrace(struct lwp *l, const struct freebsd_sys_utrace_args *uap, register_t *retval)
{
	/* {
		syscallarg(void *) addr;
		syscallarg(size_t) len;
	} */

	return ktruser("FreeBSD utrace", SCARG(uap, addr), SCARG(uap, len),
	    0);
}
