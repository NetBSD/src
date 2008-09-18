/*
 * Copyright (C) 1997-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2005 Red Hat, Inc. All rights reserved.
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

#ifndef SPTYPE_NAMES_H
#define SPTYPE_NAMES_H

/* This must be kept up to date with sistina/pool/module/pool_sptypes.h */

/*  Generic Labels  */
#define SPTYPE_DATA                (0x00000000)

/*  GFS specific labels  */
#define SPTYPE_GFS_DATA            (0x68011670)
#define SPTYPE_GFS_JOURNAL         (0x69011670)

struct sptype_name {
	const char *name;
	uint32_t label;
};

static const struct sptype_name sptype_names[] = {
	{"data",	SPTYPE_DATA},

	{"gfs_data",	SPTYPE_GFS_DATA},
	{"gfs_journal",	SPTYPE_GFS_JOURNAL},

	{"", 0x0}		/*  This must be the last flag.  */
};

#endif
