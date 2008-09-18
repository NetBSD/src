/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2005 Red Hat, Inc. All rights reserved.
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

#ifndef _LVM_BTREE_H
#define _LVM_BTREE_H

struct btree;

struct btree *btree_create(struct dm_pool *mem);

void *btree_lookup(const struct btree *t, uint32_t k);
int btree_insert(struct btree *t, uint32_t k, void *data);

struct btree_iter;
void *btree_get_data(const struct btree_iter *it);

struct btree_iter *btree_first(const struct btree *t);
struct btree_iter *btree_next(const struct btree_iter *it);

#endif
