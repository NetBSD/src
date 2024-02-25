/*	$NetBSD: types.h,v 1.5.2.1 2024/02/25 15:43:07 martin Exp $	*/

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

/*! \file */

#include <dns/types.h>

typedef struct named_cache named_cache_t;
typedef ISC_LIST(named_cache_t) named_cachelist_t;
typedef struct named_server   named_server_t;
typedef struct named_xmld     named_xmld_t;
typedef struct named_xmldmgr  named_xmldmgr_t;
typedef struct named_controls named_controls_t;
typedef struct named_dispatch named_dispatch_t;
typedef ISC_LIST(named_dispatch_t) named_dispatchlist_t;
typedef struct named_statschannel named_statschannel_t;
typedef ISC_LIST(named_statschannel_t) named_statschannellist_t;

/*%
 * Used for server->reload_status as printed by `rndc status`
 */
typedef enum {
	NAMED_RELOAD_DONE,
	NAMED_RELOAD_IN_PROGRESS,
	NAMED_RELOAD_FAILED,
} named_reload_t;
