/*	$NetBSD: com_pcmcia.c,v 1.9.2.1 1998/08/08 03:06:50 eeh Exp $	*/

/*-
 * Copyright (c) 1993, 1994, 1995, 1996
 *	Charles M. Hannum.  All rights reserved.
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
#include <sys/types.h>
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
	char *name;
	int manufacturer;
	int product;
	char *cis1_info[4];
};

static struct com_dev com_devs[] = {
	{ PCMCIA_STR_3COM_3C562,
	  PCMCIA_VENDOR_3COM, PCMCIA_PRODUCT_3COM_3C562,
	  PCMCIA_CIS_3COM_3C562 },

	{ PCMCIA_STR_MOTOROLA_POWER144,
	  PCMCIA_VENDOR_MOTOROLA, PCMCIA_PRODUCT_MOTOROLA_POWER144,
	  PCMCIA_CIS_MOTOROLA_POWER144 },

	{ PCMCIA_STR_IBM_HOME_AND_AWAY,
	  PCMCIA_VENDOR_IBM, PCMCIA_PRODUCT_IBM_HOME_AND_AWAY,
	  PCMCIA_CIS_IBM_HOME_AND_AWAY },

	{ PCMCIA_STR_MEGAHERTZ_XJ4288,
	  PCMCIA_VENDOR_MEGAHERTZ, PCMCIA_PRODUCT_MEGAHERTZ_XJ4288,
	  PCMCIA_CIS_MEGAHERTZ_XJ4288 },

	{ PCMCIA_STR_MEGAHERTZ_XJ4288,
	  PCMCIA_VENDOR_INVALID, PCMCIA_PRODUCT_MEGAHERTZ_XJ4288,
	  PCMCIA_CIS_MEGAHERTZ_XJ4288 },

	{ PCMCIA_STR_USROBOTICS_WORLDPORT144,
	  PCMCIA_VENDOR_USROBOTICS, PCMCIA_PRODUCT_USROBOTICS_WORLDPORT144,
	  PCMCIA_CIS_USROBOTICS_WORLDPORT144 },

	{ PCMCIA_STR_SOCKET_PAGECARD,
	  PCMCIA_VENDOR_SOCKET, PCMCIA_PRODUCT_SOCKET_PAGECARD,
	  PCMCIA_CIS_SOCKET_PAGECARD },

	{ PCMCIA_STR_MOTOROLA_PM100C,
	  PCMCIA_VENDOR_MOTOROLA, PCMCIA_PRODUCT_MOTOROLA_PM100C,
	  PCMCIA_CIS_MOTOROLA_PM100C },

	{ PCMCIA_STR_SIMPLETECH_COMMUNICATOR288,
	  PCMCIA_VENDOR_SIMPLETECH, PCMCIA_PRODUCT_SIMPLETECH_COMMUNICATOR288,
	  PCMCIA_CIS_SIMPLETECH_COMMUNICATOR288 },
};

static struct com_dev generic = {
	"Generic Modem",
	PCMCIA_VENDOR_INVALID, PCMCIA_PRODUCT_INVALID,
	PCMCIA_CIS_INVALID
};

static int com_devs_size = sizeof(com_devs) / sizeof(com_devs[0]);
static struct com_dev *com_dev_match __P((struct pcmcia_card *));

int com_pcmcia_match __P((struct device *, struct cfdata *, void *));
void com_pcmcia_attach __P((struct device *, struct device *, void *));
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
	sizeof(struct com_pcmcia_softc), com_pcmcia_match, com_pcmcia_attach
};

static struct com_dev *
com_dev_match(card)
	struct pcmcia_card *card;
{
	int i;

	/* 1. check our table */
	for (i = 0; i < com_devs_size; i++) {
		if (com_devs[i].manufacturer != -1) {
			/* manufacturer/product match */
			if (com_devs[i].manufacturer == card->manufacturer &&
			    com_devs[i].product == card->product)
				return &com_devs[i];
		} else {
			/* cis strings match */
			int j;

			for (j = 0; j < 4; j++)
				if (com_devs[i].cis1_info[j] &&
				    strcmp(com_devs[i].cis1_info[j],
				    card->cis1_info[j]))
					break;
			if (j == 4)
				return &com_devs[i];
		}
	}
			

	/*
	 * 2. Be selective about devices by checking for the word modem in
	 *    the cis strings.
	 */
	for (i = 0; i < 4; i++)
		if (card->cis1_info[i] &&
		    pmatch(card->cis1_info[i],
			   "*[Mm][Oo][Dd][Ee][Mm]*", NULL) > 0)
			return &generic;

	return NULL;
}


int
com_pcmcia_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;

	/* find a cfe we can use (if it matches a standard COM port) */
	for (cfe = pa->pf->cfe_head.sqh_first; cfe;
	    cfe = cfe->cfe_list.sqe_next)
		if (cfe->iospace[0].start == IO_COM1 ||
		    cfe->iospace[0].start == IO_COM2 ||
		    cfe->iospace[0].start == IO_COM3 ||
		    cfe->iospace[0].start == IO_COM4)
			break;

	/* No appropriate cfe, bail out */
	if (cfe == NULL)
		return 0;

	/*
	 * We found a cfe at a standard com port location; check if it
	 * is really a modem/serial device
	 */
	if (com_dev_match(pa->card) == NULL)
		return 0;

	return 1;
}

void
com_pcmcia_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct com_pcmcia_softc *psc = (void *) self;
	struct com_softc *sc = &psc->sc_com;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;
	struct com_dev *comdev;

	psc->sc_pf = pa->pf;

	/* find a cfe we can use */

	for (cfe = pa->pf->cfe_head.sqh_first; cfe;
	     cfe = cfe->cfe_list.sqe_next) {
		if (cfe->num_memspace != 0)
			continue;
     
		if (cfe->num_iospace != 1)
			continue;

		if (cfe->iomask == 3) {
			if (pcmcia_io_alloc(pa->pf, 0, cfe->iospace[0].length,
			    cfe->iospace[0].length, &psc->sc_pcioh)) {
				continue;
			}
		} else {
			if (pcmcia_io_alloc(pa->pf, cfe->iospace[0].start,
			    cfe->iospace[0].length, 0, &psc->sc_pcioh)) {
				continue;
			}
		}

		break;
	}

	if (!cfe) {
		printf(": can't allocate i/o space\n");
		return;
	}

	sc->sc_iot = psc->sc_pcioh.iot;
	sc->sc_ioh = psc->sc_pcioh.ioh;

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

	sc->sc_iobase = -1;
	sc->sc_frequency = COM_FREQ;

	sc->enable = com_pcmcia_enable;
	sc->disable = com_pcmcia_disable;

	if ((comdev = com_dev_match(pa->card)) != NULL)
		printf(": %s", comdev->name);

	com_attach_subr(sc);

	sc->enabled = 0;
	
	com_pcmcia_disable1(sc);
}

int
com_pcmcia_enable(sc)
	struct com_softc *sc;
{
	struct com_pcmcia_softc *psc = (struct com_pcmcia_softc *) sc;
	struct pcmcia_function *pf = psc->sc_pf;

	/* establish the interrupt. */
	psc->sc_ih = pcmcia_intr_establish(pf, IPL_SERIAL, comintr, sc);
	if (psc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n",
		       sc->sc_dev.dv_xname);
		return (1);
	}
	return com_pcmcia_enable1(sc);
}

int
com_pcmcia_enable1(sc)
	struct com_softc *sc;
{
	struct com_pcmcia_softc *psc = (struct com_pcmcia_softc *) sc;
	struct pcmcia_function *pf = psc->sc_pf;
	int ret;

	if ((ret = pcmcia_function_enable(pf)))
	    return(ret);

	if (psc->sc_pf->sc->card.product == PCMCIA_PRODUCT_3COM_3C562) {
		int reg;

		/* turn off the ethernet-disable bit */

		reg = pcmcia_ccr_read(pf, PCMCIA_CCR_OPTION);
		if (reg & 0x08) {
		    reg &= ~0x08;
		    pcmcia_ccr_write(pf, PCMCIA_CCR_OPTION, reg);
		}
	}

	return(ret);
}

void
com_pcmcia_disable(sc)
	struct com_softc *sc;
{
	struct com_pcmcia_softc *psc = (struct com_pcmcia_softc *) sc;

	com_pcmcia_disable1(sc);
	pcmcia_intr_disestablish(psc->sc_pf, psc->sc_ih);
}

void
com_pcmcia_disable1(sc)
	struct com_softc *sc;
{
	struct com_pcmcia_softc *psc = (struct com_pcmcia_softc *) sc;

	pcmcia_function_disable(psc->sc_pf);
}
