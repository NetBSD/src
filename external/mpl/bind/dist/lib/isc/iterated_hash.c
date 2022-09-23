/*	$NetBSD: iterated_hash.c,v 1.6 2022/09/23 12:15:33 christos Exp $	*/

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

#include <stdio.h>

#include <isc/iterated_hash.h>
#include <isc/md.h>
#include <isc/util.h>

int
isc_iterated_hash(unsigned char *out, const unsigned int hashalg,
		  const int iterations, const unsigned char *salt,
		  const int saltlength, const unsigned char *in,
		  const int inlength) {
	isc_md_t *md;
	isc_result_t result;
	int n = 0;
	unsigned int outlength = 0;
	size_t len;
	const unsigned char *buf;

	REQUIRE(out != NULL);

	if (hashalg != 1) {
		return (0);
	}

	if ((md = isc_md_new()) == NULL) {
		return (0);
	}

	len = inlength;
	buf = in;
	do {
		result = isc_md_init(md, ISC_MD_SHA1);
		if (result != ISC_R_SUCCESS) {
			goto md_fail;
		}
		result = isc_md_update(md, buf, len);
		if (result != ISC_R_SUCCESS) {
			goto md_fail;
		}
		result = isc_md_update(md, salt, saltlength);
		if (result != ISC_R_SUCCESS) {
			goto md_fail;
		}
		result = isc_md_final(md, out, &outlength);
		if (result != ISC_R_SUCCESS) {
			goto md_fail;
		}
		result = isc_md_reset(md);
		if (result != ISC_R_SUCCESS) {
			goto md_fail;
		}
		buf = out;
		len = outlength;
	} while (n++ < iterations);

	isc_md_free(md);

	return (outlength);
md_fail:
	isc_md_free(md);
	return (0);
}
#undef RETERR
