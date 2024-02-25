/*	$NetBSD: jemalloc_shim.h,v 1.2.2.2 2024/02/25 15:47:16 martin Exp $	*/

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

#if defined(HAVE_MALLOC_SIZE) || defined(HAVE_MALLOC_USABLE_SIZE)

#include <stdlib.h>

static inline void *
mallocx(size_t size, int flags) {
	UNUSED(flags);

	return (malloc(size));
}

static inline void
sdallocx(void *ptr, size_t size, int flags) {
	UNUSED(size);
	UNUSED(flags);

	free(ptr);
}

static inline void *
rallocx(void *ptr, size_t size, int flags) {
	UNUSED(flags);
	REQUIRE(size != 0);

	return (realloc(ptr, size));
}

#ifdef HAVE_MALLOC_SIZE

#include <malloc/malloc.h>

static inline size_t
sallocx(void *ptr, int flags) {
	UNUSED(flags);

	return (malloc_size(ptr));
}

#elif HAVE_MALLOC_USABLE_SIZE

#ifdef __DragonFly__
/*
 * On DragonFly BSD 'man 3 malloc' advises us to include the following
 * header to have access to malloc_usable_size().
 */
#include <malloc_np.h>
#else
#include <malloc.h>
#endif

static inline size_t
sallocx(void *ptr, int flags) {
	UNUSED(flags);

	return (malloc_usable_size(ptr));
}

#endif /* HAVE_MALLOC_SIZE */

#else /* defined(HAVE_MALLOC_SIZE) || defined (HAVE_MALLOC_USABLE_SIZE) */

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
sdallocx(void *ptr, size_t size, int flags) {
	size_info *si = &(((size_info *)ptr)[-1]);

	UNUSED(size);
	UNUSED(flags);

	free(si);
}

static inline size_t
sallocx(void *ptr, int flags) {
	size_info *si = &(((size_info *)ptr)[-1]);

	UNUSED(flags);

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

#endif /* defined(HAVE_MALLOC_SIZE) || defined (HAVE_MALLOC_USABLE_SIZE) */

#endif /* !defined(HAVE_JEMALLOC) */
