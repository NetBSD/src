/*	$NetBSD: callbacks.c,v 1.5 2014/12/10 04:37:58 christos Exp $	*/

/*
 * Copyright (C) 2004, 2005, 2007, 2011, 2012  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* Id: callbacks.c,v 1.19.40.1 2012/02/07 00:44:13 each Exp  */

/*! \file */

#include <config.h>

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

