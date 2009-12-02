/*	$NetBSD: snapshot.c,v 1.1.1.2 2009/12/02 00:26:27 haad Exp $	*/

/*
 * Copyright (C) 2003-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2008 Red Hat, Inc. All rights reserved.
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
#include "text_export.h"
#include "config.h"
#include "activate.h"
#include "str_list.h"
#ifdef DMEVENTD
#  include "sharedlib.h"
#  include "libdevmapper-event.h"
#endif

static const char *_snap_name(const struct lv_segment *seg)
{
	return seg->segtype->name;
}

static int _snap_text_import(struct lv_segment *seg, const struct config_node *sn,
			struct dm_hash_table *pv_hash __attribute((unused)))
{
	uint32_t chunk_size;
	const char *org_name, *cow_name;
	struct logical_volume *org, *cow;
	int old_suppress;

	if (!get_config_uint32(sn, "chunk_size", &chunk_size)) {
		log_error("Couldn't read chunk size for snapshot.");
		return 0;
	}

	old_suppress = log_suppress(1);

	if (!(cow_name = find_config_str(sn, "cow_store", NULL))) {
		log_suppress(old_suppress);
		log_error("Snapshot cow storage not specified.");
		return 0;
	}

	if (!(org_name = find_config_str(sn, "origin", NULL))) {
		log_suppress(old_suppress);
		log_error("Snapshot origin not specified.");
		return 0;
	}

	log_suppress(old_suppress);

	if (!(cow = find_lv(seg->lv->vg, cow_name))) {
		log_error("Unknown logical volume specified for "
			  "snapshot cow store.");
		return 0;
	}

	if (!(org = find_lv(seg->lv->vg, org_name))) {
		log_error("Unknown logical volume specified for "
			  "snapshot origin.");
		return 0;
	}

	init_snapshot_seg(seg, org, cow, chunk_size);

	return 1;
}

static int _snap_text_export(const struct lv_segment *seg, struct formatter *f)
{
	outf(f, "chunk_size = %u", seg->chunk_size);
	outf(f, "origin = \"%s\"", seg->origin->name);
	outf(f, "cow_store = \"%s\"", seg->cow->name);

	return 1;
}

#ifdef DEVMAPPER_SUPPORT
static int _snap_target_percent(void **target_state __attribute((unused)),
				percent_range_t *percent_range,
				struct dm_pool *mem __attribute((unused)),
				struct cmd_context *cmd __attribute((unused)),
				struct lv_segment *seg __attribute((unused)),
				char *params, uint64_t *total_numerator,
				uint64_t *total_denominator)
{
	uint64_t numerator, denominator;

	if (sscanf(params, "%" PRIu64 "/%" PRIu64,
		   &numerator, &denominator) == 2) {
		*total_numerator += numerator;
		*total_denominator += denominator;
		if (!numerator)
			*percent_range = PERCENT_0;
		else if (numerator == denominator)
			*percent_range = PERCENT_100;
		else
			*percent_range = PERCENT_0_TO_100;
	} else if (!strcmp(params, "Invalid"))
		*percent_range = PERCENT_INVALID;
	else
		return 0;

	return 1;
}

static int _snap_target_present(struct cmd_context *cmd,
				const struct lv_segment *seg __attribute((unused)),
				unsigned *attributes __attribute((unused)))
{
	static int _snap_checked = 0;
	static int _snap_present = 0;

	if (!_snap_checked)
		_snap_present = target_present(cmd, "snapshot", 1) &&
		    target_present(cmd, "snapshot-origin", 0);

	_snap_checked = 1;

	return _snap_present;
}

#ifdef DMEVENTD
static int _get_snapshot_dso_path(struct cmd_context *cmd, char **dso)
{
	char *path;
	const char *libpath;

	if (!(path = dm_pool_alloc(cmd->mem, PATH_MAX))) {
		log_error("Failed to allocate dmeventd library path.");
		return 0;
	}

	libpath = find_config_tree_str(cmd, "dmeventd/snapshot_library", NULL);
	if (!libpath)
		return 0;

	get_shared_library_path(cmd, libpath, path, PATH_MAX);

	*dso = path;

	return 1;
}

static struct dm_event_handler *_create_dm_event_handler(const char *dmname,
							 const char *dso,
							 const int timeout,
							 enum dm_event_mask mask)
{
	struct dm_event_handler *dmevh;

	if (!(dmevh = dm_event_handler_create()))
		return_0;

       if (dm_event_handler_set_dso(dmevh, dso))
		goto fail;

	if (dm_event_handler_set_dev_name(dmevh, dmname))
		goto fail;

	dm_event_handler_set_timeout(dmevh, timeout);
	dm_event_handler_set_event_mask(dmevh, mask);
	return dmevh;

fail:
	dm_event_handler_destroy(dmevh);
	return NULL;
}

static int _target_registered(struct lv_segment *seg, int *pending)
{
	char *dso, *name;
	struct logical_volume *lv;
	struct volume_group *vg;
	enum dm_event_mask evmask = 0;
	struct dm_event_handler *dmevh;

	lv = seg->lv;
	vg = lv->vg;

	*pending = 0;
	if (!_get_snapshot_dso_path(vg->cmd, &dso))
		return_0;

	if (!(name = build_dm_name(vg->cmd->mem, vg->name, seg->cow->name, NULL)))
		return_0;

	if (!(dmevh = _create_dm_event_handler(name, dso, 0, DM_EVENT_ALL_ERRORS)))
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
			      int events __attribute((unused)), int set)
{
	char *dso, *name;
	struct volume_group *vg = seg->lv->vg;
	struct dm_event_handler *dmevh;
	int r;

	if (!_get_snapshot_dso_path(vg->cmd, &dso))
		return_0;

	if (!(name = build_dm_name(vg->cmd->mem, vg->name, seg->cow->name, NULL)))
		return_0;

	/* FIXME: make timeout configurable */
	if (!(dmevh = _create_dm_event_handler(name, dso, 10,
		DM_EVENT_ALL_ERRORS|DM_EVENT_TIMEOUT)))
		return_0;

	r = set ? dm_event_register_handler(dmevh) : dm_event_unregister_handler(dmevh);
	dm_event_handler_destroy(dmevh);
	if (!r)
		return_0;

	log_info("%s %s for events", set ? "Registered" : "Unregistered", name);

	return 1;
}

static int _target_register_events(struct lv_segment *seg,
				   int events)
{
	return _target_set_events(seg, events, 1);
}

static int _target_unregister_events(struct lv_segment *seg,
				     int events)
{
	return _target_set_events(seg, events, 0);
}

#endif /* DMEVENTD */
#endif

static int _snap_modules_needed(struct dm_pool *mem,
				const struct lv_segment *seg __attribute((unused)),
				struct dm_list *modules)
{
	if (!str_list_add(mem, modules, "snapshot")) {
		log_error("snapshot string list allocation failed");
		return 0;
	}

	return 1;
}

static void _snap_destroy(const struct segment_type *segtype)
{
	dm_free((void *)segtype);
}

static struct segtype_handler _snapshot_ops = {
	.name = _snap_name,
	.text_import = _snap_text_import,
	.text_export = _snap_text_export,
#ifdef DEVMAPPER_SUPPORT
	.target_percent = _snap_target_percent,
	.target_present = _snap_target_present,
#ifdef DMEVENTD
	.target_monitored = _target_registered,
	.target_monitor_events = _target_register_events,
	.target_unmonitor_events = _target_unregister_events,
#endif
#endif
	.modules_needed = _snap_modules_needed,
	.destroy = _snap_destroy,
};

#ifdef SNAPSHOT_INTERNAL
struct segment_type *init_snapshot_segtype(struct cmd_context *cmd)
#else				/* Shared */
struct segment_type *init_segtype(struct cmd_context *cmd);
struct segment_type *init_segtype(struct cmd_context *cmd)
#endif
{
	struct segment_type *segtype = dm_malloc(sizeof(*segtype));
#ifdef DMEVENTD
	char *dso;
#endif

	if (!segtype)
		return_NULL;

	segtype->cmd = cmd;
	segtype->ops = &_snapshot_ops;
	segtype->name = "snapshot";
	segtype->private = NULL;
	segtype->flags = SEG_SNAPSHOT;

#ifdef DMEVENTD
	if (_get_snapshot_dso_path(cmd, &dso))
		segtype->flags |= SEG_MONITORED;
#endif
	log_very_verbose("Initialised segtype: %s", segtype->name);

	return segtype;
}
