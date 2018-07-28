/* $NetBSD: fdt_gpio.c,v 1.5.4.1 2018/07/28 04:37:44 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fdt_gpio.c,v 1.5.4.1 2018/07/28 04:37:44 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/queue.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

struct fdtbus_gpio_controller {
	device_t gc_dev;
	int gc_phandle;
	const struct fdtbus_gpio_controller_func *gc_funcs;

	LIST_ENTRY(fdtbus_gpio_controller) gc_next;
};

static LIST_HEAD(, fdtbus_gpio_controller) fdtbus_gpio_controllers =
    LIST_HEAD_INITIALIZER(fdtbus_gpio_controllers);

int
fdtbus_register_gpio_controller(device_t dev, int phandle,
    const struct fdtbus_gpio_controller_func *funcs)
{
	struct fdtbus_gpio_controller *gc;

	gc = kmem_alloc(sizeof(*gc), KM_SLEEP);
	gc->gc_dev = dev;
	gc->gc_phandle = phandle;
	gc->gc_funcs = funcs;

	LIST_INSERT_HEAD(&fdtbus_gpio_controllers, gc, gc_next);

	return 0;
}

static struct fdtbus_gpio_controller *
fdtbus_get_gpio_controller(int phandle)
{
	struct fdtbus_gpio_controller *gc;

	LIST_FOREACH(gc, &fdtbus_gpio_controllers, gc_next) {
		if (gc->gc_phandle == phandle)
			return gc;
	}

	return NULL;
}

struct fdtbus_gpio_pin *
fdtbus_gpio_acquire(int phandle, const char *prop, int flags)
{
	return fdtbus_gpio_acquire_index(phandle, prop, 0, flags);
}

struct fdtbus_gpio_pin *
fdtbus_gpio_acquire_index(int phandle, const char *prop,
    int index, int flags)
{
	struct fdtbus_gpio_controller *gc;
	struct fdtbus_gpio_pin *gp = NULL;
	const uint32_t *gpios, *p;
	u_int n, gpio_cells;
	int len, resid;

	gpios = fdtbus_get_prop(phandle, prop, &len);
	if (gpios == NULL)
		return NULL;

	p = gpios;
	for (n = 0, resid = len; resid > 0; n++) {
		const int gc_phandle =
		    fdtbus_get_phandle_from_native(be32toh(p[0]));
		if (of_getprop_uint32(gc_phandle, "#gpio-cells", &gpio_cells))
			break;
		if (n == index) {
			gc = fdtbus_get_gpio_controller(gc_phandle);
			if (gc == NULL)
				return NULL;
			gp = kmem_alloc(sizeof(*gp), KM_SLEEP);
			gp->gp_gc = gc;
			gp->gp_priv = gc->gc_funcs->acquire(gc->gc_dev,
			    &p[0], (gpio_cells + 1) * 4, flags);
			if (gp->gp_priv == NULL) {
				kmem_free(gp, sizeof(*gp));
				return NULL;
			}
			break;
		}
		resid -= (gpio_cells + 1) * 4;
		p += gpio_cells + 1;
	}

	return gp;
}

void
fdtbus_gpio_release(struct fdtbus_gpio_pin *gp)
{
	struct fdtbus_gpio_controller *gc = gp->gp_gc;

	gc->gc_funcs->release(gc->gc_dev, gp->gp_priv);
	kmem_free(gp, sizeof(*gp));
}

int
fdtbus_gpio_read(struct fdtbus_gpio_pin *gp)
{
	struct fdtbus_gpio_controller *gc = gp->gp_gc;

	return gc->gc_funcs->read(gc->gc_dev, gp->gp_priv, false);
}

void
fdtbus_gpio_write(struct fdtbus_gpio_pin *gp, int val)
{
	struct fdtbus_gpio_controller *gc = gp->gp_gc;

	gc->gc_funcs->write(gc->gc_dev, gp->gp_priv, val, false);
}

int
fdtbus_gpio_read_raw(struct fdtbus_gpio_pin *gp)
{
	struct fdtbus_gpio_controller *gc = gp->gp_gc;

	return gc->gc_funcs->read(gc->gc_dev, gp->gp_priv, true);
}

void
fdtbus_gpio_write_raw(struct fdtbus_gpio_pin *gp, int val)
{
	struct fdtbus_gpio_controller *gc = gp->gp_gc;

	gc->gc_funcs->write(gc->gc_dev, gp->gp_priv, val, true);
}
