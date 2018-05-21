/* $NetBSD: fdt_dai.c,v 1.1.2.2 2018/05/21 04:36:05 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: fdt_dai.c,v 1.1.2.2 2018/05/21 04:36:05 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

#include <dev/audio_dai.h>

struct fdtbus_dai_controller {
	device_t dc_dev;
	int dc_phandle;
	const struct fdtbus_dai_controller_func *dc_funcs;

	struct fdtbus_dai_controller *dc_next;
};

static struct fdtbus_dai_controller *fdtbus_dc = NULL;

int
fdtbus_register_dai_controller(device_t dev, int phandle,
    const struct fdtbus_dai_controller_func *funcs)
{
	struct fdtbus_dai_controller *dc;

	dc = kmem_alloc(sizeof(*dc), KM_SLEEP);
	dc->dc_dev = dev;
	dc->dc_phandle = phandle;
	dc->dc_funcs = funcs;

	dc->dc_next = fdtbus_dc;
	fdtbus_dc = dc;

	return 0;
}

static struct fdtbus_dai_controller *
fdtbus_get_dai_controller(int phandle)
{
	struct fdtbus_dai_controller *dc;

	for (dc = fdtbus_dc; dc; dc = dc->dc_next) {
		if (dc->dc_phandle == phandle) {
			return dc;
		}
	}

	return NULL;
}

audio_dai_tag_t
fdtbus_dai_acquire(int phandle, const char *prop)
{
	return fdtbus_dai_acquire_index(phandle, prop, 0);
}

audio_dai_tag_t
fdtbus_dai_acquire_index(int phandle, const char *prop, int index)
{
	struct fdtbus_dai_controller *dc;
	const uint32_t *dais, *p;
	u_int n, dai_cells;
	int len, resid;

	dais = fdtbus_get_prop(phandle, prop, &len);
	if (dais == NULL)
		return NULL;

	p = dais;
	for (n = 0, resid = len; resid > 0; n++) {
		const int dc_phandle =
		    fdtbus_get_phandle_from_native(be32toh(p[0]));
		if (of_getprop_uint32(dc_phandle, "#sound-dai-cells", &dai_cells))
			dai_cells = 0;
		if (n == index) {
			dc = fdtbus_get_dai_controller(dc_phandle);
			if (dc == NULL)
				return NULL;
			return dc->dc_funcs->get_tag(dc->dc_dev,
			    &p[0], (dai_cells + 1) * 4);
		}
		resid -= (dai_cells + 1) * 4;
		p += dai_cells + 1;
	}

	return NULL;
}
