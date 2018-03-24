/*	$NetBSD: compat_80_mod.c,v 1.1.2.4 2018/03/24 01:59:15 pgoyette Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: compat_80_mod.c,v 1.1.2.4 2018/03/24 01:59:15 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#include "opt_compat_43.h"
#endif

#include <sys/systm.h>
#include <sys/module.h>

#include <net/if.h>
#include <net/route.h>

#include <compat/common/compat_util.h>
#include <compat/common/compat_mod.h>
#include <compat/sys/sockio.h>

#include <compat/net/route.h>
#include <compat/net/route_70.h>

int compat_80_init(void)
{
#ifdef NOTYET
	int error;

	error = raidframe_80_init();
	if (error != 0)
		return error;
#endif

	return 0;
}

int compat_80_fini(void)
{
#ifdef NOTYET
	int error;

	error = raidframe_80_fini();
	if (error != 0)
		return error;
#endif

	return 0;
}

#ifdef _MODULE

#define REQD_80 NULL
MODULE(MODULE_CLASS_EXEC, compat_80, REQD_80);

static int
compat_80_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return compat_80_init();

	case MODULE_CMD_FINI:
		return compat_80_fini();

	default:
		return ENOTTY;
	}
}
#endif /* _MODULE */
