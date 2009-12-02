/*	$NetBSD: activate.h,v 1.1.1.2 2009/12/02 00:25:45 haad Exp $	*/

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

#ifndef LVM_ACTIVATE_H
#define LVM_ACTIVATE_H

#include "metadata-exported.h"

struct lvinfo {
	int exists;
	int suspended;
	unsigned int open_count;
	int major;
	int minor;
	int read_only;
	int live_table;
	int inactive_table;
	uint32_t read_ahead;
};

/* target attribute flags */
#define MIRROR_LOG_CLUSTERED	0x00000001U

void set_activation(int activation);
int activation(void);

int driver_version(char *version, size_t size);
int library_version(char *version, size_t size);
int lvm1_present(struct cmd_context *cmd);

int module_present(struct cmd_context *cmd, const char *target_name);
int target_present(struct cmd_context *cmd, const char *target_name,
		   int use_modprobe);
int target_version(const char *target_name, uint32_t *maj,
                   uint32_t *min, uint32_t *patchlevel);
int list_segment_modules(struct dm_pool *mem, const struct lv_segment *seg,
			 struct dm_list *modules);
int list_lv_modules(struct dm_pool *mem, const struct logical_volume *lv,
		    struct dm_list *modules);

void activation_release(void);
void activation_exit(void);

int lv_suspend(struct cmd_context *cmd, const char *lvid_s);
int lv_suspend_if_active(struct cmd_context *cmd, const char *lvid_s);
int lv_resume(struct cmd_context *cmd, const char *lvid_s);
int lv_resume_if_active(struct cmd_context *cmd, const char *lvid_s);
int lv_activate(struct cmd_context *cmd, const char *lvid_s, int exclusive);
int lv_activate_with_filter(struct cmd_context *cmd, const char *lvid_s,
			    int exclusive);
int lv_deactivate(struct cmd_context *cmd, const char *lvid_s);

int lv_mknodes(struct cmd_context *cmd, const struct logical_volume *lv);

/*
 * Returns 1 if info structure has been populated, else 0.
 */
int lv_info(struct cmd_context *cmd, const struct logical_volume *lv, struct lvinfo *info,
	    int with_open_count, int with_read_ahead);
int lv_info_by_lvid(struct cmd_context *cmd, const char *lvid_s,
		    struct lvinfo *info, int with_open_count, int with_read_ahead);

/*
 * Returns 1 if activate_lv has been set: 1 = activate; 0 = don't.
 */
int lv_activation_filter(struct cmd_context *cmd, const char *lvid_s,
			 int *activate_lv);

/*
 * Returns 1 if percent has been set, else 0.
 */
int lv_snapshot_percent(const struct logical_volume *lv, float *percent,
			percent_range_t *percent_range);
int lv_mirror_percent(struct cmd_context *cmd, struct logical_volume *lv,
		      int wait, float *percent, percent_range_t *percent_range,
		      uint32_t *event_nr);

/*
 * Return number of LVs in the VG that are active.
 */
int lvs_in_vg_activated(struct volume_group *vg);
int lvs_in_vg_activated_by_uuid_only(struct volume_group *vg);
int lvs_in_vg_opened(const struct volume_group *vg);

int lv_is_active(struct logical_volume *lv);

int monitor_dev_for_events(struct cmd_context *cmd,
			    struct logical_volume *lv, int do_reg);

/*
 * Returns 1 if PV has a dependency tree that uses anything in VG.
 */
int pv_uses_vg(struct physical_volume *pv,
	       struct volume_group *vg);

/*
 * Returns 1 if mapped device is not suspended.
 */
int device_is_usable(dev_t dev);

#endif
