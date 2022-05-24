/*	$NetBSD: fdt_port.h,v 1.6 2022/05/24 20:50:19 andvar Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Manuel Bouyer.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * ports and endpoints management. from
 * linux/Documentation/devicetree/bindings/graph.txt
 * 2 endpoints can be connected together in the device tree. In this case
 * an endpoint will have a remote endpoint.
 * A pair of connected endpoints can be activated by a driver; when it choose
 * to use this path.
 * A pair of active endpoints can be enabled; when a driver starts to send
 * a signal. Disabling a pair of endpoints can cause appropriate actions
 * to save power (stop clocks, disable output buffers, turn on/off power, ...)
 */

#ifndef _DEV_FDT_FDT_PORT_H_
#define _DEV_FDT_FDT_PORT_H_

struct drm_device;

struct fdt_port;
struct fdt_endpoint;

struct fdt_device_ports {
	struct fdt_port *dp_port; /* this device's ports */
	int		dp_nports; /* number of ports for this device */
	device_t	dp_dev;
	/* callbacks to device drivers owning endpoints */
	void		(*dp_ep_connect)(device_t, struct fdt_endpoint *, bool);
	int		(*dp_ep_activate)(device_t, struct fdt_endpoint *, bool);
	int		(*dp_ep_enable)(device_t, struct fdt_endpoint *, bool);
	void *		(*dp_ep_get_data)(device_t, struct fdt_endpoint *);
	SLIST_ENTRY(fdt_device_ports) dp_list;
};

enum endpoint_type {
	EP_OTHER = 0,
	EP_CONNECTOR,
	EP_PANEL,

	EP_DRM_BRIDGE,		/* struct drm_bridge */
	EP_DRM_CONNECTOR,	/* struct drm_connector */
	EP_DRM_CRTC,		/* struct drm_crtc */
	EP_DRM_ENCODER,		/* struct drm_encoder */
	EP_DRM_PANEL,		/* struct drm_panel */
};


/*
 * register a device's ports and enpoints into the provided fdt_device_ports.
 * when and endpoint is connected to a remote endpoint, dp_ep_connect
 * is called for the devices associated to both endpoints
 */
int fdt_ports_register(struct fdt_device_ports *, device_t,
					int, enum endpoint_type);

/* various methods to retrieve an enpoint descriptor */
struct fdt_endpoint *fdt_endpoint_get_from_phandle(int);
struct fdt_endpoint *fdt_endpoint_get_from_index(struct fdt_device_ports *,
							int, int);
struct fdt_endpoint *fdt_endpoint_remote(struct fdt_endpoint *);
struct fdt_endpoint *fdt_endpoint_remote_from_index(struct fdt_device_ports *,
							int, int);

/*
 * get informations/data for a given endpoint
 */
int fdt_endpoint_port_index(struct fdt_endpoint *);
int fdt_endpoint_index(struct fdt_endpoint *);
int fdt_endpoint_phandle(struct fdt_endpoint *);
device_t fdt_endpoint_device(struct fdt_endpoint *);
bool fdt_endpoint_is_active(struct fdt_endpoint *);
bool fdt_endpoint_is_enabled(struct fdt_endpoint *);
enum endpoint_type fdt_endpoint_type(struct fdt_endpoint *);
/*
 * call dp_ep_get_data() for the endpoint. The returned pointer is
 * type of driver-specific.
 */
void * fdt_endpoint_get_data(struct fdt_endpoint *);

/*
 * Activate/deactivate an endpoint. This causes dp_ep_activate() to be
 * called for the remote endpoint
 */
int fdt_endpoint_activate(struct fdt_endpoint *, bool);

/*
 * Activate/deactivate an endpoint by direct reference.
 */
int fdt_endpoint_activate_direct(struct fdt_endpoint *, bool);

/*
 * Enable/disable an endpoint. This causes dp_ep_enable() to be called for
 * the remote endpoint
 */
int fdt_endpoint_enable(struct fdt_endpoint *, bool);

#endif /* _DEV_FDT_FDT_PORT_H */
