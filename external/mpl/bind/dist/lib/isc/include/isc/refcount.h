/*	$NetBSD: refcount.h,v 1.6.2.1 2024/02/25 15:47:22 martin Exp $	*/

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

#include <inttypes.h>

#include <isc/assertions.h>
#include <isc/atomic.h>
#include <isc/error.h>
#include <isc/lang.h>
#include <isc/mutex.h>
#include <isc/types.h>

/*! \file isc/refcount.h
 * \brief Implements a locked reference counter.
 *
 * These macros uses C11(-like) atomic functions to implement reference
 * counting.  The isc_refcount_t type must not be accessed directly.
 */

ISC_LANG_BEGINDECLS

typedef atomic_uint_fast32_t isc_refcount_t;

/** \def isc_refcount_init(ref, n)
 *  \brief Initialize the reference counter.
 *  \param[in] ref pointer to reference counter.
 *  \param[in] n an initial number of references.
 *  \return nothing.
 *
 *  \warning No memory barrier are being imposed here.
 */
#define isc_refcount_init(target, value) atomic_init(target, value)

/** \def isc_refcount_current(ref)
 *  \brief Returns current number of references.
 *  \param[in] ref pointer to reference counter.
 *  \returns current value of reference counter.
 *
 *   Undo implicit promotion to 64 bits in our Windows implementation of
 *   atomic_load_explicit() by casting to uint_fast32_t.
 */

#define isc_refcount_current(target) (uint_fast32_t) atomic_load_acquire(target)

/** \def isc_refcount_destroy(ref)
 *  \brief a destructor that makes sure that all references were cleared.
 *  \param[in] ref pointer to reference counter.
 *  \returns nothing.
 */
#define isc_refcount_destroy(target) \
	ISC_REQUIRE(isc_refcount_current(target) == 0)

/** \def isc_refcount_increment0(ref)
 *  \brief increases reference counter by 1.
 *  \param[in] ref pointer to reference counter.
 *  \returns previous value of reference counter.
 */
#if _MSC_VER
static inline uint_fast32_t
isc_refcount_increment0(isc_refcount_t *target) {
	uint_fast32_t __v;
	__v = (uint_fast32_t)atomic_fetch_add_relaxed(target, 1);
	INSIST(__v < UINT32_MAX);
	return (__v);
}
#else /* _MSC_VER */
#define isc_refcount_increment0(target)                    \
	({                                                 \
		uint_fast32_t __v;                         \
		__v = atomic_fetch_add_relaxed(target, 1); \
		INSIST(__v < UINT32_MAX);                  \
		__v;                                       \
	})
#endif /* _MSC_VER */

/** \def isc_refcount_increment(ref)
 *  \brief increases reference counter by 1.
 *  \param[in] ref pointer to reference counter.
 *  \returns previous value of reference counter.
 */
#if _MSC_VER
static inline uint_fast32_t
isc_refcount_increment(isc_refcount_t *target) {
	uint_fast32_t __v;
	__v = (uint_fast32_t)atomic_fetch_add_relaxed(target, 1);
	INSIST(__v > 0 && __v < UINT32_MAX);
	return (__v);
}
#else /* _MSC_VER */
#define isc_refcount_increment(target)                     \
	({                                                 \
		uint_fast32_t __v;                         \
		__v = atomic_fetch_add_relaxed(target, 1); \
		INSIST(__v > 0 && __v < UINT32_MAX);       \
		__v;                                       \
	})
#endif /* _MSC_VER */

/** \def isc_refcount_decrement(ref)
 *  \brief decreases reference counter by 1.
 *  \param[in] ref pointer to reference counter.
 *  \returns previous value of reference counter.
 */
#if _MSC_VER
static inline uint_fast32_t
isc_refcount_decrement(isc_refcount_t *target) {
	uint_fast32_t __v;
	__v = (uint_fast32_t)atomic_fetch_sub_acq_rel(target, 1);
	INSIST(__v > 0);
	return (__v);
}
#else /* _MSC_VER */
#define isc_refcount_decrement(target)                     \
	({                                                 \
		uint_fast32_t __v;                         \
		__v = atomic_fetch_sub_acq_rel(target, 1); \
		INSIST(__v > 0);                           \
		__v;                                       \
	})
#endif /* _MSC_VER */

#define isc_refcount_decrementz(target)                               \
	do {                                                          \
		uint_fast32_t _refs = isc_refcount_decrement(target); \
		ISC_INSIST(_refs == 1);                               \
	} while (0)

#define isc_refcount_decrement1(target)                               \
	do {                                                          \
		uint_fast32_t _refs = isc_refcount_decrement(target); \
		ISC_INSIST(_refs > 1);                                \
	} while (0)

#define isc_refcount_decrement0(target)                               \
	do {                                                          \
		uint_fast32_t _refs = isc_refcount_decrement(target); \
		ISC_INSIST(_refs > 0);                                \
	} while (0)

#define ISC_REFCOUNT_TRACE_DECL(name)                                         \
	name##_t *name##__ref(name##_t *ptr, const char *func,                \
			      const char *file, unsigned int line);           \
	void name##__unref(name##_t *ptr, const char *func, const char *file, \
			   unsigned int line);                                \
	void name##__attach(name##_t *ptr, name##_t **ptrp, const char *func, \
			    const char *file, unsigned int line);             \
	void name##__detach(name##_t **ptrp, const char *func,                \
			    const char *file, unsigned int line)

#define ISC_REFCOUNT_TRACE_IMPL(name, destroy)                                \
	name##_t *name##__ref(name##_t *ptr, const char *func,                \
			      const char *file, unsigned int line) {          \
		REQUIRE(ptr != NULL);                                         \
		uint_fast32_t refs =                                          \
			isc_refcount_increment(&ptr->references) + 1;         \
		fprintf(stderr,                                               \
			"%s:%s:%s:%u:%p->references = %" PRIuFAST32 "\n",     \
			__func__, func, file, line, ptr, refs);               \
		return (ptr);                                                 \
	}                                                                     \
                                                                              \
	void name##__unref(name##_t *ptr, const char *func, const char *file, \
			   unsigned int line) {                               \
		REQUIRE(ptr != NULL);                                         \
		uint_fast32_t refs =                                          \
			isc_refcount_decrement(&ptr->references) - 1;         \
		if (refs == 0) {                                              \
			destroy(ptr);                                         \
		}                                                             \
		fprintf(stderr,                                               \
			"%s:%s:%s:%u:%p->references = %" PRIuFAST32 "\n",     \
			__func__, func, file, line, ptr, refs);               \
	}                                                                     \
	void name##__attach(name##_t *ptr, name##_t **ptrp, const char *func, \
			    const char *file, unsigned int line) {            \
		REQUIRE(ptrp != NULL && *ptrp == NULL);                       \
		uint_fast32_t refs =                                          \
			isc_refcount_increment(&ptr->references) + 1;         \
		fprintf(stderr,                                               \
			"%s:%s:%s:%u:%p->references = %" PRIuFAST32 "\n",     \
			__func__, func, file, line, ptr, refs);               \
		*ptrp = ptr;                                                  \
	}                                                                     \
                                                                              \
	void name##__detach(name##_t **ptrp, const char *func,                \
			    const char *file, unsigned int line) {            \
		REQUIRE(ptrp != NULL && *ptrp != NULL);                       \
		name##_t *ptr = *ptrp;                                        \
		*ptrp = NULL;                                                 \
		uint_fast32_t refs =                                          \
			isc_refcount_decrement(&ptr->references) - 1;         \
		if (refs == 0) {                                              \
			destroy(ptr);                                         \
		}                                                             \
		fprintf(stderr,                                               \
			"%s:%s:%s:%u:%p->references = %" PRIuFAST32 "\n",     \
			__func__, func, file, line, ptr, refs);               \
	}

#define ISC_REFCOUNT_DECL(name)                                  \
	name##_t *name##_ref(name##_t *ptr);                     \
	void	  name##_unref(name##_t *ptr);                   \
	void	  name##_attach(name##_t *ptr, name##_t **ptrp); \
	void	  name##_detach(name##_t **ptrp)

#define ISC_REFCOUNT_IMPL(name, destroy)                             \
	name##_t *name##_ref(name##_t *ptr) {                        \
		REQUIRE(ptr != NULL);                                \
		isc_refcount_increment(&ptr->references);            \
		return (ptr);                                        \
	}                                                            \
                                                                     \
	void name##_unref(name##_t *ptr) {                           \
		REQUIRE(ptr != NULL);                                \
		if (isc_refcount_decrement(&ptr->references) == 1) { \
			destroy(ptr);                                \
		}                                                    \
	}                                                            \
	void name##_attach(name##_t *ptr, name##_t **ptrp) {         \
		REQUIRE(ptrp != NULL && *ptrp == NULL);              \
		name##_ref(ptr);                                     \
		*ptrp = ptr;                                         \
	}                                                            \
                                                                     \
	void name##_detach(name##_t **ptrp) {                        \
		REQUIRE(ptrp != NULL && *ptrp != NULL);              \
		name##_t *ptr = *ptrp;                               \
		*ptrp = NULL;                                        \
		name##_unref(ptr);                                   \
	}

ISC_LANG_ENDDECLS
