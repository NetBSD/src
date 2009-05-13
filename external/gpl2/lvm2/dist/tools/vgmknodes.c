/*	$NetBSD: vgmknodes.c,v 1.1.1.1.2.1 2009/05/13 18:52:48 jym Exp $	*/

/*
 * Copyright (C) 2003-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2006 Red Hat, Inc. All rights reserved.
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

static int _vgmknodes_single(struct cmd_context *cmd, struct logical_volume *lv,
			     void *handle __attribute((unused)))
{
	if (arg_count(cmd, refresh_ARG) && lv_is_visible(lv))
		if (!lv_refresh(cmd, lv))
			return ECMD_FAILED;

	if (!lv_mknodes(cmd, lv))
		return ECMD_FAILED;

	return ECMD_PROCESSED;
}

int vgmknodes(struct cmd_context *cmd, int argc, char **argv)
{
	if (!lv_mknodes(cmd, NULL))
		return ECMD_FAILED;

	return process_each_lv(cmd, argc, argv, LCK_VG_READ, NULL,
			    &_vgmknodes_single);
}
