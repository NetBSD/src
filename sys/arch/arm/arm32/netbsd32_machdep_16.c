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

__KERNEL_RCSID(1, "$NetBSD: netbsd32_machdep_16.c,v 1.1.2.3 2018/09/27 03:53:30 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#include "opt_compat_netbsd32.h"
#include "opt_coredump.h"
#endif

#include <sys/param.h>
#include <sys/core.h>
#include <sys/exec.h>
#include <sys/lwp.h>
#include <sys/signalvar.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>

#include <uvm/uvm_extern.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_exec.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

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

static struct syscall_package compat_arm32_netbsd32_16_syscalls[] = {
        { NETBSD32_SYS_compat_16_netbsd32___sigreturn14, 0,
            (sy_call_t *)compat_16_netbsd32___sigreturn14 },
	{ 0, 0, NULL }
};

void
netbsd32_machdep_md_16_init(void)
{
 
	(void)syscall_establish(&emul_netbsd32,
	    compat_arm32_netbsd32_16_syscalls);
}
   
void
netbsd32_machdep_md_16_fini(void)
{

	(void)syscall_disestablish(&emul_netbsd32,
	    compat_arm32_netbsd32_16_syscalls);
}
#endif
