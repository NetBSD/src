/*	$NetBSD: mem.c,v 1.17 2025/01/26 16:25:37 christos Exp $	*/

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

/*! \file */

#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <isc/hash.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/once.h>
#include <isc/os.h>
#include <isc/overflow.h>
#include <isc/refcount.h>
#include <isc/strerr.h>
#include <isc/string.h>
#include <isc/types.h>
#include <isc/urcu.h>
#include <isc/util.h>

#ifdef HAVE_LIBXML2
#include <libxml/xmlwriter.h>
#define ISC_XMLCHAR (const xmlChar *)
#endif /* HAVE_LIBXML2 */

#ifdef HAVE_JSON_C
#include <json_object.h>
#endif /* HAVE_JSON_C */

/* On DragonFly BSD the header does not provide jemalloc API */
#if defined(HAVE_MALLOC_NP_H) && !defined(__DragonFly__)
#include <malloc_np.h>
#define JEMALLOC_API_SUPPORTED 1
#elif defined(HAVE_JEMALLOC)
#include <jemalloc/jemalloc.h>
#define JEMALLOC_API_SUPPORTED 1
#else
#include "jemalloc_shim.h"
#endif

#include "mem_p.h"

#define MCTXLOCK(m)   LOCK(&m->lock)
#define MCTXUNLOCK(m) UNLOCK(&m->lock)

#ifndef ISC_MEM_DEBUGGING
#define ISC_MEM_DEBUGGING 0
#endif /* ifndef ISC_MEM_DEBUGGING */
unsigned int isc_mem_debugging = ISC_MEM_DEBUGGING;
unsigned int isc_mem_defaultflags = ISC_MEMFLAG_DEFAULT;

#define ISC_MEM_ILLEGAL_ARENA (UINT_MAX)

volatile void *isc__mem_malloc = mallocx;

/*
 * Constants.
 */

#define ZERO_ALLOCATION_SIZE sizeof(void *)
#define ALIGNMENT	     8U /*%< must be a power of 2 */
#define ALIGNMENT_SIZE	     sizeof(size_info)
#define DEBUG_TABLE_COUNT    512U

/*
 * Types.
 */
#if ISC_MEM_TRACKLINES
typedef struct debuglink debuglink_t;
struct debuglink {
	ISC_LINK(debuglink_t) link;
	const void *ptr;
	size_t size;
	const char *file;
	unsigned int line;
};

typedef ISC_LIST(debuglink_t) debuglist_t;

#define FLARG_PASS , file, line
#define FLARG	   , const char *file, unsigned int line
#else /* if ISC_MEM_TRACKLINES */
#define FLARG_PASS
#define FLARG
#endif /* if ISC_MEM_TRACKLINES */

typedef struct element element;
struct element {
	element *next;
};

#define MEM_MAGIC	 ISC_MAGIC('M', 'e', 'm', 'C')
#define VALID_CONTEXT(c) ISC_MAGIC_VALID(c, MEM_MAGIC)

/* List of all active memory contexts. */

static ISC_LIST(isc_mem_t) contexts;

static isc_once_t init_once = ISC_ONCE_INIT;
static isc_once_t shut_once = ISC_ONCE_INIT;
static isc_mutex_t contextslock;

struct isc_mem {
	unsigned int magic;
	unsigned int flags;
	unsigned int jemalloc_flags;
	unsigned int jemalloc_arena;
	unsigned int debugging;
	isc_mutex_t lock;
	bool checkfree;
	isc_refcount_t references;
	char name[16];
	atomic_size_t inuse;
	atomic_bool hi_called;
	atomic_bool is_overmem;
	atomic_size_t hi_water;
	atomic_size_t lo_water;
	ISC_LIST(isc_mempool_t) pools;
	unsigned int poolcnt;

#if ISC_MEM_TRACKLINES
	debuglist_t *debuglist;
	size_t debuglistcnt;
#endif /* if ISC_MEM_TRACKLINES */

	ISC_LINK(isc_mem_t) link;
};

#define MEMPOOL_MAGIC	 ISC_MAGIC('M', 'E', 'M', 'p')
#define VALID_MEMPOOL(c) ISC_MAGIC_VALID(c, MEMPOOL_MAGIC)

struct isc_mempool {
	/* always unlocked */
	unsigned int magic;
	isc_mem_t *mctx;	      /*%< our memory context */
	ISC_LINK(isc_mempool_t) link; /*%< next pool in this mem context */
	element *items;		      /*%< low water item list */
	size_t size;		      /*%< size of each item on this pool */
	size_t allocated;	      /*%< # of items currently given out */
	size_t freecount;	      /*%< # of items on reserved list */
	size_t freemax;		      /*%< # of items allowed on free list */
	size_t fillcount;	      /*%< # of items to fetch on each fill */
	/*%< Stats only. */
	size_t gets; /*%< # of requests to this pool */
	/*%< Debugging only. */
	char name[16]; /*%< printed name in stats reports */
};

/*
 * Private Inline-able.
 */

#if !ISC_MEM_TRACKLINES
#define ADD_TRACE(mctx, ptr, size, file, line)
#define DELETE_TRACE(mctx, ptr, size, file, line)
#define ISC_MEMFUNC_SCOPE
#else /* if !ISC_MEM_TRACKLINES */
#define TRACE_OR_RECORD (ISC_MEM_DEBUGTRACE | ISC_MEM_DEBUGRECORD)

#define SHOULD_TRACE_OR_RECORD(mctx, ptr) \
	(((mctx)->debugging & TRACE_OR_RECORD) != 0 && ptr != NULL)

#define ADD_TRACE(mctx, ptr, size, file, line)                \
	if (SHOULD_TRACE_OR_RECORD(mctx, ptr)) {              \
		add_trace_entry(mctx, ptr, size, file, line); \
	}

#define DELETE_TRACE(mctx, ptr, size, file, line)                \
	if (SHOULD_TRACE_OR_RECORD(mctx, ptr)) {                 \
		delete_trace_entry(mctx, ptr, size, file, line); \
	}

static void
print_active(isc_mem_t *ctx, FILE *out);
#endif /* ISC_MEM_TRACKLINES */

#if ISC_MEM_TRACKLINES
/*!
 * mctx must not be locked.
 */
static void
add_trace_entry(isc_mem_t *mctx, const void *ptr, size_t size FLARG) {
	debuglink_t *dl = NULL;
	uint32_t hash;
	uint32_t idx;

	MCTXLOCK(mctx);

	if ((mctx->debugging & ISC_MEM_DEBUGTRACE) != 0) {
		fprintf(stderr, "add %p size %zu file %s line %u mctx %p\n",
			ptr, size, file, line, mctx);
	}

	if (mctx->debuglist == NULL) {
		goto unlock;
	}

#ifdef __COVERITY__
	/*
	 * Use simple conversion from pointer to hash to avoid
	 * tainting 'ptr' due to byte swap in isc_hash32.
	 */
	hash = (uintptr_t)ptr >> 3;
#else
	hash = isc_hash32(&ptr, sizeof(ptr), true);
#endif
	idx = hash % DEBUG_TABLE_COUNT;

	dl = mallocx(sizeof(debuglink_t), mctx->jemalloc_flags);
	INSIST(dl != NULL);

	ISC_LINK_INIT(dl, link);
	dl->ptr = ptr;
	dl->size = size;
	dl->file = file;
	dl->line = line;

	ISC_LIST_PREPEND(mctx->debuglist[idx], dl, link);
	mctx->debuglistcnt++;
unlock:
	MCTXUNLOCK(mctx);
}

static void
delete_trace_entry(isc_mem_t *mctx, const void *ptr, size_t size,
		   const char *file, unsigned int line) {
	debuglink_t *dl = NULL;
	uint32_t hash;
	uint32_t idx;

	MCTXLOCK(mctx);

	if ((mctx->debugging & ISC_MEM_DEBUGTRACE) != 0) {
		fprintf(stderr, "del %p size %zu file %s line %u mctx %p\n",
			ptr, size, file, line, mctx);
	}

	if (mctx->debuglist == NULL) {
		goto unlock;
	}

#ifdef __COVERITY__
	/*
	 * Use simple conversion from pointer to hash to avoid
	 * tainting 'ptr' due to byte swap in isc_hash32.
	 */
	hash = (uintptr_t)ptr >> 3;
#else
	hash = isc_hash32(&ptr, sizeof(ptr), true);
#endif
	idx = hash % DEBUG_TABLE_COUNT;

	dl = ISC_LIST_HEAD(mctx->debuglist[idx]);
	while (dl != NULL) {
		if (dl->ptr == ptr) {
			ISC_LIST_UNLINK(mctx->debuglist[idx], dl, link);
			sdallocx(dl, sizeof(*dl), mctx->jemalloc_flags);
			goto unlock;
		}
		dl = ISC_LIST_NEXT(dl, link);
	}

	/*
	 * If we get here, we didn't find the item on the list.  We're
	 * screwed.
	 */
	UNREACHABLE();
unlock:
	MCTXUNLOCK(mctx);
}
#endif /* ISC_MEM_TRACKLINES */

#define ADJUST_ZERO_ALLOCATION_SIZE(s)    \
	if (s == 0) {                     \
		s = ZERO_ALLOCATION_SIZE; \
	}

/*!
 * Perform a malloc, doing memory filling and overrun detection as necessary.
 */
static void *
mem_get(isc_mem_t *ctx, size_t size, int flags) {
	char *ret = NULL;

	ADJUST_ZERO_ALLOCATION_SIZE(size);

	ret = mallocx(size, flags | ctx->jemalloc_flags);
	INSIST(ret != NULL);

	if ((flags & ISC__MEM_ZERO) == 0 &&
	    (ctx->flags & ISC_MEMFLAG_FILL) != 0)
	{
		memset(ret, 0xbe, size); /* Mnemonic for "beef". */
	}

	return ret;
}

/*!
 * Perform a free, doing memory filling and overrun detection as necessary.
 */
/* coverity[+free : arg-1] */
static void
mem_put(isc_mem_t *ctx, void *mem, size_t size, int flags) {
	ADJUST_ZERO_ALLOCATION_SIZE(size);

	if ((ctx->flags & ISC_MEMFLAG_FILL) != 0) {
		memset(mem, 0xde, size); /* Mnemonic for "dead". */
	}
	sdallocx(mem, size, flags | ctx->jemalloc_flags);
}

static void *
mem_realloc(isc_mem_t *ctx, void *old_ptr, size_t old_size, size_t new_size,
	    int flags) {
	void *new_ptr = NULL;

	ADJUST_ZERO_ALLOCATION_SIZE(new_size);

	new_ptr = rallocx(old_ptr, new_size, flags | ctx->jemalloc_flags);
	INSIST(new_ptr != NULL);

	if ((flags & ISC__MEM_ZERO) == 0 &&
	    (ctx->flags & ISC_MEMFLAG_FILL) != 0)
	{
		ssize_t diff_size = new_size - old_size;
		void *diff_ptr = (uint8_t *)new_ptr + old_size;
		if (diff_size > 0) {
			/* Mnemonic for "beef". */
			memset(diff_ptr, 0xbe, diff_size);
		}
	}

	return new_ptr;
}

/*!
 * Update internal counters after a memory get.
 */
static void
mem_getstats(isc_mem_t *ctx, size_t size) {
	atomic_fetch_add_relaxed(&ctx->inuse, size);
}

/*!
 * Update internal counters after a memory put.
 */
static void
mem_putstats(isc_mem_t *ctx, size_t size) {
	atomic_size_t s = atomic_fetch_sub_relaxed(&ctx->inuse, size);
	INSIST(s >= size);
}

/*
 * Private.
 */

static bool
mem_jemalloc_arena_create(unsigned int *pnew_arenano) {
	REQUIRE(pnew_arenano != NULL);

#ifdef JEMALLOC_API_SUPPORTED
	unsigned int arenano = 0;
	size_t len = sizeof(arenano);
	int res = 0;

	res = mallctl("arenas.create", &arenano, &len, NULL, 0);
	if (res != 0) {
		return false;
	}

	*pnew_arenano = arenano;

	return true;
#else
	*pnew_arenano = ISC_MEM_ILLEGAL_ARENA;
	return true;
#endif /* JEMALLOC_API_SUPPORTED */
}

static bool
mem_jemalloc_arena_destroy(unsigned int arenano) {
#ifdef JEMALLOC_API_SUPPORTED
	int res = 0;
	char buf[256] = { 0 };

	(void)snprintf(buf, sizeof(buf), "arena.%u.destroy", arenano);
	res = mallctl(buf, NULL, NULL, NULL, 0);
	if (res != 0) {
		return false;
	}

	return true;
#else
	UNUSED(arenano);
	return true;
#endif /* JEMALLOC_API_SUPPORTED */
}

static void
mem_initialize(void) {
/*
 * Check if the values copied from jemalloc still match
 */
#ifdef JEMALLOC_API_SUPPORTED
	RUNTIME_CHECK(ISC__MEM_ZERO == MALLOCX_ZERO);
#endif /* JEMALLOC_API_SUPPORTED */

	isc_mutex_init(&contextslock);
	ISC_LIST_INIT(contexts);
}

void
isc__mem_initialize(void) {
	isc_once_do(&init_once, mem_initialize);
}

static void
mem_shutdown(void) {
	bool empty;

	isc__mem_checkdestroyed();

	LOCK(&contextslock);
	empty = ISC_LIST_EMPTY(contexts);
	UNLOCK(&contextslock);

	if (empty) {
		isc_mutex_destroy(&contextslock);
	}
}

void
isc__mem_shutdown(void) {
	isc_once_do(&shut_once, mem_shutdown);
}

static void
mem_create(isc_mem_t **ctxp, unsigned int debugging, unsigned int flags,
	   unsigned int jemalloc_flags) {
	isc_mem_t *ctx = NULL;

	REQUIRE(ctxp != NULL && *ctxp == NULL);

	ctx = mallocx(sizeof(*ctx), jemalloc_flags);
	INSIST(ctx != NULL);

	*ctx = (isc_mem_t){
		.magic = MEM_MAGIC,
		.debugging = debugging,
		.flags = flags,
		.jemalloc_flags = jemalloc_flags,
		.jemalloc_arena = ISC_MEM_ILLEGAL_ARENA,
		.checkfree = true,
	};

	isc_mutex_init(&ctx->lock);
	isc_refcount_init(&ctx->references, 1);

	atomic_init(&ctx->inuse, 0);
	atomic_init(&ctx->hi_water, 0);
	atomic_init(&ctx->lo_water, 0);
	atomic_init(&ctx->hi_called, false);
	atomic_init(&ctx->is_overmem, false);

	ISC_LIST_INIT(ctx->pools);

#if ISC_MEM_TRACKLINES
	if ((ctx->debugging & ISC_MEM_DEBUGRECORD) != 0) {
		unsigned int i;

		ctx->debuglist = mallocx(
			ISC_CHECKED_MUL(DEBUG_TABLE_COUNT, sizeof(debuglist_t)),
			jemalloc_flags);
		INSIST(ctx->debuglist != NULL);

		for (i = 0; i < DEBUG_TABLE_COUNT; i++) {
			ISC_LIST_INIT(ctx->debuglist[i]);
		}
	}
#endif /* if ISC_MEM_TRACKLINES */

	LOCK(&contextslock);
	ISC_LIST_INITANDAPPEND(contexts, ctx, link);
	UNLOCK(&contextslock);

	*ctxp = ctx;
}

/*
 * Public.
 */

static void
destroy(isc_mem_t *ctx) {
	unsigned int arena_no;
	LOCK(&contextslock);
	ISC_LIST_UNLINK(contexts, ctx, link);
	UNLOCK(&contextslock);

	ctx->magic = 0;

	arena_no = ctx->jemalloc_arena;

	INSIST(ISC_LIST_EMPTY(ctx->pools));

#if ISC_MEM_TRACKLINES
	if (ctx->debuglist != NULL) {
		debuglink_t *dl;
		for (size_t i = 0; i < DEBUG_TABLE_COUNT; i++) {
			for (dl = ISC_LIST_HEAD(ctx->debuglist[i]); dl != NULL;
			     dl = ISC_LIST_HEAD(ctx->debuglist[i]))
			{
				if (ctx->checkfree && dl->ptr != NULL) {
					print_active(ctx, stderr);
				}
				INSIST(!ctx->checkfree || dl->ptr == NULL);

				ISC_LIST_UNLINK(ctx->debuglist[i], dl, link);
				sdallocx(dl, sizeof(*dl), ctx->jemalloc_flags);
			}
		}

		sdallocx(
			ctx->debuglist,
			ISC_CHECKED_MUL(DEBUG_TABLE_COUNT, sizeof(debuglist_t)),
			ctx->jemalloc_flags);
	}
#endif /* if ISC_MEM_TRACKLINES */

	isc_mutex_destroy(&ctx->lock);

	if (ctx->checkfree) {
		INSIST(atomic_load(&ctx->inuse) == 0);
	}
	sdallocx(ctx, sizeof(*ctx), ctx->jemalloc_flags);

	if (arena_no != ISC_MEM_ILLEGAL_ARENA) {
		RUNTIME_CHECK(mem_jemalloc_arena_destroy(arena_no) == true);
	}
}

void
isc_mem_attach(isc_mem_t *source, isc_mem_t **targetp) {
	REQUIRE(VALID_CONTEXT(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	isc_refcount_increment(&source->references);

	*targetp = source;
}

void
isc__mem_detach(isc_mem_t **ctxp FLARG) {
	isc_mem_t *ctx = NULL;

	REQUIRE(ctxp != NULL && VALID_CONTEXT(*ctxp));

	ctx = *ctxp;
	*ctxp = NULL;

	if (isc_refcount_decrement(&ctx->references) == 1) {
		isc_refcount_destroy(&ctx->references);
#if ISC_MEM_TRACKLINES
		if ((ctx->debugging & ISC_MEM_DEBUGTRACE) != 0) {
			fprintf(stderr, "destroy mctx %p file %s line %u\n",
				ctx, file, line);
		}
#endif
		destroy(ctx);
	}
}

/*
 * isc_mem_putanddetach() is the equivalent of:
 *
 * mctx = NULL;
 * isc_mem_attach(ptr->mctx, &mctx);
 * isc_mem_detach(&ptr->mctx);
 * isc_mem_put(mctx, ptr, sizeof(*ptr);
 * isc_mem_detach(&mctx);
 */

void
isc__mem_putanddetach(isc_mem_t **ctxp, void *ptr, size_t size,
		      int flags FLARG) {
	REQUIRE(ctxp != NULL && VALID_CONTEXT(*ctxp));
	REQUIRE(ptr != NULL);
	REQUIRE(size != 0);

	isc_mem_t *ctx = *ctxp;
	*ctxp = NULL;

	isc__mem_put(ctx, ptr, size, flags FLARG_PASS);
	isc__mem_detach(&ctx FLARG_PASS);
}

void
isc__mem_destroy(isc_mem_t **ctxp FLARG) {
	isc_mem_t *ctx = NULL;

	/*
	 * This routine provides legacy support for callers who use mctxs
	 * without attaching/detaching.
	 */

	REQUIRE(ctxp != NULL && VALID_CONTEXT(*ctxp));

	ctx = *ctxp;
	*ctxp = NULL;

	rcu_barrier();

#if ISC_MEM_TRACKLINES
	if ((ctx->debugging & ISC_MEM_DEBUGTRACE) != 0) {
		fprintf(stderr, "destroy mctx %p file %s line %u\n", ctx, file,
			line);
	}

	if (isc_refcount_decrement(&ctx->references) > 1) {
		print_active(ctx, stderr);
	}
#else  /* if ISC_MEM_TRACKLINES */
	isc_refcount_decrementz(&ctx->references);
#endif /* if ISC_MEM_TRACKLINES */
	isc_refcount_destroy(&ctx->references);
	destroy(ctx);

	*ctxp = NULL;
}

void *
isc__mem_get(isc_mem_t *ctx, size_t size, int flags FLARG) {
	void *ptr = NULL;

	REQUIRE(VALID_CONTEXT(ctx));

	ptr = mem_get(ctx, size, flags);

	mem_getstats(ctx, size);
	ADD_TRACE(ctx, ptr, size, file, line);

	return ptr;
}

void
isc__mem_put(isc_mem_t *ctx, void *ptr, size_t size, int flags FLARG) {
	REQUIRE(VALID_CONTEXT(ctx));

	DELETE_TRACE(ctx, ptr, size, file, line);

	mem_putstats(ctx, size);
	mem_put(ctx, ptr, size, flags);
}

#if ISC_MEM_TRACKLINES
static void
print_active(isc_mem_t *mctx, FILE *out) {
	if (mctx->debuglist != NULL) {
		debuglink_t *dl;
		unsigned int i;
		bool found;

		fprintf(out, "Dump of all outstanding memory "
			     "allocations:\n");
		found = false;
		for (i = 0; i < DEBUG_TABLE_COUNT; i++) {
			dl = ISC_LIST_HEAD(mctx->debuglist[i]);

			if (dl != NULL) {
				found = true;
			}

			while (dl != NULL) {
				if (dl->ptr != NULL) {
					fprintf(out,
						"\tptr %p size %zu "
						"file %s "
						"line %u\n",
						dl->ptr, dl->size, dl->file,
						dl->line);
				}
				dl = ISC_LIST_NEXT(dl, link);
			}
		}

		if (!found) {
			fprintf(out, "\tNone.\n");
		}
	}
}
#endif /* if ISC_MEM_TRACKLINES */

/*
 * Print the stats[] on the stream "out" with suitable formatting.
 */
void
isc_mem_stats(isc_mem_t *ctx, FILE *out) {
	isc_mempool_t *pool = NULL;

	REQUIRE(VALID_CONTEXT(ctx));

	MCTXLOCK(ctx);

	/*
	 * Note that since a pool can be locked now, these stats might
	 * be somewhat off if the pool is in active use at the time the
	 * stats are dumped.  The link fields are protected by the
	 * isc_mem_t's lock, however, so walking this list and
	 * extracting integers from stats fields is always safe.
	 */
	pool = ISC_LIST_HEAD(ctx->pools);
	if (pool != NULL) {
		fprintf(out, "[Pool statistics]\n");
		fprintf(out, "%15s %10s %10s %10s %10s %10s %10s %1s\n", "name",
			"size", "allocated", "freecount", "freemax",
			"fillcount", "gets", "L");
	}
	while (pool != NULL) {
		fprintf(out,
			"%15s %10zu %10zu %10zu %10zu %10zu %10zu %10zu %s\n",
			pool->name, pool->size, (size_t)0, pool->allocated,
			pool->freecount, pool->freemax, pool->fillcount,
			pool->gets, "N");
		pool = ISC_LIST_NEXT(pool, link);
	}

#if ISC_MEM_TRACKLINES
	print_active(ctx, out);
#endif /* if ISC_MEM_TRACKLINES */

	MCTXUNLOCK(ctx);
}

void *
isc__mem_allocate(isc_mem_t *ctx, size_t size, int flags FLARG) {
	void *ptr = NULL;

	REQUIRE(VALID_CONTEXT(ctx));

	ptr = mem_get(ctx, size, flags);

	/* Recalculate the real allocated size */
	size = sallocx(ptr, flags | ctx->jemalloc_flags);

	mem_getstats(ctx, size);
	ADD_TRACE(ctx, ptr, size, file, line);

	return ptr;
}

void *
isc__mem_reget(isc_mem_t *ctx, void *old_ptr, size_t old_size, size_t new_size,
	       int flags FLARG) {
	void *new_ptr = NULL;

	if (old_ptr == NULL) {
		REQUIRE(old_size == 0);
		new_ptr = isc__mem_get(ctx, new_size, flags FLARG_PASS);
	} else if (new_size == 0) {
		isc__mem_put(ctx, old_ptr, old_size, flags FLARG_PASS);
	} else {
		DELETE_TRACE(ctx, old_ptr, old_size, file, line);
		mem_putstats(ctx, old_size);

		new_ptr = mem_realloc(ctx, old_ptr, old_size, new_size, flags);

		mem_getstats(ctx, new_size);
		ADD_TRACE(ctx, new_ptr, new_size, file, line);

		/*
		 * We want to postpone the call to water in edge case
		 * where the realloc will exactly hit on the boundary of
		 * the water and we would call water twice.
		 */
	}

	return new_ptr;
}

void *
isc__mem_reallocate(isc_mem_t *ctx, void *old_ptr, size_t new_size,
		    int flags FLARG) {
	void *new_ptr = NULL;

	REQUIRE(VALID_CONTEXT(ctx));

	if (old_ptr == NULL) {
		new_ptr = isc__mem_allocate(ctx, new_size, flags FLARG_PASS);
	} else if (new_size == 0) {
		isc__mem_free(ctx, old_ptr, flags FLARG_PASS);
	} else {
		size_t old_size = sallocx(old_ptr, flags | ctx->jemalloc_flags);

		DELETE_TRACE(ctx, old_ptr, old_size, file, line);
		mem_putstats(ctx, old_size);

		new_ptr = mem_realloc(ctx, old_ptr, old_size, new_size, flags);

		/* Recalculate the real allocated size */
		new_size = sallocx(new_ptr, flags | ctx->jemalloc_flags);

		mem_getstats(ctx, new_size);
		ADD_TRACE(ctx, new_ptr, new_size, file, line);

		/*
		 * We want to postpone the call to water in edge case
		 * where the realloc will exactly hit on the boundary of
		 * the water and we would call water twice.
		 */
	}

	return new_ptr;
}

void
isc__mem_free(isc_mem_t *ctx, void *ptr, int flags FLARG) {
	size_t size = 0;

	REQUIRE(VALID_CONTEXT(ctx));
	REQUIRE(ptr != NULL);

	size = sallocx(ptr, flags | ctx->jemalloc_flags);

	DELETE_TRACE(ctx, ptr, size, file, line);

	mem_putstats(ctx, size);
	mem_put(ctx, ptr, size, flags);
}

/*
 * Other useful things.
 */

char *
isc__mem_strdup(isc_mem_t *mctx, const char *s FLARG) {
	size_t len;
	char *ns = NULL;

	REQUIRE(VALID_CONTEXT(mctx));
	REQUIRE(s != NULL);

	len = strlen(s) + 1;

	ns = isc__mem_allocate(mctx, len, 0 FLARG_PASS);

	strlcpy(ns, s, len);

	return ns;
}

char *
isc__mem_strndup(isc_mem_t *mctx, const char *s, size_t size FLARG) {
	size_t len;
	char *ns = NULL;

	REQUIRE(VALID_CONTEXT(mctx));
	REQUIRE(s != NULL);
	REQUIRE(size != 0);

	len = strlen(s) + 1;
	if (len > size) {
		len = size;
	}

	ns = isc__mem_allocate(mctx, len, 0 FLARG_PASS);

	strlcpy(ns, s, len);

	return ns;
}

void
isc_mem_setdestroycheck(isc_mem_t *ctx, bool flag) {
	REQUIRE(VALID_CONTEXT(ctx));

	MCTXLOCK(ctx);

	ctx->checkfree = flag;

	MCTXUNLOCK(ctx);
}

size_t
isc_mem_inuse(isc_mem_t *ctx) {
	REQUIRE(VALID_CONTEXT(ctx));

	return atomic_load_relaxed(&ctx->inuse);
}

void
isc_mem_clearwater(isc_mem_t *mctx) {
	isc_mem_setwater(mctx, 0, 0);
}

void
isc_mem_setwater(isc_mem_t *ctx, size_t hiwater, size_t lowater) {
	REQUIRE(VALID_CONTEXT(ctx));
	REQUIRE(hiwater >= lowater);

	atomic_store_release(&ctx->hi_water, hiwater);
	atomic_store_release(&ctx->lo_water, lowater);

	return;
}

bool
isc_mem_isovermem(isc_mem_t *ctx) {
	REQUIRE(VALID_CONTEXT(ctx));

	bool is_overmem = atomic_load_relaxed(&ctx->is_overmem);

	if (!is_overmem) {
		/* We are not overmem, check whether we should be? */
		size_t hiwater = atomic_load_relaxed(&ctx->hi_water);
		if (hiwater == 0) {
			return false;
		}

		size_t inuse = atomic_load_relaxed(&ctx->inuse);
		if (inuse <= hiwater) {
			return false;
		}

		if ((isc_mem_debugging & ISC_MEM_DEBUGUSAGE) != 0) {
			fprintf(stderr,
				"overmem mctx %p inuse %zu hi_water %zu\n", ctx,
				inuse, hiwater);
		}

		atomic_store_relaxed(&ctx->is_overmem, true);
		return true;
	} else {
		/* We are overmem, check whether we should not be? */
		size_t lowater = atomic_load_relaxed(&ctx->lo_water);
		if (lowater == 0) {
			return false;
		}

		size_t inuse = atomic_load_relaxed(&ctx->inuse);
		if (inuse >= lowater) {
			return true;
		}

		if ((isc_mem_debugging & ISC_MEM_DEBUGUSAGE) != 0) {
			fprintf(stderr,
				"overmem mctx %p inuse %zu lo_water %zu\n", ctx,
				inuse, lowater);
		}
		atomic_store_relaxed(&ctx->is_overmem, false);
		return false;
	}
}

void
isc_mem_setname(isc_mem_t *ctx, const char *name) {
	REQUIRE(VALID_CONTEXT(ctx));

	LOCK(&ctx->lock);
	strlcpy(ctx->name, name, sizeof(ctx->name));
	UNLOCK(&ctx->lock);
}

const char *
isc_mem_getname(isc_mem_t *ctx) {
	REQUIRE(VALID_CONTEXT(ctx));

	if (ctx->name[0] == 0) {
		return "";
	}

	return ctx->name;
}

/*
 * Memory pool stuff
 */

void
isc__mempool_create(isc_mem_t *restrict mctx, const size_t element_size,
		    isc_mempool_t **restrict mpctxp FLARG) {
	isc_mempool_t *restrict mpctx = NULL;
	size_t size = element_size;

	REQUIRE(VALID_CONTEXT(mctx));
	REQUIRE(size > 0U);
	REQUIRE(mpctxp != NULL && *mpctxp == NULL);

	/*
	 * Mempools are stored as a linked list of element.
	 */
	if (size < sizeof(element)) {
		size = sizeof(element);
	}

	/*
	 * Allocate space for this pool, initialize values, and if all
	 * works well, attach to the memory context.
	 */
	mpctx = isc_mem_get(mctx, sizeof(isc_mempool_t));

	*mpctx = (isc_mempool_t){
		.size = size,
		.freemax = 1,
		.fillcount = 1,
	};

#if ISC_MEM_TRACKLINES
	if ((mctx->debugging & ISC_MEM_DEBUGTRACE) != 0) {
		fprintf(stderr, "create pool %p file %s line %u mctx %p\n",
			mpctx, file, line, mctx);
	}
#endif /* ISC_MEM_TRACKLINES */

	isc_mem_attach(mctx, &mpctx->mctx);
	mpctx->magic = MEMPOOL_MAGIC;

	*mpctxp = (isc_mempool_t *)mpctx;

	MCTXLOCK(mctx);
	ISC_LIST_INITANDAPPEND(mctx->pools, mpctx, link);
	mctx->poolcnt++;
	MCTXUNLOCK(mctx);
}

void
isc_mempool_setname(isc_mempool_t *restrict mpctx, const char *name) {
	REQUIRE(VALID_MEMPOOL(mpctx));
	REQUIRE(name != NULL);

	strlcpy(mpctx->name, name, sizeof(mpctx->name));
}

void
isc__mempool_destroy(isc_mempool_t **restrict mpctxp FLARG) {
	isc_mempool_t *restrict mpctx = NULL;
	isc_mem_t *mctx = NULL;
	element *restrict item = NULL;

	REQUIRE(mpctxp != NULL);
	REQUIRE(VALID_MEMPOOL(*mpctxp));

	mpctx = *mpctxp;
	*mpctxp = NULL;

	mctx = mpctx->mctx;

#if ISC_MEM_TRACKLINES
	if ((mctx->debugging & ISC_MEM_DEBUGTRACE) != 0) {
		fprintf(stderr, "destroy pool %p file %s line %u mctx %p\n",
			mpctx, file, line, mctx);
	}
#endif

	if (mpctx->allocated > 0) {
		UNEXPECTED_ERROR("mempool %s leaked memory", mpctx->name);
	}
	REQUIRE(mpctx->allocated == 0);

	/*
	 * Return any items on the free list
	 */
	while (mpctx->items != NULL) {
		INSIST(mpctx->freecount > 0);
		mpctx->freecount--;

		item = mpctx->items;
		mpctx->items = item->next;

		mem_putstats(mctx, mpctx->size);
		mem_put(mctx, item, mpctx->size, 0);
	}

	/*
	 * Remove our linked list entry from the memory context.
	 */
	MCTXLOCK(mctx);
	ISC_LIST_UNLINK(mctx->pools, mpctx, link);
	mctx->poolcnt--;
	MCTXUNLOCK(mctx);

	mpctx->magic = 0;

	isc_mem_putanddetach(&mpctx->mctx, mpctx, sizeof(isc_mempool_t));
}

void *
isc__mempool_get(isc_mempool_t *restrict mpctx FLARG) {
	element *restrict item = NULL;

	REQUIRE(VALID_MEMPOOL(mpctx));

	mpctx->allocated++;

	if (mpctx->items == NULL) {
		isc_mem_t *mctx = mpctx->mctx;
#if !__SANITIZE_ADDRESS__
		const size_t fillcount = mpctx->fillcount;
#else
		const size_t fillcount = 1;
#endif
		/*
		 * We need to dip into the well.  Fill up our free list.
		 */
		for (size_t i = 0; i < fillcount; i++) {
			item = mem_get(mctx, mpctx->size, 0);
			mem_getstats(mctx, mpctx->size);
			item->next = mpctx->items;
			mpctx->items = item;
			mpctx->freecount++;
		}
	}

	INSIST(mpctx->items != NULL);
	item = mpctx->items;

	mpctx->items = item->next;

	INSIST(mpctx->freecount > 0);
	mpctx->freecount--;
	mpctx->gets++;

	ADD_TRACE(mpctx->mctx, item, mpctx->size, file, line);

	return item;
}

/* coverity[+free : arg-1] */
void
isc__mempool_put(isc_mempool_t *restrict mpctx, void *mem FLARG) {
	element *restrict item = NULL;

	REQUIRE(VALID_MEMPOOL(mpctx));
	REQUIRE(mem != NULL);

	isc_mem_t *mctx = mpctx->mctx;
	const size_t freecount = mpctx->freecount;
#if !__SANITIZE_ADDRESS__
	const size_t freemax = mpctx->freemax;
#else
	const size_t freemax = 0;
#endif

	INSIST(mpctx->allocated > 0);
	mpctx->allocated--;

	DELETE_TRACE(mctx, mem, mpctx->size, file, line);

	/*
	 * If our free list is full, return this to the mctx directly.
	 */
	if (freecount >= freemax) {
		mem_putstats(mctx, mpctx->size);
		mem_put(mctx, mem, mpctx->size, 0);
		return;
	}

	/*
	 * Otherwise, attach it to our free list and bump the counter.
	 */
	item = (element *)mem;
	item->next = mpctx->items;
	mpctx->items = item;
	mpctx->freecount++;
}

/*
 * Quotas
 */

void
isc_mempool_setfreemax(isc_mempool_t *restrict mpctx,
		       const unsigned int limit) {
	REQUIRE(VALID_MEMPOOL(mpctx));
	mpctx->freemax = limit;
}

unsigned int
isc_mempool_getfreemax(isc_mempool_t *restrict mpctx) {
	REQUIRE(VALID_MEMPOOL(mpctx));

	return mpctx->freemax;
}

unsigned int
isc_mempool_getfreecount(isc_mempool_t *restrict mpctx) {
	REQUIRE(VALID_MEMPOOL(mpctx));

	return mpctx->freecount;
}

unsigned int
isc_mempool_getallocated(isc_mempool_t *restrict mpctx) {
	REQUIRE(VALID_MEMPOOL(mpctx));

	return mpctx->allocated;
}

void
isc_mempool_setfillcount(isc_mempool_t *restrict mpctx,
			 unsigned int const limit) {
	REQUIRE(VALID_MEMPOOL(mpctx));
	REQUIRE(limit > 0);

	mpctx->fillcount = limit;
}

unsigned int
isc_mempool_getfillcount(isc_mempool_t *restrict mpctx) {
	REQUIRE(VALID_MEMPOOL(mpctx));

	return mpctx->fillcount;
}

/*
 * Requires contextslock to be held by caller.
 */
#if ISC_MEM_TRACKLINES
static void
print_contexts(FILE *file) {
	isc_mem_t *ctx;

	for (ctx = ISC_LIST_HEAD(contexts); ctx != NULL;
	     ctx = ISC_LIST_NEXT(ctx, link))
	{
		fprintf(file, "context: %p (%s): %" PRIuFAST32 " references\n",
			ctx, ctx->name[0] == 0 ? "<unknown>" : ctx->name,
			isc_refcount_current(&ctx->references));
		print_active(ctx, file);
	}
	fflush(file);
}
#endif

static atomic_uintptr_t checkdestroyed = 0;

void
isc_mem_checkdestroyed(FILE *file) {
	atomic_store_release(&checkdestroyed, (uintptr_t)file);
}

void
isc__mem_checkdestroyed(void) {
	FILE *file = (FILE *)atomic_load_acquire(&checkdestroyed);

	if (file == NULL) {
		return;
	}

	LOCK(&contextslock);
	if (!ISC_LIST_EMPTY(contexts)) {
#if ISC_MEM_TRACKLINES
		if ((isc_mem_debugging & TRACE_OR_RECORD) != 0) {
			print_contexts(file);
		}
#endif /* if ISC_MEM_TRACKLINES */
		UNREACHABLE();
	}
	UNLOCK(&contextslock);
}

unsigned int
isc_mem_references(isc_mem_t *ctx) {
	return isc_refcount_current(&ctx->references);
}

#ifdef HAVE_LIBXML2
#define TRY0(a)                     \
	do {                        \
		xmlrc = (a);        \
		if (xmlrc < 0)      \
			goto error; \
	} while (0)
static int
xml_renderctx(isc_mem_t *ctx, size_t *inuse, xmlTextWriterPtr writer) {
	REQUIRE(VALID_CONTEXT(ctx));

	int xmlrc;

	MCTXLOCK(ctx);

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "context"));

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "id"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%p", ctx));
	TRY0(xmlTextWriterEndElement(writer)); /* id */

	if (ctx->name[0] != 0) {
		TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "name"));
		TRY0(xmlTextWriterWriteFormatString(writer, "%s", ctx->name));
		TRY0(xmlTextWriterEndElement(writer)); /* name */
	}

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "references"));
	TRY0(xmlTextWriterWriteFormatString(
		writer, "%" PRIuFAST32,
		isc_refcount_current(&ctx->references)));
	TRY0(xmlTextWriterEndElement(writer)); /* references */

	*inuse += isc_mem_inuse(ctx);
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "inuse"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64 "",
					    (uint64_t)isc_mem_inuse(ctx)));
	TRY0(xmlTextWriterEndElement(writer)); /* inuse */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "malloced"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64 "",
					    (uint64_t)isc_mem_inuse(ctx)));
	TRY0(xmlTextWriterEndElement(writer)); /* malloced */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "pools"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%u", ctx->poolcnt));
	TRY0(xmlTextWriterEndElement(writer)); /* pools */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "hiwater"));
	TRY0(xmlTextWriterWriteFormatString(
		writer, "%" PRIu64 "",
		(uint64_t)atomic_load_relaxed(&ctx->hi_water)));
	TRY0(xmlTextWriterEndElement(writer)); /* hiwater */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "lowater"));
	TRY0(xmlTextWriterWriteFormatString(
		writer, "%" PRIu64 "",
		(uint64_t)atomic_load_relaxed(&ctx->lo_water)));
	TRY0(xmlTextWriterEndElement(writer)); /* lowater */

	TRY0(xmlTextWriterEndElement(writer)); /* context */

error:
	MCTXUNLOCK(ctx);

	return xmlrc;
}

int
isc_mem_renderxml(void *writer0) {
	isc_mem_t *ctx;
	size_t inuse = 0;
	int xmlrc;
	xmlTextWriterPtr writer = (xmlTextWriterPtr)writer0;

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "contexts"));

	LOCK(&contextslock);
	for (ctx = ISC_LIST_HEAD(contexts); ctx != NULL;
	     ctx = ISC_LIST_NEXT(ctx, link))
	{
		xmlrc = xml_renderctx(ctx, &inuse, writer);
		if (xmlrc < 0) {
			UNLOCK(&contextslock);
			goto error;
		}
	}
	UNLOCK(&contextslock);

	TRY0(xmlTextWriterEndElement(writer)); /* contexts */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "summary"));

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "Malloced"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64 "",
					    (uint64_t)inuse));
	TRY0(xmlTextWriterEndElement(writer)); /* malloced */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "InUse"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64 "",
					    (uint64_t)inuse));
	TRY0(xmlTextWriterEndElement(writer)); /* InUse */

	TRY0(xmlTextWriterEndElement(writer)); /* summary */
error:
	return xmlrc;
}

#endif /* HAVE_LIBXML2 */

#ifdef HAVE_JSON_C
#define CHECKMEM(m) RUNTIME_CHECK(m != NULL)

static isc_result_t
json_renderctx(isc_mem_t *ctx, size_t *inuse, json_object *array) {
	REQUIRE(VALID_CONTEXT(ctx));
	REQUIRE(array != NULL);

	json_object *ctxobj, *obj;
	char buf[1024];

	MCTXLOCK(ctx);

	*inuse += isc_mem_inuse(ctx);

	ctxobj = json_object_new_object();
	CHECKMEM(ctxobj);

	snprintf(buf, sizeof(buf), "%p", ctx);
	obj = json_object_new_string(buf);
	CHECKMEM(obj);
	json_object_object_add(ctxobj, "id", obj);

	if (ctx->name[0] != 0) {
		obj = json_object_new_string(ctx->name);
		CHECKMEM(obj);
		json_object_object_add(ctxobj, "name", obj);
	}

	obj = json_object_new_int64(isc_refcount_current(&ctx->references));
	CHECKMEM(obj);
	json_object_object_add(ctxobj, "references", obj);

	obj = json_object_new_int64(isc_mem_inuse(ctx));
	CHECKMEM(obj);
	json_object_object_add(ctxobj, "malloced", obj);

	obj = json_object_new_int64(isc_mem_inuse(ctx));
	CHECKMEM(obj);
	json_object_object_add(ctxobj, "inuse", obj);

	obj = json_object_new_int64(ctx->poolcnt);
	CHECKMEM(obj);
	json_object_object_add(ctxobj, "pools", obj);

	obj = json_object_new_int64(atomic_load_relaxed(&ctx->hi_water));
	CHECKMEM(obj);
	json_object_object_add(ctxobj, "hiwater", obj);

	obj = json_object_new_int64(atomic_load_relaxed(&ctx->lo_water));
	CHECKMEM(obj);
	json_object_object_add(ctxobj, "lowater", obj);

	MCTXUNLOCK(ctx);
	json_object_array_add(array, ctxobj);
	return ISC_R_SUCCESS;
}

isc_result_t
isc_mem_renderjson(void *memobj0) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_mem_t *ctx;
	size_t inuse = 0;
	json_object *ctxarray, *obj;
	json_object *memobj = (json_object *)memobj0;

	ctxarray = json_object_new_array();
	CHECKMEM(ctxarray);

	LOCK(&contextslock);
	for (ctx = ISC_LIST_HEAD(contexts); ctx != NULL;
	     ctx = ISC_LIST_NEXT(ctx, link))
	{
		result = json_renderctx(ctx, &inuse, ctxarray);
		if (result != ISC_R_SUCCESS) {
			UNLOCK(&contextslock);
			goto error;
		}
	}
	UNLOCK(&contextslock);

	obj = json_object_new_int64(inuse);
	CHECKMEM(obj);
	json_object_object_add(memobj, "InUse", obj);

	obj = json_object_new_int64(inuse);
	CHECKMEM(obj);
	json_object_object_add(memobj, "Malloced", obj);

	json_object_object_add(memobj, "contexts", ctxarray);
	return ISC_R_SUCCESS;

error:
	if (ctxarray != NULL) {
		json_object_put(ctxarray);
	}
	return result;
}
#endif /* HAVE_JSON_C */

void
isc__mem_create(isc_mem_t **mctxp FLARG) {
	mem_create(mctxp, isc_mem_debugging, isc_mem_defaultflags, 0);
#if ISC_MEM_TRACKLINES
	if ((isc_mem_debugging & ISC_MEM_DEBUGTRACE) != 0) {
		fprintf(stderr, "create mctx %p file %s line %u\n", *mctxp,
			file, line);
	}
#endif /* ISC_MEM_TRACKLINES */
}

void
isc__mem_create_arena(isc_mem_t **mctxp FLARG) {
	unsigned int arena_no = ISC_MEM_ILLEGAL_ARENA;

	RUNTIME_CHECK(mem_jemalloc_arena_create(&arena_no));

	/*
	 * We use MALLOCX_TCACHE_NONE to bypass the tcache and route
	 * allocations directly to the arena. That is a recommendation
	 * from jemalloc developers:
	 *
	 * https://github.com/jemalloc/jemalloc/issues/2483#issuecomment-1698173849
	 */
	mem_create(mctxp, isc_mem_debugging, isc_mem_defaultflags,
		   arena_no == ISC_MEM_ILLEGAL_ARENA
			   ? 0
			   : MALLOCX_ARENA(arena_no) | MALLOCX_TCACHE_NONE);
	(*mctxp)->jemalloc_arena = arena_no;
#if ISC_MEM_TRACKLINES
	if ((isc_mem_debugging & ISC_MEM_DEBUGTRACE) != 0) {
		fprintf(stderr,
			"create mctx %p file %s line %u for jemalloc arena "
			"%u\n",
			*mctxp, file, line, arena_no);
	}
#endif /* ISC_MEM_TRACKLINES */
}

#ifdef JEMALLOC_API_SUPPORTED
static bool
jemalloc_set_ssize_value(const char *valname, ssize_t newval) {
	int ret;

	ret = mallctl(valname, NULL, NULL, &newval, sizeof(newval));
	return ret == 0;
}
#endif /* JEMALLOC_API_SUPPORTED */

static isc_result_t
mem_set_arena_ssize_value(isc_mem_t *mctx, const char *arena_valname,
			  const ssize_t newval) {
	REQUIRE(VALID_CONTEXT(mctx));
#ifdef JEMALLOC_API_SUPPORTED
	bool ret;
	char buf[256] = { 0 };

	if (mctx->jemalloc_arena == ISC_MEM_ILLEGAL_ARENA) {
		return ISC_R_UNEXPECTED;
	}

	(void)snprintf(buf, sizeof(buf), "arena.%u.%s", mctx->jemalloc_arena,
		       arena_valname);

	ret = jemalloc_set_ssize_value(buf, newval);

	if (!ret) {
		return ISC_R_FAILURE;
	}

	return ISC_R_SUCCESS;
#else
	UNUSED(arena_valname);
	UNUSED(newval);
	return ISC_R_NOTIMPLEMENTED;
#endif
}

isc_result_t
isc_mem_arena_set_muzzy_decay_ms(isc_mem_t *mctx, const ssize_t decay_ms) {
	return mem_set_arena_ssize_value(mctx, "muzzy_decay_ms", decay_ms);
}

isc_result_t
isc_mem_arena_set_dirty_decay_ms(isc_mem_t *mctx, const ssize_t decay_ms) {
	return mem_set_arena_ssize_value(mctx, "dirty_decay_ms", decay_ms);
}

void
isc__mem_printactive(isc_mem_t *ctx, FILE *file) {
#if ISC_MEM_TRACKLINES
	REQUIRE(VALID_CONTEXT(ctx));
	REQUIRE(file != NULL);

	print_active(ctx, file);
#else  /* if ISC_MEM_TRACKLINES */
	UNUSED(ctx);
	UNUSED(file);
#endif /* if ISC_MEM_TRACKLINES */
}
