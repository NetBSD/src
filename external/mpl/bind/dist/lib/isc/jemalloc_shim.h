/*	$NetBSD: jemalloc_shim.h,v 1.3 2024/09/22 00:14:08 christos Exp $	*/

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

#pragma once

#if !defined(HAVE_JEMALLOC)

#include <stddef.h>

#include <isc/util.h>

const char *malloc_conf = NULL;

/* Without jemalloc, isc_mem_get_align() is equal to isc_mem_get() */
#define MALLOCX_ALIGN(a)    (a & 0) /* Clear the flag */
#define MALLOCX_ZERO	    ((int)0x40)
#define MALLOCX_TCACHE_NONE (0)
#define MALLOCX_ARENA(a)    (0)

#include <stdlib.h>

typedef union {
	size_t size;
	max_align_t __alignment;
} size_info;

static inline void *
mallocx(size_t size, int flags) {
	void *ptr = NULL;

	UNUSED(flags);

	size_info *si = malloc(size + sizeof(*si));
	INSIST(si != NULL);

	si->size = size;
	ptr = &si[1];

	return (ptr);
}

static inline void
sdallocx(void *ptr, size_t size ISC_ATTR_UNUSED, int flags ISC_ATTR_UNUSED) {
	size_info *si = &(((size_info *)ptr)[-1]);

	free(si);
}

static inline size_t
sallocx(void *ptr, int flags ISC_ATTR_UNUSED) {
	size_info *si = &(((size_info *)ptr)[-1]);

	return (si[0].size);
}

static inline void *
rallocx(void *ptr, size_t size, int flags) {
	size_info *si = &(((size_info *)ptr)[-1]);

	UNUSED(flags);

	si = realloc(si, size + sizeof(*si));
	INSIST(si != NULL);

	si->size = size;
	ptr = &si[1];

	return (ptr);
}

#endif /* !defined(HAVE_JEMALLOC) */
