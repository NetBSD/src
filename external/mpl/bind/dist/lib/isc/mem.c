/*	$NetBSD: mem.c,v 1.13 2023/01/25 21:43:31 christos Exp $	*/

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

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <isc/bind9.h>
#include <isc/hash.h>
#include <isc/lib.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/once.h>
#include <isc/print.h>
#include <isc/refcount.h>
#include <isc/strerr.h>
#include <isc/string.h>
#include <isc/util.h>

#ifdef HAVE_LIBXML2
#include <libxml/xmlwriter.h>
#define ISC_XMLCHAR (const xmlChar *)
#endif /* HAVE_LIBXML2 */

#ifdef HAVE_JSON_C
#include <json_object.h>
#endif /* HAVE_JSON_C */

#include "mem_p.h"

#define MCTXLOCK(m)   LOCK(&m->lock)
#define MCTXUNLOCK(m) UNLOCK(&m->lock)

#ifndef ISC_MEM_DEBUGGING
#define ISC_MEM_DEBUGGING 0
#endif /* ifndef ISC_MEM_DEBUGGING */
LIBISC_EXTERNAL_DATA unsigned int isc_mem_debugging = ISC_MEM_DEBUGGING;
LIBISC_EXTERNAL_DATA unsigned int isc_mem_defaultflags = ISC_MEMFLAG_DEFAULT;

/*
 * Constants.
 */

#define DEF_MAX_SIZE	  1100
#define DEF_MEM_TARGET	  4096
#define ALIGNMENT_SIZE	  8U /*%< must be a power of 2 */
#define NUM_BASIC_BLOCKS  64 /*%< must be > 1 */
#define TABLE_INCREMENT	  1024
#define DEBUG_TABLE_COUNT 512U

/*
 * Types.
 */
typedef struct isc__mem isc__mem_t;
typedef struct isc__mempool isc__mempool_t;

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

typedef struct {
	/*!
	 * This structure must be ALIGNMENT_SIZE bytes.
	 */
	union {
		size_t size;
		isc__mem_t *ctx;
		char bytes[ALIGNMENT_SIZE];
	} u;
} size_info;

struct stats {
	unsigned long gets;
	unsigned long totalgets;
	unsigned long blocks;
	unsigned long freefrags;
};

#define MEM_MAGIC	 ISC_MAGIC('M', 'e', 'm', 'C')
#define VALID_CONTEXT(c) ISC_MAGIC_VALID(c, MEM_MAGIC)

/* List of all active memory contexts. */

static ISC_LIST(isc__mem_t) contexts;

static isc_once_t init_once = ISC_ONCE_INIT;
static isc_once_t shut_once = ISC_ONCE_INIT;
static isc_mutex_t contextslock;

/*%
 * Total size of lost memory due to a bug of external library.
 * Locked by the global lock.
 */
static uint64_t totallost;

/*%
 * Memory allocation and free function definitions.
 * isc__memalloc_t must deal with memory allocation failure
 * and must never return NULL.
 */
typedef void *(*isc__memalloc_t)(size_t);
typedef void (*isc__memfree_t)(void *);

struct isc__mem {
	isc_mem_t common;
	unsigned int flags;
	isc_mutex_t lock;
	isc__memalloc_t memalloc;
	isc__memfree_t memfree;
	size_t max_size;
	bool checkfree;
	struct stats *stats;
	isc_refcount_t references;
	char name[16];
	void *tag;
	size_t total;
	size_t inuse;
	size_t maxinuse;
	size_t malloced;
	size_t maxmalloced;
	size_t hi_water;
	size_t lo_water;
	bool hi_called;
	bool is_overmem;
	isc_mem_water_t water;
	void *water_arg;
	ISC_LIST(isc__mempool_t) pools;
	unsigned int poolcnt;

	/*  ISC_MEMFLAG_INTERNAL */
	size_t mem_target;
	element **freelists;
	element *basic_blocks;
	unsigned char **basic_table;
	unsigned int basic_table_count;
	unsigned int basic_table_size;
	unsigned char *lowest;
	unsigned char *highest;

#if ISC_MEM_TRACKLINES
	debuglist_t *debuglist;
	size_t debuglistcnt;
#endif /* if ISC_MEM_TRACKLINES */

	ISC_LINK(isc__mem_t) link;
};

#define MEMPOOL_MAGIC	 ISC_MAGIC('M', 'E', 'M', 'p')
#define VALID_MEMPOOL(c) ISC_MAGIC_VALID(c, MEMPOOL_MAGIC)

struct isc__mempool {
	/* always unlocked */
	isc_mempool_t common;	       /*%< common header of mempool's */
	isc__mem_t *mctx;	       /*%< our memory context */
	ISC_LINK(isc__mempool_t) link; /*%< next pool in this mem context */
	element *items;		       /*%< low water item list */
	size_t size;		       /*%< size of each item on this pool */
	unsigned int maxalloc;	       /*%< max number of items allowed */
	unsigned int allocated;	       /*%< # of items currently given out */
	unsigned int freecount;	       /*%< # of items on reserved list */
	unsigned int freemax;	       /*%< # of items allowed on free list */
	unsigned int fillcount;	       /*%< # of items to fetch on each fill */
	/*%< Stats only. */
	unsigned int gets; /*%< # of requests to this pool */
			   /*%< Debugging only. */
#if ISC_MEMPOOL_NAMES
	char name[16]; /*%< printed name in stats reports */
#endif		       /* if ISC_MEMPOOL_NAMES */
};

/*
 * Private Inline-able.
 */

#if !ISC_MEM_TRACKLINES
#define ADD_TRACE(a, b, c, d, e)
#define DELETE_TRACE(a, b, c, d, e)
#define ISC_MEMFUNC_SCOPE
#else /* if !ISC_MEM_TRACKLINES */
#define TRACE_OR_RECORD (ISC_MEM_DEBUGTRACE | ISC_MEM_DEBUGRECORD)
#define ADD_TRACE(a, b, c, d, e)                                               \
	do {                                                                   \
		if (ISC_UNLIKELY((isc_mem_debugging & TRACE_OR_RECORD) != 0 && \
				 b != NULL))                                   \
			add_trace_entry(a, b, c, d, e);                        \
	} while (0)
#define DELETE_TRACE(a, b, c, d, e)                                            \
	do {                                                                   \
		if (ISC_UNLIKELY((isc_mem_debugging & TRACE_OR_RECORD) != 0 && \
				 b != NULL))                                   \
			delete_trace_entry(a, b, c, d, e);                     \
	} while (0)

static void
print_active(isc__mem_t *ctx, FILE *out);

#endif /* ISC_MEM_TRACKLINES */

static void *
isc___mem_get(isc_mem_t *ctx, size_t size FLARG);
static void
isc___mem_put(isc_mem_t *ctx, void *ptr, size_t size FLARG);
static void
isc___mem_putanddetach(isc_mem_t **ctxp, void *ptr, size_t size FLARG);
static void *
isc___mem_allocate(isc_mem_t *ctx, size_t size FLARG);
static void *
isc___mem_reallocate(isc_mem_t *ctx, void *ptr, size_t size FLARG);
static char *
isc___mem_strdup(isc_mem_t *mctx, const char *s FLARG);
static char *
isc___mem_strndup(isc_mem_t *mctx0, const char *s, size_t size FLARG);
static void
isc___mem_free(isc_mem_t *ctx, void *ptr FLARG);

static isc_memmethods_t memmethods = {
	isc___mem_get,	    isc___mem_put,	  isc___mem_putanddetach,
	isc___mem_allocate, isc___mem_reallocate, isc___mem_strdup,
	isc___mem_strndup,  isc___mem_free,
};

#if ISC_MEM_TRACKLINES
/*!
 * mctx must be locked.
 */
static void
add_trace_entry(isc__mem_t *mctx, const void *ptr, size_t size FLARG) {
	debuglink_t *dl;
	uint32_t hash;
	uint32_t idx;

	if ((isc_mem_debugging & ISC_MEM_DEBUGTRACE) != 0) {
		fprintf(stderr, "add %p size %zu file %s line %u mctx %p\n",
			ptr, size, file, line, mctx);
	}

	if (mctx->debuglist == NULL) {
		return;
	}

#ifdef __COVERITY__
	/*
	 * Use simple conversion from pointer to hash to avoid
	 * tainting 'ptr' due to byte swap in isc_hash_function.
	 */
	hash = (uintptr_t)ptr >> 3;
#else
	hash = isc_hash_function(&ptr, sizeof(ptr), true);
#endif
	idx = hash % DEBUG_TABLE_COUNT;

	dl = malloc(sizeof(debuglink_t));
	INSIST(dl != NULL);
	mctx->malloced += sizeof(debuglink_t);
	if (mctx->malloced > mctx->maxmalloced) {
		mctx->maxmalloced = mctx->malloced;
	}

	ISC_LINK_INIT(dl, link);
	dl->ptr = ptr;
	dl->size = size;
	dl->file = file;
	dl->line = line;

	ISC_LIST_PREPEND(mctx->debuglist[idx], dl, link);
	mctx->debuglistcnt++;
}

static void
delete_trace_entry(isc__mem_t *mctx, const void *ptr, size_t size,
		   const char *file, unsigned int line) {
	debuglink_t *dl;
	uint32_t hash;
	uint32_t idx;

	if ((isc_mem_debugging & ISC_MEM_DEBUGTRACE) != 0) {
		fprintf(stderr, "del %p size %zu file %s line %u mctx %p\n",
			ptr, size, file, line, mctx);
	}

	if (mctx->debuglist == NULL) {
		return;
	}

#ifdef __COVERITY__
	/*
	 * Use simple conversion from pointer to hash to avoid
	 * tainting 'ptr' due to byte swap in isc_hash_function.
	 */
	hash = (uintptr_t)ptr >> 3;
#else
	hash = isc_hash_function(&ptr, sizeof(ptr), true);
#endif
	idx = hash % DEBUG_TABLE_COUNT;

	dl = ISC_LIST_HEAD(mctx->debuglist[idx]);
	while (ISC_LIKELY(dl != NULL)) {
		if (ISC_UNLIKELY(dl->ptr == ptr)) {
			ISC_LIST_UNLINK(mctx->debuglist[idx], dl, link);
			mctx->malloced -= sizeof(*dl);
			free(dl);
			return;
		}
		dl = ISC_LIST_NEXT(dl, link);
	}

	/*
	 * If we get here, we didn't find the item on the list.  We're
	 * screwed.
	 */
	UNREACHABLE();
}
#endif /* ISC_MEM_TRACKLINES */

static size_t
rmsize(size_t size) {
	/*
	 * round down to ALIGNMENT_SIZE
	 */
	return (size & (~(ALIGNMENT_SIZE - 1)));
}

static size_t
quantize(size_t size) {
	/*!
	 * Round up the result in order to get a size big
	 * enough to satisfy the request and be aligned on ALIGNMENT_SIZE
	 * byte boundaries.
	 */

	if (size == 0U) {
		return (ALIGNMENT_SIZE);
	}
	return ((size + ALIGNMENT_SIZE - 1) & (~(ALIGNMENT_SIZE - 1)));
}

static void
more_basic_blocks(isc__mem_t *ctx) {
	void *tmp;
	unsigned char *curr, *next;
	unsigned char *first, *last;
	unsigned char **table;
	unsigned int table_size;

	/* Require: we hold the context lock. */

	INSIST(ctx->basic_table_count <= ctx->basic_table_size);
	if (ctx->basic_table_count == ctx->basic_table_size) {
		table_size = ctx->basic_table_size + TABLE_INCREMENT;
		table = (ctx->memalloc)(table_size * sizeof(unsigned char *));
		ctx->malloced += table_size * sizeof(unsigned char *);
		if (ctx->malloced > ctx->maxmalloced) {
			ctx->maxmalloced = ctx->malloced;
		}
		if (ctx->basic_table_size != 0) {
			memmove(table, ctx->basic_table,
				ctx->basic_table_size *
					sizeof(unsigned char *));
			(ctx->memfree)(ctx->basic_table);
			ctx->malloced -= ctx->basic_table_size *
					 sizeof(unsigned char *);
		}
		ctx->basic_table = table;
		ctx->basic_table_size = table_size;
	}

	tmp = (ctx->memalloc)(NUM_BASIC_BLOCKS * ctx->mem_target);
	ctx->total += NUM_BASIC_BLOCKS * ctx->mem_target;
	ctx->basic_table[ctx->basic_table_count] = tmp;
	ctx->basic_table_count++;
	ctx->malloced += NUM_BASIC_BLOCKS * ctx->mem_target;
	if (ctx->malloced > ctx->maxmalloced) {
		ctx->maxmalloced = ctx->malloced;
	}

	curr = tmp;
	next = curr + ctx->mem_target;
	for (int i = 0; i < (NUM_BASIC_BLOCKS - 1); i++) {
		((element *)curr)->next = (element *)next;
		curr = next;
		next += ctx->mem_target;
	}
	/*
	 * curr is now pointing at the last block in the
	 * array.
	 */
	((element *)curr)->next = NULL;
	first = tmp;
	last = first + NUM_BASIC_BLOCKS * ctx->mem_target - 1;
	if (first < ctx->lowest || ctx->lowest == NULL) {
		ctx->lowest = first;
	}
	if (last > ctx->highest) {
		ctx->highest = last;
	}
	ctx->basic_blocks = tmp;
}

static void
more_frags(isc__mem_t *ctx, size_t new_size) {
	int frags;
	size_t total_size;
	void *tmp;
	unsigned char *curr, *next;

	/*!
	 * Try to get more fragments by chopping up a basic block.
	 */

	if (ctx->basic_blocks == NULL) {
		more_basic_blocks(ctx);
	}
	INSIST(ctx->basic_blocks != NULL);

	total_size = ctx->mem_target;
	tmp = ctx->basic_blocks;
	ctx->basic_blocks = ctx->basic_blocks->next;
	frags = (int)(total_size / new_size);
	ctx->stats[new_size].blocks++;
	ctx->stats[new_size].freefrags += frags;
	/*
	 * Set up a linked-list of blocks of size
	 * "new_size".
	 */
	curr = tmp;
	next = curr + new_size;
	total_size -= new_size;
	for (int i = 0; i < (frags - 1); i++) {
		((element *)curr)->next = (element *)next;
		curr = next;
		next += new_size;
		total_size -= new_size;
	}
	/*
	 * Add the remaining fragment of the basic block to a free list.
	 */
	total_size = rmsize(total_size);
	if (total_size > 0U) {
		((element *)next)->next = ctx->freelists[total_size];
		ctx->freelists[total_size] = (element *)next;
		ctx->stats[total_size].freefrags++;
	}
	/*
	 * curr is now pointing at the last block in the
	 * array.
	 */
	((element *)curr)->next = NULL;
	ctx->freelists[new_size] = tmp;
}

static void *
mem_getunlocked(isc__mem_t *ctx, size_t size) {
	size_t new_size = quantize(size);
	void *ret;

	if (new_size >= ctx->max_size) {
		/*
		 * memget() was called on something beyond our upper limit.
		 */
		ret = (ctx->memalloc)(size);
		ctx->total += size;
		ctx->inuse += size;
		ctx->stats[ctx->max_size].gets++;
		ctx->stats[ctx->max_size].totalgets++;
		ctx->malloced += size;
		if (ctx->malloced > ctx->maxmalloced) {
			ctx->maxmalloced = ctx->malloced;
		}
		/*
		 * If we don't set new_size to size, then the
		 * ISC_MEMFLAG_FILL code might write over bytes we don't
		 * own.
		 */
		new_size = size;
		goto done;
	}
	/*
	 * If there are no blocks in the free list for this size, get a chunk
	 * of memory and then break it up into "new_size"-sized blocks, adding
	 * them to the free list.
	 */
	if (ctx->freelists[new_size] == NULL) {
		more_frags(ctx, new_size);
	}
	INSIST(ctx->freelists[new_size] != NULL);

	/*
	 * The free list uses the "rounded-up" size "new_size".
	 */

	ret = ctx->freelists[new_size];
	ctx->freelists[new_size] = ctx->freelists[new_size]->next;

	/*
	 * The stats[] uses the _actual_ "size" requested by the
	 * caller, with the caveat (in the code above) that "size" >= the
	 * max. size (max_size) ends up getting recorded as a call to
	 * max_size.
	 */
	ctx->stats[size].gets++;
	ctx->stats[size].totalgets++;
	ctx->stats[new_size].freefrags--;
	ctx->inuse += new_size;

done:
	if (ISC_UNLIKELY((ctx->flags & ISC_MEMFLAG_FILL) != 0) &&
	    ISC_LIKELY(ret != NULL))
	{
		memset(ret, 0xbe, new_size); /* Mnemonic for "beef". */
	}

	return (ret);
}

#if ISC_MEM_CHECKOVERRUN
static void
check_overrun(void *mem, size_t size, size_t new_size) {
	unsigned char *cp;

	cp = (unsigned char *)mem;
	cp += size;
	while (size < new_size) {
		INSIST(*cp == 0xbe);
		cp++;
		size++;
	}
}
#endif /* if ISC_MEM_CHECKOVERRUN */

/* coverity[+free : arg-1] */
static void
mem_putunlocked(isc__mem_t *ctx, void *mem, size_t size) {
	size_t new_size = quantize(size);

	if (new_size >= ctx->max_size) {
		/*
		 * memput() called on something beyond our upper limit.
		 */
		if (ISC_UNLIKELY((ctx->flags & ISC_MEMFLAG_FILL) != 0)) {
			memset(mem, 0xde, size); /* Mnemonic for "dead". */
		}

		(ctx->memfree)(mem);
		INSIST(ctx->stats[ctx->max_size].gets != 0U);
		ctx->stats[ctx->max_size].gets--;
		INSIST(size <= ctx->inuse);
		ctx->inuse -= size;
		ctx->malloced -= size;
		return;
	}

	if (ISC_UNLIKELY((ctx->flags & ISC_MEMFLAG_FILL) != 0)) {
#if ISC_MEM_CHECKOVERRUN
		check_overrun(mem, size, new_size);
#endif					     /* if ISC_MEM_CHECKOVERRUN */
		memset(mem, 0xde, new_size); /* Mnemonic for "dead". */
	}

	/*
	 * The free list uses the "rounded-up" size "new_size".
	 */
	((element *)mem)->next = ctx->freelists[new_size];
	ctx->freelists[new_size] = (element *)mem;

	/*
	 * The stats[] uses the _actual_ "size" requested by the
	 * caller, with the caveat (in the code above) that "size" >= the
	 * max. size (max_size) ends up getting recorded as a call to
	 * max_size.
	 */
	INSIST(ctx->stats[size].gets != 0U);
	ctx->stats[size].gets--;
	ctx->stats[new_size].freefrags++;
	ctx->inuse -= new_size;
}

/*!
 * Perform a malloc, doing memory filling and overrun detection as necessary.
 */
static void *
mem_get(isc__mem_t *ctx, size_t size) {
	char *ret;

#if ISC_MEM_CHECKOVERRUN
	size += 1;
#endif /* if ISC_MEM_CHECKOVERRUN */
	ret = (ctx->memalloc)(size);

	if (ISC_UNLIKELY((ctx->flags & ISC_MEMFLAG_FILL) != 0)) {
		if (ISC_LIKELY(ret != NULL)) {
			memset(ret, 0xbe, size); /* Mnemonic for "beef". */
		}
	}
#if ISC_MEM_CHECKOVERRUN
	else
	{
		if (ISC_LIKELY(ret != NULL)) {
			ret[size - 1] = 0xbe;
		}
	}
#endif /* if ISC_MEM_CHECKOVERRUN */

	return (ret);
}

/*!
 * Perform a free, doing memory filling and overrun detection as necessary.
 */
/* coverity[+free : arg-1] */
static void
mem_put(isc__mem_t *ctx, void *mem, size_t size) {
#if ISC_MEM_CHECKOVERRUN
	INSIST(((unsigned char *)mem)[size] == 0xbe);
	size += 1;
#endif /* if ISC_MEM_CHECKOVERRUN */
	if (ISC_UNLIKELY((ctx->flags & ISC_MEMFLAG_FILL) != 0)) {
		memset(mem, 0xde, size); /* Mnemonic for "dead". */
	}
	(ctx->memfree)(mem);
}

/*!
 * Update internal counters after a memory get.
 */
static void
mem_getstats(isc__mem_t *ctx, size_t size) {
	ctx->total += size;
	ctx->inuse += size;

	if (size > ctx->max_size) {
		ctx->stats[ctx->max_size].gets++;
		ctx->stats[ctx->max_size].totalgets++;
	} else {
		ctx->stats[size].gets++;
		ctx->stats[size].totalgets++;
	}

#if ISC_MEM_CHECKOVERRUN
	size += 1;
#endif /* if ISC_MEM_CHECKOVERRUN */
	ctx->malloced += size;
	if (ctx->malloced > ctx->maxmalloced) {
		ctx->maxmalloced = ctx->malloced;
	}
}

/*!
 * Update internal counters after a memory put.
 */
static void
mem_putstats(isc__mem_t *ctx, void *ptr, size_t size) {
	UNUSED(ptr);

	INSIST(ctx->inuse >= size);
	ctx->inuse -= size;

	if (size > ctx->max_size) {
		INSIST(ctx->stats[ctx->max_size].gets > 0U);
		ctx->stats[ctx->max_size].gets--;
	} else {
		INSIST(ctx->stats[size].gets > 0U);
		ctx->stats[size].gets--;
	}
#if ISC_MEM_CHECKOVERRUN
	size += 1;
#endif /* if ISC_MEM_CHECKOVERRUN */
	ctx->malloced -= size;
}

/*
 * Private.
 */

static void *
default_memalloc(size_t size) {
	void *ptr;

	ptr = malloc(size);

	/*
	 * If the space cannot be allocated, a null pointer is returned. If the
	 * size of the space requested is zero, the behavior is
	 * implementation-defined: either a null pointer is returned, or the
	 * behavior is as if the size were some nonzero value, except that the
	 * returned pointer shall not be used to access an object.
	 * [ISO9899 § 7.22.3]
	 *
	 * [ISO9899]
	 *   ISO/IEC WG 9899:2011: Programming languages - C.
	 *   International Organization for Standardization, Geneva,
	 * Switzerland.
	 *   http://www.open-std.org/JTC1/SC22/WG14/www/docs/n1570.pdf
	 */

	if (ptr == NULL && size != 0) {
		char strbuf[ISC_STRERRORSIZE];
		strerror_r(errno, strbuf, sizeof(strbuf));
		isc_error_fatal(__FILE__, __LINE__, "malloc failed: %s",
				strbuf);
	}

	return (ptr);
}

static void
default_memfree(void *ptr) {
	free(ptr);
}

static void
mem_initialize(void) {
	isc_mutex_init(&contextslock);
	ISC_LIST_INIT(contexts);
	totallost = 0;
}

void
isc__mem_initialize(void) {
	RUNTIME_CHECK(isc_once_do(&init_once, mem_initialize) == ISC_R_SUCCESS);
}

static void
mem_shutdown(void) {
	isc__mem_checkdestroyed();

	isc_mutex_destroy(&contextslock);
}

void
isc__mem_shutdown(void) {
	RUNTIME_CHECK(isc_once_do(&shut_once, mem_shutdown) == ISC_R_SUCCESS);
}

static void
mem_create(isc_mem_t **ctxp, unsigned int flags) {
	REQUIRE(ctxp != NULL && *ctxp == NULL);
#if __SANITIZE_ADDRESS__
	REQUIRE((flags & ISC_MEMFLAG_INTERNAL) == 0);
#endif

	isc__mem_t *ctx;

	isc_enable_constructors();

	STATIC_ASSERT((ALIGNMENT_SIZE & (ALIGNMENT_SIZE - 1)) == 0,
		      "wrong alignment size");

	ctx = (default_memalloc)(sizeof(*ctx));

	isc_mutex_init(&ctx->lock);

	ctx->max_size = DEF_MAX_SIZE;
	ctx->flags = flags;
	isc_refcount_init(&ctx->references, 1);
	memset(ctx->name, 0, sizeof(ctx->name));
	ctx->tag = NULL;
	ctx->total = 0;
	ctx->inuse = 0;
	ctx->maxinuse = 0;
	ctx->malloced = sizeof(*ctx);
	ctx->maxmalloced = sizeof(*ctx);
	ctx->hi_water = 0;
	ctx->lo_water = 0;
	ctx->hi_called = false;
	ctx->is_overmem = false;
	ctx->water = NULL;
	ctx->water_arg = NULL;
	ctx->common.impmagic = MEM_MAGIC;
	ctx->common.magic = ISCAPI_MCTX_MAGIC;
	ctx->common.methods = (isc_memmethods_t *)&memmethods;
	ctx->memalloc = default_memalloc;
	ctx->memfree = default_memfree;
	ctx->stats = NULL;
	ctx->checkfree = true;
#if ISC_MEM_TRACKLINES
	ctx->debuglist = NULL;
	ctx->debuglistcnt = 0;
#endif /* if ISC_MEM_TRACKLINES */
	ISC_LIST_INIT(ctx->pools);
	ctx->poolcnt = 0;
	ctx->freelists = NULL;
	ctx->basic_blocks = NULL;
	ctx->basic_table = NULL;
	ctx->basic_table_count = 0;
	ctx->basic_table_size = 0;
	ctx->lowest = NULL;
	ctx->highest = NULL;

	ctx->stats =
		(ctx->memalloc)((ctx->max_size + 1) * sizeof(struct stats));

	memset(ctx->stats, 0, (ctx->max_size + 1) * sizeof(struct stats));
	ctx->malloced += (ctx->max_size + 1) * sizeof(struct stats);
	ctx->maxmalloced += (ctx->max_size + 1) * sizeof(struct stats);

	if ((flags & ISC_MEMFLAG_INTERNAL) != 0) {
		ctx->mem_target = DEF_MEM_TARGET;
		ctx->freelists =
			(ctx->memalloc)(ctx->max_size * sizeof(element *));
		memset(ctx->freelists, 0, ctx->max_size * sizeof(element *));
		ctx->malloced += ctx->max_size * sizeof(element *);
		ctx->maxmalloced += ctx->max_size * sizeof(element *);
	}

#if ISC_MEM_TRACKLINES
	if (ISC_UNLIKELY((isc_mem_debugging & ISC_MEM_DEBUGRECORD) != 0)) {
		unsigned int i;

		ctx->debuglist = (ctx->memalloc)(
			(DEBUG_TABLE_COUNT * sizeof(debuglist_t)));
		for (i = 0; i < DEBUG_TABLE_COUNT; i++) {
			ISC_LIST_INIT(ctx->debuglist[i]);
		}
		ctx->malloced += DEBUG_TABLE_COUNT * sizeof(debuglist_t);
		ctx->maxmalloced += DEBUG_TABLE_COUNT * sizeof(debuglist_t);
	}
#endif /* if ISC_MEM_TRACKLINES */

	LOCK(&contextslock);
	ISC_LIST_INITANDAPPEND(contexts, ctx, link);
	UNLOCK(&contextslock);

	*ctxp = (isc_mem_t *)ctx;
}

/*
 * Public.
 */

static void
destroy(isc__mem_t *ctx) {
	unsigned int i;

	LOCK(&contextslock);
	ISC_LIST_UNLINK(contexts, ctx, link);
	totallost += ctx->inuse;
	UNLOCK(&contextslock);

	ctx->common.impmagic = 0;
	ctx->common.magic = 0;

	INSIST(ISC_LIST_EMPTY(ctx->pools));

#if ISC_MEM_TRACKLINES
	if (ISC_UNLIKELY(ctx->debuglist != NULL)) {
		debuglink_t *dl;
		for (i = 0; i < DEBUG_TABLE_COUNT; i++) {
			for (dl = ISC_LIST_HEAD(ctx->debuglist[i]); dl != NULL;
			     dl = ISC_LIST_HEAD(ctx->debuglist[i]))
			{
				if (ctx->checkfree && dl->ptr != NULL) {
					print_active(ctx, stderr);
				}
				INSIST(!ctx->checkfree || dl->ptr == NULL);

				ISC_LIST_UNLINK(ctx->debuglist[i], dl, link);
				free(dl);
				ctx->malloced -= sizeof(*dl);
			}
		}

		(ctx->memfree)(ctx->debuglist);
		ctx->malloced -= DEBUG_TABLE_COUNT * sizeof(debuglist_t);
	}
#endif /* if ISC_MEM_TRACKLINES */

	if (ctx->checkfree) {
		for (i = 0; i <= ctx->max_size; i++) {
			if (ctx->stats[i].gets != 0U) {
				fprintf(stderr,
					"Failing assertion due to probable "
					"leaked memory in context %p (\"%s\") "
					"(stats[%u].gets == %lu).\n",
					ctx, ctx->name, i, ctx->stats[i].gets);
#if ISC_MEM_TRACKLINES
				print_active(ctx, stderr);
#endif /* if ISC_MEM_TRACKLINES */
				INSIST(ctx->stats[i].gets == 0U);
			}
		}
	}

	(ctx->memfree)(ctx->stats);
	ctx->malloced -= (ctx->max_size + 1) * sizeof(struct stats);

	if ((ctx->flags & ISC_MEMFLAG_INTERNAL) != 0) {
		for (i = 0; i < ctx->basic_table_count; i++) {
			(ctx->memfree)(ctx->basic_table[i]);
			ctx->malloced -= NUM_BASIC_BLOCKS * ctx->mem_target;
		}
		(ctx->memfree)(ctx->freelists);
		ctx->malloced -= ctx->max_size * sizeof(element *);
		if (ctx->basic_table != NULL) {
			(ctx->memfree)(ctx->basic_table);
			ctx->malloced -= ctx->basic_table_size *
					 sizeof(unsigned char *);
		}
	}

	isc_mutex_destroy(&ctx->lock);

	ctx->malloced -= sizeof(*ctx);
	if (ctx->checkfree) {
		INSIST(ctx->malloced == 0);
	}
	(ctx->memfree)(ctx);
}

void
isc_mem_attach(isc_mem_t *source0, isc_mem_t **targetp) {
	REQUIRE(VALID_CONTEXT(source0));
	REQUIRE(targetp != NULL && *targetp == NULL);

	isc__mem_t *source = (isc__mem_t *)source0;

	isc_refcount_increment(&source->references);

	*targetp = (isc_mem_t *)source;
}

void
isc_mem_detach(isc_mem_t **ctxp) {
	REQUIRE(ctxp != NULL && VALID_CONTEXT(*ctxp));

	isc__mem_t *ctx = (isc__mem_t *)*ctxp;
	*ctxp = NULL;

	if (isc_refcount_decrement(&ctx->references) == 1) {
		isc_refcount_destroy(&ctx->references);
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
isc___mem_putanddetach(isc_mem_t **ctxp, void *ptr, size_t size FLARG) {
	REQUIRE(ctxp != NULL && VALID_CONTEXT(*ctxp));
	REQUIRE(ptr != NULL);

	isc__mem_t *ctx = (isc__mem_t *)*ctxp;
	*ctxp = NULL;

	if (ISC_UNLIKELY((isc_mem_debugging &
			  (ISC_MEM_DEBUGSIZE | ISC_MEM_DEBUGCTX)) != 0))
	{
		if ((isc_mem_debugging & ISC_MEM_DEBUGSIZE) != 0) {
			size_info *si = &(((size_info *)ptr)[-1]);
			size_t oldsize = si->u.size - ALIGNMENT_SIZE;
			if ((isc_mem_debugging & ISC_MEM_DEBUGCTX) != 0) {
				oldsize -= ALIGNMENT_SIZE;
			}
			INSIST(oldsize == size);
		}
		isc__mem_free((isc_mem_t *)ctx, ptr FLARG_PASS);

		goto destroy;
	}

	MCTXLOCK(ctx);

	DELETE_TRACE(ctx, ptr, size, file, line);

	if ((ctx->flags & ISC_MEMFLAG_INTERNAL) != 0) {
		mem_putunlocked(ctx, ptr, size);
	} else {
		mem_putstats(ctx, ptr, size);
		mem_put(ctx, ptr, size);
	}
	MCTXUNLOCK(ctx);

destroy:
	if (isc_refcount_decrement(&ctx->references) == 1) {
		isc_refcount_destroy(&ctx->references);
		destroy(ctx);
	}
}

void
isc_mem_destroy(isc_mem_t **ctxp) {
	/*
	 * This routine provides legacy support for callers who use mctxs
	 * without attaching/detaching.
	 */

	REQUIRE(ctxp != NULL && VALID_CONTEXT(*ctxp));

	isc__mem_t *ctx = (isc__mem_t *)*ctxp;

#if ISC_MEM_TRACKLINES
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
isc___mem_get(isc_mem_t *ctx0, size_t size FLARG) {
	REQUIRE(VALID_CONTEXT(ctx0));

	isc__mem_t *ctx = (isc__mem_t *)ctx0;
	void *ptr;
	bool call_water = false;

	if (ISC_UNLIKELY((isc_mem_debugging &
			  (ISC_MEM_DEBUGSIZE | ISC_MEM_DEBUGCTX)) != 0))
	{
		return (isc__mem_allocate(ctx0, size FLARG_PASS));
	}

	if ((ctx->flags & ISC_MEMFLAG_INTERNAL) != 0) {
		MCTXLOCK(ctx);
		ptr = mem_getunlocked(ctx, size);
	} else {
		ptr = mem_get(ctx, size);
		MCTXLOCK(ctx);
		if (ptr != NULL) {
			mem_getstats(ctx, size);
		}
	}

	ADD_TRACE(ctx, ptr, size, file, line);

	if (ctx->hi_water != 0U && ctx->inuse > ctx->hi_water) {
		ctx->is_overmem = true;
		if (!ctx->hi_called) {
			call_water = true;
		}
	}
	if (ctx->inuse > ctx->maxinuse) {
		ctx->maxinuse = ctx->inuse;
		if (ctx->hi_water != 0U && ctx->inuse > ctx->hi_water &&
		    (isc_mem_debugging & ISC_MEM_DEBUGUSAGE) != 0)
		{
			fprintf(stderr, "maxinuse = %lu\n",
				(unsigned long)ctx->inuse);
		}
	}
	MCTXUNLOCK(ctx);

	if (call_water && (ctx->water != NULL)) {
		(ctx->water)(ctx->water_arg, ISC_MEM_HIWATER);
	}

	return (ptr);
}

void
isc___mem_put(isc_mem_t *ctx0, void *ptr, size_t size FLARG) {
	REQUIRE(VALID_CONTEXT(ctx0));
	REQUIRE(ptr != NULL);

	isc__mem_t *ctx = (isc__mem_t *)ctx0;
	bool call_water = false;
	size_info *si;
	size_t oldsize;

	if (ISC_UNLIKELY((isc_mem_debugging &
			  (ISC_MEM_DEBUGSIZE | ISC_MEM_DEBUGCTX)) != 0))
	{
		if ((isc_mem_debugging & ISC_MEM_DEBUGSIZE) != 0) {
			si = &(((size_info *)ptr)[-1]);
			oldsize = si->u.size - ALIGNMENT_SIZE;
			if ((isc_mem_debugging & ISC_MEM_DEBUGCTX) != 0) {
				oldsize -= ALIGNMENT_SIZE;
			}
			INSIST(oldsize == size);
		}
		isc__mem_free((isc_mem_t *)ctx, ptr FLARG_PASS);
		return;
	}

	MCTXLOCK(ctx);

	DELETE_TRACE(ctx, ptr, size, file, line);

	if ((ctx->flags & ISC_MEMFLAG_INTERNAL) != 0) {
		mem_putunlocked(ctx, ptr, size);
	} else {
		mem_putstats(ctx, ptr, size);
		mem_put(ctx, ptr, size);
	}

	/*
	 * The check against ctx->lo_water == 0 is for the condition
	 * when the context was pushed over hi_water but then had
	 * isc_mem_setwater() called with 0 for hi_water and lo_water.
	 */
	if ((ctx->inuse < ctx->lo_water) || (ctx->lo_water == 0U)) {
		ctx->is_overmem = false;
		if (ctx->hi_called) {
			call_water = true;
		}
	}

	MCTXUNLOCK(ctx);

	if (call_water && (ctx->water != NULL)) {
		(ctx->water)(ctx->water_arg, ISC_MEM_LOWATER);
	}
}

void
isc_mem_waterack(isc_mem_t *ctx0, int flag) {
	REQUIRE(VALID_CONTEXT(ctx0));

	isc__mem_t *ctx = (isc__mem_t *)ctx0;

	MCTXLOCK(ctx);
	if (flag == ISC_MEM_LOWATER) {
		ctx->hi_called = false;
	} else if (flag == ISC_MEM_HIWATER) {
		ctx->hi_called = true;
	}
	MCTXUNLOCK(ctx);
}

#if ISC_MEM_TRACKLINES
static void
print_active(isc__mem_t *mctx, FILE *out) {
	if (mctx->debuglist != NULL) {
		debuglink_t *dl;
		unsigned int i;
		bool found;

		fputs("Dump of all outstanding memory allocations:\n", out);
		found = false;
		for (i = 0; i < DEBUG_TABLE_COUNT; i++) {
			dl = ISC_LIST_HEAD(mctx->debuglist[i]);

			if (dl != NULL) {
				found = true;
			}

			while (dl != NULL) {
				if (dl->ptr != NULL) {
					fprintf(out,
						"\tptr %p size %zu file %s "
						"line %u\n",
						dl->ptr, dl->size, dl->file,
						dl->line);
				}
				dl = ISC_LIST_NEXT(dl, link);
			}
		}

		if (!found) {
			fputs("\tNone.\n", out);
		}
	}
}
#endif /* if ISC_MEM_TRACKLINES */

/*
 * Print the stats[] on the stream "out" with suitable formatting.
 */
void
isc_mem_stats(isc_mem_t *ctx0, FILE *out) {
	REQUIRE(VALID_CONTEXT(ctx0));

	isc__mem_t *ctx = (isc__mem_t *)ctx0;
	size_t i;
	const struct stats *s;
	const isc__mempool_t *pool;

	MCTXLOCK(ctx);

	for (i = 0; i <= ctx->max_size; i++) {
		s = &ctx->stats[i];

		if (s->totalgets == 0U && s->gets == 0U) {
			continue;
		}
		fprintf(out, "%s%5lu: %11lu gets, %11lu rem",
			(i == ctx->max_size) ? ">=" : "  ", (unsigned long)i,
			s->totalgets, s->gets);
		if ((ctx->flags & ISC_MEMFLAG_INTERNAL) != 0 &&
		    (s->blocks != 0U || s->freefrags != 0U))
		{
			fprintf(out, " (%lu bl, %lu ff)", s->blocks,
				s->freefrags);
		}
		fputc('\n', out);
	}

	/*
	 * Note that since a pool can be locked now, these stats might be
	 * somewhat off if the pool is in active use at the time the stats
	 * are dumped.  The link fields are protected by the isc_mem_t's
	 * lock, however, so walking this list and extracting integers from
	 * stats fields is always safe.
	 */
	pool = ISC_LIST_HEAD(ctx->pools);
	if (pool != NULL) {
		fputs("[Pool statistics]\n", out);
		fprintf(out, "%15s %10s %10s %10s %10s %10s %10s %10s %1s\n",
			"name", "size", "maxalloc", "allocated", "freecount",
			"freemax", "fillcount", "gets", "L");
	}
	while (pool != NULL) {
		fprintf(out, "%15s %10lu %10u %10u %10u %10u %10u %10u %s\n",
#if ISC_MEMPOOL_NAMES
			pool->name,
#else  /* if ISC_MEMPOOL_NAMES */
			"(not tracked)",
#endif /* if ISC_MEMPOOL_NAMES */
			(unsigned long)pool->size, pool->maxalloc,
			pool->allocated, pool->freecount, pool->freemax,
			pool->fillcount, pool->gets, "N");
		pool = ISC_LIST_NEXT(pool, link);
	}

#if ISC_MEM_TRACKLINES
	print_active(ctx, out);
#endif /* if ISC_MEM_TRACKLINES */

	MCTXUNLOCK(ctx);
}

/*
 * Replacements for malloc() and free() -- they implicitly remember the
 * size of the object allocated (with some additional overhead).
 */

static void *
mem_allocateunlocked(isc_mem_t *ctx0, size_t size) {
	isc__mem_t *ctx = (isc__mem_t *)ctx0;
	size_info *si;

	size += ALIGNMENT_SIZE;
	if (ISC_UNLIKELY((isc_mem_debugging & ISC_MEM_DEBUGCTX) != 0)) {
		size += ALIGNMENT_SIZE;
	}

	if ((ctx->flags & ISC_MEMFLAG_INTERNAL) != 0) {
		si = mem_getunlocked(ctx, size);
	} else {
		si = mem_get(ctx, size);
	}

	if (ISC_UNLIKELY((isc_mem_debugging & ISC_MEM_DEBUGCTX) != 0)) {
		si->u.ctx = ctx;
		si++;
	}
	si->u.size = size;
	return (&si[1]);
}

void *
isc___mem_allocate(isc_mem_t *ctx0, size_t size FLARG) {
	REQUIRE(VALID_CONTEXT(ctx0));

	isc__mem_t *ctx = (isc__mem_t *)ctx0;
	size_info *si;
	bool call_water = false;

	MCTXLOCK(ctx);
	si = mem_allocateunlocked((isc_mem_t *)ctx, size);
	if (((ctx->flags & ISC_MEMFLAG_INTERNAL) == 0)) {
		mem_getstats(ctx, si[-1].u.size);
	}

	ADD_TRACE(ctx, si, si[-1].u.size, file, line);
	if (ctx->hi_water != 0U && ctx->inuse > ctx->hi_water &&
	    !ctx->is_overmem)
	{
		ctx->is_overmem = true;
	}

	if (ctx->hi_water != 0U && !ctx->hi_called &&
	    ctx->inuse > ctx->hi_water)
	{
		ctx->hi_called = true;
		call_water = true;
	}
	if (ctx->inuse > ctx->maxinuse) {
		ctx->maxinuse = ctx->inuse;
		if (ISC_UNLIKELY(ctx->hi_water != 0U &&
				 ctx->inuse > ctx->hi_water &&
				 (isc_mem_debugging & ISC_MEM_DEBUGUSAGE) != 0))
		{
			fprintf(stderr, "maxinuse = %lu\n",
				(unsigned long)ctx->inuse);
		}
	}
	MCTXUNLOCK(ctx);

	if (call_water) {
		(ctx->water)(ctx->water_arg, ISC_MEM_HIWATER);
	}

	return (si);
}

void *
isc___mem_reallocate(isc_mem_t *ctx0, void *ptr, size_t size FLARG) {
	REQUIRE(VALID_CONTEXT(ctx0));

	void *new_ptr = NULL;
	size_t oldsize, copysize;

	/*
	 * This function emulates the realloc(3) standard library function:
	 * - if size > 0, allocate new memory; and if ptr is non NULL, copy
	 *   as much of the old contents to the new buffer and free the old one.
	 *   Note that when allocation fails the original pointer is intact;
	 *   the caller must free it.
	 * - if size is 0 and ptr is non NULL, simply free the given ptr.
	 * - this function returns:
	 *     pointer to the newly allocated memory, or
	 *     NULL if allocation fails or doesn't happen.
	 */
	if (size > 0U) {
		new_ptr = isc__mem_allocate(ctx0, size FLARG_PASS);
		if (new_ptr != NULL && ptr != NULL) {
			oldsize = (((size_info *)ptr)[-1]).u.size;
			INSIST(oldsize >= ALIGNMENT_SIZE);
			oldsize -= ALIGNMENT_SIZE;
			if (ISC_UNLIKELY((isc_mem_debugging &
					  ISC_MEM_DEBUGCTX) != 0))
			{
				INSIST(oldsize >= ALIGNMENT_SIZE);
				oldsize -= ALIGNMENT_SIZE;
			}
			copysize = (oldsize > size) ? size : oldsize;
			memmove(new_ptr, ptr, copysize);
			isc__mem_free(ctx0, ptr FLARG_PASS);
		}
	} else if (ptr != NULL) {
		isc__mem_free(ctx0, ptr FLARG_PASS);
	}

	return (new_ptr);
}

void
isc___mem_free(isc_mem_t *ctx0, void *ptr FLARG) {
	REQUIRE(VALID_CONTEXT(ctx0));
	REQUIRE(ptr != NULL);

	isc__mem_t *ctx = (isc__mem_t *)ctx0;
	size_info *si;
	size_t size;
	bool call_water = false;

	if (ISC_UNLIKELY((isc_mem_debugging & ISC_MEM_DEBUGCTX) != 0)) {
		si = &(((size_info *)ptr)[-2]);
		REQUIRE(si->u.ctx == ctx);
		size = si[1].u.size;
	} else {
		si = &(((size_info *)ptr)[-1]);
		size = si->u.size;
	}

	MCTXLOCK(ctx);

	DELETE_TRACE(ctx, ptr, size, file, line);

	if ((ctx->flags & ISC_MEMFLAG_INTERNAL) != 0) {
		mem_putunlocked(ctx, si, size);
	} else {
		mem_putstats(ctx, si, size);
		mem_put(ctx, si, size);
	}

	/*
	 * The check against ctx->lo_water == 0 is for the condition
	 * when the context was pushed over hi_water but then had
	 * isc_mem_setwater() called with 0 for hi_water and lo_water.
	 */
	if (ctx->is_overmem &&
	    (ctx->inuse < ctx->lo_water || ctx->lo_water == 0U))
	{
		ctx->is_overmem = false;
	}

	if (ctx->hi_called &&
	    (ctx->inuse < ctx->lo_water || ctx->lo_water == 0U))
	{
		ctx->hi_called = false;

		if (ctx->water != NULL) {
			call_water = true;
		}
	}
	MCTXUNLOCK(ctx);

	if (call_water) {
		(ctx->water)(ctx->water_arg, ISC_MEM_LOWATER);
	}
}

/*
 * Other useful things.
 */

char *
isc___mem_strdup(isc_mem_t *mctx0, const char *s FLARG) {
	REQUIRE(VALID_CONTEXT(mctx0));
	REQUIRE(s != NULL);

	isc__mem_t *mctx = (isc__mem_t *)mctx0;
	size_t len;
	char *ns;

	len = strlen(s) + 1;

	ns = isc__mem_allocate((isc_mem_t *)mctx, len FLARG_PASS);

	if (ns != NULL) {
		strlcpy(ns, s, len);
	}

	return (ns);
}

char *
isc___mem_strndup(isc_mem_t *mctx0, const char *s, size_t size FLARG) {
	REQUIRE(VALID_CONTEXT(mctx0));
	REQUIRE(s != NULL);

	isc__mem_t *mctx = (isc__mem_t *)mctx0;
	size_t len;
	char *ns;

	len = strlen(s) + 1;
	if (len > size) {
		len = size;
	}

	ns = isc__mem_allocate((isc_mem_t *)mctx, len FLARG_PASS);

	if (ns != NULL) {
		strlcpy(ns, s, len);
	}

	return (ns);
}

void
isc_mem_setdestroycheck(isc_mem_t *ctx0, bool flag) {
	REQUIRE(VALID_CONTEXT(ctx0));

	isc__mem_t *ctx = (isc__mem_t *)ctx0;

	MCTXLOCK(ctx);

	ctx->checkfree = flag;

	MCTXUNLOCK(ctx);
}

size_t
isc_mem_inuse(isc_mem_t *ctx0) {
	REQUIRE(VALID_CONTEXT(ctx0));

	isc__mem_t *ctx = (isc__mem_t *)ctx0;
	size_t inuse;

	MCTXLOCK(ctx);

	inuse = ctx->inuse;

	MCTXUNLOCK(ctx);

	return (inuse);
}

size_t
isc_mem_maxinuse(isc_mem_t *ctx0) {
	REQUIRE(VALID_CONTEXT(ctx0));

	isc__mem_t *ctx = (isc__mem_t *)ctx0;
	size_t maxinuse;

	MCTXLOCK(ctx);

	maxinuse = ctx->maxinuse;

	MCTXUNLOCK(ctx);

	return (maxinuse);
}

size_t
isc_mem_total(isc_mem_t *ctx0) {
	REQUIRE(VALID_CONTEXT(ctx0));

	isc__mem_t *ctx = (isc__mem_t *)ctx0;
	size_t total;

	MCTXLOCK(ctx);

	total = ctx->total;

	MCTXUNLOCK(ctx);

	return (total);
}

void
isc_mem_setwater(isc_mem_t *ctx0, isc_mem_water_t water, void *water_arg,
		 size_t hiwater, size_t lowater) {
	REQUIRE(VALID_CONTEXT(ctx0));
	REQUIRE(hiwater >= lowater);

	isc__mem_t *ctx = (isc__mem_t *)ctx0;
	bool callwater = false;
	isc_mem_water_t oldwater;
	void *oldwater_arg;

	MCTXLOCK(ctx);
	oldwater = ctx->water;
	oldwater_arg = ctx->water_arg;
	if (water == NULL) {
		callwater = ctx->hi_called;
		ctx->water = NULL;
		ctx->water_arg = NULL;
		ctx->hi_water = 0;
		ctx->lo_water = 0;
	} else {
		if (ctx->hi_called &&
		    (ctx->water != water || ctx->water_arg != water_arg ||
		     ctx->inuse < lowater || lowater == 0U))
		{
			callwater = true;
		}
		ctx->water = water;
		ctx->water_arg = water_arg;
		ctx->hi_water = hiwater;
		ctx->lo_water = lowater;
	}
	MCTXUNLOCK(ctx);

	if (callwater && oldwater != NULL) {
		(oldwater)(oldwater_arg, ISC_MEM_LOWATER);
	}
}

ISC_NO_SANITIZE_THREAD bool
isc_mem_isovermem(isc_mem_t *ctx0) {
	REQUIRE(VALID_CONTEXT(ctx0));

	isc__mem_t *ctx = (isc__mem_t *)ctx0;

	/*
	 * We don't bother to lock the context because 100% accuracy isn't
	 * necessary (and even if we locked the context the returned value
	 * could be different from the actual state when it's used anyway)
	 */
	return (ctx->is_overmem);
}

void
isc_mem_setname(isc_mem_t *ctx0, const char *name, void *tag) {
	REQUIRE(VALID_CONTEXT(ctx0));

	isc__mem_t *ctx = (isc__mem_t *)ctx0;

	LOCK(&ctx->lock);
	strlcpy(ctx->name, name, sizeof(ctx->name));
	ctx->tag = tag;
	UNLOCK(&ctx->lock);
}

const char *
isc_mem_getname(isc_mem_t *ctx0) {
	REQUIRE(VALID_CONTEXT(ctx0));

	isc__mem_t *ctx = (isc__mem_t *)ctx0;

	if (ctx->name[0] == 0) {
		return ("");
	}

	return (ctx->name);
}

void *
isc_mem_gettag(isc_mem_t *ctx0) {
	REQUIRE(VALID_CONTEXT(ctx0));

	isc__mem_t *ctx = (isc__mem_t *)ctx0;

	return (ctx->tag);
}

/*
 * Memory pool stuff
 */

void
isc_mempool_create(isc_mem_t *mctx0, size_t size, isc_mempool_t **mpctxp) {
	REQUIRE(VALID_CONTEXT(mctx0));
	REQUIRE(size > 0U);
	REQUIRE(mpctxp != NULL && *mpctxp == NULL);

	isc__mem_t *mctx = (isc__mem_t *)mctx0;
	isc__mempool_t *mpctx;

	/*
	 * Allocate space for this pool, initialize values, and if all works
	 * well, attach to the memory context.
	 */
	mpctx = isc_mem_get((isc_mem_t *)mctx, sizeof(isc__mempool_t));

	mpctx->common.impmagic = MEMPOOL_MAGIC;
	mpctx->common.magic = ISCAPI_MPOOL_MAGIC;
	mpctx->mctx = NULL;
	isc_mem_attach((isc_mem_t *)mctx, (isc_mem_t **)&mpctx->mctx);
	/*
	 * Mempools are stored as a linked list of element.
	 */
	if (size < sizeof(element)) {
		size = sizeof(element);
	}
	mpctx->size = size;
	mpctx->maxalloc = UINT_MAX;
	mpctx->allocated = 0;
	mpctx->freecount = 0;
	mpctx->freemax = 1;
	mpctx->fillcount = 1;
	mpctx->gets = 0;
#if ISC_MEMPOOL_NAMES
	mpctx->name[0] = 0;
#endif /* if ISC_MEMPOOL_NAMES */
	mpctx->items = NULL;

	*mpctxp = (isc_mempool_t *)mpctx;

	MCTXLOCK(mctx);
	ISC_LIST_INITANDAPPEND(mctx->pools, mpctx, link);
	mctx->poolcnt++;
	MCTXUNLOCK(mctx);
}

void
isc_mempool_setname(isc_mempool_t *mpctx0, const char *name) {
	REQUIRE(VALID_MEMPOOL(mpctx0));
	REQUIRE(name != NULL);

	isc__mempool_t *mpctx = (isc__mempool_t *)mpctx0;

#if ISC_MEMPOOL_NAMES
	strlcpy(mpctx->name, name, sizeof(mpctx->name));
#else  /* if ISC_MEMPOOL_NAMES */
	UNUSED(mpctx);
	UNUSED(name);
#endif /* if ISC_MEMPOOL_NAMES */
}

void
isc_mempool_destroy(isc_mempool_t **mpctxp) {
	REQUIRE(mpctxp != NULL);
	REQUIRE(VALID_MEMPOOL(*mpctxp));

	isc__mempool_t *mpctx;
	isc__mem_t *mctx;
	element *item;

	mpctx = (isc__mempool_t *)*mpctxp;
#if ISC_MEMPOOL_NAMES
	if (mpctx->allocated > 0) {
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "isc_mempool_destroy(): mempool %s "
				 "leaked memory",
				 mpctx->name);
	}
#endif /* if ISC_MEMPOOL_NAMES */
	REQUIRE(mpctx->allocated == 0);

	mctx = mpctx->mctx;

	/*
	 * Return any items on the free list
	 */
	MCTXLOCK(mctx);
	while (mpctx->items != NULL) {
		INSIST(mpctx->freecount > 0);
		mpctx->freecount--;
		item = mpctx->items;
		mpctx->items = item->next;
		mem_putstats(mctx, item, mpctx->size);
		mem_put(mctx, item, mpctx->size);
	}
	MCTXUNLOCK(mctx);

	/*
	 * Remove our linked list entry from the memory context.
	 */
	MCTXLOCK(mctx);
	ISC_LIST_UNLINK(mctx->pools, mpctx, link);
	mctx->poolcnt--;
	MCTXUNLOCK(mctx);

	mpctx->common.impmagic = 0;
	mpctx->common.magic = 0;

	isc_mem_putanddetach((isc_mem_t **)&mpctx->mctx, mpctx,
			     sizeof(isc__mempool_t));

	*mpctxp = NULL;
}

#if __SANITIZE_ADDRESS__
void *
isc__mempool_get(isc_mempool_t *mpctx0 FLARG) {
	void *item = NULL;

	REQUIRE(VALID_MEMPOOL(mpctx0));

	isc__mempool_t *mpctx = (isc__mempool_t *)mpctx0;
	isc_mem_t *mctx = (isc_mem_t *)mpctx->mctx;

	/*
	 * Don't let the caller go over quota
	 */
	if (ISC_UNLIKELY(mpctx->allocated >= mpctx->maxalloc)) {
		goto out;
	}

	item = isc__mem_get(mctx, mpctx->size FLARG_PASS);
	mpctx->gets++;
	mpctx->allocated++;

out:
	return (item);
}

void
isc__mempool_put(isc_mempool_t *mpctx0, void *mem FLARG) {
	REQUIRE(VALID_MEMPOOL(mpctx0));

	isc__mempool_t *mpctx = (isc__mempool_t *)mpctx0;
	isc_mem_t *mctx = (isc_mem_t *)mpctx->mctx;

	REQUIRE(mem != NULL);

	INSIST(mpctx->allocated > 0);
	mpctx->allocated--;

	isc__mem_put(mctx, mem, mpctx->size FLARG_PASS);
}

#else /* __SANITIZE_ADDRESS__ */
void *
isc__mempool_get(isc_mempool_t *mpctx0 FLARG) {
	REQUIRE(VALID_MEMPOOL(mpctx0));

	isc__mempool_t *mpctx = (isc__mempool_t *)mpctx0;
	element *item;
	isc__mem_t *mctx;
	unsigned int i;

	mctx = mpctx->mctx;

	/*
	 * Don't let the caller go over quota
	 */
	if (ISC_UNLIKELY(mpctx->allocated >= mpctx->maxalloc)) {
		item = NULL;
		goto out;
	}

	if (ISC_UNLIKELY(mpctx->items == NULL)) {
		/*
		 * We need to dip into the well.  Lock the memory context
		 * here and fill up our free list.
		 */
		MCTXLOCK(mctx);
		for (i = 0; i < mpctx->fillcount; i++) {
			item = mem_get(mctx, mpctx->size);
			mem_getstats(mctx, mpctx->size);
			item->next = mpctx->items;
			mpctx->items = item;
			mpctx->freecount++;
		}
		MCTXUNLOCK(mctx);
	}

	/*
	 * If we didn't get any items, return NULL.
	 */
	item = mpctx->items;
	if (ISC_UNLIKELY(item == NULL)) {
		goto out;
	}

	mpctx->items = item->next;
	INSIST(mpctx->freecount > 0);
	mpctx->freecount--;
	mpctx->gets++;
	mpctx->allocated++;

out:
#if ISC_MEM_TRACKLINES
	if (ISC_UNLIKELY(((isc_mem_debugging & TRACE_OR_RECORD) != 0) &&
			 item != NULL))
	{
		MCTXLOCK(mctx);
		ADD_TRACE(mctx, item, mpctx->size, file, line);
		MCTXUNLOCK(mctx);
	}
#endif /* ISC_MEM_TRACKLINES */

	return (item);
}

/* coverity[+free : arg-1] */
void
isc__mempool_put(isc_mempool_t *mpctx0, void *mem FLARG) {
	REQUIRE(VALID_MEMPOOL(mpctx0));
	REQUIRE(mem != NULL);

	isc__mempool_t *mpctx = (isc__mempool_t *)mpctx0;
	isc__mem_t *mctx = mpctx->mctx;
	element *item;

	INSIST(mpctx->allocated > 0);
	mpctx->allocated--;

#if ISC_MEM_TRACKLINES
	if (ISC_UNLIKELY((isc_mem_debugging & TRACE_OR_RECORD) != 0)) {
		MCTXLOCK(mctx);
		DELETE_TRACE(mctx, mem, mpctx->size, file, line);
		MCTXUNLOCK(mctx);
	}
#endif /* ISC_MEM_TRACKLINES */

	/*
	 * If our free list is full, return this to the mctx directly.
	 */
	if (mpctx->freecount >= mpctx->freemax) {
		MCTXLOCK(mctx);
		mem_putstats(mctx, mem, mpctx->size);
		mem_put(mctx, mem, mpctx->size);
		MCTXUNLOCK(mctx);
		return;
	}

	/*
	 * Otherwise, attach it to our free list and bump the counter.
	 */
	mpctx->freecount++;
	item = (element *)mem;
	item->next = mpctx->items;
	mpctx->items = item;
}

#endif /* __SANITIZE_ADDRESS__ */

/*
 * Quotas
 */

void
isc_mempool_setfreemax(isc_mempool_t *mpctx0, unsigned int limit) {
	REQUIRE(VALID_MEMPOOL(mpctx0));

	isc__mempool_t *mpctx = (isc__mempool_t *)mpctx0;

	mpctx->freemax = limit;
}

unsigned int
isc_mempool_getfreemax(isc_mempool_t *mpctx0) {
	REQUIRE(VALID_MEMPOOL(mpctx0));

	isc__mempool_t *mpctx = (isc__mempool_t *)mpctx0;

	return (mpctx->freemax);
}

unsigned int
isc_mempool_getfreecount(isc_mempool_t *mpctx0) {
	REQUIRE(VALID_MEMPOOL(mpctx0));

	isc__mempool_t *mpctx = (isc__mempool_t *)mpctx0;

	return (mpctx->freecount);
}

void
isc_mempool_setmaxalloc(isc_mempool_t *mpctx0, unsigned int limit) {
	REQUIRE(VALID_MEMPOOL(mpctx0));
	REQUIRE(limit > 0);

	isc__mempool_t *mpctx = (isc__mempool_t *)mpctx0;

	mpctx->maxalloc = limit;
}

unsigned int
isc_mempool_getmaxalloc(isc_mempool_t *mpctx0) {
	REQUIRE(VALID_MEMPOOL(mpctx0));

	isc__mempool_t *mpctx = (isc__mempool_t *)mpctx0;

	return (mpctx->maxalloc);
}

unsigned int
isc_mempool_getallocated(isc_mempool_t *mpctx0) {
	REQUIRE(VALID_MEMPOOL(mpctx0));

	isc__mempool_t *mpctx = (isc__mempool_t *)mpctx0;

	return (mpctx->allocated);
}

void
isc_mempool_setfillcount(isc_mempool_t *mpctx0, unsigned int limit) {
	REQUIRE(VALID_MEMPOOL(mpctx0));
	REQUIRE(limit > 0);

	isc__mempool_t *mpctx = (isc__mempool_t *)mpctx0;

	mpctx->fillcount = limit;
}

unsigned int
isc_mempool_getfillcount(isc_mempool_t *mpctx0) {
	REQUIRE(VALID_MEMPOOL(mpctx0));

	isc__mempool_t *mpctx = (isc__mempool_t *)mpctx0;

	return (mpctx->fillcount);
}

/*
 * Requires contextslock to be held by caller.
 */
static void
print_contexts(FILE *file) {
	isc__mem_t *ctx;

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
		if (ISC_UNLIKELY((isc_mem_debugging & TRACE_OR_RECORD) != 0)) {
			print_contexts(file);
		}
#endif /* if ISC_MEM_TRACKLINES */
		UNREACHABLE();
	}
	UNLOCK(&contextslock);
}

unsigned int
isc_mem_references(isc_mem_t *ctx0) {
	isc__mem_t *ctx = (isc__mem_t *)ctx0;
	return (isc_refcount_current(&ctx->references));
}

typedef struct summarystat {
	uint64_t total;
	uint64_t inuse;
	uint64_t malloced;
	uint64_t blocksize;
	uint64_t contextsize;
} summarystat_t;

#ifdef HAVE_LIBXML2
#define TRY0(a)                     \
	do {                        \
		xmlrc = (a);        \
		if (xmlrc < 0)      \
			goto error; \
	} while (0)
static int
xml_renderctx(isc__mem_t *ctx, summarystat_t *summary,
	      xmlTextWriterPtr writer) {
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

	summary->contextsize += sizeof(*ctx) +
				(ctx->max_size + 1) * sizeof(struct stats) +
				ctx->max_size * sizeof(element *) +
				ctx->basic_table_count * sizeof(char *);
#if ISC_MEM_TRACKLINES
	if (ctx->debuglist != NULL) {
		summary->contextsize += DEBUG_TABLE_COUNT *
						sizeof(debuglist_t) +
					ctx->debuglistcnt * sizeof(debuglink_t);
	}
#endif /* if ISC_MEM_TRACKLINES */
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "references"));
	TRY0(xmlTextWriterWriteFormatString(
		writer, "%" PRIuFAST32,
		isc_refcount_current(&ctx->references)));
	TRY0(xmlTextWriterEndElement(writer)); /* references */

	summary->total += ctx->total;
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "total"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64 "",
					    (uint64_t)ctx->total));
	TRY0(xmlTextWriterEndElement(writer)); /* total */

	summary->inuse += ctx->inuse;
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "inuse"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64 "",
					    (uint64_t)ctx->inuse));
	TRY0(xmlTextWriterEndElement(writer)); /* inuse */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "maxinuse"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64 "",
					    (uint64_t)ctx->maxinuse));
	TRY0(xmlTextWriterEndElement(writer)); /* maxinuse */

	summary->malloced += ctx->malloced;
	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "malloced"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64 "",
					    (uint64_t)ctx->malloced));
	TRY0(xmlTextWriterEndElement(writer)); /* malloced */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "maxmalloced"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64 "",
					    (uint64_t)ctx->maxmalloced));
	TRY0(xmlTextWriterEndElement(writer)); /* maxmalloced */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "blocksize"));
	if ((ctx->flags & ISC_MEMFLAG_INTERNAL) != 0) {
		summary->blocksize += ctx->basic_table_count *
				      NUM_BASIC_BLOCKS * ctx->mem_target;
		TRY0(xmlTextWriterWriteFormatString(
			writer, "%" PRIu64 "",
			(uint64_t)ctx->basic_table_count * NUM_BASIC_BLOCKS *
				ctx->mem_target));
	} else {
		TRY0(xmlTextWriterWriteFormatString(writer, "%s", "-"));
	}
	TRY0(xmlTextWriterEndElement(writer)); /* blocksize */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "pools"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%u", ctx->poolcnt));
	TRY0(xmlTextWriterEndElement(writer)); /* pools */
	summary->contextsize += ctx->poolcnt * sizeof(isc_mempool_t);

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "hiwater"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64 "",
					    (uint64_t)ctx->hi_water));
	TRY0(xmlTextWriterEndElement(writer)); /* hiwater */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "lowater"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64 "",
					    (uint64_t)ctx->lo_water));
	TRY0(xmlTextWriterEndElement(writer)); /* lowater */

	TRY0(xmlTextWriterEndElement(writer)); /* context */

error:
	MCTXUNLOCK(ctx);

	return (xmlrc);
}

int
isc_mem_renderxml(void *writer0) {
	isc__mem_t *ctx;
	summarystat_t summary;
	uint64_t lost;
	int xmlrc;
	xmlTextWriterPtr writer = (xmlTextWriterPtr)writer0;

	memset(&summary, 0, sizeof(summary));

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "contexts"));

	LOCK(&contextslock);
	lost = totallost;
	for (ctx = ISC_LIST_HEAD(contexts); ctx != NULL;
	     ctx = ISC_LIST_NEXT(ctx, link))
	{
		xmlrc = xml_renderctx(ctx, &summary, writer);
		if (xmlrc < 0) {
			UNLOCK(&contextslock);
			goto error;
		}
	}
	UNLOCK(&contextslock);

	TRY0(xmlTextWriterEndElement(writer)); /* contexts */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "summary"));

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "TotalUse"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64 "",
					    summary.total));
	TRY0(xmlTextWriterEndElement(writer)); /* TotalUse */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "InUse"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64 "",
					    summary.inuse));
	TRY0(xmlTextWriterEndElement(writer)); /* InUse */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "Malloced"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64 "",
					    summary.malloced));
	TRY0(xmlTextWriterEndElement(writer)); /* InUse */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "BlockSize"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64 "",
					    summary.blocksize));
	TRY0(xmlTextWriterEndElement(writer)); /* BlockSize */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "ContextSize"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64 "",
					    summary.contextsize));
	TRY0(xmlTextWriterEndElement(writer)); /* ContextSize */

	TRY0(xmlTextWriterStartElement(writer, ISC_XMLCHAR "Lost"));
	TRY0(xmlTextWriterWriteFormatString(writer, "%" PRIu64 "", lost));
	TRY0(xmlTextWriterEndElement(writer)); /* Lost */

	TRY0(xmlTextWriterEndElement(writer)); /* summary */
error:
	return (xmlrc);
}

#endif /* HAVE_LIBXML2 */

#ifdef HAVE_JSON_C
#define CHECKMEM(m) RUNTIME_CHECK(m != NULL)

static isc_result_t
json_renderctx(isc__mem_t *ctx, summarystat_t *summary, json_object *array) {
	REQUIRE(VALID_CONTEXT(ctx));
	REQUIRE(summary != NULL);
	REQUIRE(array != NULL);

	json_object *ctxobj, *obj;
	char buf[1024];

	MCTXLOCK(ctx);

	summary->contextsize += sizeof(*ctx) +
				(ctx->max_size + 1) * sizeof(struct stats) +
				ctx->max_size * sizeof(element *) +
				ctx->basic_table_count * sizeof(char *);
	summary->total += ctx->total;
	summary->inuse += ctx->inuse;
	summary->malloced += ctx->malloced;
	if ((ctx->flags & ISC_MEMFLAG_INTERNAL) != 0) {
		summary->blocksize += ctx->basic_table_count *
				      NUM_BASIC_BLOCKS * ctx->mem_target;
	}
#if ISC_MEM_TRACKLINES
	if (ctx->debuglist != NULL) {
		summary->contextsize += DEBUG_TABLE_COUNT *
						sizeof(debuglist_t) +
					ctx->debuglistcnt * sizeof(debuglink_t);
	}
#endif /* if ISC_MEM_TRACKLINES */

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

	obj = json_object_new_int64(ctx->total);
	CHECKMEM(obj);
	json_object_object_add(ctxobj, "total", obj);

	obj = json_object_new_int64(ctx->inuse);
	CHECKMEM(obj);
	json_object_object_add(ctxobj, "inuse", obj);

	obj = json_object_new_int64(ctx->maxinuse);
	CHECKMEM(obj);
	json_object_object_add(ctxobj, "maxinuse", obj);

	obj = json_object_new_int64(ctx->malloced);
	CHECKMEM(obj);
	json_object_object_add(ctxobj, "malloced", obj);

	obj = json_object_new_int64(ctx->maxmalloced);
	CHECKMEM(obj);
	json_object_object_add(ctxobj, "maxmalloced", obj);

	if ((ctx->flags & ISC_MEMFLAG_INTERNAL) != 0) {
		uint64_t blocksize;
		blocksize = ctx->basic_table_count * NUM_BASIC_BLOCKS *
			    ctx->mem_target;
		obj = json_object_new_int64(blocksize);
		CHECKMEM(obj);
		json_object_object_add(ctxobj, "blocksize", obj);
	}

	obj = json_object_new_int64(ctx->poolcnt);
	CHECKMEM(obj);
	json_object_object_add(ctxobj, "pools", obj);

	summary->contextsize += ctx->poolcnt * sizeof(isc_mempool_t);

	obj = json_object_new_int64(ctx->hi_water);
	CHECKMEM(obj);
	json_object_object_add(ctxobj, "hiwater", obj);

	obj = json_object_new_int64(ctx->lo_water);
	CHECKMEM(obj);
	json_object_object_add(ctxobj, "lowater", obj);

	MCTXUNLOCK(ctx);
	json_object_array_add(array, ctxobj);
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_mem_renderjson(void *memobj0) {
	isc_result_t result = ISC_R_SUCCESS;
	isc__mem_t *ctx;
	summarystat_t summary;
	uint64_t lost;
	json_object *ctxarray, *obj;
	json_object *memobj = (json_object *)memobj0;

	memset(&summary, 0, sizeof(summary));

	ctxarray = json_object_new_array();
	CHECKMEM(ctxarray);

	LOCK(&contextslock);
	lost = totallost;
	for (ctx = ISC_LIST_HEAD(contexts); ctx != NULL;
	     ctx = ISC_LIST_NEXT(ctx, link))
	{
		result = json_renderctx(ctx, &summary, ctxarray);
		if (result != ISC_R_SUCCESS) {
			UNLOCK(&contextslock);
			goto error;
		}
	}
	UNLOCK(&contextslock);

	obj = json_object_new_int64(summary.total);
	CHECKMEM(obj);
	json_object_object_add(memobj, "TotalUse", obj);

	obj = json_object_new_int64(summary.inuse);
	CHECKMEM(obj);
	json_object_object_add(memobj, "InUse", obj);

	obj = json_object_new_int64(summary.malloced);
	CHECKMEM(obj);
	json_object_object_add(memobj, "Malloced", obj);

	obj = json_object_new_int64(summary.blocksize);
	CHECKMEM(obj);
	json_object_object_add(memobj, "BlockSize", obj);

	obj = json_object_new_int64(summary.contextsize);
	CHECKMEM(obj);
	json_object_object_add(memobj, "ContextSize", obj);

	obj = json_object_new_int64(lost);
	CHECKMEM(obj);
	json_object_object_add(memobj, "Lost", obj);

	json_object_object_add(memobj, "contexts", ctxarray);
	return (ISC_R_SUCCESS);

error:
	if (ctxarray != NULL) {
		json_object_put(ctxarray);
	}
	return (result);
}
#endif /* HAVE_JSON_C */

void
isc_mem_create(isc_mem_t **mctxp) {
	mem_create(mctxp, isc_mem_defaultflags);
}

void *
isc__mem_get(isc_mem_t *mctx, size_t size FLARG) {
	REQUIRE(ISCAPI_MCTX_VALID(mctx));

	return (mctx->methods->memget(mctx, size FLARG_PASS));
}

void
isc__mem_put(isc_mem_t *mctx, void *ptr, size_t size FLARG) {
	REQUIRE(ISCAPI_MCTX_VALID(mctx));

	mctx->methods->memput(mctx, ptr, size FLARG_PASS);
}

void
isc__mem_putanddetach(isc_mem_t **mctxp, void *ptr, size_t size FLARG) {
	REQUIRE(mctxp != NULL && ISCAPI_MCTX_VALID(*mctxp));

	(*mctxp)->methods->memputanddetach(mctxp, ptr, size FLARG_PASS);
}

void *
isc__mem_allocate(isc_mem_t *mctx, size_t size FLARG) {
	REQUIRE(ISCAPI_MCTX_VALID(mctx));

	return (mctx->methods->memallocate(mctx, size FLARG_PASS));
}

void *
isc__mem_reallocate(isc_mem_t *mctx, void *ptr, size_t size FLARG) {
	REQUIRE(ISCAPI_MCTX_VALID(mctx));

	return (mctx->methods->memreallocate(mctx, ptr, size FLARG_PASS));
}

char *
isc__mem_strdup(isc_mem_t *mctx, const char *s FLARG) {
	REQUIRE(ISCAPI_MCTX_VALID(mctx));

	return (mctx->methods->memstrdup(mctx, s FLARG_PASS));
}

char *
isc__mem_strndup(isc_mem_t *mctx, const char *s, size_t size FLARG) {
	REQUIRE(ISCAPI_MCTX_VALID(mctx));

	return (mctx->methods->memstrndup(mctx, s, size FLARG_PASS));
}

void
isc__mem_free(isc_mem_t *mctx, void *ptr FLARG) {
	REQUIRE(ISCAPI_MCTX_VALID(mctx));

	mctx->methods->memfree(mctx, ptr FLARG_PASS);
}

void
isc__mem_printactive(isc_mem_t *ctx0, FILE *file) {
#if ISC_MEM_TRACKLINES
	REQUIRE(VALID_CONTEXT(ctx0));
	REQUIRE(file != NULL);

	isc__mem_t *ctx = (isc__mem_t *)ctx0;

	print_active(ctx, file);
#else  /* if ISC_MEM_TRACKLINES */
	UNUSED(ctx0);
	UNUSED(file);
#endif /* if ISC_MEM_TRACKLINES */
}
