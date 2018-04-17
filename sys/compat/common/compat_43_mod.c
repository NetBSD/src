/*	$NetBSD: compat_43_mod.c,v 1.1.2.1 2018/04/17 08:07:13 pgoyette Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation
 * by Paul Goyette
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
__KERNEL_RCSID(0, "$NetBSD: compat_43_mod.c,v 1.1.2.1 2018/04/17 08:07:13 pgoyette Exp $");

#include <sys/systm.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>

#include <compat/common/compat_util.h>
#include <compat/common/compat_mod.h>

int
compat_43_init(void)
{
	int error;

	error = kern_exit_43_init();
	if (error != 0)
		return error;

	error = kern_info_43_init();
	if (error != 0)
		goto out8;

	error = kern_resource_43_init();
	if (error != 0)
		goto out7;

	error = kern_sig_43_init();
	if (error != 0)
		goto out6;

	error = tty_43_init();
	if (error != 0)
		goto out5;

	error = uipc_syscalls_43_init();
	if (error != 0)
		goto out4;

	error = vfs_syscalls_43_init();
	if (error != 0)
		goto out3;

	error = vm_43_init();
	if (error != 0)
		goto out2;

	error = if_43_init();
	if (error != 0)
		goto out1;

	return 0;

 out1:
	vm_43_fini();
 out2:
	vfs_syscalls_43_fini();
 out3:
	uipc_syscalls_43_fini();
 out4:
	tty_43_fini();
 out5:
	kern_sig_43_fini();
 out6:
	kern_resource_43_fini();
 out7:
	kern_info_43_fini();
 out8:
	kern_exit_43_fini();

	return error;
}

int
compat_43_fini(void)
{
	int error;

	error = if_43_fini();
	if (error != 0)
		return error;

	error = vm_43_fini();
	if (error != 0)
		goto out8;

	error = vfs_syscalls_43_fini();
	if (error != 0)
		goto out7;

	error = uipc_syscalls_43_fini();
	if (error != 0)
		goto out6;

	error = tty_43_fini();
	if (error != 0)
		goto out5;

	error = kern_sig_43_fini();
	if (error != 0)
		goto out4;

	error = kern_resource_43_fini();
	if (error != 0)
		goto out3;

	error = kern_info_43_fini();
	if (error != 0)
		goto out2;

	error = kern_exit_43_fini();
	if (error != 0)
		goto out1;

	return 0;

 out1:
	kern_info_43_init();
 out2:
	kern_resource_43_init();
 out3:
	kern_sig_43_init();
 out4:
	tty_43_init();
 out5:
	uipc_syscalls_43_init();
 out6:
	vfs_syscalls_43_init();
 out7:
	vm_43_init();
 out8:
	if_43_init();

	return error;
}

#ifdef _MODULE

/*
 * XXX We "require" the compat_60 module to ensure proper ordering of
 * XXX vectoring the ttcompatvec replacement routine!
 */
MODULE(MODULE_CLASS_EXEC, compat_43,
    "compat_sysctl_09_43,compat_util,compat_60");

static int
compat_43_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return compat_43_init();
	case MODULE_CMD_FINI:
		return compat_43_init();
	default:
		return ENOTTY;
	}
}
#endif

