/*	$NetBSD: i80321_mainbus.c,v 1.8 2003/02/06 03:17:49 briggs Exp $	*/

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
 * IQ80321 front-end for the i80321 I/O Processor.  We take care
 * of setting up the i80321 memory map, PCI interrupt routing, etc.,
 * which are all specific to the board the i80321 is wired up to.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <evbarm/iq80321/iq80321reg.h>
#include <evbarm/iq80321/iq80321var.h>

#include <arm/xscale/i80321reg.h>
#include <arm/xscale/i80321var.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

int	i80321_mainbus_match(struct device *, struct cfdata *, void *);
void	i80321_mainbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(iopxs_mainbus, sizeof(struct i80321_softc),
    i80321_mainbus_match, i80321_mainbus_attach, NULL, NULL);

/* There can be only one. */
int	i80321_mainbus_found;

int
i80321_mainbus_match(struct device *parent, struct cfdata *cf, void *aux)
{
#if 0
	struct mainbus_attach_args *ma = aux;
#endif

	if (i80321_mainbus_found)
		return (0);

#if 1
	/* XXX Shoot arch/arm/mainbus in the head. */
	return (1);
#else
	if (strcmp(cf->cf_name, ma->ma_name) == 0)
		return (1);

	return (0);
#endif
}

void
i80321_mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct i80321_softc *sc = (void *) self;
	paddr_t memstart;
	psize_t memsize;

	i80321_mainbus_found = 1;

	/*
	 * Fill in the space tag for the i80321's own devices,
	 * and hand-craft the space handle for it (the device
	 * was mapped during early bootstrap).
	 */
	i80321_bs_init(&i80321_bs_tag, sc);
	sc->sc_st = &i80321_bs_tag;
	sc->sc_sh = IQ80321_80321_VBASE;

	/*
	 * Slice off a subregion for the Memory Controller -- we need it
	 * here in order read the memory size.
	 */
	if (bus_space_subregion(sc->sc_st, sc->sc_sh, VERDE_MCU_BASE,
	    VERDE_MCU_SIZE, &sc->sc_mcu_sh))
		panic("%s: unable to subregion MCU registers",
		    sc->sc_dev.dv_xname);

	/*
	 * We have mapped the the PCI I/O windows in the early
	 * bootstrap phase.
	 */
	sc->sc_iow_vaddr = IQ80321_IOW_VBASE;

	/* Some boards are always considered "host". */
	sc->sc_is_host = 1;		/* XXX */

	printf(": i80321 I/O Processor, acting as PCI %s\n",
	    sc->sc_is_host ? "host" : "slave");

	i80321_sdram_bounds(sc->sc_st, sc->sc_mcu_sh, &memstart, &memsize);

	/*
	 * We set up the Inbound Windows as follows:
	 *
	 *	0	Access to i80321 PMMRs
	 *
	 *	1	Reserve space for private devices
	 *
	 *	2	Unused.
	 *
	 *	3	RAM access
	 *
	 * This chunk needs to be customized for each IOP321 application.
	 */
#if 0
	sc->sc_iwin[0].iwin_base_lo = VERDE_PMMR_BASE;
	sc->sc_iwin[0].iwin_base_hi = 0;
	sc->sc_iwin[0].iwin_xlate = VERDE_PMMR_BASE;
	sc->sc_iwin[0].iwin_size = VERDE_PMMR_SIZE;
#endif

	if (sc->sc_is_host) {
		/* Map PCI:Local 1:1. */
		sc->sc_iwin[1].iwin_base_lo = VERDE_OUT_XLATE_MEM_WIN0_BASE |
		    PCI_MAPREG_MEM_PREFETCHABLE_MASK |
		    PCI_MAPREG_MEM_TYPE_64BIT;
		sc->sc_iwin[1].iwin_base_hi = 0;
		sc->sc_iwin[1].iwin_xlate = VERDE_OUT_XLATE_MEM_WIN0_BASE;
		sc->sc_iwin[1].iwin_size = VERDE_OUT_XLATE_MEM_WIN_SIZE;
	} else {
		panic("i80321: iwin[1] slave");
	}

	if (sc->sc_is_host) {
		sc->sc_iwin[2].iwin_base_lo = memstart |
		    PCI_MAPREG_MEM_PREFETCHABLE_MASK |
		    PCI_MAPREG_MEM_TYPE_64BIT;
		sc->sc_iwin[2].iwin_base_hi = 0;
		sc->sc_iwin[2].iwin_xlate = memstart;
		sc->sc_iwin[2].iwin_size = memsize;
	} else {
		panic("i80321: iwin[2] slave");
	}

	sc->sc_iwin[3].iwin_base_lo = 0 |
	    PCI_MAPREG_MEM_PREFETCHABLE_MASK |
	    PCI_MAPREG_MEM_TYPE_64BIT;
	sc->sc_iwin[3].iwin_base_hi = 0;
	sc->sc_iwin[3].iwin_xlate = 0;
	sc->sc_iwin[3].iwin_size = 0;

	/*
	 * We set up the Outbound Windows as follows:
	 *
	 *	0	Access to private PCI space.
	 *
	 *	1	Unused.
	 */
	sc->sc_owin[0].owin_xlate_lo =
	    PCI_MAPREG_MEM_ADDR(sc->sc_iwin[1].iwin_base_lo);
	sc->sc_owin[0].owin_xlate_hi = sc->sc_iwin[1].iwin_base_hi;

	/*
	 * Set the Secondary Outbound I/O window to map
	 * to PCI address 0 for all 64K of the I/O space.
	 */
	sc->sc_ioout_xlate = 0;

	/*
	 * Initialize the interrupt part of our PCI chipset tag.
	 */
	iq80321_pci_init(&sc->sc_pci_chipset, sc);

	i80321_attach(sc);
}
