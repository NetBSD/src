/*	$NetBSD: amdgpio.c,v 1.3 2025/03/02 13:47:35 riastradh Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared McNeill <jmcneill@invisible.ca>.
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
__KERNEL_RCSID(0, "$NetBSD: amdgpio.c,v 1.3 2025/03/02 13:47:35 riastradh Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/gpio.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/queue.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_event.h>
#include <dev/acpi/acpi_gpio.h>
#include <dev/acpi/acpi_intr.h>
#include <dev/acpi/amdgpioreg.h>

#include <dev/gpio/gpiovar.h>

struct amdgpio_config {
	u_int	num_pins;
	int	(*translate)(ACPI_RESOURCE_GPIO *);
};

struct amdgpio_intr_handler {
	int	(*ih_func)(void *);
	void	*ih_arg;
	int	ih_pin;
	LIST_ENTRY(amdgpio_intr_handler) ih_list;
};

struct amdgpio_softc {
	device_t			sc_dev;
	device_t			sc_gpiodev;
	bus_space_handle_t		sc_bsh;
	bus_space_tag_t			sc_bst;
	const struct amdgpio_config	*sc_config;
	struct gpio_chipset_tag		sc_gc;
	gpio_pin_t			*sc_pins;
	LIST_HEAD(, amdgpio_intr_handler) sc_intrs;
	kmutex_t			sc_lock;
};

#define RD4(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define WR4(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	amdgpio_match(device_t, cfdata_t, void *);
static void	amdgpio_attach(device_t, device_t, void *);

static int	amdgpio_pin_read(void *, int);
static void	amdgpio_pin_write(void *, int, int);
static void	amdgpio_pin_ctl(void *, int, int);
static void *	amdgpio_intr_establish(void *, int, int, int,
		    int (*)(void *), void *);
static void	amdgpio_intr_disestablish(void *, void *);
static bool	amdgpio_intr_str(void *, int, int, char *, size_t);
static void	amdgpio_intr_mask(void *, void *);
static void	amdgpio_intr_unmask(void *, void *);

static int	amdgpio_acpi_translate(void *, ACPI_RESOURCE_GPIO *, void **);
static void	amdgpio_register_event(void *, struct acpi_event *,
		    ACPI_RESOURCE_GPIO *);
static int	amdgpio_intr(void *);

CFATTACH_DECL_NEW(amdgpio, sizeof(struct amdgpio_softc),
    amdgpio_match, amdgpio_attach, NULL, NULL);

#define AMDGPIO_NUM_PINS	184

static int
amdgpio_translate(ACPI_RESOURCE_GPIO *gpio)
{
	const ACPI_INTEGER pin = gpio->PinTable[0];

	if (pin < AMDGPIO_NUM_PINS) {
		return gpio->PinTable[0];
	}

	switch (pin) {
	case 0x0:
	case 0x8: /* TPDD */
	case 0x28: /* TPNL */
	case 0x3a:
	case 0x3b:
	case 0x3d:
	case 0x3e:
		return pin;
	default:
		return -1;
	}
}

static struct amdgpio_config amdgpio_config = {
	.num_pins = AMDGPIO_NUM_PINS,
	.translate = amdgpio_translate, /* TODO: REMOVE */
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "AMDI0030",	.data = &amdgpio_config },
	DEVICE_COMPAT_EOL
};

static int
amdgpio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compat_data);
}

static void
amdgpio_attach(device_t parent, device_t self, void *aux)
{
	struct amdgpio_softc * const sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	struct gpiobus_attach_args gba;
	ACPI_HANDLE hdl = aa->aa_node->ad_handle;
	struct acpi_resources res;
	struct acpi_mem *mem;
	struct acpi_irq *irq;
	ACPI_STATUS rv;
	int error, pin;
	void *ih;

	sc->sc_dev = self;
	sc->sc_config = acpi_compatible_lookup(aa, compat_data)->data;
	sc->sc_bst = aa->aa_memt;
	KASSERT(sc->sc_config != NULL);
	LIST_INIT(&sc->sc_intrs);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);

	rv = acpi_resource_parse(sc->sc_dev, hdl, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv)) {
		return;
	}

	mem = acpi_res_mem(&res, 0);
	if (mem == NULL) {
		aprint_error_dev(self, "couldn't find mem resource\n");
		goto done;
	}

	irq = acpi_res_irq(&res, 0);
	if (irq == NULL) {
		aprint_error_dev(self, "couldn't find irq resource\n");
		goto done;
	}

	error = bus_space_map(sc->sc_bst, mem->ar_base, mem->ar_length, 0,
	    &sc->sc_bsh);
	if (error) {
		aprint_error_dev(self, "couldn't map registers\n");
		goto done;
	}

	sc->sc_pins = kmem_zalloc(sizeof(*sc->sc_pins) *
	    sc->sc_config->num_pins, KM_SLEEP);
	for (pin = 0; pin < sc->sc_config->num_pins; pin++) {
		sc->sc_pins[pin].pin_num = pin;
		sc->sc_pins[pin].pin_caps = GPIO_PIN_INPUT | GPIO_PIN_OUTPUT;
		sc->sc_pins[pin].pin_intrcaps =
		    GPIO_INTR_POS_EDGE | GPIO_INTR_NEG_EDGE |
		    GPIO_INTR_DOUBLE_EDGE | GPIO_INTR_HIGH_LEVEL |
		    GPIO_INTR_LOW_LEVEL | GPIO_INTR_MPSAFE;
		/*
		 * It's not safe to read all pins, so leave pin state
		 * unknown
		 */
		sc->sc_pins[pin].pin_state = 0;
	}

	sc->sc_gc.gp_cookie = sc;
	sc->sc_gc.gp_pin_read = amdgpio_pin_read;
	sc->sc_gc.gp_pin_write = amdgpio_pin_write;
	sc->sc_gc.gp_pin_ctl = amdgpio_pin_ctl;
	sc->sc_gc.gp_intr_establish = amdgpio_intr_establish;
	sc->sc_gc.gp_intr_disestablish = amdgpio_intr_disestablish;
	sc->sc_gc.gp_intr_str = amdgpio_intr_str;
	sc->sc_gc.gp_intr_mask = amdgpio_intr_mask;
	sc->sc_gc.gp_intr_unmask = amdgpio_intr_unmask;

	rv = acpi_event_create_gpio(self, hdl, amdgpio_register_event, sc);
	if (ACPI_FAILURE(rv)) {
		if (rv != AE_NOT_FOUND) {
			aprint_error_dev(self, "failed to create events: %s\n",
			    AcpiFormatException(rv));
		}
		goto done;
	}

	ih = acpi_intr_establish(self, (uint64_t)(uintptr_t)hdl,
	    IPL_VM, false, amdgpio_intr, sc, device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		goto done;
	}

	memset(&gba, 0, sizeof(gba));
	gba.gba_gc = &sc->sc_gc;
	gba.gba_pins = sc->sc_pins;
	gba.gba_npins = sc->sc_config->num_pins;
	sc->sc_gpiodev = config_found(self, &gba, gpiobus_print,
	    CFARGS(.iattr = "gpiobus"));
	if (sc->sc_gpiodev != NULL) {
		acpi_gpio_register(aa->aa_node, self,
		    amdgpio_acpi_translate, sc);
	}

done:
	acpi_resource_cleanup(&res);
}

static int
amdgpio_acpi_translate(void *priv, ACPI_RESOURCE_GPIO *gpio, void **gpiop)
{
	struct amdgpio_softc * const sc = priv;
	const ACPI_INTEGER pin = gpio->PinTable[0];
	int xpin;

	xpin = sc->sc_config->translate(gpio);

	aprint_debug_dev(sc->sc_dev, "translate %#lx -> %u\n", pin, xpin);

	if (gpiop != NULL) {
		if (sc->sc_gpiodev != NULL) {
			*gpiop = device_private(sc->sc_gpiodev);
		} else {
			device_printf(sc->sc_dev,
			    "no gpiodev for pin %#lx -> %u\n", pin, xpin);
			xpin = -1;
		}
	}

	return xpin;
}

static int
amdgpio_acpi_event(void *priv)
{
	struct acpi_event * const ev = priv;

	acpi_event_notify(ev);

	return 1;
}

static void
amdgpio_register_event(void *priv, struct acpi_event *ev,
    ACPI_RESOURCE_GPIO *gpio)
{
	struct amdgpio_softc * const sc = priv;
	int irqmode;
	void *ih;

	const int pin = amdgpio_acpi_translate(sc, gpio, NULL);

	if (pin < 0 || pin == 0x8) {
		aprint_error_dev(sc->sc_dev,
		    "ignoring event for pin %#x (out of range)\n",
		    gpio->PinTable[0]);
		return;
	}

	if (gpio->Triggering == ACPI_LEVEL_SENSITIVE) {
		irqmode = gpio->Polarity == ACPI_ACTIVE_HIGH ?
		    GPIO_INTR_HIGH_LEVEL : GPIO_INTR_LOW_LEVEL;
	} else {
		KASSERT(gpio->Triggering == ACPI_EDGE_SENSITIVE);
		if (gpio->Polarity == ACPI_ACTIVE_LOW) {
			irqmode = GPIO_INTR_NEG_EDGE;
		} else if (gpio->Polarity == ACPI_ACTIVE_HIGH) {
			irqmode = GPIO_INTR_POS_EDGE;
		} else {
			KASSERT(gpio->Polarity == ACPI_ACTIVE_BOTH);
			irqmode = GPIO_INTR_DOUBLE_EDGE;
		}
	}

	ih = amdgpio_intr_establish(sc, pin, IPL_VM, irqmode,
	    amdgpio_acpi_event, ev);
	if (ih == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't register event for pin %#x\n",
		    gpio->PinTable[0]);
		return;
	}
	if (gpio->Triggering == ACPI_LEVEL_SENSITIVE) {
		acpi_event_set_intrcookie(ev, ih);
	}
}

static int
amdgpio_pin_read(void *priv, int pin)
{
	struct amdgpio_softc * const sc = priv;
	uint32_t val;

	if (pin < 0 || pin >= sc->sc_config->num_pins) {
		return 0;
	}
	if ((sc->sc_pins[pin].pin_caps & GPIO_PIN_INPUT) == 0) {
		return 0;
	}

	val = RD4(sc, AMDGPIO_PIN_REG(pin));
	return (val & AMDGPIO_CONF_GPIORXSTATE) ? 1 : 0;
}

static void
amdgpio_pin_write(void *priv, int pin, int pinval)
{
	struct amdgpio_softc * const sc = priv;
	uint32_t val;

	if (pin < 0 || pin >= sc->sc_config->num_pins) {
		return;
	}
	if ((sc->sc_pins[pin].pin_caps & GPIO_PIN_OUTPUT) == 0) {
		return;
	}

	val = RD4(sc, AMDGPIO_PIN_REG(pin));
	if (pinval) {
		val |= AMDGPIO_CONF_GPIOTXSTATE;
	} else {
		val &= ~AMDGPIO_CONF_GPIOTXSTATE;
	}
	WR4(sc, AMDGPIO_PIN_REG(pin), val);
}

static void
amdgpio_pin_ctl(void *priv, int pin, int flags)
{
	/* Nothing to do here, as firmware has already configured pins. */
}

static void *
amdgpio_intr_establish(void *priv, int pin, int ipl, int irqmode,
    int (*func)(void *), void *arg)
{
	struct amdgpio_softc * const sc = priv;
	struct amdgpio_intr_handler *aih, *aihp;
	uint32_t dect;
	uint32_t val;

	if (pin < 0 || pin >= sc->sc_config->num_pins) {
		return NULL;
	}
	if (ipl != IPL_VM) {
		device_printf(sc->sc_dev, "%s: only IPL_VM supported\n",
		    __func__);
		return NULL;
	}

	aih = kmem_alloc(sizeof(*aih), KM_SLEEP);
	aih->ih_func = func;
	aih->ih_arg = arg;
	aih->ih_pin = pin;

	mutex_enter(&sc->sc_lock);

	LIST_FOREACH(aihp, &sc->sc_intrs, ih_list) {
		if (aihp->ih_pin == aih->ih_pin) {
			mutex_exit(&sc->sc_lock);
			kmem_free(aih, sizeof(*aih));
			device_printf(sc->sc_dev,
			    "%s: pin %d already establish\n", __func__, pin);
			return NULL;
		}
	}

	LIST_INSERT_HEAD(&sc->sc_intrs, aih, ih_list);

	if ((irqmode & GPIO_INTR_LEVEL_MASK) != 0) {
		dect = AMDGPIO_CONF_LEVEL;
	} else {
		KASSERT((irqmode & GPIO_INTR_EDGE_MASK) != 0);
		if ((irqmode & GPIO_INTR_NEG_EDGE) != 0) {
			dect = AMDGPIO_CONF_ACTLO;
		} else if ((irqmode & GPIO_INTR_POS_EDGE) != 0) {
			dect = 0;
		} else {
			KASSERT((irqmode & GPIO_INTR_DOUBLE_EDGE) != 0);
			dect = AMDGPIO_CONF_ACTBOTH;
		}
	}

	val = RD4(sc, AMDGPIO_PIN_REG(pin));
	val |= dect;
	val |= AMDGPIO_CONF_INTR_MASK_EN | AMDGPIO_CONF_INTR_EN;
	WR4(sc, AMDGPIO_PIN_REG(pin), val);

	mutex_exit(&sc->sc_lock);

	return aih;
}

static void
amdgpio_intr_disestablish(void *priv, void *ih)
{
	struct amdgpio_softc * const sc = priv;
	struct amdgpio_intr_handler *aih = ih;
	uint32_t val;

	mutex_enter(&sc->sc_lock);

	LIST_REMOVE(aih, ih_list);

	val = RD4(sc, AMDGPIO_PIN_REG(aih->ih_pin));
	val &= ~(AMDGPIO_CONF_INTR_EN | AMDGPIO_CONF_INTR_MASK_EN);
	WR4(sc, AMDGPIO_PIN_REG(aih->ih_pin), val);

	mutex_exit(&sc->sc_lock);

	kmem_free(aih, sizeof(*aih));
}

static bool
amdgpio_intr_str(void *priv, int pin, int irqmode, char *buf, size_t buflen)
{
	struct amdgpio_softc * const sc = priv;
	int rv;

	rv = snprintf(buf, buflen, "%s pin %d", device_xname(sc->sc_dev), pin);

	return rv < buflen;
}

static void
amdgpio_intr_mask(void *priv, void *ih)
{
	struct amdgpio_softc * const sc = priv;
	struct amdgpio_intr_handler *aih = ih;
	uint32_t val;

	val = RD4(sc, AMDGPIO_PIN_REG(aih->ih_pin));
	val &= ~AMDGPIO_CONF_INTR_MASK_EN;
	WR4(sc, AMDGPIO_PIN_REG(aih->ih_pin), val);
}

static void
amdgpio_intr_unmask(void *priv, void *ih)
{
	struct amdgpio_softc * const sc = priv;
	struct amdgpio_intr_handler *aih = ih;
	uint32_t val;

	val = RD4(sc, AMDGPIO_PIN_REG(aih->ih_pin));
	val |= AMDGPIO_CONF_INTR_MASK_EN;
	WR4(sc, AMDGPIO_PIN_REG(aih->ih_pin), val);
}

static int
amdgpio_intr(void *priv)
{
	struct amdgpio_softc * const sc = priv;
	struct amdgpio_intr_handler *aih;
	int rv = 0;
	uint64_t status;
	uint32_t val;

	mutex_enter(&sc->sc_lock);

	status = RD4(sc, AMDGPIO_INTR_STATUS(1));
	status <<= 32;
	status |= RD4(sc, AMDGPIO_INTR_STATUS(0));
	status &= __BITS(0, AMDGPIO_INTR_STATUS_NBITS - 1);

	if (status == 0) {
		rv = 1;
		goto out;
	}

	LIST_FOREACH(aih, &sc->sc_intrs, ih_list) {
		const int pin = aih->ih_pin;

		if ((status & __BIT(pin / 4)) == 0) {
			continue;
		}

		val = RD4(sc, AMDGPIO_PIN_REG(pin));
		if ((val & AMDGPIO_CONF_INTR_STATUS) != 0) {
			rv |= aih->ih_func(aih->ih_arg);

			val &= ~(AMDGPIO_CONF_INTR_MASK_EN |
			    AMDGPIO_CONF_INTR_EN);
			WR4(sc, AMDGPIO_PIN_REG(pin), val);
		}
	}

	/* Signal end of interrupt */
	val = RD4(sc, AMDGPIO_INTR_MASTER);
	val |= AMDGPIO_INTR_MASTER_EIO;
	WR4(sc, AMDGPIO_INTR_MASTER, val);

out:
	mutex_exit(&sc->sc_lock);

	return rv;
}
