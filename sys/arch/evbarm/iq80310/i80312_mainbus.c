/*	$NetBSD: i80312_mainbus.c,v 1.5 2002/02/08 02:31:12 thorpej Exp $	*/

/*
 * Copyright (c) 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
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

/*
 * IQ80310 front-end for the i80312 Companion I/O chip.  We take care
 * of setting up the i80312 memory map, PCI interrupt routing, etc.,
 * which are all specific to the board the i80312 is wired up to.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <evbarm/iq80310/iq80310reg.h>
#include <evbarm/iq80310/iq80310var.h>

#include <arm/xscale/i80312reg.h>
#include <arm/xscale/i80312var.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

int	i80312_mainbus_match(struct device *, struct cfdata *, void *);
void	i80312_mainbus_attach(struct device *, struct device *, void *);

struct cfattach iopxs_mainbus_ca = {
	sizeof(struct i80312_softc), i80312_mainbus_match,
	    i80312_mainbus_attach,
};

/* There can be only one. */
int	i80312_mainbus_found;

int
i80312_mainbus_match(struct device *parent, struct cfdata *cf, void *aux)
{
#if 0
	struct mainbus_attach_args *ma = aux;
#endif

	if (i80312_mainbus_found)
		return (0);

#if 1
	/* XXX Shoot arch/arm/mainbus in the head. */
	return (1);
#else
	if (strcmp(cf->cf_driver->cd_name, ma->ma_name) == 0)
		return (1);

	return (0);
#endif
}

void
i80312_mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct i80312_softc *sc = (void *) self;
	paddr_t memstart;
	psize_t memsize;

	i80312_mainbus_found = 1;

	/*
	 * Fill in the space tag for the i80312's own devices,
	 * and hand-craft the space handle for it (the device
	 * was mapped during early bootstrap).
	 */
	i80312_bs_init(&i80312_bs_tag, sc);
	sc->sc_st = &i80312_bs_tag;
	sc->sc_sh = IQ80310_80312_VBASE;

	/*
	 * Slice off a subregion for the Memory Controller -- we need it
	 * here in order read the memory size.
	 */
	if (bus_space_subregion(sc->sc_st, sc->sc_sh, I80312_MEM_BASE,
	    I80312_MEM_SIZE, &sc->sc_mem_sh))
		panic("%s: unable to subregion MEM registers\n",
		    sc->sc_dev.dv_xname);

	/*
	 * We have mapped the the PCI I/O windows in the early
	 * bootstrap phase.
	 */
	sc->sc_piow_vaddr = IQ80310_PIOW_VBASE;
	sc->sc_siow_vaddr = IQ80310_SIOW_VBASE;

	/* Some boards are always considered "host". */
#if defined(IOP310_TEAMASA_NPWR)
	sc->sc_is_host = 1;
#else /* Default to stock IQ80310 */
	sc->sc_is_host = CPLD_READ(IQ80310_BACKPLANE_DET) & 1;

	/*
	 * Set the subsystem vendor/device IDs to "Cyclone" "PCI-700",
	 * which is the board-specific identification.
	 */
	bus_space_write_4(sc->sc_st, sc->sc_sh,
	    I80312_ATU_BASE + PCI_SUBSYS_ID_REG,
	    PCI_ID_CODE(PCI_VENDOR_CYCLONE, PCI_PRODUCT_CYCLONE_PCI_700));
#endif

	printf(": i80312 Companion I/O, acting as PCI %s\n",
	    sc->sc_is_host ? "host" : "slave");

	i80312_sdram_bounds(sc->sc_st, sc->sc_mem_sh, &memstart, &memsize);

	/*
	 * Set the Primary Inbound window xlate base to the start
	 * of RAM.  Set the size to 4K, for now.  Just for testing
	 * in a host.  This obviously has to be customized for each
	 * IQ310 application.
	 *
	 * Note the first 4K of the window is reserved for the
	 * messaging unit, so no RAM is going to be accessed here.
	 *
	 * ..unless we're a host -- in which case, make it work like
	 * the Secondary Inbound window (below).
	 */
	if (sc->sc_is_host) {
		sc->sc_pin_base = memstart;
		sc->sc_pin_xlate = memstart;
		sc->sc_pin_size = memsize;
	} else {
		sc->sc_pin_xlate = memstart;
		sc->sc_pin_size = 4096;
	}

	/*
	 * Map the Secondary Inbound window 1:1 with local RAM.
	 */
	sc->sc_sin_base = memstart;
	sc->sc_sin_xlate = memstart;
	sc->sc_sin_size = memsize;

	/*
	 * XXX Don't use the Primary Outbound windows, for now.
	 */
	sc->sc_pmemout_size = 0;
	sc->sc_pioout_size = 0;

	/*
	 * Set the Secondary Outbound Memory window to map 1:1
	 * PCI:Local.
	 */
	sc->sc_smemout_base = I80312_PCI_XLATE_SMW_BASE;
	sc->sc_smemout_size = I80312_PCI_XLATE_MSIZE;

	/*
	 * Set the Secondary Outbound I/O window to map
	 * to PCI address 0 for all 64K of the I/O space.
	 */
	sc->sc_sioout_base = 0;
	sc->sc_sioout_size = I80312_PCI_XLATE_IOSIZE;

	/*
	 * XXX For now, suppress all secondary IDSELs (thus making all
	 * devices from S_AD[11]..S_AD[25] private).
	 */
	sc->sc_sisr = 0x3ff;

	/*
	 * XXX For now, make the entire Secondary Outbound address
	 * spaces private.
	 */
	sc->sc_privio_base = sc->sc_sioout_base;
	sc->sc_privio_size = sc->sc_sioout_size;
	sc->sc_privmem_base = sc->sc_smemout_base;
	sc->sc_privmem_size = sc->sc_smemout_size;

	/*
	 * Initialize the interrupt part of our PCI chipset tag.
	 */
	iq80310_pci_init(&sc->sc_pci_chipset, sc);

	i80312_attach(sc);
}
