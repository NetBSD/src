/*	$NetBSD: vgreduce.c,v 1.1.1.2 2009/12/02 00:25:57 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2009 Red Hat, Inc. All rights reserved.
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

#include "tools.h"
#include "lv_alloc.h"

static int _remove_pv(struct volume_group *vg, struct pv_list *pvl, int silent)
{
	char uuid[64] __attribute((aligned(8)));

	if (vg->pv_count == 1) {
		log_error("Volume Groups must always contain at least one PV");
		return 0;
	}

	if (!id_write_format(&pvl->pv->id, uuid, sizeof(uuid)))
		return_0;

	log_verbose("Removing PV with UUID %s from VG %s", uuid, vg->name);

	if (pvl->pv->pe_alloc_count) {
		if (!silent)
			log_error("LVs still present on PV with UUID %s: "
				  "Can't remove from VG %s", uuid, vg->name);
		return 0;
	}

	vg->free_count -= pvl->pv->pe_count;
	vg->extent_count -= pvl->pv->pe_count;
	vg->pv_count--;

	dm_list_del(&pvl->list);

	return 1;
}

static int _remove_lv(struct cmd_context *cmd, struct logical_volume *lv,
		      int *list_unsafe, struct dm_list *lvs_changed)
{
	struct lv_segment *snap_seg;
	struct dm_list *snh, *snht;
	struct logical_volume *cow;
	struct lv_list *lvl;
	struct lvinfo info;
	int first = 1;

	log_verbose("%s/%s has missing extents: removing (including "
		    "dependencies)", lv->vg->name, lv->name);

	/* FIXME Cope properly with stacked devices & snapshots. */

	/* If snapshot device is missing, deactivate origin. */
	if (lv_is_cow(lv) && (snap_seg = find_cow(lv))) {
		log_verbose("Deactivating (if active) logical volume %s "
			    "(origin of %s)", snap_seg->origin->name, lv->name);

		if (!test_mode() && !deactivate_lv(cmd, snap_seg->origin)) {
			log_error("Failed to deactivate LV %s",
				  snap_seg->origin->name);
			return 0;
		}

		/* Use the origin LV */
		lv = snap_seg->origin;
	}

	/* Remove snapshot dependencies */
	dm_list_iterate_safe(snh, snht, &lv->snapshot_segs) {
		snap_seg = dm_list_struct_base(snh, struct lv_segment,
					    origin_list);
		cow = snap_seg->cow;

		if (first && !test_mode() &&
		    !deactivate_lv(cmd, snap_seg->origin)) {
			log_error("Failed to deactivate LV %s",
				  snap_seg->origin->name);
			return 0;
		}

		*list_unsafe = 1;	/* May remove caller's lvht! */
		if (!vg_remove_snapshot(cow))
			return_0;
		log_verbose("Removing LV %s from VG %s", cow->name,
			    lv->vg->name);
		if (!lv_remove(cow))
			return_0;

		first = 0;
	}

	/*
	 * If LV is active, replace it with error segment
	 * and add to list of LVs to be removed later.
	 * Doesn't apply to snapshots/origins yet - they're already deactivated.
	 */
	/*
	 * If the LV is a part of mirror segment,
	 * the mirrored LV also should be cleaned up.
	 * Clean-up is currently done by caller (_make_vg_consistent()).
	 */
	if ((lv_info(cmd, lv, &info, 0, 0) && info.exists) ||
	    find_mirror_seg(first_seg(lv))) {
		if (!replace_lv_with_error_segment(lv))
			return_0;

		if (!(lvl = dm_pool_alloc(cmd->mem, sizeof(*lvl)))) {
			log_error("lv_list alloc failed");
			return 0;
		}
		lvl->lv = lv;
		dm_list_add(lvs_changed, &lvl->list);
	} else {
		/* Remove LV immediately. */
		log_verbose("Removing LV %s from VG %s", lv->name, lv->vg->name);
		if (!lv_remove(lv))
			return_0;
	}

	return 1;
}

static int _consolidate_vg(struct cmd_context *cmd, struct volume_group *vg)
{
	struct pv_list *pvl;
	struct lv_list *lvl;
	int r = 1;

	dm_list_iterate_items(lvl, &vg->lvs)
		if (lvl->lv->status & PARTIAL_LV) {
			log_warn("WARNING: Partial LV %s needs to be repaired "
				 "or removed. ", lvl->lv->name);
			r = 0;
		}

	if (!r) {
		cmd->handles_missing_pvs = 1;
		log_warn("WARNING: There are still partial LVs in VG %s.", vg->name);
		log_warn("To remove them unconditionally use: vgreduce --removemissing --force.");
		log_warn("Proceeding to remove empty missing PVs.");
	}

	dm_list_iterate_items(pvl, &vg->pvs) {
		if (pvl->pv->dev && !(pvl->pv->status & MISSING_PV))
			continue;
		if (r && !_remove_pv(vg, pvl, 0))
			return_0;
	}

	return r;
}

static int _make_vg_consistent(struct cmd_context *cmd, struct volume_group *vg)
{
	struct dm_list *pvh, *pvht;
	struct dm_list *lvh, *lvht;
	struct pv_list *pvl;
	struct lv_list *lvl, *lvl2, *lvlt;
	struct logical_volume *lv;
	struct physical_volume *pv;
	struct lv_segment *seg, *mirrored_seg;
	unsigned s;
	uint32_t mimages, remove_log;
	int list_unsafe, only_mirror_images_found;
	DM_LIST_INIT(lvs_changed);
	only_mirror_images_found = 1;

	/* Deactivate & remove necessary LVs */
      restart_loop:
	list_unsafe = 0;	/* Set if we delete a different list-member */

	dm_list_iterate_safe(lvh, lvht, &vg->lvs) {
		lv = dm_list_item(lvh, struct lv_list)->lv;

		/* Are any segments of this LV on missing PVs? */
		dm_list_iterate_items(seg, &lv->segments) {
			for (s = 0; s < seg->area_count; s++) {
				if (seg_type(seg, s) != AREA_PV)
					continue;

				/* FIXME Also check for segs on deleted LVs (incl pvmove) */

				pv = seg_pv(seg, s);
				if (!pv || !pv_dev(pv) ||
				    (pv->status & MISSING_PV)) {
					if (arg_count(cmd, mirrorsonly_ARG) &&
					    !(lv->status & MIRROR_IMAGE)) {
						log_error("Non-mirror-image LV %s found: can't remove.", lv->name);
						only_mirror_images_found = 0;
						continue;
					}
					if (!_remove_lv(cmd, lv, &list_unsafe, &lvs_changed))
						return_0;
					if (list_unsafe)
						goto restart_loop;
				}
			}
		}
	}

	if (!only_mirror_images_found) {
		log_error("Aborting because --mirrorsonly was specified.");
		return 0;
	}

	/*
	 * Remove missing PVs. FIXME: This duplicates _consolidate_vg above,
	 * but we cannot use that right now, since the LV removal code in this
	 * function leaves the VG in a "somewhat inconsistent" state and
	 * _consolidate_vg doesn't like that -- specifically, mirrors are fixed
	 * up *after* the PVs are removed. All this should be gradually
	 * superseded by lvconvert --repair.
	 */
	dm_list_iterate_safe(pvh, pvht, &vg->pvs) {
		pvl = dm_list_item(pvh, struct pv_list);
		if (pvl->pv->dev)
			continue;
		if (!_remove_pv(vg, pvl, 0))
			return_0;
	}

	/* FIXME Recovery.  For now people must clean up by hand. */

	if (!dm_list_empty(&lvs_changed)) {
		if (!vg_write(vg)) {
			log_error("Failed to write out a consistent VG for %s",
				  vg->name);
			return 0;
		}

		if (!test_mode()) {
			/* Suspend lvs_changed */
			if (!suspend_lvs(cmd, &lvs_changed)) {
				stack;
				vg_revert(vg);
				return 0;
			}
		}

		if (!vg_commit(vg)) {
			log_error("Failed to commit consistent VG for %s",
				  vg->name);
			vg_revert(vg);
			return 0;
		}

		if (!test_mode()) {
			if (!resume_lvs(cmd, &lvs_changed)) {
				log_error("Failed to resume LVs using error segments.");
				return 0;
			}
		}

  lvs_changed_altered:
		/* Remove lost mirror images from mirrors */
		dm_list_iterate_items(lvl, &vg->lvs) {
  mirrored_seg_altered:
			mirrored_seg = first_seg(lvl->lv);
			if (!seg_is_mirrored(mirrored_seg))
				continue;

			mimages = mirrored_seg->area_count;
			remove_log = 0;

			for (s = 0; s < mirrored_seg->area_count; s++) {
				dm_list_iterate_items_safe(lvl2, lvlt, &lvs_changed) {
					if (seg_type(mirrored_seg, s) != AREA_LV ||
					    lvl2->lv != seg_lv(mirrored_seg, s))
						continue;
					dm_list_del(&lvl2->list);
					if (!shift_mirror_images(mirrored_seg, s))
						return_0;
					mimages--;	/* FIXME Assumes uniqueness */
				}
			}

			if (mirrored_seg->log_lv) {
				dm_list_iterate_items(seg, &mirrored_seg->log_lv->segments) {
					/* FIXME: The second test shouldn't be required */
					if ((seg->segtype ==
					     get_segtype_from_string(vg->cmd, "error"))) {
						log_print("The log device for %s/%s has failed.",
							  vg->name, mirrored_seg->lv->name);
						remove_log = 1;
						break;
					}
					if (!strcmp(seg->segtype->name, "error")) {
						log_print("Log device for %s/%s has failed.",
							  vg->name, mirrored_seg->lv->name);
						remove_log = 1;
						break;
					}
				}
			}

			if ((mimages != mirrored_seg->area_count) || remove_log){
				if (!reconfigure_mirror_images(mirrored_seg, mimages,
							       NULL, remove_log))
					return_0;

				if (!vg_write(vg)) {
					log_error("Failed to write out updated "
						  "VG for %s", vg->name);
					return 0;
				}
		
				if (!vg_commit(vg)) {
					log_error("Failed to commit updated VG "
						  "for %s", vg->name);
					vg_revert(vg);
					return 0;
				}

				/* mirrored LV no longer has valid mimages.
				 * So add it to lvs_changed for removal.
				 * For this LV may be an area of other mirror,
				 * restart the loop. */
				if (!mimages) {
					if (!_remove_lv(cmd, lvl->lv,
						 &list_unsafe, &lvs_changed))
						return_0;
					goto lvs_changed_altered;
				}

				/* As a result of reconfigure_mirror_images(),
				 * first_seg(lv) may now be different seg.
				 * e.g. a temporary layer might be removed.
				 * So check the mirrored_seg again. */
				goto mirrored_seg_altered;
			}
		}

		/* Deactivate error LVs */
		if (!test_mode()) {
			dm_list_iterate_items_safe(lvl, lvlt, &lvs_changed) {
				log_verbose("Deactivating (if active) logical volume %s",
					    lvl->lv->name);

				if (!deactivate_lv(cmd, lvl->lv)) {
					log_error("Failed to deactivate LV %s",
						  lvl->lv->name);
					/*
					 * We failed to deactivate.
					 * Probably because this was a mirror log.
					 * Don't try to lv_remove it.
					 * Continue work on others.
					 */
					dm_list_del(&lvl->list);
				}
			}
		}

		/* Remove remaining LVs */
		dm_list_iterate_items(lvl, &lvs_changed) {
			log_verbose("Removing LV %s from VG %s", lvl->lv->name,
				    lvl->lv->vg->name);
				/* Skip LVs already removed by mirror code */
				if (find_lv_in_vg(vg, lvl->lv->name) &&
				    !lv_remove(lvl->lv))
					return_0;
		}
	}

	return 1;
}

/* Or take pv_name instead? */
static int _vgreduce_single(struct cmd_context *cmd, struct volume_group *vg,
			    struct physical_volume *pv,
			    void *handle __attribute((unused)))
{
	struct pv_list *pvl;
	struct volume_group *orphan_vg = NULL;
	int r = ECMD_FAILED;
	const char *name = pv_dev_name(pv);

	if (pv_pe_alloc_count(pv)) {
		log_error("Physical volume \"%s\" still in use", name);
		return ECMD_FAILED;
	}

	if (vg->pv_count == 1) {
		log_error("Can't remove final physical volume \"%s\" from "
			  "volume group \"%s\"", name, vg->name);
		return ECMD_FAILED;
	}

	if (!lock_vol(cmd, VG_ORPHANS, LCK_VG_WRITE)) {
		log_error("Can't get lock for orphan PVs");
		return ECMD_FAILED;
	}

	pvl = find_pv_in_vg(vg, name);

	if (!archive(vg))
		goto_bad;

	log_verbose("Removing \"%s\" from volume group \"%s\"", name, vg->name);

	if (pvl)
		dm_list_del(&pvl->list);

	pv->vg_name = vg->fid->fmt->orphan_vg_name;
	pv->status = ALLOCATABLE_PV;

	if (!dev_get_size(pv_dev(pv), &pv->size)) {
		log_error("%s: Couldn't get size.", pv_dev_name(pv));
		goto bad;
	}

	vg->pv_count--;
	vg->free_count -= pv_pe_count(pv) - pv_pe_alloc_count(pv);
	vg->extent_count -= pv_pe_count(pv);

	orphan_vg = vg_read_for_update(cmd, vg->fid->fmt->orphan_vg_name,
				       NULL, 0);

	if (vg_read_error(orphan_vg))
		goto bad;

	if (!vg_split_mdas(cmd, vg, orphan_vg) || !vg->pv_count) {
		log_error("Cannot remove final metadata area on \"%s\" from \"%s\"",
			  name, vg->name);
		goto bad;
	}

	if (!vg_write(vg) || !vg_commit(vg)) {
		log_error("Removal of physical volume \"%s\" from "
			  "\"%s\" failed", name, vg->name);
		goto bad;
	}

	if (!pv_write(cmd, pv, NULL, INT64_C(-1))) {
		log_error("Failed to clear metadata from physical "
			  "volume \"%s\" "
			  "after removal from \"%s\"", name, vg->name);
		goto bad;
	}

	backup(vg);

	log_print("Removed \"%s\" from volume group \"%s\"", name, vg->name);
	r = ECMD_PROCESSED;
bad:
	unlock_and_release_vg(cmd, orphan_vg, VG_ORPHANS);
	return r;
}

int vgreduce(struct cmd_context *cmd, int argc, char **argv)
{
	struct volume_group *vg;
	char *vg_name;
	int ret = ECMD_FAILED;
	int fixed = 1;
	int repairing = arg_count(cmd, removemissing_ARG);
	int saved_ignore_suspended_devices = ignore_suspended_devices();

	if (!argc && !repairing) {
		log_error("Please give volume group name and "
			  "physical volume paths");
		return EINVALID_CMD_LINE;
	}
	
	if (!argc && repairing) {
		log_error("Please give volume group name");
		return EINVALID_CMD_LINE;
	}

	if (arg_count(cmd, mirrorsonly_ARG) && !repairing) {
		log_error("--mirrorsonly requires --removemissing");
		return EINVALID_CMD_LINE;
	}

	if (argc == 1 && !arg_count(cmd, all_ARG) && !repairing) {
		log_error("Please enter physical volume paths or option -a");
		return EINVALID_CMD_LINE;
	}

	if (argc > 1 && arg_count(cmd, all_ARG)) {
		log_error("Option -a and physical volume paths mutually "
			  "exclusive");
		return EINVALID_CMD_LINE;
	}

	if (argc > 1 && repairing) {
		log_error("Please only specify the volume group");
		return EINVALID_CMD_LINE;
	}

	vg_name = skip_dev_dir(cmd, argv[0], NULL);
	argv++;
	argc--;

	log_verbose("Finding volume group \"%s\"", vg_name);

	if (repairing) {
		init_ignore_suspended_devices(1);
		cmd->handles_missing_pvs = 1;
	}

	vg = vg_read_for_update(cmd, vg_name, NULL, READ_ALLOW_EXPORTED);
	if (vg_read_error(vg) == FAILED_ALLOCATION ||
	    vg_read_error(vg) == FAILED_NOTFOUND)
		goto_out;

	/* FIXME We want to allow read-only VGs to be changed here? */
	if (vg_read_error(vg) && vg_read_error(vg) != FAILED_READ_ONLY
	    && !arg_count(cmd, removemissing_ARG))
		goto_out;

	if (repairing) {
		if (!vg_read_error(vg) && !vg_missing_pv_count(vg)) {
			log_error("Volume group \"%s\" is already consistent",
				  vg_name);
			ret = ECMD_PROCESSED;
			goto out;
		}

		vg_release(vg);
		log_verbose("Trying to open VG %s for recovery...", vg_name);

		vg = vg_read_for_update(cmd, vg_name, NULL,
					READ_ALLOW_INCONSISTENT
					| READ_ALLOW_EXPORTED);

		if (vg_read_error(vg) && vg_read_error(vg) != FAILED_READ_ONLY
		    && vg_read_error(vg) != FAILED_INCONSISTENT)
			goto_out;

		if (!archive(vg))
			goto_out;

		if (arg_count(cmd, force_ARG)) {
			if (!_make_vg_consistent(cmd, vg))
				goto_out;
		} else
			fixed = _consolidate_vg(cmd, vg);

		if (!vg_write(vg) || !vg_commit(vg)) {
			log_error("Failed to write out a consistent VG for %s",
				  vg_name);
			goto out;
		}
		backup(vg);

		if (fixed) {
			log_print("Wrote out consistent volume group %s",
				  vg_name);
			ret = ECMD_PROCESSED;
		} else
			ret = ECMD_FAILED;

	} else {
		if (!vg_check_status(vg, EXPORTED_VG | LVM_WRITE | RESIZEABLE_VG))
			goto_out;

		/* FIXME: Pass private struct through to all these functions */
		/* and update in batch here? */
		ret = process_each_pv(cmd, argc, argv, vg, READ_FOR_UPDATE, 0, NULL,
				      _vgreduce_single);

	}
out:
	init_ignore_suspended_devices(saved_ignore_suspended_devices);
	unlock_and_release_vg(cmd, vg, vg_name);

	return ret;

/******* FIXME
	log_error ("no empty physical volumes found in volume group \"%s\"", vg_name);

	log_verbose
	    ("volume group \"%s\" will be reduced by %d physical volume%s",
	     vg_name, np, np > 1 ? "s" : "");
	log_verbose ("reducing volume group \"%s\" by physical volume \"%s\"",
		     vg_name, pv_names[p]);

	log_print
	    ("volume group \"%s\" %ssuccessfully reduced by physical volume%s:",
	     vg_name, error > 0 ? "NOT " : "", p > 1 ? "s" : "");
		log_print("%s", pv_this[p]->pv_name);
********/

}
