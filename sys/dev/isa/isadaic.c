/*-
 * Copyright (c) 1997, 2001 Martin Husemann <martin@duskware.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
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
 *
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/device.h>

#if defined(__NetBSD__) && __NetBSD_Version__ >= 104230000
#include <sys/callout.h>
#endif

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/bus.h>

#include <sys/socket.h>
#include <net/if.h>
#include <dev/isa/isavar.h>
#include <netisdn/i4b_ioctl.h>
#include <netisdn/i4b_l3l4.h>
#include <dev/ic/daicvar.h>

/* driver state */
struct isa_daic_softc {
	struct daic sc_daic;		/* MI driver state */
	void *sc_ih;			/* interrupt handler */
};

/* local functions */
#ifdef __BROKEN_INDIRECT_CONFIG
static int isa_daic_probe __P((struct device *, void *, void *));
#else
static int isa_daic_probe __P((struct device *, struct cfdata *, void *));
#endif
static void isa_daic_attach __P((struct device *, struct device *, void *));
static int isa_daic_intr __P((void *));

struct cfattach isa_daic_ca = {
	sizeof(struct isa_daic_softc), isa_daic_probe, isa_daic_attach
};

static int
#ifdef __BROKEN_INDIRECT_CONFIG
isa_daic_probe(parent, match, aux)
#else
isa_daic_probe(parent, cf, aux)
#endif
	struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
	void *match;
#else
	struct cfdata *cf;
#endif
	void *aux;
{
	struct isa_attach_args *ia = aux;
	bus_space_tag_t memt = ia->ia_memt;
	bus_space_handle_t memh;
	int card;

	/* We need some controller memory to comunicate! */
	if (ia->ia_maddr == MADDRUNK || ia->ia_msize == -1)
		goto bad;

	/* Map card RAM. */
	ia->ia_msize = DAIC_ISA_MEMSIZE;
	ia->ia_iosize = 0;
	if (bus_space_map(memt, ia->ia_maddr, ia->ia_msize,
	    0, &memh))
		goto bad;

	/* MI check for card at this location */
	card = daic_probe(memt, memh);
	if (card < 0)
		goto bad;
	if (card == DAIC_TYPE_QUAD)
		ia->ia_msize = DAIC_ISA_QUADSIZE;

	bus_space_unmap(memt, memh, DAIC_ISA_MEMSIZE);
	return 1;

bad:
	/* unmap card RAM */
	bus_space_unmap(memt, memh, DAIC_ISA_MEMSIZE);
	return 0;
}

static void
isa_daic_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct isa_daic_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;
	bus_space_tag_t memt = ia->ia_memt;
	bus_space_handle_t memh;

	/* Map card RAM. */
	if (bus_space_map(memt, ia->ia_maddr, ia->ia_msize,
	    0, &memh))
		return;

	sc->sc_daic.sc_iot = memt;
	sc->sc_daic.sc_ioh = memh;

	/* MI initialization of card */
	daic_attach(self, &sc->sc_daic);

	sc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq, IST_EDGE,
	    IPL_NET, isa_daic_intr, sc);
}

/*
 * Controller interrupt.
 */
static int
isa_daic_intr(arg)
	void *arg;
{
	struct isa_daic_softc *sc = arg;
	return daic_intr(&sc->sc_daic);
}
