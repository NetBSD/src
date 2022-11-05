/* $NetBSD: cdnsiic.c,v 1.1 2022/11/05 17:31:37 jmcneill Exp $ */

/*-
 * Copyright (c) 2022 Jared McNeill <jmcneill@invisible.ca>
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

/*
 * Cadence I2C controller
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: cdnsiic.c,v 1.1 2022/11/05 17:31:37 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kmem.h>

#include <dev/clk/clk.h>
#include <dev/i2c/i2cvar.h>

#include <dev/ic/cdnsiicvar.h>

/* From Zynq-7000 SoC Technical Reference Manual, "Supports 16-byte FIFO" */
#define	FIFO_DEPTH	16

/* Poll timeout, in microseconds. */
#define	POLL_TIMEOUT	10000

#define	CR_REG		0x00
#define	 CR_DIV_A	__BITS(15,14)
#define	 CR_DIV_B	__BITS(13,8)
#define	 CR_CLR_FIFO	__BIT(6)
#define	 CR_HOLD	__BIT(4)
#define	 CR_ACKEN	__BIT(3)
#define	 CR_NEA		__BIT(2)
#define	 CR_MS		__BIT(1)
#define	 CR_RD_WR	__BIT(0)
#define	SR_REG		0x04
#define	 SR_TXDV	__BIT(6)
#define	 SR_RXDV	__BIT(5)
#define	ADDR_REG	0x08
#define	DATA_REG	0x0c
#define	ISR_REG		0x10
#define	 ISR_ARB_LOST	__BIT(9)
#define	 ISR_RX_UNF	__BIT(7)
#define	 ISR_TX_OVR	__BIT(6)
#define	 ISR_RX_OVR	__BIT(5)
#define	 ISR_SLV_RDY	__BIT(4)
#define	 ISR_TO		__BIT(3)
#define	 ISR_NACK	__BIT(2)
#define	 ISR_DATA	__BIT(1)
#define	 ISR_COMP	__BIT(0)
#define	 ISR_ERROR_MASK	(ISR_ARB_LOST | ISR_TX_OVR | ISR_RX_OVR | ISR_NACK)
#define	TRANS_SIZE_REG	0x14
#define	SLV_PAUSE_REG	0x18
#define	TIME_OUT_REG	0x1c
#define	IMR_REG		0x20
#define	IER_REG		0x24
#define	IDR_REG		0x28

#define	RD4(sc, reg)				\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)			\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
cdnsiic_init(struct cdnsiic_softc *sc)
{
	int diva, divb;
	int diff, calc_bus_freq;
	int best_diva, best_divb, best_diff;
	u_int pclk;

	/*
	 * SCL frequency is calculated by the following formula:
	 *
	 * SCL Divisor = 22 * (divisor_a + 1) * (divisor_b + 1)
	 * SCL = PCLK / SCLK Divisor
	 */

	pclk = clk_get_rate(sc->sc_pclk);
	best_diff = sc->sc_bus_freq;
	best_diva = best_divb = 0;

	for (diva = 0; diva <= 0x3; diva++) {
		divb = howmany(pclk, 22 * sc->sc_bus_freq * (diva + 1)) - 1;
		if (divb < 0 || divb > 0x3f) {
			continue;
		}
		calc_bus_freq = pclk / (22 * (diva + 1) * (divb + 1));
		diff = sc->sc_bus_freq - calc_bus_freq;
		if (diff < best_diff) {
			best_diff = diff;
			best_diva = diva;
			best_divb = divb;
		}
	}
	if (best_diff == sc->sc_bus_freq) {
		return ENXIO;
	}

	WR4(sc, CR_REG,
	    __SHIFTIN(best_diva, CR_DIV_A) |
	    __SHIFTIN(best_divb, CR_DIV_B) |
	    CR_CLR_FIFO |
	    CR_ACKEN |
	    CR_NEA |
	    CR_MS);
	WR4(sc, TIME_OUT_REG, 0xff);

	return 0;
}

static int
cdnsiic_poll_fifo(struct cdnsiic_softc *sc, uint32_t sr_mask, uint32_t sr_maskval)
{
	uint32_t sr_val, isr_val;
	int retry = POLL_TIMEOUT;

	while (--retry > 0) {
		sr_val = RD4(sc, SR_REG);
		isr_val = RD4(sc, ISR_REG);
		if ((isr_val & ISR_ERROR_MASK) != 0) {
			return EIO;
		}
		if ((sr_val & sr_mask) == sr_maskval) {
			return 0;
		}
		delay(1);
	}

	return ETIMEDOUT;
}

static int
cdnsiic_poll_transfer_complete(struct cdnsiic_softc *sc)
{
	uint32_t val;
	int retry = POLL_TIMEOUT;

	while (--retry > 0) {
		val = RD4(sc, ISR_REG);
		if ((val & ISR_COMP) != 0) {
			return 0;
		}
		delay(1);
	}

	return ETIMEDOUT;
}

static int
cdnsiic_write(struct cdnsiic_softc *sc, i2c_addr_t addr,
    const uint8_t *data, size_t datalen, bool send_stop)
{
	uint32_t val;
	u_int xferlen, fifo_space, n;
	bool write_addr = true;
	int error;

	if (datalen == 0 || datalen > 256) {
		return EINVAL;
	}

	val = RD4(sc, CR_REG);
	val |= CR_CLR_FIFO;
	val &= ~CR_RD_WR;
	WR4(sc, CR_REG, val);
	WR4(sc, ISR_REG, RD4(sc, ISR_REG));

	while (datalen > 0) {
		fifo_space = FIFO_DEPTH - RD4(sc, TRANS_SIZE_REG);
		xferlen = uimin(datalen, fifo_space);
		for (n = 0; n < xferlen; n++, data++) {
			WR4(sc, DATA_REG, *data);
		}
		if (write_addr) {
			WR4(sc, ADDR_REG, addr);
			write_addr = false;
		}
		datalen -= xferlen;
		error = cdnsiic_poll_fifo(sc, SR_TXDV, 0);
		if (error != 0) {
			return error;
		}
	}

	return cdnsiic_poll_transfer_complete(sc);
}

static int
cdnsiic_read(struct cdnsiic_softc *sc, i2c_addr_t addr,
    uint8_t *data, size_t datalen)
{
	uint32_t val;
	int error;

	if (datalen == 0 || datalen > 255) {
		return EINVAL;
	}

	val = RD4(sc, CR_REG);
	val |= CR_CLR_FIFO | CR_RD_WR;
	WR4(sc, CR_REG, val);
	WR4(sc, ISR_REG, RD4(sc, ISR_REG));
	WR4(sc, TRANS_SIZE_REG, datalen);
	WR4(sc, ADDR_REG, addr);

	while (datalen > 0) {
		error = cdnsiic_poll_fifo(sc, SR_RXDV, SR_RXDV);
		if (error != 0) {
			return error;
		}
		*data = RD4(sc, DATA_REG) & 0xff;
		data++;
		datalen--;
	}

	return cdnsiic_poll_transfer_complete(sc);
}

static int
cdnsiic_exec(void *priv, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *buf, size_t buflen, int flags)
{
	struct cdnsiic_softc * const sc = priv;
	uint32_t val;
	int error;

	val = RD4(sc, CR_REG);
	WR4(sc, CR_REG, val | CR_HOLD);

	if (cmdlen > 0) {
		error = cdnsiic_write(sc, addr, cmdbuf, cmdlen, false);
		if (error != 0) {
			goto done;
		}
	}
	if (I2C_OP_READ_P(op)) {
		error = cdnsiic_read(sc, addr, buf, buflen);
	} else {
		error = cdnsiic_write(sc, addr, buf, buflen, true);
	}

done:
	val = RD4(sc, CR_REG);
	WR4(sc, CR_REG, val & ~CR_HOLD);

	return error;
}

int
cdnsiic_attach(struct cdnsiic_softc *sc)
{
	int error;

	aprint_naive("\n");
	aprint_normal(": Cadence I2C (%u Hz)\n", sc->sc_bus_freq);

	error = cdnsiic_init(sc);
	if (error != 0) {
		return error;
	}

	iic_tag_init(&sc->sc_ic);
	sc->sc_ic.ic_cookie = sc;
	sc->sc_ic.ic_exec = cdnsiic_exec;

	return 0;
}
