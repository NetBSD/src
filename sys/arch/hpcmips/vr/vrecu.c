/* $NetBSD: vrecu.c,v 1.9 2009/09/14 13:41:15 tsutsui Exp $ */

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Naoto Shimazaki of YOKOGAWA Electric Corporation.
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
__KERNEL_RCSID(0, "$NetBSD: vrecu.c,v 1.9 2009/09/14 13:41:15 tsutsui Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <hpcmips/vr/vrcpudef.h>
#include <hpcmips/vr/vripif.h>
#include <hpcmips/vr/vr4181ecureg.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciachip.h>

#include <dev/ic/i82365reg.h>
#include <dev/ic/i82365var.h>
#include <dev/isa/i82365_isavar.h>

static int pcic_vrip_match(struct device *, struct cfdata *, void *);
static void pcic_vrip_attach(struct device *, struct device *, void *);
static void *pcic_vrip_chip_intr_establish(pcmcia_chipset_handle_t,
					   struct pcmcia_function *, int,
					   int (*)(void *), void *);
static void pcic_vrip_chip_intr_disestablish(pcmcia_chipset_handle_t, void *);
static int pcic_vrip_intr(void *);

struct pcic_vrip_softc {
	struct pcic_softc	sc_pcic;	/* real pcic softc */
	uint16_t		sc_intr_mask;
	uint16_t		sc_intr_valid;
	struct intrhand {
		int	(*ih_fun)(void *);
		void	*ih_arg;
	} 			sc_intrhand[ECU_MAX_INTR];
};

CFATTACH_DECL(pcic_vrip, sizeof(struct pcic_vrip_softc),
	      pcic_vrip_match, pcic_vrip_attach, NULL, NULL);

static struct pcmcia_chip_functions pcic_vrip_functions = {
	.mem_alloc		= pcic_chip_mem_alloc,
	.mem_free		= pcic_chip_mem_free,
	.mem_map		= pcic_chip_mem_map,
	.mem_unmap		= pcic_chip_mem_unmap,

	.io_alloc		= pcic_chip_io_alloc,
	.io_free		= pcic_chip_io_free,
	.io_map			= pcic_chip_io_map,
	.io_unmap		= pcic_chip_io_unmap,

	.intr_establish		= pcic_vrip_chip_intr_establish,
	.intr_disestablish	= pcic_vrip_chip_intr_disestablish,

	.socket_enable		= pcic_chip_socket_enable,
	.socket_disable		= pcic_chip_socket_disable,
	.socket_settype		= pcic_chip_socket_settype,
};


static int
pcic_vrip_match(struct device *parent, struct cfdata *match, void *aux)
{
	return 1;
}

static void
pcic_vrip_attach(struct device *parent, struct device *self, void *aux)
{
	struct pcic_vrip_softc	*vsc = device_private(self);
	struct pcic_softc	*sc = &vsc->sc_pcic;
	struct vrip_attach_args	*va = aux;
	bus_space_handle_t	ioh;
	bus_space_handle_t	memh;
	int			i;

	vsc->sc_intr_valid = PCIC_INTR_IRQ_VALIDMASK;
	vsc->sc_intr_mask = 0xffff;
	for (i = 0; i < ECU_MAX_INTR; i++)
		vsc->sc_intrhand[i].ih_fun = NULL;

	if ((sc->ih = vrip_intr_establish(va->va_vc, va->va_unit, 0,
					  IPL_NET, pcic_vrip_intr, vsc))
	    == NULL) {
		printf(": can't establish interrupt");
	}

        /* Map i/o space. */
        if (bus_space_map(va->va_iot, va->va_addr, ECU_SIZE, 0, &ioh)) {
                printf(": can't map pcic register space\n");
                return;
        }

	/* init CFG_REG_1 */
	bus_space_write_2(va->va_iot, ioh, ECU_CFG_REG_1_W, 0x0001);

	/* mask all interrupt */
	bus_space_write_2(va->va_iot, ioh, ECU_INTMSK_REG_W,
			  vsc->sc_intr_mask);

	/* Map mem space. */
#if 1
	if (bus_space_map(va->va_iot, VR_ISA_MEM_BASE, 0x4000, 0, &memh))
		panic("pcic_pci_attach: can't map mem space");

	sc->membase = VR_ISA_MEM_BASE;
	sc->subregionmask = (1 << (0x4000 / PCIC_MEM_PAGESIZE)) - 1;

	sc->iobase = VR_ISA_PORT_BASE + 0x400;
	sc->iosize = 0xbff;
#else 
	if (bus_space_map(va->va_iot, VR_ISA_MEM_BASE, 0x70000, 0, &memh))
		panic("pcic_pci_attach: can't map mem space");

	sc->membase = VR_ISA_MEM_BASE;
	sc->subregionmask = (1 << (0x70000 / PCIC_MEM_PAGESIZE)) - 1;

	sc->iobase = VR_ISA_PORT_BASE;
	sc->iosize = 0x10000;
#endif

	sc->pct = &pcic_vrip_functions;

	sc->iot = va->va_iot;
	sc->ioh = ioh;
	sc->memt = va->va_iot;
	sc->memh = memh;

	printf("\n");

	sc->irq = -1;

	pcic_attach(sc);
	pcic_attach_sockets(sc);
        pcic_attach_sockets_finish(sc);
}

static void *
pcic_vrip_chip_intr_establish(pcmcia_chipset_handle_t pch,
			      struct pcmcia_function *pf,
			      int ipl,
			      int (*ih_fun)(void *),
			      void *ih_arg)
{
	struct pcic_handle	*h;
	struct pcic_softc	*sc;
	struct pcic_vrip_softc	*vsc;
	struct intrhand		*ih;

	int	irq;
	int	r;


	/*
	 * XXXXXXXXXXXXXXXXXXXXXXXXXXXX
	 * XXXXXXXXXXXXXXXXXXXXXXXXXXXX
	 * XXXXXXXXXXXXXXXXXXXXXXXXXXXX
	 */
	irq = 11;
	/*
	 * XXXXXXXXXXXXXXXXXXXXXXXXXXXX
	 * XXXXXXXXXXXXXXXXXXXXXXXXXXXX
	 * XXXXXXXXXXXXXXXXXXXXXXXXXXXX
	 */


	h = (struct pcic_handle *) pch;
	vsc = device_private(h->ph_parent);
	sc = &vsc->sc_pcic;


	ih = &vsc->sc_intrhand[irq];
	if (ih->ih_fun) /* cannot share ecu interrupt */
		return NULL;
	ih->ih_fun = ih_fun;
	ih->ih_arg = ih_arg;

	h->ih_irq = irq;
	if (h->flags & PCIC_FLAG_ENABLED) {
		r = pcic_read(h, PCIC_INTR);
		r &= ~PCIC_INTR_IRQ_MASK;
		pcic_write(h, PCIC_INTR, r | irq);
        }

	vsc->sc_intr_mask &= ~(1 << irq);
	bus_space_write_2(sc->iot, sc->ioh, ECU_INTMSK_REG_W,
			  vsc->sc_intr_mask);

	return ih;
}

static void 
pcic_vrip_chip_intr_disestablish(pcmcia_chipset_handle_t pch, void *arg)
{
	struct pcic_handle	*h;
	struct pcic_softc	*sc;
	struct pcic_vrip_softc	*vsc;
	struct intrhand		*ih = arg;

	int	s;
	int	r;

	h = (struct pcic_handle *) pch;
	vsc = device_private(h->ph_parent);
	sc = &vsc->sc_pcic;

	if (ih != &vsc->sc_intrhand[h->ih_irq])
		panic("pcic_vrip_chip_intr_disestablish: bad handler");

	s = splhigh();

	vsc->sc_intr_mask |= 1 << h->ih_irq;
	bus_space_write_2(sc->iot, sc->ioh, ECU_INTMSK_REG_W,
			  vsc->sc_intr_mask);

	h->ih_irq = 0;
	if (h->flags & PCIC_FLAG_ENABLED) {
		r = pcic_read(h, PCIC_INTR);
		r &= ~(PCIC_INTR_IRQ_MASK | PCIC_INTR_ENABLE);
		pcic_write(h, PCIC_INTR, r);
        }

	ih->ih_fun = NULL;
	ih->ih_arg = NULL;

	splx(s);
}

/*
 * interrupt handler
 */
static int
pcic_vrip_intr(void *arg)
{
	struct pcic_vrip_softc	*vsc = arg;
	struct pcic_softc	*sc = &vsc->sc_pcic;
	int			i;
	uint16_t		r;

	r = bus_space_read_2(sc->iot, sc->ioh, ECU_INTSTAT_REG_W)
		& ~vsc->sc_intr_mask;

	for (i = 0; i < ECU_MAX_INTR; i++) {
		struct intrhand	*ih = &vsc->sc_intrhand[i];
		if (ih->ih_fun && (r & (1 << i)))
			ih->ih_fun(ih->ih_arg);
	}
	return 1;
}
