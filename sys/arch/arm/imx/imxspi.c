/*	$NetBSD: imxspi.c,v 1.9.2.2 2022/01/18 00:14:20 thorpej Exp $	*/

/*-
 * Copyright (c) 2014  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * this module support CSPI and eCSPI.
 * i.MX51 have 2 eCSPI and 1 CSPI modules.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imxspi.c,v 1.9.2.2 2022/01/18 00:14:20 thorpej Exp $");

#include "opt_imxspi.h"
#include "opt_fdt.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/intr.h>

#include <sys/bus.h>
#include <machine/cpu.h>
#include <machine/intr.h>

#include <arm/imx/imxspivar.h>
#include <arm/imx/imxspireg.h>

/* SPI service routines */
static int imxspi_configure_enhanced(void *, int, int, int);
static int imxspi_configure(void *, int, int, int);
static int imxspi_transfer(void *, struct spi_transfer *);

/* internal stuff */
void imxspi_done(struct imxspi_softc *, int);
void imxspi_send(struct imxspi_softc *);
void imxspi_recv(struct imxspi_softc *);
void imxspi_sched(struct imxspi_softc *);

#define	IMXCSPI_TYPE(type, x)						      \
	((sc->sc_type == IMX31_CSPI) ? __CONCAT(CSPI_IMX31_, x) :	      \
	 (sc->sc_type == IMX35_CSPI) ? __CONCAT(CSPI_IMX35_, x) : 0)
#define	IMXCSPI(x)	__CONCAT(CSPI_, x)
#define	IMXESPI(x)	__CONCAT(ECSPI_, x)
#define	IMXSPI(x)	((sc->sc_enhanced) ? IMXESPI(x) : IMXCSPI(x))
#define	IMXSPI_TYPE(x)	((sc->sc_enhanced) ? IMXESPI(x) : IMXCSPI_TYPE(sc->sc_type, x))
#define	READ_REG(sc, x)							      \
	bus_space_read_4(sc->sc_iot, sc->sc_ioh, IMXSPI(x))
#define	WRITE_REG(sc, x, v)						      \
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IMXSPI(x), (v))

#ifdef IMXSPI_DEBUG
int imxspi_debug = IMXSPI_DEBUG;
#define	DPRINTFN(n,x)   if (imxspi_debug>(n)) printf x;
#else
#define	DPRINTFN(n,x)
#endif

int
imxspi_attach_common(device_t self)
{
	struct imxspi_softc * const sc = device_private(self);

	aprint_normal("i.MX %sCSPI Controller (clock %ld Hz)\n",
	    ((sc->sc_enhanced) ? "e" : ""), sc->sc_freq);

	/* Initialize SPI controller */
	sc->sc_dev = self;
	sc->sc_spi.sct_cookie = sc;
	if (sc->sc_enhanced)
		sc->sc_spi.sct_configure = imxspi_configure_enhanced;
	else
		sc->sc_spi.sct_configure = imxspi_configure;
	sc->sc_spi.sct_transfer = imxspi_transfer;

	/* sc->sc_spi.sct_nslaves must have been initialized by machdep code */
	sc->sc_spi.sct_nslaves = sc->sc_nslaves;
	if (!sc->sc_spi.sct_nslaves)
		aprint_error_dev(sc->sc_dev, "no slaves!\n");

	/* initialize the queue */
	SIMPLEQ_INIT(&sc->sc_q);

	/* configure SPI */
	/* Setup Control Register */
	WRITE_REG(sc, CONREG,
	    __SHIFTIN(0, IMXSPI_TYPE(CON_DRCTL)) |
	    __SHIFTIN(8 - 1, IMXSPI_TYPE(CON_BITCOUNT)) |
	    __SHIFTIN(0xf, IMXSPI(CON_MODE)) | IMXSPI(CON_ENABLE));
	/* TC and RR interruption */
	WRITE_REG(sc, INTREG, (IMXSPI_TYPE(INTR_TC_EN) | IMXSPI(INTR_RR_EN)));
	WRITE_REG(sc, STATREG, IMXSPI_TYPE(STAT_CLR));

	WRITE_REG(sc, PERIODREG, 0x0);

	struct spibus_attach_args sba = {
		.sba_controller = &sc->sc_spi,
	};
	config_found(self, &sba, spibus_print,
	    CFARGS(.devhandle = device_handle(self)));

	return 0;
}

static int
imxspi_configure(void *arg, int slave, int mode, int speed)
{
	struct imxspi_softc *sc = arg;
	uint32_t div_cnt = 0;
	uint32_t div;
	uint32_t contrl = 0;

	div = (sc->sc_freq + (speed - 1)) / speed;
	div = div - 1;
	for (div_cnt = 0; div > 0; div_cnt++)
		div >>= 1;

	div_cnt = div_cnt - 2;
	if (div_cnt >= 7)
		div_cnt = 7;

	contrl = READ_REG(sc, CONREG);
	contrl &= ~CSPI_CON_DIV;
	contrl |= __SHIFTIN(div_cnt, CSPI_CON_DIV);

	contrl &= ~(CSPI_CON_POL | CSPI_CON_PHA);
	switch (mode) {
	case SPI_MODE_0:
		/* CPHA = 0, CPOL = 0 */
		break;
	case SPI_MODE_1:
		/* CPHA = 1, CPOL = 0 */
		contrl |= CSPI_CON_PHA;
		break;
	case SPI_MODE_2:
		/* CPHA = 0, CPOL = 1 */
		contrl |= CSPI_CON_POL;
		break;
	case SPI_MODE_3:
		/* CPHA = 1, CPOL = 1 */
		contrl |= CSPI_CON_POL;
		contrl |= CSPI_CON_PHA;
		break;
	default:
		return EINVAL;
	}
	WRITE_REG(sc, CONREG, contrl);

	DPRINTFN(3, ("%s: slave %d mode %d speed %d\n",
		__func__, slave, mode, speed));

	return 0;
}

static int
imxspi_configure_enhanced(void *arg, int slave, int mode, int speed)
{
	struct imxspi_softc *sc = arg;
	uint32_t div_cnt = 0;
	uint32_t div;
	uint32_t contrl = 0;
	uint32_t config = 0;

	div = (sc->sc_freq + (speed - 1)) / speed;
	for (div_cnt = 0; div > 0; div_cnt++)
		div >>= 1;

	if (div_cnt >= 15)
		div_cnt = 15;

	contrl = READ_REG(sc, CONREG);
	contrl |= __SHIFTIN(div_cnt, ECSPI_CON_DIV);
	contrl |= __SHIFTIN(slave, ECSPI_CON_CS);
	contrl |= __SHIFTIN(__BIT(slave), ECSPI_CON_MODE);
	WRITE_REG(sc, CONREG, contrl);

	config = bus_space_read_4(sc->sc_iot, sc->sc_ioh, ECSPI_CONFIGREG);
	config &= ~(__SHIFTIN(__BIT(slave), ECSPI_CONFIG_SCLK_POL) |
	    __SHIFTIN(__BIT(slave), ECSPI_CONFIG_SCLK_CTL) |
	    __SHIFTIN(__BIT(slave), ECSPI_CONFIG_SCLK_PHA));
	switch (mode) {
	case SPI_MODE_0:
		/* CPHA = 0, CPOL = 0 */
		break;
	case SPI_MODE_1:
		/* CPHA = 1, CPOL = 0 */
		config |= __SHIFTIN(__BIT(slave), ECSPI_CONFIG_SCLK_PHA);
		break;
	case SPI_MODE_2:
		/* CPHA = 0, CPOL = 1 */
		config |= __SHIFTIN(__BIT(slave), ECSPI_CONFIG_SCLK_POL);
		config |= __SHIFTIN(__BIT(slave), ECSPI_CONFIG_SCLK_CTL);
		break;
	case SPI_MODE_3:
		/* CPHA = 1, CPOL = 1 */
		config |= __SHIFTIN(__BIT(slave), ECSPI_CONFIG_SCLK_PHA);
		config |= __SHIFTIN(__BIT(slave), ECSPI_CONFIG_SCLK_POL);
		config |= __SHIFTIN(__BIT(slave), ECSPI_CONFIG_SCLK_CTL);
		break;
	default:
		return EINVAL;
	}
	config |= __SHIFTIN(__BIT(slave), ECSPI_CONFIG_SSB_CTL);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, ECSPI_CONFIGREG, config);

	DPRINTFN(3, ("%s: slave %d mode %d speed %d\n",
		__func__, slave, mode, speed));

	return 0;
}

void
imxspi_send(struct imxspi_softc *sc)
{
	uint32_t data;
	struct spi_chunk *chunk;

	/* fill the fifo */
	while ((chunk = sc->sc_wchunk) != NULL) {
		while (chunk->chunk_wresid) {
			/* transmit fifo full? */
			if (READ_REG(sc, STATREG) & IMXSPI(STAT_TF))
				goto out;

			if (chunk->chunk_wptr) {
				data = *chunk->chunk_wptr;
				chunk->chunk_wptr++;
			} else {
				data = 0xff;
			}
			chunk->chunk_wresid--;

			WRITE_REG(sc, TXDATA, data);
		}
		/* advance to next transfer */
		sc->sc_wchunk = sc->sc_wchunk->chunk_next;
	}
out:
	if (!(READ_REG(sc, STATREG) & IMXSPI(INTR_TE_EN)))
		WRITE_REG(sc, CONREG, READ_REG(sc, CONREG) | IMXSPI(CON_XCH));
}

void
imxspi_recv(struct imxspi_softc *sc)
{
	uint32_t		data;
	struct spi_chunk	*chunk;

	while ((chunk = sc->sc_rchunk) != NULL) {
		while (chunk->chunk_rresid) {
			/* rx fifo empty? */
			if ((!(READ_REG(sc, STATREG) & IMXSPI(STAT_RR))))
				return;

			/* collect rx data */
			data = READ_REG(sc, RXDATA);
			if (chunk->chunk_rptr) {
				*chunk->chunk_rptr = data & 0xff;
				chunk->chunk_rptr++;
			}

			chunk->chunk_rresid--;
		}
		/* advance next to next transfer */
		sc->sc_rchunk = sc->sc_rchunk->chunk_next;
	}
}


void
imxspi_sched(struct imxspi_softc *sc)
{
	struct spi_transfer *st;
	uint32_t chipselect;

	while ((st = spi_transq_first(&sc->sc_q)) != NULL) {
		/* remove the item */
		spi_transq_dequeue(&sc->sc_q);

		/* note that we are working on it */
		sc->sc_transfer = st;

		/* chip slect */
		if (sc->sc_tag->spi_cs_enable != NULL)
			sc->sc_tag->spi_cs_enable(sc->sc_tag->cookie,
			    st->st_slave);

		/* chip slect */
		chipselect = READ_REG(sc, CONREG);
		chipselect &= ~IMXSPI_TYPE(CON_CS);
		chipselect |= __SHIFTIN(st->st_slave, IMXSPI_TYPE(CON_CS));
		WRITE_REG(sc, CONREG, chipselect);

		delay(1);

		/* setup chunks */
		sc->sc_rchunk = sc->sc_wchunk = st->st_chunks;

		/* now kick the master start to get the chip running */
		imxspi_send(sc);

		sc->sc_running = TRUE;
		return;
	}

	DPRINTFN(2, ("%s: nothing to do anymore\n", __func__));
	sc->sc_running = FALSE;
}

void
imxspi_done(struct imxspi_softc *sc, int err)
{
	struct spi_transfer *st;

	/* called from interrupt handler */
	if ((st = sc->sc_transfer) != NULL) {
		if (sc->sc_tag->spi_cs_disable != NULL)
			sc->sc_tag->spi_cs_disable(sc->sc_tag->cookie,
			    st->st_slave);

		sc->sc_transfer = NULL;
		spi_done(st, err);
	}
	/* make sure we clear these bits out */
	sc->sc_wchunk = sc->sc_rchunk = NULL;
	imxspi_sched(sc);
}

int
imxspi_intr(void *arg)
{
	struct imxspi_softc *sc = arg;
	uint32_t intr, sr;
	int err = 0;

	if ((intr = READ_REG(sc, INTREG)) == 0) {
		/* interrupts are not enabled, get out */
		DPRINTFN(4, ("%s: interrupts are not enabled\n", __func__));
		return 0;
	}

	sr = READ_REG(sc, STATREG);
	if (!(sr & intr)) {
		/* interrupt did not happen, get out */
		DPRINTFN(3, ("%s: interrupts did not happen\n", __func__));
		return 0;
	}

	/* RXFIFO ready? */
	if (sr & IMXSPI(INTR_RR_EN)) {
		imxspi_recv(sc);
		if (sc->sc_wchunk == NULL && sc->sc_rchunk == NULL)
			imxspi_done(sc, err);
	}

	/* Transfer Complete? */
	if (sr & IMXSPI_TYPE(INTR_TC_EN))
		imxspi_send(sc);

	/* status register clear */
	WRITE_REG(sc, STATREG, sr);

	return 1;
}

int
imxspi_transfer(void *arg, struct spi_transfer *st)
{
	struct imxspi_softc *sc = arg;
	int s;

	/* make sure we select the right chip */
	s = splbio();
	spi_transq_enqueue(&sc->sc_q, st);
	if (sc->sc_running == FALSE)
		imxspi_sched(sc);
	splx(s);

	return 0;
}
