/*	$NetBSD: compat_09_mod.c,v 1.1.2.1 2018/04/17 08:07:13 pgoyette Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: compat_09_mod.c,v 1.1.2.1 2018/04/17 08:07:13 pgoyette Exp $");

#include <sys/systm.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>
#include <sys/compat_stub.h>

#include <compat/common/compat_util.h>
#include <compat/common/compat_mod.h>


static int saved_pgid_mask;

int
compat_09_init(void)
{

	kern_info_09_init();
	saved_pgid_mask = kern_sig_43_pgid_mask;
	kern_sig_43_pgid_mask = 0xffff;
	return 0;
}

int
compat_09_fini(void)
{

	kern_info_09_fini();
	kern_sig_43_pgid_mask = saved_pgid_mask;
	return 0;
}

#ifdef _MODULE

#define REQD_09_1	"compat_80,compat_70,compat_60,compat_50,"
#define REQD_09_2	"compat_40,compat_30,compat_20,compat_16,"
#define REQD_09_3	"compat_14,compat_13,compat_12,compat_10,"
#define REQD_09_4	"compat_util,compat_sysctl_09_43"

MODULE(MODULE_CLASS_EXEC, compat_09, REQD_09_1 REQD_09_2 REQD_09_3 REQD_09_4);

static int
compat_09_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return compat_09_init();
	case MODULE_CMD_FINI:
		return compat_09_init();
	default:
		return ENOTTY;
	}
}
#endif
