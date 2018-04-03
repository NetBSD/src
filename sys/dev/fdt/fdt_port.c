/*	$NetBSD: fdt_port.c,v 1.1 2018/04/03 12:40:20 bouyer Exp $	*/

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
 * Given a device and its node, it enumerates all ports and endpoints for this
 * device, and register connections with the remote endpoints.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: fdt_port.c,v 1.1 2018/04/03 12:40:20 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_port.h>

struct fdt_endpoint;

struct fdt_port {
	int	port_id;
	int	port_phandle; /* port's node */
	struct fdt_endpoint *port_ep; /* this port's endpoints */
	int	port_nep; /* number of endpoints for this port */
	struct fdt_device_ports *port_dp; /* this port's device */
};

struct fdt_endpoint {
	int		ep_id;
	enum endpoint_type ep_type;
	int		ep_phandle;
	struct fdt_port	*ep_port; /* parent of this endpoint */
	int		ep_rphandle; /* report endpoint */
	struct fdt_endpoint *ep_rep;
	bool		ep_active;
	bool		ep_enabled;
};

SLIST_HEAD(, fdt_device_ports) fdt_port_devices =
    SLIST_HEAD_INITIALIZER(&fdt_port_devices);

static void fdt_endpoints_register(int, struct fdt_port *, enum endpoint_type);
static const char *ep_name(struct fdt_endpoint *, char *, int);

struct fdt_endpoint *
fdt_endpoint_get_from_phandle(int rphandle)
{
	struct fdt_device_ports *ports;
	int p, e;

	if (rphandle < 0)
		return NULL;

	SLIST_FOREACH(ports, &fdt_port_devices, dp_list) {
		for (p = 0; p < ports->dp_nports; p++) {
			struct fdt_port *port = &ports->dp_port[p];
			for (e = 0; e < port->port_nep; e++) {
				struct fdt_endpoint *ep = &port->port_ep[e];
				if (ep->ep_phandle == rphandle)
					return ep;
			}
		}
	}
	return NULL;

}

struct fdt_endpoint *
fdt_endpoint_get_from_index(struct fdt_device_ports *device_ports,
    int port_index, int ep_index)
{
	int p, e;
	for (p = 0; p < device_ports->dp_nports; p++) {
		struct fdt_port *port = &device_ports->dp_port[p];
		if (port->port_id != port_index)
			continue;
		for (e = 0; e < port->port_nep; e++) {
			struct fdt_endpoint *ep = &port->port_ep[e];
			if (ep->ep_id == ep_index) {
				return ep;
			}
		}
	}
	return NULL;
}

struct fdt_endpoint *
fdt_endpoint_remote(struct fdt_endpoint *ep)
{
	return ep->ep_rep;
}

int
fdt_endpoint_port_index(struct fdt_endpoint *ep)
{
	return ep->ep_port->port_id;
}

int
fdt_endpoint_index(struct fdt_endpoint *ep)
{
	return ep->ep_id;
}

device_t
fdt_endpoint_device(struct fdt_endpoint *ep)
{
	return ep->ep_port->port_dp->dp_dev;
}

bool
fdt_endpoint_is_active(struct fdt_endpoint *ep)
{
	return ep->ep_active;
}

bool
fdt_endpoint_is_enabled(struct fdt_endpoint *ep)
{
	return ep->ep_enabled;
}

int
fdt_endpoint_activate(struct fdt_endpoint *ep, bool activate)
{
	struct fdt_endpoint *rep = fdt_endpoint_remote(ep);
	struct fdt_device_ports *rdp;
	int error = 0;

	if (rep == NULL)
		return ENODEV;

	KASSERT(ep->ep_active == rep->ep_active);
	KASSERT(ep->ep_enabled == rep->ep_enabled);
	if (!activate && ep->ep_enabled)
		return EBUSY;

	rdp = rep->ep_port->port_dp;
	if (rdp->dp_ep_activate)
		error = rdp->dp_ep_activate(rdp->dp_dev, rep, activate);

	if (error == 0)
		rep->ep_active = ep->ep_active = activate;
	return error;
}

int
fdt_endpoint_enable(struct fdt_endpoint *ep, bool enable)
{
	struct fdt_endpoint *rep = fdt_endpoint_remote(ep);
	struct fdt_device_ports *rdp;
	int error = 0;

	if (rep == NULL)
		return EINVAL;

	KASSERT(ep->ep_active == rep->ep_active);
	KASSERT(ep->ep_enabled == rep->ep_enabled);
	if (ep->ep_active == false)
		return EINVAL;

	rdp = rep->ep_port->port_dp;
	if (rdp->dp_ep_enable)
		error = rdp->dp_ep_enable(rdp->dp_dev, rep, enable);

	if (error == 0)
		rep->ep_enabled = ep->ep_enabled = enable;
	return error;
}

void *
fdt_endpoint_get_data(struct fdt_endpoint *ep)
{
	struct fdt_device_ports *dp = ep->ep_port->port_dp;

	if (dp->dp_ep_get_data)
		return dp->dp_ep_get_data(dp->dp_dev, ep);

	return NULL;
}

int
fdt_ports_register(struct fdt_device_ports *ports, device_t self,
    int phandle, enum endpoint_type type)
{
	int port_phandle, child;
	int i;
	char buf[20];
	uint64_t id;

	ports->dp_dev = self;
	SLIST_INSERT_HEAD(&fdt_port_devices, ports, dp_list);

	/*
	 * walk the childs looking for ports. ports may be grouped under
	 * an optional ports node
	 */
	port_phandle = phandle;
again:
	ports->dp_nports = 0;
	for (child = OF_child(port_phandle); child; child = OF_peer(child)) {
		if (OF_getprop(child, "name", buf, sizeof(buf)) <= 0)
			continue;
		if (strcmp(buf, "ports") == 0) {
			port_phandle = child;
			goto again;
		}
		if (strcmp(buf, "port") != 0)
			continue;
		ports->dp_nports++;
	}
	if (ports->dp_nports == 0)
		return 0;

	ports->dp_port =
	    kmem_zalloc(sizeof(struct fdt_port) * ports->dp_nports, KM_SLEEP);
	KASSERT(ports->dp_port != NULL);
	/* now scan again ports, looking for endpoints */
	for (child = OF_child(port_phandle), i = 0; child;
	    child = OF_peer(child)) {
		if (OF_getprop(child, "name", buf, sizeof(buf)) <= 0)
			continue;
		if (strcmp(buf, "ports") == 0) {
			panic("fdt_ports_register: undetected ports");
		}
		if (strcmp(buf, "port") != 0)
			continue;
		if (fdtbus_get_reg64(child, 0, &id, NULL) != 0) {
			if (ports->dp_nports > 1)
				aprint_error_dev(self,
				    "%s: missing reg property",
				    fdtbus_get_string(child, "name"));
			id = i;
		}
		ports->dp_port[i].port_id = id;
		ports->dp_port[i].port_phandle = child;
		ports->dp_port[i].port_dp = ports;
		fdt_endpoints_register(child, &ports->dp_port[i], type);
		i++;
	}
	KASSERT(i == ports->dp_nports);
	return 0;
}


static void
fdt_endpoints_register(int phandle, struct fdt_port *port,
    enum endpoint_type type)
{
	int child;
	int i;
	char buf[128];
	uint64_t id;
	struct fdt_endpoint *ep, *rep;
	struct fdt_device_ports *dp;

	port->port_nep = 0;
	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		if (OF_getprop(child, "name", buf, sizeof(buf)) <= 0)
			continue;
		if (strcmp(buf, "endpoint") != 0)
			continue;
		port->port_nep++;
	}
	if (port->port_nep == 0) {
		port->port_ep = NULL;
		return;
	}

	port->port_ep =
	    kmem_zalloc(sizeof(struct fdt_endpoint) * port->port_nep, KM_SLEEP);
	KASSERT(port->port_ep != NULL);
	/* now scan again ports, looking for endpoints */
	for (child = OF_child(phandle), i = 0; child; child = OF_peer(child)) {
		if (OF_getprop(child, "name", buf, sizeof(buf)) <= 0)
			continue;
		if (strcmp(buf, "endpoint") != 0)
			continue;
		if (fdtbus_get_reg64(child, 0, &id, NULL) != 0) {
			if (port->port_nep > 1)
				aprint_error_dev(port->port_dp->dp_dev,
				    "%s: missing reg property",
				    fdtbus_get_string(child, "name"));
			id = i;
		}
		ep = &port->port_ep[i];
		ep->ep_id = id;
		ep->ep_type = type;
		ep->ep_phandle = child;
		ep->ep_port = port;
		ep->ep_rphandle = fdtbus_get_phandle(child, "remote-endpoint");
		ep->ep_rep = fdt_endpoint_get_from_phandle(
		    port->port_ep[i].ep_rphandle);
		rep = ep->ep_rep;
		if (rep != NULL && rep->ep_rep != NULL) {
			aprint_error("%s: ", ep_name(ep, buf, sizeof(buf)));
			aprint_error("remote endpoint %s ",
			    ep_name(rep, buf, sizeof(buf)));
			aprint_error("already connected to %s\n",
			    ep_name(rep->ep_rep, buf, sizeof(buf)));
		} else if (rep != NULL) {
			rep->ep_rep = ep;
			rep->ep_rphandle = child;
			aprint_verbose("%s ", ep_name(ep, buf, sizeof(buf)));
			aprint_verbose("connected to %s\n",
			    ep_name(rep, buf, sizeof(buf)));
			if (rep->ep_type == EP_OTHER)
				rep->ep_type = ep->ep_type;
			else if (ep->ep_type == EP_OTHER)
				ep->ep_type = rep->ep_type;
			dp = port->port_dp;
			if (dp->dp_ep_connect)
				dp->dp_ep_connect(dp->dp_dev, ep, true);
			dp = rep->ep_port->port_dp;
			if (dp->dp_ep_connect)
				dp->dp_ep_connect(dp->dp_dev, rep, true);
		}
		i++;
	}
	KASSERT(i == port->port_nep);
}

static const char *
ep_name(struct fdt_endpoint *ep, char *buf, int size)
{
	int a;

	a = snprintf(&buf[0], size, "%s",
	    device_xname(ep->ep_port->port_dp->dp_dev));
	if (ep->ep_port->port_id >= 0 && a < size)
		a += snprintf(&buf[a], size - a, " port %d",
		    ep->ep_port->port_id);
	if (ep->ep_id >= 0 && a < size)
		snprintf(&buf[a], size - a, " endpoint %d", ep->ep_id);
	return buf;
}
