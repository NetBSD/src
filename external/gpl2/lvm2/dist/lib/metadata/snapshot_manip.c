/*	$NetBSD: snapshot_manip.c,v 1.1.1.1.2.1 2009/05/13 18:52:43 jym Exp $	*/

/*
 * Copyright (C) 2002-2004 Sistina Software, Inc. All rights reserved.
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

#include "lib.h"
#include "metadata.h"
#include "toolcontext.h"
#include "lv_alloc.h"

int lv_is_origin(const struct logical_volume *lv)
{
	return lv->origin_count ? 1 : 0;
}

int lv_is_cow(const struct logical_volume *lv)
{
	return lv->snapshot ? 1 : 0;
}

int lv_is_visible(const struct logical_volume *lv)
{
	if (lv_is_cow(lv))
		return lv_is_visible(find_cow(lv)->lv);

	return lv->status & VISIBLE_LV ? 1 : 0;
}

int lv_is_displayable(const struct logical_volume *lv)
{
	if (lv->status & SNAPSHOT)
		return 0;

	return (lv->status & VISIBLE_LV) || lv_is_cow(lv) ? 1 : 0;
}

/* Given a cow LV, return the snapshot lv_segment that uses it */
struct lv_segment *find_cow(const struct logical_volume *lv)
{
	return lv->snapshot;
}

/* Given a cow LV, return its origin */
struct logical_volume *origin_from_cow(const struct logical_volume *lv)
{
	return lv->snapshot->origin;
}

int vg_add_snapshot(const char *name, struct logical_volume *origin,
		    struct logical_volume *cow, union lvid *lvid,
		    uint32_t extent_count, uint32_t chunk_size)
{
	struct logical_volume *snap;
	struct lv_segment *seg;

	/*
	 * Is the cow device already being used ?
	 */
	if (lv_is_cow(cow)) {
		log_err("'%s' is already in use as a snapshot.", cow->name);
		return 0;
	}

	if (cow == origin) {
		log_error("Snapshot and origin LVs must differ.");
		return 0;
	}

	if (!(snap = lv_create_empty(name ? name : "snapshot%d",
				     lvid, LVM_READ | LVM_WRITE | VISIBLE_LV,
				     ALLOC_INHERIT, 1, origin->vg)))
		return_0;

	snap->le_count = extent_count;

	if (!(seg = alloc_snapshot_seg(snap, 0, 0)))
		return_0;

	seg->chunk_size = chunk_size;
	seg->origin = origin;
	seg->cow = cow;
	seg->lv->status |= SNAPSHOT;

	origin->origin_count++;
	origin->vg->snapshot_count++;
	origin->vg->lv_count--;
	cow->snapshot = seg;

	cow->status &= ~VISIBLE_LV;

	dm_list_add(&origin->snapshot_segs, &seg->origin_list);

	return 1;
}

int vg_remove_snapshot(struct logical_volume *cow)
{
	dm_list_del(&cow->snapshot->origin_list);
	cow->snapshot->origin->origin_count--;

	if (!lv_remove(cow->snapshot->lv)) {
		log_error("Failed to remove internal snapshot LV %s",
			  cow->snapshot->lv->name);
		return 0;
	}

	cow->snapshot = NULL;

	cow->vg->snapshot_count--;
	cow->vg->lv_count++;
	cow->status |= VISIBLE_LV;

	return 1;
}
