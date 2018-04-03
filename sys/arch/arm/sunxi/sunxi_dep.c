/*	$NetBSD: sunxi_dep.c,v 1.2 2018/04/03 13:38:13 bouyer Exp $	*/

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


#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: sunxi_dep.c,v 1.2 2018/04/03 13:38:13 bouyer Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <libfdt.h>

#include <dev/fdt/fdtvar.h>
#include <arm/sunxi/sunxi_display.h>

#include "sunxi_debe.h"

struct sunxi_dep_softc {
	device_t sc_dev;
	int	sc_phandle;
};

static const struct of_compat_data compat_data[] = {
	{"allwinner,sun4i-a10-display-engine", 0},
	{"allwinner,sun7i-a20-display-engine", 0},
	{NULL}
};

static int sunxi_dep_match(device_t, cfdata_t, void *);
static void sunxi_dep_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(sunxi_dep, sizeof(struct sunxi_dep_softc),
	sunxi_dep_match, sunxi_dep_attach, NULL, NULL);

static int
sunxi_dep_match(device_t parent, cfdata_t cf, void *aux)
{
#if NSUNXI_DEBE > 0
	struct fdt_attach_args * const faa = aux;
	if (!of_match_compat_data(faa->faa_phandle, compat_data))
		return 0;
	return 1;
#else
	return 0;
#endif
}

static void
sunxi_dep_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_dep_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	int len;
	const u_int *buf;
	u_int ref;
	int error;

	sc->sc_dev = self;

	buf = fdt_getprop(fdtbus_get_data(),
	    fdtbus_phandle2offset(phandle), "allwinner,pipelines", &len);
	if (buf == NULL || len < sizeof(ref) || (len % sizeof(ref)) != 0) {
		aprint_error("bad/missing allwinner,pipelines property\n");
		return;
	}
	aprint_naive("\n");
	aprint_normal(": ");
#if NSUNXI_DEBE > 0
	for (int i = 0; i < (len / sizeof(ref)); i++) {
		if (i > 0)
			aprint_normal_dev(self, "");
		ref = be32dec(&buf[i]);
		error = sunxi_debe_pipeline(
		    fdtbus_get_phandle_from_native(ref), true);
		if (error)
			aprint_error("can't activate pipeline %d\n", i);
	}
#else
	aprint_error("debe not configured\n");
	return;
#endif
}
