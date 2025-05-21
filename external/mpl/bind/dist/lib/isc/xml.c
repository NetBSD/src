/*	$NetBSD: xml.c,v 1.3 2025/05/21 14:48:05 christos Exp $	*/

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

#ifndef LIBXML_HAS_DEPRECATED_MEMORY_ALLOCATION_FUNCTIONS
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

#endif /* !LIBXML_HAS_DEPRECATED_MEMORY_ALLOCATION_FUNCTIONS) */
#endif /* HAVE_LIBXML2 */

void
isc__xml_initialize(void) {
#ifdef HAVE_LIBXML2
#ifndef LIBXML_HAS_DEPRECATED_MEMORY_ALLOCATION_FUNCTIONS
	isc_mem_create(&isc__xml_mctx);
	isc_mem_setname(isc__xml_mctx, "libxml2");
	isc_mem_setdestroycheck(isc__xml_mctx, false);

	RUNTIME_CHECK(xmlMemSetup(isc__xml_free, isc__xml_malloc,
				  isc__xml_realloc, isc__xml_strdup) == 0);
#endif /* !LIBXML_HAS_DEPRECATED_MEMORY_ALLOCATION_FUNCTIONS */

	xmlInitParser();
#endif /* HAVE_LIBXML2 */
}

void
isc__xml_shutdown(void) {
#ifdef HAVE_LIBXML2
	xmlCleanupParser();

#ifndef LIBXML_HAS_DEPRECATED_MEMORY_ALLOCATION_FUNCTIONS
	isc_mem_destroy(&isc__xml_mctx);
#endif /* !LIBXML_HAS_DEPRECATED_MEMORY_ALLOCATION_FUNCTIONS */
#endif /* HAVE_LIBXML2 */
}

void
isc__xml_setdestroycheck(bool check ISC_ATTR_UNUSED) {
#ifdef HAVE_LIBXML2
#ifndef LIBXML_HAS_DEPRECATED_MEMORY_ALLOCATION_FUNCTIONS
	isc_mem_setdestroycheck(isc__xml_mctx, check);
#endif /* LIBXML_HAS_DEPRECATED_MEMORY_ALLOCATION_FUNCTIONS */
#endif /* HAVE_LIBXML2 */
}
