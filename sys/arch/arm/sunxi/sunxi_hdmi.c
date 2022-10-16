/* $NetBSD: sunxi_hdmi.c,v 1.14.16.2 2022/10/16 14:56:04 bouyer Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_hdmi.c,v 1.14.16.2 2022/10/16 14:56:04 bouyer Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kmem.h>
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

#include <arm/sunxi/sunxi_hdmireg.h>
#include <arm/sunxi/sunxi_display.h>

#include <arm/sunxi/sunxi_drm.h>

#include <drm/drm_bridge.h>
#include <drm/drm_connector.h>
#include <drm/drm_drv.h>
#include <drm/drm_modes.h>

enum sunxi_hdmi_type {
	HDMI_A10 = 1,
	HDMI_A31,
};

struct sunxi_hdmi_softc;

struct sunxi_hdmi_encoder {
	struct drm_encoder      base;
	struct sunxi_hdmi_softc *sc;
	struct drm_display_mode curmode;
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

	struct sunxi_hdmi_encoder sc_encoder;
	struct drm_connector sc_connector;
	struct i2c_adapter sc_i2c_adapt;

	enum sc_display_mode {
		DISPLAY_MODE_NONE = 0,
		DISPLAY_MODE_DVI,
		DISPLAY_MODE_HDMI
	} sc_display_mode;

	kmutex_t sc_exec_lock;

	kmutex_t sc_pwr_lock;
	int	sc_pwr_refcount; /* reference who needs HDMI */

	uint32_t sc_ver;

	struct fdt_device_ports sc_ports;
	struct fdt_endpoint *sc_in_ep;
	struct fdt_endpoint *sc_in_rep;
	struct fdt_endpoint *sc_out_ep;
	struct drm_display_mode sc_curmode;
};

#define to_sunxi_hdmi_encoder(x)	container_of(x, struct sunxi_hdmi_encoder, base)

#define connector_to_hdmi(x)		container_of(x, struct sunxi_hdmi_softc, sc_connector)

static void
sunxi_hdmi_destroy(struct drm_encoder *encoder)
{
	drm_encoder_cleanup(encoder);
}

static const struct drm_encoder_funcs sunxi_hdmi_enc_funcs = {
	.destroy = sunxi_hdmi_destroy,
};

static int sunxi_hdmi_atomic_check(struct drm_encoder *,
    struct drm_crtc_state *, struct drm_connector_state *);
static void sunxi_hdmi_disable(struct drm_encoder *);
static void sunxi_hdmi_enable(struct drm_encoder *);
static void sunxi_hdmi_mode_set(struct drm_encoder *,
    struct drm_display_mode *, struct drm_display_mode *);
static void sunxi_hdmi_mode_valid(struct drm_encoder *,
    struct drm_display_mode *);

static const struct drm_encoder_helper_funcs sunxi_hdmi_enc_helper_funcs = {
	.atomic_check	= sunxi_hdmi_atomic_check,
	.disable	= sunxi_hdmi_disable,
	.enable	 	= sunxi_hdmi_enable,
	.mode_set	= sunxi_hdmi_mode_set,
	.mode_valid	= sunxi_hdmi_mode_valid,
};

static enum drm_connector_status sunxi_hdmi_connector_detect(
    struct drm_connector *, bool);

static const struct drm_connector_funcs sunxi_hdmi_connector_funcs = {
	.detect			= sunxi_hdmi_connector_detect,
	.fill_modes		= drm_helper_probe_single_connector_modes,
	.destroy		= drm_connector_cleanup,
	.reset			= drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_connector_destroy_state,
};

static int sunxi_hdmi_get_modes(struct drm_connector *);

static const struct drm_connector_helper_funcs sunxi_hdmi_connector_helper_funcs
 = {
	 .get_modes      = sunxi_hdmi_get_modes,
};

#define HDMI_READ(sc, reg)			\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define HDMI_WRITE(sc, reg, val)		\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val));

#define HDMI_1_3_P(sc)	((sc)->sc_ver == 0x00010003)
#define HDMI_1_4_P(sc)	((sc)->sc_ver == 0x00010004)

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun4i-a10-hdmi", .value = HDMI_A10},
	{ .compat = "allwinner,sun7i-a20-hdmi", .value = HDMI_A10},
	DEVICE_COMPAT_EOL
};

static int	sunxi_hdmi_match(device_t, cfdata_t, void *);
static void	sunxi_hdmi_attach(device_t, device_t, void *);
static void	sunxi_hdmi_i2c_init(struct sunxi_hdmi_softc *);
static int	sunxi_hdmi_i2c_drm_xfer(struct i2c_adapter *,
				   struct i2c_msg *, int);
static int	sunxi_hdmi_i2c_xfer(struct sunxi_hdmi_softc *,
					struct i2c_msg *);
static int	sunxi_hdmi_i2c_reset(struct sunxi_hdmi_softc *);

static int	sunxi_hdmi_ep_activate(device_t, struct fdt_endpoint *, bool);
static int	sunxi_hdmi_ep_enable(device_t, struct fdt_endpoint *, bool);
static void	sunxi_hdmi_do_enable(struct sunxi_hdmi_softc *);
#if 0
static void	sunxi_hdmi_read_edid(struct sunxi_hdmi_softc *);
static int	sunxi_hdmi_read_edid_block(struct sunxi_hdmi_softc *, uint8_t *,
					  uint8_t);
static u_int	sunxi_hdmi_get_display_mode(struct sunxi_hdmi_softc *,
					   const struct edid_info *);
#endif
static void	sunxi_hdmi_video_enable(struct sunxi_hdmi_softc *, bool);
static void	sunxi_hdmi_set_audiomode(struct sunxi_hdmi_softc *, int);
#if 0
static void	sunxi_hdmi_set_videomode(struct sunxi_hdmi_softc *,
					const struct videomode *, u_int);
static void	sunxi_hdmi_hpd(struct sunxi_hdmi_softc *);
static void	sunxi_hdmi_thread(void *);
#endif
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

	return of_compatible_match(faa->faa_phandle, compat_data);
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

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;
	sc->sc_encoder.sc = sc;

	sc->sc_type =
	    of_compatible_lookup(faa->faa_phandle, compat_data)->value;

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

	if (clk_enable(sc->sc_clk_ahb) != 0) {
		aprint_error(": couldn't enable ahb clock\n");
		return;
	}
	ver = HDMI_READ(sc, SUNXI_HDMI_VERSION_ID_REG);

	const int vmaj = __SHIFTOUT(ver, SUNXI_HDMI_VERSION_ID_H);
	const int vmin = __SHIFTOUT(ver, SUNXI_HDMI_VERSION_ID_L);

	aprint_naive("\n");
	aprint_normal(": HDMI %d.%d\n", vmaj, vmin);

	sc->sc_ver = ver;

	sc->sc_ports.dp_ep_activate = sunxi_hdmi_ep_activate;
	sc->sc_ports.dp_ep_enable = sunxi_hdmi_ep_enable;
	fdt_ports_register(&sc->sc_ports, self, phandle, EP_OTHER);

	mutex_init(&sc->sc_pwr_lock, MUTEX_DEFAULT, IPL_NONE);
	sunxi_hdmi_i2c_init(sc);
}

void
sunxi_hdmi_doreset(void)
{
	device_t dev;
	struct sunxi_hdmi_softc *sc;
	int error;

	for (int i = 0;;i++) {
		dev = device_find_by_driver_unit("sunxihdmi", i);
		if (dev == NULL)
			return;
		sc = device_private(dev);
	
		error = clk_disable(sc->sc_clk_mod);
		if (error) {
			aprint_error_dev(dev, ": couldn't disable mod clock\n");
			return;
		}

#if defined(SUNXI_HDMI_DEBUG)
		sunxi_hdmi_dump_regs();
#endif

		/*
		 * reset device, in case it has been setup by firmware in an
		 * incompatible way
		 */
		for (int j = 0; j <= 0x500; j += 4) {
			HDMI_WRITE(sc, j, 0);
		}

		if (clk_disable(sc->sc_clk_ahb) != 0) {
			aprint_error_dev(dev, ": couldn't disable ahb clock\n");
			return;
		}
	}
}

static int
sun4i_hdmi_atomic_check(struct drm_encoder *encoder,
    struct drm_crtc_state * crtc_state,
    struct drm_connector_state *conn_state)
{
	if (crtc_state->mode.flags & DRM_MODE_FLAG_DBLCLK)
		return -EINVAL;
	return 0;
}

static void
sunxi_hdmi_disable(struct drm_encoder *enc)
{
	struct sunxi_hdmi_encoder *hdmi_encoder = to_sunxi_hdmi_encoder(enc);
	struct sunxi_hdmi_softc *sc = hdmi_encoder->sc;
	sunxi_hdmi_video_enable(sc, false);
}

static void
sunxi_hdmi_enable(struct drm_encoder *enc)
{
	struct sunxi_hdmi_encoder *hdmi_encoder = to_sunxi_hdmi_encoder(enc);
	struct sunxi_hdmi_softc *sc = hdmi_encoder->sc;
	struct drm_display_mode *mode = &enc->crtc->state->adjusted_mode;
	sunxi_hdmi_video_enable(sc, true);
}

static uint32_t sunxi_hdmi_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}


static const struct i2c_algorithm sunxi_hdmi_i2c_algorithm = {
        .master_xfer    = sunxi_hdmi_i2c_drm_xfer,
	.functionality  = sunxi_hdmi_i2c_func, 
};      

static void
sunxi_hdmi_i2c_init(struct sunxi_hdmi_softc *sc)
{
	mutex_init(&sc->sc_exec_lock, MUTEX_DEFAULT, IPL_NONE);

	sc->sc_i2c_adapt.owner = THIS_MODULE;
	sc->sc_i2c_adapt.class = I2C_CLASS_DDC;
	sc->sc_i2c_adapt.algo = &sunxi_hdmi_i2c_algorithm;
	strlcpy(sc->sc_i2c_adapt.name, "sunxi_hdmi_i2c",
	    sizeof(sc->sc_i2c_adapt.name));
}

static int
sunxi_hdmi_i2c_drm_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num){
	struct sunxi_hdmi_softc *sc = i2c_get_adapdata(adap);
	int i, err;

	mutex_enter(&sc->sc_exec_lock);

	for (i = 0; i < num; i++) {
		if (msg[i].len == 0 ||
		    msg[i].len > SUNXI_HDMI_DDC_BYTE_COUNTER) 
			return -EINVAL;
	}

	err = sunxi_hdmi_i2c_reset(sc);
	if (err)
		goto done;

	for (i = 0; i < num; i++) {
		err = sunxi_hdmi_i2c_xfer(sc, &msg[i]);
		if (err)
			break;
#ifdef SUNXI_HDMI_DEBUG
		printf("msg %d:", i);
		for (int j = 0; j < msg[i].len; j++)
			printf(" %02x", msg[i].buf[j]);
		printf("\n");
#endif
	}

done:
	mutex_exit(&sc->sc_exec_lock);
	return err;
}

#define SUNXI_HDMI_DDC_INT_STATUS_ERR_MASK ( \
		SUNXI_HDMI_DDC_INT_STATUS_ILLEGAL_FIFO_OP | \
		SUNXI_HDMI_DDC_INT_STATUS_RX_UNDERFLOW | \
		SUNXI_HDMI_DDC_INT_STATUS_TX_OVERFLOW | \
		SUNXI_HDMI_DDC_INT_STATUS_ARB_ERR | \
		SUNXI_HDMI_DDC_INT_STATUS_ACK_ERR | \
		SUNXI_HDMI_DDC_INT_STATUS_BUS_ERR)

static int
sunxi_hdmi_i2c_xfer_1_3(struct sunxi_hdmi_softc *sc, struct i2c_msg *msg)
{
	struct sunxi_hdmi_softc *sc = priv;
	uint32_t val;
	uint8_t *buf;
	int len;
	int retry;

	val = HDMI_READ(sc, SUNXI_HDMI_DDC_CTRL_REG);
	val &= ~SUNXI_HDMI_DDC_CTRL_FIFO_DIR;
	if (msg->flags & I2C_M_RD) 
		val |= SUNXI_HDMI_DDC_CTRL_FIFO_DIR_READ;
	else
		val |= SUNXI_HDMI_DDC_CTRL_FIFO_DIR_WRITE;
	HDMI_WRITE(sc, SUNXI_HDMI_DDC_CTRL_REG, val);

	val = __SHIFTIN(msg->addr, SUNXI_HDMI_DDC_SLAVE_ADDR_3);
	HDMI_WRITE(sc, SUNXI_HDMI_DDC_SLAVE_ADDR_REG, val);

	val = __SHIFTIN(0, SUNXI_HDMI_DDC_FIFO_CTRL_TX_TRIGGER_THRESH);
	val |= __SHIFTIN(SUNXI_HDMI_DDC_FIFO_CTRL_TRIGGER_MAX,
	    SUNXI_HDMI_DDC_FIFO_CTRL_RX_TRIGGER_THRESH);
	val |= SUNXI_HDMI_DDC_FIFO_CTRL_ADDR_CLEAR;
	HDMI_WRITE(sc, SUNXI_HDMI_DDC_FIFO_CTRL_REG, val);

	retry = 20;
	while (HDMI_READ(sc, SUNXI_HDMI_DDC_FIFO_CTRL_REG) & SUNXI_HDMI_DDC_FIFO_CTRL_ADDR_CLEAR) {
		if (--retry == 0)
			return -EIO;
		delay(100);
	}

	HDMI_WRITE(sc, SUNXI_HDMI_DDC_BYTE_COUNTER_REG, msg->len);

	HDMI_WRITE(sc, SUNXI_HDMI_DDC_COMMAND_REG,
	    msg->flags & I2C_M_RD ?
	    SUNXI_HDMI_DDC_COMMAND_ACCESS_CMD_IOREAD :
	    SUNXI_HDMI_DDC_COMMAND_ACCESS_CMD_IOWRITE;

	val = SUNXI_HDMI_DDC_INT_STATUS_FIFO_REQ |
	       SUNXI_HDMI_DDC_INT_STATUS_ERR_MASK |
	       SUNXI_HDMI_DDC_INT_STATUS_TRANSFER_COMPLETE;
	HDMI_WRITE(sc, SUNXI_HDMI_DDC_INT_STATUS_REG, val);

	val = HDMI_READ(sc, SUNXI_HDMI_DDC_CTRL_REG);
	val |= SUNXI_HDMI_DDC_CTRL_ACCESS_CMD_START;
	HDMI_WRITE(sc, SUNXI_HDMI_DDC_CTRL_REG, val);

	/* read data when some is available */
	len = msg->len;
	buf = msg->buf;
	while (len > 0) {
		int blklen = imin(SUNXI_HDMI_DDC_FIFO_CTRL_TRIGGER_MAX, len);
		retry = 0;
		do {
			val = HDMI_READ(sc, SUNXI_HDMI_DDC_INT_STATUS_REG);
			if (++retry > 1000)
				return -ETIMEOUT;
		} while (val & (SUNXI_HDMI_DDC_INT_STATUS_FIFO_REQ |
			      SUNXI_HDMI_DDC_INT_STATUS_TRANSFER_COMPLETE |
			      SUNXI_HDMI_DDC_INT_STATUS_ERR_MASK) == 0);
		if (val & SUNXI_HDMI_DDC_INT_STATUS_ERR_MASK) {
			device_printf(sc->sc_dev,
			    "xfer failed, status=%08x\n", val);
			return -EIO;
		}
		if (msg->flags & I2C_M_RD)
			bus_space_read_multi_1(sc->sc_bst, sc->sc_bsh,
			    SUNXI_HDMI_DDC_FIFO_ACCESS_REG, buf, blklen);
		else 
			bus_space_write_multi_1(sc->sc_bst, sc->sc_bsh,
			    SUNXI_HDMI_DDC_FIFO_ACCESS_REG, buf, blklen);
		buf += blklen;
		len -= blklen;
		HDMI_WRITE(sc, SUNXI_HDMI_DDC_INT_STATUS_REG,
		    SUNXI_HDMI_DDC_INT_STATUS_FIFO_REQ);
	}

	retry = 0;
	while (HDMI_READ(sc, SUNXI_HDMI_DDC_CTRL_REG) & SUNXI_HDMI_DDC_CTRL_ACCESS_CMD_START) {
		if (++retry == 1000)
			return EIO;
		delay(1000);
	}

	val = HDMI_READ(sc, SUNXI_HDMI_DDC_INT_STATUS_REG);
	if ((val & SUNXI_HDMI_DDC_INT_STATUS_TRANSFER_COMPLETE) == 0 ||
	    (val & SUNXI_HDMI_DDC_INT_STATUS_ERR_MASK)) {
		device_printf(sc->sc_dev, "xfer failed, status=%08x\n", val);
		return -EIO;
	}
	return 0;
}

static int
sunxi_hdmi_i2c_xfer_1_4(struct sunxi_hdmi_softc *sc, struct i2c_msg *msg)
{
	uint32_t val;
	uint8_t *buf;
	int len;
	int retry;

	val = __SHIFTIN(1, SUNXI_HDMI_DDC_FIFO_CTRL_TX_TRIGGER_THRESH);
	val |= __SHIFTIN(SUNXI_HDMI_DDC_FIFO_CTRL_TRIGGER_MAX,
	    SUNXI_HDMI_DDC_FIFO_CTRL_RX_TRIGGER_THRESH);
	val |= SUNXI_A31_HDMI_DDC_FIFO_CTRL_RST;
	HDMI_WRITE(sc, SUNXI_A31_HDMI_DDC_FIFO_CTRL_REG, val);
	retry = 0;
	while (HDMI_READ(sc, SUNXI_A31_HDMI_DDC_FIFO_CTRL_REG) &
	    SUNXI_A31_HDMI_DDC_FIFO_CTRL_RST) {
		if (++rety > 2000)
			return -EIO;
		delay(100);
	}

	val = __SHIFTIN(msg->addr, SUNXI_HDMI_DDC_SLAVE_ADDR_3);
	HDMI_WRITE(sc, SUNXI_A31_HDMI_DDC_SLAVE_ADDR_REG, val);

	val = __SHIFTIN(msg->len, SUNXI_A31_HDMI_DDC_COMMAND_DTC);
	val |= __SHIFTIN(msg->flags & I2C_M_RD ?
		SUNXI_HDMI_DDC_COMMAND_ACCESS_CMD_IOREAD :
		SUNXI_HDMI_DDC_COMMAND_ACCESS_CMD_IOWRITE);
	HDMI_WRITE(sc, SUNXI_A31_HDMI_DDC_COMMAND_REG, val,
	    SUNXI_A31_HDMI_DDC_COMMAND_CMD));

	val = SUNXI_HDMI_DDC_INT_STATUS_FIFO_REQ |
	       SUNXI_HDMI_DDC_INT_STATUS_ERR_MASK |
	       SUNXI_HDMI_DDC_INT_STATUS_TRANSFER_COMPLETE;
	HDMI_WRITE(sc, SUNXI_A31_HDMI_DDC_INT_STATUS_REG, val);

	val = HDMI_READ(sc, SUNXI_A31_HDMI_DDC_CTRL_REG);
	val |= SUNXI_A31_HDMI_DDC_CTRL_ACCESS_CMD_START;
	HDMI_WRITE(sc, SUNXI_A31_HDMI_DDC_CTRL_REG, val);

	/* read data when some is available */
	len = msg->len;
	buf = msg->buf;
	while (len > 0) {
		int blklen;
		retry = 0;
		do {
			val = HDMI_READ(sc, SUNXI_A31_HDMI_DDC_INT_STATUS_REG);
			if (++retry > 1000)
				return -ETIMEOUT;
		} while (val & (SUNXI_HDMI_DDC_INT_STATUS_FIFO_REQ |
			      SUNXI_HDMI_DDC_INT_STATUS_TRANSFER_COMPLETE |
			      SUNXI_HDMI_DDC_INT_STATUS_ERR_MASK) == 0);
		if (val & SUNXI_HDMI_DDC_INT_STATUS_ERR_MASK) {
			device_printf(sc->sc_dev,
			    "xfer failed, status=%08x\n", val);
			return -EIO;
		}
		if (msg->flags & I2C_M_RD) {
			blklen =
			    imin(SUNXI_HDMI_DDC_FIFO_CTRL_TRIGGER_MAX - 1, len);
			bus_space_read_multi_1(sc->sc_bst, sc->sc_bsh,
			    SUNXI_A31_HDMI_DDC_FIFO_ACCESS_REG, buf, blklen);
		} else {
			blklen =
			    imin(SUNXI_HDMI_DDC_FIFO_CTRL_TRIGGER_MAX, len);
			bus_space_write_multi_1(sc->sc_bst, sc->sc_bsh,
			    SUNXI_A31_HDMI_DDC_FIFO_ACCESS_REG, buf, blklen);
		}
		buf += blklen;
		len -= blklen;
		HDMI_WRITE(sc, SUNXI_A31_HDMI_DDC_INT_STATUS_REG,
		    SUNXI_HDMI_DDC_INT_STATUS_FIFO_REQ);
	}

	retry = 1000;
	while (HDMI_READ(sc, SUNXI_A31_HDMI_DDC_CTRL_REG) & SUNXI_A31_HDMI_DDC_CTRL_ACCESS_CMD_START) {
		if (--retry == 0)
			return -ETIMEDOUT;
		delay(1000);
	}
	val = HDMI_READ(sc, SUNXI_A31_HDMI_DDC_INT_STATUS_REG);
	if ((val & SUNXI_HDMI_DDC_INT_STATUS_TRANSFER_COMPLETE) == 0 ||
	    (val & SUNXI_HDMI_DDC_INT_STATUS_ERR_MASK)) {
		device_printf(sc->sc_dev, "xfer failed, status=%08x\n", val);
		return -EIO;
	}

	return 0;
}

static int
sunxi_hdmi_i2c_xfer(struct sunxi_hdmi_softc *sc, struct i2c_msg *msg)
{
	if (HDMI_1_3_P(sc)) {
		return sunxi_hdmi_i2c_xfer_1_3(sc, msg);
	} else {
		return sunxi_hdmi_i2c_xfer_1_4(sc, msg);
	}
}

static int
sunxi_hdmi_i2c_reset(struct sunxi_hdmi_softc *sc)
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
	struct drm_crtc *crtc;
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
	sc->sc_in_rep = fdt_endpoint_remote(ep);
	KASSERT(sc->sc_in_rep != NULL);

	if (fdt_endpoint_type(sc->sc_in_rep) != EP_DRM_ENCODER) {
		device_printf(sc->sc_dev, ": in endpoint %d wrong type\n",
		    fdt_endpoint_type(sc->sc_in_rep));
		panic("hdmi fdt_endpoint_type");
	}
	crtc = sunxi_tcon_get_crtc(fdt_endpoint_device(sc->sc_in_rep));

	sc->sc_encoder.sc = sc;
	sc->sc_encoder.base.possible_crtcs = 1 << drm_crtc_index(crtc);
	drm_encoder_helper_add(&sc->sc_encoder.base,
	    &sunxi_hdmi_enc_helper_funcs);
	error = drm_encoder_init(crtc->dev, &sc->sc_encoder.base,
	    sunxi_hdmi_enc_funcs, DRM_MODE_ENCODER_TMDS, NULL);
	if (error)
		return -error;
	
	drm_connector_helper_add(&sc->sc_connector,
	    &sunxi_hdmi_connector_helper_funcs);
	error = drm_connector_init_with_ddc(crtc->dev, &sc->sc_connector,
	    DRM_MODE_CONNECTOR_HDMIA, &sc->sc_i2c_adapt);
	if (error)
		return -error;
	drm_connector_attach_encoder(&sc->sc_connector, &sc->sc_encoder.base);

	error = fdt_endpoint_activate(out_ep, activate);
	if (error == 0) {
		sc->sc_in_ep = ep;
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

		mutex_enter(&sc->sc_pwr_lock);
		error = sunxi_hdmi_poweron(sc, enable);
		mutex_exit(&sc->sc_pwr_lock);
		return error;
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

#if 0
#define EDID_BLOCK_SIZE 128

static int
sunxi_hdmi_read_edid_block(struct sunxi_hdmi_softc *sc, uint8_t *data,
    uint8_t block)
{
	i2c_tag_t tag = &sc->sc_ic;
	uint8_t wbuf[2];
	int error;

	if ((error = iic_acquire_bus(tag, 0)) != 0)
		return error;

	wbuf[0] = block;	/* start address */

	error = iic_exec(tag, I2C_OP_READ_WITH_STOP, DDC_ADDR, wbuf, 1,
	    data, EDID_BLOCK_SIZE, 0);
	iic_release_bus(tag, 0);
	return error;
}

static void
sunxi_hdmi_read_edid(struct sunxi_hdmi_softc *sc)
{
	const struct videomode *mode;
	char *edid;
	struct edid_info *eip;
	int retry = 4;
	u_int display_mode;

	edid = kmem_zalloc(EDID_BLOCK_SIZE, KM_SLEEP);
	eip = kmem_zalloc(sizeof(struct edid_info), KM_SLEEP);

	while (--retry > 0) {
		if (!sunxi_hdmi_read_edid_block(sc, edid, 0))
			break;
	}
	if (retry == 0) {
		device_printf(sc->sc_dev, "failed to read EDID\n");
	} else {
		if (edid_parse(edid, eip) != 0) {
			device_printf(sc->sc_dev, "failed to parse EDID\n");
		}
#ifdef SUNXI_HDMI_DEBUG
		else {
			edid_print(eip);
		}
#endif
	}

	if (sc->sc_display_mode == DISPLAY_MODE_AUTO)
		display_mode = sunxi_hdmi_get_display_mode(sc, eip);
	else
		display_mode = sc->sc_display_mode;

	const char *forced = sc->sc_display_mode == DISPLAY_MODE_AUTO ?
	    "auto-detected" : "forced";
	device_printf(sc->sc_dev, "%s mode (%s)\n",
	    display_mode == DISPLAY_MODE_HDMI ? "HDMI" : "DVI", forced);

	strlcpy(sc->sc_display_vendor, eip->edid_vendorname,
	    sizeof(sc->sc_display_vendor));
	strlcpy(sc->sc_display_product, eip->edid_productname,
	    sizeof(sc->sc_display_product));
	sc->sc_current_display_mode = display_mode;

	mode = eip->edid_preferred_mode;
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
	kmem_free(edid, EDID_BLOCK_SIZE);
	kmem_free(eip, sizeof(struct edid_info));
}

static u_int
sunxi_hdmi_get_display_mode(struct sunxi_hdmi_softc *sc,
    const struct edid_info *ei)
{
	char *edid;
	bool found_hdmi = false;
	unsigned int n, p;
	edid = kmem_zalloc(EDID_BLOCK_SIZE, KM_SLEEP);

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

	kmem_free(edid, EDID_BLOCK_SIZE);
	return found_hdmi ? DISPLAY_MODE_HDMI : DISPLAY_MODE_DVI;
}
#endif

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

static int
hdmi_clk_find(struct sunxi_hdmi_softc *sc, int rate, int *div, int *dbl)
{
	int best_diff;
	int parent_rate;
	struct clk *clk_pll;
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
	*div = *dbl = 0;
	best_diff = INT_MAX;
	for (int d = 2; d > 0 && best_diff != 0; d--) {
		for (int m = 1; m <= 16 && best_diff != 0; m++) {
			int cur_rate = parent_rate / m / d;
			int diff = abs(target_rate - cur_rate);
			if (diff >= 0 && diff < best_diff) {
				best_diff = diff;
				*div = m;
				*dbl = d;
			}
		}
	}

#ifdef SUNXI_HDMI_DEBUG
	device_printf(sc->sc_dev, "parent rate: %d\n", parent_rate);
	device_printf(sc->sc_dev, "crtc_clock: %d\n", mode->crtc_clock);
	device_printf(sc->sc_dev, "clkdiv: %d\n", best_div);
	device_printf(sc->sc_dev, "clkdbl: %c\n", (best_dbl == 1) ? 'Y' : 'N');
#endif
	return best_diff;
}

static void
sunxi_hdmi_mode_set(struct drm_encoder *enc,
    struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode)
{
	struct sunxi_hdmi_encoder *hdmi_encoder = to_sunxi_hdmi_encoder(enc);
	struct sunxi_hdmi_softc *sc = hdmi_encoder->sc;
	uint32_t val;
	const u_int dblscan_p = !!(mode->flags & DRM_MODE_FLAG_DBLSCAN);
	const u_int interlace_p = !!(mode->flags & DRM_MODE_FLAG_INTERLACE);
	const u_int phsync_p = !!(mode->flags & DRM_MODE_FLAG_PHSYNC);
	const u_int pvsync_p = !!(mode->flags & DRM_MODE_FLAG_PVSYNC);
	const u_int hfp = mode->hsync_start - mode->hdisplay;
	const u_int hspw = mode->hsync_end - mode->hsync_start;
	const u_int hbp = mode->htotal - mode->hsync_start;
	const u_int vfp = mode->vsync_start - mode->vdisplay;
	const u_int vspw = mode->vsync_end - mode->vsync_start;
	const u_int vbp = mode->vtotal - mode->vsync_start;
	int div, dbl, diff;
	int target_rate = mode->crtc_clock * 1000;

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

	diff = hdmi_clk_find(target_rate, &div, &dbl);

	if (div == 0) {
		device_printf(sc->sc_dev, "ERROR: TCON clk not configured\n");
		return;
	}

	uint32_t pll_ctrl, pad_ctrl0, pad_ctrl1;
	if (HDMI_1_4_P(sc)) {
		pad_ctrl0 = 0x7e8000ff;
		pad_ctrl1 = 0x01ded030;
		pll_ctrl = 0xba48a308;
		pll_ctrl |= __SHIFTIN(div - 1, SUNXI_HDMI_PLL_CTRL_PREDIV);
	} else {
		pad_ctrl0 = 0xfe800000;
		pad_ctrl1 = 0x00d8c830;
		pll_ctrl = 0xfa4ef708;
		pll_ctrl |= __SHIFTIN(div, SUNXI_HDMI_PLL_CTRL_PREDIV);
	}
	if (dbl == 2)
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
	if (sc->sc_display_mode == DISPLAY_MODE_DVI) {
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
	sunxi_hdmi_set_audiomode(sc, mode->crtc_clock);
}

static void
sunxi_hdmi_mode_valid(struct drm_encoder *enc,
    struct drm_display_mode *mode)
{
	struct sunxi_hdmi_encoder *hdmi_encoder = to_sunxi_hdmi_encoder(enc);
	struct sunxi_hdmi_softc *sc = hdmi_encoder->sc;
	int diff, div, dbl;
	int target_rate = mode->clock * 10000;

	if (target_rate > 165000000) /* max pixelclock for HDMI <= 1.2 */
		return MODE_CLOCK_HIGH;
	diff = hdmi_clk_find(target_rate, &div, &dbl);
	if (diff >= target_rate / 200) /* HDMI allows max 0.5% */
		return MODE_NOCLOCK;
	return MODE_OK;
}

static void
sunxi_hdmi_set_audiomode(struct sunxi_hdmi_softc *sc, int dot_clock)
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
	if (sc->sc_display_mode != DISPLAY_MODE_HDMI) {
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
	cts = ((dot_clock * 10) * (n / 128)) / 480;
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

static enum drm_connector_status   
sunxi_hdmi_connector_detect(struct drm_connector *connector, bool force)
{
	struct sunxi_hdmi_softc *sc = connector_to_hdmi(connector);

	if (HDMI_READ(sc, SUNXI_HDMI_HPD_REG) & SUNXI_HDMI_HPD_HOTPLUG_DET) {
		return connector_status_connected;
	}
	sc->sc_display_mode = DISPLAY_MODE_NONE;
	return connector_status_disconnected;
}

static int
sunxi_hdmi_get_modes(struct drm_connector * connector)
{
	struct sunxi_hdmi_softc *sc = connector_to_hdmi(connector);
	struct edid *edid;
	int ret;

	edid = drm_get_edid(connector, &sc->sc_i2c_adapt);
	if (!edid) {
		sc->sc_display_mode = DISPLAY_MODE_NONE;
		return 0;
	}

	if (drm_detect_hdmi_monitor(edid)) {
		sc->sc_display_mode = DISPLAY_MODE_HDMI;
	} else {
		sc->sc_display_mode = DISPLAY_MODE_DVI;
	}
	drm_connector_update_edid_property(connector, edid);
	ret = drm_add_edid_modes(connector, edid);
	kfree(edid);
	return ret;
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
