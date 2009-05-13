/*	$NetBSD: vgrename.c,v 1.1.1.1.2.1 2009/05/13 18:52:48 jym Exp $	*/

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

static int vg_rename_path(struct cmd_context *cmd, const char *old_vg_path,
			  const char *new_vg_path)
{
	char *dev_dir;
	struct id id;
	int consistent = 1;
	int match = 0;
	int found_id = 0;
	struct dm_list *vgids;
	struct str_list *sl;
	char *vg_name_new;
	const char *vgid = NULL, *vg_name, *vg_name_old;
	char old_path[NAME_LEN], new_path[NAME_LEN];
	struct volume_group *vg, *vg_new;

	vg_name_old = skip_dev_dir(cmd, old_vg_path, NULL);
	vg_name_new = skip_dev_dir(cmd, new_vg_path, NULL);

	dev_dir = cmd->dev_dir;

	if (!validate_vg_rename_params(cmd, vg_name_old, vg_name_new))
		return 0;

	log_verbose("Checking for existing volume group \"%s\"", vg_name_old);

	/* Avoid duplicates */
	if (!(vgids = get_vgids(cmd, 0)) || dm_list_empty(vgids)) {
		log_error("No complete volume groups found");
		return 0;
	}

	dm_list_iterate_items(sl, vgids) {
		vgid = sl->str;
		if (!vgid || !(vg_name = vgname_from_vgid(NULL, vgid)) ||
		    is_orphan_vg(vg_name))
			continue;
		if (!strcmp(vg_name, vg_name_old)) {
			if (match) {
				log_error("Found more than one VG called %s. "
					  "Please supply VG uuid.", vg_name_old);
				return 0;
			}
			match = 1;
		}
	}

	log_suppress(2);
	found_id = id_read_format(&id, vg_name_old);
	log_suppress(0);
	if (found_id && (vg_name = vgname_from_vgid(cmd->mem, (char *)id.uuid))) {
		vg_name_old = vg_name;
		vgid = (char *)id.uuid;
	} else
		vgid = NULL;

	if (!lock_vol(cmd, vg_name_old, LCK_VG_WRITE)) {
		log_error("Can't get lock for %s", vg_name_old);
		return 0;
	}

	if (!(vg = vg_read(cmd, vg_name_old, vgid, &consistent)) || !consistent) {
		log_error("Volume group %s %s%s%snot found.", vg_name_old,
		vgid ? "(" : "", vgid ? vgid : "", vgid ? ") " : "");
		unlock_vg(cmd, vg_name_old);
		return 0;
	}

	if (!vg_check_status(vg, CLUSTERED | LVM_WRITE)) {
		unlock_vg(cmd, vg_name_old);
		return 0;
	}

	/* Don't return failure for EXPORTED_VG */
	vg_check_status(vg, EXPORTED_VG);

	if (lvs_in_vg_activated_by_uuid_only(vg)) {
		unlock_vg(cmd, vg_name_old);
		log_error("Volume group \"%s\" still has active LVs",
			  vg_name_old);
		/* FIXME Remove this restriction */
		return 0;
	}

	log_verbose("Checking for new volume group \"%s\"", vg_name_new);

	if (!lock_vol(cmd, vg_name_new, LCK_VG_WRITE | LCK_NONBLOCK)) {
		unlock_vg(cmd, vg_name_old);
		log_error("Can't get lock for %s", vg_name_new);
		return 0;
	}

	consistent = 0;
	if ((vg_new = vg_read(cmd, vg_name_new, NULL, &consistent))) {
		log_error("New volume group \"%s\" already exists",
			  vg_name_new);
		goto error;
	}

	if (!archive(vg))
		goto error;

	/* Remove references based on old name */
	drop_cached_metadata(vg);

	/* Change the volume group name */
	vg_rename(cmd, vg, vg_name_new);

	/* store it on disks */
	log_verbose("Writing out updated volume group");
	if (!vg_write(vg) || !vg_commit(vg)) {
		goto error;
	}

	sprintf(old_path, "%s%s", dev_dir, vg_name_old);
	sprintf(new_path, "%s%s", dev_dir, vg_name_new);

	if (activation() && dir_exists(old_path)) {
		log_verbose("Renaming \"%s\" to \"%s\"", old_path, new_path);

		if (test_mode())
			log_verbose("Test mode: Skipping rename.");

		else if (lvs_in_vg_activated_by_uuid_only(vg)) {
			if (!vg_refresh_visible(cmd, vg)) {
				log_error("Renaming \"%s\" to \"%s\" failed", 
					old_path, new_path);
				goto error;
			}
		}
	}

/******* FIXME Rename any active LVs! *****/

	backup(vg);

	unlock_vg(cmd, vg_name_new);
	unlock_vg(cmd, vg_name_old);

	log_print("Volume group \"%s\" successfully renamed to \"%s\"",
		  vg_name_old, vg_name_new);

	/* FIXME lvmcache corruption - vginfo duplicated instead of renamed */
	persistent_filter_wipe(cmd->filter);
	lvmcache_destroy(cmd, 1);

	return 1;

      error:
	unlock_vg(cmd, vg_name_new);
	unlock_vg(cmd, vg_name_old);
	return 0;
}

int vgrename(struct cmd_context *cmd, int argc, char **argv)
{
	if (argc != 2) {
		log_error("Old and new volume group names need specifying");
		return EINVALID_CMD_LINE;
	}

	if (!vg_rename_path(cmd, argv[0], argv[1]))
		return ECMD_FAILED;
	
	return ECMD_PROCESSED;
}

