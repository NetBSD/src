/*	$NetBSD: filter.h,v 1.1.1.2 2009/12/02 00:25:44 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
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

#ifndef _LVM_FILTER_H
#define _LVM_FILTER_H

#include "config.h"

#include <sys/stat.h>

#ifdef linux
#  define MAJOR(dev)	((dev & 0xfff00) >> 8)
#  define MINOR(dev)	((dev & 0xff) | ((dev >> 12) & 0xfff00))
#  define MKDEV(ma,mi)	((mi & 0xff) | (ma << 8) | ((mi & ~0xff) << 12))
#else
#  define MAJOR(x) major((x))
#  define MINOR(x) minor((x))
#  define MKDEV(x,y) makedev((x),(y))
#endif

struct dev_filter *lvm_type_filter_create(const char *proc,
					  const struct config_node *cn);

void lvm_type_filter_destroy(struct dev_filter *f);

int md_major(void);
int blkext_major(void);
int max_partitions(int major);

int dev_subsystem_part_major(const struct device *dev);
const char *dev_subsystem_name(const struct device *dev);

#endif
