/*	$NetBSD: esl_pcmcia.c,v 1.1 2001/09/29 14:00:57 augustss Exp $	*/

/*
 * Copyright (c) 2000 Jared D. McNeill <jmcneill@invisible.yi.org>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Jared D. McNeill.
 * 4. Neither the name of the author nor the names of any contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/audioio.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/audio_if.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciadevs.h>

#include <dev/pcmcia/eslreg.h>
#include <dev/pcmcia/eslvar.h>

static const struct esl_pcmcia_product {
	char *name;
	int32_t manufacturer;
	int32_t product;
	char *cis_info[4];
	int function;
} esl_pcmcia_products[] = {
	{ PCMCIA_STR_EIGERLABS_EPX_AA2000,
	  PCMCIA_VENDOR_INVALID, PCMCIA_PRODUCT_INVALID,
	  PCMCIA_CIS_EIGERLABS_EPX_AA2000, 0 },

	{ NULL }
};

int	esl_pcmcia_match __P((struct device *, struct cfdata *, void *)); 
void	esl_pcmcia_attach __P((struct device *, struct device *, void *));  
int	esl_pcmcia_detach __P((struct device *, int));

int	esl_pcmcia_enable __P((struct esl_pcmcia_softc *));
void	esl_pcmcia_disable __P((struct esl_pcmcia_softc *));

struct cfattach esl_pcmcia_ca = {
	sizeof(struct esl_pcmcia_softc), esl_pcmcia_match, esl_pcmcia_attach,
	esl_pcmcia_detach
};

#define ESL_NDEVS (sizeof(esl_pcmcia_products) / sizeof(esl_pcmcia_products[0]))

#define esl_pcmcia_product_lookup(card, fct, n) \
	(((card)->manufacturer != PCMCIA_VENDOR_INVALID) && \
	 ((card)->product != PCMCIA_PRODUCT_INVALID) && \
	 (esl_pcmcia_products[(n)].cis_info[0]) && \
	 (esl_pcmcia_products[(n)].cis_info[1]) && \
	 (strcmp((card)->cis1_info[0], esl_pcmcia_products[(n)].cis_info[0]) \
	    == 0) && \
	 (strcmp((card)->cis1_info[1], esl_pcmcia_products[(n)].cis_info[1]) \
	    == 0) && \
	 ((fct) == esl_pcmcia_products[(n)].function) ? \
	 &esl_pcmcia_products[(n)] : NULL)

int
esl_pcmcia_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pcmcia_attach_args *pa = aux;
	int i;

	for (i = 0; i < ESL_NDEVS; i++)
		if (esl_pcmcia_product_lookup(pa->card, pa->pf->number, i))
			return(2); 

	return (0);
}

void
esl_pcmcia_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct esl_pcmcia_softc *esc = (void *)self;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;
	struct pcmcia_function *pf = pa->pf;
	const struct esl_pcmcia_product *pp;
	int i;

	esc->sc_pf = pf;

	for (cfe = SIMPLEQ_FIRST(&pf->cfe_head); cfe != NULL;
	    cfe = SIMPLEQ_NEXT(cfe, cfe_list)) {
		if (cfe->num_memspace != 0 ||
		    cfe->num_iospace != 1)
			continue;

		if (pcmcia_io_alloc(pa->pf, cfe->iospace[0].start,
		    cfe->iospace[0].length, 0, &esc->sc_pcioh) == 0)
			break;
	}

	if (cfe == 0) {
		printf(": can't alloc i/o space\n");
		goto no_config_entry;
	}

	/* Setup power management hooks */
	esc->sc_enable = esl_pcmcia_enable;
	esc->sc_disable = esl_pcmcia_disable;

	/* Enable the card. */
	pcmcia_function_init(pf, cfe);
	if (pcmcia_function_enable(pf)) {
		printf(": function enable failed\n");
		goto enable_failed;
	}

	/* Map in the I/O space */
	if (pcmcia_io_map(pa->pf, PCMCIA_WIDTH_AUTO, 0, esc->sc_pcioh.size,
	    &esc->sc_pcioh, &esc->sc_io_window)) {
		printf(": can't map i/o space\n");
		goto iomap_failed;
	}

	pp = NULL;
	for (i = 0; i < ESL_NDEVS; i++) {
		pp = esl_pcmcia_product_lookup(pa->card, pa->pf->number, i);
		if (pp != NULL)
			break;
	}

	if (pp == NULL) {
		printf("\n");
		panic("esl_pcmcia_attach: impossible");
	}

	printf(": %s\n", pp->name);

	if (esl_init(esc)) {
		printf("esl_init: failed\n");
		goto init_failed;
	}

	pcmcia_function_disable(esc->sc_pf);
	return;

init_failed:
	/* Unmap I/O space */
	pcmcia_io_unmap(esc->sc_pf, esc->sc_io_window);

iomap_failed:
	/* Disable the device. */
	pcmcia_function_disable(esc->sc_pf);

enable_failed:
	/* Free our I/O space. */
	pcmcia_io_free(esc->sc_pf, &esc->sc_pcioh);

no_config_entry:
	return;
}

int
esl_pcmcia_detach(struct device *self, int flags)
{
	struct esl_pcmcia_softc *esc = (void *)self;
	int rv = 0;

	if (esc->sc_io_window == -1)
		/* Nothing to detach */
		return(0);

	if (esc->sc_opldev != NULL)
		config_detach(esc->sc_opldev, flags);
	if (esc->sc_audiodev != NULL)
		rv = config_detach(esc->sc_audiodev, flags);
	if (rv)
		return(rv);

	/* unmap i/o window and i/o space */
	pcmcia_io_unmap(esc->sc_pf, esc->sc_io_window);
	pcmcia_io_free(esc->sc_pf, &esc->sc_pcioh);

	return (rv);
}

int
esl_pcmcia_enable(struct esl_pcmcia_softc *sc)
{

	/* Establish an interrupt */
	sc->sc_ih = pcmcia_intr_establish(sc->sc_pf, IPL_AUDIO, esl_intr,
	    sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n",
		    sc->sc_esl.sc_dev.dv_xname);
		goto fail_1;
	}

	if (pcmcia_function_enable(sc->sc_pf))
		goto fail_2;

	return(0);

fail_2:
	pcmcia_intr_disestablish(sc->sc_pf, sc->sc_ih);
fail_1:
	return(1);
}

void
esl_pcmcia_disable(struct esl_pcmcia_softc *sc)
{

	pcmcia_function_disable(sc->sc_pf);
	pcmcia_intr_disestablish(sc->sc_pf, sc->sc_ih);

	return;
}
