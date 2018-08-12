/*	$NetBSD: log.c,v 1.1.1.1 2018/08/12 12:07:38 christos Exp $	*/

/*
 * Copyright (C) 2009-2015  Red Hat ; see COPYRIGHT for license
 */

#include <config.h>

#include <isc/util.h>

#include <dns/log.h>

#include "log.h"

void
log_write(int level, const char *format, ...) {
	va_list args;

	va_start(args, format);
	isc_log_vwrite(dns_lctx, DNS_LOGCATEGORY_DATABASE, DNS_LOGMODULE_DYNDB,
		       level, format, args);
	va_end(args);
}
