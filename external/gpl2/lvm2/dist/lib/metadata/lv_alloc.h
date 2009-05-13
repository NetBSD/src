/*	$NetBSD: lv_alloc.h,v 1.1.1.1.2.1 2009/05/13 18:52:43 jym Exp $	*/

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

#ifndef _LVM_LV_ALLOC_H

struct lv_segment *alloc_lv_segment(struct dm_pool *mem,
				    const struct segment_type *segtype,
				    struct logical_volume *lv,
				    uint32_t le, uint32_t len,
				    uint32_t status,
				    uint32_t stripe_size,
				    struct logical_volume *log_lv,
				    uint32_t area_count,
				    uint32_t area_len,
				    uint32_t chunk_size,
				    uint32_t region_size,
				    uint32_t extents_copied);

struct lv_segment *alloc_snapshot_seg(struct logical_volume *lv,
				      uint32_t status, uint32_t old_le_count);

int set_lv_segment_area_pv(struct lv_segment *seg, uint32_t area_num,
			   struct physical_volume *pv, uint32_t pe);
int set_lv_segment_area_lv(struct lv_segment *seg, uint32_t area_num,
			   struct logical_volume *lv, uint32_t le,
			   uint32_t flags);
int move_lv_segment_area(struct lv_segment *seg_to, uint32_t area_to,
			 struct lv_segment *seg_from, uint32_t area_from);
void release_lv_segment_area(struct lv_segment *seg, uint32_t s,
			     uint32_t area_reduction);

struct alloc_handle;
struct alloc_handle *allocate_extents(struct volume_group *vg,
				      struct logical_volume *lv,
                                      const struct segment_type *segtype,
                                      uint32_t stripes,
                                      uint32_t mirrors, uint32_t log_count,
				      uint32_t log_region_size, uint32_t extents,
                                      struct dm_list *allocatable_pvs,
				      alloc_policy_t alloc,
				      struct dm_list *parallel_areas);

int lv_add_segment(struct alloc_handle *ah,
		   uint32_t first_area, uint32_t num_areas,
		   struct logical_volume *lv,
                   const struct segment_type *segtype,
                   uint32_t stripe_size,
                   uint32_t status,   
		   uint32_t region_size,
                   struct logical_volume *log_lv);

int lv_add_mirror_areas(struct alloc_handle *ah,
			struct logical_volume *lv, uint32_t le,
			uint32_t region_size);
int lv_add_mirror_lvs(struct logical_volume *lv,
		      struct logical_volume **sub_lvs,
		      uint32_t num_extra_areas,
		      uint32_t status, uint32_t region_size);

int lv_add_log_segment(struct alloc_handle *ah, struct logical_volume *log_lv);
int lv_add_virtual_segment(struct logical_volume *lv, uint32_t status,
                           uint32_t extents, const struct segment_type *segtype);

void alloc_destroy(struct alloc_handle *ah);

struct dm_list *build_parallel_areas_from_lv(struct cmd_context *cmd,
					  struct logical_volume *lv);

#endif
