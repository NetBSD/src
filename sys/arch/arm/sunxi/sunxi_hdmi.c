/* $NetBSD: sunxi_hdmi.c,v 1.3.2.2 2018/04/07 04:12:12 pgoyette Exp $ */

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

#include "opt_ddb.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunxi_hdmi.c,v 1.3.2.2 2018/04/07 04:12:12 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/kthread.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_port.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/ddcvar.h>
#include <dev/i2c/ddcreg.h>
#include <dev/videomode/videomode.h>
#include <dev/videomode/edidvar.h>

#include <arm/sunxi/sunxi_hdmireg.h>
#include <arm/sunxi/sunxi_display.h>

enum sunxi_hdmi_type {
	HDMI_A10 = 1,
	HDMI_A31,
};

struct sunxi_hdmi_softc {
	device_t sc_dev;
	int sc_phandle;
	enum sunxi_hdmi_type sc_type;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	struct clk *sc_clk_ahb;
	struct clk *sc_clk_mod;
	struct clk *sc_clk_pll0;
	struct clk *sc_clk_pll1;
	void *sc_ih;
	lwp_t *sc_thread;

	struct i2c_controller sc_ic;
	kmutex_t sc_ic_lock;

	bool sc_display_connected;
	char sc_display_vendor[16];
	char sc_display_product[16];

	u_int sc_display_mode;
	u_int sc_current_display_mode;
#define DISPLAY_MODE_AUTO	0
#define DISPLAY_MODE_HDMI	1
#define DISPLAY_MODE_DVI	2
	
	kmutex_t sc_pwr_lock;
	int	sc_pwr_refcount; /* reference who needs HDMI */

	uint32_t sc_ver;
	unsigned int sc_i2c_blklen;

	struct fdt_device_ports sc_ports;
	struct fdt_endpoint *sc_in_ep;
	struct fdt_endpoint *sc_in_rep;
	struct fdt_endpoint *sc_out_ep;
};

#define HDMI_READ(sc, reg)			\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define HDMI_WRITE(sc, reg, val)		\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val));

#define HDMI_1_3_P(sc)	((sc)->sc_ver == 0x00010003)
#define HDMI_1_4_P(sc)	((sc)->sc_ver == 0x00010004)

static const struct of_compat_data compat_data[] = {
	{"allwinner,sun4i-a10-hdmi", HDMI_A10},
	{"allwinner,sun7i-a20-hdmi", HDMI_A10},
	{NULL}
};

static int	sunxi_hdmi_match(device_t, cfdata_t, void *);
static void	sunxi_hdmi_attach(device_t, device_t, void *);
static void	sunxi_hdmi_i2c_init(struct sunxi_hdmi_softc *);
static int	sunxi_hdmi_i2c_acquire_bus(void *, int);
static void	sunxi_hdmi_i2c_release_bus(void *, int);
static int	sunxi_hdmi_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *,
				   size_t, void *, size_t, int);
static int	sunxi_hdmi_i2c_xfer(void *, i2c_addr_t, uint8_t, uint8_t,
				   size_t, int, int);
static int	sunxi_hdmi_i2c_reset(struct sunxi_hdmi_softc *, int);

static int	sunxi_hdmi_ep_activate(device_t, struct fdt_endpoint *, bool);
static int	sunxi_hdmi_ep_enable(device_t, struct fdt_endpoint *, bool);
static void	sunxi_hdmi_do_enable(struct sunxi_hdmi_softc *);
static void	sunxi_hdmi_read_edid(struct sunxi_hdmi_softc *);
static int	sunxi_hdmi_read_edid_block(struct sunxi_hdmi_softc *, uint8_t *,
					  uint8_t);
static u_int	sunxi_hdmi_get_display_mode(struct sunxi_hdmi_softc *,
					   const struct edid_info *);
static void	sunxi_hdmi_video_enable(struct sunxi_hdmi_softc *, bool);
static void	sunxi_hdmi_set_videomode(struct sunxi_hdmi_softc *,
					const struct videomode *, u_int);
static void	sunxi_hdmi_set_audiomode(struct sunxi_hdmi_softc *,
					const struct videomode *, u_int);
static void	sunxi_hdmi_hpd(struct sunxi_hdmi_softc *);
static void	sunxi_hdmi_thread(void *);
static int	sunxi_hdmi_poweron(struct sunxi_hdmi_softc *, bool);
#if 0
static int	sunxi_hdmi_intr(void *);
#endif

#if defined(DDB)
void		sunxi_hdmi_dump_regs(void);
#endif

CFATTACH_DECL_NEW(sunxi_hdmi, sizeof(struct sunxi_hdmi_softc),
	sunxi_hdmi_match, sunxi_hdmi_attach, NULL, NULL);

static int
sunxi_hdmi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
sunxi_hdmi_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_hdmi_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	uint32_t ver;
	int error;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;

	sc->sc_type = of_search_compatible(faa->faa_phandle, compat_data)->data;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	sc->sc_clk_ahb = fdtbus_clock_get(phandle, "ahb");
	sc->sc_clk_mod = fdtbus_clock_get(phandle, "mod");
	sc->sc_clk_pll0 = fdtbus_clock_get(phandle, "pll-0");
	sc->sc_clk_pll1 = fdtbus_clock_get(phandle, "pll-1");

	if (sc->sc_clk_ahb == NULL || sc->sc_clk_mod == NULL
	    || sc->sc_clk_pll0 == NULL || sc->sc_clk_pll1 == NULL) {
		aprint_error(": couldn't get clocks\n");
		aprint_debug_dev(self, "clk ahb %s mod %s pll-0 %s pll-1 %s\n",
		    sc->sc_clk_ahb == NULL ? "missing" : "present",
		    sc->sc_clk_mod == NULL ? "missing" : "present",
		    sc->sc_clk_pll0 == NULL ? "missing" : "present",
		    sc->sc_clk_pll1 == NULL ? "missing" : "present");
		return;
	}

	error = clk_disable(sc->sc_clk_mod);
	if (error) {
		aprint_error(": couldn't disable mod clock\n");
		return;
	}

	if (clk_enable(sc->sc_clk_ahb) != 0) {
		aprint_error(": couldn't enable ahb clock\n");
		return;
	}
#if defined(SUNXI_HDMI_DEBUG)
	sunxi_hdmi_dump_regs();
#endif

	/*
	 * reset device, in case it has been setup by firmware in an
	 * incompatible way
	 */
	for (int i = 0; i <= 0x500; i += 4) {
		HDMI_WRITE(sc, i, 0);
	}

	ver = HDMI_READ(sc, SUNXI_HDMI_VERSION_ID_REG);

	const int vmaj = __SHIFTOUT(ver, SUNXI_HDMI_VERSION_ID_H);
	const int vmin = __SHIFTOUT(ver, SUNXI_HDMI_VERSION_ID_L);

	aprint_naive("\n");
	aprint_normal(": HDMI %d.%d\n", vmaj, vmin);

	sc->sc_ver = ver;
	sc->sc_i2c_blklen = 16;

	sc->sc_ports.dp_ep_activate = sunxi_hdmi_ep_activate;
	sc->sc_ports.dp_ep_enable = sunxi_hdmi_ep_enable;
	fdt_ports_register(&sc->sc_ports, self, phandle, EP_OTHER);

	mutex_init(&sc->sc_pwr_lock, MUTEX_DEFAULT, IPL_NONE);
	sunxi_hdmi_i2c_init(sc);

	if (clk_disable(sc->sc_clk_ahb) != 0) {
		aprint_error(": couldn't disable ahb clock\n");
		return;
	}
}

static void
sunxi_hdmi_i2c_init(struct sunxi_hdmi_softc *sc)
{
	struct i2c_controller *ic = &sc->sc_ic;

	mutex_init(&sc->sc_ic_lock, MUTEX_DEFAULT, IPL_NONE);

	ic->ic_cookie = sc;
	ic->ic_acquire_bus = sunxi_hdmi_i2c_acquire_bus;
	ic->ic_release_bus = sunxi_hdmi_i2c_release_bus;
	ic->ic_exec = sunxi_hdmi_i2c_exec;
}

static int
sunxi_hdmi_i2c_acquire_bus(void *priv, int flags)
{
	struct sunxi_hdmi_softc *sc = priv;

	if (flags & I2C_F_POLL) {
		if (!mutex_tryenter(&sc->sc_ic_lock))
			return EBUSY;
	} else {
		mutex_enter(&sc->sc_ic_lock);
	}

	return 0;
}

static void
sunxi_hdmi_i2c_release_bus(void *priv, int flags)
{
	struct sunxi_hdmi_softc *sc = priv;

	mutex_exit(&sc->sc_ic_lock);
}

static int
sunxi_hdmi_i2c_exec(void *priv, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *buf, size_t len, int flags)
{
	struct sunxi_hdmi_softc *sc = priv;
	uint8_t *pbuf;
	uint8_t block;
	int resid;
	off_t off;
	int err;

	KASSERT(mutex_owned(&sc->sc_ic_lock));
	KASSERT(op == I2C_OP_READ_WITH_STOP);
	KASSERT(addr == DDC_ADDR);
	KASSERT(cmdlen > 0);
	KASSERT(buf != NULL);

	err = sunxi_hdmi_i2c_reset(sc, flags);
	if (err)
		goto done;

	block = *(const uint8_t *)cmdbuf;
	off = (block & 1) ? 128 : 0;

	pbuf = buf;
	resid = len;
	while (resid > 0) {
		size_t blklen = min(resid, sc->sc_i2c_blklen);

		err = sunxi_hdmi_i2c_xfer(sc, addr, block >> 1, off, blklen,
		      SUNXI_HDMI_DDC_COMMAND_ACCESS_CMD_EOREAD, flags);
		if (err)
			goto done;

		if (HDMI_1_3_P(sc)) {
			bus_space_read_multi_1(sc->sc_bst, sc->sc_bsh,
			    SUNXI_HDMI_DDC_FIFO_ACCESS_REG, pbuf, blklen);
		} else {
			bus_space_read_multi_1(sc->sc_bst, sc->sc_bsh,
			    SUNXI_A31_HDMI_DDC_FIFO_ACCESS_REG, pbuf, blklen);
		}

#ifdef SUNXI_HDMI_DEBUG
		printf("off=%d:", (int)off);
		for (int i = 0; i < blklen; i++)
			printf(" %02x", pbuf[i]);
		printf("\n");
#endif

		pbuf += blklen;
		off += blklen;
		resid -= blklen;
	}

done:
	return err;
}

static int
sunxi_hdmi_i2c_xfer_1_3(void *priv, i2c_addr_t addr, uint8_t block, uint8_t reg,
    size_t len, int type, int flags)
{
	struct sunxi_hdmi_softc *sc = priv;
	uint32_t val;
	int retry;

	val = HDMI_READ(sc, SUNXI_HDMI_DDC_CTRL_REG);
	val &= ~SUNXI_HDMI_DDC_CTRL_FIFO_DIR;
	HDMI_WRITE(sc, SUNXI_HDMI_DDC_CTRL_REG, val);

	val |= __SHIFTIN(block, SUNXI_HDMI_DDC_SLAVE_ADDR_0);
	val |= __SHIFTIN(0x60, SUNXI_HDMI_DDC_SLAVE_ADDR_1);
	val |= __SHIFTIN(reg, SUNXI_HDMI_DDC_SLAVE_ADDR_2);
	val |= __SHIFTIN(addr, SUNXI_HDMI_DDC_SLAVE_ADDR_3);
	HDMI_WRITE(sc, SUNXI_HDMI_DDC_SLAVE_ADDR_REG, val);

	val = HDMI_READ(sc, SUNXI_HDMI_DDC_FIFO_CTRL_REG);
	val |= SUNXI_HDMI_DDC_FIFO_CTRL_ADDR_CLEAR;
	HDMI_WRITE(sc, SUNXI_HDMI_DDC_FIFO_CTRL_REG, val);

	HDMI_WRITE(sc, SUNXI_HDMI_DDC_BYTE_COUNTER_REG, len);

	HDMI_WRITE(sc, SUNXI_HDMI_DDC_COMMAND_REG, type);

	val = HDMI_READ(sc, SUNXI_HDMI_DDC_CTRL_REG);
	val |= SUNXI_HDMI_DDC_CTRL_ACCESS_CMD_START;
	HDMI_WRITE(sc, SUNXI_HDMI_DDC_CTRL_REG, val);

	retry = 1000;
	while (--retry > 0) {
		val = HDMI_READ(sc, SUNXI_HDMI_DDC_CTRL_REG);
		if ((val & SUNXI_HDMI_DDC_CTRL_ACCESS_CMD_START) == 0)
			break;
		delay(1000);
	}
	if (retry == 0)
		return ETIMEDOUT;

	val = HDMI_READ(sc, SUNXI_HDMI_DDC_INT_STATUS_REG);
	if ((val & SUNXI_HDMI_DDC_INT_STATUS_TRANSFER_COMPLETE) == 0) {
		device_printf(sc->sc_dev, "xfer failed, status=%08x\n", val);
		return EIO;
	}

	return 0;
}

static int
sunxi_hdmi_i2c_xfer_1_4(void *priv, i2c_addr_t addr, uint8_t block, uint8_t reg,
    size_t len, int type, int flags)
{
	struct sunxi_hdmi_softc *sc = priv;
	uint32_t val;
	int retry;

	val = HDMI_READ(sc, SUNXI_A31_HDMI_DDC_FIFO_CTRL_REG);
	val |= SUNXI_A31_HDMI_DDC_FIFO_CTRL_RST;
	HDMI_WRITE(sc, SUNXI_A31_HDMI_DDC_FIFO_CTRL_REG, val);

	val = __SHIFTIN(block, SUNXI_A31_HDMI_DDC_SLAVE_ADDR_SEG_PTR);
	val |= __SHIFTIN(0x60, SUNXI_A31_HDMI_DDC_SLAVE_ADDR_DDC_CMD);
	val |= __SHIFTIN(reg, SUNXI_A31_HDMI_DDC_SLAVE_ADDR_OFF_ADR);
	val |= __SHIFTIN(addr, SUNXI_A31_HDMI_DDC_SLAVE_ADDR_DEV_ADR);
	HDMI_WRITE(sc, SUNXI_A31_HDMI_DDC_SLAVE_ADDR_REG, val);

	HDMI_WRITE(sc, SUNXI_A31_HDMI_DDC_COMMAND_REG,
	    __SHIFTIN(len, SUNXI_A31_HDMI_DDC_COMMAND_DTC) |
	    __SHIFTIN(type, SUNXI_A31_HDMI_DDC_COMMAND_CMD));

	val = HDMI_READ(sc, SUNXI_A31_HDMI_DDC_CTRL_REG);
	val |= SUNXI_A31_HDMI_DDC_CTRL_ACCESS_CMD_START;
	HDMI_WRITE(sc, SUNXI_A31_HDMI_DDC_CTRL_REG, val);

	retry = 1000;
	while (--retry > 0) {
		val = HDMI_READ(sc, SUNXI_A31_HDMI_DDC_CTRL_REG);
		if ((val & SUNXI_A31_HDMI_DDC_CTRL_ACCESS_CMD_START) == 0)
			break;
		if (cold)
			delay(1000);
		else
			kpause("hdmiddc", false, mstohz(10), &sc->sc_ic_lock);
	}
	if (retry == 0)
		return ETIMEDOUT;

	return 0;
}

static int
sunxi_hdmi_i2c_xfer(void *priv, i2c_addr_t addr, uint8_t block, uint8_t reg,
    size_t len, int type, int flags)
{
	struct sunxi_hdmi_softc *sc = priv;
	int rv;

	if (HDMI_1_3_P(sc)) {
		rv = sunxi_hdmi_i2c_xfer_1_3(priv, addr, block, reg, len,
		    type, flags);
	} else {
		rv = sunxi_hdmi_i2c_xfer_1_4(priv, addr, block, reg, len,
		    type, flags);
	}

	return rv;
}

static int
sunxi_hdmi_i2c_reset(struct sunxi_hdmi_softc *sc, int flags)
{
	uint32_t hpd, ctrl;

	hpd = HDMI_READ(sc, SUNXI_HDMI_HPD_REG);
	if ((hpd & SUNXI_HDMI_HPD_HOTPLUG_DET) == 0) {
		device_printf(sc->sc_dev, "no device detected\n");
		return ENODEV;	/* no device plugged in */
	}

	if (HDMI_1_3_P(sc)) {
		HDMI_WRITE(sc, SUNXI_HDMI_DDC_FIFO_CTRL_REG, 0);
		HDMI_WRITE(sc, SUNXI_HDMI_DDC_CTRL_REG,
		    SUNXI_HDMI_DDC_CTRL_EN | SUNXI_HDMI_DDC_CTRL_SW_RST); 

		delay(1000);

		ctrl = HDMI_READ(sc, SUNXI_HDMI_DDC_CTRL_REG);
		if (ctrl & SUNXI_HDMI_DDC_CTRL_SW_RST) {
			device_printf(sc->sc_dev, "reset failed (1.3)\n");
			return EBUSY;
		}

		/* N=5,M=1 */
		HDMI_WRITE(sc, SUNXI_HDMI_DDC_CLOCK_REG,
		    __SHIFTIN(5, SUNXI_HDMI_DDC_CLOCK_N) |
		    __SHIFTIN(1, SUNXI_HDMI_DDC_CLOCK_M));

		HDMI_WRITE(sc, SUNXI_HDMI_DDC_DBG_REG, 0x300);
	} else {
		HDMI_WRITE(sc, SUNXI_A31_HDMI_DDC_CTRL_REG,
		    SUNXI_A31_HDMI_DDC_CTRL_SW_RST);

		/* N=1,M=12 */
		HDMI_WRITE(sc, SUNXI_A31_HDMI_DDC_CLOCK_REG,
		    __SHIFTIN(1, SUNXI_HDMI_DDC_CLOCK_N) |
		    __SHIFTIN(12, SUNXI_HDMI_DDC_CLOCK_M));

		HDMI_WRITE(sc, SUNXI_A31_HDMI_DDC_CTRL_REG,
		    SUNXI_A31_HDMI_DDC_CTRL_SDA_PAD_EN |
		    SUNXI_A31_HDMI_DDC_CTRL_SCL_PAD_EN |
		    SUNXI_A31_HDMI_DDC_CTRL_EN);
	}

	return 0;
}

static int
sunxi_hdmi_ep_activate(device_t dev, struct fdt_endpoint *ep, bool activate)
{
	struct sunxi_hdmi_softc *sc = device_private(dev);
	struct fdt_endpoint *in_ep, *out_ep;
	int error;

	/* our input is activated by tcon, we activate our output */
	if (fdt_endpoint_port_index(ep) != SUNXI_PORT_INPUT) {
		panic("sunxi_hdmi_ep_activate: port %d",
		    fdt_endpoint_port_index(ep));
	}

	if (!activate)
		return EOPNOTSUPP;

	/* check that out other input is not active */
	switch (fdt_endpoint_index(ep)) {
	case 0:
		in_ep = fdt_endpoint_get_from_index(&sc->sc_ports,
		    SUNXI_PORT_INPUT, 1);
		break;
	case 1:
		in_ep = fdt_endpoint_get_from_index(&sc->sc_ports,
		    SUNXI_PORT_INPUT, 0);
		break;
	default:
		in_ep = NULL;
		panic("sunxi_hdmi_ep_activate: input index %d",
		    fdt_endpoint_index(ep));
	}
	if (in_ep != NULL) {
		if (fdt_endpoint_is_active(in_ep))
			return EBUSY;
	}
	/* only one output */
	out_ep = fdt_endpoint_get_from_index(&sc->sc_ports,
		   SUNXI_PORT_OUTPUT, 0);
	if (out_ep == NULL) {
		aprint_error_dev(dev, "no output endpoint\n");
		return ENODEV;
	}
	error = fdt_endpoint_activate(out_ep, activate);
	if (error == 0) {
		sc->sc_in_ep = ep;
		sc->sc_in_rep = fdt_endpoint_remote(ep);
		sc->sc_out_ep = out_ep;
		sunxi_hdmi_do_enable(sc);
		return 0;
	}
	return error;
}

static int
sunxi_hdmi_ep_enable(device_t dev, struct fdt_endpoint *ep, bool enable)
{
	struct sunxi_hdmi_softc *sc = device_private(dev);
	int error;

	if (fdt_endpoint_port_index(ep) == SUNXI_PORT_INPUT) {
		KASSERT(ep == sc->sc_in_ep);
		if (sc->sc_thread == NULL) {
			if (enable) {
				delay(50000);
				mutex_enter(&sc->sc_pwr_lock);
				sunxi_hdmi_hpd(sc);
				mutex_exit(&sc->sc_pwr_lock);
				kthread_create(PRI_NONE, KTHREAD_MPSAFE, NULL,
				    sunxi_hdmi_thread, sc, &sc->sc_thread, "%s",
				    device_xname(dev));
			}
			return 0;
		} else {
			mutex_enter(&sc->sc_pwr_lock);
			error = sunxi_hdmi_poweron(sc, enable);
			mutex_exit(&sc->sc_pwr_lock);
			return error;
		}
	}
	panic("sunxi_hdmi_ep_enable");
}

static void
sunxi_hdmi_do_enable(struct sunxi_hdmi_softc *sc)
{
	/* complete attach */
	struct clk *clk;
	int error;
	uint32_t dbg0_reg;

	if (clk_enable(sc->sc_clk_ahb) != 0) {
		aprint_error_dev(sc->sc_dev, "couldn't enable ahb clock\n");
		return;
	}
	/* assume tcon0 uses pll3, tcon1 uses pll7 */
	switch(fdt_endpoint_index(sc->sc_in_ep)) {
	case 0:
		clk = sc->sc_clk_pll0;
		dbg0_reg = (0<<21);
		break;
	case 1:
		clk = sc->sc_clk_pll1;
		dbg0_reg = (1<<21);
		break;
	default:
		panic("sunxi_hdmi pll");
	}
	error = clk_set_rate(clk, 270000000);
	if (error) {
		clk = clk_get_parent(clk);
		/* probably because this is pllx2 */
		error = clk_set_rate(clk, 270000000);
	}
	if (error) {
		aprint_error_dev(sc->sc_dev, ": couldn't init pll clock\n");
		return;
	}
	error = clk_set_parent(sc->sc_clk_mod, clk);
	if (error) {
		aprint_error_dev(sc->sc_dev, ": couldn't set mod clock parent\n");
		return;
	}
	error = clk_enable(sc->sc_clk_mod);
	if (error) {
		aprint_error_dev(sc->sc_dev, ": couldn't enable mod clock\n");
		return;
	}
	delay(1000);

	HDMI_WRITE(sc, SUNXI_HDMI_CTRL_REG, SUNXI_HDMI_CTRL_MODULE_EN);
	delay(1000);
	if (sc->sc_type == HDMI_A10) {
		HDMI_WRITE(sc, SUNXI_HDMI_PAD_CTRL0_REG, 0xfe800000);
		HDMI_WRITE(sc, SUNXI_HDMI_PAD_CTRL1_REG, 0x00d8c830);
	} else if (sc->sc_type == HDMI_A31) {
		HDMI_WRITE(sc, SUNXI_HDMI_PAD_CTRL0_REG, 0x7e80000f);
		HDMI_WRITE(sc, SUNXI_HDMI_PAD_CTRL1_REG, 0x01ded030);
	}
	HDMI_WRITE(sc, SUNXI_HDMI_PLL_DBG0_REG, dbg0_reg);
	delay(1000);
}

static int
sunxi_hdmi_read_edid_block(struct sunxi_hdmi_softc *sc, uint8_t *data,
    uint8_t block)
{
	i2c_tag_t tag = &sc->sc_ic;
	uint8_t wbuf[2];
	int error;

	if ((error = iic_acquire_bus(tag, I2C_F_POLL)) != 0)
		return error;

	wbuf[0] = block;	/* start address */

	if ((error = iic_exec(tag, I2C_OP_READ_WITH_STOP, DDC_ADDR, wbuf, 1,
	    data, 128, I2C_F_POLL)) != 0) {
		iic_release_bus(tag, I2C_F_POLL);
		return error;
	}
	iic_release_bus(tag, I2C_F_POLL);

	return 0;
}

static void
sunxi_hdmi_read_edid(struct sunxi_hdmi_softc *sc)
{
	const struct videomode *mode;
	char edid[128];
	struct edid_info ei;
	int retry = 4;
	u_int display_mode;

	memset(edid, 0, sizeof(edid));
	memset(&ei, 0, sizeof(ei));

	while (--retry > 0) {
		if (!sunxi_hdmi_read_edid_block(sc, edid, 0))
			break;
	}
	if (retry == 0) {
		device_printf(sc->sc_dev, "failed to read EDID\n");
	} else {
		if (edid_parse(edid, &ei) != 0) {
			device_printf(sc->sc_dev, "failed to parse EDID\n");
		}
#ifdef SUNXI_HDMI_DEBUG
		else {
			edid_print(&ei);
		}
#endif
	}

	if (sc->sc_display_mode == DISPLAY_MODE_AUTO)
		display_mode = sunxi_hdmi_get_display_mode(sc, &ei);
	else
		display_mode = sc->sc_display_mode;

	const char *forced = sc->sc_display_mode == DISPLAY_MODE_AUTO ?
	    "auto-detected" : "forced";
	device_printf(sc->sc_dev, "%s mode (%s)\n",
	    display_mode == DISPLAY_MODE_HDMI ? "HDMI" : "DVI", forced);

	strlcpy(sc->sc_display_vendor, ei.edid_vendorname,
	    sizeof(sc->sc_display_vendor));
	strlcpy(sc->sc_display_product, ei.edid_productname,
	    sizeof(sc->sc_display_product));
	sc->sc_current_display_mode = display_mode;

	mode = ei.edid_preferred_mode;
	if (mode == NULL)
		mode = pick_mode_by_ref(640, 480, 60);

	if (mode != NULL) {
		sunxi_hdmi_video_enable(sc, false);
		fdt_endpoint_enable(sc->sc_in_ep, false);
		delay(20000);

		sunxi_tcon1_set_videomode(
		    fdt_endpoint_device(sc->sc_in_rep), mode);
		sunxi_hdmi_set_videomode(sc, mode, display_mode);
		sunxi_hdmi_set_audiomode(sc, mode, display_mode);
		fdt_endpoint_enable(sc->sc_in_ep, true);
		delay(20000);
		sunxi_hdmi_video_enable(sc, true);
	}
}

static u_int
sunxi_hdmi_get_display_mode(struct sunxi_hdmi_softc *sc,
    const struct edid_info *ei)
{
	char edid[128];
	bool found_hdmi = false;
	unsigned int n, p;

	/*
	 * Scan through extension blocks, looking for a CEA-861-D v3
	 * block. If an HDMI Vendor-Specific Data Block (HDMI VSDB) is
	 * found in that, assume HDMI mode.
	 */
	for (n = 1; n <= MIN(ei->edid_ext_block_count, 4); n++) {
		if (sunxi_hdmi_read_edid_block(sc, edid, n)) {
#ifdef SUNXI_HDMI_DEBUG
			device_printf(sc->sc_dev,
			    "Failed to read EDID block %d\n", n);
#endif
			break;
		}

#ifdef SUNXI_HDMI_DEBUG
		device_printf(sc->sc_dev, "EDID block #%d:\n", n);
#endif

		const uint8_t tag = edid[0];
		const uint8_t rev = edid[1];
		const uint8_t off = edid[2];

#ifdef SUNXI_HDMI_DEBUG
		device_printf(sc->sc_dev, "  Tag %d, Revision %d, Offset %d\n",
		    tag, rev, off);
		device_printf(sc->sc_dev, "  Flags: 0x%02x\n", edid[3]);
#endif

		/* We are looking for a CEA-861-D tag (02h) with revision 3 */
		if (tag != 0x02 || rev != 3)
			continue;
		/*
		 * CEA data block collection starts at byte 4, so the
		 * DTD blocks must start after it.
		 */
		if (off <= 4)
			continue;

		/* Parse the CEA data blocks */
		for (p = 4; p < off;) {
			const uint8_t btag = (edid[p] >> 5) & 0x7;
			const uint8_t blen = edid[p] & 0x1f;

#ifdef SUNXI_HDMI_DEBUG
			device_printf(sc->sc_dev, "  CEA data block @ %d\n", p);
			device_printf(sc->sc_dev, "    Tag %d, Length %d\n",
			    btag, blen);
#endif

			/* Make sure the length is sane */
			if (p + blen + 1 > off)
				break;
			/* Looking for a VSDB tag */
			if (btag != 3)
				goto next_block;
			/* HDMI VSDB is at least 5 bytes long */
			if (blen < 5)
				goto next_block;

#ifdef SUNXI_HDMI_DEBUG
			device_printf(sc->sc_dev, "    ID: %02x%02x%02x\n",
			    edid[p + 1], edid[p + 2], edid[p + 3]);
#endif

			/* HDMI 24-bit IEEE registration ID is 0x000C03 */
			if (memcmp(&edid[p + 1], "\x03\x0c\x00", 3) == 0)
				found_hdmi = true;

next_block:
			p += (1 + blen);
		}
	}

	return found_hdmi ? DISPLAY_MODE_HDMI : DISPLAY_MODE_DVI;
}

static void
sunxi_hdmi_video_enable(struct sunxi_hdmi_softc *sc, bool enable)
{
	uint32_t val;

	fdt_endpoint_enable(sc->sc_out_ep, enable);

	val = HDMI_READ(sc, SUNXI_HDMI_VID_CTRL_REG);
	val &= ~SUNXI_HDMI_VID_CTRL_SRC_SEL;
#ifdef SUNXI_HDMI_CBGEN
	val |= __SHIFTIN(SUNXI_HDMI_VID_CTRL_SRC_SEL_CBGEN,
			 SUNXI_HDMI_VID_CTRL_SRC_SEL);
#else
	val |= __SHIFTIN(SUNXI_HDMI_VID_CTRL_SRC_SEL_RGB,
			 SUNXI_HDMI_VID_CTRL_SRC_SEL);
#endif
	if (enable) {
		val |= SUNXI_HDMI_VID_CTRL_VIDEO_EN;
	} else {
		val &= ~SUNXI_HDMI_VID_CTRL_VIDEO_EN;
	}
	HDMI_WRITE(sc, SUNXI_HDMI_VID_CTRL_REG, val);

#if defined(SUNXI_HDMI_DEBUG)
	sunxi_hdmi_dump_regs();
#endif
}

static void
sunxi_hdmi_set_videomode(struct sunxi_hdmi_softc *sc,
    const struct videomode *mode, u_int display_mode)
{
	uint32_t val;
	const u_int dblscan_p = !!(mode->flags & VID_DBLSCAN);
	const u_int interlace_p = !!(mode->flags & VID_INTERLACE);
	const u_int phsync_p = !!(mode->flags & VID_PHSYNC);
	const u_int pvsync_p = !!(mode->flags & VID_PVSYNC);
	const u_int hfp = mode->hsync_start - mode->hdisplay;
	const u_int hspw = mode->hsync_end - mode->hsync_start;
	const u_int hbp = mode->htotal - mode->hsync_start;
	const u_int vfp = mode->vsync_start - mode->vdisplay;
	const u_int vspw = mode->vsync_end - mode->vsync_start;
	const u_int vbp = mode->vtotal - mode->vsync_start;
	struct clk *clk_pll;
	int parent_rate;
	int best_div, best_dbl, best_diff;
	int target_rate = mode->dot_clock * 1000;

#ifdef SUNXI_HDMI_DEBUG
	device_printf(sc->sc_dev,
	    "dblscan %d, interlace %d, phsync %d, pvsync %d\n",
	    dblscan_p, interlace_p, phsync_p, pvsync_p);
	device_printf(sc->sc_dev, "h: %u %u %u %u\n",
	    mode->hdisplay, hbp, hfp, hspw);
	device_printf(sc->sc_dev, "v: %u %u %u %u\n",
	    mode->vdisplay, vbp, vfp, vspw);
#endif

	HDMI_WRITE(sc, SUNXI_HDMI_INT_STATUS_REG, 0xffffffff);

	/* assume tcon0 uses pll3, tcon1 uses pll7 */
	switch(fdt_endpoint_index(sc->sc_in_ep)) {
	case 0:
		clk_pll = sc->sc_clk_pll0;
		break;
	case 1:
		clk_pll = sc->sc_clk_pll1;
		break;
	default:
		panic("sunxi_hdmi pll");
	}
	parent_rate = clk_get_rate(clk_pll);
	KASSERT(parent_rate > 0);
	best_div = best_dbl = 0;
	best_diff = INT_MAX;
	for (int d = 2; d > 0 && best_diff != 0; d--) {
		for (int m = 1; m <= 16 && best_diff != 0; m++) {
			int cur_rate = parent_rate / m / d;
			int diff = abs(target_rate - cur_rate);
			if (diff >= 0 && diff < best_diff) {
				best_diff = diff;
				best_div = m;
				best_dbl = d;
			}
		}
	}

#ifdef SUNXI_HDMI_DEBUG
	device_printf(sc->sc_dev, "parent rate: %d\n", parent_rate);
	device_printf(sc->sc_dev, "dot_clock: %d\n", mode->dot_clock);
	device_printf(sc->sc_dev, "clkdiv: %d\n", best_div);
	device_printf(sc->sc_dev, "clkdbl: %c\n", (best_dbl == 1) ? 'Y' : 'N');
#endif

	if (best_div == 0) {
		device_printf(sc->sc_dev, "ERROR: TCON clk not configured\n");
		return;
	}

	uint32_t pll_ctrl, pad_ctrl0, pad_ctrl1;
	if (HDMI_1_4_P(sc)) {
		pad_ctrl0 = 0x7e8000ff;
		pad_ctrl1 = 0x01ded030;
		pll_ctrl = 0xba48a308;
		pll_ctrl |= __SHIFTIN(best_div - 1, SUNXI_HDMI_PLL_CTRL_PREDIV);
	} else {
		pad_ctrl0 = 0xfe800000;
		pad_ctrl1 = 0x00d8c830;
		pll_ctrl = 0xfa4ef708;
		pll_ctrl |= __SHIFTIN(best_div, SUNXI_HDMI_PLL_CTRL_PREDIV);
	}
	if (best_dbl == 2)
		pad_ctrl1 |= 0x40;

	HDMI_WRITE(sc, SUNXI_HDMI_PAD_CTRL0_REG, pad_ctrl0);
	HDMI_WRITE(sc, SUNXI_HDMI_PAD_CTRL1_REG, pad_ctrl1);
	HDMI_WRITE(sc, SUNXI_HDMI_PLL_CTRL_REG, pll_ctrl);
	/* assume tcon0 uses pll3, tcon1 uses pll7 */
	switch(fdt_endpoint_index(sc->sc_in_ep)) {
	case 0:
		HDMI_WRITE(sc, SUNXI_HDMI_PLL_DBG0_REG, (0<<21));
		break;
	case 1:
		HDMI_WRITE(sc, SUNXI_HDMI_PLL_DBG0_REG, (1<<21));
		break;
	default:
		panic("sunxi_hdmi pll");
	}

	val = HDMI_READ(sc, SUNXI_HDMI_VID_CTRL_REG);
	val &= ~SUNXI_HDMI_VID_CTRL_HDMI_MODE;
	if (display_mode == DISPLAY_MODE_DVI) {
		val |= __SHIFTIN(SUNXI_HDMI_VID_CTRL_HDMI_MODE_DVI,
				 SUNXI_HDMI_VID_CTRL_HDMI_MODE);
	} else {
		val |= __SHIFTIN(SUNXI_HDMI_VID_CTRL_HDMI_MODE_HDMI,
				 SUNXI_HDMI_VID_CTRL_HDMI_MODE);
	}
	val &= ~SUNXI_HDMI_VID_CTRL_REPEATER_SEL;
	if (dblscan_p) {
		val |= __SHIFTIN(SUNXI_HDMI_VID_CTRL_REPEATER_SEL_2X,
				 SUNXI_HDMI_VID_CTRL_REPEATER_SEL);
	}
	val &= ~SUNXI_HDMI_VID_CTRL_OUTPUT_FMT;
	if (interlace_p) {
		val |= __SHIFTIN(SUNXI_HDMI_VID_CTRL_OUTPUT_FMT_INTERLACE,
				 SUNXI_HDMI_VID_CTRL_OUTPUT_FMT);
	}
	HDMI_WRITE(sc, SUNXI_HDMI_VID_CTRL_REG, val);

	val = __SHIFTIN((mode->hdisplay << dblscan_p) - 1,
			SUNXI_HDMI_VID_TIMING_0_ACT_H);
	val |= __SHIFTIN(mode->vdisplay - 1,
			 SUNXI_HDMI_VID_TIMING_0_ACT_V);
	HDMI_WRITE(sc, SUNXI_HDMI_VID_TIMING_0_REG, val);

	val = __SHIFTIN((hbp << dblscan_p) - 1,
			SUNXI_HDMI_VID_TIMING_1_HBP);
	val |= __SHIFTIN(vbp - 1,
			 SUNXI_HDMI_VID_TIMING_1_VBP);
	HDMI_WRITE(sc, SUNXI_HDMI_VID_TIMING_1_REG, val);

	val = __SHIFTIN((hfp << dblscan_p) - 1,
			SUNXI_HDMI_VID_TIMING_2_HFP);
	val |= __SHIFTIN(vfp - 1,
			 SUNXI_HDMI_VID_TIMING_2_VFP);
	HDMI_WRITE(sc, SUNXI_HDMI_VID_TIMING_2_REG, val);

	val = __SHIFTIN((hspw << dblscan_p) - 1,
			SUNXI_HDMI_VID_TIMING_3_HSPW);
	val |= __SHIFTIN(vspw - 1,
			 SUNXI_HDMI_VID_TIMING_3_VSPW);
	HDMI_WRITE(sc, SUNXI_HDMI_VID_TIMING_3_REG, val);

	val = 0;
	if (phsync_p) {
		val |= SUNXI_HDMI_VID_TIMING_4_HSYNC_ACTIVE_SEL;
	}
	if (pvsync_p) {
		val |= SUNXI_HDMI_VID_TIMING_4_VSYNC_ACTIVE_SEL;
	}
	val |= __SHIFTIN(SUNXI_HDMI_VID_TIMING_4_TX_CLOCK_NORMAL,
			 SUNXI_HDMI_VID_TIMING_4_TX_CLOCK);
	HDMI_WRITE(sc, SUNXI_HDMI_VID_TIMING_4_REG, val);

	/* Packet control */
	HDMI_WRITE(sc, SUNXI_HDMI_GP_PKT0_REG, 0);
	HDMI_WRITE(sc, SUNXI_HDMI_GP_PKT1_REG, 0);
	HDMI_WRITE(sc, SUNXI_HDMI_PKT_CTRL0_REG, 0x00005321);
	HDMI_WRITE(sc, SUNXI_HDMI_PKT_CTRL1_REG, 0x0000000f);
}

static void
sunxi_hdmi_set_audiomode(struct sunxi_hdmi_softc *sc,
    const struct videomode *mode, u_int display_mode)
{
	uint32_t cts, n, val;

	/*
	 * Before changing audio parameters, disable and reset the
	 * audio module. Wait for the soft reset bit to clear before
	 * configuring the audio parameters.
	 */
	val = HDMI_READ(sc, SUNXI_HDMI_AUD_CTRL_REG);
	val &= ~SUNXI_HDMI_AUD_CTRL_EN;
	val |= SUNXI_HDMI_AUD_CTRL_RST;
	HDMI_WRITE(sc, SUNXI_HDMI_AUD_CTRL_REG, val);
	do {
		val = HDMI_READ(sc, SUNXI_HDMI_AUD_CTRL_REG);
	} while (val & SUNXI_HDMI_AUD_CTRL_RST);

	/* No audio support in DVI mode */
	if (display_mode != DISPLAY_MODE_HDMI) {
		return;
	}

	/* DMA & FIFO control */
	val = HDMI_READ(sc, SUNXI_HDMI_ADMA_CTRL_REG);
	if (sc->sc_type == HDMI_A31) {
		val |= SUNXI_HDMI_ADMA_CTRL_SRC_DMA_MODE;	/* NDMA */
	} else {
		val &= ~SUNXI_HDMI_ADMA_CTRL_SRC_DMA_MODE;	/* DDMA */
	}
	val &= ~SUNXI_HDMI_ADMA_CTRL_SRC_DMA_SAMPLE_RATE;
	val &= ~SUNXI_HDMI_ADMA_CTRL_SRC_SAMPLE_LAYOUT;
	val &= ~SUNXI_HDMI_ADMA_CTRL_SRC_WORD_LEN;
	val &= ~SUNXI_HDMI_ADMA_CTRL_DATA_SEL;
	HDMI_WRITE(sc, SUNXI_HDMI_ADMA_CTRL_REG, val);

	/* Audio format control */
	val = HDMI_READ(sc, SUNXI_HDMI_AUD_FMT_REG);
	val &= ~SUNXI_HDMI_AUD_FMT_SRC_SEL;
	val &= ~SUNXI_HDMI_AUD_FMT_SEL;
	val &= ~SUNXI_HDMI_AUD_FMT_DSD_FMT;
	val &= ~SUNXI_HDMI_AUD_FMT_LAYOUT;
	val &= ~SUNXI_HDMI_AUD_FMT_SRC_CH_CFG;
	val |= __SHIFTIN(1, SUNXI_HDMI_AUD_FMT_SRC_CH_CFG);
	HDMI_WRITE(sc, SUNXI_HDMI_AUD_FMT_REG, val);

	/* PCM control (channel map) */
	HDMI_WRITE(sc, SUNXI_HDMI_AUD_PCM_CTRL_REG, 0x76543210);

	/* Clock setup */
	n = 6144;	/* 48 kHz */
	cts = ((mode->dot_clock * 10) * (n / 128)) / 480;
	HDMI_WRITE(sc, SUNXI_HDMI_AUD_CTS_REG, cts);
	HDMI_WRITE(sc, SUNXI_HDMI_AUD_N_REG, n);

	/* Audio PCM channel status 0 */
	val = __SHIFTIN(SUNXI_HDMI_AUD_CH_STATUS0_FS_FREQ_48,
			SUNXI_HDMI_AUD_CH_STATUS0_FS_FREQ);
	HDMI_WRITE(sc, SUNXI_HDMI_AUD_CH_STATUS0_REG, val);

	/* Audio PCM channel status 1 */
	val = HDMI_READ(sc, SUNXI_HDMI_AUD_CH_STATUS1_REG);
	val &= ~SUNXI_HDMI_AUD_CH_STATUS1_CGMS_A;
	val &= ~SUNXI_HDMI_AUD_CH_STATUS1_ORIGINAL_FS;
	val &= ~SUNXI_HDMI_AUD_CH_STATUS1_WORD_LEN;
	val |= __SHIFTIN(5, SUNXI_HDMI_AUD_CH_STATUS1_WORD_LEN);
	val |= SUNXI_HDMI_AUD_CH_STATUS1_WORD_LEN_MAX;
	HDMI_WRITE(sc, SUNXI_HDMI_AUD_CH_STATUS1_REG, val);

	/* Re-enable */
	val = HDMI_READ(sc, SUNXI_HDMI_AUD_CTRL_REG);
	val |= SUNXI_HDMI_AUD_CTRL_EN;
	HDMI_WRITE(sc, SUNXI_HDMI_AUD_CTRL_REG, val);

#if defined(SUNXI_HDMI_DEBUG)
	sunxi_hdmi_dump_regs();
#endif
}

static void
sunxi_hdmi_hpd(struct sunxi_hdmi_softc *sc)
{
	uint32_t hpd = HDMI_READ(sc, SUNXI_HDMI_HPD_REG);
	bool con = !!(hpd & SUNXI_HDMI_HPD_HOTPLUG_DET);

	KASSERT(mutex_owned(&sc->sc_pwr_lock));
	if (sc->sc_display_connected == con)
		return;

	if (con) {
		device_printf(sc->sc_dev, "display connected\n");
		sc->sc_pwr_refcount  = 1;
		sunxi_hdmi_read_edid(sc);
	} else {
		device_printf(sc->sc_dev, "display disconnected\n");
		sc->sc_pwr_refcount = 0;
		sunxi_hdmi_video_enable(sc, false);
		fdt_endpoint_enable(sc->sc_in_ep, false);
		sunxi_tcon1_set_videomode(
		    fdt_endpoint_device(sc->sc_in_rep), NULL);
	}

	sc->sc_display_connected = con;
}

static void
sunxi_hdmi_thread(void *priv)
{
	struct sunxi_hdmi_softc *sc = priv;

	for (;;) {
		mutex_enter(&sc->sc_pwr_lock);
		sunxi_hdmi_hpd(sc);
		mutex_exit(&sc->sc_pwr_lock);
		kpause("hdmihotplug", false, mstohz(1000), NULL);
	}
}

static int
sunxi_hdmi_poweron(struct sunxi_hdmi_softc *sc, bool enable)
{
	int error = 0;
	KASSERT(mutex_owned(&sc->sc_pwr_lock));
	if (!sc->sc_display_connected)
		return EOPNOTSUPP;
	if (enable) {
		KASSERT(sc->sc_pwr_refcount >= 0);
		if (sc->sc_pwr_refcount == 0) {
			error = fdt_endpoint_enable(sc->sc_in_ep, true);
			if (error)
				return error;
			sunxi_hdmi_video_enable(sc, true);
		}
		sc->sc_pwr_refcount++;
	} else {
		sc->sc_pwr_refcount--;
		KASSERT(sc->sc_pwr_refcount >= 0);
		if (sc->sc_pwr_refcount == 0) {
			sunxi_hdmi_video_enable(sc, false);
			error = fdt_endpoint_enable(sc->sc_in_ep, false);
		}
	}
	return error;
}
#if 0
static int
sunxi_hdmi_intr(void *priv)
{
	struct sunxi_hdmi_softc *sc = priv;
	uint32_t intsts;

	intsts = HDMI_READ(sc, SUNXI_HDMI_INT_STATUS_REG);
	if (!(intsts & 0x73))
		return 0;

	HDMI_WRITE(sc, SUNXI_HDMI_INT_STATUS_REG, intsts);

	device_printf(sc->sc_dev, "INT_STATUS %08X\n", intsts);

	return 1;
}
#endif

#if 0 /* XXX audio */
void
sunxi_hdmi_get_info(struct sunxi_hdmi_info *info)
{
	struct sunxi_hdmi_softc *sc;
	device_t dev;

	memset(info, 0, sizeof(*info));

	dev = device_find_by_driver_unit("sunxihdmi", 0);
	if (dev == NULL) {
		info->display_connected = false;
		return;
	}
	sc = device_private(dev);

	info->display_connected = sc->sc_display_connected;
	if (info->display_connected) {
		strlcpy(info->display_vendor, sc->sc_display_vendor,
		    sizeof(info->display_vendor));
		strlcpy(info->display_product, sc->sc_display_product,
		    sizeof(info->display_product));
		info->display_hdmimode =
		    sc->sc_current_display_mode == DISPLAY_MODE_HDMI;
	}
}
#endif

#if defined(SUNXI_HDMI_DEBUG)
void
sunxi_hdmi_dump_regs(void)
{
	static const struct {
		const char *name;
		uint16_t reg;
	} regs[] = {
		{ "CTRL", SUNXI_HDMI_CTRL_REG },
		{ "INT_STATUS", SUNXI_HDMI_INT_STATUS_REG },
		{ "VID_CTRL", SUNXI_HDMI_VID_CTRL_REG },
		{ "VID_TIMING_0", SUNXI_HDMI_VID_TIMING_0_REG },
		{ "VID_TIMING_1", SUNXI_HDMI_VID_TIMING_1_REG },
		{ "VID_TIMING_2", SUNXI_HDMI_VID_TIMING_2_REG },
		{ "VID_TIMING_3", SUNXI_HDMI_VID_TIMING_3_REG },
		{ "VID_TIMING_4", SUNXI_HDMI_VID_TIMING_4_REG },
		{ "PAD_CTRL0", SUNXI_HDMI_PAD_CTRL0_REG },
		{ "PAD_CTRL1", SUNXI_HDMI_PAD_CTRL1_REG },
		{ "PLL_CTRL", SUNXI_HDMI_PLL_CTRL_REG },
		{ "PLL_DBG0", SUNXI_HDMI_PLL_DBG0_REG },
		{ "PLL_DBG1", SUNXI_HDMI_PLL_DBG1_REG },
	};
	struct sunxi_hdmi_softc *sc;
	device_t dev;

	dev = device_find_by_driver_unit("sunxihdmi", 0);
	if (dev == NULL)
		return;
	sc = device_private(dev);

	for (int i = 0; i < __arraycount(regs); i++) {
		printf("%s: 0x%08x\n", regs[i].name,
		    HDMI_READ(sc, regs[i].reg));
	}
}
#endif
