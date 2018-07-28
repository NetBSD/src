/* $NetBSD: fdt_reset.c,v 1.1.20.1 2018/07/28 04:37:44 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fdt_reset.c,v 1.1.20.1 2018/07/28 04:37:44 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/queue.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

struct fdtbus_reset_controller {
	device_t rc_dev;
	int rc_phandle;
	const struct fdtbus_reset_controller_func *rc_funcs;

	LIST_ENTRY(fdtbus_reset_controller) rc_next;
};

static LIST_HEAD(, fdtbus_reset_controller) fdtbus_reset_controllers =
    LIST_HEAD_INITIALIZER(fdtbus_reset_controllers);

int
fdtbus_register_reset_controller(device_t dev, int phandle,
    const struct fdtbus_reset_controller_func *funcs)
{
	struct fdtbus_reset_controller *rc;

	rc = kmem_alloc(sizeof(*rc), KM_SLEEP);
	rc->rc_dev = dev;
	rc->rc_phandle = phandle;
	rc->rc_funcs = funcs;

	LIST_INSERT_HEAD(&fdtbus_reset_controllers, rc, rc_next);

	return 0;
}

static struct fdtbus_reset_controller *
fdtbus_get_reset_controller(int phandle)
{
	struct fdtbus_reset_controller *rc;

	LIST_FOREACH(rc, &fdtbus_reset_controllers, rc_next) {
		if (rc->rc_phandle == phandle)
			return rc;
	}

	return NULL;
}

struct fdtbus_reset *
fdtbus_reset_get_index(int phandle, u_int index)
{
	struct fdtbus_reset_controller *rc;
	struct fdtbus_reset *rst = NULL;
	void *rst_priv = NULL;
	uint32_t *resets = NULL;
	uint32_t *p;
	u_int n, reset_cells;
	int len, resid;

	len = OF_getproplen(phandle, "resets");
	if (len <= 0)
		return NULL;

	resets = kmem_alloc(len, KM_SLEEP);
	if (OF_getprop(phandle, "resets", resets, len) != len) {
		kmem_free(resets, len);
		return NULL;
	}

	p = resets;
	for (n = 0, resid = len; resid > 0; n++) {
		const int rc_phandle =
		    fdtbus_get_phandle_from_native(be32toh(p[0]));
		if (of_getprop_uint32(rc_phandle, "#reset-cells", &reset_cells))
			break;
		if (n == index) {
			rc = fdtbus_get_reset_controller(rc_phandle);
			if (rc == NULL)
				goto done;
			rst_priv = rc->rc_funcs->acquire(rc->rc_dev,
			    reset_cells > 0 ? &p[1] : NULL, reset_cells * 4);
			if (rst_priv) {
				rst = kmem_alloc(sizeof(*rst), KM_SLEEP);
				rst->rst_rc = rc;
				rst->rst_priv = rst_priv;
			}
			break;
		}
		resid -= (reset_cells + 1) * 4;
		p += reset_cells + 1;
	}

done:
	if (resets)
		kmem_free(resets, len);

	return rst;
}

struct fdtbus_reset *
fdtbus_reset_get(int phandle, const char *rstname)
{
	struct fdtbus_reset *rst = NULL;
	char *reset_names = NULL;
	const char *p;
	u_int index;
	int len, resid;

	len = OF_getproplen(phandle, "reset-names");
	if (len <= 0)
		return NULL;

	reset_names = kmem_alloc(len, KM_SLEEP);
	if (OF_getprop(phandle, "reset-names", reset_names, len) != len) {
		kmem_free(reset_names, len);
		return NULL;
	}

	p = reset_names;
	for (index = 0, resid = len; resid > 0; index++) {
		if (strcmp(p, rstname) == 0) {
			rst = fdtbus_reset_get_index(phandle, index);
			break;
		}
		resid -= strlen(p);
		p += strlen(p) + 1;
	}

	if (reset_names)
		kmem_free(reset_names, len);

	return rst;
}

void
fdtbus_reset_put(struct fdtbus_reset *rst)
{
	struct fdtbus_reset_controller *rc = rst->rst_rc;

	rc->rc_funcs->release(rc->rc_dev, rst->rst_priv);
	kmem_free(rst, sizeof(*rst));
}

int
fdtbus_reset_assert(struct fdtbus_reset *rst)
{
	struct fdtbus_reset_controller *rc = rst->rst_rc;

	return rc->rc_funcs->reset_assert(rc->rc_dev, rst->rst_priv);
}

int
fdtbus_reset_deassert(struct fdtbus_reset *rst)
{
	struct fdtbus_reset_controller *rc = rst->rst_rc;

	return rc->rc_funcs->reset_deassert(rc->rc_dev, rst->rst_priv);
}
