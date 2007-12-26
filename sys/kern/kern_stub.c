/*	$NetBSD: kern_stub.c,v 1.4.10.1 2007/12/26 19:57:09 ad Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
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
 * Copyright (c) 1982, 1986, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)subr_xxx.c	8.3 (Berkeley) 3/29/95
 */

/*
 * Stubs for system calls and facilities not included in the system.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_stub.c,v 1.4.10.1 2007/12/26 19:57:09 ad Exp $");

#include "opt_ptrace.h"
#include "opt_ktrace.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/syscallargs.h>
#include <sys/ktrace.h>

/*
 * Nonexistent system call-- signal process (may want to handle it).  Flag
 * error in case process won't see signal immediately (blocked or ignored).
 */
#ifndef PTRACE
__weak_alias(sys_ptrace,sys_nosys);
#endif	/* PTRACE */

/*
 * ktrace stubs.  ktruser() goes to enosys as we want to fail the syscall,
 * but not kill the process: utrace() is a debugging feature.
 */
#ifndef KTRACE
__weak_alias(ktr_csw,nullop);		/* Probes */
__weak_alias(ktr_emul,nullop);
__weak_alias(ktr_geniov,nullop);
__weak_alias(ktr_genio,nullop);
__weak_alias(ktr_mibio,nullop);
__weak_alias(ktr_namei,nullop);
__weak_alias(ktr_namei2,nullop);
__weak_alias(ktr_psig,nullop);
__weak_alias(ktr_syscall,nullop);
__weak_alias(ktr_sysret,nullop);
__weak_alias(ktr_kuser,nullop);
__weak_alias(ktr_mmsg,nullop);
__weak_alias(ktr_mib,nullop);
__weak_alias(ktr_mool,nullop);
__weak_alias(ktr_execarg,nullop);
__weak_alias(ktr_execenv,nullop);

__weak_alias(sys_fktrace,sys_nosys);	/* Syscalls */
__weak_alias(sys_ktrace,sys_nosys);
__weak_alias(sys_utrace,sys_nosys);

int	ktrace_on;			/* Misc */
__weak_alias(ktruser,enosys);
__weak_alias(ktr_point,nullop);
#endif	/* KTRACE */

/*
 * Scheduler activations system calls.  These need to remain until libc's
 * major version is bumped.
 */
__strong_alias(sys_sa_register,sys_nosys);
__strong_alias(sys_sa_stacks,sys_nosys);
__strong_alias(sys_sa_enable,sys_nosys);
__strong_alias(sys_sa_setconcurrency,sys_nosys);
__strong_alias(sys_sa_yield,sys_nosys);
__strong_alias(sys_sa_preempt,sys_nosys);
__strong_alias(sys_sa_unblockyield,sys_nosys);

/* ARGSUSED */
int
sys_nosys(struct lwp *l, const void *v, register_t *retval)
{

	mutex_enter(&proclist_mutex);
	psignal(l->l_proc, SIGSYS);
	mutex_exit(&proclist_mutex);
	return ENOSYS;
}

/*
 * Unsupported device function (e.g. writing to read-only device).
 */
int
enodev(void)
{

	return (ENODEV);
}

/*
 * Unconfigured device function; driver not configured.
 */
int
enxio(void)
{

	return (ENXIO);
}

/*
 * Unsupported ioctl function.
 */
int
enoioctl(void)
{

	return (ENOTTY);
}

/*
 * Unsupported system function.
 * This is used for an otherwise-reasonable operation
 * that is not supported by the current system binary.
 */
int
enosys(void)
{

	return (ENOSYS);
}

/*
 * Return error for operation not supported
 * on a specific object or file type.
 */
int
eopnotsupp(void)
{

	return (EOPNOTSUPP);
}

/*
 * Generic null operation, always returns success.
 */
/*ARGSUSED*/
int
nullop(void *v)
{

	return (0);
}
