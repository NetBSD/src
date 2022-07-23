/* $NetBSD: acpi_i2c.c,v 1.12 2022/07/23 03:08:17 thorpej Exp $ */

/*-
 * Copyright (c) 2017, 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Manuel Bouyer.
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
__KERNEL_RCSID(0, "$NetBSD: acpi_i2c.c,v 1.12 2022/07/23 03:08:17 thorpej Exp $");

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_i2c.h>
#include <dev/i2c/i2cvar.h>

#include <sys/kmem.h>

#define _COMPONENT	ACPI_BUS_COMPONENT
ACPI_MODULE_NAME	("acpi_i2c")

struct acpi_i2c_context {
	uint16_t i2c_addr;
};

static ACPI_STATUS
acpi_i2c_resource_parse_callback(ACPI_RESOURCE *res, void *context)
{
	struct acpi_i2c_context *i2cc = context;

	switch (res->Type) {
	case ACPI_RESOURCE_TYPE_END_TAG:
		break;
	case ACPI_RESOURCE_TYPE_SERIAL_BUS:
		switch (res->Data.I2cSerialBus.Type) {
		case ACPI_RESOURCE_SERIAL_TYPE_I2C:
			i2cc->i2c_addr = res->Data.I2cSerialBus.SlaveAddress;
			break;
		}
		break;
	case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
		break;
	default:
		printf("resource type 0x%x ignored\n", res->Type);
	}
	return_ACPI_STATUS(AE_OK);
}

static void
acpi_enter_i2c_device(struct acpi_devnode *ad, prop_array_t array)
{
	prop_dictionary_t dev;
	struct acpi_i2c_context i2cc;
	ACPI_STATUS rv;
	char *clist;
	size_t clist_size;

	memset(&i2cc, 0, sizeof(i2cc));
	rv = AcpiWalkResources(ad->ad_handle, "_CRS",
	     acpi_i2c_resource_parse_callback, &i2cc);
	if (ACPI_FAILURE(rv)) {
		aprint_error("ACPI: unable to get resources "
		   "for %s: %s\n", ad->ad_name,
		   AcpiFormatException(rv));
		return;
	}
	if (i2cc.i2c_addr == 0)
		return;
	dev = prop_dictionary_create();
	if (dev == NULL) {
		aprint_error("ignoring device %s (no memory)\n",
		    ad->ad_name);
		return;
	}
	clist = acpi_pack_compat_list(ad, &clist_size);
	if (clist == NULL) {
		prop_object_release(dev);
		aprint_error("ignoring device %s (no _HID or _CID)\n",
		    ad->ad_name);
		return;
	}
	prop_dictionary_set_string(dev, "name", ad->ad_name);
	prop_dictionary_set_uint32(dev, "addr", i2cc.i2c_addr);
	prop_dictionary_set_uint64(dev, "cookie", (uintptr_t)ad->ad_handle);
	prop_dictionary_set_uint32(dev, "cookietype", I2C_COOKIE_ACPI);
	prop_dictionary_set_data(dev, "compatible", clist, clist_size);
	kmem_free(clist, clist_size);

	prop_array_add(array, dev);
	prop_object_release(dev);
}

prop_array_t
acpi_enter_i2c_devs(device_t dev, struct acpi_devnode *devnode)
{
	struct acpi_devnode *ad;
	prop_array_t array = prop_array_create();

	if (array == NULL)
		return NULL;

	SIMPLEQ_FOREACH(ad, &devnode->ad_child_head, ad_child_list) {
		if (ad->ad_devinfo->Type != ACPI_TYPE_DEVICE)
			continue;
		if (!acpi_device_present(ad->ad_handle))
			continue;
		acpi_enter_i2c_device(ad, array);
	}

	if (dev != NULL) {
		acpi_claim_childdevs(dev, devnode);
	}

	return array;
}
