/*	$NetBSD: dev_manager.h,v 1.1.1.2 2009/12/02 00:26:22 haad Exp $	*/

/*
 * Copyright (C) 2002-2004 Sistina Software, Inc. All rights reserved.  
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

#ifndef _LVM_DEV_MANAGER_H
#define _LVM_DEV_MANAGER_H

#include "metadata-exported.h"

struct logical_volume;
struct volume_group;
struct cmd_context;
struct dev_manager;
struct dm_info;
struct device;

/*
 * Constructor and destructor.
 */
struct dev_manager *dev_manager_create(struct cmd_context *cmd,
				       const char *vg_name);
void dev_manager_destroy(struct dev_manager *dm);
void dev_manager_release(void);
void dev_manager_exit(void);

/*
 * The device handler is responsible for creating all the layered
 * dm devices, and ensuring that all constraints are maintained
 * (eg, an origin is created before its snapshot, but is not
 * unsuspended until the snapshot is also created.)
 */
int dev_manager_info(struct dm_pool *mem, const char *name,
		     const struct logical_volume *lv,
		     int mknodes, int with_open_count, int with_read_ahead,
		     struct dm_info *info, uint32_t *read_ahead);
int dev_manager_snapshot_percent(struct dev_manager *dm,
				 const struct logical_volume *lv,
				 float *percent,
				 percent_range_t *percent_range);
int dev_manager_mirror_percent(struct dev_manager *dm,
			       struct logical_volume *lv, int wait,
			       float *percent, percent_range_t *percent_range,
			       uint32_t *event_nr);
int dev_manager_suspend(struct dev_manager *dm, struct logical_volume *lv,
			int lockfs, int flush_required);
int dev_manager_activate(struct dev_manager *dm, struct logical_volume *lv);
int dev_manager_preload(struct dev_manager *dm, struct logical_volume *lv,
			int *flush_required);
int dev_manager_deactivate(struct dev_manager *dm, struct logical_volume *lv);

int dev_manager_lv_mknodes(const struct logical_volume *lv);
int dev_manager_lv_rmnodes(const struct logical_volume *lv);

/*
 * Put the desired changes into effect.
 */
int dev_manager_execute(struct dev_manager *dm);

int dev_manager_device_uses_vg(struct device *dev,
			       struct volume_group *vg);

#endif
