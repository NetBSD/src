/*	$NetBSD: vgcreate.c,v 1.1.1.1 2008/12/22 00:19:09 haad Exp $	*/

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

int vgcreate(struct cmd_context *cmd, int argc, char **argv)
{
	struct vgcreate_params vp_new;
	struct vgcreate_params vp_def;
	struct volume_group *vg;
	const char *tag;
	const char *clustered_message = "";

	if (!argc) {
		log_error("Please provide volume group name and "
			  "physical volumes");
		return EINVALID_CMD_LINE;
	}

	if (argc == 1) {
		log_error("Please enter physical volume name(s)");
		return EINVALID_CMD_LINE;
	}

	vp_def.vg_name = NULL;
	vp_def.extent_size = DEFAULT_EXTENT_SIZE * 2;
	vp_def.max_pv = 0;
	vp_def.max_lv = 0;
	vp_def.alloc = ALLOC_NORMAL;
	vp_def.clustered = 0;
	if (fill_vg_create_params(cmd, argv[0], &vp_new, &vp_def))
		return EINVALID_CMD_LINE;

	if (validate_vg_create_params(cmd, &vp_new))
	    return EINVALID_CMD_LINE;

	/* Create the new VG */
	if (!(vg = vg_create(cmd, vp_new.vg_name, vp_new.extent_size,
			     vp_new.max_pv, vp_new.max_lv, vp_new.alloc,
			     argc - 1, argv + 1)))
		return ECMD_FAILED;

	if (vp_new.max_lv != vg->max_lv)
		log_warn("WARNING: Setting maxlogicalvolumes to %d "
			 "(0 means unlimited)", vg->max_lv);

	if (vp_new.max_pv != vg->max_pv)
		log_warn("WARNING: Setting maxphysicalvolumes to %d "
			 "(0 means unlimited)", vg->max_pv);

	if (arg_count(cmd, addtag_ARG)) {
		if (!(tag = arg_str_value(cmd, addtag_ARG, NULL))) {
			log_error("Failed to get tag");
			return ECMD_FAILED;
		}

		if (!(vg->fid->fmt->features & FMT_TAGS)) {
			log_error("Volume group format does not support tags");
			return ECMD_FAILED;
		}

		if (!str_list_add(cmd->mem, &vg->tags, tag)) {
			log_error("Failed to add tag %s to volume group %s",
				  tag, vp_new.vg_name);
			return ECMD_FAILED;
		}
	}

	/* FIXME: move this inside vg_create? */
	if (vp_new.clustered) {
		vg->status |= CLUSTERED;
		clustered_message = "Clustered ";
	} else {
		vg->status &= ~CLUSTERED;
		if (locking_is_clustered())
			clustered_message = "Non-clustered ";
	}

	if (!lock_vol(cmd, VG_ORPHANS, LCK_VG_WRITE)) {
		log_error("Can't get lock for orphan PVs");
		return ECMD_FAILED;
	}

	if (!lock_vol(cmd, vp_new.vg_name, LCK_VG_WRITE | LCK_NONBLOCK)) {
		log_error("Can't get lock for %s", vp_new.vg_name);
		unlock_vg(cmd, VG_ORPHANS);
		return ECMD_FAILED;
	}

	if (!archive(vg)) {
		unlock_vg(cmd, vp_new.vg_name);
		unlock_vg(cmd, VG_ORPHANS);
		return ECMD_FAILED;
	}

	/* Store VG on disk(s) */
	if (!vg_write(vg) || !vg_commit(vg)) {
		unlock_vg(cmd, vp_new.vg_name);
		unlock_vg(cmd, VG_ORPHANS);
		return ECMD_FAILED;
	}

	unlock_vg(cmd, vp_new.vg_name);
	unlock_vg(cmd, VG_ORPHANS);

	backup(vg);

	log_print("%s%colume group \"%s\" successfully created",
		  clustered_message, *clustered_message ? 'v' : 'V', vg->name);

	return ECMD_PROCESSED;
}
