/*	$NetBSD: netbsd32_compat_50_quota.c,v 1.2 2021/01/19 03:20:13 simonb Exp $	*/

/*-
 * Copyright (c) 2008, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_compat_50_quota.c,v 1.2 2021/01/19 03:20:13 simonb Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd.h"
#include "opt_compat_netbsd32.h"
#include "opt_quota.h"
#endif


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/module.h>
#include <sys/syscallargs.h>
#include <sys/syscallvar.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#if defined(COMPAT_50) && defined(QUOTA)

int
compat_50_netbsd32_quotactl(struct lwp *l, const struct compat_50_netbsd32_quotactl_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) cmd;
		syscallarg(int) uid;
		syscallarg(netbsd32_voidp) arg;
	} */
	struct compat_50_sys_quotactl_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(cmd);
	NETBSD32TO64_UAP(uid);
	NETBSD32TOP_UAP(arg, void *);
	return compat_50_sys_quotactl(l, &ua, retval);
}

static struct syscall_package compat_netbsd32_quota_50_syscalls[] = {
	{ NETBSD32_SYS_compat_50_netbsd32_quotactl, 0,
	    (sy_call_t *)compat_50_netbsd32_quotactl }, 
	{ 0, 0, NULL }
}; 

MODULE(MODULE_CLASS_EXEC, compat_netbsd32_quota_50,
    "compat_netbsd32_50,compat_50,compat_50_quota");

static int
compat_netbsd32_quota_50_modcmd(modcmd_t cmd, void *arg)
{
	int ret;

	switch (cmd) {
	case MODULE_CMD_INIT:
                ret = syscall_establish(&emul_netbsd32,
		    compat_netbsd32_quota_50_syscalls);
		return ret;

	case MODULE_CMD_FINI:
                ret = syscall_disestablish(&emul_netbsd32,
		    compat_netbsd32_quota_50_syscalls);
		return ret;

	default:
		return ENOTTY;
	}
}
#endif /* COMPAT_50 && QUOTA */
