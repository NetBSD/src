/*	$NetBSD: import-extents.c,v 1.1.1.2 2009/12/02 00:26:49 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
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
#include "disk-rep.h"
#include "lv_alloc.h"
#include "display.h"
#include "segtype.h"

/*
 * After much thought I have decided it is easier,
 * and probably no less efficient, to convert the
 * pe->le map to a full le->pe map, and then
 * process this to get the segments form that
 * we're after.  Any code which goes directly from
 * the pe->le map to segments would be gladly
 * accepted, if it is less complicated than this
 * file.
 */
struct pe_specifier {
	struct physical_volume *pv;
	uint32_t pe;
};

struct lv_map {
	struct logical_volume *lv;
	uint32_t stripes;
	uint32_t stripe_size;
	struct pe_specifier *map;
};

static struct dm_hash_table *_create_lv_maps(struct dm_pool *mem,
					  struct volume_group *vg)
{
	struct dm_hash_table *maps = dm_hash_create(32);
	struct lv_list *ll;
	struct lv_map *lvm;

	if (!maps) {
		log_error("Unable to create hash table for holding "
			  "extent maps.");
		return NULL;
	}

	dm_list_iterate_items(ll, &vg->lvs) {
		if (ll->lv->status & SNAPSHOT)
			continue;

		if (!(lvm = dm_pool_alloc(mem, sizeof(*lvm))))
			goto_bad;

		lvm->lv = ll->lv;
		if (!(lvm->map = dm_pool_zalloc(mem, sizeof(*lvm->map)
					     * ll->lv->le_count)))
			goto_bad;

		if (!dm_hash_insert(maps, ll->lv->name, lvm))
			goto_bad;
	}

	return maps;

      bad:
	dm_hash_destroy(maps);
	return NULL;
}

static int _fill_lv_array(struct lv_map **lvs,
			  struct dm_hash_table *maps, struct disk_list *dl)
{
	struct lvd_list *ll;
	struct lv_map *lvm;

	memset(lvs, 0, sizeof(*lvs) * MAX_LV);

	dm_list_iterate_items(ll, &dl->lvds) {
		if (!(lvm = dm_hash_lookup(maps, strrchr((char *)ll->lvd.lv_name, '/')
					+ 1))) {
			log_error("Physical volume (%s) contains an "
				  "unknown logical volume (%s).",
				dev_name(dl->dev), ll->lvd.lv_name);
			return 0;
		}

		lvm->stripes = ll->lvd.lv_stripes;
		lvm->stripe_size = ll->lvd.lv_stripesize;

		lvs[ll->lvd.lv_number] = lvm;
	}

	return 1;
}

static int _fill_maps(struct dm_hash_table *maps, struct volume_group *vg,
		      struct dm_list *pvds)
{
	struct disk_list *dl;
	struct physical_volume *pv;
	struct lv_map *lvms[MAX_LV], *lvm;
	struct pe_disk *e;
	uint32_t i, lv_num, le;

	dm_list_iterate_items(dl, pvds) {
		pv = find_pv(vg, dl->dev);
		e = dl->extents;

		/* build an array of lv's for this pv */
		if (!_fill_lv_array(lvms, maps, dl))
			return_0;

		for (i = 0; i < dl->pvd.pe_total; i++) {
			lv_num = e[i].lv_num;

			if (lv_num == UNMAPPED_EXTENT)
				continue;

			else {
				lv_num--;
				lvm = lvms[lv_num];

				if (!lvm) {
					log_error("Invalid LV in extent map "
						  "(PV %s, PE %" PRIu32
						  ", LV %" PRIu32
						  ", LE %" PRIu32 ")",
						  dev_name(pv->dev), i,
						  lv_num, e[i].le_num);
					return 0;
				}

				le = e[i].le_num;

				if (le >= lvm->lv->le_count) {
					log_error("logical extent number "
						  "out of bounds");
					return 0;
				}

				if (lvm->map[le].pv) {
					log_error("logical extent (%u) "
						  "already mapped.", le);
					return 0;
				}

				lvm->map[le].pv = pv;
				lvm->map[le].pe = i;
			}
		}
	}

	return 1;
}

static int _check_single_map(struct lv_map *lvm)
{
	uint32_t i;

	for (i = 0; i < lvm->lv->le_count; i++) {
		if (!lvm->map[i].pv) {
			log_error("Logical volume (%s) contains an incomplete "
				  "mapping table.", lvm->lv->name);
			return 0;
		}
	}

	return 1;
}

static int _check_maps_are_complete(struct dm_hash_table *maps)
{
	struct dm_hash_node *n;
	struct lv_map *lvm;

	for (n = dm_hash_get_first(maps); n; n = dm_hash_get_next(maps, n)) {
		lvm = (struct lv_map *) dm_hash_get_data(maps, n);

		if (!_check_single_map(lvm))
			return_0;
	}
	return 1;
}

static uint32_t _area_length(struct lv_map *lvm, uint32_t le)
{
	uint32_t len = 0;

	do
		len++;
	while ((lvm->map[le + len].pv == lvm->map[le].pv) &&
		 (lvm->map[le].pv &&
		  lvm->map[le + len].pe == lvm->map[le].pe + len));

	return len;
}

static int _read_linear(struct cmd_context *cmd, struct lv_map *lvm)
{
	uint32_t le = 0, len;
	struct lv_segment *seg;
	struct segment_type *segtype;

	if (!(segtype = get_segtype_from_string(cmd, "striped")))
		return_0;

	while (le < lvm->lv->le_count) {
		len = _area_length(lvm, le);

		if (!(seg = alloc_lv_segment(cmd->mem, segtype, lvm->lv, le,
					     len, 0, 0, NULL, 1, len, 0, 0, 0))) {
			log_error("Failed to allocate linear segment.");
			return 0;
		}

		if (!set_lv_segment_area_pv(seg, 0, lvm->map[le].pv,
					    lvm->map[le].pe))
			return_0;

		dm_list_add(&lvm->lv->segments, &seg->list);

		le += seg->len;
	}

	return 1;
}

static int _check_stripe(struct lv_map *lvm, uint32_t area_count,
			 uint32_t area_len, uint32_t base_le,
			 uint32_t total_area_len)
{
	uint32_t st;

	/*
	 * Is the next physical extent in every stripe adjacent to the last?
	 */
	for (st = 0; st < area_count; st++)
		if ((lvm->map[base_le + st * total_area_len + area_len].pv !=
		     lvm->map[base_le + st * total_area_len].pv) ||
		    (lvm->map[base_le + st * total_area_len].pv &&
		     lvm->map[base_le + st * total_area_len + area_len].pe !=
		     lvm->map[base_le + st * total_area_len].pe + area_len))
			return 0;

	return 1;
}

static int _read_stripes(struct cmd_context *cmd, struct lv_map *lvm)
{
	uint32_t st, first_area_le = 0, total_area_len;
	uint32_t area_len;
	struct lv_segment *seg;
	struct segment_type *segtype;

	/*
	 * Work out overall striped length
	 */
	if (lvm->lv->le_count % lvm->stripes) {
		log_error("Number of stripes (%u) incompatible "
			  "with logical extent count (%u) for %s",
			  lvm->stripes, lvm->lv->le_count, lvm->lv->name);
	}

	total_area_len = lvm->lv->le_count / lvm->stripes;

	if (!(segtype = get_segtype_from_string(cmd, "striped")))
		return_0;

	while (first_area_le < total_area_len) {
		area_len = 1;

		/*
		 * Find how many extents are contiguous in all stripes
		 * and so can form part of this segment
		 */
		while (_check_stripe(lvm, lvm->stripes,
				     area_len, first_area_le, total_area_len))
			area_len++;

		if (!(seg = alloc_lv_segment(cmd->mem, segtype, lvm->lv,
					     lvm->stripes * first_area_le,
					     lvm->stripes * area_len,
					     0, lvm->stripe_size, NULL,
					     lvm->stripes,
					     area_len, 0, 0, 0))) {
			log_error("Failed to allocate striped segment.");
			return 0;
		}

		/*
		 * Set up start positions of each stripe in this segment
		 */
		for (st = 0; st < seg->area_count; st++)
			if (!set_lv_segment_area_pv(seg, st,
			      lvm->map[first_area_le + st * total_area_len].pv,
			      lvm->map[first_area_le + st * total_area_len].pe))
				return_0;

		dm_list_add(&lvm->lv->segments, &seg->list);

		first_area_le += area_len;
	}

	return 1;
}

static int _build_segments(struct cmd_context *cmd, struct lv_map *lvm)
{
	return (lvm->stripes > 1 ? _read_stripes(cmd, lvm) :
		_read_linear(cmd, lvm));
}

static int _build_all_segments(struct cmd_context *cmd, struct dm_hash_table *maps)
{
	struct dm_hash_node *n;
	struct lv_map *lvm;

	for (n = dm_hash_get_first(maps); n; n = dm_hash_get_next(maps, n)) {
		lvm = (struct lv_map *) dm_hash_get_data(maps, n);
		if (!_build_segments(cmd, lvm))
			return_0;
	}

	return 1;
}

int import_extents(struct cmd_context *cmd, struct volume_group *vg,
		   struct dm_list *pvds)
{
	int r = 0;
	struct dm_pool *scratch = dm_pool_create("lvm1 import_extents", 10 * 1024);
	struct dm_hash_table *maps;

	if (!scratch)
		return_0;

	if (!(maps = _create_lv_maps(scratch, vg))) {
		log_error("Couldn't allocate logical volume maps.");
		goto out;
	}

	if (!_fill_maps(maps, vg, pvds)) {
		log_error("Couldn't fill logical volume maps.");
		goto out;
	}

	if (!_check_maps_are_complete(maps) && !(vg->status & PARTIAL_VG))
		goto_out;

	if (!_build_all_segments(cmd, maps)) {
		log_error("Couldn't build extent segments.");
		goto out;
	}
	r = 1;

      out:
	if (maps)
		dm_hash_destroy(maps);
	dm_pool_destroy(scratch);
	return r;
}
