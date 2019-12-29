/*	$NetBSD: compat_30_mod.c,v 1.2.6.1 2019/12/29 09:36:27 martin Exp $	*/

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

/*
 * Linkage for the compat module: spaghetti.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: compat_30_mod.c,v 1.2.6.1 2019/12/29 09:36:27 martin Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd.h"
#endif

#include <sys/systm.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>

#include <dev/biovar.h>

#include <compat/common/compat_util.h>
#include <compat/common/compat_mod.h>

int
compat_30_init(void)
{
	int error = 0;

	error = vfs_syscalls_30_init();
	if (error != 0)
		return error;

	error = kern_time_30_init();
	if (error != 0) {
		vfs_syscalls_30_fini();
		return error;
	}
	error = uipc_syscalls_30_init();
	if (error != 0) {
		kern_time_30_fini();
		vfs_syscalls_30_fini();
		return error;
	}
	bio_30_init();
	vnd_30_init();
	usb_30_init();

	return error;
}

int
compat_30_fini(void)
{
	int error = 0;

	usb_30_fini();
	vnd_30_fini();
	bio_30_fini();

	error = uipc_syscalls_30_fini();
	if (error != 0)
		goto err1;

	error = kern_time_30_fini();
	if (error != 0)
		goto err2;

	error = vfs_syscalls_30_fini();
	if (error != 0)
		goto err3;

	return 0;

 err3:
	kern_time_30_init();
 err2:
	uipc_syscalls_30_init();
 err1:
	bio_30_init();
	vnd_30_init();
	usb_30_init();

	return error;
}

MODULE(MODULE_CLASS_EXEC, compat_30, "compat_util,compat_40");

static int
compat_30_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return compat_30_init();
	case MODULE_CMD_FINI:
		return compat_30_fini();
	default:
		return ENOTTY;
	}
}
