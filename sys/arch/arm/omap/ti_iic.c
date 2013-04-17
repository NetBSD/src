/* $NetBSD: ti_iic.c,v 1.1 2013/04/17 14:33:06 bouyer Exp $ */

/*
 * Copyright (c) 2013 Manuel Bouyer.  All rights reserved.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
__KERNEL_RCSID(0, "$NetBSD: ti_iic.c,v 1.1 2013/04/17 14:33:06 bouyer Exp $");

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
#include <arm/omap/ti_iicreg.h>

#ifdef TI_AM335X
#  include <arm/omap/am335x_prcm.h>
#  include <arm/omap/omap2_prcm.h>
#  include <arm/omap/sitara_cm.h>
#  include <arm/omap/sitara_cmreg.h>
#endif

#ifndef OMAP2_I2C_SLAVE_ADDR
#define OMAP2_I2C_SLAVE_ADDR	0x01
#endif

#ifdef I2CDEBUG
#define DPRINTF(args)	printf args
#else
#define DPRINTF(args)
#endif

struct ti_iic_softc {
	device_t		sc_dev;
	struct i2c_controller	sc_ic;
	kmutex_t		sc_lock;
	device_t		sc_i2cdev;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

	int			sc_rxthres;
	int			sc_txthres;
};

#define I2C_READ_REG(sc, reg)		\
	bus_space_read_2((sc)->sc_iot, (sc)->sc_ioh, (reg))
#define I2C_READ_DATA(sc)		\
	bus_space_read_1((sc)->sc_iot, (sc)->sc_ioh, OMAP2_I2C_DATA);
#define I2C_WRITE_REG(sc, reg, val)	\
	bus_space_write_2((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define I2C_WRITE_DATA(sc, val)		\
	bus_space_write_1((sc)->sc_iot, (sc)->sc_ioh, OMAP2_I2C_DATA, (val))

static int	ti_iic_match(device_t, cfdata_t, void *);
static void	ti_iic_attach(device_t, device_t, void *);
static int	ti_iic_rescan(device_t, const char *, const int *);
static void	ti_iic_childdet(device_t, device_t);

static int	ti_iic_acquire_bus(void *, int);
static void	ti_iic_release_bus(void *, int);
static int	ti_iic_exec(void *, i2c_op_t, i2c_addr_t, const void *,
			       size_t, void *, size_t, int);

static int	ti_iic_reset(struct ti_iic_softc *);
static int	ti_iic_read(struct ti_iic_softc *, i2c_addr_t,
			       uint8_t *, size_t, int);
static int	ti_iic_write(struct ti_iic_softc *, i2c_addr_t,
				const uint8_t *, size_t, int);
static int	ti_iic_wait(struct ti_iic_softc *, uint16_t, uint16_t);
static int	ti_iic_stat(struct ti_iic_softc *);
static int	ti_iic_flush(struct ti_iic_softc *);

i2c_tag_t	ti_iic_get_tag(device_t);

#ifdef TI_AM335X
struct am335x_iic {
	const char *as_name;
	bus_addr_t as_base_addr;
	int as_intr;
	struct omap_module as_module;
};

static const struct am335x_iic am335x_iic[] = {
	{ "I2C0", OMAP2_I2C0_BASE, 70, { AM335X_PRCM_CM_WKUP, 0xb8 } },
	{ "I2C1", OMAP2_I2C1_BASE, 71, { AM335X_PRCM_CM_PER, 0x48 } },
	{ "I2C2", OMAP2_I2C1_BASE, 30, { AM335X_PRCM_CM_PER, 0x44 } },
};
#endif


CFATTACH_DECL2_NEW(ti_iic, sizeof(struct ti_iic_softc),
    ti_iic_match, ti_iic_attach, NULL, NULL,
    ti_iic_rescan, ti_iic_childdet);

static int
ti_iic_match(device_t parent, cfdata_t match, void *opaque)
{
	struct obio_attach_args *obio = opaque;

#if defined(TI_AM335X)
	if (obio->obio_addr == OMAP2_I2C0_BASE ||
	    obio->obio_addr == OMAP2_I2C1_BASE ||
	    obio->obio_addr == OMAP2_I2C2_BASE)
		return 1;
#endif

	return 0;
}

static void
ti_iic_attach(device_t parent, device_t self, void *opaque)
{
	struct ti_iic_softc *sc = device_private(self);
	struct obio_attach_args *obio = opaque;
	uint16_t rev;
#ifdef TI_AM335X
	int i;
	const char *mode;
	u_int state;
#endif

	aprint_naive("\n");

	sc->sc_dev = self;
	sc->sc_iot = obio->obio_iot;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	sc->sc_ic.ic_cookie = sc;
	sc->sc_ic.ic_acquire_bus = ti_iic_acquire_bus;
	sc->sc_ic.ic_release_bus = ti_iic_release_bus;
	sc->sc_ic.ic_exec = ti_iic_exec;

	sc->sc_rxthres = sc->sc_txthres = 4;

	if (bus_space_map(obio->obio_iot, obio->obio_addr, obio->obio_size,
	    0, &sc->sc_ioh) != 0) {
		aprint_error(": couldn't map address space\n");
		return;
	}
#ifdef TI_AM335X
	for (i = 0; i < __arraycount(am335x_iic); i++) {
		if ((obio->obio_addr == am335x_iic[i].as_base_addr) &&
		    (obio->obio_intr == am335x_iic[i].as_intr)) {
			prcm_module_enable(&am335x_iic[i].as_module);
			break;
		}
	}
	KASSERT(i < __arraycount(am335x_iic));
	if (sitara_cm_padconf_get("I2C0_SDA", &mode, &state) == 0) {
		aprint_debug(": SDA mode %s state %d ", mode, state);
	}
	if (sitara_cm_padconf_get("I2C0_SCL", &mode, &state) == 0) {
		aprint_debug(": SCL mode %s state %d ", mode, state);
	}
	if (sitara_cm_padconf_set("I2C0_SDA", "I2C0_SDA",
	    (0x01 << 4) | (0x01 << 5) | (0x01 << 6)) != 0) {
		aprint_error(": can't switch SDA pad\n");
		return;
	}
	if (sitara_cm_padconf_set("I2C0_SCL", "I2C0_SCL",
	    (0x01 << 4) | (0x01 << 5) | (0x01 << 6)) != 0) {
		aprint_error(": can't switch SCL pad\n");
		return;
	}
#endif

	rev = I2C_READ_REG(sc, OMAP2_I2C_REVNB_LO);
	aprint_normal(": rev %d.%d\n",
	    (int)I2C_REVNB_LO_MAJOR(rev),
	    (int)I2C_REVNB_LO_MINOR(rev));

	ti_iic_reset(sc);
	ti_iic_flush(sc);

	ti_iic_rescan(self, NULL, NULL);
}

static int
ti_iic_rescan(device_t self, const char *ifattr, const int *locs)
{
	struct ti_iic_softc *sc = device_private(self);
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
ti_iic_childdet(device_t self, device_t child)
{
	struct ti_iic_softc *sc = device_private(self);

	if (sc->sc_i2cdev == child)
		sc->sc_i2cdev = NULL;
}

static int
ti_iic_acquire_bus(void *opaque, int flags)
{
	struct ti_iic_softc *sc = opaque;

	if (flags & I2C_F_POLL) {
		if (!mutex_tryenter(&sc->sc_lock))
			return EBUSY;
	} else {
		mutex_enter(&sc->sc_lock);
	}

	return 0;
}

static void
ti_iic_release_bus(void *opaque, int flags)
{
	struct ti_iic_softc *sc = opaque;

	mutex_exit(&sc->sc_lock);
}

static int
ti_iic_exec(void *opaque, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *buf, size_t len, int flags)
{
	struct ti_iic_softc *sc = opaque;
	int err;

	DPRINTF(("ti_iic_exec: op 0x%x cmdlen %zd len %zd flags 0x%x\n",
	    op, cmdlen, len, flags));

	if (cmdlen > 0) {
		err = ti_iic_write(sc, addr, cmdbuf, cmdlen,
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
		err = ti_iic_read(sc, addr, buf, len, flags);
	} else {
		err = ti_iic_write(sc, addr, buf, len, flags);
	}

done:
	if (err)
		ti_iic_reset(sc);

	ti_iic_flush(sc);

	DPRINTF(("ti_iic_exec: done %d\n", err));
	return err;
}

static int
ti_iic_reset(struct ti_iic_softc *sc)
{
	uint32_t psc, scll, sclh;
	int i;

	/* Disable */
	I2C_WRITE_REG(sc, OMAP2_I2C_CON, 0);
	/* Soft reset */
	I2C_WRITE_REG(sc, OMAP2_I2C_SYSC, I2C_SYSC_SRST);
	delay(1000);
	/* enable so that we can check for reset complete */
	I2C_WRITE_REG(sc, OMAP2_I2C_CON, I2C_CON_EN);
	delay(1000);
	for (i = 0; i < 1000; i++) { /* 1s delay for reset */
		if (I2C_READ_REG(sc, OMAP2_I2C_SYSS) & I2C_SYSS_RDONE)
			break;
	}
	/* Disable again */
	I2C_WRITE_REG(sc, OMAP2_I2C_CON, 0);
	delay(50000);

	if (i >= 1000) {
		aprint_error_dev(sc->sc_dev, ": couldn't reset module\n");
		return 1;
	}
			

	/* XXX standard speed only */
	psc = 3;
	scll = 53;
	sclh = 55;

	/* Clocks */
	I2C_WRITE_REG(sc, OMAP2_I2C_PSC, psc);
	I2C_WRITE_REG(sc, OMAP2_I2C_SCLL, scll);
	I2C_WRITE_REG(sc, OMAP2_I2C_SCLH, sclh);

	/* Own I2C address */
	I2C_WRITE_REG(sc, OMAP2_I2C_OA, OMAP2_I2C_SLAVE_ADDR);

	/* 5 bytes fifo */
	I2C_WRITE_REG(sc, OMAP2_I2C_BUF,
	    I2C_BUF_RXTRSH(sc->sc_rxthres) | I2C_BUF_TXTRSH(sc->sc_txthres));

	/* Enable */
	I2C_WRITE_REG(sc, OMAP2_I2C_CON, I2C_CON_EN);

	return 0;
}

static int
ti_iic_read(struct ti_iic_softc *sc, i2c_addr_t addr, uint8_t *buf,
    size_t buflen, int flags)
{
	uint16_t con, stat;
	int err, i, retry;
	size_t len;

	err = ti_iic_wait(sc, I2C_IRQSTATUS_BB, 0);
	if (err) {
		DPRINTF(("ti_iic_read: wait error %d\n", err));
		return err;
	}

	con = I2C_CON_EN;
	con |= I2C_CON_MST;
	con |= I2C_CON_STT;;
	if (flags & I2C_F_STOP)
		con |= I2C_CON_STP;
	if (addr & ~0x7f)
		con |= I2C_CON_XSA;

	I2C_WRITE_REG(sc, OMAP2_I2C_CON, I2C_CON_EN | I2C_CON_MST | I2C_CON_STP);
	DPRINTF(("ti_iic_read: con 0x%x ", con));
	I2C_WRITE_REG(sc, OMAP2_I2C_CNT, buflen);
	I2C_WRITE_REG(sc, OMAP2_I2C_SA, (addr & I2C_SA_MASK));
	DPRINTF(("SA 0x%x len %d\n", I2C_READ_REG(sc, OMAP2_I2C_SA), I2C_READ_REG(sc, OMAP2_I2C_CNT)));
	I2C_WRITE_REG(sc, OMAP2_I2C_CON, con);

	i = 0;
	while (1) {
		stat = ti_iic_stat(sc);
		DPRINTF(("ti_iic_read stat 0x%x\n", stat));
		if (stat & (I2C_IRQSTATUS_NACK|I2C_IRQSTATUS_AL)) {
			err = EIO;
			break;
		}
		if (stat & I2C_IRQSTATUS_ARDY) {
			err = 0;
			break;
		}
			
		if (stat & I2C_IRQSTATUS_RDR) {
			len = I2C_READ_REG(sc, OMAP2_I2C_BUFSTAT);
			len = I2C_BUFSTAT_RXSTAT(len);
			DPRINTF(("ti_iic_read receive drain len %zd left %d\n",
			    len, I2C_READ_REG(sc, OMAP2_I2C_CNT)));
		} else if (stat & I2C_IRQSTATUS_RRDY) {
			len = sc->sc_rxthres + 1;
			DPRINTF(("ti_iic_read receive len %zd left %d\n",
			    len, I2C_READ_REG(sc, OMAP2_I2C_CNT)));
		} else {
			DELAY(1);
			continue;
		}
		for (; i < buflen && len > 0; i++, len--) {
			buf[i] = I2C_READ_DATA(sc);
			DPRINTF(("ti_iic_read got b[%d]=0x%x\n",
			    i, buf[i]));
		}
		I2C_WRITE_REG(sc, OMAP2_I2C_IRQSTATUS, stat);
	}

	I2C_WRITE_REG(sc, OMAP2_I2C_IRQSTATUS, stat);
	retry = 10000;
	I2C_WRITE_REG(sc, OMAP2_I2C_CON, 0);
	while (I2C_READ_REG(sc, OMAP2_I2C_IRQSTATUS_RAW) ||
	       (I2C_READ_REG(sc, OMAP2_I2C_CON) & I2C_CON_MST)) {
		delay(100);
		if (--retry == 0)
			break;
	}

	return err;
}

static int
ti_iic_write(struct ti_iic_softc *sc, i2c_addr_t addr, const uint8_t *buf,
    size_t buflen, int flags)
{
	uint16_t con, stat;
	int err, i, retry;
	size_t len;

	err = ti_iic_wait(sc, I2C_IRQSTATUS_BB, 0);
	if (err) {
		DPRINTF(("ti_iic_write wait error %d\n", err));
		return err;
	}

	con = I2C_CON_EN;
	con |= I2C_CON_MST;
	con |= I2C_CON_STT;
	if (flags & I2C_F_STOP)
		con |= I2C_CON_STP;
	con |= I2C_CON_TRX;
	if (addr & ~0x7f)
		con |= I2C_CON_XSA;

	I2C_WRITE_REG(sc, OMAP2_I2C_CON, I2C_CON_EN | I2C_CON_MST | I2C_CON_STP);
	DPRINTF(("ti_iic_write: con 0x%x ", con));
	I2C_WRITE_REG(sc, OMAP2_I2C_SA, (addr & I2C_SA_MASK));
	I2C_WRITE_REG(sc, OMAP2_I2C_CNT, buflen);
	DPRINTF(("SA 0x%x len %d\n", I2C_READ_REG(sc, OMAP2_I2C_SA), I2C_READ_REG(sc, OMAP2_I2C_CNT)));
	I2C_WRITE_REG(sc, OMAP2_I2C_CON, con);

	i = 0;
	while (1) {
		stat = ti_iic_stat(sc);
		DPRINTF(("ti_iic_write stat 0x%x\n", stat));
		if (stat & (I2C_IRQSTATUS_NACK|I2C_IRQSTATUS_AL)) {
			err = EIO;
			break;
		}
		if (stat & I2C_IRQSTATUS_ARDY) {
			err = 0;
			break;
		}

		if (stat & I2C_IRQSTATUS_XDR) {
			len = I2C_READ_REG(sc, OMAP2_I2C_BUFSTAT);
			len = I2C_BUFSTAT_TXSTAT(len);
			DPRINTF(("ti_iic_write transmit drain len %zd left %d\n",
			    len, I2C_READ_REG(sc, OMAP2_I2C_CNT)));
		} else if (stat & I2C_IRQSTATUS_XRDY) {
			len = sc->sc_txthres + 1;
			DPRINTF(("ti_iic_write transmit len %zd left %d\n",
			    len, I2C_READ_REG(sc, OMAP2_I2C_CNT)));
		} else {
			DELAY(1);
			continue;
		}
		for (; i < buflen && len > 0; i++, len--) {
			DPRINTF(("ti_iic_write send b[%d]=0x%x\n",
			    i, buf[i]));
			I2C_WRITE_DATA(sc, buf[i]);
		}
		I2C_WRITE_REG(sc, OMAP2_I2C_IRQSTATUS, stat);
	}

	I2C_WRITE_REG(sc, OMAP2_I2C_IRQSTATUS, stat);
	retry = 10000;
	I2C_WRITE_REG(sc, OMAP2_I2C_CON, 0);
	while (I2C_READ_REG(sc, OMAP2_I2C_IRQSTATUS_RAW) ||
	       (I2C_READ_REG(sc, OMAP2_I2C_CON) & I2C_CON_MST)) {
		delay(100);
		if (--retry == 0)
			break;
	}

	return err;
}

static int
ti_iic_wait(struct ti_iic_softc *sc, uint16_t mask, uint16_t val)
{
	int retry = 10;
	uint16_t v;

	while (((v = I2C_READ_REG(sc, OMAP2_I2C_IRQSTATUS_RAW)) & mask) != val) {
		--retry;
		if (retry == 0) {
			aprint_error_dev(sc->sc_dev, ": wait timeout, "
			    "mask = %#x val = %#x stat = %#x\n",
			    mask, val, v);
			return EBUSY;
		}
		delay(50000);
	}

	return 0;
}

static int
ti_iic_stat(struct ti_iic_softc *sc)
{
	uint16_t v;
	int retry = 100;

	while (--retry > 0) {
		v = I2C_READ_REG(sc, OMAP2_I2C_IRQSTATUS_RAW);
		if ((v & (I2C_IRQSTATUS_ROVR|I2C_IRQSTATUS_XUDF|
			  I2C_IRQSTATUS_XDR|I2C_IRQSTATUS_RDR|
			  I2C_IRQSTATUS_XRDY|I2C_IRQSTATUS_RRDY|
			  I2C_IRQSTATUS_ARDY|I2C_IRQSTATUS_NACK|
			  I2C_IRQSTATUS_AL)) != 0)
			break;
		delay(100);
	}

	return v;
}

static int
ti_iic_flush(struct ti_iic_softc *sc)
{
#if 0
	int retry = 1000;
	uint16_t v;

	while ((v = I2C_READ_REG(sc, OMAP2_I2C_IRQSTATUS_RAW)) & I2C_IRQSTATUS_RRDY) {
		if (--retry == 0) {
			aprint_error_dev(sc->sc_dev,
			    ": flush timeout, stat = %#x\n", v);
			return EBUSY;
		}
		(void)I2C_READ_DATA(sc);
		delay(1000);
	}
#endif

	I2C_WRITE_REG(sc, OMAP2_I2C_CNT, 0);
	return 0;
}

i2c_tag_t
ti_iic_get_tag(device_t dev)
{
	struct ti_iic_softc *sc;

	if (dev == NULL)
		return NULL;
	sc = device_private(dev);

	return &sc->sc_ic;
}
