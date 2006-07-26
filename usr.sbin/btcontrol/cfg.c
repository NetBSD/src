/*	$NetBSD: cfg.c,v 1.2 2006/07/26 10:31:00 tron Exp $	*/

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
 *
 * $Id: cfg.c,v 1.2 2006/07/26 10:31:00 tron Exp $
 * $FreeBSD: src/usr.sbin/bluetooth/bthidcontrol/sdp.c,v 1.2 2006/02/10 19:54:17 markus Exp $
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: cfg.c,v 1.2 2006/07/26 10:31:00 tron Exp $");

#include <dev/bluetooth/btdev.h>
#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>
#include <prop/proplib.h>

#include <bluetooth.h>
#include <err.h>
#include <errno.h>
#include <sdp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <usbhid.h>

#include "btcontrol.h"

static int32_t parse_l2cap_psm(sdp_attr_t *);
static int32_t parse_rfcomm_channel(sdp_attr_t *);
static int32_t parse_hid_descriptor(sdp_attr_t *);
static int32_t parse_boolean(sdp_attr_t *);

static int config_hid(prop_dictionary_t);
static int config_hset(prop_dictionary_t);
static int config_hfp(prop_dictionary_t);

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

uint16_t hfp_services[] = {
	SDP_SERVICE_CLASS_HANDSFREE_AUDIO_GATEWAY
};

uint32_t hfp_attrs[] = {
	SDP_ATTR_RANGE(	SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST,
			SDP_ATTR_PROTOCOL_DESCRIPTOR_LIST),
};

#define NUM(v)		(sizeof(v) / sizeof(v[0]))

static struct {
	const char	*name;
	int		(*handler)(prop_dictionary_t);
	const char	*description;
	uint16_t	*services;
	int		nservices;
	uint32_t	*attrs;
	int		nattrs;
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
	"HFP",		config_hfp,	"Hands Free Profile",
	hfp_services,	NUM(hfp_services),
	hfp_attrs,	NUM(hfp_attrs),
    },
};

static sdp_attr_t	values[8];
static uint8_t		buffer[NUM(values)][512];
static int		mtu;

int
cfg_print(int ac, char **av)
{
	prop_dictionary_t cfg, dev;
	char *xml;

	cfg = read_config();
	if (cfg == NULL)
		return EXIT_FAILURE;

	dev = prop_dictionary_get(cfg, control_file);
	if (dev == NULL)
		errx(EXIT_FAILURE, "%s not found", control_file);

	if (prop_object_type(dev) != PROP_TYPE_DICTIONARY)
		errx(EXIT_FAILURE, "%s invalid config", control_file);

	xml = prop_dictionary_externalize(dev);
	if (xml == NULL)
		err(EXIT_FAILURE, "prop_dictionary_externalize");

	printf("%s", xml);

	free(xml);
	prop_object_release(cfg);
	return EXIT_SUCCESS;
}

int
cfg_query(int ac, char **av)
{
	bdaddr_t		 laddr, raddr;
	prop_dictionary_t	 cfg, dev;
	prop_object_t		 obj;
	void			*ss;
	char			*service;
	int			 rv, i, ch;

	bdaddr_copy(&laddr, BDADDR_ANY);
	bdaddr_copy(&raddr, BDADDR_ANY);
	service = NULL;
	mtu = 0;

	while ((ch = getopt(ac, av, "a:d:m:s:")) != -1) {
		switch (ch) {
		case 'a': /* remote address */
			if (!bt_aton(optarg, &raddr)) {
				struct hostent  *he = NULL;

				if ((he = bt_gethostbyname(optarg)) == NULL)
					errx(EXIT_FAILURE, "%s: %s",
						optarg, hstrerror(h_errno));

				bdaddr_copy(&raddr, (bdaddr_t *)he->h_addr);
			}
			break;

		case 'd': /* local device address */
			if (!bt_devaddr(optarg, &laddr))
				err(EXIT_FAILURE, "%s", optarg);

			break;

		case 'm': /* mtu for SCO Audio */
			mtu = atoi(optarg);
			break;

		case 's': /* service */
			service = optarg;
			break;

		default:
			err(EXIT_FAILURE, "unknown option -%c", ch);
		}
	}

	if (bdaddr_any(&raddr))
		errx(EXIT_FAILURE, "must specify remote device address");

	if (service == NULL)
		errx(EXIT_FAILURE, "must specify service to configure");

	dev = prop_dictionary_create();
	if (dev == NULL)
		err(EXIT_FAILURE, "prop_dictinary_create");

	if (!bdaddr_any(&laddr)) {
		obj = prop_data_create_data_nocopy(&laddr, sizeof(laddr));
		if (obj == NULL || !prop_dictionary_set(dev, "local-bdaddr", obj))
			err(EXIT_FAILURE, "proplib");;
	}

	obj = prop_data_create_data_nocopy(&raddr, sizeof(raddr));
	if (obj == NULL || !prop_dictionary_set(dev, "remote-bdaddr", obj))
		err(EXIT_FAILURE, "proplib");;

	for (i = 0 ; i < NUM(values) ; i++) {
		values[i].flags = SDP_ATTR_INVALID;
		values[i].attr = 0;
		values[i].vlen = sizeof(buffer[i]);
		values[i].value = buffer[i];
	}

	for (i = 0 ; i < NUM(cfgtype) ; i++) {
		if (strcasecmp(service, cfgtype[i].name) == 0) {
			ss = sdp_open(&laddr, &raddr);

			if (ss == NULL || (errno = sdp_error(ss)) != 0)
				err(EXIT_FAILURE, "sdp_open");

			rv = sdp_search(ss,
				cfgtype[i].nservices, cfgtype[i].services,
				cfgtype[i].nattrs, cfgtype[i].attrs,
				NUM(values), values);

			if (rv != 0) {
				errno = sdp_error(ss);
				err(EXIT_FAILURE, "sdp_search");
			}
			sdp_close(ss);

			rv = (*cfgtype[i].handler)(dev);
			if (rv != 0)
				errx(EXIT_FAILURE, "%s: %s", cfgtype[i].name, strerror(rv));

			cfg = read_config();
			if (cfg == NULL && errno == ENOENT)
				cfg = prop_dictionary_create();

			if (cfg == NULL)
				err(EXIT_FAILURE, "%s", config_file);

			if (!prop_dictionary_set(cfg, control_file, dev))
				err(EXIT_FAILURE, "prop_dictionary_set");

			if (!write_config(cfg))
				err(EXIT_FAILURE, "%s", config_file);

			prop_object_release(cfg);
			return EXIT_SUCCESS;
		}
	}

	printf("Known config types:\n");
	for (i = 0 ; i < NUM(cfgtype) ; i++)
		printf("\t%s\t%s\n", cfgtype[i].name, cfgtype[i].description);

	return EXIT_FAILURE;
}

int
cfg_remove(int ac, char **av)
{
	prop_dictionary_t cfg, dev;

	cfg = read_config();
	if (cfg == NULL)
		return EXIT_FAILURE;

	dev = prop_dictionary_get(cfg, control_file);
	if (dev == NULL)
		errx(EXIT_FAILURE, "%s not found", control_file);

	prop_dictionary_remove(cfg, control_file);

	if (!write_config(cfg))
		err(EXIT_FAILURE, "%s", config_file);

	prop_object_release(cfg);
	return EXIT_SUCCESS;
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
	if (obj == NULL || !prop_dictionary_set(dict, "device-type", obj))
		return errno;

	obj = prop_number_create_integer(control_psm);
	if (obj == NULL || !prop_dictionary_set(dict, "control-psm", obj))
		return errno;

	obj = prop_number_create_integer(interrupt_psm);
	if (obj == NULL || !prop_dictionary_set(dict, "interrupt-psm", obj))
		return errno;

	obj = prop_data_create_data(hid_descriptor, hid_length);
	if (obj == NULL || !prop_dictionary_set(dict, "descriptor", obj))
		return errno;

	if (!reconnect_initiate) {
		obj = prop_bool_create(TRUE);
		if (obj == NULL || !prop_dictionary_set(dict, "reconnect", obj))
			return errno;
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
	if (obj == NULL || !prop_dictionary_set(dict, "device-type", obj))
		return errno;

	obj = prop_number_create_integer(channel);
	if (obj == NULL || !prop_dictionary_set(dict, "rfcomm-channel", obj))
		return errno;

	if (mtu != 0) {
		obj = prop_number_create_integer(mtu);
		if (obj == NULL || !prop_dictionary_set(dict, "mtu", obj))
			return errno;
	}

	return 0;
}

/*
 * Configure HFP results
 */
static int
config_hfp(prop_dictionary_t dict)
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
	if (obj == NULL || !prop_dictionary_set(dict, "device-type", obj))
		return errno;

	obj = prop_bool_create(TRUE);
	if (obj == NULL || !prop_dictionary_set(dict, "listen", obj))
		return errno;

	obj = prop_number_create_integer(channel);
	if (obj == NULL || !prop_dictionary_set(dict, "rfcomm-channel", obj))
		return errno;

	if (mtu != 0) {
		obj = prop_number_create_integer(mtu);
		if (obj == NULL || !prop_dictionary_set(dict, "mtu", obj))
			return errno;
	}

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
