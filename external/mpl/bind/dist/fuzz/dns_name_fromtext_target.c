/*	$NetBSD: dns_name_fromtext_target.c,v 1.5 2022/09/23 12:15:29 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <stddef.h>
#include <stdint.h>

#include <isc/buffer.h>
#include <isc/util.h>

#include <dns/fixedname.h>
#include <dns/name.h>

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
	isc_buffer_t buf;
	isc_result_t result;
	dns_fixedname_t origin;
	char *de_const;

	if (size < 5) {
		return (0);
	}

	dns_fixedname_init(&origin);
	DE_CONST(data, de_const);
	isc_buffer_init(&buf, (void *)de_const, size);
	isc_buffer_add(&buf, size);
	result = dns_name_fromtext(dns_fixedname_name(&origin), &buf,
				   dns_rootname, 0, NULL);
	UNUSED(result);
	return (0);
}
