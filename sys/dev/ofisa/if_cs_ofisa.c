/*	$NetBSD: if_cs_ofisa.c,v 1.7 2002/06/08 17:07:54 yamt Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_cs_ofisa.c,v 1.7 2002/06/08 17:07:54 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include "rnd.h"
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ofw/openfirm.h>
#include <dev/isa/isavar.h>
#include <dev/ofisa/ofisavar.h>

#include <dev/ic/cs89x0reg.h>
#include <dev/ic/cs89x0var.h>
#include <dev/isa/cs89x0isavar.h>

int	cs_ofisa_match __P((struct device *, struct cfdata *, void *));
void	cs_ofisa_attach __P((struct device *, struct device *, void *));

struct cfattach cs_ofisa_ca = {
	sizeof(struct cs_softc_isa), cs_ofisa_match, cs_ofisa_attach
};

int
cs_ofisa_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct ofisa_attach_args *aa = aux;
	static const char *const compatible_strings[] = {
		"CRUS,CS8900",
		/* XXX CS8920, CS8920M? */
		/* XXX PNP names? */
		NULL,
	};
	int rv = 0;

	if (of_compatible(aa->oba.oba_phandle, compatible_strings) != -1)
		rv = 5;
#ifdef _CS_OFISA_MD_MATCH
	if (rv == 0)
		rv = cs_ofisa_md_match(parent, cf, aux);
#endif
	return (rv);
}

void
cs_ofisa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct cs_softc *sc = (struct cs_softc *) self;
	struct cs_softc_isa *isc = (void *)sc;
	struct ofisa_attach_args *aa = aux;
	struct ofisa_reg_desc reg[2];
	struct ofisa_intr_desc intr;
	struct ofisa_dma_desc dma;
	int i, n, *media, nmedia, defmedia;
	bus_addr_t io_addr, mem_addr;
	char *model = NULL;
	const char *message = NULL;
	u_int8_t enaddr[6];

	isc->sc_ic = aa->ic;
	sc->sc_iot = aa->iot;
	sc->sc_memt = aa->memt;

	/*
	 * We're living on an OFW.  We have to ask the OFW what our
	 * registers and interrupts properties look like.
	 *
	 * We expect:
	 *
	 *	1 i/o register region
	 *	0 or 1 memory region
	 *	1 interrupt
	 *	0 or 1 dma channel
	 */

	io_addr = mem_addr = -1;

	n = ofisa_reg_get(aa->oba.oba_phandle, reg, 2);
#ifdef _CS_OFISA_MD_REG_FIXUP
	n = cs_ofisa_md_reg_fixup(parent, self, aux, reg, 2, n);
#endif
	if (n < 1 || n > 2) {
		printf(": error getting register data\n");
		return;
	}

	for (i = 0; i < n; i++) {
		if (reg[i].type == OFISA_REG_TYPE_IO) {
			if (io_addr != (bus_addr_t) -1) {
				printf(": multiple I/O regions\n");
				return;
			}
			if (reg[i].len != CS8900_IOSIZE) {
				printf(": weird register size (%lu, expected %d)\n",
				    (unsigned long)reg[i].len, CS8900_IOSIZE);
				return;
			}
			io_addr = reg[i].addr;
		} else {
			if (mem_addr != (bus_addr_t) -1) {
				printf(": multiple memory regions\n");
				return;
			}
			if (reg[i].len != CS8900_MEMSIZE) {
				printf(": weird register size (%lu, expected %d)\n",
				    (unsigned long)reg[i].len, CS8900_MEMSIZE);
				return;
			}
			mem_addr = reg[i].addr;
		}
	}

	n = ofisa_intr_get(aa->oba.oba_phandle, &intr, 1);
#ifdef _CS_OFISA_MD_INTR_FIXUP
	n = cs_ofisa_md_intr_fixup(parent, self, aux, &intr, 1, n);
#endif
	if (n != 1) {
		printf(": error getting interrupt data\n");
		return;
	}
	sc->sc_irq = intr.irq;

	if (CS8900_IRQ_ISVALID(sc->sc_irq) == 0) {
		printf(": invalid IRQ %d\n", sc->sc_irq);
		return;
	}

	isc->sc_drq = ISACF_DRQ_DEFAULT;
	n = ofisa_dma_get(aa->oba.oba_phandle, &dma, 1);
#ifdef _CS_OFISA_MD_DMA_FIXUP
	n = cs_ofisa_md_dma_fixup(parent, self, aux, &dma, 1, n);
#endif
	if (n == 1)
		isc->sc_drq = dma.drq;

	if (io_addr == (bus_addr_t) -1) {
		printf(": no I/O space\n");
		return;
	}
	if (bus_space_map(sc->sc_iot, io_addr, CS8900_IOSIZE, 0,
	    &sc->sc_ioh)) {
		printf(": unable to map register space\n");
		return;
	}

	if (mem_addr != (bus_addr_t) -1) {
		if (bus_space_map(sc->sc_memt, mem_addr, CS8900_MEMSIZE, 0,
		    &sc->sc_memh)) {
			message = "unable to map memory space";
		} else {
			sc->sc_cfgflags |= CFGFLG_MEM_MODE;
			sc->sc_pktpgaddr = mem_addr;
		}
	}

	/* Dig MAC address out of the firmware. */
	if (OF_getprop(aa->oba.oba_phandle, "mac-address", enaddr,
	    sizeof(enaddr)) < 0) {
		printf(": unable to get Ethernet address\n");
		return;
	}

	/* Dig media out of the firmware. */
	media = of_network_decode_media(aa->oba.oba_phandle, &nmedia,
	    &defmedia);
#ifdef _CS_OFISA_MD_MEDIA_FIXUP
	media = cs_ofisa_md_media_fixup(parent, self, aux, media, &nmedia,
	    &defmedia);
#endif
	if (media == NULL) {
		printf(": unable to get media information\n");
		return;
	}

	n = OF_getproplen(aa->oba.oba_phandle, "model");
	if (n > 0) {
		model = alloca(n);
		if (OF_getprop(aa->oba.oba_phandle, "model", model, n) != n)
			model = NULL;	/* Safe; alloca is on-stack */
	}
	if (model != NULL)
		printf(": %s\n", model);
	else
		printf("\n");

	if (message != NULL)
		printf("%s: %s\n", sc->sc_dev.dv_xname, message);

	if (defmedia == -1) {
		printf("%s: unable to get default media\n",
		    sc->sc_dev.dv_xname);
		defmedia = media[0];	/* XXX What to do? */
	}

	sc->sc_ih = isa_intr_establish(isc->sc_ic, sc->sc_irq, intr.share,
	    IPL_NET, cs_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: unable to establish interrupt\n",
		    sc->sc_dev.dv_xname);
		return;
	}

#ifdef _CS_OFISA_MD_CFGFLAGS_FIXUP
	sc->sc_cfgflags |= cs_ofisa_md_cfgflags_fixup(parent, self, aux);
#endif

	sc->sc_dma_chipinit = cs_isa_dma_chipinit;
	sc->sc_dma_attach = cs_isa_dma_attach;
	sc->sc_dma_process_rx = cs_process_rx_dma;

	cs_attach(sc, enaddr, media, nmedia, defmedia);

	/* This is malloc'd. */
	free(media, M_DEVBUF);
}
