/*	$NetBSD: i82365_pci.c,v 1.6 1998/12/17 17:45:08 msaitoh Exp $	*/

/*
 * Copyright (c) 1997 Marc Horowitz.  All rights reserved.
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
 *	This product includes software developed by Marc Horowitz.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/ic/i82365reg.h>
#include <dev/ic/i82365var.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

/*
 * PCI constants.
 * XXX These should be in a common file!
 */
#define	PCI_CBIO		0x10	/* Configuration Base IO Address */

int	pcic_pci_match __P((struct device *, struct cfdata *, void *));
void	pcic_pci_attach __P((struct device *, struct device *, void *));

void	*pcic_pci_chip_intr_establish __P((pcmcia_chipset_handle_t,
	    struct pcmcia_function *, int, int (*) (void *), void *));
void	pcic_pci_chip_intr_disestablish __P((pcmcia_chipset_handle_t, void *));

struct cfattach pcic_pci_ca = {
	sizeof(struct pcic_softc), pcic_pci_match, pcic_pci_attach
};

static struct pcmcia_chip_functions pcic_pci_functions = {
	pcic_chip_mem_alloc,
	pcic_chip_mem_free,
	pcic_chip_mem_map,
	pcic_chip_mem_unmap,

	pcic_chip_io_alloc,
	pcic_chip_io_free,
	pcic_chip_io_map,
	pcic_chip_io_unmap,

	pcic_pci_chip_intr_establish,
	pcic_pci_chip_intr_disestablish,

	pcic_chip_socket_enable,
	pcic_chip_socket_disable,
};

int
pcic_pci_match(parent, match, aux)
	struct device *parent;
	struct cfdata  *match;
	void *aux;
{
	struct pci_attach_args *pa = (struct pci_attach_args *) aux;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_CIRRUS)
		return (0);

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_CIRRUS_CL_PD6729:
		break;
	default:
		return (0);
	}

	return (1);
}

void
pcic_pci_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pcic_softc *sc = (void *) self;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	bus_space_tag_t memt = pa->pa_memt;
	bus_space_handle_t memh;
	pci_intr_handle_t ih;
	char *model;
	const char *intrstr = NULL;

	if (pci_mapreg_map(pa, PCI_CBIO, PCI_MAPREG_TYPE_IO, 0,
	    &sc->iot, &sc->ioh, NULL, NULL)) {
		printf(": can't map i/o space\n");
		return;
	}

	/*
	 * XXX need some memory for mapping pcmcia cards into. Ideally, this
	 * would be completely dynamic.  Practically this doesn't work,
	 * because the extent mapper doesn't know about all the devices all
	 * the time.  With ISA we could finesse the issue by specifying the
	 * memory region in the config line.  We can't do that here, so we
	 * cheat for now. Jason Thorpe, you are my Savior, come up with a fix
	 * :-)
	 */

	/* Map mem space. */
	if (bus_space_map(memt, 0xd0000, 0x4000, 0, &memh))
		panic("pcic_pci_attach: can't map mem space");

	sc->membase = 0xd0000;
	sc->subregionmask = (1 << (0x4000 / PCIC_MEM_PAGESIZE)) - 1;

	/* same deal for io allocation */

	sc->iobase = 0x400;
	sc->iosize = 0xbff;

	/* end XXX */

	sc->intr_est = pc;
	sc->pct = (pcmcia_chipset_tag_t) & pcic_pci_functions;

	sc->memt = memt;
	sc->memh = memh;

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_CIRRUS_CL_PD6729:
		model = "Cirrus Logic PD6729 PCMCIA controller";
		break;
	default:
		model = "Model unknown";
		break;
	}

	printf(": %s\n", model);

	/* Enable the card. */
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG) |
	    PCI_COMMAND_MASTER_ENABLE);

	pcic_attach(sc);

	/* Map and establish the interrupt. */
	if (pci_intr_map(pc, pa->pa_intrtag, pa->pa_intrpin,
	    pa->pa_intrline, &ih)) {
		printf("%s: couldn't map interrupt\n", sc->dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->ih = pci_intr_establish(pc, ih, IPL_NET, pcic_intr, sc);
	if (sc->ih == NULL) {
		printf("%s: couldn't establish interrupt",
		       sc->dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->dev.dv_xname, intrstr);

	pcic_attach_sockets(sc);
}

/*
 * XXX This is almost totally wrong!  We need to map to PCI interupts,
 * XXX which themselves map to somthing else.
 */

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

void *
pcic_pci_chip_intr_establish(pch, pf, ipl, fct, arg)
	pcmcia_chipset_handle_t pch;
	struct pcmcia_function *pf;
	int ipl;
	int (*fct) (void *);
	void *arg;
{
	struct pcic_handle *h = (struct pcic_handle *) pch;
	pci_chipset_tag_t pc = h->sc->intr_est;
	int irq;
	pci_intr_handle_t piht;
	void *ih;
	int reg;

	/*
	 * XXX this will work for x86, but is guaranteed to lose elsewhere.
	 * Hopefully Jason can bail me out of this one, too.
	 */

	isa_intr_alloc(NULL, 0xffff, IST_PULSE, &irq);

	if (pci_intr_map(pc, pci_make_tag(NULL, 0, 0, 0),
	    1, irq, &piht)) {
		printf("%s: couldn't map interrupt\n", h->sc->dev.dv_xname);
		return (NULL);
	}
	if ((ih = pci_intr_establish(pc, piht, ipl, fct,
	    arg)) == NULL)
		return (NULL);

	reg = pcic_read(h, PCIC_INTR);
	reg |= PCIC_INTR_ENABLE;
	reg |= irq;
	pcic_write(h, PCIC_INTR, reg);

	printf("%s: card irq %d\n", h->pcmcia->dv_xname, irq);

	return (ih);
}

void 
pcic_pci_chip_intr_disestablish(pch, ih)
	pcmcia_chipset_handle_t pch;
	void *ih;
{
	struct pcic_handle *h = (struct pcic_handle *) pch;
	pci_chipset_tag_t pc = h->sc->intr_est;

	pci_intr_disestablish(pc, ih);
}
