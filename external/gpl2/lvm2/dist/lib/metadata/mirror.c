/*	$NetBSD: mirror.c,v 1.1.1.1.2.1 2009/05/13 18:52:43 jym Exp $	*/

/*
 * Copyright (C) 2003-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2008 Red Hat, Inc. All rights reserved.
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
#include "segtype.h"
#include "display.h"
#include "archiver.h"
#include "activate.h"
#include "lv_alloc.h"
#include "lvm-string.h"
#include "str_list.h"
#include "locking.h"	/* FIXME Should not be used in this file */

#include "defaults.h" /* FIXME: should this be defaults.h? */

/* These are necessary for _write_log_header() */
#include "xlate.h"
#define MIRROR_MAGIC 0x4D695272
#define MIRROR_DISK_VERSION 2

/* These are the flags that represent the mirror failure restoration policies */
#define MIRROR_REMOVE		 0
#define MIRROR_ALLOCATE		 1
#define MIRROR_ALLOCATE_ANYWHERE 2

/*
 * Returns true if the lv is temporary mirror layer for resync
 */
int is_temporary_mirror_layer(const struct logical_volume *lv)
{
	if (lv->status & MIRROR_IMAGE
	    && lv->status & MIRRORED
	    && !(lv->status & LOCKED))
		return 1;

	return 0;
}

/*
 * Return a temporary LV for resyncing added mirror image.
 * Add other mirror legs to lvs list.
 */
struct logical_volume *find_temporary_mirror(const struct logical_volume *lv)
{
	struct lv_segment *seg;

	if (!(lv->status & MIRRORED))
		return NULL;

	seg = first_seg(lv);

	/* Temporary mirror is always area_num == 0 */
	if (seg_type(seg, 0) == AREA_LV &&
	    is_temporary_mirror_layer(seg_lv(seg, 0)))
		return seg_lv(seg, 0);

	return NULL;
}

/*
 * Returns the number of mirrors of the LV
 */
uint32_t lv_mirror_count(const struct logical_volume *lv)
{
	struct lv_segment *seg;
	uint32_t s, mirrors;

	if (!(lv->status & MIRRORED))
		return 1;

	seg = first_seg(lv);
	mirrors = seg->area_count;

	for (s = 0; s < seg->area_count; s++) {
		if (seg_type(seg, s) != AREA_LV)
			continue;
		if (is_temporary_mirror_layer(seg_lv(seg, s)))
			mirrors += lv_mirror_count(seg_lv(seg, s)) - 1;
	}

	return mirrors;
}

struct lv_segment *find_mirror_seg(struct lv_segment *seg)
{
	struct lv_segment *mirror_seg;

	mirror_seg = get_only_segment_using_this_lv(seg->lv);

	if (!mirror_seg) {
		log_error("Failed to find mirror_seg for %s", seg->lv->name);
		return NULL;
	}

	if (!seg_is_mirrored(mirror_seg)) {
		log_error("%s on %s is not a mirror segments",
			  mirror_seg->lv->name, seg->lv->name);
		return NULL;
	}

	return mirror_seg;
}

/*
 * Reduce the region size if necessary to ensure
 * the volume size is a multiple of the region size.
 */
uint32_t adjusted_mirror_region_size(uint32_t extent_size, uint32_t extents,
				     uint32_t region_size)
{
	uint64_t region_max;

	region_max = (1 << (ffs((int)extents) - 1)) * (uint64_t) extent_size;

	if (region_max < UINT32_MAX && region_size > region_max) {
		region_size = (uint32_t) region_max;
		log_print("Using reduced mirror region size of %" PRIu32
			  " sectors", region_size);
	}

	return region_size;
}

/*
 * shift_mirror_images
 * @mirrored_seg
 * @mimage:  The position (index) of the image to move to the end
 *
 * When dealing with removal of legs, we often move a 'removable leg'
 * to the back of the 'areas' array.  It is critically important not
 * to simply swap it for the last area in the array.  This would have
 * the affect of reordering the remaining legs - altering position of
 * the primary.  So, we must shuffle all of the areas in the array
 * to maintain their relative position before moving the 'removable
 * leg' to the end.
 *
 * Short illustration of the problem:
 *   - Mirror consists of legs A, B, C and we want to remove A
 *   - We swap A and C and then remove A, leaving C, B
 * This scenario is problematic in failure cases where A dies, because
 * B becomes the primary.  If the above happens, we effectively throw
 * away any changes made between the time of failure and the time of
 * restructuring the mirror.
 *
 * So, any time we want to move areas to the end to be removed, use
 * this function.
 */
int shift_mirror_images(struct lv_segment *mirrored_seg, unsigned mimage)
{
	int i;
	struct lv_segment_area area;

	if (mimage >= mirrored_seg->area_count) {
		log_error("Invalid index (%u) of mirror image supplied "
			  "to shift_mirror_images()", mimage);
		return 0;
	}

	area = mirrored_seg->areas[mimage];

	/* Shift remaining images down to fill the hole */
	for (i = mimage + 1; i < mirrored_seg->area_count; i++)
		mirrored_seg->areas[i-1] = mirrored_seg->areas[i];

	/* Place this one at the end */
	mirrored_seg->areas[i-1] = area;

	return 1;
}

/*
 * This function writes a new header to the mirror log header to the lv
 *
 * Returns: 1 on success, 0 on failure
 */
static int _write_log_header(struct cmd_context *cmd, struct logical_volume *lv)
{
	struct device *dev;
	char *name;
	struct { /* The mirror log header */
		uint32_t magic;
		uint32_t version;
		uint64_t nr_regions;
	} log_header;

	log_header.magic = xlate32(MIRROR_MAGIC);
	log_header.version = xlate32(MIRROR_DISK_VERSION);
	log_header.nr_regions = xlate64((uint64_t)-1);

	if (!(name = dm_pool_alloc(cmd->mem, PATH_MAX))) {
		log_error("Name allocation failed - log header not written (%s)",
			lv->name);
		return 0;
	}

	if (dm_snprintf(name, PATH_MAX, "%s%s/%s", cmd->dev_dir,
			 lv->vg->name, lv->name) < 0) {
		log_error("Name too long - log header not written (%s)", lv->name);
		return 0;
	}

	log_verbose("Writing log header to device, %s", lv->name);

	if (!(dev = dev_cache_get(name, NULL))) {
		log_error("%s: not found: log header not written", name);
		return 0;
	}

	if (!dev_open_quiet(dev))
		return 0;

	if (!dev_write(dev, UINT64_C(0), sizeof(log_header), &log_header)) {
		log_error("Failed to write log header to %s", name);
		dev_close_immediate(dev);
		return 0;
	}

	dev_close_immediate(dev);

	return 1;
}

/*
 * Initialize mirror log contents
 */
static int _init_mirror_log(struct cmd_context *cmd,
			    struct logical_volume *log_lv, int in_sync,
			    struct dm_list *tags, int remove_on_failure)
{
	struct str_list *sl;
	struct lvinfo info;
	uint32_t orig_status = log_lv->status;
	int was_active = 0;

	if (!activation() && in_sync) {
		log_error("Aborting. Unable to create in-sync mirror log "
			  "while activation is disabled.");
		return 0;
	}

	/* If the LV is active, deactivate it first. */
	if (lv_info(cmd, log_lv, &info, 0, 0) && info.exists) {
		if (!deactivate_lv(cmd, log_lv))
			return_0;
		was_active = 1;
	}

	/* Temporary make it visible for set_lv() */
	log_lv->status |= VISIBLE_LV;

	/* Temporary tag mirror log for activation */
	dm_list_iterate_items(sl, tags)
		if (!str_list_add(cmd->mem, &log_lv->tags, sl->str)) {
			log_error("Aborting. Unable to tag mirror log.");
			goto activate_lv;
		}

	/* store mirror log on disk(s) */
	if (!vg_write(log_lv->vg))
		goto activate_lv;

	backup(log_lv->vg);

	if (!vg_commit(log_lv->vg))
		goto activate_lv;

	if (!activate_lv(cmd, log_lv)) {
		log_error("Aborting. Failed to activate mirror log.");
		goto revert_new_lv;
	}

	/* Remove the temporary tags */
	dm_list_iterate_items(sl, tags)
		if (!str_list_del(&log_lv->tags, sl->str))
			log_error("Failed to remove tag %s from mirror log.",
				  sl->str);

	if (activation() && !set_lv(cmd, log_lv, log_lv->size,
				    in_sync ? -1 : 0)) {
		log_error("Aborting. Failed to wipe mirror log.");
		goto deactivate_and_revert_new_lv;
	}

	if (activation() && !_write_log_header(cmd, log_lv)) {
		log_error("Aborting. Failed to write mirror log header.");
		goto deactivate_and_revert_new_lv;
	}

	if (!deactivate_lv(cmd, log_lv)) {
		log_error("Aborting. Failed to deactivate mirror log. "
			  "Manual intervention required.");
		return 0;
	}

	log_lv->status &= ~VISIBLE_LV;

	if (was_active && !activate_lv(cmd, log_lv))
		return_0;

	return 1;

deactivate_and_revert_new_lv:
	if (!deactivate_lv(cmd, log_lv)) {
		log_error("Unable to deactivate mirror log LV. "
			  "Manual intervention required.");
		return 0;
	}

revert_new_lv:
	log_lv->status = orig_status;

	dm_list_iterate_items(sl, tags)
		if (!str_list_del(&log_lv->tags, sl->str))
			log_error("Failed to remove tag %s from mirror log.",
				  sl->str);

	if (remove_on_failure && !lv_remove(log_lv)) {
		log_error("Manual intervention may be required to remove "
			  "abandoned log LV before retrying.");
		return 0;
	}

	if (!vg_write(log_lv->vg) ||
	    (backup(log_lv->vg), !vg_commit(log_lv->vg)))
		log_error("Manual intervention may be required to "
			  "remove/restore abandoned log LV before retrying.");
activate_lv:
	if (was_active && !remove_on_failure && !activate_lv(cmd, log_lv))
		return_0;

	return 0;
}

/*
 * Delete independent/orphan LV, it must acquire lock.
 */
static int _delete_lv(struct logical_volume *mirror_lv, struct logical_volume *lv)
{
	struct cmd_context *cmd = mirror_lv->vg->cmd;
	struct str_list *sl;

	/* Inherit tags - maybe needed for activation */
	if (!str_list_match_list(&mirror_lv->tags, &lv->tags)) {
		dm_list_iterate_items(sl, &mirror_lv->tags)
			if (!str_list_add(cmd->mem, &lv->tags, sl->str)) {
				log_error("Aborting. Unable to tag.");
				return 0;
			}

		if (!vg_write(mirror_lv->vg) ||
		    !vg_commit(mirror_lv->vg)) {
			log_error("Intermediate VG commit for orphan volume failed.");
			return 0;
		}
	}

	if (!activate_lv(cmd, lv))
		return_0;

	if (!deactivate_lv(cmd, lv))
		return_0;

	if (!lv_remove(lv))
		return_0;

	return 1;
}

static int _merge_mirror_images(struct logical_volume *lv,
				const struct dm_list *mimages)
{
	uint32_t addition = dm_list_size(mimages);
	struct logical_volume **img_lvs;
	struct lv_list *lvl;
	int i = 0;

	if (!addition)
		return 1;

	if (!(img_lvs = alloca(sizeof(*img_lvs) * addition)))
		return_0;

	dm_list_iterate_items(lvl, mimages)
		img_lvs[i++] = lvl->lv;

	return lv_add_mirror_lvs(lv, img_lvs, addition,
				 MIRROR_IMAGE, first_seg(lv)->region_size);
}

/* Unlink the relationship between the segment and its log_lv */
struct logical_volume *detach_mirror_log(struct lv_segment *mirrored_seg)
{
	struct logical_volume *log_lv;

	if (!mirrored_seg->log_lv)
		return NULL;

	log_lv = mirrored_seg->log_lv;
	mirrored_seg->log_lv = NULL;
	log_lv->status |= VISIBLE_LV;
	log_lv->status &= ~MIRROR_LOG;
	remove_seg_from_segs_using_this_lv(log_lv, mirrored_seg);

	return log_lv;
}

/* Check if mirror image LV is removable with regard to given removable_pvs */
static int _is_mirror_image_removable(struct logical_volume *mimage_lv,
				      struct dm_list *removable_pvs)
{
	struct physical_volume *pv;
	struct lv_segment *seg;
	int pv_found;
	struct pv_list *pvl;
	uint32_t s;

	dm_list_iterate_items(seg, &mimage_lv->segments) {
		for (s = 0; s < seg->area_count; s++) {
			if (seg_type(seg, s) != AREA_PV) {
				/* FIXME Recurse for AREA_LV? */
				/* Structure of seg_lv is unknown.
				 * Not removing this LV for safety. */
				return 0;
			}

			pv = seg_pv(seg, s);

			pv_found = 0;
			dm_list_iterate_items(pvl, removable_pvs) {
				if (pv->dev->dev == pvl->pv->dev->dev) {
					pv_found = 1;
					break;
				}
			}
			if (!pv_found)
				return 0;
		}
	}

	return 1;
}

/*
 * Remove num_removed images from mirrored_seg
 *
 * Arguments:
 *   num_removed:   the requested (maximum) number of mirrors to be removed
 *   removable_pvs: if not NULL, only mirrors using PVs in this list
 *                  will be removed
 *   remove_log:    if non-zero, log_lv will be removed
 *                  (even if it's 0, log_lv will be removed if there is no
 *                   mirror remaining after the removal)
 *   collapse:      if non-zero, instead of removing, remove the temporary
 *                  mirror layer and merge mirrors to the original LV.
 *                  removable_pvs should be NULL and num_removed should be
 *                  seg->area_count - 1.
 *   removed:       if non NULL, the number of removed mirror images is set
 *                  as a result
 *
 * If collapse is non-zero, <removed> is guaranteed to be equal to num_removed.
 *
 * Return values:
 *   Failure (0) means something unexpected has happend and
 *   the caller should abort.
 *   Even if no mirror was removed (e.g. no LV matches to 'removable_pvs'),
 *   returns success (1).
 */
static int _remove_mirror_images(struct logical_volume *lv,
				 uint32_t num_removed,
				 struct dm_list *removable_pvs,
				 unsigned remove_log, unsigned collapse,
				 uint32_t *removed)
{
	uint32_t m;
	uint32_t s;
	struct logical_volume *sub_lv;
	struct logical_volume *detached_log_lv = NULL;
	struct logical_volume *lv1 = NULL;
	struct lv_segment *mirrored_seg = first_seg(lv);
	uint32_t old_area_count = mirrored_seg->area_count;
	uint32_t new_area_count = mirrored_seg->area_count;
	struct lv_list *lvl;
	struct dm_list tmp_orphan_lvs;

	if (removed)
		*removed = 0;

	log_very_verbose("Reducing mirror set from %" PRIu32 " to %"
			 PRIu32 " image(s)%s.",
			 old_area_count, old_area_count - num_removed,
			 remove_log ? " and no log volume" : "");

	if (collapse &&
	    (removable_pvs || (old_area_count - num_removed != 1))) {
		log_error("Incompatible parameters to _remove_mirror_images");
		return 0;
	}

	/* Move removable_pvs to end of array */
	if (removable_pvs) {
		for (s = 0; s < mirrored_seg->area_count &&
			    old_area_count - new_area_count < num_removed; s++) {
			sub_lv = seg_lv(mirrored_seg, s);

			if (!is_temporary_mirror_layer(sub_lv) &&
			    _is_mirror_image_removable(sub_lv, removable_pvs)) {
				if (!shift_mirror_images(mirrored_seg, s))
					return_0;
				new_area_count--;
			}
		}
		if (num_removed && old_area_count == new_area_count)
			return 1;
	} else
		new_area_count = old_area_count - num_removed;

	/* Remove mimage LVs from the segment */
	dm_list_init(&tmp_orphan_lvs);
	for (m = new_area_count; m < mirrored_seg->area_count; m++) {
		seg_lv(mirrored_seg, m)->status &= ~MIRROR_IMAGE;
		seg_lv(mirrored_seg, m)->status |= VISIBLE_LV;
		if (!(lvl = dm_pool_alloc(lv->vg->cmd->mem, sizeof(*lvl)))) {
			log_error("lv_list alloc failed");
			return 0;
		}
		lvl->lv = seg_lv(mirrored_seg, m);
		dm_list_add(&tmp_orphan_lvs, &lvl->list);
		release_lv_segment_area(mirrored_seg, m, mirrored_seg->area_len);
	}
	mirrored_seg->area_count = new_area_count;

	/* If no more mirrors, remove mirror layer */
	/* As an exceptional case, if the lv is temporary layer,
	 * leave the LV as mirrored and let the lvconvert completion
	 * to remove the layer. */
	if (new_area_count == 1 && !is_temporary_mirror_layer(lv)) {
		lv1 = seg_lv(mirrored_seg, 0);
		lv1->status &= ~MIRROR_IMAGE;
		lv1->status |= VISIBLE_LV;
		detached_log_lv = detach_mirror_log(mirrored_seg);
		if (!remove_layer_from_lv(lv, lv1))
			return_0;
		lv->status &= ~MIRRORED;
		lv->status &= ~MIRROR_NOTSYNCED;
		if (collapse && !_merge_mirror_images(lv, &tmp_orphan_lvs)) {
			log_error("Failed to add mirror images");
			return 0;
		}
	} else if (new_area_count == 0) {
		log_very_verbose("All mimages of %s are gone", lv->name);

		/* All mirror images are gone.
		 * It can happen for vgreduce --removemissing. */
		detached_log_lv = detach_mirror_log(mirrored_seg);
		lv->status &= ~MIRRORED;
		lv->status &= ~MIRROR_NOTSYNCED;
		if (!replace_lv_with_error_segment(lv))
			return_0;
	} else if (remove_log)
		detached_log_lv = detach_mirror_log(mirrored_seg);

	/*
	 * To successfully remove these unwanted LVs we need to
	 * remove the LVs from the mirror set, commit that metadata
	 * then deactivate and remove them fully.
	 */

	if (!vg_write(mirrored_seg->lv->vg)) {
		log_error("intermediate VG write failed.");
		return 0;
	}

	if (!suspend_lv(mirrored_seg->lv->vg->cmd, mirrored_seg->lv)) {
		log_error("Failed to lock %s", mirrored_seg->lv->name);
		vg_revert(mirrored_seg->lv->vg);
		return 0;
	}

	if (!vg_commit(mirrored_seg->lv->vg)) {
		resume_lv(mirrored_seg->lv->vg->cmd, mirrored_seg->lv);
		return 0;
	}

	log_very_verbose("Updating \"%s\" in kernel", mirrored_seg->lv->name);

	/*
	 * Avoid having same mirror target loaded twice simultaneously by first
	 * resuming the removed LV which now contains an error segment.
	 * As it's now detached from mirrored_seg->lv we must resume it
	 * explicitly.
	 */
	if (lv1 && !resume_lv(lv1->vg->cmd, lv1)) {
		log_error("Problem resuming temporary LV, %s", lv1->name);
		return 0;
	}

	if (!resume_lv(mirrored_seg->lv->vg->cmd, mirrored_seg->lv)) {
		log_error("Problem reactivating %s", mirrored_seg->lv->name);
		return 0;
	}

	/* Save or delete the 'orphan' LVs */
	if (!collapse) {
		dm_list_iterate_items(lvl, &tmp_orphan_lvs)
			if (!_delete_lv(lv, lvl->lv))
				return_0;
	}

	if (lv1 && !_delete_lv(lv, lv1))
		return_0;

	if (detached_log_lv && !_delete_lv(lv, detached_log_lv))
		return_0;

	/* Mirror with only 1 area is 'in sync'. */
	if (new_area_count == 1 && is_temporary_mirror_layer(lv)) {
		if (first_seg(lv)->log_lv &&
		    !_init_mirror_log(lv->vg->cmd, first_seg(lv)->log_lv,
				      1, &lv->tags, 0)) {
			/* As a result, unnecessary sync may run after
			 * collapsing. But safe.*/
			log_error("Failed to initialize log device");
			return_0;
		}
	}

	if (removed)
		*removed = old_area_count - new_area_count;

	log_very_verbose("%" PRIu32 " image(s) removed from %s",
			 old_area_count - num_removed, lv->name);

	return 1;
}

/*
 * Remove the number of mirror images from the LV
 */
int remove_mirror_images(struct logical_volume *lv, uint32_t num_mirrors,
			 struct dm_list *removable_pvs, unsigned remove_log)
{
	uint32_t num_removed, removed_once, r;
	uint32_t existing_mirrors = lv_mirror_count(lv);
	struct logical_volume *next_lv = lv;

	num_removed = existing_mirrors - num_mirrors;

	/* num_removed can be 0 if the function is called just to remove log */
	do {
		if (num_removed < first_seg(next_lv)->area_count)
			removed_once = num_removed;
		else
			removed_once = first_seg(next_lv)->area_count - 1;

		if (!_remove_mirror_images(next_lv, removed_once,
					   removable_pvs, remove_log, 0, &r))
			return_0;

		if (r < removed_once) {
			/* Some mirrors are removed from the temporary mirror,
			 * but the temporary layer still exists.
			 * Down the stack and retry for remainder. */
			next_lv = find_temporary_mirror(next_lv);
		}

		num_removed -= r;
	} while (next_lv && num_removed);

	if (num_removed) {
		if (num_removed == existing_mirrors - num_mirrors)
			log_error("No mirror images found using specified PVs.");
		else {
			log_error("%u images are removed out of requested %u.",
				  existing_mirrors - lv_mirror_count(lv),
				  existing_mirrors - num_mirrors);
		}
		return 0;
	}

	return 1;
}

static int _mirrored_lv_in_sync(struct logical_volume *lv)
{
	float sync_percent;

	if (!lv_mirror_percent(lv->vg->cmd, lv, 0, &sync_percent, NULL)) {
		log_error("Unable to determine mirror sync status of %s/%s.",
			  lv->vg->name, lv->name);
		return 0;
	}

	if (sync_percent >= 100.0)
		return 1;

	return 0;
}

/*
 * Collapsing temporary mirror layers.
 *
 * When mirrors are added to already-mirrored LV, a temporary mirror layer
 * is inserted at the top of the stack to reduce resync work.
 * The function will remove the intermediate layer and collapse the stack
 * as far as mirrors are in-sync.
 *
 * The function is destructive: to remove intermediate mirror layers,
 * VG metadata commits and suspend/resume are necessary.
 */
int collapse_mirrored_lv(struct logical_volume *lv)
{
	struct logical_volume *tmp_lv;
	struct lv_segment *mirror_seg;

	while ((tmp_lv = find_temporary_mirror(lv))) {
		mirror_seg = find_mirror_seg(first_seg(tmp_lv));
		if (!mirror_seg) {
			log_error("Failed to find mirrored LV for %s",
				  tmp_lv->name);
			return 0;
		}

		if (!_mirrored_lv_in_sync(mirror_seg->lv)) {
			log_verbose("Not collapsing %s: out-of-sync",
				    mirror_seg->lv->name);
			return 1;
		}

		if (!_remove_mirror_images(mirror_seg->lv,
					   mirror_seg->area_count - 1,
					   NULL, 1, 1, NULL)) {
			log_error("Failed to release mirror images");
			return 0;
		}
	}

	return 1;
}

static int get_mirror_fault_policy(struct cmd_context *cmd __attribute((unused)),
				   int log_policy)
{
	const char *policy;

	if (log_policy)
		policy = find_config_str(NULL, "activation/mirror_log_fault_policy",
					 DEFAULT_MIRROR_LOG_FAULT_POLICY);
	else
		policy = find_config_str(NULL, "activation/mirror_device_fault_policy",
					 DEFAULT_MIRROR_DEV_FAULT_POLICY);

	if (!strcmp(policy, "remove"))
		return MIRROR_REMOVE;
	else if (!strcmp(policy, "allocate"))
		return MIRROR_ALLOCATE;
	else if (!strcmp(policy, "allocate_anywhere"))
		return MIRROR_ALLOCATE_ANYWHERE;

	if (log_policy)
		log_error("Bad activation/mirror_log_fault_policy");
	else
		log_error("Bad activation/mirror_device_fault_policy");

	return MIRROR_REMOVE;
}

static int get_mirror_log_fault_policy(struct cmd_context *cmd)
{
	return get_mirror_fault_policy(cmd, 1);
}

static int get_mirror_device_fault_policy(struct cmd_context *cmd)
{
	return get_mirror_fault_policy(cmd, 0);
}

/*
 * replace_mirror_images
 * @mirrored_seg: segment (which may be linear now) to restore
 * @num_mirrors: number of copies we should end up with
 * @replace_log: replace log if not present
 * @in_sync: was the original mirror in-sync?
 *
 * in_sync will be set to 0 if new mirror devices are being added
 * In other words, it is only useful if the log (and only the log)
 * is being restored.
 *
 * Returns: 0 on failure, 1 on reconfig, -1 if no reconfig done
 */
static int replace_mirror_images(struct lv_segment *mirrored_seg,
				 uint32_t num_mirrors,
				 int log_policy, int in_sync)
{
	int r = -1;
	struct logical_volume *lv = mirrored_seg->lv;

	/* FIXME: Use lvconvert rather than duplicating its code */

	if (mirrored_seg->area_count < num_mirrors) {
		log_error("WARNING: Failed to replace mirror device in %s/%s",
			  mirrored_seg->lv->vg->name, mirrored_seg->lv->name);

		if ((mirrored_seg->area_count > 1) && !mirrored_seg->log_lv)
			log_error("WARNING: Use 'lvconvert -m %d %s/%s --corelog' to replace failed devices",
				  num_mirrors - 1, lv->vg->name, lv->name);
		else
			log_error("WARNING: Use 'lvconvert -m %d %s/%s' to replace failed devices",
				  num_mirrors - 1, lv->vg->name, lv->name);
		r = 0;

		/* REMEMBER/FIXME: set in_sync to 0 if a new mirror device was added */
		in_sync = 0;
	}

	/*
	 * FIXME: right now, we ignore the allocation policy specified to
	 * allocate the new log.
	 */
	if ((mirrored_seg->area_count > 1) && !mirrored_seg->log_lv &&
	    (log_policy != MIRROR_REMOVE)) {
		log_error("WARNING: Failed to replace mirror log device in %s/%s",
			  lv->vg->name, lv->name);

		log_error("WARNING: Use 'lvconvert -m %d %s/%s' to replace failed devices",
			  mirrored_seg->area_count - 1 , lv->vg->name, lv->name);
		r = 0;
	}

	return r;
}

int reconfigure_mirror_images(struct lv_segment *mirrored_seg, uint32_t num_mirrors,
			      struct dm_list *removable_pvs, unsigned remove_log)
{
	int r;
	int in_sync;
	int log_policy, dev_policy;
	uint32_t old_num_mirrors = mirrored_seg->area_count;
	int had_log = (mirrored_seg->log_lv) ? 1 : 0;

	/* was the mirror in-sync before problems? */
	in_sync = _mirrored_lv_in_sync(mirrored_seg->lv);

	/*
	 * While we are only removing devices, we can have sync set.
	 * Setting this is only useful if we are moving to core log
	 * otherwise the disk log will contain the sync information
	 */
	init_mirror_in_sync(in_sync);

	r = _remove_mirror_images(mirrored_seg->lv, old_num_mirrors - num_mirrors,
				  removable_pvs, remove_log, 0, NULL);
	if (!r)
		/* Unable to remove bad devices */
		return 0;

	log_warn("WARNING: Bad device removed from mirror volume, %s/%s",
		  mirrored_seg->lv->vg->name, mirrored_seg->lv->name);

	log_policy = get_mirror_log_fault_policy(mirrored_seg->lv->vg->cmd);
	dev_policy = get_mirror_device_fault_policy(mirrored_seg->lv->vg->cmd);

	r = replace_mirror_images(mirrored_seg,
				  (dev_policy != MIRROR_REMOVE) ?
				  old_num_mirrors : num_mirrors,
				  log_policy, in_sync);

	if (!r)
		/* Failed to replace device(s) */
		log_error("WARNING: Unable to find substitute device for mirror volume, %s/%s",
			  mirrored_seg->lv->vg->name, mirrored_seg->lv->name);
	else if (r > 0)
		/* Success in replacing device(s) */
		log_warn("WARNING: Mirror volume, %s/%s restored - substitute for failed device found.",
			  mirrored_seg->lv->vg->name, mirrored_seg->lv->name);
	else
		/* Bad device removed, but not replaced because of policy */
		if (mirrored_seg->area_count == 1) {
			log_warn("WARNING: Mirror volume, %s/%s converted to linear due to device failure.",
				  mirrored_seg->lv->vg->name, mirrored_seg->lv->name);
		} else if (had_log && !mirrored_seg->log_lv) {
			log_warn("WARNING: Mirror volume, %s/%s disk log removed due to device failure.",
				  mirrored_seg->lv->vg->name, mirrored_seg->lv->name);
		}
	/*
	 * If we made it here, we at least removed the bad device.
	 * Consider this success.
	 */
	return 1;
}

static int _create_mimage_lvs(struct alloc_handle *ah,
			      uint32_t num_mirrors,
			      struct logical_volume *lv,
			      struct logical_volume **img_lvs)
{
	uint32_t m;
	char *img_name;
	size_t len;
	
	len = strlen(lv->name) + 32;
	if (!(img_name = alloca(len))) {
		log_error("img_name allocation failed. "
			  "Remove new LV and retry.");
		return 0;
	}

	if (dm_snprintf(img_name, len, "%s_mimage_%%d", lv->name) < 0) {
		log_error("img_name allocation failed. "
			  "Remove new LV and retry.");
		return 0;
	}

	for (m = 0; m < num_mirrors; m++) {
		if (!(img_lvs[m] = lv_create_empty(img_name,
					     NULL, LVM_READ | LVM_WRITE,
					     ALLOC_INHERIT, 0, lv->vg))) {
			log_error("Aborting. Failed to create mirror image LV. "
				  "Remove new LV and retry.");
			return 0;
		}

		if (!lv_add_segment(ah, m, 1, img_lvs[m],
				    get_segtype_from_string(lv->vg->cmd,
							    "striped"),
				    0, 0, 0, NULL)) {
			log_error("Aborting. Failed to add mirror image segment "
				  "to %s. Remove new LV and retry.",
				  img_lvs[m]->name);
			return 0;
		}
	}

	return 1;
}

/*
 * Remove mirrors from each segment.
 * 'new_mirrors' is the number of mirrors after the removal. '0' for linear.
 * If 'status_mask' is non-zero, the removal happens only when all segments
 * has the status bits on.
 */
int remove_mirrors_from_segments(struct logical_volume *lv,
				 uint32_t new_mirrors, uint32_t status_mask)
{
	struct lv_segment *seg;
	uint32_t s;

	/* Check the segment params are compatible */
	dm_list_iterate_items(seg, &lv->segments) {
		if (!seg_is_mirrored(seg)) {
			log_error("Segment is not mirrored: %s:%" PRIu32,
				  lv->name, seg->le);
			return 0;
		} if ((seg->status & status_mask) != status_mask) {
			log_error("Segment status does not match: %s:%" PRIu32
				  " status:0x%x/0x%x", lv->name, seg->le,
				  seg->status, status_mask);
			return 0;
		}
	}

	/* Convert the segments */
	dm_list_iterate_items(seg, &lv->segments) {
		if (!new_mirrors && seg->extents_copied == seg->area_len) {
			if (!move_lv_segment_area(seg, 0, seg, 1))
				return_0;
		}

		for (s = new_mirrors + 1; s < seg->area_count; s++)
			release_lv_segment_area(seg, s, seg->area_len);

		seg->area_count = new_mirrors + 1;

		if (!new_mirrors)
			seg->segtype = get_segtype_from_string(lv->vg->cmd,
							       "striped");
	}

	return 1;
}

const char *get_pvmove_pvname_from_lv_mirr(struct logical_volume *lv_mirr)
{
	struct lv_segment *seg;

	dm_list_iterate_items(seg, &lv_mirr->segments) {
		if (!seg_is_mirrored(seg))
			continue;
		if (seg_type(seg, 0) != AREA_PV)
			continue;
		return dev_name(seg_dev(seg, 0));
	}

	return NULL;
}

const char *get_pvmove_pvname_from_lv(struct logical_volume *lv)
{
	struct lv_segment *seg;
	uint32_t s;

	dm_list_iterate_items(seg, &lv->segments) {
		for (s = 0; s < seg->area_count; s++) {
			if (seg_type(seg, s) != AREA_LV)
				continue;
			return get_pvmove_pvname_from_lv_mirr(seg_lv(seg, s));
		}
	}

	return NULL;
}

struct logical_volume *find_pvmove_lv(struct volume_group *vg,
				      struct device *dev,
				      uint32_t lv_type)
{
	struct lv_list *lvl;
	struct logical_volume *lv;
	struct lv_segment *seg;

	/* Loop through all LVs */
	dm_list_iterate_items(lvl, &vg->lvs) {
		lv = lvl->lv;

		if (!(lv->status & lv_type))
			continue;

		/* Check segment origins point to pvname */
		dm_list_iterate_items(seg, &lv->segments) {
			if (seg_type(seg, 0) != AREA_PV)
				continue;
			if (seg_dev(seg, 0) != dev)
				continue;
			return lv;
		}
	}

	return NULL;
}

struct logical_volume *find_pvmove_lv_from_pvname(struct cmd_context *cmd,
					 	  struct volume_group *vg,
				      		  const char *name,
				      		  uint32_t lv_type)
{
	struct physical_volume *pv;

	if (!(pv = find_pv_by_name(cmd, name)))
		return_NULL;

	return find_pvmove_lv(vg, pv->dev, lv_type);
}

struct dm_list *lvs_using_lv(struct cmd_context *cmd, struct volume_group *vg,
			  struct logical_volume *lv)
{
	struct dm_list *lvs;
	struct logical_volume *lv1;
	struct lv_list *lvl, *lvl1;
	struct lv_segment *seg;
	uint32_t s;

	if (!(lvs = dm_pool_alloc(cmd->mem, sizeof(*lvs)))) {
		log_error("lvs list alloc failed");
		return NULL;
	}

	dm_list_init(lvs);

	/* Loop through all LVs except the one supplied */
	dm_list_iterate_items(lvl1, &vg->lvs) {
		lv1 = lvl1->lv;
		if (lv1 == lv)
			continue;

		/* Find whether any segment points at the supplied LV */
		dm_list_iterate_items(seg, &lv1->segments) {
			for (s = 0; s < seg->area_count; s++) {
				if (seg_type(seg, s) != AREA_LV ||
				    seg_lv(seg, s) != lv)
					continue;
				if (!(lvl = dm_pool_alloc(cmd->mem, sizeof(*lvl)))) {
					log_error("lv_list alloc failed");
					return NULL;
				}
				lvl->lv = lv1;
				dm_list_add(lvs, &lvl->list);
				goto next_lv;
			}
		}
	      next_lv:
		;
	}

	return lvs;
}

float copy_percent(struct logical_volume *lv_mirr)
{
	uint32_t numerator = 0u, denominator = 0u;
	struct lv_segment *seg;

	dm_list_iterate_items(seg, &lv_mirr->segments) {
		denominator += seg->area_len;

		if (seg_is_mirrored(seg) && seg->area_count > 1)
			numerator += seg->extents_copied;
		else
			numerator += seg->area_len;
	}

	return denominator ? (float) numerator *100 / denominator : 100.0;
}

/*
 * Fixup mirror pointers after single-pass segment import
 */
int fixup_imported_mirrors(struct volume_group *vg)
{
	struct lv_list *lvl;
	struct lv_segment *seg;

	dm_list_iterate_items(lvl, &vg->lvs) {
		dm_list_iterate_items(seg, &lvl->lv->segments) {
			if (seg->segtype !=
			    get_segtype_from_string(vg->cmd, "mirror"))
				continue;

			if (seg->log_lv && !add_seg_to_segs_using_this_lv(seg->log_lv, seg))
				return_0;
		}
	}

	return 1;
}

/*
 * Add mirrors to "linear" or "mirror" segments
 */
int add_mirrors_to_segments(struct cmd_context *cmd, struct logical_volume *lv,
			    uint32_t mirrors, uint32_t region_size,
			    struct dm_list *allocatable_pvs, alloc_policy_t alloc)
{
	struct alloc_handle *ah;
	const struct segment_type *segtype;
	struct dm_list *parallel_areas;
	uint32_t adjusted_region_size;

	if (!(parallel_areas = build_parallel_areas_from_lv(cmd, lv)))
		return_0;

	if (!(segtype = get_segtype_from_string(cmd, "mirror")))
		return_0;

	adjusted_region_size = adjusted_mirror_region_size(lv->vg->extent_size,
							   lv->le_count,
							   region_size);

	if (!(ah = allocate_extents(lv->vg, NULL, segtype, 1, mirrors, 0, 0,
				    lv->le_count, allocatable_pvs, alloc,
				    parallel_areas))) {
		log_error("Unable to allocate mirror extents for %s.", lv->name);
		return 0;
	}

	if (!lv_add_mirror_areas(ah, lv, 0, adjusted_region_size)) {
		log_error("Failed to add mirror areas to %s", lv->name);
		return 0;
	}

	return 1;
}

/*
 * Convert mirror log
 *
 * FIXME: Can't handle segment-by-segment mirror (like pvmove)
 */
int remove_mirror_log(struct cmd_context *cmd,
		      struct logical_volume *lv,
		      struct dm_list *removable_pvs)
{
	float sync_percent;
	struct lvinfo info;
	struct volume_group *vg = lv->vg;

	/* Unimplemented features */
	if (dm_list_size(&lv->segments) != 1) {
		log_error("Multiple-segment mirror is not supported");
		return 0;
	}

	/* Had disk log, switch to core. */
	if (lv_info(cmd, lv, &info, 0, 0) && info.exists) {
		if (!lv_mirror_percent(cmd, lv, 0, &sync_percent, NULL)) {
			log_error("Unable to determine mirror sync status.");
			return 0;
		}
	} else if (vg_is_clustered(vg)) {
		log_error("Unable to convert the log of inactive "
			  "cluster mirror %s", lv->name);
		return 0;
	} else if (yes_no_prompt("Full resync required to convert "
				 "inactive mirror %s to core log. "
				 "Proceed? [y/n]: "))
		sync_percent = 0;
	else
		return 0;

	if (sync_percent >= 100.0)
		init_mirror_in_sync(1);
	else {
		/* A full resync will take place */
		lv->status &= ~MIRROR_NOTSYNCED;
		init_mirror_in_sync(0);
	}

	if (!remove_mirror_images(lv, lv_mirror_count(lv),
				  removable_pvs, 1U))
		return_0;

	return 1;
}

static struct logical_volume *_create_mirror_log(struct logical_volume *lv,
						 struct alloc_handle *ah,
						 alloc_policy_t alloc,
						 const char *lv_name,
						 const char *suffix)
{
	struct logical_volume *log_lv;
	char *log_name;
	size_t len;

	len = strlen(lv_name) + 32;
	if (!(log_name = alloca(len))) {
		log_error("log_name allocation failed.");
		return NULL;
	}

	if (dm_snprintf(log_name, len, "%s%s", lv_name, suffix) < 0) {
		log_error("log_name allocation failed.");
		return NULL;
	}

	if (!(log_lv = lv_create_empty(log_name, NULL,
				       VISIBLE_LV | LVM_READ | LVM_WRITE,
				       alloc, 0, lv->vg)))
		return_NULL;

	if (!lv_add_log_segment(ah, log_lv))
		return_NULL;

	return log_lv;
}

static struct logical_volume *_set_up_mirror_log(struct cmd_context *cmd,
						 struct alloc_handle *ah,
						 struct logical_volume *lv,
						 uint32_t log_count,
						 uint32_t region_size __attribute((unused)),
						 alloc_policy_t alloc,
						 int in_sync)
{
	struct logical_volume *log_lv;
	const char *suffix, *c;
	char *lv_name;
	size_t len;
	struct lv_segment *seg;

	init_mirror_in_sync(in_sync);

	if (log_count != 1) {
		log_error("log_count != 1 is not supported.");
		return NULL;
	}

	/* Mirror log name is lv_name + suffix, determined as the following:
	 *   1. suffix is:
	 *        o "_mlog" for the original mirror LV.
	 *        o "_mlogtmp_%d" for temporary mirror LV,
	 *   2. lv_name is:
	 *        o lv->name, if the log is temporary
	 *        o otherwise, the top-level LV name
	 */
	seg = first_seg(lv);
	if (seg_type(seg, 0) == AREA_LV &&
	    strstr(seg_lv(seg, 0)->name, MIRROR_SYNC_LAYER)) {
		lv_name = lv->name;
		suffix = "_mlogtmp_%d";
	} else if ((c = strstr(lv->name, MIRROR_SYNC_LAYER))) {
		len = c - lv->name + 1;
		if (!(lv_name = alloca(len)) ||
		    !dm_snprintf(lv_name, len, "%s", lv->name)) {
			log_error("mirror log name allocation failed");
			return 0;
		}
		suffix = "_mlog";
	} else {
		lv_name = lv->name;
		suffix = "_mlog";
	}

	if (!(log_lv = _create_mirror_log(lv, ah, alloc,
					  (const char *) lv_name, suffix))) {
		log_error("Failed to create mirror log.");
		return NULL;
	}

	if (!_init_mirror_log(cmd, log_lv, in_sync, &lv->tags, 1)) {
		log_error("Failed to create mirror log.");
		return NULL;
	}

	return log_lv;
}

int attach_mirror_log(struct lv_segment *seg, struct logical_volume *log_lv)
{
	seg->log_lv = log_lv;
	log_lv->status |= MIRROR_LOG;
	log_lv->status &= ~VISIBLE_LV;
	return add_seg_to_segs_using_this_lv(log_lv, seg);
}

int add_mirror_log(struct cmd_context *cmd, struct logical_volume *lv,
		   uint32_t log_count, uint32_t region_size,
		   struct dm_list *allocatable_pvs, alloc_policy_t alloc)
{
	struct alloc_handle *ah;
	const struct segment_type *segtype;
	struct dm_list *parallel_areas;
	float sync_percent;
	int in_sync;
	struct logical_volume *log_lv;
	struct lvinfo info;

	/* Unimplemented features */
	if (log_count > 1) {
		log_error("log_count > 1 is not supported");
		return 0;
	}

	if (dm_list_size(&lv->segments) != 1) {
		log_error("Multiple-segment mirror is not supported");
		return 0;
	}

	/*
	 * We are unable to convert the log of inactive cluster mirrors
	 * due to the inability to detect whether the mirror is active
	 * on remote nodes (even though it is inactive on this node)
	 */
	if (vg_is_clustered(lv->vg) &&
	    !(lv_info(cmd, lv, &info, 0, 0) && info.exists)) {
		log_error("Unable to convert the log of inactive "
			  "cluster mirror %s", lv->name);
		return 0;
	}

	if (!(parallel_areas = build_parallel_areas_from_lv(cmd, lv)))
		return_0;

	if (!(segtype = get_segtype_from_string(cmd, "mirror")))
		return_0;

	if (activation() && segtype->ops->target_present &&
	    !segtype->ops->target_present(NULL, NULL)) {
		log_error("%s: Required device-mapper target(s) not "
			  "detected in your kernel", segtype->name);
		return 0;
	}

	/* allocate destination extents */
	ah = allocate_extents(lv->vg, NULL, segtype,
			      0, 0, log_count, region_size, 0,
			      allocatable_pvs, alloc, parallel_areas);
	if (!ah) {
		log_error("Unable to allocate extents for mirror log.");
		return 0;
	}

	/* check sync status */
	if (lv_mirror_percent(cmd, lv, 0, &sync_percent, NULL) &&
	    sync_percent >= 100.0)
		in_sync = 1;
	else
		in_sync = 0;

	if (!(log_lv = _set_up_mirror_log(cmd, ah, lv, log_count,
					  region_size, alloc, in_sync)))
		return_0;

	if (!attach_mirror_log(first_seg(lv), log_lv))
		return_0;

	alloc_destroy(ah);
	return 1;
}

/*
 * Convert "linear" LV to "mirror".
 */
int add_mirror_images(struct cmd_context *cmd, struct logical_volume *lv,
		      uint32_t mirrors, uint32_t stripes, uint32_t region_size,
		      struct dm_list *allocatable_pvs, alloc_policy_t alloc,
		      uint32_t log_count)
{
	struct alloc_handle *ah;
	const struct segment_type *segtype;
	struct dm_list *parallel_areas;
	struct logical_volume **img_lvs;
	struct logical_volume *log_lv = NULL;

	if (stripes > 1) {
		log_error("stripes > 1 is not supported");
		return 0;
	}

	/*
	 * allocate destination extents
	 */

	if (!(parallel_areas = build_parallel_areas_from_lv(cmd, lv)))
		return_0;

	if (!(segtype = get_segtype_from_string(cmd, "mirror")))
		return_0;

	ah = allocate_extents(lv->vg, NULL, segtype,
			      stripes, mirrors, log_count, region_size, lv->le_count,
			      allocatable_pvs, alloc, parallel_areas);
	if (!ah) {
		log_error("Unable to allocate extents for mirror(s).");
		return 0;
	}

	/*
	 * create and initialize mirror log
	 */
	if (log_count &&
	    !(log_lv = _set_up_mirror_log(cmd, ah, lv, log_count, region_size,
					  alloc, mirror_in_sync())))
		return_0;

	/* The log initialization involves vg metadata commit.
	   So from here on, if failure occurs, the log must be explicitly
	   removed and the updated vg metadata should be committed. */

	/*
	 * insert a mirror layer
	 */
	if (dm_list_size(&lv->segments) != 1 ||
	    seg_type(first_seg(lv), 0) != AREA_LV)
		if (!insert_layer_for_lv(cmd, lv, 0, "_mimage_%d"))
			goto out_remove_log;

	/*
	 * create mirror image LVs
	 */
	if (!(img_lvs = alloca(sizeof(*img_lvs) * mirrors))) {
		log_error("img_lvs allocation failed. "
			  "Remove new LV and retry.");
		goto out_remove_log;
	}

	if (!_create_mimage_lvs(ah, mirrors, lv, img_lvs))
		goto out_remove_log;

	if (!lv_add_mirror_lvs(lv, img_lvs, mirrors,
			       MIRROR_IMAGE | (lv->status & LOCKED),
			       region_size)) {
		log_error("Aborting. Failed to add mirror segment. "
			  "Remove new LV and retry.");
		goto out_remove_imgs;
	}

	if (log_count && !attach_mirror_log(first_seg(lv), log_lv))
		stack;

	alloc_destroy(ah);
	return 1;

  out_remove_log:
	if (log_lv && (!lv_remove(log_lv) || !vg_write(log_lv->vg) ||
		       (backup(log_lv->vg), !vg_commit(log_lv->vg))))
		log_error("Manual intervention may be required to remove "
			  "abandoned log LV before retrying.");

  out_remove_imgs:
	return 0;
}

/*
 * Generic interface for adding mirror and/or mirror log.
 * 'mirror' is the number of mirrors to be added.
 * 'pvs' is either allocatable pvs.
 */
int lv_add_mirrors(struct cmd_context *cmd, struct logical_volume *lv,
		   uint32_t mirrors, uint32_t stripes,
		   uint32_t region_size, uint32_t log_count,
		   struct dm_list *pvs, alloc_policy_t alloc, uint32_t flags)
{
	if (!mirrors && !log_count) {
		log_error("No conversion is requested");
		return 0;
	}

	/* For corelog mirror, activation code depends on
	 * the global mirror_in_sync status. As we are adding
	 * a new mirror, it should be set as 'out-of-sync'
	 * so that the sync starts. */
	/* However, MIRROR_SKIP_INIT_SYNC even overrides it. */
	if (flags & MIRROR_SKIP_INIT_SYNC)
		init_mirror_in_sync(1);
	else if (!log_count)
		init_mirror_in_sync(0);

	if (flags & MIRROR_BY_SEG) {
		if (log_count) {
			log_error("Persistent log is not supported on "
				  "segment-by-segment mirroring");
			return 0;
		}
		if (stripes > 1) {
			log_error("Striped-mirroring is not supported on "
				  "segment-by-segment mirroring");
			return 0;
		}

		return add_mirrors_to_segments(cmd, lv, mirrors,
					       region_size, pvs, alloc);
	} else if (flags & MIRROR_BY_LV) {
		if (!mirrors)
			return add_mirror_log(cmd, lv, log_count,
					      region_size, pvs, alloc);
		return add_mirror_images(cmd, lv, mirrors,
					 stripes, region_size,
					 pvs, alloc, log_count);
	}

	log_error("Unsupported mirror conversion type");
	return 0;
}

/*
 * Generic interface for removing mirror and/or mirror log.
 * 'mirror' is the number of mirrors to be removed.
 * 'pvs' is removable pvs.
 */
int lv_remove_mirrors(struct cmd_context *cmd __attribute((unused)),
		      struct logical_volume *lv,
		      uint32_t mirrors, uint32_t log_count, struct dm_list *pvs,
		      uint32_t status_mask)
{
	uint32_t new_mirrors;
	struct lv_segment *seg;

	if (!mirrors && !log_count) {
		log_error("No conversion is requested");
		return 0;
	}

	seg = first_seg(lv);
	if (!seg_is_mirrored(seg)) {
		log_error("Not a mirror segment");
		return 0;
	}

	if (lv_mirror_count(lv) <= mirrors) {
		log_error("Removing more than existing: %d <= %d",
			  seg->area_count, mirrors);
		return 0;
	}
	new_mirrors = lv_mirror_count(lv) - mirrors - 1;

	/* MIRROR_BY_LV */
	if (seg_type(seg, 0) == AREA_LV &&
	    seg_lv(seg, 0)->status & MIRROR_IMAGE)
		return remove_mirror_images(lv, new_mirrors + 1,
					    pvs, log_count ? 1U : 0);

	/* MIRROR_BY_SEG */
	if (log_count) {
		log_error("Persistent log is not supported on "
			  "segment-by-segment mirroring");
		return 0;
	}
	return remove_mirrors_from_segments(lv, new_mirrors, status_mask);
}

