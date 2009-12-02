/*	$NetBSD: hash.c,v 1.1.1.2 2009/12/02 00:26:11 haad Exp $	*/

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

struct dm_hash_node {
	struct dm_hash_node *next;
	void *data;
	unsigned keylen;
	char key[0];
};

struct dm_hash_table {
	unsigned num_nodes;
	unsigned num_slots;
	struct dm_hash_node **slots;
};

/* Permutation of the Integers 0 through 255 */
static unsigned char _nums[] = {
	1, 14, 110, 25, 97, 174, 132, 119, 138, 170, 125, 118, 27, 233, 140, 51,
	87, 197, 177, 107, 234, 169, 56, 68, 30, 7, 173, 73, 188, 40, 36, 65,
	49, 213, 104, 190, 57, 211, 148, 223, 48, 115, 15, 2, 67, 186, 210, 28,
	12, 181, 103, 70, 22, 58, 75, 78, 183, 167, 238, 157, 124, 147, 172,
	144,
	176, 161, 141, 86, 60, 66, 128, 83, 156, 241, 79, 46, 168, 198, 41, 254,
	178, 85, 253, 237, 250, 154, 133, 88, 35, 206, 95, 116, 252, 192, 54,
	221,
	102, 218, 255, 240, 82, 106, 158, 201, 61, 3, 89, 9, 42, 155, 159, 93,
	166, 80, 50, 34, 175, 195, 100, 99, 26, 150, 16, 145, 4, 33, 8, 189,
	121, 64, 77, 72, 208, 245, 130, 122, 143, 55, 105, 134, 29, 164, 185,
	194,
	193, 239, 101, 242, 5, 171, 126, 11, 74, 59, 137, 228, 108, 191, 232,
	139,
	6, 24, 81, 20, 127, 17, 91, 92, 251, 151, 225, 207, 21, 98, 113, 112,
	84, 226, 18, 214, 199, 187, 13, 32, 94, 220, 224, 212, 247, 204, 196,
	43,
	249, 236, 45, 244, 111, 182, 153, 136, 129, 90, 217, 202, 19, 165, 231,
	71,
	230, 142, 96, 227, 62, 179, 246, 114, 162, 53, 160, 215, 205, 180, 47,
	109,
	44, 38, 31, 149, 135, 0, 216, 52, 63, 23, 37, 69, 39, 117, 146, 184,
	163, 200, 222, 235, 248, 243, 219, 10, 152, 131, 123, 229, 203, 76, 120,
	209
};

static struct dm_hash_node *_create_node(const char *str, unsigned len)
{
	struct dm_hash_node *n = dm_malloc(sizeof(*n) + len);

	if (n) {
		memcpy(n->key, str, len);
		n->keylen = len;
	}

	return n;
}

static unsigned long _hash(const char *str, unsigned len)
{
	unsigned long h = 0, g;
	unsigned i;

	for (i = 0; i < len; i++) {
		h <<= 4;
		h += _nums[(unsigned char) *str++];
		g = h & ((unsigned long) 0xf << 16u);
		if (g) {
			h ^= g >> 16u;
			h ^= g >> 5u;
		}
	}

	return h;
}

struct dm_hash_table *dm_hash_create(unsigned size_hint)
{
	size_t len;
	unsigned new_size = 16u;
	struct dm_hash_table *hc = dm_malloc(sizeof(*hc));

	if (!hc) {
		stack;
		return 0;
	}

	memset(hc, 0, sizeof(*hc));

	/* round size hint up to a power of two */
	while (new_size < size_hint)
		new_size = new_size << 1;

	hc->num_slots = new_size;
	len = sizeof(*(hc->slots)) * new_size;
	if (!(hc->slots = dm_malloc(len))) {
		stack;
		goto bad;
	}
	memset(hc->slots, 0, len);
	return hc;

      bad:
	dm_free(hc->slots);
	dm_free(hc);
	return 0;
}

static void _free_nodes(struct dm_hash_table *t)
{
	struct dm_hash_node *c, *n;
	unsigned i;

	for (i = 0; i < t->num_slots; i++)
		for (c = t->slots[i]; c; c = n) {
			n = c->next;
			dm_free(c);
		}
}

void dm_hash_destroy(struct dm_hash_table *t)
{
	_free_nodes(t);
	dm_free(t->slots);
	dm_free(t);
}

static struct dm_hash_node **_find(struct dm_hash_table *t, const char *key,
				   uint32_t len)
{
	unsigned h = _hash(key, len) & (t->num_slots - 1);
	struct dm_hash_node **c;

	for (c = &t->slots[h]; *c; c = &((*c)->next)) {
		if ((*c)->keylen != len)
			continue;

		if (!memcmp(key, (*c)->key, len))
			break;
	}

	return c;
}

void *dm_hash_lookup_binary(struct dm_hash_table *t, const char *key,
			 uint32_t len)
{
	struct dm_hash_node **c = _find(t, key, len);

	return *c ? (*c)->data : 0;
}

int dm_hash_insert_binary(struct dm_hash_table *t, const char *key,
			  uint32_t len, void *data)
{
	struct dm_hash_node **c = _find(t, key, len);

	if (*c)
		(*c)->data = data;
	else {
		struct dm_hash_node *n = _create_node(key, len);

		if (!n)
			return 0;

		n->data = data;
		n->next = 0;
		*c = n;
		t->num_nodes++;
	}

	return 1;
}

void dm_hash_remove_binary(struct dm_hash_table *t, const char *key,
			uint32_t len)
{
	struct dm_hash_node **c = _find(t, key, len);

	if (*c) {
		struct dm_hash_node *old = *c;
		*c = (*c)->next;
		dm_free(old);
		t->num_nodes--;
	}
}

void *dm_hash_lookup(struct dm_hash_table *t, const char *key)
{
	return dm_hash_lookup_binary(t, key, strlen(key) + 1);
}

int dm_hash_insert(struct dm_hash_table *t, const char *key, void *data)
{
	return dm_hash_insert_binary(t, key, strlen(key) + 1, data);
}

void dm_hash_remove(struct dm_hash_table *t, const char *key)
{
	dm_hash_remove_binary(t, key, strlen(key) + 1);
}

unsigned dm_hash_get_num_entries(struct dm_hash_table *t)
{
	return t->num_nodes;
}

void dm_hash_iter(struct dm_hash_table *t, dm_hash_iterate_fn f)
{
	struct dm_hash_node *c, *n;
	unsigned i;

	for (i = 0; i < t->num_slots; i++)
		for (c = t->slots[i]; c; c = n) {
			n = c->next;
			f(c->data);
		}
}

void dm_hash_wipe(struct dm_hash_table *t)
{
	_free_nodes(t);
	memset(t->slots, 0, sizeof(struct dm_hash_node *) * t->num_slots);
	t->num_nodes = 0u;
}

char *dm_hash_get_key(struct dm_hash_table *t __attribute((unused)),
		      struct dm_hash_node *n)
{
	return n->key;
}

void *dm_hash_get_data(struct dm_hash_table *t __attribute((unused)),
		       struct dm_hash_node *n)
{
	return n->data;
}

static struct dm_hash_node *_next_slot(struct dm_hash_table *t, unsigned s)
{
	struct dm_hash_node *c = NULL;
	unsigned i;

	for (i = s; i < t->num_slots && !c; i++)
		c = t->slots[i];

	return c;
}

struct dm_hash_node *dm_hash_get_first(struct dm_hash_table *t)
{
	return _next_slot(t, 0);
}

struct dm_hash_node *dm_hash_get_next(struct dm_hash_table *t, struct dm_hash_node *n)
{
	unsigned h = _hash(n->key, n->keylen) & (t->num_slots - 1);

	return n->next ? n->next : _next_slot(t, h + 1);
}
