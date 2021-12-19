/* $NetBSD: hdmi_connector.c,v 1.3 2021/12/19 11:00:46 riastradh Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: hdmi_connector.c,v 1.3 2021/12/19 11:00:46 riastradh Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/gpio.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_port.h>

#include <dev/i2c/ddcvar.h>

#include <drm/drm_drv.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_edid.h>
#include <drm/drm_probe_helper.h>

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "hdmi-connector" },
	DEVICE_COMPAT_EOL
};

struct dispcon_hdmi_connector {
	struct drm_connector		base;
	struct fdtbus_gpio_pin		*hpd;
	i2c_tag_t			ddc;

	int				type;	/* DRM_MODE_CONNECTOR_* */
};

struct dispcon_hdmi_softc {
	struct fdt_device_ports		sc_ports;
	struct dispcon_hdmi_connector	sc_connector;
};

#define	to_dispcon_hdmi_connector(x)	container_of(x, struct dispcon_hdmi_connector, base)

static enum drm_connector_status
dispcon_hdmi_connector_detect(struct drm_connector *connector, bool force)
{
	struct dispcon_hdmi_connector *hdmi_connector = to_dispcon_hdmi_connector(connector);
	bool con;

	if (hdmi_connector->hpd == NULL) {
		/*
		 * No hotplug detect pin available. Assume that we are connected.
		 */
		return connector_status_connected;
	}

	/*
	 * Read connect status from hotplug detect pin.
	 */
	con = fdtbus_gpio_read(hdmi_connector->hpd);
	if (con) {
		return connector_status_connected;
	} else {
		return connector_status_disconnected;
	}
}

static void
dispcon_hdmi_connector_destroy(struct drm_connector *connector)
{
	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);
}

static const struct drm_connector_funcs dispcon_hdmi_connector_funcs = {
	.dpms = drm_helper_connector_dpms,
	.detect = dispcon_hdmi_connector_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = dispcon_hdmi_connector_destroy,
};

static int
dispcon_hdmi_connector_mode_valid(struct drm_connector *connector, struct drm_display_mode *mode)
{
	return MODE_OK;
}

static int
dispcon_hdmi_connector_get_modes(struct drm_connector *connector)
{
	struct dispcon_hdmi_connector *hdmi_connector = to_dispcon_hdmi_connector(connector);
	char edid[EDID_LENGTH * 4];
	struct edid *pedid = NULL;
	int error, block;

	if (hdmi_connector->ddc != NULL) {
		memset(edid, 0, sizeof(edid));
		for (block = 0; block < 4; block++) {
			error = ddc_read_edid_block(hdmi_connector->ddc,
			    &edid[block * EDID_LENGTH], EDID_LENGTH, block);
			if (error)
				break;
			if (block == 0) {
				pedid = (struct edid *)edid;
				if (edid[0x7e] == 0)
					break;
			}
		}
	}

	drm_connector_update_edid_property(connector, pedid);
	if (pedid == NULL)
		return 0;

	return drm_add_edid_modes(connector, pedid);
}

static const struct drm_connector_helper_funcs dispcon_hdmi_connector_helper_funcs = {
	.mode_valid = dispcon_hdmi_connector_mode_valid,
	.get_modes = dispcon_hdmi_connector_get_modes,
};

static int
dispcon_hdmi_ep_activate(device_t dev, struct fdt_endpoint *ep, bool activate)
{
	struct drm_connector *connector = fdt_endpoint_get_data(ep);
	struct dispcon_hdmi_connector *hdmi_connector = to_dispcon_hdmi_connector(connector);
	struct fdt_endpoint *rep = fdt_endpoint_remote(ep);
	struct drm_encoder *encoder;

	if (fdt_endpoint_port_index(ep) != 0)
		return EINVAL;

	if (fdt_endpoint_type(rep) != EP_DRM_ENCODER)
		return EINVAL;

	if (activate) {
		encoder = fdt_endpoint_get_data(rep);

		connector->polled = DRM_CONNECTOR_POLL_CONNECT | DRM_CONNECTOR_POLL_DISCONNECT;
		connector->interlace_allowed = 0;
		connector->doublescan_allowed = 0;

		drm_connector_init(encoder->dev, connector, &dispcon_hdmi_connector_funcs,
		    hdmi_connector->type);
		drm_connector_helper_add(connector, &dispcon_hdmi_connector_helper_funcs);
		drm_connector_register(connector);
		drm_connector_attach_encoder(connector, encoder);
	}

	return 0;
}

static void *
dispcon_hdmi_ep_get_data(device_t dev, struct fdt_endpoint *ep)
{
	struct dispcon_hdmi_softc * const sc = device_private(dev);

	return &sc->sc_connector.base;
}

static int
dispcon_hdmi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
dispcon_hdmi_attach(device_t parent, device_t self, void *aux)
{
	struct dispcon_hdmi_softc * const sc = device_private(self);
	struct dispcon_hdmi_connector * const hdmi_connector = &sc->sc_connector;
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	aprint_naive("\n");
	aprint_normal(": HDMI connector\n");

	hdmi_connector->type = DRM_MODE_CONNECTOR_HDMIA;
	hdmi_connector->hpd = fdtbus_gpio_acquire(phandle, "hpd-gpios", GPIO_PIN_INPUT);
	hdmi_connector->ddc = fdtbus_i2c_acquire(phandle, "ddc-i2c-bus");

	sc->sc_ports.dp_ep_activate = dispcon_hdmi_ep_activate;
	sc->sc_ports.dp_ep_get_data = dispcon_hdmi_ep_get_data;
	fdt_ports_register(&sc->sc_ports, self, phandle, EP_DRM_CONNECTOR);
}

CFATTACH_DECL_NEW(dispcon_hdmi, sizeof(struct dispcon_hdmi_softc),
	dispcon_hdmi_match, dispcon_hdmi_attach, NULL, NULL);
