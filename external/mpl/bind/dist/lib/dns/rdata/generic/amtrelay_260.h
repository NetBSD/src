/*	$NetBSD: amtrelay_260.h,v 1.1.1.1 2019/02/24 18:56:52 christos Exp $	*/

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


#ifndef GENERIC_AMTRELAY_260_H
#define GENERIC_AMTRELAY_260_H 1

typedef struct dns_rdata_amtrelay {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
	uint8_t			precedence;
	bool			discovery;
	uint8_t			gateway_type;
	struct in_addr		in_addr;	/* gateway type 1 */
	struct in6_addr		in6_addr;	/* gateway type 2 */
	dns_name_t		gateway;	/* gateway type 3 */
	unsigned char		*data;		/* gateway type > 3 */
	uint16_t		length;
} dns_rdata_amtrelay_t;

#endif /* GENERIC_AMTRELAY_260_H */
