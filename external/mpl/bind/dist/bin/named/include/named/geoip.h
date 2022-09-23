/*	$NetBSD: geoip.h,v 1.6 2022/09/23 12:15:21 christos Exp $	*/

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

extern dns_geoip_databases_t *named_g_geoip;

void
named_geoip_init(void);

void
named_geoip_load(char *dir);

void
named_geoip_unload(void);

void
named_geoip_shutdown(void);
