/*	$NetBSD: intel_busclock.c,v 1.13.12.2 2017/12/03 11:36:50 jdolecek Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden,  and by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: intel_busclock.c,v 1.13.12.2 2017/12/03 11:36:50 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/cpu.h>

#include <machine/specialreg.h>
#include <machine/pio.h>

#include <x86/cpuvar.h>
#include <x86/cpufunc.h>
#include <x86/est.h>

int
via_get_bus_clock(struct cpu_info *ci)
{
	uint64_t msr;
	int bus, bus_clock = 0;

	msr = rdmsr(MSR_EBL_CR_POWERON);
	bus = (msr >> 18) & 0x3;
	switch (bus) {
	case 0:
		bus_clock = 10000;
		break;
	case 1:
		bus_clock = 13333;
		break;
	case 2:
		bus_clock = 20000;
		break;
	case 3:
		bus_clock = 16667;
		break;
	default:
		break;
	}

	return bus_clock;
}

int
viac7_get_bus_clock(struct cpu_info *ci)
{
	uint64_t msr;
	int mult;

	msr = rdmsr(MSR_PERF_STATUS);
	mult = (msr >> 8) & 0xff;
	if (mult == 0)
		return 0;

	return ((ci->ci_data.cpu_cc_freq + 10000000) / 10000000 * 10000000) /
		 mult / 10000;
}

int
p3_get_bus_clock(struct cpu_info *ci)
{
	uint64_t msr;
	int bus, bus_clock = 0;
	uint32_t model;

	model = CPUID_TO_MODEL(ci->ci_signature);
		
	switch (model) {
	case 0x9: /* Pentium M (130 nm, Banias) */
		bus_clock = 10000;
		break;
	case 0xc: /* Core i7, Atom, model 1 */
		/*
		 * Newer CPUs will GP when attemping to access MSR_FSB_FREQ.
		 * In the long-term, use ACPI instead of all this.
		 */
		if (rdmsr_safe(MSR_FSB_FREQ, &msr) == EFAULT) {
			aprint_debug_dev(ci->ci_dev,
			    "unable to determine bus speed");
			goto print_msr;
		}
		bus = (msr >> 0) & 0x7;
		switch (bus) {
		case 1:
			bus_clock = 13333;
			break;
		default:
			aprint_debug("%s: unknown Atom FSB_FREQ "
			    "value %d", device_xname(ci->ci_dev), bus);
			goto print_msr;
		}
		break;
	case 0xd: /* Pentium M (90 nm, Dothan) */
		if (rdmsr_safe(MSR_FSB_FREQ, &msr) == EFAULT) {
			aprint_debug_dev(ci->ci_dev,
			    "unable to determine bus speed");
			goto print_msr;
		}
		bus = (msr >> 0) & 0x7;
		switch (bus) {
		case 0:
			bus_clock = 10000;
			break;
		case 1:
			bus_clock = 13333;
			break;
		default:
			aprint_debug("%s: unknown Pentium M FSB_FREQ "
			    "value %d", device_xname(ci->ci_dev), bus);
			goto print_msr;
		}
		break;
	case 0xe: /* Core Duo/Solo */
	case 0xf: /* Core Xeon */
	case 0x17: /* Xeon [35]000, Core 2 Quad [89]00 */
		if (rdmsr_safe(MSR_FSB_FREQ, &msr) == EFAULT) {
			aprint_debug_dev(ci->ci_dev,
			    "unable to determine bus speed");
			goto print_msr;
		}
		bus = (msr >> 0) & 0x7;
		switch (bus) {
		case 5:
			bus_clock = 10000;
			break;
		case 1:
			bus_clock = 13333;
			break;
		case 3:
			bus_clock = 16667;
			break;
		case 2:
			bus_clock = 20000;
			break;
		case 0:
			bus_clock = 26667;
			break;
		case 4:
			bus_clock = 33333;
			break;
		case 6:
			bus_clock = 40000;
			break;
		default:
			aprint_debug("%s: unknown Core FSB_FREQ value %d",
			    device_xname(ci->ci_dev), bus);
			goto print_msr;
		}
		break;
	case 0x1: /* Pentium Pro, model 1 */
	case 0x3: /* Pentium II, model 3 */
	case 0x5: /* Pentium II, II Xeon, Celeron, model 5 */
	case 0x6: /* Celeron, model 6 */
	case 0x7: /* Pentium III, III Xeon, model 7 */
	case 0x8: /* Pentium III, III Xeon, Celeron, model 8 */
	case 0xa: /* Pentium III Xeon, model A */
	case 0xb: /* Pentium III, model B */
		msr = rdmsr(MSR_EBL_CR_POWERON);
		bus = (msr >> 18) & 0x3;
		switch (bus) {
		case 0:
			bus_clock = 6666;
			break;
		case 1:
			bus_clock = 13333;
			break;
		case 2:
			bus_clock = 10000;
			break;
		case 3:
			bus_clock = 10666;
			break;
		default:
			aprint_debug("%s: unknown i686 EBL_CR_POWERON "
			    "value %d ", device_xname(ci->ci_dev), bus);
			goto print_msr;
		}
		break;
	case 0x1c: /* Atom */
	case 0x26:
	case 0x27:
	case 0x35:
	case 0x36:
		if (rdmsr_safe(MSR_FSB_FREQ, &msr) == EFAULT) {
			aprint_debug_dev(ci->ci_dev,
			    "unable to determine bus speed");
			goto print_msr;
		}
		bus = (msr >> 0) & 0x7;
		switch (bus) {
		case 7:
			bus_clock =  8333;
			break;
		case 5:
			bus_clock = 10000;
			break;
		case 1:
			bus_clock = 13333;
			break;
		case 3:
			bus_clock = 16667;
			break;
		default:
			aprint_debug("%s: unknown Atom FSB_FREQ value %d",
			    device_xname(ci->ci_dev), bus);
			goto print_msr;
		}
		break;
	case 0x37: /* Silvermont */
	case 0x4a:
	case 0x4d:
	case 0x5a:
	case 0x5d:
		if (rdmsr_safe(MSR_FSB_FREQ, &msr) == EFAULT) {
			aprint_debug_dev(ci->ci_dev,
			    "unable to determine bus speed");
			goto print_msr;
		}
		bus = (msr >> 0) & 0x7;
		switch (bus) {
		case 4:
			bus_clock =  8000;
			break;
		case 0:
			bus_clock =  8333;
			break;
		case 1:
			bus_clock = 10000;
			break;
		case 2:
			bus_clock = 13333;
			break;
		case 3:
			bus_clock = 11667;
			break;
		default:
			aprint_debug("%s: unknown Silvermont FSB_FREQ value %d",
			    device_xname(ci->ci_dev), bus);
			goto print_msr;
		}
		break;
	case 0x4c: /* Airmont */
		if (rdmsr_safe(MSR_FSB_FREQ, &msr) == EFAULT) {
			aprint_debug_dev(ci->ci_dev,
			    "unable to determine bus speed");
			goto print_msr;
		}
		bus = (msr >> 0) & 0x0f;
		switch (bus) {
		case 0:
			bus_clock =  8333;
			break;
		case 1:
			bus_clock = 10000;
			break;
		case 2:
			bus_clock = 13333;
			break;
		case 3:
			bus_clock = 11666;
			break;
		case 4:
			bus_clock =  8000;
			break;
		case 5:
			bus_clock =  9333;
			break;
		case 6:
			bus_clock =  9000;
			break;
		case 7:
			bus_clock =  8888;
			break;
		case 8:
			bus_clock =  8750;
			break;
		default:
			aprint_debug("%s: unknown Airmont FSB_FREQ value %d",
			    device_xname(ci->ci_dev), bus);
			goto print_msr;
		}
		break;
	default:
		aprint_debug("%s: unknown i686 model %02x, can't get bus clock",
		    device_xname(ci->ci_dev),
		    CPUID_TO_MODEL(ci->ci_signature));
print_msr:
		/*
		 * Show the EBL_CR_POWERON MSR, so we'll at least have
		 * some extra information, such as clock ratio, etc.
		 */
		aprint_debug(" (0x%" PRIu64 ")\n", rdmsr(MSR_EBL_CR_POWERON));
		break;
	}

	return bus_clock;
}

int
p4_get_bus_clock(struct cpu_info *ci)
{
	uint64_t msr;
	int bus, bus_clock = 0;

	msr = rdmsr(MSR_EBC_FREQUENCY_ID);
	if (CPUID_TO_MODEL(ci->ci_signature) < 2) {
		bus = (msr >> 21) & 0x7;
		switch (bus) {
		case 0:
			bus_clock = 10000;
			break;
		case 1:
			bus_clock = 13333;
			break;
		default:
			aprint_debug("%s: unknown Pentium 4 (model %d) "
			    "EBC_FREQUENCY_ID value %d\n",
			    device_xname(ci->ci_dev),
			    CPUID_TO_MODEL(ci->ci_signature), bus);
			break;
		}
	} else {
		bus = (msr >> 16) & 0x7;
		switch (bus) {
		case 0:
			bus_clock = (CPUID_TO_MODEL(ci->ci_signature) == 2) ?
			    10000 : 26667;
			break;
		case 1:
			bus_clock = 13333;
			break;
		case 2:
			bus_clock = 20000;
			break;
		case 3:
			bus_clock = 16667;
			break;
		case 4:
			bus_clock = 33333;
			break;
		default:
			aprint_debug("%s: unknown Pentium 4 (model %d) "
			    "EBC_FREQUENCY_ID value %d\n",
			    device_xname(ci->ci_dev),
			    CPUID_TO_MODEL(ci->ci_signature), bus);
			break;
		}
	}

	return bus_clock;
}
