/*	$NetBSD: sa11x0_pcmcia.c,v 1.2 2001/02/23 04:31:19 ichiro Exp $	*/

/*
 * Copyright (c) 2000 Robert Swindells.  All rights reserved.
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
#include <sys/callout.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciachip.h>

#include <hpcarm/sa11x0/sa11x0_reg.h>
#include <hpcarm/sa11x0/sa11x0_var.h>
#include <hpcarm/sa11x0/sa11x0_pcmciavar.h>

int	sa11x0_pcmcia_match(struct device *, struct cfdata *, void *);
void	sa11x0_pcmcia_attach(struct device *, struct device *, void *);

static int sa11x0_pcmcia_mem_alloc(pcmcia_chipset_handle_t, bus_size_t,
				     struct pcmcia_mem_handle *);
static void sa11x0_pcmcia_mem_free(pcmcia_chipset_handle_t,
				     struct pcmcia_mem_handle *);
static int sa11x0_pcmcia_mem_map(pcmcia_chipset_handle_t, int, bus_addr_t,
				   bus_size_t, struct pcmcia_mem_handle *,
				   bus_addr_t *, int *);
static void sa11x0_pcmcia_mem_unmap(pcmcia_chipset_handle_t, int);
static int sa11x0_pcmcia_io_alloc(pcmcia_chipset_handle_t, bus_addr_t,
				    bus_size_t, bus_size_t,
				    struct pcmcia_io_handle *);
static void sa11x0_pcmcia_io_free(pcmcia_chipset_handle_t,
				    struct pcmcia_io_handle *);
static int sa11x0_pcmcia_io_map(pcmcia_chipset_handle_t, int, bus_addr_t,
				  bus_size_t, struct pcmcia_io_handle *,
				  int *);
static void sa11x0_pcmcia_io_unmap(pcmcia_chipset_handle_t, int);
static void *sa11x0_pcmcia_intr_establish(pcmcia_chipset_handle_t,
					    struct pcmcia_function *, int,
					    int (*)(void *), void *);
static void sa11x0_pcmcia_intr_disestablish(pcmcia_chipset_handle_t, void *);
static void sa11x0_pcmcia_socket_enable(pcmcia_chipset_handle_t);
static void sa11x0_pcmcia_socket_disable(pcmcia_chipset_handle_t);

struct cfattach sapcic_ca = {
	sizeof(struct sa11x0_pcmcia_softc), sa11x0_pcmcia_match, sa11x0_pcmcia_attach
};

static struct pcmcia_chip_functions sa11x0_pcmcia_functions = {
	sa11x0_pcmcia_mem_alloc,
	sa11x0_pcmcia_mem_free,
	sa11x0_pcmcia_mem_map,
	sa11x0_pcmcia_mem_unmap,

	sa11x0_pcmcia_io_alloc,
	sa11x0_pcmcia_io_free,
	sa11x0_pcmcia_io_map,
	sa11x0_pcmcia_io_unmap,

	sa11x0_pcmcia_intr_establish,
	sa11x0_pcmcia_intr_disestablish,

	sa11x0_pcmcia_socket_enable,
	sa11x0_pcmcia_socket_disable,
};

int
sa11x0_pcmcia_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return (0);
}

void
sa11x0_pcmcia_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct sa11x0_pcmcia_softc *sc = (struct sa11x0_pcmcia_softc*)self;
	struct sa11x0_attach_args *sa = aux;

	sc->sc_iot = sa->sa_iot;
	sc->sc_baseaddr = sa->sa_addr;
	
printf("0\n");
	/* Map i/o space. */
	if (bus_space_map(sc->sc_iot, sa->sa_addr, sa->sa_size, 0,
		&sc->sc_ioh)) {
		printf(": can't map i/o space\n");
		return;
	}
printf("1\n");

	sc->sc_membase = sa->sa_membase;

	/* Map mem space. */
	if (bus_space_map(sc->sc_memt, sa->sa_membase, sa->sa_memsize, 0,
		&sc->sc_memh)) {
		printf(": can't map mem space\n");
		return;
	}

printf("2\n");

	sc->sc_pct = (pcmcia_chipset_tag_t) &sa11x0_pcmcia_functions;

	/*sc->irq = sa->saa_spa.spa_irq;*/

	printf("\n");
	printf(": SA-11x0 PCMCIA enable\n");
/*
	pcic_attach(sc);
	pcic_attach_sockets(sc);
*/
	/*config_interrupts(self, pcic_isa_config_interrupts);*/
}

static int
sa11x0_pcmcia_mem_alloc(pch, size, pmh)
	pcmcia_chipset_handle_t pch;
	bus_size_t size;
	struct pcmcia_mem_handle *pmh;
{
	return 0;
}


static void
sa11x0_pcmcia_mem_free(pch, pmh)
	pcmcia_chipset_handle_t pch;
	struct pcmcia_mem_handle *pmh;
{
}

static int
sa11x0_pcmcia_mem_map(pch, kind, card_addr, size, pmh, offsetp, windowp)
	pcmcia_chipset_handle_t pch;
	int kind;
	bus_addr_t card_addr;
	bus_size_t size;
	struct pcmcia_mem_handle *pmh;
	bus_addr_t *offsetp;
	int *windowp;
{
	return 0;
}

static void
sa11x0_pcmcia_mem_unmap(pch, window)
	pcmcia_chipset_handle_t pch;
	int window;
{
}

static int
sa11x0_pcmcia_io_alloc(pch, start, size, align, pih)
	pcmcia_chipset_handle_t pch;
	bus_addr_t start;
	bus_size_t size;
	bus_size_t align;
	struct pcmcia_io_handle *pih;
{
	return 0;
}

static void
sa11x0_pcmcia_io_free(pch, pih)
	pcmcia_chipset_handle_t pch;
	struct pcmcia_io_handle *pih;
{
}

static int
sa11x0_pcmcia_io_map(pch, width, offset, size, pih, windowp)
	pcmcia_chipset_handle_t pch;
	int width;
	bus_addr_t offset;
	bus_size_t size;
	struct pcmcia_io_handle *pih;
	int *windowp;
{
	return 0;
}

static void sa11x0_pcmcia_io_unmap(pch, window)
	pcmcia_chipset_handle_t pch;
	int window;
{
}

static void *
sa11x0_pcmcia_intr_establish(pch, pf, ipl, fct, arg)
	pcmcia_chipset_handle_t pch;
	struct pcmcia_function *pf;
	int ipl;
	int (*fct)(void *);
	void *arg;
{
	return NULL;
}

static void
sa11x0_pcmcia_intr_disestablish(pch, ih)
	pcmcia_chipset_handle_t pch;
	void *ih;
{
}

static void
sa11x0_pcmcia_socket_enable(pch)
	pcmcia_chipset_handle_t pch;
{
}

static void
sa11x0_pcmcia_socket_disable(pch)
	pcmcia_chipset_handle_t pch;
{
}

