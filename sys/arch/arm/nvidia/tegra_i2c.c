/* $NetBSD: tegra_i2c.c,v 1.20 2018/08/05 15:37:10 skrll Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_i2c.c,v 1.20 2018/08/05 15:37:10 skrll Exp $");

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

#include <dev/fdt/fdtvar.h>

static int	tegra_i2c_match(device_t, cfdata_t, void *);
static void	tegra_i2c_attach(device_t, device_t, void *);

static i2c_tag_t tegra_i2c_get_tag(device_t);

struct fdtbus_i2c_controller_func tegra_i2c_funcs = {
	.get_tag = tegra_i2c_get_tag
};

struct tegra_i2c_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	void *			sc_ih;
	struct clk *		sc_clk;
	struct fdtbus_reset *	sc_rst;
	u_int			sc_cid;

	struct i2c_controller	sc_ic;
	kmutex_t		sc_lock;
	kcondvar_t		sc_cv;
};

static void	tegra_i2c_init(struct tegra_i2c_softc *);
static int	tegra_i2c_intr(void *);

static int	tegra_i2c_acquire_bus(void *, int);
static void	tegra_i2c_release_bus(void *, int);
static int	tegra_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *,
			       size_t, void *, size_t, int);

static int	tegra_i2c_wait(struct tegra_i2c_softc *, int);
static int	tegra_i2c_write(struct tegra_i2c_softc *, i2c_addr_t,
				const uint8_t *, size_t, int, bool);
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
	const char * const compatible[] = {
		"nvidia,tegra210-i2c",
		"nvidia,tegra124-i2c",
		"nvidia,tegra114-i2c",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra_i2c_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_i2c_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	sc->sc_clk = fdtbus_clock_get(phandle, "div-clk");
	if (sc->sc_clk == NULL) {
		aprint_error(": couldn't get clock div-clk\n");
		return;
	}
	sc->sc_rst = fdtbus_reset_get(phandle, "i2c");
	if (sc->sc_rst == NULL) {
		aprint_error(": couldn't get reset i2c\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_cid = device_unit(self);
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#" PRIxBUSADDR ": %d",
		    addr, error);
		return;
	}
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&sc->sc_cv, device_xname(self));

	aprint_naive("\n");
	aprint_normal(": I2C\n");

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_VM,
	    FDT_INTR_MPSAFE, tegra_i2c_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	/*
	 * Recommended setting for standard mode is to use an I2C source div
	 * of 20 (Tegra K1 Technical Reference Manual, Table 137)
	 */
	fdtbus_reset_assert(sc->sc_rst);
	error = clk_set_rate(sc->sc_clk, 20400000);
	if (error) {
		aprint_error_dev(self, "couldn't set frequency: %d\n", error);
		return;
	}
	error = clk_enable(sc->sc_clk);
	if (error) {
		aprint_error_dev(self, "couldn't enable clock: %d\n", error);
		return;
	}
	fdtbus_reset_deassert(sc->sc_rst);

	tegra_i2c_init(sc);

	sc->sc_ic.ic_cookie = sc;
	sc->sc_ic.ic_acquire_bus = tegra_i2c_acquire_bus;
	sc->sc_ic.ic_release_bus = tegra_i2c_release_bus;
	sc->sc_ic.ic_exec = tegra_i2c_exec;

	fdtbus_register_i2c_controller(self, phandle, &tegra_i2c_funcs);

	fdtbus_attach_i2cbus(self, phandle, &sc->sc_ic, iicbus_print);
}

static i2c_tag_t
tegra_i2c_get_tag(device_t dev)
{
	struct tegra_i2c_softc * const sc = device_private(dev);

	return &sc->sc_ic;
}

static void
tegra_i2c_init(struct tegra_i2c_softc *sc)
{
	int retry = 10000;

	I2C_WRITE(sc, I2C_CLK_DIVISOR_REG,
	    __SHIFTIN(0x19, I2C_CLK_DIVISOR_STD_FAST_MODE) |
	    __SHIFTIN(0x1, I2C_CLK_DIVISOR_HSMODE));

	I2C_WRITE(sc, I2C_INTERRUPT_MASK_REG, 0);
	I2C_WRITE(sc, I2C_CNFG_REG,
	    I2C_CNFG_NEW_MASTER_FSM | I2C_CNFG_PACKET_MODE_EN);
	I2C_SET_CLEAR(sc, I2C_SL_CNFG_REG, I2C_SL_CNFG_NEWSL, 0);
	I2C_WRITE(sc, I2C_FIFO_CONTROL_REG,
	    __SHIFTIN(7, I2C_FIFO_CONTROL_TX_FIFO_TRIG) |
	    __SHIFTIN(0, I2C_FIFO_CONTROL_RX_FIFO_TRIG));

	I2C_WRITE(sc, I2C_BUS_CONFIG_LOAD_REG,
	    I2C_BUS_CONFIG_LOAD_MSTR_CONFIG_LOAD);
	while (--retry > 0) {
		if (I2C_READ(sc, I2C_BUS_CONFIG_LOAD_REG) == 0)
			break;
		delay(10);
	}
	if (retry == 0) {
		device_printf(sc->sc_dev, "config load timeout\n");
	}
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
		error = tegra_i2c_write(sc, addr, cmdbuf, cmdlen, flags,
		    buflen > 0 ? true : false);
		if (error) {
			goto done;
		}
	}

	if (I2C_OP_READ_P(op)) {
		error = tegra_i2c_read(sc, addr, buf, buflen, flags);
	} else {
		error = tegra_i2c_write(sc, addr, buf, buflen, flags, false);
	}

done:
	if ((flags & I2C_F_POLL) == 0) {
		I2C_WRITE(sc, I2C_INTERRUPT_MASK_REG, 0);
	}

	if (error) {
		tegra_i2c_init(sc);
	}

	return error;
}

static int
tegra_i2c_wait(struct tegra_i2c_softc *sc, int flags)
{
	int error, retry;
	uint32_t stat = 0;

	retry = (flags & I2C_F_POLL) ? 100000 : 100;

	while (--retry > 0) {
		if ((flags & I2C_F_POLL) == 0) {
			error = cv_timedwait_sig(&sc->sc_cv, &sc->sc_lock,
			    max(mstohz(10), 1));
			if (error) {
				return error;
			}
		}
		stat = I2C_READ(sc, I2C_INTERRUPT_STATUS_REG);
		if (stat & I2C_INTERRUPT_STATUS_PACKET_XFER_COMPLETE) {
			break;
		}
		if (flags & I2C_F_POLL) {
			delay(10);
		}
	}
	if (retry == 0) {
		stat = I2C_READ(sc, I2C_INTERRUPT_STATUS_REG);
		device_printf(sc->sc_dev, "timed out, status = %#x\n", stat);
		return ETIMEDOUT;
	}

	const uint32_t err_mask =
	    I2C_INTERRUPT_STATUS_NOACK |
	    I2C_INTERRUPT_STATUS_ARB_LOST |
	    I2C_INTERRUPT_MASK_TIMEOUT;

	if (stat & err_mask) {
		device_printf(sc->sc_dev, "error, status = %#x\n", stat);
		return EIO;
	}

	return 0;
}

static int
tegra_i2c_write(struct tegra_i2c_softc *sc, i2c_addr_t addr, const uint8_t *buf,
    size_t buflen, int flags, bool repeat_start)
{
	const uint8_t *p = buf;
	size_t n, resid = buflen;
	uint32_t data;
	int retry;

	const uint32_t istatus = I2C_READ(sc, I2C_INTERRUPT_STATUS_REG);
	I2C_WRITE(sc, I2C_INTERRUPT_STATUS_REG, istatus);

	/* Generic Header 0 */
	I2C_WRITE(sc, I2C_TX_PACKET_FIFO_REG,
	    __SHIFTIN(I2C_IOPACKET_WORD0_PROTHDRSZ_REQ,
		      I2C_IOPACKET_WORD0_PROTHDRSZ) |
	    __SHIFTIN(sc->sc_cid, I2C_IOPACKET_WORD0_CONTROLLERID) |
	    __SHIFTIN(1, I2C_IOPACKET_WORD0_PKTID) |
	    __SHIFTIN(I2C_IOPACKET_WORD0_PROTOCOL_I2C,
		      I2C_IOPACKET_WORD0_PROTOCOL) |
	    __SHIFTIN(I2C_IOPACKET_WORD0_PKTTYPE_REQ,
		      I2C_IOPACKET_WORD0_PKTTYPE));
	/* Generic Header 1 */
	I2C_WRITE(sc, I2C_TX_PACKET_FIFO_REG,
	    __SHIFTIN(buflen - 1, I2C_IOPACKET_WORD1_PAYLOADSIZE));
	/* I2C Master Transmit Packet Header */
	I2C_WRITE(sc, I2C_TX_PACKET_FIFO_REG,
	    I2C_IOPACKET_XMITHDR_IE |
	    (repeat_start ? I2C_IOPACKET_XMITHDR_REPEAT_STARTSTOP : 0) |
	    __SHIFTIN((addr << 1), I2C_IOPACKET_XMITHDR_SLAVE_ADDR));

	/* Transmit data */
	while (resid > 0) {
		retry = 10000;
		while (--retry > 0) {
			const uint32_t fs = I2C_READ(sc, I2C_FIFO_STATUS_REG);
			const u_int cnt =
			    __SHIFTOUT(fs, I2C_FIFO_STATUS_TX_FIFO_EMPTY_CNT);
			if (cnt > 0)
				break;
			delay(10);
		}
		if (retry == 0) {
			device_printf(sc->sc_dev, "TX FIFO timeout\n");
			return ETIMEDOUT;
		}

		for (n = 0, data = 0; n < min(resid, 4); n++) {
			data |= (uint32_t)p[n] << (n * 8);
		}
		I2C_WRITE(sc, I2C_TX_PACKET_FIFO_REG, data);
		p += min(resid, 4);
		resid -= min(resid, 4);
	}

	return tegra_i2c_wait(sc, flags);
}

static int
tegra_i2c_read(struct tegra_i2c_softc *sc, i2c_addr_t addr, uint8_t *buf,
    size_t buflen, int flags)
{
	uint8_t *p = buf;
	size_t n, resid = buflen;
	uint32_t data;
	int retry;

	const uint32_t istatus = I2C_READ(sc, I2C_INTERRUPT_STATUS_REG);
	I2C_WRITE(sc, I2C_INTERRUPT_STATUS_REG, istatus);

	/* Generic Header 0 */
	I2C_WRITE(sc, I2C_TX_PACKET_FIFO_REG,
	    __SHIFTIN(I2C_IOPACKET_WORD0_PROTHDRSZ_REQ,
		      I2C_IOPACKET_WORD0_PROTHDRSZ) |
	    __SHIFTIN(sc->sc_cid, I2C_IOPACKET_WORD0_CONTROLLERID) |
	    __SHIFTIN(1, I2C_IOPACKET_WORD0_PKTID) |
	    __SHIFTIN(I2C_IOPACKET_WORD0_PROTOCOL_I2C,
		      I2C_IOPACKET_WORD0_PROTOCOL) |
	    __SHIFTIN(I2C_IOPACKET_WORD0_PKTTYPE_REQ,
		      I2C_IOPACKET_WORD0_PKTTYPE));
	/* Generic Header 1 */
	I2C_WRITE(sc, I2C_TX_PACKET_FIFO_REG,
	    __SHIFTIN(buflen - 1, I2C_IOPACKET_WORD1_PAYLOADSIZE));
	/* I2C Master Transmit Packet Header */
	I2C_WRITE(sc, I2C_TX_PACKET_FIFO_REG,
	    I2C_IOPACKET_XMITHDR_IE | I2C_IOPACKET_XMITHDR_READ |
	    __SHIFTIN((addr << 1) | 1, I2C_IOPACKET_XMITHDR_SLAVE_ADDR));

	while (resid > 0) {
		retry = 10000;
		while (--retry > 0) {
			const uint32_t fs = I2C_READ(sc, I2C_FIFO_STATUS_REG);
			const u_int cnt =
			    __SHIFTOUT(fs, I2C_FIFO_STATUS_RX_FIFO_FULL_CNT);
			if (cnt > 0)
				break;
			delay(10);
		}
		if (retry == 0) {
			device_printf(sc->sc_dev, "RX FIFO timeout\n");
			return ETIMEDOUT;
		}

		data = I2C_READ(sc, I2C_RX_FIFO_REG);
		for (n = 0; n < min(resid, 4); n++) {
			p[n] = (data >> (n * 8)) & 0xff;
		}
		p += min(resid, 4);
		resid -= min(resid, 4);
	}

	return tegra_i2c_wait(sc, flags);
}
