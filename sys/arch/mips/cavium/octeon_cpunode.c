/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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
#define __INTR_PRIVATE
#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD");

#include "locators.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/lwp.h>
#include <sys/cpu.h>

#include <mips/cache.h>
#include <mips/cpuset.h>
#include <mips/mips_opcode.h>

#include <mips/cavium/octeonvar.h>
#include <mips/cavium/dev/octeon_ciureg.h>
#include <mips/cavium/dev/octeon_corereg.h>

struct cpunode_attach_args {
	const char *cnaa_name;
	int cnaa_cpunum;
};

static int cpunode_mainbus_match(device_t, cfdata_t, void *);
static void cpunode_mainbus_attach(device_t, device_t, void *);

static int cpu_cpunode_match(device_t, cfdata_t, void *);
static void cpu_cpunode_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpunode, 0,
    cpunode_mainbus_match, cpunode_mainbus_attach, NULL, NULL);

CFATTACH_DECL_NEW(cpunode_cpu, 0,
    cpu_cpunode_match, cpu_cpunode_attach, NULL, NULL);

volatile __cpuset_t cpus_booted = 1;

static int
cpunode_mainbus_print(void *aux, const char *pnp)
{
	struct cpunode_attach_args * const cnaa = aux;

	aprint_normal(" core %d", cnaa->cnaa_cpunum);

	return UNCONF;
}

int
cpunode_mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	
	return 1;
}

void
cpunode_mainbus_attach(device_t parent, device_t self, void *aux)
{
	uint64_t fuse = octeon_xkphys_read_8(CIU_FUSE);
	int cpunum = 0;

	aprint_naive(": %u core%s\n",
	    popcount32((uint32_t)fuse),
	    fuse == 1 ? "" : "s");

	aprint_normal(": %u core%s",
	    popcount32((uint32_t)fuse),
	    fuse == 1 ? "" : "s");
	const uint64_t cvmctl = mips_cp0_cvmctl_read();
	aprint_normal(", %scrypto", (cvmctl & CP0_CVMCTL_NOCRYPTO) ? "no " : "");
	aprint_normal((cvmctl & CP0_CVMCTL_KASUMI) ? "+kasumi" : "");
	aprint_normal(", %s64bit-mul", (cvmctl & CP0_CVMCTL_NOMUL) ? "no " : "");
	if (cvmctl & CP0_CVMCTL_REPUN)
		aprint_normal(", unaligned-access ok");
	aprint_normal("\n");

	for (; fuse != 0; fuse >>= 1, cpunum++) {
		struct cpunode_attach_args cnaa = {
			.cnaa_name = "cpu",
			.cnaa_cpunum = cpunum,
		};
		config_found(self, &cnaa, cpunode_mainbus_print);
	}
}

int
cpu_cpunode_match(device_t parent, cfdata_t cf, void *aux)
{
	struct cpunode_attach_args * const cnaa = aux;
	const int cpunum = cf->cf_loc[CPUNODECF_CORE];

	return cpunum == CPUNODECF_CORE_DEFAULT
	    || cpunum == cnaa->cnaa_cpunum;
}

#if defined(MULTIPROCESSOR)
static bool
octeon_fixup_cpu_info_references(int32_t load_addr, uint32_t new_insns[2],
    void *arg)
{
	struct cpu_info * const ci = arg;

	KASSERT(MIPS_KSEG0_P(load_addr));
	KASSERT(!MIPS_CACHE_VIRTUAL_ALIAS);
#ifdef MULTIPROCESSOR
	KASSERT(!CPU_IS_PRIMARY(curcpu()));
#endif
	load_addr += (intptr_t)ci - (intptr_t)&cpu_info_store;

	KASSERT((intptr_t)ci <= load_addr);
	KASSERT(load_addr < (intptr_t)(ci + 1));

	KASSERT(INSN_LUI_P(new_insns[0]));
	KASSERT(INSN_LOAD_P(new_insns[1]) || INSN_STORE_P(new_insns[1]));

	/*
	 * Use the lui and load/store instruction as a prototype and
	 * make it refer to cpu1_info_store instead of cpu_info_store.
	 */
	new_insns[0] &= __BITS(31,16);
	new_insns[1] &= __BITS(31,16);
	new_insns[0] |= (uint16_t)((load_addr + 0x8000) >> 16);
	new_insns[1] |= (uint16_t)load_addr;
#ifdef DEBUG_VERBOSE
	printf("%s: %08x: insn#1 %08x: lui r%u, %d\n",
	    __func__, (int32_t)load_addr, new_insns[0],
	    (new_insns[0] >> 16) & 31,
	    (int16_t)new_insns[0]);
	printf("%s: %08x: insn#2 %08x: %c%c r%u, %d(r%u)\n",
	    __func__, (int32_t)load_addr, new_insns[0],
	    INSN_LOAD_P(new_insns[1]) ? 'l' : 's',
	    INSN_LW_P(new_insns[1]) ? 'w' : 'd',
	    (new_insns[0] >> 16) & 31,
	    (int16_t)new_insns[1],
	    (new_insns[0] >> 21) & 31);
#endif
	return true;
}

static void
octeon_cpu_init(struct cpu_info *ci)
{
	bool ok __diagused;

	// First thing is setup the execption vectors for this cpu.
	mips64r2_vector_init(&mips_splsw);

	// Next rewrite those exceptions to use this cpu's cpu_info.
	ok = mips_fixup_exceptions(octeon_fixup_cpu_info_references, ci);
	KASSERT(ok);

	(void) splhigh();

#ifdef DEBUG
	KASSERT((mipsNN_cp0_ebase_read() & MIPS_EBASE_CPUNUM) == ci->ci_cpuid);
	KASSERT(curcpu() == ci);
#endif
}

static void
octeon_cpu_run(struct cpu_info *ci)
{
}
#endif /* MULTIPROCESSOR */

static void
cpu_cpunode_attach_common(device_t self, struct cpu_info *ci)
{
	ci->ci_dev = self;
	self->dv_private = ci;

	aprint_normal(": %lu.%02luMHz (hz cycles = %lu, delay divisor = %lu)\n",
	    ci->ci_cpu_freq / 1000000,
	    (ci->ci_cpu_freq % 1000000) / 10000,
	    ci->ci_cycles_per_hz, ci->ci_divisor_delay);

	aprint_normal("%s: ", device_xname(self));
	cpu_identify(self);
	cpu_attach_common(self, ci);
}

void
cpu_cpunode_attach(device_t parent, device_t self, void *aux)
{
	struct cpunode_attach_args * const cnaa = aux;
	const int cpunum = cnaa->cnaa_cpunum;

	if (cpunum == 0) {
		cpu_cpunode_attach_common(self, curcpu());
#ifdef MULTIPROCESSOR
		mips_locoresw.lsw_cpu_init = octeon_cpu_init;
		mips_locoresw.lsw_cpu_run = octeon_cpu_run;
#endif
		return;
	}
#ifdef MULTIPROCESSOR
	KASSERTMSG(cpunum == 1, "cpunum %d", cpunum);
	if (!CPUSET_HAS_P(cpus_booted, cpunum)) {
		aprint_naive(" disabled\n");
		aprint_normal(" disabled (unresponsive)\n");
		return;
	}
	struct cpu_info * const ci = cpu_info_alloc(NULL, cpunum, 0, cpunum, 0);

	ci->ci_softc = &octeon_cpu1_softc;
	ci->ci_softc->cpu_ci = ci;

	cpu_cpunode_attach_common(self, ci);
#else
	aprint_naive(": disabled\n");
	aprint_normal(": disabled (uniprocessor kernel)\n");
#endif
}
