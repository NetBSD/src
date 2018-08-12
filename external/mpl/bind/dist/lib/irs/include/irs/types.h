/*	$NetBSD: types.h,v 1.2 2018/08/12 13:02:37 christos Exp $	*/

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


#ifndef IRS_TYPES_H
#define IRS_TYPES_H 1

/* Core Types.  Alphabetized by defined type. */

/*%< per-thread IRS context */
typedef struct irs_context		irs_context_t;
/*%< resolv.conf configuration information */
typedef struct irs_resconf		irs_resconf_t;
/*%< advanced DNS-related configuration information */
typedef struct irs_dnsconf		irs_dnsconf_t;

#endif /* IRS_TYPES_H */
