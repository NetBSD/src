/*	$NetBSD: tls.h,v 1.1.1.1 2021/04/29 16:46:32 christos Exp $	*/

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

#include <isc/mem.h>
#include <isc/region.h>
#include <isc/result.h>
#include <isc/tls.h>
#include <isc/types.h>

typedef struct ssl_ctx_st isc_tlsctx_t;

void
isc_tlsctx_free(isc_tlsctx_t **ctpx);
/*%
 * Free the TLS client/server context.
 *
 * Require:
 *\li	'ctxp' != NULL and '*ctxp' != NULL.
 */

isc_result_t
isc_tlsctx_createserver(const char *keyfile, const char *certfile,
			isc_tlsctx_t **ctxp);
/*%
 * Set up TLS server context.
 *
 * Require:
 *\li	'ctxp' != NULL and '*ctxp' == NULL.
 */

isc_result_t
isc_tlsctx_createclient(isc_tlsctx_t **ctxp);
/*%
 * Set up TLS client context.
 *
 * Require:
 *\li	'ctxp' != NULL and '*ctxp' == NULL.
 */
