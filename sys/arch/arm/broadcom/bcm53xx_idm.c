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

#include "opt_broadcom.h"
#include "locators.h"

#define CRU_PRIVATE
#define IDM_PRIVATE

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: bcm53xx_idm.c,v 1.3 2012/12/12 00:01:28 matt Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <arm/mainbus/mainbus.h>

#include <arm/broadcom/bcm53xx_reg.h>
#include <arm/broadcom/bcm53xx_var.h>

struct idm_info {
	bus_size_t idm_offset;
	const char *idm_name;
	int idm_port;
	bool (*idm_unreset)(bus_space_tag_t, bus_space_handle_t,
	    const struct idm_info *);
};

static bool
bcmeth_unreset(bus_space_tag_t bst, bus_space_handle_t bsh,
    const struct idm_info *idm)
{ 
	/*
	 * To enable any GMAC, we must enable all off them.
	 */
	static const bus_size_t regoff[] = {
		IDM_BASE + IDM_AMAC0_BASE,
		IDM_BASE + IDM_AMAC1_BASE,
		IDM_BASE + IDM_AMAC2_BASE,
		IDM_BASE + IDM_AMAC3_BASE,
	};
	static bool bcmeth_init_done;
	if (!bcmeth_init_done) {
		for (size_t idx = 0; idx < __arraycount(regoff); idx++) {
			const bus_size_t off = regoff[idx];
			bus_space_write_4(bst, bsh, off + IDM_RESET_CONTROL, 0);
			uint32_t v = bus_space_read_4(bst, bsh,
			    off + IDM_IO_CONTROL_DIRECT);
			/*
			 * Clear read-allocate and write-allocate bits from
			 * ACP cache access so we don't pollute the caches with
			 * DMA traffic.
			 */
			v &= ~IO_CONTROL_DIRECT_ARCACHE;
			v &= ~IO_CONTROL_DIRECT_AWCACHE;
#if 0
			v |= __SHIFTIN(AXCACHE_WA, IO_CONTROL_DIRECT_ARCACHE);
			v |= __SHIFTIN(AXCACHE_RA, IO_CONTROL_DIRECT_AWCACHE);
#endif
			v |= __SHIFTIN(AXCACHE_C|AXCACHE_B, IO_CONTROL_DIRECT_ARCACHE);
			v |= __SHIFTIN(AXCACHE_C|AXCACHE_B, IO_CONTROL_DIRECT_AWCACHE);
			/*
			 * These are the default but make sure they are
			 * properly set.
			 */
			v |= __SHIFTIN(0x1F, IO_CONTROL_DIRECT_ARUSER);
			v |= __SHIFTIN(0x1F, IO_CONTROL_DIRECT_AWUSER);
			v |= IO_CONTROL_DIRECT_CLK_250_SEL;
			v |= IO_CONTROL_DIRECT_DIRECT_GMII_MODE;
			v |= IO_CONTROL_DIRECT_SOURCE_SYNC_MODE_EN;
			v |= IO_CONTROL_DIRECT_CLK_GATING_EN;

			bus_space_write_4(bst, bsh, off + IDM_IO_CONTROL_DIRECT,
			    v);
		}
		bcmeth_init_done = true;
	}
	return true;
}

static bool
bcmccb_idm_unreset(bus_space_tag_t bst, bus_space_handle_t bsh,
    const struct idm_info *idm)
{
	if (idm->idm_offset == 0)
		return true;

	/*
	 * If the device might be in reset, let's try to take it out of it.
	 */
	bus_size_t o = IDM_BASE + idm->idm_offset + IDM_RESET_CONTROL;
	uint32_t v = bus_space_read_4(bst, bsh, o);
	if (v & 1) {
		v &= ~1;
		bus_space_write_4(bst, bsh, o, v);
	}
	return true;
}

static bool
bcmpax2_idm_unreset(bus_space_tag_t bst, bus_space_handle_t bsh,
    const struct idm_info *idm)
{
	uint32_t v = bus_space_read_4(bst, bsh, CRU_BASE + CRU_STRAPS_CONTROL);

	if (v & STRAP_USB3_SEL)
		return false;

	return bcmccb_idm_unreset(bst, bsh, idm);
}

static bool
bcmxhci_idm_unreset(bus_space_tag_t bst, bus_space_handle_t bsh,
    const struct idm_info *idm)
{
	uint32_t v = bus_space_read_4(bst, bsh, CRU_BASE + CRU_STRAPS_CONTROL);

	if ((v & STRAP_USB3_SEL) == 0)
		return false;

	return bcmccb_idm_unreset(bst, bsh, idm);
}

static const struct idm_info bcm53xx_idm_info[] = {
	{ 0, "bcmi2c", BCMCCBCF_PORT_DEFAULT, bcmccb_idm_unreset },
	{ 0, "bcmmdio", BCMCCBCF_PORT_DEFAULT, bcmccb_idm_unreset },
	{ 0, "bcmrng", BCMCCBCF_PORT_DEFAULT, bcmccb_idm_unreset },
	{ IDM_PCIE_M0_BASE, "bcmpax", 0, bcmccb_idm_unreset },
	{ IDM_PCIE_M1_BASE, "bcmpax", 1, bcmccb_idm_unreset },
	{ IDM_PCIE_M2_BASE, "bcmpax", 2, bcmpax2_idm_unreset },
	{ IDM_AMAC0_BASE, "bcmeth", 0, bcmeth_unreset },
	{ IDM_AMAC1_BASE, "bcmeth", 1, bcmeth_unreset },
	{ IDM_AMAC2_BASE, "bcmeth", 2, bcmeth_unreset },
	{ IDM_AMAC3_BASE, "bcmeth", 3, bcmeth_unreset },
	{ IDM_USB3_BASE, "xhci", BCMCCBCF_PORT_DEFAULT, bcmxhci_idm_unreset },
	{ IDM_SDIO_BASE, "sdhc", BCMCCBCF_PORT_DEFAULT, bcmccb_idm_unreset },
	{ IDM_USB2_BASE, "bcmusb", BCMCCBCF_PORT_DEFAULT, bcmccb_idm_unreset },
};

static const struct idm_info *
bcmccb_idm_lookup(const struct bcm_locators * const loc)
{
	const struct idm_info *idm = bcm53xx_idm_info;
	for (size_t i = 0; i < __arraycount(bcm53xx_idm_info); i++, idm++) {
		if (strcmp(idm->idm_name, loc->loc_name) == 0
		    && idm->idm_port == loc->loc_port) {
			return idm;
		}
	}
	return NULL;
}

bool
bcm53xx_idm_device_init(const struct bcm_locators *loc, bus_space_tag_t bst,
	bus_space_handle_t bsh)
{
	const struct idm_info * const idm = bcmccb_idm_lookup(loc);
	if (idm == NULL)
		return false;

	/*
	 * If the device might be in reset, let's try to take it out of it.
	 */
	return (*idm->idm_unreset)(bst, bsh, idm);
}
