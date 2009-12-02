/*	$NetBSD: lv_manip.c,v 1.4 2009/12/02 00:58:03 haad Exp $	*/

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
#include "locking.h"
#include "pv_map.h"
#include "lvm-string.h"
#include "toolcontext.h"
#include "lv_alloc.h"
#include "pv_alloc.h"
#include "display.h"
#include "segtype.h"
#include "archiver.h"
#include "activate.h"
#include "str_list.h"

struct lv_names {
	const char *old;
	const char *new;
};

int add_seg_to_segs_using_this_lv(struct logical_volume *lv,
				  struct lv_segment *seg)
{
	struct seg_list *sl;

	dm_list_iterate_items(sl, &lv->segs_using_this_lv) {
		if (sl->seg == seg) {
			sl->count++;
			return 1;
		}
	}

	log_very_verbose("Adding %s:%" PRIu32 " as an user of %s",
			 seg->lv->name, seg->le, lv->name);

	if (!(sl = dm_pool_zalloc(lv->vg->cmd->mem, sizeof(*sl)))) {
		log_error("Failed to allocate segment list");
		return 0;
	}

	sl->count = 1;
	sl->seg = seg;
	dm_list_add(&lv->segs_using_this_lv, &sl->list);

	return 1;
}

int remove_seg_from_segs_using_this_lv(struct logical_volume *lv,
				       struct lv_segment *seg)
{
	struct seg_list *sl;

	dm_list_iterate_items(sl, &lv->segs_using_this_lv) {
		if (sl->seg != seg)
			continue;
		if (sl->count > 1)
			sl->count--;
		else {
			log_very_verbose("%s:%" PRIu32 " is no longer a user "
					 "of %s", seg->lv->name, seg->le,
					 lv->name);
			dm_list_del(&sl->list);
		}
		return 1;
	}

	return 0;
}

/*
 * This is a function specialized for the common case where there is
 * only one segment which uses the LV.
 * e.g. the LV is a layer inserted by insert_layer_for_lv().
 *
 * In general, walk through lv->segs_using_this_lv.
 */
struct lv_segment *get_only_segment_using_this_lv(struct logical_volume *lv)
{
	struct seg_list *sl;

	if (dm_list_size(&lv->segs_using_this_lv) != 1) {
		log_error("%s is expected to have only one segment using it, "
			  "while it has %d", lv->name,
			  dm_list_size(&lv->segs_using_this_lv));
		return NULL;
	}

	sl = dm_list_item(dm_list_first(&lv->segs_using_this_lv), struct seg_list);

	if (sl->count != 1) {
		log_error("%s is expected to have only one segment using it, "
			  "while %s:%" PRIu32 " uses it %d times",
			  lv->name, sl->seg->lv->name, sl->seg->le, sl->count);
		return NULL;
	}

	return sl->seg;
}

/*
 * PVs used by a segment of an LV
 */
struct seg_pvs {
	struct dm_list list;

	struct dm_list pvs;	/* struct pv_list */

	uint32_t le;
	uint32_t len;
};

static struct seg_pvs *_find_seg_pvs_by_le(struct dm_list *list, uint32_t le)
{
	struct seg_pvs *spvs;

	dm_list_iterate_items(spvs, list)
		if (le >= spvs->le && le < spvs->le + spvs->len)
			return spvs;

	return NULL;
}

/*
 * Find first unused LV number.
 */
uint32_t find_free_lvnum(struct logical_volume *lv)
{
	int lvnum_used[MAX_RESTRICTED_LVS + 1];
	uint32_t i = 0;
	struct lv_list *lvl;
	int lvnum;

	memset(&lvnum_used, 0, sizeof(lvnum_used));

	dm_list_iterate_items(lvl, &lv->vg->lvs) {
		lvnum = lvnum_from_lvid(&lvl->lv->lvid);
		if (lvnum <= MAX_RESTRICTED_LVS)
			lvnum_used[lvnum] = 1;
	}

	while (lvnum_used[i])
		i++;

	/* FIXME What if none are free? */

	return i;
}

/*
 * All lv_segments get created here.
 */
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
				    uint32_t extents_copied)
{
	struct lv_segment *seg;
	uint32_t areas_sz = area_count * sizeof(*seg->areas);

	if (!(seg = dm_pool_zalloc(mem, sizeof(*seg))))
		return_NULL;

	if (!(seg->areas = dm_pool_zalloc(mem, areas_sz))) {
		dm_pool_free(mem, seg);
		return_NULL;
	}

	if (!segtype) {
		log_error("alloc_lv_segment: Missing segtype.");
		return NULL;
	}

	seg->segtype = segtype;
	seg->lv = lv;
	seg->le = le;
	seg->len = len;
	seg->status = status;
	seg->stripe_size = stripe_size;
	seg->area_count = area_count;
	seg->area_len = area_len;
	seg->chunk_size = chunk_size;
	seg->region_size = region_size;
	seg->extents_copied = extents_copied;
	seg->log_lv = log_lv;
	dm_list_init(&seg->tags);

	if (log_lv && !attach_mirror_log(seg, log_lv))
		return_NULL;

	return seg;
}

struct lv_segment *alloc_snapshot_seg(struct logical_volume *lv,
				      uint32_t status, uint32_t old_le_count)
{
	struct lv_segment *seg;
	const struct segment_type *segtype;

	segtype = get_segtype_from_string(lv->vg->cmd, "snapshot");
	if (!segtype) {
		log_error("Failed to find snapshot segtype");
		return NULL;
	}

	if (!(seg = alloc_lv_segment(lv->vg->cmd->mem, segtype, lv, old_le_count,
				     lv->le_count - old_le_count, status, 0,
				     NULL, 0, lv->le_count - old_le_count,
				     0, 0, 0))) {
		log_error("Couldn't allocate new snapshot segment.");
		return NULL;
	}

	dm_list_add(&lv->segments, &seg->list);
	lv->status |= VIRTUAL;

	return seg;
}

void release_lv_segment_area(struct lv_segment *seg, uint32_t s,
			     uint32_t area_reduction)
{
	if (seg_type(seg, s) == AREA_UNASSIGNED)
		return;

	if (seg_type(seg, s) == AREA_PV) {
		if (release_pv_segment(seg_pvseg(seg, s), area_reduction) &&
		    seg->area_len == area_reduction)
			seg_type(seg, s) = AREA_UNASSIGNED;
		return;
	}

	if (seg_lv(seg, s)->status & MIRROR_IMAGE) {
		lv_reduce(seg_lv(seg, s), area_reduction);
		return;
	}

	if (area_reduction == seg->area_len) {
		log_very_verbose("Remove %s:%" PRIu32 "[%" PRIu32 "] from "
				 "the top of LV %s:%" PRIu32,
				 seg->lv->name, seg->le, s,
				 seg_lv(seg, s)->name, seg_le(seg, s));

		remove_seg_from_segs_using_this_lv(seg_lv(seg, s), seg);
		seg_lv(seg, s) = NULL;
		seg_le(seg, s) = 0;
		seg_type(seg, s) = AREA_UNASSIGNED;
	}
}

/*
 * Move a segment area from one segment to another
 */
int move_lv_segment_area(struct lv_segment *seg_to, uint32_t area_to,
			 struct lv_segment *seg_from, uint32_t area_from)
{
	struct physical_volume *pv;
	struct logical_volume *lv;
	uint32_t pe, le;

	switch (seg_type(seg_from, area_from)) {
	case AREA_PV:
		pv = seg_pv(seg_from, area_from);
		pe = seg_pe(seg_from, area_from);

		release_lv_segment_area(seg_from, area_from,
					seg_from->area_len);
		release_lv_segment_area(seg_to, area_to, seg_to->area_len);

		if (!set_lv_segment_area_pv(seg_to, area_to, pv, pe))
			return_0;

		break;

	case AREA_LV:
		lv = seg_lv(seg_from, area_from);
		le = seg_le(seg_from, area_from);

		release_lv_segment_area(seg_from, area_from,
					seg_from->area_len);
		release_lv_segment_area(seg_to, area_to, seg_to->area_len);

		if (!set_lv_segment_area_lv(seg_to, area_to, lv, le, 0))
			return_0;

		break;

	case AREA_UNASSIGNED:
		release_lv_segment_area(seg_to, area_to, seg_to->area_len);
	}

	return 1;
}

/*
 * Link part of a PV to an LV segment.
 */
int set_lv_segment_area_pv(struct lv_segment *seg, uint32_t area_num,
			   struct physical_volume *pv, uint32_t pe)
{
	seg->areas[area_num].type = AREA_PV;

	if (!(seg_pvseg(seg, area_num) =
	      assign_peg_to_lvseg(pv, pe, seg->area_len, seg, area_num)))
		return_0;

	return 1;
}

/*
 * Link one LV segment to another.  Assumes sizes already match.
 */
int set_lv_segment_area_lv(struct lv_segment *seg, uint32_t area_num,
			   struct logical_volume *lv, uint32_t le,
			   uint32_t flags)
{
	log_very_verbose("Stack %s:%" PRIu32 "[%" PRIu32 "] on LV %s:%" PRIu32,
			 seg->lv->name, seg->le, area_num, lv->name, le);

	seg->areas[area_num].type = AREA_LV;
	seg_lv(seg, area_num) = lv;
	seg_le(seg, area_num) = le;
	lv->status |= flags;

	if (!add_seg_to_segs_using_this_lv(lv, seg))
		return_0;

	return 1;
}

/*
 * Prepare for adding parallel areas to an existing segment.
 */
static int _lv_segment_add_areas(struct logical_volume *lv,
				 struct lv_segment *seg,
				 uint32_t new_area_count)
{
	struct lv_segment_area *newareas;
	uint32_t areas_sz = new_area_count * sizeof(*newareas);

	if (!(newareas = dm_pool_zalloc(lv->vg->cmd->mem, areas_sz)))
		return_0;

	memcpy(newareas, seg->areas, seg->area_count * sizeof(*seg->areas));

	seg->areas = newareas;
	seg->area_count = new_area_count;

	return 1;
}

/*
 * Reduce the size of an lv_segment.  New size can be zero.
 */
static int _lv_segment_reduce(struct lv_segment *seg, uint32_t reduction)
{
	uint32_t area_reduction, s;

	/* Caller must ensure exact divisibility */
	if (seg_is_striped(seg)) {
		if (reduction % seg->area_count) {
			log_error("Segment extent reduction %" PRIu32
				  "not divisible by #stripes %" PRIu32,
				  reduction, seg->area_count);
			return 0;
		}
		area_reduction = (reduction / seg->area_count);
	} else
		area_reduction = reduction;

	for (s = 0; s < seg->area_count; s++)
		release_lv_segment_area(seg, s, area_reduction);

	seg->len -= reduction;
	seg->area_len -= area_reduction;

	return 1;
}

/*
 * Entry point for all LV reductions in size.
 */
static int _lv_reduce(struct logical_volume *lv, uint32_t extents, int delete)
{
	struct lv_segment *seg;
	uint32_t count = extents;
	uint32_t reduction;

	dm_list_iterate_back_items(seg, &lv->segments) {
		if (!count)
			break;

		if (seg->len <= count) {
			/* remove this segment completely */
			/* FIXME Check this is safe */
			if (seg->log_lv && !lv_remove(seg->log_lv))
				return_0;
			dm_list_del(&seg->list);
			reduction = seg->len;
		} else
			reduction = count;

		if (!_lv_segment_reduce(seg, reduction))
			return_0;
		count -= reduction;
	}

	lv->le_count -= extents;
	lv->size = (uint64_t) lv->le_count * lv->vg->extent_size;

	if (!delete)
		return 1;

	/* Remove the LV if it is now empty */
	if (!lv->le_count && !unlink_lv_from_vg(lv))
		return_0;
	else if (lv->vg->fid->fmt->ops->lv_setup &&
		   !lv->vg->fid->fmt->ops->lv_setup(lv->vg->fid, lv))
		return_0;

	return 1;
}

/*
 * Empty an LV.
 */
int lv_empty(struct logical_volume *lv)
{
	return _lv_reduce(lv, lv->le_count, 0);
}

/*
 * Empty an LV and add error segment.
 */
int replace_lv_with_error_segment(struct logical_volume *lv)
{
	uint32_t len = lv->le_count;

	if (!lv_empty(lv))
		return_0;

	if (!lv_add_virtual_segment(lv, 0, len,
				    get_segtype_from_string(lv->vg->cmd,
							    "error")))
		return_0;

	return 1;
}

/*
 * Remove given number of extents from LV.
 */
int lv_reduce(struct logical_volume *lv, uint32_t extents)
{
	return _lv_reduce(lv, extents, 1);
}

/*
 * Completely remove an LV.
 */
int lv_remove(struct logical_volume *lv)
{

	if (!lv_reduce(lv, lv->le_count))
		return_0;

	return 1;
}

/*
 * A set of contiguous physical extents allocated
 */
struct alloced_area {
	struct dm_list list;

	struct physical_volume *pv;
	uint32_t pe;
	uint32_t len;
};

/*
 * Details of an allocation attempt
 */
struct alloc_handle {
	struct cmd_context *cmd;
	struct dm_pool *mem;

	alloc_policy_t alloc;		/* Overall policy */
	uint32_t area_count;		/* Number of parallel areas */
	uint32_t area_multiple;		/* seg->len = area_len * area_multiple */
	uint32_t log_count;		/* Number of parallel 1-extent logs */
	uint32_t log_region_size;	/* region size for log device */
	uint32_t total_area_len;	/* Total number of parallel extents */

	struct dm_list *parallel_areas;	/* PVs to avoid */

	struct alloced_area log_area;	/* Extent used for log */
	struct dm_list alloced_areas[0];	/* Lists of areas in each stripe */
};

static uint32_t calc_area_multiple(const struct segment_type *segtype,
				   const uint32_t area_count)
{
	if (!segtype_is_striped(segtype) || !area_count)
		return 1;

	return area_count;
}

/*
 * Preparation for a specific allocation attempt
 */
static struct alloc_handle *_alloc_init(struct cmd_context *cmd,
					struct dm_pool *mem,
					const struct segment_type *segtype,
					alloc_policy_t alloc,
					uint32_t mirrors,
					uint32_t stripes,
					uint32_t log_count,
					uint32_t log_region_size,
					struct dm_list *parallel_areas)
{
	struct alloc_handle *ah;
	uint32_t s, area_count;

	if (stripes > 1 && mirrors > 1) {
		log_error("Striped mirrors are not supported yet");
		return NULL;
	}

	if (log_count && stripes > 1) {
		log_error("Can't mix striping with a mirror log yet.");
		return NULL;
	}

	if (segtype_is_virtual(segtype))
		area_count = 0;
	else if (mirrors > 1)
		area_count = mirrors;
	else
		area_count = stripes;

	if (!(ah = dm_pool_zalloc(mem, sizeof(*ah) + sizeof(ah->alloced_areas[0]) * area_count))) {
		log_error("allocation handle allocation failed");
		return NULL;
	}

	if (segtype_is_virtual(segtype))
		return ah;

	ah->cmd = cmd;

	if (!(ah->mem = dm_pool_create("allocation", 1024))) {
		log_error("allocation pool creation failed");
		return NULL;
	}

	ah->area_count = area_count;
	ah->log_count = log_count;
	ah->log_region_size = log_region_size;
	ah->alloc = alloc;
	ah->area_multiple = calc_area_multiple(segtype, area_count);

	for (s = 0; s < ah->area_count; s++)
		dm_list_init(&ah->alloced_areas[s]);

	ah->parallel_areas = parallel_areas;

	return ah;
}

void alloc_destroy(struct alloc_handle *ah)
{
	if (ah->mem)
		dm_pool_destroy(ah->mem);
}

static int _log_parallel_areas(struct dm_pool *mem, struct dm_list *parallel_areas)
{
	struct seg_pvs *spvs;
	struct pv_list *pvl;
	char *pvnames;

	if (!parallel_areas)
		return 1;

	dm_list_iterate_items(spvs, parallel_areas) {
		if (!dm_pool_begin_object(mem, 256)) {
			log_error("dm_pool_begin_object failed");
			return 0;
		}

		dm_list_iterate_items(pvl, &spvs->pvs) {
			if (!dm_pool_grow_object(mem, pv_dev_name(pvl->pv), strlen(pv_dev_name(pvl->pv)))) {
				log_error("dm_pool_grow_object failed");
				dm_pool_abandon_object(mem);
				return 0;
			}
			if (!dm_pool_grow_object(mem, " ", 1)) {
				log_error("dm_pool_grow_object failed");
				dm_pool_abandon_object(mem);
				return 0;
			}
		}

		if (!dm_pool_grow_object(mem, "\0", 1)) {
			log_error("dm_pool_grow_object failed");
			dm_pool_abandon_object(mem);
			return 0;
		}

		pvnames = dm_pool_end_object(mem);
		log_debug("Parallel PVs at LE %" PRIu32 " length %" PRIu32 ": %s",
			  spvs->le, spvs->len, pvnames);
		dm_pool_free(mem, pvnames);
	}

	return 1;
}

static int _setup_alloced_segment(struct logical_volume *lv, uint32_t status,
				  uint32_t area_count,
				  uint32_t stripe_size,
				  const struct segment_type *segtype,
				  struct alloced_area *aa,
				  uint32_t region_size,
				  struct logical_volume *log_lv __attribute((unused)))
{
	uint32_t s, extents, area_multiple;
	struct lv_segment *seg;

	area_multiple = calc_area_multiple(segtype, area_count);

	/* log_lv gets set up elsehere */
	if (!(seg = alloc_lv_segment(lv->vg->cmd->mem, segtype, lv,
				     lv->le_count,
				     aa[0].len * area_multiple,
				     status, stripe_size, NULL,
				     area_count,
				     aa[0].len, 0u, region_size, 0u))) {
		log_error("Couldn't allocate new LV segment.");
		return 0;
	}

	for (s = 0; s < area_count; s++)
		if (!set_lv_segment_area_pv(seg, s, aa[s].pv, aa[s].pe))
			return_0;

	dm_list_add(&lv->segments, &seg->list);

	extents = aa[0].len * area_multiple;
	lv->le_count += extents;
	lv->size += (uint64_t) extents *lv->vg->extent_size;

	if (segtype_is_mirrored(segtype))
		lv->status |= MIRRORED;

	return 1;
}

static int _setup_alloced_segments(struct logical_volume *lv,
				   struct dm_list *alloced_areas,
				   uint32_t area_count,
				   uint32_t status,
				   uint32_t stripe_size,
				   const struct segment_type *segtype,
				   uint32_t region_size,
				   struct logical_volume *log_lv)
{
	struct alloced_area *aa;

	dm_list_iterate_items(aa, &alloced_areas[0]) {
		if (!_setup_alloced_segment(lv, status, area_count,
					    stripe_size, segtype, aa,
					    region_size, log_lv))
			return_0;
	}

	return 1;
}

/*
 * Returns log device size in extents, algorithm from kernel code
 */
#define BYTE_SHIFT 3
static uint32_t mirror_log_extents(uint32_t region_size, uint32_t pe_size, uint32_t area_len)
{
	size_t area_size, bitset_size, log_size, region_count;

	area_size = area_len * pe_size;
	region_count = dm_div_up(area_size, region_size);

	/* Work out how many "unsigned long"s we need to hold the bitset. */
	bitset_size = dm_round_up(region_count, sizeof(uint32_t) << BYTE_SHIFT);
	bitset_size >>= BYTE_SHIFT;

	/* Log device holds both header and bitset. */
	log_size = dm_round_up((MIRROR_LOG_OFFSET << SECTOR_SHIFT) + bitset_size, 1 << SECTOR_SHIFT);
	log_size >>= SECTOR_SHIFT;

	return dm_div_up(log_size, pe_size);
}

/*
 * This function takes a list of pv_areas and adds them to allocated_areas.
 * If the complete area is not needed then it gets split.
 * The part used is removed from the pv_map so it can't be allocated twice.
 */
static int _alloc_parallel_area(struct alloc_handle *ah, uint32_t needed,
				struct pv_area **areas,
				uint32_t *ix, struct pv_area *log_area,
				uint32_t log_len)
{
	uint32_t area_len, remaining;
	uint32_t s;
	struct alloced_area *aa;

	remaining = needed - *ix;
	area_len = remaining / ah->area_multiple;

	/* Reduce area_len to the smallest of the areas */
	for (s = 0; s < ah->area_count; s++)
		if (area_len > areas[s]->count)
			area_len = areas[s]->count;

	if (!(aa = dm_pool_alloc(ah->mem, sizeof(*aa) *
			      (ah->area_count + (log_area ? 1 : 0))))) {
		log_error("alloced_area allocation failed");
		return 0;
	}

	for (s = 0; s < ah->area_count; s++) {
		aa[s].pv = areas[s]->map->pv;
		aa[s].pe = areas[s]->start;
		aa[s].len = area_len;
		dm_list_add(&ah->alloced_areas[s], &aa[s].list);
	}

	ah->total_area_len += area_len;

	for (s = 0; s < ah->area_count; s++)
		consume_pv_area(areas[s], area_len);

	if (log_area) {
		ah->log_area.pv = log_area->map->pv;
		ah->log_area.pe = log_area->start;
		ah->log_area.len = log_len;
		consume_pv_area(log_area, ah->log_area.len);
	}

	*ix += area_len * ah->area_multiple;

	return 1;
}

/*
 * Call fn for each AREA_PV used by the LV segment at lv:le of length *max_seg_len.
 * If any constituent area contains more than one segment, max_seg_len is
 * reduced to cover only the first.
 * fn should return 0 on error, 1 to continue scanning or >1 to terminate without error.
 * In the last case, this function passes on the return code.
 */
static int _for_each_pv(struct cmd_context *cmd, struct logical_volume *lv,
			uint32_t le, uint32_t len, uint32_t *max_seg_len,
			uint32_t first_area, uint32_t max_areas,
			int top_level_area_index,
			int only_single_area_segments,
			int (*fn)(struct cmd_context *cmd,
				  struct pv_segment *peg, uint32_t s,
				  void *data),
			void *data)
{
	struct lv_segment *seg;
	uint32_t s;
	uint32_t remaining_seg_len, area_len, area_multiple;
	int r = 1;

	if (!(seg = find_seg_by_le(lv, le))) {
		log_error("Failed to find segment for %s extent %" PRIu32,
			  lv->name, le);
		return 0;
	}

	/* Remaining logical length of segment */
	remaining_seg_len = seg->len - (le - seg->le);

	if (remaining_seg_len > len)
		remaining_seg_len = len;

	if (max_seg_len && *max_seg_len > remaining_seg_len)
		*max_seg_len = remaining_seg_len;

	area_multiple = calc_area_multiple(seg->segtype, seg->area_count);
	area_len = remaining_seg_len / area_multiple ? : 1;

	for (s = first_area;
	     s < seg->area_count && (!max_areas || s <= max_areas);
	     s++) {
		if (seg_type(seg, s) == AREA_LV) {
			if (!(r = _for_each_pv(cmd, seg_lv(seg, s),
					       seg_le(seg, s) +
					       (le - seg->le) / area_multiple,
					       area_len, max_seg_len,
					       only_single_area_segments ? 0 : 0,
					       only_single_area_segments ? 1U : 0U,
					       top_level_area_index != -1 ? top_level_area_index : (int) s,
					       only_single_area_segments, fn,
					       data)))
				stack;
		} else if (seg_type(seg, s) == AREA_PV)
			if (!(r = fn(cmd, seg_pvseg(seg, s), top_level_area_index != -1 ? (uint32_t) top_level_area_index : s, data)))
				stack;
		if (r != 1)
			return r;
	}

	/* FIXME only_single_area_segments used as workaround to skip log LV - needs new param? */
	if (!only_single_area_segments && seg_is_mirrored(seg) && seg->log_lv) {
		if (!(r = _for_each_pv(cmd, seg->log_lv, 0, seg->log_lv->le_count,
				       NULL, 0, 0, 0, only_single_area_segments,
				       fn, data)))
			stack;
		if (r != 1)
			return r;
	}

	/* FIXME Add snapshot cow LVs etc. */

	return 1;
}

static int _comp_area(const void *l, const void *r)
{
	const struct pv_area *lhs = *((const struct pv_area **) l);
	const struct pv_area *rhs = *((const struct pv_area **) r);

	if (lhs->count < rhs->count)
		return 1;

	else if (lhs->count > rhs->count)
		return -1;

	return 0;
}

/*
 * Search for pvseg that matches condition
 */
struct pv_match {
	int (*condition)(struct pv_segment *pvseg, struct pv_area *pva);

	struct pv_area **areas;
	struct pv_area *pva;
	uint32_t areas_size;
	int s;	/* Area index of match */
};

/*
 * Is PV area on the same PV?
 */
static int _is_same_pv(struct pv_segment *pvseg, struct pv_area *pva)
{
	if (pvseg->pv != pva->map->pv)
		return 0;

	return 1;
}

/*
 * Is PV area contiguous to PV segment?
 */
static int _is_contiguous(struct pv_segment *pvseg, struct pv_area *pva)
{
	if (pvseg->pv != pva->map->pv)
		return 0;

	if (pvseg->pe + pvseg->len != pva->start)
		return 0;

	return 1;
}

static int _is_condition(struct cmd_context *cmd __attribute((unused)),
			 struct pv_segment *pvseg, uint32_t s,
			 void *data)
{
	struct pv_match *pvmatch = data;

	if (!pvmatch->condition(pvseg, pvmatch->pva))
		return 1;	/* Continue */

	if (s >= pvmatch->areas_size)
		return 1;

	pvmatch->areas[s] = pvmatch->pva;

	return 2;	/* Finished */
}

/*
 * Is pva on same PV as any existing areas?
 */
static int _check_cling(struct cmd_context *cmd,
			struct lv_segment *prev_lvseg, struct pv_area *pva,
			struct pv_area **areas, uint32_t areas_size)
{
	struct pv_match pvmatch;
	int r;

	pvmatch.condition = _is_same_pv;
	pvmatch.areas = areas;
	pvmatch.areas_size = areas_size;
	pvmatch.pva = pva;

	/* FIXME Cope with stacks by flattening */
	if (!(r = _for_each_pv(cmd, prev_lvseg->lv,
			       prev_lvseg->le + prev_lvseg->len - 1, 1, NULL,
			       0, 0, -1, 1,
			       _is_condition, &pvmatch)))
		stack;

	if (r != 2)
		return 0;

	return 1;
}

/*
 * Is pva contiguous to any existing areas or on the same PV?
 */
static int _check_contiguous(struct cmd_context *cmd,
			     struct lv_segment *prev_lvseg, struct pv_area *pva,
			     struct pv_area **areas, uint32_t areas_size)
{
	struct pv_match pvmatch;
	int r;

	pvmatch.condition = _is_contiguous;
	pvmatch.areas = areas;
	pvmatch.areas_size = areas_size;
	pvmatch.pva = pva;

	/* FIXME Cope with stacks by flattening */
	if (!(r = _for_each_pv(cmd, prev_lvseg->lv,
			       prev_lvseg->le + prev_lvseg->len - 1, 1, NULL,
			       0, 0, -1, 1,
			       _is_condition, &pvmatch)))
		stack;

	if (r != 2)
		return 0;

	return 1;
}

/*
 * Choose sets of parallel areas to use, respecting any constraints.
 */
static int _find_parallel_space(struct alloc_handle *ah, alloc_policy_t alloc,
				struct dm_list *pvms, struct pv_area **areas,
				uint32_t areas_size, unsigned can_split,
				struct lv_segment *prev_lvseg,
				uint32_t *allocated, uint32_t needed)
{
	struct pv_map *pvm;
	struct pv_area *pva;
	struct pv_list *pvl;
	unsigned already_found_one = 0;
	unsigned contiguous = 0, cling = 0, preferred_count = 0;
	unsigned ix;
	unsigned ix_offset = 0;	/* Offset for non-preferred allocations */
	unsigned too_small_for_log_count; /* How many too small for log? */
	uint32_t max_parallel;	/* Maximum extents to allocate */
	uint32_t next_le;
	struct seg_pvs *spvs;
	struct dm_list *parallel_pvs;
	uint32_t free_pes;
	uint32_t log_len;
	struct pv_area *log_area;
	unsigned log_needs_allocating;

	/* Is there enough total space? */
	free_pes = pv_maps_size(pvms);
	if (needed - *allocated > free_pes) {
		log_error("Insufficient free space: %" PRIu32 " extents needed,"
			  " but only %" PRIu32 " available",
			  needed - *allocated, free_pes);
		return 0;
	}

	/* FIXME Select log PV appropriately if there isn't one yet */

	/* Are there any preceding segments we must follow on from? */
	if (prev_lvseg) {
		ix_offset = prev_lvseg->area_count;
		if ((alloc == ALLOC_CONTIGUOUS))
			contiguous = 1;
		else if ((alloc == ALLOC_CLING))
			cling = 1;
		else
			ix_offset = 0;
	}

	/* FIXME This algorithm needs a lot of cleaning up! */
	/* FIXME anywhere doesn't find all space yet */
	/* ix_offset holds the number of allocations that must be contiguous */
	/* ix holds the number of areas found on other PVs */
	do {
		ix = 0;
		preferred_count = 0;

		parallel_pvs = NULL;
		max_parallel = needed;

		/*
		 * If there are existing parallel PVs, avoid them and reduce
		 * the maximum we can allocate in one go accordingly.
		 */
		if (ah->parallel_areas) {
			next_le = (prev_lvseg ? prev_lvseg->le + prev_lvseg->len : 0) + *allocated / ah->area_multiple;
			dm_list_iterate_items(spvs, ah->parallel_areas) {
				if (next_le >= spvs->le + spvs->len)
					continue;

				if (max_parallel > (spvs->le + spvs->len) * ah->area_multiple)
					max_parallel = (spvs->le + spvs->len) * ah->area_multiple;
				parallel_pvs = &spvs->pvs;
				break;
			}
		}

		/*
		 * Put the smallest area of each PV that is at least the
		 * size we need into areas array.  If there isn't one
		 * that fits completely and we're allowed more than one
		 * LV segment, then take the largest remaining instead.
		 */
		dm_list_iterate_items(pvm, pvms) {
			if (dm_list_empty(&pvm->areas))
				continue;	/* Next PV */

			if (alloc != ALLOC_ANYWHERE) {
				/* Don't allocate onto the log pv */
				if (ah->log_count &&
				    pvm->pv == ah->log_area.pv)
					continue;	/* Next PV */

				/* Avoid PVs used by existing parallel areas */
				if (parallel_pvs)
					dm_list_iterate_items(pvl, parallel_pvs)
						if (pvm->pv == pvl->pv)
							goto next_pv;
			}

			already_found_one = 0;
			/* First area in each list is the largest */
			dm_list_iterate_items(pva, &pvm->areas) {
				if (contiguous) {
					if (prev_lvseg &&
					    _check_contiguous(ah->cmd,
							      prev_lvseg,
							      pva, areas,
							      areas_size)) {
						preferred_count++;
						goto next_pv;
					}
					continue;
				}

				if (cling) {
					if (prev_lvseg &&
					    _check_cling(ah->cmd,
							   prev_lvseg,
							   pva, areas,
							   areas_size)) {
						preferred_count++;
					}
					goto next_pv;
				}

				/* Is it big enough on its own? */
				if (pva->count * ah->area_multiple <
				    max_parallel - *allocated &&
				    ((!can_split && !ah->log_count) ||
				     (already_found_one &&
				      !(alloc == ALLOC_ANYWHERE))))
					goto next_pv;

				if (!already_found_one ||
				    alloc == ALLOC_ANYWHERE) {
					ix++;
					already_found_one = 1;
				}

				areas[ix + ix_offset - 1] = pva;

				goto next_pv;
			}
		next_pv:
			if (ix >= areas_size)
				break;
		}

		if ((contiguous || cling) && (preferred_count < ix_offset))
			break;

		log_needs_allocating = (ah->log_count && !ah->log_area.len) ?
				       1 : 0;

		if (ix + ix_offset < ah->area_count +
		   (log_needs_allocating ? ah->log_count : 0))
			break;

		/* sort the areas so we allocate from the biggest */
		if (ix > 1)
			qsort(areas + ix_offset, ix, sizeof(*areas),
			      _comp_area);

		/*
		 * First time around, if there's a log, allocate it on the
		 * smallest device that has space for it.
		 *
		 * FIXME decide which PV to use at top of function instead
		 */

		too_small_for_log_count = 0;

		if (!log_needs_allocating) {
			log_len = 0;
			log_area = NULL;
		} else {
			log_len = mirror_log_extents(ah->log_region_size,
			    pv_pe_size((*areas)->map->pv),
			    (max_parallel - *allocated) / ah->area_multiple);

			/* How many areas are too small for the log? */
			while (too_small_for_log_count < ix_offset + ix &&
			       (*(areas + ix_offset + ix - 1 -
				  too_small_for_log_count))->count < log_len)
				too_small_for_log_count++;

			log_area = *(areas + ix_offset + ix - 1 -
				     too_small_for_log_count);
		}

		if (ix + ix_offset < ah->area_count +
		    (log_needs_allocating ? ah->log_count +
					    too_small_for_log_count : 0))
			/* FIXME With ALLOC_ANYWHERE, need to split areas */
			break;

		if (!_alloc_parallel_area(ah, max_parallel, areas, allocated,
					  log_area, log_len))
			return_0;

	} while (!contiguous && *allocated != needed && can_split);

	return 1;
}

/*
 * Allocate several segments, each the same size, in parallel.
 * If mirrored_pv and mirrored_pe are supplied, it is used as
 * the first area, and additional areas are allocated parallel to it.
 */
static int _allocate(struct alloc_handle *ah,
		     struct volume_group *vg,
		     struct logical_volume *lv,
		     uint32_t new_extents,
		     unsigned can_split,
		     struct dm_list *allocatable_pvs)
{
	struct pv_area **areas;
	uint32_t allocated = lv ? lv->le_count : 0;
	uint32_t old_allocated;
	struct lv_segment *prev_lvseg = NULL;
	int r = 0;
	struct dm_list *pvms;
	uint32_t areas_size;
	alloc_policy_t alloc;

	if (allocated >= new_extents && !ah->log_count) {
		log_error("_allocate called with no work to do!");
		return 1;
	}

	if (ah->alloc == ALLOC_CONTIGUOUS)
		can_split = 0;

	if (lv && !dm_list_empty(&lv->segments))
		prev_lvseg = dm_list_item(dm_list_last(&lv->segments),
				       struct lv_segment);
	/*
	 * Build the sets of available areas on the pv's.
	 */
	if (!(pvms = create_pv_maps(ah->mem, vg, allocatable_pvs)))
		return_0;

	if (!_log_parallel_areas(ah->mem, ah->parallel_areas))
		stack;

	areas_size = dm_list_size(pvms);
	if (areas_size && areas_size < (ah->area_count + ah->log_count)) {
		if (ah->alloc != ALLOC_ANYWHERE) {
			log_error("Not enough PVs with free space available "
				  "for parallel allocation.");
			log_error("Consider --alloc anywhere if desperate.");
			return 0;
		}
		areas_size = ah->area_count + ah->log_count;
	}

	/* Upper bound if none of the PVs in prev_lvseg is in pvms */
	/* FIXME Work size out properly */
	if (prev_lvseg)
		areas_size += prev_lvseg->area_count;

	/* Allocate an array of pv_areas to hold the largest space on each PV */
	if (!(areas = dm_malloc(sizeof(*areas) * areas_size))) {
		log_error("Couldn't allocate areas array.");
		return 0;
	}

	/* Attempt each defined allocation policy in turn */
	for (alloc = ALLOC_CONTIGUOUS; alloc < ALLOC_INHERIT; alloc++) {
		old_allocated = allocated;
		if (!_find_parallel_space(ah, alloc, pvms, areas,
					  areas_size, can_split,
					  prev_lvseg, &allocated, new_extents))
			goto_out;
		if ((allocated == new_extents) || (ah->alloc == alloc) ||
		    (!can_split && (allocated != old_allocated)))
			break;
	}

	if (allocated != new_extents) {
		log_error("Insufficient suitable %sallocatable extents "
			  "for logical volume %s: %u more required",
			  can_split ? "" : "contiguous ",
			  lv ? lv->name : "",
			  (new_extents - allocated) * ah->area_count
			  / ah->area_multiple);
		goto out;
	}

	if (ah->log_count && !ah->log_area.len) {
		log_error("Insufficient extents for log allocation "
			  "for logical volume %s.",
			  lv ? lv->name : "");
		goto out;
	}

	r = 1;

      out:
	dm_free(areas);
	return r;
}

int lv_add_virtual_segment(struct logical_volume *lv, uint32_t status,
			   uint32_t extents, const struct segment_type *segtype)
{
	struct lv_segment *seg;

	if (!(seg = alloc_lv_segment(lv->vg->cmd->mem, segtype, lv,
				     lv->le_count, extents, status, 0,
				     NULL, 0, extents, 0, 0, 0))) {
		log_error("Couldn't allocate new zero segment.");
		return 0;
	}

	dm_list_add(&lv->segments, &seg->list);

	lv->le_count += extents;
	lv->size += (uint64_t) extents *lv->vg->extent_size;

	lv->status |= VIRTUAL;

	return 1;
}

/*
 * Entry point for all extent allocations.
 */
struct alloc_handle *allocate_extents(struct volume_group *vg,
				      struct logical_volume *lv,
				      const struct segment_type *segtype,
				      uint32_t stripes,
				      uint32_t mirrors, uint32_t log_count,
				      uint32_t log_region_size, uint32_t extents,
				      struct dm_list *allocatable_pvs,
				      alloc_policy_t alloc,
				      struct dm_list *parallel_areas)
{
	struct alloc_handle *ah;

	if (segtype_is_virtual(segtype)) {
		log_error("allocate_extents does not handle virtual segments");
		return NULL;
	}

	if (vg->fid->fmt->ops->segtype_supported &&
	    !vg->fid->fmt->ops->segtype_supported(vg->fid, segtype)) {
		log_error("Metadata format (%s) does not support required "
			  "LV segment type (%s).", vg->fid->fmt->name,
			  segtype->name);
		log_error("Consider changing the metadata format by running "
			  "vgconvert.");
		return NULL;
	}

	if (alloc == ALLOC_INHERIT)
		alloc = vg->alloc;

	if (!(ah = _alloc_init(vg->cmd, vg->cmd->mem, segtype, alloc, mirrors,
			       stripes, log_count, log_region_size, parallel_areas)))
		return_NULL;

	if (!segtype_is_virtual(segtype) &&
	    !_allocate(ah, vg, lv, (lv ? lv->le_count : 0) + extents,
		       1, allocatable_pvs)) {
		alloc_destroy(ah);
		return_NULL;
	}

	return ah;
}

/*
 * Add new segments to an LV from supplied list of areas.
 */
int lv_add_segment(struct alloc_handle *ah,
		   uint32_t first_area, uint32_t num_areas,
		   struct logical_volume *lv,
		   const struct segment_type *segtype,
		   uint32_t stripe_size,
		   uint32_t status,
		   uint32_t region_size,
		   struct logical_volume *log_lv)
{
	if (!segtype) {
		log_error("Missing segtype in lv_add_segment().");
		return 0;
	}

	if (segtype_is_virtual(segtype)) {
		log_error("lv_add_segment cannot handle virtual segments");
		return 0;
	}

	if (!_setup_alloced_segments(lv, &ah->alloced_areas[first_area],
				     num_areas, status,
				     stripe_size, segtype,
				     region_size, log_lv))
		return_0;

	if ((segtype->flags & SEG_CAN_SPLIT) && !lv_merge_segments(lv)) {
		log_error("Couldn't merge segments after extending "
			  "logical volume.");
		return 0;
	}

	if (lv->vg->fid->fmt->ops->lv_setup &&
	    !lv->vg->fid->fmt->ops->lv_setup(lv->vg->fid, lv))
		return_0;

	return 1;
}

/*
 * "mirror" segment type doesn't support split.
 * So, when adding mirrors to linear LV segment, first split it,
 * then convert it to "mirror" and add areas.
 */
static struct lv_segment *_convert_seg_to_mirror(struct lv_segment *seg,
						 uint32_t region_size,
						 struct logical_volume *log_lv)
{
	struct lv_segment *newseg;
	uint32_t s;

	if (!seg_is_striped(seg)) {
		log_error("Can't convert non-striped segment to mirrored.");
		return NULL;
	}

	if (seg->area_count > 1) {
		log_error("Can't convert striped segment with multiple areas "
			  "to mirrored.");
		return NULL;
	}

	if (!(newseg = alloc_lv_segment(seg->lv->vg->cmd->mem,
					get_segtype_from_string(seg->lv->vg->cmd, "mirror"),
					seg->lv, seg->le, seg->len,
					seg->status, seg->stripe_size,
					log_lv,
					seg->area_count, seg->area_len,
					seg->chunk_size, region_size,
					seg->extents_copied))) {
		log_error("Couldn't allocate converted LV segment");
		return NULL;
	}

	for (s = 0; s < seg->area_count; s++)
		if (!move_lv_segment_area(newseg, s, seg, s))
			return_NULL;

	dm_list_add(&seg->list, &newseg->list);
	dm_list_del(&seg->list);

	return newseg;
}

/*
 * Add new areas to mirrored segments
 */
int lv_add_mirror_areas(struct alloc_handle *ah,
			struct logical_volume *lv, uint32_t le,
			uint32_t region_size)
{
	struct alloced_area *aa;
	struct lv_segment *seg;
	uint32_t current_le = le;
	uint32_t s, old_area_count, new_area_count;

	dm_list_iterate_items(aa, &ah->alloced_areas[0]) {
		if (!(seg = find_seg_by_le(lv, current_le))) {
			log_error("Failed to find segment for %s extent %"
				  PRIu32, lv->name, current_le);
			return 0;
		}

		/* Allocator assures aa[0].len <= seg->area_len */
		if (aa[0].len < seg->area_len) {
			if (!lv_split_segment(lv, seg->le + aa[0].len)) {
				log_error("Failed to split segment at %s "
					  "extent %" PRIu32, lv->name, le);
				return 0;
			}
		}

		if (!seg_is_mirrored(seg) &&
		    (!(seg = _convert_seg_to_mirror(seg, region_size, NULL))))
			return_0;

		old_area_count = seg->area_count;
		new_area_count = old_area_count + ah->area_count;

		if (!_lv_segment_add_areas(lv, seg, new_area_count))
			return_0;

		for (s = 0; s < ah->area_count; s++) {
			if (!set_lv_segment_area_pv(seg, s + old_area_count,
						    aa[s].pv, aa[s].pe))
				return_0;
		}

		current_le += seg->area_len;
	}

	lv->status |= MIRRORED;

	if (lv->vg->fid->fmt->ops->lv_setup &&
	    !lv->vg->fid->fmt->ops->lv_setup(lv->vg->fid, lv))
		return_0;

	return 1;
}

/*
 * Add mirror image LVs to mirrored segments
 */
int lv_add_mirror_lvs(struct logical_volume *lv,
		      struct logical_volume **sub_lvs,
		      uint32_t num_extra_areas,
		      uint32_t status, uint32_t region_size)
{
	struct lv_segment *seg;
	uint32_t old_area_count, new_area_count;
	uint32_t m;
	struct segment_type *mirror_segtype;

	seg = first_seg(lv);

	if (dm_list_size(&lv->segments) != 1 || seg_type(seg, 0) != AREA_LV) {
		log_error("Mirror layer must be inserted before adding mirrors");
		return_0;
	}

	mirror_segtype = get_segtype_from_string(lv->vg->cmd, "mirror");
	if (seg->segtype != mirror_segtype)
		if (!(seg = _convert_seg_to_mirror(seg, region_size, NULL)))
			return_0;

	if (region_size && region_size != seg->region_size) {
		log_error("Conflicting region_size");
		return 0;
	}

	old_area_count = seg->area_count;
	new_area_count = old_area_count + num_extra_areas;

	if (!_lv_segment_add_areas(lv, seg, new_area_count)) {
		log_error("Failed to allocate widened LV segment for %s.",
			  lv->name);
		return 0;
	}

	for (m = 0; m < old_area_count; m++)
		seg_lv(seg, m)->status |= status;

	for (m = old_area_count; m < new_area_count; m++) {
		if (!set_lv_segment_area_lv(seg, m, sub_lvs[m - old_area_count],
					    0, status))
			return_0;
		lv_set_hidden(sub_lvs[m - old_area_count]);
	}

	lv->status |= MIRRORED;

	return 1;
}

/*
 * Turn an empty LV into a mirror log.
 */
int lv_add_log_segment(struct alloc_handle *ah, struct logical_volume *log_lv)
{
	struct lv_segment *seg;

	if (dm_list_size(&log_lv->segments)) {
		log_error("Log segments can only be added to an empty LV");
		return 0;
	}

	if (!(seg = alloc_lv_segment(log_lv->vg->cmd->mem,
				     get_segtype_from_string(log_lv->vg->cmd,
							     "striped"),
				     log_lv, 0, ah->log_area.len, MIRROR_LOG,
				     0, NULL, 1, ah->log_area.len, 0, 0, 0))) {
		log_error("Couldn't allocate new mirror log segment.");
		return 0;
	}

	if (!set_lv_segment_area_pv(seg, 0, ah->log_area.pv, ah->log_area.pe))
		return_0;

	dm_list_add(&log_lv->segments, &seg->list);
	log_lv->le_count += ah->log_area.len;
	log_lv->size += (uint64_t) log_lv->le_count * log_lv->vg->extent_size;

	if (log_lv->vg->fid->fmt->ops->lv_setup &&
	    !log_lv->vg->fid->fmt->ops->lv_setup(log_lv->vg->fid, log_lv))
		return_0;

	return 1;
}

static int _lv_extend_mirror(struct alloc_handle *ah,
			     struct logical_volume *lv,
			     uint32_t extents, uint32_t first_area)
{
	struct lv_segment *seg;
	uint32_t m, s;

	seg = first_seg(lv);
	for (m = first_area, s = 0; s < seg->area_count; s++) {
		if (is_temporary_mirror_layer(seg_lv(seg, s))) {
			if (!_lv_extend_mirror(ah, seg_lv(seg, s), extents, m))
				return_0;
			m += lv_mirror_count(seg_lv(seg, s));
			continue;
		}

		if (!lv_add_segment(ah, m++, 1, seg_lv(seg, s),
				    get_segtype_from_string(lv->vg->cmd,
							    "striped"),
				    0, 0, 0, NULL)) {
			log_error("Aborting. Failed to extend %s.",
				  seg_lv(seg, s)->name);
			return 0;
		}
	}
	seg->area_len += extents;
	seg->len += extents;
	lv->le_count += extents;
	lv->size += (uint64_t) extents *lv->vg->extent_size;

	return 1;
}

/*
 * Entry point for single-step LV allocation + extension.
 */
int lv_extend(struct logical_volume *lv,
	      const struct segment_type *segtype,
	      uint32_t stripes, uint32_t stripe_size,
	      uint32_t mirrors, uint32_t extents,
	      struct physical_volume *mirrored_pv __attribute((unused)),
	      uint32_t mirrored_pe __attribute((unused)),
	      uint32_t status, struct dm_list *allocatable_pvs,
	      alloc_policy_t alloc)
{
	int r = 1;
	struct alloc_handle *ah;

	if (segtype_is_virtual(segtype))
		return lv_add_virtual_segment(lv, status, extents, segtype);

	if (!(ah = allocate_extents(lv->vg, lv, segtype, stripes, mirrors, 0, 0,
				    extents, allocatable_pvs, alloc, NULL)))
		return_0;

	if (mirrors < 2)
		r = lv_add_segment(ah, 0, ah->area_count, lv, segtype,
				   stripe_size, status, 0, NULL);
	else
		r = _lv_extend_mirror(ah, lv, extents, 0);

	alloc_destroy(ah);
	return r;
}

/*
 * Minimal LV renaming function.
 * Metadata transaction should be made by caller.
 * Assumes new_name is allocated from cmd->mem pool.
 */
static int _rename_single_lv(struct logical_volume *lv, char *new_name)
{
	struct volume_group *vg = lv->vg;

	if (find_lv_in_vg(vg, new_name)) {
		log_error("Logical volume \"%s\" already exists in "
			  "volume group \"%s\"", new_name, vg->name);
		return 0;
	}

	if (lv->status & LOCKED) {
		log_error("Cannot rename locked LV %s", lv->name);
		return 0;
	}

	lv->name = new_name;

	return 1;
}

/*
 * Rename sub LV.
 * 'lv_name_old' and 'lv_name_new' are old and new names of the main LV.
 */
static int _rename_sub_lv(struct cmd_context *cmd,
			  struct logical_volume *lv,
			  const char *lv_name_old, const char *lv_name_new)
{
	char *suffix, *new_name;
	size_t len;

	/*
	 * A sub LV name starts with lv_name_old + '_'.
	 * The suffix follows lv_name_old and includes '_'.
	 */
	len = strlen(lv_name_old);
	if (strncmp(lv->name, lv_name_old, len) || lv->name[len] != '_') {
		log_error("Cannot rename \"%s\": name format not recognized "
			  "for internal LV \"%s\"",
			  lv_name_old, lv->name);
		return 0;
	}
	suffix = lv->name + len;

	/*
	 * Compose a new name for sub lv:
	 *   e.g. new name is "lvol1_mlog"
	 *        if the sub LV is "lvol0_mlog" and
	 *        a new name for main LV is "lvol1"
	 */
	len = strlen(lv_name_new) + strlen(suffix) + 1;
	new_name = dm_pool_alloc(cmd->mem, len);
	if (!new_name) {
		log_error("Failed to allocate space for new name");
		return 0;
	}
	if (!dm_snprintf(new_name, len, "%s%s", lv_name_new, suffix)) {
		log_error("Failed to create new name");
		return 0;
	}

	/* Rename it */
	return _rename_single_lv(lv, new_name);
}

/* Callback for _for_each_sub_lv */
static int _rename_cb(struct cmd_context *cmd, struct logical_volume *lv,
		      void *data)
{
	struct lv_names *lv_names = (struct lv_names *) data;

	return _rename_sub_lv(cmd, lv, lv_names->old, lv_names->new);
}

/*
 * Loop down sub LVs and call "func" for each.
 * "func" is responsible to log necessary information on failure.
 */
static int _for_each_sub_lv(struct cmd_context *cmd, struct logical_volume *lv,
			    int (*func)(struct cmd_context *cmd,
					struct logical_volume *lv,
					void *data),
			    void *data)
{
	struct logical_volume *org;
	struct lv_segment *seg;
	uint32_t s;

	if (lv_is_cow(lv) && lv_is_virtual_origin(org = origin_from_cow(lv)))
		if (!func(cmd, org, data))
			return_0;

	dm_list_iterate_items(seg, &lv->segments) {
		if (seg->log_lv && !func(cmd, seg->log_lv, data))
			return_0;
		for (s = 0; s < seg->area_count; s++) {
			if (seg_type(seg, s) != AREA_LV)
				continue;
			if (!func(cmd, seg_lv(seg, s), data))
				return_0;
			if (!_for_each_sub_lv(cmd, seg_lv(seg, s), func, data))
				return_0;
		}
	}

	return 1;
}


/*
 * Core of LV renaming routine.
 * VG must be locked by caller.
 */
int lv_rename(struct cmd_context *cmd, struct logical_volume *lv,
	      const char *new_name)
{
	struct volume_group *vg = lv->vg;
	struct lv_names lv_names;
	DM_LIST_INIT(lvs_changed);
	struct lv_list lvl, lvl2;
	int r = 0;

	/* rename is not allowed on sub LVs */
	if (!lv_is_visible(lv)) {
		log_error("Cannot rename internal LV \"%s\".", lv->name);
		return 0;
	}

	if (find_lv_in_vg(vg, new_name)) {
		log_error("Logical volume \"%s\" already exists in "
			  "volume group \"%s\"", new_name, vg->name);
		return 0;
	}

	if (lv->status & LOCKED) {
		log_error("Cannot rename locked LV %s", lv->name);
		return 0;
	}

	if (!archive(vg))
		return 0;

	/* rename sub LVs */
	lv_names.old = lv->name;
	lv_names.new = new_name;
	if (!_for_each_sub_lv(cmd, lv, _rename_cb, (void *) &lv_names))
		return 0;

	/* rename main LV */
	if (!(lv->name = dm_pool_strdup(cmd->mem, new_name))) {
		log_error("Failed to allocate space for new name");
		return 0;
	}

	lvl.lv = lv;
	dm_list_add(&lvs_changed, &lvl.list);

	/* rename active virtual origin too */
	if (lv_is_cow(lv) && lv_is_virtual_origin(lvl2.lv = origin_from_cow(lv)))
		dm_list_add_h(&lvs_changed, &lvl2.list);

	log_verbose("Writing out updated volume group");
	if (!vg_write(vg))
		return 0;


	if (!suspend_lvs(cmd, &lvs_changed)) {
		vg_revert(vg);
		goto_out;
	}

	if (!(r = vg_commit(vg)))
		stack;

	resume_lvs(cmd, &lvs_changed);
out:
	backup(vg);
	return r;
}

char *generate_lv_name(struct volume_group *vg, const char *format,
		       char *buffer, size_t len)
{
	struct lv_list *lvl;
	int high = -1, i;

	dm_list_iterate_items(lvl, &vg->lvs) {
		if (sscanf(lvl->lv->name, format, &i) != 1)
			continue;

		if (i > high)
			high = i;
	}

	if (dm_snprintf(buffer, len, format, high + 1) < 0)
		return NULL;

	return buffer;
}

int vg_max_lv_reached(struct volume_group *vg)
{
	if (!vg->max_lv)
		return 0;

	if (vg->max_lv > vg_visible_lvs(vg))
		return 0;

	log_verbose("Maximum number of logical volumes (%u) reached "
		    "in volume group %s", vg->max_lv, vg->name);

	return 1;
}

struct logical_volume *alloc_lv(struct dm_pool *mem)
{
	struct logical_volume *lv;

	if (!(lv = dm_pool_zalloc(mem, sizeof(*lv)))) {
		log_error("Unable to allocate logical volume structure");
		return NULL;
	}

	lv->snapshot = NULL;
	dm_list_init(&lv->snapshot_segs);
	dm_list_init(&lv->segments);
	dm_list_init(&lv->tags);
	dm_list_init(&lv->segs_using_this_lv);

	return lv;
}

/*
 * Create a new empty LV.
 */
struct logical_volume *lv_create_empty(const char *name,
				       union lvid *lvid,
				       uint32_t status,
				       alloc_policy_t alloc,
				       struct volume_group *vg)
{
	struct format_instance *fi = vg->fid;
	struct logical_volume *lv;
	char dname[NAME_LEN];

	if (vg_max_lv_reached(vg))
		stack;

	if (strstr(name, "%d") &&
	    !(name = generate_lv_name(vg, name, dname, sizeof(dname)))) {
		log_error("Failed to generate unique name for the new "
			  "logical volume");
		return NULL;
	} else if (find_lv_in_vg(vg, name)) {
		log_error("Unable to create LV %s in Volume Group %s: "
			  "name already in use.", name, vg->name);
		return NULL;
	}

	log_verbose("Creating logical volume %s", name);

	if (!(lv = alloc_lv(vg->vgmem)))
		return_NULL;

	if (!(lv->name = dm_pool_strdup(vg->vgmem, name)))
		goto_bad;

	lv->status = status;
	lv->alloc = alloc;
	lv->read_ahead = vg->cmd->default_settings.read_ahead;
	lv->major = -1;
	lv->minor = -1;
	lv->size = UINT64_C(0);
	lv->le_count = 0;

	if (lvid)
		lv->lvid = *lvid;

	if (!link_lv_to_vg(vg, lv))
		goto_bad;
 
	if (fi->fmt->ops->lv_setup && !fi->fmt->ops->lv_setup(fi, lv))
		goto_bad;
 
	return lv;
bad:
	dm_pool_free(vg->vgmem, lv);
	return NULL;
}

static int _add_pvs(struct cmd_context *cmd, struct pv_segment *peg,
		    uint32_t s __attribute((unused)), void *data)
{
	struct seg_pvs *spvs = (struct seg_pvs *) data;
	struct pv_list *pvl;

	/* Don't add again if it's already on list. */
	if (find_pv_in_pv_list(&spvs->pvs, peg->pv))
			return 1;

	if (!(pvl = dm_pool_alloc(cmd->mem, sizeof(*pvl)))) {
		log_error("pv_list allocation failed");
		return 0;
	}

	pvl->pv = peg->pv;

	dm_list_add(&spvs->pvs, &pvl->list);

	return 1;
}

/*
 * Construct dm_list of segments of LVs showing which PVs they use.
 */
struct dm_list *build_parallel_areas_from_lv(struct cmd_context *cmd,
					  struct logical_volume *lv)
{
	struct dm_list *parallel_areas;
	struct seg_pvs *spvs;
	uint32_t current_le = 0;

	if (!(parallel_areas = dm_pool_alloc(cmd->mem, sizeof(*parallel_areas)))) {
		log_error("parallel_areas allocation failed");
		return NULL;
	}

	dm_list_init(parallel_areas);

	do {
		if (!(spvs = dm_pool_zalloc(cmd->mem, sizeof(*spvs)))) {
			log_error("allocation failed");
			return NULL;
		}

		dm_list_init(&spvs->pvs);

		spvs->le = current_le;
		spvs->len = lv->le_count - current_le;

		dm_list_add(parallel_areas, &spvs->list);

		/* Find next segment end */
		/* FIXME Unnecessary nesting! */
		if (!_for_each_pv(cmd, lv, current_le, spvs->len, &spvs->len,
				  0, 0, -1, 0, _add_pvs, (void *) spvs))
			return_NULL;

		current_le = spvs->le + spvs->len;
	} while (current_le < lv->le_count);

	/* FIXME Merge adjacent segments with identical PV lists (avoids need for contiguous allocation attempts between successful allocations) */

	return parallel_areas;
}

int link_lv_to_vg(struct volume_group *vg, struct logical_volume *lv)
{
	struct lv_list *lvl;

	if (vg_max_lv_reached(vg))
		stack;

	if (!(lvl = dm_pool_zalloc(vg->vgmem, sizeof(*lvl))))
		return_0;

	lvl->lv = lv;
	lv->vg = vg;
	dm_list_add(&vg->lvs, &lvl->list);

	return 1;
}

int unlink_lv_from_vg(struct logical_volume *lv)
{
	struct lv_list *lvl;

	if (!(lvl = find_lv_in_vg(lv->vg, lv->name)))
		return_0;

	dm_list_del(&lvl->list);

	return 1;
}

void lv_set_visible(struct logical_volume *lv)
{
	if (lv_is_visible(lv))
		return;

	lv->status |= VISIBLE_LV;

	log_debug("LV %s in VG %s is now visible.",  lv->name, lv->vg->name);
}

void lv_set_hidden(struct logical_volume *lv)
{
	if (!lv_is_visible(lv))
		return;

	lv->status &= ~VISIBLE_LV;

	log_debug("LV %s in VG %s is now hidden.",  lv->name, lv->vg->name);
}

int lv_remove_single(struct cmd_context *cmd, struct logical_volume *lv,
		     const force_t force)
{
	struct volume_group *vg;
	struct lvinfo info;
	struct logical_volume *origin = NULL;

	vg = lv->vg;

	if (!vg_check_status(vg, LVM_WRITE))
		return 0;

	if (lv_is_origin(lv)) {
		log_error("Can't remove logical volume \"%s\" under snapshot",
			  lv->name);
		return 0;
	}

	if (lv->status & MIRROR_IMAGE) {
		log_error("Can't remove logical volume %s used by a mirror",
			  lv->name);
		return 0;
	}

	if (lv->status & MIRROR_LOG) {
		log_error("Can't remove logical volume %s used as mirror log",
			  lv->name);
		return 0;
	}

	if (lv->status & LOCKED) {
		log_error("Can't remove locked LV %s", lv->name);
		return 0;
	}

	/* FIXME Ensure not referred to by another existing LVs */

	if (lv_info(cmd, lv, &info, 1, 0)) {
		if (info.open_count) {
			log_error("Can't remove open logical volume \"%s\"",
				  lv->name);
			return 0;
		}

		if (lv_is_active(lv) && (force == PROMPT) &&
		    lv_is_visible(lv) &&
		    yes_no_prompt("Do you really want to remove active "
				  "%slogical volume %s? [y/n]: ",
				  vg_is_clustered(vg) ? "clustered " : "",
				  lv->name) == 'n') {
			log_print("Logical volume %s not removed", lv->name);
			return 0;
		}
	}

	if (!archive(vg))
		return 0;

	/* FIXME Snapshot commit out of sequence if it fails after here? */
	if (!deactivate_lv(cmd, lv)) {
		log_error("Unable to deactivate logical volume \"%s\"",
			  lv->name);
		return 0;
	}

	if (lv_is_cow(lv)) {
		origin = origin_from_cow(lv);
		log_verbose("Removing snapshot %s", lv->name);
		if (!vg_remove_snapshot(lv))
			return_0;
	}

	log_verbose("Releasing logical volume \"%s\"", lv->name);
	if (!lv_remove(lv)) {
		log_error("Error releasing logical volume \"%s\"", lv->name);
		return 0;
	}

	/* store it on disks */
	if (!vg_write(vg) || !vg_commit(vg))
		return_0;

	backup(vg);

	/* If no snapshots left, reload without -real. */
	if (origin && !lv_is_origin(origin)) {
		if (!suspend_lv(cmd, origin))
			log_error("Failed to refresh %s without snapshot.", origin->name);
		else if (!resume_lv(cmd, origin))
			log_error("Failed to resume %s.", origin->name);
	}

	if (lv_is_visible(lv))
		log_print("Logical volume \"%s\" successfully removed", lv->name);

	return 1;
}

/*
 * remove LVs with its dependencies - LV leaf nodes should be removed first
 */
int lv_remove_with_dependencies(struct cmd_context *cmd, struct logical_volume *lv,
				const force_t force)
{
	struct dm_list *snh, *snht;

        if (lv_is_origin(lv)) {
		/* remove snapshot LVs first */
		dm_list_iterate_safe(snh, snht, &lv->snapshot_segs) {
			if (!lv_remove_with_dependencies(cmd, dm_list_struct_base(snh, struct lv_segment,
									       origin_list)->cow,
							 force))
				return 0;
		}
	}

        return lv_remove_single(cmd, lv, force);
}

/*
 * insert_layer_for_segments_on_pv() inserts a layer segment for a segment area.
 * However, layer modification could split the underlying layer segment.
 * This function splits the parent area according to keep the 1:1 relationship
 * between the parent area and the underlying layer segment.
 * Since the layer LV might have other layers below, build_parallel_areas()
 * is used to find the lowest-level segment boundaries.
 */
static int _split_parent_area(struct lv_segment *seg, uint32_t s,
			      struct dm_list *layer_seg_pvs)
{
	uint32_t parent_area_len, parent_le, layer_le;
	uint32_t area_multiple;
	struct seg_pvs *spvs;

	if (seg_is_striped(seg))
		area_multiple = seg->area_count;
	else
		area_multiple = 1;

	parent_area_len = seg->area_len;
	parent_le = seg->le;
	layer_le = seg_le(seg, s);

	while (parent_area_len > 0) {
		/* Find the layer segment pointed at */
		if (!(spvs = _find_seg_pvs_by_le(layer_seg_pvs, layer_le))) {
			log_error("layer segment for %s:%" PRIu32 " not found",
				  seg->lv->name, parent_le);
			return 0;
		}

		if (spvs->le != layer_le) {
			log_error("Incompatible layer boundary: "
				  "%s:%" PRIu32 "[%" PRIu32 "] on %s:%" PRIu32,
				  seg->lv->name, parent_le, s,
				  seg_lv(seg, s)->name, layer_le);
			return 0;
		}

		if (spvs->len < parent_area_len) {
			parent_le += spvs->len * area_multiple;
			if (!lv_split_segment(seg->lv, parent_le))
				return_0;
		}

		parent_area_len -= spvs->len;
		layer_le += spvs->len;
	}

	return 1;
}

/*
 * Split the parent LV segments if the layer LV below it is splitted.
 */
int split_parent_segments_for_layer(struct cmd_context *cmd,
				    struct logical_volume *layer_lv)
{
	struct lv_list *lvl;
	struct logical_volume *parent_lv;
	struct lv_segment *seg;
	uint32_t s;
	struct dm_list *parallel_areas;

	if (!(parallel_areas = build_parallel_areas_from_lv(cmd, layer_lv)))
		return_0;

	/* Loop through all LVs except itself */
	dm_list_iterate_items(lvl, &layer_lv->vg->lvs) {
		parent_lv = lvl->lv;
		if (parent_lv == layer_lv)
			continue;

		/* Find all segments that point at the layer LV */
		dm_list_iterate_items(seg, &parent_lv->segments) {
			for (s = 0; s < seg->area_count; s++) {
				if (seg_type(seg, s) != AREA_LV ||
				    seg_lv(seg, s) != layer_lv)
					continue;

				if (!_split_parent_area(seg, s, parallel_areas))
					return_0;
			}
		}
	}

	return 1;
}

/* Remove a layer from the LV */
int remove_layers_for_segments(struct cmd_context *cmd,
			       struct logical_volume *lv,
			       struct logical_volume *layer_lv,
			       uint32_t status_mask, struct dm_list *lvs_changed)
{
	struct lv_segment *seg, *lseg;
	uint32_t s;
	int lv_changed = 0;
	struct lv_list *lvl;

	log_very_verbose("Removing layer %s for segments of %s",
			 layer_lv->name, lv->name);

	/* Find all segments that point at the temporary mirror */
	dm_list_iterate_items(seg, &lv->segments) {
		for (s = 0; s < seg->area_count; s++) {
			if (seg_type(seg, s) != AREA_LV ||
			    seg_lv(seg, s) != layer_lv)
				continue;

			/* Find the layer segment pointed at */
			if (!(lseg = find_seg_by_le(layer_lv, seg_le(seg, s)))) {
				log_error("Layer segment found: %s:%" PRIu32,
					  layer_lv->name, seg_le(seg, s));
				return 0;
			}

			/* Check the segment params are compatible */
			if (!seg_is_striped(lseg) || lseg->area_count != 1) {
				log_error("Layer is not linear: %s:%" PRIu32,
					  layer_lv->name, lseg->le);
				return 0;
			}
			if ((lseg->status & status_mask) != status_mask) {
				log_error("Layer status does not match: "
					  "%s:%" PRIu32 " status: 0x%x/0x%x",
					  layer_lv->name, lseg->le,
					  lseg->status, status_mask);
				return 0;
			}
			if (lseg->le != seg_le(seg, s) ||
			    lseg->area_len != seg->area_len) {
				log_error("Layer boundary mismatch: "
					  "%s:%" PRIu32 "-%" PRIu32 " on "
					  "%s:%" PRIu32 " / "
					  "%" PRIu32 "-%" PRIu32 " / ",
					  lv->name, seg->le, seg->area_len,
					  layer_lv->name, seg_le(seg, s),
					  lseg->le, lseg->area_len);
				return 0;
			}

			if (!move_lv_segment_area(seg, s, lseg, 0))
				return_0;

			/* Replace mirror with error segment */
			if (!(lseg->segtype =
			      get_segtype_from_string(lv->vg->cmd, "error"))) {
				log_error("Missing error segtype");
				return 0;
			}
			lseg->area_count = 0;

			/* First time, add LV to list of LVs affected */
			if (!lv_changed && lvs_changed) {
				if (!(lvl = dm_pool_alloc(cmd->mem, sizeof(*lvl)))) {
					log_error("lv_list alloc failed");
					return 0;
				}
				lvl->lv = lv;
				dm_list_add(lvs_changed, &lvl->list);
				lv_changed = 1;
			}
		}
	}
	if (lv_changed && !lv_merge_segments(lv))
		stack;

	return 1;
}

/* Remove a layer */
int remove_layers_for_segments_all(struct cmd_context *cmd,
				   struct logical_volume *layer_lv,
				   uint32_t status_mask,
				   struct dm_list *lvs_changed)
{
	struct lv_list *lvl;
	struct logical_volume *lv1;

	/* Loop through all LVs except the temporary mirror */
	dm_list_iterate_items(lvl, &layer_lv->vg->lvs) {
		lv1 = lvl->lv;
		if (lv1 == layer_lv)
			continue;

		if (!remove_layers_for_segments(cmd, lv1, layer_lv,
						status_mask, lvs_changed))
			return_0;
	}

	if (!lv_empty(layer_lv))
		return_0;

	return 1;
}

static int _move_lv_segments(struct logical_volume *lv_to,
			     struct logical_volume *lv_from,
			     uint32_t set_status, uint32_t reset_status)
{
	struct lv_segment *seg;

	dm_list_iterate_items(seg, &lv_to->segments) {
		if (seg->origin) {
			log_error("Can't move snapshot segment");
			return 0;
		}
	}

	lv_to->segments = lv_from->segments;
	lv_to->segments.n->p = &lv_to->segments;
	lv_to->segments.p->n = &lv_to->segments;

	dm_list_iterate_items(seg, &lv_to->segments) {
		seg->lv = lv_to;
		seg->status &= ~reset_status;
		seg->status |= set_status;
	}

	dm_list_init(&lv_from->segments);

	lv_to->le_count = lv_from->le_count;
	lv_to->size = lv_from->size;

	lv_from->le_count = 0;
	lv_from->size = 0;

	return 1;
}

/* Remove a layer from the LV */
int remove_layer_from_lv(struct logical_volume *lv,
			 struct logical_volume *layer_lv)
{
	struct logical_volume *parent;
	struct lv_segment *parent_seg;
	struct segment_type *segtype;

	log_very_verbose("Removing layer %s for %s", layer_lv->name, lv->name);

	if (!(parent_seg = get_only_segment_using_this_lv(layer_lv))) {
		log_error("Failed to find layer %s in %s",
		layer_lv->name, lv->name);
		return 0;
	}
	parent = parent_seg->lv;

	/*
	 * Before removal, the layer should be cleaned up,
	 * i.e. additional segments and areas should have been removed.
	 */
	if (dm_list_size(&parent->segments) != 1 ||
	    parent_seg->area_count != 1 ||
	    seg_type(parent_seg, 0) != AREA_LV ||
	    layer_lv != seg_lv(parent_seg, 0) ||
	    parent->le_count != layer_lv->le_count)
		return_0;

	if (!lv_empty(parent))
		return_0;

	if (!_move_lv_segments(parent, layer_lv, 0, 0))
		return_0;

	/* Replace the empty layer with error segment */
	segtype = get_segtype_from_string(lv->vg->cmd, "error");
	if (!lv_add_virtual_segment(layer_lv, 0, parent->le_count, segtype))
		return_0;

	return 1;
}

/*
 * Create and insert a linear LV "above" lv_where.
 * After the insertion, a new LV named lv_where->name + suffix is created
 * and all segments of lv_where is moved to the new LV.
 * lv_where will have a single segment which maps linearly to the new LV.
 */
struct logical_volume *insert_layer_for_lv(struct cmd_context *cmd,
					   struct logical_volume *lv_where,
					   uint32_t status,
					   const char *layer_suffix)
{
	struct logical_volume *layer_lv;
	char *name;
	size_t len;
	struct segment_type *segtype;
	struct lv_segment *mapseg;

	/* create an empty layer LV */
	len = strlen(lv_where->name) + 32;
	if (!(name = alloca(len))) {
		log_error("layer name allocation failed. "
			  "Remove new LV and retry.");
		return NULL;
	}

	if (dm_snprintf(name, len, "%s%s", lv_where->name, layer_suffix) < 0) {
		log_error("layer name allocation failed. "
			  "Remove new LV and retry.");
		return NULL;
	}

	if (!(layer_lv = lv_create_empty(name, NULL, LVM_READ | LVM_WRITE,
					 ALLOC_INHERIT, lv_where->vg))) {
		log_error("Creation of layer LV failed");
		return NULL;
	}

	if (lv_is_active(lv_where) && strstr(name, "_mimagetmp")) {
		log_very_verbose("Creating transient LV %s for mirror conversion in VG %s.", name, lv_where->vg->name);

		segtype = get_segtype_from_string(cmd, "error");

		if (!lv_add_virtual_segment(layer_lv, 0, lv_where->le_count, segtype)) {
			log_error("Creation of transient LV %s for mirror conversion in VG %s failed.", name, lv_where->vg->name);
			return NULL;
		}

		if (!vg_write(lv_where->vg)) {
			log_error("Failed to write intermediate VG %s metadata for mirror conversion.", lv_where->vg->name);
			return NULL;
		}

		if (!vg_commit(lv_where->vg)) {
			log_error("Failed to commit intermediate VG %s metadata for mirror conversion.", lv_where->vg->name);
			vg_revert(lv_where->vg);
			return NULL;
		}

		if (!activate_lv(cmd, layer_lv)) {
			log_error("Failed to resume transient error LV %s for mirror conversion in VG %s.", name, lv_where->vg->name);
			return NULL;
		}
	}

	log_very_verbose("Inserting layer %s for %s",
			 layer_lv->name, lv_where->name);

	if (!_move_lv_segments(layer_lv, lv_where, 0, 0))
		return_NULL;

	if (!(segtype = get_segtype_from_string(cmd, "striped")))
		return_NULL;

	/* allocate a new linear segment */
	if (!(mapseg = alloc_lv_segment(cmd->mem, segtype,
					lv_where, 0, layer_lv->le_count,
					status, 0, NULL, 1, layer_lv->le_count,
					0, 0, 0)))
		return_NULL;

	/* map the new segment to the original underlying are */
	if (!set_lv_segment_area_lv(mapseg, 0, layer_lv, 0, 0))
		return_NULL;

	/* add the new segment to the layer LV */
	dm_list_add(&lv_where->segments, &mapseg->list);
	lv_where->le_count = layer_lv->le_count;
	lv_where->size = lv_where->le_count * lv_where->vg->extent_size;

	return layer_lv;
}

/*
 * Extend and insert a linear layer LV beneath the source segment area.
 */
static int _extend_layer_lv_for_segment(struct logical_volume *layer_lv,
					struct lv_segment *seg, uint32_t s,
					uint32_t status)
{
	struct lv_segment *mapseg;
	struct segment_type *segtype;
	struct physical_volume *src_pv = seg_pv(seg, s);
	uint32_t src_pe = seg_pe(seg, s);

	if (seg_type(seg, s) != AREA_PV && seg_type(seg, s) != AREA_LV)
		return_0;

	if (!(segtype = get_segtype_from_string(layer_lv->vg->cmd, "striped")))
		return_0;

	/* FIXME Incomplete message? Needs more context */
	log_very_verbose("Inserting %s:%" PRIu32 "-%" PRIu32 " of %s/%s",
			 pv_dev_name(src_pv),
			 src_pe, src_pe + seg->area_len - 1,
			 seg->lv->vg->name, seg->lv->name);

	/* allocate a new segment */
	if (!(mapseg = alloc_lv_segment(layer_lv->vg->cmd->mem, segtype,
					layer_lv, layer_lv->le_count,
					seg->area_len, status, 0,
					NULL, 1, seg->area_len, 0, 0, 0)))
		return_0;

	/* map the new segment to the original underlying are */
	if (!move_lv_segment_area(mapseg, 0, seg, s))
		return_0;

	/* add the new segment to the layer LV */
	dm_list_add(&layer_lv->segments, &mapseg->list);
	layer_lv->le_count += seg->area_len;
	layer_lv->size += seg->area_len * layer_lv->vg->extent_size;

	/* map the original area to the new segment */
	if (!set_lv_segment_area_lv(seg, s, layer_lv, mapseg->le, 0))
		return_0;

	return 1;
}

/*
 * Match the segment area to PEs in the pvl
 * (the segment area boundary should be aligned to PE ranges by
 *  _adjust_layer_segments() so that there is no partial overlap.)
 */
static int _match_seg_area_to_pe_range(struct lv_segment *seg, uint32_t s,
				       struct pv_list *pvl)
{
	struct pe_range *per;
	uint32_t pe_start, per_end;

	if (!pvl)
		return 1;

	if (seg_type(seg, s) != AREA_PV || seg_dev(seg, s) != pvl->pv->dev)
		return 0;

	pe_start = seg_pe(seg, s);

	/* Do these PEs match to any of the PEs in pvl? */
	dm_list_iterate_items(per, pvl->pe_ranges) {
		per_end = per->start + per->count - 1;

		if ((pe_start < per->start) || (pe_start > per_end))
			continue;

		/* FIXME Missing context in this message - add LV/seg details */
		log_debug("Matched PE range %s:%" PRIu32 "-%" PRIu32 " against "
			  "%s %" PRIu32 " len %" PRIu32, dev_name(pvl->pv->dev),
			  per->start, per_end, dev_name(seg_dev(seg, s)),
			  seg_pe(seg, s), seg->area_len);

		return 1;
	}

	return 0;
}

/*
 * For each segment in lv_where that uses a PV in pvl directly,
 * split the segment if it spans more than one underlying PV.
 */
static int _align_segment_boundary_to_pe_range(struct logical_volume *lv_where,
					       struct pv_list *pvl)
{
	struct lv_segment *seg;
	struct pe_range *per;
	uint32_t pe_start, pe_end, per_end, stripe_multiplier, s;

	if (!pvl)
		return 1;

	/* Split LV segments to match PE ranges */
	dm_list_iterate_items(seg, &lv_where->segments) {
		for (s = 0; s < seg->area_count; s++) {
			if (seg_type(seg, s) != AREA_PV ||
			    seg_dev(seg, s) != pvl->pv->dev)
				continue;

			/* Do these PEs match with the condition? */
			dm_list_iterate_items(per, pvl->pe_ranges) {
				pe_start = seg_pe(seg, s);
				pe_end = pe_start + seg->area_len - 1;
				per_end = per->start + per->count - 1;

				/* No overlap? */
				if ((pe_end < per->start) ||
				    (pe_start > per_end))
					continue;

				if (seg_is_striped(seg))
					stripe_multiplier = seg->area_count;
				else
					stripe_multiplier = 1;

				if ((per->start != pe_start &&
				     per->start > pe_start) &&
				    !lv_split_segment(lv_where, seg->le +
						      (per->start - pe_start) *
						      stripe_multiplier))
					return_0;

				if ((per_end != pe_end &&
				     per_end < pe_end) &&
				    !lv_split_segment(lv_where, seg->le +
						      (per_end - pe_start + 1) *
						      stripe_multiplier))
					return_0;
			}
		}
	}

	return 1;
}

/*
 * Scan lv_where for segments on a PV in pvl, and for each one found
 * append a linear segment to lv_layer and insert it between the two.
 *
 * If pvl is empty, a layer is placed under the whole of lv_where.
 * If the layer is inserted, lv_where is added to lvs_changed.
 */
int insert_layer_for_segments_on_pv(struct cmd_context *cmd,
				    struct logical_volume *lv_where,
				    struct logical_volume *layer_lv,
				    uint32_t status,
				    struct pv_list *pvl,
				    struct dm_list *lvs_changed)
{
	struct lv_segment *seg;
	struct lv_list *lvl;
	int lv_used = 0;
	uint32_t s;

	log_very_verbose("Inserting layer %s for segments of %s on %s",
			 layer_lv->name, lv_where->name,
			 pvl ? pv_dev_name(pvl->pv) : "any");

	if (!_align_segment_boundary_to_pe_range(lv_where, pvl))
		return_0;

	/* Work through all segments on the supplied PV */
	dm_list_iterate_items(seg, &lv_where->segments) {
		for (s = 0; s < seg->area_count; s++) {
			if (!_match_seg_area_to_pe_range(seg, s, pvl))
				continue;

			/* First time, add LV to list of LVs affected */
			if (!lv_used && lvs_changed) {
				if (!(lvl = dm_pool_alloc(cmd->mem, sizeof(*lvl)))) {
					log_error("lv_list alloc failed");
					return 0;
				}
				lvl->lv = lv_where;
				dm_list_add(lvs_changed, &lvl->list);
				lv_used = 1;
			}

			if (!_extend_layer_lv_for_segment(layer_lv, seg, s,
							  status)) {
				log_error("Failed to insert segment in layer "
					  "LV %s under %s:%" PRIu32 "-%" PRIu32,
					  layer_lv->name, lv_where->name,
					  seg->le, seg->le + seg->len);
				return 0;
			}
		}
	}

	return 1;
}

/*
 * Initialize the LV with 'value'.
 */
int set_lv(struct cmd_context *cmd, struct logical_volume *lv,
	   uint64_t sectors, int value)
{
	struct device *dev;
	char *name;

	/*
	 * FIXME:
	 * <clausen> also, more than 4k
	 * <clausen> say, reiserfs puts it's superblock 32k in, IIRC
	 * <ejt_> k, I'll drop a fixme to that effect
	 *	   (I know the device is at least 4k, but not 32k)
	 */
	if (!(name = dm_pool_alloc(cmd->mem, PATH_MAX))) {
		log_error("Name allocation failed - device not cleared");
		return 0;
	}
#ifdef __NetBSD__
	if (dm_snprintf(name, PATH_MAX, "%s%s/r%s", cmd->dev_dir,
			lv->vg->name, lv->name) < 0) {
		log_error("Name too long - device not cleared (%s)", lv->name);
		return 0;
	}
#else
	if (dm_snprintf(name, PATH_MAX, "%s%s/%s", cmd->dev_dir,
			lv->vg->name, lv->name) < 0) {
		log_error("Name too long - device not cleared (%s)", lv->name);
		return 0;
	}
#endif
	log_verbose("Clearing start of logical volume \"%s\"", lv->name);

	if (!(dev = dev_cache_get(name, NULL))) {
		log_error("%s: not found: device not cleared", name);
		return 0;
	}

	if (!dev_open_quiet(dev))
		return_0;

	if (!sectors)
		sectors = UINT64_C(4096) >> SECTOR_SHIFT;

	if (sectors > lv->size)
		sectors = lv->size;

	dev_set(dev, UINT64_C(0), (size_t) sectors << SECTOR_SHIFT, value);
	dev_flush(dev);
	dev_close_immediate(dev);

	return 1;
}


static struct logical_volume *_create_virtual_origin(struct cmd_context *cmd,
						     struct volume_group *vg,
						     const char *lv_name,
						     uint32_t permission,
						     uint64_t voriginextents)
{
	const struct segment_type *segtype;
	size_t len;
	char *vorigin_name;
	struct logical_volume *lv;

	if (!(segtype = get_segtype_from_string(cmd, "zero"))) {
		log_error("Zero segment type for virtual origin not found");
		return NULL;
	}

	len = strlen(lv_name) + 32;
	if (!(vorigin_name = alloca(len)) ||
	    dm_snprintf(vorigin_name, len, "%s_vorigin", lv_name) < 0) {
		log_error("Virtual origin name allocation failed.");
		return NULL;
	}

	if (!(lv = lv_create_empty(vorigin_name, NULL, permission,
				   ALLOC_INHERIT, vg)))
		return_NULL;

	if (!lv_extend(lv, segtype, 1, 0, 1, voriginextents, NULL, 0u, 0u,
		       NULL, ALLOC_INHERIT))
		return_NULL;

	/* store vg on disk(s) */
	if (!vg_write(vg) || !vg_commit(vg))
		return_NULL;

	backup(vg);

	return lv;
}

int lv_create_single(struct volume_group *vg,
		     struct lvcreate_params *lp)
{
	struct cmd_context *cmd = vg->cmd;
	uint32_t size_rest;
	uint32_t status = 0;
	struct logical_volume *lv, *org = NULL;
	int origin_active = 0;
	char lv_name_buf[128];
	const char *lv_name;
	struct lvinfo info;

	if (lp->lv_name && find_lv_in_vg(vg, lp->lv_name)) {
		log_error("Logical volume \"%s\" already exists in "
			  "volume group \"%s\"", lp->lv_name, lp->vg_name);
		return 0;
	}

	if (vg_max_lv_reached(vg)) {
		log_error("Maximum number of logical volumes (%u) reached "
			  "in volume group %s", vg->max_lv, vg->name);
		return 0;
	}

	if (lp->mirrors > 1 && !(vg->fid->fmt->features & FMT_SEGMENTS)) {
		log_error("Metadata does not support mirroring.");
		return 0;
	}

	if (lp->read_ahead != DM_READ_AHEAD_AUTO &&
	    lp->read_ahead != DM_READ_AHEAD_NONE &&
	    (vg->fid->fmt->features & FMT_RESTRICTED_READAHEAD) &&
	    (lp->read_ahead < 2 || lp->read_ahead > 120)) {
		log_error("Metadata only supports readahead values between 2 and 120.");
		return 0;
	}

	if (lp->stripe_size > vg->extent_size) {
		log_error("Reducing requested stripe size %s to maximum, "
			  "physical extent size %s",
			  display_size(cmd, (uint64_t) lp->stripe_size),
			  display_size(cmd, (uint64_t) vg->extent_size));
		lp->stripe_size = vg->extent_size;
	}

	/* Need to check the vg's format to verify this - the cmd format isn't setup properly yet */
	if (lp->stripes > 1 &&
	    !(vg->fid->fmt->features & FMT_UNLIMITED_STRIPESIZE) &&
	    (lp->stripe_size > STRIPE_SIZE_MAX)) {
		log_error("Stripe size may not exceed %s",
			  display_size(cmd, (uint64_t) STRIPE_SIZE_MAX));
		return 0;
	}

	if ((size_rest = lp->extents % lp->stripes)) {
		log_print("Rounding size (%d extents) up to stripe boundary "
			  "size (%d extents)", lp->extents,
			  lp->extents - size_rest + lp->stripes);
		lp->extents = lp->extents - size_rest + lp->stripes;
	}

	if (lp->zero && !activation()) {
		log_error("Can't wipe start of new LV without using "
			  "device-mapper kernel driver");
		return 0;
	}

	status |= lp->permission | VISIBLE_LV;

	if (lp->snapshot) {
		if (!activation()) {
			log_error("Can't create snapshot without using "
				  "device-mapper kernel driver");
			return 0;
		}
		/* FIXME Allow exclusive activation. */
		if (vg_is_clustered(vg)) {
			log_error("Clustered snapshots are not yet supported.");
			return 0;
		}

		/* Must zero cow */
		status |= LVM_WRITE;

		if (lp->voriginsize)
			origin_active = 1;
		else {

			if (!(org = find_lv(vg, lp->origin))) {
				log_error("Couldn't find origin volume '%s'.",
					  lp->origin);
				return 0;
			}
			if (lv_is_virtual_origin(org)) {
				log_error("Can't share virtual origins. "
					  "Use --virtualsize.");
				return 0;
			}
			if (lv_is_cow(org)) {
				log_error("Snapshots of snapshots are not "
					  "supported yet.");
				return 0;
			}
			if (org->status & LOCKED) {
				log_error("Snapshots of locked devices are not "
					  "supported yet");
				return 0;
			}
			if ((org->status & MIRROR_IMAGE) ||
			    (org->status & MIRROR_LOG)) {
				log_error("Snapshots of mirror %ss "
					  "are not supported",
					  (org->status & MIRROR_LOG) ?
					  "log" : "image");
				return 0;
			}

			if (!lv_info(cmd, org, &info, 0, 0)) {
				log_error("Check for existence of snapshot "
					  "origin '%s' failed.", org->name);
				return 0;
			}
			origin_active = info.exists;
		}
	}

	if (!lp->extents) {
		log_error("Unable to create new logical volume with no extents");
		return 0;
	}

	if (!seg_is_virtual(lp) &&
	    vg->free_count < lp->extents) {
		log_error("Insufficient free extents (%u) in volume group %s: "
			  "%u required", vg->free_count, vg->name, lp->extents);
		return 0;
	}

	if (lp->stripes > dm_list_size(lp->pvh) && lp->alloc != ALLOC_ANYWHERE) {
		log_error("Number of stripes (%u) must not exceed "
			  "number of physical volumes (%d)", lp->stripes,
			  dm_list_size(lp->pvh));
		return 0;
	}

	if (lp->mirrors > 1 && !activation()) {
		log_error("Can't create mirror without using "
			  "device-mapper kernel driver.");
		return 0;
	}

	/* The snapshot segment gets created later */
	if (lp->snapshot &&
	    !(lp->segtype = get_segtype_from_string(cmd, "striped")))
		return_0;

	if (!archive(vg))
		return 0;

	if (lp->lv_name)
		lv_name = lp->lv_name;
	else {
		if (!generate_lv_name(vg, "lvol%d", lv_name_buf, sizeof(lv_name_buf))) {
			log_error("Failed to generate LV name.");
			return 0;
		}
		lv_name = &lv_name_buf[0];
	}

	if (lp->tag) {
		if (!(vg->fid->fmt->features & FMT_TAGS)) {
			log_error("Volume group %s does not support tags",
				  vg->name);
			return 0;
		}
	}

	if (lp->mirrors > 1) {
		init_mirror_in_sync(lp->nosync);

		if (lp->nosync) {
			log_warn("WARNING: New mirror won't be synchronised. "
				  "Don't read what you didn't write!");
			status |= MIRROR_NOTSYNCED;
		}
	}

	if (!(lv = lv_create_empty(lv_name ? lv_name : "lvol%d", NULL,
				   status, lp->alloc, vg)))
		return_0;

	if (lp->read_ahead) {
		log_verbose("Setting read ahead sectors");
		lv->read_ahead = lp->read_ahead;
	}

	if (lp->minor >= 0) {
		lv->major = lp->major;
		lv->minor = lp->minor;
		lv->status |= FIXED_MINOR;
		log_verbose("Setting device number to (%d, %d)", lv->major,
			    lv->minor);
	}

	if (lp->tag && !str_list_add(cmd->mem, &lv->tags, lp->tag)) {
		log_error("Failed to add tag %s to %s/%s",
			  lp->tag, lv->vg->name, lv->name);
		return 0;
	}

	if (!lv_extend(lv, lp->segtype, lp->stripes, lp->stripe_size,
		       1, lp->extents, NULL, 0u, 0u, lp->pvh, lp->alloc))
		return_0;

	if (lp->mirrors > 1) {
		if (!lv_add_mirrors(cmd, lv, lp->mirrors - 1, lp->stripes,
				    adjusted_mirror_region_size(
						vg->extent_size,
						lv->le_count,
						lp->region_size),
				    lp->corelog ? 0U : 1U, lp->pvh, lp->alloc,
				    MIRROR_BY_LV |
				    (lp->nosync ? MIRROR_SKIP_INIT_SYNC : 0))) {
			stack;
			goto revert_new_lv;
		}
	}

	/* store vg on disk(s) */
	if (!vg_write(vg) || !vg_commit(vg))
		return_0;

	backup(vg);

	if (lp->snapshot) {
		if (!activate_lv_excl(cmd, lv)) {
			log_error("Aborting. Failed to activate snapshot "
				  "exception store.");
			goto revert_new_lv;
		}
	} else if (!activate_lv(cmd, lv)) {
		if (lp->zero) {
			log_error("Aborting. Failed to activate new LV to wipe "
				  "the start of it.");
			goto deactivate_and_revert_new_lv;
		}
		log_error("Failed to activate new LV.");
		return 0;
	}

	if (!lp->zero && !lp->snapshot)
		log_error("WARNING: \"%s\" not zeroed", lv->name);
	else if (!set_lv(cmd, lv, UINT64_C(0), 0)) {
		log_error("Aborting. Failed to wipe %s.",
			  lp->snapshot ? "snapshot exception store" :
					 "start of new LV");
		goto deactivate_and_revert_new_lv;
	}

	if (lp->snapshot) {
		/* Reset permission after zeroing */
		if (!(lp->permission & LVM_WRITE))
			lv->status &= ~LVM_WRITE;

		/* COW area must be deactivated if origin is not active */
		if (!origin_active && !deactivate_lv(cmd, lv)) {
			log_error("Aborting. Couldn't deactivate snapshot "
				  "COW area. Manual intervention required.");
			return 0;
		}

		/* A virtual origin must be activated explicitly. */
		if (lp->voriginsize &&
		    (!(org = _create_virtual_origin(cmd, vg, lv->name,
						    lp->permission,
						    lp->voriginextents)) ||
		     !activate_lv(cmd, org))) {
			log_error("Couldn't create virtual origin for LV %s",
				  lv->name);
			if (org && !lv_remove(org))
				stack;
			goto deactivate_and_revert_new_lv;
		}

		/* cow LV remains active and becomes snapshot LV */

		if (!vg_add_snapshot(org, lv, NULL,
				     org->le_count, lp->chunk_size)) {
			log_error("Couldn't create snapshot.");
			goto deactivate_and_revert_new_lv;
		}

		/* store vg on disk(s) */
		if (!vg_write(vg))
			return_0;

		if (!suspend_lv(cmd, org)) {
			log_error("Failed to suspend origin %s", org->name);
			vg_revert(vg);
			return 0;
		}

		if (!vg_commit(vg))
			return_0;

		if (!resume_lv(cmd, org)) {
			log_error("Problem reactivating origin %s", org->name);
			return 0;
		}
	}
	/* FIXME out of sequence */
	backup(vg);

	log_print("Logical volume \"%s\" created", lv->name);

	/*
	 * FIXME: as a sanity check we could try reading the
	 * last block of the device ?
	 */

	return 1;

deactivate_and_revert_new_lv:
	if (!deactivate_lv(cmd, lv)) {
		log_error("Unable to deactivate failed new LV. "
			  "Manual intervention required.");
		return 0;
	}

revert_new_lv:
	/* FIXME Better to revert to backup of metadata? */
	if (!lv_remove(lv) || !vg_write(vg) || !vg_commit(vg))
		log_error("Manual intervention may be required to remove "
			  "abandoned LV(s) before retrying.");
	else
		backup(vg);

	return 0;
}

