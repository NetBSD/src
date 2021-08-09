/* $NetBSD: acpi_i2c.c,v 1.11.14.1 2021/08/09 00:30:09 thorpej Exp $ */

/*-
 * Copyright (c) 2017, 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Manuel Bouyer and Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: acpi_i2c.c,v 1.11.14.1 2021/08/09 00:30:09 thorpej Exp $");

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
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

static bool
acpi_i2c_enumerate_device(device_t dev, struct acpi_devnode *ad,
    struct i2c_enumerate_devices_args * const args)
{
	char *clist;
	size_t clist_size;
	prop_dictionary_t props;
	struct acpi_i2c_context i2cc;
	bool cbrv;
	ACPI_STATUS rv;

	memset(&i2cc, 0, sizeof(i2cc));
	rv = AcpiWalkResources(ad->ad_handle, "_CRS",
	     acpi_i2c_resource_parse_callback, &i2cc);
	if (ACPI_FAILURE(rv)) {
		aprint_error("ACPI: unable to get resources "
		   "for %s: %s\n", ad->ad_name,
		   AcpiFormatException(rv));
		return true;	/* keep enumerating */
	}
	if (i2cc.i2c_addr == 0)
		return true;	/* keep enumerating */

	clist = acpi_pack_compat_list(ad->ad_devinfo, &clist_size);
	if (clist == NULL) {
		aprint_error("ACPI: ignoring device %s (no _HID or _CID)\n",
		    ad->ad_name);
		return true;	/* keep enumerating */
	}
	props = prop_dictionary_create();

	args->ia->ia_addr = i2cc.i2c_addr;
	args->ia->ia_name = ad->ad_name;
	args->ia->ia_clist = clist;
	args->ia->ia_clist_size = clist_size;
	args->ia->ia_prop = props;
	args->ia->ia_devhandle = devhandle_from_acpi(ad->ad_handle);

	cbrv = args->callback(dev, args);

	prop_object_release(props);
	kmem_free(clist, clist_size);

	return cbrv;	/* callback decides if we keep enumerating */
}

static int
acpi_i2c_enumerate_devices(device_t dev, devhandle_t call_handle, void *v)
{
	struct i2c_enumerate_devices_args *args = v;
	struct acpi_devnode *ad, *devnode;
	ACPI_HANDLE *hdl = devhandle_to_acpi(call_handle);

	devnode = acpi_match_node(hdl);
	if (devnode == NULL) {
		aprint_verbose_dev(dev, "%s: no devnode matching handle\n",
		    __func__);
		return 0;
	}

	SIMPLEQ_FOREACH(ad, &devnode->ad_child_head, ad_child_list) {
		if (ad->ad_devinfo->Type != ACPI_TYPE_DEVICE)
			continue;
		if (!acpi_device_present(ad->ad_handle))
			continue;
		if (!acpi_i2c_enumerate_device(dev, ad, args))
			break;
	}

	acpi_claim_childdevs(dev, devnode);

	return 0;
}
ACPI_DEVICE_CALL_REGISTER("i2c-enumerate-devices", acpi_i2c_enumerate_devices)
