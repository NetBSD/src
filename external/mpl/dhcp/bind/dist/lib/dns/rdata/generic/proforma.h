/*	$NetBSD: proforma.h,v 1.1.4.2 2024/02/29 11:38:54 martin Exp $	*/

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

/* */
#ifndef GENERIC_PROFORMA_H
#define GENERIC_PROFORMA_H 1

typedef struct dns_rdata_ #{
	dns_rdatacommon_t common;
	isc_mem_t *mctx; /* if required */
			 /* type & class specific elements */
}
dns_rdata_ #_t;

#endif /* GENERIC_PROFORMA_H */
