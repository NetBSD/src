/* $NetBSD: rockchip_i2c.c,v 1.1 2014/12/30 17:15:31 jmcneill Exp $ */

/*-
 * Copyright (c) 2014 Jared D. McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rockchip_i2c.c,v 1.1 2014/12/30 17:15:31 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <arm/rockchip/rockchip_reg.h>
#include <arm/rockchip/rockchip_var.h>

#include <arm/rockchip/rockchip_i2creg.h>

#include <dev/i2c/i2cvar.h>

#define RKIIC_CLOCK_RATE	1000000

struct rkiic_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	void *sc_ih;

	struct i2c_controller sc_ic;
	kmutex_t sc_lock;
	kcondvar_t sc_cv;
	device_t sc_i2cdev;
	u_int sc_port;

	uint32_t sc_intr_ipd;
};

static int	rkiic_match(device_t, cfdata_t, void *);
static void	rkiic_attach(device_t, device_t, void *);

static int	rkiic_intr(void *);

static int	rkiic_acquire_bus(void *, int);
static void	rkiic_release_bus(void *, int);
static int	rkiic_exec(void *, i2c_op_t, i2c_addr_t, const void *,
			   size_t, void *, size_t, int);

static int	rkiic_wait(struct rkiic_softc *, uint32_t, int, int);
static int	rkiic_read(struct rkiic_softc *, i2c_addr_t,
			   uint8_t *, size_t, int);
static int	rkiic_write(struct rkiic_softc *, i2c_addr_t,
			    const uint8_t *, size_t, int);
static int	rkiic_set_rate(struct rkiic_softc *, u_int);

CFATTACH_DECL_NEW(rkiic, sizeof(struct rkiic_softc),
	rkiic_match, rkiic_attach, NULL, NULL);

#define I2C_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define I2C_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))

static int
rkiic_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args * const obio = aux;

	if (obio->obio_port == OBIOCF_PORT_DEFAULT)
		return 0;

	return 1;
}

static void
rkiic_attach(device_t parent, device_t self, void *aux)
{
	struct rkiic_softc *sc = device_private(self);
	struct obio_attach_args * const obio = aux;
	struct i2cbus_attach_args iba;

	sc->sc_dev = self;
	sc->sc_bst = obio->obio_bst;
	bus_space_subregion(obio->obio_bst, obio->obio_bsh, obio->obio_offset,
	    obio->obio_size, &sc->sc_bsh);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SCHED);
	cv_init(&sc->sc_cv, device_xname(self));
	sc->sc_port = obio->obio_port;

	aprint_naive("\n");
	aprint_normal(": I2C%u\n", sc->sc_port);

	if (obio->obio_intr != OBIOCF_INTR_DEFAULT) {
		sc->sc_ih = intr_establish(obio->obio_intr, IPL_SCHED,
		    IST_LEVEL, rkiic_intr, sc);
		if (sc->sc_ih == NULL) {
			aprint_error_dev(self,
			    "couldn't establish interrupt\n");
			/* Not fatal; will use polling mode */
		}
	}

	sc->sc_ic.ic_cookie = sc;
	sc->sc_ic.ic_acquire_bus = rkiic_acquire_bus;
	sc->sc_ic.ic_release_bus = rkiic_release_bus;
	sc->sc_ic.ic_exec = rkiic_exec;

	iba.iba_tag = &sc->sc_ic;
	sc->sc_i2cdev = config_found_ia(self, "i2cbus",
	    &iba, iicbus_print);
}

static int
rkiic_intr(void *priv)
{
	struct rkiic_softc *sc = priv;
	uint32_t ipd;

	ipd = I2C_READ(sc, I2C_IPD_REG);
	if (!ipd)
		return 0;

	I2C_WRITE(sc, I2C_IPD_REG, ipd);

	mutex_enter(&sc->sc_lock);
	sc->sc_intr_ipd |= ipd;
	cv_broadcast(&sc->sc_cv);
	mutex_exit(&sc->sc_lock);
	
	return 1;
}

static int
rkiic_acquire_bus(void *priv, int flags)
{
	struct rkiic_softc *sc = priv;

	mutex_enter(&sc->sc_lock);

	return 0;
}

static void
rkiic_release_bus(void *priv, int flags)
{
	struct rkiic_softc *sc = priv;

	mutex_exit(&sc->sc_lock);
}

static int
rkiic_exec(void *priv, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *buf, size_t len, int flags)
{
	struct rkiic_softc *sc = priv;
	uint32_t con;
	u_int mode;
	int error;

	KASSERT(mutex_owned(&sc->sc_lock));

	if (sc->sc_ih == NULL) {
		flags |= I2C_F_POLL;
	}

	error = rkiic_set_rate(sc, RKIIC_CLOCK_RATE);
	if (error)
		return error;

	if (cmdlen > 0) {
		if (I2C_OP_READ_P(op)) {
			mode = I2C_CON_MODE_RRX;
		} else {
			mode = I2C_CON_MODE_TRX;
		}
	} else {
		if (I2C_OP_READ_P(op)) {
			mode = I2C_CON_MODE_RX;
		} else {
			mode = I2C_CON_MODE_TX;
		}
	}
	con = I2C_CON_START | I2C_CON_EN | __SHIFTIN(mode, I2C_CON_MODE);

	I2C_WRITE(sc, I2C_CON_REG, con);

	if (cmdlen > 0) {
		error = rkiic_write(sc, addr, cmdbuf, cmdlen, flags);
		if (error) {
			goto done;
		}
	}

	if (I2C_OP_READ_P(op)) {
		error = rkiic_read(sc, addr, buf, len, flags);
	} else {
		error = rkiic_write(sc, addr, buf, len, flags);
	}

done:
	if (I2C_OP_STOP_P(op)) {
		I2C_WRITE(sc, I2C_CON_REG, I2C_CON_STOP);
	}

	return error;
}

static int
rkiic_wait(struct rkiic_softc *sc, uint32_t mask, int timeout, int flags)
{
	int retry, error;

	KASSERT(mutex_owned(&sc->sc_lock));

	if (sc->sc_intr_ipd & mask)
		return 0;

	if (flags & I2C_F_POLL) {
		retry = (timeout / hz) * 1000000;
		while (retry > 0) {
			if (I2C_READ(sc, I2C_IPD_REG) & mask)
				return 0;
			delay(1);
			--retry;
		}
	} else {
		retry = timeout / hz;

		while (retry > 0) {
			error = cv_timedwait(&sc->sc_cv, &sc->sc_lock, hz);
			if (error && error != EWOULDBLOCK)
				return error;
			if (sc->sc_intr_ipd & mask)
				return 0;
			--retry;
		}
	}

	return ETIMEDOUT;
}

static int
rkiic_read(struct rkiic_softc *sc, i2c_addr_t addr, uint8_t *buf,
    size_t buflen, int flags)
{
	u_int off, byte;
	size_t resid;
	int error;

	if (buflen > 32)
		return EINVAL;

	sc->sc_intr_ipd = 0;

	if (!(flags & I2C_F_POLL)) {
		I2C_WRITE(sc, I2C_IEN_REG, I2C_INT_MBRF);
	}

	I2C_WRITE(sc, I2C_MRXCNT_REG, __SHIFTIN(buflen, I2C_MRXCNT_COUNT));

	error = rkiic_wait(sc, I2C_INT_MBRF, hz, flags);
	if (error)
		goto done;

	for (off = 0, resid = buflen; off < 8 && resid > 0; off++) {
		const uint32_t data = I2C_READ(sc, I2C_RXDATA_REG(off));
		for (byte = 0; byte < 4 && resid > 0; byte++, resid--) {
			buf[off * 4 + byte] = (data >> (byte * 8)) & 0xff;
		}
	}

done:
	if (!(flags & I2C_F_POLL)) {
		I2C_WRITE(sc, I2C_IEN_REG, 0);
	}

	return error;
}

static int
rkiic_write(struct rkiic_softc *sc, i2c_addr_t addr, const uint8_t *buf,
    size_t buflen, int flags)
{
	u_int off, byte;
	size_t resid;
	int error;

	if (buflen > 32)
		return EINVAL;

	sc->sc_intr_ipd = 0;

	if (!(flags & I2C_F_POLL)) {
		I2C_WRITE(sc, I2C_IEN_REG, I2C_INT_MBTF);
	}

	for (off = 0, resid = buflen; off < 8 && resid > 0; off++) {
		uint32_t data = 0;
		for (byte = 0; byte < 4 && resid > 0; byte++, resid--) {
			data |= buf[off * 4 + byte] << (byte * 8);
		}
		I2C_WRITE(sc, I2C_TXDATA_REG(off), data);
	}

	I2C_WRITE(sc, I2C_MTXCNT_REG, __SHIFTIN(buflen, I2C_MTXCNT_COUNT));

	error = rkiic_wait(sc, I2C_INT_MBTF, hz, flags);
	if (error)
		goto done;

done:
	if (!(flags & I2C_F_POLL)) {
		I2C_WRITE(sc, I2C_IEN_REG, 0);
	}

	return error;

}

static int
rkiic_set_rate(struct rkiic_softc *sc, u_int rate)
{
	u_int i2c_rate = rockchip_i2c_get_rate(sc->sc_port);
	u_int div, divh, divl;

	if (i2c_rate == 0)
		return ENXIO;

	/*
	 * From RK3188 datasheet:
	 *   SCL Divisor = 8*(CLKDIVL + CLKDIVH)
	 *   SCL = PCLK/ SCLK Divisor
	 */
	div = howmany(i2c_rate, rate * 8);
	divh = divl = howmany(div, 2);
	I2C_WRITE(sc, I2C_CLKDIV_REG,
	    __SHIFTIN(divh, I2C_CON_CLKDIVH) |
	    __SHIFTIN(divl, I2C_CON_CLKDIVL));

	return 0;
}
