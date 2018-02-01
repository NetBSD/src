/*	$NetBSD: sun6i_spi.c,v 1.2 2018/02/01 14:50:36 jakllsch Exp $	*/

/*
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
__KERNEL_RCSID(0, "$NetBSD: sun6i_spi.c,v 1.2 2018/02/01 14:50:36 jakllsch Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/bus.h>
#include <sys/intr.h>
#include <sys/kernel.h>

#include <sys/bitops.h>
#include <dev/spi/spivar.h>

#include <arm/sunxi/sun6i_spireg.h>

#include <dev/fdt/fdtvar.h>

#include <arm/fdt/arm_fdtvar.h>

#define SPI_IER_DEFAULT (SPI_IER_TC_INT_EN | SPI_IER_TF_UDR_INT_EN | \
	SPI_IER_TF_OVF_INT_EN | SPI_IER_RF_UDR_INT_EN | SPI_IER_RF_OVF_INT_EN)

struct sun6ispi_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	void			*sc_intrh;
	struct spi_controller	sc_spi;
	SIMPLEQ_HEAD(,spi_transfer) sc_q;
	struct spi_transfer	*sc_transfer;
	struct spi_chunk	*sc_wchunk;
	struct spi_chunk	*sc_rchunk;
	uint32_t		sc_TCR;
	u_int			sc_modclkrate;
	volatile bool		sc_running;
};

static int sun6ispi_match(device_t, cfdata_t, void *);
static void sun6ispi_attach(device_t, device_t, void *);

static int sun6ispi_configure(void *, int, int, int);
static int sun6ispi_transfer(void *, struct spi_transfer *);

static void sun6ispi_start(struct sun6ispi_softc * const);
static int sun6ispi_intr(void *);

static void sun6ispi_send(struct sun6ispi_softc * const);
static void sun6ispi_recv(struct sun6ispi_softc * const);

CFATTACH_DECL_NEW(sun6i_spi, sizeof(struct sun6ispi_softc),
    sun6ispi_match, sun6ispi_attach, NULL, NULL);

static int
sun6ispi_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = {
		"allwinner,sun8i-h3-spi",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sun6ispi_attach(device_t parent, device_t self, void *aux)
{
	struct sun6ispi_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	struct spibus_attach_args sba;
	struct fdtbus_reset *rst;
	struct clk *clk, *modclk;
	uint32_t gcr, isr;
	char intrstr[128];

	aprint_naive("\n");
	aprint_normal(": SPI\n");

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;
	SIMPLEQ_INIT(&sc->sc_q);

	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error_dev(sc->sc_dev, "missing 'reg' property\n");
		return;
	}

	if (bus_space_map(sc->sc_iot, addr, size, 0, &sc->sc_ioh) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}

	if ((clk = fdtbus_clock_get_index(phandle, 0)) != NULL) {
		if (clk_enable(clk) != 0) {
			aprint_error_dev(sc->sc_dev, "couldn't enable clock\n");
			return;
		}
		device_printf(self, "%s ahb @ %uHz\n", __func__, clk_get_rate(clk));
	}

	if ((modclk = fdtbus_clock_get(phandle, "mod")) != NULL) {
		/* 200MHz max on H3,H5 */
		if (clk_set_rate(modclk, 200000000) != 0) {
			aprint_error_dev(sc->sc_dev, "couldn't configure module clock\n");
			return;
		}
		if (clk_enable(modclk) != 0) {
			aprint_error_dev(sc->sc_dev, "couldn't enable module clock\n");
			return;
		}
		sc->sc_modclkrate = clk_get_rate(modclk);
		device_printf(self, "%s %uHz\n", __func__, sc->sc_modclkrate);
	}

	if ((rst = fdtbus_reset_get_index(phandle, 0)) != NULL)
		if (fdtbus_reset_deassert(rst) != 0) {
			aprint_error(": couldn't de-assert reset\n");
			return;
		}

	isr = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_INT_STA);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_INT_STA, isr);

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	sc->sc_intrh = fdtbus_intr_establish(phandle, 0, IPL_VM, 0,
	    sun6ispi_intr, sc);
	if (sc->sc_intrh == NULL) {
		aprint_error_dev(sc->sc_dev, "unable to establish interrupt\n");
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	gcr = SPI_GCR_SRST;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_GCR, gcr);
	for (u_int i = 0; ; i++) {
		if (i >= 1000000) {
			aprint_error_dev(self, "reset timeout\n");
			return;
		}
		gcr = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_GCR);
		if ((gcr & SPI_GCR_SRST) == 0)
			break;
		else
			DELAY(1);
	}
	gcr = SPI_GCR_TP_EN | SPI_GCR_MODE | SPI_GCR_EN;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_GCR, gcr);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_IER, SPI_IER_DEFAULT);

	sc->sc_spi.sct_cookie = sc;
	sc->sc_spi.sct_configure = sun6ispi_configure;
	sc->sc_spi.sct_transfer = sun6ispi_transfer;
	sc->sc_spi.sct_nslaves = 4;

	sba.sba_controller = &sc->sc_spi;

	(void) config_found_ia(self, "spibus", &sba, spibus_print);
}

static int
sun6ispi_configure(void *cookie, int slave, int mode, int speed)
{
	struct sun6ispi_softc * const sc = cookie;
	uint32_t tcr, cctl;

#if 0
	device_printf(sc->sc_dev, "%s slave %d mode %d speed %d\n", __func__, slave, mode, speed);

	if (speed > 200000)
		speed = 200000;
#endif

	tcr = SPI_TCR_SS_LEVEL | SPI_TCR_SPOL;

	if (slave > 3)
		return EINVAL;

	if (speed <= 0)
		return EINVAL;

	switch (mode) {
	case SPI_MODE_0:
		tcr |= 0;
		break;
	case SPI_MODE_1:
		tcr |= SPI_TCR_CPHA;
		break;
	case SPI_MODE_2:
		tcr |= SPI_TCR_CPOL;
		break;
	case SPI_MODE_3:
		tcr |= SPI_TCR_CPHA|SPI_TCR_CPOL;
		break;
	default:
		return EINVAL;
	}

	sc->sc_TCR = tcr;

	if (speed < 3000 || speed > 200000000)
		return EINVAL;

	if (speed > sc->sc_modclkrate / 2 || speed < sc->sc_modclkrate / 512) {
		for (cctl = 0; cctl <= __SHIFTOUT_MASK(SPI_CCTL_CDR1); cctl++) {
			if ((sc->sc_modclkrate / (1<<cctl)) <= speed)
				goto cdr1_found;
		}
		return EINVAL;
cdr1_found:
		cctl = __SHIFTIN(cctl, SPI_CCTL_CDR1);
	} else {
		cctl = howmany(sc->sc_modclkrate, 2 * speed) - 1;
		cctl = SPI_CCTL_DRS|__SHIFTIN(cctl, SPI_CCTL_CDR2);
	}

	device_printf(sc->sc_dev, "%s CCTL 0x%08x\n", __func__, cctl);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_CCTL, cctl);

	return 0;
}

static int
sun6ispi_transfer(void *cookie, struct spi_transfer *st)
{
	struct sun6ispi_softc * const sc = cookie;
	int s;

	s = splbio();
	spi_transq_enqueue(&sc->sc_q, st);
	if (sc->sc_running == false) {
		sun6ispi_start(sc);
	}
	splx(s);
	return 0;
}

static size_t
chunks_total_count(struct spi_transfer * st)
{
	struct spi_chunk *chunk = st->st_chunks;
	size_t len = 0;

	do {
		len += chunk->chunk_count;
	} while ((chunk = chunk->chunk_next) != NULL);

	return len;
}

static void
sun6ispi_start(struct sun6ispi_softc * const sc)
{
	struct spi_transfer *st;
	uint32_t isr, tcr;
	size_t ctc;

	//device_printf(sc->sc_dev, "%s\n", __func__);
	while ((st = spi_transq_first(&sc->sc_q)) != NULL) {

		spi_transq_dequeue(&sc->sc_q);

		KASSERT(sc->sc_transfer == NULL);
		sc->sc_transfer = st;
		sc->sc_rchunk = sc->sc_wchunk = st->st_chunks;
		sc->sc_running = true;

		isr = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_INT_STA);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_INT_STA, isr);

		//printf("%s fcr 0x%08x\n", __func__, bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_FCR));

		ctc = chunks_total_count(st);
		//printf("%s total count %zu\n", __func__, ctc);

		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_BC, __SHIFTIN(ctc, SPI_BC_MBC));
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_TC, __SHIFTIN(ctc, SPI_TC_MWTC));
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_BCC, __SHIFTIN(ctc, SPI_BCC_STC));

		KASSERT(st->st_slave <= 3);
		tcr = sc->sc_TCR | __SHIFTIN(st->st_slave, SPI_TCR_SS_SEL);

		//device_printf(sc->sc_dev, "%s before TCR write\n", __func__);
		//bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_TCR, tcr);

		sun6ispi_send(sc);

		const uint32_t ier = SPI_IER_DEFAULT | SPI_IER_RF_RDY_INT_EN | SPI_IER_TX_ERQ_INT_EN;
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_IER, ier);

		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_TCR, tcr|SPI_TCR_XCH);
		//device_printf(sc->sc_dev, "%s after TCR write\n", __func__);

		if (!cold)
			return;

		//device_printf(sc->sc_dev, "%s cold\n", __func__);

		int s = splbio();
		for (;;) {
			sun6ispi_intr(sc);
			if (ISSET(st->st_flags, SPI_F_DONE))
				break;
		}
		splx(s);
	}

	sc->sc_running = false;
//	device_printf(sc->sc_dev, "%s finishes\n", __func__);
}

static void
sun6ispi_send(struct sun6ispi_softc * const sc)
{
	uint8_t fd;
	uint32_t fsr;
	struct spi_chunk *chunk;

	//device_printf(sc->sc_dev, "%s\n", __func__);

	while ((chunk = sc->sc_wchunk) != NULL) {
		while (chunk->chunk_wresid) {
			fsr = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_FSR);
			if (__SHIFTOUT(fsr, SPI_FSR_TF_CNT) >= 64) {
				//printf("%s fsr 0x%08x\n", __func__, fsr);
				return;
			}
			if (chunk->chunk_wptr) {
				fd = *chunk->chunk_wptr++;
			} else {
				fd = '\0';
			}
			//printf("%s fd %02x\n", __func__, fd);
			bus_space_write_1(sc->sc_iot, sc->sc_ioh, SPI_TXD, fd);
			chunk->chunk_wresid--;
		}
		sc->sc_wchunk = sc->sc_wchunk->chunk_next;
	}
}

static void
sun6ispi_recv(struct sun6ispi_softc * const sc)
{
	uint8_t fd;
	uint32_t fsr;
	struct spi_chunk *chunk;

	//device_printf(sc->sc_dev, "%s\n", __func__);

	while ((chunk = sc->sc_rchunk) != NULL) {
		while (chunk->chunk_rresid) {
			fsr = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_FSR);
			if (__SHIFTOUT(fsr, SPI_FSR_RF_CNT) == 0) {
				//printf("%s fsr 0x%08x\n", __func__, fsr);
				return;
			}
			fd = bus_space_read_1(sc->sc_iot, sc->sc_ioh, SPI_RXD);
			//printf("%s fd %02x\n", __func__, fd);
			if (chunk->chunk_rptr) {
				*chunk->chunk_rptr++ = fd;
			}
			chunk->chunk_rresid--;
		}
		sc->sc_rchunk = sc->sc_rchunk->chunk_next;
	}
}

static int
sun6ispi_intr(void *cookie)
{
	struct sun6ispi_softc * const sc = cookie;
	struct spi_transfer *st;
	uint32_t isr;

	isr = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SPI_INT_STA);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_INT_STA, isr);
	//device_printf(sc->sc_dev, "%s ISR 0x%08x\n", __func__, isr);

	if (ISSET(isr, SPI_ISR_RX_RDY)) {
		sun6ispi_recv(sc);
		sun6ispi_send(sc);
	}

	if (ISSET(isr, SPI_ISR_TC)) {
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SPI_IER, SPI_IER_DEFAULT);

		sc->sc_rchunk = sc->sc_wchunk = NULL;
		st = sc->sc_transfer;
		sc->sc_transfer = NULL;
		KASSERT(st != NULL);
		spi_done(st, 0);
		sc->sc_running = false;
	}

	return isr;
}
