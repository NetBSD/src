/*	$NetBSD: compat_90_mod.c,v 1.3.28.1 2023/12/10 13:06:16 martin Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
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

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd.h"
#endif

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: compat_90_mod.c,v 1.3.28.1 2023/12/10 13:06:16 martin Exp $");

#include <sys/systm.h>
#include <sys/module.h>

#include <compat/common/compat_util.h>
#include <compat/common/compat_mod.h>

int
compat_90_init(void)
{

#ifdef INET6
	net_inet6_nd_90_init();
#endif
	return vfs_syscalls_90_init();
}

int
compat_90_fini(void)
{
	int error;

	error = vfs_syscalls_90_fini();
	if (error != 0)
		return error;

#ifdef INET6
	net_inet6_nd_90_fini();
#endif
	return error;
}

MODULE(MODULE_CLASS_EXEC, compat_90, NULL);

static int
compat_90_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return compat_90_init();

	case MODULE_CMD_FINI:
		return compat_90_fini();

	default:
		return ENOTTY;
	}
}

struct timespec boottime;	/* For access by older vmstat */
