/*	$NetBSD: hid.c,v 1.1 2006/06/19 15:44:56 gdamore Exp $	*/

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
 * hid.c
 *
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
 * $Id: hid.c,v 1.1 2006/06/19 15:44:56 gdamore Exp $
 * $FreeBSD: src/usr.sbin/bluetooth/bthidcontrol/hid.c,v 1.1 2004/04/10 00:18:00 emax Exp $
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: hid.c,v 1.1 2006/06/19 15:44:56 gdamore Exp $");

#include <bluetooth.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <usbhid.h>
#include <unistd.h>

#include <dev/bluetooth/btdev.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>

#include "btcontrol.h"

static void hid_dump_item	(char const *, struct hid_item *);

int
hid_parse(bdaddr_t *laddr, bdaddr_t *raddr, int argc, char **argv)
{
	bt_cfgentry_t	*cfg;
	bt_handle_t	 handle;
	report_desc_t	 r;
	hid_data_t	 d;
	struct hid_item	 h;

	hid_init(NULL);

	handle = bt_openconfig(config_file);
	if (handle == NULL)
		err(EXIT_FAILURE, "Could not open config file");

	cfg = bt_getconfig(handle, raddr);
	if (cfg == NULL)
		errx(EXIT_FAILURE, "Config entry not found");

	if (cfg->hid_descriptor == NULL)
		errx(EXIT_FAILURE, "No HID descriptor");

	if (cfg->type != BTDEV_HID)
		warnx("Config entry type is not HID");

	r = hid_use_report_desc(cfg->hid_descriptor, cfg->hid_length);
	if (r == NULL)
		errx(EXIT_FAILURE, "Can't use hid_descriptor");

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

	bt_freeconfig(cfg);
	bt_closeconfig(handle);

	return EXIT_SUCCESS;
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
