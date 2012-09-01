/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
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

#define PCIE_PRIVATE

#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: bcm53xx_pax.c,v 1.1 2012/09/01 00:04:44 matt Exp $");

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <arm/broadcom/bcm53xx_reg.h>
#include <arm/broadcom/bcm53xx_var.h>

static int bcmpax_ccb_match(device_t, cfdata_t, void *);
static void bcmpax_ccb_attach(device_t, device_t, void *);

struct bcmpax_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_dma_tag_t sc_dmat;
};

static inline uint32_t
bcmpax_read_4(struct bcmpax_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, o);
}

static inline void
bcmpax_write_4(struct bcmpax_softc *sc, bus_size_t o, uint32_t v)
{
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, o, v);
}

CFATTACH_DECL_NEW(bcmpax_ccb, sizeof(struct bcmpax_softc),
	bcmpax_ccb_match, bcmpax_ccb_attach, NULL, NULL);

static int
bcmpax_ccb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct bcmccb_attach_args * const ccbaa = aux;
	const struct bcm_locators * const loc = &ccbaa->ccbaa_loc;

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

#ifdef DIAGNOSTIC
	const int port = cf->cf_loc[BCMCCBCF_PORT];
#endif
	KASSERT(port == BCMCCBCF_PORT_DEFAULT || port == loc->loc_port);

	return 1;
}

static void
bcmpax_ccb_attach(device_t parent, device_t self, void *aux)
{
	struct bcmpax_softc * const sc = device_private(self);
	struct bcmccb_attach_args * const ccbaa = aux;
	const struct bcm_locators * const loc = &ccbaa->ccbaa_loc;

	sc->sc_dev = self;

	sc->sc_bst = ccbaa->ccbaa_ccb_bst;
	sc->sc_dmat = ccbaa->ccbaa_dmat;
	bus_space_subregion(sc->sc_bst, ccbaa->ccbaa_ccb_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	uint32_t v = bcmpax_read_4(sc, PCIE_STRAP_STATUS);
	const bool enabled = (v & STRAP_PCIE_IF_ENABLE) != 0;
	const bool is_v2_p = (v & STRAP_PCIE_USER_FOR_CE_GEN1) == 0;
	const bool is_x2_p = (v & STRAP_PCIE_USER_FOR_CE_1LANE) == 0;
	const bool is_rc_p = (v & STRAP_PCIE_USER_RC_MODE) != 0;

	aprint_naive("\n");
	aprint_normal(": PCI Express V%u %u-lane %s Controller%s\n",
	    is_v2_p ? 2 : 1,
	    is_x2_p ? 2 : 1,
	    is_rc_p ? "RC" : "EP",
	    enabled ? "" : "(disabled)");
	if (!enabled || !is_rc_p)
		return;
}
#if 0
static void
bcmpax_decompose_tag(void *v, pcitag_t tag, int *busp, int *devp, int *funcp)
{
	if (busp)
		*busp = __SHIFTOUT(tag, CFG_ADDR_BUS);
	if (devp)
		*devp = __SHIFTOUT(tag, CFG_ADDR_DEV);
	if (funcp)
		*funcp = __SHIFTOUT(tag, CFG_ADDR_FUNC);
}       

static pcitag_t
bcmpax_make_tag(void *v, int bus, int dev, int func)
{
	return __SHIFTIN(bus, CFG_ADDR_BUS)
	   | __SHIFTIN(dev, CFG_ADDR_DEV)
	   | __SHIFTIN(func, CFG_ADDR_FUNC)
	   | (bus == 0 ? CFG_ADDR_TYPE0 : CFG_ADDR_TYPE1);
}

static inline void
bcmpax_conf_addr_write(struct bcmpax_softc *sc, pcitag_t tag)
{
	if ((tag & (CFG_ADDR_BUS|CFG_ADDR_DEV)) == 0) {
		uint32_t reg = __SHIFTOUT(tag, CFG_ADDR_REG);
		uint32_t func = __SHIFTOUT(tag, CFG_ADDR_FUNC);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, PCIE_CFG_IND_ADDR,
		    __SHIFTIN(func, CFG_IND_ADDR_FUNC)
		    | __SHIFTIN(reg, CFG_IND_ADDR_REG));
	} else {
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, PCIE_CFG_ADDR, tag);
	}
	__asm __volatile("dsb");
}

#endif
