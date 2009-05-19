/*	$NetBSD: rpst.c,v 1.1 2009/05/19 12:39:56 yamt Exp $	*/

/*-
 * Copyright (c)2009 YAMAMOTO Takashi,
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
 * radix priority search tree
 *
 * described in:
 *	SIAM J. COMPUT.
 *	Vol. 14, No. 2, May 1985
 *	PRIORITY SEARCH TREES
 *	EDWARD M. McCREIGHT
 *
 * ideas from linux:
 *	- grow tree height on-demand.
 *	- allow duplicated X values.  in that case, we act as a heap.
 */

#include <sys/cdefs.h>

#if defined(_KERNEL)
__KERNEL_RCSID(0, "$NetBSD: rpst.c,v 1.1 2009/05/19 12:39:56 yamt Exp $");
#include <sys/param.h>
#else /* defined(_KERNEL) */
__RCSID("$NetBSD: rpst.c,v 1.1 2009/05/19 12:39:56 yamt Exp $");
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#define	KASSERT	assert
#endif /* defined(_KERNEL) */

#include <sys/rpst.h>

/*
 * rpst_init_tree: initialize a tree.
 */

void
rpst_init_tree(struct rpst_tree *t)
{

	t->t_root = NULL;
	t->t_height = 0;
}

/*
 * rpst_height2max: calculate the maximum index which can be handled by
 * a tree with the given height.
 *
 * 0  ... 0x0000000000000001
 * 1  ... 0x0000000000000003
 * 2  ... 0x0000000000000007
 * 3  ... 0x000000000000000f
 *
 * 31 ... 0x00000000ffffffff
 *
 * 63 ... 0xffffffffffffffff
 */

static uint64_t
rpst_height2max(unsigned int height)
{

	KASSERT(height < 64);
	if (height == 63) {
		return UINT64_MAX;
	}
	return (UINT64_C(1) << (height + 1)) - 1;
}

/*
 * rpst_startmask: calculate the mask for the start of a search.
 * (ie. the mask for the top-most bit)
 */

static uint64_t
rpst_startmask(const struct rpst_tree *t)
{

	return UINT64_C(1) << t->t_height;
}

/*
 * rpst_enlarge_tree: enlarge tree so that 'index' can be stored
 */

static void
rpst_enlarge_tree(struct rpst_tree *t, uint64_t idx)
{

	while (idx > rpst_height2max(t->t_height)) {
		struct rpst_node *n = t->t_root;

		if (n != NULL) {
			rpst_remove_node(t, n);
			memset(&n->n_children, 0, sizeof(n->n_children));
			n->n_children[0] = t->t_root;
			t->t_root = n;
		}
		t->t_height++;
	}
}

/*
 * rpst_insert_node1: a helper for rpst_insert_node.
 */

static struct rpst_node *
rpst_insert_node1(struct rpst_node **where, struct rpst_node *n, uint64_t mask)
{
	struct rpst_node *cur;
	unsigned int idx;

	KASSERT((n->n_x & ((-mask) << 1)) == 0);
next:
	cur = *where;
	if (cur == NULL) {
		memset(&n->n_children, 0, sizeof(n->n_children));
		*where = n;
		return NULL;
	}
	if (n->n_y == cur->n_y && n->n_x != cur->n_x) {
		return cur;
	}
	if (n->n_y < cur->n_y) {
		/* swap cur and n */
		memcpy(n->n_children, cur->n_children, sizeof(n->n_children));
		*where = n;
		n = cur;
		cur = *where;
	}
	KASSERT(*where == cur);
	idx = (n->n_x & mask) != 0;
	where = &cur->n_children[idx];
	mask >>= 1;
	goto next;
}

/*
 * rpst_insert_node: insert a node into the tree.
 *
 * => return NULL on success.
 * => if a duplicated node (a node with the same X,Y pair as ours) is found,
 *    return the node.  in that case, the tree is intact.
 */

struct rpst_node *
rpst_insert_node(struct rpst_tree *t, struct rpst_node *n)
{

	rpst_enlarge_tree(t, n->n_x);
	return rpst_insert_node1(&t->t_root, n, rpst_startmask(t));
}

/*
 * rpst_find_pptr: find a pointer to the given node.
 *
 * also, return the parent node via parentp.  (NULL for the root node.)
 *
 * XXX is it better to simply make each nodes have a pointer to parent?
 */

static struct rpst_node **
rpst_find_pptr(struct rpst_node **where, struct rpst_node *n, uint64_t mask,
    struct rpst_node **parentp)
{
	struct rpst_node *pn = NULL;
	struct rpst_node *cur;
	unsigned int idx;

next:
	cur = *where;
	KASSERT(cur != NULL);
	if (cur == n) {
		*parentp = pn;
		return where;
	}
	idx = (n->n_x & mask) != 0;
	pn = cur;
	where = &cur->n_children[idx];
	mask >>= 1;
	goto next;
}

/*
 * rpst_remove_node_at: remove a node at *where.
 */

static void
rpst_remove_node_at(struct rpst_node **where)
{
	struct rpst_node *tmp[2];
	struct rpst_node *cur;
	struct rpst_node *selected;
	unsigned int selected_idx;
	unsigned int i;

	cur = *where;
	KASSERT(cur != NULL);
next:
	selected = NULL;
	for (i = 0; i < 2; i++) {
		struct rpst_node *c;

		c = cur->n_children[i];
		if (selected == NULL || (c != NULL && c->n_y < selected->n_y)) {
			selected = c;
			selected_idx = i;
		}
	}
	*where = selected;
	if (selected == NULL) {
		return;
	}
	/*
	 * swap selected->n_children and cur->n_children.
	 */
	memcpy(tmp, selected->n_children, sizeof(tmp));
	memcpy(selected->n_children, cur->n_children, sizeof(tmp));
	memcpy(cur->n_children, tmp, sizeof(tmp));
	where = &selected->n_children[selected_idx];
	goto next;
}

/*
 * rpst_remove_node: remove a node from the tree.
 */

void
rpst_remove_node(struct rpst_tree *t, struct rpst_node *n)
{
	struct rpst_node *parent;
	struct rpst_node **where;

	where = rpst_find_pptr(&t->t_root, n, rpst_startmask(t), &parent);
	KASSERT(*where == n);
	rpst_remove_node_at(where);
}

static bool __unused
rpst_iterator_match_p(const struct rpst_node *n, const struct rpst_iterator *it)
{

	if (n->n_y > it->it_max_y) {
		return false;
	}
	if (n->n_x < it->it_min_x) {
		return false;
	}
	if (n->n_x > it->it_max_x) {
		return false;
	}
	return true;
}

static uint64_t
rpst_level2mask(const struct rpst_tree *t, unsigned int level)
{

	if (t->t_height < level) {
		return 0;
	}
	return UINT64_C(1) << (t->t_height - level);
}

struct rpst_node *
rpst_iterate_first(struct rpst_tree *t, uint64_t max_y, uint64_t min_x,
    uint64_t max_x, struct rpst_iterator *it)
{
	struct rpst_node *n;

	KASSERT(min_x <= max_x);
	n = t->t_root;
	if (n == NULL) {
		return NULL;
	}
	it->it_tree = t;
	it->it_cur = n;
	it->it_idx = 0;
	it->it_level = 0;
	it->it_max_y = max_y;
	it->it_min_x = min_x;
	it->it_max_x = max_x;
	return rpst_iterate_next(it);
}

struct rpst_node *
rpst_iterate_next(struct rpst_iterator *it)
{
	struct rpst_tree *t;
	struct rpst_node *n;
	struct rpst_node *next;
	const uint64_t max_y = it->it_max_y;
	const uint64_t min_x = it->it_min_x;
	const uint64_t max_x = it->it_max_x;
	unsigned int idx;
	unsigned int maxidx;
	unsigned int level;
	uint64_t mask;

	t = it->it_tree;
	n = it->it_cur;
	idx = it->it_idx;
	level = it->it_level;
	mask = rpst_level2mask(t, level);
	maxidx = (max_x & mask) != 0;
	KASSERT(n == t->t_root || rpst_iterator_match_p(n, it));
next:
	KASSERT(mask == rpst_level2mask(t, level));
	KASSERT(maxidx == ((max_x & mask) != 0));
	KASSERT(idx <= maxidx + 2);
	KASSERT(n != NULL);
#if 0
	printf("%s: cur=%p, idx=%u maxidx=%u level=%u mask=%" PRIx64 "\n",
	    __func__, (void *)n, idx, maxidx, level, mask);
#endif
	if (idx == maxidx + 1) { /* visit the current node */
		idx++;
		if (min_x <= n->n_x && n->n_x <= max_x) {
			it->it_tree = t;
			it->it_cur = n;
			it->it_idx = idx;
			it->it_level = level;
			KASSERT(rpst_iterator_match_p(n, it));
			return n; /* report */
		}
		goto next;
	} else if (idx == maxidx + 2) { /* back to the parent */
		struct rpst_node **where;

		where = rpst_find_pptr(&t->t_root, n, rpst_startmask(t), &next);
		if (next == NULL) {
			KASSERT(level == 0);
			KASSERT(t->t_root == n);
			KASSERT(&t->t_root == where);
			return NULL; /* done */
		}
		KASSERT(level > 0);
		level--;
		mask = rpst_level2mask(t, level);
		maxidx = (max_x & mask) != 0;
		n = next;
		idx = where - n->n_children + 1;
		KASSERT(idx < 2 + 1);
		goto next;
	}
	/* go to a child */
	KASSERT(idx < 2);
	next = n->n_children[idx];
	if (next == NULL || next->n_y > max_y) {
		idx++;
		goto next;
	}
	level++;
	mask >>= 1;
	maxidx = (max_x & mask) != 0;
	n = next;
	idx = (min_x & mask) != 0;
	goto next;
}

#if defined(UNITTEST)
#include <sys/time.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

static void
rpst_dump_node(const struct rpst_node *n, unsigned int depth)
{
	unsigned int i;

	for (i = 0; i < depth; i++) {
		printf("  ");
	}
	printf("[%u]", depth);
	if (n == NULL) {
		printf("NULL\n");
		return;
	}
	printf("%p x=%" PRIx64 "(%" PRIu64 ") y=%" PRIx64 "(%" PRIu64 ")\n",
	    (const void *)n, n->n_x, n->n_x, n->n_y, n->n_y);
	for (i = 0; i < 2; i++) {
		rpst_dump_node(n->n_children[i], depth + 1);
	}
}

static void
rpst_dump_tree(const struct rpst_tree *t)
{

	printf("pst %p bits=%u\n", (const void *)t, t->t_height);
	rpst_dump_node(t->t_root, 0);
}

struct testnode {
	struct rpst_node n;
	struct testnode *next;
};

int
main(int argc, char *argv[])
{
	struct rpst_tree t;
	struct testnode *h = NULL;
	struct testnode *n;
	struct rpst_node *rn;
	unsigned int i = 500000;
	struct rpst_iterator it;
	struct timeval start;
	struct timeval end;
	unsigned int done;

	rpst_init_tree(&t);
	rpst_dump_tree(&t);
	assert(NULL == rpst_iterate_first(&t, UINT64_MAX, 0, UINT64_MAX, &it));

	done = 0;
	gettimeofday(&start, NULL);
	while (i > 0) {
		n = malloc(sizeof(*n));
		if (i > 499000) {
			n->n.n_x = 10;
			n->n.n_y = random();
		} else if (i > 400000) {
			n->n.n_x = i;
			n->n.n_y = random();
		} else {
			n->n.n_x = random();
			n->n.n_y = random();
		}
#if 0
		printf("[%u] insert %p x=%" PRIu64 " y=%" PRIu64 "\n",
		    i, n, n->n.n_x, n->n.n_y);
#endif
		rpst_insert_node(&t, &n->n);
		n->next = h;
		h = n;
		i--;
		done++;
	}
	gettimeofday(&end, NULL);
	printf("%u nodes inserted in %ju usecs\n", done,
	    (uintmax_t)end.tv_sec * 1000000 + end.tv_usec -
	    start.tv_sec * 1000000 - start.tv_usec);

	done = 0;
	gettimeofday(&start, NULL);
	for (rn = rpst_iterate_first(&t, 100000000, 10, 1000000, &it);
	    rn != NULL;
	    rn = rpst_iterate_next(&it)) {
		done++;
#if 0
		printf("found %p x=%" PRIu64 " y=%" PRIu64 "\n",
		    (void *)rn, rn->n_x, rn->n_y);
#endif
	}
	gettimeofday(&end, NULL);
	printf("%u nodes found in %ju usecs\n", done,
	    (uintmax_t)end.tv_sec * 1000000 + end.tv_usec -
	    start.tv_sec * 1000000 - start.tv_usec);

	done = 0;
	gettimeofday(&start, NULL);
	while (h != NULL) {
		n = h;
		h = n->next;
#if 0
		printf("[%u] remove %p x=%" PRIu64 " y=%" PRIu64 "\n",
		    n, n->n.n_x, n->n.n_y);
#endif
		rpst_remove_node(&t, &n->n);
		i++;
		done++;
	}
	gettimeofday(&end, NULL);
	printf("%u nodes removed in %ju usecs\n", done,
	    (uintmax_t)end.tv_sec * 1000000 + end.tv_usec -
	    start.tv_sec * 1000000 - start.tv_usec);

	rpst_dump_tree(&t);
}
#endif /* defined(UNITTEST) */
