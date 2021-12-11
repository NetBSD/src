/*	$NetBSD: arm_cpu_topology.c,v 1.6 2021/12/11 19:24:20 mrg Exp $	*/

/*
 * Copyright (c) 2020 Matthew R. Green
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

/* CPU topology support for ARMv7 and ARMv8 systems.  */

#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: arm_cpu_topology.c,v 1.6 2021/12/11 19:24:20 mrg Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <arm/cpu.h>
#include <arm/cpu_topology.h>
#include <arm/armreg.h>

#include <prop/proplib.h>

void
arm_cpu_topology_set(struct cpu_info * const ci, mpidr_t mpidr)
{
#ifdef MULTIPROCESSOR
	uint pkgid, coreid, smtid, numaid = 0;

	if (mpidr & MPIDR_MT) {
		pkgid = __SHIFTOUT(mpidr, MPIDR_AFF2);
		coreid = __SHIFTOUT(mpidr, MPIDR_AFF1);
		smtid = __SHIFTOUT(mpidr, MPIDR_AFF0);
	} else {
		pkgid = __SHIFTOUT(mpidr, MPIDR_AFF1);
		coreid = __SHIFTOUT(mpidr, MPIDR_AFF0);
		smtid = 0;
	}
	cpu_topology_set(ci, pkgid, coreid, smtid, numaid);
#endif /* MULTIPROCESSOR */
}

void
arm_cpu_do_topology(struct cpu_info *const newci)
{
#ifdef MULTIPROCESSOR
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;
	prop_dictionary_t dict;
	uint32_t capacity_dmips_mhz = 0;
	static uint32_t best_cap = 0;

	dict = device_properties(newci->ci_dev);
	if (prop_dictionary_get_uint32(dict, "capacity_dmips_mhz",
	    &capacity_dmips_mhz))
		newci->ci_capacity_dmips_mhz = capacity_dmips_mhz;

	if (newci->ci_capacity_dmips_mhz > best_cap)
		best_cap = newci->ci_capacity_dmips_mhz;

	/*
	 * CPU_INFO_FOREACH() doesn't always work for this CPU until
	 * mi_cpu_attach() is called and ncpu is bumped, so call it
	 * directly here.  This also handles the not-MP case.
	 */
	cpu_topology_setspeed(newci, newci->ci_capacity_dmips_mhz < best_cap);

	/*
	 * Using saved largest capacity, refresh previous topology info.
	 * It's supposed to be OK to re-set topology.
	 */
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci == newci)
			continue;
		cpu_topology_setspeed(ci,
		    ci->ci_capacity_dmips_mhz < best_cap);
	}
#endif /* MULTIPROCESSOR */
}
