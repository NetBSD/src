/*	$NetBSD: sdp.c,v 1.7 2009/05/12 18:39:20 plunky Exp $	*/

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
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: sdp.c,v 1.7 2009/05/12 18:39:20 plunky Exp $");

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
#include <strings.h>
#include <usbhid.h>

#include "btdevctl.h"

static bool parse_hid_descriptor(sdp_data_t *);
static int32_t parse_boolean(sdp_data_t *);
static int32_t parse_pdl_param(sdp_data_t *, uint16_t);
static int32_t parse_pdl(sdp_data_t *, uint16_t);
static int32_t parse_apdl(sdp_data_t *, uint16_t);

static int config_hid(prop_dictionary_t, sdp_data_t *);
static int config_hset(prop_dictionary_t, sdp_data_t *);
static int config_hf(prop_dictionary_t, sdp_data_t *);

uint16_t hid_services[] = {
	SDP_SERVICE_CLASS_HUMAN_INTERFACE_DEVICE,
};

uint16_t hset_services[] = {
	SDP_SERVICE_CLASS_HEADSET,
};

uint16_t hf_services[] = {
	SDP_SERVICE_CLASS_HANDSFREE_AUDIO_GATEWAY,
};

static struct {
	const char		*name;
	int			(*handler)(prop_dictionary_t, sdp_data_t *);
	const char		*description;
	uint16_t		*services;
	size_t			nservices;
} cfgtype[] = {
    {
	"HID",		config_hid,	"Human Interface Device",
	hid_services,	__arraycount(hid_services),
    },
    {
	"HSET",		config_hset,	"Headset",
	hset_services,	__arraycount(hset_services),
    },
    {
	"HF",		config_hf,	"Handsfree",
	hf_services,	__arraycount(hf_services),
    },
};

#define MAX_SSP		(2 + 1 * 3)	/* largest nservices is 1 */

prop_dictionary_t
cfg_query(bdaddr_t *laddr, bdaddr_t *raddr, const char *service)
{
	prop_dictionary_t dict;
	sdp_session_t ss;
	uint8_t buf[MAX_SSP];
	sdp_data_t ssp, rsp, rec;
	size_t i, n;
	bool rv;

	dict = prop_dictionary_create();
	if (dict == NULL)
		return NULL;

	for (i = 0; i < __arraycount(cfgtype); i++) {
		if (strcasecmp(service, cfgtype[i].name) == 0) {
			ss = sdp_open(laddr, raddr);
			if (ss == NULL)
				return NULL;

			/* build ServiceSearchPattern */
			ssp.next = buf;
			ssp.end = buf + sizeof(buf);

			for (n = 0; n < cfgtype[i].nservices; n++)
				sdp_put_uuid16(&ssp, cfgtype[i].services[n]);

			ssp.end = ssp.next;
			ssp.next = buf;

			rv = sdp_service_search_attribute(ss, &ssp, NULL, &rsp);
			if (!rv) {
				prop_object_release(dict);
				sdp_close(ss);
				return NULL;
			}

			while (sdp_get_seq(&rsp, &rec)) {
				errno = (*cfgtype[i].handler)(dict, &rec);
				if (errno == 0) {
					sdp_close(ss);
					return dict;
				}
			}

			sdp_close(ss);
			prop_object_release(dict);
			return NULL;
		}
	}

	printf("Known config types:\n");
	for (i = 0; i < __arraycount(cfgtype); i++)
		printf("\t%s\t%s\n", cfgtype[i].name, cfgtype[i].description);

	exit(EXIT_FAILURE);
}

/*
 * Configure HID results
 */
static int
config_hid(prop_dictionary_t dict, sdp_data_t *rec)
{
	prop_object_t obj;
	int32_t control_psm, interrupt_psm,
		reconnect_initiate, hid_length;
	uint8_t *hid_descriptor;
	sdp_data_t value;
	const char *mode;
	uint16_t attr;

	control_psm = -1;
	interrupt_psm = -1;
	reconnect_initiate = -1;
	hid_descriptor = NULL;
	hid_length = -1;

	while (sdp_get_attr(rec, &attr, &value)) {
		switch (attr) {
		case SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST:
			control_psm = parse_pdl(&value, SDP_UUID_PROTOCOL_L2CAP);
			break;

		case SDP_ATTR_ADDITIONAL_PROTOCOL_DESCRIPTOR_LISTS:
			interrupt_psm = parse_apdl(&value, SDP_UUID_PROTOCOL_L2CAP);
			break;

		case 0x0205: /* HIDReconnectInitiate */
			reconnect_initiate = parse_boolean(&value);
			break;

		case 0x0206: /* HIDDescriptorList */
			if (parse_hid_descriptor(&value)) {
				hid_descriptor = value.next;
				hid_length = value.end - value.next;
			}
			break;

		default:
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

	mode = hid_mode(obj);
	prop_object_release(obj);

	obj = prop_string_create_cstring_nocopy(mode);
	if (obj == NULL || !prop_dictionary_set(dict, BTDEVmode, obj))
		return errno;

	prop_object_release(obj);

	if (!reconnect_initiate) {
		obj = prop_bool_create(true);
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
config_hset(prop_dictionary_t dict, sdp_data_t *rec)
{
	prop_object_t obj;
	sdp_data_t value;
	int32_t channel;
	uint16_t attr;

	channel = -1;

	while (sdp_get_attr(rec, &attr, &value)) {
		switch (attr) {
		case SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST:
			channel = parse_pdl(&value, SDP_UUID_PROTOCOL_RFCOMM);
			break;

		default:
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
config_hf(prop_dictionary_t dict, sdp_data_t *rec)
{
	prop_object_t obj;
	sdp_data_t value;
	int32_t channel;
	uint16_t attr;

	channel = -1;

	while (sdp_get_attr(rec, &attr, &value)) {
		switch (attr) {
		case SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST:
			channel = parse_pdl(&value, SDP_UUID_PROTOCOL_RFCOMM);
			break;

		default:
			break;
		}
	}

	if (channel == -1)
		return ENOATTR;

	obj = prop_string_create_cstring_nocopy("btsco");
	if (obj == NULL || !prop_dictionary_set(dict, BTDEVtype, obj))
		return errno;

	prop_object_release(obj);

	obj = prop_bool_create(true);
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
 * Parse HIDDescriptorList . This is a sequence of HIDDescriptors, of which
 * each is a data element sequence containing, minimally, a ClassDescriptorType
 * and ClassDescriptorData containing a byte array of data. Any extra elements
 * should be ignored.
 *
 * If a ClassDescriptorType "Report" is found, set SDP data value to the
 * ClassDescriptorData content and return true. Note that we don't need to
 * extract the actual length as the SDP data is guaranteed valid.
 */

static bool
parse_hid_descriptor(sdp_data_t *value)
{
	sdp_data_t list, desc;
	uintmax_t type;
	char *str;
	size_t len;

	if (!sdp_get_seq(value, &list))
		return false;

	while (sdp_get_seq(&list, &desc)) {
		if (sdp_get_uint(&desc, &type)
		    && type == UDESC_REPORT
		    && sdp_get_str(&desc, &str, &len)) {
			value->next = (uint8_t *)str;
			value->end = (uint8_t *)(str + len);
			return true;
		}
	}

	return false;
}

static int32_t
parse_boolean(sdp_data_t *value)
{
	bool bv;

	if (!sdp_get_bool(value, &bv))
		return -1;

	return bv;
}

/*
 * The ProtocolDescriptorList attribute describes one or
 * more protocol stacks that may be used to gain access to
 * the service dscribed by the service record.
 *
 * If the ProtocolDescriptorList describes a single stack,
 * the attribute value takes the form of a data element
 * sequence in which each element of the sequence is a
 * protocol descriptor.
 *
 *	seq
 *	  <list>
 *
 * If it is possible for more than one kind of protocol
 * stack to be used to gain access to the service, the
 * ProtocolDescriptorList takes the form of a data element
 * alternative where each member is a data element sequence
 * consisting of a list of sequences describing each protocol
 *
 *	alt
 *	  seq
 *	    <list>
 *	  seq
 *	    <list>
 *
 * Each ProtocolDescriptorList is a list containing a sequence for
 * each protocol, where each sequence contains the protocol UUUID
 * and any protocol specific parameters.
 *
 *	seq
 *	  uuid		L2CAP
 *	  uint16	psm
 *	seq
 *	  uuid		RFCOMM
 *	  uint8		channel
 *
 * We want to extract the ProtocolSpecificParameter#1 for the
 * given protocol, which will be an unsigned int.
 */
static int32_t
parse_pdl_param(sdp_data_t *pdl, uint16_t proto)
{
	sdp_data_t seq;
	uintmax_t param;

	while (sdp_get_seq(pdl, &seq)) {
		if (!sdp_match_uuid16(&seq, proto))
			continue;

		if (sdp_get_uint(&seq, &param))
			return param;

		break;
	}

	return -1;
}

static int32_t
parse_pdl(sdp_data_t *value, uint16_t proto)
{
	sdp_data_t seq;
	int32_t param = -1;

	sdp_get_alt(value, value);	/* strip any alt header */

	while (param == -1 && sdp_get_seq(value, &seq))
		param = parse_pdl_param(&seq, proto);

	return param;
}

/*
 * Parse AdditionalProtocolDescriptorList
 */
static int32_t
parse_apdl(sdp_data_t *value, uint16_t proto)
{
	sdp_data_t seq;
	int32_t param = -1;

	sdp_get_seq(value, value);	/* strip seq header */

	while (param == -1 && sdp_get_seq(value, &seq))
		param = parse_pdl_param(&seq, proto);

	return param;
}

/*
 * return appropriate mode for HID descriptor
 */
const char *
hid_mode(prop_data_t desc)
{
	report_desc_t r;
	hid_data_t d;
	struct hid_item h;
	const char *mode;

	hid_init(NULL);

	mode = BTDEVauth;	/* default */

	r = hid_use_report_desc(prop_data_data_nocopy(desc),
				prop_data_size(desc));
	if (r == NULL)
		err(EXIT_FAILURE, "hid_use_report_desc");

	d = hid_start_parse(r, ~0, -1);
	while (hid_get_item(d, &h) > 0) {
		if (h.kind == hid_collection
		    && HID_PAGE(h.usage) == HUP_GENERIC_DESKTOP
		    && HID_USAGE(h.usage) == HUG_KEYBOARD)
			mode = BTDEVencrypt;
	}

	hid_end_parse(d);
	hid_dispose_report_desc(r);

	return mode;
}
