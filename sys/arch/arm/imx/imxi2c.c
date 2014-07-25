/*	$NetBSD: imxi2c.c,v 1.1 2014/07/25 07:07:47 hkenken Exp $	*/

/*
 * Copyright (c) 2012  Genetec Corporation.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imxi2c.c,v 1.1 2014/07/25 07:07:47 hkenken Exp $");

#include "opt_imx.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/mutex.h>

#include <dev/i2c/i2cvar.h>
#include <arm/imx/imxi2creg.h>
#include <arm/imx/imxi2cvar.h>

#include <arm/imx/imx51_ccmvar.h>

#define I2C_READ(sc, reg)					      \
	bus_space_read_2((sc)->sc_iot, (sc)->sc_ioh, reg)
#define I2C_WRITE(sc, reg, val)					      \
	bus_space_write_2((sc)->sc_iot, (sc)->sc_ioh, reg, val)

#define	I2C_TIMEOUT		1000	/* protocol timeout, in uSecs */

static int imxi2c_i2c_acquire_bus(void *, int);
static void imxi2c_i2c_release_bus(void *, int);
static int imxi2c_i2c_exec(void *, u_int, u_int16_t, const void *, size_t, void *,
    size_t, int);

static int imxi2c_wait(struct imxi2c_softc *, int);
static int imxi2c_wait_bus(struct imxi2c_softc *, int);

struct clk_div {
	uint8_t ic_val;
	int div;
};

static const struct clk_div i2c_clk_div[] = {
	{0x20, 22},   {0x21, 24},   {0x22, 26},   {0x23, 28},
	{0x00, 30},   {0x01, 32},   {0x24, 32},   {0x02, 36},
	{0x25, 36},   {0x26, 40},   {0x03, 42},   {0x27, 44},
	{0x04, 48},   {0x28, 48},   {0x05, 52},   {0x29, 56},
	{0x06, 60},   {0x2A, 64},   {0x07, 72},   {0x2B, 72},
	{0x08, 80},   {0x2C, 80},   {0x09, 88},   {0x2D, 96},
	{0x0A, 104},  {0x2E, 112},  {0x0B, 128},  {0x2F, 128},
	{0x0C, 144},  {0x0D, 160},  {0x30, 160},  {0x0E, 192},
	{0x31, 192},  {0x32, 224},  {0x0F, 240},  {0x33, 256},
	{0x10, 288},  {0x11, 320},  {0x34, 320},  {0x12, 384},
	{0x35, 384},  {0x36, 448},  {0x13, 480},  {0x37, 512},
	{0x14, 576},  {0x15, 640},  {0x38, 640},  {0x16, 768},
	{0x39, 768},  {0x3A, 896},  {0x17, 960},  {0x3B, 1024},
	{0x18, 1152}, {0x19, 1280}, {0x3C, 1280}, {0x1A, 1536},
	{0x3D, 1536}, {0x3E, 1792}, {0x1B, 1920}, {0x3F, 2048},
	{0x1C, 2304}, {0x1D, 2560}, {0x1E, 3072}, {0x1F, 3840},
};

CFATTACH_DECL_NEW(imxi2c, sizeof(struct imxi2c_softc),
    imxi2c_match, imxi2c_attach, NULL, NULL);

int
imxi2c_attach_common(device_t parent, device_t self,
    bus_space_tag_t iot, paddr_t iobase, size_t size, int intr, int flags)
{
	struct imxi2c_softc *sc = device_private(self);
	struct i2cbus_attach_args iba;
	int error;

	aprint_normal(": i.MX IIC bus controller\n");

	sc->sc_dev = self;
	sc->sc_iot = iot;
	if (size <= 0)
		size = I2C_SIZE;
	error = bus_space_map(sc->sc_iot, iobase, size, 0, &sc->sc_ioh);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		        "failed to map registers (errno=%d)\n", error);
		return 1;
	}

	mutex_init(&sc->sc_buslock, MUTEX_DEFAULT, IPL_NONE);

	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = imxi2c_i2c_acquire_bus;
	sc->sc_i2c.ic_release_bus = imxi2c_i2c_release_bus;
	sc->sc_i2c.ic_send_start = NULL;
	sc->sc_i2c.ic_send_stop = NULL;
	sc->sc_i2c.ic_initiate_xfer = NULL;
	sc->sc_i2c.ic_read_byte = NULL;
	sc->sc_i2c.ic_write_byte = NULL;
	sc->sc_i2c.ic_exec = imxi2c_i2c_exec;

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_i2c;

	return 0;
}

int
imxi2c_set_freq(device_t self, long freq, int speed)
{
	struct imxi2c_softc *sc = device_private(self);
	bool found = false;
	int index;

	for (index = 0; index < __arraycount(i2c_clk_div); index++) {
		if (freq / i2c_clk_div[index].div < speed) {
			found = true;
			break;
		}
	}

	if (found == false)
		I2C_WRITE(sc, I2C_IFDR, 0x1f);
	else
		I2C_WRITE(sc, I2C_IFDR, i2c_clk_div[index].ic_val);

	return 0;
}


static int
imxi2c_wait(struct imxi2c_softc *sc, int flags)
{
	for (int i = I2C_TIMEOUT; i >= 0; --i) {
		uint16_t sr = I2C_READ(sc, I2C_I2SR);
		if (sr & I2SR_IIF) {
			I2C_WRITE(sc, I2C_I2SR, 0);
			if (sr & I2SR_IAL)
				return EIO;
			if ((sr & I2SR_ICF) == 0)
				return EIO;
			if ((flags & I2C_F_READ) == 0 && (sr & I2SR_RXAK))
				return EIO;
			return 0;
		}
		delay(1);
	}

	return ETIMEDOUT;
}

static int
imxi2c_wait_bus(struct imxi2c_softc *sc, int status)
{
	for (int i = I2C_TIMEOUT; i >= 0; --i) {
		uint16_t sr = I2C_READ(sc, I2C_I2SR);
		if ((sr & I2SR_IBB) == status)
			return 0;
		delay(1);
	}

	return ETIMEDOUT;
}

static int
imxi2c_i2c_acquire_bus(void *cookie, int flags)
{
	struct imxi2c_softc *sc = cookie;

	mutex_enter(&sc->sc_buslock);
	return 0;
}

static void
imxi2c_i2c_release_bus(void *cookie, int flags)
{
	struct imxi2c_softc *sc = cookie;

	mutex_exit(&sc->sc_buslock);
	return;
}

static int
imxi2c_i2c_exec(void *cookie, i2c_op_t op, i2c_addr_t addr, const void *vcmd,
    size_t cmdlen, void *vbuf, size_t buflen, int flags)
{
	struct imxi2c_softc *sc = cookie;
	const uint8_t *cmdbuf = vcmd;
	uint8_t *buf = vbuf;
	int err = 0;
	size_t len;
	uint16_t val;

	/* Clear the bus. */
	I2C_WRITE(sc, I2C_I2SR, 0);
	I2C_WRITE(sc, I2C_I2CR, I2CR_IEN);
	err = imxi2c_wait_bus(sc, 0);
	if (err)
		return err;

	if (cmdlen > 0) {
		I2C_WRITE(sc, I2C_I2CR, I2CR_IEN | I2CR_MSTA | I2CR_MTX);
		err = imxi2c_wait_bus(sc, I2SR_IBB);
		if (err)
			goto out;
		I2C_WRITE(sc, I2C_I2DR, addr<<1);
		err = imxi2c_wait(sc, I2C_F_WRITE);
		if (err)
			goto out;
		len = cmdlen;
		while (len--) {
			I2C_WRITE(sc, I2C_I2DR, *cmdbuf++);
			err = imxi2c_wait(sc, I2C_F_WRITE);
			if (err)
				goto out;
		}
	}

	if (I2C_OP_READ_P(op) && buflen > 0) {
		/* RESTART if we did write a command above. */
		val = I2CR_IEN | I2CR_MSTA | I2CR_MTX;
		if (cmdlen > 0)
			val |= I2CR_RSTA;
		I2C_WRITE(sc, I2C_I2CR, val);
		err = imxi2c_wait_bus(sc, I2SR_IBB);
		if (err)
			goto out;
		I2C_WRITE(sc, I2C_I2DR, addr<<1 | 0x1);
		err = imxi2c_wait(sc, I2C_F_WRITE);
		if (err)
			goto out;

		/* NACK if we're only sending one byte. */
		val = I2CR_IEN | I2CR_MSTA;
		if (buflen == 1)
			val |= I2CR_TXAK;
		I2C_WRITE(sc, I2C_I2CR, val);

		/* Dummy read. */
		I2C_READ(sc, I2C_I2DR);

		len = buflen;
		while (len--) {
			err = imxi2c_wait(sc, I2C_F_READ);
			if (err)
				goto out;

			if (len == 1) {
				/* NACK on last byte. */
				val = I2CR_IEN | I2CR_MSTA | I2CR_TXAK;
				I2C_WRITE(sc, I2C_I2CR, val);
			} else if (len == 0) {
				/* STOP after last byte. */
				val = I2CR_IEN | I2CR_TXAK;
				I2C_WRITE(sc, I2C_I2CR, val);
			}
			*buf++ = I2C_READ(sc, I2C_I2DR);
		}
	}

	if (I2C_OP_WRITE_P(op) && cmdlen == 0 && buflen > 0) {
		/* START if we didn't write a command. */
		I2C_WRITE(sc, I2C_I2CR, I2CR_IEN | I2CR_MSTA | I2CR_MTX);
		err = imxi2c_wait_bus(sc, I2SR_IBB);
		if (err)
			goto out;
		I2C_WRITE(sc, I2C_I2DR, addr<<1);
		err = imxi2c_wait(sc, I2C_F_WRITE);
		if (err)
			goto out;
	}

	if (I2C_OP_WRITE_P(op) && buflen > 0) {
		len = buflen;
		while (len--) {
			I2C_WRITE(sc, I2C_I2DR, *buf++);
			err = imxi2c_wait(sc, I2C_F_WRITE);
			if (err)
				goto out;
		}
	}

out:
	if (err)
		printf("%s: i2c bus error\n", __func__);

	/* STOP if we're still holding the bus. */
	I2C_WRITE(sc, I2C_I2CR, I2CR_IEN);
	imxi2c_wait_bus(sc, 0);

	return err;
}
