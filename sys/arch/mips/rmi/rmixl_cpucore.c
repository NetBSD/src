/*	$NetBSD: rmixl_cpucore.c,v 1.5 2011/04/29 22:00:03 matt Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rmixl_cpucore.c,v 1.5 2011/04/29 22:00:03 matt Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_cpunodevar.h>
#include <mips/rmi/rmixl_cpucorevar.h>
#include <mips/rmi/rmixl_fmnvar.h>

static int	cpucore_rmixl_match(device_t, cfdata_t, void *);
static void	cpucore_rmixl_attach(device_t, device_t, void *);
static int	cpucore_rmixl_print(void *, const char *);

CFATTACH_DECL_NEW(cpucore_rmixl, sizeof(struct cpucore_softc),
	cpucore_rmixl_match, cpucore_rmixl_attach, NULL, NULL);

static int
cpucore_rmixl_match(device_t parent, cfdata_t cf, void *aux)
{
	struct cpunode_attach_args *na = aux;
	int core = cf->cf_loc[CPUNODECF_CORE];

	if (!cpu_rmixl(mips_options.mips_cpu))
		return 0;

	if (strncmp(na->na_name, cf->cf_name, strlen(cf->cf_name)) == 0
#ifndef MULTIPROCESSOR
	    && na->na_core == 0
#endif
	    && (core == CPUNODECF_CORE_DEFAULT || core == na->na_core))
		return 1;

	return 0;
}

static void
cpucore_rmixl_attach(device_t parent, device_t self, void *aux)
{
	struct cpucore_softc * const sc = device_private(self);
	struct cpunode_attach_args *na = aux;
	struct cpucore_attach_args ca;
	u_int nthreads;
	struct rmixl_config *rcp = &rmixl_configuration;

	sc->sc_dev = self;
	sc->sc_core = na->na_core;
	KASSERT(sc->sc_hatched == false);

#if 0
#ifdef MULTIPROCESSOR
	/*
	 * Create the TLB structure needed - one per core and core0 uses the
	 * default one for the system.
	 */
	if (sc->sc_core == 0) {
		sc->sc_tlbinfo = &pmap_tlb0_info;
	} else {
		const vaddr_t va = (vaddr_t)&sc->sc_tlbinfo0;
		paddr_t pa;

		if (! pmap_extract(pmap_kernel(), va, &pa))
			panic("%s: pmap_extract fail, va %#"PRIxVADDR, __func__, va);
#ifdef _LP64
		sc->sc_tlbinfo = (struct pmap_tlb_info *)
			MIPS_PHYS_TO_XKPHYS_CACHED(pa);
#else
		sc->sc_tlbinfo = (struct pmap_tlb_info *)
			MIPS_PHYS_TO_KSEG0(pa);
#endif
		pmap_tlb_info_init(sc->sc_tlbinfo);
	}
#endif
#endif

	aprint_normal("\n");
	aprint_normal_dev(self, "%lu.%02luMHz (hz cycles = %lu, "
	    "delay divisor = %lu)\n",
	    curcpu()->ci_cpu_freq / 1000000,
	    (curcpu()->ci_cpu_freq % 1000000) / 10000,
	    curcpu()->ci_cycles_per_hz, curcpu()->ci_divisor_delay);

	aprint_normal("%s: ", device_xname(self));
	cpu_identify(self);

	nthreads = MIPS_CIDFL_RMI_NTHREADS(mips_options.mips_cpu->cpu_cidflags);
	aprint_normal_dev(self, "%d %s on core\n", nthreads,
		nthreads == 1 ? "thread" : "threads");

	/*
	 * Attach CPU (RMI thread contexts) devices
	 * according to userapp_cpu_map bitmask.
	 */
	u_int thread_mask = (1 << nthreads) - 1;
	u_int core_shft = sc->sc_core * nthreads;
	u_int threads_enb =
		(u_int)(rcp->rc_psb_info.userapp_cpu_map >> core_shft) & thread_mask;
	u_int threads_dis = (~threads_enb) & thread_mask;

	sc->sc_threads_dis = threads_dis;
	if (threads_dis != 0) {
		aprint_normal_dev(self, "threads");
		u_int d = threads_dis;
		while (d != 0) {
			const u_int t = ffs(d) - 1;
			d ^= (1 << t);
			aprint_normal(" %d%s", t, (d==0) ? "" : ",");
		}
		aprint_normal(" offline (disabled by firmware)\n");
	}

	u_int threads_try_attach = threads_enb;
	while (threads_try_attach != 0) {
		const u_int t = ffs(threads_try_attach) - 1;
		const u_int bit = 1 << t;
		threads_try_attach ^= bit;
		ca.ca_name = "cpu";
		ca.ca_thread = t;
		ca.ca_core = sc->sc_core;
		if (config_found(self, &ca, cpucore_rmixl_print) == NULL) {
			/*
			 * thread did not attach, e.g. not configured
			 * arrange to have it disabled in THREADEN PCR
			 */
			threads_enb ^= bit;
			threads_dis |= bit;
		}
	}

	sc->sc_threads_enb = threads_enb;
	sc->sc_threads_dis = threads_dis;

	/*
	 * when attaching the core of the primary cpu,
	 * do the post-running initialization here
	 */
	if (sc->sc_core == RMIXL_CPU_CORE((curcpu()->ci_cpuid)))
		cpucore_rmixl_run(self);
}

static int
cpucore_rmixl_print(void *aux, const char *pnp)
{
	struct cpucore_attach_args *ca = aux;

	if (pnp != NULL)
		aprint_normal("%s:", pnp);
	aprint_normal(" thread %d", ca->ca_thread);

	return (UNCONF);
}

/*
 * cpucore_rmixl_run
 *	called from cpucore_rmixl_attach for primary core
 *	and called from cpu_rmixl_run for each hatched cpu
 *	the first call for each cpucore causes init of per-core features:
 *	- disable unused threads
 *	- set Fine-grained (Round Robin) thread scheduling mode
 */
void
cpucore_rmixl_run(device_t self)
{
	struct cpucore_softc * const sc = device_private(self);

	if (sc->sc_running == false) {
		sc->sc_running = true;
		rmixl_mtcr(RMIXL_PCR_THREADEN, sc->sc_threads_enb);
		rmixl_mtcr(RMIXL_PCR_SCHEDULING, 0);
	}
}

#ifdef MULTIPROCESSOR
/*
 * cpucore_rmixl_hatch
 *	called from cpu_rmixl_hatch for each cpu
 *	the first call for each cpucore causes init of per-core features
 */
void
cpucore_rmixl_hatch(device_t self)
{
	struct cpucore_softc * const sc = device_private(self);

	if (sc->sc_hatched == false) {
		/* PCRs for core#0 are set up in mach_init() */
		if (sc->sc_core != 0)
			rmixl_pcr_init_core();
		rmixl_fmn_init_core();
		sc->sc_hatched = true;
	}
}
#endif	/* MULTIPROCESSOR */
