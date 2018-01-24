/*	$NetBSD: linux_sys_machdep.c,v 1.15 2018/01/24 09:04:45 skrll Exp $	*/

/*-
 * Copyright (c) 2002 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: linux_sys_machdep.c,v 1.15 2018/01/24 09:04:45 skrll Exp $");

#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/lwp.h>
#include <sys/cpu.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_siginfo.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/linux_syscallargs.h>

#include <arm/cpufunc.h>

int
linux_sys_breakpoint(struct lwp *l, const void *v, register_t *retval)
{
	ksiginfo_t ksi;

	KSI_INIT_TRAP(&ksi);
	ksi.ksi_signo = SIGTRAP;
	ksi.ksi_code = TRAP_BRKPT;
	trapsignal(l, &ksi);
	return ERESTART; /* Leave PC pointing back at the breakpoint. */
}

int
linux_sys_cacheflush(struct lwp *l, const struct linux_sys_cacheflush_args *uap, register_t *retval)
{
	/* {
		syscallarg(uintptr_t) from;
		syscallarg(uintptr_t) to;
		syscallarg(int) flags;
	} */

	cpu_icache_sync_range(SCARG(uap, from),
	    SCARG(uap, to) - SCARG(uap, from) + 1);
	*retval = 0;
	return 0;
}

int
linux_sys_set_tls(struct lwp *l, const struct linux_sys_set_tls_args *uap, register_t *retval)
{

	return lwp_setprivate(l, SCARG(uap, tls));
}
