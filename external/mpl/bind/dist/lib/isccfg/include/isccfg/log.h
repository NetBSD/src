/*	$NetBSD: log.h,v 1.1.1.1 2018/08/12 12:08:28 christos Exp $	*/

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


#ifndef ISCCFG_LOG_H
#define ISCCFG_LOG_H 1

/*! \file isccfg/log.h */

#include <isc/lang.h>
#include <isc/log.h>

LIBISCCFG_EXTERNAL_DATA extern isc_logcategory_t cfg_categories[];
LIBISCCFG_EXTERNAL_DATA extern isc_logmodule_t cfg_modules[];

#define CFG_LOGCATEGORY_CONFIG	(&cfg_categories[0])

#define CFG_LOGMODULE_PARSER	(&cfg_modules[0])

ISC_LANG_BEGINDECLS

void
cfg_log_init(isc_log_t *lctx);
/*%<
 * Make the libisccfg categories and modules available for use with the
 * ISC logging library.
 *
 * Requires:
 *\li	lctx is a valid logging context.
 *
 *\li	cfg_log_init() is called only once.
 *
 * Ensures:
 * \li	The categories and modules defined above are available for
 * 	use by isc_log_usechannnel() and isc_log_write().
 */

ISC_LANG_ENDDECLS

#endif /* ISCCFG_LOG_H */
