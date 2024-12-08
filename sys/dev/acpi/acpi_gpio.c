/* $NetBSD: acpi_gpio.c,v 1.1 2024/12/08 20:49:14 jmcneill Exp $ */

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

/*
 * ACPI GPIO resource support.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_gpio.c,v 1.1 2024/12/08 20:49:14 jmcneill Exp $");

#include <sys/param.h>
#include <sys/gpio.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_gpio.h>

int
acpi_gpio_register(struct acpi_devnode *ad, device_t dev,
    int (*translate)(void *, ACPI_INTEGER, void **), void *priv)
{
	if (ad->ad_gpiodev != NULL) {
		device_printf(dev, "%s already registered\n",
		    device_xname(ad->ad_gpiodev));
		return EBUSY;
	}

	ad->ad_gpiodev = dev;
	ad->ad_gpio_translate = translate;
	ad->ad_gpio_priv = priv;

	return 0;
}

static ACPI_STATUS
acpi_gpio_translate(ACPI_RESOURCE_GPIO *res, void **gpiop, int *pin)
{
	struct acpi_devnode *ad, *gpioad = NULL;
	ACPI_HANDLE hdl;
	ACPI_RESOURCE_SOURCE *rs;
	ACPI_STATUS rv;
	int xpin;

	/* Find the device node providing the GPIO resource. */
	rs = &res->ResourceSource;
	if (rs->StringPtr == NULL) {
		return AE_NOT_FOUND;
	}
	rv = AcpiGetHandle(NULL, rs->StringPtr, &hdl);
	if (ACPI_FAILURE(rv)) {
		return rv;
	}
	SIMPLEQ_FOREACH(ad, &acpi_softc->sc_head, ad_list) {
		if (ad->ad_handle == hdl) {
			gpioad = ad;
			break;
		}
	}
	if (gpioad == NULL) {
		/* No device node found. */
		return AE_NOT_FOUND;
	}

	if (gpioad->ad_gpiodev == NULL) {
		/* No resource provider is registered. */
		return AE_NO_HANDLER;
	}

	xpin = gpioad->ad_gpio_translate(gpioad->ad_gpio_priv,
	    res->PinTable[0], gpiop);
	if (xpin == -1) {
		/* Pin could not be translated. */
		return AE_NOT_IMPLEMENTED;
	}

	*pin = xpin;

	return AE_OK;
}

struct acpi_gpio_resource_context {
	u_int index;
	u_int conntype;
	u_int curindex;
	ACPI_RESOURCE_GPIO *res;
};

static ACPI_STATUS
acpi_gpio_parse(ACPI_RESOURCE *res, void *context)
{
	struct acpi_gpio_resource_context *ctx = context;

	if (res->Type != ACPI_RESOURCE_TYPE_GPIO) {
		return AE_OK;
	}
	if (res->Data.Gpio.ConnectionType != ctx->conntype) {
		return AE_OK;
	}
	if (ctx->curindex == ctx->index) {
		ctx->res = &res->Data.Gpio;
		return AE_CTRL_TERMINATE;
	}
	ctx->curindex++;
	return AE_OK;
	
}

ACPI_STATUS
acpi_gpio_get_int(ACPI_HANDLE hdl, u_int index, void **gpiop, int *pin,
    int *irqmode)
{
	struct acpi_gpio_resource_context ctx = {
		.index = index,
		.conntype = ACPI_RESOURCE_GPIO_TYPE_INT,
	};
	ACPI_RESOURCE_GPIO *gpio;
	ACPI_STATUS rv;

	rv = AcpiWalkResources(hdl, "_CRS", acpi_gpio_parse, &ctx);
	if (ACPI_FAILURE(rv)) {
		return rv;
	}

	rv = acpi_gpio_translate(ctx.res, gpiop, pin);
	if (ACPI_FAILURE(rv)) {
		printf("%s: translate failed: %s\n", __func__,
		    AcpiFormatException(rv));
		return rv;
	}

	gpio = ctx.res;
        if (gpio->Triggering == ACPI_LEVEL_SENSITIVE) {
                *irqmode = gpio->Polarity == ACPI_ACTIVE_HIGH ? 
                    GPIO_INTR_HIGH_LEVEL : GPIO_INTR_LOW_LEVEL;
        } else {
                KASSERT(gpio->Triggering == ACPI_EDGE_SENSITIVE);
                if (gpio->Polarity == ACPI_ACTIVE_LOW) { 
                        *irqmode = GPIO_INTR_NEG_EDGE;
                } else if (gpio->Polarity == ACPI_ACTIVE_HIGH) {
                        *irqmode = GPIO_INTR_POS_EDGE;
                } else {
                        KASSERT(gpio->Polarity == ACPI_ACTIVE_BOTH);
                        *irqmode = GPIO_INTR_DOUBLE_EDGE;
                }
        }

	return AE_OK;
}

ACPI_STATUS
acpi_gpio_get_io(ACPI_HANDLE hdl, u_int index, void **gpio, int *pin)
{
	struct acpi_gpio_resource_context ctx = {
		.index = index,
		.conntype = ACPI_RESOURCE_GPIO_TYPE_INT,
	};
	ACPI_STATUS rv;

	rv = AcpiWalkResources(hdl, "_CRS", acpi_gpio_parse, &ctx);
	if (ACPI_FAILURE(rv)) {
		return rv;
	}

	return acpi_gpio_translate(ctx.res, gpio, pin);
}
