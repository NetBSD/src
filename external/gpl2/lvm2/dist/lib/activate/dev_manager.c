/*	$NetBSD: dev_manager.c,v 1.1.1.1.2.1 2009/05/13 18:52:42 jym Exp $	*/

/*
 * Copyright (C) 2002-2004 Sistina Software, Inc. All rights reserved.
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

#include "lib.h"
#include "str_list.h"
#include "dev_manager.h"
#include "lvm-string.h"
#include "fs.h"
#include "defaults.h"
#include "segtype.h"
#include "display.h"
#include "toolcontext.h"
#include "targets.h"
#include "config.h"
#include "filter.h"
#include "activate.h"

#include <limits.h>
#include <dirent.h>

#define MAX_TARGET_PARAMSIZE 50000
#define UUID_PREFIX "LVM-"

typedef enum {
	PRELOAD,
	ACTIVATE,
	DEACTIVATE,
	SUSPEND,
	SUSPEND_WITH_LOCKFS,
	CLEAN
} action_t;

struct dev_manager {
	struct dm_pool *mem;

	struct cmd_context *cmd;

	void *target_state;
	uint32_t pvmove_mirror_count;

	char *vg_name;
};

struct lv_layer {
	struct logical_volume *lv;
	const char *old_name;
};

static char *_build_dlid(struct dm_pool *mem, const char *lvid, const char *layer)
{
	char *dlid;
	size_t len;

	if (!layer)
		layer = "";

	len = sizeof(UUID_PREFIX) + sizeof(union lvid) + strlen(layer);

	if (!(dlid = dm_pool_alloc(mem, len))) {
		log_error("_build_dlid: pool allocation failed for %" PRIsize_t
			  " %s %s.", len, lvid, layer);
		return NULL;
	}

	sprintf(dlid, UUID_PREFIX "%s%s%s", lvid, (*layer) ? "-" : "", layer);

	return dlid;
}

char *build_dlid(struct dev_manager *dm, const char *lvid, const char *layer)
{
	return _build_dlid(dm->mem, lvid, layer);
}

static int _read_only_lv(struct logical_volume *lv)
{
	return (!(lv->vg->status & LVM_WRITE) || !(lv->status & LVM_WRITE));
}

/*
 * Low level device-layer operations.
 */
static struct dm_task *_setup_task(const char *name, const char *uuid,
				   uint32_t *event_nr, int task,
				   uint32_t major, uint32_t minor)
{
	struct dm_task *dmt;

	if (!(dmt = dm_task_create(task)))
		return_NULL;

	if (name)
		dm_task_set_name(dmt, name);

	if (uuid && *uuid)
		dm_task_set_uuid(dmt, uuid);

	if (event_nr)
		dm_task_set_event_nr(dmt, *event_nr);

	if (major) {
		dm_task_set_major(dmt, major);
		dm_task_set_minor(dmt, minor);
	}

	return dmt;
}

static int _info_run(const char *name, const char *dlid, struct dm_info *info,
		     uint32_t *read_ahead, int mknodes, int with_open_count,
		     int with_read_ahead, uint32_t major, uint32_t minor)
{
	int r = 0;
	struct dm_task *dmt;
	int dmtask;

	dmtask = mknodes ? DM_DEVICE_MKNODES : DM_DEVICE_INFO;

	if (!(dmt = _setup_task(name, dlid, 0, dmtask, major, minor)))
		return_0;

	if (!with_open_count)
		if (!dm_task_no_open_count(dmt))
			log_error("Failed to disable open_count");

	if (!dm_task_run(dmt))
		goto_out;

	if (!dm_task_get_info(dmt, info))
		goto_out;

	if (with_read_ahead) {
		if (!dm_task_get_read_ahead(dmt, read_ahead))
			goto_out;
	} else if (read_ahead)
		*read_ahead = DM_READ_AHEAD_NONE;

	r = 1;

      out:
	dm_task_destroy(dmt);
	return r;
}

int device_is_usable(dev_t dev)
{
	struct dm_task *dmt;
	struct dm_info info;
	const char *name;
	uint64_t start, length;
	char *target_type = NULL;
	char *params;
	void *next = NULL;
	int r = 0;

	if (!(dmt = dm_task_create(DM_DEVICE_STATUS))) {
		log_error("Failed to allocate dm_task struct to check dev status");
		return 0;
	}

	if (!dm_task_set_major(dmt, MAJOR(dev)) || !dm_task_set_minor(dmt, MINOR(dev)))
		goto_out;

	if (!dm_task_run(dmt)) {
		log_error("Failed to get state of mapped device");
		goto out;
	}

	if (!dm_task_get_info(dmt, &info))
		goto_out;

	if (!info.exists || info.suspended)
		goto out;

	name = dm_task_get_name(dmt);

	/* FIXME Also check for mirror block_on_error and mpath no paths */
	/* For now, we exclude all mirrors */

	do {
		next = dm_get_next_target(dmt, next, &start, &length,
					  &target_type, &params);
		/* Skip if target type doesn't match */
		if (target_type && !strcmp(target_type, "mirror"))
			goto out;
	} while (next);

	/* FIXME Also check dependencies? */

	r = 1;

      out:
	dm_task_destroy(dmt);
	return r;
}

static int _info(const char *name, const char *dlid, int mknodes,
		 int with_open_count, int with_read_ahead,
		 struct dm_info *info, uint32_t *read_ahead)
{
	if (!mknodes && dlid && *dlid) {
		if (_info_run(NULL, dlid, info, read_ahead, 0, with_open_count,
			      with_read_ahead, 0, 0) &&
	    	    info->exists)
			return 1;
		else if (_info_run(NULL, dlid + sizeof(UUID_PREFIX) - 1, info,
				   read_ahead, 0, with_open_count,
				   with_read_ahead, 0, 0) &&
			 info->exists)
			return 1;
	}

	if (name)
		return _info_run(name, NULL, info, read_ahead, mknodes,
				 with_open_count, with_read_ahead, 0, 0);

	return 0;
}

static int _info_by_dev(uint32_t major, uint32_t minor, struct dm_info *info)
{
	return _info_run(NULL, NULL, info, NULL, 0, 0, 0, major, minor);
}

int dev_manager_info(struct dm_pool *mem, const char *name,
		     const struct logical_volume *lv, int with_mknodes,
		     int with_open_count, int with_read_ahead,
		     struct dm_info *info, uint32_t *read_ahead)
{
	const char *dlid;

	if (!(dlid = _build_dlid(mem, lv->lvid.s, NULL))) {
		log_error("dlid build failed for %s", lv->name);
		return 0;
	}

	return _info(name, dlid, with_mknodes, with_open_count, with_read_ahead,
		     info, read_ahead);
}

/* FIXME Interface must cope with multiple targets */
static int _status_run(const char *name, const char *uuid,
		       unsigned long long *s, unsigned long long *l,
		       char **t, uint32_t t_size, char **p, uint32_t p_size)
{
	int r = 0;
	struct dm_task *dmt;
	struct dm_info info;
	void *next = NULL;
	uint64_t start, length;
	char *type = NULL;
	char *params = NULL;

	if (!(dmt = _setup_task(name, uuid, 0, DM_DEVICE_STATUS, 0, 0)))
		return_0;

	if (!dm_task_no_open_count(dmt))
		log_error("Failed to disable open_count");

	if (!dm_task_run(dmt))
		goto_out;

	if (!dm_task_get_info(dmt, &info) || !info.exists)
		goto_out;

	do {
		next = dm_get_next_target(dmt, next, &start, &length,
					  &type, &params);
		if (type) {
			*s = start;
			*l = length;
			/* Make sure things are null terminated */
			strncpy(*t, type, t_size);
			(*t)[t_size - 1] = '\0';
			strncpy(*p, params, p_size);
			(*p)[p_size - 1] = '\0';

			r = 1;
			/* FIXME Cope with multiple targets! */
			break;
		}

	} while (next);

      out:
	dm_task_destroy(dmt);
	return r;
}

static int _status(const char *name, const char *uuid,
		   unsigned long long *start, unsigned long long *length,
		   char **type, uint32_t type_size, char **params,
		   uint32_t param_size) __attribute__ ((unused));

static int _status(const char *name, const char *uuid,
		   unsigned long long *start, unsigned long long *length,
		   char **type, uint32_t type_size, char **params,
		   uint32_t param_size)
{
	if (uuid && *uuid) {
		if (_status_run(NULL, uuid, start, length, type,
				type_size, params, param_size) &&
		    *params)
			return 1;
		else if (_status_run(NULL, uuid + sizeof(UUID_PREFIX) - 1, start,
				     length, type, type_size, params,
				     param_size) &&
			 *params)
			return 1;
	}

	if (name && _status_run(name, NULL, start, length, type, type_size,
				params, param_size))
		return 1;

	return 0;
}

static int _percent_run(struct dev_manager *dm, const char *name,
			const char *dlid,
			const char *target_type, int wait,
			struct logical_volume *lv, float *percent,
			uint32_t *event_nr)
{
	int r = 0;
	struct dm_task *dmt;
	struct dm_info info;
	void *next = NULL;
	uint64_t start, length;
	char *type = NULL;
	char *params = NULL;
	struct dm_list *segh = &lv->segments;
	struct lv_segment *seg = NULL;
	struct segment_type *segtype;

	uint64_t total_numerator = 0, total_denominator = 0;

	*percent = -1;

	if (!(dmt = _setup_task(name, dlid, event_nr,
				wait ? DM_DEVICE_WAITEVENT : DM_DEVICE_STATUS, 0, 0)))
		return_0;

	if (!dm_task_no_open_count(dmt))
		log_error("Failed to disable open_count");

	if (!dm_task_run(dmt))
		goto_out;

	if (!dm_task_get_info(dmt, &info) || !info.exists)
		goto_out;

	if (event_nr)
		*event_nr = info.event_nr;

	do {
		next = dm_get_next_target(dmt, next, &start, &length, &type,
					  &params);
		if (lv) {
			if (!(segh = dm_list_next(&lv->segments, segh))) {
				log_error("Number of segments in active LV %s "
					  "does not match metadata", lv->name);
				goto out;
			}
			seg = dm_list_item(segh, struct lv_segment);
		}

		if (!type || !params || strcmp(type, target_type))
			continue;

		if (!(segtype = get_segtype_from_string(dm->cmd, type)))
			continue;

		if (segtype->ops->target_percent &&
		    !segtype->ops->target_percent(&dm->target_state, dm->mem,
						  dm->cmd, seg, params,
						  &total_numerator,
						  &total_denominator))
			goto_out;

	} while (next);

	if (lv && (segh = dm_list_next(&lv->segments, segh))) {
		log_error("Number of segments in active LV %s does not "
			  "match metadata", lv->name);
		goto out;
	}

	if (total_denominator)
		*percent = (float) total_numerator *100 / total_denominator;
	else
		*percent = 100;

	log_debug("LV percent: %f", *percent);
	r = 1;

      out:
	dm_task_destroy(dmt);
	return r;
}

static int _percent(struct dev_manager *dm, const char *name, const char *dlid,
		    const char *target_type, int wait,
		    struct logical_volume *lv, float *percent,
		    uint32_t *event_nr)
{
	if (dlid && *dlid) {
		if (_percent_run(dm, NULL, dlid, target_type, wait, lv, percent,
				 event_nr))
			return 1;
		else if (_percent_run(dm, NULL, dlid + sizeof(UUID_PREFIX) - 1,
				      target_type, wait, lv, percent,
				      event_nr))
			return 1;
	}

	if (name && _percent_run(dm, name, NULL, target_type, wait, lv, percent,
				 event_nr))
		return 1;

	return 0;
}

/*
 * dev_manager implementation.
 */
struct dev_manager *dev_manager_create(struct cmd_context *cmd,
				       const char *vg_name)
{
	struct dm_pool *mem;
	struct dev_manager *dm;

	if (!(mem = dm_pool_create("dev_manager", 16 * 1024)))
		return_NULL;

	if (!(dm = dm_pool_alloc(mem, sizeof(*dm))))
		goto_bad;

	dm->cmd = cmd;
	dm->mem = mem;

	if (!(dm->vg_name = dm_pool_strdup(dm->mem, vg_name)))
		goto_bad;

	dm->target_state = NULL;

	return dm;

      bad:
	dm_pool_destroy(mem);
	return NULL;
}

void dev_manager_destroy(struct dev_manager *dm)
{
	dm_pool_destroy(dm->mem);
}

void dev_manager_release(void)
{
	dm_lib_release();
}

void dev_manager_exit(void)
{
	dm_lib_exit();
}

int dev_manager_snapshot_percent(struct dev_manager *dm,
				 const struct logical_volume *lv,
				 float *percent)
{
	char *name;
	const char *dlid;

	/*
	 * Build a name for the top layer.
	 */
	if (!(name = build_dm_name(dm->mem, lv->vg->name, lv->name, NULL)))
		return_0;

	if (!(dlid = build_dlid(dm, lv->lvid.s, NULL)))
		return_0;

	/*
	 * Try and get some info on this device.
	 */
	log_debug("Getting device status percentage for %s", name);
	if (!(_percent(dm, name, dlid, "snapshot", 0, NULL, percent,
		       NULL)))
		return_0;

	/* FIXME dm_pool_free ? */

	/* If the snapshot isn't available, percent will be -1 */
	return 1;
}

/* FIXME Merge with snapshot_percent, auto-detecting target type */
/* FIXME Cope with more than one target */
int dev_manager_mirror_percent(struct dev_manager *dm,
			       struct logical_volume *lv, int wait,
			       float *percent, uint32_t *event_nr)
{
	char *name;
	const char *dlid;

	/*
	 * Build a name for the top layer.
	 */
	if (!(name = build_dm_name(dm->mem, lv->vg->name, lv->name, NULL)))
		return_0;

	/* FIXME dm_pool_free ? */

	if (!(dlid = build_dlid(dm, lv->lvid.s, NULL))) {
		log_error("dlid build failed for %s", lv->name);
		return 0;
	}

	log_debug("Getting device mirror status percentage for %s", name);
	if (!(_percent(dm, name, dlid, "mirror", wait, lv, percent,
		       event_nr)))
		return_0;

	return 1;
}

#if 0
	log_very_verbose("%s %s", sus ? "Suspending" : "Resuming", name);

	log_verbose("Loading %s", dl->name);
			log_very_verbose("Activating %s read-only", dl->name);
	log_very_verbose("Activated %s %s %03u:%03u", dl->name,
			 dl->dlid, dl->info.major, dl->info.minor);

	if (_get_flag(dl, VISIBLE))
		log_verbose("Removing %s", dl->name);
	else
		log_very_verbose("Removing %s", dl->name);

	log_debug("Adding target: %" PRIu64 " %" PRIu64 " %s %s",
		  extent_size * seg->le, extent_size * seg->len, target, params);

	log_debug("Adding target: 0 %" PRIu64 " snapshot-origin %s",
		  dl->lv->size, params);
	log_debug("Adding target: 0 %" PRIu64 " snapshot %s", size, params);
	log_debug("Getting device info for %s", dl->name);

	/* Rename? */
		if ((suffix = strrchr(dl->dlid + sizeof(UUID_PREFIX) - 1, '-')))
			suffix++;
		new_name = build_dm_name(dm->mem, dm->vg_name, dl->lv->name,
					suffix);

static int _belong_to_vg(const char *vgname, const char *name)
{
	const char *v = vgname, *n = name;

	while (*v) {
		if ((*v != *n) || (*v == '-' && *(++n) != '-'))
			return 0;
		v++, n++;
	}

	if (*n == '-' && *(n + 1) != '-')
		return 1;
	else
		return 0;
}

	if (!(snap_seg = find_cow(lv)))
		return 1;

	old_origin = snap_seg->origin;

	/* Was this the last active snapshot with this origin? */
	dm_list_iterate_items(lvl, active_head) {
		active = lvl->lv;
		if ((snap_seg = find_cow(active)) &&
		    snap_seg->origin == old_origin) {
			return 1;
		}
	}

#endif

/*************************/
/*  NEW CODE STARTS HERE */
/*************************/

int dev_manager_lv_mknodes(const struct logical_volume *lv)
{
	char *name;

	if (!(name = build_dm_name(lv->vg->cmd->mem, lv->vg->name,
				   lv->name, NULL)))
		return_0;

	return fs_add_lv(lv, name);
}

int dev_manager_lv_rmnodes(const struct logical_volume *lv)
{
	return fs_del_lv(lv);
}

static int _add_dev_to_dtree(struct dev_manager *dm, struct dm_tree *dtree,
			       struct logical_volume *lv, const char *layer)
{
	char *dlid, *name;
	struct dm_info info, info2;

	if (!(name = build_dm_name(dm->mem, lv->vg->name, lv->name, layer)))
		return_0;

	if (!(dlid = build_dlid(dm, lv->lvid.s, layer)))
		return_0;

	log_debug("Getting device info for %s [%s]", name, dlid);
	if (!_info(name, dlid, 0, 1, 0, &info, NULL)) {
		log_error("Failed to get info for %s [%s].", name, dlid);
		return 0;
	}

	/*
	 * For top level volumes verify that existing device match
	 * requested major/minor and that major/minor pair is available for use
	 */
	if (!layer && lv->major != -1 && lv->minor != -1) {
		if (info.exists && (info.major != lv->major || info.minor != lv->minor)) {
			log_error("Volume %s (%" PRIu32 ":%" PRIu32")"
				  " differs from already active device "
				  "(%" PRIu32 ":%" PRIu32")",
				  lv->name, lv->major, lv->minor, info.major, info.minor);
			return 0;
		}
		if (!info.exists && _info_by_dev(lv->major, lv->minor, &info2) &&
		    info2.exists) {
			log_error("The requested major:minor pair "
				  "(%" PRIu32 ":%" PRIu32") is already used",
				  lv->major, lv->minor);
			return 0;
		}
	}

	if (info.exists && !dm_tree_add_dev(dtree, info.major, info.minor)) {
		log_error("Failed to add device (%" PRIu32 ":%" PRIu32") to dtree",
			  info.major, info.minor);
		return 0;
	}

	return 1;
}

/*
 * Add LV and any known dependencies
 */
static int _add_lv_to_dtree(struct dev_manager *dm, struct dm_tree *dtree, struct logical_volume *lv)
{
	if (!_add_dev_to_dtree(dm, dtree, lv, NULL))
		return_0;

	/* FIXME Can we avoid doing this every time? */
	if (!_add_dev_to_dtree(dm, dtree, lv, "real"))
		return_0;

	if (!_add_dev_to_dtree(dm, dtree, lv, "cow"))
		return_0;

	if (!_add_dev_to_dtree(dm, dtree, lv, "_mlog"))
		return_0;

	return 1;
}

static struct dm_tree *_create_partial_dtree(struct dev_manager *dm, struct logical_volume *lv)
{
	struct dm_tree *dtree;
	struct dm_list *snh, *snht;
	struct lv_segment *seg;
	uint32_t s;

	if (!(dtree = dm_tree_create())) {
		log_error("Partial dtree creation failed for %s.", lv->name);
		return NULL;
	}

	if (!_add_lv_to_dtree(dm, dtree, lv))
		goto_bad;

	/* Add any snapshots of this LV */
	dm_list_iterate_safe(snh, snht, &lv->snapshot_segs)
		if (!_add_lv_to_dtree(dm, dtree, dm_list_struct_base(snh, struct lv_segment, origin_list)->cow))
			goto_bad;

	/* Add any LVs used by segments in this LV */
	dm_list_iterate_items(seg, &lv->segments)
		for (s = 0; s < seg->area_count; s++)
			if (seg_type(seg, s) == AREA_LV && seg_lv(seg, s)) {
				if (!_add_lv_to_dtree(dm, dtree, seg_lv(seg, s)))
					goto_bad;
			}

	return dtree;

bad:
	dm_tree_free(dtree);
	return NULL;
}

static char *_add_error_device(struct dev_manager *dm, struct dm_tree *dtree,
			       struct lv_segment *seg, int s)
{
	char *id, *name;
	char errid[32];
	struct dm_tree_node *node;
	struct lv_segment *seg_i;
	int segno = -1, i = 0;;
	uint64_t size = seg->len * seg->lv->vg->extent_size;

	dm_list_iterate_items(seg_i, &seg->lv->segments) {
		if (seg == seg_i)
			segno = i;
		++i;
	}

	if (segno < 0) {
		log_error("_add_error_device called with bad segment");
		return_NULL;
	}

	sprintf(errid, "missing_%d_%d", segno, s);

	if (!(id = build_dlid(dm, seg->lv->lvid.s, errid))) 
		return_NULL;

	if (!(name = build_dm_name(dm->mem, seg->lv->vg->name,
				   seg->lv->name, errid)))
		return_NULL;
	if (!(node = dm_tree_add_new_dev(dtree, name, id, 0, 0, 0, 0, 0)))
		return_NULL;
	if (!dm_tree_node_add_error_target(node, size))
		return_NULL;

	return id;
}

static int _add_error_area(struct dev_manager *dm, struct dm_tree_node *node,
			   struct lv_segment *seg, int s)
{
	char *dlid;
	uint64_t extent_size = seg->lv->vg->extent_size;

	if (!strcmp(dm->cmd->stripe_filler, "error")) {
		/*
		 * FIXME, the tree pointer is first field of dm_tree_node, but
		 * we don't have the struct definition available.
		 */
		struct dm_tree **tree = (struct dm_tree **) node;
		dlid = _add_error_device(dm, *tree, seg, s);
		if (!dlid)
			return_0;
		dm_tree_node_add_target_area(node, NULL, dlid,
					     extent_size * seg_le(seg, s));
	} else
		dm_tree_node_add_target_area(node,
					     dm->cmd->stripe_filler,
					     NULL, UINT64_C(0));

	return 1;
}

int add_areas_line(struct dev_manager *dm, struct lv_segment *seg,
		   struct dm_tree_node *node, uint32_t start_area,
		   uint32_t areas)
{
	uint64_t extent_size = seg->lv->vg->extent_size;
	uint32_t s;
	char *dlid;

	for (s = start_area; s < areas; s++) {
		if ((seg_type(seg, s) == AREA_PV &&
		     (!seg_pvseg(seg, s) ||
		      !seg_pv(seg, s) ||
		      !seg_dev(seg, s))) ||
		    (seg_type(seg, s) == AREA_LV && !seg_lv(seg, s))) {
			if (!_add_error_area(dm, node, seg, s))
				return_0;
		} else if (seg_type(seg, s) == AREA_PV)
			dm_tree_node_add_target_area(node,
							dev_name(seg_dev(seg, s)),
							NULL,
							(seg_pv(seg, s)->pe_start +
							 (extent_size * seg_pe(seg, s))));
		else if (seg_type(seg, s) == AREA_LV) {
			if (!(dlid = build_dlid(dm,
						 seg_lv(seg, s)->lvid.s,
						 NULL)))
				return_0;
			dm_tree_node_add_target_area(node, NULL, dlid,
							extent_size * seg_le(seg, s));
		} else {
			log_error("Internal error: Unassigned area found in LV %s.",
				  seg->lv->name);
			return 0;
		}
	}

	return 1;
}

static int _add_origin_target_to_dtree(struct dev_manager *dm,
					 struct dm_tree_node *dnode,
					 struct logical_volume *lv)
{
	const char *real_dlid;

	if (!(real_dlid = build_dlid(dm, lv->lvid.s, "real")))
		return_0;

	if (!dm_tree_node_add_snapshot_origin_target(dnode, lv->size, real_dlid))
		return_0;

	return 1;
}

static int _add_snapshot_target_to_dtree(struct dev_manager *dm,
					   struct dm_tree_node *dnode,
					   struct logical_volume *lv)
{
	const char *origin_dlid;
	const char *cow_dlid;
	struct lv_segment *snap_seg;
	uint64_t size;

	if (!(snap_seg = find_cow(lv))) {
		log_error("Couldn't find snapshot for '%s'.", lv->name);
		return 0;
	}

	if (!(origin_dlid = build_dlid(dm, snap_seg->origin->lvid.s, "real")))
		return_0;

	if (!(cow_dlid = build_dlid(dm, snap_seg->cow->lvid.s, "cow")))
		return_0;

	size = (uint64_t) snap_seg->len * snap_seg->origin->vg->extent_size;

	if (!dm_tree_node_add_snapshot_target(dnode, size, origin_dlid, cow_dlid, 1, snap_seg->chunk_size))
		return_0;

	return 1;
}

static int _add_target_to_dtree(struct dev_manager *dm,
				  struct dm_tree_node *dnode,
				  struct lv_segment *seg)
{
	uint64_t extent_size = seg->lv->vg->extent_size;

	if (!seg->segtype->ops->add_target_line) {
		log_error("_emit_target: Internal error: Can't handle "
			  "segment type %s", seg->segtype->name);
		return 0;
	}

	return seg->segtype->ops->add_target_line(dm, dm->mem, dm->cmd,
						  &dm->target_state, seg,
						  dnode,
						  extent_size * seg->len,
						  &dm-> pvmove_mirror_count);
}

static int _add_new_lv_to_dtree(struct dev_manager *dm, struct dm_tree *dtree,
				  struct logical_volume *lv, const char *layer);

static int _add_segment_to_dtree(struct dev_manager *dm,
				   struct dm_tree *dtree,
				   struct dm_tree_node *dnode,
				   struct lv_segment *seg,
				   const char *layer)
{
	uint32_t s;
	struct dm_list *snh;
	struct lv_segment *seg_present;

	/* Ensure required device-mapper targets are loaded */
	seg_present = find_cow(seg->lv) ? : seg;

	log_debug("Checking kernel supports %s segment type for %s%s%s",
		  seg_present->segtype->name, seg->lv->name,
		  layer ? "-" : "", layer ? : "");

	if (seg_present->segtype->ops->target_present &&
	    !seg_present->segtype->ops->target_present(seg_present, NULL)) {
		log_error("Can't expand LV %s: %s target support missing "
			  "from kernel?", seg->lv->name, seg_present->segtype->name);
		return 0;
	}

	/* Add mirror log */
	if (seg->log_lv &&
	    !_add_new_lv_to_dtree(dm, dtree, seg->log_lv, NULL))
		return_0;

	/* If this is a snapshot origin, add real LV */
	if (lv_is_origin(seg->lv) && !layer) {
		if (vg_is_clustered(seg->lv->vg)) {
			log_error("Clustered snapshots are not yet supported");
			return 0;
		}
		if (!_add_new_lv_to_dtree(dm, dtree, seg->lv, "real"))
			return_0;
	} else if (lv_is_cow(seg->lv) && !layer) {
		if (!_add_new_lv_to_dtree(dm, dtree, seg->lv, "cow"))
			return_0;
	} else {
		/* Add any LVs used by this segment */
		for (s = 0; s < seg->area_count; s++)
			if ((seg_type(seg, s) == AREA_LV) &&
			    (!_add_new_lv_to_dtree(dm, dtree, seg_lv(seg, s), NULL)))
				return_0;
	}

	/* Now we've added its dependencies, we can add the target itself */
	if (lv_is_origin(seg->lv) && !layer) {
		if (!_add_origin_target_to_dtree(dm, dnode, seg->lv))
			return_0;
	} else if (lv_is_cow(seg->lv) && !layer) {
		if (!_add_snapshot_target_to_dtree(dm, dnode, seg->lv))
			return_0;
	} else if (!_add_target_to_dtree(dm, dnode, seg))
		return_0;

	if (lv_is_origin(seg->lv) && !layer)
		/* Add any snapshots of this LV */
		dm_list_iterate(snh, &seg->lv->snapshot_segs)
			if (!_add_new_lv_to_dtree(dm, dtree, dm_list_struct_base(snh, struct lv_segment, origin_list)->cow, NULL))
				return_0;

	return 1;
}

static int _add_new_lv_to_dtree(struct dev_manager *dm, struct dm_tree *dtree,
				  struct logical_volume *lv, const char *layer)
{
	struct lv_segment *seg;
	struct lv_layer *lvlayer;
	struct dm_tree_node *dnode;
	char *name, *dlid;
	uint32_t max_stripe_size = UINT32_C(0);
	uint32_t read_ahead = lv->read_ahead;
	uint32_t read_ahead_flags = UINT32_C(0);

	if (!(name = build_dm_name(dm->mem, lv->vg->name, lv->name, layer)))
		return_0;

	if (!(dlid = build_dlid(dm, lv->lvid.s, layer)))
		return_0;

	/* We've already processed this node if it already has a context ptr */
	if ((dnode = dm_tree_find_node_by_uuid(dtree, dlid)) &&
	    dm_tree_node_get_context(dnode))
		return 1;

	if (!(lvlayer = dm_pool_alloc(dm->mem, sizeof(*lvlayer)))) {
		log_error("_add_new_lv_to_dtree: pool alloc failed for %s %s.", lv->name, layer);
		return 0;
	}

	lvlayer->lv = lv;

	/*
	 * Add LV to dtree.
	 * If we're working with precommitted metadata, clear any
	 * existing inactive table left behind.
	 * Major/minor settings only apply to the visible layer.
	 */
	if (!(dnode = dm_tree_add_new_dev(dtree, name, dlid,
					     layer ? UINT32_C(0) : (uint32_t) lv->major,
					     layer ? UINT32_C(0) : (uint32_t) lv->minor,
					     _read_only_lv(lv),
					     (lv->vg->status & PRECOMMITTED) ? 1 : 0,
					     lvlayer)))
		return_0;

	/* Store existing name so we can do rename later */
	lvlayer->old_name = dm_tree_node_get_name(dnode);

	/* Create table */
	dm->pvmove_mirror_count = 0u;
	dm_list_iterate_items(seg, &lv->segments) {
		if (!_add_segment_to_dtree(dm, dtree, dnode, seg, layer))
			return_0;
		/* These aren't real segments in the LVM2 metadata */
		if (lv_is_origin(lv) && !layer)
			break;
		if (lv_is_cow(lv) && !layer)
			break;
		if (max_stripe_size < seg->stripe_size * seg->area_count)
			max_stripe_size = seg->stripe_size * seg->area_count;
	}

	if (read_ahead == DM_READ_AHEAD_AUTO) {
		/* we need RA at least twice a whole stripe - see the comment in md/raid0.c */
		read_ahead = max_stripe_size * 2;
		read_ahead_flags = DM_READ_AHEAD_MINIMUM_FLAG;
	}

	dm_tree_node_set_read_ahead(dnode, read_ahead, read_ahead_flags);

	return 1;
}

/* FIXME: symlinks should be created/destroyed at the same time
 * as the kernel devices but we can't do that from within libdevmapper
 * at present so we must walk the tree twice instead. */

/*
 * Create LV symlinks for children of supplied root node.
 */
static int _create_lv_symlinks(struct dev_manager *dm, struct dm_tree_node *root)
{
	void *handle = NULL;
	struct dm_tree_node *child;
	struct lv_layer *lvlayer;
	char *old_vgname, *old_lvname, *old_layer;
	char *new_vgname, *new_lvname, *new_layer;
	const char *name;
	int r = 1;

	while ((child = dm_tree_next_child(&handle, root, 0))) {
		if (!(lvlayer = (struct lv_layer *) dm_tree_node_get_context(child)))
			continue;

		/* Detect rename */
		name = dm_tree_node_get_name(child);

		if (name && lvlayer->old_name && *lvlayer->old_name && strcmp(name, lvlayer->old_name)) {
			if (!dm_split_lvm_name(dm->mem, lvlayer->old_name, &old_vgname, &old_lvname, &old_layer)) {
				log_error("_create_lv_symlinks: Couldn't split up old device name %s", lvlayer->old_name);
				return 0;
			}
			if (!dm_split_lvm_name(dm->mem, name, &new_vgname, &new_lvname, &new_layer)) {
				log_error("_create_lv_symlinks: Couldn't split up new device name %s", name);
				return 0;
			}
			if (!fs_rename_lv(lvlayer->lv, name, old_vgname, old_lvname))
				r = 0;
		} else if (!dev_manager_lv_mknodes(lvlayer->lv))
			r = 0;
	}

	return r;
}

/*
 * Remove LV symlinks for children of supplied root node.
 */
static int _remove_lv_symlinks(struct dev_manager *dm, struct dm_tree_node *root)
{
	void *handle = NULL;
	struct dm_tree_node *child;
	char *vgname, *lvname, *layer;
	int r = 1;

	while ((child = dm_tree_next_child(&handle, root, 0))) {
		if (!dm_split_lvm_name(dm->mem, dm_tree_node_get_name(child), &vgname, &lvname, &layer)) {
			r = 0;
			continue;
		}

		if (!*vgname)
			continue;

		/* only top level layer has symlinks */
		if (*layer)
			continue;

		fs_del_lv_byname(dm->cmd->dev_dir, vgname, lvname);
	}

	return r;
}

static int _clean_tree(struct dev_manager *dm, struct dm_tree_node *root)
{
	void *handle = NULL;
	struct dm_tree_node *child;
	char *vgname, *lvname, *layer;
	const char *name, *uuid;

	while ((child = dm_tree_next_child(&handle, root, 0))) {
		if (!(name = dm_tree_node_get_name(child)))
			continue;

		if (!(uuid = dm_tree_node_get_uuid(child)))
			continue;

		if (!dm_split_lvm_name(dm->mem, name, &vgname, &lvname, &layer)) {
			log_error("_clean_tree: Couldn't split up device name %s.", name);
			return 0;
		}

		/* Not meant to be top level? */
		if (!*layer)
			continue;

		if (!dm_tree_deactivate_children(root, uuid, strlen(uuid)))
			return_0;
	}

	return 1;
}

static int _tree_action(struct dev_manager *dm, struct logical_volume *lv, action_t action)
{
	struct dm_tree *dtree;
	struct dm_tree_node *root;
	char *dlid;
	int r = 0;

	if (!(dtree = _create_partial_dtree(dm, lv)))
		return_0;

	if (!(root = dm_tree_find_node(dtree, 0, 0))) {
		log_error("Lost dependency tree root node");
		goto out;
	}

	if (!(dlid = build_dlid(dm, lv->lvid.s, NULL)))
		goto_out;

	/* Only process nodes with uuid of "LVM-" plus VG id. */
	switch(action) {
	case CLEAN:
		/* Deactivate any unused non-toplevel nodes */
		if (!_clean_tree(dm, root))
			goto_out;
		break;
	case DEACTIVATE:
 		/* Deactivate LV and all devices it references that nothing else has open. */
		if (!dm_tree_deactivate_children(root, dlid, ID_LEN + sizeof(UUID_PREFIX) - 1))
			goto_out;
		if (!_remove_lv_symlinks(dm, root))
			log_error("Failed to remove all device symlinks associated with %s.", lv->name);
		break;
	case SUSPEND:
		dm_tree_skip_lockfs(root);
		if ((lv->status & MIRRORED) && !(lv->status & PVMOVE))
			dm_tree_use_no_flush_suspend(root);
	case SUSPEND_WITH_LOCKFS:
		if (!dm_tree_suspend_children(root, dlid, ID_LEN + sizeof(UUID_PREFIX) - 1))
			goto_out;
		break;
	case PRELOAD:
	case ACTIVATE:
		/* Add all required new devices to tree */
		if (!_add_new_lv_to_dtree(dm, dtree, lv, NULL))
			goto_out;

		/* Preload any devices required before any suspensions */
		if (!dm_tree_preload_children(root, dlid, ID_LEN + sizeof(UUID_PREFIX) - 1))
			goto_out;

		if ((action == ACTIVATE) &&
		    !dm_tree_activate_children(root, dlid, ID_LEN + sizeof(UUID_PREFIX) - 1))
			goto_out;

		if (!_create_lv_symlinks(dm, root)) {
			log_error("Failed to create symlinks for %s.", lv->name);
			goto out;
		}
		break;
	default:
		log_error("_tree_action: Action %u not supported.", action);
		goto out;
	}	

	r = 1;

out:
	dm_tree_free(dtree);

	return r;
}

int dev_manager_activate(struct dev_manager *dm, struct logical_volume *lv)
{
	if (!_tree_action(dm, lv, ACTIVATE))
		return_0;

	return _tree_action(dm, lv, CLEAN);
}

int dev_manager_preload(struct dev_manager *dm, struct logical_volume *lv)
{
	/* FIXME Update the pvmove implementation! */
	if ((lv->status & PVMOVE) || (lv->status & LOCKED))
		return 1;

	return _tree_action(dm, lv, PRELOAD);
}

int dev_manager_deactivate(struct dev_manager *dm, struct logical_volume *lv)
{
	int r;

	r = _tree_action(dm, lv, DEACTIVATE);

	fs_del_lv(lv);

	return r;
}

int dev_manager_suspend(struct dev_manager *dm, struct logical_volume *lv,
			int lockfs)
{
	return _tree_action(dm, lv, lockfs ? SUSPEND_WITH_LOCKFS : SUSPEND);
}

/*
 * Does device use VG somewhere in its construction?
 * Returns 1 if uncertain.
 */
int dev_manager_device_uses_vg(struct device *dev,
			       struct volume_group *vg)
{
	struct dm_tree *dtree;
	struct dm_tree_node *root;
	char dlid[sizeof(UUID_PREFIX) + sizeof(struct id) - 1] __attribute((aligned(8)));
	int r = 1;

	if (!(dtree = dm_tree_create())) {
		log_error("partial dtree creation failed");
		return r;
	}

	if (!dm_tree_add_dev(dtree, (uint32_t) MAJOR(dev->dev), (uint32_t) MINOR(dev->dev))) {
		log_error("Failed to add device %s (%" PRIu32 ":%" PRIu32") to dtree",
			  dev_name(dev), (uint32_t) MAJOR(dev->dev), (uint32_t) MINOR(dev->dev));
		goto out;
	}

	memcpy(dlid, UUID_PREFIX, sizeof(UUID_PREFIX) - 1);
	memcpy(dlid + sizeof(UUID_PREFIX) - 1, &vg->id.uuid[0], sizeof(vg->id));

	if (!(root = dm_tree_find_node(dtree, 0, 0))) {
		log_error("Lost dependency tree root node");
		goto out;
	}

	if (dm_tree_children_use_uuid(root, dlid, sizeof(UUID_PREFIX) + sizeof(vg->id) - 1))
		goto_out;

	r = 0;

out:
	dm_tree_free(dtree);
	return r;
}
