/* $NetBSD: acpi_gpio.c,v 1.5 2024/12/15 10:15:55 hannken Exp $ */

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

#include "gpio.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_gpio.c,v 1.5 2024/12/15 10:15:55 hannken Exp $");

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <dev/gpio/gpiovar.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_gpio.h>

#if NGPIO > 0

#define _COMPONENT	ACPI_RESOURCE_COMPONENT
ACPI_MODULE_NAME	("acpi_gpio")

struct acpi_gpio_address_space_context {
	ACPI_CONNECTION_INFO conn_info;	/* must be first */
	struct acpi_devnode *ad;
};

static ACPI_STATUS
acpi_gpio_address_space_init(ACPI_HANDLE region_hdl, UINT32 function,
    void *handler_ctx, void **region_ctx)
{
	if (function == ACPI_REGION_DEACTIVATE) {
		*region_ctx = NULL;
	} else {
		*region_ctx = region_hdl;
	}
	return AE_OK;
}

static ACPI_STATUS
acpi_gpio_address_space_handler(UINT32 function,
    ACPI_PHYSICAL_ADDRESS address, UINT32 bit_width, UINT64 *value,
    void *handler_ctx, void *region_ctx)
{
	ACPI_OPERAND_OBJECT *region_obj = region_ctx;
	struct acpi_gpio_address_space_context *context = handler_ctx;
	ACPI_CONNECTION_INFO *conn_info = &context->conn_info;
	struct acpi_devnode *ad = context->ad;
	ACPI_RESOURCE *res;
	ACPI_STATUS rv;
	struct gpio_pinmap pinmap;
	int pins[1];
	void *gpiop;
	int pin;

	if (region_obj->Region.Type != ACPI_TYPE_REGION) {
		return AE_OK;
	}

	if (ad->ad_gpiodev == NULL) {
		return AE_NO_HANDLER;
	}

	rv = AcpiBufferToResource(conn_info->Connection,
	    conn_info->Length, &res);
	if (ACPI_FAILURE(rv)) {
		return rv;
	}

	if (res->Data.Gpio.PinTableLength != 1) {
		/* TODO */
		aprint_debug_dev(ad->ad_gpiodev,
		    "Pin table length %u not implemented\n",
		    res->Data.Gpio.PinTableLength);
		rv = AE_NOT_IMPLEMENTED;
		goto done;
	}

	pin = ad->ad_gpio_translate(ad->ad_gpio_priv,
	    &res->Data.Gpio, &gpiop);
	if (pin == -1) {
		/* Pin could not be translated. */
		rv = AE_SUPPORT;
		goto done;
	}

	pinmap.pm_map = pins;
	if (gpio_pin_map(gpiop, pin, 1, &pinmap) != 0) {
		rv = AE_NOT_ACQUIRED;
		goto done;
	}
	if (function & ACPI_IO_MASK) {
		gpio_pin_write(gpiop, &pinmap, 0, *value & 1);
	} else {
		*value = gpio_pin_read(gpiop, &pinmap, 0);
	}
	gpio_pin_unmap(gpiop, &pinmap);

done:
	ACPI_FREE(res);

	return rv;
}
#endif

ACPI_STATUS
acpi_gpio_register(struct acpi_devnode *ad, device_t dev,
    int (*translate)(void *, ACPI_RESOURCE_GPIO *, void **), void *priv)
{
#if NGPIO > 0
	struct acpi_gpio_address_space_context *context;
	ACPI_STATUS rv;

	if (ad->ad_gpiodev != NULL) {
		device_printf(dev, "%s already registered\n",
		    device_xname(ad->ad_gpiodev));
		return AE_ALREADY_EXISTS;
	}

	context = kmem_zalloc(sizeof(*context), KM_SLEEP);
	context->ad = ad;

	rv = AcpiInstallAddressSpaceHandler(ad->ad_handle,
	    ACPI_ADR_SPACE_GPIO,
	    acpi_gpio_address_space_handler,
	    acpi_gpio_address_space_init,
	    context);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(dev,
		    "couldn't install address space handler: %s",
		    AcpiFormatException(rv));
		return rv;
	}

	ad->ad_gpiodev = dev;
	ad->ad_gpio_translate = translate;
	ad->ad_gpio_priv = priv;

	return AE_OK;
#else
	return AE_NOT_CONFIGURED;
#endif
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
	    res, gpiop);
	if (xpin == -1) {
		/* Pin could not be translated. */
		return AE_SUPPORT;
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
	gpio = ctx.res;

	rv = acpi_gpio_translate(gpio, gpiop, pin);
	if (ACPI_FAILURE(rv)) {
		printf("%s: translate failed: %s\n", __func__,
		    AcpiFormatException(rv));
		return rv;
	}

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
acpi_gpio_get_io(ACPI_HANDLE hdl, u_int index, void **gpiop, int *pin)
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

	return acpi_gpio_translate(ctx.res, gpiop, pin);
}
