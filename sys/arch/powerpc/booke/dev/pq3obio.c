/*	$NetBSD: pq3obio.c,v 1.1.4.1 2011/06/06 09:06:26 jruoho Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#define	LBC_PRIVATE

#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: pq3obio.c,v 1.1.4.1 2011/06/06 09:06:26 jruoho Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>
#include <sys/extent.h>

#include <powerpc/booke/cpuvar.h>
#include <powerpc/booke/e500var.h>
#include <powerpc/booke/e500reg.h>
#include <powerpc/booke/obiovar.h>

static int pq3obio_match(device_t, cfdata_t, void *);
static void pq3obio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(pq3obio, sizeof(struct pq3obio_softc),
   pq3obio_match, pq3obio_attach, NULL, NULL);

static int
pq3obio_match(device_t parent, cfdata_t cf, void *aux)
{

	if (!e500_cpunode_submatch(parent, cf, "lbc", aux))
		return 0;

	return 1;
}

static int
pq3obio_print(void *aux, const char *pnp)
{
	struct generic_attach_args * const ga = aux;

	if (pnp)
		aprint_normal(" at %s", pnp);

	if (ga->ga_cs != OBIOCF_CS_DEFAULT)
		aprint_normal(" cs %d", ga->ga_cs);

	if (ga->ga_addr != OBIOCF_ADDR_DEFAULT) {
		aprint_normal(" addr %#x", ga->ga_addr);
		if (ga->ga_size != OBIOCF_SIZE_DEFAULT)
			aprint_normal(" size %#x", ga->ga_size);
	}
	if (ga->ga_irq != OBIOCF_IRQ_DEFAULT)
		aprint_normal(" irq %d", ga->ga_irq);

	return UNCONF;
}

static int
pq3obio_search(device_t parent, cfdata_t cf, const int *slocs, void *aux)
{
	struct pq3obio_softc * const sc = device_private(parent);
	struct generic_attach_args ga;
	bool tryagain;

	do {
		const struct pq3lbc_softc *lbc;
		ga.ga_name = "obio";
		ga.ga_addr = cf->cf_loc[OBIOCF_ADDR];
		ga.ga_size = cf->cf_loc[OBIOCF_SIZE];
		ga.ga_irq = cf->cf_loc[OBIOCF_IRQ];
		ga.ga_cs = cf->cf_loc[OBIOCF_CS];
		ga.ga_bst = &sc->sc_obio_bst;

		if (ga.ga_cs != OBIOCF_CS_DEFAULT) {
			lbc = &sc->sc_lbcs[ga.ga_cs];
			if ((u_int) ga.ga_cs >= __arraycount(sc->sc_lbcs))
				return 0;
			if (ga.ga_addr != OBIOCF_ADDR_DEFAULT) {
				if (ga.ga_addr < lbc->lbc_base
				    || ga.ga_addr > lbc->lbc_limit)
					return 0;
			} else {
				ga.ga_addr = lbc->lbc_base;
			}
		} else {
			u_int cs;
			if (ga.ga_addr == OBIOCF_ADDR_DEFAULT)
				return 0;
			for (cs = 0, lbc = sc->sc_lbcs;
			     cs < __arraycount(sc->sc_lbcs);
			     cs++, lbc++) {
				if (ga.ga_addr >= lbc->lbc_base
				    && ga.ga_addr <= lbc->lbc_limit) {
					ga.ga_cs = cs;
					break;
				}
			}
			if (ga.ga_cs == OBIOCF_CS_DEFAULT)
				return 0;
		}

		if (ga.ga_size != OBIOCF_SIZE_DEFAULT) {
			if (ga.ga_size >= lbc->lbc_limit - ga.ga_addr)
				return 0;
		} else {
			ga.ga_size = lbc->lbc_limit + 1 - lbc->lbc_base;
		}

		tryagain = false;
		if (config_match(parent, cf, &ga) > 0) {
			int floc[OBIOCF_NLOCS] = {
			    [OBIOCF_ADDR] = ga.ga_addr,
			    [OBIOCF_SIZE] = ga.ga_size,
			    [OBIOCF_IRQ] = ga.ga_irq,
			    [OBIOCF_CS] = ga.ga_cs,
			};
			config_attach_loc(parent, cf, floc, &ga, pq3obio_print);
			tryagain = (cf->cf_fstate == FSTATE_STAR);
		}
	} while (tryagain);

	return (0);
}

static const char br_msel_strings[8][6] = {
    [__SHIFTOUT(BR_MSEL_GPCM,BR_MSEL)] = "GPCM",
    [__SHIFTOUT(BR_MSEL_FCM,BR_MSEL)] = "FCM",
    [__SHIFTOUT(BR_MSEL_SDRAM,BR_MSEL)] = "SDRAM",
    [__SHIFTOUT(BR_MSEL_UPMA,BR_MSEL)] = "UPMA",
    [__SHIFTOUT(BR_MSEL_UPMB,BR_MSEL)] = "UPMB",
    [__SHIFTOUT(BR_MSEL_UPMC,BR_MSEL)] = "UPMC",
};

static void
pq3obio_attach(device_t parent, device_t self, void *aux)
{
	struct cpunode_softc * const psc = device_private(parent);
	struct pq3obio_softc * const sc = device_private(self);
	struct cpunode_attach_args * const cna = aux;
	struct cpunode_locators * const cnl = &cna->cna_locs;
	struct powerpc_bus_space * const t = &sc->sc_obio_bst;
	u_int found = 0;
	int error;

	psc->sc_children |= cna->cna_childmask;
	sc->sc_dev = self;
	sc->sc_bst = cna->cna_memt;

	error = bus_space_map(sc->sc_bst, cnl->cnl_addr, cnl->cnl_size, 0,
		&sc->sc_bsh);
	if (error) {
		aprint_error("failed to map registers: %d\n", error);
		return;
	}

	for (u_int i = 0; i < 8; i++) {
		struct pq3lbc_softc * const lbc = &sc->sc_lbcs[i];
		uint32_t br = bus_space_read_4(sc->sc_bst, sc->sc_bsh, BRn(i));
		if (br & BR_V) {
			found++;
			lbc->lbc_br = br;
			lbc->lbc_or = bus_space_read_4(sc->sc_bst,
			    sc->sc_bsh, ORn(i));
			lbc->lbc_base = lbc->lbc_br & BR_BA & lbc->lbc_or;
			lbc->lbc_limit = lbc->lbc_base + ~(lbc->lbc_or & OR_AM);
		}
	}

	aprint_normal(": %u of 8 ports enabled\n", found);
	if (found == 0)
		return;

	t->pbs_base = 0xffffffff;
	t->pbs_limit = 0;
	t->pbs_flags = _BUS_SPACE_BIG_ENDIAN;

	u_int sorted[found];
	u_int nsorted = 0;

	for (u_int i = 0; i < 8; i++) {
		struct pq3lbc_softc * const lbc = &sc->sc_lbcs[i];
		if ((lbc->lbc_br & BR_V) == 0)
			continue;

		u_int j;
		for (j = 0; j < nsorted; j++) {
			if (lbc->lbc_base < sc->sc_lbcs[sorted[j]].lbc_base)
				break;
		}
		for (u_int k = ++nsorted; k > j; k--) {
			sorted[k] = sorted[k - 1];
		}
		sorted[j] = i;

		if (lbc->lbc_base < t->pbs_base)
			t->pbs_base = lbc->lbc_base;
		if (lbc->lbc_limit > t->pbs_limit)
			t->pbs_limit = lbc->lbc_limit;
#if 0
		aprint_normal_dev(sc->sc_dev,
		    "cs%u: br=%#x or=%#x", i, lbc->lbc_br, lbc->lbc_or);
#endif
		u_int n = ffs(~(lbc->lbc_or & OR_AM) + 1) - 1;
		aprint_normal_dev(sc->sc_dev,
		    "cs%u: %u%cB %u-bit %s region at %#x\n",
		    i, 
		    1 << (n % 10), " KMGTPE"[n / 10],
		    1 << (__SHIFTOUT(lbc->lbc_br, BR_PS) + 2),
		    br_msel_strings[__SHIFTOUT(lbc->lbc_br,BR_MSEL)],
		    lbc->lbc_base);
	}

	error = bus_space_init(t, device_xname(sc->sc_dev), NULL, 0);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "failed to initialize obio bus space: %d\n", error);
		return;
	}

	/*
	 * We've created a bus space which covers all the chip selects
	 * but there might be gaps inbetween them.  We need to make sure
	 * bus_space_map will fail for those.  To do that, we reserve
	 * the gaps between the chip-selects.
	 */
	for (u_int i = 1; i < found; i++) {
		struct pq3lbc_softc * const lbc_lo = &sc->sc_lbcs[sorted[i-1]];
		struct pq3lbc_softc * const lbc_hi = &sc->sc_lbcs[sorted[i]];
		if (lbc_lo->lbc_limit + 1 == lbc_hi->lbc_base)
			continue;
		error = extent_alloc_region(t->pbs_extent,
		    lbc_lo->lbc_limit + 1,
		    lbc_hi->lbc_base - (lbc_lo->lbc_limit + 1), 0);
		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "failed to reserve %#x..%#x: %d\n",
			    lbc_lo->lbc_limit + 1,
			    lbc_hi->lbc_base - 1,
			    error);
			return;
		}
	}

	/*
	 * Now we can do the device configuration to find what might
	 * be using obio.
	 */
	int locs[OBIOCF_NLOCS];
	locs[OBIOCF_ADDR] = OBIOCF_ADDR_DEFAULT;
	locs[OBIOCF_SIZE] = OBIOCF_SIZE_DEFAULT;
	locs[OBIOCF_CS] = OBIOCF_CS_DEFAULT;
	config_search_loc(pq3obio_search, self, "obio", locs, NULL);
}
