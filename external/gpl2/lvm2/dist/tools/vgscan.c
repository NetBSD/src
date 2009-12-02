/*	$NetBSD: vgscan.c,v 1.1.1.2 2009/12/02 00:25:58 haad Exp $	*/

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

static int vgscan_single(struct cmd_context *cmd, const char *vg_name,
			 struct volume_group *vg,
			 void *handle __attribute((unused)))
{
	log_print("Found %svolume group \"%s\" using metadata type %s",
		  vg_is_exported(vg) ? "exported " : "", vg_name,
		  vg->fid->fmt->name);

	check_current_backup(vg);

	return ECMD_PROCESSED;
}

int vgscan(struct cmd_context *cmd, int argc, char **argv)
{
	int maxret, ret;

	if (argc) {
		log_error("Too many parameters on command line");
		return EINVALID_CMD_LINE;
	}

	if (!lock_vol(cmd, VG_GLOBAL, LCK_VG_WRITE)) {
		log_error("Unable to obtain global lock.");
		return ECMD_FAILED;
	}

	persistent_filter_wipe(cmd->filter);
	lvmcache_destroy(cmd, 1);

	log_print("Reading all physical volumes.  This may take a while...");

	maxret = process_each_vg(cmd, argc, argv, 0, NULL,
				 &vgscan_single);

	if (arg_count(cmd, mknodes_ARG)) {
		ret = vgmknodes(cmd, argc, argv);
		if (ret > maxret)
			maxret = ret;
	}

	unlock_vg(cmd, VG_GLOBAL);
	return maxret;
}
