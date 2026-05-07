/*	$NetBSD: dsync.h,v 1.2.2.2 2026/05/07 16:18:41 martin Exp $	*/

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

#define DNS_DSYNCSCHEME_NOTIFY (1)

#define DNS_DSYNCSCHEMEFORMAT_SIZE (7)

isc_result_t
dns_dsyncscheme_fromtext(dns_dsyncscheme_t *schemep, isc_textregion_t *source);

isc_result_t
dns_dsyncscheme_totext(dns_dsyncscheme_t scheme, isc_buffer_t *target);

void
dns_dsyncscheme_format(dns_dsyncscheme_t scheme, char *cp, unsigned int size);
