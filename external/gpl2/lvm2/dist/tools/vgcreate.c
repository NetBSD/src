/*	$NetBSD: vgcreate.c,v 1.1.1.3 2009/12/02 00:25:57 haad Exp $	*/

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

int vgcreate(struct cmd_context *cmd, int argc, char **argv)
{
	struct vgcreate_params vp_new;
	struct vgcreate_params vp_def;
	struct volume_group *vg;
	const char *tag;
	const char *clustered_message = "";
	char *vg_name;
	struct pvcreate_params pp;

	if (!argc) {
		log_error("Please provide volume group name and "
			  "physical volumes");
		return EINVALID_CMD_LINE;
	}

	vg_name = argv[0];
	argc--;
	argv++;

	if (arg_count(cmd, metadatacopies_ARG)) {
		log_error("Invalid option --metadatacopies, "
			  "use --pvmetadatacopies instead.");
		return EINVALID_CMD_LINE;
	}
	pvcreate_params_set_defaults(&pp);
	if (!pvcreate_params_validate(cmd, argc, argv, &pp)) {
		return EINVALID_CMD_LINE;
	}

	vgcreate_params_set_defaults(&vp_def, NULL);
	vp_def.vg_name = vg_name;
	if (vgcreate_params_set_from_args(cmd, &vp_new, &vp_def))
		return EINVALID_CMD_LINE;

	if (vgcreate_params_validate(cmd, &vp_new))
	    return EINVALID_CMD_LINE;

	/* Create the new VG */
	vg = vg_create(cmd, vp_new.vg_name);
	if (vg_read_error(vg))
		goto_bad;

	if (!vg_set_extent_size(vg, vp_new.extent_size) ||
	    !vg_set_max_lv(vg, vp_new.max_lv) ||
	    !vg_set_max_pv(vg, vp_new.max_pv) ||
	    !vg_set_alloc_policy(vg, vp_new.alloc) ||
	    !vg_set_clustered(vg, vp_new.clustered))
		goto_bad;

	if (!lock_vol(cmd, VG_ORPHANS, LCK_VG_WRITE)) {
		log_error("Can't get lock for orphan PVs");
		goto bad_orphan;
	}

	/* attach the pv's */
	if (!vg_extend(vg, argc, argv, &pp))
		goto_bad;

	if (vp_new.max_lv != vg->max_lv)
		log_warn("WARNING: Setting maxlogicalvolumes to %d "
			 "(0 means unlimited)", vg->max_lv);

	if (vp_new.max_pv != vg->max_pv)
		log_warn("WARNING: Setting maxphysicalvolumes to %d "
			 "(0 means unlimited)", vg->max_pv);

	if (arg_count(cmd, addtag_ARG)) {
		if (!(tag = arg_str_value(cmd, addtag_ARG, NULL))) {
			log_error("Failed to get tag");
			goto bad;
		}

		if (!(vg->fid->fmt->features & FMT_TAGS)) {
			log_error("Volume group format does not support tags");
			goto bad;
		}

		if (!str_list_add(cmd->mem, &vg->tags, tag)) {
			log_error("Failed to add tag %s to volume group %s",
				  tag, vp_new.vg_name);
			goto bad;
		}
	}

	if (vg_is_clustered(vg)) {
		clustered_message = "Clustered ";
	} else {
		if (locking_is_clustered())
			clustered_message = "Non-clustered ";
	}

	if (!archive(vg))
		goto_bad;

	/* Store VG on disk(s) */
	if (!vg_write(vg) || !vg_commit(vg))
		goto_bad;

	unlock_vg(cmd, VG_ORPHANS);
	unlock_vg(cmd, vp_new.vg_name);

	backup(vg);

	log_print("%s%colume group \"%s\" successfully created",
		  clustered_message, *clustered_message ? 'v' : 'V', vg->name);

	vg_release(vg);
	return ECMD_PROCESSED;

bad:
	unlock_vg(cmd, VG_ORPHANS);
bad_orphan:
	vg_release(vg);
	unlock_vg(cmd, vp_new.vg_name);
	return ECMD_FAILED;
}
