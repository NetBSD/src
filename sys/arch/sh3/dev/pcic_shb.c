/* $NetBSD: pcic_shb.c,v 1.3 2000/06/29 07:44:02 mrg Exp $ */

#define	PCICSHBDEBUG

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

/*
 *  PCMCIA I/F for SH
 *
 *  T.Horiuichi
 *  Brains Corp. 1998.8.25
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/intr.h>

#if 0
#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#endif
#include <machine/shbvar.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciachip.h>

#include <sh3/dev/shpcicreg.h>
#include <sh3/dev/shpcicvar.h>

#ifdef TODO
#include <dev/isa/i82365_isavar.h>
#endif

#ifndef PCIC_SHB_INTR_ALLOC_MASK
#define	PCIC_SHB_INTR_ALLOC_MASK	0xfbff
#endif

int	pcic_shb_intr_alloc_mask = PCIC_SHB_INTR_ALLOC_MASK;

#ifdef PCICSHBDEBUG
int	pcicshb_debug = 1 /* XXX */ ;
#define	DPRINTF(arg) if (pcicshb_debug) printf arg;
#else
#define	DPRINTF(arg)
#endif

int	pcic_shb_probe __P((struct device *, struct cfdata *, void *));
void	pcic_shb_attach __P((struct device *, struct device *, void *));

void	*pcic_shb_chip_intr_establish __P((pcmcia_chipset_handle_t,
	    struct pcmcia_function *, int, int (*) (void *), void *));
void	pcic_shb_chip_intr_disestablish __P((pcmcia_chipset_handle_t, void *));

struct cfattach pcic_shb_ca = {
	sizeof(struct shpcic_softc),
	pcic_shb_probe,
	pcic_shb_attach
};

static struct pcmcia_chip_functions pcic_shb_functions = {
	shpcic_chip_mem_alloc,
	shpcic_chip_mem_free,
	shpcic_chip_mem_map,
	shpcic_chip_mem_unmap,

	shpcic_chip_io_alloc,
	shpcic_chip_io_free,
	shpcic_chip_io_map,
	shpcic_chip_io_unmap,

	pcic_shb_chip_intr_establish,
	pcic_shb_chip_intr_disestablish,

	shpcic_chip_socket_enable,
	shpcic_chip_socket_disable,
};

int
pcic_shb_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
#if 0
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh, memh;
	int val, found;

	/* Disallow wildcarded i/o address. */
	if (ia->ia_iobase == SHBCF_PORT_DEFAULT)
		return (0);

	if (bus_space_map(iot, ia->ia_iobase, SHPCIC_IOSIZE, 0, &ioh))
		return (0);

	if (ia->ia_msize == -1)
		ia->ia_msize = SHPCIC_MEMSIZE;

	if (bus_space_map(ia->ia_memt, ia->ia_maddr, ia->ia_msize, 0, &memh))
		return (0);

	found = 0;

	/*
	 * this could be done with a loop, but it would violate the
	 * abstraction
	 */

	bus_space_write_1(iot, ioh, SHPCIC_REG_INDEX, C0SA + SHPCIC_IDENT);

	val = bus_space_read_1(iot, ioh, SHPCIC_REG_DATA);

	if (shpcic_ident_ok(val))
		found++;


	bus_space_write_1(iot, ioh, SHPCIC_REG_INDEX, C0SB + SHPCIC_IDENT);

	val = bus_space_read_1(iot, ioh, SHPCIC_REG_DATA);

	if (shpcic_ident_ok(val))
		found++;


	bus_space_write_1(iot, ioh, SHPCIC_REG_INDEX, C1SA + SHPCIC_IDENT);

	val = bus_space_read_1(iot, ioh, SHPCIC_REG_DATA);

	if (shpcic_ident_ok(val))
		found++;


	bus_space_write_1(iot, ioh, SHPCIC_REG_INDEX, C1SB + SHPCIC_IDENT);

	val = bus_space_read_1(iot, ioh, SHPCIC_REG_DATA);

	if (shpcic_ident_ok(val))
		found++;


	bus_space_unmap(iot, ioh, SHPCIC_IOSIZE);
	bus_space_unmap(ia->ia_memt, memh, ia->ia_msize);

	if (!found)
		return (0);

	ia->ia_iosize = SHPCIC_IOSIZE;
#endif
	return (1);
}

void
pcic_shb_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct shpcic_softc *sc = (void *) self;
	struct shb_attach_args *ia = aux;
#if 0
	isa_chipset_tag_t ic = ia->ia_ic;
#endif
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_tag_t memt = ia->ia_memt;
	bus_space_handle_t ioh;
	bus_space_handle_t memh;


	/* Map i/o space. */
	if (bus_space_map(iot, ia->ia_iobase, ia->ia_iosize, 0, &ioh)) {
		printf(": can't map i/o space\n");
		return;
	}

	/* Map mem space. */
	if (bus_space_map(memt, ia->ia_maddr, ia->ia_msize, 0, &memh)) {
		printf(": can't map mem space\n");
		return;
	}

	sc->membase = ia->ia_maddr;
#if 1   /* 1999.05.17 T.Horiuchi for R1.4 */
	sc->subregionmask = 1;
#else
	sc->subregionmask = (1 << (ia->ia_msize / SHPCIC_MEM_PAGESIZE)) - 1;
#endif

#if 0
	sc->intr_est = ic;
#endif
	sc->pct = (pcmcia_chipset_tag_t) & pcic_shb_functions;

	sc->iot = iot;
	sc->ioh = ioh;
	sc->memt = memt;
	sc->memh = memh;

	/*
	 * allocate an irq.  it will be used by both controllers.  I could
	 * use two different interrupts, but interrupts are relatively
	 * scarce, shareable, and for PCIC controllers, very infrequent.
	 */

	if ((sc->irq = ia->ia_irq) == IRQUNK) {
		if (sh_intr_alloc(
		    SHPCIC_CSC_INTR_IRQ_VALIDMASK & pcic_shb_intr_alloc_mask,
		    IST_EDGE, &sc->irq)) {
			printf("\n%s: can't allocate interrupt\n",
			    sc->dev.dv_xname);
			return;
		}
		printf(": using irq %d", sc->irq);
	}
	printf("\n");

	shpcic_attach(sc);

#if 0
	pcic_shb_bus_width_probe (sc, iot, ioh, ia->ia_iobase, ia->ia_iosize);
#endif

	sc->ih = shb_intr_establish(sc->irq, IST_EDGE, IPL_TTY,
	    shpcic_intr, sc);
	if (sc->ih == NULL) {
		printf("%s: can't establish interrupt\n", sc->dev.dv_xname);
		return;
	}

	shpcic_attach_sockets(sc);
}
