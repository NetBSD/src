/* $NetBSD: awin_p2wi.c,v 1.5.16.2 2017/12/03 11:35:51 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: awin_p2wi.c,v 1.5.16.2 2017/12/03 11:35:51 jdolecek Exp $");

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

#define AWIN_RSB_ADDR_AXP809	0x3a3
#define AWIN_RSB_ADDR_AXP806	0x745
#define AWIN_RSB_ADDR_AC100	0xe89

#define AWIN_RSB_RTA_AXP809	0x2d
#define AWIN_RSB_RTA_AXP806	0x3a
#define AWIN_RSB_RTA_AC100	0x4e

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

	bool sc_rsb_p;
	uint16_t sc_rsb_last_da;
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
static int	awin_p2wi_wait(struct awin_p2wi_softc *, int);
static int	awin_p2wi_rsb_config(struct awin_p2wi_softc *,
				     uint8_t, i2c_addr_t, int);

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
	sc->sc_rsb_p = awin_chip_id() == AWIN_CHIP_ID_A80;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SCHED);
	cv_init(&sc->sc_cv, "awinp2wi");
	bus_space_subregion(sc->sc_bst,
	    sc->sc_rsb_p ? aio->aio_a80_rcpus_bsh : aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	aprint_naive("\n");
	aprint_normal(": %s\n", sc->sc_rsb_p ? "RSB" : "P2WI");

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
awin_p2wi_wait(struct awin_p2wi_softc *sc, int flags)
{
	int error = 0, retry;

	/* Wait up to 5 seconds for a transfer to complete */
	sc->sc_stat = 0;
	for (retry = (flags & I2C_F_POLL) ? 100 : 5; retry > 0; retry--) {
		if (flags & I2C_F_POLL) {
			sc->sc_stat |= P2WI_READ(sc, AWIN_A31_P2WI_STAT_REG);
		} else {
			error = cv_timedwait(&sc->sc_cv, &sc->sc_lock, hz);
			if (error && error != EWOULDBLOCK) {
				break;
			}
		}
		if (sc->sc_stat & AWIN_A31_P2WI_STAT_MASK) {
			break;
		}
		if (flags & I2C_F_POLL) {
			delay(10000);
		}
	}
	if (retry == 0)
		error = EAGAIN;

	if (flags & I2C_F_POLL) {
		P2WI_WRITE(sc, AWIN_A31_P2WI_STAT_REG,
		    sc->sc_stat & AWIN_A31_P2WI_STAT_MASK);
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

	return 0;
}

static int
awin_p2wi_rsb_config(struct awin_p2wi_softc *sc, uint8_t rta, i2c_addr_t da,
    int flags)
{
	uint32_t dar, ctrl;

	KASSERT(mutex_owned(&sc->sc_lock));

	P2WI_WRITE(sc, AWIN_A31_P2WI_STAT_REG,
	    P2WI_READ(sc, AWIN_A31_P2WI_STAT_REG) & AWIN_A31_P2WI_STAT_MASK);

	dar = __SHIFTIN(rta, AWIN_A80_RSB_DAR_RTA);
	dar |= __SHIFTIN(da, AWIN_A80_RSB_DAR_DA);
	P2WI_WRITE(sc, AWIN_A80_RSB_DAR_REG, dar);
	P2WI_WRITE(sc, AWIN_A80_RSB_CMD_REG, AWIN_A80_RSB_CMD_IDX_SRTA);

	/* Make sure the controller is idle */
	ctrl = P2WI_READ(sc, AWIN_A31_P2WI_CTRL_REG);
	if (ctrl & AWIN_A31_P2WI_CTRL_START_TRANS) {
		device_printf(sc->sc_dev, "device is busy\n");
		return EBUSY;
	}

	/* Start the transfer */
	P2WI_WRITE(sc, AWIN_A31_P2WI_CTRL_REG,
	    ctrl | AWIN_A31_P2WI_CTRL_START_TRANS);

	return awin_p2wi_wait(sc, flags);
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
	uint8_t rta;
	int error;

	KASSERT(mutex_owned(&sc->sc_lock));

	if (cmdlen != 1 || (len != 1 && len != 2 && len != 4))
		return EINVAL;

	if (sc->sc_rsb_p && sc->sc_rsb_last_da != addr) {
		switch (addr) {
		case AWIN_RSB_ADDR_AXP809:
			rta = AWIN_RSB_RTA_AXP809;
			break;
		case AWIN_RSB_ADDR_AXP806:
			rta = AWIN_RSB_RTA_AXP806;
			break;
		case AWIN_RSB_ADDR_AC100:
			rta = AWIN_RSB_RTA_AC100;
			break;
		default:
			return ENXIO;
		}
		error = awin_p2wi_rsb_config(sc, rta, addr, flags);
		if (error) {
			device_printf(sc->sc_dev,
			    "SRTA failed, flags = %x, error = %d\n",
			    flags, error);
			sc->sc_rsb_last_da = 0;
			return error;
		}

		sc->sc_rsb_last_da = addr;
	}

	/* Data byte register */
	P2WI_WRITE(sc, AWIN_A31_P2WI_DADDR0_REG, *(const uint8_t *)cmdbuf);

	if (I2C_OP_WRITE_P(op)) {
		uint8_t *pbuf = buf;
		uint32_t data;
		/* Write data */
		switch (len) {
		case 1:
			data = pbuf[0];
			break;
		case 2:
			data = pbuf[0] | (pbuf[1] << 8);
			break;
		case 4:
			data = pbuf[0] | (pbuf[1] << 8) |
			    (pbuf[2] << 16) | (pbuf[3] << 24);
			break;
		default:
			return EINVAL;
		}
		P2WI_WRITE(sc, AWIN_A31_P2WI_DATA0_REG, data);
	}

	if (sc->sc_rsb_p) {
		uint8_t cmd;
		if (I2C_OP_WRITE_P(op)) {
			switch (len) {
			case 1:	cmd = AWIN_A80_RSB_CMD_IDX_WR8; break;
			case 2: cmd = AWIN_A80_RSB_CMD_IDX_WR16; break;
			case 4: cmd = AWIN_A80_RSB_CMD_IDX_WR32; break;
			default: return EINVAL;
			}
		} else {
			switch (len) {
			case 1:	cmd = AWIN_A80_RSB_CMD_IDX_RD8; break;
			case 2: cmd = AWIN_A80_RSB_CMD_IDX_RD16; break;
			case 4: cmd = AWIN_A80_RSB_CMD_IDX_RD32; break;
			default: return EINVAL;
			}
		}
		P2WI_WRITE(sc, AWIN_A80_RSB_CMD_REG, cmd);
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

	error = awin_p2wi_wait(sc, flags);
	if (error) {
		return error;
	}

	if (I2C_OP_READ_P(op)) {
		uint32_t data = P2WI_READ(sc, AWIN_A31_P2WI_DATA0_REG);
		switch (len) {
		case 4:
			*(uint32_t *)buf = data;
			break;
		case 2:
			*(uint16_t *)buf = data & 0xffff;
			break;
		case 1:
			*(uint8_t *)buf = data & 0xff;
			break;
		default:
			return EINVAL;
		}
	}

	return 0;
}
