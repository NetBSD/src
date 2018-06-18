/* $NetBSD: psci.c,v 1.1.4.3 2018/06/18 15:34:34 martin Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
 */

#include "opt_diagnostic.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: psci.c,v 1.1.4.3 2018/06/18 15:34:34 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <arm/arm/psci.h>

/* PSCI methods */
#define	PSCI_VERSION		0x84000000
#define	PSCI_SYSTEM_OFF		0x84000008
#define	PSCI_SYSTEM_RESET	0x84000009
#if defined(__aarch64__)
#define	PSCI_CPU_ON		0xc4000003
#else
#define	PSCI_CPU_ON		0x84000003
#endif

static psci_fn psci_call_fn;

static uint32_t psci_functions[PSCI_FUNC_MAX] = {
        [PSCI_FUNC_VERSION] = PSCI_VERSION,
        [PSCI_FUNC_SYSTEM_OFF] = PSCI_SYSTEM_OFF,
	[PSCI_FUNC_SYSTEM_RESET] = PSCI_SYSTEM_RESET,
	[PSCI_FUNC_CPU_ON] = PSCI_CPU_ON,
};

static int
psci_call(register_t fid, register_t arg1, register_t arg2, register_t arg3)
{
	KASSERT(psci_call_fn != NULL);

	if (fid == 0)
		return PSCI_NOT_SUPPORTED;

	return psci_call_fn(fid, arg1, arg2, arg3);
} 

uint32_t
psci_version(void)
{
	if (psci_functions[PSCI_FUNC_VERSION] == 0) {
		/* Pre-0.2 implementation */
		return __SHIFTIN(0, PSCI_VERSION_MAJOR) |
		       __SHIFTIN(1, PSCI_VERSION_MINOR);
	}

	return psci_call(psci_functions[PSCI_FUNC_VERSION], 0, 0, 0);
}

int
psci_cpu_on(register_t target_cpu, register_t entry_point_address,
    register_t context_id)
{
	return psci_call(psci_functions[PSCI_FUNC_CPU_ON], target_cpu,
	    entry_point_address, context_id);
}

void
psci_system_off(void)
{
	psci_call(psci_functions[PSCI_FUNC_SYSTEM_OFF], 0, 0, 0);
}

void
psci_system_reset(void)
{
	psci_call(psci_functions[PSCI_FUNC_SYSTEM_RESET], 0, 0, 0);
}

void
psci_init(psci_fn fn)
{
	psci_call_fn = fn;
}

void
psci_clearfunc(void)
{
	for (int i = 0; i < PSCI_FUNC_MAX; i++)
		psci_setfunc(i, 0);
}

void
psci_setfunc(enum psci_function func, uint32_t fid)
{
	psci_functions[func] = fid;
}
