/*	$NetBSD: pq3duart.c,v 1.1.4.1 2011/06/06 09:06:26 jruoho Exp $	*/
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

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/tty.h>

#include "ioconf.h"

#include <sys/intr.h>
#include <sys/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <powerpc/booke/cpuvar.h>
#include <powerpc/booke/e500var.h>
#include <powerpc/booke/e500reg.h>

struct pq3duart_softc {
	device_t dsc_dev;
	struct com_softc *dsc_sc[2];
	bus_space_tag_t dsc_memt;
	bus_addr_t dsc_base;
	void *dsc_ih;
};

struct pq3duart_attach_args {
	const char *da_busname;
	bus_space_tag_t da_memt;
	bus_addr_t da_addr; 
	bus_addr_t da_size; 
	u_int da_port;
};

static int pq3duart_match(device_t, cfdata_t, void *);
static void pq3duart_attach(device_t, device_t, void *);
static int com_pq3duart_match(device_t, cfdata_t, void *);
static void com_pq3duart_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(pq3duart, sizeof(struct pq3duart_softc),
    pq3duart_match, pq3duart_attach, NULL, NULL);

CFATTACH_DECL_NEW(com_pq3duart, sizeof(struct com_softc),
    com_pq3duart_match, com_pq3duart_attach, NULL, NULL);

static int
pq3duart_match(device_t parent, cfdata_t cf, void *aux)
{

	if (!e500_cpunode_submatch(parent, cf, cf->cf_name, aux))
		return 0;

	return 1;
}

static int
com_pq3duart_match(device_t parent, cfdata_t cfdata, void *aux)
{
	struct pq3duart_softc * const dsc = device_private(parent);
	struct pq3duart_attach_args * const da = aux;
	struct com_regs regs;

	if ((da->da_port != 1 && da->da_port != 2)
	    || dsc->dsc_sc[da->da_port-1] != NULL)
		return 0;

	bus_space_tag_t memt = da->da_memt;
	bus_addr_t addr = da->da_addr;
	bus_addr_t size = da->da_size;
	bus_space_handle_t memh;

	if (com_is_console(memt, addr, &memh))
		return 1;

	if (bus_space_map(memt, addr, size, 0, &memh))
		return 0;

	COM_INIT_REGS(regs, memt, memh, addr);

	int rv = com_probe_subr(&regs);

	bus_space_unmap(memt, memh, size);

	return rv;
}

static int
pq3duart_intr(void *arg)
{
	struct pq3duart_softc * const dsc = arg;
	int rv = 0;

	if (dsc->dsc_sc[0] != NULL)
		rv += comintr(dsc->dsc_sc[0]);
	if (dsc->dsc_sc[1] != NULL)
		rv += comintr(dsc->dsc_sc[1]);

	return rv;
}

static int
pq3duart_print(void *aux, const char *pnp)
{
	struct pq3duart_attach_args * const da = aux;

	if (pnp != NULL)
		return QUIET;

	aprint_normal(" port %d", da->da_port);

	return UNCONF;
}

static void
pq3duart_attach(device_t parent, device_t self, void *aux)
{
	struct cpunode_softc * const psc = device_private(parent);
	struct pq3duart_softc * const dsc = device_private(self);
	struct cpunode_attach_args * const cna = aux;
	struct cpunode_locators * const cnl = &cna->cna_locs;
	struct pq3duart_attach_args da;
	u_int nports = cnl->cnl_size / DUART_SIZE;

	psc->sc_children |= cna->cna_childmask;
	dsc->dsc_dev = self;

	aprint_normal(": %u ports\n", nports);

	for (u_int port = 1; port <= min(2, nports); port++) {
		da.da_memt = cna->cna_memt;
		da.da_port = port;
		da.da_addr = cnl->cnl_addr + (port - 1) * DUART_SIZE;
		da.da_size = COM_NPORTS;
		(void)config_found_sm_loc(self, "duart", NULL,
		    &da, pq3duart_print, NULL);
	}

	if (dsc->dsc_sc[0] != NULL || dsc->dsc_sc[1] != NULL) {
		dsc->dsc_ih = intr_establish(cnl->cnl_intrs[0], IPL_SERIAL,
		    IST_ONCHIP, pq3duart_intr, dsc);
		if (dsc->dsc_ih == NULL)
			aprint_error_dev(self,
			    "failed to establish interrupt %d\n",
			     cnl->cnl_intrs[0]);
		else
			aprint_normal_dev(self,
			    "interrupting on irq %d\n",
			     cnl->cnl_intrs[0]);
	}
}

static void
com_pq3duart_attach(device_t parent, device_t self, void *aux)
{
	struct pq3duart_softc * const dsc = device_private(parent);
	struct com_softc * const sc = device_private(self);
	struct pq3duart_attach_args * const da = aux;

	dsc->dsc_sc[da->da_port-1] = sc;
	sc->sc_dev = self;
	sc->sc_frequency = (int) board_info_get_number("bus-frequency");

	bus_space_tag_t memt = da->da_memt;
	bus_addr_t addr = da->da_addr;
	bus_addr_t size = da->da_size;
	bus_space_handle_t memh;

	if (!com_is_console(memt, addr, &memh)) {
		int error = bus_space_map(memt, addr, size, 0, &memh);
		if (error) {
			aprint_error(": failed to map registers: %d\n", error);
			return;
		}
	}

	COM_INIT_REGS(sc->sc_regs, memt, memh, addr);
	sc->sc_regs.cr_nports = size;

	com_attach_subr(sc);

}
