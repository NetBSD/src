/* $NetBSD: meson_i2c.c,v 1.1 2025/07/05 19:28:10 rjs Exp $ */

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Written by Vincent Defert for The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(1, "$NetBSD: meson_i2c.c,v 1.1 2025/07/05 19:28:10 rjs Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <dev/i2c/i2cvar.h>
#include <dev/fdt/fdtvar.h>

#define MESONI2C_CTRL_REG		0x00
#define MESONI2C_CTRL_START		__BIT(0)
#define MESONI2C_CTRL_ACK_IGNORE	__BIT(1)
#define MESONI2C_CTRL_STATUS		__BIT(2)
#define MESONI2C_CTRL_ERROR		__BIT(3)
#define MESONI2C_CTRL_READ_DATA_SHIFT	8
#define MESONI2C_CTRL_READ_DATA_MASK	__BITS(11, MESONI2C_CTRL_READ_DATA_SHIFT)
#define MESONI2C_CTRL_CLKDIV_SHIFT	12
#define MESONI2C_CTRL_CLKDIV_MASK	__BITS(21, MESONI2C_CTRL_CLKDIV_SHIFT)
#define MESONI2C_CTRL_CLKDIVEXT_SHIFT	28
#define MESONI2C_CTRL_CLKDIVEXT_MASK	__BITS(29, MESONI2C_CTRL_CLKDIVEXT_SHIFT)

#define MESONI2C_SLAVE_ADDR_REG		0x04
#define MESONI2C_SLAVE_ADDR_MASK	__BITS(7, 0)
#define MESONI2C_SLAVE_SDA_FILTER_MASK	__BITS(10, 8)
#define MESONI2C_SLAVE_SCL_FILTER_MASK	__BITS(13, 11)
#define MESONI2C_SLAVE_SCL_LOW_SHIFT	16
#define MESONI2C_SLAVE_SCL_LOW_MASK	__BITS(27, MESONI2C_SLAVE_SCL_LOW_SHIFT)
#define MESONI2C_SLAVE_SCL_LOW_EN	__BIT(28)

#define MESONI2C_TOKEN_LIST_REG0	0x08
#define MESONI2C_TOKEN_LIST_REG1	0x0c
#define MESONI2C_TOKEN_WDATA_REG0	0x10
#define MESONI2C_TOKEN_WDATA_REG1	0x14
#define MESONI2C_TOKEN_RDATA_REG0	0x18
#define MESONI2C_TOKEN_RDATA_REG1	0x1c

#define MESONI2C_TOKEN_BITS		4
#define MESONI2C_TOKEN_MASK		((1 << MESONI2C_TOKEN_BITS) - 1)
#define MESONI2C_TOKEN_REG_HALF		(32 / MESONI2C_TOKEN_BITS)
#define MESONI2C_TOKEN_REG_FULL		(64 / MESONI2C_TOKEN_BITS)

#define MESONI2C_DATA_BITS		8
#define MESONI2C_DATA_MASK		((1 << MESONI2C_DATA_BITS) - 1)
#define MESONI2C_DATA_REG_HALF		(32 / MESONI2C_DATA_BITS)
#define MESONI2C_DATA_REG_FULL		(64 / MESONI2C_DATA_BITS)

#define I2C_TIMEOUT_MS			1000
#define FILTER_DELAY			15

enum mesoni2c_token {
	TOKEN_END = 0,
	TOKEN_START,
	TOKEN_SLAVE_ADDR_WRITE,
	TOKEN_SLAVE_ADDR_READ,
	TOKEN_DATA,
	TOKEN_DATA_LAST,
	TOKEN_STOP,
};

enum mesoni2c_type {
	TYPE_MESON6,
	TYPE_GXBB,
	TYPE_AXG,
};

struct mesoni2c_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	struct clk		*sc_clk;
	u_int			sc_clkfreq;
	struct i2c_controller	sc_ic;
	kcondvar_t		sc_cv;
	kmutex_t		sc_mtx;
	void			*sc_ih;

	u_int			sc_token_index;
	u_int			sc_wdata_index;
	u_int			sc_rdata_index;

	const uint8_t		*sc_cmdbuf;
	size_t			sc_cmdlen;
	uint8_t			*sc_databuf;
	size_t			sc_datalen;
	i2c_op_t		sc_op;

	size_t			sc_curlen;
	i2c_op_t		sc_curop;
	int			sc_sendingCmd;
	int			sc_error;
};

static void mesoni2c_set_mask(struct mesoni2c_softc *sc, bus_size_t reg, uint32_t mask, uint32_t value);
static void mesoni2c_reset_state(struct mesoni2c_softc *sc);
static int mesoni2c_push_token(struct mesoni2c_softc *sc, enum mesoni2c_token token);
static void mesoni2c_write_byte(struct mesoni2c_softc *sc, uint8_t data);
static uint8_t mesoni2c_get_byte(struct mesoni2c_softc *sc);
static void mesoni2c_prepare_xfer(struct mesoni2c_softc *sc, i2c_op_t op);
static void mesoni2c_partial_xfer(struct mesoni2c_softc *sc);
static int mesoni2c_exec(void *priv, i2c_op_t op, i2c_addr_t addr, const void *cmdbuf, size_t cmdlen, void *buf, size_t buflen, int flags);
static int mesoni2c_intr(void *arg);
static void mesoni2c_set_clock_div_meson6(struct mesoni2c_softc *sc);
static void mesoni2c_set_clock_div_gxbb_axg(struct mesoni2c_softc *sc);
static int mesoni2c_match(device_t parent, cfdata_t cf, void *aux);
static void mesoni2c_attach(device_t parent, device_t self, void *aux);

CFATTACH_DECL_NEW(meson_i2c, sizeof(struct mesoni2c_softc),
    mesoni2c_match, mesoni2c_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "amlogic,meson6-i2c",	.value = TYPE_MESON6 },
	{ .compat = "amlogic,meson-gxbb-i2c",	.value = TYPE_GXBB },
	{ .compat = "amlogic,meson-axg-i2c",	.value = TYPE_AXG },
	DEVICE_COMPAT_EOL
};

#define	RD4(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))

#define	WR4(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static void
mesoni2c_set_mask(struct mesoni2c_softc *sc, bus_size_t reg, uint32_t mask,
	uint32_t value)
{
	uint32_t data = RD4(sc, reg);
	data &= ~mask;
	data |= value & mask;
	WR4(sc, reg, data);
}

static void
mesoni2c_reset_state(struct mesoni2c_softc *sc)
{
	sc->sc_token_index = 0;
	sc->sc_wdata_index = 0;
	sc->sc_rdata_index = 0;
}

static int
mesoni2c_push_token(struct mesoni2c_softc *sc, enum mesoni2c_token token)
{
	bus_size_t reg;
	u_int pos;

	if (sc->sc_token_index >= MESONI2C_TOKEN_REG_HALF) {
		reg = MESONI2C_TOKEN_LIST_REG1;
		pos = sc->sc_token_index - MESONI2C_TOKEN_REG_HALF;
	} else {
		reg = MESONI2C_TOKEN_LIST_REG0;
		pos = sc->sc_token_index;
	}

	sc->sc_token_index++;
	pos *= MESONI2C_TOKEN_BITS;
	mesoni2c_set_mask(sc, reg, MESONI2C_TOKEN_MASK << pos, token << pos);

	return sc->sc_token_index == MESONI2C_TOKEN_REG_FULL;
}

static void
mesoni2c_write_byte(struct mesoni2c_softc *sc, uint8_t data)
{
	bus_size_t reg;
	u_int pos;

	if (sc->sc_wdata_index >= MESONI2C_DATA_REG_HALF) {
		reg = MESONI2C_TOKEN_WDATA_REG1;
		pos = sc->sc_wdata_index - MESONI2C_DATA_REG_HALF;
	} else {
		reg = MESONI2C_TOKEN_WDATA_REG0;
		pos = sc->sc_wdata_index;
	}

	sc->sc_wdata_index++;
	pos *= MESONI2C_DATA_BITS;
	mesoni2c_set_mask(sc, reg, MESONI2C_DATA_MASK << pos, ((uint32_t) data) << pos);
	mesoni2c_push_token(sc, TOKEN_DATA);
}

static uint8_t
mesoni2c_get_byte(struct mesoni2c_softc *sc)
{
	bus_size_t reg;
	u_int pos;

	if (sc->sc_rdata_index >= MESONI2C_DATA_REG_HALF) {
		reg = MESONI2C_TOKEN_RDATA_REG1;
		pos = sc->sc_rdata_index - MESONI2C_DATA_REG_HALF;
	} else {
		reg = MESONI2C_TOKEN_RDATA_REG0;
		pos = sc->sc_rdata_index;
	}

	sc->sc_rdata_index++;
	pos *= MESONI2C_DATA_BITS;

	return (RD4(sc, reg) >> pos) & MESONI2C_DATA_MASK;
}

static void
mesoni2c_prepare_xfer(struct mesoni2c_softc *sc, i2c_op_t op)
{
	mesoni2c_reset_state(sc);
	mesoni2c_push_token(sc, TOKEN_START);
	mesoni2c_push_token(sc, I2C_OP_WRITE_P(op) ? TOKEN_SLAVE_ADDR_WRITE : TOKEN_SLAVE_ADDR_READ);
}

static void
mesoni2c_partial_xfer(struct mesoni2c_softc *sc)
{
	int dataBufferFree = MESONI2C_DATA_REG_FULL;

	while (sc->sc_curlen && dataBufferFree) {
		sc->sc_curlen--;
		dataBufferFree--;

		if (I2C_OP_WRITE_P(sc->sc_curop)) {
			if (sc->sc_sendingCmd) {
				uint8_t c = *(sc->sc_cmdbuf++);
				mesoni2c_write_byte(sc, c);
			} else {
				mesoni2c_write_byte(sc, *(sc->sc_databuf++));
			}
		} else {
			mesoni2c_push_token(sc, sc->sc_curlen ? TOKEN_DATA : TOKEN_DATA_LAST);
		}
	}

	if (sc->sc_curlen == 0 && I2C_OP_STOP_P(sc->sc_curop)) {
		mesoni2c_push_token(sc, TOKEN_STOP);
	}

	mesoni2c_push_token(sc, TOKEN_END);

	mesoni2c_set_mask(sc, MESONI2C_CTRL_REG, MESONI2C_CTRL_START, 0);
	mesoni2c_set_mask(sc, MESONI2C_CTRL_REG, MESONI2C_CTRL_START, 1);
}

static int
mesoni2c_exec(void *priv, i2c_op_t op, i2c_addr_t addr, const void *cmdbuf,
	size_t cmdlen, void *databuf, size_t datalen, int flags)
{
	struct mesoni2c_softc * const sc = priv;

	mutex_enter(&sc->sc_mtx);

	sc->sc_cmdbuf = cmdbuf;
	sc->sc_cmdlen = cmdlen;
	sc->sc_databuf = databuf;
	sc->sc_datalen = datalen;
	sc->sc_op = op;
	sc->sc_error = 0;

	mesoni2c_set_mask(sc, MESONI2C_SLAVE_ADDR_REG, MESONI2C_SLAVE_ADDR_MASK, addr << 1);
	
	if (cmdlen) {
		sc->sc_curlen = cmdlen;
		sc->sc_curop = datalen ? I2C_OP_WRITE : op;
		sc->sc_sendingCmd = 1;
	} else {
		sc->sc_curlen = datalen;
		sc->sc_curop = op;
		sc->sc_sendingCmd = 0;
	}

	mesoni2c_prepare_xfer(sc, sc->sc_curop);
	mesoni2c_partial_xfer(sc);

	if (cv_timedwait(&sc->sc_cv, &sc->sc_mtx, mstohz(I2C_TIMEOUT_MS)) == EWOULDBLOCK) {
		sc->sc_error = EIO;
	}

	mutex_exit(&sc->sc_mtx);

	return sc->sc_error;
}

static int
mesoni2c_intr(void *arg)
{
	struct mesoni2c_softc *sc = arg;
	int done = 0;

	mutex_enter(&sc->sc_mtx);

	if (RD4(sc, MESONI2C_CTRL_REG) & MESONI2C_CTRL_ERROR) {
		/*
		 * The ERROR bit is set when the ACK_IGNORE bit is cleared
		 * in MESONI2C_CTRL_REG and the device didn't respond with
		 * an ACK. In this case, the I2C controller automatically
		 * generates a STOP condition.
		 */
		sc->sc_error = EIO;
		done = 1;
	} else {
		if (I2C_OP_READ_P(sc->sc_curop)) {
			/* Read data bytes */
			u_int count = (RD4(sc, MESONI2C_CTRL_REG) & MESONI2C_CTRL_READ_DATA_MASK)
				>> MESONI2C_CTRL_READ_DATA_SHIFT;
			
			while (count--) {
				*(sc->sc_databuf++) = mesoni2c_get_byte(sc);
			}
		}

		if (sc->sc_curlen) {
			/* Continue transfer */
			mesoni2c_reset_state(sc);
			mesoni2c_partial_xfer(sc);
		} else {
			if (sc->sc_sendingCmd && sc->sc_datalen) {
				/*
				 * We've just finished transfering the command
				 * bytes, we must now tranfer the data.
				 */
				sc->sc_curlen = sc->sc_datalen;
				sc->sc_curop = sc->sc_op;
				sc->sc_sendingCmd = 0;

				if (I2C_OP_READ_P(sc->sc_curop)) {
					mesoni2c_prepare_xfer(sc, sc->sc_curop);
				} else {
					mesoni2c_reset_state(sc);
				}

				mesoni2c_partial_xfer(sc);
			} else {
				done = 1;
			}
		}
	}

	if (done) {
		/* Tell mesoni2c_exec() we're done. */
		cv_broadcast(&sc->sc_cv);
	}

	mutex_exit(&sc->sc_mtx);

	return 1;
}

static void
mesoni2c_set_clock_div_meson6(struct mesoni2c_softc *sc)
{
	u_int rate = clk_get_rate(sc->sc_clk);
	u_int div = howmany(rate, sc->sc_clkfreq) - FILTER_DELAY;
	div = howmany(div, 4);

	/* Set prescaler */
	mesoni2c_set_mask(sc, MESONI2C_CTRL_REG, MESONI2C_CTRL_CLKDIV_MASK, (div & __BITS(9, 0)) << MESONI2C_CTRL_CLKDIV_SHIFT);
	mesoni2c_set_mask(sc, MESONI2C_CTRL_REG, MESONI2C_CTRL_CLKDIVEXT_MASK, (div >> 10) << MESONI2C_CTRL_CLKDIVEXT_SHIFT);

	/* Disable HIGH/LOW mode */
	mesoni2c_set_mask(sc, MESONI2C_SLAVE_ADDR_REG, MESONI2C_SLAVE_SCL_LOW_EN, 0);
}

static void
mesoni2c_set_clock_div_gxbb_axg(struct mesoni2c_softc *sc)
{
	u_int rate = clk_get_rate(sc->sc_clk);
	u_int divh, divl;

	/*
	 * According to I2C-BUS Spec 2.1, in FAST-MODE, the minimum
	 * LOW period is 1.3uS, and minimum HIGH is least 0.6us.
	 * For
	 * 400000 freq, the period is 2.5us. To keep within the specs,
	 * give 40% of period to HIGH and 60% to LOW. This means HIGH
	 * at 1.0us and LOW 1.5us.
	 * The same applies for Fast-mode plus, where LOW is 0.5us and
	 * HIGH is 0.26us.
	 * Duty = H/(H + L) = 2/5
	 */
	if (sc->sc_clkfreq <= 100000) {
		divh = howmany(rate, sc->sc_clkfreq);
		divl = howmany(divh, 4);
		divh = howmany(divh, 2) - FILTER_DELAY;
	} else {
		divh = howmany(rate * 2, sc->sc_clkfreq * 5) - FILTER_DELAY;
		divl = howmany(rate * 3, sc->sc_clkfreq * 5 * 2);
	}

	/* Set prescaler */
	mesoni2c_set_mask(sc, MESONI2C_CTRL_REG, MESONI2C_CTRL_CLKDIV_MASK, (divh & __BITS(9, 0)) << MESONI2C_CTRL_CLKDIV_SHIFT);
	mesoni2c_set_mask(sc, MESONI2C_CTRL_REG, MESONI2C_CTRL_CLKDIVEXT_MASK, (divh >> 10) << MESONI2C_CTRL_CLKDIVEXT_SHIFT);

	/* Set SCL low delay */
	mesoni2c_set_mask(sc, MESONI2C_SLAVE_ADDR_REG, MESONI2C_SLAVE_SCL_LOW_MASK, divl << MESONI2C_SLAVE_SCL_LOW_SHIFT);

	/* Enable HIGH/LOW mode */
	mesoni2c_set_mask(sc, MESONI2C_SLAVE_ADDR_REG, MESONI2C_SLAVE_SCL_LOW_EN, MESONI2C_SLAVE_SCL_LOW_EN);
}

static int
mesoni2c_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
mesoni2c_attach(device_t parent, device_t self, void *aux)
{
	struct mesoni2c_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	char intrstr[128];

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error_dev(self, "couldn't get registers\n");
		return;
	}

	sc->sc_clk = fdtbus_clock_get_index(phandle, 0);
	
	if (sc->sc_clk == NULL || clk_enable(sc->sc_clk) != 0) {
		aprint_error_dev(self, "couldn't enable clock\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;

	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error_dev(self, "couldn't map registers\n");
		return;
	}

	fdtbus_clock_assign(phandle);

	if (of_getprop_uint32(phandle, "clock-frequency", &sc->sc_clkfreq)) {
		sc->sc_clkfreq = 100000;
	} else {
		if (sc->sc_clkfreq < 100000) {
			sc->sc_clkfreq = 100000;
		} else if (sc->sc_clkfreq > 400000) {
			sc->sc_clkfreq = 400000;
		}
	}

	aprint_naive("\n");
	aprint_normal(": Meson I2C (%u Hz)\n", sc->sc_clkfreq);

	enum mesoni2c_type type = of_compatible_lookup(phandle, compat_data)->value;
	
	switch (type) {
	case TYPE_MESON6:
		mesoni2c_set_clock_div_meson6(sc);
		break;

	case TYPE_GXBB:
	case TYPE_AXG:
		mesoni2c_set_clock_div_gxbb_axg(sc);
		break;
	}

	mutex_init(&sc->sc_mtx, MUTEX_DEFAULT, IPL_VM);
	cv_init(&sc->sc_cv, "mesoniic");

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish_xname(phandle, 0, IPL_VM, 0,
		mesoni2c_intr, sc, device_xname(self));

	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		return;
	}

	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	iic_tag_init(&sc->sc_ic);
	sc->sc_ic.ic_cookie = sc;
	sc->sc_ic.ic_exec = mesoni2c_exec;
	fdtbus_register_i2c_controller(&sc->sc_ic, phandle);

	fdtbus_attach_i2cbus(self, phandle, &sc->sc_ic, iicbus_print);
}
