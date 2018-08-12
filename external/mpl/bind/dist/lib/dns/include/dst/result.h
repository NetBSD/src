/*	$NetBSD: result.h,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

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


#ifndef DST_RESULT_H
#define DST_RESULT_H 1

/*! \file dst/result.h */

#include <isc/lang.h>
#include <isc/resultclass.h>

/*
 * Nothing in this file truly depends on <isc/result.h>, but the
 * DST result codes are considered to be publicly derived from
 * the ISC result codes, so including this file buys you the ISC_R_
 * namespace too.
 */
#include <isc/result.h>		/* Contractual promise. */

#define DST_R_UNSUPPORTEDALG		(ISC_RESULTCLASS_DST + 0)
#define DST_R_CRYPTOFAILURE		(ISC_RESULTCLASS_DST + 1)
/* compat */
#define DST_R_OPENSSLFAILURE		DST_R_CRYPTOFAILURE
#define DST_R_NOCRYPTO			(ISC_RESULTCLASS_DST + 2)
#define DST_R_NULLKEY			(ISC_RESULTCLASS_DST + 3)
#define DST_R_INVALIDPUBLICKEY		(ISC_RESULTCLASS_DST + 4)
#define DST_R_INVALIDPRIVATEKEY		(ISC_RESULTCLASS_DST + 5)
/* 6 is unused */
#define DST_R_WRITEERROR		(ISC_RESULTCLASS_DST + 7)
#define DST_R_INVALIDPARAM		(ISC_RESULTCLASS_DST + 8)
/* 9 is unused */
/* 10 is unused */
#define DST_R_SIGNFAILURE		(ISC_RESULTCLASS_DST + 11)
/* 12 is unused */
/* 13 is unused */
#define DST_R_VERIFYFAILURE		(ISC_RESULTCLASS_DST + 14)
#define DST_R_NOTPUBLICKEY		(ISC_RESULTCLASS_DST + 15)
#define DST_R_NOTPRIVATEKEY		(ISC_RESULTCLASS_DST + 16)
#define DST_R_KEYCANNOTCOMPUTESECRET	(ISC_RESULTCLASS_DST + 17)
#define DST_R_COMPUTESECRETFAILURE	(ISC_RESULTCLASS_DST + 18)
#define DST_R_NORANDOMNESS		(ISC_RESULTCLASS_DST + 19)
#define DST_R_BADKEYTYPE		(ISC_RESULTCLASS_DST + 20)
#define DST_R_NOENGINE			(ISC_RESULTCLASS_DST + 21)
#define DST_R_EXTERNALKEY		(ISC_RESULTCLASS_DST + 22)

#define DST_R_NRESULTS			23	/* Number of results */

ISC_LANG_BEGINDECLS

const char *
dst_result_totext(isc_result_t);

void
dst_result_register(void);

ISC_LANG_ENDDECLS

#endif /* DST_RESULT_H */
