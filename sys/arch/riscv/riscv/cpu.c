/*	$NetBSD: cpu.c,v 1.1 2023/05/07 12:41:48 skrll Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.1 2023/05/07 12:41:48 skrll Exp $");

#include <sys/param.h>

#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/sysctl.h>

#include <riscv/cpu.h>
#include <riscv/cpuvar.h>

#ifdef MULTIPROCESSOR
#define NCPUINFO	MAXCPUS
#else
#define NCPUINFO	1
#endif /* MULTIPROCESSOR */

static void
cache_nullop(vaddr_t va, paddr_t pa, psize_t sz)
{
}

void (*cpu_sdcache_wbinv_range)(vaddr_t, paddr_t, psize_t) = cache_nullop;
void (*cpu_sdcache_inv_range)(vaddr_t, paddr_t, psize_t) = cache_nullop;
void (*cpu_sdcache_wb_range)(vaddr_t, paddr_t, psize_t) = cache_nullop;

u_int   riscv_dcache_align = CACHE_LINE_SIZE;
u_int   riscv_dcache_align_mask = CACHE_LINE_SIZE - 1;

/*
 * Our exported cpu_info structs; these will be first used by the
 * secondary cpus as part of cpu_mpstart and the hatching process.
 */
struct cpu_info cpu_info_store[NCPUINFO] = {
	[0] = {
		.ci_cpl = IPL_HIGH,
		.ci_curlwp = &lwp0
	}
};


/*
 * setup the per-cpu sysctl tree.
 */
static void
cpu_setup_sysctl(device_t dv, struct cpu_info *ci)
{
	const struct sysctlnode *cpunode = NULL;

	sysctl_createv(NULL, 0, NULL, &cpunode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, device_xname(dv), NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP,
		       CTL_CREATE, CTL_EOL);

	if (cpunode == NULL)
		return;
}


void
cpu_attach(device_t dv, cpuid_t id)
{
	struct cpu_info *ci = NULL;
	const int unit = device_unit(dv);

	if (unit == 0) {
		ci = curcpu();
		ci->ci_cpuid = id;
	} else {
#ifdef MULTIPROCESSOR
		if ((boothowto & RB_MD1) != 0) {
			aprint_naive("\n");
			aprint_normal(": multiprocessor boot disabled\n");
			return;
		}

		KASSERT(unit < MAXCPUS);
		ci = &cpu_info_store[unit];

		ci->ci_cpl = IPL_HIGH;
		ci->ci_cpuid = id;
		/* ci_cpuid is stored by own cpus when hatching */

		cpu_info[ncpu] = ci;
		if (cpu_hatched_p(unit) == 0) {
			ci->ci_dev = dv;
			device_set_private(dv, ci);
			ci->ci_index = -1;

			aprint_naive(": disabled\n");
			aprint_normal(": disabled (unresponsive)\n");
			return;
		}
#else /* MULTIPROCESSOR */
		aprint_naive(": disabled\n");
		aprint_normal(": disabled (uniprocessor kernel)\n");
		return;
#endif /* MULTIPROCESSOR */
	}

	ci->ci_dev = dv;
	device_set_private(dv, ci);
	aprint_naive("\n");
	aprint_normal("\n");
	cpu_setup_sysctl(dv, ci);

#ifdef MULTIPROCESSOR
	if (unit != 0) {
		mi_cpu_attach(ci);
		pmap_tlb_info_attach(&pmap_tlb0_info, ci);
	}
#endif /* MULTIPROCESSOR */

	if (unit != 0) {
	    return;
	}
}

#ifdef MULTIPROCESSOR
/*
 * Initialise a secondary processor.
 *
 * printf isn't available as kmutex(9) relies on curcpu which isn't setup yet.
 *
 */
void __noasan
cpu_init_secondary_processor(int cpuindex)
{
#if 0
    struct cpu_info * ci = &cpu_info_store[cpuindex];
#endif

}


/*
 * When we are called, the MMU and caches are on and we are running on the stack
 * of the idlelwp for this cpu.
 */
void
cpu_hatch(struct cpu_info *ci)
{
	KASSERT(curcpu() == ci);
}
#endif /* MULTIPROCESSOR */
