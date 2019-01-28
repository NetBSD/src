/*	$NetBSD: netbsd32_compat_40.c,v 1.4 2019/01/28 18:53:52 mlelstv Exp $	*/

/*
 * Copyright (c) 1998, 2001, 2008, 2018 Matthew R. Green
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: NetBSD: netbsd32_netbsd.c,v 1.221 2018/12/24 20:44:39 mrg Exp $
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_compat_40.c,v 1.4 2019/01/28 18:53:52 mlelstv Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/module.h>
#include <sys/mount.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

int
compat_40_netbsd32_mount(struct lwp *l,
			 const struct compat_40_netbsd32_mount_args *uap,
			 register_t *retval)
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

	return compat_40_sys_mount(l, &ua, retval);
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

