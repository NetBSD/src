/*	$NetBSD: filter-sysfs.h,v 1.1.1.2 2008/12/12 11:42:21 haad Exp $	*/

/*
 * Copyright (C) 2004 Red Hat, Inc. All rights reserved.
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

#ifndef _LVM_FILTER_SYSFS_H
#define _LVM_FILTER_SYSFS_H

#include "config.h"
#include "dev-cache.h"

struct dev_filter *sysfs_filter_create(const char *sysfs_dir);

#endif
