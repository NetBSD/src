/*	$NetBSD: ixp425.c,v 1.4 2003/07/02 11:02:28 ichiro Exp $ */

/*
 * Copyright (c) 2003
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
 * All rights reserved.
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
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ixp425.c,v 1.4 2003/07/02 11:02:28 ichiro Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <uvm/uvm.h>

#include <machine/bus.h>

#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425var.h>

#include <evbarm/ixdp425/ixdp425reg.h>

int		ixp425_pcibus_print(void *, const char *);
static struct	ixp425_softc *ixp425_softc;

void
ixp425_attach(struct ixp425_softc *sc)
{
	ixp425_softc = sc;

	printf("\n");

#if 0
	/*
	 * PCI bus reset
	 */
	/* disable PCI command */

	/* XXX assert PCI reset Mode */

	/* We setup a 1:1 memory map of bus<->physical addresses */

	/*
	 * Enable bus mastering and I/O,memory access
	 */
	

	/*
	 * Initialize the bus space and DMA tags and the PCI chipset tag.
	 */
	ixp425_io_bs_init(&sc->ia_pci_iot, sc);
	ixp425_mem_bs_init(&sc->ia_pci_memt, sc);
	ixp425_pci_init(&sc->ia_pci_chipset, sc);

	/*
	 * Initialize the DMA tags.
	 */
	ixp425_pci_dma_init(sc);

	/*
	 * Attach the PCI bus.
	 */
	pba.pba_busname = "pci";
	pba.pba_pc = &sc->ia_pci_chipset;
	pba.pba_iot = &sc->ia_pci_iot;
	pba.pba_memt = &sc->ia_pci_memt;
	pba.pba_dmat = &sc->ia_pci_dmat;
	pba.pba_bus = 0;	/* bus number = 0 */
	pba.pba_intrswiz = 0;	/* XXX */
	pba.pba_intrtag = 0;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED |
			PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY |
			PCI_FLAGS_MWI_OKAY;
	(void) config_found(&sc->sc_dev, &pba, ixp425_pcibus_print);
#endif
}

int
ixp425_pcibus_print(void *aux, const char *pnp)
{
	struct pcibus_attach_args *pba = aux;

	if (pnp)
		aprint_normal("%s at %s", pba->pba_busname, pnp);

	aprint_normal(" bus %d", pba->pba_bus);

	return (UNCONF);
}
