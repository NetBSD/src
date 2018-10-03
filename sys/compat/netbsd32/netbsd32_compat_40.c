/*	$NetBSD: netbsd32_compat_40.c,v 1.1.2.3 2018/10/03 11:59:21 pgoyette Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_compat_40.c,v 1.1.2.3 2018/10/03 11:59:21 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/module.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/syscallvar.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

int
compat_40_netbsd32_mount(struct lwp *l,
    const struct compat_40_netbsd32_mount_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) type;
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) flags;
		syscallarg(netbsd32_voidp) data;
	} */
	struct compat_40_sys_mount_args ua;
        
	NETBSD32TOP_UAP(type, const char);
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);
	NETBSD32TOP_UAP(data, void);
	return (compat_40_sys_mount(l, &ua, retval));
}

static struct syscall_package compat_netbsd32_40_syscalls[] = {
	{ NETBSD32_SYS_compat_40_netbsd32_mount, 0,
	    (sy_call_t *)compat_40_netbsd32_mount },
	{ 0, 0, NULL }
}; 


MODULE(MODULE_CLASS_EXEC, compat_netbsd32_40, "compat_netbsd32_50,compat_40");

static int
compat_netbsd32_40_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return syscall_establish(&emul_netbsd32,
		    compat_netbsd32_40_syscalls);

	case MODULE_CMD_FINI:
		return syscall_disestablish(&emul_netbsd32,
		    compat_netbsd32_40_syscalls);

	default:
		return ENOTTY;
	}
}
