/*	$NetBSD: driver.c,v 1.7.2.1 2024/02/25 15:44:08 martin Exp $	*/

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
 * Copyright (C) Red Hat
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

/*
 * Driver API implementation and main entry point for BIND.
 *
 * BIND calls dyndb_version() before loading, dyndb_init() during startup
 * and dyndb_destroy() during shutdown.
 *
 * It is completely up to implementation what to do.
 *
 * dyndb <name> <driver> {} sections in named.conf are independent so
 * driver init() and destroy() functions are called independently for
 * each section even if they reference the same driver/library. It is
 * up to driver implementation to detect and catch this situation if
 * it is undesirable.
 */

#include <isc/commandline.h>
#include <isc/hash.h>
#include <isc/mem.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/dyndb.h>
#include <dns/types.h>

#include "db.h"
#include "instance.h"
#include "log.h"
#include "util.h"

/* aliases for the exported symbols */

dns_dyndb_destroy_t dyndb_destroy;
dns_dyndb_register_t dyndb_init;
dns_dyndb_version_t dyndb_version;

/*
 * Driver init is called for each dyndb section in named.conf
 * once during startup and then again on every reload.
 *
 * @code
 * dyndb example-name "sample.so" { param1 param2 };
 * @endcode
 *
 * @param[in] name        User-defined string from dyndb "name" {}; definition
 *                        in named.conf.
 *                        The example above will have name = "example-name".
 * @param[in] parameters  User-defined parameters from dyndb section as one
 *                        string. The example above will have
 *                        params = "param1 param2";
 * @param[in] file	  The name of the file from which the parameters
 *                        were read.
 * @param[in] line	  The line number from which the parameters were read.
 * @param[out] instp      Pointer to instance-specific data
 *                        (for one dyndb section).
 */
isc_result_t
dyndb_init(isc_mem_t *mctx, const char *name, const char *parameters,
	   const char *file, unsigned long line, const dns_dyndbctx_t *dctx,
	   void **instp) {
	isc_result_t result;
	unsigned int argc;
	char **argv = NULL;
	char *s = NULL;
	sample_instance_t *sample_inst = NULL;

	REQUIRE(name != NULL);
	REQUIRE(dctx != NULL);

	s = isc_mem_strdup(mctx, parameters);

	result = isc_commandline_strtoargv(mctx, s, &argc, &argv, 0);
	if (result != ISC_R_SUCCESS) {
		log_write(ISC_LOG_ERROR,
			  "dyndb_init: isc_commandline_strtoargv -> %s\n",
			  isc_result_totext(result));
		goto cleanup;
	}

	log_write(ISC_LOG_DEBUG(9), "loading params for dyndb '%s' from %s:%lu",
		  name, file, line);

	/* Finally, create the instance. */
	result = new_sample_instance(mctx, name, argc, argv, dctx,
				     &sample_inst);
	if (result != ISC_R_SUCCESS) {
		log_write(ISC_LOG_ERROR,
			  "dyndb_init: new_sample_instance -> %s\n",
			  isc_result_totext(result));
		goto cleanup;
	}

	/*
	 * This is an example so we create and load zones
	 * right now.  This step can be arbitrarily postponed.
	 */
	result = load_sample_instance_zones(sample_inst);
	if (result != ISC_R_SUCCESS) {
		log_write(ISC_LOG_ERROR,
			  "dyndb_init: load_sample_instance_zones -> %s\n",
			  isc_result_totext(result));
		goto cleanup;
	}

	*instp = sample_inst;

cleanup:
	isc_mem_free(mctx, s);
	if (argv != NULL) {
		isc_mem_put(mctx, argv, argc * sizeof(*argv));
	}

	return (result);
}

/*
 * Driver destroy is called for every instance on every reload and then once
 * during shutdown.
 *
 * @param[out] instp Pointer to instance-specific data (for one dyndb section).
 */
void
dyndb_destroy(void **instp) {
	destroy_sample_instance((sample_instance_t **)instp);
}

/*
 * Driver version is called when loading the driver to ensure there
 * is no API mismatch between the driver and the caller.
 */
int
dyndb_version(unsigned int *flags) {
	UNUSED(flags);

	return (DNS_DYNDB_VERSION);
}
