/*	$NetBSD: vgsplit.c,v 1.1.1.2 2008/12/12 11:43:05 haad Exp $	*/

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

#include "tools.h"

static int _move_pv(struct volume_group *vg_from, struct volume_group *vg_to,
		    const char *pv_name)
{
	struct physical_volume *pv;
	struct pv_list *pvl;

	/* FIXME: handle tags */
	if (!(pvl = find_pv_in_vg(vg_from, pv_name))) {
		log_error("Physical volume %s not in volume group %s",
			  pv_name, vg_from->name);
		return 0;
	}

	dm_list_move(&vg_to->pvs, &pvl->list);

	vg_from->pv_count--;
	vg_to->pv_count++;

	pv = pvl->pv;

	vg_from->extent_count -= pv_pe_count(pv);
	vg_to->extent_count += pv_pe_count(pv);

	vg_from->free_count -= pv_pe_count(pv) - pv_pe_alloc_count(pv);
	vg_to->free_count += pv_pe_count(pv) - pv_pe_alloc_count(pv);

	return 1;
}

static int _move_pvs_used_by_lv(struct volume_group *vg_from,
				   struct volume_group *vg_to,
				   const char *lv_name)
{
	struct lv_segment *lvseg;
	unsigned s;
	struct lv_list *lvl;
	struct logical_volume *lv;

	/* FIXME: handle tags */
	if (!(lvl = find_lv_in_vg(vg_from, lv_name))) {
		log_error("Logical volume %s not in volume group %s",
			  lv_name, vg_from->name);
		return 0;
	}

	dm_list_iterate_items(lvseg, &lvl->lv->segments) {
		if (lvseg->log_lv)
			if (!_move_pvs_used_by_lv(vg_from, vg_to,
						     lvseg->log_lv->name))
				return_0;
		for (s = 0; s < lvseg->area_count; s++) {
			if (seg_type(lvseg, s) == AREA_PV) {
				if (!_move_pv(vg_from, vg_to,
					      pv_dev_name(seg_pv(lvseg, s))))
					return_0;
			} else if (seg_type(lvseg, s) == AREA_LV) {
				lv = seg_lv(lvseg, s);
				if (!_move_pvs_used_by_lv(vg_from, vg_to,
							     lv->name))
				    return_0;
			}
		}
	}
	return 1;
}

/* FIXME Why not (lv->vg == vg) ? */
static int _lv_is_in_vg(struct volume_group *vg, struct logical_volume *lv)
{
	struct lv_list *lvl;

	dm_list_iterate_items(lvl, &vg->lvs)
		if (lv == lvl->lv)
			 return 1;

	return 0;
}

static int _move_one_lv(struct volume_group *vg_from,
			 struct volume_group *vg_to,
			 struct dm_list *lvh)
{
	struct logical_volume *lv = dm_list_item(lvh, struct lv_list)->lv;

	dm_list_move(&vg_to->lvs, lvh);
	
	if (lv_is_active(lv)) {
		log_error("Logical volume \"%s\" must be inactive", lv->name);
		return 0;
	}

	if (lv->status & SNAPSHOT) {
		vg_from->snapshot_count--;
		vg_to->snapshot_count++;
	} else if (!lv_is_cow(lv)) {
		vg_from->lv_count--;
		vg_to->lv_count++;
	}
	return 1;
}	

static int _move_lvs(struct volume_group *vg_from, struct volume_group *vg_to)
{
	struct dm_list *lvh, *lvht;
	struct logical_volume *lv;
	struct lv_segment *seg;
	struct physical_volume *pv;
	struct volume_group *vg_with;
	unsigned s;

	dm_list_iterate_safe(lvh, lvht, &vg_from->lvs) {
		lv = dm_list_item(lvh, struct lv_list)->lv;

		if ((lv->status & SNAPSHOT))
			continue;

		if ((lv->status & MIRRORED))
			continue;

		/* Ensure all the PVs used by this LV remain in the same */
		/* VG as each other */
		vg_with = NULL;
		dm_list_iterate_items(seg, &lv->segments) {
			for (s = 0; s < seg->area_count; s++) {
				/* FIXME Check AREA_LV too */
				if (seg_type(seg, s) != AREA_PV)
					continue;

				pv = seg_pv(seg, s);
				if (vg_with) {
					if (!pv_is_in_vg(vg_with, pv)) {
						log_error("Can't split Logical "
							  "Volume %s between "
							  "two Volume Groups",
							  lv->name);
						return 0;
					}
					continue;
				}

				if (pv_is_in_vg(vg_from, pv)) {
					vg_with = vg_from;
					continue;
				}
				if (pv_is_in_vg(vg_to, pv)) {
					vg_with = vg_to;
					continue;
				}
				log_error("Physical Volume %s not found",
					  pv_dev_name(pv));
				return 0;
			}

		}
			
		if (vg_with == vg_from)
			continue;

		/* Move this LV */
		if (!_move_one_lv(vg_from, vg_to, lvh))
			return_0;
	}

	/* FIXME Ensure no LVs contain segs pointing at LVs in the other VG */

	return 1;
}

/*
 * Move the hidden / internal "snapshotN" LVs.from 'vg_from' to 'vg_to'.
 */
static int _move_snapshots(struct volume_group *vg_from,
			   struct volume_group *vg_to)
{
	struct dm_list *lvh, *lvht;
	struct logical_volume *lv;
	struct lv_segment *seg;
	int cow_from = 0;
	int origin_from = 0;

	dm_list_iterate_safe(lvh, lvht, &vg_from->lvs) {
		lv = dm_list_item(lvh, struct lv_list)->lv;

		if (!(lv->status & SNAPSHOT))
			continue;

		dm_list_iterate_items(seg, &lv->segments) {
			cow_from = _lv_is_in_vg(vg_from, seg->cow);
			origin_from = _lv_is_in_vg(vg_from, seg->origin);

			if (cow_from && origin_from)
				continue;
			if ((!cow_from && origin_from) ||
			     (cow_from && !origin_from)) {
				log_error("Can't split snapshot %s between"
					  " two Volume Groups", seg->cow->name);
				return 0;
			}

			/*
			 * At this point, the cow and origin should already be
			 * in vg_to.
			 */
			if (_lv_is_in_vg(vg_to, seg->cow) &&
			    _lv_is_in_vg(vg_to, seg->origin)) {
				if (!_move_one_lv(vg_from, vg_to, lvh))
					return_0;
			}
		}

	}

	return 1;
}

static int _move_mirrors(struct volume_group *vg_from,
			 struct volume_group *vg_to)
{
	struct dm_list *lvh, *lvht;
	struct logical_volume *lv;
	struct lv_segment *seg;
	unsigned s, seg_in, log_in;

	dm_list_iterate_safe(lvh, lvht, &vg_from->lvs) {
		lv = dm_list_item(lvh, struct lv_list)->lv;

		if (!(lv->status & MIRRORED))
			continue;

		seg = first_seg(lv);

		seg_in = 0;
		for (s = 0; s < seg->area_count; s++)
			if (_lv_is_in_vg(vg_to, seg_lv(seg, s)))
			    seg_in++;

		log_in = (!seg->log_lv || _lv_is_in_vg(vg_to, seg->log_lv));
		
		if ((seg_in && seg_in < seg->area_count) ||
		    (seg_in && seg->log_lv && !log_in) ||
		    (!seg_in && seg->log_lv && log_in)) {
			log_error("Can't split mirror %s between "
				  "two Volume Groups", lv->name);
			return 0;
		}

		if (seg_in == seg->area_count && log_in) {
			if (!_move_one_lv(vg_from, vg_to, lvh))
				return_0;
		}
	}

	return 1;
}

/*
 * Has the user given an option related to a new vg as the split destination?
 */
static int new_vg_option_specified(struct cmd_context *cmd)
{
	return(arg_count(cmd, clustered_ARG) ||
	       arg_count(cmd, alloc_ARG) ||
	       arg_count(cmd, maxphysicalvolumes_ARG) ||
	       arg_count(cmd, maxlogicalvolumes_ARG));
}

int vgsplit(struct cmd_context *cmd, int argc, char **argv)
{
	struct vgcreate_params vp_new;
	struct vgcreate_params vp_def;
	char *vg_name_from, *vg_name_to;
	struct volume_group *vg_to, *vg_from;
	int opt;
	int existing_vg;
	int consistent;
	const char *lv_name;

	if ((arg_count(cmd, name_ARG) + argc) < 3) {
		log_error("Existing VG, new VG and either physical volumes "
			  "or logical volume required.");
		return EINVALID_CMD_LINE;
	}

	if (arg_count(cmd, name_ARG) && (argc > 2)) {
		log_error("A logical volume name cannot be given with "
			  "physical volumes.");
		return ECMD_FAILED;
	}

	if (arg_count(cmd, name_ARG))
		lv_name = arg_value(cmd, name_ARG);
	else
		lv_name = NULL;

	vg_name_from = skip_dev_dir(cmd, argv[0], NULL);
	vg_name_to = skip_dev_dir(cmd, argv[1], NULL);
	argc -= 2;
	argv += 2;

	if (!strcmp(vg_name_to, vg_name_from)) {
		log_error("Duplicate volume group name \"%s\"", vg_name_from);
		return ECMD_FAILED;
	}

	log_verbose("Checking for volume group \"%s\"", vg_name_from);
	if (!(vg_from = vg_lock_and_read(cmd, vg_name_from, NULL, LCK_VG_WRITE,
				       CLUSTERED | EXPORTED_VG |
				       RESIZEABLE_VG | LVM_WRITE,
				       CORRECT_INCONSISTENT | FAIL_INCONSISTENT)))
		 return ECMD_FAILED;

	log_verbose("Checking for new volume group \"%s\"", vg_name_to);
	if (!lock_vol(cmd, vg_name_to, LCK_VG_WRITE | LCK_NONBLOCK)) {
		log_error("Can't get lock for %s", vg_name_to);
		unlock_vg(cmd, vg_name_from);
		return ECMD_FAILED;
	}

	consistent = 0;
	if ((vg_to = vg_read(cmd, vg_name_to, NULL, &consistent))) {
		existing_vg = 1;
		if (new_vg_option_specified(cmd)) {
			log_error("Volume group \"%s\" exists, but new VG "
				    "option specified", vg_name_to);
			goto_bad;
		}
		if (!vgs_are_compatible(cmd, vg_from,vg_to))
			goto_bad;
	} else {
		existing_vg = 0;

		/* Set metadata format of original VG */
		/* FIXME: need some common logic */
		cmd->fmt = vg_from->fid->fmt;

		vp_def.vg_name = NULL;
		vp_def.extent_size = vg_from->extent_size;
		vp_def.max_pv = vg_from->max_pv;
		vp_def.max_lv = vg_from->max_lv;
		vp_def.alloc = vg_from->alloc;
		vp_def.clustered = 0;

		if (fill_vg_create_params(cmd, vg_name_to, &vp_new, &vp_def)) {
			unlock_vg(cmd, vg_name_from);
			unlock_vg(cmd, vg_name_to);
			return EINVALID_CMD_LINE;
		}

		if (validate_vg_create_params(cmd, &vp_new)) {
			unlock_vg(cmd, vg_name_from);
			unlock_vg(cmd, vg_name_to);
			return EINVALID_CMD_LINE;
		}

		if (!(vg_to = vg_create(cmd, vg_name_to, vp_new.extent_size,
					vp_new.max_pv, vp_new.max_lv,
					vp_new.alloc, 0, NULL)))
			goto_bad;

		if (vg_is_clustered(vg_from))
			vg_to->status |= CLUSTERED;
	}

	/* Archive vg_from before changing it */
	if (!archive(vg_from))
		goto_bad;

	/* Move PVs across to new structure */
	for (opt = 0; opt < argc; opt++) {
		if (!_move_pv(vg_from, vg_to, argv[opt]))
			goto_bad;
	}

	/* If an LV given on the cmdline, move used_by PVs */
	if (lv_name && !_move_pvs_used_by_lv(vg_from, vg_to, lv_name))
		goto_bad;

	/* Move required LVs across, checking consistency */
	if (!(_move_lvs(vg_from, vg_to)))
		goto_bad;

	/* Move required snapshots across */
	if (!(_move_snapshots(vg_from, vg_to)))
		goto_bad;

	/* Move required mirrors across */
	if (!(_move_mirrors(vg_from, vg_to)))
		goto_bad;

	/* Split metadata areas and check if both vgs have at least one area */
	if (!(vg_split_mdas(cmd, vg_from, vg_to)) && vg_from->pv_count) {
		log_error("Cannot split: Nowhere to store metadata for new Volume Group");
		goto_bad;
	}

	/* Set proper name for all PVs in new VG */
	if (!vg_rename(cmd, vg_to, vg_name_to))
		goto_bad;

	/* store it on disks */
	log_verbose("Writing out updated volume groups");

	/*
	 * First, write out the new VG as EXPORTED.  We do this first in case
	 * there is a crash - we will still have the new VG information, in an
	 * exported state.  Recovery after this point would be removal of the
	 * new VG and redoing the vgsplit.
	 * FIXME: recover automatically or instruct the user?
	 */
	vg_to->status |= EXPORTED_VG;

	if (!archive(vg_to))
		goto_bad;

	if (!vg_write(vg_to) || !vg_commit(vg_to))
		goto_bad;

	backup(vg_to);

	/*
	 * Next, write out the updated old VG.  If we crash after this point,
	 * recovery is a vgimport on the new VG.
	 * FIXME: recover automatically or instruct the user?
	 */
	if (vg_from->pv_count) {
		if (!vg_write(vg_from) || !vg_commit(vg_from))
			goto_bad;

		backup(vg_from);
	}

	/*
	 * Finally, remove the EXPORTED flag from the new VG and write it out.
	 */
	consistent = 1;
	if (!test_mode() &&
	    (!(vg_to = vg_read(cmd, vg_name_to, NULL, &consistent)) ||
	     !consistent)) {
		log_error("Volume group \"%s\" became inconsistent: please "
			  "fix manually", vg_name_to);
		goto_bad;
	}

	vg_to->status &= ~EXPORTED_VG;

	if (!vg_write(vg_to) || !vg_commit(vg_to))
		goto_bad;

	backup(vg_to);

	unlock_vg(cmd, vg_name_from);
	unlock_vg(cmd, vg_name_to);

	log_print("%s volume group \"%s\" successfully split from \"%s\"",
		  existing_vg ? "Existing" : "New",
		  vg_to->name, vg_from->name);
	return ECMD_PROCESSED;

      bad:
	unlock_vg(cmd, vg_name_from);
	unlock_vg(cmd, vg_name_to);
	return ECMD_FAILED;
}
