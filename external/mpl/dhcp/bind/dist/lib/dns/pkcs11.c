/*	$NetBSD: pkcs11.c,v 1.1.2.2 2024/02/24 13:07:00 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#if USE_PKCS11

#include <isc/util.h>

#include <pk11/internal.h>
#include <pk11/pk11.h>

#include <dns/log.h>
#include <dns/result.h>

#include "dst_internal.h"
#include "dst_pkcs11.h"

isc_result_t
dst__pkcs11_toresult(const char *funcname, const char *file, int line,
		     isc_result_t fallback, CK_RV rv) {
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL, DNS_LOGMODULE_CRYPTO,
		      ISC_LOG_WARNING, "%s:%d: %s: Error = 0x%.8lX\n", file,
		      line, funcname, rv);
	if (rv == CKR_HOST_MEMORY) {
		return (ISC_R_NOMEMORY);
	}
	return (fallback);
}

#endif /* USE_PKCS11 */
/*! \file */
