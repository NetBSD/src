/*	$NetBSD: callbacks.c,v 1.1.1.1 2018/08/12 12:08:09 christos Exp $	*/

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


/*! \file */

#include <config.h>

#include <isc/print.h>
#include <isc/util.h>

#include <dns/callbacks.h>
#include <dns/log.h>

static void
stdio_error_warn_callback(dns_rdatacallbacks_t *, const char *, ...)
     ISC_FORMAT_PRINTF(2, 3);

static void
isclog_error_callback(dns_rdatacallbacks_t *callbacks, const char *fmt, ...)
     ISC_FORMAT_PRINTF(2, 3);

static void
isclog_warn_callback(dns_rdatacallbacks_t *callbacks, const char *fmt, ...)
     ISC_FORMAT_PRINTF(2, 3);

/*
 * Private
 */

static void
stdio_error_warn_callback(dns_rdatacallbacks_t *callbacks,
			  const char *fmt, ...)
{
	va_list ap;

	UNUSED(callbacks);

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

static void
isclog_error_callback(dns_rdatacallbacks_t *callbacks, const char *fmt, ...) {
	va_list ap;

	UNUSED(callbacks);

	va_start(ap, fmt);
	isc_log_vwrite(dns_lctx, DNS_LOGCATEGORY_GENERAL,
		       DNS_LOGMODULE_MASTER, /* XXX */
		       ISC_LOG_ERROR, fmt, ap);
	va_end(ap);
}

static void
isclog_warn_callback(dns_rdatacallbacks_t *callbacks, const char *fmt, ...) {
	va_list ap;

	UNUSED(callbacks);

	va_start(ap, fmt);

	isc_log_vwrite(dns_lctx, DNS_LOGCATEGORY_GENERAL,
		       DNS_LOGMODULE_MASTER, /* XXX */
		       ISC_LOG_WARNING, fmt, ap);
	va_end(ap);
}

static void
dns_rdatacallbacks_initcommon(dns_rdatacallbacks_t *callbacks) {
	REQUIRE(callbacks != NULL);

	callbacks->magic = DNS_CALLBACK_MAGIC;
	callbacks->add = NULL;
	callbacks->rawdata = NULL;
	callbacks->zone = NULL;
	callbacks->add_private = NULL;
	callbacks->error_private = NULL;
	callbacks->warn_private = NULL;
}

/*
 * Public.
 */

void
dns_rdatacallbacks_init(dns_rdatacallbacks_t *callbacks) {
	dns_rdatacallbacks_initcommon(callbacks);
	callbacks->error = isclog_error_callback;
	callbacks->warn = isclog_warn_callback;
}

void
dns_rdatacallbacks_init_stdio(dns_rdatacallbacks_t *callbacks) {
	dns_rdatacallbacks_initcommon(callbacks);
	callbacks->error = stdio_error_warn_callback;
	callbacks->warn = stdio_error_warn_callback;
}

