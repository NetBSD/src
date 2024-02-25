/*	$NetBSD: rootns.h,v 1.5.2.1 2024/02/25 15:46:58 martin Exp $	*/

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

/*! \file dns/rootns.h */

#include <isc/lang.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

isc_result_t
dns_rootns_create(isc_mem_t *mctx, dns_rdataclass_t rdclass,
		  const char *filename, dns_db_t **target);

void
dns_root_checkhints(dns_view_t *view, dns_db_t *hints, dns_db_t *db);
/*
 * Reports differences between hints and the real roots.
 *
 * Requires view, hints and (cache) db to be valid.
 */

ISC_LANG_ENDDECLS
