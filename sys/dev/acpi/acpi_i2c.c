/* $NetBSD: acpi_i2c.c,v 1.19 2025/01/11 11:40:43 jmcneill Exp $ */

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

#include "iic.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_i2c.c,v 1.19 2025/01/11 11:40:43 jmcneill Exp $");

#include <sys/device.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_i2c.h>
#include <external/bsd/acpica/dist/include/acinterp.h>
#include <external/bsd/acpica/dist/include/amlcode.h>
#include <dev/i2c/i2cvar.h>

#include <sys/kmem.h>

#define _COMPONENT	ACPI_BUS_COMPONENT
ACPI_MODULE_NAME	("acpi_i2c")

struct acpi_i2c_address_space_context {
	ACPI_CONNECTION_INFO conn_info;	/* must be first */
	i2c_tag_t tag;
};

static const struct device_compatible_entry hid_compat_data[] = {
	{ .compat = "PNP0C50" },
	DEVICE_COMPAT_EOL
};

#if NIIC > 0
struct acpi_i2c_context {
	uint16_t i2c_addr;
	struct acpi_devnode *res_src;
};
#endif

static struct acpi_devnode *
acpi_i2c_resource_find_source(ACPI_RESOURCE_SOURCE *rs)
{
	ACPI_STATUS rv;
	ACPI_HANDLE hdl;
	struct acpi_devnode *ad;

	if (rs->StringPtr == NULL) {
		return NULL;
	}

	rv = AcpiGetHandle(NULL, rs->StringPtr, &hdl);
	if (ACPI_FAILURE(rv)) {
		printf("%s: couldn't lookup '%s': %s\n", __func__,
		    rs->StringPtr, AcpiFormatException(rv));
		return NULL;
	}

	SIMPLEQ_FOREACH(ad, &acpi_softc->sc_head, ad_list) {
		if (ad->ad_handle == hdl) {
			return ad;
		}
	}

	printf("%s: no acpi devnode matching resource source '%s'\n",
	    __func__, rs->StringPtr);
	return NULL;
}

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
			i2cc->res_src = acpi_i2c_resource_find_source(
			    &res->Data.I2cSerialBus.ResourceSource);
			break;
		}
		break;
	case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
		break;
	default:
		break;
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

static void
acpi_enter_i2chid_devs(device_t dev, struct acpi_devnode *devnode,
    prop_array_t array)
{
	struct acpi_devnode *ad;

	KASSERT(dev != NULL);

	SIMPLEQ_FOREACH(ad, &acpi_softc->sc_head, ad_list) {
		struct acpi_attach_args aa = {
			.aa_node = ad
		};
		struct acpi_i2c_context i2cc;
		ACPI_STATUS rv;

		if (!acpi_device_present(ad->ad_handle))
			continue;
		if (ad->ad_device != NULL)
			continue;
		if (acpi_compatible_match(&aa, hid_compat_data) == 0)
			continue;

		memset(&i2cc, 0, sizeof(i2cc));
		rv = AcpiWalkResources(ad->ad_handle, "_CRS",
		    acpi_i2c_resource_parse_callback, &i2cc);
		if (ACPI_SUCCESS(rv) &&
		    i2cc.i2c_addr != 0 &&
		    i2cc.res_src == devnode) {
			aprint_debug_dev(dev, "claiming %s\n", ad->ad_name);
			ad->ad_device = dev;
			acpi_claim_childdevs(dev, ad, NULL);
			acpi_enter_i2c_device(ad, array);
		}
	}
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
		acpi_claim_childdevs(dev, devnode, "_CRS");
		acpi_claim_childdevs(dev, devnode, "_ADR");
		acpi_enter_i2chid_devs(dev, devnode, array);
	}

	return array;
}

#if NIIC > 0
static ACPI_STATUS
acpi_i2c_gsb_init(ACPI_HANDLE region_hdl, UINT32 function,
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
acpi_i2c_gsb_handler(UINT32 function, ACPI_PHYSICAL_ADDRESS address,
    UINT32 bit_width, UINT64 *value, void *handler_ctx,
    void *region_ctx)
{
	ACPI_OPERAND_OBJECT *region_obj = region_ctx;
	struct acpi_i2c_address_space_context *context = handler_ctx;
	UINT8 *buf = ACPI_CAST_PTR(uint8_t, value);
	ACPI_PHYSICAL_ADDRESS base_address;
	ACPI_RESOURCE *res;
	ACPI_STATUS rv;
	ACPI_CONNECTION_INFO *conn_info = &context->conn_info;
	i2c_tag_t tag = context->tag;
	i2c_addr_t i2c_addr;
	i2c_op_t op;
	union {
		uint8_t cmd8;
		uint16_t cmd16;
		uint32_t cmd32;
	} cmd;
	size_t buflen;
	size_t cmdlen;
	bool do_xfer = true;

	if (region_obj->Region.Type != ACPI_TYPE_REGION) {
		return AE_OK;
	}

	base_address = region_obj->Region.Address;
	KASSERT(region_obj->Region.SpaceId == ACPI_ADR_SPACE_GSBUS);

	rv = AcpiBufferToResource(conn_info->Connection, conn_info->Length,
	    &res);
	if (ACPI_FAILURE(rv)) {
		return rv;
	}
	if (res->Type != ACPI_RESOURCE_TYPE_SERIAL_BUS ||
	    res->Data.CommonSerialBus.Type != ACPI_RESOURCE_SERIAL_TYPE_I2C) {
		return AE_TYPE;
	}

	i2c_addr = res->Data.I2cSerialBus.SlaveAddress;
	if ((function & ACPI_IO_MASK) != 0) {
		op = I2C_OP_WRITE_WITH_STOP;
	} else {
		op = I2C_OP_READ_WITH_STOP;
	}

#ifdef ACPI_I2C_DEBUG
	UINT32 length;
	rv = AcpiExGetProtocolBufferLength(function >> 16, &length);
	if (ACPI_FAILURE(rv)) {
		printf("%s AcpiExGetProtocolBufferLength failed: %s\n",
		    __func__, AcpiFormatException(rv));
		length = UINT32_MAX;
	}
	printf("%s %s: %s Attr %X Addr %.4X BaseAddr %.4X Length %.2X BitWidth %X BufLen %X",
	       __func__, AcpiUtGetRegionName(region_obj->Region.SpaceId),
	       (function & ACPI_IO_MASK) ? "Write" : "Read ",
	       (UINT32) (function >> 16),
	       (UINT32) address, (UINT32) base_address,
	       length, bit_width, buf[1]);
	printf(" [AccessLength %.2X Connection %p]\n",
	       conn_info->AccessLength, conn_info->Connection);
#endif

	switch ((UINT32)(function >> 16)) {
	case AML_FIELD_ATTRIB_QUICK:
		cmdlen = 0;
		buflen = 0;
		break;
	case AML_FIELD_ATTRIB_SEND_RECEIVE:
		cmdlen = 0;
		buflen = 1;
		break;
	case AML_FIELD_ATTRIB_BYTE:
		cmdlen = bit_width / NBBY;
		buflen = 1;
		break;
	case AML_FIELD_ATTRIB_WORD:
		cmdlen = bit_width / NBBY;
		buflen = 2;
		break;
	case AML_FIELD_ATTRIB_BYTES:
		cmdlen = bit_width / NBBY;
		buflen = buf[1];
		break;
	case AML_FIELD_ATTRIB_BLOCK:
		cmdlen = bit_width / NBBY;
		buflen = buf[1];
		op |= I2C_OPMASK_BLKMODE;
		break;
	case AML_FIELD_ATTRIB_RAW_BYTES:
	case AML_FIELD_ATTRIB_RAW_PROCESS_BYTES:
	case AML_FIELD_ATTRIB_PROCESS_CALL:
	default:
		cmdlen = 0;
		do_xfer = false;
#ifdef ACPI_I2C_DEBUG
		printf("field attrib 0x%x not supported\n",
		    (UINT32)(function >> 16));
#endif
		break;
	}

	switch (cmdlen) {
	case 0:
	case 1:
		cmd.cmd8 = (uint8_t)(base_address + address);
		break;
	case 2:
		cmd.cmd16 = (uint16_t)(base_address + address);
		break;
	case 4:
		cmd.cmd32 = (uint32_t)(base_address + address);
		break;
	default:
		do_xfer = false;
#ifdef ACPI_I2C_DEBUG
		printf("cmdlen %zu not supported\n", cmdlen);
#endif
		break;
	}

	if (!do_xfer) {
		buf[0] = EINVAL;
	} else {
		const int flags = I2C_F_POLL;
		iic_acquire_bus(tag, flags);
		buf[0] = iic_exec(tag, op, i2c_addr,
				  &cmd, cmdlen, &buf[2], buflen, flags);
		iic_release_bus(tag, flags);
		if (buf[0] == 0) {
			buf[1] = buflen;
		}
#ifdef ACPI_I2C_DEBUG
		printf("%s iic_exec op %u addr 0x%x len %zu/%zu returned %d\n",
		    __func__, op, res->Data.I2cSerialBus.SlaveAddress, cmdlen,
		    buflen, buf[0]);
#endif
	}

	ACPI_FREE(res);

	return AE_OK;
}
#endif

ACPI_STATUS
acpi_i2c_register(struct acpi_devnode *devnode, device_t dev, i2c_tag_t tag)
{
#if NIIC > 0
	struct acpi_i2c_address_space_context *context;
	ACPI_STATUS rv;

	context = kmem_zalloc(sizeof(*context), KM_SLEEP);
	context->tag = tag;

	rv = AcpiInstallAddressSpaceHandler(devnode->ad_handle,
	    ACPI_ADR_SPACE_GSBUS, acpi_i2c_gsb_handler, acpi_i2c_gsb_init,
	    context);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(dev,
		    "couldn't install address space handler: %s",
		    AcpiFormatException(rv));
	}

	return rv;
#else
	return AE_NOT_CONFIGURED;
#endif
}
