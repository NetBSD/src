/*	$NetBSD: pvmove.c,v 1.1.1.2 2009/12/02 00:25:54 haad Exp $	*/

/*
 * Copyright (C) 2003-2004 Sistina Software, Inc. All rights reserved.
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
#include "polldaemon.h"
#include "display.h"

#define PVMOVE_FIRST_TIME   0x00000001      /* Called for first time */

static int _pvmove_target_present(struct cmd_context *cmd, int clustered)
{
	const struct segment_type *segtype;
	unsigned attr = 0;
	int found = 1;
	static int _clustered_found = -1;

	if (clustered && _clustered_found >= 0)
		return _clustered_found;

	if (!(segtype = get_segtype_from_string(cmd, "mirror")))
		return_0;

	if (activation() && segtype->ops->target_present &&
	    !segtype->ops->target_present(cmd, NULL, clustered ? &attr : NULL))
		found = 0;

	if (activation() && clustered) {
		if (found && (attr & MIRROR_LOG_CLUSTERED))
			_clustered_found = found = 1;
		else
			_clustered_found = found = 0;
	}

	return found;
}

static unsigned _pvmove_is_exclusive(struct cmd_context *cmd,
				     struct volume_group *vg)
{
	if (vg_is_clustered(vg))
		if (!_pvmove_target_present(cmd, 1))
			return 1;

	return 0;
}

/* Allow /dev/vgname/lvname, vgname/lvname or lvname */
static const char *_extract_lvname(struct cmd_context *cmd, const char *vgname,
				   const char *arg)
{
	const char *lvname;

	/* Is an lvname supplied directly? */
	if (!strchr(arg, '/'))
		return arg;

	lvname = skip_dev_dir(cmd, arg, NULL);
	while (*lvname == '/')
		lvname++;
	if (!strchr(lvname, '/')) {
		log_error("--name takes a logical volume name");
		return NULL;
	}
	if (strncmp(vgname, lvname, strlen(vgname)) ||
	    (lvname += strlen(vgname), *lvname != '/')) {
		log_error("Named LV and old PV must be in the same VG");
		return NULL;
	}
	while (*lvname == '/')
		lvname++;
	if (!*lvname) {
		log_error("Incomplete LV name supplied with --name");
		return NULL;
	}
	return lvname;
}

static struct volume_group *_get_vg(struct cmd_context *cmd, const char *vgname)
{
	dev_close_all();

	return vg_read_for_update(cmd, vgname, NULL, 0);
}

/* Create list of PVs for allocation of replacement extents */
static struct dm_list *_get_allocatable_pvs(struct cmd_context *cmd, int argc,
					 char **argv, struct volume_group *vg,
					 struct physical_volume *pv,
					 alloc_policy_t alloc)
{
	struct dm_list *allocatable_pvs, *pvht, *pvh;
	struct pv_list *pvl;

	if (argc)
		allocatable_pvs = create_pv_list(cmd->mem, vg, argc, argv, 1);
	else
		allocatable_pvs = clone_pv_list(cmd->mem, &vg->pvs);

	if (!allocatable_pvs)
		return_NULL;

	dm_list_iterate_safe(pvh, pvht, allocatable_pvs) {
		pvl = dm_list_item(pvh, struct pv_list);

		/* Don't allocate onto the PV we're clearing! */
		if ((alloc != ALLOC_ANYWHERE) && (pvl->pv->dev == pv_dev(pv))) {
			dm_list_del(&pvl->list);
			continue;
		}

		/* Remove PV if full */
		if ((pvl->pv->pe_count == pvl->pv->pe_alloc_count))
			dm_list_del(&pvl->list);
	}

	if (dm_list_empty(allocatable_pvs)) {
		log_error("No extents available for allocation");
		return NULL;
	}

	return allocatable_pvs;
}

/*
 * Replace any LV segments on given PV with temporary mirror.
 * Returns list of LVs changed.
 */
static int _insert_pvmove_mirrors(struct cmd_context *cmd,
				  struct logical_volume *lv_mirr,
				  struct dm_list *source_pvl,
				  struct logical_volume *lv,
				  struct dm_list *lvs_changed)

{
	struct pv_list *pvl;
	uint32_t prev_le_count;

	/* Only 1 PV may feature in source_pvl */
	pvl = dm_list_item(source_pvl->n, struct pv_list);

	prev_le_count = lv_mirr->le_count;
	if (!insert_layer_for_segments_on_pv(cmd, lv, lv_mirr, PVMOVE,
					     pvl, lvs_changed))
		return_0;

	/* check if layer was inserted */
	if (lv_mirr->le_count - prev_le_count) {
		lv->status |= LOCKED;

		log_verbose("Moving %u extents of logical volume %s/%s",
			    lv_mirr->le_count - prev_le_count,
			    lv->vg->name, lv->name);
	}

	return 1;
}

/* Create new LV with mirror segments for the required copies */
static struct logical_volume *_set_up_pvmove_lv(struct cmd_context *cmd,
						struct volume_group *vg,
						struct dm_list *source_pvl,
						const char *lv_name,
						struct dm_list *allocatable_pvs,
						alloc_policy_t alloc,
						struct dm_list **lvs_changed)
{
	struct logical_volume *lv_mirr, *lv;
	struct lv_list *lvl;
	uint32_t log_count = 0;
	int lv_found = 0;

	/* FIXME Cope with non-contiguous => splitting existing segments */
	if (!(lv_mirr = lv_create_empty("pvmove%d", NULL,
					LVM_READ | LVM_WRITE,
					ALLOC_CONTIGUOUS, vg))) {
		log_error("Creation of temporary pvmove LV failed");
		return NULL;
	}

	lv_mirr->status |= (PVMOVE | LOCKED);

	if (!(*lvs_changed = dm_pool_alloc(cmd->mem, sizeof(**lvs_changed)))) {
		log_error("lvs_changed list struct allocation failed");
		return NULL;
	}

	dm_list_init(*lvs_changed);

	/* Find segments to be moved and set up mirrors */
	dm_list_iterate_items(lvl, &vg->lvs) {
		lv = lvl->lv;
		if ((lv == lv_mirr))
			continue;
		if (lv_name) {
			if (strcmp(lv->name, lv_name))
				continue;
			lv_found = 1;
		}
		if (lv_is_origin(lv) || lv_is_cow(lv)) {
			log_print("Skipping snapshot-related LV %s", lv->name);
			continue;
		}
		if (lv->status & MIRRORED) {
			log_print("Skipping mirror LV %s", lv->name);
			continue;
		}
		if (lv->status & MIRROR_LOG) {
			log_print("Skipping mirror log LV %s", lv->name);
			continue;
		}
		if (lv->status & MIRROR_IMAGE) {
			log_print("Skipping mirror image LV %s", lv->name);
			continue;
		}
		if (lv->status & LOCKED) {
			log_print("Skipping locked LV %s", lv->name);
			continue;
		}
		if (!_insert_pvmove_mirrors(cmd, lv_mirr, source_pvl, lv,
					    *lvs_changed))
			return_NULL;
	}

	if (lv_name && !lv_found) {
		log_error("Logical volume %s not found.", lv_name);
		return NULL;
	}

	/* Is temporary mirror empty? */
	if (!lv_mirr->le_count) {
		log_error("No data to move for %s", vg->name);
		return NULL;
	}

	if (!lv_add_mirrors(cmd, lv_mirr, 1, 1, 0, log_count,
			    allocatable_pvs, alloc, MIRROR_BY_SEG)) {
		log_error("Failed to convert pvmove LV to mirrored");
		return_NULL;
	}

	if (!split_parent_segments_for_layer(cmd, lv_mirr)) {
		log_error("Failed to split segments being moved");
		return_NULL;
	}

	return lv_mirr;
}

static int _activate_lv(struct cmd_context *cmd, struct logical_volume *lv_mirr,
			unsigned exclusive)
{
	if (exclusive)
		return activate_lv_excl(cmd, lv_mirr);

	return activate_lv(cmd, lv_mirr);
}

static int _finish_pvmove(struct cmd_context *cmd, struct volume_group *vg,
			  struct logical_volume *lv_mirr,
			  struct dm_list *lvs_changed);

static int _update_metadata(struct cmd_context *cmd, struct volume_group *vg,
			    struct logical_volume *lv_mirr,
			    struct dm_list *lvs_changed, unsigned flags)
{
	unsigned exclusive = _pvmove_is_exclusive(cmd, vg);
	unsigned first_time = (flags & PVMOVE_FIRST_TIME) ? 1 : 0;
	int r = 0;

	log_verbose("Updating volume group metadata");
	if (!vg_write(vg)) {
		log_error("ABORTING: Volume group metadata update failed.");
		return 0;
	}

	/* Suspend lvs_changed */
	if (!suspend_lvs(cmd, lvs_changed))
		goto_out;

	/* Suspend mirrors on subsequent calls */
	if (!first_time) {
		if (!suspend_lv(cmd, lv_mirr)) {
			resume_lvs(cmd, lvs_changed);
			vg_revert(vg);
			goto_out;
		}
	}

	/* Commit on-disk metadata */
	if (!vg_commit(vg)) {
		log_error("ABORTING: Volume group metadata update failed.");
		if (!first_time)
			resume_lv(cmd, lv_mirr);
		resume_lvs(cmd, lvs_changed);
		goto out;
	}

	/* Activate the temporary mirror LV */
	/* Only the first mirror segment gets activated as a mirror */
	/* FIXME: Add option to use a log */
	if (first_time) {
		if (!_activate_lv(cmd, lv_mirr, exclusive)) {
			if (test_mode())
				goto out;

			/*
			 * Nothing changed yet, try to revert pvmove.
			 */
			log_error("Temporary pvmove mirror activation failed.");
			if (!_finish_pvmove(cmd, vg, lv_mirr, lvs_changed))
				log_error("ABORTING: Restoring original configuration "
					  "before pvmove failed. Run pvmove --abort.");
			goto out;
		}
	} else if (!resume_lv(cmd, lv_mirr)) {
		log_error("Unable to reactivate logical volume \"%s\"",
			  lv_mirr->name);
		resume_lvs(cmd, lvs_changed);
		goto out;
	}

	/* Unsuspend LVs */
	if (!resume_lvs(cmd, lvs_changed)) {
		log_error("Unable to resume logical volumes");
		goto out;
	}

	r = 1;
out:
	backup(vg);
	return r;
}

static int _set_up_pvmove(struct cmd_context *cmd, const char *pv_name,
			  int argc, char **argv)
{
	const char *lv_name = NULL;
	char *pv_name_arg;
	struct volume_group *vg;
	struct dm_list *source_pvl;
	struct dm_list *allocatable_pvs;
	alloc_policy_t alloc;
	struct dm_list *lvs_changed;
	struct physical_volume *pv;
	struct logical_volume *lv_mirr;
	unsigned first_time = 1;
	unsigned exclusive;
	int r = ECMD_FAILED;

	pv_name_arg = argv[0];
	argc--;
	argv++;

	/* Find PV (in VG) */
	if (!(pv = find_pv_by_name(cmd, pv_name))) {
		stack;
		return EINVALID_CMD_LINE;
	}

	if (arg_count(cmd, name_ARG)) {
		if (!(lv_name = _extract_lvname(cmd, pv_vg_name(pv),
						arg_value(cmd, name_ARG)))) {
			stack;
			return EINVALID_CMD_LINE;
		}

		if (!validate_name(lv_name)) {
			log_error("Logical volume name %s is invalid", lv_name);
			return EINVALID_CMD_LINE;
		}
	}

	/* Read VG */
	log_verbose("Finding volume group \"%s\"", pv_vg_name(pv));

	vg = _get_vg(cmd, pv_vg_name(pv));
	if (vg_read_error(vg)) {
		vg_release(vg);
		stack;
		return ECMD_FAILED;
	}

	exclusive = _pvmove_is_exclusive(cmd, vg);

	if ((lv_mirr = find_pvmove_lv(vg, pv_dev(pv), PVMOVE))) {
		log_print("Detected pvmove in progress for %s", pv_name);
		if (argc || lv_name)
			log_error("Ignoring remaining command line arguments");

		if (!(lvs_changed = lvs_using_lv(cmd, vg, lv_mirr))) {
			log_error("ABORTING: Failed to generate list of moving LVs");
			goto out;
		}

		/* Ensure mirror LV is active */
		if (!_activate_lv(cmd, lv_mirr, exclusive)) {
			log_error("ABORTING: Temporary mirror activation failed.");
			goto out;
		}

		first_time = 0;
	} else {
		/* Determine PE ranges to be moved */
		if (!(source_pvl = create_pv_list(cmd->mem, vg, 1,
						  &pv_name_arg, 0)))
			goto_out;

		alloc = arg_uint_value(cmd, alloc_ARG, ALLOC_INHERIT);
		if (alloc == ALLOC_INHERIT)
			alloc = vg->alloc;

		/* Get PVs we can use for allocation */
		if (!(allocatable_pvs = _get_allocatable_pvs(cmd, argc, argv,
							     vg, pv, alloc)))
			goto_out;

		if (!archive(vg))
			goto_out;

		if (!(lv_mirr = _set_up_pvmove_lv(cmd, vg, source_pvl, lv_name,
						  allocatable_pvs, alloc,
						  &lvs_changed)))
			goto_out;
	}

	/* Lock lvs_changed and activate (with old metadata) */
	if (!activate_lvs(cmd, lvs_changed, exclusive))
		goto_out;

	/* FIXME Presence of a mirror once set PVMOVE - now remove associated logic */
	/* init_pvmove(1); */
	/* vg->status |= PVMOVE; */

	if (first_time) {
		if (!_update_metadata
		    (cmd, vg, lv_mirr, lvs_changed, PVMOVE_FIRST_TIME))
			goto_out;
	}

	/* LVs are all in status LOCKED */
	r = ECMD_PROCESSED;
out:
	unlock_and_release_vg(cmd, vg, pv_vg_name(pv));
	return r;
}

static int _finish_pvmove(struct cmd_context *cmd, struct volume_group *vg,
			  struct logical_volume *lv_mirr,
			  struct dm_list *lvs_changed)
{
	int r = 1;
	struct dm_list lvs_completed;
	struct lv_list *lvl;

	/* Update metadata to remove mirror segments and break dependencies */
	dm_list_init(&lvs_completed);
	if (!lv_remove_mirrors(cmd, lv_mirr, 1, 0, NULL, PVMOVE) ||
	    !remove_layers_for_segments_all(cmd, lv_mirr, PVMOVE,
					    &lvs_completed)) {
		log_error("ABORTING: Removal of temporary mirror failed");
		return 0;
	}

	dm_list_iterate_items(lvl, &lvs_completed)
		/* FIXME Assumes only one pvmove at a time! */
		lvl->lv->status &= ~LOCKED;

	/* Store metadata without dependencies on mirror segments */
	if (!vg_write(vg)) {
		log_error("ABORTING: Failed to write new data locations "
			  "to disk.");
		return 0;
	}

	/* Suspend LVs changed */
	if (!suspend_lvs(cmd, lvs_changed)) {
		log_error("Locking LVs to remove temporary mirror failed");
		r = 0;
	}

	/* Suspend mirror LV to flush pending I/O */
	if (!suspend_lv(cmd, lv_mirr)) {
		log_error("Suspension of temporary mirror LV failed");
		r = 0;
	}

	/* Store metadata without dependencies on mirror segments */
	if (!vg_commit(vg)) {
		log_error("ABORTING: Failed to write new data locations "
			  "to disk.");
		vg_revert(vg);
		resume_lv(cmd, lv_mirr);
		resume_lvs(cmd, lvs_changed);
		return 0;
	}

	/* Release mirror LV.  (No pending I/O because it's been suspended.) */
	if (!resume_lv(cmd, lv_mirr)) {
		log_error("Unable to reactivate logical volume \"%s\"",
			  lv_mirr->name);
		r = 0;
	}

	/* Unsuspend LVs */
	resume_lvs(cmd, lvs_changed);

	/* Deactivate mirror LV */
	if (!deactivate_lv(cmd, lv_mirr)) {
		log_error("ABORTING: Unable to deactivate temporary logical "
			  "volume \"%s\"", lv_mirr->name);
		r = 0;
	}

	log_verbose("Removing temporary pvmove LV");
	if (!lv_remove(lv_mirr)) {
		log_error("ABORTING: Removal of temporary pvmove LV failed");
		return 0;
	}

	/* Store it on disks */
	log_verbose("Writing out final volume group after pvmove");
	if (!vg_write(vg) || !vg_commit(vg)) {
		log_error("ABORTING: Failed to write new data locations "
			  "to disk.");
		return 0;
	}

	/* FIXME backup positioning */
	backup(vg);

	return r;
}

static struct volume_group *_get_move_vg(struct cmd_context *cmd,
					 const char *name, const char *uuid)
{
	struct physical_volume *pv;

	/* Reread all metadata in case it got changed */
	if (!(pv = find_pv_by_name(cmd, name))) {
		log_error("ABORTING: Can't reread PV %s", name);
		/* What more could we do here? */
		return NULL;
	}

	return _get_vg(cmd, pv_vg_name(pv));
}

static struct poll_functions _pvmove_fns = {
	.get_copy_name_from_lv = get_pvmove_pvname_from_lv_mirr,
	.get_copy_vg = _get_move_vg,
	.get_copy_lv = find_pvmove_lv_from_pvname,
	.poll_progress = poll_mirror_progress,
	.update_metadata = _update_metadata,
	.finish_copy = _finish_pvmove,
};

int pvmove_poll(struct cmd_context *cmd, const char *pv_name,
		unsigned background)
{
	return poll_daemon(cmd, pv_name, NULL, background, PVMOVE, &_pvmove_fns,
			   "Moved");
}

int pvmove(struct cmd_context *cmd, int argc, char **argv)
{
	char *pv_name = NULL;
	char *colon;
	int ret;

	/* dm raid1 target must be present in every case */
	if (!_pvmove_target_present(cmd, 0)) {
		log_error("Required device-mapper target(s) not "
			  "detected in your kernel");
		return ECMD_FAILED;
	}

	if (argc) {
		pv_name = argv[0];

		/* Drop any PE lists from PV name */
		if ((colon = strchr(pv_name, ':'))) {
			if (!(pv_name = dm_pool_strndup(cmd->mem, pv_name,
						     (unsigned) (colon -
								 pv_name)))) {
				log_error("Failed to clone PV name");
				return ECMD_FAILED;
			}
		}

		if (!arg_count(cmd, abort_ARG) &&
		    (ret = _set_up_pvmove(cmd, pv_name, argc, argv)) !=
		    ECMD_PROCESSED) {
			stack;
			return ret;
		}
	}

	return pvmove_poll(cmd, pv_name, arg_is_set(cmd, background_ARG));
}
