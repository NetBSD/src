/*	$NetBSD: com_pcmcia.c,v 1.1.2.13 1997/10/15 21:53:24 thorpej Exp $	*/

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

#include <dev/isa/comreg.h>
#include <dev/isa/comvar.h>
#include <dev/isa/isareg.h>

#define PCMCIA_MANUFACTURER_3COM		0x101
#define PCMCIA_PRODUCT_3COM_3C562		0x562

#define PCMCIA_MANUFACTURER_MOTOROLA		0x109
#define PCMCIA_PRODUCT_MOTOROLA_POWER144	0x105

#define	PCMCIA_MANUFACTURER_IBM			0xa4
#define	PCMCIA_PRODUCT_IBM_HOME_AND_AWAY	0x2e

#ifdef __BROKEN_INDIRECT_CONFIG
int com_pcmcia_match __P((struct device *, void *, void *));
#else
int com_pcmcia_match __P((struct device *, struct cfdata *, void *));
#endif
void com_pcmcia_attach __P((struct device *, struct device *, void *));
void com_pcmcia_cleanup __P((void *));

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

int
com_pcmcia_match(parent, match, aux)
	struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;

	if ((pa->manufacturer == PCMCIA_MANUFACTURER_3COM) &&
	    (pa->product == PCMCIA_PRODUCT_3COM_3C562) &&
	    (pa->pf->number == 1))
		return(1);

	if ((pa->manufacturer == PCMCIA_MANUFACTURER_MOTOROLA) &&
	    (pa->product == PCMCIA_PRODUCT_MOTOROLA_POWER144) &&
	    (pa->pf->number == 0))
		return(1);

	if ((pa->manufacturer == PCMCIA_MANUFACTURER_IBM) &&
	    (pa->product == PCMCIA_PRODUCT_IBM_HOME_AND_AWAY) &&
	    (pa->pf->number == 1))
		return(1);

	/* find a cfe we can use (if it matches a standard COM port) */

	for (cfe = pa->pf->cfe_head.sqh_first; cfe;
	     cfe = cfe->cfe_list.sqe_next) {
		if (cfe->iospace[0].start == IO_COM1 ||
		    cfe->iospace[0].start == IO_COM2 ||
		    cfe->iospace[0].start == IO_COM3 ||
		    cfe->iospace[0].start == IO_COM4)
			return(1);
	}

	return(0);
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
	char *model;

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
	if (pcmcia_function_enable(pa->pf))
		printf(": function enable failed\n");

	/* turn off the bit which disables the ethernet */
	if (pa->product == PCMCIA_PRODUCT_3COM_3C562) {
		int reg;

		reg = pcmcia_ccr_read(pa->pf, PCMCIA_CCR_OPTION);
		reg &= ~0x08;
		pcmcia_ccr_write(pa->pf, PCMCIA_CCR_OPTION, reg);
	}

	/* map in the io space */

	if (pcmcia_io_map(pa->pf, ((cfe->flags & PCMCIA_CFE_IO16) ?
	    PCMCIA_WIDTH_IO16 : PCMCIA_WIDTH_IO8), 0, psc->sc_pcioh.size,
	    &psc->sc_pcioh, &psc->sc_io_window)) {
		printf(": can't map i/o space\n");
		return;
	}

	sc->sc_iobase = -1;
	sc->sc_frequency = COM_FREQ;

	com_attach_subr(sc);

	switch (pa->product) {
	case PCMCIA_PRODUCT_3COM_3C562:
		model = "3Com 3C562 Modem";
		break;
	case PCMCIA_PRODUCT_MOTOROLA_POWER144:
		model = "Motorola Power 14.4 Modem";
		break;
	default:
		model = NULL;
		break;
	}

	if (model != NULL)
		printf(": %s\n", model);

	/* establish the interrupt. */
	psc->sc_ih = pcmcia_intr_establish(pa->pf, IPL_SERIAL, comintr, sc);
	if (psc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n",
		       sc->sc_dev.dv_xname);
		return;
	}
}
