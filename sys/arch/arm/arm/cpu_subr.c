/*	$NetBSD: cpu_subr.c,v 1.5 2021/11/14 16:56:32 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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

#include "opt_cputypes.h"
#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu_subr.c,v 1.5 2021/11/14 16:56:32 riastradh Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/reboot.h>

#include <arm/cpufunc.h>

#ifdef VERBOSE_INIT_ARM
#define VPRINTF(...)	printf(__VA_ARGS__)
#else
#define VPRINTF(...)	__nothing
#endif

#ifdef MULTIPROCESSOR
#define NCPUINFO	MAXCPUS
#else
#define NCPUINFO	1
#endif /* MULTIPROCESSOR */

mpidr_t cpu_mpidr[NCPUINFO] = {
	[0 ... NCPUINFO - 1] = ~0,
};

struct cpu_info *cpu_info[NCPUINFO] __read_mostly = {
	[0] = &cpu_info_store[0]
};

#ifdef MULTIPROCESSOR

#define	CPUINDEX_DIVISOR	(sizeof(u_long) * NBBY)

volatile u_long arm_cpu_hatched[howmany(MAXCPUS, CPUINDEX_DIVISOR)] __cacheline_aligned = { 0 };
volatile u_long arm_cpu_mbox[howmany(MAXCPUS, CPUINDEX_DIVISOR)] __cacheline_aligned = { 0 };
u_int arm_cpu_max = 1;

void
cpu_boot_secondary_processors(void)
{
	u_int cpuno;

	if ((boothowto & RB_MD1) != 0)
		return;

	VPRINTF("%s: starting secondary processors\n", __func__);

	/* send mbox to have secondary processors do cpu_hatch() */
	dmb(ish);	/* store-release matches locore.S/armv6_start.S */
	for (size_t n = 0; n < __arraycount(arm_cpu_mbox); n++)
		atomic_or_ulong(&arm_cpu_mbox[n], arm_cpu_hatched[n]);

	dsb(ishst);
	sev();

	/* wait all cpus have done cpu_hatch() */
	for (cpuno = 1; cpuno < ncpu; cpuno++) {
		if (!cpu_hatched_p(cpuno))
			continue;

		const size_t off = cpuno / CPUINDEX_DIVISOR;
		const u_long bit = __BIT(cpuno % CPUINDEX_DIVISOR);

		/* load-acquire matches cpu_clr_mbox */
		while (atomic_load_acquire(&arm_cpu_mbox[off]) & bit) {
			__asm __volatile ("wfe");
		}
		/* Add processor to kcpuset */
		kcpuset_set(kcpuset_attached, cpuno);
	}

	VPRINTF("%s: secondary processors hatched\n", __func__);
}

bool
cpu_hatched_p(u_int cpuindex)
{
	const u_int off = cpuindex / CPUINDEX_DIVISOR;
	const u_int bit = cpuindex % CPUINDEX_DIVISOR;

	/* load-acquire matches cpu_set_hatched */
	return (atomic_load_acquire(&arm_cpu_hatched[off]) & __BIT(bit)) != 0;
}

void
cpu_set_hatched(int cpuindex)
{

	const size_t off = cpuindex / CPUINDEX_DIVISOR;
	const u_long bit = __BIT(cpuindex % CPUINDEX_DIVISOR);

	dmb(ish);		/* store-release matches cpu_hatched_p */
	atomic_or_ulong(&arm_cpu_hatched[off], bit);
	dsb(ishst);
	sev();
}

void
cpu_clr_mbox(int cpuindex)
{

	const size_t off = cpuindex / CPUINDEX_DIVISOR;
	const u_long bit = __BIT(cpuindex % CPUINDEX_DIVISOR);

	/* Notify cpu_boot_secondary_processors that we're done */
	dmb(ish);		/* store-release */
	atomic_and_ulong(&arm_cpu_mbox[off], ~bit);
	dsb(ishst);
	sev();
}

#endif
