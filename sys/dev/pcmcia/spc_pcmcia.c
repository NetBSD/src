/*	$NetBSD: spc_pcmcia.c,v 1.2 2004/08/08 23:17:13 mycroft Exp $	*/

/*-
 * Copyright (c) 2000, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: spc_pcmcia.c,v 1.2 2004/08/08 23:17:13 mycroft Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/buf.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciadevs.h>

#include <dev/ic/mb89352var.h>

struct spc_pcmcia_softc {
	struct spc_softc	sc_spc;		/* glue to MI code */

	/* PCMCIA-specific goo. */
	struct pcmcia_io_handle sc_pcioh;	/* PCMCIA i/o space info */
	int sc_io_window;			/* our i/o window */
	struct pcmcia_function *sc_pf;		/* our PCMCIA function */
	void *sc_ih;				/* interrupt handler */
	int sc_flags;
#define	SPC_PCMCIA_ATTACHED	1		/* attach completed */
#define SPC_PCMCIA_ATTACHING	2		/* attach in progress */
};

int	spc_pcmcia_match __P((struct device *, struct cfdata *, void *)); 
void	spc_pcmcia_attach __P((struct device *, struct device *, void *));  
int	spc_pcmcia_detach __P((struct device *, int));
int	spc_pcmcia_enable __P((struct device *, int));

CFATTACH_DECL(spc_pcmcia, sizeof(struct spc_pcmcia_softc),
    spc_pcmcia_match, spc_pcmcia_attach, spc_pcmcia_detach, NULL);

static const struct spc_pcmcia_product *
    spc_pcmcia_lookup __P((struct pcmcia_attach_args *));

const struct spc_pcmcia_product {
	u_int32_t	epp_vendor;		/* vendor ID */
	u_int32_t	epp_product;		/* product ID */
	const char	*epp_cisinfo[4];	/* CIS information */
} spc_pcmcia_products[] = {
	{ PCMCIA_VENDOR_FUJITSU, PCMCIA_PRODUCT_FUJITSU_SCSI600,
	  PCMCIA_CIS_FUJITSU_SCSI600 },
};

static const struct spc_pcmcia_product *
spc_pcmcia_lookup(pa)
	struct pcmcia_attach_args *pa;
{
	const struct spc_pcmcia_product *epp;
	int n;

	for (epp = spc_pcmcia_products,
	    n = sizeof(spc_pcmcia_products) / sizeof(spc_pcmcia_products[0]);
	    n; epp++, n--) {
		/* match by CIS information */
		if (pa->card->cis1_info[0] != NULL &&
		    epp->epp_cisinfo[0] != NULL &&
		    strcmp(pa->card->cis1_info[0], epp->epp_cisinfo[0]) == 0 &&
		    pa->card->cis1_info[1] != NULL &&
		    epp->epp_cisinfo[1] != NULL &&
		    strcmp(pa->card->cis1_info[1], epp->epp_cisinfo[1]) == 0)
			return (epp);

		/* match by vendor/product id */
		if (pa->manufacturer != PCMCIA_VENDOR_INVALID &&
		    pa->manufacturer == epp->epp_vendor &&
		    pa->product != PCMCIA_PRODUCT_INVALID &&
		    pa->product == epp->epp_product)
			return (epp);
	}

	return (NULL);
}

int
spc_pcmcia_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pcmcia_attach_args *pa = aux;

	if (spc_pcmcia_lookup(pa) != NULL)
		return (1);
	return (0);
}

void
spc_pcmcia_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct spc_pcmcia_softc *sc = (void *)self;
	struct spc_softc *spc = (void *)self;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;
	struct pcmcia_function *pf = pa->pf;
	const struct spc_pcmcia_product *epp;

	aprint_normal("\n");

	SIMPLEQ_FOREACH(cfe, &pf->cfe_head, cfe_list) {
		if (cfe->num_memspace != 0 ||
		    cfe->num_iospace != 1)
			continue;

		if (pcmcia_io_alloc(pf, cfe->iospace[0].start,
		    cfe->iospace[0].length, 0, &sc->sc_pcioh) == 0)
			break;
	}

	if (cfe == 0) {
		aprint_error("%s: can't alloc i/o space\n", self->dv_xname);
		goto no_config_entry;
	}

	sc->sc_pf = pf;

	/* Enable the card. */
	pcmcia_function_init(pf, cfe);
	if (pcmcia_function_enable(pf)) {
		printf("%s: function enable failed\n", self->dv_xname);
		goto enable_failed;
	}

	/* Map in the I/O space */
	if (pcmcia_io_map(pf, PCMCIA_WIDTH_AUTO, &sc->sc_pcioh,
	    &sc->sc_io_window)) {
		printf("%s: can't map i/o space\n", self->dv_xname);
		goto iomap_failed;
	}

	epp = spc_pcmcia_lookup(pa);
	if (epp == NULL)
		panic("spc_pcmcia_attach: impossible");

	spc->sc_iot = sc->sc_pcioh.iot;
	spc->sc_ioh = sc->sc_pcioh.ioh;
	spc->sc_initiator = 7; /* XXX */

	/*
	 *  Initialize nca board itself.
	 */
	sc->sc_flags |= SPC_PCMCIA_ATTACHING;

#if 1
	if (spc_pcmcia_enable(self, 1))
		aprint_error("%s: enable failed\n", self->dv_xname);
	else
#endif
		spc_attach(spc);

	sc->sc_flags &= ~SPC_PCMCIA_ATTACHING;
	sc->sc_flags |= SPC_PCMCIA_ATTACHED;
	return;

iomap_failed:
	/* Disable the device. */
	pcmcia_function_disable(pf);

enable_failed:
	/* Unmap our I/O space. */
	pcmcia_io_free(pf, &sc->sc_pcioh);

no_config_entry:
	return;
}

int
spc_pcmcia_detach(self, flags)
	struct device *self;
	int flags;
{
	struct spc_pcmcia_softc *sc = (void *)self;
#if 0
	int error;
#endif

	if ((sc->sc_flags & SPC_PCMCIA_ATTACHED) == 0) {
		/* Nothing to detach. */
		return (0);
	}

#if 0
	error = spc_detach(&sc->sc_spc, flags);
	if (error)
		return (error);
#else
	panic("spc_pcmcia_detach");
#endif

	/* Unmap our i/o window and i/o space. */
	pcmcia_io_unmap(sc->sc_pf, sc->sc_io_window);
	pcmcia_io_free(sc->sc_pf, &sc->sc_pcioh);

	return (0);
}

int
spc_pcmcia_enable(arg, onoff)
	struct device *arg;
	int onoff;
{
	struct spc_pcmcia_softc *sc = (void *) arg;

	if (onoff) {
		/* Establish the interrupt handler. */
		sc->sc_ih = pcmcia_intr_establish(sc->sc_pf, IPL_BIO,
		    spc_intr, &sc->sc_spc);
		if (sc->sc_ih == NULL) {
			printf("%s: couldn't establish interrupt handler\n",
			    sc->sc_spc.sc_dev.dv_xname);
			return (EIO);
		}

		/*
		 * If attach is in progress, we know that card power is
		 * enabled and chip will be initialized later.
		 * Otherwise, enable and reset now.
		 */
		if ((sc->sc_flags & SPC_PCMCIA_ATTACHING) == 0) {
			if (pcmcia_function_enable(sc->sc_pf)) {
				printf("%s: couldn't enable PCMCIA function\n",
				    sc->sc_spc.sc_dev.dv_xname);
				pcmcia_intr_disestablish(sc->sc_pf, sc->sc_ih);
				return (EIO);
			}

			/* Initialize only chip.  */
			spc_init(&sc->sc_spc);
		}
	} else {
		pcmcia_function_disable(sc->sc_pf);
		pcmcia_intr_disestablish(sc->sc_pf, sc->sc_ih);
	}

	return (0);
}
