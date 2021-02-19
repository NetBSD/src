/*	$NetBSD: geoip.h,v 1.5 2021/02/19 16:42:10 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
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
