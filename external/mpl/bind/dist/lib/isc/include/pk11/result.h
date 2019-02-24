/*	$NetBSD: result.h,v 1.3 2019/02/24 20:01:31 christos Exp $	*/

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

#ifndef PK11_RESULT_H
#define PK11_RESULT_H 1

/*! \file pk11/result.h */

#include <isc/lang.h>
#include <isc/resultclass.h>

/*
 * Nothing in this file truly depends on <isc/result.h>, but the
 * PK11 result codes are considered to be publicly derived from
 * the ISC result codes, so including this file buys you the ISC_R_
 * namespace too.
 */
#include <isc/result.h>		/* Contractual promise. */

#define PK11_R_INITFAILED		(ISC_RESULTCLASS_PK11 + 0)
#define PK11_R_NOPROVIDER		(ISC_RESULTCLASS_PK11 + 1)
#define PK11_R_NORANDOMSERVICE		(ISC_RESULTCLASS_PK11 + 2)
#define PK11_R_NODIGESTSERVICE		(ISC_RESULTCLASS_PK11 + 3)
#define PK11_R_NOAESSERVICE		(ISC_RESULTCLASS_PK11 + 4)

#define PK11_R_NRESULTS			5	/* Number of results */

ISC_LANG_BEGINDECLS

const char *
pk11_result_totext(isc_result_t);

void
pk11_result_register(void);

ISC_LANG_ENDDECLS

#endif /* PK11_RESULT_H */
