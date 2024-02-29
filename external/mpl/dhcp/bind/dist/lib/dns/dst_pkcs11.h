/*	$NetBSD: dst_pkcs11.h,v 1.1.4.2 2024/02/29 11:38:39 martin Exp $	*/

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

#ifndef DST_PKCS11_H
#define DST_PKCS11_H 1

#include <isc/lang.h>
#include <isc/log.h>
#include <isc/result.h>

ISC_LANG_BEGINDECLS

isc_result_t
dst__pkcs11_toresult(const char *funcname, const char *file, int line,
		     isc_result_t fallback, CK_RV rv);

#define PK11_CALL(func, args, fallback)                                 \
	((void)(((rv = (func)args) == CKR_OK) ||                        \
		((ret = dst__pkcs11_toresult(#func, __FILE__, __LINE__, \
					     fallback, rv)),            \
		 0)))

#define PK11_RET(func, args, fallback)                                  \
	((void)(((rv = (func)args) == CKR_OK) ||                        \
		((ret = dst__pkcs11_toresult(#func, __FILE__, __LINE__, \
					     fallback, rv)),            \
		 0)));                                                  \
	if (rv != CKR_OK)                                               \
		goto err;

ISC_LANG_ENDDECLS

#endif /* DST_PKCS11_H */
