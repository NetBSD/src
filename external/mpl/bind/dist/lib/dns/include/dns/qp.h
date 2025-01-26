/*	$NetBSD: qp.h,v 1.2 2025/01/26 16:25:28 christos Exp $	*/

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

/*
 * A qp-trie is a kind of key -> value map, supporting lookups that are
 * aware of the lexicographic order of keys.
 *
 * Keys are `dns_qpkey_t`, which is a string-like thing, usually created
 * from a DNS name. You can use both relative and absolute DNS names as
 * keys, even in the same trie, except for one caveat: if a trie contains
 * names relative to the zone apex, the natural way to represent the apex
 * itself (spelled `@` in zone files) is a zero-length name; but a
 * zero-length name has the same qpkey representation as the root zone
 * (apart from its length), so they collide.
 *
 * Leaf values are a pair of a `void *` pointer and a `uint32_t`
 * (because that is what fits inside an internal qp-trie leaf node).
 *
 * The trie does not store keys; instead keys are derived from leaf values
 * by calling a method provided by the user.
 *
 * There are a few flavours of qp-trie.
 *
 * The basic `dns_qp_t` supports single-threaded read/write access.
 *
 * A `dns_qpmulti_t` is a wrapper that supports multithreaded access.
 * There can be many concurrent readers and a single writer. Writes are
 * transactional, and support multi-version concurrency.
 *
 * The concurrency strategy uses copy-on-write. When making changes during
 * a transaction, the caller must not modify leaf values in place, but
 * instead delete the old leaf from the trie and insert a replacement. Leaf
 * values have reference counts, which will indicate when the old leaf
 * value can be freed after it is no longer needed by readers using an old
 * version of the trie.
 *
 * For fast concurrent reads, call `dns_qpmulti_query()` to fill in a
 * `dns_qpread_t`, which must be allocated on the stack. This gives
 * the reader access to a single version of the trie. The reader's
 * thread must be registered with `liburcu`, which is normally taken
 * care of by `libisc`. Readers are not blocked by any write activity,
 * and vice versa.
 *
 * For reads that need a stable view of the trie for multiple cycles
 * of an isc_loop, or which can be used from any thread, call
 * `dns_qpmulti_snapshot()` to get a `dns_qpsnap_t`. A snapshot is for
 * relatively heavy long-running read-only operations such as zone
 * transfers.
 *
 * You can start one read-write transaction at a time using
 * `dns_qpmulti_write()` or `dns_qpmulti_update()`. Either way, you
 * get a `dns_qp_t` that can be modified like a single-threaded trie,
 * without affecting other read-only query or snapshot users of the
 * `dns_qpmulti_t`.
 *
 * "Update" transactions are heavyweight. They allocate working memory to
 * hold modifications to the trie, and compact the trie before committing.
 * For extra space savings, a partially-used allocation chunk is shrunk to
 * the smallest size possible. Unlike "write" transactions, an "update"
 * transaction can be rolled back instead of committed. (Update
 * transactions are intended for things like authoritative zones, where it
 * is important to keep the per-trie memory overhead low because there can
 * be a very large number of them.)
 *
 * "Write" transactions are more lightweight: they skip the allocation and
 * compaction at the start and end of the transaction. (Write transactions
 * are intended for frequent small changes, as in the DNS cache.)
 */

/***********************************************************************
 *
 *  types
 */

#include <isc/attributes.h>

#include <dns/name.h>
#include <dns/types.h>

/*%
 * A `dns_qp_t` supports single-threaded read/write access.
 */
typedef struct dns_qp dns_qp_t;

/*%
 * A `dns_qpmulti_t` supports multi-version wait-free concurrent reads
 * and one transactional modification at a time.
 */
typedef struct dns_qpmulti dns_qpmulti_t;

/*%
 * Read-only parts of a qp-trie.
 *
 * A `dns_qpreader_t` is the common prefix of the `dns_qpreadable`
 * types, containing just the fields neded for the hot path. The
 * internals of a `dns_qpreader_t` are private; they are only exposed
 * so that callers can allocate a `dns_qpread_t` on the stack.
 *
 * Ranty aside: annoyingly, C doesn't allow us to use a predeclared
 * structure type as an anonymous struct member, so we have to use a
 * macro. (GCC and Clang have the feature we want under -fms-extensions,
 * but a non-standard extension won't make these declarations neater if
 * we must also have a standard alternative.)
 */
#define DNS_QPREADER_FIELDS                   \
	uint32_t		    magic;    \
	dns_qpref_t		    root_ref; \
	dns_qpbase_t		   *base;     \
	void			   *uctx;     \
	const struct dns_qpmethods *methods

typedef struct dns_qpbase dns_qpbase_t; /* private, declared in qp_p.h */

/*%
 * A unique twig reference; this can be converted to chunk and cell
 * values to find a specific location.
 */
typedef uint32_t dns_qpref_t;

typedef struct dns_qpreader {
	DNS_QPREADER_FIELDS;
} dns_qpreader_t;

/*%
 * A `dns_qpread_t` is a read-only handle on a `dns_qpmulti_t`.
 * The caller provides space for it on the stack; it can be
 * used by only one thread. As well as the `DNS_QPREADER_FIELDS`,
 * it contains a thread ID to check for incorrect usage.
 *
 * The internals of a `dns_qpread_t` are private; they are only
 * exposed so that callers can allocate an instance on the stack.
 */
typedef struct dns_qpread {
	DNS_QPREADER_FIELDS;
	uint32_t tid;
} dns_qpread_t;

/*%
 * A `dns_qpsnap_t` is a read-only snapshot of a `dns_qpmulti_t`.
 * It requires allocation and taking the `dns_qpmulti_t` mutex to
 * create; it can be used from any thread.
 */
typedef struct dns_qpsnap dns_qpsnap_t;

/*%
 * The read-only qp-trie functions can work on either of the read-only
 * qp-trie types dns_qpsnap_t or dns_qpread_t, or the general-purpose
 * read-write `dns_qp_t`. They rely on the fact that all the
 * `dns_qpreadable_t` structures start with a `dns_qpreader_t`
 */
typedef union dns_qpreadable {
	dns_qpreader_t *qp;
	dns_qpread_t   *qpr;
	dns_qpsnap_t   *qps;
	dns_qp_t       *qpt;
} dns_qpreadable_t __attribute__((__transparent_union__));

#define dns_qpreader(qpr) ((qpr).qp)

/*%
 * The maximum size of a key is also the maximum depth of a trie.
 *
 * A domain name can be up to 255 bytes. When converted to a key, each
 * character in the name corresponds to one byte in the key if it is a
 * common hostname character; otherwise unusual characters are escaped,
 * using two bytes in the key. So we allow keys to be up to 512 bytes.
 * (The actual max is (255 - 5) * 2 + 6 == 506)
 */
#define DNS_QP_MAXKEY 512

/*
 * C is not strict enough with its integer types for the following typedefs
 * to improve type safety, but it helps to have annotations saying what
 * particular kind of number we are dealing with.
 */

/*%
 * The bit number, or position of a bit inside a word. (Valid values 0..63)
 * A dns_qpkey_t (below) is an array of these; each element within dns_qpkey
 * must satisfy:
 *
 *	SHIFT_NOBYTE <= key[off] && key[off] < SHIFT_OFFSET
 */
typedef uint8_t dns_qpshift_t;

/*%
 * The number of bits set in a word (i.e, Hamming weight or popcount).
 * This is used to determine the position of a node in the packed sparse
 * vector of twigs. Valid values are 0..47 (because our bitmap does not
 * fill the entire word).
 */
typedef uint8_t dns_qpweight_t;

/*
 * Chunk and cell numbers, used to identify a specific location in
 * one of the chunks stored in the QP base pointer array. Each cell
 * within a chunk can contain a node.
 */
typedef uint32_t dns_qpchunk_t;
typedef uint32_t dns_qpcell_t;

/*%
 * A trie lookup key is a small array, allocated on the stack during trie
 * searches. Keys are usually created on demand from DNS names using
 * `dns_qpkey_fromname()`, but in principle you can define your own
 * functions to convert other types to trie lookup keys.
 */
typedef dns_qpshift_t dns_qpkey_t[DNS_QP_MAXKEY];

/*%
 * A QP iterator traverses a trie starting with the root and passing
 * though each leaf node in lexicographic order; it is used by
 * `dns_qpiter_init()` and `dns_qpiter_next()`. It is also used
 * internally by `dns_qp_findname_iterator()` to locate the predecessor
 * of a searched-for name.
 */
typedef struct dns_qpiter {
	unsigned int	magic;
	dns_qpreader_t *qp;
	uint16_t	sp;
	dns_qpnode_t   *stack[DNS_QP_MAXKEY];
} dns_qpiter_t;

/*%
 * A QP chain holds references to each populated node between the root and
 * a given leaf. It is used internally by `dns_qp_lookup()` to return a
 * partial match if the specific name requested is not found; optionally it
 * can be passed back to the caller so that individual nodes can be
 * accessed.
 */
typedef struct dns_qpchain {
	unsigned int	magic;
	dns_qpreader_t *qp;
	uint8_t		len;
	struct {
		dns_qpnode_t *node;
		size_t	      offset;
	} chain[DNS_NAME_MAXLABELS];
} dns_qpchain_t;

/*%
 * These leaf methods allow the qp-trie code to call back to the code
 * responsible for the leaf values that are stored in the trie. The
 * methods are provided for a whole trie when the trie is created.
 *
 * When you create a qp-trie, you provide a context pointer that is
 * passed to the methods. The context pointer can tell the methods
 * something about the trie as a whole, in addition to a particular
 * leaf's values.
 *
 * The `attach` and `detach` methods adjust reference counts on value
 * objects. They support copy-on-write and safe memory reclamation
 * needed for multi-version concurrency. The methods are only called
 * when the `dns_qpmulti_t` mutex is held.
 *
 * Note: When a value object reference count is greater than one, the
 * object is in use by concurrent readers so it must not be modified. A
 * refcount equal to one does not indicate whether or not the object is
 * mutable: its refcount can be 1 while it is only in use by readers (and
 * must be left unchanged), or newly created by a writer (and therefore
 * mutable).
 *
 * The `makekey` method fills in a `dns_qpkey_t` corresponding to a
 * value object stored in the qp-trie. It returns the length of the
 * key, which must be less than `sizeof(dns_qpkey_t)`. This method
 * will typically call dns_qpkey_fromname() with a name stored in the
 * value object.
 *
 * For logging and tracing, the `triename` method copies a human-
 * readable identifier into `buf` which has max length `size`.
 */
typedef struct dns_qpmethods {
	void (*attach)(void *uctx, void *pval, uint32_t ival);
	void (*detach)(void *uctx, void *pval, uint32_t ival);
	size_t (*makekey)(dns_qpkey_t key, void *uctx, void *pval,
			  uint32_t ival);
	void (*triename)(void *uctx, char *buf, size_t size);
} dns_qpmethods_t;

/*%
 * Buffers for use by the `triename()` method need to be large enough
 * to hold a zone name and a few descriptive words.
 */
#define DNS_QP_TRIENAME_MAX 300

/*%
 * A container for the counters returned by `dns_qp_memusage()`
 */
typedef struct dns_qp_memusage {
	void  *uctx;	    /*%< qp-trie method context */
	size_t leaves;	    /*%< values in the trie */
	size_t live;	    /*%< nodes in use */
	size_t used;	    /*%< allocated nodes */
	size_t hold;	    /*%< nodes retained for readers */
	size_t free;	    /*%< nodes to be reclaimed */
	size_t node_size;   /*%< in bytes */
	size_t chunk_size;  /*%< nodes per chunk */
	size_t chunk_count; /*%< allocated chunks */
	size_t bytes;	    /*%< total memory in chunks and metadata */
	bool   fragmented;  /*%< trie needs compaction */
} dns_qp_memusage_t;

/*%
 * Choice of mode for `dns_qp_compact()`
 */
typedef enum dns_qpgc {
	DNS_QPGC_MAYBE,
	DNS_QPGC_NOW,
	DNS_QPGC_ALL,
} dns_qpgc_t;

/***********************************************************************
 *
 *  functions - create, destory, enquire
 */

void
dns_qp_create(isc_mem_t *mctx, const dns_qpmethods_t *methods, void *uctx,
	      dns_qp_t **qptp);
/*%<
 * Create a single-threaded qp-trie.
 *
 * Requires:
 * \li  `mctx` is a pointer to a valid memory context.
 * \li  all the methods are non-NULL
 * \li  `qptp != NULL && *qptp == NULL`
 *
 * Ensures:
 * \li  `*qptp` is a pointer to a valid single-threaded qp-trie
 */

void
dns_qp_destroy(dns_qp_t **qptp);
/*%<
 * Destroy a single-threaded qp-trie.
 *
 * Requires:
 * \li  `qptp != NULL`
 * \li  `*qptp` is a pointer to a valid single-threaded qp-trie
 *
 * Ensures:
 * \li  all memory allocated by the qp-trie has been released
 * \li  `*qptp` is NULL
 */

void
dns_qpmulti_create(isc_mem_t *mctx, const dns_qpmethods_t *methods, void *uctx,
		   dns_qpmulti_t **qpmp);
/*%<
 * Create a multi-threaded qp-trie.
 *
 * Requires:
 * \li  `mctx` is a pointer to a valid memory context.
 * \li  all the methods are non-NULL
 * \li  `qpmp != NULL && *qpmp == NULL`
 *
 * Ensures:
 * \li  `*qpmp` is a pointer to a valid multi-threaded qp-trie
 */

void
dns_qpmulti_destroy(dns_qpmulti_t **qpmp);
/*%<
 * Destroy a multi-threaded qp-trie.
 *
 * Requires:
 * \li  `qptp != NULL`
 * \li  `*qptp` is a pointer to a valid multi-threaded qp-trie
 * \li  there are no write or update transactions in progress
 * \li  no snapshots exist
 *
 * Ensures:
 * \li  all memory allocated by the qp-trie has been released
 * \li  `*qpmp` is NULL
 */

void
dns_qp_compact(dns_qp_t *qp, dns_qpgc_t mode);
/*%<
 * Defragment the qp-trie and release unused memory.
 *
 * When modifications make a trie too fragmented, it is automatically
 * compacted. However, automatic compaction is limited when a
 * multithreaded trie has lots of immutable memory from past
 * transactions, and lightweight write transactions do not compact on
 * commit like heavyweight update transactions.
 *
 * This function can be used with a single-threaded qp-trie and during a
 * transaction on a multi-threaded trie.
 *
 * \li	If `mode == DNS_QPGC_MAYBE`, the trie is cleaned if it is fragmented
 *
 * \li	If `mode == DNS_QPGC_NOW`, the trie is cleaned while avoiding
 *	unnecessary work
 *
 * \li	If `mode == DNS_QPGC_ALL`, the entire trie is compacted
 *
 * Requires:
 * \li  `qp` is a pointer to a valid qp-trie
 */

void
dns_qp_gctime(uint64_t *compact_us, uint64_t *recover_us,
	      uint64_t *rollback_us);
/*%<
 * Get the total times spent on garbage collection in microseconds.
 *
 * These counters are global, covering every qp-trie in the program.
 *
 * XXXFANF This is a placeholder until we can record times in histograms.
 */

dns_qp_memusage_t
dns_qp_memusage(dns_qp_t *qp);
/*%<
 * Get the memory counters from a qp-trie
 *
 * Requires:
 * \li  `qp` is a pointer to a valid qp-trie
 *
 * Returns:
 * \li  a `dns_qp_memusage_t` structure described above
 */

dns_qp_memusage_t
dns_qpmulti_memusage(dns_qpmulti_t *multi);
/*%<
 * Get the memory counters from multi-threaded qp-trie outside the
 * context of a transaction.
 *
 * Requires:
 * \li  `multi` is a pointer to a valid dns_qpmulti_t
 *
 * Returns:
 * \li  a `dns_qp_memusage_t` structure described above
 */

/***********************************************************************
 *
 *  functions - search, modify
 */

/*
 * XXXFANF todo, based on what we discover BIND needs
 *
 * more fancy searches: lexicographic predecessor (for NSEC),
 * successor (for modification-safe iteration), etc.
 *
 * do we need specific lookup functions to find out if the
 * returned value is readonly or mutable?
 *
 * richer modification such as dns_qp_replace{key,name}
 */

size_t
dns_qpkey_fromname(dns_qpkey_t key, const dns_name_t *name);
/*%<
 * Convert a DNS name into a trie lookup key.
 *
 * Requires:
 * \li  `name` is a pointer to a valid `dns_name_t`
 *
 * Ensures:
 * \li	returned length is less than `sizeof(dns_qpkey_t)`
 *
 * Returns:
 * \li  the length of the key
 */

void
dns_qpkey_toname(const dns_qpkey_t key, size_t keylen, dns_name_t *name);
/*%<
 * Convert a trie lookup key back into a DNS name.
 *
 * Requires:
 * \li  `name` is a pointer to a valid `dns_name_t`
 * \li  `name->buffer` is not NULL
 * \li  `name->offsets` is not NULL
 */

isc_result_t
dns_qp_getkey(dns_qpreadable_t qpr, const dns_qpkey_t search_key,
	      size_t search_keylen, void **pval_r, uint32_t *ival_r);
/*%<
 * Find a leaf in a qp-trie that matches the given search key
 *
 * The leaf values are assigned to whichever of `*pval_r` and `*ival_r`
 * are not null, unless the return value is ISC_R_NOTFOUND.
 *
 * Requires:
 * \li  `qpr` is a pointer to a readable qp-trie
 * \li	`search_keylen < sizeof(dns_qpkey_t)`
 *
 * Returns:
 * \li  ISC_R_NOTFOUND if the trie has no leaf with a matching key
 * \li  ISC_R_SUCCESS if the leaf was found
 */

isc_result_t
dns_qp_getname(dns_qpreadable_t qpr, const dns_name_t *name, void **pval_r,
	       uint32_t *ival_r);
/*%<
 * Find a leaf in a qp-trie that matches the given DNS name
 *
 * The leaf values are assigned to whichever of `*pval_r` and `*ival_r`
 * are not null, unless the return value is ISC_R_NOTFOUND.
 *
 * Requires:
 * \li  `qpr` is a pointer to a readable qp-trie
 * \li  `name` is a pointer to a valid `dns_name_t`
 *
 * Returns:
 * \li  ISC_R_NOTFOUND if the trie has no leaf with a matching key
 * \li  ISC_R_SUCCESS if the leaf was found
 */

isc_result_t
dns_qp_lookup(dns_qpreadable_t qpr, const dns_name_t *name,
	      dns_name_t *foundname, dns_qpiter_t *iter, dns_qpchain_t *chain,
	      void **pval_r, uint32_t *ival_r);
/*%<
 * Look up a leaf in a qp-trie that is equal to, or an ancestor domain of,
 * 'name'.
 *
 * If 'foundname' is not NULL, it will be updated to contain the name
 * that was found (if any). The return code, ISC_R_SUCCESS or
 * DNS_R_PARTIALMATCH, indicates whether the name found is the name
 * that was requested, or an ancestor. If the result is ISC_R_NOTFOUND,
 * 'foundname' will not be updated. (NOTE: the name will be constructed
 * from the QP key of the found node, and this can be time-consuming.
 * In performance-critical code, it is faster to store a copy of the
 * name in the node data and use that instead of passing 'foundname'.)
 *
 * If 'chain' is not NULL, it is updated to contain a QP chain with
 * references to the populated nodes in the tree between the root and
 * the name that was found. If the return code is DNS_R_PARTIALMATCH
 * then the chain terminates at the closest ancestor found; if it is
 * ISC_R_SUCCESS then it terminates at the name that was requested.
 * If the result is ISC_R_NOTFOUND, 'chain' will not be updated.
 *
 * If 'iter' is not NULL, it will be updated to point to a QP iterator
 * which is pointed at the searched-for name if it exists in the trie,
 * or the closest predecessor if it doesn't.
 *
 * The leaf data for the node that was found will be assigned to
 * whichever of `*pval_r` and `*ival_r` are not NULL, unless the
 * return value is ISC_R_NOTFOUND.
 *
 * Requires:
 * \li  `qpr` is a pointer to a readable qp-trie
 * \li  `name` is a pointer to a valid `dns_name_t`
 * \li  `foundname` is a pointer to a valid `dns_name_t` with
 *       buffer and offset space available, or is NULL
 *
 * Returns:
 * \li  ISC_R_SUCCESS if an exact match was found
 * \li  ISC_R_PARTIALMATCH if an ancestor domain was found
 * \li  ISC_R_NOTFOUND if no match was found
 */

isc_result_t
dns_qp_insert(dns_qp_t *qp, void *pval, uint32_t ival);
/*%<
 * Insert a leaf into a qp-trie
 *
 * Requires:
 * \li  `qp` is a pointer to a valid qp-trie
 * \li  `pval != NULL`
 * \li  `alignof(pval) >= 4`
 *
 * Returns:
 * \li  ISC_R_EXISTS if the trie already has a leaf with the same key
 * \li  ISC_R_SUCCESS if the leaf was added to the trie
 */

isc_result_t
dns_qp_deletekey(dns_qp_t *qp, const dns_qpkey_t key, size_t keylen,
		 void **pval_r, uint32_t *ival_r);
/*%<
 * Delete a leaf from a qp-trie that matches the given key
 *
 * The leaf values are assigned to whichever of `*pval_r` and `*ival_r`
 * are not null, unless the return value is ISC_R_NOTFOUND.
 *
 * Requires:
 * \li  `qp` is a pointer to a valid qp-trie
 * \li	`keylen < sizeof(dns_qpkey_t)`
 *
 * Returns:
 * \li  ISC_R_NOTFOUND if the trie has no leaf with a matching key
 * \li  ISC_R_SUCCESS if the leaf was deleted from the trie
 */

isc_result_t
dns_qp_deletename(dns_qp_t *qp, const dns_name_t *name, void **pval_r,
		  uint32_t *ival_r);
/*%<
 * Delete a leaf from a qp-trie that matches the given DNS name
 *
 * The leaf values are assigned to whichever of `*pval_r` and `*ival_r`
 * are not null, unless the return value is ISC_R_NOTFOUND.
 *
 * Requires:
 * \li  `qp` is a pointer to a valid qp-trie
 * \li  `name` is a pointer to a valid qp-trie
 *
 * Returns:
 * \li  ISC_R_NOTFOUND if the trie has no leaf with a matching name
 * \li  ISC_R_SUCCESS if the leaf was deleted from the trie
 */

void
dns_qpiter_init(dns_qpreadable_t qpr, dns_qpiter_t *qpi);
/*%<
 * Initialize an iterator
 *
 * SAFETY NOTE: If `qpr` is a `dns_qp_t`, it is not safe to modify the
 * trie during iteration. If `qpr` is a `dns_qpread_t` or `dns_qpsnap_t`
 * then (like any other read-only access) modifications will not affect
 * iteration.
 *
 * Requires:
 * \li  `qp` is a pointer to a valid qp-trie
 * \li  `qpi` is a pointer to a qp iterator
 */

isc_result_t
dns_qpiter_next(dns_qpiter_t *qpi, dns_name_t *name, void **pval_r,
		uint32_t *ival_r);
isc_result_t
dns_qpiter_prev(dns_qpiter_t *qpi, dns_name_t *name, void **pval_r,
		uint32_t *ival_r);
/*%<
 * Iterate forward/backward through a QP trie in lexicographic order.
 *
 * The leaf values are assigned to whichever of `*pval_r` and `*ival_r`
 * are not null, unless the return value is ISC_R_NOMORE. Similarly,
 * if `name` is not null, it is updated to contain the node name.
 *
 * NOTE: see the safety note under `dns_qpiter_init()`.
 *
 * For example,
 *
 *	dns_qpiter_t qpi;
 *	void *pval;
 *	uint32_t ival;
 *	dns_qpiter_init(qp, &qpi);
 *	while (dns_qpiter_next(&qpi, &pval, &ival) == ISC_R_SUCCESS) {
 *		// do something with pval and ival
 *	}
 *
 * Requires:
 * \li  `qpi` is a pointer to a valid qp iterator
 *
 * Returns:
 * \li  ISC_R_SUCCESS if a leaf was found and pval_r and ival_r were set
 * \li  ISC_R_NOMORE otherwise
 */

isc_result_t
dns_qpiter_current(dns_qpiter_t *qpi, dns_name_t *name, void **pval_r,
		   uint32_t *ival_r);
/*%<
 * Sets the values of `name`, `pval_r` and `ival_r` to those at the
 * node currently pointed to by `qpi`, but without moving the iterator
 * in either direction. If the iterator is not currently pointed at a
 * leaf node, ISC_R_FAILURE is returned.
 * Requires:
 *
 * \li  `qpi` is a pointer to a valid qp iterator
 *
 * Returns:
 * \li  ISC_R_SUCCESS if a leaf was found and pval_r and ival_r were set
 * \li  ISC_R_FAILURE if the iterator is not initialized or not pointing
 *      at a leaf node
 */

void
dns_qpchain_init(dns_qpreadable_t qpr, dns_qpchain_t *chain);
/*%<
 * Initialize a QP chain.
 *
 * Requires:
 * \li  `qpr` is a pointer to a valid qp-trie
 * \li  `chain` is not NULL
 */

unsigned int
dns_qpchain_length(dns_qpchain_t *chain);
/*%<
 * Returns the length of a QP chain.
 *
 * Requires:
 * \li  `chain` is a pointer to an initialized QP chain object
 */

void
dns_qpchain_node(dns_qpchain_t *chain, unsigned int level, dns_name_t *name,
		 void **pval_r, uint32_t *ival_r);
/*%<
 * Sets 'name' to the name of the leaf referenced at `chain->stack[level]`.
 *
 * The leaf values are assigned to whichever of `*pval_r` and `*ival_r`
 * are not null.
 *
 * Requires:
 * \li  `chain` is a pointer to an initialized QP chain object
 * \li  `level` is less than `chain->len`
 */

/***********************************************************************
 *
 *  functions - transactions
 */

void
dns_qpmulti_query(dns_qpmulti_t *multi, dns_qpread_t *qpr);
/*%<
 * Start a lightweight (brief) read-only transaction
 *
 * The `dns_qpmulti_query()` function must be called from an isc_loop
 * thread and its 'qpr' argument must be allocated on the stack.
 *
 * Requires:
 * \li  `multi` is a pointer to a valid multi-threaded qp-trie
 * \li  `qpr != NULL`
 *
 * Returns:
 * \li  `qpr` is a valid read-only qp-trie handle
 */

void
dns_qpmulti_lockedread(dns_qpmulti_t *multi, dns_qpread_t *qpr);
/*%<
 * Start a read-only transaction that takes the `dns_qpmulti_t` mutex.
 *
 * The `dns_qpmulti_lockedread()` function must NOT be called from an
 * isc_loop thread. We keep query and read transactions separate to
 * avoid accidentally taking or failing to take the mutex.
 *
 * Requires:
 * \li  `multi` is a pointer to a valid multi-threaded qp-trie
 * \li  `qpr != NULL`
 *
 * Returns:
 * \li  `qpr` is a valid read-only qp-trie handle
 */

void
dns_qpread_destroy(dns_qpmulti_t *multi, dns_qpread_t *qpr);
/*%<
 * End a lightweight query or read transaction.
 *
 * Requires:
 * \li  `multi` is a pointer to a valid multi-threaded qp-trie
 * \li  `qpr` is a read-only qp-trie handle obtained from `multi`
 *
 * Returns:
 * \li  `qpr` is invalidated
 */

void
dns_qpmulti_snapshot(dns_qpmulti_t *multi, dns_qpsnap_t **qpsp);
/*%<
 * Start a heavyweight (long) read-only transaction
 *
 * This function briefly takes and releases the modification mutex
 * while allocating a copy of the trie's metadata. While the snapshot
 * exists it does not interfere with other read-only or read-write
 * transactions on the trie.
 *
 * Requires:
 * \li  `multi` is a pointer to a valid multi-threaded qp-trie
 * \li  `qpsp != NULL`
 * \li  `*qpsp == NULL`
 *
 * Returns:
 * \li  `*qpsp` is a pointer to a snapshot obtained from `multi`
 */

void
dns_qpsnap_destroy(dns_qpmulti_t *multi, dns_qpsnap_t **qpsp);
/*%<
 * End a heavyweight read transaction
 *
 * Requires:
 * \li  `multi` is a pointer to a valid multi-threaded qp-trie
 * \li  `qpsp != NULL`
 * \li  `*qpsp` is a pointer to a snapshot obtained from `multi`
 *
 * Returns:
 * \li  `*qpsp == NULL`
 */

void
dns_qpmulti_update(dns_qpmulti_t *multi, dns_qp_t **qptp);
/*%<
 * Start a heavyweight write transaction
 *
 * This style of transaction allocates a copy of the trie's metadata to
 * support rollback, and it aims to minimize the memory usage of the
 * trie between transactions. The trie is compacted when the transaction
 * commits, and any partly-used chunk is shrunk to fit.
 *
 * During the transaction, the modification mutex is held.
 *
 * Requires:
 * \li  `multi` is a pointer to a valid multi-threaded qp-trie
 * \li  `qptp != NULL`
 * \li  `*qptp == NULL`
 *
 * Returns:
 * \li  `*qptp` is a pointer to the modifiable qp-trie inside `multi`
 */

void
dns_qpmulti_write(dns_qpmulti_t *multi, dns_qp_t **qptp);
/*%<
 * Start a lightweight write transaction
 *
 * This style of transaction does not need extra allocations in addition
 * to the ones required by insert and delete operations. It is intended
 * for a large trie that gets frequent small writes, such as a DNS
 * cache.
 *
 * A sequence of lightweight write transactions can accumulate
 * garbage that the automatic compact/recycle cannot reclaim.
 * To reclaim this space, you can use the `dns_qp_memusage
 * fragmented` flag to trigger a call to dns_qp_compact(), or you
 * can use occasional update transactions to compact the trie.
 *
 * During the transaction, the modification mutex is held.
 *
 * Requires:
 * \li  `multi` is a pointer to a valid multi-threaded qp-trie
 * \li  `qptp != NULL`
 * \li  `*qptp == NULL`
 *
 * Returns:
 * \li  `*qptp` is a pointer to the modifiable qp-trie inside `multi`
 */

void
dns_qpmulti_commit(dns_qpmulti_t *multi, dns_qp_t **qptp);
/*%<
 * Complete a modification transaction
 *
 * Apart from memory management logistics, the commit itself only
 * requires flipping the read pointer inside `multi` from the old
 * version of the trie to the new version. Readers are not blocked.
 *
 * This function releases the modification mutex after the post-commit
 * memory reclamation is completed.
 *
 * Requires:
 * \li  `multi` is a pointer to a valid multi-threaded qp-trie
 * \li  `qptp != NULL`
 * \li  `*qptp` is a pointer to the modifiable qp-trie inside `multi`
 *
 * Returns:
 * \li  `*qptp == NULL`
 */

void
dns_qpmulti_rollback(dns_qpmulti_t *multi, dns_qp_t **qptp);
/*%<
 * Abandon an update transaction
 *
 * This function reclaims the memory allocated during the transaction
 * and releases the modification mutex.
 *
 * Requires:
 * \li  `multi` is a pointer to a valid multi-threaded qp-trie
 * \li  `qptp != NULL`
 * \li  `*qptp` is a pointer to the modifiable qp-trie inside `multi`
 * \li  `*qptp` was obtained from `dns_qpmulti_update()`
 *
 * Returns:
 * \li  `*qptp == NULL`
 */

/**********************************************************************/
