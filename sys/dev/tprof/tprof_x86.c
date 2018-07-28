/*	$NetBSD: tprof_x86.c,v 1.1.2.2 2018/07/28 04:37:57 pgoyette Exp $	*/

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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
__KERNEL_RCSID(0, "$NetBSD: tprof_x86.c,v 1.1.2.2 2018/07/28 04:37:57 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/module.h>

#include <sys/cpu.h>

#include <dev/tprof/tprof.h>

#include <machine/cpufunc.h>
#include <machine/cputypes.h>	/* CPUVENDOR_* */
#include <machine/cpuvar.h>	/* cpu_vendor */

MODULE(MODULE_CLASS_DRIVER, tprof_x86, "tprof");

extern const tprof_backend_ops_t tprof_amd_ops;
extern const tprof_backend_ops_t tprof_intel_ops;

static int
tprof_x86_init(void)
{
	switch (cpu_vendor) {
	case CPUVENDOR_AMD:
		return tprof_backend_register("tprof_amd", &tprof_amd_ops,
		    TPROF_BACKEND_VERSION);
	case CPUVENDOR_INTEL:
		return tprof_backend_register("tprof_intel", &tprof_intel_ops,
		    TPROF_BACKEND_VERSION);
	default:
		return ENOTSUP;
	}
}

static int
tprof_x86_fini(void)
{
	switch (cpu_vendor) {
	case CPUVENDOR_AMD:
		return tprof_backend_unregister("tprof_amd");
	case CPUVENDOR_INTEL:
		return tprof_backend_unregister("tprof_intel");
	default:
		return ENOTSUP;
	}
}

static int
tprof_x86_modcmd(modcmd_t cmd, void *arg)
{
	switch (cmd) {
	case MODULE_CMD_INIT:
		return tprof_x86_init();
	case MODULE_CMD_FINI:
		return tprof_x86_fini();
	default:
		return ENOTTY;
	}
}
