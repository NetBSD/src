/*	$NetBSD: dma_sbus.c,v 1.11.2.1 2002/03/26 17:24:58 eeh Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
 * Copyright (c) 1994 Peter Galbavy.  All rights reserved.
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
 *	This product includes software developed by Peter Galbavy.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dma_sbus.c,v 1.11.2.1 2002/03/26 17:24:58 eeh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/properties.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/autoconf.h>
#include <machine/openfirm.h>

#include <dev/sbus/sbusvar.h>

#include <dev/ic/lsi64854reg.h>
#include <dev/ic/lsi64854var.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/ncr53c9xreg.h>
#include <dev/ic/ncr53c9xvar.h>

struct dma_softc {
	struct lsi64854_softc	sc_lsi64854;	/* base device */
	struct sbusdev	sc_sd;			/* sbus device */
};

int	dmamatch_sbus	__P((struct device *, struct cfdata *, void *));
void	dmaattach_sbus	__P((struct device *, struct device *, void *));

int	dmaprint_sbus	__P((void *, const char *));

void	*dmabus_intr_establish __P((
		bus_space_tag_t,
		int,			/*bus interrupt priority*/
		int,			/*`device class' level*/
		int,			/*flags*/
		int (*) __P((void *)),	/*handler*/
		void *));		/*handler arg*/

static	bus_space_tag_t dma_alloc_bustag __P((struct dma_softc *sc));

DEV_CFA_DECL(dma_sbus_ca, sizeof(struct dma_softc), 
	dmamatch_sbus, dmaattach_sbus);
DEV_CFA_DECL(ledma_ca, sizeof(struct dma_softc), dmamatch_sbus, dmaattach_sbus);

int
dmaprint_sbus(aux, busname)
	void *aux;
	const char *busname;
{
	struct sbus_attach_args *sa = aux;
	bus_space_tag_t t = sa->sa_bustag;
	struct dma_softc *sc = t->cookie;

	sa->sa_bustag = sc->sc_lsi64854.sc_bustag;	/* XXX */
	sbus_print(aux, busname);	/* XXX */
	sa->sa_bustag = t;		/* XXX */
	return (UNCONF);
}

int
dmamatch_sbus(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct device *dev = (struct device *)aux;
	char name[30];
	char cdname[30];

	if (dev_getprop(dev, "name", name, sizeof(name), NULL, 0) == -1)
		return (0);

	if (strcmp("espdma", name) == 0) return (1);

	if (dev_getprop(dev, "cd-name", cdname, sizeof(cdname), NULL, 0) == -1)
		return (0);

	if (strcmp(cdname, name) == 0) return (1);
	return (0);
}

void
dmaattach_sbus(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct dma_softc *dsc = (struct dma_softc *)DEV_PRIVATE(self);
	struct lsi64854_softc *sc = &dsc->sc_lsi64854;
	struct sbus_reg reg[2];
	bus_space_tag_t sbt;
	int sbusburst, burst;
	int promaddr[2], npromaddr, nreg;
	int node;

	if (dev_getprop(self, "node", &node, sizeof(node), NULL, 0) != 
		sizeof(node)) 
		panic("dmaattach_sbus: no \"node\" property");

	dev_getprop(parent, "bus-tag", &sc->sc_bustag, sizeof(sc->sc_bustag),
		NULL, 1);
	dev_getprop(parent, "dma-tag", &sc->sc_dmatag, sizeof(sc->sc_dmatag),
		NULL, 1);

	npromaddr = dev_getprop(self, "address", &promaddr,
		sizeof(promaddr), NULL, 0);
	if (npromaddr > 0)
		npromaddr /= sizeof(promaddr[0]);

	nreg = dev_getprop(self, "reg", &reg, sizeof(reg), NULL, 0);
	if (nreg > 0) nreg /= sizeof(reg[0]);

	/* Map registers */
	if (npromaddr > 0) {
		sbus_promaddr_to_handle(sc->sc_bustag, promaddr[0],
			&sc->sc_regs);
	} else {
		if (sbus_bus_map(sc->sc_bustag, reg[0].sbr_slot,
			reg[0].sbr_offset, reg[0].sbr_size,
			0, &sc->sc_regs) != 0) {
			printf("%s: cannot map dma registers\n",
				self->dv_xname);
			return;
		}
	}

	/*
	 * Get transfer burst size from PROM and plug it into the
	 * controller registers. This is needed on the Sun4m; do
	 * others need it too?
	 */
	if (dev_getprop(parent, "burst-sizes", &sbusburst, sizeof(sbusburst), 
		NULL, 0) == -1)
		sbusburst = SBUS_BURST_32 - 1; /* 1->16 */

	if (dev_getprop(self, "burst-sizes", &burst, sizeof(burst), 
		NULL, 0) == -1)
		burst = -1;

	if (burst == -1)
		/* take SBus burst sizes */
		burst = sbusburst;

	/* Clamp at parent's burst sizes */
	burst &= sbusburst;
	sc->sc_burst = (burst & SBUS_BURST_32) ? 32 :
		       (burst & SBUS_BURST_16) ? 16 : 0;

	if (self->dv_cfdata->cf_attach == &ledma_ca) {
		char cabletype[10];
		u_int32_t csr;
		/*
		 * Check to see which cable type is currently active and
		 * set the appropriate bit in the ledma csr so that it
		 * gets used. If we didn't netboot, the PROM won't have
		 * the "cable-selection" property; default to TP and then
		 * the user can change it via a "media" option to ifconfig.
		 */
		dev_getprop(self, "cable-selection", cabletype,
			sizeof(cabletype), NULL, 0);
		csr = L64854_GCSR(sc);
		if (strcmp(cabletype, "tpe") == 0) {
			csr |= E_TP_AUI;
		} else if (strcmp(cabletype, "aui") == 0) {
			csr &= ~E_TP_AUI;
		} else {
			/* assume TP if nothing there */
			csr |= E_TP_AUI;
		}
		L64854_SCSR(sc, csr);
		delay(20000);	/* manual says we need a 20ms delay */
		sc->sc_channel = L64854_CHANNEL_ENET;
	} else {
		sc->sc_channel = L64854_CHANNEL_SCSI;
	}

	sbus_establish(&dsc->sc_sd, self);
	sbt = dma_alloc_bustag(dsc);
	dev_setprop(self, "bus-tag", &sbt, sizeof(sbt),	
		PROP_ARRAY|sizeof(sbt), 0);
	lsi64854_attach(sc);

	/* Attach children */
	for (node = firstchild(node); node; node = nextsibling(node)) {
		struct sbus_attach_args sa;
		struct device *child;

		if ((child = sbus_setup_attach_args(parent, sbt, sc->sc_dmatag,
			node, &sa)) == 0)  {
			char name[32];

			OF_getprop(node, "name", name, sizeof(name));
			name[sizeof(name) -1] = 0;
			printf("dma_attach: %s: incomplete\n", name);
			continue;
		}
		(void) config_found_sad(self, (void *)&sa, dmaprint_sbus,
			NULL, child);
		sbus_destroy_attach_args(&sa);
	}
}

void *
dmabus_intr_establish(t, pri, level, flags, handler, arg)
	bus_space_tag_t t;
	int pri;
	int level;
	int flags;
	int (*handler) __P((void *));
	void *arg;
{
	struct lsi64854_softc *sc = t->cookie;

	/* XXX - for now only le; do esp later */
	if (sc->sc_channel == L64854_CHANNEL_ENET) {
		sc->sc_intrchain = handler;
		sc->sc_intrchainarg = arg;
		handler = lsi64854_enet_intr;
		arg = sc;
	}
	return (bus_intr_establish(sc->sc_bustag, pri, level, flags,
				   handler, arg));
}

bus_space_tag_t
dma_alloc_bustag(sc)
	struct dma_softc *sc;
{
	bus_space_tag_t sbt;

	sbt = (bus_space_tag_t) malloc(sizeof(struct sparc_bus_space_tag),
	    M_DEVBUF, M_NOWAIT|M_ZERO);
	if (sbt == NULL)
		return (NULL);

	sbt->cookie = sc;
	sbt->parent = sc->sc_lsi64854.sc_bustag;
	sbt->sparc_intr_establish = dmabus_intr_establish;
	return (sbt);
}
