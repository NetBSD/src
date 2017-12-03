/* $NetBSD: fdt_dma.c,v 1.1.12.2 2017/12/03 11:37:01 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: fdt_dma.c,v 1.1.12.2 2017/12/03 11:37:01 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

struct fdtbus_dma_controller {
	device_t dma_dev;
	int dma_phandle;
	const struct fdtbus_dma_controller_func *dma_funcs;

	struct fdtbus_dma_controller *dma_next;
};

static struct fdtbus_dma_controller *fdtbus_dma = NULL;

int
fdtbus_register_dma_controller(device_t dev, int phandle,
    const struct fdtbus_dma_controller_func *funcs)
{
	struct fdtbus_dma_controller *dma;

	dma = kmem_alloc(sizeof(*dma), KM_SLEEP);
	dma->dma_dev = dev;
	dma->dma_phandle = phandle;
	dma->dma_funcs = funcs;

	dma->dma_next = fdtbus_dma;
	fdtbus_dma = dma;

	return 0;
}

static struct fdtbus_dma_controller *
fdtbus_get_dma_controller(int phandle)
{
	struct fdtbus_dma_controller *dma;

	for (dma = fdtbus_dma; dma; dma = dma->dma_next) {
		if (dma->dma_phandle == phandle) {
			return dma;
		}
	}

	return NULL;
}

struct fdtbus_dma *
fdtbus_dma_get_index(int phandle, u_int index, void (*cb)(void *), void *cbarg)
{
	struct fdtbus_dma_controller *dc;
	struct fdtbus_dma *dma = NULL;
	void *dma_priv = NULL;
	uint32_t *dmas = NULL;
	uint32_t *p;
	u_int n, dma_cells;
	int len, resid;

	len = OF_getproplen(phandle, "dmas");
	if (len <= 0)
		return NULL;

	dmas = kmem_alloc(len, KM_SLEEP);
	if (OF_getprop(phandle, "dmas", dmas, len) != len) {
		kmem_free(dmas, len);
		return NULL;
	}

	p = dmas;
	for (n = 0, resid = len; resid > 0; n++) {
		const int dc_phandle =
		    fdtbus_get_phandle_from_native(be32toh(p[0]));
		if (of_getprop_uint32(dc_phandle, "#dma-cells", &dma_cells))
			break;
		if (n == index) {
			dc = fdtbus_get_dma_controller(dc_phandle);
			if (dc == NULL)
				goto done;
			dma_priv = dc->dma_funcs->acquire(dc->dma_dev,
			    dma_cells > 0 ? &p[1] : NULL, dma_cells * 4,
			    cb, cbarg);
			if (dma_priv) {
				dma = kmem_alloc(sizeof(*dma), KM_SLEEP);
				dma->dma_dc = dc;
				dma->dma_priv = dma_priv;
			}
			break;
		}
		resid -= (dma_cells + 1) * 4;
		p += dma_cells + 1;
	}

done:
	if (dmas)
		kmem_free(dmas, len);

	return dma;
}

struct fdtbus_dma *
fdtbus_dma_get(int phandle, const char *name, void (*cb)(void *), void *cbarg)
{
	struct fdtbus_dma *dma = NULL;
	char *dma_names = NULL;
	const char *p;
	u_int index;
	int len, resid;

	len = OF_getproplen(phandle, "dma-names");
	if (len <= 0)
		return NULL;

	dma_names = kmem_alloc(len, KM_SLEEP);
	if (OF_getprop(phandle, "dma-names", dma_names, len) != len) {
		kmem_free(dma_names, len);
		return NULL;
	}

	p = dma_names;
	for (index = 0, resid = len; resid > 0; index++) {
		if (strcmp(p, name) == 0) {
			dma = fdtbus_dma_get_index(phandle, index, cb, cbarg);
			break;
		}
		resid -= strlen(p);
		p += strlen(p) + 1;
	}

	if (dma_names)
		kmem_free(dma_names, len);

	return dma;
}

void
fdtbus_dma_put(struct fdtbus_dma *dma)
{
	struct fdtbus_dma_controller *dc = dma->dma_dc;

	dc->dma_funcs->release(dc->dma_dev, dma->dma_priv);
	kmem_free(dma, sizeof(*dma));
}

int
fdtbus_dma_transfer(struct fdtbus_dma *dma, struct fdtbus_dma_req *req)
{
	struct fdtbus_dma_controller *dc = dma->dma_dc;

	return dc->dma_funcs->transfer(dc->dma_dev, dma->dma_priv, req);
}

void
fdtbus_dma_halt(struct fdtbus_dma *dma)
{
	struct fdtbus_dma_controller *dc = dma->dma_dc;

	return dc->dma_funcs->halt(dc->dma_dev, dma->dma_priv);
}
