/*	$NetBSD: com_pcmcia.c,v 1.51.16.3 2006/06/17 02:37:30 gdamore Exp $	 */

/*-
 * Copyright (c) 1998, 2004 The NetBSD Foundation, Inc.
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

/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)com.c	7.5 (Berkeley) 5/16/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: com_pcmcia.c,v 1.51.16.3 2006/06/17 02:37:30 gdamore Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciadevs.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <dev/isa/isareg.h>

int com_pcmcia_match(struct device *, struct cfdata *, void *);
int com_pcmcia_validate_config(struct pcmcia_config_entry *);
void com_pcmcia_attach(struct device *, struct device *, void *);
int com_pcmcia_detach(struct device *, int);
void com_pcmcia_cleanup(void *);

int com_pcmcia_enable(struct com_softc *);
void com_pcmcia_disable(struct com_softc *);

struct com_pcmcia_softc {
	struct com_softc sc_com;		/* real "com" softc */

	/* PCMCIA-specific goo */
	struct pcmcia_function *sc_pf;		/* our PCMCIA function */
	void *sc_ih;				/* interrupt handler */
	int sc_attached;
};

CFATTACH_DECL(com_pcmcia, sizeof(struct com_pcmcia_softc),
    com_pcmcia_match, com_pcmcia_attach, com_pcmcia_detach, com_activate);

static const struct pcmcia_product com_pcmcia_products[] = {
	{ PCMCIA_VENDOR_INVALID, PCMCIA_PRODUCT_INVALID,
	  PCMCIA_CIS_MEGAHERTZ_XJ2288 },
};
static const size_t com_pcmcia_nproducts =
    sizeof(com_pcmcia_products) / sizeof(com_pcmcia_products[0]);

int
com_pcmcia_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	int comportmask;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;

	/* 1. Does it claim to be a serial device? */
	if (pa->pf->function == PCMCIA_FUNCTION_SERIAL)
		return 1;

	/* 2. Does it have all four 'standard' port ranges? */
	comportmask = 0;
	SIMPLEQ_FOREACH(cfe, &pa->pf->cfe_head, cfe_list) {
		switch (cfe->iospace[0].start) {
		case IO_COM1:
			comportmask |= 1;
			break;
		case IO_COM2:
			comportmask |= 2;
			break;
		case IO_COM3:
			comportmask |= 4;
			break;
		case IO_COM4:
			comportmask |= 8;
			break;
		}
	}

	if (comportmask == 15)
		return 1;

	/* 3. Is this a card we know about? */
	if (pcmcia_product_lookup(pa, com_pcmcia_products, com_pcmcia_nproducts,
	    sizeof(com_pcmcia_products[0]), NULL))
		return 1;

	return 0;
}

int
com_pcmcia_validate_config(cfe)
	struct pcmcia_config_entry *cfe;
{
	if (cfe->iftype != PCMCIA_IFTYPE_IO ||
	    cfe->num_iospace != 1)
		return (EINVAL);
	/* Some cards have a memory space, but we don't use it. */
	cfe->num_memspace = 0;
	return (0);
}

void
com_pcmcia_attach(parent, self, aux)
	struct device  *parent, *self;
	void *aux;
{
	struct com_pcmcia_softc *psc = (void *) self;
	struct com_softc *sc = &psc->sc_com;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;
	int error;

	psc->sc_pf = pa->pf;

	error = pcmcia_function_configure(pa->pf, com_pcmcia_validate_config);
	if (error) {
		aprint_error("%s: configure failed, error=%d\n", self->dv_xname,
		    error);
		return;
	}

	cfe = pa->pf->cfe;
	sc->sc_iot = cfe->iospace[0].handle.iot;
	sc->sc_ioh = cfe->iospace[0].handle.ioh;

	error = com_pcmcia_enable(sc);
	if (error)
		goto fail;

	sc->enabled = 1;

	sc->sc_iobase = -1;
	sc->sc_frequency = COM_FREQ;

	sc->enable = com_pcmcia_enable;
	sc->disable = com_pcmcia_disable;

	aprint_normal("%s", self->dv_xname);

	com_attach_subr(sc);

	sc->enabled = 0;

	psc->sc_attached = 1;
	com_pcmcia_disable(sc);
	return;

fail:
	pcmcia_function_unconfigure(pa->pf);
}

int
com_pcmcia_detach(self, flags)
	struct device  *self;
	int flags;
{
	struct com_pcmcia_softc *psc = (struct com_pcmcia_softc *) self;
	int error;

	if (!psc->sc_attached)
		return (0);

	if ((error = com_detach(self, flags)) != 0)
		return error;

	pcmcia_function_unconfigure(psc->sc_pf);
	return (0);
}

int
com_pcmcia_enable(sc)
	struct com_softc *sc;
{
	struct com_pcmcia_softc *psc = (struct com_pcmcia_softc *) sc;
	struct pcmcia_function *pf = psc->sc_pf;
	int error;

	/* establish the interrupt. */
	psc->sc_ih = pcmcia_intr_establish(pf, IPL_SERIAL, comintr, sc);
	if (psc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n",
		    sc->sc_dev.dv_xname);
		return (1);
	}

	if ((error = pcmcia_function_enable(pf)) != 0) {
		pcmcia_intr_disestablish(pf, psc->sc_ih);
		psc->sc_ih = 0;
		return (error);
	}

	if ((psc->sc_pf->sc->card.product == PCMCIA_PRODUCT_3COM_3C562) ||
	    (psc->sc_pf->sc->card.product == PCMCIA_PRODUCT_3COM_3CXEM556) ||
	    (psc->sc_pf->sc->card.product == PCMCIA_PRODUCT_3COM_3CXEM556INT)) {
		int reg;

		/* turn off the ethernet-disable bit */
		reg = pcmcia_ccr_read(pf, PCMCIA_CCR_OPTION);
		if (reg & 0x08) {
			reg &= ~0x08;
			pcmcia_ccr_write(pf, PCMCIA_CCR_OPTION, reg);
		}
	}

	return (0);
}

void
com_pcmcia_disable(sc)
	struct com_softc *sc;
{
	struct com_pcmcia_softc *psc = (struct com_pcmcia_softc *) sc;

	pcmcia_function_disable(psc->sc_pf);
	pcmcia_intr_disestablish(psc->sc_pf, psc->sc_ih);
	psc->sc_ih = 0;
}
