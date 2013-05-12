/* $NetBSD: omap2_spi.c,v 1.1.2.3 2013/05/12 20:11:39 khorben Exp $ */

/*
 * Texas Instruments OMAP2/3 Multichannel SPI driver.
 *
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Pierre Pronchery (khorben@defora.org).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: omap2_spi.c,v 1.1.2.3 2013/05/12 20:11:39 khorben Exp $");

#include "opt_omap.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <dev/spi/spivar.h>

#include <arm/omap/omap2_obiovar.h>

#include <arm/omap/omap2_reg.h>
#include <arm/omap/omap2_spireg.h>

#define OMAP2_MCSPI_MAX_CHANNELS	4

struct omap2_spi_channel {
	SIMPLEQ_HEAD(,spi_transfer) queue;
	struct spi_transfer	*transfer;
	struct spi_chunk	*wchunk;
	struct spi_chunk	*rchunk;

	bool			running;
};

struct omap2_spi_softc {
	device_t			sc_dev;
	bus_space_tag_t			sc_iot;
	bus_space_handle_t		sc_ioh;
	void				*sc_intr;

	struct spi_controller		sc_spi;

	struct omap2_spi_channel	sc_channels[OMAP2_MCSPI_MAX_CHANNELS];
};

#define SPI_READ_REG(sc, reg)		\
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define SPI_WRITE_REG(sc, reg, val)	\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))

static int	omap2_spi_match(device_t, cfdata_t, void *);
static void	omap2_spi_attach(device_t, device_t, void *);

static int	omap2_spi_configure(void *, int, int, int);
static int	omap2_spi_transfer(void *, struct spi_transfer *);

static void	omap2_spi_start(struct omap2_spi_softc * const, int);
static void	omap2_spi_stop(struct omap2_spi_softc * const, int);
static int	omap2_spi_intr(void *v);

static int	omap2_spi_reset(struct omap2_spi_softc * const);
static void	omap2_spi_send(struct omap2_spi_softc * const, int);
static void	omap2_spi_recv(struct omap2_spi_softc * const, int);

CFATTACH_DECL_NEW(omap2_spi, sizeof(struct omap2_spi_softc),
    omap2_spi_match, omap2_spi_attach, NULL, NULL);


static int
omap2_spi_match(device_t parent, cfdata_t match, void *opaque)
{
	struct obio_attach_args *obio = opaque;

#if defined(OMAP_3430) || defined(OMAP_3530)
	if (obio->obio_addr == SPI1_BASE_3530 ||
	    obio->obio_addr == SPI2_BASE_3530 ||
	    obio->obio_addr == SPI3_BASE_3530 ||
	    obio->obio_addr == SPI4_BASE_3530)
		return 1;
#endif

	return 0;
}

static void
omap2_spi_attach(device_t parent, device_t self, void *opaque)
{
	struct omap2_spi_softc *sc = device_private(self);
	struct obio_attach_args *obio = opaque;
	int nslaves;
	uint32_t rev;
	struct spibus_attach_args sba;
	int i;

	aprint_naive("\n");

	sc->sc_dev = self;
	sc->sc_iot = obio->obio_iot;

#if defined(OMAP_3430) || defined(OMAP_3530)
	switch (obio->obio_addr) {
		case SPI1_BASE_3530:
			nslaves = 4;
			break;
		case SPI2_BASE_3530:
		case SPI3_BASE_3530:
			nslaves = 2;
			break;
		case SPI4_BASE_3530:
			nslaves = 1;
			break;
		default:
			return;
	}
#endif

	if (bus_space_map(obio->obio_iot, obio->obio_addr, obio->obio_size, 0,
				&sc->sc_ioh) != 0) {
		aprint_error(": couldn't map address space\n");
		return;
	}

	rev = SPI_READ_REG(sc, OMAP2_MCSPI_REVISION);
	aprint_normal(": rev %u.%u\n",
			(unsigned int)((rev & OMAP2_MCSPI_REV_MAJ_MASK)
				>> OMAP2_MCSPI_REV_MAJ_SHIFT),
			(unsigned int)((rev & OMAP2_MCSPI_REV_MIN_MASK)
				>> OMAP2_MCSPI_REV_MIN_SHIFT));

	omap2_spi_reset(sc);

	if (obio->obio_intr < 0) {
		sc->sc_intr = NULL;
	} else {
		sc->sc_intr = intr_establish(obio->obio_intr, IPL_BIO,
				IST_LEVEL, omap2_spi_intr, sc);
		if (sc->sc_intr == NULL) {
			aprint_error_dev(self,
					"couldn't establish interrupt\n");
		}
	}

	sc->sc_spi.sct_cookie = sc;
	sc->sc_spi.sct_configure = omap2_spi_configure;
	sc->sc_spi.sct_transfer = omap2_spi_transfer;
	sc->sc_spi.sct_nslaves = nslaves;

	/* initialize the channels */
	for (i = 0; i < nslaves; i++) {
		spi_transq_init(&sc->sc_channels[i].queue);
	}

	/* initialize and attach the SPI bus */
	sba.sba_controller = &sc->sc_spi;
	config_found_ia(self, "spibus", &sba, spibus_print);
}

static int
omap2_spi_configure(void *cookie, int slave, int mode, int speed)
{
	struct omap2_spi_softc *sc = cookie;
	uint32_t conf = 0;
	uint32_t div;

	if (slave >= sc->sc_spi.sct_nslaves)
		return EINVAL;

	if (speed <= 0)
		return EINVAL;

	/* set the speed */
	/* XXX implement lower speeds */
	if (speed <= 187500)
		div = 0x8;
	else if (speed <= 375000)
		div = 0x7;
	else if (speed <= 750000)
		div = 0x6;
	else if (speed <= 1500000)
		div = 0x5;
	else if (speed <= 3000000)
		div = 0x4;
	else if (speed <= 6000000)
		div = 0x3;
	else if (speed <= 12000000)
		div = 0x2;
	else if (speed <= 24000000)
		div = 0x1;
	else
		div = 0;
	conf |= (div << OMAP2_MCSPI_CHXCONF_CLKD_SHIFT);

	/* set the word size to 8 bits */
	conf &= ~OMAP2_MCSPI_CHXCONF_WL;
	conf |= (0x7 << OMAP2_MCSPI_CHXCONF_WL_SHIFT);

	/* disable the FIFO */
	conf &= ~OMAP2_MCSPI_CHXCONF_FFER;
	conf &= ~OMAP2_MCSPI_CHXCONF_FFEW;

	switch (mode) {
		case SPI_MODE_0:
			break;
		case SPI_MODE_1:
			conf |= OMAP2_MCSPI_CHXCONF_PHA;
			break;
		case SPI_MODE_2:
			conf |= OMAP2_MCSPI_CHXCONF_POL;
			break;
		case SPI_MODE_3:
			conf |= OMAP2_MCSPI_CHXCONF_POL
				| OMAP2_MCSPI_CHXCONF_PHA;
			break;
		default:
			return EINVAL;
	}

	SPI_WRITE_REG(sc, OMAP2_MCSPI_CHXCONF(slave), conf);

	return 0;
}

static int
omap2_spi_transfer(void *cookie, struct spi_transfer *st)
{
	struct omap2_spi_softc *sc = cookie;
	int s;
	int channel;

	channel = st->st_slave;
	s = splbio();
	spi_transq_enqueue(&sc->sc_channels[channel].queue, st);
	omap2_spi_start(sc, channel);
	splx(s);
	return 0;
}

static void
omap2_spi_start(struct omap2_spi_softc * const sc, int channel)
{
	struct omap2_spi_channel *chan;
	struct spi_transfer *st;
	uint32_t enableval = 0;
	uint32_t ctrlreg;
	uint32_t ctrlval;
	uint32_t u32;

	chan = &sc->sc_channels[channel];
	if (chan->running)
		return;

	if ((st = spi_transq_first(&chan->queue)) == NULL)
		return;
	spi_transq_dequeue(&chan->queue);

	chan->running = true;

	KASSERT(chan->transfer == NULL);
	chan->transfer = st;
	chan->rchunk = chan->wchunk = st->st_chunks;

	switch (channel) {
		case 0:
			enableval = OMAP2_MCSPI_IRQENABLE_RX0_FULL
				| OMAP2_MCSPI_IRQENABLE_TX0_UNDERFLOW
				| OMAP2_MCSPI_IRQENABLE_TX0_EMPTY;
			break;
		case 1:
			enableval = OMAP2_MCSPI_IRQENABLE_RX1_FULL
				| OMAP2_MCSPI_IRQENABLE_TX1_UNDERFLOW
				| OMAP2_MCSPI_IRQENABLE_TX1_EMPTY;
			break;
		case 2:
			enableval = OMAP2_MCSPI_IRQENABLE_RX2_FULL
				| OMAP2_MCSPI_IRQENABLE_TX2_UNDERFLOW
				| OMAP2_MCSPI_IRQENABLE_TX2_EMPTY;
			break;
		case 3:
			enableval = OMAP2_MCSPI_IRQENABLE_RX3_FULL
				| OMAP2_MCSPI_IRQENABLE_TX3_UNDERFLOW
				| OMAP2_MCSPI_IRQENABLE_TX3_EMPTY;
			break;
	}

	/* enable interrupts */
	u32 = SPI_READ_REG(sc, OMAP2_MCSPI_IRQENABLE);
	SPI_WRITE_REG(sc, OMAP2_MCSPI_IRQENABLE, u32 | enableval);

	/* start the channel */
	ctrlreg = OMAP2_MCSPI_CHXCTRL(channel);
	u32 = SPI_READ_REG(sc, ctrlreg);
	ctrlval = OMAP2_MCSPI_CHXCTRL_EN;
	SPI_WRITE_REG(sc, ctrlreg, u32 | ctrlval);
}

static void
omap2_spi_stop(struct omap2_spi_softc * const sc, int channel)
{
	struct omap2_spi_channel *chan;
	uint32_t ctrlreg;
	uint32_t u32;

	chan = &sc->sc_channels[channel];

	chan->running = false;

	/* stop the channel */
	ctrlreg = OMAP2_MCSPI_CHXCTRL(channel);
	u32 = SPI_READ_REG(sc, ctrlreg);
	u32 &= ~OMAP2_MCSPI_CHXCTRL_EN;
	SPI_WRITE_REG(sc, ctrlreg, u32);
}

static int
omap2_spi_intr(void *v)
{
	struct omap2_spi_softc *sc = v;
	int i;
	struct omap2_spi_channel *chan;
	struct spi_transfer *st;
	uint32_t status;
	uint32_t u32;
	uint32_t tx_empty = 0;
	uint32_t rx_full = 0;

	/* reset the channel status bits */
	status = SPI_READ_REG(sc, OMAP2_MCSPI_IRQSTATUS);
	u32 = 0;

	/* check every channel */
	for (i = 0; i < sc->sc_spi.sct_nslaves; i++) {
		chan = &sc->sc_channels[i];

		switch (i) {
			case 0:
				u32 |= OMAP2_MCSPI_IRQSTATUS_RX0_OVERFLOW
					| OMAP2_MCSPI_IRQSTATUS_RX0_FULL
					| OMAP2_MCSPI_IRQSTATUS_TX0_UNDERFLOW
					| OMAP2_MCSPI_IRQSTATUS_TX0_EMPTY;
				tx_empty = status
					& OMAP2_MCSPI_IRQSTATUS_TX0_EMPTY;
				rx_full = status
					& OMAP2_MCSPI_IRQSTATUS_RX0_FULL;
				break;
			case 1:
				u32 |= OMAP2_MCSPI_IRQSTATUS_RX1_FULL
					| OMAP2_MCSPI_IRQSTATUS_TX1_UNDERFLOW
					| OMAP2_MCSPI_IRQSTATUS_TX1_EMPTY;
				tx_empty = status
					& OMAP2_MCSPI_IRQSTATUS_TX1_EMPTY;
				rx_full = status
					& OMAP2_MCSPI_IRQSTATUS_RX1_FULL;
				break;
			case 2:
				u32 |= OMAP2_MCSPI_IRQSTATUS_RX2_FULL
					| OMAP2_MCSPI_IRQSTATUS_TX2_UNDERFLOW
					| OMAP2_MCSPI_IRQSTATUS_TX2_EMPTY;
				tx_empty = status
					& OMAP2_MCSPI_IRQSTATUS_TX2_EMPTY;
				rx_full = status
					& OMAP2_MCSPI_IRQSTATUS_RX2_FULL;
				break;
			case 3:
				u32 |= OMAP2_MCSPI_IRQSTATUS_RX3_FULL
					| OMAP2_MCSPI_IRQSTATUS_TX3_UNDERFLOW
					| OMAP2_MCSPI_IRQSTATUS_TX3_EMPTY;
				tx_empty = status
					& OMAP2_MCSPI_IRQSTATUS_TX3_EMPTY;
				rx_full = status
					& OMAP2_MCSPI_IRQSTATUS_RX3_FULL;
				break;
		}

		if (chan->wchunk != NULL) {
			if (tx_empty) {
				omap2_spi_send(sc, i);
				if (chan->wchunk == NULL)
					omap2_spi_recv(sc, i);
			}
		}
		else if (rx_full) {
			omap2_spi_recv(sc, i);
		}

		if (chan->wchunk == NULL && chan->rchunk == NULL
				&& chan->transfer != NULL) {
			st = chan->transfer;
			chan->transfer = NULL;
			spi_done(st, 0);

			spi_transq_dequeue(&chan->queue);
			if ((st = spi_transq_first(&chan->queue)) == NULL)
				omap2_spi_stop(sc, i);
			else
				chan->transfer = st;

		}
	}

	/* acknowledge the interrupt */
	SPI_WRITE_REG(sc, OMAP2_MCSPI_IRQSTATUS, u32);

	return 1;
}

static int
omap2_spi_reset(struct omap2_spi_softc * const sc)
{
	int retry;
	uint32_t status;
	int i;

	/* proceed to a software reset */
	SPI_WRITE_REG(sc, OMAP2_MCSPI_SYSCONFIG,
			OMAP2_MCSPI_SYSCONFIG_SOFTRESET);

	/* poll until the reset is done */
	retry = 1000;
	status = SPI_READ_REG(sc, OMAP2_MCSPI_SYSSTATUS);
	while ((status & OMAP2_MCSPI_SYSSTATUS_RESETDONE) == 0) {
		if (--retry == 0)
			return EBUSY;
		delay(1000);
		status = SPI_READ_REG(sc, OMAP2_MCSPI_SYSSTATUS);
	}

	/* disable every channel available */
	for (i = 0; i < sc->sc_spi.sct_nslaves; i++) {
		SPI_WRITE_REG(sc, OMAP2_MCSPI_CHXCTRL(i), 0);
	}

	/* disable and clear all interrupts */
	SPI_WRITE_REG(sc, OMAP2_MCSPI_IRQENABLE, 0);
	SPI_WRITE_REG(sc, OMAP2_MCSPI_IRQSTATUS, 0x1777f);

	/* set the module in master and multichannel mode */
	SPI_WRITE_REG(sc, OMAP2_MCSPI_MODULCTRL, 0);

	return 0;
}

static void
omap2_spi_send(struct omap2_spi_softc * const sc, int channel)
{
	struct spi_chunk *chunk;
	const uint32_t stat = OMAP2_MCSPI_CHXSTAT(channel);
	uint32_t u32;
	uint32_t data;

	if ((chunk = sc->sc_channels[channel].wchunk) == NULL)
		return;

	if (chunk->chunk_wresid) {
		u32 = SPI_READ_REG(sc, stat);
		if ((u32 & OMAP2_MCSPI_CHXSTAT_TXS) == 0)
			return;
		if (chunk->chunk_wptr) {
			data = *chunk->chunk_wptr++;
			SPI_WRITE_REG(sc, OMAP2_MCSPI_TX(channel), data);
		}
	}

	if (--chunk->chunk_wresid == 0) {
		sc->sc_channels[channel].wchunk
			= sc->sc_channels[channel].wchunk->chunk_next;
	}
}

static void
omap2_spi_recv(struct omap2_spi_softc * const sc, int channel)
{
	struct spi_chunk *chunk;
	const uint32_t stat = OMAP2_MCSPI_CHXSTAT(channel);
	uint32_t u32;
	uint32_t data;

	while ((chunk = sc->sc_channels[channel].rchunk) != NULL
			&& chunk->chunk_rresid == 0) {
		sc->sc_channels[channel].rchunk = chunk->chunk_next;
	}

	if (chunk == NULL)
		return;

	u32 = SPI_READ_REG(sc, stat);
	if ((u32 & OMAP2_MCSPI_CHXSTAT_RXS) == 0)
		return;
	data = SPI_READ_REG(sc, OMAP2_MCSPI_RX(channel));
	if (chunk->chunk_rptr) {
		*chunk->chunk_rptr++ = data & 0xff;
	}

	if (--chunk->chunk_rresid == 0) {
		sc->sc_channels[channel].rchunk
			= sc->sc_channels[channel].rchunk->chunk_next;
	}
}
