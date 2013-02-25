/*	$NetBSD: xsurf.c,v 1.1.6.2 2013/02/25 00:28:22 tls Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Bernd Ernesti.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xsurf.c,v 1.1.6.2 2013/02/25 00:28:22 tls Exp $");

/*
 * X-Surf driver, split from ne_zbus. 
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/bus.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <dev/ic/dp8390reg.h>
#include <dev/ic/dp8390var.h>

#include <dev/ic/ne2000reg.h>
#include <dev/ic/ne2000var.h>

#include <dev/ic/rtl80x9reg.h>
#include <dev/ic/rtl80x9var.h>

#include <amiga/amiga/device.h>
#include <amiga/amiga/isr.h>

#include <amiga/dev/zbusvar.h>
#include <amiga/dev/xsurfvar.h>

//#include <amiga/clockport/clockportvar.h>

int	xsurf_match(device_t, cfdata_t , void *);
void	xsurf_attach(device_t, device_t, void *);
static int xsurf_print(void *aux, const char *w);

struct xsurf_softc {
	device_t sc_dev;	
};

CFATTACH_DECL_NEW(xsurf, sizeof(struct xsurf_softc),
    xsurf_match, xsurf_attach, NULL, NULL);

#define XSURF_NE_OFFSET		0x8000
#define XSURF_WDC_OFFSET	0xB000

/*
 * Clockport offsets.
 */
#define XSURF_CP1_BASE		0xA001
#define XSURF_CP2_BASE		0xC000

/*
 * E3B Deneb firmware v11 creates fake X-Surf autoconfig entry.
 * Do not attach ne driver to this fake card, otherwise kernel panic
 * may occur.
 */
#define DENEB_XSURF_SERNO	0xC0FFEE01	/* Serial of the fake card */

int
xsurf_match(device_t parent, cfdata_t cf, void *aux)
{
	struct zbus_args *zap = aux;

	/* X-surf ethernet card */
	if (zap->manid == 4626 && zap->prodid == 23) {
			return (1);
	}

	return (0);
}

/*
 * Install interface into kernel networking data structures.
 */
void
xsurf_attach(device_t parent, device_t self, void *aux)
{
	struct xsurf_softc *sc;
	struct xsurfbus_attach_args xaa_ne;
	struct xsurfbus_attach_args xaa_gencp1;
	struct xsurfbus_attach_args xaa_gencp2;
	struct xsurfbus_attach_args xaa_wdc;

	struct zbus_args *zap = aux;

	sc = device_private(self);
	sc->sc_dev = self;

	aprint_normal(": Individual Computers X-Surf\n");

	/* Add clockport. */
	xaa_gencp1.xaa_base = (bus_addr_t)zap->va + XSURF_CP2_BASE;
	strcpy(xaa_gencp1.xaa_name, "gencp_xsurf");
	config_found_ia(sc->sc_dev, "xsurfbus", &xaa_gencp1, xsurf_print);

	/* Now... if we are a fake X-Surf that's enough. */
	if (zap->serno == DENEB_XSURF_SERNO) {
		aprint_naive_dev(sc->sc_dev, "fake X-Surf on E3B Deneb");
		return;
	}

	/* Otherwise add one more clockport and continue... */
	xaa_gencp2.xaa_base = (bus_addr_t)zap->va + XSURF_CP1_BASE;
	strcpy(xaa_gencp2.xaa_name, "gencp_xsurf");
	config_found_ia(sc->sc_dev, "xsurfbus", &xaa_gencp2, xsurf_print);
		
	/* Add ne(4). */
	xaa_ne.xaa_base = (bus_addr_t)zap->va + XSURF_NE_OFFSET;
	strcpy(xaa_ne.xaa_name, "ne_xsurf");
	config_found_ia(sc->sc_dev, "xsurfbus", &xaa_ne, xsurf_print);

	/* Add wdc(4). */
	xaa_wdc.xaa_base = (bus_addr_t)zap->va + XSURF_WDC_OFFSET;
	strcpy(xaa_wdc.xaa_name, "wdc_xsurf");
	config_found_ia(sc->sc_dev, "xsurfbus", &xaa_wdc, xsurf_print);

}

static int
xsurf_print(void *aux, const char *w)
{
	if (w == NULL)
		return 0;

	return 0;
}

