/* $NetBSD: sableio.c,v 1.1 2000/12/21 20:51:57 thorpej Exp $ */

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Driver glue for the Sable STDIO module.
 *
 * This is kind of a hack.  The STDIO is really a junk I/O module with
 * your regular ISA junk peripherals and their regular ISA I/O addresses.
 * However, the main issue we have here is *interrupts*.  Not only are
 * devices IRQs strange (i.e. not what you would expect to find if they
 * were attached to a real ISA) IRQs, the keyboard controller isn't even
 * connected to the (E)ISA IRQ space at all!
 *
 * In short, we're gluing together the following things:
 *
 *	- Standard ISA junk I/O chip
 *	- ISA DMA
 *	- Pre-mapped "PCI" interrupts that are *edge triggered*
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: sableio.c,v 1.1 2000/12/21 20:51:57 thorpej Exp $");

#include "isadma.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/pci/pcivar.h>

#include <alpha/sableio/sableiovar.h>

/*
 * The devices built-in to the Sable STDIO module.
 */
const struct sableio_dev {
	const char *sd_name;		/* device name */
	bus_addr_t sd_ioaddr;		/* I/O space address */
	int sd_sableirq[2];		/* Sable IRQs */
	int sd_drq;			/* ISA DRQ */
} sableio_devs[] = {
	/*
	 * See alpha/pci/pci_2100_a500.c for interrupt information.
	 */
	{ "pckbc",	IO_KBD,		{ 6, 3 },	-1 },
	{ "fdc",	IO_FD1,		{ 7, -1 },	2 },
	{ "com",	IO_COM1,	{ 15, -1 },	-1 },
	{ "com",	IO_COM2,	{ 8, -1 },	-1 },
	{ "lpt",	IO_LPT3,	{ 9, -1 },	-1 },
	{ NULL,		0,		{ -1, -1 },	-1 },
};

struct sableio_softc {
	struct device	sc_dev;		/* base device */

	/*
	 * We have to deal with ISA DMA, so that means we have to
	 * hold the ISA chipset, since we attach STDIO devices
	 * before we attach the PCI (and thus EISA) bus.
	 */
	struct alpha_isa_chipset sc_isa_chipset;
};

int	sableio_match(struct device *, struct cfdata *, void *);
void	sableio_attach(struct device *, struct device *, void *);

struct cfattach sableio_ca = {
	sizeof(struct sableio_softc), sableio_match, sableio_attach
};

int	sableio_print(void *, const char *);
int	sableio_submatch(struct device *, struct cfdata *, void *);

struct sableio_softc *sableio_attached;

int
sableio_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct pcibus_attach_args *pba = aux;

	if (strcmp(pba->pba_busname, cf->cf_driver->cd_name) != 0)
		return (0);

	/*
	 * These are really ISA devices, and thus must be on
	 * PCI bus 0.
	 */
	if (cf->pcibuscf_bus != PCIBUS_UNK_BUS &&
	    cf->pcibuscf_bus != pba->pba_bus)
		return (0);

	/* sanity */
	if (pba->pba_bus != 0)
		return (0);

	/* There can be only one. */
	if (sableio_attached != NULL)
		return (0);

	return (1);
}

void
sableio_attach(struct device *parent, struct device *self, void *aux)
{
	struct sableio_softc *sc = (void *) self;
	struct pcibus_attach_args *pba = aux;
	struct sableio_attach_args sa;
	bus_dma_tag_t dmat;
	int i;

	printf(": Sable STDIO module\n");

	sableio_attached = sc;

	dmat = alphabus_dma_get_tag(pba->pba_dmat, ALPHA_BUS_ISA);

#if NISADMA > 0
	/*
	 * Initialize our DMA state.
	 */
	isa_dmainit(&sc->sc_isa_chipset, pba->pba_iot, dmat, self);
#endif

	for (i = 0; sableio_devs[i].sd_name != NULL; i++) {
		sa.sa_name = sableio_devs[i].sd_name;
		sa.sa_ioaddr = sableio_devs[i].sd_ioaddr;
		sa.sa_sableirq[0] = sableio_devs[i].sd_sableirq[0];
		sa.sa_sableirq[1] = sableio_devs[i].sd_sableirq[1];
		sa.sa_drq = sableio_devs[i].sd_drq;

		sa.sa_iot = pba->pba_iot;
		sa.sa_dmat = dmat;
		sa.sa_ic = &sc->sc_isa_chipset;
		sa.sa_pc = pba->pba_pc;

		(void) config_found_sm(self, &sa, sableio_print,
		    sableio_submatch);
	}
}

int
sableio_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct sableio_attach_args *sa = aux;

	if (cf->cf_loc[SABLEIOCF_PORT] != SABLEIOCF_PORT_DEFAULT &&
	    cf->cf_loc[SABLEIOCF_PORT] != sa->sa_ioaddr)
		return (0);

	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

int
sableio_print(void *aux, const char *pnp)
{
	struct sableio_attach_args *sa = aux;

	if (pnp != NULL)
		printf("%s at %s", sa->sa_name, pnp);

	printf(" port 0x%lx", sa->sa_ioaddr);
	return (UNCONF);
}

isa_chipset_tag_t
sableio_pickisa(void)
{

	if (sableio_attached == NULL)
		panic("sableio_pickisa");

	return (&sableio_attached->sc_isa_chipset);
}
