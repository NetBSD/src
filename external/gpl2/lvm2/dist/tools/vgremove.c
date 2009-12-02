/*	$NetBSD: vgremove.c,v 1.1.1.2 2009/12/02 00:25:58 haad Exp $	*/

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

static int vgremove_single(struct cmd_context *cmd, const char *vg_name,
			   struct volume_group *vg,
			   void *handle __attribute((unused)))
{
	unsigned lv_count;
	force_t force;

	if (!vg_check_status(vg, EXPORTED_VG)) {
		stack;
		return ECMD_FAILED;
	}

	lv_count = vg_visible_lvs(vg);

	force = arg_count(cmd, force_ARG);
	if (lv_count) {
		if ((force == PROMPT) &&
		    (yes_no_prompt("Do you really want to remove volume "
				   "group \"%s\" containing %u "
				   "logical volumes? [y/n]: ",
				   vg_name, lv_count) == 'n')) {
			log_print("Volume group \"%s\" not removed", vg_name);
			return ECMD_FAILED;
		}
		if (!remove_lvs_in_vg(cmd, vg, force)) {
			stack;
			return ECMD_FAILED;
		}
	}

	if (!vg_remove_check(vg)) {
		stack;
		return ECMD_FAILED;
	}

	if (!vg_remove(vg)) {
		stack;
		return ECMD_FAILED;
	}

	return ECMD_PROCESSED;
}

int vgremove(struct cmd_context *cmd, int argc, char **argv)
{
	int ret;

	if (!argc) {
		log_error("Please enter one or more volume group paths");
		return EINVALID_CMD_LINE;
	}

	ret = process_each_vg(cmd, argc, argv,
			      READ_FOR_UPDATE,
			      NULL, &vgremove_single);

	return ret;
}
