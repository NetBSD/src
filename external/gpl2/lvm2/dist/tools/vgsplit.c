/*	$NetBSD: vgsplit.c,v 1.1.1.2 2009/12/02 00:25:46 haad Exp $	*/

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
 * Create or open the destination of the vgsplit operation.
 * Returns
 * - non-NULL: VG handle w/VG lock held
 * - NULL: no VG lock held
 */
static struct volume_group *_vgsplit_to(struct cmd_context *cmd,
					const char *vg_name_to,
					int *existing_vg)
{
	struct volume_group *vg_to = NULL;

	log_verbose("Checking for new volume group \"%s\"", vg_name_to);
	/*
	 * First try to create a new VG.  If we cannot create it,
	 * and we get FAILED_EXIST (we will not be holding a lock),
	 * a VG must already exist with this name.  We then try to
	 * read the existing VG - the vgsplit will be into an existing VG.
	 *
	 * Otherwise, if the lock was successful, it must be the case that
	 * we obtained a WRITE lock and could not find the vgname in the
	 * system.  Thus, the split will be into a new VG.
	 */
	vg_to = vg_create(cmd, vg_name_to);
	if (vg_read_error(vg_to) == FAILED_LOCKING) {
		log_error("Can't get lock for %s", vg_name_to);
		vg_release(vg_to);
		return NULL;
	}
	if (vg_read_error(vg_to) == FAILED_EXIST) {
		*existing_vg = 1;
		vg_release(vg_to);
		vg_to = vg_read_for_update(cmd, vg_name_to, NULL, 0);

		if (vg_read_error(vg_to)) {
			vg_release(vg_to);
			stack;
			return NULL;
		}

	} else if (vg_read_error(vg_to) == SUCCESS) {
		*existing_vg = 0;
	}
	return vg_to;
}

/*
 * Open the source of the vgsplit operation.
 * Returns
 * - non-NULL: VG handle w/VG lock held
 * - NULL: no VG lock held
 */
static struct volume_group *_vgsplit_from(struct cmd_context *cmd,
					  const char *vg_name_from)
{
	struct volume_group *vg_from;

	log_verbose("Checking for volume group \"%s\"", vg_name_from);

	vg_from = vg_read_for_update(cmd, vg_name_from, NULL, 0);
	if (vg_read_error(vg_from)) {
		vg_release(vg_from);
		return NULL;
	}
	return vg_from;
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
	struct volume_group *vg_to = NULL, *vg_from = NULL;
	int opt;
	int existing_vg = 0;
	int r = ECMD_FAILED;
	const char *lv_name;
	int lock_vg_from_first = 1;

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

	if (strcmp(vg_name_to, vg_name_from) < 0)
		lock_vg_from_first = 0;

	if (lock_vg_from_first) {
		vg_from = _vgsplit_from(cmd, vg_name_from);
		if (!vg_from) {
			stack;
			return ECMD_FAILED;
		}
		/*
		 * Set metadata format of original VG.
		 * NOTE: We must set the format before calling vg_create()
		 * since vg_create() calls the per-format constructor.
		 */
		cmd->fmt = vg_from->fid->fmt;

		vg_to = _vgsplit_to(cmd, vg_name_to, &existing_vg);
		if (!vg_to) {
			unlock_and_release_vg(cmd, vg_from, vg_name_from);
			stack;
			return ECMD_FAILED;
		}
	} else {
		vg_to = _vgsplit_to(cmd, vg_name_to, &existing_vg);
		if (!vg_to) {
			stack;
			return ECMD_FAILED;
		}
		vg_from = _vgsplit_from(cmd, vg_name_from);
		if (!vg_from) {
			unlock_and_release_vg(cmd, vg_to, vg_name_to);
			stack;
			return ECMD_FAILED;
		}

		if (cmd->fmt != vg_from->fid->fmt) {
			/* In this case we don't know the vg_from->fid->fmt */
			log_error("Unable to set new VG metadata type based on "
				  "source VG format - use -M option.");
			goto bad;
		}
	}

	if (existing_vg) {
		if (new_vg_option_specified(cmd)) {
			log_error("Volume group \"%s\" exists, but new VG "
				    "option specified", vg_name_to);
			goto bad;
		}
		if (!vgs_are_compatible(cmd, vg_from,vg_to))
			goto_bad;
	} else {
		vgcreate_params_set_defaults(&vp_def, vg_from);
		vp_def.vg_name = vg_name_to;
		if (vgcreate_params_set_from_args(cmd, &vp_new, &vp_def)) {
			r = EINVALID_CMD_LINE;
			goto_bad;
		}

		if (vgcreate_params_validate(cmd, &vp_new)) {
			r = EINVALID_CMD_LINE;
			goto_bad;
		}

		if (!vg_set_extent_size(vg_to, vp_new.extent_size) ||
		    !vg_set_max_lv(vg_to, vp_new.max_lv) ||
		    !vg_set_max_pv(vg_to, vp_new.max_pv) ||
		    !vg_set_alloc_policy(vg_to, vp_new.alloc) ||
		    !vg_set_clustered(vg_to, vp_new.clustered))
			goto_bad;
	}

	/* Archive vg_from before changing it */
	if (!archive(vg_from))
		goto_bad;

	/* Move PVs across to new structure */
	for (opt = 0; opt < argc; opt++) {
		if (!move_pv(vg_from, vg_to, argv[opt]))
			goto_bad;
	}

	/* If an LV given on the cmdline, move used_by PVs */
	if (lv_name && !move_pvs_used_by_lv(vg_from, vg_to, lv_name))
		goto_bad;

	/* Move required LVs across, checking consistency */
	if (!(_move_lvs(vg_from, vg_to)))
		goto_bad;

	/* FIXME Separate the 'move' from the 'validation' to fix dev stacks */
	/* Move required mirrors across */
	if (!(_move_mirrors(vg_from, vg_to)))
		goto_bad;

	/* Move required snapshots across */
	if (!(_move_snapshots(vg_from, vg_to)))
		goto_bad;

	/* Split metadata areas and check if both vgs have at least one area */
	if (!(vg_split_mdas(cmd, vg_from, vg_to)) && vg_from->pv_count) {
		log_error("Cannot split: Nowhere to store metadata for new Volume Group");
		goto bad;
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
	if (!test_mode()) {
		vg_release(vg_to);
		vg_to = vg_read_for_update(cmd, vg_name_to, NULL,
					   READ_ALLOW_EXPORTED);
		if (vg_read_error(vg_to)) {
			log_error("Volume group \"%s\" became inconsistent: "
				  "please fix manually", vg_name_to);
			goto bad;
		}
	}

	vg_to->status &= ~EXPORTED_VG;

	if (!vg_write(vg_to) || !vg_commit(vg_to))
		goto_bad;

	backup(vg_to);

	log_print("%s volume group \"%s\" successfully split from \"%s\"",
		  existing_vg ? "Existing" : "New",
		  vg_to->name, vg_from->name);

	r = ECMD_PROCESSED;

bad:
	if (lock_vg_from_first) {
		unlock_and_release_vg(cmd, vg_to, vg_name_to);
		unlock_and_release_vg(cmd, vg_from, vg_name_from);
	} else {
		unlock_and_release_vg(cmd, vg_from, vg_name_from);
		unlock_and_release_vg(cmd, vg_to, vg_name_to);
	}
	return r;
}
