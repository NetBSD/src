/*	$NetBSD: fips.c,v 1.2 2025/01/26 16:25:37 christos Exp $	*/

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

/*! \file */

#include <isc/fips.h>
#include <isc/util.h>

#if defined(HAVE_EVP_DEFAULT_PROPERTIES_ENABLE_FIPS)
#include <openssl/evp.h>
bool
isc_fips_mode(void) {
	return EVP_default_properties_is_fips_enabled(NULL) != 0;
}

isc_result_t
isc_fips_set_mode(int mode) {
	return EVP_default_properties_enable_fips(NULL, mode) != 0
		       ? ISC_R_SUCCESS
		       : ISC_R_FAILURE;
}
#elif defined(HAVE_FIPS_MODE)
#include <openssl/crypto.h>

bool
isc_fips_mode(void) {
	return FIPS_mode() != 0;
}

isc_result_t
isc_fips_set_mode(int mode) {
	return FIPS_mode_set(mode) != 0 ? ISC_R_SUCCESS : ISC_R_FAILURE;
}
#else
bool
isc_fips_mode(void) {
	return false;
}

isc_result_t
isc_fips_set_mode(int mode) {
	UNUSED(mode);
	return ISC_R_NOTIMPLEMENTED;
}
#endif
