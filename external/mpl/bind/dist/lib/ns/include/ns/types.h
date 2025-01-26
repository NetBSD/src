/*	$NetBSD: types.h,v 1.9 2025/01/26 16:25:46 christos Exp $	*/

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

typedef struct ns_altsecret ns_altsecret_t;
typedef ISC_LIST(ns_altsecret_t) ns_altsecretlist_t;
typedef struct ns_client    ns_client_t;
typedef struct ns_clientmgr ns_clientmgr_t;
typedef struct ns_plugin    ns_plugin_t;
typedef ISC_LIST(ns_plugin_t) ns_plugins_t;
typedef struct ns_interface    ns_interface_t;
typedef struct ns_interfacemgr ns_interfacemgr_t;
typedef struct ns_query	       ns_query_t;
typedef struct ns_server       ns_server_t;
typedef struct ns_stats	       ns_stats_t;
typedef struct ns_hookasync    ns_hookasync_t;

typedef enum { ns_cookiealg_siphash24 } ns_cookiealg_t;

#define NS_COOKIE_VERSION_1 1
