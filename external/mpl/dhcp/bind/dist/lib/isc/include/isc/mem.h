/*	$NetBSD: mem.h,v 1.1.2.2 2024/02/24 13:07:25 martin Exp $	*/

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

#ifndef ISC_MEM_H
#define ISC_MEM_H 1

/*! \file isc/mem.h */

#include <stdbool.h>
#include <stdio.h>

#include <isc/lang.h>
#include <isc/mutex.h>
#include <isc/platform.h>
#include <isc/types.h>
#include <isc/util.h>

ISC_LANG_BEGINDECLS

#define ISC_MEM_LOWATER 0
#define ISC_MEM_HIWATER 1
typedef void (*isc_mem_water_t)(void *, int);

/*%
 * Define ISC_MEM_TRACKLINES=1 to turn on detailed tracing of memory
 * allocation and freeing by file and line number.
 */
#ifndef ISC_MEM_TRACKLINES
#define ISC_MEM_TRACKLINES 1
#endif /* ifndef ISC_MEM_TRACKLINES */

/*%
 * Define ISC_MEM_CHECKOVERRUN=1 to turn on checks for using memory outside
 * the requested space.  This will increase the size of each allocation.
 *
 * If we are performing a Coverity static analysis then ISC_MEM_CHECKOVERRUN
 * can hide bugs that would otherwise discovered so force to zero.
 */
#ifdef __COVERITY__
#undef ISC_MEM_CHECKOVERRUN
#define ISC_MEM_CHECKOVERRUN 0
#endif /* ifdef __COVERITY__ */
#ifndef ISC_MEM_CHECKOVERRUN
#define ISC_MEM_CHECKOVERRUN 1
#endif /* ifndef ISC_MEM_CHECKOVERRUN */

/*%
 * Define ISC_MEMPOOL_NAMES=1 to make memory pools store a symbolic
 * name so that the leaking pool can be more readily identified in
 * case of a memory leak.
 */
#ifndef ISC_MEMPOOL_NAMES
#define ISC_MEMPOOL_NAMES 1
#endif /* ifndef ISC_MEMPOOL_NAMES */

LIBISC_EXTERNAL_DATA extern unsigned int isc_mem_debugging;
LIBISC_EXTERNAL_DATA extern unsigned int isc_mem_defaultflags;

/*@{*/
#define ISC_MEM_DEBUGTRACE  0x00000001U
#define ISC_MEM_DEBUGRECORD 0x00000002U
#define ISC_MEM_DEBUGUSAGE  0x00000004U
#define ISC_MEM_DEBUGSIZE   0x00000008U
#define ISC_MEM_DEBUGCTX    0x00000010U
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
 *
 * \li #ISC_MEM_DEBUGSIZE
 *	Check the size argument being passed to isc_mem_put() matches
 *	that passed to isc_mem_get().
 *
 * \li #ISC_MEM_DEBUGCTX
 *	Check the mctx argument being passed to isc_mem_put() matches
 *	that passed to isc_mem_get().
 */
/*@}*/

#if ISC_MEM_TRACKLINES
#define _ISC_MEM_FILELINE , __FILE__, __LINE__
#define _ISC_MEM_FLARG	  , const char *, unsigned int
#else /* if ISC_MEM_TRACKLINES */
#define _ISC_MEM_FILELINE
#define _ISC_MEM_FLARG
#endif /* if ISC_MEM_TRACKLINES */

/*!
 * Define ISC_MEM_USE_INTERNAL_MALLOC=1 to use the internal malloc()
 * implementation in preference to the system one.  The internal malloc()
 * is very space-efficient, and quite fast on uniprocessor systems.  It
 * performs poorly on multiprocessor machines.
 * JT: we can overcome the performance issue on multiprocessor machines
 * by carefully separating memory contexts.
 */

#if !defined(ISC_MEM_USE_INTERNAL_MALLOC) && !__SANITIZE_ADDRESS__
#define ISC_MEM_USE_INTERNAL_MALLOC 0
#endif /* ifndef ISC_MEM_USE_INTERNAL_MALLOC */

/*
 * Flags for isc_mem_create() calls.
 */
#define ISC_MEMFLAG_RESERVED 0x00000001 /* reserved, obsoleted, don't use */
#define ISC_MEMFLAG_INTERNAL 0x00000002 /* use internal malloc */
#define ISC_MEMFLAG_FILL \
	0x00000004 /* fill with pattern after alloc and frees */

/*%
 * Define ISC_MEM_DEFAULTFILL=1 to turn filling the memory with pattern
 * after alloc and free.
 */
#if !ISC_MEM_USE_INTERNAL_MALLOC

#if ISC_MEM_DEFAULTFILL
#define ISC_MEMFLAG_DEFAULT ISC_MEMFLAG_FILL
#else /* if ISC_MEM_DEFAULTFILL */
#define ISC_MEMFLAG_DEFAULT 0
#endif /* if ISC_MEM_DEFAULTFILL */

#else /* if !ISC_MEM_USE_INTERNAL_MALLOC */

#if ISC_MEM_DEFAULTFILL
#define ISC_MEMFLAG_DEFAULT ISC_MEMFLAG_INTERNAL | ISC_MEMFLAG_FILL
#else /* if ISC_MEM_DEFAULTFILL */
#define ISC_MEMFLAG_DEFAULT ISC_MEMFLAG_INTERNAL
#endif /* if ISC_MEM_DEFAULTFILL */

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

/*% memory and memory pool methods */
typedef struct isc_memmethods {
	void *(*memget)(isc_mem_t *mctx, size_t size _ISC_MEM_FLARG);
	void (*memput)(isc_mem_t *mctx, void *ptr, size_t size _ISC_MEM_FLARG);
	void (*memputanddetach)(isc_mem_t **mctxp, void *ptr,
				size_t size _ISC_MEM_FLARG);
	void *(*memallocate)(isc_mem_t *mctx, size_t size _ISC_MEM_FLARG);
	void *(*memreallocate)(isc_mem_t *mctx, void *ptr,
			       size_t size _ISC_MEM_FLARG);
	char *(*memstrdup)(isc_mem_t *mctx, const char *s _ISC_MEM_FLARG);
	char *(*memstrndup)(isc_mem_t *mctx, const char *s,
			    size_t size _ISC_MEM_FLARG);
	void (*memfree)(isc_mem_t *mctx, void *ptr _ISC_MEM_FLARG);
} isc_memmethods_t;

/*%
 * This structure is actually just the common prefix of a memory context
 * implementation's version of an isc_mem_t.
 * \brief
 * Direct use of this structure by clients is forbidden.  mctx implementations
 * may change the structure.  'magic' must be ISCAPI_MCTX_MAGIC for any of the
 * isc_mem_ routines to work.  mctx implementations must maintain all mctx
 * invariants.
 */
struct isc_mem {
	unsigned int	  impmagic;
	unsigned int	  magic;
	isc_memmethods_t *methods;
};

#define ISCAPI_MCTX_MAGIC    ISC_MAGIC('A', 'm', 'c', 'x')
#define ISCAPI_MCTX_VALID(m) ((m) != NULL && (m)->magic == ISCAPI_MCTX_MAGIC)

/*%
 * This is the common prefix of a memory pool context.  The same note as
 * that for the mem structure applies.
 */
struct isc_mempool {
	unsigned int impmagic;
	unsigned int magic;
};

#define ISCAPI_MPOOL_MAGIC ISC_MAGIC('A', 'm', 'p', 'l')
#define ISCAPI_MPOOL_VALID(mp) \
	((mp) != NULL && (mp)->magic == ISCAPI_MPOOL_MAGIC)

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

#define isc_mem_get(c, s)      ISCMEMFUNC(get)((c), (s)_ISC_MEM_FILELINE)
#define isc_mem_allocate(c, s) ISCMEMFUNC(allocate)((c), (s)_ISC_MEM_FILELINE)
#define isc_mem_reallocate(c, p, s) \
	ISCMEMFUNC(reallocate)((c), (p), (s)_ISC_MEM_FILELINE)
#define isc_mem_strdup(c, p) ISCMEMFUNC(strdup)((c), (p)_ISC_MEM_FILELINE)
#define isc_mem_strndup(c, p, s) \
	ISCMEMFUNC(strndup)((c), (p), (s)_ISC_MEM_FILELINE)
#define isc_mempool_get(c) ISCMEMPOOLFUNC(get)((c)_ISC_MEM_FILELINE)

#define isc_mem_put(c, p, s)                                     \
	do {                                                     \
		ISCMEMFUNC(put)((c), (p), (s)_ISC_MEM_FILELINE); \
		(p) = NULL;                                      \
	} while (0)
#define isc_mem_putanddetach(c, p, s)                                     \
	do {                                                              \
		ISCMEMFUNC(putanddetach)((c), (p), (s)_ISC_MEM_FILELINE); \
		(p) = NULL;                                               \
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
void
isc_mem_create(isc_mem_t **mctxp);

/*!<
 * \brief Create a memory context.
 *
 * Requires:
 * mctxp != NULL && *mctxp == NULL */
/*@}*/

/*@{*/
void
isc_mem_attach(isc_mem_t *, isc_mem_t **);
void
isc_mem_detach(isc_mem_t **);
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

void
isc_mem_destroy(isc_mem_t **);
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

bool
isc_mem_isovermem(isc_mem_t *mctx);
/*%<
 * Return true iff the memory context is in "over memory" state, i.e.,
 * a hiwater mark has been set and the used amount of memory has exceeds
 * the mark.
 */

void
isc_mem_setwater(isc_mem_t *mctx, isc_mem_water_t water, void *water_arg,
		 size_t hiwater, size_t lowater);
/*%<
 * Set high and low water marks for this memory context.
 *
 * When the memory usage of 'mctx' exceeds 'hiwater',
 * '(water)(water_arg, #ISC_MEM_HIWATER)' will be called.  'water' needs to
 * call isc_mem_waterack() with #ISC_MEM_HIWATER to acknowledge the state
 * change.  'water' may be called multiple times.
 *
 * When the usage drops below 'lowater', 'water' will again be called, this
 * time with #ISC_MEM_LOWATER.  'water' need to calls isc_mem_waterack() with
 * #ISC_MEM_LOWATER to acknowledge the change.
 *
 *	static void
 *	water(void *arg, int mark) {
 *		struct foo *foo = arg;
 *
 *		LOCK(&foo->marklock);
 *		if (foo->mark != mark) {
 * 			foo->mark = mark;
 *			....
 *			isc_mem_waterack(foo->mctx, mark);
 *		}
 *		UNLOCK(&foo->marklock);
 *	}
 *
 * If 'water' is NULL then 'water_arg', 'hi_water' and 'lo_water' are
 * ignored and the state is reset.
 *
 * Requires:
 *
 *	'water' is not NULL.
 *	hi_water >= lo_water
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
isc_mem_setname(isc_mem_t *ctx, const char *name, void *tag);
/*%<
 * Name 'ctx'.
 *
 * Notes:
 *
 *\li	Only the first 15 characters of 'name' will be copied.
 *
 *\li	'tag' is for debugging purposes only.
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

void *
isc_mem_gettag(isc_mem_t *ctx);
/*%<
 * Get the tag value for  'task', as previously set using isc_mem_setname().
 *
 * Requires:
 *\li	'ctx' is a valid ctx.
 *
 * Notes:
 *\li	This function is for debugging purposes only.
 *
 * Requires:
 *\li	'ctx' is a valid task.
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

void
isc_mempool_create(isc_mem_t *mctx, size_t size, isc_mempool_t **mpctxp);
/*%<
 * Create a memory pool.
 *
 * Requires:
 *\li	mctx is a valid memory context.
 *\li	size > 0
 *\li	mpctxp != NULL and *mpctxp == NULL
 *
 * Defaults:
 *\li	maxalloc = UINT_MAX
 *\li	freemax = 1
 *\li	fillcount = 1
 *
 * Returns:
 *\li	#ISC_R_NOMEMORY		-- not enough memory to create pool
 *\li	#ISC_R_SUCCESS		-- all is well.
 */

void
isc_mempool_destroy(isc_mempool_t **mpctxp);
/*%<
 * Destroy a memory pool.
 *
 * Requires:
 *\li	mpctxp != NULL && *mpctxp is a valid pool.
 *\li	The pool has no un"put" allocations outstanding
 */

void
isc_mempool_setname(isc_mempool_t *mpctx, const char *name);
/*%<
 * Associate a name with a memory pool.  At most 15 characters may be used.
 *
 * Requires:
 *\li	mpctx is a valid pool.
 *\li	name != NULL;
 */

/*
 * The following functions get/set various parameters.  Note that due to
 * the unlocked nature of pools these are potentially random values unless
 * the imposed externally provided locking protocols are followed.
 *
 * Also note that the quota limits will not always take immediate effect.
 * For instance, setting "maxalloc" to a number smaller than the currently
 * allocated count is permitted.  New allocations will be refused until
 * the count drops below this threshold.
 *
 * All functions require (in addition to other requirements):
 *	mpctx is a valid memory pool
 */

unsigned int
isc_mempool_getfreemax(isc_mempool_t *mpctx);
/*%<
 * Returns the maximum allowed size of the free list.
 */

void
isc_mempool_setfreemax(isc_mempool_t *mpctx, unsigned int limit);
/*%<
 * Sets the maximum allowed size of the free list.
 */

unsigned int
isc_mempool_getfreecount(isc_mempool_t *mpctx);
/*%<
 * Returns current size of the free list.
 */

unsigned int
isc_mempool_getmaxalloc(isc_mempool_t *mpctx);
/*!<
 * Returns the maximum allowed number of allocations.
 */

void
isc_mempool_setmaxalloc(isc_mempool_t *mpctx, unsigned int limit);
/*%<
 * Sets the maximum allowed number of allocations.
 *
 * Additional requirements:
 *\li	limit > 0
 */

unsigned int
isc_mempool_getallocated(isc_mempool_t *mpctx);
/*%<
 * Returns the number of items allocated from this pool.
 */

unsigned int
isc_mempool_getfillcount(isc_mempool_t *mpctx);
/*%<
 * Returns the number of items allocated as a block from the parent memory
 * context when the free list is empty.
 */

void
isc_mempool_setfillcount(isc_mempool_t *mpctx, unsigned int limit);
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
void ISCMEMFUNC(putanddetach)(isc_mem_t **, void *, size_t _ISC_MEM_FLARG);
void ISCMEMFUNC(put)(isc_mem_t *, void *, size_t _ISC_MEM_FLARG);
void ISCMEMFUNC(free)(isc_mem_t *, void *_ISC_MEM_FLARG);

ISC_ATTR_RETURNS_NONNULL
ISC_ATTR_MALLOC_DEALLOCATOR_IDX(ISCMEMFUNC(put), 2)
void *ISCMEMFUNC(get)(isc_mem_t *, size_t _ISC_MEM_FLARG);

ISC_ATTR_RETURNS_NONNULL
ISC_ATTR_MALLOC_DEALLOCATOR_IDX(ISCMEMFUNC(free), 2)
void *ISCMEMFUNC(allocate)(isc_mem_t *, size_t _ISC_MEM_FLARG);

ISC_ATTR_RETURNS_NONNULL
ISC_ATTR_DEALLOCATOR_IDX(ISCMEMFUNC(free), 2)
void *ISCMEMFUNC(reallocate)(isc_mem_t *, void *, size_t _ISC_MEM_FLARG);

ISC_ATTR_RETURNS_NONNULL
ISC_ATTR_MALLOC_DEALLOCATOR_IDX(ISCMEMFUNC(free), 2)
char *ISCMEMFUNC(strdup)(isc_mem_t *, const char *_ISC_MEM_FLARG);
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

#endif /* ISC_MEM_H */
