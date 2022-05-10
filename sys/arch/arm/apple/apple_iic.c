/*	$NetBSD: apple_iic.c,v 1.1 2022/05/10 08:05:32 skrll Exp $	*/
/*	$OpenBSD: apliic.c,v 1.3 2022/02/14 14:55:53 kettenis Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
 * Copyright (c) 2021 Patrick Wildt <patrick@blueri.se>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/device.h>

#include <dev/i2c/i2cvar.h>

#include <dev/fdt/fdtvar.h>

/* Registers. */
#define I2C_MTXFIFO		0x00
#define  I2C_MTXFIFO_DATA_MASK		__BITS(7,0)
#define  I2C_MTXFIFO_START		__BIT(8)
#define  I2C_MTXFIFO_STOP		__BIT(9)
#define  I2C_MTXFIFO_READ		__BIT(10)
#define I2C_MRXFIFO		0x04
#define  I2C_MRXFIFO_DATA_MASK		__BITS(7,0)
#define  I2C_MRXFIFO_EMPTY		__BIT(8)
#define I2C_SMSTA		0x14
#define  I2C_SMSTA_MTN			__BIT(21)
#define  I2C_SMSTA_XEN			__BIT(27)
#define  I2C_SMSTA_XBUSY		__BIT(28)
#define I2C_CTL			0x1c
#define  I2C_CTL_CLK_MASK		__BITS(7,0)
#define  I2C_CTL_MTR			__BIT(9)
#define  I2C_CTL_MRR			__BIT(10)
#define  I2C_CTL_EN			__BIT(11)
#define I2C_REV			0x28

#define IIC_READ(sc, reg)						\
	(bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg)))
#define IIC_WRITE(sc, reg, val)						\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define HSET4(sc, reg, bits)						\
	IIC_WRITE((sc), (reg), IIC_READ((sc), (reg)) | (bits))
#define HCLR4(sc, reg, bits)						\
	IIC_WRITE((sc), (reg), IIC_READ((sc), (reg)) & ~(bits))

struct apple_iic_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct clk *		sc_clk;
	int			sc_hwrev;
	uint32_t		sc_clkdiv;
	struct i2c_controller	sc_i2c;
};


static int
apple_iic_acquire_bus(void *cookie, int flags)
{
	return 0;
}

static void
apple_iic_release_bus(void *cookie, int flags)
{
}

static int
apple_iic_exec(void *cookie, i2c_op_t op, i2c_addr_t addr, const void *cmd,
    size_t cmdlen, void *buf, size_t buflen, int flags)
{
	struct apple_iic_softc *sc = cookie;
	const uint8_t * const cmdbytes = cmd;
	uint8_t * const bufbytes = buf;
	uint32_t reg;
	int i;

	if (!I2C_OP_STOP_P(op))
		return EINVAL;

	IIC_WRITE(sc, I2C_SMSTA, 0xffffffff);

	if (cmdlen > 0) {
		IIC_WRITE(sc, I2C_MTXFIFO, I2C_MTXFIFO_START | addr << 1);
		for (i = 0; i < cmdlen - 1; i++)
			IIC_WRITE(sc, I2C_MTXFIFO, cmdbytes[i]);
		IIC_WRITE(sc, I2C_MTXFIFO, cmdbytes[cmdlen - 1] |
		    (buflen == 0 ? I2C_MTXFIFO_STOP : 0));
	}

	if (buflen == 0)
		return 0;

	if (I2C_OP_READ_P(op)) {
		IIC_WRITE(sc, I2C_MTXFIFO, I2C_MTXFIFO_START | addr << 1 | 1);
		IIC_WRITE(sc, I2C_MTXFIFO, I2C_MTXFIFO_READ | buflen |
		    I2C_MTXFIFO_STOP);
		for (i = 10; i > 0; i--) {
			delay(1000);
			reg = IIC_READ(sc, I2C_SMSTA);
			if (reg & I2C_SMSTA_XEN)
				break;
		}
		if (reg & I2C_SMSTA_MTN)
			return ENXIO;
		if (i == 0)
			return ETIMEDOUT;
		IIC_WRITE(sc, I2C_SMSTA, I2C_SMSTA_XEN);
		for (i = 0; i < buflen; i++) {
			reg = IIC_READ(sc, I2C_MRXFIFO);
			if (reg & I2C_MRXFIFO_EMPTY)
				return EIO;
			bufbytes[i] = reg & I2C_MRXFIFO_DATA_MASK;
		}
	} else {
		if (cmdlen == 0)
			IIC_WRITE(sc, I2C_MTXFIFO, I2C_MTXFIFO_START | addr << 1);
		for (i = 0; i < buflen - 1; i++)
			IIC_WRITE(sc, I2C_MTXFIFO, bufbytes[i]);
		IIC_WRITE(sc, I2C_MTXFIFO, bufbytes[buflen - 1] |
		    I2C_MTXFIFO_STOP);
	}

	return 0;
}


static const struct device_compatible_entry compat_data[] = {
	{ .compat = "apple,i2c" },
	DEVICE_COMPAT_EOL
};

static int
apple_iic_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
apple_iic_attach(device_t parent, device_t self, void *aux)
{
	struct apple_iic_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	uint32_t clock_speed, bus_speed;
	bus_addr_t addr;
	bus_size_t size;

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;

	int error = fdtbus_get_reg(phandle, 0, &addr, &size);
	if (error) {
		aprint_error(": unable to get device registers\n");
		return;
	}

	/* Enable clock */
	sc->sc_clk = fdtbus_clock_get_index(phandle, 0);
	if (sc->sc_clk == NULL) {
		aprint_error(": couldn't acquire clock\n");
		return;
	}

	if (clk_enable(sc->sc_clk) != 0) {
		aprint_error(": failed to enable clock\n");
		return;
	}

	clock_speed = clk_get_rate(sc->sc_clk);

	if (of_getprop_uint32(phandle, "clock-frequency",
	    &bus_speed) != 0) {
		bus_speed = 100000;
	}
	bus_speed *= 16;

	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh)) {
		aprint_error(": unable to map device\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Apple I2C\n");

	sc->sc_clkdiv = howmany(clock_speed, bus_speed);
	KASSERT(sc->sc_clkdiv <= __SHIFTOUT_MASK(I2C_CTL_CLK_MASK));

	sc->sc_hwrev = IIC_READ(sc, I2C_REV);

	IIC_WRITE(sc, I2C_CTL,
	    __SHIFTIN(sc->sc_clkdiv, I2C_CTL_CLK_MASK) |
	    I2C_CTL_MTR | I2C_CTL_MRR |
	    (sc->sc_hwrev >= 6 ? I2C_CTL_EN : 0));

	iic_tag_init(&sc->sc_i2c);
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = apple_iic_acquire_bus;
	sc->sc_i2c.ic_release_bus = apple_iic_release_bus;
	sc->sc_i2c.ic_exec = apple_iic_exec;

	fdtbus_register_i2c_controller(&sc->sc_i2c, phandle);

	fdtbus_attach_i2cbus(self, phandle, &sc->sc_i2c, iicbus_print);
}


CFATTACH_DECL_NEW(apple_iic, sizeof(struct apple_iic_softc),
    apple_iic_match, apple_iic_attach, NULL, NULL);
