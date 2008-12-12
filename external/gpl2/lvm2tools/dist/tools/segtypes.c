/*	$NetBSD: segtypes.c,v 1.1.1.1.2.2 2008/12/12 16:33:05 haad Exp $	*/

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

int segtypes(struct cmd_context *cmd, int argc __attribute((unused)),
	     char **argv __attribute((unused)))
{
	display_segtypes(cmd);

	return ECMD_PROCESSED;
}
