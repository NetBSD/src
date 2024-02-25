/*	$NetBSD: mem.h,v 1.9.2.1 2024/02/25 15:47:21 martin Exp $	*/

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

/*! \file isc/mem.h */

#include <stdbool.h>
#include <stdio.h>

#include <isc/attributes.h>
#include <isc/lang.h>
#include <isc/mutex.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

#define ISC_MEM_LOWATER 0
#define ISC_MEM_HIWATER 1
typedef void (*isc_mem_water_t)(void *, int);

/*%
 * Define ISC_MEM_TRACKLINES=1 to turn on detailed tracing of memory
 * allocation and freeing by file and line number.
 */
#ifndef ISC_MEM_TRACKLINES
#define ISC_MEM_TRACKLINES 0
#endif /* ifndef ISC_MEM_TRACKLINES */

extern unsigned int isc_mem_debugging;
extern unsigned int isc_mem_defaultflags;

/*@{*/
#define ISC_MEM_DEBUGTRACE  0x00000001U
#define ISC_MEM_DEBUGRECORD 0x00000002U
#define ISC_MEM_DEBUGUSAGE  0x00000004U
#define ISC_MEM_DEBUGALL    0x0000001FU
/*!<
 * The variable isc_mem_debugging holds a set of flags for
 * turning certain memory debugging options on or off at
 * runtime.  It is initialized to the value ISC_MEM_DEGBUGGING,
 * which is 0 by default but may be overridden at compile time.
 * The following flags can be specified:
 *
 * \li #ISC_MEM_DEBUGTRACE
 *	Log each allocation and free to isc_lctx.
 *
 * \li #ISC_MEM_DEBUGRECORD
 *	Remember each allocation, and match them up on free.
 *	Crash if a free doesn't match an allocation.
 *
 * \li #ISC_MEM_DEBUGUSAGE
 *	If a hi_water mark is set, print the maximum inuse memory
 *	every time it is raised once it exceeds the hi_water mark.
 */
/*@}*/

#if ISC_MEM_TRACKLINES
#define _ISC_MEM_FILELINE , __FILE__, __LINE__
#define _ISC_MEM_FLARG	  , const char *, unsigned int
#else /* if ISC_MEM_TRACKLINES */
#define _ISC_MEM_FILELINE
#define _ISC_MEM_FLARG
#endif /* if ISC_MEM_TRACKLINES */

/*
 * Flags for isc_mem_create() calls.
 */
#define ISC_MEMFLAG_RESERVED1 0x00000001 /* reserved, obsoleted, don't use */
#define ISC_MEMFLAG_RESERVED2 0x00000002 /* reserved, obsoleted, don't use */
#define ISC_MEMFLAG_FILL \
	0x00000004 /* fill with pattern after alloc and frees */

/*%
 * Define ISC_MEM_DEFAULTFILL=1 to turn filling the memory with pattern
 * after alloc and free.
 */
#if ISC_MEM_DEFAULTFILL
#define ISC_MEMFLAG_DEFAULT ISC_MEMFLAG_FILL
#else /* if !ISC_MEM_USE_INTERNAL_MALLOC */
#define ISC_MEMFLAG_DEFAULT 0
#endif /* if !ISC_MEM_USE_INTERNAL_MALLOC */

/*%
 * isc_mem_putanddetach() is a convenience function for use where you
 * have a structure with an attached memory context.
 *
 * Given:
 *
 * \code
 * struct {
 *	...
 *	isc_mem_t *mctx;
 *	...
 * } *ptr;
 *
 * isc_mem_t *mctx;
 *
 * isc_mem_putanddetach(&ptr->mctx, ptr, sizeof(*ptr));
 * \endcode
 *
 * is the equivalent of:
 *
 * \code
 * mctx = NULL;
 * isc_mem_attach(ptr->mctx, &mctx);
 * isc_mem_detach(&ptr->mctx);
 * isc_mem_put(mctx, ptr, sizeof(*ptr));
 * isc_mem_detach(&mctx);
 * \endcode
 */

/*%
 * These functions are actually implemented in isc__mem_<function>
 * (two underscores). The single-underscore macros are used to pass
 * __FILE__ and __LINE__, and in the case of the put functions, to
 * set the pointer being freed to NULL in the calling function.
 *
 * Many of these functions have a further isc___mem_<function>
 * (three underscores) implementation, which is called indirectly
 * via the isc_memmethods structure in the mctx so that dynamically
 * loaded modules can use them even if named is statically linked.
 */

#define ISCMEMFUNC(sfx)	    isc__mem_##sfx
#define ISCMEMPOOLFUNC(sfx) isc__mempool_##sfx

#define isc_mem_get(c, s) ISCMEMFUNC(get)((c), (s), 0 _ISC_MEM_FILELINE)
#define isc_mem_get_aligned(c, s, a) \
	ISCMEMFUNC(get)((c), (s), (a)_ISC_MEM_FILELINE)
#define isc_mem_reget(c, p, o, n) \
	ISCMEMFUNC(reget)((c), (p), (o), (n), 0 _ISC_MEM_FILELINE)
#define isc_mem_reget_aligned(c, p, o, n, a) \
	ISCMEMFUNC(reget)((c), (p), (o), (n), (a)_ISC_MEM_FILELINE)
#define isc_mem_allocate(c, s) ISCMEMFUNC(allocate)((c), (s)_ISC_MEM_FILELINE)
#define isc_mem_reallocate(c, p, s) \
	ISCMEMFUNC(reallocate)((c), (p), (s)_ISC_MEM_FILELINE)
#define isc_mem_strdup(c, p) ISCMEMFUNC(strdup)((c), (p)_ISC_MEM_FILELINE)
#define isc_mem_strndup(c, p, l) \
	ISCMEMFUNC(strndup)((c), (p), (l)_ISC_MEM_FILELINE)
#define isc_mempool_get(c) ISCMEMPOOLFUNC(get)((c)_ISC_MEM_FILELINE)

#define isc_mem_put(c, p, s)                                         \
	do {                                                         \
		ISCMEMFUNC(put)((c), (p), (s), 0 _ISC_MEM_FILELINE); \
		(p) = NULL;                                          \
	} while (0)
#define isc_mem_put_aligned(c, p, s, a)                \
	do {                                           \
		ISCMEMFUNC(put)                        \
		((c), (p), (s), (a)_ISC_MEM_FILELINE); \
		(p) = NULL;                            \
	} while (0)
#define isc_mem_putanddetach(c, p, s)                                         \
	do {                                                                  \
		ISCMEMFUNC(putanddetach)((c), (p), (s), 0 _ISC_MEM_FILELINE); \
		(p) = NULL;                                                   \
	} while (0)
#define isc_mem_putanddetach_aligned(c, p, s, a)       \
	do {                                           \
		ISCMEMFUNC(putanddetach)               \
		((c), (p), (s), (a)_ISC_MEM_FILELINE); \
		(p) = NULL;                            \
	} while (0)
#define isc_mem_free(c, p)                                   \
	do {                                                 \
		ISCMEMFUNC(free)((c), (p)_ISC_MEM_FILELINE); \
		(p) = NULL;                                  \
	} while (0)
#define isc_mempool_put(c, p)                                   \
	do {                                                    \
		ISCMEMPOOLFUNC(put)((c), (p)_ISC_MEM_FILELINE); \
		(p) = NULL;                                     \
	} while (0)

/*@{*/
#define isc_mem_create(cp) ISCMEMFUNC(create)((cp)_ISC_MEM_FILELINE)
void ISCMEMFUNC(create)(isc_mem_t **_ISC_MEM_FLARG);

/*!<
 * \brief Create a memory context.
 *
 * Requires:
 * mctxp != NULL && *mctxp == NULL */
/*@}*/

#define isc_mem_create_arena(cp) isc__mem_create_arena((cp)_ISC_MEM_FILELINE)
void
isc__mem_create_arena(isc_mem_t **_ISC_MEM_FLARG);
/*!<
 * \brief Create a memory context that routs all its operations to a
 * dedicated jemalloc arena (when available). When jemalloc is not
 * available, the function is, effectively, an alias to
 * isc_mem_create().
 *
 * Requires:
 * mctxp != NULL && *mctxp == NULL */
/*@}*/

isc_result_t
isc_mem_arena_set_muzzy_decay_ms(isc_mem_t *mctx, const ssize_t decay_ms);

isc_result_t
isc_mem_arena_set_dirty_decay_ms(isc_mem_t *mctx, const ssize_t decay_ms);
/*!<
 * \brief These two functions set the given parameters on the
 * jemalloc arena associated with the memory context (if there is
 * one). When jemalloc is not available, these are no-op.
 *
 * NOTE: The "muzzy_decay_ms" and "dirty_decay_ms" are the most common
 * parameters to adjust when the defaults do not work well (per the
 * official jemalloc tuning guide:
 * https://github.com/jemalloc/jemalloc/blob/dev/TUNING.md).
 *
 * Requires:
 * mctx - a valid memory context.
 */
/*@}*/

void
isc_mem_attach(isc_mem_t *, isc_mem_t **);

/*@{*/
void
isc_mem_attach(isc_mem_t *, isc_mem_t **);
#define isc_mem_detach(cp) ISCMEMFUNC(detach)((cp)_ISC_MEM_FILELINE)
void ISCMEMFUNC(detach)(isc_mem_t **_ISC_MEM_FLARG);
/*!<
 * \brief Attach to / detach from a memory context.
 *
 * This is intended for applications that use multiple memory contexts
 * in such a way that it is not obvious when the last allocations from
 * a given context has been freed and destroying the context is safe.
 *
 * Most applications do not need to call these functions as they can
 * simply create a single memory context at the beginning of main()
 * and destroy it at the end of main(), thereby guaranteeing that it
 * is not destroyed while there are outstanding allocations.
 */
/*@}*/

#define isc_mem_destroy(cp) ISCMEMFUNC(destroy)((cp)_ISC_MEM_FILELINE)
void ISCMEMFUNC(destroy)(isc_mem_t **_ISC_MEM_FLARG);
/*%<
 * Destroy a memory context.
 */

void
isc_mem_stats(isc_mem_t *mctx, FILE *out);
/*%<
 * Print memory usage statistics for 'mctx' on the stream 'out'.
 */

void
isc_mem_setdestroycheck(isc_mem_t *mctx, bool on);
/*%<
 * If 'on' is true, 'mctx' will check for memory leaks when
 * destroyed and abort the program if any are present.
 */

size_t
isc_mem_inuse(isc_mem_t *mctx);
/*%<
 * Get an estimate of the amount of memory in use in 'mctx', in bytes.
 * This includes quantization overhead, but does not include memory
 * allocated from the system but not yet used.
 */

size_t
isc_mem_maxinuse(isc_mem_t *mctx);
/*%<
 * Get an estimate of the largest amount of memory that has been in
 * use in 'mctx' at any time.
 */

size_t
isc_mem_total(isc_mem_t *mctx);
/*%<
 * Get the total amount of memory in 'mctx', in bytes, including memory
 * not yet used.
 */

size_t
isc_mem_malloced(isc_mem_t *ctx);
/*%<
 * Get an estimate of the amount of memory allocated in 'mctx', in bytes.
 */

size_t
isc_mem_maxmalloced(isc_mem_t *ctx);
/*%<
 * Get an estimate of the largest amount of memory that has been
 * allocated in 'mctx' at any time.
 */

bool
isc_mem_isovermem(isc_mem_t *mctx);
/*%<
 * Return true iff the memory context is in "over memory" state, i.e.,
 * a hiwater mark has been set and the used amount of memory has exceeds
 * the mark.
 */

void
isc_mem_clearwater(isc_mem_t *mctx);
void
isc_mem_setwater(isc_mem_t *mctx, isc_mem_water_t water, void *water_arg,
		 size_t hiwater, size_t lowater);
/*%<
 * Set high and low water marks for this memory context.
 *
 * When the memory usage of 'mctx' exceeds 'hiwater',
 * '(water)(water_arg, #ISC_MEM_HIWATER)' will be called.  'water' needs
 * to call isc_mem_waterack() with #ISC_MEM_HIWATER to acknowledge the
 * state change.  'water' may be called multiple times.
 *
 * When the usage drops below 'lowater', 'water' will again be called,
 * this time with #ISC_MEM_LOWATER.  'water' need to calls
 * isc_mem_waterack() with #ISC_MEM_LOWATER to acknowledge the change.
 *
 *	static void
 *	water(void *arg, int mark) {
 *		struct foo *foo = arg;
 *
 *		LOCK(&foo->marklock);
 *		if (foo->mark != mark) {
 *			foo->mark = mark;
 *			....
 *			isc_mem_waterack(foo->mctx, mark);
 *		}
 *		UNLOCK(&foo->marklock);
 *	}
 *
 * if 'water' is set to NULL, the 'hiwater' and 'lowater' must set to 0, and
 * high- and low-water processing are disabled for this memory context.  There's
 * a convenient function isc_mem_clearwater().
 *
 * Requires:
 *
 *\li   If 'water' is NULL, 'hiwater' and 'lowater' must be set to 0.
 *\li	If 'water' and 'water_arg' have previously been set, they are
	unchanged.
 *\li	'hiwater' >= 'lowater'
 */

void
isc_mem_waterack(isc_mem_t *ctx, int mark);
/*%<
 * Called to acknowledge changes in signaled by calls to 'water'.
 */

void
isc_mem_checkdestroyed(FILE *file);
/*%<
 * Check that all memory contexts have been destroyed.
 * Prints out those that have not been.
 * Fatally fails if there are still active contexts.
 */

unsigned int
isc_mem_references(isc_mem_t *ctx);
/*%<
 * Return the current reference count.
 */

void
isc_mem_setname(isc_mem_t *ctx, const char *name);
/*%<
 * Name 'ctx'.
 *
 * Notes:
 *
 *\li	Only the first 15 characters of 'name' will be copied.
 *
 * Requires:
 *
 *\li	'ctx' is a valid ctx.
 */

const char *
isc_mem_getname(isc_mem_t *ctx);
/*%<
 * Get the name of 'ctx', as previously set using isc_mem_setname().
 *
 * Requires:
 *\li	'ctx' is a valid ctx.
 *
 * Returns:
 *\li	A non-NULL pointer to a null-terminated string.
 * 	If the ctx has not been named, the string is
 * 	empty.
 */

#ifdef HAVE_LIBXML2
int
isc_mem_renderxml(void *writer0);
/*%<
 * Render all contexts' statistics and status in XML for writer.
 */
#endif /* HAVE_LIBXML2 */

#ifdef HAVE_JSON_C
isc_result_t
isc_mem_renderjson(void *memobj0);
/*%<
 * Render all contexts' statistics and status in JSON.
 */
#endif /* HAVE_JSON_C */

/*
 * Memory pools
 */

#define isc_mempool_create(c, s, mp) \
	isc__mempool_create((c), (s), (mp)_ISC_MEM_FILELINE)
void
isc__mempool_create(isc_mem_t *restrict mctx, const size_t element_size,
		    isc_mempool_t **mpctxp _ISC_MEM_FLARG);
/*%<
 * Create a memory pool.
 *
 * Requires:
 *\li	mctx is a valid memory context.
 *\li	size > 0
 *\li	mpctxp != NULL and *mpctxp == NULL
 *
 * Defaults:
 *\li	freemax = 1
 *\li	fillcount = 1
 *
 * Returns:
 *\li	#ISC_R_NOMEMORY		-- not enough memory to create pool
 *\li	#ISC_R_SUCCESS		-- all is well.
 */

#define isc_mempool_destroy(mp) isc__mempool_destroy((mp)_ISC_MEM_FILELINE)
void
isc__mempool_destroy(isc_mempool_t **restrict mpctxp _ISC_MEM_FLARG);
/*%<
 * Destroy a memory pool.
 *
 * Requires:
 *\li	mpctxp != NULL && *mpctxp is a valid pool.
 *\li	The pool has no un"put" allocations outstanding
 */

void
isc_mempool_setname(isc_mempool_t *restrict mpctx, const char *name);
/*%<
 * Associate a name with a memory pool.  At most 15 characters may be
 *used.
 *
 * Requires:
 *\li	mpctx is a valid pool.
 *\li	name != NULL;
 */

/*
 * The following functions get/set various parameters.  Note that due to
 * the unlocked nature of pools these are potentially random values
 *unless the imposed externally provided locking protocols are followed.
 *
 * Also note that the quota limits will not always take immediate
 * effect.
 *
 * All functions require (in addition to other requirements):
 *	mpctx is a valid memory pool
 */

unsigned int
isc_mempool_getfreemax(isc_mempool_t *restrict mpctx);
/*%<
 * Returns the maximum allowed size of the free list.
 */

void
isc_mempool_setfreemax(isc_mempool_t *restrict mpctx, const unsigned int limit);
/*%<
 * Sets the maximum allowed size of the free list.
 */

unsigned int
isc_mempool_getfreecount(isc_mempool_t *restrict mpctx);
/*%<
 * Returns current size of the free list.
 */

unsigned int
isc_mempool_getallocated(isc_mempool_t *restrict mpctx);
/*%<
 * Returns the number of items allocated from this pool.
 */

unsigned int
isc_mempool_getfillcount(isc_mempool_t *restrict mpctx);
/*%<
 * Returns the number of items allocated as a block from the parent
 * memory context when the free list is empty.
 */

void
isc_mempool_setfillcount(isc_mempool_t *restrict mpctx,
			 const unsigned int limit);
/*%<
 * Sets the fillcount.
 *
 * Additional requirements:
 *\li	limit > 0
 */

#if defined(UNIT_TESTING) && defined(malloc)
/*
 * cmocka.h redefined malloc as a macro, we #undef it
 * to avoid replacing ISC_ATTR_MALLOC with garbage.
 */
#pragma push_macro("malloc")
#undef malloc
#define POP_MALLOC_MACRO 1
#endif

/*
 * Pseudo-private functions for use via macros.  Do not call directly.
 */
void ISCMEMFUNC(putanddetach)(isc_mem_t **, void *, size_t,
			      size_t _ISC_MEM_FLARG);
void ISCMEMFUNC(put)(isc_mem_t *, void *, size_t, size_t _ISC_MEM_FLARG);
void ISCMEMFUNC(free)(isc_mem_t *, void *_ISC_MEM_FLARG);

ISC_ATTR_MALLOC_DEALLOCATOR_IDX(ISCMEMFUNC(put), 2)
void *ISCMEMFUNC(get)(isc_mem_t *, size_t, size_t _ISC_MEM_FLARG);

ISC_ATTR_DEALLOCATOR_IDX(ISCMEMFUNC(put), 2)
void *ISCMEMFUNC(reget)(isc_mem_t *, void *, size_t, size_t,
			size_t _ISC_MEM_FLARG);

ISC_ATTR_MALLOC_DEALLOCATOR_IDX(ISCMEMFUNC(free), 2)
void *ISCMEMFUNC(allocate)(isc_mem_t *, size_t _ISC_MEM_FLARG);

ISC_ATTR_DEALLOCATOR_IDX(ISCMEMFUNC(free), 2)
void *ISCMEMFUNC(reallocate)(isc_mem_t *, void *, size_t _ISC_MEM_FLARG);

ISC_ATTR_RETURNS_NONNULL
ISC_ATTR_MALLOC_DEALLOCATOR_IDX(ISCMEMFUNC(free), 2)
char *ISCMEMFUNC(strdup)(isc_mem_t *, const char *_ISC_MEM_FLARG);

ISC_ATTR_RETURNS_NONNULL
ISC_ATTR_MALLOC_DEALLOCATOR_IDX(ISCMEMFUNC(free), 2)
char *ISCMEMFUNC(strndup)(isc_mem_t *, const char *, size_t _ISC_MEM_FLARG);

ISC_ATTR_MALLOC_DEALLOCATOR_IDX(ISCMEMPOOLFUNC(put), 2)
void *ISCMEMPOOLFUNC(get)(isc_mempool_t *_ISC_MEM_FLARG);

void ISCMEMPOOLFUNC(put)(isc_mempool_t *, void *_ISC_MEM_FLARG);

#ifdef POP_MALLOC_MACRO
/*
 * Restore cmocka.h macro for malloc.
 */
#pragma pop_macro("malloc")
#endif

ISC_LANG_ENDDECLS
