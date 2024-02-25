/*	$NetBSD: instance.h,v 1.5.2.1 2024/02/25 15:44:08 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0 AND ISC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*
 * Copyright (C) 2009-2015 Red Hat
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND AUTHORS DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * Driver instance object.
 */

#pragma once

#include <stdbool.h>

#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/types.h>

struct sample_instance {
	isc_mem_t *mctx;
	char *db_name;
	dns_dbimplementation_t *db_imp;

	/* These are needed for zone creation. */
	dns_view_t *view;
	dns_zonemgr_t *zmgr;
	isc_task_t *task;
	bool exiting;

	dns_zone_t *zone1;
	dns_fixedname_t zone1_fn;
	dns_name_t *zone1_name;

	dns_zone_t *zone2;
	dns_fixedname_t zone2_fn;
	dns_name_t *zone2_name;
};

typedef struct sample_instance sample_instance_t;

isc_result_t
new_sample_instance(isc_mem_t *mctx, const char *db_name, int argc, char **argv,
		    const dns_dyndbctx_t *dctx,
		    sample_instance_t **sample_instp);

isc_result_t
load_sample_instance_zones(sample_instance_t *inst);

void
destroy_sample_instance(sample_instance_t **sample_instp);
