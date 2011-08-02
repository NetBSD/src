/*	$NetBSD: pq3ddrc.c,v 1.1 2011/08/02 00:21:00 matt Exp $	*/
/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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
#define	DDRC_PRIVATE

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: pq3ddrc.c,v 1.1 2011/08/02 00:21:00 matt Exp $");

#include "ioconf.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/intr.h>

#include <powerpc/booke/cpuvar.h>
#include <powerpc/booke/e500var.h>
#include <powerpc/booke/e500reg.h>

struct pq3ddrc_softc {
	device_t sc_dev;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_memh;
	void *sc_ih;
	struct evcnt sc_ev_sbe;
};

static int pq3ddrc_match(device_t, cfdata_t, void *);
static void pq3ddrc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(pq3ddrc, sizeof(struct pq3ddrc_softc),
    pq3ddrc_match, pq3ddrc_attach, NULL, NULL);

static int
pq3ddrc_match(device_t parent, cfdata_t cf, void *aux)
{
	if (!e500_cpunode_submatch(parent, cf, cf->cf_name, aux))
		return 0;

	return 1;
}

static int
pq3ddrc_intr(void *arg)
{
	struct pq3ddrc_softc * const sc = arg;
	uint32_t v;

	v = bus_space_read_4(sc->sc_memt, sc->sc_memh, ERR_DETECT);
	bus_space_write_4(sc->sc_memt, sc->sc_memh, ERR_DETECT, v);

	if (v & ERR_SBEE) {
		v = bus_space_read_4(sc->sc_memt, sc->sc_memh, ERR_SBE);
		sc->sc_ev_sbe.ev_count += __SHIFTIN(v, ERR_SBE_SBEC);
		v &= ~ERR_SBE_SBEC;
		bus_space_write_4(sc->sc_memt, sc->sc_memh, ERR_SBE, v);
	}

	return 1;
}

static void
pq3ddrc_attach(device_t parent, device_t self, void *aux)
{
	struct cpunode_softc * const psc = device_private(parent);
	struct pq3ddrc_softc * const sc = device_private(self);
	struct cpunode_attach_args * const cna = aux;
	struct cpunode_locators * const cnl = &cna->cna_locs;
	uint32_t v;

	psc->sc_children |= cna->cna_childmask;
	sc->sc_dev = self;
	sc->sc_memt = cna->cna_memt;

	int error = bus_space_map(cna->cna_memt, cnl->cnl_addr, cnl->cnl_size,
	    0, &sc->sc_memh);
	if (error) {
		aprint_error(": failed to map registers: %d\n", error);
		return;
	}

	v = bus_space_read_4(sc->sc_memt, sc->sc_memh, DDR_SDRAM_CFG);
	if ((v & SDRAM_CFG_ECC_EN) == 0) {
		aprint_normal(": ECC disabled\n");
		return;
	}

	evcnt_attach_dynamic(&sc->sc_ev_sbe, EVCNT_TYPE_MISC, NULL,
	    device_xname(self), "single-bit ecc errors");

	/*
	 * Clear errors.
	 */
	v = bus_space_read_4(sc->sc_memt, sc->sc_memh, ERR_DETECT);
	bus_space_write_4(sc->sc_memt, sc->sc_memh, ERR_DETECT, v);

	/*
	 * Make sure ECC errors are not disabled.
	 */
	v = bus_space_read_4(sc->sc_memt, sc->sc_memh, ERR_DISABLE);
	v &= ~(ERR_MBEE|ERR_SBEE);
	bus_space_write_4(sc->sc_memt, sc->sc_memh, ERR_DISABLE, v);

	/*
	 * Make sure ECC errors generate interrupts
	 */
	v = bus_space_read_4(sc->sc_memt, sc->sc_memh, ERR_INT_EN);
	v |= ERR_MBEE|ERR_SBEE;
	bus_space_write_4(sc->sc_memt, sc->sc_memh, ERR_INT_EN, v);

	sc->sc_ih = intr_establish(cnl->cnl_intrs[0], IPL_VM,
	    IST_ONCHIP, pq3ddrc_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     cnl->cnl_intrs[0]);
	} else {
		aprint_normal_dev(self, "interrupting on irq %d\n",
		     cnl->cnl_intrs[0]);
	}
}
