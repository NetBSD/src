/*	$NetBSD: cpu.c,v 1.2 2023/06/12 19:04:14 skrll Exp $	*/

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

#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.2 2023/06/12 19:04:14 skrll Exp $");

#include <sys/param.h>

#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/reboot.h>
#include <sys/sysctl.h>

#include <riscv/cpu.h>
#include <riscv/cpuvar.h>
#include <riscv/machdep.h>
#include <riscv/sbi.h>

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

#define CPU_VENDOR_SIFIVE	0x489

#define CPU_ARCH_7SERIES	0x8000000000000007

struct cpu_arch {
	uint64_t	 ca_id;
	const char	*ca_name;
};

struct cpu_arch cpu_arch_sifive[] = {
    {
	.ca_id = CPU_ARCH_7SERIES,
	.ca_name = "7-Series Processor (E7, S7, U7 series)",
    },
    { },	// terminator
};

struct cpu_vendor {
	uint32_t	 	 cv_id;
	const char		*cv_name;
	struct cpu_arch		*cv_arch;
} cpu_vendors[] = {
    {
	.cv_id = CPU_VENDOR_SIFIVE,
	.cv_name = "SiFive",
	.cv_arch = cpu_arch_sifive,
    },
};

/*
 * Our exported cpu_info structs; these will be first used by the
 * secondary cpus as part of cpu_mpstart and the hatching process.
 */
struct cpu_info cpu_info_store[NCPUINFO] = {
	[0] = {
		.ci_cpl = IPL_HIGH,
		.ci_curlwp = &lwp0,
		.ci_cpl = IPL_HIGH,
#ifdef MULTIPROCESSOR
		.ci_tlb_info = &pmap_tlb0_info,
		.ci_flags = CPUF_PRIMARY | CPUF_PRESENT | CPUF_RUNNING,
#endif
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


static void
cpu_identify(device_t self, struct cpu_info *ci)
{
	const register_t mvendorid = sbi_get_mvendorid().value;
	const register_t marchid = sbi_get_marchid().value;
	const uint32_t mimpid = sbi_get_mimpid().value;
	struct cpu_arch *cv_arch = NULL;
	const char *cv_name = NULL;
	const char *ca_name = NULL;
	char vendor[128];
	char arch[128];

	for (size_t i = 0; i < __arraycount(cpu_vendors); i++) {
		if (mvendorid == cpu_vendors[i].cv_id) {
			cv_name = cpu_vendors[i].cv_name;
			cv_arch = cpu_vendors[i].cv_arch;
			break;
		}
	}

	if (cv_arch != NULL) {
		for (size_t i = 0; cv_arch[i].ca_name != NULL; i++) {
			if (marchid == cv_arch[i].ca_id) {
				ca_name = cv_arch[i].ca_name;
				break;
			}
		}
	}

	if (cv_name == NULL) {
		snprintf(vendor, sizeof(vendor), "vendor %" PRIxREGISTER, mvendorid);
		cv_name = vendor;
	}
	if (ca_name == NULL) {
		snprintf(arch, sizeof(arch), "arch %" PRIxREGISTER, marchid);
		ca_name = arch;
	}

	aprint_naive("\n");
	aprint_normal(": %s %s imp. %" PRIx32 "\n", cv_name, ca_name, mimpid);
        aprint_verbose_dev(ci->ci_dev,
	    "vendor 0x%" PRIxREGISTER " arch. %" PRIxREGISTER " imp. %" PRIx32 "\n",
	    mvendorid, marchid, mimpid);
}


void
cpu_attach(device_t dv, cpuid_t id)
{
	const int unit = device_unit(dv);
	struct cpu_info *ci;

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

	cpu_identify(dv, ci);

#ifdef MULTIPROCESSOR
	kcpuset_create(&ci->ci_shootdowncpus, true);

	ipi_init(ci);

	kcpuset_create(&ci->ci_multicastcpus, true);
	kcpuset_create(&ci->ci_watchcpus, true);
	kcpuset_create(&ci->ci_ddbcpus, true);

	if (unit != 0) {
		mi_cpu_attach(ci);
		struct pmap_tlb_info *ti = kmem_zalloc(sizeof(*ti), KM_SLEEP);
		pmap_tlb_info_init(ti);
		pmap_tlb_info_attach(ti, ci);
	}
#endif /* MULTIPROCESSOR */
	cpu_setup_sysctl(dv, ci);

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
	cpu_set_hatched(cpuindex);

	/*
	 * return to assembly to wait for cpu_boot_secondary_processors
	 */
}


/*
 * When we are called, the MMU and caches are on and we are running on the stack
 * of the idlelwp for this cpu.
 */
void
cpu_hatch(struct cpu_info *ci)
{
	KASSERT(curcpu() == ci);

	ci->ci_cpu_freq = riscv_timer_frequency_get();

	riscv_timer_init();

	/*
	 * clear my bit of the mailbox to tell cpu_boot_secondary_processors().
	 * Consider that if there are cpu0, 1, 2, 3, and cpu2 is unresponsive,
	 * ci_index for each would be cpu0=0, cpu1=1, cpu2=undef, cpu3=2.
	 * therefore we have to use device_unit instead of ci_index for mbox.
	 */

	cpu_clr_mbox(device_unit(ci->ci_dev));
}
#endif /* MULTIPROCESSOR */
