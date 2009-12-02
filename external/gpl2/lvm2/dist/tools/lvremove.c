/*	$NetBSD: lvremove.c,v 1.1.1.3 2009/12/02 00:25:52 haad Exp $	*/

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

static int lvremove_single(struct cmd_context *cmd, struct logical_volume *lv,
			   void *handle __attribute((unused)))
{
	struct logical_volume *origin;

	/*
	 * If this is a sparse device, remove its origin too.
	 */
        if (lv_is_cow(lv) && lv_is_virtual_origin(origin = origin_from_cow(lv)))
                lv = origin;

	if (!lv_remove_with_dependencies(cmd, lv, arg_count(cmd, force_ARG))) {
		stack;
		return ECMD_FAILED;
	}

	return ECMD_PROCESSED;
}

int lvremove(struct cmd_context *cmd, int argc, char **argv)
{
	if (!argc) {
		log_error("Please enter one or more logical volume paths");
		return EINVALID_CMD_LINE;
	}

	cmd->handles_missing_pvs = 1;

	return process_each_lv(cmd, argc, argv, READ_FOR_UPDATE, NULL,
			       &lvremove_single);
}
