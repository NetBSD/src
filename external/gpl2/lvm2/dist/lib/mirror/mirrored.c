/*	$NetBSD: mirrored.c,v 1.1.1.2 2009/12/02 00:26:26 haad Exp $	*/

/*
 * Copyright (C) 2003-2004 Sistina Software, Inc. All rights reserved.
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
#include "toolcontext.h"
#include "metadata.h"
#include "segtype.h"
#include "display.h"
#include "text_export.h"
#include "text_import.h"
#include "config.h"
#include "defaults.h"
#include "lvm-string.h"
#include "targets.h"
#include "activate.h"
#include "sharedlib.h"
#include "str_list.h"

#ifdef DMEVENTD
#  include "libdevmapper-event.h"
#endif

static int _block_on_error_available = 0;
static unsigned _mirror_attributes = 0;

enum {
	MIRR_DISABLED,
	MIRR_RUNNING,
	MIRR_COMPLETED
};

struct mirror_state {
	uint32_t default_region_size;
};

static const char *_mirrored_name(const struct lv_segment *seg)
{
	return seg->segtype->name;
}

static void _mirrored_display(const struct lv_segment *seg)
{
	const char *size;
	uint32_t s;

	log_print("  Mirrors\t\t%u", seg->area_count);
	log_print("  Mirror size\t\t%u", seg->area_len);
	if (seg->log_lv)
		log_print("  Mirror log volume\t%s", seg->log_lv->name);

	if (seg->region_size) {
		size = display_size(seg->lv->vg->cmd,
				    (uint64_t) seg->region_size);
		log_print("  Mirror region size\t%s", size);
	}

	log_print("  Mirror original:");
	display_stripe(seg, 0, "    ");
	log_print("  Mirror destinations:");
	for (s = 1; s < seg->area_count; s++)
		display_stripe(seg, s, "    ");
	log_print(" ");
}

static int _mirrored_text_import_area_count(struct config_node *sn, uint32_t *area_count)
{
	if (!get_config_uint32(sn, "mirror_count", area_count)) {
		log_error("Couldn't read 'mirror_count' for "
			  "segment '%s'.", config_parent_name(sn));
		return 0;
	}

	return 1;
}

static int _mirrored_text_import(struct lv_segment *seg, const struct config_node *sn,
			struct dm_hash_table *pv_hash)
{
	const struct config_node *cn;
	char *logname = NULL;

	if (find_config_node(sn, "extents_moved")) {
		if (get_config_uint32(sn, "extents_moved",
				      &seg->extents_copied))
			seg->status |= PVMOVE;
		else {
			log_error("Couldn't read 'extents_moved' for "
				  "segment %s of logical volume %s.",
				  config_parent_name(sn), seg->lv->name);
			return 0;
		}
	}

	if (find_config_node(sn, "region_size")) {
		if (!get_config_uint32(sn, "region_size",
				      &seg->region_size)) {
			log_error("Couldn't read 'region_size' for "
				  "segment %s of logical volume %s.",
				  config_parent_name(sn), seg->lv->name);
			return 0;
		}
	}

	if ((cn = find_config_node(sn, "mirror_log"))) {
		if (!cn->v || !cn->v->v.str) {
			log_error("Mirror log type must be a string.");
			return 0;
		}
		logname = cn->v->v.str;
		if (!(seg->log_lv = find_lv(seg->lv->vg, logname))) {
			log_error("Unrecognised mirror log in "
				  "segment %s of logical volume %s.",
				  config_parent_name(sn), seg->lv->name);
			return 0;
		}
		seg->log_lv->status |= MIRROR_LOG;
	}

	if (logname && !seg->region_size) {
		log_error("Missing region size for mirror log for "
			  "segment %s of logical volume %s.",
			  config_parent_name(sn), seg->lv->name);
		return 0;
	}

	if (!(cn = find_config_node(sn, "mirrors"))) {
		log_error("Couldn't find mirrors array for "
			  "segment %s of logical volume %s.",
			  config_parent_name(sn), seg->lv->name);
		return 0;
	}

	return text_import_areas(seg, sn, cn, pv_hash, MIRROR_IMAGE);
}

static int _mirrored_text_export(const struct lv_segment *seg, struct formatter *f)
{
	outf(f, "mirror_count = %u", seg->area_count);
	if (seg->status & PVMOVE)
		out_size(f, (uint64_t) seg->extents_copied * seg->lv->vg->extent_size,
			 "extents_moved = %" PRIu32, seg->extents_copied);
	if (seg->log_lv)
		outf(f, "mirror_log = \"%s\"", seg->log_lv->name);
	if (seg->region_size)
		outf(f, "region_size = %" PRIu32, seg->region_size);

	return out_areas(f, seg, "mirror");
}

#ifdef DEVMAPPER_SUPPORT
static struct mirror_state *_mirrored_init_target(struct dm_pool *mem,
					 struct cmd_context *cmd)
{
	struct mirror_state *mirr_state;

	if (!(mirr_state = dm_pool_alloc(mem, sizeof(*mirr_state)))) {
		log_error("struct mirr_state allocation failed");
		return NULL;
	}

	mirr_state->default_region_size = 2 *
	    find_config_tree_int(cmd,
			    "activation/mirror_region_size",
			    DEFAULT_MIRROR_REGION_SIZE);

	return mirr_state;
}

static int _mirrored_target_percent(void **target_state,
				    percent_range_t *percent_range,
				    struct dm_pool *mem,
				    struct cmd_context *cmd,
				    struct lv_segment *seg, char *params,
				    uint64_t *total_numerator,
				    uint64_t *total_denominator)
{
	struct mirror_state *mirr_state;
	uint64_t numerator, denominator;
	unsigned mirror_count, m;
	int used;
	char *pos = params;

	if (!*target_state)
		*target_state = _mirrored_init_target(mem, cmd);

	mirr_state = *target_state;

	/* Status line: <#mirrors> (maj:min)+ <synced>/<total_regions> */
	log_debug("Mirror status: %s", params);

	if (sscanf(pos, "%u %n", &mirror_count, &used) != 1) {
		log_error("Failure parsing mirror status mirror count: %s",
			  params);
		return 0;
	}
	pos += used;

	for (m = 0; m < mirror_count; m++) {
		if (sscanf(pos, "%*x:%*x %n", &used) != 0) {
			log_error("Failure parsing mirror status devices: %s",
				  params);
			return 0;
		}
		pos += used;
	}

	if (sscanf(pos, "%" PRIu64 "/%" PRIu64 "%n", &numerator, &denominator,
		   &used) != 2) {
		log_error("Failure parsing mirror status fraction: %s", params);
		return 0;
	}
	pos += used;

	*total_numerator += numerator;
	*total_denominator += denominator;

	if (seg)
		seg->extents_copied = seg->area_len * numerator / denominator;

	if (numerator == denominator)
		*percent_range = PERCENT_100;
	else if (numerator == 0)
		*percent_range = PERCENT_0;
	else
		*percent_range = PERCENT_0_TO_100;

	return 1;
}

static int _add_log(struct dev_manager *dm, struct lv_segment *seg,
		    struct dm_tree_node *node, uint32_t area_count, uint32_t region_size)
{
	unsigned clustered = 0;
	char *log_dlid = NULL;
	uint32_t log_flags = 0;

	/*
	 * Use clustered mirror log for non-exclusive activation
	 * in clustered VG.
	 */
	if ((!(seg->lv->status & ACTIVATE_EXCL) &&
	      (vg_is_clustered(seg->lv->vg))))
		clustered = 1;

	if (seg->log_lv) {
		/* If disk log, use its UUID */
		if (!(log_dlid = build_dlid(dm, seg->log_lv->lvid.s, NULL))) {
			log_error("Failed to build uuid for log LV %s.",
				  seg->log_lv->name);
			return 0;
		}
	} else {
		/* If core log, use mirror's UUID and set DM_CORELOG flag */
		if (!(log_dlid = build_dlid(dm, seg->lv->lvid.s, NULL))) {
			log_error("Failed to build uuid for mirror LV %s.",
				  seg->lv->name);
			return 0;
		}
		log_flags |= DM_CORELOG;
	}

	if (mirror_in_sync() && !(seg->status & PVMOVE))
		log_flags |= DM_NOSYNC;

	if (_block_on_error_available && !(seg->status & PVMOVE))
		log_flags |= DM_BLOCK_ON_ERROR;

	return dm_tree_node_add_mirror_target_log(node, region_size, clustered, log_dlid, area_count, log_flags);
}

static int _mirrored_add_target_line(struct dev_manager *dm, struct dm_pool *mem,
				struct cmd_context *cmd, void **target_state,
				struct lv_segment *seg,
				struct dm_tree_node *node, uint64_t len,
				uint32_t *pvmove_mirror_count)
{
	struct mirror_state *mirr_state;
	uint32_t area_count = seg->area_count;
	unsigned start_area = 0u;
	int mirror_status = MIRR_RUNNING;
	uint32_t region_size;
	int r;

	if (!*target_state)
		*target_state = _mirrored_init_target(mem, cmd);

	mirr_state = *target_state;

	/*
	 * Mirror segment could have only 1 area temporarily
	 * if the segment is under conversion.
	 */
 	if (seg->area_count == 1)
		mirror_status = MIRR_DISABLED;

	/*
	 * For pvmove, only have one mirror segment RUNNING at once.
	 * Segments before this are COMPLETED and use 2nd area.
	 * Segments after this are DISABLED and use 1st area.
	 */
	if (seg->status & PVMOVE) {
		if (seg->extents_copied == seg->area_len) {
			mirror_status = MIRR_COMPLETED;
			start_area = 1;
		} else if ((*pvmove_mirror_count)++) {
			mirror_status = MIRR_DISABLED;
			area_count = 1;
		}
		/* else MIRR_RUNNING */
	}

	if (mirror_status != MIRR_RUNNING) {
		if (!dm_tree_node_add_linear_target(node, len))
			return_0;
		goto done;
	}

	if (!(seg->status & PVMOVE)) {
		if (!seg->region_size) {
			log_error("Missing region size for mirror segment.");
			return 0;
		}
		region_size = seg->region_size;

	} else
		region_size = adjusted_mirror_region_size(seg->lv->vg->extent_size,
							  seg->area_len,
							  mirr_state->default_region_size);

	if (!dm_tree_node_add_mirror_target(node, len))
		return_0;

	if ((r = _add_log(dm, seg, node, area_count, region_size)) <= 0) {
		stack;
		return r;
	}

      done:
	return add_areas_line(dm, seg, node, start_area, area_count);
}

static int _mirrored_target_present(struct cmd_context *cmd,
				    const struct lv_segment *seg,
				    unsigned *attributes)
{
	static int _mirrored_checked = 0;
	static int _mirrored_present = 0;
	uint32_t maj, min, patchlevel;
	unsigned maj2, min2, patchlevel2;
	char vsn[80];

	if (!_mirrored_checked) {
		_mirrored_present = target_present(cmd, "mirror", 1);

		/*
		 * block_on_error available as "block_on_error" log
		 * argument with mirror target >= 1.1 and <= 1.11
		 * or with 1.0 in RHEL4U3 driver >= 4.5
		 *
		 * block_on_error available as "handle_errors" mirror
		 * argument with mirror target >= 1.12.
		 *
		 * libdm-deptree.c is smart enough to handle the differences
		 * between block_on_error and handle_errors for all
		 * mirror target versions >= 1.1
		 */
		/* FIXME Move this into libdevmapper */

		if (target_version("mirror", &maj, &min, &patchlevel) &&
		    maj == 1 &&
		    ((min >= 1) ||
		     (min == 0 && driver_version(vsn, sizeof(vsn)) &&
		      sscanf(vsn, "%u.%u.%u", &maj2, &min2, &patchlevel2) == 3 &&
		      maj2 == 4 && min2 == 5 && patchlevel2 == 0)))	/* RHEL4U3 */
			_block_on_error_available = 1;
	}

	/*
	 * Check only for modules if atttributes requested and no previous check.
	 * FIXME: Fails incorrectly if cmirror was built into kernel.
	 */
	if (attributes) {
		if (!_mirror_attributes && module_present(cmd, "log-clustered"))
			_mirror_attributes |= MIRROR_LOG_CLUSTERED;
		*attributes = _mirror_attributes;
	}
	_mirrored_checked = 1;

	return _mirrored_present;
}

#ifdef DMEVENTD
static int _get_mirror_dso_path(struct cmd_context *cmd, char **dso)
{
	char *path;
	const char *libpath;

	if (!(path = dm_pool_alloc(cmd->mem, PATH_MAX))) {
		log_error("Failed to allocate dmeventd library path.");
		return 0;
	}

	libpath = find_config_tree_str(cmd, "dmeventd/mirror_library",
				       DEFAULT_DMEVENTD_MIRROR_LIB);

	get_shared_library_path(cmd, libpath, path, PATH_MAX);

	*dso = path;

	return 1;
}

static struct dm_event_handler *_create_dm_event_handler(const char *dmname,
							 const char *dso,
							 enum dm_event_mask mask)
{
	struct dm_event_handler *dmevh;

	if (!(dmevh = dm_event_handler_create()))
		return_0;

       if (dm_event_handler_set_dso(dmevh, dso))
		goto fail;

	if (dm_event_handler_set_dev_name(dmevh, dmname))
		goto fail;

	dm_event_handler_set_event_mask(dmevh, mask);
	return dmevh;

fail:
	dm_event_handler_destroy(dmevh);
	return NULL;
}

static int _target_monitored(struct lv_segment *seg, int *pending)
{
	char *dso, *name;
	struct logical_volume *lv;
	struct volume_group *vg;
	enum dm_event_mask evmask = 0;
	struct dm_event_handler *dmevh;

	lv = seg->lv;
	vg = lv->vg;

	*pending = 0;
	if (!_get_mirror_dso_path(vg->cmd, &dso))
		return_0;

	if (!(name = build_dm_name(vg->cmd->mem, vg->name, lv->name, NULL)))
		return_0;

	if (!(dmevh = _create_dm_event_handler(name, dso, DM_EVENT_ALL_ERRORS)))
		return_0;

	if (dm_event_get_registered_device(dmevh, 0)) {
		dm_event_handler_destroy(dmevh);
		return 0;
	}

	evmask = dm_event_handler_get_event_mask(dmevh);
	if (evmask & DM_EVENT_REGISTRATION_PENDING) {
		*pending = 1;
		evmask &= ~DM_EVENT_REGISTRATION_PENDING;
	}

	dm_event_handler_destroy(dmevh);

	return evmask;
}

/* FIXME This gets run while suspended and performs banned operations. */
static int _target_set_events(struct lv_segment *seg,
			      int evmask __attribute((unused)), int set)
{
	char *dso, *name;
	struct logical_volume *lv;
	struct volume_group *vg;
	struct dm_event_handler *dmevh;
	int r;

	lv = seg->lv;
	vg = lv->vg;

	if (!_get_mirror_dso_path(vg->cmd, &dso))
		return_0;

	if (!(name = build_dm_name(vg->cmd->mem, vg->name, lv->name, NULL)))
		return_0;

	if (!(dmevh = _create_dm_event_handler(name, dso, DM_EVENT_ALL_ERRORS)))
		return_0;

	r = set ? dm_event_register_handler(dmevh) : dm_event_unregister_handler(dmevh);
	dm_event_handler_destroy(dmevh);
	if (!r)
		return_0;

	log_info("%s %s for events", set ? "Monitored" : "Unmonitored", name);

	return 1;
}

static int _target_monitor_events(struct lv_segment *seg, int events)
{
	return _target_set_events(seg, events, 1);
}

static int _target_unmonitor_events(struct lv_segment *seg, int events)
{
	return _target_set_events(seg, events, 0);
}

#endif /* DMEVENTD */
#endif /* DEVMAPPER_SUPPORT */

static int _mirrored_modules_needed(struct dm_pool *mem,
				    const struct lv_segment *seg,
				    struct dm_list *modules)
{
	if (seg->log_lv &&
	    !list_segment_modules(mem, first_seg(seg->log_lv), modules))
		return_0;

	if (vg_is_clustered(seg->lv->vg) &&
	    !str_list_add(mem, modules, "clog")) {
		log_error("cluster log string list allocation failed");
		return 0;
	}

	if (!str_list_add(mem, modules, "mirror")) {
		log_error("mirror string list allocation failed");
		return 0;
	}

	return 1;
}

static void _mirrored_destroy(const struct segment_type *segtype)
{
	dm_free((void *) segtype);
}

static struct segtype_handler _mirrored_ops = {
	.name = _mirrored_name,
	.display = _mirrored_display,
	.text_import_area_count = _mirrored_text_import_area_count,
	.text_import = _mirrored_text_import,
	.text_export = _mirrored_text_export,
#ifdef DEVMAPPER_SUPPORT
	.add_target_line = _mirrored_add_target_line,
	.target_percent = _mirrored_target_percent,
	.target_present = _mirrored_target_present,
#ifdef DMEVENTD
	.target_monitored = _target_monitored,
	.target_monitor_events = _target_monitor_events,
	.target_unmonitor_events = _target_unmonitor_events,
#endif
#endif
	.modules_needed = _mirrored_modules_needed,
	.destroy = _mirrored_destroy,
};

#ifdef MIRRORED_INTERNAL
struct segment_type *init_mirrored_segtype(struct cmd_context *cmd)
#else				/* Shared */
struct segment_type *init_segtype(struct cmd_context *cmd);
struct segment_type *init_segtype(struct cmd_context *cmd)
#endif
{
	struct segment_type *segtype = dm_malloc(sizeof(*segtype));

	if (!segtype)
		return_NULL;

	segtype->cmd = cmd;
	segtype->ops = &_mirrored_ops;
	segtype->name = "mirror";
	segtype->private = NULL;
	segtype->flags = SEG_AREAS_MIRRORED | SEG_MONITORED;

	log_very_verbose("Initialised segtype: %s", segtype->name);

	return segtype;
}
