/*	$NetBSD: str_list.h,v 1.1.1.1 2008/12/22 00:18:47 haad Exp $	*/

/*
 * Copyright (C) 2003-2004 Sistina Software, Inc. All rights reserved.  
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

#ifndef _LVM_STR_LIST_H
#define _LVM_STR_LIST_H

struct dm_list *str_list_create(struct dm_pool *mem);
int str_list_add(struct dm_pool *mem, struct dm_list *sll, const char *str);
int str_list_del(struct dm_list *sll, const char *str);
int str_list_match_item(const struct dm_list *sll, const char *str);
int str_list_match_list(const struct dm_list *sll, const struct dm_list *sll2);
int str_list_lists_equal(const struct dm_list *sll, const struct dm_list *sll2);
int str_list_dup(struct dm_pool *mem, struct dm_list *sllnew,
		 const struct dm_list *sllold);

#endif
