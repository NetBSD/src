/* $NetBSD: plgpio_acpi.c,v 1.2 2018/10/21 18:31:58 jmcneill Exp $ */

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: plgpio_acpi.c,v 1.2 2018/10/21 18:31:58 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/gpio.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <dev/gpio/gpiovar.h>
#include <dev/ic/pl061var.h>
#include <dev/ic/pl061reg.h>

struct plgpio_acpi_softc;

struct plgpio_acpi_event {
	struct plgpio_acpi_softc *ev_sc;
	u_int			ev_pin;
	ACPI_HANDLE		ev_method;
	bool			ev_method_evt;
};

struct plgpio_acpi_softc {
	struct plgpio_softc	sc_base;

	ACPI_HANDLE		sc_handle;
	uint32_t		sc_aei_pins;		/* bitmask */
	ACPI_RESOURCE_GPIO	sc_aei_res[8];

	struct plgpio_acpi_event sc_event[8];
};

static int	plgpio_acpi_match(device_t, cfdata_t, void *);
static void	plgpio_acpi_attach(device_t, device_t, void *);

static void	plgpio_acpi_init(struct plgpio_acpi_softc *);
static void	plgpio_acpi_notify(void *);
static int	plgpio_acpi_intr(void *);

CFATTACH_DECL_NEW(plgpio_acpi, sizeof(struct plgpio_acpi_softc), plgpio_acpi_match, plgpio_acpi_attach, NULL, NULL);

static const char * const compatible[] = {
	"ARMH0061",
	NULL
};

static int
plgpio_acpi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, compatible);
}

static void
plgpio_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct plgpio_acpi_softc * const asc = device_private(self);
	struct plgpio_softc * const sc = &asc->sc_base;
	struct acpi_attach_args *aa = aux;
	struct acpi_resources res;
	struct acpi_mem *mem;
	struct acpi_irq *irq;
	ACPI_STATUS rv;
	int error;
	void *ih;

	sc->sc_dev = self;
	asc->sc_handle = aa->aa_node->ad_handle;

	rv = acpi_resource_parse(sc->sc_dev, aa->aa_node->ad_handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;

	mem = acpi_res_mem(&res, 0);
	if (mem == NULL) {
		aprint_error_dev(self, "couldn't find mem resource\n");
		goto done;
	}

	irq = acpi_res_irq(&res, 0);
	if (mem == NULL) {
		aprint_error_dev(self, "couldn't find irq resource\n");
		goto done;
	}

	sc->sc_dev = self;
	sc->sc_bst = aa->aa_memt;
	error = bus_space_map(sc->sc_bst, mem->ar_base, mem->ar_length, 0, &sc->sc_bsh);
	if (error) {
		aprint_error_dev(self, "couldn't map registers\n");
		return;
	}

	plgpio_attach(sc);

	const int type = (irq->ar_type == ACPI_EDGE_SENSITIVE) ? IST_EDGE : IST_LEVEL;
	ih = intr_establish(irq->ar_irq, IPL_VM, type, plgpio_acpi_intr, asc);
	if (ih == NULL)
		aprint_error_dev(self, "couldn't establish interrupt\n");

	plgpio_acpi_init(asc);

done:
	acpi_resource_cleanup(&res);
}

static ACPI_STATUS
plgpio_acpi_resource_cb(ACPI_RESOURCE *res, void *ctx)
{
	struct plgpio_acpi_softc * const asc = ctx;
	ACPI_RESOURCE_GPIO *gpio;
	UINT16 pin;

	if (res->Type != ACPI_RESOURCE_TYPE_GPIO)
		return AE_OK;

	gpio = &res->Data.Gpio;
	if (gpio->ConnectionType != ACPI_RESOURCE_GPIO_TYPE_INT ||
	    gpio->PinTableLength != 1)
		return AE_OK;

	pin = gpio->PinTable[0];
	if (pin >= __arraycount(asc->sc_aei_res)) {
		aprint_error_dev(asc->sc_base.sc_dev, "_AEI pin %u out of range\n", pin);
		return AE_OK;
	}

	asc->sc_aei_pins |= __BIT(pin);
	asc->sc_aei_res[pin] = *gpio;

	return AE_OK;
}

static void
plgpio_acpi_init(struct plgpio_acpi_softc *asc)
{
	struct plgpio_softc * const sc = &asc->sc_base;
	ACPI_RESOURCE_GPIO *gpio;
	ACPI_HANDLE handle;
	char namebuf[5];
	ACPI_STATUS rv;
	uint32_t ibe, iev, is, ie;
	int pin;

	rv = AcpiWalkResources(asc->sc_handle, "_AEI", plgpio_acpi_resource_cb, asc);
	if (ACPI_FAILURE(rv)) {
		if (rv != AE_NOT_FOUND)
			aprint_error_dev(asc->sc_base.sc_dev, "failed to parse _AEI: %s\n",
			    AcpiFormatException(rv));
		return;
	}

	if (!asc->sc_aei_pins)
		return;

	aprint_verbose_dev(asc->sc_base.sc_dev, "ACPI event pins: %#x\n", asc->sc_aei_pins);

	sc->sc_reserved_mask = asc->sc_aei_pins;

	/*
	 * For each event pin, find the corresponding event method (_Exx, _Lxx, or _EVT).
	 */
	for (pin = 0; pin < __arraycount(asc->sc_aei_res); pin++) {
		if ((asc->sc_aei_pins & __BIT(pin)) == 0)
			continue;

		gpio = &asc->sc_aei_res[pin];

		const char trig = gpio->Triggering == ACPI_LEVEL_SENSITIVE ? 'L' : 'E';
		handle = NULL;
		snprintf(namebuf, sizeof(namebuf), "_%c%02X", trig, pin);
		if (ACPI_FAILURE(AcpiGetHandle(asc->sc_handle, namebuf, &handle))) {
			(void)AcpiGetHandle(asc->sc_handle, "_EVT", &handle);
			if (handle != NULL)
				asc->sc_event[pin].ev_method_evt = true;
		}

		if (handle == NULL)
			continue;

		asc->sc_event[pin].ev_sc = asc;
		asc->sc_event[pin].ev_pin = pin;
		asc->sc_event[pin].ev_method = handle;

		/*
		 * Configure and enable interrupts for this pin.
		 */

		ibe = PLGPIO_READ(sc, PL061_GPIOIBE_REG);
		iev = PLGPIO_READ(sc, PL061_GPIOIEV_REG);
		switch (gpio->Polarity) {
		case ACPI_ACTIVE_HIGH:
			ibe &= ~__BIT(pin);
			iev |= __BIT(pin);
			break;
		case ACPI_ACTIVE_LOW:
			ibe &= ~__BIT(pin);
			iev &= ~__BIT(pin);
			break;
		case ACPI_ACTIVE_BOTH:
			ibe |= __BIT(pin);
			break;
		}
		PLGPIO_WRITE(sc, PL061_GPIOIBE_REG, ibe);
		PLGPIO_WRITE(sc, PL061_GPIOIEV_REG, iev);

		is = PLGPIO_READ(sc, PL061_GPIOIS_REG);
		switch (gpio->Triggering) {
		case ACPI_LEVEL_SENSITIVE:
			is |= __BIT(pin);
			break;
		case ACPI_EDGE_SENSITIVE:
			is &= ~__BIT(pin);
			break;
		}
		PLGPIO_WRITE(sc, PL061_GPIOIS_REG, is);

		delay(20);

		PLGPIO_WRITE(sc, PL061_GPIOIC_REG, __BIT(pin));

		ie = PLGPIO_READ(sc, PL061_GPIOIE_REG);
		ie |= __BIT(pin);
		PLGPIO_WRITE(sc, PL061_GPIOIE_REG, ie);
	}
}

static void
plgpio_acpi_notify(void *priv)
{
	struct plgpio_acpi_event * const ev = priv;
	struct plgpio_acpi_softc * const asc = ev->ev_sc;
	struct plgpio_softc * const sc = &asc->sc_base;
	ACPI_STATUS rv;

	if (ev->ev_method_evt) {
		ACPI_OBJECT_LIST objs;
		ACPI_OBJECT obj[1];
		objs.Count = 1;
		objs.Pointer = obj;
		obj[0].Type = ACPI_TYPE_INTEGER;
		obj[0].Integer.Value = ev->ev_pin;
		rv = AcpiEvaluateObject(ev->ev_method, NULL, &objs, NULL);
	} else {
		rv = AcpiEvaluateObject(ev->ev_method, NULL, NULL, NULL);
	}

	if (ACPI_FAILURE(rv))
		device_printf(sc->sc_dev, "failed to handle %s event: %s\n",
		    acpi_name(ev->ev_method), AcpiFormatException(rv));
}

static int
plgpio_acpi_intr(void *priv)
{
	struct plgpio_acpi_softc * const asc = priv;
	struct plgpio_softc * const sc = &asc->sc_base;
	uint32_t mis;
	int bit;

	mis = PLGPIO_READ(sc, PL061_GPIOMIS_REG);
	PLGPIO_WRITE(sc, PL061_GPIOIC_REG, mis);

	while ((bit = __builtin_ffs(mis)) != 0) {
		const int pin = bit - 1;
		struct plgpio_acpi_event * const ev = &asc->sc_event[pin];

		KASSERT(ev->ev_method != NULL);

		AcpiOsExecute(OSL_NOTIFY_HANDLER, plgpio_acpi_notify, ev);

		mis &= ~__BIT(pin);
	}

	return 1;
}
