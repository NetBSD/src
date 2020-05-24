/*	$NetBSD: geoip.h,v 1.4 2020/05/24 19:46:12 christos Exp $	*/

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
