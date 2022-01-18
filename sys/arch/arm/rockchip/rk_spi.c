/*	$NetBSD: rk_spi.c,v 1.7.8.2 2022/01/18 00:14:20 thorpej Exp $	*/

/*
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tobias Nygren.
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
__KERNEL_RCSID(0, "$NetBSD: rk_spi.c,v 1.7.8.2 2022/01/18 00:14:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/bitops.h>
#include <dev/spi/spivar.h>
#include <dev/fdt/fdtvar.h>
#include <arm/fdt/arm_fdtvar.h>

#define SPI_CTRLR0		0x00
#define SPI_CTRLR0_MTM		__BIT(21)
#define SPI_CTRLR0_OPM		__BIT(20)
#define SPI_CTRLR0_XFM		__BITS(19, 18)
#define SPI_CTRLR0_FRF		__BITS(17, 16)
#define SPI_CTRLR0_RSD		__BITS(15, 14)
#define SPI_CTRLR0_BHT		__BIT(13)
#define SPI_CTRLR0_FBM		__BIT(12)
#define SPI_CTRLR0_EM		__BIT(11)
#define SPI_CTRLR0_RW		__BIT(10)
#define SPI_CTRLR0_CSM		__BITS(9, 8)
#define SPI_CTRLR0_SCPOL	__BIT(7)
#define SPI_CTRLR0_SCPH		__BIT(6)
#define SPI_CTRLR0_CFS		__BITS(5, 2)
#define SPI_CTRLR0_DFS		__BITS(1, 0)
#define SPI_CTRLR0_DFS_4BIT	0x0
#define SPI_CTRLR0_DFS_8BIT	0x1
#define SPI_CTRLR0_DFS_16BIT	0x2

#define SPI_CTRLR1		0x04
#define SPI_CTRLR1_NDM		__BITS(15, 0)

#define SPI_ENR			0x08
#define SPI_ENR_ENR		__BIT(0)

#define SPI_SER			0x0c
#define SPI_SER_SER1		__BIT(1)
#define SPI_SER_SER0		__BIT(0)

#define SPI_BAUDR		0x10
#define SPI_BAUDR_BAUDR		__BITS(15, 0)

#define SPI_TXFTLR		0x14
#define SPI_TXFTLR_TXFLTR	__BITS(4, 0)

#define SPI_RXFTLR		0x18
#define SPI_RXFLTR_RXFLTR	__BITS(4, 0)

#define SPI_TXFLR		0x1c
#define SPI_TXFLR_TXFLR		__BITS(5, 0)

#define SPI_RXFLR		0x20
#define SPI_RXFLR_RXFLR		__BITS(5, 0)

#define SPI_SR			0x24
#define SPI_SR_RFF		__BIT(4)
#define SPI_SR_RFE		__BIT(3)
#define SPI_SR_TFE		__BIT(2)
#define SPI_SR_TFF		__BIT(1)
#define SPI_SR_BSF		__BIT(0)

#define SPI_IPR			0x28
#define SPI_IPR_IPR		__BIT(0)

#define SPI_IMR			0x2c
#define SPI_IMR_RFFIM		__BIT(4)
#define SPI_IMR_RFOIM		__BIT(3)
#define SPI_IMR_RFUIM		__BIT(2)
#define SPI_IMR_TFOIM		__BIT(1)
#define SPI_IMR_TFEIM		__BIT(0)

#define SPI_ISR			0x30
#define SPI_ISR_RFFIS		__BIT(4)
#define SPI_ISR_RFOIS		__BIT(3)
#define SPI_ISR_RFUIS		__BIT(2)
#define SPI_ISR_TFOIS		__BIT(1)
#define SPI_ISR_TFEIS		__BIT(0)

#define SPI_RISR		0x34
#define SPI_RISR_RFFRIS		__BIT(4)
#define SPI_RISR_RFORIS		__BIT(3)
#define SPI_RISR_RFURIS		__BIT(2)
#define SPI_RISR_TFORIS		__BIT(1)
#define SPI_RISR_TFERIS		__BIT(0)

#define SPI_ICR			0x38
#define SPI_ICR_CTFOI		__BIT(3)
#define SPI_ICR_CRFOI		__BIT(2)
#define SPI_ICR_CRFUI		__BIT(1)
#define SPI_ICR_CCI		__BIT(0)
#define SPI_ICR_ALL		__BITS(3, 0)

#define SPI_DMACR		0x3c
#define SPI_DMACR_TDE		__BIT(1)
#define SPI_DMACR_RDE		__BIT(0)

#define SPI_DMATDLR		0x40
#define SPI_DMATDLR_TDL		__BITS(4, 0)

#define SPI_DMARDLR		0x44
#define SPI_DMARDLR_RDL		__BITS(4, 0)

#define SPI_TXDR		0x400
#define SPI_TXDR_TXDR		__BITS(15, 0)

#define SPI_RXDR		0x800
#define SPI_RXDR_RXDR		__BITS(15, 0)

#define SPI_FIFOLEN		32

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "rockchip,rk3066-spi" },
	{ .compat = "rockchip,rk3328-spi" },
	{ .compat = "rockchip,rk3399-spi" },
	DEVICE_COMPAT_EOL
};

struct rk_spi_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	void			*sc_ih;
	u_int			sc_spi_freq;
	struct spi_controller	sc_spi;
	SIMPLEQ_HEAD(,spi_transfer) sc_q;
	struct spi_transfer	*sc_transfer;
	struct spi_chunk	*sc_rchunk, *sc_wchunk;
	volatile bool		sc_running;
};

#define SPIREG_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define SPIREG_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int rk_spi_match(device_t, cfdata_t, void *);
static void rk_spi_attach(device_t, device_t, void *);

static int rk_spi_configure(void *, int, int, int);
static int rk_spi_transfer(void *, struct spi_transfer *);

static void rk_spi_txfifo_fill(struct rk_spi_softc * const, size_t);
static void rk_spi_rxfifo_drain(struct rk_spi_softc * const, size_t);
static void rk_spi_rxtx(struct rk_spi_softc * const);
static void rk_spi_set_interrupt_mask(struct rk_spi_softc * const);
static void rk_spi_start(struct rk_spi_softc * const);
static int rk_spi_intr(void *);

CFATTACH_DECL_NEW(rk_spi, sizeof(struct rk_spi_softc),
    rk_spi_match, rk_spi_attach, NULL, NULL);

static int
rk_spi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
rk_spi_attach(device_t parent, device_t self, void *aux)
{
	struct rk_spi_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	struct clk *sclk, *pclk;
	char intrstr[128];

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	SIMPLEQ_INIT(&sc->sc_q);

	if ((sclk = fdtbus_clock_get(phandle, "spiclk")) == NULL
	    || clk_enable(sclk) != 0) {
		aprint_error(": couldn't enable sclk\n");
		return;
	}

	if ((pclk = fdtbus_clock_get(phandle, "apb_pclk")) == NULL
	    || clk_enable(pclk) != 0) {
		aprint_error(": couldn't enable pclk\n");
		return;
	}

	sc->sc_spi_freq = clk_get_rate(sclk);

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0
	    || bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	SPIREG_WRITE(sc, SPI_ENR, 0);
	SPIREG_WRITE(sc, SPI_IMR, 0);

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish_xname(phandle, 0, IPL_VM, 0,
	    rk_spi_intr, sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error(": unable to establish interrupt\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": SPI\n");
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	sc->sc_spi.sct_cookie = sc;
	sc->sc_spi.sct_configure = rk_spi_configure;
	sc->sc_spi.sct_transfer = rk_spi_transfer;
	sc->sc_spi.sct_nslaves = 2;

	struct spibus_attach_args sba = {
		.sba_controller = &sc->sc_spi,
	};
	config_found(self, &sba, spibus_print,
	    CFARGS(.devhandle = device_handle(self)));
}

static int
rk_spi_configure(void *cookie, int slave, int mode, int speed)
{
	struct rk_spi_softc * const sc = cookie;
	uint32_t ctrlr0;
	uint16_t divider;

	divider = (sc->sc_spi_freq / speed) & ~1;
	if (divider < 2) {
		aprint_error_dev(sc->sc_dev,
		    "spi_clk %u is too low for speed %u, using speed %u\n",
		     sc->sc_spi_freq, speed, sc->sc_spi_freq / 2);
		divider = 2;
	}

	if (slave >= sc->sc_spi.sct_nslaves)
		return EINVAL;

	ctrlr0 = SPI_CTRLR0_BHT | __SHIFTIN(SPI_CTRLR0_DFS_8BIT, SPI_CTRLR0_DFS);

	switch (mode) {
	case SPI_MODE_0:
		ctrlr0 |= 0;
		break;
	case SPI_MODE_1:
		ctrlr0 |= SPI_CTRLR0_SCPH;
		break;
	case SPI_MODE_2:
		ctrlr0 |= SPI_CTRLR0_SCPOL;
		break;
	case SPI_MODE_3:
		ctrlr0 |= SPI_CTRLR0_SCPH | SPI_CTRLR0_SCPOL;
		break;
	default:
		return EINVAL;
	}

	SPIREG_WRITE(sc, SPI_ENR, 0);
	SPIREG_WRITE(sc, SPI_SER, 0);
	SPIREG_WRITE(sc, SPI_CTRLR0, ctrlr0);
	SPIREG_WRITE(sc, SPI_BAUDR, divider);

	SPIREG_WRITE(sc, SPI_DMACR, 0);
	SPIREG_WRITE(sc, SPI_DMATDLR, 0);
	SPIREG_WRITE(sc, SPI_DMARDLR, 0);

	SPIREG_WRITE(sc, SPI_IPR, 0);
	SPIREG_WRITE(sc, SPI_IMR, 0);
	SPIREG_WRITE(sc, SPI_ICR, SPI_ICR_ALL);

	SPIREG_WRITE(sc, SPI_ENR, 1);

	return 0;
}

static int
rk_spi_transfer(void *cookie, struct spi_transfer *st)
{
	struct rk_spi_softc * const sc = cookie;
	int s;

	s = splbio();
	spi_transq_enqueue(&sc->sc_q, st);
	if (sc->sc_running == false) {
		rk_spi_start(sc);
	}
	splx(s);

	return 0;
}

static void
rk_spi_txfifo_fill(struct rk_spi_softc * const sc, size_t maxlen)
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
		bus_space_write_1(sc->sc_bst, sc->sc_bsh, SPI_TXDR, b);
	}
	if (sc->sc_wchunk->chunk_wresid == 0) {
		sc->sc_wchunk = sc->sc_wchunk->chunk_next;
	}
}

static void
rk_spi_rxfifo_drain(struct rk_spi_softc * const sc, size_t maxlen)
{
	struct spi_chunk *chunk = sc->sc_rchunk;
	size_t len;
	uint8_t b;

	if (chunk == NULL)
		return;

	len = MIN(maxlen, chunk->chunk_rresid);
	chunk->chunk_rresid -= len;

	while (len--) {
		b = bus_space_read_1(sc->sc_bst, sc->sc_bsh, SPI_RXDR);
		if (chunk->chunk_rptr) {
			*chunk->chunk_rptr++ = b;
		}
	}
	if (sc->sc_rchunk->chunk_rresid == 0) {
		sc->sc_rchunk = sc->sc_rchunk->chunk_next;
	}
}

static void
rk_spi_rxtx(struct rk_spi_softc * const sc)
{
	bool again;
	uint32_t reg;
	size_t avail;

	/* Service both FIFOs until no more progress can be made. */
	again = true;
	while (again) {
		again = false;
		reg = SPIREG_READ(sc, SPI_RXFLR);
		avail = __SHIFTOUT(reg, SPI_RXFLR_RXFLR);
		if (avail > 0) {
			KASSERT(sc->sc_rchunk != NULL);
			rk_spi_rxfifo_drain(sc, avail);
			again = true;
		}
		reg = SPIREG_READ(sc, SPI_TXFLR);
		avail = SPI_FIFOLEN - __SHIFTOUT(reg, SPI_TXFLR_TXFLR);
		if (avail > 0 && sc->sc_wchunk != NULL) {
			rk_spi_txfifo_fill(sc, avail);
			again = true;
		}
	}
}

static void
rk_spi_set_interrupt_mask(struct rk_spi_softc * const sc)
{
	uint32_t imr = SPI_IMR_RFOIM | SPI_IMR_RFUIM | SPI_IMR_TFOIM;
	int len;

	/*
	 * Delay rx interrupts until the FIFO has the # of bytes we'd
	 * ideally like to receive, or FIFO is half full.
	 */
	len = sc->sc_rchunk != NULL
	    ? MIN(sc->sc_rchunk->chunk_rresid, SPI_FIFOLEN / 2) : 0;
	if (len > 0) {
		SPIREG_WRITE(sc, SPI_RXFTLR, len - 1);
		imr |= SPI_IMR_RFFIM;
	}

	/*
	 * Delay tx interrupts until the FIFO can accept the # of bytes we'd
	 * ideally like to transmit, or the FIFO is half empty.
	 */
	len = sc->sc_wchunk != NULL
	    ? MIN(sc->sc_wchunk->chunk_wresid, SPI_FIFOLEN / 2) : 0;
	if (len > 0) {
		SPIREG_WRITE(sc, SPI_TXFTLR, SPI_FIFOLEN - len);
		imr |= SPI_IMR_TFEIM;
	}

	/* If xfer is done, then interrupt as soon as the tx fifo is empty. */
	if (!ISSET(imr, (SPI_IMR_RFFIM | SPI_IMR_TFEIM))) {
		SPIREG_WRITE(sc, SPI_TXFTLR, 0);
		imr |= SPI_IMR_TFEIM;
	}

	SPIREG_WRITE(sc, SPI_IMR, imr);
}

static void
rk_spi_start(struct rk_spi_softc * const sc)
{
	struct spi_transfer *st;

	while ((st = spi_transq_first(&sc->sc_q)) != NULL) {
		spi_transq_dequeue(&sc->sc_q);
		KASSERT(sc->sc_transfer == NULL);
		sc->sc_transfer = st;
		sc->sc_rchunk = sc->sc_wchunk = st->st_chunks;
		sc->sc_running = true;

		KASSERT(st->st_slave < sc->sc_spi.sct_nslaves);
		SPIREG_WRITE(sc, SPI_SER, 1 << st->st_slave);

		rk_spi_rxtx(sc);
		rk_spi_set_interrupt_mask(sc);

		if (!cold)
			return;

		for (;;) {
			(void) rk_spi_intr(sc);
			if (ISSET(st->st_flags, SPI_F_DONE))
				break;
		}
	}
	sc->sc_running = false;
}

static int
rk_spi_intr(void *cookie)
{
	struct rk_spi_softc * const sc = cookie;
	struct spi_transfer *st;
	uint32_t isr;
	uint32_t sr;
	uint32_t icr = SPI_ICR_CCI;

	isr = SPIREG_READ(sc, SPI_ISR);
	if (!isr)
		return 0;

	if (ISSET(isr, SPI_ISR_RFOIS)) {
		device_printf(sc->sc_dev, "RXFIFO overflow\n");
		icr |= SPI_ICR_CRFOI;
	}
	if (ISSET(isr, SPI_ISR_RFUIS)) {
		device_printf(sc->sc_dev, "RXFIFO underflow\n");
		icr |= SPI_ICR_CRFUI;
	}
	if (ISSET(isr, SPI_ISR_TFOIS)) {
		device_printf(sc->sc_dev, "TXFIFO overflow\n");
		icr |= SPI_ICR_CTFOI;
	}

	rk_spi_rxtx(sc);

	if (sc->sc_rchunk == NULL && sc->sc_wchunk == NULL) {
		do {
			sr = SPIREG_READ(sc, SPI_SR);
		} while (ISSET(sr, SPI_SR_BSF));
		SPIREG_WRITE(sc, SPI_IMR, 0);
		SPIREG_WRITE(sc, SPI_SER, 0);
		st = sc->sc_transfer;
		sc->sc_transfer = NULL;
		KASSERT(st != NULL);
		spi_done(st, 0);
		sc->sc_running = false;
	} else {
		rk_spi_set_interrupt_mask(sc);
	}

	SPIREG_WRITE(sc, SPI_ICR, icr);

	return 1;
}
