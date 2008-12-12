/*	$NetBSD: vgck.c,v 1.1.1.1.2.2 2008/12/12 16:33:05 haad Exp $	*/

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

static int vgck_single(struct cmd_context *cmd __attribute((unused)),
		       const char *vg_name,
		       struct volume_group *vg, int consistent,
		       void *handle __attribute((unused)))
{
	if (!vg) {
		log_error("Volume group \"%s\" not found", vg_name);
		return ECMD_FAILED;
	}

	if (!consistent) {
		log_error("Volume group \"%s\" inconsistent", vg_name);
		return ECMD_FAILED;
	}

	if (!vg_check_status(vg, EXPORTED_VG))
		return ECMD_FAILED;

	return ECMD_PROCESSED;
}

int vgck(struct cmd_context *cmd, int argc, char **argv)
{
	return process_each_vg(cmd, argc, argv, LCK_VG_READ, 0, NULL,
			       &vgck_single);
}
