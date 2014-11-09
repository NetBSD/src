/* $NetBSD: awin_p2wi.c,v 1.1.2.2 2014/11/09 14:42:33 martin Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: awin_p2wi.c,v 1.1.2.2 2014/11/09 14:42:33 martin Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <dev/i2c/i2cvar.h>

struct awin_p2wi_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	struct i2c_controller sc_ic;
	kmutex_t sc_lock;
	kcondvar_t sc_cv;
	device_t sc_i2cdev;
	void *sc_ih;
	uint32_t sc_stat;
};

#define P2WI_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define P2WI_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	awin_p2wi_acquire_bus(void *, int);
static void     awin_p2wi_release_bus(void *, int);
static int	awin_p2wi_exec(void *, i2c_op_t, i2c_addr_t, const void *,
			       size_t, void *, size_t, int);

static int	awin_p2wi_intr(void *);

static int	awin_p2wi_match(device_t, cfdata_t, void *);
static void	awin_p2wi_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(awin_p2wi, sizeof(struct awin_p2wi_softc),
	awin_p2wi_match, awin_p2wi_attach, NULL, NULL);

static int
awin_p2wi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

	return 1;
}

static void
awin_p2wi_attach(device_t parent, device_t self, void *aux)
{
	struct awin_p2wi_softc *sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	struct i2cbus_attach_args iba;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SCHED);
	cv_init(&sc->sc_cv, "awinp2wi");
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	aprint_naive("\n");
	aprint_normal(": P2WI\n");

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_SCHED, IST_LEVEL,
	    awin_p2wi_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", loc->loc_intr);

	/* Enable interrupts */
	P2WI_WRITE(sc, AWIN_A31_P2WI_INTE_REG,
	    AWIN_A31_P2WI_INTE_LOAD_BSY_ENB |
	    AWIN_A31_P2WI_INTE_TRANS_ERR_ENB |
	    AWIN_A31_P2WI_INTE_TRANS_OVER_ENB);
	P2WI_WRITE(sc, AWIN_A31_P2WI_CTRL_REG,
	    AWIN_A31_P2WI_CTRL_GLOBAL_INT_ENB);

	sc->sc_ic.ic_cookie = sc;
	sc->sc_ic.ic_acquire_bus = awin_p2wi_acquire_bus;
	sc->sc_ic.ic_release_bus = awin_p2wi_release_bus;
	sc->sc_ic.ic_exec = awin_p2wi_exec;

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_ic;
	sc->sc_i2cdev = config_found_ia(self, "i2cbus", &iba, iicbus_print);
}

static int
awin_p2wi_intr(void *priv)
{
	struct awin_p2wi_softc *sc = priv;
	uint32_t stat;

	stat = P2WI_READ(sc, AWIN_A31_P2WI_STAT_REG);
	if ((stat & AWIN_A31_P2WI_STAT_MASK) == 0)
		return 0;

	P2WI_WRITE(sc, AWIN_A31_P2WI_STAT_REG, stat & AWIN_A31_P2WI_STAT_MASK);

	mutex_enter(&sc->sc_lock);
	sc->sc_stat |= stat;
	cv_broadcast(&sc->sc_cv);
	mutex_exit(&sc->sc_lock);

	return 1;
}

static int
awin_p2wi_acquire_bus(void *priv, int flags)
{
	struct awin_p2wi_softc *sc = priv;

	if (flags & I2C_F_POLL) {
		if (!mutex_tryenter(&sc->sc_lock))
			return EBUSY;
	} else {
		mutex_enter(&sc->sc_lock);
	}

	return 0;
}

static void
awin_p2wi_release_bus(void *priv, int flags)
{
	struct awin_p2wi_softc *sc = priv;

	mutex_exit(&sc->sc_lock);
}

static int
awin_p2wi_exec(void *priv, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *buf, size_t len, int flags)
{
	struct awin_p2wi_softc *sc = priv;
	uint32_t dlen, ctrl;
	int error, retry;

	KASSERT(mutex_owned(&sc->sc_lock));

	if (cmdlen != 1 || len != 1)
		return EINVAL;

	/* Data byte register */
	P2WI_WRITE(sc, AWIN_A31_P2WI_DADDR0_REG, *(const uint8_t *)cmdbuf);

	if (I2C_OP_WRITE_P(op)) {
		/* Write data byte */
		P2WI_WRITE(sc, AWIN_A31_P2WI_DATA0_REG, *(uint8_t *)buf);
	}

	/* Program data length register; if reading, set read/write bit */
	dlen = __SHIFTIN(len - 1, AWIN_A31_P2WI_DLEN_ACCESS_LENGTH);
	if (I2C_OP_READ_P(op)) {
		dlen |= AWIN_A31_P2WI_DLEN_READ_WRITE_FLAG;
	}
	P2WI_WRITE(sc, AWIN_A31_P2WI_DLEN_REG, dlen);

	/* Make sure the controller is idle */
	ctrl = P2WI_READ(sc, AWIN_A31_P2WI_CTRL_REG);
	if (ctrl & AWIN_A31_P2WI_CTRL_START_TRANS) {
		device_printf(sc->sc_dev, "device is busy\n");
		return EBUSY;
	}

	/* Start the transfer */
	P2WI_WRITE(sc, AWIN_A31_P2WI_CTRL_REG,
	    ctrl | AWIN_A31_P2WI_CTRL_START_TRANS);

	/* Wait up to 5 seconds for an interrupt */
	sc->sc_stat = 0;
	for (retry = 5; retry > 0; retry--) {
		error = cv_timedwait(&sc->sc_cv, &sc->sc_lock, hz);
		if (error && error != EWOULDBLOCK) {
			break;
		}
		if (sc->sc_stat & AWIN_A31_P2WI_STAT_MASK) {
			break;
		}
	}

	if (error) {
		/* Abort transaction */
		device_printf(sc->sc_dev, "transfer timeout, error = %d\n",
		    error);
		P2WI_WRITE(sc, AWIN_A31_P2WI_CTRL_REG,
		    AWIN_A31_P2WI_CTRL_ABORT_TRANS);
		return error;
	}

	if (sc->sc_stat & AWIN_A31_P2WI_STAT_LOAD_BSY) {
		device_printf(sc->sc_dev, "transfer busy\n");
		return EBUSY;
	}
	if (sc->sc_stat & AWIN_A31_P2WI_STAT_TRANS_ERR) {
		device_printf(sc->sc_dev, "transfer error, id 0x%02llx\n",
		    __SHIFTOUT(sc->sc_stat, AWIN_A31_P2WI_STAT_TRANS_ERR_ID));
		return EIO;
	}

	if (I2C_OP_READ_P(op)) {
		*(uint8_t *)buf = P2WI_READ(sc, AWIN_A31_P2WI_DATA0_REG) & 0xff;
	}

	return 0;
}
