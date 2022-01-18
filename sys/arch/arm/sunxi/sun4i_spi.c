/*	$NetBSD: sun4i_spi.c,v 1.7.14.2 2022/01/18 00:14:20 thorpej Exp $	*/

/*
 * Copyright (c) 2019 Tobias Nygren
 * Copyright (c) 2018 Jonathan A. Kollasch
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sun4i_spi.c,v 1.7.14.2 2022/01/18 00:14:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/bitops.h>
#include <dev/spi/spivar.h>
#include <arm/sunxi/sun4i_spireg.h>
#include <dev/fdt/fdtvar.h>

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun4i-a10-spi" },
	DEVICE_COMPAT_EOL
};

struct sun4ispi_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	void			*sc_intrh;
	struct spi_controller	sc_spi;
	SIMPLEQ_HEAD(,spi_transfer) sc_q;
	struct spi_transfer	*sc_transfer;
	struct spi_chunk	*sc_rchunk, *sc_wchunk;
	uint32_t		sc_CTL;
	u_int			sc_modclkrate;
	volatile bool		sc_running;
};

#define SPIREG_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define SPIREG_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int sun4ispi_match(device_t, cfdata_t, void *);
static void sun4ispi_attach(device_t, device_t, void *);

static int sun4ispi_configure(void *, int, int, int);
static int sun4ispi_transfer(void *, struct spi_transfer *);

static void sun4ispi_txfifo_fill(struct sun4ispi_softc * const, size_t);
static void sun4ispi_rxfifo_drain(struct sun4ispi_softc * const, size_t);
static void sun4ispi_rxtx(struct sun4ispi_softc * const);
static void sun4ispi_set_interrupt_mask(struct sun4ispi_softc * const);
static void sun4ispi_start(struct sun4ispi_softc * const);
static int sun4ispi_intr(void *);

CFATTACH_DECL_NEW(sun4i_spi, sizeof(struct sun4ispi_softc),
    sun4ispi_match, sun4ispi_attach, NULL, NULL);

static int
sun4ispi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sun4ispi_attach(device_t parent, device_t self, void *aux)
{
	struct sun4ispi_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	struct clk *clk, *modclk;
	char intrstr[128];

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	SIMPLEQ_INIT(&sc->sc_q);

	if ((clk = fdtbus_clock_get_index(phandle, 0)) == NULL
	    || clk_enable(clk) != 0) {
		aprint_error(": couldn't enable clock\n");
		return;
	}

	if ((modclk = fdtbus_clock_get(phandle, "mod")) == NULL
	    || clk_set_rate(modclk, clk_get_rate(clk)) != 0
	    || clk_enable(modclk) != 0) {
		aprint_error(": couldn't enable module clock\n");
		return;
	}
	sc->sc_modclkrate = clk_get_rate(modclk);

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0
	    || bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	SPIREG_WRITE(sc, SPI_CTL, SPI_CTL_SSPOL | SPI_CTL_RF_RST
	    | SPI_CTL_TF_RST | SPI_CTL_MODE);
	SPIREG_WRITE(sc, SPI_DMACTL, 0);
	SPIREG_WRITE(sc, SPI_WAIT, 0);
	SPIREG_WRITE(sc, SPI_INTCTL, 0);
	SPIREG_WRITE(sc, SPI_INT_STA, ~0);

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	sc->sc_intrh = fdtbus_intr_establish_xname(phandle, 0, IPL_VM, 0,
	    sun4ispi_intr, sc, device_xname(self));
	if (sc->sc_intrh == NULL) {
		aprint_error(": unable to establish interrupt\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": SPI\n");
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	sc->sc_spi.sct_cookie = sc;
	sc->sc_spi.sct_configure = sun4ispi_configure;
	sc->sc_spi.sct_transfer = sun4ispi_transfer;
	(void) of_getprop_uint32(phandle, "num-cs", &sc->sc_spi.sct_nslaves);

	struct spibus_attach_args sba = {
		.sba_controller = &sc->sc_spi,
	};
	config_found(self, &sba, spibus_print,
	    CFARGS(.devhandle = device_handle(self)));
}

static int
sun4ispi_configure(void *cookie, int slave, int mode, int speed)
{
	struct sun4ispi_softc * const sc = cookie;
	uint32_t ctl, cctl;
	uint32_t minfreq, maxfreq;

	minfreq = sc->sc_modclkrate >> 16;
	maxfreq = sc->sc_modclkrate >> 1;

	if (speed <= 0 || speed < minfreq || speed > maxfreq)
		return EINVAL;

	if (slave >= sc->sc_spi.sct_nslaves)
		return EINVAL;

	ctl = SPI_CTL_SDM | SPI_CTL_TP_EN | SPI_CTL_SSPOL | SPI_CTL_MODE | SPI_CTL_EN;

	switch (mode) {
	case SPI_MODE_0:
		ctl |= 0;
		break;
	case SPI_MODE_1:
		ctl |= SPI_CTL_PHA;
		break;
	case SPI_MODE_2:
		ctl |= SPI_CTL_POL;
		break;
	case SPI_MODE_3:
		ctl |= SPI_CTL_PHA | SPI_CTL_POL;
		break;
	default:
		return EINVAL;
	}

	if (speed < sc->sc_modclkrate / 512) {
		for (cctl = 0; cctl <= __SHIFTOUT_MASK(SPI_CCTL_CDR1); cctl++) {
			if ((sc->sc_modclkrate / (1 << cctl)) <= speed)
				goto cdr1_found;
		}
		return EINVAL;
cdr1_found:
		cctl = __SHIFTIN(cctl, SPI_CCTL_CDR1);
	} else {
		cctl = howmany(sc->sc_modclkrate, 2 * speed) - 1;
		cctl = SPI_CCTL_DRS|__SHIFTIN(cctl, SPI_CCTL_CDR2);
	}

	device_printf(sc->sc_dev, "ctl 0x%x, cctl 0x%x, CLK %uHz, SCLK %uHz\n",
	    ctl, cctl, sc->sc_modclkrate,
	    (cctl & SPI_CCTL_DRS)
	    ? (sc->sc_modclkrate / (u_int)(2 * (__SHIFTOUT(cctl, SPI_CCTL_CDR2) + 1)))
	    : (sc->sc_modclkrate >> (__SHIFTOUT(cctl, SPI_CCTL_CDR1) + 1))
	);

	sc->sc_CTL = ctl;
	SPIREG_WRITE(sc, SPI_CTL, (ctl | SPI_CTL_RF_RST | SPI_CTL_TF_RST) & ~SPI_CTL_EN);
	SPIREG_WRITE(sc, SPI_CCTL, cctl);
	SPIREG_WRITE(sc, SPI_CTL, ctl);

	return 0;
}

static int
sun4ispi_transfer(void *cookie, struct spi_transfer *st)
{
	struct sun4ispi_softc * const sc = cookie;
	int s;

	s = splbio();
	spi_transq_enqueue(&sc->sc_q, st);
	if (sc->sc_running == false) {
		sun4ispi_start(sc);
	}
	splx(s);

	return 0;
}

static void
sun4ispi_txfifo_fill(struct sun4ispi_softc * const sc, size_t maxlen)
{
	struct spi_chunk *chunk = sc->sc_wchunk;
	size_t len;
	uint8_t b;

	if (chunk == NULL)
		return;

	len = MIN(maxlen, chunk->chunk_wresid);
	chunk->chunk_wresid -= len;
	while (len--) {
		if (chunk->chunk_wptr) {
			b = *chunk->chunk_wptr++;
		} else {
			b = 0;
		}
		bus_space_write_1(sc->sc_bst, sc->sc_bsh, SPI_TXDATA, b);
	}
	if (sc->sc_wchunk->chunk_wresid == 0) {
		sc->sc_wchunk = sc->sc_wchunk->chunk_next;
	}
}

static void
sun4ispi_rxfifo_drain(struct sun4ispi_softc * const sc, size_t maxlen)
{
	struct spi_chunk *chunk = sc->sc_rchunk;
	size_t len;
	uint8_t b;

	if (chunk == NULL)
		return;

	len = MIN(maxlen, chunk->chunk_rresid);
	chunk->chunk_rresid -= len;

	while (len--) {
		b = bus_space_read_1(sc->sc_bst, sc->sc_bsh, SPI_RXDATA);
		if (chunk->chunk_rptr) {
			*chunk->chunk_rptr++ = b;
		}
	}
	if (sc->sc_rchunk->chunk_rresid == 0) {
		sc->sc_rchunk = sc->sc_rchunk->chunk_next;
	}
}

static void
sun4ispi_rxtx(struct sun4ispi_softc * const sc)
{
	bool again;
	size_t rxavail, txavail;
	uint32_t fsr;

	/* service both FIFOs until no more progress can be made */
	again = true;
	while (again) {
		again = false;
		fsr = SPIREG_READ(sc, SPI_FIFO_STA);
		rxavail = __SHIFTOUT(fsr, SPI_FIFO_STA_RF_CNT);
		txavail = 64 - __SHIFTOUT(fsr, SPI_FIFO_STA_TF_CNT);
		if (rxavail > 0) {
			KASSERT(sc->sc_rchunk != NULL);
			sun4ispi_rxfifo_drain(sc, rxavail);
			again = true;
		}
		if (txavail > 0 && sc->sc_wchunk != NULL) {
			sun4ispi_txfifo_fill(sc, txavail);
			again = true;
		}
	}
}

static void
sun4ispi_set_interrupt_mask(struct sun4ispi_softc * const sc)
{
	uint32_t intctl;

	intctl = SPI_INTCTL_TX_INT_EN;
	intctl |= SPI_INTCTL_RF_OF_INT_EN;
	intctl |= SPI_INTCTL_TF_UR_INT_EN;

	if (sc->sc_rchunk) {
		if (sc->sc_rchunk->chunk_rresid >= 32) {
			intctl |= SPI_INTCTL_RF_HALF_FU_INT_EN;
		} else {
			intctl |= SPI_INTCTL_RF_RDY_INT_EN;
		}
	}
	if (sc->sc_wchunk) {
		intctl |= SPI_INTCTL_TF_HALF_EMP_INT_EN;
	}
	SPIREG_WRITE(sc, SPI_INTCTL, intctl);
}

static void
sun4ispi_start(struct sun4ispi_softc * const sc)
{
	struct spi_transfer *st;
	uint32_t ctl;
	struct spi_chunk *chunk;
	size_t burstcount;

	while ((st = spi_transq_first(&sc->sc_q)) != NULL) {

		spi_transq_dequeue(&sc->sc_q);

		KASSERT(sc->sc_transfer == NULL);
		sc->sc_transfer = st;
		sc->sc_rchunk = sc->sc_wchunk = st->st_chunks;
		sc->sc_running = true;

		burstcount = 0;
		for (chunk = st->st_chunks; chunk; chunk = chunk->chunk_next) {
			burstcount += chunk->chunk_count;
		}
		KASSERT(burstcount <= SPI_BC_BC);
		SPIREG_WRITE(sc, SPI_BC, __SHIFTIN(burstcount, SPI_BC_BC));
		SPIREG_WRITE(sc, SPI_TC, __SHIFTIN(burstcount, SPI_TC_WTC));

		sun4ispi_rxtx(sc);
		sun4ispi_set_interrupt_mask(sc);

		KASSERT(st->st_slave < sc->sc_spi.sct_nslaves);
		ctl = sc->sc_CTL | __SHIFTIN(st->st_slave, SPI_CTL_SS) | SPI_CTL_XCH;
		SPIREG_WRITE(sc, SPI_CTL, ctl);

		if (!cold)
			return;

		for (;;) {
			(void) sun4ispi_intr(sc);
			if (ISSET(st->st_flags, SPI_F_DONE))
				break;
		}
	}
	sc->sc_running = false;
}

static int
sun4ispi_intr(void *cookie)
{
	struct sun4ispi_softc * const sc = cookie;
	struct spi_transfer *st;
	uint32_t isr;

	isr = SPIREG_READ(sc, SPI_INT_STA);
	if (!isr)
		return 0;

	if (ISSET(isr, SPI_INT_STA_RO)) {
		device_printf(sc->sc_dev, "RXFIFO overflow\n");
	}
	if (ISSET(isr, SPI_INT_STA_TU)) {
		device_printf(sc->sc_dev, "TXFIFO underrun\n");
	}

	sun4ispi_rxtx(sc);

	if (ISSET(isr, SPI_INT_STA_TC)) {
		SPIREG_WRITE(sc, SPI_INTCTL, 0);
		KASSERT(sc->sc_rchunk == NULL);
		KASSERT(sc->sc_wchunk == NULL);
		st = sc->sc_transfer;
		sc->sc_transfer = NULL;
		KASSERT(st != NULL);
		spi_done(st, 0);
		sc->sc_running = false;
	} else {
		sun4ispi_set_interrupt_mask(sc);
	}
	SPIREG_WRITE(sc, SPI_INT_STA, isr);

	return 1;
}
