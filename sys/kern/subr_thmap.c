/*	$NetBSD: subr_thmap.c,v 1.11 2022/04/01 00:16:40 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 Mindaugas Rasiukevicius <rmind at noxt eu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Upstream: https://github.com/rmind/thmap/
 */

/*
 * Concurrent trie-hash map.
 *
 * The data structure is conceptually a radix trie on hashed keys.
 * Keys are hashed using a 32-bit function.  The root level is a special
 * case: it is managed using the compare-and-swap (CAS) atomic operation
 * and has a fanout of 64.  The subsequent levels are constructed using
 * intermediate nodes with a fanout of 16 (using 4 bits).  As more levels
 * are created, more blocks of the 32-bit hash value might be generated
 * by incrementing the seed parameter of the hash function.
 *
 * Concurrency
 *
 * - READERS: Descending is simply walking through the slot values of
 *   the intermediate nodes.  It is lock-free as there is no intermediate
 *   state: the slot is either empty or has a pointer to the child node.
 *   The main assumptions here are the following:
 *
 *   i) modifications must preserve consistency with the respect to the
 *   readers i.e. the readers can only see the valid node values;
 *
 *   ii) any invalid view must "fail" the reads, e.g. by making them
 *   re-try from the root; this is a case for deletions and is achieved
 *   using the NODE_DELETED flag.
 *
 *   iii) the node destruction must be synchronized with the readers,
 *   e.g. by using the Epoch-based reclamation or other techniques.
 *
 * - WRITERS AND LOCKING: Each intermediate node has a spin-lock (which
 *   is implemented using the NODE_LOCKED bit) -- it provides mutual
 *   exclusion amongst concurrent writers.  The lock order for the nodes
 *   is "bottom-up" i.e. they are locked as we ascend the trie.  A key
 *   constraint here is that parent pointer never changes.
 *
 * - DELETES: In addition to writer's locking, the deletion keeps the
 *   intermediate nodes in a valid state and sets the NODE_DELETED flag,
 *   to indicate that the readers must re-start the walk from the root.
 *   As the levels are collapsed, NODE_DELETED gets propagated up-tree.
 *   The leaf nodes just stay as-is until they are reclaimed.
 *
 * - ROOT LEVEL: The root level is a special case, as it is implemented
 *   as an array (rather than intermediate node).  The root-level slot can
 *   only be set using CAS and it can only be set to a valid intermediate
 *   node.  The root-level slot can only be cleared when the node it points
 *   at becomes empty, is locked and marked as NODE_DELETED (this causes
 *   the insert/delete operations to re-try until the slot is set to NULL).
 *
 * References:
 *
 *	W. Litwin, 1981, Trie Hashing.
 *	Proceedings of the 1981 ACM SIGMOD, p. 19-29
 *	https://dl.acm.org/citation.cfm?id=582322
 *
 *	P. L. Lehman and S. B. Yao.
 *	Efficient locking for concurrent operations on B-trees.
 *	ACM TODS, 6(4):650-670, 1981
 *	https://www.csd.uoc.gr/~hy460/pdf/p650-lehman.pdf
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/thmap.h>
#include <sys/kmem.h>
#include <sys/lock.h>
#include <sys/atomic.h>
#include <sys/hash.h>
#include <sys/cprng.h>
#define THMAP_RCSID(a) __KERNEL_RCSID(0, a)
#else
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>
#define THMAP_RCSID(a) __RCSID(a)

#include "thmap.h"
#include "utils.h"
#endif

THMAP_RCSID("$NetBSD: subr_thmap.c,v 1.11 2022/04/01 00:16:40 riastradh Exp $");

#include <crypto/blake2/blake2s.h>

/*
 * NetBSD kernel wrappers
 */
#ifdef _KERNEL
#define	ASSERT KASSERT
#define	atomic_thread_fence(x) membar_exit() /* only used for release order */
#define	atomic_compare_exchange_weak_explicit_32(p, e, n, m1, m2) \
    (atomic_cas_32((p), *(e), (n)) == *(e))
#define	atomic_compare_exchange_weak_explicit_ptr(p, e, n, m1, m2) \
    (atomic_cas_ptr((p), *(void **)(e), (void *)(n)) == *(void **)(e))
#define	atomic_exchange_explicit(o, n, m1) atomic_swap_ptr((o), (n))
#define	murmurhash3 murmurhash2
#endif

/*
 * The root level fanout is 64 (indexed by the last 6 bits of the hash
 * value XORed with the length).  Each subsequent level, represented by
 * intermediate nodes, has a fanout of 16 (using 4 bits).
 *
 * The hash function produces 32-bit values.
 */

#define	HASHVAL_SEEDLEN	(16)
#define	HASHVAL_BITS	(32)
#define	HASHVAL_MOD	(HASHVAL_BITS - 1)
#define	HASHVAL_SHIFT	(5)

#define	ROOT_BITS	(6)
#define	ROOT_SIZE	(1 << ROOT_BITS)
#define	ROOT_MASK	(ROOT_SIZE - 1)
#define	ROOT_MSBITS	(HASHVAL_BITS - ROOT_BITS)

#define	LEVEL_BITS	(4)
#define	LEVEL_SIZE	(1 << LEVEL_BITS)
#define	LEVEL_MASK	(LEVEL_SIZE - 1)

/*
 * Instead of raw pointers, we use offsets from the base address.
 * This accommodates the use of this data structure in shared memory,
 * where mappings can be in different address spaces.
 *
 * The pointers must be aligned, since pointer tagging is used to
 * differentiate the intermediate nodes from leaves.  We reserve the
 * least significant bit.
 */
typedef uintptr_t thmap_ptr_t;
typedef uintptr_t atomic_thmap_ptr_t;			// C11 _Atomic

#define	THMAP_NULL		((thmap_ptr_t)0)

#define	THMAP_LEAF_BIT		(0x1)

#define	THMAP_ALIGNED_P(p)	(((uintptr_t)(p) & 3) == 0)
#define	THMAP_ALIGN(p)		((uintptr_t)(p) & ~(uintptr_t)3)
#define	THMAP_INODE_P(p)	(((uintptr_t)(p) & THMAP_LEAF_BIT) == 0)

#define	THMAP_GETPTR(th, p)	((void *)((th)->baseptr + (uintptr_t)(p)))
#define	THMAP_GETOFF(th, p)	((thmap_ptr_t)((uintptr_t)(p) - (th)->baseptr))
#define	THMAP_NODE(th, p)	THMAP_GETPTR(th, THMAP_ALIGN(p))

/*
 * State field.
 */

#define	NODE_LOCKED		(1U << 31)		// lock (writers)
#define	NODE_DELETED		(1U << 30)		// node deleted
#define	NODE_COUNT(s)		((s) & 0x3fffffff)	// slot count mask

/*
 * There are two types of nodes:
 * - Intermediate nodes -- arrays pointing to another level or a leaf;
 * - Leaves, which store a key-value pair.
 */

typedef struct {
	uint32_t		state;			// C11 _Atomic
	thmap_ptr_t		parent;
	atomic_thmap_ptr_t	slots[LEVEL_SIZE];
} thmap_inode_t;

#define	THMAP_INODE_LEN	sizeof(thmap_inode_t)

typedef struct {
	thmap_ptr_t	key;
	size_t		len;
	void *		val;
} thmap_leaf_t;

typedef struct {
	const uint8_t *	seed;		// secret seed
	unsigned	rslot;		// root-level slot index
	unsigned	level;		// current level in the tree
	unsigned	hashidx;	// current hash index (block of bits)
	uint32_t	hashval;	// current hash value
} thmap_query_t;

typedef struct {
	uintptr_t	addr;
	size_t		len;
	void *		next;
} thmap_gc_t;

#define	THMAP_ROOT_LEN	(sizeof(thmap_ptr_t) * ROOT_SIZE)

struct thmap {
	uintptr_t		baseptr;
	atomic_thmap_ptr_t *	root;
	unsigned		flags;
	const thmap_ops_t *	ops;
	thmap_gc_t *		gc_list;		// C11 _Atomic
	uint8_t			seed[HASHVAL_SEEDLEN];
};

static void	stage_mem_gc(thmap_t *, uintptr_t, size_t);

/*
 * A few low-level helper routines.
 */

static uintptr_t
alloc_wrapper(size_t len)
{
	return (uintptr_t)kmem_intr_alloc(len, KM_NOSLEEP);
}

static void
free_wrapper(uintptr_t addr, size_t len)
{
	kmem_intr_free((void *)addr, len);
}

static const thmap_ops_t thmap_default_ops = {
	.alloc = alloc_wrapper,
	.free = free_wrapper
};

/*
 * NODE LOCKING.
 */

static inline bool __diagused
node_locked_p(thmap_inode_t *node)
{
	return (atomic_load_relaxed(&node->state) & NODE_LOCKED) != 0;
}

static void
lock_node(thmap_inode_t *node)
{
	unsigned bcount = SPINLOCK_BACKOFF_MIN;
	uint32_t s;
again:
	s = atomic_load_relaxed(&node->state);
	if (s & NODE_LOCKED) {
		SPINLOCK_BACKOFF(bcount);
		goto again;
	}
	/* Acquire from prior release in unlock_node.() */
	if (!atomic_compare_exchange_weak_explicit_32(&node->state,
	    &s, s | NODE_LOCKED, memory_order_acquire, memory_order_relaxed)) {
		bcount = SPINLOCK_BACKOFF_MIN;
		goto again;
	}
}

static void
unlock_node(thmap_inode_t *node)
{
	uint32_t s = atomic_load_relaxed(&node->state) & ~NODE_LOCKED;

	ASSERT(node_locked_p(node));
	/* Release to subsequent acquire in lock_node(). */
	atomic_store_release(&node->state, s);
}

/*
 * HASH VALUE AND KEY OPERATIONS.
 */

static inline uint32_t
hash(const uint8_t seed[static HASHVAL_SEEDLEN], const void *key, size_t len,
    uint32_t level)
{
	struct blake2s B;
	uint32_t h;

	if (level == 0)
		return murmurhash3(key, len, 0);

	/*
	 * Byte order is not significant here because this is
	 * intentionally secret and independent for each thmap.
	 *
	 * XXX We get 32 bytes of output at a time; we could march
	 * through them sequentially rather than throwing away 28 bytes
	 * and recomputing BLAKE2 each time.  But the number of
	 * iterations ought to be geometric in the collision
	 * probability at each level which should be very small anyway.
	 */
	blake2s_init(&B, sizeof h, seed, HASHVAL_SEEDLEN);
	blake2s_update(&B, &level, sizeof level);
	blake2s_update(&B, key, len);
	blake2s_final(&B, &h);

	return h;
}

static inline void
hashval_init(thmap_query_t *query, const uint8_t seed[static HASHVAL_SEEDLEN],
    const void * restrict key, size_t len)
{
	const uint32_t hashval = hash(seed, key, len, 0);

	query->seed = seed;
	query->rslot = ((hashval >> ROOT_MSBITS) ^ len) & ROOT_MASK;
	query->level = 0;
	query->hashval = hashval;
	query->hashidx = 0;
}

/*
 * hashval_getslot: given the key, compute the hash (if not already cached)
 * and return the offset for the current level.
 */
static unsigned
hashval_getslot(thmap_query_t *query, const void * restrict key, size_t len)
{
	const unsigned offset = query->level * LEVEL_BITS;
	const unsigned shift = offset & HASHVAL_MOD;
	const unsigned i = offset >> HASHVAL_SHIFT;

	if (query->hashidx != i) {
		/* Generate a hash value for a required range. */
		query->hashval = hash(query->seed, key, len, i);
		query->hashidx = i;
	}
	return (query->hashval >> shift) & LEVEL_MASK;
}

static unsigned
hashval_getleafslot(const thmap_t *thmap,
    const thmap_leaf_t *leaf, unsigned level)
{
	const void *key = THMAP_GETPTR(thmap, leaf->key);
	const unsigned offset = level * LEVEL_BITS;
	const unsigned shift = offset & HASHVAL_MOD;
	const unsigned i = offset >> HASHVAL_SHIFT;

	return (hash(thmap->seed, key, leaf->len, i) >> shift) & LEVEL_MASK;
}

static inline unsigned
hashval_getl0slot(const thmap_t *thmap, const thmap_query_t *query,
    const thmap_leaf_t *leaf)
{
	if (__predict_true(query->hashidx == 0)) {
		return query->hashval & LEVEL_MASK;
	}
	return hashval_getleafslot(thmap, leaf, 0);
}

static bool
key_cmp_p(const thmap_t *thmap, const thmap_leaf_t *leaf,
    const void * restrict key, size_t len)
{
	const void *leafkey = THMAP_GETPTR(thmap, leaf->key);
	return len == leaf->len && memcmp(key, leafkey, len) == 0;
}

/*
 * INTER-NODE OPERATIONS.
 */

static thmap_inode_t *
node_create(thmap_t *thmap, thmap_inode_t *parent)
{
	thmap_inode_t *node;
	uintptr_t p;

	p = thmap->ops->alloc(THMAP_INODE_LEN);
	if (!p) {
		return NULL;
	}
	node = THMAP_GETPTR(thmap, p);
	ASSERT(THMAP_ALIGNED_P(node));

	memset(node, 0, THMAP_INODE_LEN);
	if (parent) {
		/* Not yet published, no need for ordering. */
		atomic_store_relaxed(&node->state, NODE_LOCKED);
		node->parent = THMAP_GETOFF(thmap, parent);
	}
	return node;
}

static void
node_insert(thmap_inode_t *node, unsigned slot, thmap_ptr_t child)
{
	ASSERT(node_locked_p(node) || node->parent == THMAP_NULL);
	ASSERT((atomic_load_relaxed(&node->state) & NODE_DELETED) == 0);
	ASSERT(atomic_load_relaxed(&node->slots[slot]) == THMAP_NULL);

	ASSERT(NODE_COUNT(atomic_load_relaxed(&node->state)) < LEVEL_SIZE);

	/*
	 * If node is public already, caller is responsible for issuing
	 * release fence; if node is not public, no ordering is needed.
	 * Hence relaxed ordering.
	 */
	atomic_store_relaxed(&node->slots[slot], child);
	atomic_store_relaxed(&node->state,
	    atomic_load_relaxed(&node->state) + 1);
}

static void
node_remove(thmap_inode_t *node, unsigned slot)
{
	ASSERT(node_locked_p(node));
	ASSERT((atomic_load_relaxed(&node->state) & NODE_DELETED) == 0);
	ASSERT(atomic_load_relaxed(&node->slots[slot]) != THMAP_NULL);

	ASSERT(NODE_COUNT(atomic_load_relaxed(&node->state)) > 0);
	ASSERT(NODE_COUNT(atomic_load_relaxed(&node->state)) <= LEVEL_SIZE);

	/* Element will be GC-ed later; no need for ordering here. */
	atomic_store_relaxed(&node->slots[slot], THMAP_NULL);
	atomic_store_relaxed(&node->state,
	    atomic_load_relaxed(&node->state) - 1);
}

/*
 * LEAF OPERATIONS.
 */

static thmap_leaf_t *
leaf_create(const thmap_t *thmap, const void *key, size_t len, void *val)
{
	thmap_leaf_t *leaf;
	uintptr_t leaf_off, key_off;

	leaf_off = thmap->ops->alloc(sizeof(thmap_leaf_t));
	if (!leaf_off) {
		return NULL;
	}
	leaf = THMAP_GETPTR(thmap, leaf_off);
	ASSERT(THMAP_ALIGNED_P(leaf));

	if ((thmap->flags & THMAP_NOCOPY) == 0) {
		/*
		 * Copy the key.
		 */
		key_off = thmap->ops->alloc(len);
		if (!key_off) {
			thmap->ops->free(leaf_off, sizeof(thmap_leaf_t));
			return NULL;
		}
		memcpy(THMAP_GETPTR(thmap, key_off), key, len);
		leaf->key = key_off;
	} else {
		/* Otherwise, we use a reference. */
		leaf->key = (uintptr_t)key;
	}
	leaf->len = len;
	leaf->val = val;
	return leaf;
}

static void
leaf_free(const thmap_t *thmap, thmap_leaf_t *leaf)
{
	if ((thmap->flags & THMAP_NOCOPY) == 0) {
		thmap->ops->free(leaf->key, leaf->len);
	}
	thmap->ops->free(THMAP_GETOFF(thmap, leaf), sizeof(thmap_leaf_t));
}

static thmap_leaf_t *
get_leaf(const thmap_t *thmap, thmap_inode_t *parent, unsigned slot)
{
	thmap_ptr_t node;

	/* Consume from prior release in thmap_put(). */
	node = atomic_load_consume(&parent->slots[slot]);
	if (THMAP_INODE_P(node)) {
		return NULL;
	}
	return THMAP_NODE(thmap, node);
}

/*
 * ROOT OPERATIONS.
 */

/*
 * root_try_put: Try to set a root pointer at query->rslot.
 *
 * => Implies release operation on success.
 * => Implies no ordering on failure.
 */
static inline int
root_try_put(thmap_t *thmap, const thmap_query_t *query, thmap_leaf_t *leaf)
{
	thmap_ptr_t expected;
	const unsigned i = query->rslot;
	thmap_inode_t *node;
	thmap_ptr_t nptr;
	unsigned slot;

	/*
	 * Must pre-check first.  No ordering required because we will
	 * check again before taking any actions, and start over if
	 * this changes from null.
	 */
	if (atomic_load_relaxed(&thmap->root[i])) {
		return EEXIST;
	}

	/*
	 * Create an intermediate node.  Since there is no parent set,
	 * it will be created unlocked and the CAS operation will
	 * release it to readers.
	 */
	node = node_create(thmap, NULL);
	if (__predict_false(node == NULL)) {
		return ENOMEM;
	}
	slot = hashval_getl0slot(thmap, query, leaf);
	node_insert(node, slot, THMAP_GETOFF(thmap, leaf) | THMAP_LEAF_BIT);
	nptr = THMAP_GETOFF(thmap, node);
again:
	if (atomic_load_relaxed(&thmap->root[i])) {
		thmap->ops->free(nptr, THMAP_INODE_LEN);
		return EEXIST;
	}
	/* Release to subsequent consume in find_edge_node(). */
	expected = THMAP_NULL;
	if (!atomic_compare_exchange_weak_explicit_ptr(&thmap->root[i], &expected,
	    nptr, memory_order_release, memory_order_relaxed)) {
		goto again;
	}
	return 0;
}

/*
 * find_edge_node: given the hash, traverse the tree to find the edge node.
 *
 * => Returns an aligned (clean) pointer to the parent node.
 * => Returns the slot number and sets current level.
 */
static thmap_inode_t *
find_edge_node(const thmap_t *thmap, thmap_query_t *query,
    const void * restrict key, size_t len, unsigned *slot)
{
	thmap_ptr_t root_slot;
	thmap_inode_t *parent;
	thmap_ptr_t node;
	unsigned off;

	ASSERT(query->level == 0);

	/* Consume from prior release in root_try_put(). */
	root_slot = atomic_load_consume(&thmap->root[query->rslot]);
	parent = THMAP_NODE(thmap, root_slot);
	if (!parent) {
		return NULL;
	}
descend:
	off = hashval_getslot(query, key, len);
	/* Consume from prior release in thmap_put(). */
	node = atomic_load_consume(&parent->slots[off]);

	/* Descend the tree until we find a leaf or empty slot. */
	if (node && THMAP_INODE_P(node)) {
		parent = THMAP_NODE(thmap, node);
		query->level++;
		goto descend;
	}
	/*
	 * NODE_DELETED does not become stale until GC runs, which
	 * cannot happen while we are in the middle of an operation,
	 * hence relaxed ordering.
	 */
	if (atomic_load_relaxed(&parent->state) & NODE_DELETED) {
		return NULL;
	}
	*slot = off;
	return parent;
}

/*
 * find_edge_node_locked: traverse the tree, like find_edge_node(),
 * but attempt to lock the edge node.
 *
 * => Returns NULL if the deleted node is found.  This indicates that
 *    the caller must re-try from the root, as the root slot might have
 *    changed too.
 */
static thmap_inode_t *
find_edge_node_locked(const thmap_t *thmap, thmap_query_t *query,
    const void * restrict key, size_t len, unsigned *slot)
{
	thmap_inode_t *node;
	thmap_ptr_t target;
retry:
	/*
	 * Find the edge node and lock it!  Re-check the state since
	 * the tree might change by the time we acquire the lock.
	 */
	node = find_edge_node(thmap, query, key, len, slot);
	if (!node) {
		/* The root slot is empty -- let the caller decide. */
		query->level = 0;
		return NULL;
	}
	lock_node(node);
	if (__predict_false(atomic_load_relaxed(&node->state) & NODE_DELETED)) {
		/*
		 * The node has been deleted.  The tree might have a new
		 * shape now, therefore we must re-start from the root.
		 */
		unlock_node(node);
		query->level = 0;
		return NULL;
	}
	target = atomic_load_relaxed(&node->slots[*slot]);
	if (__predict_false(target && THMAP_INODE_P(target))) {
		/*
		 * The target slot has been changed and it is now an
		 * intermediate node.  Re-start from the top internode.
		 */
		unlock_node(node);
		query->level = 0;
		goto retry;
	}
	return node;
}

/*
 * thmap_get: lookup a value given the key.
 */
void *
thmap_get(thmap_t *thmap, const void *key, size_t len)
{
	thmap_query_t query;
	thmap_inode_t *parent;
	thmap_leaf_t *leaf;
	unsigned slot;

	hashval_init(&query, thmap->seed, key, len);
	parent = find_edge_node(thmap, &query, key, len, &slot);
	if (!parent) {
		return NULL;
	}
	leaf = get_leaf(thmap, parent, slot);
	if (!leaf) {
		return NULL;
	}
	if (!key_cmp_p(thmap, leaf, key, len)) {
		return NULL;
	}
	return leaf->val;
}

/*
 * thmap_put: insert a value given the key.
 *
 * => If the key is already present, return the associated value.
 * => Otherwise, on successful insert, return the given value.
 */
void *
thmap_put(thmap_t *thmap, const void *key, size_t len, void *val)
{
	thmap_query_t query;
	thmap_leaf_t *leaf, *other;
	thmap_inode_t *parent, *child;
	unsigned slot, other_slot;
	thmap_ptr_t target;

	/*
	 * First, pre-allocate and initialize the leaf node.
	 */
	leaf = leaf_create(thmap, key, len, val);
	if (__predict_false(!leaf)) {
		return NULL;
	}
	hashval_init(&query, thmap->seed, key, len);
retry:
	/*
	 * Try to insert into the root first, if its slot is empty.
	 */
	switch (root_try_put(thmap, &query, leaf)) {
	case 0:
		/* Success: the leaf was inserted; no locking involved. */
		return val;
	case EEXIST:
		break;
	case ENOMEM:
		return NULL;
	default:
		__unreachable();
	}

	/*
	 * Release node via store in node_insert (*) to subsequent
	 * consume in get_leaf() or find_edge_node().
	 */
	atomic_thread_fence(memory_order_release);

	/*
	 * Find the edge node and the target slot.
	 */
	parent = find_edge_node_locked(thmap, &query, key, len, &slot);
	if (!parent) {
		goto retry;
	}
	target = atomic_load_relaxed(&parent->slots[slot]); // tagged offset
	if (THMAP_INODE_P(target)) {
		/*
		 * Empty slot: simply insert the new leaf.  The release
		 * fence is already issued for us.
		 */
		target = THMAP_GETOFF(thmap, leaf) | THMAP_LEAF_BIT;
		node_insert(parent, slot, target); /* (*) */
		goto out;
	}

	/*
	 * Collision or duplicate.
	 */
	other = THMAP_NODE(thmap, target);
	if (key_cmp_p(thmap, other, key, len)) {
		/*
		 * Duplicate.  Free the pre-allocated leaf and
		 * return the present value.
		 */
		leaf_free(thmap, leaf);
		val = other->val;
		goto out;
	}
descend:
	/*
	 * Collision -- expand the tree.  Create an intermediate node
	 * which will be locked (NODE_LOCKED) for us.  At this point,
	 * we advance to the next level.
	 */
	child = node_create(thmap, parent);
	if (__predict_false(!child)) {
		leaf_free(thmap, leaf);
		val = NULL;
		goto out;
	}
	query.level++;

	/*
	 * Insert the other (colliding) leaf first.  The new child is
	 * not yet published, so memory order is relaxed.
	 */
	other_slot = hashval_getleafslot(thmap, other, query.level);
	target = THMAP_GETOFF(thmap, other) | THMAP_LEAF_BIT;
	node_insert(child, other_slot, target);

	/*
	 * Insert the intermediate node into the parent node.
	 * It becomes the new parent for the our new leaf.
	 *
	 * Ensure that stores to the child (and leaf) reach global
	 * visibility before it gets inserted to the parent, as
	 * consumed by get_leaf() or find_edge_node().
	 */
	atomic_store_release(&parent->slots[slot], THMAP_GETOFF(thmap, child));

	unlock_node(parent);
	ASSERT(node_locked_p(child));
	parent = child;

	/*
	 * Get the new slot and check for another collision
	 * at the next level.
	 */
	slot = hashval_getslot(&query, key, len);
	if (slot == other_slot) {
		/* Another collision -- descend and expand again. */
		goto descend;
	}

	/*
	 * Insert our new leaf once we expanded enough.  The release
	 * fence is already issued for us.
	 */
	target = THMAP_GETOFF(thmap, leaf) | THMAP_LEAF_BIT;
	node_insert(parent, slot, target); /* (*) */
out:
	unlock_node(parent);
	return val;
}

/*
 * thmap_del: remove the entry given the key.
 */
void *
thmap_del(thmap_t *thmap, const void *key, size_t len)
{
	thmap_query_t query;
	thmap_leaf_t *leaf;
	thmap_inode_t *parent;
	unsigned slot;
	void *val;

	hashval_init(&query, thmap->seed, key, len);
	parent = find_edge_node_locked(thmap, &query, key, len, &slot);
	if (!parent) {
		/* Root slot empty: not found. */
		return NULL;
	}
	leaf = get_leaf(thmap, parent, slot);
	if (!leaf || !key_cmp_p(thmap, leaf, key, len)) {
		/* Not found. */
		unlock_node(parent);
		return NULL;
	}

	/* Remove the leaf. */
	ASSERT(THMAP_NODE(thmap, atomic_load_relaxed(&parent->slots[slot]))
	    == leaf);
	node_remove(parent, slot);

	/*
	 * Collapse the levels if removing the last item.
	 */
	while (query.level &&
	    NODE_COUNT(atomic_load_relaxed(&parent->state)) == 0) {
		thmap_inode_t *node = parent;

		ASSERT(atomic_load_relaxed(&node->state) == NODE_LOCKED);

		/*
		 * Ascend one level up.
		 * => Mark our current parent as deleted.
		 * => Lock the parent one level up.
		 */
		query.level--;
		slot = hashval_getslot(&query, key, len);
		parent = THMAP_NODE(thmap, node->parent);
		ASSERT(parent != NULL);

		lock_node(parent);
		ASSERT((atomic_load_relaxed(&parent->state) & NODE_DELETED)
		    == 0);

		/*
		 * Lock is exclusive, so nobody else can be writing at
		 * the same time, and no need for atomic R/M/W, but
		 * readers may read without the lock and so need atomic
		 * load/store.  No ordering here needed because the
		 * entry itself stays valid until GC.
		 */
		atomic_store_relaxed(&node->state,
		    atomic_load_relaxed(&node->state) | NODE_DELETED);
		unlock_node(node); // memory_order_release

		ASSERT(THMAP_NODE(thmap,
		    atomic_load_relaxed(&parent->slots[slot])) == node);
		node_remove(parent, slot);

		/* Stage the removed node for G/C. */
		stage_mem_gc(thmap, THMAP_GETOFF(thmap, node), THMAP_INODE_LEN);
	}

	/*
	 * If the top node is empty, then we need to remove it from the
	 * root level.  Mark the node as deleted and clear the slot.
	 *
	 * Note: acquiring the lock on the top node effectively prevents
	 * the root slot from changing.
	 */
	if (NODE_COUNT(atomic_load_relaxed(&parent->state)) == 0) {
		const unsigned rslot = query.rslot;
		const thmap_ptr_t nptr =
		    atomic_load_relaxed(&thmap->root[rslot]);

		ASSERT(query.level == 0);
		ASSERT(parent->parent == THMAP_NULL);
		ASSERT(THMAP_GETOFF(thmap, parent) == nptr);

		/* Mark as deleted and remove from the root-level slot. */
		atomic_store_relaxed(&parent->state,
		    atomic_load_relaxed(&parent->state) | NODE_DELETED);
		atomic_store_relaxed(&thmap->root[rslot], THMAP_NULL);

		stage_mem_gc(thmap, nptr, THMAP_INODE_LEN);
	}
	unlock_node(parent);

	/*
	 * Save the value and stage the leaf for G/C.
	 */
	val = leaf->val;
	if ((thmap->flags & THMAP_NOCOPY) == 0) {
		stage_mem_gc(thmap, leaf->key, leaf->len);
	}
	stage_mem_gc(thmap, THMAP_GETOFF(thmap, leaf), sizeof(thmap_leaf_t));
	return val;
}

/*
 * G/C routines.
 */

static void
stage_mem_gc(thmap_t *thmap, uintptr_t addr, size_t len)
{
	thmap_gc_t *head, *gc;

	gc = kmem_intr_alloc(sizeof(thmap_gc_t), KM_NOSLEEP);
	gc->addr = addr;
	gc->len = len;
retry:
	head = atomic_load_relaxed(&thmap->gc_list);
	gc->next = head; // not yet published

	/* Release to subsequent acquire in thmap_stage_gc(). */
	if (!atomic_compare_exchange_weak_explicit_ptr(&thmap->gc_list, &head, gc,
	    memory_order_release, memory_order_relaxed)) {
		goto retry;
	}
}

void *
thmap_stage_gc(thmap_t *thmap)
{
	/* Acquire from prior release in stage_mem_gc(). */
	return atomic_exchange_explicit(&thmap->gc_list, NULL,
	    memory_order_acquire);
}

void
thmap_gc(thmap_t *thmap, void *ref)
{
	thmap_gc_t *gc = ref;

	while (gc) {
		thmap_gc_t *next = gc->next;
		thmap->ops->free(gc->addr, gc->len);
		kmem_intr_free(gc, sizeof(thmap_gc_t));
		gc = next;
	}
}

/*
 * thmap_create: construct a new trie-hash map object.
 */
thmap_t *
thmap_create(uintptr_t baseptr, const thmap_ops_t *ops, unsigned flags)
{
	thmap_t *thmap;
	uintptr_t root;

	/*
	 * Setup the map object.
	 */
	if (!THMAP_ALIGNED_P(baseptr)) {
		return NULL;
	}
	thmap = kmem_zalloc(sizeof(thmap_t), KM_SLEEP);
	if (!thmap) {
		return NULL;
	}
	thmap->baseptr = baseptr;
	thmap->ops = ops ? ops : &thmap_default_ops;
	thmap->flags = flags;

	if ((thmap->flags & THMAP_SETROOT) == 0) {
		/* Allocate the root level. */
		root = thmap->ops->alloc(THMAP_ROOT_LEN);
		thmap->root = THMAP_GETPTR(thmap, root);
		if (!thmap->root) {
			kmem_free(thmap, sizeof(thmap_t));
			return NULL;
		}
		memset(thmap->root, 0, THMAP_ROOT_LEN);
	}

	cprng_strong(kern_cprng, thmap->seed, sizeof thmap->seed, 0);

	return thmap;
}

int
thmap_setroot(thmap_t *thmap, uintptr_t root_off)
{
	if (thmap->root) {
		return -1;
	}
	thmap->root = THMAP_GETPTR(thmap, root_off);
	return 0;
}

uintptr_t
thmap_getroot(const thmap_t *thmap)
{
	return THMAP_GETOFF(thmap, thmap->root);
}

void
thmap_destroy(thmap_t *thmap)
{
	uintptr_t root = THMAP_GETOFF(thmap, thmap->root);
	void *ref;

	ref = thmap_stage_gc(thmap);
	thmap_gc(thmap, ref);

	if ((thmap->flags & THMAP_SETROOT) == 0) {
		thmap->ops->free(root, THMAP_ROOT_LEN);
	}
	kmem_free(thmap, sizeof(thmap_t));
}
