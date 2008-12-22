/*	$NetBSD: hash.h,v 1.1.1.1 2008/12/22 00:18:36 haad Exp $	*/

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

#ifndef _DM_HASH_H
#define _DM_HASH_H

struct hash_table;
struct hash_node;

typedef void (*iterate_fn) (void *data);

struct hash_table *hash_create(unsigned size_hint);
void hash_destroy(struct hash_table *t);
void hash_wipe(struct hash_table *t);

void *hash_lookup(struct hash_table *t, const char *key);
int hash_insert(struct hash_table *t, const char *key, void *data);
void hash_remove(struct hash_table *t, const char *key);

void *hash_lookup_binary(struct hash_table *t, const char *key, uint32_t len);
int hash_insert_binary(struct hash_table *t, const char *key, uint32_t len,
		       void *data);
void hash_remove_binary(struct hash_table *t, const char *key, uint32_t len);

unsigned hash_get_num_entries(struct hash_table *t);
void hash_iter(struct hash_table *t, iterate_fn f);

char *hash_get_key(struct hash_table *t, struct hash_node *n);
void *hash_get_data(struct hash_table *t, struct hash_node *n);
struct hash_node *hash_get_first(struct hash_table *t);
struct hash_node *hash_get_next(struct hash_table *t, struct hash_node *n);

#define hash_iterate(v, h) \
	for (v = hash_get_first(h); v; \
	     v = hash_get_next(h, v))

#endif
