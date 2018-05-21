/* $NetBSD: sunxi_rsb.c,v 1.1.10.1 2018/05/21 04:35:59 pgoyette Exp $ */

/*-
 * Copyright (c) 2014-2017 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: sunxi_rsb.c,v 1.1.10.1 2018/05/21 04:35:59 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <dev/i2c/i2cvar.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_rsb.h>

enum sunxi_rsb_type {
	SUNXI_P2WI,
	SUNXI_RSB,
};

static const struct of_compat_data compat_data[] = {
	{ "allwinner,sun6i-a31-p2wi",	SUNXI_P2WI },
	{ "allwinner,sun8i-a23-rsb",	SUNXI_RSB },
	{ NULL }
};

#define RSB_ADDR_PMIC_PRIMARY	0x3a3
#define RSB_ADDR_PMIC_SECONDARY	0x745
#define RSB_ADDR_PERIPH_IC	0xe89

/*
 * Device address to Run-time address mappings.
 *
 * Run-time address (RTA) is an 8-bit value used to address the device during
 * a read or write transaction. The following are valid RTAs:
 *  0x17 0x2d 0x3a 0x4e 0x59 0x63 0x74 0x8b 0x9c 0xa6 0xb1 0xc5 0xd2 0xe8 0xff
 *
 * Allwinner uses RTA 0x2d for the primary PMIC, 0x3a for the secondary PMIC,
 * and 0x4e for the peripheral IC (where applicable).
 */
static const struct {
	uint16_t        addr;
	uint8_t         rta;
} rsb_rtamap[] = {
	{ .addr = RSB_ADDR_PMIC_PRIMARY,	.rta = 0x2d },
	{ .addr = RSB_ADDR_PMIC_SECONDARY,	.rta = 0x3a },
	{ .addr = RSB_ADDR_PERIPH_IC,		.rta = 0x4e },
	{ .addr = 0,				.rta = 0 }
};

struct sunxi_rsb_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	enum sunxi_rsb_type sc_type;
	struct i2c_controller sc_ic;
	kmutex_t sc_lock;
	kcondvar_t sc_cv;
	device_t sc_i2cdev;
	void *sc_ih;
	uint32_t sc_stat;

	uint16_t sc_rsb_last_da;
};

#define RSB_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define RSB_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	sunxi_rsb_acquire_bus(void *, int);
static void     sunxi_rsb_release_bus(void *, int);
static int	sunxi_rsb_exec(void *, i2c_op_t, i2c_addr_t, const void *,
			       size_t, void *, size_t, int);

static int	sunxi_rsb_intr(void *);
static int	sunxi_rsb_wait(struct sunxi_rsb_softc *, int);
static int	sunxi_rsb_rsb_config(struct sunxi_rsb_softc *,
				     uint8_t, i2c_addr_t, int);

static int	sunxi_rsb_match(device_t, cfdata_t, void *);
static void	sunxi_rsb_attach(device_t, device_t, void *);

static i2c_tag_t
sunxi_rsb_get_tag(device_t dev)
{
	struct sunxi_rsb_softc * const sc = device_private(dev);

	return &sc->sc_ic;
}

static const struct fdtbus_i2c_controller_func sunxi_rsb_funcs = {
	.get_tag = sunxi_rsb_get_tag,
};

CFATTACH_DECL_NEW(sunxi_rsb, sizeof(struct sunxi_rsb_softc),
	sunxi_rsb_match, sunxi_rsb_attach, NULL, NULL);

static int
sunxi_rsb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sunxi_rsb_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_rsb_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct i2cbus_attach_args iba;
	prop_dictionary_t devs;
	uint32_t address_cells;
	struct fdtbus_reset *rst;
	struct clk *clk;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": couldn't decode interrupt\n");
		return;
	}

	if ((clk = fdtbus_clock_get_index(phandle, 0)) != NULL)
		if (clk_enable(clk) != 0) {
			aprint_error(": couldn't enable clock\n");
			return;
		}
	if ((rst = fdtbus_reset_get_index(phandle, 0)) != NULL)
		if (fdtbus_reset_deassert(rst) != 0) {
			aprint_error(": couldn't de-assert reset\n");
			return;
		}

	sc->sc_dev = self;
	sc->sc_type = of_search_compatible(phandle, compat_data)->data;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SCHED);
	cv_init(&sc->sc_cv, "awinp2wi");

	aprint_naive("\n");
	aprint_normal(": %s\n", sc->sc_type == SUNXI_P2WI ? "P2WI" : "RSB");

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_VM, 0,
	    sunxi_rsb_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	/* Enable interrupts */
	RSB_WRITE(sc, RSB_INTE_REG,
	    RSB_INTE_LOAD_BSY_ENB |
	    RSB_INTE_TRANS_ERR_ENB |
	    RSB_INTE_TRANS_OVER_ENB);
	RSB_WRITE(sc, RSB_CTRL_REG,
	    RSB_CTRL_GLOBAL_INT_ENB);

	sc->sc_ic.ic_cookie = sc;
	sc->sc_ic.ic_acquire_bus = sunxi_rsb_acquire_bus;
	sc->sc_ic.ic_release_bus = sunxi_rsb_release_bus;
	sc->sc_ic.ic_exec = sunxi_rsb_exec;

	fdtbus_register_i2c_controller(self, phandle, &sunxi_rsb_funcs);

	devs = prop_dictionary_create();
	if (of_getprop_uint32(phandle, "#address-cells", &address_cells))
		address_cells = 1;

	of_enter_i2c_devs(devs, phandle, address_cells * 4, 0);

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_ic;
	iba.iba_child_devices = prop_dictionary_get(devs, "i2c-child-devices");
	if (iba.iba_child_devices)
		prop_object_retain(iba.iba_child_devices);
	prop_object_release(devs);

	sc->sc_i2cdev = config_found_ia(self, "i2cbus", &iba, iicbus_print);
}

static int
sunxi_rsb_intr(void *priv)
{
	struct sunxi_rsb_softc *sc = priv;
	uint32_t stat;

	stat = RSB_READ(sc, RSB_STAT_REG);
	if ((stat & RSB_STAT_MASK) == 0)
		return 0;

	RSB_WRITE(sc, RSB_STAT_REG, stat & RSB_STAT_MASK);

	mutex_enter(&sc->sc_lock);
	sc->sc_stat |= stat;
	cv_broadcast(&sc->sc_cv);
	mutex_exit(&sc->sc_lock);

	return 1;
}

static int
sunxi_rsb_wait(struct sunxi_rsb_softc *sc, int flags)
{
	int error = 0, retry;

	/* Wait up to 5 seconds for a transfer to complete */
	sc->sc_stat = 0;
	for (retry = (flags & I2C_F_POLL) ? 100 : 5; retry > 0; retry--) {
		if (flags & I2C_F_POLL) {
			sc->sc_stat |= RSB_READ(sc, RSB_STAT_REG);
		} else {
			error = cv_timedwait(&sc->sc_cv, &sc->sc_lock, hz);
			if (error && error != EWOULDBLOCK) {
				break;
			}
		}
		if (sc->sc_stat & RSB_STAT_MASK) {
			break;
		}
		if (flags & I2C_F_POLL) {
			delay(10000);
		}
	}
	if (retry == 0)
		error = EAGAIN;

	if (flags & I2C_F_POLL) {
		RSB_WRITE(sc, RSB_STAT_REG,
		    sc->sc_stat & RSB_STAT_MASK);
	}

	if (error) {
		/* Abort transaction */
		device_printf(sc->sc_dev, "transfer timeout, error = %d\n",
		    error);
		RSB_WRITE(sc, RSB_CTRL_REG,
		    RSB_CTRL_ABORT_TRANS);
		return error;
	}

	if (sc->sc_stat & RSB_STAT_LOAD_BSY) {
		device_printf(sc->sc_dev, "transfer busy\n");
		return EBUSY;
	}
	if (sc->sc_stat & RSB_STAT_TRANS_ERR) {
		device_printf(sc->sc_dev, "transfer error, id 0x%02llx\n",
		    __SHIFTOUT(sc->sc_stat, RSB_STAT_TRANS_ERR_ID));
		return EIO;
	}

	return 0;
}

static int
sunxi_rsb_rsb_config(struct sunxi_rsb_softc *sc, uint8_t rta, i2c_addr_t da,
    int flags)
{
	uint32_t dar, ctrl;

	KASSERT(mutex_owned(&sc->sc_lock));

	RSB_WRITE(sc, RSB_STAT_REG,
	    RSB_READ(sc, RSB_STAT_REG) & RSB_STAT_MASK);

	dar = __SHIFTIN(rta, RSB_DAR_RTA);
	dar |= __SHIFTIN(da, RSB_DAR_DA);
	RSB_WRITE(sc, RSB_DAR_REG, dar);
	RSB_WRITE(sc, RSB_CMD_REG, RSB_CMD_IDX_SRTA);

	/* Make sure the controller is idle */
	ctrl = RSB_READ(sc, RSB_CTRL_REG);
	if (ctrl & RSB_CTRL_START_TRANS) {
		device_printf(sc->sc_dev, "device is busy\n");
		return EBUSY;
	}

	/* Start the transfer */
	RSB_WRITE(sc, RSB_CTRL_REG,
	    ctrl | RSB_CTRL_START_TRANS);

	return sunxi_rsb_wait(sc, flags);
}

static int
sunxi_rsb_acquire_bus(void *priv, int flags)
{
	struct sunxi_rsb_softc *sc = priv;

	if (flags & I2C_F_POLL) {
		if (!mutex_tryenter(&sc->sc_lock))
			return EBUSY;
	} else {
		mutex_enter(&sc->sc_lock);
	}

	return 0;
}

static void
sunxi_rsb_release_bus(void *priv, int flags)
{
	struct sunxi_rsb_softc *sc = priv;

	mutex_exit(&sc->sc_lock);
}

static int
sunxi_rsb_exec(void *priv, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *buf, size_t len, int flags)
{
	struct sunxi_rsb_softc *sc = priv;
	uint32_t dlen, ctrl;
	uint8_t rta;
	int error, i;

	KASSERT(mutex_owned(&sc->sc_lock));

	if (cmdlen != 1 || (len != 1 && len != 2 && len != 4))
		return EINVAL;

	if (sc->sc_type == SUNXI_RSB && sc->sc_rsb_last_da != addr) {
		/* Lookup run-time address for given device address */
		for (rta = 0, i = 0; rsb_rtamap[i].rta != 0; i++)
			if (rsb_rtamap[i].addr == addr) {
				rta = rsb_rtamap[i].rta;
				break;
			}
		if (rta == 0) {
			device_printf(sc->sc_dev,
			    "RTA not known for address %#x\n", addr);
			return ENXIO;
		}
		error = sunxi_rsb_rsb_config(sc, rta, addr, flags);
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
	RSB_WRITE(sc, RSB_DADDR0_REG, *(const uint8_t *)cmdbuf);

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
		RSB_WRITE(sc, RSB_DATA0_REG, data);
	}

	if (sc->sc_type == SUNXI_RSB) {
		uint8_t cmd;
		if (I2C_OP_WRITE_P(op)) {
			switch (len) {
			case 1:	cmd = RSB_CMD_IDX_WR8; break;
			case 2: cmd = RSB_CMD_IDX_WR16; break;
			case 4: cmd = RSB_CMD_IDX_WR32; break;
			default: return EINVAL;
			}
		} else {
			switch (len) {
			case 1:	cmd = RSB_CMD_IDX_RD8; break;
			case 2: cmd = RSB_CMD_IDX_RD16; break;
			case 4: cmd = RSB_CMD_IDX_RD32; break;
			default: return EINVAL;
			}
		}
		RSB_WRITE(sc, RSB_CMD_REG, cmd);
	}

	/* Program data length register; if reading, set read/write bit */
	dlen = __SHIFTIN(len - 1, RSB_DLEN_ACCESS_LENGTH);
	if (I2C_OP_READ_P(op)) {
		dlen |= RSB_DLEN_READ_WRITE_FLAG;
	}
	RSB_WRITE(sc, RSB_DLEN_REG, dlen);

	/* Make sure the controller is idle */
	ctrl = RSB_READ(sc, RSB_CTRL_REG);
	if (ctrl & RSB_CTRL_START_TRANS) {
		device_printf(sc->sc_dev, "device is busy\n");
		return EBUSY;
	}

	/* Start the transfer */
	RSB_WRITE(sc, RSB_CTRL_REG,
	    ctrl | RSB_CTRL_START_TRANS);

	error = sunxi_rsb_wait(sc, flags);
	if (error) {
		return error;
	}

	if (I2C_OP_READ_P(op)) {
		uint32_t data = RSB_READ(sc, RSB_DATA0_REG);
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
