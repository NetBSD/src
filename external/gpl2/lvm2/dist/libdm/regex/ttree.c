/*	$NetBSD: ttree.c,v 1.1.1.1 2008/12/22 00:18:37 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of the device-mapper userspace tools.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "dmlib.h"
#include "ttree.h"

struct node {
	unsigned k;
	struct node *l, *m, *r;
	void *data;
};

struct ttree {
	int klen;
	struct dm_pool *mem;
	struct node *root;
};

static struct node **_lookup_single(struct node **c, unsigned int k)
{
	while (*c) {
		if (k < (*c)->k)
			c = &((*c)->l);

		else if (k > (*c)->k)
			c = &((*c)->r);

		else {
			c = &((*c)->m);
			break;
		}
	}

	return c;
}

void *ttree_lookup(struct ttree *tt, unsigned *key)
{
	struct node **c = &tt->root;
	int count = tt->klen;

	while (*c && count) {
		c = _lookup_single(c, *key++);
		count--;
	}

	return *c ? (*c)->data : NULL;
}

static struct node *_tree_node(struct dm_pool *mem, unsigned int k)
{
	struct node *n = dm_pool_zalloc(mem, sizeof(*n));

	if (n)
		n->k = k;

	return n;
}

int ttree_insert(struct ttree *tt, unsigned int *key, void *data)
{
	struct node **c = &tt->root;
	int count = tt->klen;
	unsigned int k;

	do {
		k = *key++;
		c = _lookup_single(c, k);
		count--;

	} while (*c && count);

	if (!*c) {
		count++;

		while (count--) {
			if (!(*c = _tree_node(tt->mem, k))) {
				stack;
				return 0;
			}

			k = *key++;

			if (count)
				c = &((*c)->m);
		}
	}
	(*c)->data = data;

	return 1;
}

struct ttree *ttree_create(struct dm_pool *mem, unsigned int klen)
{
	struct ttree *tt;

	if (!(tt = dm_pool_zalloc(mem, sizeof(*tt)))) {
		stack;
		return NULL;
	}

	tt->klen = klen;
	tt->mem = mem;
	return tt;
}
