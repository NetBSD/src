/*	$NetBSD: linux_sys_machdep.c,v 1.12 2006/05/11 11:54:36 yamt Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: linux_sys_machdep.c,v 1.12 2006/05/11 11:54:36 yamt Exp $");

#include <sys/systm.h>
#include <sys/signalvar.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_siginfo.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/linux_syscallargs.h>

#include <arm/cpufunc.h>

int
linux_sys_breakpoint(struct lwp *l, void *v, register_t *retval)
{
	ksiginfo_t ksi;

	KSI_INIT_TRAP(&ksi);
	ksi.ksi_signo = SIGTRAP;
	ksi.ksi_code = TRAP_BRKPT;
	trapsignal(l, &ksi);
	return ERESTART; /* Leave PC pointing back at the breakpoint. */
}

int
linux_sys_cacheflush(struct lwp *l, void *v, register_t *retval)
{
#ifndef acorn26
	struct linux_sys_cacheflush_args /* {
		syscallarg(uintptr_t) from;
		syscallarg(uintptr_t) to;
	} */ *uap = v;

	cpu_icache_sync_range(SCARG(uap, from),
	    SCARG(uap, to) - SCARG(uap, from) + 1);
#endif
	*retval = 0;
	return 0;
}
