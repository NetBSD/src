/* $NetBSD: anxedp.c,v 1.8 2021/12/19 11:01:10 riastradh Exp $ */

/*-
 * Copyright (c) 2019 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: anxedp.c,v 1.8 2021/12/19 11:01:10 riastradh Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <dev/ic/dw_hdmi.h>

#include <dev/i2c/ddcreg.h>
#include <dev/i2c/ddcvar.h>
#include <dev/i2c/i2cvar.h>
#include <dev/videomode/edidvar.h>

#include <dev/fdt/fdt_port.h>
#include <dev/fdt/fdtvar.h>

#include <drm/drm_connector.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_edid.h>
#include <drm/drm_probe_helper.h>

#define	ANX_DP_AUX_CH_CTL_1	0xe5
#define	 ANX_AUX_LENGTH		__BITS(7,4)
#define	 ANX_AUX_TX_COMM	__BITS(3,0)
#define	  ANX_AUX_TX_COMM_MOT	4
#define	  ANX_AUX_TX_COMM_READ	1
#define	ANX_DP_AUX_ADDR(n)	(0xe6 + (n))
#define	ANX_DP_AUX_CH_CTL_2	0xe9
#define	 ANX_ADDR_ONLY		__BIT(1)
#define	 ANX_AUX_EN		__BIT(0)
#define	ANX_BUF_DATA(n)		(0xf0 + (n))

#define	ANX_DP_INT_STA		0xf7
#define	 ANX_RPLY_RECEIV	__BIT(1)

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "analogix,anx6345" },
	DEVICE_COMPAT_EOL
};

struct anxedp_softc;

struct anxedp_connector {
	struct drm_connector	base;
	struct anxedp_softc	*sc;
};

struct anxedp_softc {
	device_t		sc_dev;
	i2c_tag_t		sc_i2c;
	i2c_addr_t		sc_addr;
	int			sc_phandle;

	struct anxedp_connector sc_connector;
	struct drm_bridge	sc_bridge;

	struct fdt_device_ports	sc_ports;
	struct drm_display_mode	sc_curmode;
};

#define	to_anxedp_connector(x)	container_of(x, struct anxedp_connector, base)

static uint8_t
anxedp_read(struct anxedp_softc *sc, u_int off, uint8_t reg)
{
	uint8_t val;

	if (iic_smbus_read_byte(sc->sc_i2c, sc->sc_addr + off, reg, &val, 0) != 0)
		val = 0xff;

	return val;
}

static void
anxedp_write(struct anxedp_softc *sc, u_int off, uint8_t reg, uint8_t val)
{
	(void)iic_smbus_write_byte(sc->sc_i2c, sc->sc_addr + off, reg, val, 0);
}

static int
anxedp_connector_dpms(struct drm_connector *connector, int mode)
{
	int error;

	if (mode != DRM_MODE_DPMS_ON)
		pmf_event_inject(NULL, PMFE_DISPLAY_OFF);

	error = drm_helper_connector_dpms(connector, mode);

	if (mode == DRM_MODE_DPMS_ON)
		pmf_event_inject(NULL, PMFE_DISPLAY_ON);
		
	return error;
}

static enum drm_connector_status
anxedp_connector_detect(struct drm_connector *connector, bool force)
{
	return connector_status_connected;
}

static void
anxedp_connector_destroy(struct drm_connector *connector)
{
	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);
}

static const struct drm_connector_funcs anxedp_connector_funcs = {
	.dpms = anxedp_connector_dpms,
	.detect = anxedp_connector_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = anxedp_connector_destroy,
};

static int
anxedp_aux_wait(struct anxedp_softc *sc)
{
	uint8_t val;
	int retry;

	for (retry = 1000; retry > 0; retry--) {
		val = anxedp_read(sc, 0, ANX_DP_AUX_CH_CTL_2);
		if ((val & ANX_AUX_EN) == 0)
			break;
		delay(100);
	}
	if (retry == 0) {
		device_printf(sc->sc_dev, "aux transfer timeout\n");
		return ETIMEDOUT;
	}

	for (retry = 1000; retry > 0; retry--) {
		val = anxedp_read(sc, 1, ANX_DP_INT_STA);
		if ((val & ANX_RPLY_RECEIV) != 0) {
			anxedp_write(sc, 1, ANX_DP_INT_STA, val);
			break;
		}
		delay(100);
	}
	if (retry == 0) {
		device_printf(sc->sc_dev, "aux transfer timeout\n");
		return ETIMEDOUT;
	}

	return 0;
}

static int
anxedp_aux_transfer(struct anxedp_softc *sc, uint8_t comm, uint32_t addr,
    uint8_t *buf, int buflen)
{
	uint8_t ctrl[2];
	int n, error;

	ctrl[0] = __SHIFTIN(comm, ANX_AUX_TX_COMM);
	if (buflen > 0)
		ctrl[0] |= __SHIFTIN(buflen - 1, ANX_AUX_LENGTH);
	ctrl[1] = ANX_AUX_EN;
	if (buflen == 0)
		ctrl[1] |= ANX_ADDR_ONLY;

	if (comm != ANX_AUX_TX_COMM_READ) {
		for (n = 0; n < buflen; n++)
			anxedp_write(sc, 0, ANX_BUF_DATA(n), buf[n]);
	}

	anxedp_write(sc, 0, ANX_DP_AUX_ADDR(0), addr & 0xff);
	anxedp_write(sc, 0, ANX_DP_AUX_ADDR(1), (addr >> 8) & 0xff);
	anxedp_write(sc, 0, ANX_DP_AUX_ADDR(2), (addr >> 16) & 0xf);
	anxedp_write(sc, 0, ANX_DP_AUX_CH_CTL_1, ctrl[0]);
	anxedp_write(sc, 0, ANX_DP_AUX_CH_CTL_2, ctrl[1]);

	error = anxedp_aux_wait(sc);
	if (error != 0)
		return error;

	if (comm == ANX_AUX_TX_COMM_READ) {
		for (n = 0; n < buflen; n++)
			buf[n] = anxedp_read(sc, 0, ANX_BUF_DATA(n));
	}

	return 0;
}

static int
anxedp_read_edid(struct anxedp_softc *sc, uint8_t *edid, int edidlen)
{
	int error;
	uint8_t n;

	for (n = 0; n < edidlen; n += 16) {
		const int xferlen = MIN(edidlen - n, 16);

		error = anxedp_aux_transfer(sc, ANX_AUX_TX_COMM_MOT, DDC_ADDR, &n, 1);
		if (error != 0)
			return error;

		error = anxedp_aux_transfer(sc, ANX_AUX_TX_COMM_READ, DDC_ADDR, &edid[n], xferlen);
		if (error != 0)
			return error;
	}

	return 0;
}

static int
anxedp_connector_get_modes(struct drm_connector *connector)
{
	struct anxedp_connector *anxedp_connector = to_anxedp_connector(connector);
	struct anxedp_softc * const sc = anxedp_connector->sc;
	char edid[EDID_LENGTH];
	struct edid *pedid = NULL;
	int error;

	iic_acquire_bus(sc->sc_i2c, 0);
	error = anxedp_read_edid(sc, edid, sizeof(edid));
	iic_release_bus(sc->sc_i2c, 0);
	if (error == 0)
		pedid = (struct edid *)edid;

	drm_connector_update_edid_property(connector, pedid);
	if (pedid == NULL)
		return 0;

	return drm_add_edid_modes(connector, pedid);
}

static const struct drm_connector_helper_funcs anxedp_connector_helper_funcs = {
	.get_modes = anxedp_connector_get_modes,
};

static int
anxedp_bridge_attach(struct drm_bridge *bridge)
{
	struct anxedp_softc * const sc = bridge->driver_private;
	struct anxedp_connector *anxedp_connector = &sc->sc_connector;
	struct drm_connector *connector = &anxedp_connector->base;
	int error;

	anxedp_connector->sc = sc;

	connector->polled = DRM_CONNECTOR_POLL_CONNECT | DRM_CONNECTOR_POLL_DISCONNECT;
	connector->interlace_allowed = 0;
	connector->doublescan_allowed = 0;

	drm_connector_init(bridge->dev, connector, &anxedp_connector_funcs,
	    connector->connector_type);
	drm_connector_helper_add(connector, &anxedp_connector_helper_funcs);

	error = drm_connector_attach_encoder(connector, bridge->encoder);
	if (error != 0)
		return error;

	return drm_connector_register(connector);
}

static void
anxedp_bridge_enable(struct drm_bridge *bridge)
{
}

static void
anxedp_bridge_pre_enable(struct drm_bridge *bridge)
{
}

static void
anxedp_bridge_disable(struct drm_bridge *bridge)
{
}

static void
anxedp_bridge_post_disable(struct drm_bridge *bridge)
{
}

static void
anxedp_bridge_mode_set(struct drm_bridge *bridge,
    const struct drm_display_mode *mode,
    const struct drm_display_mode *adjusted_mode)
{
	struct anxedp_softc * const sc = bridge->driver_private;

	sc->sc_curmode = *adjusted_mode;
}

static bool
anxedp_bridge_mode_fixup(struct drm_bridge *bridge,
    const struct drm_display_mode *mode, struct drm_display_mode *adjusted_mode)
{
	return true;
}

static const struct drm_bridge_funcs anxedp_bridge_funcs = {
	.attach = anxedp_bridge_attach,
	.enable = anxedp_bridge_enable,
	.pre_enable = anxedp_bridge_pre_enable,
	.disable = anxedp_bridge_disable,
	.post_disable = anxedp_bridge_post_disable,
	.mode_set = anxedp_bridge_mode_set,
	.mode_fixup = anxedp_bridge_mode_fixup,
};

static int
anxedp_ep_activate(device_t dev, struct fdt_endpoint *ep, bool activate)
{
	struct anxedp_softc * const sc = device_private(dev);
	struct fdt_endpoint *in_ep = fdt_endpoint_remote(ep);
	struct drm_encoder *encoder;
	struct drm_bridge *bridge;
	int error;

	if (!activate)
		return EINVAL;

	if (fdt_endpoint_port_index(ep) != 0)
		return EINVAL;

	switch (fdt_endpoint_type(in_ep)) {
	case EP_DRM_ENCODER:
		encoder = fdt_endpoint_get_data(in_ep);
		break;
	case EP_DRM_BRIDGE:
		bridge = fdt_endpoint_get_data(in_ep);
		encoder = bridge->encoder;
		break;
	default:
		encoder = NULL;
		break;
	}

	if (encoder == NULL)
		return EINVAL;

	sc->sc_connector.base.connector_type = DRM_MODE_CONNECTOR_eDP;

	sc->sc_bridge.driver_private = sc;
	sc->sc_bridge.funcs = &anxedp_bridge_funcs;
	sc->sc_bridge.encoder = encoder;

	error = drm_bridge_attach(encoder, &sc->sc_bridge, NULL);
	if (error != 0)
		return EIO;

	return 0;
}

static void *
anxedp_ep_get_data(device_t dev, struct fdt_endpoint *ep)
{
	struct anxedp_softc * const sc = device_private(dev);

	return &sc->sc_bridge;
}

static int
anxedp_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, compat_data, &match_result))
		return match_result;

	/* This device is direct-config only */

	return 0;
}

static void
anxedp_attach(device_t parent, device_t self, void *aux)
{
	struct anxedp_softc * const sc = device_private(self);
	struct i2c_attach_args * const ia = aux;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_phandle = ia->ia_cookie;

	aprint_naive("\n");
	aprint_normal(": eDP TX\n");

	sc->sc_ports.dp_ep_activate = anxedp_ep_activate;
	sc->sc_ports.dp_ep_get_data = anxedp_ep_get_data;
	fdt_ports_register(&sc->sc_ports, self, sc->sc_phandle, EP_DRM_BRIDGE);
}

CFATTACH_DECL_NEW(anxedp, sizeof(struct anxedp_softc),
    anxedp_match, anxedp_attach, NULL, NULL);
