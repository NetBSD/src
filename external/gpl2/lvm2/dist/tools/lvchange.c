/*	$NetBSD: lvchange.c,v 1.1.1.3 2009/12/02 00:25:49 haad Exp $	*/

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

static int lvchange_permission(struct cmd_context *cmd,
			       struct logical_volume *lv)
{
	uint32_t lv_access;
	struct lvinfo info;
	int r = 0;

	lv_access = arg_uint_value(cmd, permission_ARG, 0);

	if ((lv_access & LVM_WRITE) && (lv->status & LVM_WRITE)) {
		log_error("Logical volume \"%s\" is already writable",
			  lv->name);
		return 0;
	}

	if (!(lv_access & LVM_WRITE) && !(lv->status & LVM_WRITE)) {
		log_error("Logical volume \"%s\" is already read only",
			  lv->name);
		return 0;
	}

	if ((lv->status & MIRRORED) && (vg_is_clustered(lv->vg)) &&
	    lv_info(cmd, lv, &info, 0, 0) && info.exists) {
		log_error("Cannot change permissions of mirror \"%s\" "
			  "while active.", lv->name);
		return 0;
	}

	if (lv_access & LVM_WRITE) {
		lv->status |= LVM_WRITE;
		log_verbose("Setting logical volume \"%s\" read/write",
			    lv->name);
	} else {
		lv->status &= ~LVM_WRITE;
		log_verbose("Setting logical volume \"%s\" read-only",
			    lv->name);
	}

	log_very_verbose("Updating logical volume \"%s\" on disk(s)", lv->name);
	if (!vg_write(lv->vg))
		return_0;

	if (!suspend_lv(cmd, lv)) {
		log_error("Failed to lock %s", lv->name);
		vg_revert(lv->vg);
		goto out;
	}

	if (!vg_commit(lv->vg)) {
		resume_lv(cmd, lv);
		goto_out;
	}

	log_very_verbose("Updating permissions for \"%s\" in kernel", lv->name);
	if (!resume_lv(cmd, lv)) {
		log_error("Problem reactivating %s", lv->name);
		goto out;
	}

	r = 1;
out:
	backup(lv->vg);
	return r;
}

static int lvchange_monitoring(struct cmd_context *cmd,
			       struct logical_volume *lv)
{
	struct lvinfo info;

	if (!lv_info(cmd, lv, &info, 0, 0) || !info.exists) {
		log_error("Logical volume, %s, is not active", lv->name);
		return 0;
	}

	/* do not monitor pvmove lv's */
	if (lv->status & PVMOVE)
		return 1;

	if ((dmeventd_monitor_mode() != DMEVENTD_MONITOR_IGNORE) &&
	    !monitor_dev_for_events(cmd, lv, dmeventd_monitor_mode()))
		stack;

	return 1;
}

static int lvchange_availability(struct cmd_context *cmd,
				 struct logical_volume *lv)
{
	int activate;

	activate = arg_uint_value(cmd, available_ARG, 0);

	if (activate == CHANGE_ALN) {
		log_verbose("Deactivating logical volume \"%s\" locally",
			    lv->name);
		if (!deactivate_lv_local(cmd, lv))
			return_0;
	} else if (activate == CHANGE_AN) {
		log_verbose("Deactivating logical volume \"%s\"", lv->name);
		if (!deactivate_lv(cmd, lv))
			return_0;
	} else {
		if (lv_is_origin(lv) || (activate == CHANGE_AE)) {
			log_verbose("Activating logical volume \"%s\" "
				    "exclusively", lv->name);
			if (!activate_lv_excl(cmd, lv))
				return_0;
		} else if (activate == CHANGE_ALY) {
			log_verbose("Activating logical volume \"%s\" locally",
				    lv->name);
			if (!activate_lv_local(cmd, lv))
				return_0;
		} else {
			log_verbose("Activating logical volume \"%s\"",
				    lv->name);
			if (!activate_lv(cmd, lv))
				return_0;
		}

		lv_spawn_background_polling(cmd, lv);
	}

	return 1;
}

static int lvchange_refresh(struct cmd_context *cmd, struct logical_volume *lv)
{
	log_verbose("Refreshing logical volume \"%s\" (if active)", lv->name);
	return lv_refresh(cmd, lv);
}

static int lvchange_resync(struct cmd_context *cmd,
			      struct logical_volume *lv)
{
	int active = 0;
	int monitored;
	struct lvinfo info;
	struct logical_volume *log_lv;

	if (!(lv->status & MIRRORED)) {
		log_error("Unable to resync %s because it is not mirrored.",
			  lv->name);
		return 1;
	}

	if (lv->status & PVMOVE) {
		log_error("Unable to resync pvmove volume %s", lv->name);
		return 0;
	}

	if (lv->status & LOCKED) {
		log_error("Unable to resync locked volume %s", lv->name);
		return 0;
	}

	if (lv_info(cmd, lv, &info, 1, 0)) {
		if (info.open_count) {
			log_error("Can't resync open logical volume \"%s\"",
				  lv->name);
			return 0;
		}

		if (info.exists) {
			if (!arg_count(cmd, yes_ARG) &&
			    yes_no_prompt("Do you really want to deactivate "
					  "logical volume %s to resync it? [y/n]: ",
					  lv->name) == 'n') {
				log_print("Logical volume \"%s\" not resynced",
					  lv->name);
				return 0;
			}

			if (sigint_caught())
				return 0;

			active = 1;
		}
	}

	/* Activate exclusively to ensure no nodes still have LV active */
	monitored = dmeventd_monitor_mode();
	init_dmeventd_monitor(0);

	if (!deactivate_lv(cmd, lv)) {
		log_error("Unable to deactivate %s for resync", lv->name);
		return 0;
	}

	if (vg_is_clustered(lv->vg) && lv_is_active(lv)) {
		log_error("Can't get exclusive access to clustered volume %s",
			  lv->name);
		return 0;
	}

	init_dmeventd_monitor(monitored);

	log_lv = first_seg(lv)->log_lv;

	log_very_verbose("Starting resync of %s%s%s mirror \"%s\"",
			 (active) ? "active " : "",
			 vg_is_clustered(lv->vg) ? "clustered " : "",
			 (log_lv) ? "disk-logged" : "core-logged",
			 lv->name);

	/*
	 * If this mirror has a core log (i.e. !log_lv),
	 * then simply deactivating/activating will cause
	 * it to reset the sync status.  We only need to
	 * worry about persistent logs.
	 */
	if (!log_lv && !(lv->status & MIRROR_NOTSYNCED)) {
		if (active && !activate_lv(cmd, lv)) {
			log_error("Failed to reactivate %s to resynchronize "
				  "mirror", lv->name);
			return 0;
		}
		return 1;
	}

	lv->status &= ~MIRROR_NOTSYNCED;

	if (log_lv) {
		/* Separate mirror log so we can clear it */
		detach_mirror_log(first_seg(lv));

		if (!vg_write(lv->vg)) {
			log_error("Failed to write intermediate VG metadata.");
			if (!attach_mirror_log(first_seg(lv), log_lv))
				stack;
			if (active && !activate_lv(cmd, lv))
				stack;
			return 0;
		}

		if (!vg_commit(lv->vg)) {
			log_error("Failed to commit intermediate VG metadata.");
			if (!attach_mirror_log(first_seg(lv), log_lv))
				stack;
			if (active && !activate_lv(cmd, lv))
				stack;
			return 0;
		}

		backup(lv->vg);

		if (!activate_lv(cmd, log_lv)) {
			log_error("Unable to activate %s for mirror log resync",
				  log_lv->name);
			return 0;
		}

		log_very_verbose("Clearing log device %s", log_lv->name);
		if (!set_lv(cmd, log_lv, log_lv->size, 0)) {
			log_error("Unable to reset sync status for %s", lv->name);
			if (!deactivate_lv(cmd, log_lv))
				log_error("Failed to deactivate log LV after "
					  "wiping failed");
			return 0;
		}

		if (!deactivate_lv(cmd, log_lv)) {
			log_error("Unable to deactivate log LV %s after wiping "
				  "for resync", log_lv->name);
			return 0;
		}

		/* Put mirror log back in place */
		if (!attach_mirror_log(first_seg(lv), log_lv))
			stack;
	}

	log_very_verbose("Updating logical volume \"%s\" on disk(s)", lv->name);
	if (!vg_write(lv->vg) || !vg_commit(lv->vg)) {
		log_error("Failed to update metadata on disk.");
		return 0;
	}

	if (active && !activate_lv(cmd, lv)) {
		log_error("Failed to reactivate %s after resync", lv->name);
		return 0;
	}

	return 1;
}

static int lvchange_alloc(struct cmd_context *cmd, struct logical_volume *lv)
{
	int want_contiguous = 0;
	alloc_policy_t alloc;

	want_contiguous = strcmp(arg_str_value(cmd, contiguous_ARG, "n"), "n");
	alloc = want_contiguous ? ALLOC_CONTIGUOUS : ALLOC_INHERIT;
	alloc = arg_uint_value(cmd, alloc_ARG, alloc);

	if (alloc == lv->alloc) {
		log_error("Allocation policy of logical volume \"%s\" is "
			  "already %s", lv->name, get_alloc_string(alloc));
		return 0;
	}

	lv->alloc = alloc;

	/* FIXME If contiguous, check existing extents already are */

	log_verbose("Setting contiguous allocation policy for \"%s\" to %s",
		    lv->name, get_alloc_string(alloc));

	log_very_verbose("Updating logical volume \"%s\" on disk(s)", lv->name);

	/* No need to suspend LV for this change */
	if (!vg_write(lv->vg) || !vg_commit(lv->vg))
		return_0;

	backup(lv->vg);

	return 1;
}

static int lvchange_readahead(struct cmd_context *cmd,
			      struct logical_volume *lv)
{
	unsigned read_ahead = 0;
	unsigned pagesize = (unsigned) lvm_getpagesize() >> SECTOR_SHIFT;
	int r = 0;

	read_ahead = arg_uint_value(cmd, readahead_ARG, 0);

	if (read_ahead != DM_READ_AHEAD_AUTO &&
	    (lv->vg->fid->fmt->features & FMT_RESTRICTED_READAHEAD) &&
	    (read_ahead < 2 || read_ahead > 120)) {
		log_error("Metadata only supports readahead values between 2 and 120.");
		return 0;
	}

	if (read_ahead != DM_READ_AHEAD_AUTO &&
	    read_ahead != DM_READ_AHEAD_NONE && read_ahead % pagesize) {
		if (read_ahead < pagesize)
			read_ahead = pagesize;
		else
			read_ahead = (read_ahead / pagesize) * pagesize;
		log_warn("WARNING: Overriding readahead to %u sectors, a multiple "
			    "of %uK page size.", read_ahead, pagesize >> 1);
	}

	if (lv->read_ahead == read_ahead) {
		if (read_ahead == DM_READ_AHEAD_AUTO)
			log_error("Read ahead is already auto for \"%s\"", lv->name);
		else
			log_error("Read ahead is already %u for \"%s\"",
				  read_ahead, lv->name);
		return 0;
	}

	lv->read_ahead = read_ahead;

	log_verbose("Setting read ahead to %u for \"%s\"", read_ahead,
		    lv->name);

	log_very_verbose("Updating logical volume \"%s\" on disk(s)", lv->name);
	if (!vg_write(lv->vg))
		return_0;

	if (!suspend_lv(cmd, lv)) {
		log_error("Failed to lock %s", lv->name);
		vg_revert(lv->vg);
		goto out;
	}

	if (!vg_commit(lv->vg)) {
		resume_lv(cmd, lv);
		goto_out;
	}

	log_very_verbose("Updating permissions for \"%s\" in kernel", lv->name);
	if (!resume_lv(cmd, lv)) {
		log_error("Problem reactivating %s", lv->name);
		goto out;
	}

	r = 1;
out:
	backup(lv->vg);
	return r;
}

static int lvchange_persistent(struct cmd_context *cmd,
			       struct logical_volume *lv)
{
	struct lvinfo info;
	int active = 0;

	if (!strcmp(arg_str_value(cmd, persistent_ARG, "n"), "n")) {
		if (!(lv->status & FIXED_MINOR)) {
			log_error("Minor number is already not persistent "
				  "for \"%s\"", lv->name);
			return 0;
		}
		lv->status &= ~FIXED_MINOR;
		lv->minor = -1;
		lv->major = -1;
		log_verbose("Disabling persistent device number for \"%s\"",
			    lv->name);
	} else {
		if (!arg_count(cmd, minor_ARG) && lv->minor < 0) {
			log_error("Minor number must be specified with -My");
			return 0;
		}
		if (!arg_count(cmd, major_ARG) && lv->major < 0) {
			log_error("Major number must be specified with -My");
			return 0;
		}
		if (lv_info(cmd, lv, &info, 0, 0) && info.exists)
			active = 1;
		if (active && !arg_count(cmd, force_ARG) &&
		    yes_no_prompt("Logical volume %s will be "
				  "deactivated temporarily. "
				  "Continue? [y/n]: ", lv->name) == 'n') {
			log_print("%s device number not changed.",
				  lv->name);
			return 0;
		}

		if (sigint_caught())
			return 0;

		log_verbose("Ensuring %s is inactive.", lv->name);
		if (!deactivate_lv(cmd, lv)) {
			log_error("%s: deactivation failed", lv->name);
			return 0;
		}
		lv->status |= FIXED_MINOR;
		lv->minor = arg_int_value(cmd, minor_ARG, lv->minor);
		lv->major = arg_int_value(cmd, major_ARG, lv->major);
		log_verbose("Setting persistent device number to (%d, %d) "
			    "for \"%s\"", lv->major, lv->minor, lv->name);

	}

	log_very_verbose("Updating logical volume \"%s\" on disk(s)", lv->name);
	if (!vg_write(lv->vg) || !vg_commit(lv->vg))
		return_0;

	backup(lv->vg);

	if (active) {
		log_verbose("Re-activating logical volume \"%s\"", lv->name);
		if (!activate_lv(cmd, lv)) {
			log_error("%s: reactivation failed", lv->name);
			return 0;
		}
	}

	return 1;
}

static int lvchange_tag(struct cmd_context *cmd, struct logical_volume *lv,
			int arg)
{
	const char *tag;

	if (!(tag = arg_str_value(cmd, arg, NULL))) {
		log_error("Failed to get tag");
		return 0;
	}

	if (!(lv->vg->fid->fmt->features & FMT_TAGS)) {
		log_error("Logical volume %s/%s does not support tags",
			  lv->vg->name, lv->name);
		return 0;
	}

	if ((arg == addtag_ARG)) {
		if (!str_list_add(cmd->mem, &lv->tags, tag)) {
			log_error("Failed to add tag %s to %s/%s",
				  tag, lv->vg->name, lv->name);
			return 0;
		}
	} else {
		if (!str_list_del(&lv->tags, tag)) {
			log_error("Failed to remove tag %s from %s/%s",
				  tag, lv->vg->name, lv->name);
			return 0;
		}
	}

	log_very_verbose("Updating logical volume \"%s\" on disk(s)", lv->name);

	/* No need to suspend LV for this change */
	if (!vg_write(lv->vg) || !vg_commit(lv->vg))
		return_0;

	backup(lv->vg);

	return 1;
}

static int lvchange_single(struct cmd_context *cmd, struct logical_volume *lv,
			   void *handle __attribute((unused)))
{
	int doit = 0, docmds = 0;
	int archived = 0;
	struct logical_volume *origin;

	if (!(lv->vg->status & LVM_WRITE) &&
	    (arg_count(cmd, contiguous_ARG) || arg_count(cmd, permission_ARG) ||
	     arg_count(cmd, readahead_ARG) || arg_count(cmd, persistent_ARG) ||
	     arg_count(cmd, alloc_ARG))) {
		log_error("Only -a permitted with read-only volume "
			  "group \"%s\"", lv->vg->name);
		return EINVALID_CMD_LINE;
	}

	if (lv_is_origin(lv) &&
	    (arg_count(cmd, contiguous_ARG) || arg_count(cmd, permission_ARG) ||
	     arg_count(cmd, readahead_ARG) || arg_count(cmd, persistent_ARG) ||
	     arg_count(cmd, alloc_ARG))) {
		log_error("Can't change logical volume \"%s\" under snapshot",
			  lv->name);
		return ECMD_FAILED;
	}

	if (lv_is_cow(lv) && !lv_is_virtual_origin(origin_from_cow(lv)) &&
	    arg_count(cmd, available_ARG)) {
		log_error("Can't change snapshot logical volume \"%s\"",
			  lv->name);
		return ECMD_FAILED;
	}

	if (lv->status & PVMOVE) {
		log_error("Unable to change pvmove LV %s", lv->name);
		if (arg_count(cmd, available_ARG))
			log_error("Use 'pvmove --abort' to abandon a pvmove");
		return ECMD_FAILED;
	}

	if (lv->status & MIRROR_LOG) {
		log_error("Unable to change mirror log LV %s directly", lv->name);
		return ECMD_FAILED;
	}

	if (lv->status & MIRROR_IMAGE) {
		log_error("Unable to change mirror image LV %s directly",
			  lv->name);
		return ECMD_FAILED;
	}

	/* If LV is sparse, activate origin instead */
	if (arg_count(cmd, available_ARG) && lv_is_cow(lv) &&
	    lv_is_virtual_origin(origin = origin_from_cow(lv)))
		lv = origin;

	if (!(lv_is_visible(lv)) && !lv_is_virtual_origin(lv)) {
		log_error("Unable to change internal LV %s directly",
			  lv->name);
		return ECMD_FAILED;
	}

	init_dmeventd_monitor(arg_int_value(cmd, monitor_ARG,
					    (is_static() || arg_count(cmd, ignoremonitoring_ARG)) ?
					    DMEVENTD_MONITOR_IGNORE : DEFAULT_DMEVENTD_MONITOR));

	/* access permission change */
	if (arg_count(cmd, permission_ARG)) {
		if (!archive(lv->vg)) {
			stack;
			return ECMD_FAILED;
		}
		archived = 1;
		doit += lvchange_permission(cmd, lv);
		docmds++;
	}

	/* allocation policy change */
	if (arg_count(cmd, contiguous_ARG) || arg_count(cmd, alloc_ARG)) {
		if (!archived && !archive(lv->vg)) {
			stack;
			return ECMD_FAILED;
		}
		archived = 1;
		doit += lvchange_alloc(cmd, lv);
		docmds++;
	}

	/* read ahead sector change */
	if (arg_count(cmd, readahead_ARG)) {
		if (!archived && !archive(lv->vg)) {
			stack;
			return ECMD_FAILED;
		}
		archived = 1;
		doit += lvchange_readahead(cmd, lv);
		docmds++;
	}

	/* persistent device number change */
	if (arg_count(cmd, persistent_ARG)) {
		if (!archived && !archive(lv->vg)) {
			stack;
			return ECMD_FAILED;
		}
		archived = 1;
		doit += lvchange_persistent(cmd, lv);
		docmds++;
		if (sigint_caught()) {
			stack;
			return ECMD_FAILED;
		}
	}

	/* add tag */
	if (arg_count(cmd, addtag_ARG)) {
		if (!archived && !archive(lv->vg)) {
			stack;
			return ECMD_FAILED;
		}
		archived = 1;
		doit += lvchange_tag(cmd, lv, addtag_ARG);
		docmds++;
	}

	/* del tag */
	if (arg_count(cmd, deltag_ARG)) {
		if (!archived && !archive(lv->vg)) {
			stack;
			return ECMD_FAILED;
		}
		archived = 1;
		doit += lvchange_tag(cmd, lv, deltag_ARG);
		docmds++;
	}

	if (doit)
		log_print("Logical volume \"%s\" changed", lv->name);

	if (arg_count(cmd, resync_ARG))
		if (!lvchange_resync(cmd, lv)) {
			stack;
			return ECMD_FAILED;
		}

	/* availability change */
	if (arg_count(cmd, available_ARG)) {
		if (!lvchange_availability(cmd, lv)) {
			stack;
			return ECMD_FAILED;
		}
	}

	if (arg_count(cmd, refresh_ARG))
		if (!lvchange_refresh(cmd, lv)) {
			stack;
			return ECMD_FAILED;
		}

	if (!arg_count(cmd, available_ARG) &&
	    !arg_count(cmd, refresh_ARG) &&
	    arg_count(cmd, monitor_ARG)) {
		if (!lvchange_monitoring(cmd, lv)) {
			stack;
			return ECMD_FAILED;
		}
	}

	if (doit != docmds) {
		stack;
		return ECMD_FAILED;
	}

	return ECMD_PROCESSED;
}

int lvchange(struct cmd_context *cmd, int argc, char **argv)
{
	if (!arg_count(cmd, available_ARG) && !arg_count(cmd, contiguous_ARG)
	    && !arg_count(cmd, permission_ARG) && !arg_count(cmd, readahead_ARG)
	    && !arg_count(cmd, minor_ARG) && !arg_count(cmd, major_ARG)
	    && !arg_count(cmd, persistent_ARG) && !arg_count(cmd, addtag_ARG)
	    && !arg_count(cmd, deltag_ARG) && !arg_count(cmd, refresh_ARG)
	    && !arg_count(cmd, alloc_ARG) && !arg_count(cmd, monitor_ARG)
	    && !arg_count(cmd, resync_ARG)) {
		log_error("Need 1 or more of -a, -C, -j, -m, -M, -p, -r, "
			  "--resync, --refresh, --alloc, --addtag, --deltag "
			  "or --monitor");
		return EINVALID_CMD_LINE;
	}

	int avail_only = /* i.e. only one of -a or --refresh is given */
	    !(arg_count(cmd, contiguous_ARG) || arg_count(cmd, permission_ARG) ||
	     arg_count(cmd, readahead_ARG) || arg_count(cmd, persistent_ARG) ||
	     arg_count(cmd, addtag_ARG) || arg_count(cmd, deltag_ARG) ||
	     arg_count(cmd, resync_ARG) || arg_count(cmd, alloc_ARG));

	if (arg_count(cmd, ignorelockingfailure_ARG) && !avail_only) {
		log_error("Only -a permitted with --ignorelockingfailure");
		return EINVALID_CMD_LINE;
	}

	if (avail_only)
		cmd->handles_missing_pvs = 1;

	if (!argc) {
		log_error("Please give logical volume path(s)");
		return EINVALID_CMD_LINE;
	}

	if ((arg_count(cmd, minor_ARG) || arg_count(cmd, major_ARG)) &&
	    !arg_count(cmd, persistent_ARG)) {
		log_error("--major and --minor require -My");
		return EINVALID_CMD_LINE;
	}

	if (arg_count(cmd, minor_ARG) && argc != 1) {
		log_error("Only give one logical volume when specifying minor");
		return EINVALID_CMD_LINE;
	}

	if (arg_count(cmd, contiguous_ARG) && arg_count(cmd, alloc_ARG)) {
		log_error("Only one of --alloc and --contiguous permitted");
		return EINVALID_CMD_LINE;
	}

	return process_each_lv(cmd, argc, argv,
			       avail_only ? 0 : READ_FOR_UPDATE, NULL,
			       &lvchange_single);
}
