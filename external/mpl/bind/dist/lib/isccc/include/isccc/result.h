/*	$NetBSD: result.h,v 1.5 2022/09/23 12:15:35 christos Exp $	*/

/*
 * Copyright (C) 2001 Nominum, Inc.
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

#ifndef ISCCC_RESULT_H
#define ISCCC_RESULT_H 1

/*! \file isccc/result.h */

#include <isc/lang.h>
#include <isc/result.h>
#include <isc/resultclass.h>

#include <isccc/types.h>

/*% Unknown Version */
#define ISCCC_R_UNKNOWNVERSION (ISC_RESULTCLASS_ISCCC + 0)
/*% Syntax Error */
#define ISCCC_R_SYNTAX (ISC_RESULTCLASS_ISCCC + 1)
/*% Bad Authorization */
#define ISCCC_R_BADAUTH (ISC_RESULTCLASS_ISCCC + 2)
/*% Expired */
#define ISCCC_R_EXPIRED (ISC_RESULTCLASS_ISCCC + 3)
/*% Clock Skew */
#define ISCCC_R_CLOCKSKEW (ISC_RESULTCLASS_ISCCC + 4)
/*% Duplicate */
#define ISCCC_R_DUPLICATE (ISC_RESULTCLASS_ISCCC + 5)

#define ISCCC_R_NRESULTS 6 /*%< Number of results */

ISC_LANG_BEGINDECLS

const char *
isccc_result_totext(isc_result_t result);
/*%
 * Convert a isccc_result_t into a string message describing the result.
 */

void
isccc_result_register(void);

ISC_LANG_ENDDECLS

#endif /* ISCCC_RESULT_H */
