/*	$NetBSD: esl_pcmcia.c,v 1.17.6.1 2007/02/27 14:16:38 ad Exp $	*/

/*
 * Copyright (c) 2000 Jared D. McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: esl_pcmcia.c,v 1.17.6.1 2007/02/27 14:16:38 ad Exp $");

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

#include <dev/isa/essreg.h>
#include <dev/pcmcia/eslvar.h>

static const struct pcmcia_product esl_pcmcia_products[] = {
	{ PCMCIA_VENDOR_INVALID, PCMCIA_PRODUCT_INVALID,
	  PCMCIA_CIS_EIGERLABS_EPX_AA2000 },
};
static const size_t esl_pcmcia_nproducts =
    sizeof(esl_pcmcia_products) / sizeof(esl_pcmcia_products[0]);

int	esl_pcmcia_match(struct device *, struct cfdata *, void *);
int	esl_pcmcia_validate_config(struct pcmcia_config_entry *);
void	esl_pcmcia_attach(struct device *, struct device *, void *);
int	esl_pcmcia_detach(struct device *, int);

int	esl_pcmcia_enable(struct esl_pcmcia_softc *);
void	esl_pcmcia_disable(struct esl_pcmcia_softc *);

CFATTACH_DECL(esl_pcmcia, sizeof(struct esl_pcmcia_softc),
    esl_pcmcia_match, esl_pcmcia_attach, esl_pcmcia_detach, NULL);

int
esl_pcmcia_match(struct device *parent, struct cfdata *match,
    void *aux)
{
	struct pcmcia_attach_args *pa;

	pa = aux;
	if (pcmcia_product_lookup(pa, esl_pcmcia_products, esl_pcmcia_nproducts,
	    sizeof(esl_pcmcia_products[0]), NULL))
		return 2;
	return 0;
}

int
esl_pcmcia_validate_config(struct pcmcia_config_entry *cfe)
{

	if (cfe->iftype != PCMCIA_IFTYPE_IO ||
	    cfe->num_memspace != 0 ||
	    cfe->num_iospace != 1)
		return EINVAL;
	return 0;
}

void
esl_pcmcia_attach(struct device *parent, struct device *self,
    void *aux)
{
	struct esl_pcmcia_softc *esc;
	struct pcmcia_attach_args *pa;
	struct pcmcia_config_entry *cfe;
	struct pcmcia_function *pf;
	int error;

	esc = (void *)self;
	mutex_init(&esc->sc_lock, MUTEX_DRIVER, IPL_AUDIO);

	pa = aux;
	pf = pa->pf;
	esc->sc_pf = pf;

	error = pcmcia_function_configure(pf, esl_pcmcia_validate_config);
	if (error) {
		aprint_error("%s: configure failed, error=%d\n", self->dv_xname,
		    error);
		return;
	}

	cfe = pf->cfe;
	esc->sc_iot = cfe->iospace[0].handle.iot;
	esc->sc_ioh = cfe->iospace[0].handle.ioh;

	error = esl_pcmcia_enable(esc);
	if (error)
		goto fail;

	/* Setup power management hooks */
	esc->sc_enable = esl_pcmcia_enable;
	esc->sc_disable = esl_pcmcia_disable;

	if (!esl_init(esc))
		aprint_error("%s: initialization failed\n", self->dv_xname);

	esl_pcmcia_disable(esc);
	esc->sc_state = ESL_PCMCIA_ATTACHED;
	return;

fail:
	pcmcia_function_unconfigure(pf);
}

int
esl_pcmcia_detach(struct device *self, int flags)
{
	struct esl_pcmcia_softc *esc;
	int rv;

	esc = (void *)self;

	if (esc->sc_state != ESL_PCMCIA_ATTACHED)
		return 0;

	if (esc->sc_opldev) {
		rv = config_detach(esc->sc_opldev, flags);
		if (rv)
			return rv;
	}
	if (esc->sc_audiodev) {
		rv = config_detach(esc->sc_audiodev, flags);
		if (rv)
			return rv;
	}

	pcmcia_function_unconfigure(esc->sc_pf);
	mutex_destroy(&esc->sc_lock);

	return 0;
}

int
esl_pcmcia_enable(struct esl_pcmcia_softc *sc)
{
	int error;

	sc->sc_ih = pcmcia_intr_establish(sc->sc_pf, IPL_AUDIO, esl_intr,
	    sc);
	if (!sc->sc_ih)
		return EIO;

	error = pcmcia_function_enable(sc->sc_pf);
	if (error) {
		pcmcia_intr_disestablish(sc->sc_pf, sc->sc_ih);
		sc->sc_ih = 0;
	}

	return error;
}

void
esl_pcmcia_disable(struct esl_pcmcia_softc *sc)
{

	pcmcia_function_disable(sc->sc_pf);
	pcmcia_intr_disestablish(sc->sc_pf, sc->sc_ih);
	sc->sc_ih = 0;
}
