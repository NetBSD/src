/*	$NetBSD: netbsd32_machdep_16.c,v 1.1.2.6 2018/09/29 10:02:37 pgoyette Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>.
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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_machdep_16.c,v 1.1.2.6 2018/09/29 10:02:37 pgoyette Exp $");

#include "opt_compat_netbsd.h"
#include "opt_coredump.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/exec.h>
#include <sys/cpu.h>
#include <sys/core.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/module_hook.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_exec.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <mips/cache.h>
#include <mips/sysarch.h>
#include <mips/cachectl.h>
#include <mips/locore.h>
#include <mips/frame.h>
#include <mips/regnum.h>
#include <mips/pcb.h>

#include <uvm/uvm_extern.h>

int netbsd32_sendsig_16(const ksiginfo_t *, const sigset_t *);

void sendsig_context(const ksiginfo_t *, const sigset_t *);

extern struct netbsd32_sendsig_hook_t netbsd32_sendsig_hook;

int
compat_16_netbsd32___sigreturn14(struct lwp *l,
	const struct compat_16_netbsd32___sigreturn14_args *uap,
	register_t *retval)
{
	struct compat_16_sys___sigreturn14_args ua;

	NETBSD32TOP_UAP(sigcntxp, struct sigcontext *);

	return compat_16_sys___sigreturn14(l, &ua, retval);
}

int
netbsd32_sendsig_16(const ksiginfo_t *ksi, const sigset_t *mask)
{               
	if (curproc->p_sigacts->sa_sigdesc[ksi->ksi_signo].sd_vers < 2)
		sendsig_sigcontext(ksi, mask);
	else    
		netbsd32_sendsig_siginfo(ksi, mask);

	return 0;
}       

MODULE_SET_HOOK(netbsd32_sendsig_hook, "nb32_16", netbsd32_sendsig_16); 
MODULE_UNSET_HOOK(netbsd32_sendsig_hook);

void    
netbsd32_machdep_md_init(void)
{       
                
	netbsd32_sendsig_hook_set();
}               
                
void            
netbsd32_machdep_md_fini(void)
{       

	netbsd32_sendsig_hook_unset();
}
