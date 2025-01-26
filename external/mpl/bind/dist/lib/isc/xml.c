/*	$NetBSD: xml.c,v 1.2 2025/01/26 16:25:39 christos Exp $	*/

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

#include <isc/mem.h>
#include <isc/util.h>
#include <isc/xml.h>

#ifdef HAVE_LIBXML2
#include <libxml/parser.h>
#include <libxml/xmlversion.h>

static isc_mem_t *isc__xml_mctx = NULL;

static void *
isc__xml_malloc(size_t size) {
	return isc_mem_allocate(isc__xml_mctx, size);
}

static void *
isc__xml_realloc(void *ptr, size_t size) {
	return isc_mem_reallocate(isc__xml_mctx, ptr, size);
}

static char *
isc__xml_strdup(const char *str) {
	return isc_mem_strdup(isc__xml_mctx, str);
}

static void
isc__xml_free(void *ptr) {
	if (ptr == NULL) {
		return;
	}
	isc_mem_free(isc__xml_mctx, ptr);
}

#endif /* HAVE_LIBXML2 */

void
isc__xml_initialize(void) {
#ifdef HAVE_LIBXML2
	isc_mem_create(&isc__xml_mctx);
	isc_mem_setname(isc__xml_mctx, "libxml2");
	isc_mem_setdestroycheck(isc__xml_mctx, false);

	RUNTIME_CHECK(xmlMemSetup(isc__xml_free, isc__xml_malloc,
				  isc__xml_realloc, isc__xml_strdup) == 0);

	xmlInitParser();
#endif /* HAVE_LIBXML2 */
}

void
isc__xml_shutdown(void) {
#ifdef HAVE_LIBXML2
	xmlCleanupParser();
	isc_mem_destroy(&isc__xml_mctx);
#endif /* HAVE_LIBXML2 */
}

void
isc__xml_setdestroycheck(bool check) {
#if HAVE_LIBXML2
	isc_mem_setdestroycheck(isc__xml_mctx, check);
#else
	UNUSED(check);
#endif
}
