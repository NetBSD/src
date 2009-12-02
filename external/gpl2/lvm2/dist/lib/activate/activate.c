/*	$NetBSD: activate.c,v 1.1.1.3 2009/12/02 00:26:22 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2009 Red Hat, Inc. All rights reserved.
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

#include "lib.h"
#include "metadata.h"
#include "activate.h"
#include "memlock.h"
#include "display.h"
#include "fs.h"
#include "lvm-exec.h"
#include "lvm-file.h"
#include "lvm-string.h"
#include "toolcontext.h"
#include "dev_manager.h"
#include "str_list.h"
#include "config.h"
#include "filter.h"
#include "segtype.h"

#include <limits.h>
#include <fcntl.h>
#include <unistd.h>

#define _skip(fmt, args...) log_very_verbose("Skipping: " fmt , ## args)

int lvm1_present(struct cmd_context *cmd)
{
	char path[PATH_MAX];

	if (dm_snprintf(path, sizeof(path), "%s/lvm/global", cmd->proc_dir)
	    < 0) {
		log_error("LVM1 proc global snprintf failed");
		return 0;
	}

	if (path_exists(path))
		return 1;
	else
		return 0;
}

int list_segment_modules(struct dm_pool *mem, const struct lv_segment *seg,
			 struct dm_list *modules)
{
	unsigned int s;
	struct lv_segment *seg2, *snap_seg;
	struct dm_list *snh;

	if (seg->segtype->ops->modules_needed &&
	    !seg->segtype->ops->modules_needed(mem, seg, modules)) {
		log_error("module string allocation failed");
		return 0;
	}

	if (lv_is_origin(seg->lv))
		dm_list_iterate(snh, &seg->lv->snapshot_segs)
			if (!list_lv_modules(mem,
					     dm_list_struct_base(snh,
							      struct lv_segment,
							      origin_list)->cow,
					     modules))
				return_0;

	if (lv_is_cow(seg->lv)) {
		snap_seg = find_cow(seg->lv);
		if (snap_seg->segtype->ops->modules_needed &&
		    !snap_seg->segtype->ops->modules_needed(mem, snap_seg,
							    modules)) {
			log_error("snap_seg module string allocation failed");
			return 0;
		}
	}

	for (s = 0; s < seg->area_count; s++) {
		switch (seg_type(seg, s)) {
		case AREA_LV:
			seg2 = find_seg_by_le(seg_lv(seg, s), seg_le(seg, s));
			if (seg2 && !list_segment_modules(mem, seg2, modules))
				return_0;
			break;
		case AREA_PV:
		case AREA_UNASSIGNED:
			;
		}
	}

	return 1;
}

int list_lv_modules(struct dm_pool *mem, const struct logical_volume *lv,
		    struct dm_list *modules)
{
	struct lv_segment *seg;

	dm_list_iterate_items(seg, &lv->segments)
		if (!list_segment_modules(mem, seg, modules))
			return_0;

	return 1;
}

#ifndef DEVMAPPER_SUPPORT
void set_activation(int act)
{
	static int warned = 0;

	if (warned || !act)
		return;

	log_error("Compiled without libdevmapper support. "
		  "Can't enable activation.");

	warned = 1;
}
int activation(void)
{
	return 0;
}
int library_version(char *version, size_t size)
{
	return 0;
}
int driver_version(char *version, size_t size)
{
	return 0;
}
int target_version(const char *target_name, uint32_t *maj,
		   uint32_t *min, uint32_t *patchlevel)
{
	return 0;
}
int target_present(struct cmd_context *cmd, const char *target_name,
		   int use_modprobe)
{
	return 0;
}
int lv_info(struct cmd_context *cmd, const struct logical_volume *lv, struct lvinfo *info,
	    int with_open_count, int with_read_ahead)
{
	return 0;
}
int lv_info_by_lvid(struct cmd_context *cmd, const char *lvid_s,
		    struct lvinfo *info, int with_open_count, int with_read_ahead)
{
	return 0;
}
int lv_snapshot_percent(const struct logical_volume *lv, float *percent,
			percent_range_t *percent_range)
{
	return 0;
}
int lv_mirror_percent(struct cmd_context *cmd, struct logical_volume *lv,
		      int wait, float *percent, percent_range_t *percent_range,
		      uint32_t *event_nr)
{
	return 0;
}
int lvs_in_vg_activated(struct volume_group *vg)
{
	return 0;
}
int lvs_in_vg_activated_by_uuid_only(struct volume_group *vg)
{
	return 0;
}
int lvs_in_vg_opened(struct volume_group *vg)
{
	return 0;
}
int lv_suspend(struct cmd_context *cmd, const char *lvid_s)
{
	return 1;
}
int lv_suspend_if_active(struct cmd_context *cmd, const char *lvid_s)
{
	return 1;
}
int lv_resume(struct cmd_context *cmd, const char *lvid_s)
{
	return 1;
}
int lv_resume_if_active(struct cmd_context *cmd, const char *lvid_s)
{
	return 1;
}
int lv_deactivate(struct cmd_context *cmd, const char *lvid_s)
{
	return 1;
}
int lv_activation_filter(struct cmd_context *cmd, const char *lvid_s,
			 int *activate_lv)
{
	return 1;
}
int lv_activate(struct cmd_context *cmd, const char *lvid_s, int exclusive)
{
	return 1;
}
int lv_activate_with_filter(struct cmd_context *cmd, const char *lvid_s, int exclusive)
{
	return 1;
}

int lv_mknodes(struct cmd_context *cmd, const struct logical_volume *lv)
{
	return 1;
}

int pv_uses_vg(struct physical_volume *pv,
	       struct volume_group *vg)
{
	return 0;
}

void activation_release(void)
{
	return;
}

void activation_exit(void)
{
	return;
}

#else				/* DEVMAPPER_SUPPORT */

static int _activation = 1;

void set_activation(int act)
{
	if (act == _activation)
		return;

	_activation = act;
	if (_activation)
		log_verbose("Activation enabled. Device-mapper kernel "
			    "driver will be used.");
	else
		log_warn("WARNING: Activation disabled. No device-mapper "
			  "interaction will be attempted.");
}

int activation(void)
{
	return _activation;
}

static int _passes_activation_filter(struct cmd_context *cmd,
				     struct logical_volume *lv)
{
	const struct config_node *cn;
	struct config_value *cv;
	char *str;
	char path[PATH_MAX];

	if (!(cn = find_config_tree_node(cmd, "activation/volume_list"))) {
		/* If no host tags defined, activate */
		if (dm_list_empty(&cmd->tags))
			return 1;

		/* If any host tag matches any LV or VG tag, activate */
		if (str_list_match_list(&cmd->tags, &lv->tags) ||
		    str_list_match_list(&cmd->tags, &lv->vg->tags))
			return 1;

		/* Don't activate */
		return 0;
	}

	for (cv = cn->v; cv; cv = cv->next) {
		if (cv->type != CFG_STRING) {
			log_error("Ignoring invalid string in config file "
				  "activation/volume_list");
			continue;
		}
		str = cv->v.str;
		if (!*str) {
			log_error("Ignoring empty string in config file "
				  "activation/volume_list");
			continue;
		}

		/* Tag? */
		if (*str == '@') {
			str++;
			if (!*str) {
				log_error("Ignoring empty tag in config file "
					  "activation/volume_list");
				continue;
			}
			/* If any host tag matches any LV or VG tag, activate */
			if (!strcmp(str, "*")) {
				if (str_list_match_list(&cmd->tags, &lv->tags)
				    || str_list_match_list(&cmd->tags,
							   &lv->vg->tags))
					    return 1;
				else
					continue;
			}
			/* If supplied tag matches LV or VG tag, activate */
			if (str_list_match_item(&lv->tags, str) ||
			    str_list_match_item(&lv->vg->tags, str))
				return 1;
			else
				continue;
		}
		if (!strchr(str, '/')) {
			/* vgname supplied */
			if (!strcmp(str, lv->vg->name))
				return 1;
			else
				continue;
		}
		/* vgname/lvname */
		if (dm_snprintf(path, sizeof(path), "%s/%s", lv->vg->name,
				 lv->name) < 0) {
			log_error("dm_snprintf error from %s/%s", lv->vg->name,
				  lv->name);
			continue;
		}
		if (!strcmp(path, str))
			return 1;
	}

	return 0;
}

int library_version(char *version, size_t size)
{
	if (!activation())
		return 0;

	return dm_get_library_version(version, size);
}

int driver_version(char *version, size_t size)
{
	if (!activation())
		return 0;

	log_very_verbose("Getting driver version");

	return dm_driver_version(version, size);
}

int target_version(const char *target_name, uint32_t *maj,
		   uint32_t *min, uint32_t *patchlevel)
{
	int r = 0;
	struct dm_task *dmt;
	struct dm_versions *target, *last_target;

	log_very_verbose("Getting target version for %s", target_name);
	if (!(dmt = dm_task_create(DM_DEVICE_LIST_VERSIONS)))
		return_0;

	if (!dm_task_run(dmt)) {
		log_debug("Failed to get %s target version", target_name);
		/* Assume this was because LIST_VERSIONS isn't supported */
		return 1;
	}

	target = dm_task_get_versions(dmt);

	do {
		last_target = target;

		if (!strcmp(target_name, target->name)) {
			r = 1;
			*maj = target->version[0];
			*min = target->version[1];
			*patchlevel = target->version[2];
			goto out;
		}

		target = (void *) target + target->next;
	} while (last_target != target);

      out:
	dm_task_destroy(dmt);

	return r;
}

int module_present(struct cmd_context *cmd, const char *target_name)
{
	int ret = 0;
#ifdef MODPROBE_CMD
	char module[128];
	const char *argv[3];

	if (dm_snprintf(module, sizeof(module), "dm-%s", target_name) < 0) {
		log_error("module_present module name too long: %s",
			  target_name);
		return 0;
	}

	argv[0] = MODPROBE_CMD;
	argv[1] = module;
	argv[2] = NULL;

	ret = exec_cmd(cmd, argv);
#endif
	return ret;
}

int target_present(struct cmd_context *cmd, const char *target_name,
		   int use_modprobe)
{
	uint32_t maj, min, patchlevel;

	if (!activation())
		return 0;

#ifdef MODPROBE_CMD
	if (use_modprobe) {
		if (target_version(target_name, &maj, &min, &patchlevel))
			return 1;

		if (!module_present(cmd, target_name))
			return_0;
	}
#endif

	return target_version(target_name, &maj, &min, &patchlevel);
}

/*
 * Returns 1 if info structure populated, else 0 on failure.
 */
static int _lv_info(struct cmd_context *cmd, const struct logical_volume *lv, int with_mknodes,
		    struct lvinfo *info, int with_open_count, int with_read_ahead, unsigned by_uuid_only)
{
	struct dm_info dminfo;
	char *name = NULL;

	if (!activation())
		return 0;

	if (!by_uuid_only &&
	    !(name = build_dm_name(cmd->mem, lv->vg->name, lv->name, NULL)))
		return_0;

	log_debug("Getting device info for %s", name);
	if (!dev_manager_info(lv->vg->cmd->mem, name, lv, with_mknodes,
			      with_open_count, with_read_ahead, &dminfo,
			      &info->read_ahead)) {
		if (name)
			dm_pool_free(cmd->mem, name);
		return_0;
	}

	info->exists = dminfo.exists;
	info->suspended = dminfo.suspended;
	info->open_count = dminfo.open_count;
	info->major = dminfo.major;
	info->minor = dminfo.minor;
	info->read_only = dminfo.read_only;
	info->live_table = dminfo.live_table;
	info->inactive_table = dminfo.inactive_table;

	if (name)
		dm_pool_free(cmd->mem, name);

	return 1;
}

int lv_info(struct cmd_context *cmd, const struct logical_volume *lv, struct lvinfo *info,
	    int with_open_count, int with_read_ahead)
{
	return _lv_info(cmd, lv, 0, info, with_open_count, with_read_ahead, 0);
}

int lv_info_by_lvid(struct cmd_context *cmd, const char *lvid_s,
		    struct lvinfo *info, int with_open_count, int with_read_ahead)
{
	struct logical_volume *lv;

	if (!(lv = lv_from_lvid(cmd, lvid_s, 0)))
		return 0;

	return _lv_info(cmd, lv, 0, info, with_open_count, with_read_ahead, 0);
}

/*
 * Returns 1 if percent set, else 0 on failure.
 */
int lv_snapshot_percent(const struct logical_volume *lv, float *percent,
			percent_range_t *percent_range)
{
	int r;
	struct dev_manager *dm;

	if (!activation())
		return 0;

	if (!(dm = dev_manager_create(lv->vg->cmd, lv->vg->name)))
		return_0;

	if (!(r = dev_manager_snapshot_percent(dm, lv, percent, percent_range)))
		stack;

	dev_manager_destroy(dm);

	return r;
}

/* FIXME Merge with snapshot_percent */
int lv_mirror_percent(struct cmd_context *cmd, struct logical_volume *lv,
		      int wait, float *percent, percent_range_t *percent_range,
		      uint32_t *event_nr)
{
	int r;
	struct dev_manager *dm;
	struct lvinfo info;

	/* If mirrored LV is temporarily shrinked to 1 area (= linear),
	 * it should be considered in-sync. */
	if (dm_list_size(&lv->segments) == 1 && first_seg(lv)->area_count == 1) {
		*percent = 100.0;
		return 1;
	}

	if (!activation())
		return 0;

	if (!lv_info(cmd, lv, &info, 0, 0))
		return_0;

	if (!info.exists)
		return 0;

	if (!(dm = dev_manager_create(lv->vg->cmd, lv->vg->name)))
		return_0;

	if (!(r = dev_manager_mirror_percent(dm, lv, wait, percent,
					     percent_range, event_nr)))
		stack;

	dev_manager_destroy(dm);

	return r;
}

static int _lv_active(struct cmd_context *cmd, struct logical_volume *lv,
		      unsigned by_uuid_only)
{
	struct lvinfo info;

	if (!_lv_info(cmd, lv, 0, &info, 0, 0, by_uuid_only)) {
		stack;
		return -1;
	}

	return info.exists;
}

static int _lv_open_count(struct cmd_context *cmd, struct logical_volume *lv)
{
	struct lvinfo info;

	if (!lv_info(cmd, lv, &info, 1, 0)) {
		stack;
		return -1;
	}

	return info.open_count;
}

static int _lv_activate_lv(struct logical_volume *lv)
{
	int r;
	struct dev_manager *dm;

	if (!(dm = dev_manager_create(lv->vg->cmd, lv->vg->name)))
		return_0;

	if (!(r = dev_manager_activate(dm, lv)))
		stack;

	dev_manager_destroy(dm);
	return r;
}

static int _lv_preload(struct logical_volume *lv, int *flush_required)
{
	int r;
	struct dev_manager *dm;

	if (!(dm = dev_manager_create(lv->vg->cmd, lv->vg->name)))
		return_0;

	if (!(r = dev_manager_preload(dm, lv, flush_required)))
		stack;

	dev_manager_destroy(dm);
	return r;
}

static int _lv_deactivate(struct logical_volume *lv)
{
	int r;
	struct dev_manager *dm;

	if (!(dm = dev_manager_create(lv->vg->cmd, lv->vg->name)))
		return_0;

	if (!(r = dev_manager_deactivate(dm, lv)))
		stack;

	dev_manager_destroy(dm);
	return r;
}

static int _lv_suspend_lv(struct logical_volume *lv, int lockfs, int flush_required)
{
	int r;
	struct dev_manager *dm;

	if (!(dm = dev_manager_create(lv->vg->cmd, lv->vg->name)))
		return_0;

	if (!(r = dev_manager_suspend(dm, lv, lockfs, flush_required)))
		stack;

	dev_manager_destroy(dm);
	return r;
}

/*
 * These two functions return the number of visible LVs in the state,
 * or -1 on error.
 */
static int _lvs_in_vg_activated(struct volume_group *vg, unsigned by_uuid_only)
{
	struct lv_list *lvl;
	int count = 0;

	if (!activation())
		return 0;

	dm_list_iterate_items(lvl, &vg->lvs) {
		if (lv_is_visible(lvl->lv))
			count += (_lv_active(vg->cmd, lvl->lv, by_uuid_only) == 1);
	}

	return count;
}

int lvs_in_vg_activated_by_uuid_only(struct volume_group *vg)
{
	return _lvs_in_vg_activated(vg, 1);
}

int lvs_in_vg_activated(struct volume_group *vg)
{
	return _lvs_in_vg_activated(vg, 0);
}

int lvs_in_vg_opened(const struct volume_group *vg)
{
	const struct lv_list *lvl;
	int count = 0;

	if (!activation())
		return 0;

	dm_list_iterate_items(lvl, &vg->lvs) {
		if (lv_is_visible(lvl->lv))
			count += (_lv_open_count(vg->cmd, lvl->lv) > 0);
	}

	return count;
}

/*
 * Determine whether an LV is active locally or in a cluster.
 * Assumes vg lock held.
 * Returns:
 * 0 - not active locally or on any node in cluster
 * 1 - active either locally or some node in the cluster
 */
int lv_is_active(struct logical_volume *lv)
{
	int ret;

	if (_lv_active(lv->vg->cmd, lv, 0))
		return 1;

	if (!vg_is_clustered(lv->vg))
		return 0;

	if ((ret = remote_lock_held(lv->lvid.s)) >= 0)
		return ret;

	/*
	 * Old compatibility code if locking doesn't support lock query
	 * FIXME: check status to not deactivate already activate device
	 */
	if (activate_lv_excl(lv->vg->cmd, lv)) {
		deactivate_lv(lv->vg->cmd, lv);
		return 0;
	}

	/*
	 * Exclusive local activation failed so assume it is active elsewhere.
	 */
	return 1;
}

/*
 * Returns 0 if an attempt to (un)monitor the device failed.
 * Returns 1 otherwise.
 */
int monitor_dev_for_events(struct cmd_context *cmd,
			    struct logical_volume *lv, int monitor)
{
#ifdef DMEVENTD
	int i, pending = 0, monitored;
	int r = 1;
	struct dm_list *tmp, *snh, *snht;
	struct lv_segment *seg;
	int (*monitor_fn) (struct lv_segment *s, int e);
	uint32_t s;

	/* skip dmeventd code altogether */
	if (dmeventd_monitor_mode() == DMEVENTD_MONITOR_IGNORE)
		return 1;

	/*
	 * Nothing to do if dmeventd configured not to be used.
	 */
	if (monitor && !dmeventd_monitor_mode())
		return 1;

	/*
	 * In case of a snapshot device, we monitor lv->snapshot->lv,
	 * not the actual LV itself.
	 */
	if (lv_is_cow(lv))
		return monitor_dev_for_events(cmd, lv->snapshot->lv, monitor);

	/*
	 * In case this LV is a snapshot origin, we instead monitor
	 * each of its respective snapshots (the origin itself does
	 * not need to be monitored).
	 *
	 * TODO: This may change when snapshots of mirrors are allowed.
	 */
	if (lv_is_origin(lv)) {
		dm_list_iterate_safe(snh, snht, &lv->snapshot_segs)
			if (!monitor_dev_for_events(cmd, dm_list_struct_base(snh,
				    struct lv_segment, origin_list)->cow, monitor))
				r = 0;
		return r;
	}

	dm_list_iterate(tmp, &lv->segments) {
		seg = dm_list_item(tmp, struct lv_segment);

		/* Recurse for AREA_LV */
		for (s = 0; s < seg->area_count; s++) {
			if (seg_type(seg, s) != AREA_LV)
				continue;
			if (!monitor_dev_for_events(cmd, seg_lv(seg, s),
						    monitor)) {
				log_error("Failed to %smonitor %s",
					  monitor ? "" : "un",
					  seg_lv(seg, s)->name);
				r = 0;
			}
		}

		if (!seg_monitored(seg) || (seg->status & PVMOVE))
			continue;

		monitor_fn = NULL;

		/* Check monitoring status */
		if (seg->segtype->ops->target_monitored)
			monitored = seg->segtype->ops->target_monitored(seg, &pending);
		else
			continue;  /* segtype doesn't support registration */

		/*
		 * FIXME: We should really try again if pending
		 */
		monitored = (pending) ? 0 : monitored;

		if (monitor) {
			if (monitored)
				log_verbose("%s/%s already monitored.", lv->vg->name, lv->name);
			else if (seg->segtype->ops->target_monitor_events)
				monitor_fn = seg->segtype->ops->target_monitor_events;
		} else {
			if (!monitored)
				log_verbose("%s/%s already not monitored.", lv->vg->name, lv->name);
			else if (seg->segtype->ops->target_unmonitor_events)
				monitor_fn = seg->segtype->ops->target_unmonitor_events;
		}

		/* Do [un]monitor */
		if (!monitor_fn)
			continue;

		log_verbose("%sonitoring %s/%s", monitor ? "M" : "Not m", lv->vg->name, lv->name);

		/* FIXME specify events */
		if (!monitor_fn(seg, 0)) {
			log_error("%s/%s: %s segment monitoring function failed.",
				  lv->vg->name, lv->name, seg->segtype->name);
			return 0;
		}

		/* Check [un]monitor results */
		/* Try a couple times if pending, but not forever... */
		for (i = 0; i < 10; i++) {
			pending = 0;
			monitored = seg->segtype->ops->target_monitored(seg, &pending);
			if (pending ||
			    (!monitored && monitor) ||
			    (monitored && !monitor))
				log_very_verbose("%s/%s %smonitoring still pending: waiting...",
						 lv->vg->name, lv->name, monitor ? "" : "un");
			else
				break;
			sleep(1);
		}

		r = (monitored && monitor) || (!monitored && !monitor);
	}

	return r;
#else
	return 1;
#endif
}

static int _lv_suspend(struct cmd_context *cmd, const char *lvid_s,
		       int error_if_not_suspended)
{
	struct logical_volume *lv = NULL, *lv_pre = NULL;
	struct lvinfo info;
	int r = 0, lockfs = 0, flush_required = 0;

	if (!activation())
		return 1;

	if (!(lv = lv_from_lvid(cmd, lvid_s, 0)))
		goto_out;

	/* Use precommitted metadata if present */
	if (!(lv_pre = lv_from_lvid(cmd, lvid_s, 1)))
		goto_out;

	if (test_mode()) {
		_skip("Suspending '%s'.", lv->name);
		r = 1;
		goto out;
	}

	if (!lv_info(cmd, lv, &info, 0, 0))
		goto_out;

	if (!info.exists || info.suspended) {
		r = error_if_not_suspended ? 0 : 1;
		goto out;
	}

	lv_calculate_readahead(lv, NULL);

	/* If VG was precommitted, preload devices for the LV */
	if ((lv_pre->vg->status & PRECOMMITTED)) {
		if (!_lv_preload(lv_pre, &flush_required)) {
			/* FIXME Revert preloading */
			goto_out;
		}
	}

	if (!monitor_dev_for_events(cmd, lv, 0))
		/* FIXME Consider aborting here */
		stack;

	memlock_inc();

	if (lv_is_origin(lv_pre) || lv_is_cow(lv_pre))
		lockfs = 1;

	if (!_lv_suspend_lv(lv, lockfs, flush_required)) {
		memlock_dec();
		fs_unlock();
		goto out;
	}

	r = 1;
out:
	if (lv_pre)
		vg_release(lv_pre->vg);
	if (lv)
		vg_release(lv->vg);

	return r;
}

/* Returns success if the device is not active */
int lv_suspend_if_active(struct cmd_context *cmd, const char *lvid_s)
{
	return _lv_suspend(cmd, lvid_s, 0);
}

int lv_suspend(struct cmd_context *cmd, const char *lvid_s)
{
	return _lv_suspend(cmd, lvid_s, 1);
}

static int _lv_resume(struct cmd_context *cmd, const char *lvid_s,
		      int error_if_not_active)
{
	struct logical_volume *lv;
	struct lvinfo info;
	int r = 0;

	if (!activation())
		return 1;

	if (!(lv = lv_from_lvid(cmd, lvid_s, 0)))
		goto_out;

	if (test_mode()) {
		_skip("Resuming '%s'.", lv->name);
		r = 1;
		goto out;
	}

	if (!lv_info(cmd, lv, &info, 0, 0))
		goto_out;

	if (!info.exists || !info.suspended) {
		r = error_if_not_active ? 0 : 1;
		goto_out;
	}

	if (!_lv_activate_lv(lv))
		goto_out;

	memlock_dec();
	fs_unlock();

	if (!monitor_dev_for_events(cmd, lv, 1))
		stack;

	r = 1;
out:
	if (lv)
		vg_release(lv->vg);

	return r;
}

/* Returns success if the device is not active */
int lv_resume_if_active(struct cmd_context *cmd, const char *lvid_s)
{
	return _lv_resume(cmd, lvid_s, 0);
}

int lv_resume(struct cmd_context *cmd, const char *lvid_s)
{
	return _lv_resume(cmd, lvid_s, 1);
}

static int _lv_has_open_snapshots(struct logical_volume *lv)
{
	struct lv_segment *snap_seg;
	struct lvinfo info;
	int r = 0;

	dm_list_iterate_items_gen(snap_seg, &lv->snapshot_segs, origin_list) {
		if (!lv_info(lv->vg->cmd, snap_seg->cow, &info, 1, 0)) {
			r = 1;
			continue;
		}

		if (info.exists && info.open_count) {
			log_error("LV %s/%s has open snapshot %s: "
				  "not deactivating", lv->vg->name, lv->name,
				  snap_seg->cow->name);
			r = 1;
		}
	}

	return r;
}

int lv_deactivate(struct cmd_context *cmd, const char *lvid_s)
{
	struct logical_volume *lv;
	struct lvinfo info;
	int r = 0;

	if (!activation())
		return 1;

	if (!(lv = lv_from_lvid(cmd, lvid_s, 0)))
		goto out;

	if (test_mode()) {
		_skip("Deactivating '%s'.", lv->name);
		r = 1;
		goto out;
	}

	if (!lv_info(cmd, lv, &info, 1, 0))
		goto_out;

	if (!info.exists) {
		r = 1;
		goto out;
	}

	if (lv_is_visible(lv)) {
		if (info.open_count) {
			log_error("LV %s/%s in use: not deactivating",
				  lv->vg->name, lv->name);
			goto out;
		}
		if (lv_is_origin(lv) && _lv_has_open_snapshots(lv))
			goto_out;
	}

	lv_calculate_readahead(lv, NULL);

	if (!monitor_dev_for_events(cmd, lv, 0))
		stack;

	memlock_inc();
	r = _lv_deactivate(lv);
	memlock_dec();
	fs_unlock();

	if (!lv_info(cmd, lv, &info, 1, 0) || info.exists)
		r = 0;
out:
	if (lv)
		vg_release(lv->vg);

	return r;
}

/* Test if LV passes filter */
int lv_activation_filter(struct cmd_context *cmd, const char *lvid_s,
			 int *activate_lv)
{
	struct logical_volume *lv;
	int r = 0;

	if (!activation()) {
		*activate_lv = 1;
		return 1;
	}

	if (!(lv = lv_from_lvid(cmd, lvid_s, 0)))
		goto out;

	if (!_passes_activation_filter(cmd, lv)) {
		log_verbose("Not activating %s/%s due to config file settings",
			    lv->vg->name, lv->name);
		*activate_lv = 0;
	} else
		*activate_lv = 1;
	r = 1;
out:
	if (lv)
		vg_release(lv->vg);

	return r;
}

static int _lv_activate(struct cmd_context *cmd, const char *lvid_s,
			int exclusive, int filter)
{
	struct logical_volume *lv;
	struct lvinfo info;
	int r = 0;

	if (!activation())
		return 1;

	if (!(lv = lv_from_lvid(cmd, lvid_s, 0)))
		goto out;

	if (filter && !_passes_activation_filter(cmd, lv)) {
		log_verbose("Not activating %s/%s due to config file settings",
			    lv->vg->name, lv->name);
		goto out;
	}

	if ((!lv->vg->cmd->partial_activation) && (lv->status & PARTIAL_LV)) {
		log_error("Refusing activation of partial LV %s. Use --partial to override.",
			  lv->name);
		goto_out;
	}

	if (lv_has_unknown_segments(lv)) {
		log_error("Refusing activation of LV %s containing "
			  "an unrecognised segment.", lv->name);
		goto_out;
	}

	if (test_mode()) {
		_skip("Activating '%s'.", lv->name);
		r = 1;
		goto out;
	}

	if (!lv_info(cmd, lv, &info, 0, 0))
		goto_out;

	if (info.exists && !info.suspended && info.live_table) {
		r = 1;
		goto out;
	}

	lv_calculate_readahead(lv, NULL);

	if (exclusive)
		lv->status |= ACTIVATE_EXCL;

	memlock_inc();
	if (!(r = _lv_activate_lv(lv)))
		stack;
	memlock_dec();
	fs_unlock();

	if (r && !monitor_dev_for_events(cmd, lv, 1))
		stack;

out:
	if (lv)
		vg_release(lv->vg);

	return r;
}

/* Activate LV */
int lv_activate(struct cmd_context *cmd, const char *lvid_s, int exclusive)
{
	if (!_lv_activate(cmd, lvid_s, exclusive, 0))
		return_0;

	return 1;
}

/* Activate LV only if it passes filter */
int lv_activate_with_filter(struct cmd_context *cmd, const char *lvid_s, int exclusive)
{
	if (!_lv_activate(cmd, lvid_s, exclusive, 1))
		return_0;

	return 1;
}

int lv_mknodes(struct cmd_context *cmd, const struct logical_volume *lv)
{
	struct lvinfo info;
	int r = 1;

	if (!lv) {
		r = dm_mknodes(NULL);
		fs_unlock();
		return r;
	}

	if (!_lv_info(cmd, lv, 1, &info, 0, 0, 0))
		return_0;

	if (info.exists) {
		if (lv_is_visible(lv))
			r = dev_manager_lv_mknodes(lv);
	} else
		r = dev_manager_lv_rmnodes(lv);

	fs_unlock();

	return r;
}

/*
 * Does PV use VG somewhere in its construction?
 * Returns 1 on failure.
 */
int pv_uses_vg(struct physical_volume *pv,
	       struct volume_group *vg)
{
	if (!activation())
		return 0;

	if (!dm_is_dm_major(MAJOR(pv->dev->dev)))
		return 0;

	return dev_manager_device_uses_vg(pv->dev, vg);
}

void activation_release(void)
{
	dev_manager_release();
}

void activation_exit(void)
{
	dev_manager_exit();
}
#endif
