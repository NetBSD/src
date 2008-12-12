/*	$NetBSD: targets.h,v 1.1.1.2 2008/12/12 11:42:24 haad Exp $	*/

/*
 * Copyright (C) 2003-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2006 Red Hat, Inc. All rights reserved.
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

#ifndef _LVM_TARGETS_H
#define _LVM_TARGETS_H

struct dev_manager;
struct lv_segment;

int compose_areas_line(struct dev_manager *dm, struct lv_segment *seg,
		       char *params, size_t paramsize, int *pos,
		       int start_area, int areas);

int add_areas_line(struct dev_manager *dm, struct lv_segment *seg,
                   struct dm_tree_node *node, uint32_t start_area, uint32_t areas);

int build_dev_string(struct dev_manager *dm, char *dlid, char *devbuf,
		     size_t bufsize, const char *desc);

char *build_dlid(struct dev_manager *dm, const char *lvid, const char *layer);

#endif
