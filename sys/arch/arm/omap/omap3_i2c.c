/* $NetBSD: omap3_i2c.c,v 1.2.6.2 2013/02/25 00:28:31 tls Exp $ */

/*-
 * Copyright (c) 2012 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omap3_i2c.c,v 1.2.6.2 2013/02/25 00:28:31 tls Exp $");

#include "opt_omap.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/mutex.h>

#include <dev/i2c/i2cvar.h>

#include <arm/omap/omap2_obiovar.h>

#include <arm/omap/omap2_reg.h>
#include <arm/omap/omap3_i2creg.h>

#ifndef OMAP3_I2C_SLAVE_ADDR
#define OMAP3_I2C_SLAVE_ADDR	0x01
#endif

struct omap3_i2c_softc {
	device_t		sc_dev;
	struct i2c_controller	sc_ic;
	kmutex_t		sc_lock;
	device_t		sc_i2cdev;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
};

#define I2C_READ_REG(sc, reg)		\
	bus_space_read_2((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define I2C_READ_DATA(sc)		\
	bus_space_read_1((sc)->sc_iot, (sc)->sc_ioh, OMAP3_I2C_DATA);
#define I2C_WRITE_REG(sc, reg, val)	\
	bus_space_write_2((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define I2C_WRITE_DATA(sc, val)		\
	bus_space_write_1((sc)->sc_iot, (sc)->sc_ioh, OMAP3_I2C_DATA, (val))

static int	omap3_i2c_match(device_t, cfdata_t, void *);
static void	omap3_i2c_attach(device_t, device_t, void *);
static int	omap3_i2c_rescan(device_t, const char *, const int *);
static void	omap3_i2c_childdet(device_t, device_t);

static int	omap3_i2c_acquire_bus(void *, int);
static void	omap3_i2c_release_bus(void *, int);
static int	omap3_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *,
			       size_t, void *, size_t, int);

static int	omap3_i2c_reset(struct omap3_i2c_softc *);
static int	omap3_i2c_read(struct omap3_i2c_softc *, i2c_addr_t,
			       uint8_t *, size_t, int);
static int	omap3_i2c_write(struct omap3_i2c_softc *, i2c_addr_t,
				const uint8_t *, size_t, int);
static int	omap3_i2c_wait(struct omap3_i2c_softc *, uint16_t, uint16_t);
static int	omap3_i2c_stat(struct omap3_i2c_softc *);
static int	omap3_i2c_flush(struct omap3_i2c_softc *);

i2c_tag_t	omap3_i2c_get_tag(device_t);

CFATTACH_DECL2_NEW(omap3_i2c, sizeof(struct omap3_i2c_softc),
    omap3_i2c_match, omap3_i2c_attach, NULL, NULL,
    omap3_i2c_rescan, omap3_i2c_childdet);

static int
omap3_i2c_match(device_t parent, cfdata_t match, void *opaque)
{
	struct obio_attach_args *obio = opaque;

#if defined(OMAP_3530)
	if (obio->obio_addr == I2C1_BASE_3530 ||
	    obio->obio_addr == I2C2_BASE_3530 ||
	    obio->obio_addr == I2C3_BASE_3530)
		return 1;
#endif

	return 0;
}

static void
omap3_i2c_attach(device_t parent, device_t self, void *opaque)
{
	struct omap3_i2c_softc *sc = device_private(self);
	struct obio_attach_args *obio = opaque;
	uint16_t rev;

	aprint_naive("\n");

	sc->sc_dev = self;
	sc->sc_iot = obio->obio_iot;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	sc->sc_ic.ic_cookie = sc;
	sc->sc_ic.ic_acquire_bus = omap3_i2c_acquire_bus;
	sc->sc_ic.ic_release_bus = omap3_i2c_release_bus;
	sc->sc_ic.ic_exec = omap3_i2c_exec;

	if (bus_space_map(obio->obio_iot, obio->obio_addr, obio->obio_size,
	    0, &sc->sc_ioh) != 0) {
		aprint_error(": couldn't map address space\n");
		return;
	}

	rev = I2C_READ_REG(sc, OMAP3_I2C_REV);
	aprint_normal(": rev %d.%d\n",
	    (int)((rev & OMAP3_I2C_REV_MAJ_MASK) >> OMAP3_I2C_REV_MAJ_SHIFT),
	    (int)((rev & OMAP3_I2C_REV_MIN_MASK) >> OMAP3_I2C_REV_MIN_SHIFT));

	omap3_i2c_reset(sc);
	omap3_i2c_flush(sc);

	omap3_i2c_rescan(self, NULL, NULL);
}

static int
omap3_i2c_rescan(device_t self, const char *ifattr, const int *locs)
{
	struct omap3_i2c_softc *sc = device_private(self);
	struct i2cbus_attach_args iba;

	if (ifattr_match(ifattr, "i2cbus") && sc->sc_i2cdev == NULL) {
		memset(&iba, 0, sizeof(iba));
		iba.iba_tag = &sc->sc_ic;
		sc->sc_i2cdev = config_found_ia(self, "i2cbus",
		    &iba, iicbus_print);
	}

	return 0;
}

static void
omap3_i2c_childdet(device_t self, device_t child)
{
	struct omap3_i2c_softc *sc = device_private(self);

	if (sc->sc_i2cdev == child)
		sc->sc_i2cdev = NULL;
}

static int
omap3_i2c_acquire_bus(void *opaque, int flags)
{
	struct omap3_i2c_softc *sc = opaque;

	if (flags & I2C_F_POLL) {
		if (!mutex_tryenter(&sc->sc_lock))
			return EBUSY;
	} else {
		mutex_enter(&sc->sc_lock);
	}

	return 0;
}

static void
omap3_i2c_release_bus(void *opaque, int flags)
{
	struct omap3_i2c_softc *sc = opaque;

	mutex_exit(&sc->sc_lock);
}

static int
omap3_i2c_exec(void *opaque, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *buf, size_t len, int flags)
{
	struct omap3_i2c_softc *sc = opaque;
	int err;

	if (cmdlen > 0) {
		err = omap3_i2c_write(sc, addr, cmdbuf, cmdlen,
		    I2C_OP_READ_P(op) ? 0 : I2C_F_STOP);
		if (err)
			goto done;
	}

	if (I2C_OP_STOP_P(op))
		flags |= I2C_F_STOP;

	/*
	 * I2C controller doesn't allow for zero-byte transfers.
	 */
	if (len == 0) {
		err = EINVAL;
		goto done;
	}

	if (I2C_OP_READ_P(op)) {
		err = omap3_i2c_read(sc, addr, buf, len, flags);
	} else {
		err = omap3_i2c_write(sc, addr, buf, len, flags);
	}

done:
	if (err)
		omap3_i2c_reset(sc);

	omap3_i2c_flush(sc);

	return err;
}

static int
omap3_i2c_reset(struct omap3_i2c_softc *sc)
{
	uint32_t psc, scll, sclh;

	/* Soft reset */
	I2C_WRITE_REG(sc, OMAP3_I2C_SYSC, OMAP3_I2C_SRST);
	delay(1000);
	I2C_WRITE_REG(sc, OMAP3_I2C_SYSC, 0);

	/* Disable */
	if (I2C_READ_REG(sc, OMAP3_I2C_CON) & OMAP3_I2C_CON_I2C_EN) {
		I2C_WRITE_REG(sc, OMAP3_I2C_CON, 0);
		delay(50000);
	}

	/* XXX standard speed only */
	psc = (96000000 / 19200000) - 1;
	scll = sclh = (19200000 / (2 * 100000)) - 6;

	/* Clocks */
	I2C_WRITE_REG(sc, OMAP3_I2C_PSC, psc);
	I2C_WRITE_REG(sc, OMAP3_I2C_SCLL, scll);
	I2C_WRITE_REG(sc, OMAP3_I2C_SCLH, sclh);

	/* Own I2C address */
	I2C_WRITE_REG(sc, OMAP3_I2C_OA0, OMAP3_I2C_SLAVE_ADDR);

	/* Enable */
	I2C_WRITE_REG(sc, OMAP3_I2C_CON, OMAP3_I2C_CON_I2C_EN);

	/* Enable interrupts (required even for polling mode) */
	I2C_WRITE_REG(sc, OMAP3_I2C_IE,
	    OMAP3_I2C_IE_XRDY_IE|OMAP3_I2C_IE_RRDY_IE|
	    OMAP3_I2C_IE_ARDY_IE|OMAP3_I2C_IE_NACK_IE|
	    OMAP3_I2C_IE_AL_IE);
	delay(1000);

	return 0;
}

static int
omap3_i2c_read(struct omap3_i2c_softc *sc, i2c_addr_t addr, uint8_t *buf,
    size_t buflen, int flags)
{
	uint16_t con, stat;
	int err, i, retry;

	err = omap3_i2c_wait(sc, OMAP3_I2C_STAT_BB, 0);
	if (err)
		return err;

	con = OMAP3_I2C_CON_I2C_EN;
	con |= OMAP3_I2C_CON_MST;
	con |= (OMAP3_I2C_CON_OPMODE_FASTSTD << OMAP3_I2C_CON_OPMODE_SHIFT);
	con |= OMAP3_I2C_CON_STT;
	if (flags & I2C_F_STOP)
		con |= OMAP3_I2C_CON_STP;
	if (addr & ~0x7f)
		con |= OMAP3_I2C_CON_XSA;

	I2C_WRITE_REG(sc, OMAP3_I2C_CNT, buflen);
	I2C_WRITE_REG(sc, OMAP3_I2C_SA,
	    (addr << OMAP3_I2C_SA_SHIFT) & OMAP3_I2C_SA_MASK);
	I2C_WRITE_REG(sc, OMAP3_I2C_CON, con);

	for (i = 0; i < buflen; i++) {
		stat = omap3_i2c_stat(sc);
		if ((stat & OMAP3_I2C_STAT_RRDY) == 0)
			return EBUSY;
		buf[i] = I2C_READ_DATA(sc);
	}

	delay(50000);
	if (I2C_READ_REG(sc, OMAP3_I2C_STAT) & OMAP3_I2C_STAT_NACK)
		return EIO;

	retry = 1000;
	I2C_WRITE_REG(sc, OMAP3_I2C_CON, 0);
	while (I2C_READ_REG(sc, OMAP3_I2C_STAT) ||
	       (I2C_READ_REG(sc, OMAP3_I2C_CON) & OMAP3_I2C_CON_MST)) {
		delay(1000);
		I2C_WRITE_REG(sc, OMAP3_I2C_STAT, 0xffff);
		if (--retry == 0)
			break;
	}

	return 0;
}

static int
omap3_i2c_write(struct omap3_i2c_softc *sc, i2c_addr_t addr, const uint8_t *buf,
    size_t buflen, int flags)
{
	uint16_t con, stat;
	int err, i, retry;

	err = omap3_i2c_wait(sc, OMAP3_I2C_STAT_BB, 0);
	if (err)
		return err;

	con = OMAP3_I2C_CON_I2C_EN;
	con |= OMAP3_I2C_CON_MST;
	con |= (OMAP3_I2C_CON_OPMODE_FASTSTD << OMAP3_I2C_CON_OPMODE_SHIFT);
	con |= OMAP3_I2C_CON_STT;
	if (flags & I2C_F_STOP)
		con |= OMAP3_I2C_CON_STP;
	con |= OMAP3_I2C_CON_TRX;
	if (addr & ~0x7f)
		con |= OMAP3_I2C_CON_XSA;

	I2C_WRITE_REG(sc, OMAP3_I2C_SA,
	    (addr << OMAP3_I2C_SA_SHIFT) & OMAP3_I2C_SA_MASK);
	I2C_WRITE_REG(sc, OMAP3_I2C_CNT, buflen);
	I2C_WRITE_REG(sc, OMAP3_I2C_CON, con);

	for (i = 0; i < buflen; i++) {
		stat = omap3_i2c_stat(sc);
		if ((stat & OMAP3_I2C_STAT_XRDY) == 0)
			return EBUSY;
		I2C_WRITE_DATA(sc, buf[i]);
		I2C_WRITE_REG(sc, OMAP3_I2C_STAT, OMAP3_I2C_STAT_XRDY);
	}

	delay(50000);
	if (I2C_READ_REG(sc, OMAP3_I2C_STAT) & OMAP3_I2C_STAT_NACK)
		return EIO;

	retry = 1000;
	I2C_WRITE_REG(sc, OMAP3_I2C_CON, 0);
	while (I2C_READ_REG(sc, OMAP3_I2C_STAT) ||
	       (I2C_READ_REG(sc, OMAP3_I2C_CON) & OMAP3_I2C_CON_MST)) {
		delay(1000);
		I2C_WRITE_REG(sc, OMAP3_I2C_STAT, 0xffff);
		if (--retry == 0)
			break;
	}

	return 0;
}

static int
omap3_i2c_wait(struct omap3_i2c_softc *sc, uint16_t mask, uint16_t val)
{
	int retry = 10;
	uint16_t v;

	I2C_WRITE_REG(sc, OMAP3_I2C_STAT, 0xffff);
	while (((v = I2C_READ_REG(sc, OMAP3_I2C_STAT)) & mask) != val) {
		I2C_WRITE_REG(sc, OMAP3_I2C_STAT, v);
		--retry;
		if (retry == 0) {
			printf("%s: wait timeout, "
			    "mask = %#x val = %#x stat = %#x\n",
			    device_xname(sc->sc_dev), mask, val, v);
			return EBUSY;
		}
		delay(50000);
	}
	I2C_WRITE_REG(sc, OMAP3_I2C_STAT, 0xffff);

	return 0;
}

static int
omap3_i2c_stat(struct omap3_i2c_softc *sc)
{
	uint16_t v;
	int retry = 10;

	while (--retry > 0) {
		v = I2C_READ_REG(sc, OMAP3_I2C_STAT);
		if ((v & (OMAP3_I2C_STAT_ROVR|OMAP3_I2C_STAT_XUDF|
			  OMAP3_I2C_STAT_XRDY|OMAP3_I2C_STAT_RRDY|
			  OMAP3_I2C_STAT_ARDY|OMAP3_I2C_STAT_NACK|
			  OMAP3_I2C_STAT_AL)) != 0)
			break;
		delay(1000);
	}

	return v;
}

static int
omap3_i2c_flush(struct omap3_i2c_softc *sc)
{
	int retry = 1000;
	uint16_t v;

	while ((v = I2C_READ_REG(sc, OMAP3_I2C_STAT)) & OMAP3_I2C_STAT_RRDY) {
		if (--retry == 0) {
			printf("%s: flush timeout, stat = %#x\n",
			    device_xname(sc->sc_dev), v);
			return EBUSY;
		}
		(void)I2C_READ_DATA(sc);
		I2C_WRITE_REG(sc, OMAP3_I2C_STAT, OMAP3_I2C_STAT_RRDY);
		delay(1000);
	}

	I2C_WRITE_REG(sc, OMAP3_I2C_STAT, 0xffff);
	I2C_WRITE_REG(sc, OMAP3_I2C_CNT, 0);

	return 0;
}

i2c_tag_t
omap3_i2c_get_tag(device_t dev)
{
	struct omap3_i2c_softc *sc;

	if (dev == NULL)
		return NULL;
	sc = device_private(dev);

	return &sc->sc_ic;
}
