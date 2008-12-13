/*	$NetBSD: dev-cache.h,v 1.1.2.2 2008/12/13 14:39:32 haad Exp $	*/

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

#ifndef _LVM_DEV_CACHE_H
#define _LVM_DEV_CACHE_H

#include "device.h"

/*
 * predicate for devices.
 */
struct dev_filter {
	int (*passes_filter) (struct dev_filter * f, struct device * dev);
	void (*destroy) (struct dev_filter * f);
	void *private;
};

/*
 * The global device cache.
 */
struct cmd_context;
int dev_cache_init(struct cmd_context *cmd);
void dev_cache_exit(void);

/* Trigger(1) or avoid(0) a scan */
void dev_cache_scan(int do_scan);
int dev_cache_has_scanned(void);

int dev_cache_add_dir(const char *path);
int dev_cache_add_loopfile(const char *path);
struct device *dev_cache_get(const char *name, struct dev_filter *f);

void dev_set_preferred_name(struct str_list *sl, struct device *dev);

/*
 * Object for iterating through the cache.
 */
struct dev_iter;
struct dev_iter *dev_iter_create(struct dev_filter *f, int dev_scan);
void dev_iter_destroy(struct dev_iter *iter);
struct device *dev_iter_get(struct dev_iter *iter);

#endif
