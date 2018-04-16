/*	$NetBSD: compat_50_mod.c,v 1.1.2.8 2018/04/16 03:41:34 pgoyette Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: compat_50_mod.c,v 1.1.2.8 2018/04/16 03:41:34 pgoyette Exp $");

#include <sys/systm.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>

#include <opencrypto/ocryptodev.h>

#include <compat/sys/clockctl.h>

#include <compat/common/compat_util.h>
#include <compat/common/compat_mod.h>
#include <compat/common/if_spppsubr50.h>

#include <dev/raidframe/rf_compat50_mod.h>

#include <dev/wscons/wsevent_50.h>

#include <fs/puffs/puffs_sys.h>

int
compat_50_init(void)
{
	int error = 0;

	error = kern_50_init();
	if (error != 0)
		return error;

	error = kern_time_50_init();
	if (error != 0)
		goto err1;

	error = kern_select_50_init();
	if (error != 0)
		goto err2;

	error = vfs_syscalls_50_init();
	if (error != 0)
		goto err3;

	uvm_50_init();
	uipc_syscalls_50_init();
	clockctl_50_init();
	if_spppsubr_50_init();
	cryptodev_50_init();
	raidframe_50_init();
	puffs_50_init();
	wsevent_50_init();

	return error;

/* If an error occured, undo all previous set-up before returning */

 err3:
	kern_select_50_fini();
 err2:
	kern_time_50_fini();
 err1:
	kern_50_fini();

	return error;
}

int
compat_50_fini(void)
{
	int error = 0;

	wsevent_50_fini();
	puffs_50_fini();
	raidframe_50_fini();
	cryptodev_50_fini();
	if_spppsubr_50_fini();
	clockctl_50_fini();
	uipc_syscalls_50_fini();
	uvm_50_fini();

	error = vfs_syscalls_50_fini();
	if (error != 0)
		goto err1;

	error = kern_select_50_fini();
	if (error != 0)
		goto err2;

	error = kern_time_50_fini();
	if (error != 0)
		goto err3;

	error = kern_50_fini();
	if (error != 0)
		goto err4;

	return error;

/* If an error occurred while removing something, restore everything! */
 err4:
	kern_time_50_init();
 err3:
	kern_select_50_init();
 err2:
	vfs_syscalls_50_init();
 err1:
	uvm_50_init();
	uipc_syscalls_50_init();
	clockctl_50_init();
	if_spppsubr_50_init();
	cryptodev_50_init();
	raidframe_50_init();
	puffs_50_init();
	wsevent_50_init();

	return error;
}

#ifdef _MODULE

#define REQD_50	"compat_80,compat_70,compat_60"

MODULE(MODULE_CLASS_EXEC, compat_50, REQD_50);

static int
compat_50_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return compat_50_init();
	case MODULE_CMD_FINI:
		return compat_50_init();
	default:
		return ENOTTY;
	}
}
#endif
