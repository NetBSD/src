/*	$NetBSD: com_pcmcia.c,v 1.28 2002/04/13 17:06:53 christos Exp $	 */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
__KERNEL_RCSID(0, "$NetBSD: com_pcmcia.c,v 1.28 2002/04/13 17:06:53 christos Exp $");

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

struct com_dev {
	char	*name;
	char	*cis1_info[4];
};

/* Devices that we need to match by CIS strings */
static struct com_dev com_devs[] = {
	{ PCMCIA_STR_MEGAHERTZ_XJ2288, PCMCIA_CIS_MEGAHERTZ_XJ2288 },
};


static int com_devs_size = sizeof(com_devs) / sizeof(com_devs[0]);
static struct com_dev *com_dev_match __P((struct pcmcia_card *));

int com_pcmcia_match __P((struct device *, struct cfdata *, void *));
void com_pcmcia_attach __P((struct device *, struct device *, void *));
int com_pcmcia_detach __P((struct device *, int));
void com_pcmcia_cleanup __P((void *));

int com_pcmcia_enable __P((struct com_softc *));
void com_pcmcia_disable __P((struct com_softc *));
int com_pcmcia_enable1 __P((struct com_softc *));
void com_pcmcia_disable1 __P((struct com_softc *));

struct com_pcmcia_softc {
	struct com_softc sc_com;		/* real "com" softc */

	/* PCMCIA-specific goo */
	struct pcmcia_io_handle sc_pcioh;	/* PCMCIA i/o space info */
	int sc_io_window;			/* our i/o window */
	struct pcmcia_function *sc_pf;		/* our PCMCIA function */
	void *sc_ih;				/* interrupt handler */
};

struct cfattach com_pcmcia_ca = {
	sizeof(struct com_pcmcia_softc), com_pcmcia_match, com_pcmcia_attach,
	com_pcmcia_detach, com_activate
};

/* Look for pcmcia cards with particular CIS strings */
static struct com_dev *
com_dev_match(card)
	struct pcmcia_card *card;
{
	int i, j;

	for (i = 0; i < com_devs_size; i++) {
		for (j = 0; j < 4; j++)
			if (com_devs[i].cis1_info[j] &&
			    strcmp(com_devs[i].cis1_info[j],
			    card->cis1_info[j]) != 0)
				break;
		if (j == 4)
			return &com_devs[i];
	}

	return NULL;
}


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
	for (cfe = pa->pf->cfe_head.sqh_first; cfe;
	    cfe = cfe->cfe_list.sqe_next) {
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
	if (com_dev_match(pa->card) != NULL)
		return 1;

	return 0;
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
	int             autoalloc = 0;

	psc->sc_pf = pa->pf;

	psc->sc_io_window = -1;

retry:
	/* find a cfe we can use */

	for (cfe = pa->pf->cfe_head.sqh_first; cfe;
	    cfe = cfe->cfe_list.sqe_next) {
#if 0
		/*
		 * Some modem cards (e.g. Xircom CM33) also have
		 * mem space.  Don't bother with this check.
		 */
		if (cfe->num_memspace != 0)
			continue;
#endif

		if (cfe->num_iospace != 1)
			continue;

		if (autoalloc == 0) {
			/*
			 * cfe->iomask == 3 is our test for the "generic"
			 * config table entry, which we want to avoid on the
			 * first pass and use exclusively on the second pass.
			 */
			if ((cfe->iomask != 3) && 
			    (cfe->iospace[0].start != 0)) {
				if (!pcmcia_io_alloc(pa->pf,
				    cfe->iospace[0].start, 
				    cfe->iospace[0].length, 0,
				    &psc->sc_pcioh)) {
					goto found;
				}
			}
		} else {
			if (cfe->iomask == 3) {
				if (!pcmcia_io_alloc(pa->pf, 0,
				    cfe->iospace[0].length,
				    cfe->iospace[0].length, &psc->sc_pcioh)) {
					goto found;
				}
			}
		}
	}
	if (autoalloc == 0) {
		autoalloc = 1;
		goto retry;
	} else if (!cfe) {
		printf(": can't allocate i/o space\n");
		return;
	}
found:
	/* Enable the card. */
	pcmcia_function_init(pa->pf, cfe);
	if (com_pcmcia_enable1(sc))
		printf(": function enable failed\n");

	sc->enabled = 1;

	/* map in the io space */

	if (pcmcia_io_map(pa->pf, ((cfe->flags & PCMCIA_CFE_IO16) ?
	    PCMCIA_WIDTH_IO16 : PCMCIA_WIDTH_IO8), 0, psc->sc_pcioh.size,
	    &psc->sc_pcioh, &psc->sc_io_window)) {
		printf(": can't map i/o space\n");
		return;
	}
	sc->sc_iot = psc->sc_pcioh.iot;
	sc->sc_ioh = psc->sc_pcioh.ioh;

	sc->sc_iobase = -1;
	sc->sc_frequency = COM_FREQ;

	sc->enable = com_pcmcia_enable;
	sc->disable = com_pcmcia_disable;

	printf(": serial device\n%s", sc->sc_dev.dv_xname);

	com_attach_subr(sc);

	sc->enabled = 0;

	com_pcmcia_disable1(sc);
}

int
com_pcmcia_detach(self, flags)
	struct device  *self;
	int flags;
{
	struct com_pcmcia_softc *psc = (struct com_pcmcia_softc *) self;
	int error;

	/* Unmap our i/o window. */
	if (psc->sc_io_window == -1) {
		printf("%s: I/O window not allocated.",
		    psc->sc_com.sc_dev.dv_xname);
		return 0;
	}

	if ((error = com_detach(self, flags)) != 0)
		return error;

	/* Unmap our i/o window. */
	pcmcia_io_unmap(psc->sc_pf, psc->sc_io_window);

	/* Free our i/o space. */
	pcmcia_io_free(psc->sc_pf, &psc->sc_pcioh);

	return 0;
}

int
com_pcmcia_enable(sc)
	struct com_softc *sc;
{
	struct com_pcmcia_softc *psc = (struct com_pcmcia_softc *) sc;
	struct pcmcia_function *pf = psc->sc_pf;
	int error;

	if ((error = com_pcmcia_enable1(sc)) != 0)
		return error;

	/* establish the interrupt. */
	psc->sc_ih = pcmcia_intr_establish(pf, IPL_SERIAL, comintr, sc);
	if (psc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n",
		    sc->sc_dev.dv_xname);
		com_pcmcia_disable1(sc);
		return 1;
	}
	return 0;
}

int
com_pcmcia_enable1(sc)
	struct com_softc *sc;
{
	struct com_pcmcia_softc *psc = (struct com_pcmcia_softc *) sc;
	struct pcmcia_function *pf = psc->sc_pf;
	int ret;

	if ((ret = pcmcia_function_enable(pf)) != 0)
		return ret;

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
	return ret;
}

void
com_pcmcia_disable(sc)
	struct com_softc *sc;
{
	struct com_pcmcia_softc *psc = (struct com_pcmcia_softc *) sc;

	pcmcia_intr_disestablish(psc->sc_pf, psc->sc_ih);
	com_pcmcia_disable1(sc);
}

void
com_pcmcia_disable1(sc)
	struct com_softc *sc;
{
	struct com_pcmcia_softc *psc = (struct com_pcmcia_softc *) sc;

	pcmcia_function_disable(psc->sc_pf);
}
