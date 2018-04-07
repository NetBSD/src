/*	$NetBSD: connector_fdt.c,v 1.1.2.2 2018/04/07 04:12:14 pgoyette Exp $	*/

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
 * connector driver.
 * specified in linux/Documentation/devicetree/bindings/display/connector/
 * basically it only register its endpoint.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: connector_fdt.c,v 1.1.2.2 2018/04/07 04:12:14 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_port.h>
#include <dev/fdt/connector_fdt.h>

static int fdt_connector_match(device_t, cfdata_t, void *);
static void fdt_connector_attach(device_t, device_t, void *);
static void *fdt_connector_get_data(device_t, struct fdt_endpoint *);

SLIST_HEAD(, fdt_connector_softc) fdt_connectors =
    SLIST_HEAD_INITIALIZER(&fdt_connectors);

struct fdt_connector_softc {
	device_t sc_dev;
	int sc_phandle;
	struct fdt_connector sc_con;
	SLIST_ENTRY(fdt_connector_softc) sc_list;
	struct fdt_device_ports sc_ports;
};

#define sc_type sc_con.con_type

CFATTACH_DECL_NEW(fdt_connector, sizeof(struct fdt_connector_softc),
    fdt_connector_match, fdt_connector_attach, NULL, NULL);

static const struct of_compat_data compat_data[] = {
	{"composite-video-connector", CON_TV},
	{"dvi-connector", CON_DVI},
	{"hdmi-connector", CON_HDMI},
	{"vga-connector", CON_VGA},
	{ NULL }
};

static int
fdt_connector_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct fdt_attach_args *faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
fdt_connector_attach(device_t parent, device_t self, void *aux)
{
	struct fdt_connector_softc *sc = device_private(self);
	const struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_type = of_search_compatible(phandle, compat_data)->data;

	SLIST_INSERT_HEAD(&fdt_connectors, sc, sc_list);

	aprint_naive("\n");
	switch(sc->sc_type) {
	case CON_VGA:
		aprint_normal(": VGA");
		break;
	case CON_DVI:
		aprint_normal(": DVI");
		break;
	case CON_HDMI:
		aprint_normal(": HDMI");
		break;
	case CON_TV:
		aprint_normal(": composite");
		break;
	default:
		panic("unknown connector type %d\n", sc->sc_type);
	}
	aprint_normal(" connector\n");
	sc->sc_ports.dp_ep_get_data = fdt_connector_get_data;
	fdt_ports_register(&sc->sc_ports, self, phandle, EP_CONNECTOR);
}

static void *
fdt_connector_get_data(device_t dev, struct fdt_endpoint *ep)
{
	struct fdt_connector_softc *sc = device_private(dev);

	return &sc->sc_con;
}
