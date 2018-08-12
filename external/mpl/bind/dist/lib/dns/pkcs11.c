/*	$NetBSD: pkcs11.c,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifdef PKCS11CRYPTO

#include <config.h>

#include <isc/util.h>

#include <dns/log.h>
#include <dns/result.h>

#include <pk11/pk11.h>
#include <pk11/internal.h>

#include "dst_internal.h"
#include "dst_pkcs11.h"

isc_result_t
dst__pkcs11_toresult(const char *funcname, const char *file, int line,
		     isc_result_t fallback, CK_RV rv)
{
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
		      DNS_LOGMODULE_CRYPTO, ISC_LOG_WARNING,
		      "%s:%d: %s: Error = 0x%.8lX\n",
		      file, line, funcname, rv);
	if (rv == CKR_HOST_MEMORY)
		return (ISC_R_NOMEMORY);
	return (fallback);
}

isc_result_t
dst_random_getdata(void *data, unsigned int length,
		   unsigned int *returned, unsigned int flags) {
#ifdef ISC_PLATFORM_CRYPTORANDOM
	isc_result_t ret;

#ifndef DONT_REQUIRE_DST_LIB_INIT
	INSIST(dst__memory_pool != NULL);
#endif
	REQUIRE(data != NULL);
	REQUIRE(length > 0);
	UNUSED(flags);

	ret = pk11_rand_bytes(data, (int) length);
	if ((ret == ISC_R_SUCCESS) && (returned != NULL))
		*returned = length;
	return (ret);
#else
	UNUSED(data);
	UNUSED(length);
	UNUSED(returned);
	UNUSED(flags);

	return (ISC_R_NOTIMPLEMENTED);
#endif
}

#else /* PKCS11CRYPTO */

#include <isc/util.h>

isc_result_t
dst_random_getdata(void *data, unsigned int length,
		   unsigned int *returned, unsigned int flags) {
	UNUSED(data);
	UNUSED(length);
	UNUSED(returned);
	UNUSED(flags);

	return (ISC_R_NOTIMPLEMENTED);
}

#endif /* PKCS11CRYPTO */
/*! \file */
