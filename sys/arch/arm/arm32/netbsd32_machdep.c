/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

__KERNEL_RCSID(1, "$NetBSD: netbsd32_machdep.c,v 1.1.2.2 2017/12/03 11:35:51 jdolecek Exp $");

#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/core.h>
#include <sys/exec.h>
#include <sys/lwp.h>
#include <sys/signalvar.h>
#include <sys/syscallargs.h>

#include <uvm/uvm_extern.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_exec.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

const char machine32[] = MACHINE;
const char machine_arch32[] = MACHINE_ARCH;

int
cpu_coredump32(struct lwp *l, struct coredump_iostate *iocookie,
    struct core32 *chdr)
{
	return cpu_coredump(l, iocookie, (struct core *)chdr);
}

void
netbsd32_sendsig (const ksiginfo_t *ksi, const sigset_t *ss)
{
	sendsig(ksi, ss);
}

void
startlwp32(void *arg)
{
	startlwp(arg);
}

int
cpu_mcontext32_validate(struct lwp *l, const mcontext32_t *mcp)
{
	return cpu_mcontext_validate(l, mcp);
}
void
cpu_getmcontext32(struct lwp *l, mcontext32_t *mcp, unsigned int *flagsp)
{
	cpu_getmcontext(l, mcp, flagsp);
}

int
cpu_setmcontext32(struct lwp *l, const mcontext32_t *mcp, unsigned int flags)
{
	return cpu_setmcontext(l, mcp, flags);
}

int
netbsd32_sysarch(struct lwp *l, const struct netbsd32_sysarch_args *uap,
	register_t *retval)
{
	return sys_sysarch(l, (const struct sys_sysarch_args *)uap, retval);
}

vaddr_t
netbsd32_vm_default_addr(struct proc *p, vaddr_t base, vsize_t sz,
    int topdown)
{
	if (topdown)
		return VM_DEFAULT_ADDRESS_TOPDOWN(base, sz);
	else    
		return VM_DEFAULT_ADDRESS_BOTTOMUP(base, sz);
}


#ifdef COMPAT_13
int
compat_13_netbsd32_sigreturn(struct lwp *l,
	const struct compat_13_netbsd32_sigreturn_args *uap,
	register_t *retval)
{
	struct compat_13_sys_sigreturn_args ua;

	NETBSD32TOP_UAP(sigcntxp, struct sigcontext13 *);

	return compat_13_sys_sigreturn(l, &ua, retval);
}
#endif

#ifdef COMPAT_16
int
compat_16_netbsd32___sigreturn14(struct lwp *l,
	const struct compat_16_netbsd32___sigreturn14_args *uap,
	register_t *retval)
{
	struct compat_16_sys___sigreturn14_args ua;

	NETBSD32TOP_UAP(sigcntxp, struct sigcontext *);

	return compat_16_sys___sigreturn14(l, &ua, retval);
}
#endif
