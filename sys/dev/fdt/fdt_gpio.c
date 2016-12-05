/* $NetBSD: fdt_gpio.c,v 1.3.2.3 2016/12/05 10:55:01 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fdt_gpio.c,v 1.3.2.3 2016/12/05 10:55:01 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

struct fdtbus_gpio_controller {
	device_t gc_dev;
	int gc_phandle;
	const struct fdtbus_gpio_controller_func *gc_funcs;

	struct fdtbus_gpio_controller *gc_next;
};

static struct fdtbus_gpio_controller *fdtbus_gc = NULL;

int
fdtbus_register_gpio_controller(device_t dev, int phandle,
    const struct fdtbus_gpio_controller_func *funcs)
{
	struct fdtbus_gpio_controller *gc;

	gc = kmem_alloc(sizeof(*gc), KM_SLEEP);
	gc->gc_dev = dev;
	gc->gc_phandle = phandle;
	gc->gc_funcs = funcs;

	gc->gc_next = fdtbus_gc;
	fdtbus_gc = gc;

	return 0;
}

static struct fdtbus_gpio_controller *
fdtbus_get_gpio_controller(int phandle)
{
	struct fdtbus_gpio_controller *gc;

	for (gc = fdtbus_gc; gc; gc = gc->gc_next) {
		if (gc->gc_phandle == phandle) {
			return gc;
		}
	}

	return NULL;
}

struct fdtbus_gpio_pin *
fdtbus_gpio_acquire(int phandle, const char *prop, int flags)
{
	struct fdtbus_gpio_controller *gc;
	struct fdtbus_gpio_pin *gp;
	int gpio_phandle, len;
	u_int *data;

	gpio_phandle = fdtbus_get_phandle(phandle, prop);
	if (gpio_phandle == -1) {
		return NULL;
	}

	gc = fdtbus_get_gpio_controller(gpio_phandle);
	if (gc == NULL) {
		return NULL;
	}

	len = OF_getproplen(phandle, prop);
	if (len < 4) {
		return NULL;
	}

	data = kmem_alloc(len, KM_SLEEP);
	if (OF_getprop(phandle, prop, data, len) != len) {
		kmem_free(data, len);
		return NULL;
	}

	gp = kmem_alloc(sizeof(*gp), KM_SLEEP);
	gp->gp_gc = gc;
	gp->gp_priv = gc->gc_funcs->acquire(gc->gc_dev, data, len, flags);
	if (gp->gp_priv == NULL) {
		kmem_free(data, len);
		kmem_free(gp, sizeof(*gp));
		return NULL;
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
