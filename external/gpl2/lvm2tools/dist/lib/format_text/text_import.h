/*	$NetBSD: text_import.h,v 1.1.1.1.2.3 2008/12/13 14:39:34 haad Exp $	*/

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

#ifndef _LVM_TEXT_IMPORT_H
#define _LVM_TEXT_IMPORT_H

struct lv_segment;
struct config_node;

int text_import_areas(struct lv_segment *seg, const struct config_node *sn,
		      const struct config_node *cn, struct dm_hash_table *pv_hash,
		      uint32_t flags);

#endif
