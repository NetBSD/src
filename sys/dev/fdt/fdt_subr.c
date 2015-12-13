/* $NetBSD: fdt_subr.c,v 1.1 2015/12/13 17:30:40 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fdt_subr.c,v 1.1 2015/12/13 17:30:40 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <libfdt.h>
#include <dev/ofw/openfirm.h>
#include <dev/fdt/fdt_openfirm.h>
#include <dev/fdt/fdtvar.h>

struct fdtbus_interrupt_controller {
	device_t ic_dev;
	int ic_phandle;
	const struct fdtbus_interrupt_controller_func *ic_funcs;

	struct fdtbus_interrupt_controller *ic_next;
};

static struct fdtbus_interrupt_controller *fdtbus_ic = NULL;

struct fdtbus_i2c_controller {
	device_t i2c_dev;
	int i2c_phandle;
	const struct fdtbus_i2c_controller_func *i2c_funcs;

	struct fdtbus_i2c_controller *i2c_next;
};

static struct fdtbus_i2c_controller *fdtbus_i2c = NULL;

struct fdtbus_gpio_controller {
	device_t gc_dev;
	int gc_phandle;
	const struct fdtbus_gpio_controller_func *gc_funcs;

	struct fdtbus_gpio_controller *gc_next;
};

static struct fdtbus_gpio_controller *fdtbus_gc = NULL;

struct fdtbus_regulator_controller {
	device_t rc_dev;
	int rc_phandle;
	const struct fdtbus_regulator_controller_func *rc_funcs;

	struct fdtbus_regulator_controller *rc_next;
};

static struct fdtbus_regulator_controller *fdtbus_rc = NULL;

static int
fdtbus_get_addr_cells(int phandle)
{
	int val, addr_cells, error;

	const int parent = OF_parent(phandle);
	if (parent == -1)
		return -1;

	error = OF_getprop(parent, "#address-cells", &val, sizeof(val));
	if (error <= 0) {
		addr_cells = 2;
	} else {
		addr_cells = fdt32_to_cpu(val);
	}

	return addr_cells;
}

static int
fdtbus_get_size_cells(int phandle)
{
	int val, size_cells, error;

	const int parent = OF_parent(phandle);
	if (parent == -1)
		return -1;

	error = OF_getprop(parent, "#size-cells", &val, sizeof(val));
	if (error <= 0) {
		size_cells = 0;
	} else {
		size_cells = fdt32_to_cpu(val);
	}

	return size_cells;
}

static int
fdtbus_get_interrupt_parent(int phandle)
{
	u_int interrupt_parent;
	int len;

	while (phandle >= 0) {
		len = OF_getprop(phandle, "interrupt-parent",
		    &interrupt_parent, sizeof(interrupt_parent));
		if (len == sizeof(interrupt_parent)) {
			break;
		}
		if (phandle == 0) {
			return -1;
		}
		phandle = OF_parent(phandle);
	}
	if (phandle < 0) {
		return -1;
	}

	interrupt_parent = fdt32_to_cpu(interrupt_parent);

	const void *data = fdt_openfirm_get_data();
	const int off = fdt_node_offset_by_phandle(data, interrupt_parent);
	if (off < 0) {
		return -1;
	}

	return fdt_openfirm_get_phandle(off);
}

static struct fdtbus_interrupt_controller *
fdtbus_get_interrupt_controller(int phandle)
{
	struct fdtbus_interrupt_controller *ic;

	const int ic_phandle = fdtbus_get_interrupt_parent(phandle);
	if (ic_phandle < 0) {
		return NULL;
	}

	for (ic = fdtbus_ic; ic; ic = ic->ic_next) {
		if (ic->ic_phandle == ic_phandle) {
			return ic;
		}
	}

	return NULL;
}

int
fdtbus_register_interrupt_controller(device_t dev, int phandle,
    const struct fdtbus_interrupt_controller_func *funcs)
{
	struct fdtbus_interrupt_controller *ic;

	ic = kmem_alloc(sizeof(*ic), KM_SLEEP);
	ic->ic_dev = dev;
	ic->ic_phandle = phandle;
	ic->ic_funcs = funcs;

	ic->ic_next = fdtbus_ic;
	fdtbus_ic = ic;

	return 0;
}

void *
fdtbus_intr_establish(int phandle, u_int index, int ipl, int flags,
    int (*func)(void *), void *arg)
{
	struct fdtbus_interrupt_controller *ic;

	ic = fdtbus_get_interrupt_controller(phandle);
	if (ic == NULL)
		return NULL;

	return ic->ic_funcs->establish(ic->ic_dev, phandle, index, ipl,
	    flags, func, arg);
}

void
fdtbus_intr_disestablish(int phandle, void *ih)
{
	struct fdtbus_interrupt_controller *ic;

	ic = fdtbus_get_interrupt_controller(phandle);
	KASSERT(ic != NULL);

	return ic->ic_funcs->disestablish(ic->ic_dev, ih);
}

bool
fdtbus_intr_str(int phandle, u_int index, char *buf, size_t buflen)
{
	struct fdtbus_interrupt_controller *ic;

	ic = fdtbus_get_interrupt_controller(phandle);
	if (ic == NULL)
		return false;

	return ic->ic_funcs->intrstr(ic->ic_dev, phandle, index, buf, buflen);
}

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

	return gc->gc_funcs->read(gc->gc_dev, gp->gp_priv);
}

void
fdtbus_gpio_write(struct fdtbus_gpio_pin *gp, int val)
{
	struct fdtbus_gpio_controller *gc = gp->gp_gc;

	gc->gc_funcs->write(gc->gc_dev, gp->gp_priv, val);
}

int
fdtbus_register_regulator_controller(device_t dev, int phandle,
    const struct fdtbus_regulator_controller_func *funcs)
{
	struct fdtbus_regulator_controller *rc;

	rc = kmem_alloc(sizeof(*rc), KM_SLEEP);
	rc->rc_dev = dev;
	rc->rc_phandle = phandle;
	rc->rc_funcs = funcs;

	rc->rc_next = fdtbus_rc;
	fdtbus_rc = rc;

	return 0;
}

static struct fdtbus_regulator_controller *
fdtbus_get_regulator_controller(int phandle)
{
	struct fdtbus_regulator_controller *rc;

	for (rc = fdtbus_rc; rc; rc = rc->rc_next) {
		if (rc->rc_phandle == phandle) {
			return rc;
		}
	}

	return NULL;
}

struct fdtbus_regulator *
fdtbus_regulator_acquire(int phandle, const char *prop)
{
	struct fdtbus_regulator_controller *rc;
	struct fdtbus_regulator *reg;
	int regulator_phandle;
	int error;

	regulator_phandle = fdtbus_get_phandle(phandle, prop);
	if (regulator_phandle == -1) {
		return NULL;
	}

	rc = fdtbus_get_regulator_controller(regulator_phandle);
	if (rc == NULL) {
		return NULL;
	}

	error = rc->rc_funcs->acquire(rc->rc_dev);
	if (error) {
		return NULL;
	}

	reg = kmem_alloc(sizeof(*reg), KM_SLEEP);
	reg->reg_rc = rc;

	return reg;
}

void
fdtbus_regulator_release(struct fdtbus_regulator *reg)
{
	struct fdtbus_regulator_controller *rc = reg->reg_rc;

	rc->rc_funcs->release(rc->rc_dev);

	kmem_free(reg, sizeof(*reg));
}

int
fdtbus_regulator_enable(struct fdtbus_regulator *reg)
{
	struct fdtbus_regulator_controller *rc = reg->reg_rc;

	return rc->rc_funcs->enable(rc->rc_dev, true);
}

int
fdtbus_regulator_disable(struct fdtbus_regulator *reg)
{
	struct fdtbus_regulator_controller *rc = reg->reg_rc;

	return rc->rc_funcs->enable(rc->rc_dev, false);
}

int
fdtbus_register_i2c_controller(device_t dev, int phandle,
    const struct fdtbus_i2c_controller_func *funcs)
{
	struct fdtbus_i2c_controller *i2c;

	i2c = kmem_alloc(sizeof(*i2c), KM_SLEEP);
	i2c->i2c_dev = dev;
	i2c->i2c_phandle = phandle;
	i2c->i2c_funcs = funcs;

	i2c->i2c_next = fdtbus_i2c;
	fdtbus_i2c = i2c;

	return 0;
}

static struct fdtbus_i2c_controller *
fdtbus_get_i2c_controller(int phandle)
{
	struct fdtbus_i2c_controller *i2c;

	for (i2c = fdtbus_i2c; i2c; i2c = i2c->i2c_next) {
		if (i2c->i2c_phandle == phandle) {
			return i2c;
		}
	}

	return NULL;
}

i2c_tag_t
fdtbus_get_i2c_tag(int phandle)
{
	struct fdtbus_i2c_controller *i2c;

	i2c = fdtbus_get_i2c_controller(phandle);
	if (i2c == NULL)
		return NULL;

	return i2c->i2c_funcs->get_tag(i2c->i2c_dev);
}

int
fdtbus_get_phandle(int phandle, const char *prop)
{
	u_int phandle_ref;
	u_int *buf;
	int len;

	len = OF_getproplen(phandle, prop);
	if (len < sizeof(phandle_ref))
		return -1;

	buf = kmem_alloc(len, KM_SLEEP);

	if (OF_getprop(phandle, prop, buf, len) != len) {
		kmem_free(buf, len);
		return -1;
	}

	phandle_ref = fdt32_to_cpu(buf[0]);
	kmem_free(buf, len);

	const void *data = fdt_openfirm_get_data();
	const int off = fdt_node_offset_by_phandle(data, phandle_ref);
	if (off < 0) {
		return -1;
	}

	return fdt_openfirm_get_phandle(off);
}

int
fdtbus_get_reg(int phandle, u_int index, bus_addr_t *paddr, bus_size_t *psize)
{
	bus_addr_t addr;
	bus_size_t size;
	uint8_t *buf;
	int len;

	const int addr_cells = fdtbus_get_addr_cells(phandle);
	const int size_cells = fdtbus_get_size_cells(phandle);
	if (addr_cells == -1 || size_cells == -1)
		return -1;

	const u_int reglen = size_cells * 4 + addr_cells * 4;
	if (reglen == 0)
		return -1;

	len = OF_getproplen(phandle, "reg");
	if (len <= 0)
		return -1;

	const u_int nregs = len / reglen;

	if (index >= nregs)
		return -1;

	buf = kmem_alloc(len, KM_SLEEP);
	if (buf == NULL)
		return -1;

	len = OF_getprop(phandle, "reg", buf, len);

	switch (addr_cells) {
	case 0:
		addr = 0;
		break;
	case 1:
		addr = be32dec(&buf[index * reglen + 0]);
		break;
	case 2:
		addr = be64dec(&buf[index * reglen + 0]);
		break;
	default:
		panic("fdtbus_get_reg: unsupported addr_cells %d", addr_cells);
	}

	switch (size_cells) {
	case 0:
		size = 0;
		break;
	case 1:
		size = be32dec(&buf[index * reglen + addr_cells * 4]);
		break;
	case 2:
		size = be64dec(&buf[index * reglen + addr_cells * 4]);
		break;
	default:
		panic("fdtbus_get_reg: unsupported size_cells %d", size_cells);
	}

	if (paddr)
		*paddr = addr;
	if (psize)
		*psize = size;

	kmem_free(buf, len);

	return 0;
}
