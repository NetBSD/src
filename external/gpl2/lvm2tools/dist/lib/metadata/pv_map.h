/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
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

#ifndef _LVM_PV_MAP_H
#define _LVM_PV_MAP_H

#include "metadata.h"

/*
 * The in core rep. only stores a mapping from
 * logical extents to physical extents against an
 * lv.  Sometimes, when allocating a new lv for
 * instance, it is useful to have the inverse
 * mapping available.
 */

struct pv_area {
	struct pv_map *map;
	uint32_t start;
	uint32_t count;

	struct list list;		/* pv_map.areas */
};

struct pv_map {
	struct physical_volume *pv;
	struct list areas;		/* struct pv_areas */
	uint32_t pe_count;		/* Total number of PEs */

	struct list list;
};

/*
 * Find intersection between available_pvs and free space in VG
 */
struct list *create_pv_maps(struct dm_pool *mem, struct volume_group *vg,
			    struct list *allocatable_pvs);

void consume_pv_area(struct pv_area *area, uint32_t to_go);

uint32_t pv_maps_size(struct list *pvms);

#endif
