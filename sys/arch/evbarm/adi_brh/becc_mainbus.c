/*	$NetBSD: becc_mainbus.c,v 1.2 2003/07/15 00:24:56 lukem Exp $	*/

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
 * ``Big Red Head'' front-end for the ADI Engineering Big Endian Companion
 * Chip.  We take care of setting up the BECC memory map, which is specific
 * to the board the BECC is wired up to.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: becc_mainbus.c,v 1.2 2003/07/15 00:24:56 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <evbarm/adi_brh/brhreg.h>
#include <evbarm/adi_brh/brhvar.h>

#include <arm/xscale/beccreg.h>
#include <arm/xscale/beccvar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

int	becc_mainbus_match(struct device *, struct cfdata *, void *);
void	becc_mainbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(becc_mainbus, sizeof(struct becc_softc),
    becc_mainbus_match, becc_mainbus_attach, NULL, NULL);

/* There can be only one. */
int	becc_mainbus_found;

int
becc_mainbus_match(struct device *parent, struct cfdata *cf, void *aux)
{
#if 0
	struct mainbus_attach_args *ma = aux;
#endif

	if (becc_mainbus_found)
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
becc_mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	extern paddr_t physical_start;

	struct becc_softc *sc = (void *) self;

	becc_mainbus_found = 1;

	printf(": ADI Big Endian Companion Chip, rev. %s\n",
	    becc_revisions[becc_rev]);

	/*
	 * Virtual addresses for the PCI I/O, 2 PCI MEM, and
	 * PCI CFG windows.
	 */
	sc->sc_pci_io_base = BRH_PCI_IO_VBASE;
	sc->sc_pci_mem_base[0] = BRH_PCI_MEM1_VBASE;
	sc->sc_pci_mem_base[1] = BRH_PCI_MEM2_VBASE;
	sc->sc_pci_cfg_base = BRH_PCI_CONF_VBASE;

	/*
	 * Ver <= 7: There are 2 32M inbound PCI memory windows.  Direct-
	 * map them to the first 64M of SDRAM.  We have limited SDRAM to
	 * 64M during bootstrap in this case.
	 *
	 * Ver >= 8: There is a 128M inbound PCI memory window which can
	 * cover all of SDRAM, which we obviously prefer to use.
	 *
	 * We map PCI:SDRAM 1:1, placing the two smaller windows after
	 * after the larger one.
	 */
	sc->sc_iwin[0].iwin_base = physical_start + 128 * 1024 * 1024;
	sc->sc_iwin[0].iwin_xlate = physical_start;
	sc->sc_iwin[1].iwin_base = sc->sc_iwin[0].iwin_base+BECC_PCI_MEM1_SIZE;
	sc->sc_iwin[1].iwin_xlate = physical_start + BECC_PCI_MEM1_SIZE;
	sc->sc_iwin[2].iwin_base = physical_start;
	sc->sc_iwin[2].iwin_xlate = physical_start;

	/*
	 * Ver <= 8: There are 2 32M outbound PCI memory windows.
	 * Ver >= 9: There are 3 32M outbound PCI memory windows.
	 *
	 * One of these may be byte swapped.  We don't use the third
	 * one available on >= Ver9.
	 */
	sc->sc_owin_xlate[0] = 32U * 1024 * 1024;
	sc->sc_owin_xlate[1] = 32U * 1024 * 1024;
	sc->sc_owin_xlate[2] = 32U * 1024 * 1024;

	/*
	 * Map the 1M PCI I/O window to PCI I/O address 0.
	 */
	sc->sc_ioout_xlate = 0;

	/*
	 * No platform-specific PCI interrupt routing; it's all done
	 * in the BECC.
	 */

	becc_attach(sc);
}
