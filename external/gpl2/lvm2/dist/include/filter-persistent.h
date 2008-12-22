/*	$NetBSD: filter-persistent.h,v 1.1.1.1 2008/12/22 00:18:47 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
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

#ifndef _LVM_FILTER_PERSISTENT_H
#define _LVM_FILTER_PERSISTENT_H

#include "dev-cache.h"

struct dev_filter *persistent_filter_create(struct dev_filter *f,
					    const char *file);

int persistent_filter_wipe(struct dev_filter *f);
int persistent_filter_load(struct dev_filter *f, struct config_tree **cft_out);
int persistent_filter_dump(struct dev_filter *f);

#endif
