/*	$NetBSD: qp.c,v 1.1.1.1 2025/01/26 16:12:34 christos Exp $	*/

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

/*
 * For an overview, see doc/design/qp-trie.md
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
#include <sys/mman.h>
#include <unistd.h>
#endif

#include <isc/atomic.h>
#include <isc/buffer.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/mutex.h>
#include <isc/refcount.h>
#include <isc/result.h>
#include <isc/rwlock.h>
#include <isc/tid.h>
#include <isc/time.h>
#include <isc/types.h>
#include <isc/urcu.h>
#include <isc/util.h>

#include <dns/fixedname.h>
#include <dns/log.h>
#include <dns/name.h>
#include <dns/qp.h>
#include <dns/types.h>

#include "qp_p.h"

#ifndef DNS_QP_LOG_STATS
#define DNS_QP_LOG_STATS 1
#endif
#ifndef DNS_QP_TRACE
#define DNS_QP_TRACE 0
#endif

/*
 * very basic garbage collector statistics
 *
 * XXXFANF for now we're logging GC times, but ideally we should
 * accumulate stats more quietly and report via the statschannel
 */
static atomic_uint_fast64_t compact_time;
static atomic_uint_fast64_t recycle_time;
static atomic_uint_fast64_t rollback_time;

/* for LOG_STATS() format strings */
#define PRItime " %" PRIu64 " ns "

#if DNS_QP_LOG_STATS
#define LOG_STATS(...)                                                      \
	isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE, DNS_LOGMODULE_QP, \
		      ISC_LOG_DEBUG(1), __VA_ARGS__)
#else
#define LOG_STATS(...)
#endif

#if DNS_QP_TRACE
/*
 * TRACE is generally used in allocation-related functions so it doesn't
 * trace very high-frequency ops
 */
#define TRACE(fmt, ...)                                                        \
	do {                                                                   \
		if (isc_log_wouldlog(dns_lctx, ISC_LOG_DEBUG(7))) {            \
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,      \
				      DNS_LOGMODULE_QP, ISC_LOG_DEBUG(7),      \
				      "%s:%d:%s(qp %p uctx \"%s\"):t%u: " fmt, \
				      __FILE__, __LINE__, __func__, qp,        \
				      qp ? TRIENAME(qp) : "(null)", isc_tid(), \
				      ##__VA_ARGS__);                          \
		}                                                              \
	} while (0)
#else
#define TRACE(...)
#endif

/***********************************************************************
 *
 *  converting DNS names to trie keys
 */

/*
 * Number of distinct byte values, i.e. 256
 */
#define BYTE_VALUES (UINT8_MAX + 1)

/*
 * Lookup table mapping bytes in DNS names to bit positions, used
 * by dns_qpkey_fromname() to convert DNS names to qp-trie keys.
 *
 * Each element holds one or two bit positions, bit_one in the
 * lower half and bit_two in the upper half.
 *
 * For common hostname characters, bit_two is zero (which cannot
 * be a valid bit position).
 *
 * For others, bit_one is the escape bit, and bit_two is the
 * position of the character within the escaped range.
 */
uint16_t dns_qp_bits_for_byte[BYTE_VALUES] = { 0 };

/*
 * And the reverse, mapping bit positions to characters, so the tests
 * can print diagnostics involving qp-trie keys.
 *
 * This table only handles the first bit in an escape sequence; we
 * arrange that we can calculate the byte value for both bits by
 * adding the the second bit to the first bit's byte value.
 */
uint8_t dns_qp_byte_for_bit[SHIFT_OFFSET] = { 0 };

/*
 * Fill in the lookup tables at program startup. (It doesn't matter
 * when this is initialized relative to other startup code.)
 */
static void
initialize_bits_for_byte(void) ISC_CONSTRUCTOR;

/*
 * The bit positions for bytes inside labels have to be between
 * SHIFT_BITMAP and SHIFT_OFFSET. (SHIFT_NOBYTE separates labels.)
 *
 * Each byte range in between common hostname characters has a different
 * escape character, to preserve the correct lexical order.
 *
 * Escaped byte ranges mostly fit into the space available in the
 * bitmap, except for those above 'z' (which is mostly bytes with the
 * top bit set). So, when we reach the end of the bitmap we roll over
 * to the next escape character.
 *
 * After filling the table we ensure that the bit positions for
 * hostname characters and escape characters all fit.
 */
static void
initialize_bits_for_byte(void) {
	/* zero common character marker not a valid shift position */
	INSIST(0 < SHIFT_BITMAP);
	/* first bit is common byte or escape byte */
	dns_qpshift_t bit_one = SHIFT_BITMAP;
	/* second bit is position in escaped range */
	dns_qpshift_t bit_two = SHIFT_BITMAP;
	bool escaping = true;

	for (unsigned int byte = 0; byte < BYTE_VALUES; byte++) {
		if (qp_common_character(byte)) {
			escaping = false;
			bit_one++;
			dns_qp_byte_for_bit[bit_one] = byte;
			dns_qp_bits_for_byte[byte] = bit_one;
		} else if ('A' <= byte && byte <= 'Z') {
			/* map upper case to lower case */
			dns_qpshift_t after_esc = bit_one + 1;
			dns_qpshift_t skip_punct = 'a' - '_';
			dns_qpshift_t letter = byte - 'A';
			dns_qpshift_t bit = after_esc + skip_punct + letter;
			dns_qp_bits_for_byte[byte] = bit;
			/* to simplify reverse conversion */
			bit_two++;
		} else {
			/* non-hostname characters need to be escaped */
			if (!escaping || bit_two >= SHIFT_OFFSET) {
				escaping = true;
				bit_one++;
				dns_qp_byte_for_bit[bit_one] = byte;
				bit_two = SHIFT_BITMAP;
			}
			dns_qp_bits_for_byte[byte] = bit_two << 8 | bit_one;
			bit_two++;
		}
	}
	ENSURE(bit_one < SHIFT_OFFSET);
}

/*
 * Convert a DNS name into a trie lookup key.
 *
 * Returns the length of the key.
 *
 * For performance we get our hands dirty in the guts of the name.
 *
 * We don't worry about the distinction between absolute and relative
 * names. When the trie is only used with absolute names, the first byte
 * of the key will always be SHIFT_NOBYTE and it will always be skipped
 * when traversing the trie. So keeping the root label costs little, and
 * it allows us to support tries of relative names too. In fact absolute
 * and relative names can be mixed in the same trie without causing
 * confusion, because the presence or absence of the initial
 * SHIFT_NOBYTE in the key disambiguates them (exactly like a trailing
 * dot in a zone file).
 */
size_t
dns_qpkey_fromname(dns_qpkey_t key, const dns_name_t *name) {
	size_t len, label;
	dns_fixedname_t fixed;

	REQUIRE(ISC_MAGIC_VALID(name, DNS_NAME_MAGIC));

	if (name->labels == 0) {
		key[0] = SHIFT_NOBYTE;
		return 0;
	}

	if (name->offsets == NULL) {
		dns_name_t *clone = dns_fixedname_initname(&fixed);
		dns_name_clone(name, clone);
		name = clone;
	}

	len = 0;
	label = name->labels;
	while (label-- > 0) {
		const uint8_t *ldata = name->ndata + name->offsets[label];
		size_t label_len = *ldata++;
		while (label_len-- > 0) {
			uint16_t bits = dns_qp_bits_for_byte[*ldata++];
			key[len++] = bits & 0xFF;	/* bit_one */
			if ((bits >> 8) != 0) {		/* escape? */
				key[len++] = bits >> 8; /* bit_two */
			}
		}
		/* label terminator */
		key[len++] = SHIFT_NOBYTE;
	}
	/* mark end with a double NOBYTE */
	key[len] = SHIFT_NOBYTE;
	ENSURE(len < sizeof(dns_qpkey_t));
	return len;
}

void
dns_qpkey_toname(const dns_qpkey_t key, size_t keylen, dns_name_t *name) {
	size_t locs[DNS_NAME_MAXLABELS];
	size_t loc = 0, opos = 0;
	size_t offset;

	REQUIRE(ISC_MAGIC_VALID(name, DNS_NAME_MAGIC));
	REQUIRE(name->buffer != NULL);
	REQUIRE(name->offsets != NULL);

	dns_name_reset(name);

	if (keylen == 0) {
		return;
	}

	/* Scan the key looking for label boundaries */
	for (offset = 0; offset <= keylen; offset++) {
		INSIST(key[offset] >= SHIFT_NOBYTE &&
		       key[offset] < SHIFT_OFFSET);
		INSIST(loc < DNS_NAME_MAXLABELS);
		if (qpkey_bit(key, keylen, offset) == SHIFT_NOBYTE) {
			if (qpkey_bit(key, keylen, offset + 1) == SHIFT_NOBYTE)
			{
				locs[loc] = offset + 1;
				goto scanned;
			}
			locs[loc++] = offset + 1;
		} else if (offset == 0) {
			/* This happens for a relative name */
			locs[loc++] = offset;
		}
	}
	UNREACHABLE();
scanned:

	/*
	 * In the key the labels are encoded in reverse order, so
	 * we step backward through the label boundaries, then forward
	 * through the labels, to create the DNS wire format data.
	 */
	name->labels = loc;
	while (loc-- > 0) {
		uint8_t len = 0, *lenp = NULL;

		/* Add a length byte to the name data and set an offset */
		lenp = isc_buffer_used(name->buffer);
		isc_buffer_putuint8(name->buffer, 0);
		name->offsets[opos++] = name->length++;

		/* Convert from escaped byte ranges to ASCII */
		for (offset = locs[loc]; offset < locs[loc + 1] - 1; offset++) {
			uint8_t bit = qpkey_bit(key, keylen, offset);
			uint8_t byte = dns_qp_byte_for_bit[bit];
			if (qp_common_character(byte)) {
				isc_buffer_putuint8(name->buffer, byte);
			} else {
				byte += key[++offset] - SHIFT_BITMAP;
				isc_buffer_putuint8(name->buffer, byte);
			}
			len++;
		}

		name->length += len;
		*lenp = len;
	}

	/* Add a root label for absolute names */
	if (key[0] == SHIFT_NOBYTE) {
		name->attributes.absolute = true;
		isc_buffer_putuint8(name->buffer, 0);
		name->offsets[opos++] = name->length++;
		name->labels++;
	}

	name->ndata = isc_buffer_base(name->buffer);
}

/*
 * Sentinel value for equal keys
 */
#define QPKEY_EQUAL (~(size_t)0)

/*
 * Compare two keys and return the offset where they differ.
 *
 * This offset is used to work out where a trie search diverged: when one
 * of the keys is in the trie and one is not, the common prefix (up to the
 * offset) is the part of the unknown key that exists in the trie. This
 * matters for adding new keys or finding neighbours of missing keys.
 *
 * When the keys are different lengths it is possible (but unwise) for
 * the longer key to be the same as the shorter key but with superfluous
 * trailing SHIFT_NOBYTE elements. This makes the keys equal for the
 * purpose of traversing the trie.
 */
static size_t
qpkey_compare(const dns_qpkey_t key_a, const size_t keylen_a,
	      const dns_qpkey_t key_b, const size_t keylen_b) {
	size_t keylen = ISC_MAX(keylen_a, keylen_b);
	for (size_t offset = 0; offset < keylen; offset++) {
		if (qpkey_bit(key_a, keylen_a, offset) !=
		    qpkey_bit(key_b, keylen_b, offset))
		{
			return offset;
		}
	}
	return QPKEY_EQUAL;
}

/***********************************************************************
 *
 *  allocator wrappers
 */

#if FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION

/*
 * Optionally (for debugging) during a copy-on-write transaction, use
 * memory protection to ensure that the shared chunks are not modified.
 * Once a chunk becomes shared, it remains read-only until it is freed.
 * POSIX says we have to use mmap() to get an allocation that we can
 * definitely pass to mprotect().
 */

static size_t
chunk_size_raw(void) {
	size_t size = (size_t)sysconf(_SC_PAGE_SIZE);
	return ISC_MAX(size, QP_CHUNK_BYTES);
}

static void *
chunk_get_raw(dns_qp_t *qp) {
	if (qp->write_protect) {
		size_t size = chunk_size_raw();
		void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE,
				 MAP_ANON | MAP_PRIVATE, -1, 0);
		RUNTIME_CHECK(ptr != MAP_FAILED);
		return ptr;
	} else {
		return isc_mem_allocate(qp->mctx, QP_CHUNK_BYTES);
	}
}

static void
chunk_free_raw(dns_qp_t *qp, void *ptr) {
	if (qp->write_protect) {
		RUNTIME_CHECK(munmap(ptr, chunk_size_raw()) == 0);
	} else {
		isc_mem_free(qp->mctx, ptr);
	}
}

static void *
chunk_shrink_raw(dns_qp_t *qp, void *ptr, size_t bytes) {
	if (qp->write_protect) {
		return ptr;
	} else {
		return isc_mem_reallocate(qp->mctx, ptr, bytes);
	}
}

static void
write_protect(dns_qp_t *qp, dns_qpchunk_t chunk) {
	if (qp->write_protect) {
		/* see transaction_open() wrt this special case */
		if (qp->transaction_mode == QP_WRITE && chunk == qp->bump) {
			return;
		}
		TRACE("chunk %u", chunk);
		void *ptr = qp->base->ptr[chunk];
		size_t size = chunk_size_raw();
		RUNTIME_CHECK(mprotect(ptr, size, PROT_READ) >= 0);
	}
}

#else

#define chunk_get_raw(qp)	isc_mem_allocate(qp->mctx, QP_CHUNK_BYTES)
#define chunk_free_raw(qp, ptr) isc_mem_free(qp->mctx, ptr)

#define chunk_shrink_raw(qp, ptr, size) isc_mem_reallocate(qp->mctx, ptr, size)

#define write_protect(qp, chunk)

#endif

/***********************************************************************
 *
 *  allocator
 */

/*
 * When we reuse the bump chunk across multiple write transactions,
 * it can have an immutable prefix and a mutable suffix.
 */
static inline bool
cells_immutable(dns_qp_t *qp, dns_qpref_t ref) {
	dns_qpchunk_t chunk = ref_chunk(ref);
	dns_qpcell_t cell = ref_cell(ref);
	if (chunk == qp->bump) {
		return cell < qp->fender;
	} else {
		return qp->usage[chunk].immutable;
	}
}

/*
 * Create a fresh bump chunk and allocate some twigs from it.
 */
static dns_qpref_t
chunk_alloc(dns_qp_t *qp, dns_qpchunk_t chunk, dns_qpweight_t size) {
	INSIST(qp->base->ptr[chunk] == NULL);
	INSIST(qp->usage[chunk].used == 0);
	INSIST(qp->usage[chunk].free == 0);

	qp->base->ptr[chunk] = chunk_get_raw(qp);
	qp->usage[chunk] = (qp_usage_t){ .exists = true, .used = size };
	qp->used_count += size;
	qp->bump = chunk;
	qp->fender = 0;

	if (qp->write_protect) {
		TRACE("chunk %u base %p", chunk, qp->base->ptr[chunk]);
	}
	return make_ref(chunk, 0);
}

/*
 * This is used to grow the chunk arrays when they fill up. If the old
 * base array is in use by readers, we must make a clone, otherwise we
 * can reallocate in place.
 *
 * The isc_refcount_init() and qpbase_unref() in this function are a pair.
 */
static void
realloc_chunk_arrays(dns_qp_t *qp, dns_qpchunk_t newmax) {
	size_t oldptrs = sizeof(qp->base->ptr[0]) * qp->chunk_max;
	size_t newptrs = sizeof(qp->base->ptr[0]) * newmax;
	size_t size = STRUCT_FLEX_SIZE(qp->base, ptr, newmax);

	if (qp->base == NULL || qpbase_unref(qp)) {
		qp->base = isc_mem_reallocate(qp->mctx, qp->base, size);
	} else {
		dns_qpbase_t *oldbase = qp->base;
		qp->base = isc_mem_allocate(qp->mctx, size);
		memmove(&qp->base->ptr[0], &oldbase->ptr[0], oldptrs);
	}
	memset(&qp->base->ptr[qp->chunk_max], 0, newptrs - oldptrs);
	isc_refcount_init(&qp->base->refcount, 1);
	qp->base->magic = QPBASE_MAGIC;

	/* usage array is exclusive to the writer */
	size_t oldusage = sizeof(qp->usage[0]) * qp->chunk_max;
	size_t newusage = sizeof(qp->usage[0]) * newmax;
	qp->usage = isc_mem_reallocate(qp->mctx, qp->usage, newusage);
	memset(&qp->usage[qp->chunk_max], 0, newusage - oldusage);

	qp->chunk_max = newmax;

	TRACE("qpbase %p usage %p max %u", qp->base, qp->usage, qp->chunk_max);
}

/*
 * There was no space in the bump chunk, so find a place to put a fresh
 * chunk in the chunk arrays, then allocate some twigs from it.
 */
static dns_qpref_t
alloc_slow(dns_qp_t *qp, dns_qpweight_t size) {
	dns_qpchunk_t chunk;

	for (chunk = 0; chunk < qp->chunk_max; chunk++) {
		if (!qp->usage[chunk].exists) {
			return chunk_alloc(qp, chunk, size);
		}
	}
	ENSURE(chunk == qp->chunk_max);
	realloc_chunk_arrays(qp, GROWTH_FACTOR(chunk));
	return chunk_alloc(qp, chunk, size);
}

/*
 * Ensure we are using a fresh bump chunk.
 */
static void
alloc_reset(dns_qp_t *qp) {
	(void)alloc_slow(qp, 0);
}

/*
 * Allocate some fresh twigs. This is the bump allocator fast path.
 */
static inline dns_qpref_t
alloc_twigs(dns_qp_t *qp, dns_qpweight_t size) {
	dns_qpchunk_t chunk = qp->bump;
	dns_qpcell_t cell = qp->usage[chunk].used;

	if (cell + size <= QP_CHUNK_SIZE) {
		qp->usage[chunk].used += size;
		qp->used_count += size;
		return make_ref(chunk, cell);
	} else {
		return alloc_slow(qp, size);
	}
}

/*
 * Record that some twigs are no longer being used, and if possible
 * zero them to ensure that there isn't a spurious double detach when
 * the chunk is later recycled.
 *
 * Returns true if the twigs were immediately destroyed.
 *
 * NOTE: the caller is responsible for attaching or detaching any
 * leaves as required.
 */
static inline bool
free_twigs(dns_qp_t *qp, dns_qpref_t twigs, dns_qpweight_t size) {
	dns_qpchunk_t chunk = ref_chunk(twigs);

	qp->free_count += size;
	qp->usage[chunk].free += size;
	ENSURE(qp->free_count <= qp->used_count);
	ENSURE(qp->usage[chunk].free <= qp->usage[chunk].used);

	if (cells_immutable(qp, twigs)) {
		qp->hold_count += size;
		ENSURE(qp->free_count >= qp->hold_count);
		return false;
	} else {
		zero_twigs(ref_ptr(qp, twigs), size);
		return true;
	}
}

/*
 * When some twigs have been copied, and free_twigs() could not
 * immediately destroy the old copy, we need to update the refcount
 * on any leaves that were duplicated.
 */
static void
attach_twigs(dns_qp_t *qp, dns_qpnode_t *twigs, dns_qpweight_t size) {
	for (dns_qpweight_t pos = 0; pos < size; pos++) {
		if (node_tag(&twigs[pos]) == LEAF_TAG) {
			attach_leaf(qp, &twigs[pos]);
		}
	}
}

/***********************************************************************
 *
 *  chunk reclamation
 */

/*
 * Is any of this chunk still in use?
 */
static inline dns_qpcell_t
chunk_usage(dns_qp_t *qp, dns_qpchunk_t chunk) {
	return qp->usage[chunk].used - qp->usage[chunk].free;
}

/*
 * We remove each empty chunk from the total counts when the chunk is
 * freed, or when it is scheduled for safe memory reclamation. We check
 * the chunk's phase to avoid discounting it twice in the latter case.
 */
static void
chunk_discount(dns_qp_t *qp, dns_qpchunk_t chunk) {
	if (qp->usage[chunk].discounted) {
		return;
	}
	INSIST(qp->used_count >= qp->usage[chunk].used);
	INSIST(qp->free_count >= qp->usage[chunk].free);
	qp->used_count -= qp->usage[chunk].used;
	qp->free_count -= qp->usage[chunk].free;
	qp->usage[chunk].discounted = true;
}

/*
 * When a chunk is being recycled, we need to detach any leaves that
 * remain, and free any `base` arrays that have been marked as unused.
 */
static void
chunk_free(dns_qp_t *qp, dns_qpchunk_t chunk) {
	if (qp->write_protect) {
		TRACE("chunk %u base %p", chunk, qp->base->ptr[chunk]);
	}

	dns_qpnode_t *n = qp->base->ptr[chunk];
	for (dns_qpcell_t count = qp->usage[chunk].used; count > 0;
	     count--, n++)
	{
		if (node_tag(n) == LEAF_TAG && node_pointer(n) != NULL) {
			detach_leaf(qp, n);
		} else if (count > 1 && reader_valid(n)) {
			dns_qpreader_t qpr;
			unpack_reader(&qpr, n);
			/* pairs with dns_qpmulti_commit() */
			if (qpbase_unref(&qpr)) {
				isc_mem_free(qp->mctx, qpr.base);
			}
		}
	}
	chunk_discount(qp, chunk);
	chunk_free_raw(qp, qp->base->ptr[chunk]);
	qp->base->ptr[chunk] = NULL;
	qp->usage[chunk] = (qp_usage_t){};
}

/*
 * Free any chunks that we can while a trie is in use.
 */
static void
recycle(dns_qp_t *qp) {
	unsigned int free = 0;

	isc_nanosecs_t start = isc_time_monotonic();

	for (dns_qpchunk_t chunk = 0; chunk < qp->chunk_max; chunk++) {
		if (chunk != qp->bump && chunk_usage(qp, chunk) == 0 &&
		    qp->usage[chunk].exists && !qp->usage[chunk].immutable)
		{
			chunk_free(qp, chunk);
			free++;
		}
	}

	isc_nanosecs_t time = isc_time_monotonic() - start;
	atomic_fetch_add_relaxed(&recycle_time, time);

	if (free > 0) {
		LOG_STATS("qp recycle" PRItime "free %u chunks", time, free);
		LOG_STATS("qp recycle leaf %u live %u used %u free %u hold %u",
			  qp->leaf_count, qp->used_count - qp->free_count,
			  qp->used_count, qp->free_count, qp->hold_count);
	}
}

/*
 * asynchronous cleanup
 */
static void
reclaim_chunks_cb(struct rcu_head *arg) {
	qp_rcuctx_t *rcuctx = caa_container_of(arg, qp_rcuctx_t, rcu_head);
	REQUIRE(QPRCU_VALID(rcuctx));
	dns_qpmulti_t *multi = rcuctx->multi;
	REQUIRE(QPMULTI_VALID(multi));

	LOCK(&multi->mutex);

	dns_qp_t *qp = &multi->writer;
	REQUIRE(QP_VALID(qp));

	unsigned int free = 0;
	isc_nanosecs_t start = isc_time_monotonic();

	for (unsigned int i = 0; i < rcuctx->count; i++) {
		dns_qpchunk_t chunk = rcuctx->chunk[i];
		if (qp->usage[chunk].snapshot) {
			/* cleanup when snapshot is destroyed */
			qp->usage[chunk].snapfree = true;
		} else {
			chunk_free(qp, chunk);
			free++;
		}
	}

	isc_mem_putanddetach(&rcuctx->mctx, rcuctx,
			     STRUCT_FLEX_SIZE(rcuctx, chunk, rcuctx->count));

	isc_nanosecs_t time = isc_time_monotonic() - start;
	recycle_time += time;

	if (free > 0) {
		LOG_STATS("qp reclaim" PRItime "free %u chunks", time, free);
		LOG_STATS("qp reclaim leaf %u live %u used %u free %u hold %u",
			  qp->leaf_count, qp->used_count - qp->free_count,
			  qp->used_count, qp->free_count, qp->hold_count);
	}

	UNLOCK(&multi->mutex);
}

/*
 * At the end of a transaction, schedule empty but immutable chunks
 * for reclamation later.
 */
static void
reclaim_chunks(dns_qpmulti_t *multi) {
	dns_qp_t *qp = &multi->writer;

	unsigned int count = 0;
	for (dns_qpchunk_t chunk = 0; chunk < qp->chunk_max; chunk++) {
		if (chunk != qp->bump && chunk_usage(qp, chunk) == 0 &&
		    qp->usage[chunk].exists && qp->usage[chunk].immutable &&
		    !qp->usage[chunk].discounted)
		{
			count++;
		}
	}

	if (count == 0) {
		return;
	}

	qp_rcuctx_t *rcuctx =
		isc_mem_get(qp->mctx, STRUCT_FLEX_SIZE(rcuctx, chunk, count));
	*rcuctx = (qp_rcuctx_t){
		.magic = QPRCU_MAGIC,
		.multi = multi,
		.count = count,
	};
	isc_mem_attach(qp->mctx, &rcuctx->mctx);

	unsigned int i = 0;
	for (dns_qpchunk_t chunk = 0; chunk < qp->chunk_max; chunk++) {
		if (chunk != qp->bump && chunk_usage(qp, chunk) == 0 &&
		    qp->usage[chunk].exists && qp->usage[chunk].immutable &&
		    !qp->usage[chunk].discounted)
		{
			rcuctx->chunk[i++] = chunk;
			chunk_discount(qp, chunk);
		}
	}

	call_rcu(&rcuctx->rcu_head, reclaim_chunks_cb);

	LOG_STATS("qp will reclaim %u chunks", count);
}

/*
 * When a snapshot is destroyed, clean up chunks that need free()ing
 * and are not used by any remaining snapshots.
 */
static void
marksweep_chunks(dns_qpmulti_t *multi) {
	unsigned int free = 0;

	isc_nanosecs_t start = isc_time_monotonic();

	dns_qp_t *qpw = &multi->writer;

	for (dns_qpsnap_t *qps = ISC_LIST_HEAD(multi->snapshots); qps != NULL;
	     qps = ISC_LIST_NEXT(qps, link))
	{
		for (dns_qpchunk_t chunk = 0; chunk < qps->chunk_max; chunk++) {
			if (qps->base->ptr[chunk] != NULL) {
				INSIST(qps->base->ptr[chunk] ==
				       qpw->base->ptr[chunk]);
				qpw->usage[chunk].snapmark = true;
			}
		}
	}

	for (dns_qpchunk_t chunk = 0; chunk < qpw->chunk_max; chunk++) {
		qpw->usage[chunk].snapshot = qpw->usage[chunk].snapmark;
		qpw->usage[chunk].snapmark = false;
		if (qpw->usage[chunk].snapfree && !qpw->usage[chunk].snapshot) {
			chunk_free(qpw, chunk);
			free++;
		}
	}

	isc_nanosecs_t time = isc_time_monotonic() - start;
	recycle_time += time;

	if (free > 0) {
		LOG_STATS("qp marksweep" PRItime "free %u chunks", time, free);
		LOG_STATS(
			"qp marksweep leaf %u live %u used %u free %u hold %u",
			qpw->leaf_count, qpw->used_count - qpw->free_count,
			qpw->used_count, qpw->free_count, qpw->hold_count);
	}
}

/***********************************************************************
 *
 *  garbage collector
 */

/*
 * Move a branch node's twigs to the `bump` chunk, for copy-on-write
 * or for garbage collection. We don't update the node in place
 * because `compact_recursive()` does not ensure the node itself is
 * mutable until after it discovers evacuation was necessary.
 *
 * If free_twigs() could not immediately destroy the old twigs, we have
 * to re-attach to any leaves.
 */
static dns_qpref_t
evacuate(dns_qp_t *qp, dns_qpnode_t *n) {
	dns_qpweight_t size = branch_twigs_size(n);
	dns_qpref_t old_ref = branch_twigs_ref(n);
	dns_qpref_t new_ref = alloc_twigs(qp, size);
	dns_qpnode_t *old_twigs = ref_ptr(qp, old_ref);
	dns_qpnode_t *new_twigs = ref_ptr(qp, new_ref);

	move_twigs(new_twigs, old_twigs, size);
	if (!free_twigs(qp, old_ref, size)) {
		attach_twigs(qp, new_twigs, size);
	}

	return new_ref;
}

/*
 * Immutable nodes need copy-on-write. As we walk down the trie finding the
 * right place to modify, make_root_mutable() and make_twigs_mutable()
 * are called to ensure that immutable nodes on the path from the root are
 * copied to a mutable chunk.
 */

static inline dns_qpnode_t *
make_root_mutable(dns_qp_t *qp) {
	if (cells_immutable(qp, qp->root_ref)) {
		qp->root_ref = evacuate(qp, MOVABLE_ROOT(qp));
	}
	return ref_ptr(qp, qp->root_ref);
}

static inline void
make_twigs_mutable(dns_qp_t *qp, dns_qpnode_t *n) {
	if (cells_immutable(qp, branch_twigs_ref(n))) {
		*n = make_node(branch_index(n), evacuate(qp, n));
	}
}

/*
 * Compact the trie by traversing the whole thing recursively, copying
 * bottom-up as required. The aim is to avoid evacuation as much as
 * possible, but when parts of the trie are immutable, we need to evacuate
 * the paths from the root to the parts of the trie that occupy
 * fragmented chunks.
 *
 * Without the QP_MIN_USED check, the algorithm will leave the trie
 * unchanged. If the children are all leaves, the loop changes nothing,
 * so we will return this node's original ref. If all of the children
 * that are branches did not need moving, again, the loop changes
 * nothing. So the evacuation check is the only place that the
 * algorithm introduces ref changes, that then bubble up towards the
 * root through the logic inside the loop.
 */
static dns_qpref_t
compact_recursive(dns_qp_t *qp, dns_qpnode_t *parent) {
	dns_qpweight_t size = branch_twigs_size(parent);
	dns_qpref_t twigs_ref = branch_twigs_ref(parent);
	dns_qpchunk_t chunk = ref_chunk(twigs_ref);

	if (qp->compact_all ||
	    (chunk != qp->bump && chunk_usage(qp, chunk) < QP_MIN_USED))
	{
		twigs_ref = evacuate(qp, parent);
	}
	bool immutable = cells_immutable(qp, twigs_ref);
	for (dns_qpweight_t pos = 0; pos < size; pos++) {
		dns_qpnode_t *child = ref_ptr(qp, twigs_ref) + pos;
		if (!is_branch(child)) {
			continue;
		}
		dns_qpref_t old_grandtwigs = branch_twigs_ref(child);
		dns_qpref_t new_grandtwigs = compact_recursive(qp, child);
		if (old_grandtwigs == new_grandtwigs) {
			continue;
		}
		if (immutable) {
			twigs_ref = evacuate(qp, parent);
			/* the twigs have moved */
			child = ref_ptr(qp, twigs_ref) + pos;
			immutable = false;
		}
		*child = make_node(branch_index(child), new_grandtwigs);
	}
	return twigs_ref;
}

static void
compact(dns_qp_t *qp) {
	LOG_STATS("qp compact before leaf %u live %u used %u free %u hold %u",
		  qp->leaf_count, qp->used_count - qp->free_count,
		  qp->used_count, qp->free_count, qp->hold_count);

	isc_nanosecs_t start = isc_time_monotonic();

	if (qp->usage[qp->bump].free > QP_MAX_FREE) {
		alloc_reset(qp);
	}

	if (qp->leaf_count > 0) {
		qp->root_ref = compact_recursive(qp, MOVABLE_ROOT(qp));
	}
	qp->compact_all = false;

	isc_nanosecs_t time = isc_time_monotonic() - start;
	atomic_fetch_add_relaxed(&compact_time, time);

	LOG_STATS("qp compact" PRItime
		  "leaf %u live %u used %u free %u hold %u",
		  time, qp->leaf_count, qp->used_count - qp->free_count,
		  qp->used_count, qp->free_count, qp->hold_count);
}

void
dns_qp_compact(dns_qp_t *qp, dns_qpgc_t mode) {
	REQUIRE(QP_VALID(qp));
	if (mode == DNS_QPGC_MAYBE && !QP_NEEDGC(qp)) {
		return;
	}
	if (mode == DNS_QPGC_ALL) {
		alloc_reset(qp);
		qp->compact_all = true;
	}
	compact(qp);
	recycle(qp);
}

/*
 * Free some twigs and (if they were destroyed immediately so that the
 * result from QP_MAX_GARBAGE can change) compact the trie if necessary.
 *
 * This is called by the trie modification API entry points. The
 * free_twigs() function requires the caller to attach or detach any
 * leaves as necessary. Callers of squash_twigs() satisfy this
 * requirement by calling make_twigs_mutable().
 *
 * Aside: In typical garbage collectors, compaction is triggered when
 * the allocator runs out of space. But that is because typical garbage
 * collectors do not know how much memory can be recovered, so they must
 * find out by scanning the heap. The qp-trie code was originally
 * designed to use malloc() and free(), so it has more information about
 * when garbage collection might be worthwhile. Hence we can trigger
 * collection when garbage passes a threshold.
 *
 * XXXFANF: If we need to avoid latency outliers caused by compaction in
 * write transactions, we can check qp->transaction_mode here.
 */
static inline bool
squash_twigs(dns_qp_t *qp, dns_qpref_t twigs, dns_qpweight_t size) {
	bool destroyed = free_twigs(qp, twigs, size);
	if (destroyed && QP_AUTOGC(qp)) {
		compact(qp);
		recycle(qp);
		/*
		 * This shouldn't happen if the garbage collector is
		 * working correctly. We can recover at the cost of some
		 * time and space, but recovery should be cheaper than
		 * letting compact+recycle fail repeatedly.
		 */
		if (QP_AUTOGC(qp)) {
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_DATABASE,
				      DNS_LOGMODULE_QP, ISC_LOG_NOTICE,
				      "qp %p uctx \"%s\" compact/recycle "
				      "failed to recover any space, "
				      "scheduling a full compaction",
				      qp, TRIENAME(qp));
			qp->compact_all = true;
		}
	}
	return destroyed;
}

/***********************************************************************
 *
 *  public accessors for memory management internals
 */

dns_qp_memusage_t
dns_qp_memusage(dns_qp_t *qp) {
	REQUIRE(QP_VALID(qp));

	dns_qp_memusage_t memusage = {
		.uctx = qp->uctx,
		.leaves = qp->leaf_count,
		.live = qp->used_count - qp->free_count,
		.used = qp->used_count,
		.hold = qp->hold_count,
		.free = qp->free_count,
		.node_size = sizeof(dns_qpnode_t),
		.chunk_size = QP_CHUNK_SIZE,
		.fragmented = QP_NEEDGC(qp),
	};

	for (dns_qpchunk_t chunk = 0; chunk < qp->chunk_max; chunk++) {
		if (qp->base->ptr[chunk] != NULL) {
			memusage.chunk_count += 1;
		}
	}

	/*
	 * XXXFANF does not subtract chunks that have been shrunk,
	 * and does not count unreclaimed dns_qpbase_t objects
	 */
	memusage.bytes = memusage.chunk_count * QP_CHUNK_BYTES +
			 qp->chunk_max * sizeof(qp->base->ptr[0]) +
			 qp->chunk_max * sizeof(qp->usage[0]);

	return memusage;
}

dns_qp_memusage_t
dns_qpmulti_memusage(dns_qpmulti_t *multi) {
	REQUIRE(QPMULTI_VALID(multi));
	LOCK(&multi->mutex);

	dns_qp_t *qp = &multi->writer;
	INSIST(QP_VALID(qp));

	dns_qp_memusage_t memusage = dns_qp_memusage(qp);

	if (qp->transaction_mode == QP_UPDATE) {
		memusage.bytes -= QP_CHUNK_BYTES;
		memusage.bytes += qp->usage[qp->bump].used *
				  sizeof(dns_qpnode_t);
	}

	UNLOCK(&multi->mutex);
	return memusage;
}

void
dns_qp_gctime(isc_nanosecs_t *compact_p, isc_nanosecs_t *recycle_p,
	      isc_nanosecs_t *rollback_p) {
	*compact_p = atomic_load_relaxed(&compact_time);
	*recycle_p = atomic_load_relaxed(&recycle_time);
	*rollback_p = atomic_load_relaxed(&rollback_time);
}

/***********************************************************************
 *
 *  read-write transactions
 */

static dns_qp_t *
transaction_open(dns_qpmulti_t *multi, dns_qp_t **qptp) {
	REQUIRE(QPMULTI_VALID(multi));
	REQUIRE(qptp != NULL && *qptp == NULL);

	LOCK(&multi->mutex);

	dns_qp_t *qp = &multi->writer;
	INSIST(QP_VALID(qp));

	/*
	 * Mark existing chunks as immutable.
	 *
	 * Aside: The bump chunk is special: in a series of write
	 * transactions the bump chunk is reused; the first part (up
	 * to fender) is immutable, the rest mutable. But we set its
	 * immutable flag so that when the bump chunk fills up, the
	 * first part continues to be treated as immutable. (And the
	 * rest of the chunk too, but that's OK.)
	 */
	for (dns_qpchunk_t chunk = 0; chunk < qp->chunk_max; chunk++) {
		if (qp->usage[chunk].exists) {
			qp->usage[chunk].immutable = true;
			write_protect(qp, chunk);
		}
	}

	/*
	 * Ensure QP_AUTOGC() ignores free space in immutable chunks.
	 */
	qp->hold_count = qp->free_count;

	*qptp = qp;
	return qp;
}

/*
 * a write is light
 *
 * We need to ensure we allocate from a fresh chunk if the last transaction
 * shrunk the bump chunk; but usually in a sequence of write transactions
 * we just put `fender` at the point where we started this generation.
 *
 * (Aside: Instead of keeping the previous transaction's mode, I
 * considered forcing allocation into the slow path by fiddling with
 * the bump chunk's usage counters. But that is troublesome because
 * `chunk_free()` needs to know how much of the chunk to scan.)
 */
void
dns_qpmulti_write(dns_qpmulti_t *multi, dns_qp_t **qptp) {
	dns_qp_t *qp = transaction_open(multi, qptp);
	TRACE("");

	if (qp->transaction_mode == QP_WRITE) {
		qp->fender = qp->usage[qp->bump].used;
	} else {
		alloc_reset(qp);
	}
	qp->transaction_mode = QP_WRITE;
}

/*
 * an update is heavier
 *
 * We always reset the allocator to the start of a fresh chunk,
 * because the previous transaction was probably an update that shrunk
 * the bump chunk. It simplifies rollback because `fender` is always zero.
 *
 * To rollback a transaction, we need to reset all the allocation
 * counters to their previous state, in particular we need to un-free
 * any nodes that were copied to make them mutable. This means we need
 * to make a copy of basically the whole `dns_qp_t writer`: everything
 * but the chunks holding the trie nodes.
 *
 * We do most of the transaction setup before creating the rollback
 * state so that after rollback we have a correct idea of which chunks
 * are immutable, and so we have the correct transaction mode to make
 * the next transaction allocate a new bump chunk. The exception is
 * resetting the allocator, which we do after creating the rollback
 * state; if this transaction is rolled back then the next transaction
 * will start from the rollback state and also reset the allocator as
 * one of its first actions.
 */
void
dns_qpmulti_update(dns_qpmulti_t *multi, dns_qp_t **qptp) {
	dns_qp_t *qp = transaction_open(multi, qptp);
	TRACE("");

	qp->transaction_mode = QP_UPDATE;

	dns_qp_t *rollback = isc_mem_allocate(qp->mctx, sizeof(*rollback));
	memmove(rollback, qp, sizeof(*rollback));
	/* can be uninitialized on the first transaction */
	if (rollback->base != NULL) {
		INSIST(QPBASE_VALID(rollback->base));
		INSIST(qp->usage != NULL && qp->chunk_max > 0);
		/* paired with either _commit() or _rollback() */
		isc_refcount_increment(&rollback->base->refcount);
		size_t usage_bytes = sizeof(qp->usage[0]) * qp->chunk_max;
		rollback->usage = isc_mem_allocate(qp->mctx, usage_bytes);
		memmove(rollback->usage, qp->usage, usage_bytes);
	}
	INSIST(multi->rollback == NULL);
	multi->rollback = rollback;

	alloc_reset(qp);
}

void
dns_qpmulti_commit(dns_qpmulti_t *multi, dns_qp_t **qptp) {
	REQUIRE(QPMULTI_VALID(multi));
	REQUIRE(qptp != NULL && *qptp == &multi->writer);
	REQUIRE(multi->writer.transaction_mode == QP_WRITE ||
		multi->writer.transaction_mode == QP_UPDATE);

	dns_qp_t *qp = *qptp;
	TRACE("");

	if (qp->transaction_mode == QP_UPDATE) {
		INSIST(multi->rollback != NULL);
		/* paired with dns_qpmulti_update() */
		if (qpbase_unref(multi->rollback)) {
			isc_mem_free(qp->mctx, multi->rollback->base);
		}
		if (multi->rollback->usage != NULL) {
			isc_mem_free(qp->mctx, multi->rollback->usage);
		}
		isc_mem_free(qp->mctx, multi->rollback);
	}
	INSIST(multi->rollback == NULL);

	/* not the first commit? */
	if (multi->reader_ref != INVALID_REF) {
		INSIST(cells_immutable(qp, multi->reader_ref));
		free_twigs(qp, multi->reader_ref, READER_SIZE);
	}

	if (qp->transaction_mode == QP_UPDATE) {
		/* minimize memory overhead */
		compact(qp);
		multi->reader_ref = alloc_twigs(qp, READER_SIZE);
		qp->base->ptr[qp->bump] = chunk_shrink_raw(
			qp, qp->base->ptr[qp->bump],
			qp->usage[qp->bump].used * sizeof(dns_qpnode_t));
	} else {
		multi->reader_ref = alloc_twigs(qp, READER_SIZE);
	}

	/* anchor a new version of the trie */
	dns_qpnode_t *reader = ref_ptr(qp, multi->reader_ref);
	make_reader(reader, multi);
	/* paired with chunk_free() */
	isc_refcount_increment(&qp->base->refcount);

	rcu_assign_pointer(multi->reader, reader); /* COMMIT */

	/* clean up what we can right now */
	if (qp->transaction_mode == QP_UPDATE || QP_NEEDGC(qp)) {
		recycle(qp);
	}

	/* schedule the rest for later */
	reclaim_chunks(multi);

	*qptp = NULL;
	UNLOCK(&multi->mutex);
}

/*
 * Throw away everything that was allocated during this transaction.
 */
void
dns_qpmulti_rollback(dns_qpmulti_t *multi, dns_qp_t **qptp) {
	unsigned int free = 0;

	REQUIRE(QPMULTI_VALID(multi));
	REQUIRE(multi->writer.transaction_mode == QP_UPDATE);
	REQUIRE(qptp != NULL && *qptp == &multi->writer);

	dns_qp_t *qp = *qptp;
	TRACE("");

	isc_nanosecs_t start = isc_time_monotonic();

	for (dns_qpchunk_t chunk = 0; chunk < qp->chunk_max; chunk++) {
		if (qp->base->ptr[chunk] != NULL && !qp->usage[chunk].immutable)
		{
			chunk_free(qp, chunk);
			/*
			 * we need to clear its base pointer in the rollback
			 * trie, in case the arrays were resized
			 */
			if (chunk < multi->rollback->chunk_max) {
				INSIST(!multi->rollback->usage[chunk].exists);
				multi->rollback->base->ptr[chunk] = NULL;
			}
			free++;
		}
	}

	/*
	 * multi->rollback->base and multi->writer->base are the same,
	 * unless there was a realloc_chunk_arrays() during the transaction
	 */
	if (qpbase_unref(qp)) {
		/* paired with dns_qpmulti_update() */
		isc_mem_free(qp->mctx, qp->base);
	}
	isc_mem_free(qp->mctx, qp->usage);

	/* reset allocator state */
	INSIST(multi->rollback != NULL);
	memmove(qp, multi->rollback, sizeof(*qp));
	isc_mem_free(qp->mctx, multi->rollback);
	INSIST(multi->rollback == NULL);

	isc_nanosecs_t time = isc_time_monotonic() - start;
	atomic_fetch_add_relaxed(&rollback_time, time);

	LOG_STATS("qp rollback" PRItime "free %u chunks", time, free);

	*qptp = NULL;
	UNLOCK(&multi->mutex);
}

/***********************************************************************
 *
 *  read-only transactions
 */

static dns_qpmulti_t *
reader_open(dns_qpmulti_t *multi, dns_qpreadable_t qpr) {
	dns_qpreader_t *qp = dns_qpreader(qpr);
	dns_qpnode_t *reader = rcu_dereference(multi->reader);
	if (reader == NULL) {
		QP_INIT(qp, multi->writer.methods, multi->writer.uctx);
	} else {
		multi = unpack_reader(qp, reader);
	}
	return multi;
}

/*
 * a query is light
 */

void
dns_qpmulti_query(dns_qpmulti_t *multi, dns_qpread_t *qp) {
	REQUIRE(QPMULTI_VALID(multi));
	REQUIRE(qp != NULL);

	qp->tid = isc_tid();
	rcu_read_lock();

	dns_qpmulti_t *whence = reader_open(multi, qp);
	INSIST(whence == multi);
}

void
dns_qpread_destroy(dns_qpmulti_t *multi, dns_qpread_t *qp) {
	REQUIRE(QPMULTI_VALID(multi));
	REQUIRE(QP_VALID(qp));
	REQUIRE(qp->tid == isc_tid());
	*qp = (dns_qpread_t){};
	rcu_read_unlock();
}

/*
 * a snapshot is heavy
 */

void
dns_qpmulti_snapshot(dns_qpmulti_t *multi, dns_qpsnap_t **qpsp) {
	REQUIRE(QPMULTI_VALID(multi));
	REQUIRE(qpsp != NULL && *qpsp == NULL);

	rcu_read_lock();

	LOCK(&multi->mutex);

	dns_qp_t *qpw = &multi->writer;
	size_t bytes = sizeof(dns_qpsnap_t) + sizeof(dns_qpbase_t) +
		       sizeof(qpw->base->ptr[0]) * qpw->chunk_max;
	dns_qpsnap_t *qps = isc_mem_allocate(qpw->mctx, bytes);

	qps->whence = reader_open(multi, qps);
	INSIST(qps->whence == multi);

	/* not a separate allocation */
	qps->base = (dns_qpbase_t *)(qps + 1);
	isc_refcount_init(&qps->base->refcount, 0);

	/*
	 * only copy base pointers of chunks we need, so we can
	 * reclaim unused memory in dns_qpsnap_destroy()
	 */
	qps->chunk_max = qpw->chunk_max;
	for (dns_qpchunk_t chunk = 0; chunk < qpw->chunk_max; chunk++) {
		if (qpw->usage[chunk].exists && chunk_usage(qpw, chunk) > 0) {
			qpw->usage[chunk].snapshot = true;
			qps->base->ptr[chunk] = qpw->base->ptr[chunk];
		} else {
			qps->base->ptr[chunk] = NULL;
		}
	}
	ISC_LIST_INITANDAPPEND(multi->snapshots, qps, link);

	*qpsp = qps;
	UNLOCK(&multi->mutex);

	rcu_read_unlock();
}

void
dns_qpsnap_destroy(dns_qpmulti_t *multi, dns_qpsnap_t **qpsp) {
	REQUIRE(QPMULTI_VALID(multi));
	REQUIRE(qpsp != NULL && *qpsp != NULL);

	LOCK(&multi->mutex);

	dns_qpsnap_t *qp = *qpsp;

	/* make sure the API is being used correctly */
	REQUIRE(qp->whence == multi);

	ISC_LIST_UNLINK(multi->snapshots, qp, link);

	/*
	 * eagerly reclaim chunks that are now unused, so that memory does
	 * not accumulate when a trie has a lot of updates and snapshots
	 */
	marksweep_chunks(multi);

	isc_mem_free(multi->writer.mctx, qp);

	*qpsp = NULL;
	UNLOCK(&multi->mutex);
}

/***********************************************************************
 *
 *  constructors, destructors
 */

void
dns_qp_create(isc_mem_t *mctx, const dns_qpmethods_t *methods, void *uctx,
	      dns_qp_t **qptp) {
	REQUIRE(qptp != NULL && *qptp == NULL);

	dns_qp_t *qp = isc_mem_get(mctx, sizeof(*qp));
	QP_INIT(qp, methods, uctx);
	isc_mem_attach(mctx, &qp->mctx);
	alloc_reset(qp);
	TRACE("");
	*qptp = qp;
}

void
dns_qpmulti_create(isc_mem_t *mctx, const dns_qpmethods_t *methods, void *uctx,
		   dns_qpmulti_t **qpmp) {
	REQUIRE(qpmp != NULL && *qpmp == NULL);

	dns_qpmulti_t *multi = isc_mem_get(mctx, sizeof(*multi));
	*multi = (dns_qpmulti_t){
		.magic = QPMULTI_MAGIC,
		.reader_ref = INVALID_REF,
	};
	isc_mutex_init(&multi->mutex);
	ISC_LIST_INIT(multi->snapshots);
	/*
	 * Do not waste effort allocating a bump chunk that will be thrown
	 * away when a transaction is opened. dns_qpmulti_update() always
	 * allocates; to ensure dns_qpmulti_write() does too, pretend the
	 * previous transaction was an update
	 */
	dns_qp_t *qp = &multi->writer;
	QP_INIT(qp, methods, uctx);
	isc_mem_attach(mctx, &qp->mctx);
	qp->transaction_mode = QP_UPDATE;
	TRACE("");
	*qpmp = multi;
}

static void
destroy_guts(dns_qp_t *qp) {
	if (qp->chunk_max == 0) {
		return;
	}
	for (dns_qpchunk_t chunk = 0; chunk < qp->chunk_max; chunk++) {
		if (qp->base->ptr[chunk] != NULL) {
			chunk_free(qp, chunk);
		}
	}
	ENSURE(qp->used_count == 0);
	ENSURE(qp->free_count == 0);
	ENSURE(isc_refcount_current(&qp->base->refcount) == 1);
	isc_mem_free(qp->mctx, qp->base);
	isc_mem_free(qp->mctx, qp->usage);
	qp->magic = 0;
}

void
dns_qp_destroy(dns_qp_t **qptp) {
	REQUIRE(qptp != NULL);
	REQUIRE(QP_VALID(*qptp));

	dns_qp_t *qp = *qptp;
	*qptp = NULL;

	/* do not try to destroy part of a dns_qpmulti_t */
	REQUIRE(qp->transaction_mode == QP_NONE);

	TRACE("");
	destroy_guts(qp);
	isc_mem_putanddetach(&qp->mctx, qp, sizeof(*qp));
}

static void
qpmulti_destroy_cb(struct rcu_head *arg) {
	qp_rcuctx_t *rcuctx = caa_container_of(arg, qp_rcuctx_t, rcu_head);
	REQUIRE(QPRCU_VALID(rcuctx));
	/* only nonzero for reclaim_chunks_cb() */
	REQUIRE(rcuctx->count == 0);

	dns_qpmulti_t *multi = rcuctx->multi;
	REQUIRE(QPMULTI_VALID(multi));

	/* reassure thread sanitizer */
	LOCK(&multi->mutex);

	dns_qp_t *qp = &multi->writer;
	REQUIRE(QP_VALID(qp));

	destroy_guts(qp);

	UNLOCK(&multi->mutex);

	isc_mutex_destroy(&multi->mutex);
	isc_mem_putanddetach(&rcuctx->mctx, rcuctx,
			     STRUCT_FLEX_SIZE(rcuctx, chunk, rcuctx->count));
	isc_mem_putanddetach(&qp->mctx, multi, sizeof(*multi));
}

void
dns_qpmulti_destroy(dns_qpmulti_t **qpmp) {
	dns_qp_t *qp = NULL;
	dns_qpmulti_t *multi = NULL;
	qp_rcuctx_t *rcuctx = NULL;

	REQUIRE(qpmp != NULL);
	REQUIRE(QPMULTI_VALID(*qpmp));

	multi = *qpmp;
	qp = &multi->writer;
	*qpmp = NULL;

	REQUIRE(QP_VALID(qp));
	REQUIRE(multi->rollback == NULL);
	REQUIRE(ISC_LIST_EMPTY(multi->snapshots));

	rcuctx = isc_mem_get(qp->mctx, STRUCT_FLEX_SIZE(rcuctx, chunk, 0));
	*rcuctx = (qp_rcuctx_t){
		.magic = QPRCU_MAGIC,
		.multi = multi,
	};
	isc_mem_attach(qp->mctx, &rcuctx->mctx);
	call_rcu(&rcuctx->rcu_head, qpmulti_destroy_cb);
}

/***********************************************************************
 *
 *  modification
 */

isc_result_t
dns_qp_insert(dns_qp_t *qp, void *pval, uint32_t ival) {
	dns_qpref_t new_ref, old_ref;
	dns_qpnode_t new_leaf, old_node;
	dns_qpnode_t *new_twigs = NULL, *old_twigs = NULL;
	dns_qpshift_t new_bit, old_bit;
	dns_qpweight_t old_size, new_size;
	dns_qpkey_t new_key, old_key;
	size_t new_keylen, old_keylen;
	size_t offset;
	uint64_t index;
	dns_qpshift_t bit;
	dns_qpweight_t pos;
	dns_qpnode_t *n = NULL;

	REQUIRE(QP_VALID(qp));

	new_leaf = make_leaf(pval, ival);
	new_keylen = leaf_qpkey(qp, &new_leaf, new_key);

	/* first leaf in an empty trie? */
	if (qp->leaf_count == 0) {
		new_ref = alloc_twigs(qp, 1);
		new_twigs = ref_ptr(qp, new_ref);
		*new_twigs = new_leaf;
		attach_leaf(qp, new_twigs);
		qp->leaf_count++;
		qp->root_ref = new_ref;
		return ISC_R_SUCCESS;
	}

	/*
	 * We need to keep searching down to a leaf even if our key is
	 * missing from this branch. It doesn't matter which twig we
	 * choose since the keys are all the same up to this node's
	 * offset. Note that if we simply use branch_twig_pos(n, bit)
	 * we may get an out-of-bounds access if our bit is greater
	 * than all the set bits in the node.
	 */
	n = ref_ptr(qp, qp->root_ref);
	while (is_branch(n)) {
		prefetch_twigs(qp, n);
		dns_qpref_t ref = branch_twigs_ref(n);
		bit = branch_keybit(n, new_key, new_keylen);
		pos = branch_has_twig(n, bit) ? branch_twig_pos(n, bit) : 0;
		n = ref_ptr(qp, ref + pos);
	}

	/* do the keys differ, and if so, where? */
	old_keylen = leaf_qpkey(qp, n, old_key);
	offset = qpkey_compare(new_key, new_keylen, old_key, old_keylen);
	if (offset == QPKEY_EQUAL) {
		return ISC_R_EXISTS;
	}
	new_bit = qpkey_bit(new_key, new_keylen, offset);
	old_bit = qpkey_bit(old_key, old_keylen, offset);

	/* find where to insert a branch or grow an existing branch. */
	n = make_root_mutable(qp);
	while (is_branch(n)) {
		prefetch_twigs(qp, n);
		if (offset < branch_key_offset(n)) {
			goto newbranch;
		}
		if (offset == branch_key_offset(n)) {
			goto growbranch;
		}
		make_twigs_mutable(qp, n);
		bit = branch_keybit(n, new_key, new_keylen);
		INSIST(branch_has_twig(n, bit));
		n = branch_twig_ptr(qp, n, bit);
	}
	/* fall through */

newbranch:
	new_ref = alloc_twigs(qp, 2);
	new_twigs = ref_ptr(qp, new_ref);

	/* save before overwriting. */
	old_node = *n;

	/* new branch node takes old node's place */
	index = BRANCH_TAG | (1ULL << new_bit) | (1ULL << old_bit) |
		((uint64_t)offset << SHIFT_OFFSET);
	*n = make_node(index, new_ref);

	/* populate twigs */
	new_twigs[old_bit > new_bit] = old_node;
	new_twigs[new_bit > old_bit] = new_leaf;

	attach_leaf(qp, &new_leaf);
	qp->leaf_count++;

	return ISC_R_SUCCESS;

growbranch:
	INSIST(!branch_has_twig(n, new_bit));

	/* locate twigs vectors */
	old_size = branch_twigs_size(n);
	new_size = old_size + 1;
	old_ref = branch_twigs_ref(n);
	new_ref = alloc_twigs(qp, new_size);
	old_twigs = ref_ptr(qp, old_ref);
	new_twigs = ref_ptr(qp, new_ref);

	/* embiggen branch node */
	index = branch_index(n) | (1ULL << new_bit);
	*n = make_node(index, new_ref);

	/* embiggen twigs vector */
	pos = branch_twig_pos(n, new_bit);
	move_twigs(new_twigs, old_twigs, pos);
	new_twigs[pos] = new_leaf;
	move_twigs(new_twigs + pos + 1, old_twigs + pos, old_size - pos);

	if (squash_twigs(qp, old_ref, old_size)) {
		/* old twigs destroyed, only attach to new leaf */
		attach_leaf(qp, &new_leaf);
	} else {
		/* old twigs duplicated, attach to all leaves */
		attach_twigs(qp, new_twigs, new_size);
	}
	qp->leaf_count++;

	return ISC_R_SUCCESS;
}

isc_result_t
dns_qp_deletekey(dns_qp_t *qp, const dns_qpkey_t search_key,
		 size_t search_keylen, void **pval_r, uint32_t *ival_r) {
	REQUIRE(QP_VALID(qp));
	REQUIRE(search_keylen < sizeof(dns_qpkey_t));

	if (get_root(qp) == NULL) {
		return ISC_R_NOTFOUND;
	}

	dns_qpshift_t bit = 0; /* suppress warning */
	dns_qpnode_t *parent = NULL;
	dns_qpnode_t *n = make_root_mutable(qp);
	while (is_branch(n)) {
		prefetch_twigs(qp, n);
		bit = branch_keybit(n, search_key, search_keylen);
		if (!branch_has_twig(n, bit)) {
			return ISC_R_NOTFOUND;
		}
		make_twigs_mutable(qp, n);
		parent = n;
		n = branch_twig_ptr(qp, n, bit);
	}

	dns_qpkey_t found_key;
	size_t found_keylen = leaf_qpkey(qp, n, found_key);
	if (qpkey_compare(search_key, search_keylen, found_key, found_keylen) !=
	    QPKEY_EQUAL)
	{
		return ISC_R_NOTFOUND;
	}

	SET_IF_NOT_NULL(pval_r, leaf_pval(n));
	SET_IF_NOT_NULL(ival_r, leaf_ival(n));
	detach_leaf(qp, n);
	qp->leaf_count--;

	/* trie becomes empty */
	if (qp->leaf_count == 0) {
		INSIST(parent == NULL);
		INSIST(n == get_root(qp));
		free_twigs(qp, qp->root_ref, 1);
		qp->root_ref = INVALID_REF;
		return ISC_R_SUCCESS;
	}

	/* step back to parent node */
	n = parent;
	parent = NULL;

	INSIST(bit != 0);
	dns_qpweight_t size = branch_twigs_size(n);
	dns_qpweight_t pos = branch_twig_pos(n, bit);
	dns_qpref_t ref = branch_twigs_ref(n);
	dns_qpnode_t *twigs = ref_ptr(qp, ref);

	if (size == 2) {
		/*
		 * move the other twig to the parent branch.
		 */
		*n = twigs[!pos];
		squash_twigs(qp, ref, 2);
	} else {
		/*
		 * shrink the twigs in place, to avoid using the bump
		 * chunk too fast - the gc will clean up after us
		 */
		*n = make_node(branch_index(n) & ~(1ULL << bit), ref);
		move_twigs(twigs + pos, twigs + pos + 1, size - pos - 1);
		squash_twigs(qp, ref + size - 1, 1);
	}

	return ISC_R_SUCCESS;
}

isc_result_t
dns_qp_deletename(dns_qp_t *qp, const dns_name_t *name, void **pval_r,
		  uint32_t *ival_r) {
	dns_qpkey_t key;
	size_t keylen = dns_qpkey_fromname(key, name);
	return dns_qp_deletekey(qp, key, keylen, pval_r, ival_r);
}

/***********************************************************************
 *  chains
 */
static void
maybe_set_name(dns_qpreader_t *qp, dns_qpnode_t *node, dns_name_t *name) {
	dns_qpkey_t key;
	size_t len;

	if (name == NULL) {
		return;
	}

	dns_name_reset(name);
	len = leaf_qpkey(qp, node, key);
	dns_qpkey_toname(key, len, name);
}

void
dns_qpchain_init(dns_qpreadable_t qpr, dns_qpchain_t *chain) {
	dns_qpreader_t *qp = dns_qpreader(qpr);
	REQUIRE(QP_VALID(qp));
	REQUIRE(chain != NULL);

	*chain = (dns_qpchain_t){
		.magic = QPCHAIN_MAGIC,
		.qp = qp,
	};
}

unsigned int
dns_qpchain_length(dns_qpchain_t *chain) {
	REQUIRE(QPCHAIN_VALID(chain));

	return chain->len;
}

void
dns_qpchain_node(dns_qpchain_t *chain, unsigned int level, dns_name_t *name,
		 void **pval_r, uint32_t *ival_r) {
	dns_qpnode_t *node = NULL;

	REQUIRE(QPCHAIN_VALID(chain));
	REQUIRE(level < chain->len);

	node = chain->chain[level].node;
	maybe_set_name(chain->qp, node, name);
	SET_IF_NOT_NULL(pval_r, leaf_pval(node));
	SET_IF_NOT_NULL(ival_r, leaf_ival(node));
}

/***********************************************************************
 *  iterators
 */

void
dns_qpiter_init(dns_qpreadable_t qpr, dns_qpiter_t *qpi) {
	dns_qpreader_t *qp = dns_qpreader(qpr);
	REQUIRE(QP_VALID(qp));
	REQUIRE(qpi != NULL);
	*qpi = (dns_qpiter_t){
		.qp = qp,
		.magic = QPITER_MAGIC,
	};
}

/*
 * are we at the last twig in this branch (in whichever direction
 * we're iterating)?
 */
static bool
last_twig(dns_qpiter_t *qpi, bool forward) {
	dns_qpweight_t pos = 0, max = 0;
	if (qpi->sp > 0) {
		dns_qpnode_t *child = qpi->stack[qpi->sp];
		dns_qpnode_t *parent = qpi->stack[qpi->sp - 1];
		pos = child - ref_ptr(qpi->qp, branch_twigs_ref(parent));
		if (forward) {
			max = branch_twigs_size(parent) - 1;
		}
	}
	return pos == max;
}

/*
 * move a QP iterator forward or back to the next or previous leaf.
 * note: this function can go wrong when the iterator refers to
 * a mutable view of the trie which is altered while iterating
 */
static isc_result_t
iterate(bool forward, dns_qpiter_t *qpi, dns_name_t *name, void **pval_r,
	uint32_t *ival_r) {
	dns_qpnode_t *node = NULL;
	bool initial_branch = true;

	REQUIRE(QPITER_VALID(qpi));

	dns_qpreader_t *qp = qpi->qp;

	REQUIRE(QP_VALID(qp));

	node = get_root(qp);
	if (node == NULL) {
		return ISC_R_NOMORE;
	}

	do {
		if (qpi->stack[qpi->sp] == NULL) {
			/* newly initialized iterator: use the root node */
			INSIST(qpi->sp == 0);
			qpi->stack[0] = node;
		} else if (!initial_branch) {
			/*
			 * in a prior loop, we reached a branch; from
			 * here we just need to get the highest or lowest
			 * leaf in the subtree; we don't need to bother
			 * stepping forward or backward through twigs
			 * anymore.
			 */
			INSIST(qpi->sp > 0);
		} else if (last_twig(qpi, forward)) {
			/*
			 * we've stepped to the end (or the beginning,
			 * if we're iterating backwards) of a set of twigs.
			 */
			if (qpi->sp == 0) {
				/*
				 * we've finished iterating. reinitialize
				 * the iterator, then return ISC_R_NOMORE.
				 */
				dns_qpiter_init(qpi->qp, qpi);
				return ISC_R_NOMORE;
			}

			/*
			 * pop the stack, and resume at the parent branch.
			 */
			qpi->stack[qpi->sp] = NULL;
			qpi->sp--;
			continue;
		} else {
			/*
			 * there are more twigs in the current branch,
			 * so step the node pointer forward (or back).
			 */
			qpi->stack[qpi->sp] += (forward ? 1 : -1);
			node = qpi->stack[qpi->sp];
		}

		/*
		 * if we're at a branch now, we loop down to the
		 * left- or rightmost leaf.
		 */
		if (is_branch(node)) {
			qpi->sp++;
			INSIST(qpi->sp < DNS_QP_MAXKEY);
			node = ref_ptr(qp, branch_twigs_ref(node)) +
			       (forward ? 0 : branch_twigs_size(node) - 1);
			qpi->stack[qpi->sp] = node;
			initial_branch = false;
		}
	} while (is_branch(node));

	/* we're at a leaf: return its data to the caller */
	SET_IF_NOT_NULL(pval_r, leaf_pval(node));
	SET_IF_NOT_NULL(ival_r, leaf_ival(node));
	maybe_set_name(qpi->qp, node, name);
	return ISC_R_SUCCESS;
}

isc_result_t
dns_qpiter_next(dns_qpiter_t *qpi, dns_name_t *name, void **pval_r,
		uint32_t *ival_r) {
	return iterate(true, qpi, name, pval_r, ival_r);
}

isc_result_t
dns_qpiter_prev(dns_qpiter_t *qpi, dns_name_t *name, void **pval_r,
		uint32_t *ival_r) {
	return iterate(false, qpi, name, pval_r, ival_r);
}

isc_result_t
dns_qpiter_current(dns_qpiter_t *qpi, dns_name_t *name, void **pval_r,
		   uint32_t *ival_r) {
	dns_qpnode_t *node = NULL;

	REQUIRE(QPITER_VALID(qpi));

	node = qpi->stack[qpi->sp];
	if (node == NULL || is_branch(node)) {
		return ISC_R_FAILURE;
	}

	SET_IF_NOT_NULL(pval_r, leaf_pval(node));
	SET_IF_NOT_NULL(ival_r, leaf_ival(node));
	maybe_set_name(qpi->qp, node, name);
	return ISC_R_SUCCESS;
}

/***********************************************************************
 *
 *  search
 */

isc_result_t
dns_qp_getkey(dns_qpreadable_t qpr, const dns_qpkey_t search_key,
	      size_t search_keylen, void **pval_r, uint32_t *ival_r) {
	dns_qpreader_t *qp = dns_qpreader(qpr);
	dns_qpkey_t found_key;
	size_t found_keylen;
	dns_qpshift_t bit;
	dns_qpnode_t *n = NULL;

	REQUIRE(QP_VALID(qp));
	REQUIRE(search_keylen < sizeof(dns_qpkey_t));

	n = get_root(qp);
	if (n == NULL) {
		return ISC_R_NOTFOUND;
	}

	while (is_branch(n)) {
		prefetch_twigs(qp, n);
		bit = branch_keybit(n, search_key, search_keylen);
		if (!branch_has_twig(n, bit)) {
			return ISC_R_NOTFOUND;
		}
		n = branch_twig_ptr(qp, n, bit);
	}

	found_keylen = leaf_qpkey(qp, n, found_key);
	if (qpkey_compare(search_key, search_keylen, found_key, found_keylen) !=
	    QPKEY_EQUAL)
	{
		return ISC_R_NOTFOUND;
	}

	SET_IF_NOT_NULL(pval_r, leaf_pval(n));
	SET_IF_NOT_NULL(ival_r, leaf_ival(n));
	return ISC_R_SUCCESS;
}

isc_result_t
dns_qp_getname(dns_qpreadable_t qpr, const dns_name_t *name, void **pval_r,
	       uint32_t *ival_r) {
	dns_qpkey_t key;
	size_t keylen = dns_qpkey_fromname(key, name);
	return dns_qp_getkey(qpr, key, keylen, pval_r, ival_r);
}

static inline void
add_link(dns_qpchain_t *chain, dns_qpnode_t *node, size_t offset) {
	/* prevent duplication */
	if (chain->len != 0 && chain->chain[chain->len - 1].node == node) {
		return;
	}
	chain->chain[chain->len].node = node;
	chain->chain[chain->len].offset = offset;
	chain->len++;
	INSIST(chain->len <= DNS_NAME_MAXLABELS);
}

static inline void
prevleaf(dns_qpiter_t *it) {
	isc_result_t result = dns_qpiter_prev(it, NULL, NULL, NULL);
	if (result == ISC_R_NOMORE) {
		result = dns_qpiter_prev(it, NULL, NULL, NULL);
	}
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
}

static inline void
greatest_leaf(dns_qpreadable_t qpr, dns_qpnode_t *n, dns_qpiter_t *iter) {
	while (is_branch(n)) {
		dns_qpref_t ref = branch_twigs_ref(n) + branch_twigs_size(n) -
				  1;
		iter->stack[++iter->sp] = n;
		n = ref_ptr(qpr, ref);
	}
	iter->stack[++iter->sp] = n;
}

static inline dns_qpnode_t *
anyleaf(dns_qpreader_t *qp, dns_qpnode_t *n) {
	while (is_branch(n)) {
		n = branch_twigs(qp, n);
	}
	return n;
}

static inline int
twig_offset(dns_qpnode_t *n, dns_qpshift_t sbit, dns_qpshift_t kbit,
	    dns_qpshift_t fbit) {
	dns_qpweight_t pos = branch_twig_pos(n, sbit);
	if (branch_has_twig(n, sbit)) {
		return pos - (kbit < fbit);
	}
	return pos - 1;
}

/*
 * If dns_qp_lookup() was passed an iterator, we want it to point at the
 * matching name in the case of an exact match, or at the predecessor name
 * for a non-exact match.
 *
 * If there is an exact match, then there is nothing to be done. Otherwise,
 * we pop up the iterator stack until we find a parent branch with an offset
 * that is before the position where the search key differs from the found key.
 * From there we can step to the leaf that is the predecessor of the searched
 * name.
 *
 * Requires the iterator to be pointing at a leaf node.
 */
static void
fix_iterator(dns_qpreader_t *qp, dns_qpiter_t *it, dns_qpkey_t key,
	     size_t len) {
	dns_qpnode_t *n = it->stack[it->sp];

	REQUIRE(!is_branch(n));

	dns_qpkey_t found;
	size_t foundlen = leaf_qpkey(qp, n, found);
	size_t to = qpkey_compare(key, len, found, foundlen);

	/* If the keys are equal, the iterator is already at the right node. */
	if (to == QPKEY_EQUAL) {
		return;
	}

	/*
	 * Special case: if the key differs even before the root
	 * key offset, it means the name desired either precedes or
	 * follows the entire range of names in the database, and
	 * popping up the stack won't help us, so just move the
	 * iterator one step back from the origin and return.
	 */
	if (to < branch_key_offset(it->stack[0])) {
		dns_qpiter_init(qp, it);
		prevleaf(it);
		return;
	}

	/*
	 * As long as the branch offset point is after the point where the
	 * key differs, we need to branch up and find a better node.
	 */
	while (it->sp > 0) {
		dns_qpnode_t *b = it->stack[it->sp - 1];
		if (branch_key_offset(b) < to) {
			break;
		}
		it->sp--;
	}
	n = it->stack[it->sp];

	/*
	 * Either we are now at the correct branch, or we are at the
	 * first unmatched node. Determine the bit position for the
	 * twig we need (sbit).
	 */
	dns_qpshift_t kbit = qpkey_bit(key, len, to);
	dns_qpshift_t fbit = qpkey_bit(found, foundlen, to);
	dns_qpshift_t sbit = 0;

	if (is_branch(n) && branch_key_offset(n) == to) {
		/* We are on the correct branch now. */
		sbit = kbit;
	} else if (it->sp == 0) {
		/*
		 * We are on the root branch, popping up the stack won't
		 * help us, so just move the iterator one step back from the
		 * origin and return.
		 */
		dns_qpiter_init(qp, it);
		prevleaf(it);
		return;
	} else {
		/* We are at the first unmatched node, pop up the stack. */
		n = it->stack[--it->sp];
		sbit = qpkey_bit(key, len, branch_key_offset(n));
	}

	INSIST(is_branch(n));

	prefetch_twigs(qp, n);
	dns_qpnode_t *twigs = branch_twigs(qp, n);
	int toff = twig_offset(n, sbit, kbit, fbit);
	if (toff >= 0) {
		/*
		 * The name we want would've been after some twig in
		 * this branch. Walk down from that twig to the
		 * highest leaf in its subtree to get the predecessor.
		 */
		greatest_leaf(qp, twigs + toff, it);
	} else {
		/*
		 * Every leaf below this node is greater than the one we
		 * wanted, so the previous leaf is the predecessor.
		 */
		prevleaf(it);
	}
}

/*
 * When searching for a requested name in dns_qp_lookup(), we might add
 * a leaf node to the chain, then subsequently determine that it was a
 * dead end. When this happens, the chain can be left holding a node
 * that is *not* an ancestor of the requested name. We correct for that
 * here.
 */
static void
fix_chain(dns_qpchain_t *chain, size_t offset) {
	while (chain->len > 0 && chain->chain[chain->len - 1].offset >= offset)
	{
		chain->len--;
		chain->chain[chain->len].node = NULL;
		chain->chain[chain->len].offset = 0;
	}
}

isc_result_t
dns_qp_lookup(dns_qpreadable_t qpr, const dns_name_t *name,
	      dns_name_t *foundname, dns_qpiter_t *iter, dns_qpchain_t *chain,
	      void **pval_r, uint32_t *ival_r) {
	dns_qpreader_t *qp = dns_qpreader(qpr);
	dns_qpkey_t search, found;
	size_t searchlen, foundlen;
	size_t offset = 0;
	dns_qpnode_t *n = NULL;
	dns_qpshift_t bit = SHIFT_NOBYTE;
	dns_qpchain_t oc;
	dns_qpiter_t it;
	bool matched = false;
	bool setiter = true;

	REQUIRE(QP_VALID(qp));
	REQUIRE(foundname == NULL || ISC_MAGIC_VALID(name, DNS_NAME_MAGIC));

	searchlen = dns_qpkey_fromname(search, name);

	if (chain == NULL) {
		chain = &oc;
	}
	if (iter == NULL) {
		iter = &it;
		setiter = false;
	}
	dns_qpchain_init(qp, chain);
	dns_qpiter_init(qp, iter);

	n = get_root(qp);
	if (n == NULL) {
		return ISC_R_NOTFOUND;
	}
	iter->stack[0] = n;

	/*
	 * Like `dns_qp_insert()`, we must find a leaf. However, we don't make a
	 * second pass: instead, we keep track of any leaves with shorter keys
	 * that we discover along the way. (In general, qp-trie searches can be
	 * one-pass, by recording their traversal, or two-pass, for less stack
	 * memory usage.)
	 */
	while (is_branch(n)) {
		prefetch_twigs(qp, n);

		offset = branch_key_offset(n);
		bit = qpkey_bit(search, searchlen, offset);
		dns_qpnode_t *twigs = branch_twigs(qp, n);

		/*
		 * A shorter key that can be a parent domain always has a
		 * leaf node at SHIFT_NOBYTE (indicating end of its key)
		 * where our search key has a normal character immediately
		 * after a label separator.
		 *
		 * Note 1: It is OK if `off - 1` underflows: it will
		 * become SIZE_MAX, which is greater than `searchlen`, so
		 * `qpkey_bit()` will return SHIFT_NOBYTE, which is what we
		 * want when `off == 0`.
		 *
		 * Note 2: If SHIFT_NOBYTE twig is present, it will always
		 * be in position 0, the first location in 'twigs'.
		 */
		if (bit != SHIFT_NOBYTE && branch_has_twig(n, SHIFT_NOBYTE) &&
		    qpkey_bit(search, searchlen, offset - 1) == SHIFT_NOBYTE &&
		    !is_branch(twigs))
		{
			add_link(chain, twigs, offset);
		}

		matched = branch_has_twig(n, bit);
		if (matched) {
			/*
			 * found a match: if it's a branch, we keep
			 * searching, and if it's a leaf, we drop out of
			 * the loop.
			 */
			n = branch_twig_ptr(qp, n, bit);
		} else {
			/*
			 * this branch is a dead end, and the predecessor
			 * doesn't matter. now we just need to find a leaf
			 * to end on so that qpkey_leaf() will work below.
			 */
			n = anyleaf(qp, twigs);
		}

		iter->stack[++iter->sp] = n;
	}

	if (setiter) {
		/*
		 * we found a leaf, but it might not be the leaf we wanted.
		 * if it isn't, and if the caller passed us an iterator,
		 * then we might need to reposition it.
		 */
		fix_iterator(qp, iter, search, searchlen);
		n = iter->stack[iter->sp];
	}

	/* at this point, n can only be a leaf node */
	INSIST(!is_branch(n));

	foundlen = leaf_qpkey(qp, n, found);
	offset = qpkey_compare(search, searchlen, found, foundlen);

	/* the search ended with an exact or partial match */
	if (offset == QPKEY_EQUAL || offset == foundlen) {
		isc_result_t result = ISC_R_SUCCESS;

		if (offset == foundlen) {
			fix_chain(chain, offset);
			result = DNS_R_PARTIALMATCH;
		}
		add_link(chain, n, offset);

		SET_IF_NOT_NULL(pval_r, leaf_pval(n));
		SET_IF_NOT_NULL(ival_r, leaf_ival(n));
		maybe_set_name(qp, n, foundname);
		return result;
	}

	/*
	 * the requested name was not found, but if an ancestor
	 * was, we can retrieve that from the chain.
	 */
	int len = chain->len;
	while (len-- > 0) {
		if (offset >= chain->chain[len].offset) {
			n = chain->chain[len].node;
			SET_IF_NOT_NULL(pval_r, leaf_pval(n));
			SET_IF_NOT_NULL(ival_r, leaf_ival(n));
			maybe_set_name(qp, n, foundname);
			return DNS_R_PARTIALMATCH;
		} else {
			/*
			 * oops, during the search we found and added
			 * a leaf that's longer than the requested
			 * name; remove it from the chain.
			 */
			chain->len--;
		}
	}

	/* nothing was found at all */
	return ISC_R_NOTFOUND;
}

/**********************************************************************/
