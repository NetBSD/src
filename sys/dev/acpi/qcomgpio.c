/* $NetBSD: qcomgpio.c,v 1.1 2024/12/08 20:49:14 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: qcomgpio.c,v 1.1 2024/12/08 20:49:14 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/gpio.h>
#include <sys/queue.h>
#include <sys/kmem.h>
#include <sys/mutex.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>
#include <dev/acpi/acpi_event.h>
#include <dev/acpi/acpi_gpio.h>
#include <dev/acpi/qcomgpioreg.h>

#include <dev/gpio/gpiovar.h>

typedef enum {
	QCOMGPIO_X1E,
} qcomgpio_type;

struct qcomgpio_config {
	u_int	num_pins;
	int	(*translate)(ACPI_INTEGER);
};

struct qcomgpio_intr_handler {
	int	(*ih_func)(void *);
	void	*ih_arg;
	int	ih_pin;
	LIST_ENTRY(qcomgpio_intr_handler) ih_list;
};

struct qcomgpio_softc {
	device_t			sc_dev;
	device_t			sc_gpiodev;
	bus_space_handle_t		sc_bsh;
	bus_space_tag_t			sc_bst;
	const struct qcomgpio_config	*sc_config;
	struct gpio_chipset_tag		sc_gc;
	gpio_pin_t			*sc_pins;
	LIST_HEAD(, qcomgpio_intr_handler) sc_intrs;
	kmutex_t			sc_lock;
};

#define RD4(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define WR4(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	qcomgpio_match(device_t, cfdata_t, void *);
static void	qcomgpio_attach(device_t, device_t, void *);

static int	qcomgpio_pin_read(void *, int);
static void	qcomgpio_pin_write(void *, int, int);
static void	qcomgpio_pin_ctl(void *, int, int);
static void *	qcomgpio_intr_establish(void *, int, int, int,
					int (*)(void *), void *);
static void	qcomgpio_intr_disestablish(void *, void *);
static bool	qcomgpio_intr_str(void *, int, int, char *, size_t);
static void	qcomgpio_intr_mask(void *, void *);
static void	qcomgpio_intr_unmask(void *, void *);

static int	qcomgpio_acpi_translate(void *, ACPI_INTEGER, void **);
static void	qcomgpio_register_event(void *, struct acpi_event *,
					ACPI_RESOURCE_GPIO *);
static int	qcomgpio_intr(void *);

CFATTACH_DECL_NEW(qcomgpio, sizeof(struct qcomgpio_softc),
    qcomgpio_match, qcomgpio_attach, NULL, NULL);

static int
qcomgpio_x1e_translate(ACPI_INTEGER val)
{
	switch (val) {
	case 0x33:
		return 51;
	case 0x180:
		return 67;
	case 0x380:
		return 3;
	default:
		return -1;
	}
}

static struct qcomgpio_config qcomgpio_x1e_config = {
	.num_pins = 239,
	.translate = qcomgpio_x1e_translate,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "QCOM0C0C",	.data = &qcomgpio_x1e_config },
	DEVICE_COMPAT_EOL
};

static int
qcomgpio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compat_data);
}

static void
qcomgpio_attach(device_t parent, device_t self, void *aux)
{
	struct qcomgpio_softc * const sc = device_private(self);
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
		/* It's not safe to read all pins, so leave pin state unknown */
		sc->sc_pins[pin].pin_state = 0;
	}

	sc->sc_gc.gp_cookie = sc;
	sc->sc_gc.gp_pin_read = qcomgpio_pin_read;
	sc->sc_gc.gp_pin_write = qcomgpio_pin_write;
	sc->sc_gc.gp_pin_ctl = qcomgpio_pin_ctl;
	sc->sc_gc.gp_intr_establish = qcomgpio_intr_establish;
	sc->sc_gc.gp_intr_disestablish = qcomgpio_intr_disestablish;
	sc->sc_gc.gp_intr_str = qcomgpio_intr_str;
	sc->sc_gc.gp_intr_mask = qcomgpio_intr_mask;
	sc->sc_gc.gp_intr_unmask = qcomgpio_intr_unmask;

	rv = acpi_event_create_gpio(self, hdl, qcomgpio_register_event, sc);
	if (ACPI_FAILURE(rv)) {
		if (rv != AE_NOT_FOUND) {
			aprint_error_dev(self, "failed to create events: %s\n",
			    AcpiFormatException(rv));
		}
		goto done;
	}

	ih = acpi_intr_establish(self, (uint64_t)(uintptr_t)hdl,
	    IPL_VM, false, qcomgpio_intr, sc, device_xname(self));
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
		    qcomgpio_acpi_translate, sc); 
	}

done:
	acpi_resource_cleanup(&res);
}

static int
qcomgpio_acpi_translate(void *priv, ACPI_INTEGER pin, void **gpiop)
{
	struct qcomgpio_softc * const sc = priv;
	int xpin;

	xpin = sc->sc_config->translate(pin);

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
qcomgpio_acpi_event(void *priv)
{
	struct acpi_event * const ev = priv;

	acpi_event_notify(ev);

	return 1;
}

static void
qcomgpio_register_event(void *priv, struct acpi_event *ev,
    ACPI_RESOURCE_GPIO *gpio)
{
	struct qcomgpio_softc * const sc = priv;
	int irqmode;
	void *ih;

	const int pin = qcomgpio_acpi_translate(sc, gpio->PinTable[0], NULL);

	if (pin < 0) {
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

	ih = qcomgpio_intr_establish(sc, pin, IPL_VM, irqmode,
	    qcomgpio_acpi_event, ev);
	if (ih == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't register event for pin %#x\n",
		    gpio->PinTable[0]);
	}
}

static int
qcomgpio_pin_read(void *priv, int pin)
{
	struct qcomgpio_softc * const sc = priv;
	uint32_t val;

	if (pin < 0 || pin >= sc->sc_config->num_pins) {
		return 0;
	}

	val = RD4(sc, TLMM_GPIO_IN_OUT(pin));
	return (val & TLMM_GPIO_IN_OUT_GPIO_IN) != 0;
}

static void
qcomgpio_pin_write(void *priv, int pin, int pinval)
{
	struct qcomgpio_softc * const sc = priv;
	uint32_t val;

	if (pin < 0 || pin >= sc->sc_config->num_pins) {
		return;
	}

	val = RD4(sc, TLMM_GPIO_IN_OUT(pin));
	if (pinval) {
		val |= TLMM_GPIO_IN_OUT_GPIO_OUT;
	} else {
		val &= ~TLMM_GPIO_IN_OUT_GPIO_OUT;
	}
	WR4(sc, TLMM_GPIO_IN_OUT(pin), val);
}

static void
qcomgpio_pin_ctl(void *priv, int pin, int flags)
{
	/* Nothing to do here, as firmware has already configured pins. */
}

static void *
qcomgpio_intr_establish(void *priv, int pin, int ipl, int irqmode,
			int (*func)(void *), void *arg)
{
	struct qcomgpio_softc * const sc = priv;
	struct qcomgpio_intr_handler *qih, *qihp;
	uint32_t dect, pol;
	uint32_t val;

	if (pin < 0 || pin >= sc->sc_config->num_pins) {
		return NULL;
	}
	if (ipl != IPL_VM) {
		device_printf(sc->sc_dev, "%s: only IPL_VM supported\n",
		    __func__);
		return NULL;
	}

	qih = kmem_alloc(sizeof(*qih), KM_SLEEP);
	qih->ih_func = func;
	qih->ih_arg = arg;
	qih->ih_pin = pin;

	mutex_enter(&sc->sc_lock);

	LIST_FOREACH(qihp, &sc->sc_intrs, ih_list) {
		if (qihp->ih_pin == qih->ih_pin) {
			mutex_exit(&sc->sc_lock);
			kmem_free(qih, sizeof(*qih));
			device_printf(sc->sc_dev,
			    "%s: pin %d already establish\n", __func__, pin);
			return NULL;
		}
	}

	LIST_INSERT_HEAD(&sc->sc_intrs, qih, ih_list);

	if ((irqmode & GPIO_INTR_LEVEL_MASK) != 0) {
		dect = TLMM_GPIO_INTR_CFG_INTR_DECT_CTL_LEVEL;
		pol = (irqmode & GPIO_INTR_HIGH_LEVEL) != 0 ?
		    TLMM_GPIO_INTR_CFG_INTR_POL_CTL : 0;
	} else {
		KASSERT((irqmode & GPIO_INTR_EDGE_MASK) != 0);
		if ((irqmode & GPIO_INTR_NEG_EDGE) != 0) {
			dect = TLMM_GPIO_INTR_CFG_INTR_DECT_CTL_EDGE_NEG;
			pol = TLMM_GPIO_INTR_CFG_INTR_POL_CTL;
		} else if ((irqmode & GPIO_INTR_POS_EDGE) != 0) {
			dect = TLMM_GPIO_INTR_CFG_INTR_DECT_CTL_EDGE_POS;
			pol = TLMM_GPIO_INTR_CFG_INTR_POL_CTL;
		} else {
			KASSERT((irqmode & GPIO_INTR_DOUBLE_EDGE) != 0);
			dect = TLMM_GPIO_INTR_CFG_INTR_DECT_CTL_EDGE_BOTH;
			pol = 0;
		}
	}

	val = RD4(sc, TLMM_GPIO_INTR_CFG(pin));
	val &= ~TLMM_GPIO_INTR_CFG_INTR_DECT_CTL_MASK;
	val |= __SHIFTIN(dect, TLMM_GPIO_INTR_CFG_INTR_DECT_CTL_MASK);
	val &= ~TLMM_GPIO_INTR_CFG_INTR_POL_CTL;
	val |= pol;
	val &= ~TLMM_GPIO_INTR_CFG_TARGET_PROC_MASK;
	val |= __SHIFTIN(TLMM_GPIO_INTR_CFG_TARGET_PROC_RPM,
			 TLMM_GPIO_INTR_CFG_TARGET_PROC_MASK);
	val |= TLMM_GPIO_INTR_CFG_INTR_RAW_STATUS_EN;
	val |= TLMM_GPIO_INTR_CFG_INTR_ENABLE;
	WR4(sc, TLMM_GPIO_INTR_CFG(pin), val);

	mutex_exit(&sc->sc_lock);

	return qih;
}

static void
qcomgpio_intr_disestablish(void *priv, void *ih)
{
	struct qcomgpio_softc * const sc = priv;
	struct qcomgpio_intr_handler *qih = ih;
	uint32_t val;

	mutex_enter(&sc->sc_lock);

	LIST_REMOVE(qih, ih_list);

	val = RD4(sc, TLMM_GPIO_INTR_CFG(qih->ih_pin));
	val &= ~TLMM_GPIO_INTR_CFG_INTR_ENABLE;
	WR4(sc, TLMM_GPIO_INTR_CFG(qih->ih_pin), val);

	mutex_exit(&sc->sc_lock);

	kmem_free(qih, sizeof(*qih));
}

static bool
qcomgpio_intr_str(void *priv, int pin, int irqmode, char *buf, size_t buflen)
{
	struct qcomgpio_softc * const sc = priv;
	int rv;

	rv = snprintf(buf, buflen, "%s pin %d", device_xname(sc->sc_dev), pin);

	return rv < buflen;
}

static void
qcomgpio_intr_mask(void *priv, void *ih)
{
	struct qcomgpio_softc * const sc = priv;
	struct qcomgpio_intr_handler *qih = ih;
	uint32_t val;

	val = RD4(sc, TLMM_GPIO_INTR_CFG(qih->ih_pin));
	val &= ~TLMM_GPIO_INTR_CFG_INTR_ENABLE;
	WR4(sc, TLMM_GPIO_INTR_CFG(qih->ih_pin), val);
}

static void
qcomgpio_intr_unmask(void *priv, void *ih)
{
	struct qcomgpio_softc * const sc = priv;
	struct qcomgpio_intr_handler *qih = ih;
	uint32_t val;

	val = RD4(sc, TLMM_GPIO_INTR_CFG(qih->ih_pin));
	val |= TLMM_GPIO_INTR_CFG_INTR_ENABLE;
	WR4(sc, TLMM_GPIO_INTR_CFG(qih->ih_pin), val);
}

static int
qcomgpio_intr(void *priv)
{
	struct qcomgpio_softc * const sc = priv;
	struct qcomgpio_intr_handler *qih;
	int rv = 0;

	mutex_enter(&sc->sc_lock);

	LIST_FOREACH(qih, &sc->sc_intrs, ih_list) {
		const int pin = qih->ih_pin;
		uint32_t val;

		val = RD4(sc, TLMM_GPIO_INTR_STATUS(pin));
		if ((val & TLMM_GPIO_INTR_STATUS_INTR_STATUS) != 0) {
			rv |= qih->ih_func(qih->ih_arg);

			val &= ~TLMM_GPIO_INTR_STATUS_INTR_STATUS;
			WR4(sc, TLMM_GPIO_INTR_STATUS(pin), val);
		}
	}

	mutex_exit(&sc->sc_lock);

	return rv;
}
