/*	$NetBSD: log.c,v 1.3 2020/05/24 19:46:16 christos Exp $	*/

/*
 * Copyright (C) 2009-2015  Red Hat ; see COPYRIGHT for license
 */

#include "log.h"

#include <isc/util.h>

#include <dns/log.h>

void
log_write(int level, const char *format, ...) {
	va_list args;

	va_start(args, format);
	isc_log_vwrite(dns_lctx, DNS_LOGCATEGORY_DATABASE, DNS_LOGMODULE_DYNDB,
		       level, format, args);
	va_end(args);
}
