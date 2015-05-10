/* $NetBSD: tegra_i2c.c,v 1.1 2015/05/10 23:50:21 jmcneill Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: tegra_i2c.c,v 1.1 2015/05/10 23:50:21 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/i2c/i2cvar.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_i2creg.h>
#include <arm/nvidia/tegra_var.h>

static int	tegra_i2c_match(device_t, cfdata_t, void *);
static void	tegra_i2c_attach(device_t, device_t, void *);

struct tegra_i2c_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	void *			sc_ih;

	struct i2c_controller	sc_ic;
	kmutex_t		sc_lock;
	kcondvar_t		sc_cv;
	device_t		sc_i2cdev;
};

static void	tegra_i2c_init(struct tegra_i2c_softc *);
static int	tegra_i2c_intr(void *);

static int	tegra_i2c_acquire_bus(void *, int);
static void	tegra_i2c_release_bus(void *, int);
static int	tegra_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *,
			       size_t, void *, size_t, int);

static int	tegra_i2c_wait(struct tegra_i2c_softc *, int);
static int	tegra_i2c_write(struct tegra_i2c_softc *, i2c_addr_t,
				const uint8_t *, size_t, int);
static int	tegra_i2c_read(struct tegra_i2c_softc *, i2c_addr_t, uint8_t *,
			       size_t, int);

CFATTACH_DECL_NEW(tegra_i2c, sizeof(struct tegra_i2c_softc),
	tegra_i2c_match, tegra_i2c_attach, NULL, NULL);

#define I2C_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define I2C_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define I2C_SET_CLEAR(sc, reg, setval, clrval) \
    tegra_reg_set_clear((sc)->sc_bst, (sc)->sc_bsh, (reg), (setval), (clrval))

static int
tegra_i2c_match(device_t parent, cfdata_t cf, void *aux)
{
	struct tegraio_attach_args * const tio = aux;
	const struct tegra_locators * const loc = &tio->tio_loc;

	if (loc->loc_port == TEGRAIOCF_PORT_DEFAULT)
		return 0;

	return 1;
}

static void
tegra_i2c_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_i2c_softc * const sc = device_private(self);
	struct tegraio_attach_args * const tio = aux;
	const struct tegra_locators * const loc = &tio->tio_loc;
	struct i2cbus_attach_args iba;

	sc->sc_dev = self;
	sc->sc_bst = tio->tio_bst;
	bus_space_subregion(tio->tio_bst, tio->tio_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&sc->sc_cv, device_xname(self));

	aprint_naive("\n");
	aprint_normal(": I2C%d\n", loc->loc_port + 1);

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_VM, IST_LEVEL|IST_MPSAFE,
	    tegra_i2c_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", loc->loc_intr);

	/* Recommended setting for standard mode */
	tegra_car_periph_i2c_enable(loc->loc_port, 204000000);

	tegra_i2c_init(sc);

	sc->sc_ic.ic_cookie = sc;
	sc->sc_ic.ic_acquire_bus = tegra_i2c_acquire_bus;
	sc->sc_ic.ic_release_bus = tegra_i2c_release_bus;
	sc->sc_ic.ic_exec = tegra_i2c_exec;

	iba.iba_tag = &sc->sc_ic;
	sc->sc_i2cdev = config_found_ia(self, "i2cbus", &iba, iicbus_print);
}

static void
tegra_i2c_init(struct tegra_i2c_softc *sc)
{
	I2C_WRITE(sc, I2C_CLK_DIVISOR_REG,
	    __SHIFTIN(0x19, I2C_CLK_DIVISOR_STD_FAST_MODE) |
	    __SHIFTIN(0x1, I2C_CLK_DIVISOR_HSMODE));

	I2C_WRITE(sc, I2C_INTERRUPT_MASK_REG, 0);
	I2C_WRITE(sc, I2C_CNFG_REG, I2C_CNFG_NEW_MASTER_FSM);
	I2C_SET_CLEAR(sc, I2C_SL_CNFG_REG, I2C_SL_CNFG_NEWSL, 0);
}

static int
tegra_i2c_intr(void *priv)
{
	struct tegra_i2c_softc * const sc = priv;

	const uint32_t istatus = I2C_READ(sc, I2C_INTERRUPT_STATUS_REG);
	if (istatus == 0)
		return 0;
	I2C_WRITE(sc, I2C_INTERRUPT_STATUS_REG, istatus);

	mutex_enter(&sc->sc_lock);
	cv_broadcast(&sc->sc_cv);
	mutex_exit(&sc->sc_lock);

	return 1;
}

static int
tegra_i2c_acquire_bus(void *priv, int flags)
{
	struct tegra_i2c_softc * const sc = priv;

	mutex_enter(&sc->sc_lock);

	return 0;
}

static void
tegra_i2c_release_bus(void *priv, int flags)
{
	struct tegra_i2c_softc * const sc = priv;

	mutex_exit(&sc->sc_lock);
}

static int
tegra_i2c_exec(void *priv, i2c_op_t op, i2c_addr_t addr, const void *cmdbuf,
    size_t cmdlen, void *buf, size_t buflen, int flags)
{
	struct tegra_i2c_softc * const sc = priv;
	int retry, error;

#if notyet
	if (cold)
#endif
		flags |= I2C_F_POLL;

	KASSERT(mutex_owned(&sc->sc_lock));

	if ((flags & I2C_F_POLL) == 0) {
		I2C_WRITE(sc, I2C_INTERRUPT_MASK_REG,
		    I2C_INTERRUPT_MASK_NOACK | I2C_INTERRUPT_MASK_ARB_LOST |
		    I2C_INTERRUPT_MASK_TIMEOUT |
		    I2C_INTERRUPT_MASK_ALL_PACKETS_XFER_COMPLETE);
	}

	const uint32_t flush_mask =
	    I2C_FIFO_CONTROL_TX_FIFO_FLUSH | I2C_FIFO_CONTROL_RX_FIFO_FLUSH;

	I2C_SET_CLEAR(sc, I2C_FIFO_CONTROL_REG, flush_mask, 0);
	for (retry = 10000; retry > 0; retry--) {
		const uint32_t v = I2C_READ(sc, I2C_FIFO_CONTROL_REG);
		if ((v & flush_mask) == 0)
			break;
		delay(1);
	}
	if (retry == 0) {
		device_printf(sc->sc_dev, "timeout flushing FIFO\n");
		return EIO;
	}

	if (cmdlen > 0) {
		error = tegra_i2c_write(sc, addr, cmdbuf, cmdlen, flags);
		if (error) {
			goto done;
		}
	}

	if (I2C_OP_READ_P(op)) {
		error = tegra_i2c_read(sc, addr, buf, buflen, flags);
	} else {
		error = tegra_i2c_write(sc, addr, buf, buflen, flags);
	}

done:
	if ((flags & I2C_F_POLL) == 0) {
		I2C_WRITE(sc, I2C_INTERRUPT_MASK_REG, 0);
	}
	return error;
}

static int
tegra_i2c_wait(struct tegra_i2c_softc *sc, int flags)
{
	const struct timeval timeout = { .tv_sec = 1, .tv_usec = 0 };
	struct timeval tnow, tend;
	uint32_t stat;
	int error;

	getmicrotime(&tnow);
	timeradd(&tnow, &timeout, &tend);

	for (;;) {
		getmicrotime(&tnow);
		if (timercmp(&tnow, &tend, >=)) {
			return ETIMEDOUT;
		}
		if ((flags & I2C_F_POLL) == 0) {
			struct timeval trem;
			timersub(&tend, &tnow, &trem);
			const u_int ms = (trem.tv_sec * 1000) +
			    (trem.tv_usec / 1000);
			KASSERT(ms > 0);
			error = cv_timedwait_sig(&sc->sc_cv, &sc->sc_lock,
			    max(mstohz(ms), 1));
			if (error) {
				return error;
			}
		}
		stat = I2C_READ(sc, I2C_STATUS_REG);
		if ((stat & I2C_STATUS_BUSY) == 0) {
			break;
		}
		if (flags & I2C_F_POLL) {
			delay(1);
		}
	}


	if (__SHIFTOUT(stat, I2C_STATUS_CMD1_STAT) != 0)
		return EIO;

	return 0;
}

static int
tegra_i2c_write(struct tegra_i2c_softc *sc, i2c_addr_t addr, const uint8_t *buf,
    size_t buflen, int flags)
{
	uint32_t data, cnfg;
	size_t n;

	if (buflen > 4)
		return EINVAL;

	I2C_WRITE(sc, I2C_CMD_ADDR0_REG, addr << 1);
	for (n = 0, data = 0; n < buflen; n++) {
		data |= (uint32_t)buf[n] << (n * 8);
	}
	I2C_WRITE(sc, I2C_CMD_DATA1_REG, data);

	cnfg = I2C_READ(sc, I2C_CNFG_REG);
	cnfg &= ~I2C_CNFG_DEBOUNCE_CNT;
	cnfg |= __SHIFTIN(2, I2C_CNFG_DEBOUNCE_CNT);
	cnfg &= ~I2C_CNFG_LENGTH;
	cnfg |= __SHIFTIN(buflen - 1, I2C_CNFG_LENGTH);
	cnfg &= ~I2C_CNFG_SLV2;
	cnfg &= ~I2C_CNFG_CMD1;
	cnfg &= ~I2C_CNFG_NOACK;
	cnfg &= ~I2C_CNFG_A_MOD;
	I2C_WRITE(sc, I2C_CNFG_REG, cnfg);

	I2C_SET_CLEAR(sc, I2C_BUS_CONFIG_LOAD_REG,
	    I2C_BUS_CONFIG_LOAD_MSTR_CONFIG_LOAD, 0);

	I2C_SET_CLEAR(sc, I2C_CNFG_REG, I2C_CNFG_SEND, 0);

	return tegra_i2c_wait(sc, flags);
}

static int
tegra_i2c_read(struct tegra_i2c_softc *sc, i2c_addr_t addr, uint8_t *buf,
    size_t buflen, int flags)
{
	uint32_t data, cnfg;
	int error;
	size_t n;

	if (buflen > 4)
		return EINVAL;

	I2C_WRITE(sc, I2C_CMD_ADDR0_REG, (addr << 1) | 1);
	cnfg = I2C_READ(sc, I2C_CNFG_REG);
	cnfg &= ~I2C_CNFG_SLV2;
	cnfg |= I2C_CNFG_CMD1;
	cnfg &= ~I2C_CNFG_LENGTH;
	cnfg |= __SHIFTIN(buflen - 1, I2C_CNFG_LENGTH);
	I2C_WRITE(sc, I2C_CNFG_REG, cnfg);

	I2C_SET_CLEAR(sc, I2C_CNFG_REG, I2C_CNFG_SEND, 0);

	error = tegra_i2c_wait(sc, flags);
	if (error)
		return error;

	data = I2C_READ(sc, I2C_CMD_DATA1_REG);
	for (n = 0; n < buflen; n++) {
		buf[n] = (data >> (n * 8)) & 0xff;
	}

	return 0;
}
