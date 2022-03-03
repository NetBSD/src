/*      $NetBSD: octeon_cpunode.c,v 1.22 2022/03/03 06:27:41 riastradh Exp $   */

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

__KERNEL_RCSID(0, "$NetBSD: octeon_cpunode.c,v 1.22 2022/03/03 06:27:41 riastradh Exp $");

#include "locators.h"
#include "cpunode.h"
#include "opt_multiprocessor.h"
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/lwp.h>
#include <sys/reboot.h>
#include <sys/wdog.h>

#include <uvm/uvm.h>

#include <dev/sysmon/sysmonvar.h>

#include <mips/cache.h>
#include <mips/mips_opcode.h>
#include <mips/mips3_clock.h>
#include <mips/mips3_pte.h>

#include <mips/cavium/octeonvar.h>
#include <mips/cavium/dev/octeon_ciureg.h>
#include <mips/cavium/dev/octeon_corereg.h>

extern struct cpu_softc octeon_cpu_softc[];

struct cpunode_attach_args {
	const char *cnaa_name;
	int cnaa_cpunum;
};

struct cpunode_softc {
	device_t sc_dev;
	device_t sc_wdog_dev;
};

static int cpunode_mainbus_match(device_t, cfdata_t, void *);
static void cpunode_mainbus_attach(device_t, device_t, void *);

static int cpu_cpunode_match(device_t, cfdata_t, void *);
static void cpu_cpunode_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpunode, sizeof(struct cpunode_softc),
    cpunode_mainbus_match, cpunode_mainbus_attach, NULL, NULL);

CFATTACH_DECL_NEW(cpu_cpunode, 0,
    cpu_cpunode_match, cpu_cpunode_attach, NULL, NULL);

#ifdef MULTIPROCESSOR
CTASSERT(MAXCPUS <= sizeof(uint64_t) * NBBY);
volatile uint64_t cpus_booted = __BIT(0);	/* cpu0 is always booted */
#endif

static void wdog_cpunode_poke(void *arg);

static int
cpunode_mainbus_print(void *aux, const char *pnp)
{
	struct cpunode_attach_args * const cnaa = aux;

	if (pnp)
		aprint_normal("%s", pnp);

	if (cnaa->cnaa_cpunum != CPUNODECF_CORE_DEFAULT)
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
	struct cpunode_softc * const sc = device_private(self);
	const uint64_t fuse = octeon_xkphys_read_8(CIU_FUSE);
	int cpunum = 0;

	sc->sc_dev = self;

	aprint_naive(": %u core%s\n", popcount64(fuse), fuse == 1 ? "" : "s");
	aprint_normal(": %u core%s", popcount64(fuse), fuse == 1 ? "" : "s");

	const uint64_t cvmctl = mips_cp0_cvmctl_read();
	aprint_normal(", %scrypto", (cvmctl & CP0_CVMCTL_NOCRYPTO) ? "no " : "");
	aprint_normal((cvmctl & CP0_CVMCTL_KASUMI) ? "+kasumi" : "");
	aprint_normal(", %s64bit-mul", (cvmctl & CP0_CVMCTL_NOMUL) ? "no " : "");
	if (cvmctl & CP0_CVMCTL_REPUN)
		aprint_normal(", unaligned-access ok");
#ifdef MULTIPROCESSOR
	aprint_normal(", booted %#" PRIx64, cpus_booted);
#endif
	aprint_normal("\n");

	for (uint64_t f = fuse; f != 0; f >>= 1, cpunum++) {
		struct cpunode_attach_args cnaa = {
			.cnaa_name = "cpu",
			.cnaa_cpunum = cpunum,
		};
		config_found(self, &cnaa, cpunode_mainbus_print, CFARGS_NONE);
	}
#if NWDOG > 0
	struct cpunode_attach_args cnaa = {
		.cnaa_name = "wdog",
		.cnaa_cpunum = CPUNODECF_CORE_DEFAULT,
	};
	config_found(self, &cnaa, cpunode_mainbus_print, CFARGS_NONE);
#endif
}

int
cpu_cpunode_match(device_t parent, cfdata_t cf, void *aux)
{
	struct cpunode_attach_args * const cnaa = aux;
	const int cpunum = cf->cf_loc[CPUNODECF_CORE];

	return strcmp(cnaa->cnaa_name, cf->cf_name) == 0
	    && (cpunum == CPUNODECF_CORE_DEFAULT || cpunum == cnaa->cnaa_cpunum);
}

#if defined(MULTIPROCESSOR)
static bool
octeon_fixup_cpu_info_references(int32_t load_addr, uint32_t new_insns[2],
    void *arg)
{
	struct cpu_info * const ci = arg;

	atomic_or_ulong(&curcpu()->ci_flags, CPUF_PRESENT);

	KASSERT(MIPS_KSEG0_P(load_addr));
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
	    __func__, load_addr, new_insns[0],
	    (new_insns[0] >> 16) & 31,
	    (int16_t)new_insns[0]);
	printf("%s: %08x: insn#2 %08x: %c%c r%u, %d(r%u)\n",
	    __func__, load_addr, new_insns[1],
	    INSN_LOAD_P(new_insns[1]) ? 'l' : 's',
	    INSN_LW_P(new_insns[1]) ? 'w' : 'd',
	    (new_insns[1] >> 16) & 31,
	    (int16_t)new_insns[1],
	    (new_insns[1] >> 21) & 31);
#endif
	return true;
}

static void
octeon_cpu_init(struct cpu_info *ci)
{
	extern const mips_locore_jumpvec_t mips64r2_locore_vec;
	bool ok __diagused;

	mips3_cp0_pg_mask_write(MIPS3_PG_SIZE_TO_MASK(PAGE_SIZE));
	mips3_cp0_wired_write(0);
	(*mips64r2_locore_vec.ljv_tlb_invalidate_all)();
	mips3_cp0_wired_write(pmap_tlb0_info.ti_wired);

	// First thing is setup the exception vectors for this cpu.
	mips64r2_vector_init(&mips_splsw);

	// Next rewrite those exceptions to use this cpu's cpu_info.
	ok = mips_fixup_exceptions(octeon_fixup_cpu_info_references, ci);
	KASSERT(ok);

	(void) splhigh();		// make sure interrupts are masked

	KASSERT((mipsNN_cp0_ebase_read() & MIPS_EBASE_CPUNUM) == ci->ci_cpuid);
	KASSERT(curcpu() == ci);
	KASSERT(ci->ci_cpl == IPL_HIGH);
	KASSERT((mips_cp0_status_read() & MIPS_INT_MASK) == 0);
}

static void
octeon_cpu_run(struct cpu_info *ci)
{

	octeon_intr_init(ci);

	mips3_initclocks();
	KASSERTMSG(ci->ci_cpl == IPL_NONE, "cpl %d", ci->ci_cpl);
	KASSERT(mips_cp0_status_read() & MIPS_SR_INT_IE);

	aprint_normal("%s: ", device_xname(ci->ci_dev));
	cpu_identify(ci->ci_dev);
}
#endif /* MULTIPROCESSOR */

static void
cpu_cpunode_attach_common(device_t self, struct cpu_info *ci)
{
	struct cpu_softc * const cpu __diagused = ci->ci_softc;

	ci->ci_dev = self;
	device_set_private(self, ci);

	KASSERTMSG(cpu != NULL, "ci %p index %d", ci, cpu_index(ci));

#if NWDOG > 0 || defined(DDB)
	/* XXXXXX __mips_n32 and MIPS_PHYS_TO_XKPHYS_CACHED needed here?????? */
	void **nmi_vector = (void *)MIPS_PHYS_TO_KSEG0(0x800 + 32*ci->ci_cpuid);
	*nmi_vector = octeon_reset_vector;

	struct vm_page * const pg = PMAP_ALLOC_POOLPAGE(UVM_PGA_ZERO);
	KASSERT(pg != NULL);
	const vaddr_t kva = PMAP_MAP_POOLPAGE(VM_PAGE_TO_PHYS(pg));
	KASSERT(kva != 0);
	ci->ci_nmi_stack = (void *)(kva + PAGE_SIZE - sizeof(struct kernframe));
#endif

#if NWDOG > 0
	cpu->cpu_wdog_sih = softint_establish(SOFTINT_CLOCK|SOFTINT_MPSAFE,
	    wdog_cpunode_poke, cpu);
	KASSERT(cpu->cpu_wdog_sih != NULL);
#endif

	aprint_normal(": %lu.%02luMHz\n",
	    (ci->ci_cpu_freq + 5000) / 1000000,
	    ((ci->ci_cpu_freq + 5000) % 1000000) / 10000);
	aprint_debug_dev(self, "hz cycles = %lu, delay divisor = %lu\n",
	    ci->ci_cycles_per_hz, ci->ci_divisor_delay);

	if (CPU_IS_PRIMARY(ci)) {
		aprint_normal("%s: ", device_xname(self));
		cpu_identify(self);
	}
	cpu_attach_common(self, ci);
#ifdef MULTIPROCESSOR
	KASSERT(cpuid_infos[ci->ci_cpuid] == ci);
#endif
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
	if ((boothowto & RB_MD1) != 0) {
		aprint_naive("\n");
		aprint_normal(": multiprocessor boot disabled\n");
		return;
	}

	if (!(cpus_booted & __BIT(cpunum))) {
		aprint_naive(" disabled\n");
		aprint_normal(" disabled (unresponsive)\n");
		return;
	}
	struct cpu_info * const ci = cpu_info_alloc(NULL, cpunum, 0, cpunum, 0);

	ci->ci_softc = &octeon_cpu_softc[cpunum];
	ci->ci_softc->cpu_ci = ci;

	cpu_cpunode_attach_common(self, ci);

	KASSERT(ci->ci_data.cpu_idlelwp != NULL);
	for (int i = 0; i < 100 && !kcpuset_isset(cpus_hatched, cpunum); i++) {
		delay(10000);
	}
	if (!kcpuset_isset(cpus_hatched, cpunum)) {
#ifdef DDB
		aprint_verbose_dev(self, "hatch failed ci=%p flags=%#lx\n", ci, ci->ci_flags);
		cpu_Debugger();
#endif
		panic("%s failed to hatch: ci=%p flags=%#lx",
		    cpu_name(ci), ci, ci->ci_flags);
	}
#else
	aprint_naive(": disabled\n");
	aprint_normal(": disabled (uniprocessor kernel)\n");
#endif
}

#if NWDOG > 0
struct wdog_softc {
	struct sysmon_wdog sc_smw;
	device_t sc_dev;
	u_int sc_wdog_period;
	bool sc_wdog_armed;
};

#ifndef OCTEON_WDOG_PERIOD_DEFAULT
#define OCTEON_WDOG_PERIOD_DEFAULT	4
#endif

static int wdog_cpunode_match(device_t, cfdata_t, void *);
static void wdog_cpunode_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(wdog_cpunode, sizeof(struct wdog_softc),
    wdog_cpunode_match, wdog_cpunode_attach, NULL, NULL);

static int
wdog_cpunode_setmode(struct sysmon_wdog *smw)
{
	struct wdog_softc * const sc = smw->smw_cookie;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		if (sc->sc_wdog_armed) {
			CPU_INFO_ITERATOR cii;
			struct cpu_info *ci;
			for (CPU_INFO_FOREACH(cii, ci)) {
				struct cpu_softc * const cpu = ci->ci_softc;
				uint64_t wdog = mips3_ld(cpu->cpu_wdog);
				wdog &= ~CIU_WDOGX_MODE;
				mips3_sd(cpu->cpu_pp_poke, wdog);
				aprint_verbose_dev(sc->sc_dev,
				    "%s: disable wdog=%#"PRIx64"\n",
				    cpu_name(ci), wdog);
				mips3_sd(cpu->cpu_wdog, wdog);
				mips3_sd(cpu->cpu_pp_poke, wdog);
			}
			sc->sc_wdog_armed = false;
		}
	} else if (!sc->sc_wdog_armed) {
		kpreempt_disable();
		struct cpu_info *ci = curcpu();
		if (smw->smw_period == WDOG_PERIOD_DEFAULT) {
			smw->smw_period = OCTEON_WDOG_PERIOD_DEFAULT;
		}
		uint64_t wdog_len = smw->smw_period * ci->ci_cpu_freq;
		//
		// This wdog is a 24-bit counter that decrements every 256
		// cycles.  This is then a 32-bit counter so as long wdog_len
		// doesn't overflow a 32-bit value, we are fine.  We write the
		// 16-bits of the 32-bit period.
		if ((wdog_len >> 32) != 0) {
			kpreempt_enable();
			return EINVAL;
		}
		sc->sc_wdog_period = smw->smw_period;
		CPU_INFO_ITERATOR cii;
		for (CPU_INFO_FOREACH(cii, ci)) {
			struct cpu_softc * const cpu = ci->ci_softc;
			uint64_t wdog = mips3_ld(cpu->cpu_wdog);
			wdog &= ~(CIU_WDOGX_MODE|CIU_WDOGX_LEN);
			wdog |= __SHIFTIN(3, CIU_WDOGX_MODE);
			wdog |= __SHIFTIN(wdog_len >> 16, CIU_WDOGX_LEN);
			aprint_verbose_dev(sc->sc_dev,
			    "%s: enable wdog=%#"PRIx64" (%#"PRIx64")\n",
			    cpu_name(ci), wdog, wdog_len);
			mips3_sd(cpu->cpu_wdog, wdog);
		}
		sc->sc_wdog_armed = true;
		kpreempt_enable();
	}
	return 0;
}

static void
wdog_cpunode_poke(void *arg)
{
	struct cpu_softc *cpu = arg;

	mips3_sd(cpu->cpu_pp_poke, 0);
}

static int
wdog_cpunode_tickle(struct sysmon_wdog *smw)
{

	wdog_cpunode_poke(curcpu()->ci_softc);
#ifdef MULTIPROCESSOR
	// We need to send IPIs to the other CPUs to poke their wdog.
	cpu_send_ipi(NULL, IPI_WDOG);
#endif
	return 0;
}

int
wdog_cpunode_match(device_t parent, cfdata_t cf, void *aux)
{
	struct cpunode_softc * const sc = device_private(parent);
	struct cpunode_attach_args * const cnaa = aux;
	const int cpunum = cf->cf_loc[CPUNODECF_CORE];

	return sc->sc_wdog_dev == NULL
	    && strcmp(cnaa->cnaa_name, cf->cf_name) == 0
	    && cpunum == CPUNODECF_CORE_DEFAULT;
}

void
wdog_cpunode_attach(device_t parent, device_t self, void *aux)
{
	struct cpunode_softc * const psc = device_private(parent);
	struct wdog_softc * const sc = device_private(self);
	cfdata_t const cf = device_cfdata(self);

	psc->sc_wdog_dev = self;

	sc->sc_dev = self;
	sc->sc_smw.smw_name = device_xname(self);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = wdog_cpunode_setmode;
	sc->sc_smw.smw_tickle = wdog_cpunode_tickle;
	sc->sc_smw.smw_period = OCTEON_WDOG_PERIOD_DEFAULT;
	sc->sc_wdog_period = sc->sc_smw.smw_period;

	/*
	 * We need one softint per cpu.  It's to tickle the softints on
	 * other CPUs.
	 */
#if 0 /* XXX unused? */
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	for (CPU_INFO_FOREACH(cii, ci)) {
	}
#endif

        aprint_normal(": default period is %u second%s\n",
            sc->sc_wdog_period, sc->sc_wdog_period == 1 ? "" : "s");

	if (sysmon_wdog_register(&sc->sc_smw) != 0) {
		aprint_error_dev(self, "unable to register with sysmon\n");
		return;
	}

	if (cf->cf_flags & 1) {
		int error = sysmon_wdog_setmode(&sc->sc_smw, WDOG_MODE_KTICKLE,
		    sc->sc_wdog_period);
		if (error)
			aprint_error_dev(self,
			    "failed to start kernel tickler: %d\n", error);
	}
}
#endif /* NWDOG > 0 */
