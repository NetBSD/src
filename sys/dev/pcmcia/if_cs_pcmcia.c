/* $NetBSD: if_cs_pcmcia.c,v 1.8 2004/08/09 18:41:36 mycroft Exp $ */

/*-
 * Copyright (c)2001 YAMAMOTO Takashi,
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_cs_pcmcia.c,v 1.8 2004/08/09 18:41:36 mycroft Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include "rnd.h"
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciadevs.h>

#include <dev/ic/cs89x0reg.h>
#include <dev/ic/cs89x0var.h>

#define DEVNAME(sc) ((sc)->sc_dev.dv_xname)

struct cs_pcmcia_softc;

static int cs_pcmcia_match(struct device *, struct cfdata *, void *);
static void cs_pcmcia_attach(struct device *, struct device *, void *);
static int cs_pcmcia_detach(struct device *, int);
static int cs_pcmcia_enable(struct cs_softc *);
static void cs_pcmcia_disable(struct cs_softc *);

struct cs_pcmcia_softc {
	struct cs_softc sc_cs; /* real "cs" softc */

	struct pcmcia_io_handle sc_pcioh;
	struct pcmcia_function *sc_pf;
	int sc_io_window;
	int sc_flags;
};

#define CS_PCMCIA_FLAGS_IO_ALLOCATED 1
#define CS_PCMCIA_FLAGS_IO_MAPPED 2

CFATTACH_DECL(cs_pcmcia, sizeof(struct cs_pcmcia_softc),
    cs_pcmcia_match, cs_pcmcia_attach, cs_pcmcia_detach, cs_activate);

static int
cs_pcmcia_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pcmcia_attach_args *pa = aux;

	if (pa->card->manufacturer == PCMCIA_VENDOR_IBM
		&& pa->card->product == PCMCIA_PRODUCT_IBM_ETHERJET)
		return 1;

	return 0;
}

static void
cs_pcmcia_attach(struct device *parent, struct device *self, void *aux)
{
	struct cs_pcmcia_softc *psc = (void *)self;
	struct cs_softc *sc = (void *)&psc->sc_cs;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;
	struct pcmcia_function *pf;

	aprint_normal("\n");

	pf = psc->sc_pf = pa->pf;

	cfe = SIMPLEQ_FIRST(&pa->pf->cfe_head);

	if (cfe->num_iospace != 1) {
		printf("%s: unexpected number of iospace(%d)\n",
			DEVNAME(sc), cfe->num_iospace);
		goto fail;
	}

	if (cfe->iospace[0].length < CS8900_IOSIZE) {
		printf("%s: unexpected iosize(%lu)\n",
			DEVNAME(sc), cfe->iospace[0].length);
		goto fail;
	}

	if (cfe->num_memspace != 0) {
		printf("%s: unexpected number of memspace(%d)\n",
			DEVNAME(sc), cfe->num_memspace);
		goto fail;
	}

	if (pcmcia_io_alloc(pf, cfe->iospace[0].start,
		cfe->iospace[0].length, cfe->iospace[0].length,
		&psc->sc_pcioh) != 0) {
		printf("%s: can't allocate i/o space %lx:%lx\n", DEVNAME(sc),
			cfe->iospace[0].start, cfe->iospace[0].length);
		goto fail;
	}
	psc->sc_flags |= CS_PCMCIA_FLAGS_IO_ALLOCATED;

	sc->sc_iot = psc->sc_pcioh.iot;
	sc->sc_ioh = psc->sc_pcioh.ioh;
	sc->sc_irq = -1;
#define CS_PCMCIA_HACK_FOR_CARDBUS
#ifdef CS_PCMCIA_HACK_FOR_CARDBUS
	/*
	 * XXX is there a generic way to know if it's a cardbus or not?
	 */
	sc->sc_cfgflags |= CFGFLG_CARDBUS_HACK;
#endif
	sc->sc_enable = cs_pcmcia_enable;
	sc->sc_disable = cs_pcmcia_disable;

	pcmcia_function_init(pa->pf, cfe);

	if (cs_pcmcia_enable(sc))
		goto fail;

	/* chip attach */
	if (cs_attach(sc, 0, 0, 0, 0))
		goto fail;

	cs_pcmcia_disable(sc);
	return;

fail:
	cs_pcmcia_detach((struct device *)psc, 0);
	return;
}

static int
cs_pcmcia_detach(struct device *self, int flags)
{
	struct cs_pcmcia_softc *psc = (void *)self;
	struct cs_softc *sc = &psc->sc_cs;
	struct pcmcia_function *pf = psc->sc_pf;
	int rv;

	rv = cs_detach(sc);
	if (rv)
		return rv;
	
	cs_pcmcia_disable(sc);

	if (psc->sc_flags & CS_PCMCIA_FLAGS_IO_ALLOCATED) {
		pcmcia_io_free(pf, &psc->sc_pcioh);
		psc->sc_flags &= ~CS_PCMCIA_FLAGS_IO_ALLOCATED;
	}

	return 0;
}

static int
cs_pcmcia_enable(struct cs_softc *sc)
{
	struct cs_pcmcia_softc *psc = (void *)sc;
	struct pcmcia_function *pf = psc->sc_pf;

	if (pcmcia_io_map(pf, PCMCIA_WIDTH_AUTO, &psc->sc_pcioh,
	    &psc->sc_io_window) != 0) {
		printf("%s: can't map i/o space\n", DEVNAME(sc));
		goto fail;
	}
	psc->sc_flags |= CS_PCMCIA_FLAGS_IO_MAPPED;

	sc->sc_ih = pcmcia_intr_establish(pf, IPL_NET, cs_intr, sc);
	if (sc->sc_ih == 0) {
		printf("%s: can't establish interrupt\n", DEVNAME(sc));
		goto fail;
	}

	if (pcmcia_function_enable(pf)) {
		printf("%s: can't enable function\n", DEVNAME(sc));
		pcmcia_intr_disestablish(pf, sc->sc_ih);
		goto fail;
	}

	return 0;

fail:
	return EIO;
}

static void
cs_pcmcia_disable(struct cs_softc *sc)
{
	struct cs_pcmcia_softc *psc = (void *)sc;
	struct pcmcia_function *pf = psc->sc_pf;

	pcmcia_function_disable(pf);
	
	if (sc->sc_ih != 0) {
		pcmcia_intr_disestablish(pf, sc->sc_ih);
		sc->sc_ih = 0;
	}

	if (psc->sc_flags & CS_PCMCIA_FLAGS_IO_MAPPED) {
		pcmcia_io_unmap(pf, psc->sc_io_window);
		psc->sc_flags &= ~CS_PCMCIA_FLAGS_IO_MAPPED;
	}
}
