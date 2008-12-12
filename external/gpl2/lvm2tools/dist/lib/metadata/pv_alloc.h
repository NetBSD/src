/*	$NetBSD: pv_alloc.h,v 1.1.1.2 2008/12/12 11:42:35 haad Exp $	*/

/*
 * Copyright (C) 2005 Red Hat, Inc. All rights reserved.
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

#ifndef _LVM_PV_ALLOC_H

int alloc_pv_segment_whole_pv(struct dm_pool *mem, struct physical_volume *pv);
int peg_dup(struct dm_pool *mem, struct dm_list *peg_new, struct dm_list *peg_old);
struct pv_segment *assign_peg_to_lvseg(struct physical_volume *pv, uint32_t pe,
				       uint32_t area_len,
				       struct lv_segment *seg,
				       uint32_t area_num);
int pv_split_segment(struct physical_volume *pv, uint32_t pe);
int release_pv_segment(struct pv_segment *peg, uint32_t area_reduction);
int check_pv_segments(struct volume_group *vg);
void merge_pv_segments(struct pv_segment *peg1, struct pv_segment *peg2);

#endif
