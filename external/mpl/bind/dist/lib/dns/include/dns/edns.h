/*	$NetBSD: edns.h,v 1.5.2.1 2024/02/25 15:46:56 martin Exp $	*/

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

#pragma once

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
#endif			   /* ifdef DRAFT_ANDREWS_EDNS1 */
