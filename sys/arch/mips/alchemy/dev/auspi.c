/* $NetBSD: auspi.c,v 1.1.10.1 2007/02/27 16:52:01 yamt Exp $ */

/*-
 * Copyright (c) 2006 Urbana-Champaign Independent Media Center.
 * Copyright (c) 2006 Garrett D'Amore.
 * All rights reserved.
 *
 * Portions of this code were written by Garrett D'Amore for the
 * Champaign-Urbana Community Wireless Network Project.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgements:
 *      This product includes software developed by the Urbana-Champaign
 *      Independent Media Center.
 *	This product includes software developed by Garrett D'Amore.
 * 4. Urbana-Champaign Independent Media Center's name and Garrett
 *    D'Amore's name may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER AND GARRETT D'AMORE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER OR GARRETT D'AMORE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: auspi.c,v 1.1.10.1 2007/02/27 16:52:01 yamt Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/proc.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <mips/alchemy/include/aubusvar.h>
#include <mips/alchemy/include/auvar.h>

#include <mips/alchemy/dev/aupscreg.h>
#include <mips/alchemy/dev/aupscvar.h>
#include <mips/alchemy/dev/auspireg.h>
#include <mips/alchemy/dev/auspivar.h>

#include <dev/spi/spivar.h>

struct auspi_softc {
	struct device		sc_dev;
	struct aupsc_controller	sc_psc;		/* parent controller ops */
	struct spi_controller	sc_spi;		/* SPI implementation ops */
	struct auspi_machdep	sc_md;		/* board-specific support */
	struct auspi_job	*sc_job;	/* current job */
	struct spi_chunk	*sc_wchunk;
	struct spi_chunk	*sc_rchunk;
	void			*sc_ih;		/* interrupt handler */

	struct spi_transfer	*sc_transfer;
	bool			sc_running;	/* is it processing stuff? */

	SIMPLEQ_HEAD(,spi_transfer)	sc_q;
};

#define	auspi_select(sc, slave)	\
	(sc)->sc_md.am_select((sc)->sc_md.am_cookie, (slave))

#define	STATIC

STATIC int auspi_match(struct device *, struct cfdata *, void *);
STATIC void auspi_attach(struct device *, struct device *, void *);
STATIC int auspi_intr(void *);

CFATTACH_DECL(auspi, sizeof(struct auspi_softc),
    auspi_match, auspi_attach, NULL, NULL);

/* SPI service routines */
STATIC int auspi_configure(void *, int, int, int);
STATIC int auspi_transfer(void *, struct spi_transfer *);

/* internal stuff */
STATIC void auspi_done(struct auspi_softc *, int);
STATIC void auspi_send(struct auspi_softc *);
STATIC void auspi_recv(struct auspi_softc *);
STATIC void auspi_sched(struct auspi_softc *);

#define	GETREG(sc, x)	\
	bus_space_read_4(sc->sc_psc.psc_bust, sc->sc_psc.psc_bush, x)
#define	PUTREG(sc, x, v)	\
	bus_space_write_4(sc->sc_psc.psc_bust, sc->sc_psc.psc_bush, x, v)

int
auspi_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct aupsc_attach_args *aa = aux;

	if (strcmp(aa->aupsc_name, cf->cf_name) != 0)
		return 0;

	return 1;
}

void
auspi_attach(struct device *parent, struct device *self, void *aux)
{
	struct auspi_softc *sc = device_private(self);
	struct aupsc_attach_args *aa = aux;
	struct spibus_attach_args sba;
	const struct auspi_machdep *md;

	if ((md = auspi_machdep(aa->aupsc_addr)) != NULL) {
		sc->sc_md = *md;
	}

	aprint_normal(": Alchemy PSC SPI protocol\n");

	sc->sc_psc = aa->aupsc_ctrl;

	/*
	 * Initialize SPI controller
	 */
	sc->sc_spi.sct_cookie = sc;
	sc->sc_spi.sct_configure = auspi_configure;
	sc->sc_spi.sct_transfer = auspi_transfer;

	/* fix this! */
	sc->sc_spi.sct_nslaves = sc->sc_md.am_nslaves;

	sba.sba_controller = &sc->sc_spi;

	/* enable SPI mode */
	sc->sc_psc.psc_enable(sc, AUPSC_SEL_SPI);

	/* initialize the queue */
	SIMPLEQ_INIT(&sc->sc_q);

	/* make sure interrupts disabled at the SPI */
	PUTREG(sc, AUPSC_SPIMSK, SPIMSK_ALL);

	/* enable device interrupts */
	sc->sc_ih = au_intr_establish(aa->aupsc_irq, 0, IPL_SERIAL, IST_LEVEL,
	    auspi_intr, sc);

	(void) config_found_ia(&sc->sc_dev, "spibus", &sba, spibus_print);
}

int
auspi_configure(void *arg, int slave, int mode, int speed)
{
	struct auspi_softc *sc = arg;
	int		brg, i;
	uint32_t	reg;

	/* setup interrupt registers */
	PUTREG(sc, AUPSC_SPIMSK, SPIMSK_NORM);

	reg = GETREG(sc, AUPSC_SPICFG);

	reg &= ~(SPICFG_BRG_MASK);	/* clear BRG */
	reg &= ~(SPICFG_DIV_MASK);	/* use pscn_mainclock/2 */
	reg &= ~(SPICFG_PSE);		/* disable port swap */
	reg &= ~(SPICFG_BI);		/* clear bit clock invert */
	reg &= ~(SPICFG_CDE);		/* clear clock phase delay */
	reg &= ~(SPICFG_CGE);		/* clear clock gate enable */
	//reg |= SPICFG_MO;		/* master-only mode */
	reg |= SPICFG_DE;		/* device enable */
	reg |= SPICFG_DD;		/* disable DMA */
	reg |= SPICFG_RT_1;		/* 1 byte rx fifo threshold */
	reg |= SPICFG_TT_1;		/* 1 byte tx fifo threshold */
	reg |= ((8-1) << SPICFG_LEN_SHIFT);/* always work in 8-bit chunks */

	/*
	 * We assume a base clock of 48MHz has been established by the
	 * platform code.  The clock divider reduces this to 24MHz.
	 * Next we have to figure out the BRG
	 */
#define	BASECLK	24000000
	for (brg = 0; brg < 64; brg++) {
		if (speed >= (BASECLK / ((brg + 1) * 2))) {
			break;
		}
	}

	/*
	 * Does the device want to go even slower?  Our minimum speed without
	 * changing other assumptions, and complicating the code even further,
	 * is 24MHz/128, or 187.5kHz.  That should be slow enough for any
	 * device we're likely to encounter.
	 */
	if (speed < (BASECLK / ((brg + 1) * 2))) {
		return EINVAL;
	}
	reg &= ~SPICFG_BRG_MASK;
	reg |= (brg << SPICFG_BRG_SHIFT);

	/*
	 * I'm not entirely confident that these values are correct.
	 * But at least mode 0 appears to work properly with the
	 * devices I have tested.  The documentation seems to suggest
	 * that I have the meaning of the clock delay bit inverted.
	 */
	switch (mode) {
	case SPI_MODE_0:
		reg |= 0;			/* CPHA = 0, CPOL = 0 */
		break;
	case SPI_MODE_1:
		reg |= SPICFG_CDE;		/* CPHA = 1, CPOL = 0 */
		break;
	case SPI_MODE_2:
		reg |= SPICFG_BI;		/* CPHA = 0, CPOL = 1 */
		break;
	case SPI_MODE_3:
		reg |= SPICFG_CDE | SPICFG_BI;	/* CPHA = 1, CPOL = 1 */
		break;
	default:
		return EINVAL;
	}

	PUTREG(sc, AUPSC_SPICFG, reg);

	for (i = 1000000; i; i -= 10) {
		if (GETREG(sc, AUPSC_SPISTAT) & SPISTAT_DR) {
			return 0;
		}
	}

	return ETIMEDOUT;
}

void
auspi_send(struct auspi_softc *sc)
{
	uint32_t		data;
	struct spi_chunk	*chunk;

	/* fill the fifo */
	while ((chunk = sc->sc_wchunk) != NULL) {

		while (chunk->chunk_wresid) {

			/* transmit fifo full? */
			if (GETREG(sc, AUPSC_SPISTAT) & SPISTAT_TF) {
				return;
			}

			if (chunk->chunk_wptr) {
				data = *chunk->chunk_wptr++;
			} else {
				data = 0;
			}
			chunk->chunk_wresid--;

			/* if the last outbound character, mark it */
			if ((chunk->chunk_wresid == 0) &&
			    (chunk->chunk_next == NULL)) {
				data |=  SPITXRX_LC;
			}
			PUTREG(sc, AUPSC_SPITXRX, data);
		}

		/* advance to next transfer */
		sc->sc_wchunk = sc->sc_wchunk->chunk_next;
	}
}

void
auspi_recv(struct auspi_softc *sc)
{
	uint32_t		data;
	struct spi_chunk	*chunk;

	while ((chunk = sc->sc_rchunk) != NULL) {
		while (chunk->chunk_rresid) {

			/* rx fifo empty? */
			if ((GETREG(sc, AUPSC_SPISTAT) & SPISTAT_RE) != 0) {
				return;
			}

			/* collect rx data */
			data = GETREG(sc, AUPSC_SPITXRX);
			if (chunk->chunk_rptr) {
				*chunk->chunk_rptr++ = data & 0xff;
			}

			chunk->chunk_rresid--;
		}

		/* advance next to next transfer */
		sc->sc_rchunk = sc->sc_rchunk->chunk_next;
	}
}

void
auspi_sched(struct auspi_softc *sc)
{
	struct spi_transfer	*st;
	int			err;

	while ((st = spi_transq_first(&sc->sc_q)) != NULL) {

		/* remove the item */
		spi_transq_dequeue(&sc->sc_q);

		/* note that we are working on it */
		sc->sc_transfer = st;

		if ((err = auspi_select(sc, st->st_slave)) != 0) {
			spi_done(st, err);
			continue;
		}

		/* clear the fifos */
		PUTREG(sc, AUPSC_SPIPCR, SPIPCR_RC | SPIPCR_TC);
		/* setup chunks */
		sc->sc_rchunk = sc->sc_wchunk = st->st_chunks;
		auspi_send(sc);
		/* now kick the master start to get the chip running */
		PUTREG(sc, AUPSC_SPIPCR, SPIPCR_MS);
		sc->sc_running = TRUE;
		return;
	}
	auspi_select(sc, -1);
	sc->sc_running = FALSE;
}

void
auspi_done(struct auspi_softc *sc, int err)
{
	struct spi_transfer	*st;

	/* called from interrupt handler */
	if ((st = sc->sc_transfer) != NULL) {
		sc->sc_transfer = NULL;
		spi_done(st, err);
	}
	/* make sure we clear these bits out */
	sc->sc_wchunk = sc->sc_rchunk = NULL;
	auspi_sched(sc);
}

int
auspi_intr(void *arg)
{
	struct auspi_softc	*sc = arg;
	uint32_t		ev;
	int			err = 0;


	if ((GETREG(sc, AUPSC_SPISTAT) & SPISTAT_DI) == 0) {
		return 0;
	}

	ev = GETREG(sc, AUPSC_SPIEVNT);

	if (ev & SPIMSK_MM) {
		printf("%s: multiple masters detected!\n",
		    sc->sc_dev.dv_xname);
		err = EIO;
	}
	if (ev & SPIMSK_RO) {
		printf("%s: receive overflow\n", sc->sc_dev.dv_xname);
		err = EIO;
	}
	if (ev & SPIMSK_TU) {
		printf("%s: transmit underflow\n", sc->sc_dev.dv_xname);
		err = EIO;
	}
	if (err) {
		/* clear errors */
		PUTREG(sc, AUPSC_SPIEVNT,
		    ev & (SPIMSK_MM | SPIMSK_RO | SPIMSK_TU));
		/* clear the fifos */
		PUTREG(sc, AUPSC_SPIPCR, SPIPCR_RC | SPIPCR_TC);
		auspi_done(sc, err);

	} else {

		/* do all data exchanges */
		auspi_send(sc);
		auspi_recv(sc);

		/*
		 * if the master done bit is set, make sure we do the
		 * right processing.
		 */
		if (ev & SPIMSK_MD) {
			if ((sc->sc_wchunk != NULL) ||
			    (sc->sc_rchunk != NULL)) {
				printf("%s: partial transfer?\n",
				    sc->sc_dev.dv_xname);
				err = EIO;
			} 
			auspi_done(sc, err);
		}
		/* clear interrupts */
		PUTREG(sc, AUPSC_SPIEVNT,
		    ev & (SPIMSK_TR | SPIMSK_RR | SPIMSK_MD));
	}

	return 1;
}

int
auspi_transfer(void *arg, struct spi_transfer *st)
{
	struct auspi_softc	*sc = arg;
	int			s;

	/* make sure we select the right chip */
	s = splserial();
	spi_transq_enqueue(&sc->sc_q, st);
	if (sc->sc_running == 0) {
		auspi_sched(sc);
	}
	splx(s);
	return 0;
}

