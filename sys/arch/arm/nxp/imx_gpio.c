/*	$NetBSD: imx_gpio.c,v 1.4 2021/01/27 03:10:20 thorpej Exp $	*/
/*-
 * Copyright (c) 2019 Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
__KERNEL_RCSID(0, "$NetBSD: imx_gpio.c,v 1.4 2021/01/27 03:10:20 thorpej Exp $");

#include "opt_fdt.h"
#include "gpio.h"

#define	_INTR_PRIVATE

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <dev/gpio/gpiovar.h>

#include <arm/pic/picvar.h>
#include <arm/nxp/imx6_reg.h>

#include <arm/imx/imxgpioreg.h>
#include <arm/imx/imxgpiovar.h>

#include <dev/fdt/fdtvar.h>

static void *imx6_gpio_fdt_acquire(device_t, const void *, size_t, int);
static void imx6_gpio_fdt_release(device_t, void *);
static int imx6_gpio_fdt_read(device_t, void *, bool);
static void imx6_gpio_fdt_write(device_t, void *, int, bool);

static void *imxgpio_establish(device_t, u_int *, int, int,
    int (*)(void *), void *, const char *);
static void imxgpio_disestablish(device_t, void *);
static bool imxgpio_intrstr(device_t, u_int *, char *, size_t);

static struct fdtbus_interrupt_controller_func imxgpio_funcs = {
	.establish = imxgpio_establish,
	.disestablish = imxgpio_disestablish,
	.intrstr = imxgpio_intrstr
};

static struct fdtbus_gpio_controller_func imx6_gpio_funcs = {
	.acquire = imx6_gpio_fdt_acquire,
	.release = imx6_gpio_fdt_release,
	.read = imx6_gpio_fdt_read,
	.write = imx6_gpio_fdt_write
};

const int imxgpio_ngroups = GPIO_NGROUPS;

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx35-gpio" },
	DEVICE_COMPAT_EOL
};

int
imxgpio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

void
imxgpio_attach(device_t parent, device_t self, void *aux)
{
	struct imxgpio_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	char intrstr[128];
	const int phandle = faa->faa_phandle;
	bus_space_handle_t ioh;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	error = bus_space_map(faa->faa_bst, addr, size, 0, &ioh);
	if (error) {
		aprint_error(": couldn't map %#" PRIxBUSADDR ": %d", addr, error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": GPIO (%s)\n", fdtbus_get_string(phandle, "name"));

	sc->gpio_memt = faa->faa_bst;
	sc->gpio_memh = ioh;
	sc->gpio_unit = -1;
	sc->gpio_irqbase = PIC_IRQBASE_ALLOC;

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}
	sc->gpio_is = fdtbus_intr_establish_xname(phandle, 0, IPL_HIGH, 0,
	    pic_handle_intr, &sc->gpio_pic, device_xname(self));
	if (sc->gpio_is == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	if (!fdtbus_intr_str(phandle, 1, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}
	sc->gpio_is_high = fdtbus_intr_establish_xname(phandle, 1, IPL_HIGH, 0,
	    pic_handle_intr, &sc->gpio_pic, device_xname(self));
	if (sc->gpio_is_high == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	fdtbus_register_gpio_controller(self, phandle, &imx6_gpio_funcs);

	error = fdtbus_register_interrupt_controller(self, phandle,
	    &imxgpio_funcs);
	if (error) {
		aprint_error(": couldn't register with fdtbus: %d\n", error);
		return;
	}

	imxgpio_attach_common(self);
}

static void *
imx6_gpio_fdt_acquire(device_t dev, const void *data, size_t len, int flags)
{
	struct imxgpio_softc * const sc = device_private(dev);
	struct imxgpio_pin *gpin;
	const u_int *gpio = data;

	if (len != 12)
		return NULL;

	const u_int pin = be32toh(gpio[1]);
	const bool actlo = be32toh(gpio[2]) & 1;

	gpin = kmem_zalloc(sizeof(*gpin), KM_SLEEP);
	gpin->pin_no = pin;
	gpin->pin_flags = flags;
	gpin->pin_actlo = actlo;

	imxgpio_pin_ctl(sc, gpin->pin_no, gpin->pin_flags);

	return gpin;
}

static void
imx6_gpio_fdt_release(device_t dev, void *priv)
{
	struct imxgpio_softc * const sc = device_private(dev);
	struct imxgpio_pin *gpin = priv;

	imxgpio_pin_ctl(sc, gpin->pin_no, GPIO_PIN_INPUT);
	kmem_free(gpin, sizeof(*gpin));
}

static int
imx6_gpio_fdt_read(device_t dev, void *priv, bool raw)
{
	struct imxgpio_softc * const sc = device_private(dev);
	struct imxgpio_pin *gpin = priv;
	int val;

	val = imxgpio_pin_read(sc, gpin->pin_no);

	if (!raw && gpin->pin_actlo)
		val = !val;

	return val;
}

static void
imx6_gpio_fdt_write(device_t dev, void *priv, int val, bool raw)
{
	struct imxgpio_softc * const sc = device_private(dev);
	struct imxgpio_pin *gpin = priv;

	if (!raw && gpin->pin_actlo)
		val = !val;

	imxgpio_pin_write(sc, gpin->pin_no, val);
}

static void *
imxgpio_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg, const char *xname)
{
	struct imxgpio_softc * const sc = device_private(dev);

	/* 1st cell is the interrupt number */
	/* 2nd cell is flags */

	const u_int intr = be32toh(specifier[0]);
	const u_int trig = be32toh(specifier[1]) & 0xf;
	u_int level;

	if ((trig & 0x1) && (trig & 0x2))
		level = IST_EDGE_BOTH;
	else if (trig & 0x1)
		level = IST_EDGE_RISING;
	else if (trig & 0x2)
		level = IST_EDGE_FALLING;
	else if (trig & 0x4)
		level = IST_LEVEL_HIGH;
	else
		level = IST_LEVEL_LOW;

	const u_int mpsafe = (flags & FDT_INTR_MPSAFE) ? IST_MPSAFE : 0;

	aprint_debug_dev(dev, "intr establish irq %d, level %d\n",
	    sc->gpio_irqbase + intr, level);
	return intr_establish_xname(sc->gpio_irqbase + intr, ipl,
	    level | mpsafe, func, arg, xname);
}

static void
imxgpio_disestablish(device_t dev, void *ih)
{
	intr_disestablish(ih);
}

static bool
imxgpio_intrstr(device_t dev, u_int *specifier, char *buf, size_t buflen)
{
	struct imxgpio_softc * const sc = device_private(dev);

	/* 1st cell is the interrupt number */
	/* 2nd cell is flags */

	if (!specifier)
		return false;

	const u_int intr = be32toh(specifier[0]);

	snprintf(buf, buflen, "irq %d (gpio%d %d)",
	    sc->gpio_irqbase + intr, sc->gpio_unit, intr);

	return true;
}
