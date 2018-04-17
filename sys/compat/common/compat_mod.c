/*	$NetBSD: compat_mod.c,v 1.24.14.34 2018/04/17 07:24:55 pgoyette Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation
 * by Andrew Doran.
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
__KERNEL_RCSID(0, "$NetBSD: compat_mod.c,v 1.24.14.34 2018/04/17 07:24:55 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#include "opt_compat_43.h"
#endif

#include <sys/systm.h>
#include <sys/module.h>
#include <sys/rwlock.h>
#include <sys/tty.h>
#include <sys/signalvar.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>
#include <sys/syscallvar.h>

#include <compat/common/compat_util.h>
#include <compat/common/compat_mod.h>

static const char * const compat_includes[] = {
	"compat_80", "compat_70", "compat_60", "compat_50", "compat_40",
	"compat_30", "compat_20", "compat_16", "compat_14", "compat_13",
	"compat_12", /*"compat_10", "compat_09",*/ "compat_43", NULL
};

MODULE_WITH_ALIASES(MODULE_CLASS_EXEC, compat, NULL, &compat_includes);

extern krwlock_t ttcompat_lock;

struct compat_init_fini {
	int (*init)(void);
	int (*fini)(void);
} init_fini_list[] = {
#ifdef COMPAT_80
	{ compat_80_init, compat_80_fini },
#endif
#ifdef COMPAT_70
	{ compat_70_init, compat_70_fini },
#endif
#ifdef COMPAT_60
	{ compat_60_init, compat_60_fini },
#endif
#ifdef COMPAT_50
	{ compat_50_init, compat_50_fini },
#endif
#ifdef COMPAT_40
	{ compat_40_init, compat_40_fini },
#endif
#ifdef COMPAT_30
	{ compat_30_init, compat_30_fini },
#endif
#ifdef COMPAT_20
	{ compat_20_init, compat_20_fini },
#endif
#ifdef COMPAT_16
	{ compat_16_init, compat_16_fini },
#endif
#ifdef COMPAT_14
	{ compat_14_init, compat_14_fini },
#endif
#ifdef COMPAT_13
	{ compat_13_init, compat_13_fini },
#endif
#ifdef COMPAT_12
	{ compat_12_init, compat_12_fini },
#endif
#ifdef COMPAT_10
	{ compat_10_init, compat_10_fini },
#endif
#if defined(COMPAT_09) || defined(COMPAT_43)
	{ compat_sysctl_09_43_init, compat_sysctl_09_43_fini },
#endif
#ifdef COMPAT_09
	{ compat_09_init, compat_09_fini },
#endif
#ifdef COMPAT_43
	{ compat_43_init, compat_43_fini },
#endif
};

static int
compat_modcmd(modcmd_t cmd, void *arg)
{
	int error;
	int i, j;

	switch (cmd) {
	case MODULE_CMD_INIT:
		/*
		 * Call the init() routine for all components;  if
		 * any component fails, disable (via fini() routine)
		 * those which had already been disabled before we
		 * return to prevent partial module initialization.
		 */
		for (i = 0; i < __arraycount(init_fini_list); i++) {
			error = (*init_fini_list[i].init)();
			if (error != 0) {
				for (j = i - 1; j >= 0; j--) {
					(*init_fini_list[j].fini)();
				}
				return error;
			}
		}

		return 0;

	case MODULE_CMD_FINI:
		/*
		 * Disable included components in reverse order;
		 * if any component fails to fini(), re-init those
		 * components which had already been disabled
		 */
		for (i = __arraycount(init_fini_list) - 1; i >= 0; i--) {
			error = (*init_fini_list[i].fini)();
			if (error != 0) {
				for (j = i + 1;
				     j < __arraycount(init_fini_list); j++) {
					(*init_fini_list[j].init)();
				}
				return error;
			}
		}
		return 0;

	default:
		return ENOTTY;
	}
}
