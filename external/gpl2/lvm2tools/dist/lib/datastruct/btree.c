/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "lib.h"
#include "btree.h"

struct node {
	uint32_t key;
	struct node *l, *r, *p;

	void *data;
};

struct btree {
	struct dm_pool *mem;
	struct node *root;
};

struct btree *btree_create(struct dm_pool *mem)
{
	struct btree *t = dm_pool_alloc(mem, sizeof(*t));

	if (t) {
		t->mem = mem;
		t->root = NULL;
	}

	return t;
}

/*
 * Shuffle the bits in a key, to try and remove
 * any ordering.
 */
static uint32_t _shuffle(uint32_t k)
{
#if 1
	return ((k & 0xff) << 24 |
		(k & 0xff00) << 8 |
		(k & 0xff0000) >> 8 | (k & 0xff000000) >> 24);
#else
	return k;
#endif
}

static struct node **_lookup(struct node *const *c, uint32_t key,
			     struct node **p)
{
	*p = NULL;
	while (*c) {
		*p = *c;
		if ((*c)->key == key)
			break;

		if (key < (*c)->key)
			c = &(*c)->l;

		else
			c = &(*c)->r;
	}

	return (struct node **)c;
}

void *btree_lookup(const struct btree *t, uint32_t k)
{
	uint32_t key = _shuffle(k);
	struct node *p, **c = _lookup(&t->root, key, &p);
	return (*c) ? (*c)->data : NULL;
}

int btree_insert(struct btree *t, uint32_t k, void *data)
{
	uint32_t key = _shuffle(k);
	struct node *p, **c = _lookup(&t->root, key, &p), *n;

	if (!*c) {
		if (!(n = dm_pool_alloc(t->mem, sizeof(*n))))
			return_0;

		n->key = key;
		n->data = data;
		n->l = n->r = NULL;
		n->p = p;

		*c = n;
	}

	return 1;
}

void *btree_get_data(const struct btree_iter *it)
{
	return ((struct node *) it)->data;
}

static struct node *_left(struct node *n)
{
	while (n->l)
		n = n->l;
	return n;
}

struct btree_iter *btree_first(const struct btree *t)
{
	if (!t->root)
		return NULL;

	return (struct btree_iter *) _left(t->root);
}

struct btree_iter *btree_next(const struct btree_iter *it)
{
	struct node *n = (struct node *) it;
	uint32_t k = n->key;

	if (n->r)
		return (struct btree_iter *) _left(n->r);

	do
		n = n->p;
	while (n && k > n->key);

	return (struct btree_iter *) n;
}
