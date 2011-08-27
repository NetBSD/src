/* $NetBSD: acpi_pdc.c,v 1.2.4.2 2011/08/27 15:59:49 jym Exp $ */

/*-
 * Copyright (c) 2010, 2011 Jukka Ruohonen <jruohonen@iki.fi>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_pdc.c,v 1.2.4.2 2011/08/27 15:59:49 jym Exp $");

#include <sys/param.h>

#include <x86/cpu.h>
#include <x86/cputypes.h>
#include <x86/cpuvar.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_cpu.h>

#include <machine/acpi_machdep.h>

#define _COMPONENT	ACPI_BUS_COMPONENT
ACPI_MODULE_NAME	("acpi_pdc")

static uint32_t		flags = 0;

static ACPI_STATUS	acpi_md_pdc_walk(ACPI_HANDLE, uint32_t,void *,void **);
static void		acpi_md_pdc_set(ACPI_HANDLE, uint32_t);

uint32_t
acpi_md_pdc(void)
{
	struct cpu_info *ci = curcpu();
	uint32_t regs[4];

	if (flags != 0)
		return flags;

	if (cpu_vendor != CPUVENDOR_IDT &&
	    cpu_vendor != CPUVENDOR_INTEL)
		return 0;

	/*
	 * Basic SMP C-states (required for e.g. _CST).
	 */
	flags |= ACPICPU_PDC_C_C1PT | ACPICPU_PDC_C_C2C3;

	/*
	 * Claim to support dependency coordination.
	 */
	flags |= ACPICPU_PDC_P_SW | ACPICPU_PDC_C_SW | ACPICPU_PDC_T_SW;

        /*
	 * If MONITOR/MWAIT is available, announce
	 * support for native instructions in all C-states.
	 */
        if ((ci->ci_feat_val[1] & CPUID2_MONITOR) != 0)
		flags |= ACPICPU_PDC_C_C1_FFH | ACPICPU_PDC_C_C2C3_FFH;

	/*
	 * Set native P- and T-states, if available.
	 */
        if ((ci->ci_feat_val[1] & CPUID2_EST) != 0)
		flags |= ACPICPU_PDC_P_FFH;

	if ((ci->ci_feat_val[0] & CPUID_ACPI) != 0)
		flags |= ACPICPU_PDC_T_FFH;

	/*
	 * Declare support for APERF and MPERF.
	 */
	if (cpuid_level >= 0x06) {

		x86_cpuid(0x00000006, regs);

		if ((regs[2] & CPUID_DSPM_HWF) != 0)
			flags |= ACPICPU_PDC_P_HWF;
	}

	/*
	 * As the _PDC must be evaluated before the internal namespace
	 * is built, we have no option but to walk with the interpreter.
	 */
	(void)AcpiWalkNamespace(ACPI_TYPE_ANY, ACPI_ROOT_OBJECT,
	    UINT32_MAX, acpi_md_pdc_walk, NULL, NULL, NULL);

	return flags;
}

static ACPI_STATUS
acpi_md_pdc_walk(ACPI_HANDLE hdl, uint32_t level, void *aux, void **sta)
{
	struct cpu_info *ci;

	ci = acpi_match_cpu_handle(hdl);

	if (ci != NULL)
		acpi_md_pdc_set(hdl, flags);

	return AE_OK;
}

static void
acpi_md_pdc_set(ACPI_HANDLE hdl, uint32_t val)
{
	ACPI_OBJECT_LIST arg;
	ACPI_OBJECT obj;
	uint32_t cap[3];

	arg.Count = 1;
	arg.Pointer = &obj;

	cap[0] = ACPICPU_PDC_REVID;
	cap[1] = 1;
	cap[2] = val;

	obj.Type = ACPI_TYPE_BUFFER;
	obj.Buffer.Length = sizeof(cap);
	obj.Buffer.Pointer = (void *)cap;

	(void)AcpiEvaluateObject(hdl, "_PDC", &arg, NULL);
}
