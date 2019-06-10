/*	$NetBSD: check.h,v 1.3.2.2 2019/06/10 22:04:34 christos Exp $	*/

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


#ifndef BIND9_CHECK_H
#define BIND9_CHECK_H 1

/*! \file bind9/check.h */

#include <isc/lang.h>
#include <isc/types.h>

#include <isccfg/cfg.h>

#ifndef MAX_MIN_CACHE_TTL
#define MAX_MIN_CACHE_TTL 90
#endif /* MAX_MIN_CACHE_TTL */

#ifndef MAX_MIN_NCACHE_TTL
#define MAX_MIN_NCACHE_TTL 90
#endif /* MAX_MIN_NCACHE_TTL */

#ifndef MAX_MAX_NCACHE_TTL
#define MAX_MAX_NCACHE_TTL 7 * 24 * 3600
#endif /* MAX_MAX_NCACHE_TTL */

ISC_LANG_BEGINDECLS

isc_result_t
bind9_check_namedconf(const cfg_obj_t *config, bool check_plugins,
		      isc_log_t *logctx, isc_mem_t *mctx);
/*%<
 * Check the syntactic validity of a configuration parse tree generated from
 * a named.conf file.
 *
 * If 'check_plugins' is true, load plugins and check the validity of their
 * parameters as well.
 *
 * Requires:
 *\li	config is a valid parse tree
 *
 *\li	logctx is a valid logging context.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_FAILURE
 */

isc_result_t
bind9_check_key(const cfg_obj_t *config, isc_log_t *logctx);
/*%<
 * Same as bind9_check_namedconf(), but for a single 'key' statement.
 */

ISC_LANG_ENDDECLS

#endif /* BIND9_CHECK_H */
