/*	$NetBSD: lvrename.c,v 1.1.1.2 2009/12/02 00:25:52 haad Exp $	*/

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
#include "lvm-types.h"


/*
 * lvrename command implementation.
 * Check arguments and call lv_rename() to execute the request.
 */
int lvrename(struct cmd_context *cmd, int argc, char **argv)
{
	size_t maxlen;
	char *lv_name_old, *lv_name_new;
	const char *vg_name, *vg_name_new, *vg_name_old;
	char *st;
	int r = ECMD_FAILED;

	struct volume_group *vg = NULL;
	struct lv_list *lvl;

	if (argc == 3) {
		vg_name = skip_dev_dir(cmd, argv[0], NULL);
		lv_name_old = argv[1];
		lv_name_new = argv[2];
		if (strchr(lv_name_old, '/') &&
		    (vg_name_old = extract_vgname(cmd, lv_name_old)) &&
		    strcmp(vg_name_old, vg_name)) {
			log_error("Please use a single volume group name "
				  "(\"%s\" or \"%s\")", vg_name, vg_name_old);
			return EINVALID_CMD_LINE;
		}
	} else if (argc == 2) {
		lv_name_old = argv[0];
		lv_name_new = argv[1];
		vg_name = extract_vgname(cmd, lv_name_old);
	} else {
		log_error("Old and new logical volume names required");
		return EINVALID_CMD_LINE;
	}

	if (!validate_name(vg_name)) {
		log_error("Please provide a valid volume group name");
		return EINVALID_CMD_LINE;
	}

	if (strchr(lv_name_new, '/') &&
	    (vg_name_new = extract_vgname(cmd, lv_name_new)) &&
	    strcmp(vg_name, vg_name_new)) {
		log_error("Logical volume names must "
			  "have the same volume group (\"%s\" or \"%s\")",
			  vg_name, vg_name_new);
		return EINVALID_CMD_LINE;
	}

	if ((st = strrchr(lv_name_old, '/')))
		lv_name_old = st + 1;

	if ((st = strrchr(lv_name_new, '/')))
		lv_name_new = st + 1;

	/* Check sanity of new name */
	maxlen = NAME_LEN - strlen(vg_name) - strlen(cmd->dev_dir) - 3;
	if (strlen(lv_name_new) > maxlen) {
		log_error("New logical volume path exceeds maximum length "
			  "of %" PRIsize_t "!", maxlen);
		return ECMD_FAILED;
	}

	if (!*lv_name_new) {
		log_error("New logical volume name may not be blank");
		return ECMD_FAILED;
	}

	if (!apply_lvname_restrictions(lv_name_new)) {
		stack;
		return ECMD_FAILED;
	}

	if (!validate_name(lv_name_new)) {
		log_error("New logical volume name \"%s\" is invalid",
		     lv_name_new);
		return EINVALID_CMD_LINE;
	}

	if (!strcmp(lv_name_old, lv_name_new)) {
		log_error("Old and new logical volume names must differ");
		return EINVALID_CMD_LINE;
	}

	log_verbose("Checking for existing volume group \"%s\"", vg_name);
	vg = vg_read_for_update(cmd, vg_name, NULL, 0);
	if (vg_read_error(vg)) {
		vg_release(vg);
		stack;
		return ECMD_FAILED;
	}

	if (!(lvl = find_lv_in_vg(vg, lv_name_old))) {
		log_error("Existing logical volume \"%s\" not found in "
			  "volume group \"%s\"", lv_name_old, vg_name);
		goto error;
	}

	if (!lv_rename(cmd, lvl->lv, lv_name_new))
		goto error;

	log_print("Renamed \"%s\" to \"%s\" in volume group \"%s\"",
		  lv_name_old, lv_name_new, vg_name);

	r = ECMD_PROCESSED;
error:
	unlock_and_release_vg(cmd, vg, vg_name);
	return r;
}
