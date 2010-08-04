/* $NetBSD: acpi_cpu_md.c,v 1.4 2010/08/04 16:16:55 jruoho Exp $ */

/*-
 * Copyright (c) 2010 Jukka Ruohonen <jruohonen@iki.fi>
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
__KERNEL_RCSID(0, "$NetBSD: acpi_cpu_md.c,v 1.4 2010/08/04 16:16:55 jruoho Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kcore.h>
#include <sys/xcall.h>

#include <x86/cpu.h>
#include <x86/cpuvar.h>
#include <x86/machdep.h>
#include <x86/cputypes.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpi_cpu.h>

static char	native_idle_text[16];
void	      (*native_idle)(void) = NULL;

extern uint32_t	cpus_running;

uint32_t
acpicpu_md_cap(void)
{
	struct cpu_info *ci = curcpu();
	uint32_t val = 0;

	if (cpu_vendor != CPUVENDOR_INTEL)
		return val;

	/*
	 * Basic SMP C-states (required for _CST).
	 */
	val |= ACPICPU_PDC_C_C1PT | ACPICPU_PDC_C_C2C3;

        /*
	 * If MONITOR/MWAIT is available, announce
	 * support for native instructions in all C-states.
	 */
        if ((ci->ci_feat_val[1] & CPUID2_MONITOR) != 0)
		val |= ACPICPU_PDC_C_C1_FFH | ACPICPU_PDC_C_C2C3_FFH;

	return val;
}

uint32_t
acpicpu_md_quirks(void)
{
	struct cpu_info *ci = curcpu();
	uint32_t val = 0;

	if (acpicpu_md_cpus_running() == 1)
		val |= ACPICPU_FLAG_C_BM;

	if ((ci->ci_feat_val[1] & CPUID2_MONITOR) != 0)
		val |= ACPICPU_FLAG_C_MWAIT;

	switch (cpu_vendor) {

	case CPUVENDOR_INTEL:
		val |= ACPICPU_FLAG_C_BM | ACPICPU_FLAG_C_ARB;

		/*
		 * Bus master arbitration is not
		 * needed on some recent Intel CPUs.
		 */
		if (CPUID2FAMILY(ci->ci_signature) > 15)
			val &= ~ACPICPU_FLAG_C_ARB;

		if (CPUID2FAMILY(ci->ci_signature) == 6 &&
		    CPUID2MODEL(ci->ci_signature) >= 15)
			val &= ~ACPICPU_FLAG_C_ARB;

		break;

	case CPUVENDOR_AMD:

		/*
		 * XXX: Deal with the AMD C1E extension here.
		 */
		break;
	}

	return val;
}

uint32_t
acpicpu_md_cpus_running(void)
{

	return popcount32(cpus_running);
}

int
acpicpu_md_idle_init(void)
{
	const size_t size = sizeof(native_idle_text);

	KASSERT(native_idle == NULL);

	x86_disable_intr();
	x86_cpu_idle_get(&native_idle, native_idle_text, size);
	x86_enable_intr();

	return 0;
}

int
acpicpu_md_idle_start(void)
{

	KASSERT(native_idle != NULL);
	KASSERT(native_idle_text[0] != '\0');

	x86_disable_intr();
	x86_cpu_idle_set(acpicpu_cstate_idle, "acpi");
	x86_enable_intr();

	return 0;
}

int
acpicpu_md_idle_stop(void)
{
	uint64_t xc;

	KASSERT(native_idle != NULL);
	KASSERT(native_idle_text[0] != '\0');

	x86_disable_intr();
	x86_cpu_idle_set(native_idle, native_idle_text);
	x86_enable_intr();

	/*
	 * Run a cross-call to ensure that all CPUs are
	 * out from the ACPI idle-loop before detachment.
	 */
	xc = xc_broadcast(0, (xcfunc_t)nullop, NULL, NULL);
	xc_wait(xc);

	return 0;
}

/*
 * The MD idle loop. Called with interrupts disabled.
 */
void
acpicpu_md_idle_enter(int method, int state)
{
	struct cpu_info *ci = curcpu();

	switch (method) {

	case ACPICPU_C_STATE_FFH:

		x86_enable_intr();
		x86_monitor(&ci->ci_want_resched, 0, 0);

		if (__predict_false(ci->ci_want_resched) != 0)
			return;

		x86_mwait((state - 1) << 4, 0);
		break;

	case ACPICPU_C_STATE_HALT:

		if (__predict_false(ci->ci_want_resched) != 0) {
			x86_enable_intr();
			return;
		}

		x86_stihlt();
		break;
	}
}
