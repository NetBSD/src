/*	$NetBSD: vgextend.c,v 1.1.1.2 2009/12/02 00:25:57 haad Exp $	*/

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

int vgextend(struct cmd_context *cmd, int argc, char **argv)
{
	char *vg_name;
	struct volume_group *vg = NULL;
	int r = ECMD_FAILED;
	struct pvcreate_params pp;

	if (!argc) {
		log_error("Please enter volume group name and "
			  "physical volume(s)");
		return EINVALID_CMD_LINE;
	}

	vg_name = skip_dev_dir(cmd, argv[0], NULL);
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

	log_verbose("Checking for volume group \"%s\"", vg_name);
	vg = vg_read_for_update(cmd, vg_name, NULL, 0);
	if (vg_read_error(vg)) {
		vg_release(vg);
		stack;
		return ECMD_FAILED;
	}

	if (!lock_vol(cmd, VG_ORPHANS, LCK_VG_WRITE)) {
		log_error("Can't get lock for orphan PVs");
		unlock_and_release_vg(cmd, vg, vg_name);
		return ECMD_FAILED;
	}

	if (!archive(vg))
		goto_bad;

	/* extend vg */
	if (!vg_extend(vg, argc, argv, &pp))
		goto_bad;

	/* ret > 0 */
	log_verbose("Volume group \"%s\" will be extended by %d new "
		    "physical volumes", vg_name, argc);

	/* store vg on disk(s) */
	if (!vg_write(vg) || !vg_commit(vg))
		goto_bad;

	backup(vg);
	log_print("Volume group \"%s\" successfully extended", vg_name);
	r = ECMD_PROCESSED;

bad:
	unlock_vg(cmd, VG_ORPHANS);
	unlock_and_release_vg(cmd, vg, vg_name);
	return r;
}
