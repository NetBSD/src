/*	$NetBSD: sdp.c,v 1.2 2007/04/11 20:01:01 plunky Exp $	*/

/*-
 * Copyright (c) 2006 Itronix Inc.
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
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 2004 Maksim Yevmenkin <m_evmenkin@yahoo.com>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: sdp.c,v 1.2 2007/04/11 20:01:01 plunky Exp $");

#include <sys/types.h>

#include <dev/bluetooth/btdev.h>
#include <dev/bluetooth/bthidev.h>
#include <dev/bluetooth/btsco.h>
#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>

#include <prop/proplib.h>

#include <bluetooth.h>
#include <err.h>
#include <errno.h>
#include <sdp.h>
#include <stdlib.h>
#include <usbhid.h>

#include "btdevctl.h"

static int32_t parse_l2cap_psm(sdp_attr_t *);
static int32_t parse_rfcomm_channel(sdp_attr_t *);
static int32_t parse_hid_descriptor(sdp_attr_t *);
static int32_t parse_boolean(sdp_attr_t *);

static int config_hid(prop_dictionary_t);
static int config_hset(prop_dictionary_t);
static int config_hf(prop_dictionary_t);

uint16_t hid_services[] = {
	SDP_SERVICE_CLASS_HUMAN_INTERFACE_DEVICE
};

uint32_t hid_attrs[] = {
	SDP_ATTR_RANGE(	SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST,
			SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST),
	SDP_ATTR_RANGE( SDP_ATTR_ADDITIONAL_PROTOCOL_DESCRIPTOR_LISTS,
			SDP_ATTR_ADDITIONAL_PROTOCOL_DESCRIPTOR_LISTS),
	SDP_ATTR_RANGE(	0x0205,		/* HIDReconnectInitiate */
			0x0206),	/* HIDDescriptorList */
	SDP_ATTR_RANGE(	0x0209,		/* HIDBatteryPower */
			0x0209),
	SDP_ATTR_RANGE(	0x020d,		/* HIDNormallyConnectable */
			0x020d)
};

uint16_t hset_services[] = {
	SDP_SERVICE_CLASS_HEADSET
};

uint32_t hset_attrs[] = {
	SDP_ATTR_RANGE(	SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST,
			SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST),
};

uint16_t hf_services[] = {
	SDP_SERVICE_CLASS_HANDSFREE_AUDIO_GATEWAY
};

uint32_t hf_attrs[] = {
	SDP_ATTR_RANGE(	SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST,
			SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST),
};

#define NUM(v)		(sizeof(v) / sizeof(v[0]))

static struct {
	const char		*name;
	int			(*handler)(prop_dictionary_t);
	const char		*description;
	uint16_t		*services;
	int			nservices;
	uint32_t		*attrs;
	int			nattrs;
} cfgtype[] = {
    {
	"HID",		config_hid,	"Human Interface Device",
	hid_services,	NUM(hid_services),
	hid_attrs,	NUM(hid_attrs),
    },
    {
	"HSET",		config_hset,	"Headset",
	hset_services,	NUM(hset_services),
	hset_attrs,	NUM(hset_attrs),
    },
    {
	"HF",		config_hf,	"Handsfree",
	hf_services,	NUM(hf_services),
	hf_attrs,	NUM(hf_attrs),
    },
};

static sdp_attr_t	values[8];
static uint8_t		buffer[NUM(values)][512];

prop_dictionary_t
cfg_query(bdaddr_t *laddr, bdaddr_t *raddr, const char *service)
{
	prop_dictionary_t dict;
	void *ss;
	int rv, i;

	dict = prop_dictionary_create();
	if (dict == NULL)
		return NULL;

	for (i = 0 ; i < NUM(values) ; i++) {
		values[i].flags = SDP_ATTR_INVALID;
		values[i].attr = 0;
		values[i].vlen = sizeof(buffer[i]);
		values[i].value = buffer[i];
	}

	for (i = 0 ; i < NUM(cfgtype) ; i++) {
		if (strcasecmp(service, cfgtype[i].name) == 0) {
			ss = sdp_open(laddr, raddr);

			if (ss == NULL || (errno = sdp_error(ss)) != 0)
				return NULL;

			rv = sdp_search(ss,
				cfgtype[i].nservices, cfgtype[i].services,
				cfgtype[i].nattrs, cfgtype[i].attrs,
				NUM(values), values);

			if (rv != 0) {
				errno = sdp_error(ss);
				return NULL;
			}
			sdp_close(ss);

			rv = (*cfgtype[i].handler)(dict);
			if (rv != 0)
				return NULL;

			return dict;
		}
	}

	printf("Known config types:\n");
	for (i = 0 ; i < NUM(cfgtype) ; i++)
		printf("\t%s\t%s\n", cfgtype[i].name, cfgtype[i].description);

	exit(EXIT_FAILURE);
}

/*
 * Configure HID results
 */
static int
config_hid(prop_dictionary_t dict)
{
	prop_object_t obj;
	int32_t control_psm, interrupt_psm,
		reconnect_initiate, battery_power,
		normally_connectable, hid_length;
	uint8_t *hid_descriptor;
	int i;

	control_psm = -1;
	interrupt_psm = -1;
	reconnect_initiate = -1;
	normally_connectable = 0;
	battery_power = 0;
	hid_descriptor = NULL;
	hid_length = -1;

	for (i = 0; i < NUM(values) ; i++) {
		if (values[i].flags != SDP_ATTR_OK)
			continue;

		switch (values[i].attr) {
		case SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST:
			control_psm = parse_l2cap_psm(&values[i]);
			break;

		case SDP_ATTR_ADDITIONAL_PROTOCOL_DESCRIPTOR_LISTS:
			interrupt_psm = parse_l2cap_psm(&values[i]);
			break;

		case 0x0205: /* HIDReconnectInitiate */
			reconnect_initiate = parse_boolean(&values[i]);
			break;

		case 0x0206: /* HIDDescriptorList */
			if (parse_hid_descriptor(&values[i]) == 0) {
				hid_descriptor = values[i].value;
				hid_length = values[i].vlen;
			}
			break;

		case 0x0209: /* HIDBatteryPower */
			battery_power = parse_boolean(&values[i]);
			break;

		case 0x020d: /* HIDNormallyConnectable */
			normally_connectable = parse_boolean(&values[i]);
			break;
		}
	}

	if (control_psm == -1
	    || interrupt_psm == -1
	    || reconnect_initiate == -1
	    || hid_descriptor == NULL
	    || hid_length == -1)
		return ENOATTR;

	obj = prop_string_create_cstring_nocopy("bthidev");
	if (obj == NULL || !prop_dictionary_set(dict, BTDEVtype, obj))
		return errno;

	prop_object_release(obj);

	obj = prop_number_create_integer(control_psm);
	if (obj == NULL || !prop_dictionary_set(dict, BTHIDEVcontrolpsm, obj))
		return errno;

	prop_object_release(obj);

	obj = prop_number_create_integer(interrupt_psm);
	if (obj == NULL || !prop_dictionary_set(dict, BTHIDEVinterruptpsm, obj))
		return errno;

	prop_object_release(obj);

	obj = prop_data_create_data(hid_descriptor, hid_length);
	if (obj == NULL || !prop_dictionary_set(dict, BTHIDEVdescriptor, obj))
		return errno;

	prop_object_release(obj);

	if (!reconnect_initiate) {
		obj = prop_bool_create(TRUE);
		if (obj == NULL || !prop_dictionary_set(dict, BTHIDEVreconnect, obj))
			return errno;

		prop_object_release(obj);
	}

	return 0;
}

/*
 * Configure HSET results
 */
static int
config_hset(prop_dictionary_t dict)
{
	prop_object_t obj;
	uint32_t channel;
	int i;

	channel = -1;

	for (i = 0; i < NUM(values) ; i++) {
		if (values[i].flags != SDP_ATTR_OK)
			continue;

		switch (values[i].attr) {
		case SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST:
			channel = parse_rfcomm_channel(&values[i]);
			break;
		}
	}

	if (channel == -1)
		return ENOATTR;

	obj = prop_string_create_cstring_nocopy("btsco");
	if (obj == NULL || !prop_dictionary_set(dict, BTDEVtype, obj))
		return errno;

	prop_object_release(obj);

	obj = prop_number_create_integer(channel);
	if (obj == NULL || !prop_dictionary_set(dict, BTSCOchannel, obj))
		return errno;

	prop_object_release(obj);

	return 0;
}

/*
 * Configure HF results
 */
static int
config_hf(prop_dictionary_t dict)
{
	prop_object_t obj;
	uint32_t channel;
	int i;

	channel = -1;

	for (i = 0 ; i < NUM(values) ; i++) {
		if (values[i].flags != SDP_ATTR_OK)
			continue;

		switch (values[i].attr) {
		case SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST:
			channel = parse_rfcomm_channel(&values[i]);
			break;
		}
	}

	if (channel == -1)
		return ENOATTR;

	obj = prop_string_create_cstring_nocopy("btsco");
	if (obj == NULL || !prop_dictionary_set(dict, BTDEVtype, obj))
		return errno;

	prop_object_release(obj);

	obj = prop_bool_create(TRUE);
	if (obj == NULL || !prop_dictionary_set(dict, BTSCOlisten, obj))
		return errno;

	prop_object_release(obj);

	obj = prop_number_create_integer(channel);
	if (obj == NULL || !prop_dictionary_set(dict, BTSCOchannel, obj))
		return errno;

	prop_object_release(obj);

	return 0;
}

/*
 * Parse [additional] protocol descriptor list for L2CAP PSM
 *
 * seq8 len8				2
 *	seq8 len8			2
 *		uuid16 value16		3	L2CAP
 *		uint16 value16		3	PSM
 *	seq8 len8			2
 *		uuid16 value16		3	HID Protocol
 *				      ===
 *				       15
 */

static int32_t
parse_l2cap_psm(sdp_attr_t *a)
{
	uint8_t	*ptr = a->value;
	uint8_t	*end = a->value + a->vlen;
	int32_t	 type, len, uuid, psm;

	if (end - ptr < 15)
		return (-1);

	if (a->attr == SDP_ATTR_ADDITIONAL_PROTOCOL_DESCRIPTOR_LISTS) {
		SDP_GET8(type, ptr);
		switch (type) {
		case SDP_DATA_SEQ8:
			SDP_GET8(len, ptr);
			break;

		case SDP_DATA_SEQ16:
			SDP_GET16(len, ptr);
			break;

		case SDP_DATA_SEQ32:
			SDP_GET32(len, ptr);
			break;

		default:
			return (-1);
		}
		if (ptr + len > end)
			return (-1);
	}

	SDP_GET8(type, ptr);
	switch (type) {
	case SDP_DATA_SEQ8:
		SDP_GET8(len, ptr);
		break;

	case SDP_DATA_SEQ16:
		SDP_GET16(len, ptr);
		break;

	case SDP_DATA_SEQ32:
		SDP_GET32(len, ptr);
		break;

	default:
		return (-1);
	}
	if (ptr + len > end)
		return (-1);

	/* Protocol */
	SDP_GET8(type, ptr);
	switch (type) {
	case SDP_DATA_SEQ8:
		SDP_GET8(len, ptr);
		break;

	case SDP_DATA_SEQ16:
		SDP_GET16(len, ptr);
		break;

	case SDP_DATA_SEQ32:
		SDP_GET32(len, ptr);
		break;

	default:
		return (-1);
	}
	if (ptr + len > end)
		return (-1);

	/* UUID */
	if (ptr + 3 > end)
		return (-1);
	SDP_GET8(type, ptr);
	switch (type) {
	case SDP_DATA_UUID16:
		SDP_GET16(uuid, ptr);
		if (uuid != SDP_UUID_PROTOCOL_L2CAP)
			return (-1);
		break;

	case SDP_DATA_UUID32:  /* XXX FIXME can we have 32-bit UUID */
	case SDP_DATA_UUID128: /* XXX FIXME can we have 128-bit UUID */
	default:
		return (-1);
	}

	/* PSM */
	if (ptr + 3 > end)
		return (-1);
	SDP_GET8(type, ptr);
	if (type != SDP_DATA_UINT16)
		return (-1);
	SDP_GET16(psm, ptr);

	return (psm);
}

/*
 * Parse HID descriptor string
 *
 * seq8 len8			2
 *	seq8 len8		2
 *		uint8 value8	2
 *		str value	3
 *			      ===
 *			        9
 */

static int32_t
parse_hid_descriptor(sdp_attr_t *a)
{
	uint8_t	*ptr = a->value;
	uint8_t	*end = a->value + a->vlen;
	int32_t	 type, len, descriptor_type;

	if (end - ptr < 9)
		return (-1);

	SDP_GET8(type, ptr);
	switch (type) {
	case SDP_DATA_SEQ8:
		SDP_GET8(len, ptr);
		break;

	case SDP_DATA_SEQ16:
		SDP_GET16(len, ptr);
		break;

	case SDP_DATA_SEQ32:
		SDP_GET32(len, ptr);
		break;

	default:
		return (-1);
	}
	if (ptr + len > end)
		return (-1);

	while (ptr < end) {
		/* Descriptor */
		SDP_GET8(type, ptr);
		switch (type) {
		case SDP_DATA_SEQ8:
			if (ptr + 1 > end)
				return (-1);
			SDP_GET8(len, ptr);
			break;

		case SDP_DATA_SEQ16:
			if (ptr + 2 > end)
				return (-1);
			SDP_GET16(len, ptr);
			break;

		case SDP_DATA_SEQ32:
			if (ptr + 4 > end)
				return (-1);
			SDP_GET32(len, ptr);
			break;

		default:
			return (-1);
		}

		/* Descripor type */
		if (ptr + 1 > end)
			return (-1);
		SDP_GET8(type, ptr);
		if (type != SDP_DATA_UINT8 || ptr + 1 > end)
			return (-1);
		SDP_GET8(descriptor_type, ptr);

		/* Descriptor value */
		if (ptr + 1 > end)
			return (-1);
		SDP_GET8(type, ptr);
		switch (type) {
		case SDP_DATA_STR8:
			if (ptr + 1 > end)
				return (-1);
			SDP_GET8(len, ptr);
			break;

		case SDP_DATA_STR16:
			if (ptr + 2 > end)
				return (-1);
			SDP_GET16(len, ptr);
			break;

		case SDP_DATA_STR32:
			if (ptr + 4 > end)
				return (-1);
			SDP_GET32(len, ptr);
			break;

		default:
			return (-1);
		}
		if (ptr + len > end)
			return (-1);

		if (descriptor_type == UDESC_REPORT && len > 0) {
			a->value = ptr;
			a->vlen = len;

			return (0);
		}

		ptr += len;
	}

	return (-1);
}

/*
 * Parse boolean value
 *
 * bool8 int8
 */

static int32_t
parse_boolean(sdp_attr_t *a)
{
	if (a->vlen != 2 || a->value[0] != SDP_DATA_BOOL)
		return (-1);

	return (a->value[1]);
}

/*
 * Parse protocol descriptor list for the RFCOMM channel
 *
 * seq8 len8				2
 *	seq8 len8			2
 *		uuid16 value16		3	L2CAP
 *	seq8 len8			2
 *		uuid16 value16		3	RFCOMM
 *		uint8 value8		2	channel
 *				      ===
 *				       14
 */

static int32_t
parse_rfcomm_channel(sdp_attr_t *a)
{
	uint8_t	*ptr = a->value;
	uint8_t	*end = a->value + a->vlen;
	int32_t	 type, len, uuid, channel;

	if (end - ptr < 14)
		return (-1);

	SDP_GET8(type, ptr);
	switch (type) {
	case SDP_DATA_SEQ8:
		SDP_GET8(len, ptr);
		break;

	case SDP_DATA_SEQ16:
		SDP_GET16(len, ptr);
		break;

	case SDP_DATA_SEQ32:
		SDP_GET32(len, ptr);
		break;

	default:
		return (-1);
	}
	if (ptr + len > end)
		return (-1);

	/* Protocol */
	SDP_GET8(type, ptr);
	switch (type) {
	case SDP_DATA_SEQ8:
		SDP_GET8(len, ptr);
		break;

	case SDP_DATA_SEQ16:
		SDP_GET16(len, ptr);
		break;

	case SDP_DATA_SEQ32:
		SDP_GET32(len, ptr);
		break;

	default:
		return (-1);
	}
	if (ptr + len > end)
		return (-1);

	/* UUID */
	if (ptr + 3 > end)
		return (-1);
	SDP_GET8(type, ptr);
	switch (type) {
	case SDP_DATA_UUID16:
		SDP_GET16(uuid, ptr);
		if (uuid != SDP_UUID_PROTOCOL_L2CAP)
			return (-1);
		break;

	case SDP_DATA_UUID32:  /* XXX FIXME can we have 32-bit UUID */
	case SDP_DATA_UUID128: /* XXX FIXME can we have 128-bit UUID */
	default:
		return (-1);
	}

	/* Protocol */
	SDP_GET8(type, ptr);
	switch (type) {
	case SDP_DATA_SEQ8:
		SDP_GET8(len, ptr);
		break;

	case SDP_DATA_SEQ16:
		SDP_GET16(len, ptr);
		break;

	case SDP_DATA_SEQ32:
		SDP_GET32(len, ptr);
		break;

	default:
		return (-1);
	}
	if (ptr + len > end)
		return (-1);

	/* UUID */
	if (ptr + 3 > end)
		return (-1);
	SDP_GET8(type, ptr);
	switch (type) {
	case SDP_DATA_UUID16:
		SDP_GET16(uuid, ptr);
		if (uuid != SDP_UUID_PROTOCOL_RFCOMM)
			return (-1);
		break;

	case SDP_DATA_UUID32:  /* XXX FIXME can we have 32-bit UUID */
	case SDP_DATA_UUID128: /* XXX FIXME can we have 128-bit UUID */
	default:
		return (-1);
	}

	/* channel */
	if (ptr + 2 > end)
		return (-1);

	SDP_GET8(type, ptr);
	if (type != SDP_DATA_UINT8)
		return (-1);

	SDP_GET8(channel, ptr);

	return (channel);
}
