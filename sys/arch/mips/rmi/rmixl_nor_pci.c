/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: rmixl_nor_pci.c,v 1.1.2.2 2011/12/28 05:36:11 matt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/bus.h>

#include "locators.h"

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/nor/nor.h>
#include <dev/nor/cfi.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>

#include <mips/rmi/rmixl_iobusvar.h>

#include "locators.h"

static	int xlnor_pci_match(device_t, cfdata_t, void *);
static	void xlnor_pci_attach(device_t, device_t, void *);
static	int xlnor_print(void *, const char *);
static	int xlnor_intr(void *);

struct xlnor_cs {
	struct mips_bus_space cs_bst;
	struct rmixl_region cs_region;
	uint32_t cs_devparm;
	uint32_t cs_devtime0;
	uint32_t cs_devtime1;
	u_int cs_number;
};
	

struct xlnor_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	size_t sc_ndevices;

	kmutex_t sc_buslock;

	struct xlnor_cs sc_cs[RMIXLP_NOR_NCS];
};

CFATTACH_DECL_NEW(xlnor_pci, sizeof(struct xlnor_softc),
    xlnor_pci_match, xlnor_pci_attach, NULL, NULL);

static int
xlnor_pci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct pci_attach_args * const pa = aux;

	if (pa->pa_id == PCI_ID_CODE(PCI_VENDOR_NETLOGIC, PCI_PRODUCT_NETLOGIC_XLP_NOR))
		return 1;

        return 0;
}

static void
xlnor_pci_attach(device_t parent, device_t self, void *aux)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	struct pci_attach_args * const pa = aux;
	struct xlnor_softc * const sc = device_private(self);
	// struct norbus_attach_args iba;

	sc->sc_dev = self;
	sc->sc_bst = &rcp->rc_pci_ecfg_eb_memt;

	/*
	 * Why isn't this accessible via a BAR?
	 */
	if (bus_space_subregion(sc->sc_bst, rcp->rc_pci_ecfg_eb_memh,
		    pa->pa_tag, 0, &sc->sc_bsh)) {
		aprint_error(": can't map registers\n");
		return;
	}

	mutex_init(&sc->sc_buslock, IPL_NONE, MUTEX_DEFAULT);

	/*
	 * Let's see what devices we have attached.
	 */
	for (size_t i = 0; i < RMIXLP_NOR_NCS; i++) {
		struct rmixl_region * const rp = &rcp->rc_norflash[i];
		if (rp->r_pbase == (bus_addr_t)-1 || rp->r_size == 0)
			continue;

		struct xlnor_cs * const cs = &sc->sc_cs[sc->sc_ndevices++];
		cs->cs_region = *rp;
		cs->cs_devparm = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
		    RMIXLP_NOR_CS_DEVPARMn(i));
		cs->cs_devtime0 = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
		    RMIXLP_NOR_CS_DEVTIME0n(i));
		cs->cs_devtime1 = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
		    RMIXLP_NOR_CS_DEVTIME1n(i));
		cs->cs_number = i;

		/*
		 * Each chip gets it's own bus space.
		 */
		if (cs->cs_devparm & RMIXLP_NOR_CS_DEVPARM_LE) {
			rmixl_flash_el_bus_mem_init(&cs->cs_bst,
			    &cs->cs_region);
		} else {
			rmixl_flash_eb_bus_mem_init(&cs->cs_bst,
			    &cs->cs_region);
		}
	}

	aprint_normal(": XLP NOR Controller (%zu device%s)\n",
	    sc->sc_ndevices, (sc->sc_ndevices == 1 ? "" : "s"));

	pci_intr_handle_t pcih;

	pci_intr_map(pa, &pcih);

	if (pci_intr_establish(pa->pa_pc, pcih, IPL_VM, xlnor_intr, sc) == NULL) {
		aprint_error_dev(self, "failed to establish interrupt\n");
	} else {
		const char * const intrstr = pci_intr_string(pa->pa_pc, pcih);
		aprint_normal_dev(self, "interrupting at %s\n", intrstr);
	}


	for (size_t i = 0; i < sc->sc_ndevices; i++) {
		struct xlnor_cs * const cs = &sc->sc_cs[i];
		KASSERT(cs->cs_region.r_size >= 1024);
		size_t size = cs->cs_region.r_size >> 10;
		const char units[] = "KMGTPE";
		u_int j = 0;
		for (; size >= 1024; size >>= 10) {
			j++;
		}
		KASSERT(j < sizeof(units));
		aprint_normal_dev(sc->sc_dev, "cs %u: %zu%cB "
		    "%u-bit %s endian with %smuxed lines\n",
		    cs->cs_number, size, units[j],
		    8 << __SHIFTOUT(cs->cs_devparm, RMIXLP_NOR_CS_DEVPARM_DW),
		    cs->cs_devparm & RMIXLP_NOR_CS_DEVPARM_LE
			? "little" : "big",
		    cs->cs_devparm & RMIXLP_NOR_CS_DEVPARM_MUX
			? "" : "non-");

		aprint_debug_dev(sc->sc_dev, 
		    "cs %u: devparm=%#x, devtime0=%#x, devtime1=%#x\n",
		    cs->cs_number, cs->cs_devparm,
		    cs->cs_devtime0, cs->cs_devtime1);

		struct rmixl_iobus_attach_args iaa = {
			.ia_obio_bst = NULL,
			.ia_obio_bsh = 0,
			.ia_iobus_bst = &cs->cs_bst,
			.ia_iobus_addr = 0,
			.ia_iobus_size = cs->cs_region.r_size,
			.ia_dev_parm = cs->cs_devparm,
			.ia_cs = cs->cs_number,
		};

		config_found(self, &iaa, xlnor_print);
	}
}

static int
xlnor_print(void *aux, const char *pnp)
{
	struct rmixl_iobus_attach_args *ia = aux;

	if (pnp != NULL)
		aprint_normal("%s:", pnp);
	aprint_normal(" cs %d", ia->ia_cs);

	return UNCONF;
}

static int
xlnor_intr(void *v)
{

	panic("%s(%p)", __func__, v);
	return 0;
}
