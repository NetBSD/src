/*	$NetBSD: edns.h,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

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

#ifndef DNS_EDNS_H
#define DNS_EDNS_H 1

/*%
 * The maximum version on EDNS supported by this build.
 */
#define DNS_EDNS_VERSION 0
#ifdef DRAFT_ANDREWS_EDNS1
#undef DNS_EDNS_VERSION
/*
 * Warning: this currently disables sending COOKIE requests in resolver.c
 */
#define DNS_EDNS_VERSION 1 /* draft-andrews-edns1 */
#endif

#endif
