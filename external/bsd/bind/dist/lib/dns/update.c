/*	$NetBSD: update.c,v 1.1.1.1 2011/09/11 17:18:37 christos Exp $	*/

/*
 * Copyright (C) 2011  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* Id: update.c,v 1.3 2011-07-01 23:47:44 tbox Exp */

#include "config.h"

#include <isc/stdtime.h>
#include <isc/serial.h>

#include <dns/update.h>

isc_uint32_t
dns_update_soaserial(isc_uint32_t serial, dns_updatemethod_t method) {
	isc_stdtime_t now;

	if (method == dns_updatemethod_unixtime) {
		isc_stdtime_get(&now);
		if (now != 0 && isc_serial_gt(now, serial))
			return (now);
	}

	/* RFC1982 */
	serial = (serial + 1) & 0xFFFFFFFF;
	if (serial == 0)
		serial = 1;

	return (serial);
}
