/*	$NetBSD: types.h,v 1.1.1.1 2018/08/12 12:07:44 christos Exp $	*/

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

#ifndef NAMED_TYPES_H
#define NAMED_TYPES_H 1

/*! \file */

#include <dns/types.h>

typedef struct named_cache		named_cache_t;
typedef ISC_LIST(named_cache_t)		named_cachelist_t;
typedef struct named_server 		named_server_t;
typedef struct named_xmld		named_xmld_t;
typedef struct named_xmldmgr		named_xmldmgr_t;
typedef struct named_controls		named_controls_t;
typedef struct named_dispatch		named_dispatch_t;
typedef ISC_LIST(named_dispatch_t)	named_dispatchlist_t;
typedef struct named_statschannel	named_statschannel_t;
typedef ISC_LIST(named_statschannel_t)	named_statschannellist_t;

#endif /* NAMED_TYPES_H */
