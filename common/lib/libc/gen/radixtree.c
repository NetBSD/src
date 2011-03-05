/*	$NetBSD: radixtree.c,v 1.1.2.2 2011/03/05 15:08:32 bouyer Exp $	*/

/*-
 * Copyright (c)2011 YAMAMOTO Takashi,
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
 */

/*
 * radix tree
 *
 * it's designed to work efficiently with dense index distribution.
 * the memory consumption (number of necessary intermediate nodes)
 * heavily depends on index distribution.  basically, more dense index
 * distribution consumes less nodes per item.
 * approximately,
 * the best case: about RADIX_TREE_PTR_PER_NODE items per node.
 * the worst case: RADIX_TREE_MAX_HEIGHT nodes per item.
 */

#include <sys/cdefs.h>

#if defined(_KERNEL)
__KERNEL_RCSID(0, "$NetBSD: radixtree.c,v 1.1.2.2 2011/03/05 15:08:32 bouyer Exp $");
#include <sys/param.h>
#include <sys/null.h>
#include <sys/pool.h>
#include <sys/radixtree.h>
#else /* defined(_KERNEL) */
__RCSID("$NetBSD: radixtree.c,v 1.1.2.2 2011/03/05 15:08:32 bouyer Exp $");
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#if 1
#define KASSERT assert
#else
#define KASSERT(a)	/* nothing */
#endif
/* XXX */
#if !defined(CTASSERT)
#define	CTASSERT(x)	/* nothing */
#endif
#endif /* defined(_KERNEL) */

#include <sys/radixtree.h>

#define	RADIX_TREE_BITS_PER_HEIGHT	4	/* XXX tune */
#define	RADIX_TREE_PTR_PER_NODE		(1 << RADIX_TREE_BITS_PER_HEIGHT)
#define	RADIX_TREE_MAX_HEIGHT		(64 / RADIX_TREE_BITS_PER_HEIGHT)
CTASSERT((64 % RADIX_TREE_BITS_PER_HEIGHT) == 0);

CTASSERT(((1 << RADIX_TREE_TAG_ID_MAX) & (sizeof(int) - 1)) == 0);
#define	RADIX_TREE_TAG_MASK	((1 << RADIX_TREE_TAG_ID_MAX) - 1)

static inline void *
entry_ptr(void *p)
{

	return (void *)((uintptr_t)p & ~RADIX_TREE_TAG_MASK);
}

static inline unsigned int
entry_tagmask(void *p)
{

	return (uintptr_t)p & RADIX_TREE_TAG_MASK;
}

static inline void *
entry_compose(void *p, unsigned int tagmask)
{

	return (void *)((uintptr_t)p | tagmask);
}

static inline bool
entry_match_p(void *p, unsigned int tagmask)
{

	KASSERT(entry_ptr(p) != NULL || entry_tagmask(p) == 0);
	if (p == NULL) {
		return false;
	}
	if (tagmask == 0) {
		return true;
	}
	return (entry_tagmask(p) & tagmask) != 0;
}

static inline unsigned int
tagid_to_mask(radix_tree_tagid_t id)
{

	return 1U << id;
}

/*
 * radix_tree_node: an intermediate node
 *
 * we don't care the type of leaf nodes.  they are just void *.
 */

struct radix_tree_node {
	void *n_ptrs[RADIX_TREE_PTR_PER_NODE];
	unsigned int n_nptrs;	/* # of non-NULL pointers in n_ptrs */
};

static unsigned int
any_children_tagmask(struct radix_tree_node *n)
{
	unsigned int mask;
	int i;

	mask = 0;
	for (i = 0; i < RADIX_TREE_PTR_PER_NODE; i++) {
		mask |= (unsigned int)(uintptr_t)n->n_ptrs[i];
	}
	return mask & RADIX_TREE_TAG_MASK;
}

/*
 * p_refs[0].pptr == &t->t_root
 *	:
 * p_refs[n].pptr == &(*p_refs[n-1])->n_ptrs[x]
 *	:
 *	:
 * p_refs[t->t_height].pptr == &leaf_pointer
 */

struct radix_tree_path {
	struct radix_tree_node_ref {
		void **pptr;
	} p_refs[RADIX_TREE_MAX_HEIGHT + 1]; /* +1 for the root ptr */
	int p_lastidx;
};

static inline void **
path_pptr(struct radix_tree *t, struct radix_tree_path *p,
    unsigned int height)
{

	KASSERT(height <= t->t_height);
	return p->p_refs[height].pptr;
}

static inline struct radix_tree_node *
path_node(struct radix_tree * t, struct radix_tree_path *p, unsigned int height)
{

	KASSERT(height <= t->t_height);
	return entry_ptr(*path_pptr(t, p, height));
}

static inline unsigned int
path_idx(struct radix_tree * t, struct radix_tree_path *p, unsigned int height)
{

	KASSERT(height <= t->t_height);
	return path_pptr(t, p, height + 1) - path_node(t, p, height)->n_ptrs;
}

/*
 * radix_tree_init_tree:
 *
 * initialize a tree.
 */

void
radix_tree_init_tree(struct radix_tree *t)
{

	t->t_height = 0;
	t->t_root = NULL;
}

/*
 * radix_tree_init_tree:
 *
 * clean up a tree.
 */

void
radix_tree_fini_tree(struct radix_tree *t)
{

	KASSERT(t->t_root == NULL);
	KASSERT(t->t_height == 0);
}

#if defined(_KERNEL)
pool_cache_t radix_tree_node_cache;

static int
radix_tree_node_ctor(void *dummy, void *item, int flags)
{
	struct radix_tree_node *n = item;

	KASSERT(dummy == NULL);
	memset(n, 0, sizeof(*n));
	return 0;
}

/*
 * radix_tree_init:
 *
 * initialize the subsystem.
 */

void
radix_tree_init(void)
{

	radix_tree_node_cache = pool_cache_init(sizeof(struct radix_tree_node),
	    0, 0, 0, "radix_tree_node", NULL, IPL_NONE, radix_tree_node_ctor,
	    NULL, NULL);
	KASSERT(radix_tree_node_cache != NULL);
}
#endif /* defined(_KERNEL) */

static bool __unused
radix_tree_node_clean_p(const struct radix_tree_node *n)
{
	unsigned int i;

	if (n->n_nptrs != 0) {
		return false;
	}
	for (i = 0; i < RADIX_TREE_PTR_PER_NODE; i++) {
		if (n->n_ptrs[i] != NULL) {
			return false;
		}
	}
	return true;
}

static struct radix_tree_node *
radix_tree_alloc_node(void)
{
	struct radix_tree_node *n;

#if defined(_KERNEL)
	n = pool_cache_get(radix_tree_node_cache, PR_NOWAIT);
#else /* defined(_KERNEL) */
	n = calloc(1, sizeof(*n));
#endif /* defined(_KERNEL) */
	KASSERT(n == NULL || radix_tree_node_clean_p(n));
	return n;
}

static void
radix_tree_free_node(struct radix_tree_node *n)
{

	KASSERT(radix_tree_node_clean_p(n));
#if defined(_KERNEL)
	pool_cache_put(radix_tree_node_cache, n);
#else /* defined(_KERNEL) */
	free(n);
#endif /* defined(_KERNEL) */
}

static int
radix_tree_grow(struct radix_tree *t, unsigned int newheight)
{
	const unsigned int tagmask = entry_tagmask(t->t_root);

	KASSERT(newheight <= 64 / RADIX_TREE_BITS_PER_HEIGHT);
	if (t->t_root == NULL) {
		t->t_height = newheight;
		return 0;
	}
	while (t->t_height < newheight) {
		struct radix_tree_node *n;

		n = radix_tree_alloc_node();
		if (n == NULL) {
			/*
			 * don't bother to revert our changes.
			 * the caller will likely retry.
			 */
			return ENOMEM;
		}
		n->n_nptrs = 1;
		n->n_ptrs[0] = t->t_root;
		t->t_root = entry_compose(n, tagmask);
		t->t_height++;
	}
	return 0;
}

static inline void **
radix_tree_lookup_ptr(struct radix_tree *t, uint64_t idx,
    struct radix_tree_path *path, bool alloc, const unsigned int tagmask)
{
	struct radix_tree_node *n;
	int hshift = RADIX_TREE_BITS_PER_HEIGHT * t->t_height;
	int shift;
	void **vpp;
	const uint64_t mask = (UINT64_C(1) << RADIX_TREE_BITS_PER_HEIGHT) - 1;
	struct radix_tree_node_ref *refs = NULL;

	KASSERT(tagmask == 0 || !alloc);
	KASSERT(path == NULL || !alloc);
	vpp = &t->t_root;
	if (path != NULL) {
		refs = path->p_refs;
		refs->pptr = vpp;
	}
	n = NULL;
	for (shift = 64 - RADIX_TREE_BITS_PER_HEIGHT; shift >= 0;) {
		struct radix_tree_node *c;
		void *entry;
		const uint64_t i = (idx >> shift) & mask;

		if (shift >= hshift) {
			unsigned int newheight;

			KASSERT(vpp == &t->t_root);
			if (i == 0) {
				shift -= RADIX_TREE_BITS_PER_HEIGHT;
				continue;
			}
			if (!alloc) {
				if (path != NULL) {
					KASSERT((refs - path->p_refs) == 0);
					path->p_lastidx = 0;
				}
				return NULL;
			}
			newheight = shift / RADIX_TREE_BITS_PER_HEIGHT + 1;
			if (radix_tree_grow(t, newheight)) {
				return NULL;
			}
			hshift = RADIX_TREE_BITS_PER_HEIGHT * t->t_height;
		}
		entry = *vpp;
		c = entry_ptr(entry);
		if (c == NULL ||
		    (tagmask != 0 &&
		    (entry_tagmask(entry) & tagmask) == 0)) {
			if (!alloc) {
				if (path != NULL) {
					path->p_lastidx = refs - path->p_refs;
				}
				return NULL;
			}
			c = radix_tree_alloc_node();
			if (c == NULL) {
				return NULL;
			}
			*vpp = c;
			if (n != NULL) {
				KASSERT(n->n_nptrs < RADIX_TREE_PTR_PER_NODE);
				n->n_nptrs++;
			}
		}
		n = c;
		vpp = &n->n_ptrs[i];
		if (path != NULL) {
			refs++;
			refs->pptr = vpp;
		}
		shift -= RADIX_TREE_BITS_PER_HEIGHT;
	}
	if (alloc) {
		KASSERT(*vpp == NULL);
		if (n != NULL) {
			KASSERT(n->n_nptrs < RADIX_TREE_PTR_PER_NODE);
			n->n_nptrs++;
		}
	}
	if (path != NULL) {
		path->p_lastidx = refs - path->p_refs;
	}
	return vpp;
}

/*
 * radix_tree_insert_node:
 *
 * insert the node at idx.
 * it's illegal to insert NULL.
 * it's illegal to insert a non-aligned pointer.
 *
 * this function returns ENOMEM if necessary memory allocation failed.
 * otherwise, this function returns 0.
 *
 * note that inserting a node can involves memory allocation for intermediate
 * nodes.  if _KERNEL, it's done with non-blocking IPL_NONE memory allocation.
 */

int
radix_tree_insert_node(struct radix_tree *t, uint64_t idx, void *p)
{
	void **vpp;

	KASSERT(p != NULL);
	KASSERT(entry_compose(p, 0) == p);
	vpp = radix_tree_lookup_ptr(t, idx, NULL, true, 0);
	if (vpp == NULL) {
		return ENOMEM;
	}
	KASSERT(*vpp == NULL);
	*vpp = p;
	return 0;
}

void *
radix_tree_replace_node(struct radix_tree *t, uint64_t idx, void *p)
{
	void **vpp;
	void *oldp;

	KASSERT(p != NULL);
	KASSERT(entry_compose(p, 0) == p);
	vpp = radix_tree_lookup_ptr(t, idx, NULL, false, 0);
	KASSERT(vpp != NULL);
	oldp = *vpp;
	KASSERT(oldp != NULL);
	*vpp = entry_compose(p, entry_tagmask(*vpp));
	return entry_ptr(oldp);
}

/*
 * radix_tree_remove_node:
 *
 * remove the node at idx.
 * it's illegal to try to remove a node which has not been inserted.
 */

void *
radix_tree_remove_node(struct radix_tree *t, uint64_t idx)
{
	struct radix_tree_path path;
	void **vpp;
	void *oldp;
	int i;

	vpp = radix_tree_lookup_ptr(t, idx, &path, false, 0);
	KASSERT(vpp != NULL);
	oldp = *vpp;
	KASSERT(oldp != NULL);
	KASSERT(path.p_lastidx == t->t_height);
	KASSERT(vpp == path_pptr(t, &path, path.p_lastidx));
	*vpp = NULL;
	for (i = t->t_height - 1; i >= 0; i--) {
		void *entry;
		struct radix_tree_node ** const pptr =
		    (struct radix_tree_node **)path_pptr(t, &path, i);
		struct radix_tree_node *n;

		KASSERT(pptr != NULL);
		entry = *pptr;
		n = entry_ptr(entry);
		KASSERT(n != NULL);
		KASSERT(n->n_nptrs > 0);
		n->n_nptrs--;
		if (n->n_nptrs > 0) {
			break;
		}
		radix_tree_free_node(n);
		*pptr = NULL;
	}
	/*
	 * fix up height
	 */
	if (i < 0) {
		KASSERT(t->t_root == NULL);
		t->t_height = 0;
	}
	/*
	 * update tags
	 */
	for (; i >= 0; i--) {
		void *entry;
		struct radix_tree_node ** const pptr =
		    (struct radix_tree_node **)path_pptr(t, &path, i);
		struct radix_tree_node *n;
		unsigned int newmask;

		KASSERT(pptr != NULL);
		entry = *pptr;
		n = entry_ptr(entry);
		KASSERT(n != NULL);
		KASSERT(n->n_nptrs > 0);
		newmask = any_children_tagmask(n);
		if (newmask == entry_tagmask(entry)) {
			break;
		}
		*pptr = entry_compose(n, newmask);
	}
	/*
	 * XXX is it worth to try to reduce height?
	 * if we do that, make radix_tree_grow rollback its change as well.
	 */
	return entry_ptr(oldp);
}

/*
 * radix_tree_lookup_node:
 *
 * returns the node at idx.
 * returns NULL if nothing is found at idx.
 */

void *
radix_tree_lookup_node(struct radix_tree *t, uint64_t idx)
{
	void **vpp;

	vpp = radix_tree_lookup_ptr(t, idx, NULL, false, 0);
	if (vpp == NULL) {
		return NULL;
	}
	return entry_ptr(*vpp);
}

static inline void
gang_lookup_init(struct radix_tree *t, uint64_t idx,
    struct radix_tree_path *path, const unsigned int tagmask)
{
	void **vpp;

	vpp = radix_tree_lookup_ptr(t, idx, path, false, tagmask);
	KASSERT(vpp == NULL ||
	    vpp == path_pptr(t, path, path->p_lastidx));
	KASSERT(&t->t_root == path_pptr(t, path, 0));
}

static inline unsigned int
gang_lookup_scan(struct radix_tree *t, struct radix_tree_path *path,
    void **results, unsigned int maxresults, const unsigned int tagmask)
{
	void **vpp;
	int nfound;
	unsigned int lastidx;

	KASSERT(maxresults > 0);
	lastidx = path->p_lastidx;
	if (lastidx == 0) {
		return 0;
	}
	nfound = 0;
	vpp = path_pptr(t, path, lastidx);
	while (/*CONSTCOND*/true) {
		struct radix_tree_node *n;
		int i;

		if (entry_match_p(*vpp, tagmask)) {
			KASSERT(lastidx == t->t_height);
			/*
			 * record the non-NULL leaf.
			 */
			results[nfound] = entry_ptr(*vpp);
			nfound++;
			if (nfound == maxresults) {
				return nfound;
			}
		}
scan_siblings:
		/*
		 * try to find the next non-NULL sibling.
		 */
		n = path_node(t, path, lastidx - 1);
		if (*vpp != NULL && n->n_nptrs == 1) {
			/*
			 * optimization
			 */
			goto no_siblings;
		}
		for (i = path_idx(t, path, lastidx - 1) + 1;
		    i < RADIX_TREE_PTR_PER_NODE;
		    i++) {
			if (entry_match_p(n->n_ptrs[i], tagmask)) {
				vpp = &n->n_ptrs[i];
				path->p_refs[lastidx].pptr = vpp;
				KASSERT(path_idx(t, path, lastidx - 1)
				    == i);
				break;
			}
		}
		if (i == RADIX_TREE_PTR_PER_NODE) {
no_siblings:
			/*
			 * not found.  go to parent.
			 */
			lastidx--;
			if (lastidx == 0) {
				return nfound;
			}
			vpp = path_pptr(t, path, lastidx);
			goto scan_siblings;
		}
		/*
		 * descending the left-most child node, upto the leaf or NULL.
		 */
		while (entry_match_p(*vpp, tagmask) && lastidx < t->t_height) {
			n = entry_ptr(*vpp);
			vpp = &n->n_ptrs[0];
			lastidx++;
			path->p_refs[lastidx].pptr = vpp;
		}
	}
}

/*
 * radix_tree_gang_lookup_node:
 *
 * search nodes starting from idx in the ascending order.
 * results should be an array large enough to hold maxresults pointers.
 * returns the number of nodes found, up to maxresults.
 * returning less than maxresults means there are no more nodes.
 *
 * the result of this function is semantically equivalent to what could be
 * obtained by repeated calls of radix_tree_lookup_node with increasing index.
 * but this function is much faster when node indexes are distributed sparsely.
 *
 * note that this function doesn't return exact values of node indexes of
 * found nodes.  if they are important for a caller, it's the caller's
 * responsibility to check them, typically by examinining the returned nodes
 * using some caller-specific knowledge about them.
 */

unsigned int
radix_tree_gang_lookup_node(struct radix_tree *t, uint64_t idx,
    void **results, unsigned int maxresults)
{
	struct radix_tree_path path;

	gang_lookup_init(t, idx, &path, 0);
	return gang_lookup_scan(t, &path, results, maxresults, 0);
}

/*
 * radix_tree_gang_lookup_tagged_node:
 *
 * same as radix_tree_gang_lookup_node except that this one only returns
 * nodes tagged with tagid.
 */

unsigned int
radix_tree_gang_lookup_tagged_node(struct radix_tree *t, uint64_t idx,
    void **results, unsigned int maxresults, radix_tree_tagid_t tagid)
{
	struct radix_tree_path path;
	const unsigned int tagmask = tagid_to_mask(tagid);

	gang_lookup_init(t, idx, &path, tagmask);
	return gang_lookup_scan(t, &path, results, maxresults, tagmask);
}

bool
radix_tree_get_tag(struct radix_tree *t, uint64_t idx,
    radix_tree_tagid_t tagid)
{
#if 1
	const unsigned int tagmask = tagid_to_mask(tagid);
	void **vpp;

	vpp = radix_tree_lookup_ptr(t, idx, NULL, false, tagmask);
	if (vpp == NULL) {
		return false;
	}
	KASSERT(*vpp != NULL);
	return (entry_tagmask(*vpp) & tagmask) != 0;
#else
	const unsigned int tagmask = tagid_to_mask(tagid);
	void **vpp;

	vpp = radix_tree_lookup_ptr(t, idx, NULL, false, 0);
	KASSERT(vpp != NULL);
	return (entry_tagmask(*vpp) & tagmask) != 0;
#endif
}

void
radix_tree_set_tag(struct radix_tree *t, uint64_t idx,
    radix_tree_tagid_t tagid)
{
	struct radix_tree_path path;
	const unsigned int tagmask = tagid_to_mask(tagid);
	void **vpp;
	int i;

	vpp = radix_tree_lookup_ptr(t, idx, &path, false, 0);
	KASSERT(vpp != NULL);
	KASSERT(*vpp != NULL);
	KASSERT(path.p_lastidx == t->t_height);
	KASSERT(vpp == path_pptr(t, &path, path.p_lastidx));
	for (i = t->t_height; i >= 0; i--) {
		void ** const pptr = (void **)path_pptr(t, &path, i);
		void *entry;

		KASSERT(pptr != NULL);
		entry = *pptr;
		if ((entry_tagmask(entry) & tagmask) != 0) {
			break;
		}
		*pptr = (void *)((uintptr_t)entry | tagmask);
	}
}

void
radix_tree_clear_tag(struct radix_tree *t, uint64_t idx,
    radix_tree_tagid_t tagid)
{
	struct radix_tree_path path;
	const unsigned int tagmask = tagid_to_mask(tagid);
	void **vpp;
	int i;

	vpp = radix_tree_lookup_ptr(t, idx, &path, false, 0);
	KASSERT(vpp != NULL);
	KASSERT(*vpp != NULL);
	KASSERT(path.p_lastidx == t->t_height);
	KASSERT(vpp == path_pptr(t, &path, path.p_lastidx));
	if ((entry_tagmask(*vpp) & tagmask) == 0) {
		return;
	}
	for (i = t->t_height; i >= 0; i--) {
		void ** const pptr = (void **)path_pptr(t, &path, i);
		void *entry;

		KASSERT(pptr != NULL);
		entry = *pptr;
		KASSERT((entry_tagmask(entry) & tagmask) != 0);
		*pptr = entry_compose(entry_ptr(entry),
		    entry_tagmask(entry) & ~tagmask);
		if (0 < i && i < t->t_height - 1) {
			struct radix_tree_node *n = path_node(t, &path, i - 1);

			if ((any_children_tagmask(n) & tagmask) != 0) {
				break;
			}
		}
	}
}

#if defined(UNITTEST)

#include <inttypes.h>
#include <stdio.h>

static void
radix_tree_dump_node(const struct radix_tree *t, void *vp,
    uint64_t offset, unsigned int height)
{
	struct radix_tree_node *n;
	unsigned int i;

	for (i = 0; i < t->t_height - height; i++) {
		printf(" ");
	}
	if (entry_tagmask(vp) == 0) {
		printf("[%" PRIu64 "] %p", offset, entry_ptr(vp));
	} else {
		printf("[%" PRIu64 "] %p (tagmask=0x%x)", offset, entry_ptr(vp),
		    entry_tagmask(vp));
	}
	if (height == 0) {
		printf(" (leaf)\n");
		return;
	}
	n = entry_ptr(vp);
	assert(any_children_tagmask(n) == entry_tagmask(vp));
	printf(" (%u children)\n", n->n_nptrs);
	for (i = 0; i < __arraycount(n->n_ptrs); i++) {
		void *c;

		c = n->n_ptrs[i];
		if (c == NULL) {
			continue;
		}
		radix_tree_dump_node(t, c,
		    offset + i * (UINT64_C(1) <<
		    (RADIX_TREE_BITS_PER_HEIGHT * (height - 1))), height - 1);
	}
}

void radix_tree_dump(const struct radix_tree *);

void
radix_tree_dump(const struct radix_tree *t)
{

	printf("tree %p height=%u\n", t, t->t_height);
	radix_tree_dump_node(t, t->t_root, 0, t->t_height);
}

static void
test1(void)
{
	struct radix_tree s;
	struct radix_tree *t = &s;
	void *results[3];

	radix_tree_init_tree(t);
	radix_tree_dump(t);
	assert(radix_tree_lookup_node(t, 0) == NULL);
	assert(radix_tree_lookup_node(t, 1000) == NULL);
	assert(radix_tree_insert_node(t, 1000, (void *)0xdeadbea0) == 0);
	radix_tree_dump(t);
	assert(!radix_tree_get_tag(t, 1000, 0));
	assert(!radix_tree_get_tag(t, 1000, 1));
	radix_tree_set_tag(t, 1000, 1);
	assert(!radix_tree_get_tag(t, 1000, 0));
	assert(radix_tree_get_tag(t, 1000, 1));
	radix_tree_dump(t);
	assert(radix_tree_lookup_node(t, 1000) == (void *)0xdeadbea0);
	assert(radix_tree_insert_node(t, 0, (void *)0xbea0) == 0);
	radix_tree_dump(t);
	assert(radix_tree_lookup_node(t, 0) == (void *)0xbea0);
	assert(radix_tree_lookup_node(t, 1000) == (void *)0xdeadbea0);
	assert(radix_tree_insert_node(t, UINT64_C(10000000000), (void *)0xdea0)
	    == 0);
	radix_tree_dump(t);
	assert(radix_tree_lookup_node(t, 0) == (void *)0xbea0);
	assert(radix_tree_lookup_node(t, 1000) == (void *)0xdeadbea0);
	assert(radix_tree_lookup_node(t, UINT64_C(10000000000)) ==
	    (void *)0xdea0);
	radix_tree_dump(t);
	assert(!radix_tree_get_tag(t, 0, 1));
	assert(radix_tree_get_tag(t, 1000, 1));
	assert(!radix_tree_get_tag(t, UINT64_C(10000000000), 1));
	radix_tree_set_tag(t, 0, 1);;
	radix_tree_set_tag(t, UINT64_C(10000000000), 1);
	radix_tree_dump(t);
	assert(radix_tree_get_tag(t, 0, 1));
	assert(radix_tree_get_tag(t, 1000, 1));
	assert(radix_tree_get_tag(t, UINT64_C(10000000000), 1));
	radix_tree_clear_tag(t, 0, 1);;
	radix_tree_clear_tag(t, UINT64_C(10000000000), 1);
	radix_tree_dump(t);
	assert(!radix_tree_get_tag(t, 0, 1));
	assert(radix_tree_get_tag(t, 1000, 1));
	assert(!radix_tree_get_tag(t, UINT64_C(10000000000), 1));
	radix_tree_dump(t);
	assert(radix_tree_replace_node(t, 1000, (void *)0x12345678) ==
	    (void *)0xdeadbea0);
	assert(!radix_tree_get_tag(t, 1000, 0));
	assert(radix_tree_get_tag(t, 1000, 1));
	assert(radix_tree_gang_lookup_node(t, 0, results, 3) == 3);
	assert(results[0] == (void *)0xbea0);
	assert(results[1] == (void *)0x12345678);
	assert(results[2] == (void *)0xdea0);
	assert(radix_tree_gang_lookup_node(t, 1, results, 3) == 2);
	assert(results[0] == (void *)0x12345678);
	assert(results[1] == (void *)0xdea0);
	assert(radix_tree_gang_lookup_node(t, 1001, results, 3) == 1);
	assert(results[0] == (void *)0xdea0);
	assert(radix_tree_gang_lookup_node(t, UINT64_C(10000000001), results, 3)
	    == 0);
	assert(radix_tree_gang_lookup_node(t, UINT64_C(1000000000000), results,
	    3) == 0);
	assert(radix_tree_gang_lookup_tagged_node(t, 0, results, 100, 1) == 1);
	assert(results[0] == (void *)0x12345678);
	assert(entry_tagmask(t->t_root) != 0);
	assert(radix_tree_remove_node(t, 1000) == (void *)0x12345678);
	assert(entry_tagmask(t->t_root) == 0);
	radix_tree_dump(t);
	assert(radix_tree_remove_node(t, UINT64_C(10000000000)) ==
	    (void *)0xdea0);
	radix_tree_dump(t);
	assert(radix_tree_remove_node(t, 0) == (void *)0xbea0);
	radix_tree_dump(t);
	radix_tree_fini_tree(t);
}

#include <sys/time.h>

struct testnode {
	uint64_t idx;
};

static void
printops(const char *name, unsigned int n, const struct timeval *stv,
    const struct timeval *etv)
{
	uint64_t s = stv->tv_sec * 1000000 + stv->tv_usec;
	uint64_t e = etv->tv_sec * 1000000 + etv->tv_usec;

	printf("%lf %s/s\n", (double)n / (e - s) * 1000000, name);
}

#define	TEST2_GANG_LOOKUP_NODES	16

static bool
test2_should_tag(unsigned int i, radix_tree_tagid_t tagid)
{

	if (tagid == 0) {
		return (i & 0x3) == 0;
	} else {
		return (i % 7) == 0;
	}
}

static void
test2(bool dense)
{
	struct radix_tree s;
	struct radix_tree *t = &s;
	struct testnode *n;
	unsigned int i;
	unsigned int nnodes = 100000;
	unsigned int removed;
	radix_tree_tagid_t tag;
	unsigned int ntagged[RADIX_TREE_TAG_ID_MAX];
	struct testnode *nodes;
	struct timeval stv;
	struct timeval etv;

	nodes = malloc(nnodes * sizeof(*nodes));
	for (tag = 0; tag < RADIX_TREE_TAG_ID_MAX; tag++) {
		ntagged[tag] = 0;
	}
	radix_tree_init_tree(t);
	for (i = 0; i < nnodes; i++) {
		n = &nodes[i];
		n->idx = random();
		if (sizeof(long) == 4) {
			n->idx <<= 32;
			n->idx |= (uint32_t)random();
		}
		if (dense) {
			n->idx %= nnodes * 2;
		}
		while (radix_tree_lookup_node(t, n->idx) != NULL) {
			n->idx++;
		}
		radix_tree_insert_node(t, n->idx, n);
		for (tag = 0; tag < RADIX_TREE_TAG_ID_MAX; tag++) {
			if (test2_should_tag(i, tag)) {
				radix_tree_set_tag(t, n->idx, tag);
				ntagged[tag]++;
			}
			assert(test2_should_tag(i, tag) ==
			    radix_tree_get_tag(t, n->idx, tag));
		}
	}

	gettimeofday(&stv, NULL);
	for (i = 0; i < nnodes; i++) {
		n = &nodes[i];
		assert(radix_tree_lookup_node(t, n->idx) == n);
	}
	gettimeofday(&etv, NULL);
	printops("lookup", nnodes, &stv, &etv);

	for (tag = 0; tag < RADIX_TREE_TAG_ID_MAX; tag++) {
		gettimeofday(&stv, NULL);
		for (i = 0; i < nnodes; i++) {
			n = &nodes[i];
			assert(test2_should_tag(i, tag) ==
			    radix_tree_get_tag(t, n->idx, tag));
		}
		gettimeofday(&etv, NULL);
		printops("get_tag", ntagged[tag], &stv, &etv);
	}

	gettimeofday(&stv, NULL);
	for (i = 0; i < nnodes; i++) {
		n = &nodes[i];
		radix_tree_remove_node(t, n->idx);
	}
	gettimeofday(&etv, NULL);
	printops("remove", nnodes, &stv, &etv);

	gettimeofday(&stv, NULL);
	for (i = 0; i < nnodes; i++) {
		n = &nodes[i];
		radix_tree_insert_node(t, n->idx, n);
	}
	gettimeofday(&etv, NULL);
	printops("insert", nnodes, &stv, &etv);

	for (tag = 0; tag < RADIX_TREE_TAG_ID_MAX; tag++) {
		ntagged[tag] = 0;
		gettimeofday(&stv, NULL);
		for (i = 0; i < nnodes; i++) {
			n = &nodes[i];
			if (test2_should_tag(i, tag)) {
				radix_tree_set_tag(t, n->idx, tag);
				ntagged[tag]++;
			}
		}
		gettimeofday(&etv, NULL);
		printops("set_tag", ntagged[tag], &stv, &etv);
	}

	gettimeofday(&stv, NULL);
	{
		struct testnode *results[TEST2_GANG_LOOKUP_NODES];
		uint64_t nextidx;
		unsigned int nfound;
		unsigned int total;

		nextidx = 0;
		total = 0;
		while ((nfound = radix_tree_gang_lookup_node(t, nextidx,
		    (void *)results, __arraycount(results))) > 0) {
			nextidx = results[nfound - 1]->idx + 1;
			total += nfound;
		}
		assert(total == nnodes);
	}
	gettimeofday(&etv, NULL);
	printops("ganglookup", nnodes, &stv, &etv);

	for (tag = 0; tag < RADIX_TREE_TAG_ID_MAX; tag++) {
		gettimeofday(&stv, NULL);
		{
			struct testnode *results[TEST2_GANG_LOOKUP_NODES];
			uint64_t nextidx;
			unsigned int nfound;
			unsigned int total;

			nextidx = 0;
			total = 0;
			while ((nfound = radix_tree_gang_lookup_tagged_node(t,
			    nextidx, (void *)results, __arraycount(results),
			    tag)) > 0) {
				nextidx = results[nfound - 1]->idx + 1;
				total += nfound;
			}
			assert(total == ntagged[tag]);
		}
		gettimeofday(&etv, NULL);
		printops("ganglookup_tag", ntagged[tag], &stv, &etv);
	}

	removed = 0;
	for (tag = 0; tag < RADIX_TREE_TAG_ID_MAX; tag++) {
		unsigned int total;

		total = 0;
		gettimeofday(&stv, NULL);
		{
			struct testnode *results[TEST2_GANG_LOOKUP_NODES];
			uint64_t nextidx;
			unsigned int nfound;

			nextidx = 0;
			while ((nfound = radix_tree_gang_lookup_tagged_node(t,
			    nextidx, (void *)results, __arraycount(results),
			    tag)) > 0) {
				for (i = 0; i < nfound; i++) {
					radix_tree_remove_node(t,
					    results[i]->idx);
				}
				nextidx = results[nfound - 1]->idx + 1;
				total += nfound;
			}
			assert(tag != 0 || total == ntagged[tag]);
			assert(total <= ntagged[tag]);
		}
		gettimeofday(&etv, NULL);
		printops("ganglookup_tag+remove", total, &stv, &etv);
		removed += total;
	}

	gettimeofday(&stv, NULL);
	{
		struct testnode *results[TEST2_GANG_LOOKUP_NODES];
		uint64_t nextidx;
		unsigned int nfound;
		unsigned int total;

		nextidx = 0;
		total = 0;
		while ((nfound = radix_tree_gang_lookup_node(t, nextidx,
		    (void *)results, __arraycount(results))) > 0) {
			for (i = 0; i < nfound; i++) {
				assert(results[i] == radix_tree_remove_node(t,
				    results[i]->idx));
			}
			nextidx = results[nfound - 1]->idx + 1;
			total += nfound;
		}
		assert(total == nnodes - removed);
	}
	gettimeofday(&etv, NULL);
	printops("ganglookup+remove", nnodes - removed, &stv, &etv);

	radix_tree_fini_tree(t);
	free(nodes);
}

int
main(int argc, char *argv[])
{

	test1();
	printf("dense distribution:\n");
	test2(true);
	printf("sparse distribution:\n");
	test2(false);
	return 0;
}

#endif /* defined(UNITTEST) */
