/*	$NetBSD: print.c,v 1.5 2006/09/28 09:11:04 he Exp $	*/

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Iain Hibbert for Itronix Inc.
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
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: print.c,v 1.5 2006/09/28 09:11:04 he Exp $");

#include <sys/types.h>

#include <dev/bluetooth/btdev.h>
#include <dev/bluetooth/bthidev.h>
#include <dev/bluetooth/btsco.h>
#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>

#include <prop/proplib.h>

#include <bluetooth.h>
#include <err.h>
#include <malloc.h>
#include <usbhid.h>
#include <stdlib.h>

#include "btdevctl.h"

static void cfg_bthidev(prop_dictionary_t dict);
static void cfg_btsco(prop_dictionary_t dict);

static void hid_dump_item(char const *, struct hid_item *);
static void hid_parse(prop_data_t);

void
cfg_print(prop_dictionary_t dict)
{
	prop_object_t obj;
	char *p;

	obj = prop_dictionary_get(dict, BTDEVladdr);
	if (prop_object_type(obj) != PROP_TYPE_DATA) {
		return;
	}
	printf("local bdaddr: %s\n", bt_ntoa(prop_data_data_nocopy(obj), NULL));

	obj = prop_dictionary_get(dict, BTDEVraddr);
	if (prop_object_type(obj) != PROP_TYPE_DATA) {
		return;
	}
	printf("remote bdaddr: %s\n", bt_ntoa(prop_data_data_nocopy(obj), NULL));

	obj = prop_dictionary_get(dict, BTDEVtype);
	if (prop_object_type(obj) != PROP_TYPE_STRING) {
		printf("No device type!\n");
		return;
	}
	p =  prop_string_cstring(obj);
	printf("device type: %s\n", p);
	free(p);

	if (prop_string_equals_cstring(obj, "bthidev")) {
		cfg_bthidev(dict);
		return;
	}

	if (prop_string_equals_cstring(obj, "btsco")) {
		cfg_btsco(dict);
		return;
	}

	printf("Unknown device type!\n");
}

static void
cfg_bthidev(prop_dictionary_t dict)
{
	prop_object_t obj;

	obj = prop_dictionary_get(dict, BTHIDEVcontrolpsm);
	if (prop_object_type(obj) == PROP_TYPE_NUMBER)
		printf("control psm: 0x%4.4" PRIx64 "\n",
			prop_number_integer_value(obj));

	obj = prop_dictionary_get(dict, BTHIDEVinterruptpsm);
	if (prop_object_type(obj) == PROP_TYPE_NUMBER)
		printf("interrupt psm: 0x%4.4" PRIx64 "\n",
			prop_number_integer_value(obj));

	obj = prop_dictionary_get(dict, BTHIDEVreconnect);
	if (prop_bool_true(obj))
		printf("reconnect mode: TRUE\n");

	obj = prop_dictionary_get(dict, BTHIDEVdescriptor);
	if (prop_object_type(obj) == PROP_TYPE_DATA)
		hid_parse(obj);
}

static void
cfg_btsco(prop_dictionary_t dict)
{
	prop_object_t obj;

	obj = prop_dictionary_get(dict, BTSCOlisten);
	printf("mode: %s\n", prop_bool_true(obj) ? "listen" : "connect");

	obj = prop_dictionary_get(dict, BTSCOchannel);
	if (prop_object_type(obj) == PROP_TYPE_NUMBER)
		printf("channel: %" PRId64 "\n",
			prop_number_integer_value(obj));
}

static void
hid_parse(prop_data_t desc)
{
	report_desc_t		 r;
	hid_data_t		 d;
	struct hid_item		 h;

	hid_init(NULL);

	r = hid_use_report_desc((unsigned char *)prop_data_data_nocopy(desc),
				prop_data_size(desc));
	if (r == NULL)
		return;

	d = hid_start_parse(r, ~0, -1);
	while (hid_get_item(d, &h)) {
		switch (h.kind) {
		case hid_collection:
			printf("Collection page=%s usage=%s\n",
				hid_usage_page(HID_PAGE(h.usage)),
				hid_usage_in_page(h.usage));
			break;

		case hid_endcollection:
			printf("End collection\n");
			break;

		case hid_input:
			hid_dump_item("  Input", &h);
			break;

		case hid_output:
			hid_dump_item(" Output", &h);
			break;

		case hid_feature:
			hid_dump_item("Feature", &h);
			break;
		}
	}

	hid_end_parse(d);
	hid_dispose_report_desc(r);
}

static void
hid_dump_item(char const *label, struct hid_item *h)
{

	printf("%s id=%u size=%u count=%u page=%s usage=%s%s%s%s%s%s%s%s%s%s",
		label, (uint8_t) h->report_ID, h->report_size, h->report_count,
		hid_usage_page(HID_PAGE(h->usage)),
		hid_usage_in_page(h->usage),
		(h->flags & HIO_CONST) ? " Const" : "",
		(h->flags & HIO_VARIABLE) ? " Variable" : "",
		(h->flags & HIO_RELATIVE) ? " Relative" : "",
		(h->flags & HIO_WRAP) ? " Wrap" : "",
		(h->flags & HIO_NONLINEAR) ? " NonLinear" : "",
		(h->flags & HIO_NOPREF) ? " NoPref" : "",
		(h->flags & HIO_NULLSTATE) ? " NullState" : "",
		(h->flags & HIO_VOLATILE) ? " Volatile" : "",
		(h->flags & HIO_BUFBYTES) ? " BufBytes" : "");

	printf(", logical range %d..%d",
		h->logical_minimum, h->logical_maximum);

	if (h->physical_minimum != h->physical_maximum)
		printf(", physical range %d..%d",
			h->physical_minimum, h->physical_maximum);

	if (h->unit)
		printf(", unit=0x%02x exp=%d", h->unit, h->unit_exponent);

	printf("\n");
}
