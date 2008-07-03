/*	$Id: mpcsa_spi.c,v 1.1.22.1 2008/07/03 18:37:52 simonb Exp $	*/
/*	$NetBSD: mpcsa_spi.c,v 1.1.22.1 2008/07/03 18:37:52 simonb Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy. All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: mpcsa_spi.c,v 1.1.22.1 2008/07/03 18:37:52 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/lock.h>
#include <arm/at91/at91reg.h>
#include <arm/at91/at91var.h>
#include <arm/at91/at91spivar.h>
#include <arm/at91/at91piovar.h>

#ifdef MPCSA_SPI_DEBUG
int mpcsa_spi_debug = MPCSA_SPI_DEBUG;
#define DPRINTFN(n,x)	if (mpcsa_spi_debug>(n)) printf x;
#else
#define DPRINTFN(n,x)   
#endif  

struct at91pio_softc;

struct mpcsa_spi_softc {
	struct at91spi_softc	sc_dev;
	struct at91pio_softc	*sc_pioa, *sc_piod;
};

static int mpcsa_spi_match(struct device *, struct cfdata *, void *);
static void mpcsa_spi_attach(struct device *, struct device *, void *);

CFATTACH_DECL(mpcsa_spi, sizeof(struct mpcsa_spi_softc),
	      mpcsa_spi_match, mpcsa_spi_attach, NULL, NULL);

static int mpcsa_spi_select(void *self, int sel);

struct at91spi_machdep mpcsa_spi_tag = {
	mpcsa_spi_select
};

static int
mpcsa_spi_match(struct device *parent, struct cfdata *match, void *aux)
{
	if (strcmp(match->cf_name, "at91spi") == 0 && strcmp(match->cf_atname, "mpcsa_spi") == 0)
		return 2;
	return 0;
}

#define	GPIO_SPICS0(func)	(func)(sc->sc_pioa, 3)
#define	GPIO_SPICS1(func)	(func)(sc->sc_pioa, 4)
#define	GPIO_SPICS2(func)	(func)(sc->sc_piod, 19)


static void
mpcsa_spi_attach(struct device *parent, struct device *self, void *aux)
{
	struct mpcsa_spi_softc *sc = (struct mpcsa_spi_softc *)self;

	// do some checks
	if ((sc->sc_pioa = at91pio_sc(AT91_PIOA)) == NULL) {
		printf("no PIOA!\n");
		return;
	}
	if ((sc->sc_piod = at91pio_sc(AT91_PIOD)) == NULL) {
		printf("no PIOD!\n");
		return;
	}

	// initialize softc
	sc->sc_dev.sc_spi.sct_nslaves = 3;	// number of slaves

	// configure GPIO
	GPIO_SPICS0(at91pio_out); GPIO_SPICS0(at91pio_set);
	GPIO_SPICS1(at91pio_out); GPIO_SPICS1(at91pio_set);
	GPIO_SPICS2(at91pio_out); GPIO_SPICS2(at91pio_set);

	// and call common routine
	at91spi_attach_common(parent, self, aux, &mpcsa_spi_tag);
}

static int mpcsa_spi_select(void *self, int sel)
{
	struct mpcsa_spi_softc *sc = (struct mpcsa_spi_softc *)self;

	/* first deselect everything */
	GPIO_SPICS0(at91pio_set);
	GPIO_SPICS1(at91pio_set);
	GPIO_SPICS2(at91pio_set);

	/* then select wanted target */
	switch (sel) {
	case -1:
	  break;
	case 0:
	  GPIO_SPICS0(at91pio_clear);
	  break;
	case 1:
	  GPIO_SPICS2(at91pio_clear);
	  break;
	case 2:
	  GPIO_SPICS2(at91pio_clear);
	  break;
	default:
	  return EINVAL;
	}

	return 0;
}
