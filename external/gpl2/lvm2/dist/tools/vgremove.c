/*	$NetBSD: vgremove.c,v 1.1.1.1 2008/12/22 00:19:09 haad Exp $	*/

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

static int vgremove_single(struct cmd_context *cmd, const char *vg_name,
			   struct volume_group *vg, int consistent,
			   void *handle __attribute((unused)))
{
	if (!vg_remove_single(cmd, vg_name, vg, consistent,
			      arg_count(cmd, force_ARG)))
		return ECMD_FAILED;

	return ECMD_PROCESSED;
}

int vgremove(struct cmd_context *cmd, int argc, char **argv)
{
	int ret;

	if (!argc) {
		log_error("Please enter one or more volume group paths");
		return EINVALID_CMD_LINE;
	}

	if (!lock_vol(cmd, VG_ORPHANS, LCK_VG_WRITE)) {
		log_error("Can't get lock for orphan PVs");
		return ECMD_FAILED;
	}

	ret = process_each_vg(cmd, argc, argv,
			      LCK_VG_WRITE | LCK_NONBLOCK, 1,
			      NULL, &vgremove_single);

	unlock_vg(cmd, VG_ORPHANS);

	return ret;
}
