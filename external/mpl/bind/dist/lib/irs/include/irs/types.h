/*	$NetBSD: types.h,v 1.5 2022/09/23 12:15:32 christos Exp $	*/

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

#ifndef IRS_TYPES_H
#define IRS_TYPES_H 1

/* Core Types.  Alphabetized by defined type. */

/*%< per-thread IRS context */
typedef struct irs_context irs_context_t;
/*%< resolv.conf configuration information */
typedef struct irs_resconf irs_resconf_t;
/*%< advanced DNS-related configuration information */
typedef struct irs_dnsconf irs_dnsconf_t;

#endif /* IRS_TYPES_H */
