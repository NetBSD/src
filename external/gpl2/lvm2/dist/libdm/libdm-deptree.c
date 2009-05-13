/*	$NetBSD: libdm-deptree.c,v 1.2.2.1 2009/05/13 18:52:43 jym Exp $	*/

/*
 * Copyright (C) 2005-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of the device-mapper userspace tools.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "dmlib.h"
#include "libdm-targets.h"
#include "libdm-common.h"
#include "kdev_t.h"
#include "dm-ioctl.h"

#include <stdarg.h>
#include <sys/param.h>

#define MAX_TARGET_PARAMSIZE 500000

/* FIXME Fix interface so this is used only by LVM */
#define UUID_PREFIX "LVM-"

/* Supported segment types */
enum {
	SEG_ERROR, 
	SEG_LINEAR,
	SEG_MIRRORED,
	SEG_SNAPSHOT,
	SEG_SNAPSHOT_ORIGIN,
	SEG_STRIPED,
	SEG_ZERO,
};

/* FIXME Add crypt and multipath support */

struct {
	unsigned type;
	const char *target;
} dm_segtypes[] = {
	{ SEG_ERROR, "error" },
	{ SEG_LINEAR, "linear" },
	{ SEG_MIRRORED, "mirror" },
	{ SEG_SNAPSHOT, "snapshot" },
	{ SEG_SNAPSHOT_ORIGIN, "snapshot-origin" },
	{ SEG_STRIPED, "striped" },
	{ SEG_ZERO, "zero"},
};

/* Some segment types have a list of areas of other devices attached */
struct seg_area {
	struct dm_list list;

	struct dm_tree_node *dev_node;

	uint64_t offset;
};

/* Per-segment properties */
struct load_segment {
	struct dm_list list;

	unsigned type;

	uint64_t size;

	unsigned area_count;		/* Linear + Striped + Mirrored */
	struct dm_list areas;		/* Linear + Striped + Mirrored */

	uint32_t stripe_size;		/* Striped */

	int persistent;			/* Snapshot */
	uint32_t chunk_size;		/* Snapshot */
	struct dm_tree_node *cow;	/* Snapshot */
	struct dm_tree_node *origin;	/* Snapshot + Snapshot origin */

	struct dm_tree_node *log;	/* Mirror */
	uint32_t region_size;		/* Mirror */
	unsigned clustered;		/* Mirror */
	unsigned mirror_area_count;	/* Mirror */
	uint32_t flags;			/* Mirror log */
	char *uuid;			/* Clustered mirror log */
};

/* Per-device properties */
struct load_properties {
	int read_only;
	uint32_t major;
	uint32_t minor;

	uint32_t read_ahead;
	uint32_t read_ahead_flags;

	unsigned segment_count;
	unsigned size_changed;
	struct dm_list segs;

	const char *new_name;
};

/* Two of these used to join two nodes with uses and used_by. */
struct dm_tree_link {
	struct dm_list list;
	struct dm_tree_node *node;
};

struct dm_tree_node {
	struct dm_tree *dtree;

        const char *name;
        const char *uuid;
        struct dm_info info;

        struct dm_list uses;       	/* Nodes this node uses */
        struct dm_list used_by;    	/* Nodes that use this node */

	int activation_priority;	/* 0 gets activated first */

	void *context;			/* External supplied context */

	struct load_properties props;	/* For creation/table (re)load */
};

struct dm_tree {
	struct dm_pool *mem;
	struct dm_hash_table *devs;
	struct dm_hash_table *uuids;
	struct dm_tree_node root;
	int skip_lockfs;		/* 1 skips lockfs (for non-snapshots) */
	int no_flush;		/* 1 sets noflush (mirrors/multipath) */
};

struct dm_tree *dm_tree_create(void)
{
	struct dm_tree *dtree;

	if (!(dtree = dm_malloc(sizeof(*dtree)))) {
		log_error("dm_tree_create malloc failed");
		return NULL;
	}

	memset(dtree, 0, sizeof(*dtree));
	dtree->root.dtree = dtree;
	dm_list_init(&dtree->root.uses);
	dm_list_init(&dtree->root.used_by);
	dtree->skip_lockfs = 0;
	dtree->no_flush = 0;

	if (!(dtree->mem = dm_pool_create("dtree", 1024))) {
		log_error("dtree pool creation failed");
		dm_free(dtree);
		return NULL;
	}

	if (!(dtree->devs = dm_hash_create(8))) {
		log_error("dtree hash creation failed");
		dm_pool_destroy(dtree->mem);
		dm_free(dtree);
		return NULL;
	}

	if (!(dtree->uuids = dm_hash_create(32))) {
		log_error("dtree uuid hash creation failed");
		dm_hash_destroy(dtree->devs);
		dm_pool_destroy(dtree->mem);
		dm_free(dtree);
		return NULL;
	}

	return dtree;
}

void dm_tree_free(struct dm_tree *dtree)
{
	if (!dtree)
		return;

	dm_hash_destroy(dtree->uuids);
	dm_hash_destroy(dtree->devs);
	dm_pool_destroy(dtree->mem);
	dm_free(dtree);
}

static int _nodes_are_linked(struct dm_tree_node *parent,
			     struct dm_tree_node *child)
{
	struct dm_tree_link *dlink;

	dm_list_iterate_items(dlink, &parent->uses)
		if (dlink->node == child)
			return 1;

	return 0;
}

static int _link(struct dm_list *list, struct dm_tree_node *node)
{
	struct dm_tree_link *dlink;

	if (!(dlink = dm_pool_alloc(node->dtree->mem, sizeof(*dlink)))) {
		log_error("dtree link allocation failed");
		return 0;
	}

	dlink->node = node;
	dm_list_add(list, &dlink->list);

	return 1;
}

static int _link_nodes(struct dm_tree_node *parent,
		       struct dm_tree_node *child)
{
	if (_nodes_are_linked(parent, child))
		return 1;

	if (!_link(&parent->uses, child))
		return 0;

	if (!_link(&child->used_by, parent))
		return 0;

	return 1;
}

static void _unlink(struct dm_list *list, struct dm_tree_node *node)
{
	struct dm_tree_link *dlink;

	dm_list_iterate_items(dlink, list)
		if (dlink->node == node) {
			dm_list_del(&dlink->list);
			break;
		}
}

static void _unlink_nodes(struct dm_tree_node *parent,
			  struct dm_tree_node *child)
{
	if (!_nodes_are_linked(parent, child))
		return;

	_unlink(&parent->uses, child);
	_unlink(&child->used_by, parent);
}

static int _add_to_toplevel(struct dm_tree_node *node)
{
	return _link_nodes(&node->dtree->root, node);
}

static void _remove_from_toplevel(struct dm_tree_node *node)
{
	return _unlink_nodes(&node->dtree->root, node);
}

static int _add_to_bottomlevel(struct dm_tree_node *node)
{
	return _link_nodes(node, &node->dtree->root);
}

static void _remove_from_bottomlevel(struct dm_tree_node *node)
{
	return _unlink_nodes(node, &node->dtree->root);
}

static int _link_tree_nodes(struct dm_tree_node *parent, struct dm_tree_node *child)
{
	/* Don't link to root node if child already has a parent */
	if ((parent == &parent->dtree->root)) {
		if (dm_tree_node_num_children(child, 1))
			return 1;
	} else
		_remove_from_toplevel(child);

	if ((child == &child->dtree->root)) {
		if (dm_tree_node_num_children(parent, 0))
			return 1;
	} else
		_remove_from_bottomlevel(parent);

	return _link_nodes(parent, child);
}

static struct dm_tree_node *_create_dm_tree_node(struct dm_tree *dtree,
						 const char *name,
						 const char *uuid,
						 struct dm_info *info,
						 void *context)
{
	struct dm_tree_node *node;
	uint64_t dev;

	if (!(node = dm_pool_zalloc(dtree->mem, sizeof(*node)))) {
		log_error("_create_dm_tree_node alloc failed");
		return NULL;
	}

	node->dtree = dtree;

	node->name = name;
	node->uuid = uuid;
	node->info = *info;
	node->context = context;
	node->activation_priority = 0;

	dm_list_init(&node->uses);
	dm_list_init(&node->used_by);
	dm_list_init(&node->props.segs);

	dev = MKDEV(info->major, info->minor);

	if (!dm_hash_insert_binary(dtree->devs, (const char *) &dev,
				sizeof(dev), node)) {
		log_error("dtree node hash insertion failed");
		dm_pool_free(dtree->mem, node);
		return NULL;
	}

	if (uuid && *uuid &&
	    !dm_hash_insert(dtree->uuids, uuid, node)) {
		log_error("dtree uuid hash insertion failed");
		dm_hash_remove_binary(dtree->devs, (const char *) &dev,
				      sizeof(dev));
		dm_pool_free(dtree->mem, node);
		return NULL;
	}

	return node;
}

static struct dm_tree_node *_find_dm_tree_node(struct dm_tree *dtree,
					       uint32_t major, uint32_t minor)
{
	uint64_t dev = MKDEV(major, minor);

	return dm_hash_lookup_binary(dtree->devs, (const char *) &dev,
				  sizeof(dev));
}

static struct dm_tree_node *_find_dm_tree_node_by_uuid(struct dm_tree *dtree,
						       const char *uuid)
{
	struct dm_tree_node *node;

	if ((node = dm_hash_lookup(dtree->uuids, uuid)))
		return node;

	if (strncmp(uuid, UUID_PREFIX, sizeof(UUID_PREFIX) - 1))
		return NULL;

	return dm_hash_lookup(dtree->uuids, uuid + sizeof(UUID_PREFIX) - 1);
}

static int _deps(struct dm_task **dmt, struct dm_pool *mem, uint32_t major, uint32_t minor,
		 const char **name, const char **uuid,
		 struct dm_info *info, struct dm_deps **deps)
{
	memset(info, 0, sizeof(*info));

	if (!dm_is_dm_major(major)) {
		*name = "";
		*uuid = "";
		*deps = NULL;
		info->major = major;
		info->minor = minor;
		info->exists = 0;
		info->live_table = 0;
		info->inactive_table = 0;
		info->read_only = 0;
		return 1;
	}

	if (!(*dmt = dm_task_create(DM_DEVICE_DEPS))) {
		log_error("deps dm_task creation failed");
		return 0;
	}

	if (!dm_task_set_major(*dmt, major)) {
		log_error("_deps: failed to set major for (%" PRIu32 ":%" PRIu32 ")",
			  major, minor);
		goto failed;
	}

	if (!dm_task_set_minor(*dmt, minor)) {
		log_error("_deps: failed to set minor for (%" PRIu32 ":%" PRIu32 ")",
			  major, minor);
		goto failed;
	}

	if (!dm_task_run(*dmt)) {
		log_error("_deps: task run failed for (%" PRIu32 ":%" PRIu32 ")",
			  major, minor);
		goto failed;
	}

	if (!dm_task_get_info(*dmt, info)) {
		log_error("_deps: failed to get info for (%" PRIu32 ":%" PRIu32 ")",
			  major, minor);
		goto failed;
	}

	if (!info->exists) {
		*name = "";
		*uuid = "";
		*deps = NULL;
	} else {
		if (info->major != major) {
			log_error("Inconsistent dtree major number: %u != %u",
				  major, info->major);
			goto failed;
		}
		if (info->minor != minor) {
			log_error("Inconsistent dtree minor number: %u != %u",
				  minor, info->minor);
			goto failed;
		}
		if (!(*name = dm_pool_strdup(mem, dm_task_get_name(*dmt)))) {
			log_error("name pool_strdup failed");
			goto failed;
		}
		if (!(*uuid = dm_pool_strdup(mem, dm_task_get_uuid(*dmt)))) {
			log_error("uuid pool_strdup failed");
			goto failed;
		}
		*deps = dm_task_get_deps(*dmt);
	}

	return 1;

failed:
	dm_task_destroy(*dmt);
	return 0;
}

static struct dm_tree_node *_add_dev(struct dm_tree *dtree,
				     struct dm_tree_node *parent,
				     uint32_t major, uint32_t minor)
{
	struct dm_task *dmt = NULL;
	struct dm_info info;
	struct dm_deps *deps = NULL;
	const char *name = NULL;
	const char *uuid = NULL;
	struct dm_tree_node *node = NULL;
	uint32_t i;
	int new = 0;

	/* Already in tree? */
	if (!(node = _find_dm_tree_node(dtree, major, minor))) {
		if (!_deps(&dmt, dtree->mem, major, minor, &name, &uuid, &info, &deps))
			return_NULL;

		if (!(node = _create_dm_tree_node(dtree, name, uuid,
						  &info, NULL)))
			goto_out;
		new = 1;
	}

	if (!_link_tree_nodes(parent, node)) {
		node = NULL;
		goto_out;
	}

	/* If node was already in tree, no need to recurse. */
	if (!new)
		goto out;

	/* Can't recurse if not a mapped device or there are no dependencies */
	if (!node->info.exists || !deps->count) {
		if (!_add_to_bottomlevel(node)) {
			stack;
			node = NULL;
		}
		goto out;
	}

	/* Add dependencies to tree */
	for (i = 0; i < deps->count; i++)
		if (!_add_dev(dtree, node, MAJOR(deps->device[i]),
			      MINOR(deps->device[i]))) {
			node = NULL;
			goto_out;
		}

out:
	if (dmt)
		dm_task_destroy(dmt);

	return node;
}

static int _node_clear_table(struct dm_tree_node *dnode)
{
	struct dm_task *dmt;
	struct dm_info *info;
	const char *name;
	int r;

	if (!(info = &dnode->info)) {
		log_error("_node_clear_table failed: missing info");
		return 0;
	}

	if (!(name = dm_tree_node_get_name(dnode))) {
		log_error("_node_clear_table failed: missing name");
		return 0;
	}

	/* Is there a table? */
	if (!info->exists || !info->inactive_table)
		return 1;

	log_verbose("Clearing inactive table %s (%" PRIu32 ":%" PRIu32 ")",
		    name, info->major, info->minor);

	if (!(dmt = dm_task_create(DM_DEVICE_CLEAR))) {
		dm_task_destroy(dmt);
		log_error("Table clear dm_task creation failed for %s", name);
		return 0;
	}

	if (!dm_task_set_major(dmt, info->major) ||
	    !dm_task_set_minor(dmt, info->minor)) {
		log_error("Failed to set device number for %s table clear", name);
		dm_task_destroy(dmt);
		return 0;
	}

	r = dm_task_run(dmt);

	if (!dm_task_get_info(dmt, info)) {
		log_error("_node_clear_table failed: info missing after running task for %s", name);
		r = 0;
	}

	dm_task_destroy(dmt);

	return r;
}

struct dm_tree_node *dm_tree_add_new_dev(struct dm_tree *dtree,
					    const char *name,
					    const char *uuid,
					    uint32_t major, uint32_t minor,
					    int read_only,
					    int clear_inactive,
					    void *context)
{
	struct dm_tree_node *dnode;
	struct dm_info info;
	const char *name2;
	const char *uuid2;

	/* Do we need to add node to tree? */
	if (!(dnode = dm_tree_find_node_by_uuid(dtree, uuid))) {
		if (!(name2 = dm_pool_strdup(dtree->mem, name))) {
			log_error("name pool_strdup failed");
			return NULL;
		}
		if (!(uuid2 = dm_pool_strdup(dtree->mem, uuid))) {
			log_error("uuid pool_strdup failed");
			return NULL;
		}

		info.major = 0;
		info.minor = 0;
		info.exists = 0;
		info.live_table = 0;
		info.inactive_table = 0;
		info.read_only = 0;

		if (!(dnode = _create_dm_tree_node(dtree, name2, uuid2,
						   &info, context)))
			return_NULL;

		/* Attach to root node until a table is supplied */
		if (!_add_to_toplevel(dnode) || !_add_to_bottomlevel(dnode))
			return_NULL;

		dnode->props.major = major;
		dnode->props.minor = minor;
		dnode->props.new_name = NULL;
		dnode->props.size_changed = 0;
	} else if (strcmp(name, dnode->name)) {
		/* Do we need to rename node? */
		if (!(dnode->props.new_name = dm_pool_strdup(dtree->mem, name))) {
			log_error("name pool_strdup failed");
			return 0;
		}
	}

	dnode->props.read_only = read_only ? 1 : 0;
	dnode->props.read_ahead = DM_READ_AHEAD_AUTO;
	dnode->props.read_ahead_flags = 0;

	if (clear_inactive && !_node_clear_table(dnode))
		return_NULL;

	dnode->context = context;

	return dnode;
}

void dm_tree_node_set_read_ahead(struct dm_tree_node *dnode,
				 uint32_t read_ahead,
				 uint32_t read_ahead_flags)
{                          
	dnode->props.read_ahead = read_ahead;
	dnode->props.read_ahead_flags = read_ahead_flags;
}

int dm_tree_add_dev(struct dm_tree *dtree, uint32_t major, uint32_t minor)
{
	return _add_dev(dtree, &dtree->root, major, minor) ? 1 : 0;
}

const char *dm_tree_node_get_name(struct dm_tree_node *node)
{
	return node->info.exists ? node->name : "";
}

const char *dm_tree_node_get_uuid(struct dm_tree_node *node)
{
	return node->info.exists ? node->uuid : "";
}

const struct dm_info *dm_tree_node_get_info(struct dm_tree_node *node)
{
	return &node->info;
}

void *dm_tree_node_get_context(struct dm_tree_node *node)
{
	return node->context;
}

int dm_tree_node_num_children(struct dm_tree_node *node, uint32_t inverted)
{
	if (inverted) {
		if (_nodes_are_linked(&node->dtree->root, node))
			return 0;
		return dm_list_size(&node->used_by);
	}

	if (_nodes_are_linked(node, &node->dtree->root))
		return 0;

	return dm_list_size(&node->uses);
}

/*
 * Returns 1 if no prefix supplied
 */
static int _uuid_prefix_matches(const char *uuid, const char *uuid_prefix, size_t uuid_prefix_len)
{
	if (!uuid_prefix)
		return 1;

	if (!strncmp(uuid, uuid_prefix, uuid_prefix_len))
		return 1;

	/* Handle transition: active device uuids might be missing the prefix */
	if (uuid_prefix_len <= 4)
		return 0;

	if (!strncmp(uuid, UUID_PREFIX, sizeof(UUID_PREFIX) - 1))
		return 0;

	if (strncmp(uuid_prefix, UUID_PREFIX, sizeof(UUID_PREFIX) - 1))
		return 0;

	if (!strncmp(uuid, uuid_prefix + sizeof(UUID_PREFIX) - 1, uuid_prefix_len - (sizeof(UUID_PREFIX) - 1)))
		return 1;

	return 0;
}

/*
 * Returns 1 if no children.
 */
static int _children_suspended(struct dm_tree_node *node,
			       uint32_t inverted,
			       const char *uuid_prefix,
			       size_t uuid_prefix_len)
{
	struct dm_list *list;
	struct dm_tree_link *dlink;
	const struct dm_info *dinfo;
	const char *uuid;

	if (inverted) {
		if (_nodes_are_linked(&node->dtree->root, node))
			return 1;
		list = &node->used_by;
	} else {
		if (_nodes_are_linked(node, &node->dtree->root))
			return 1;
		list = &node->uses;
	}

	dm_list_iterate_items(dlink, list) {
		if (!(uuid = dm_tree_node_get_uuid(dlink->node))) {
			stack;
			continue;
		}

		/* Ignore if it doesn't belong to this VG */
		if (!_uuid_prefix_matches(uuid, uuid_prefix, uuid_prefix_len))
			continue;

		if (!(dinfo = dm_tree_node_get_info(dlink->node))) {
			stack;	/* FIXME Is this normal? */
			return 0;
		}

		if (!dinfo->suspended)
			return 0;
	}

	return 1;
}

/*
 * Set major and minor to zero for root of tree.
 */
struct dm_tree_node *dm_tree_find_node(struct dm_tree *dtree,
					  uint32_t major,
					  uint32_t minor)
{
	if (!major && !minor)
		return &dtree->root;

	return _find_dm_tree_node(dtree, major, minor);
}

/*
 * Set uuid to NULL for root of tree.
 */
struct dm_tree_node *dm_tree_find_node_by_uuid(struct dm_tree *dtree,
						  const char *uuid)
{
	if (!uuid || !*uuid)
		return &dtree->root;

	return _find_dm_tree_node_by_uuid(dtree, uuid);
}

/*
 * First time set *handle to NULL.
 * Set inverted to invert the tree.
 */
struct dm_tree_node *dm_tree_next_child(void **handle,
					   struct dm_tree_node *parent,
					   uint32_t inverted)
{
	struct dm_list **dlink = (struct dm_list **) handle;
	struct dm_list *use_list;

	if (inverted)
		use_list = &parent->used_by;
	else
		use_list = &parent->uses;

	if (!*dlink)
		*dlink = dm_list_first(use_list);
	else
		*dlink = dm_list_next(use_list, *dlink);

	return (*dlink) ? dm_list_item(*dlink, struct dm_tree_link)->node : NULL;
}

/*
 * Deactivate a device with its dependencies if the uuid prefix matches.
 */
static int _info_by_dev(uint32_t major, uint32_t minor, int with_open_count,
			struct dm_info *info)
{
	struct dm_task *dmt;
	int r;

	if (!(dmt = dm_task_create(DM_DEVICE_INFO))) {
		log_error("_info_by_dev: dm_task creation failed");
		return 0;
	}

	if (!dm_task_set_major(dmt, major) || !dm_task_set_minor(dmt, minor)) {
		log_error("_info_by_dev: Failed to set device number");
		dm_task_destroy(dmt);
		return 0;
	}

	if (!with_open_count && !dm_task_no_open_count(dmt))
		log_error("Failed to disable open_count");

	if ((r = dm_task_run(dmt)))
		r = dm_task_get_info(dmt, info);

	dm_task_destroy(dmt);

	return r;
}

static int _deactivate_node(const char *name, uint32_t major, uint32_t minor)
{
	struct dm_task *dmt;
	int r;

	log_verbose("Removing %s (%" PRIu32 ":%" PRIu32 ")", name, major, minor);

	if (!(dmt = dm_task_create(DM_DEVICE_REMOVE))) {
		log_error("Deactivation dm_task creation failed for %s", name);
		return 0;
	}

	if (!dm_task_set_major(dmt, major) || !dm_task_set_minor(dmt, minor)) {
		log_error("Failed to set device number for %s deactivation", name);
		dm_task_destroy(dmt);
		return 0;
	}

	if (!dm_task_no_open_count(dmt))
		log_error("Failed to disable open_count");

	r = dm_task_run(dmt);

	/* FIXME Until kernel returns actual name so dm-ioctl.c can handle it */
	rm_dev_node(name);

	/* FIXME Remove node from tree or mark invalid? */

	dm_task_destroy(dmt);

	return r;
}

static int _rename_node(const char *old_name, const char *new_name, uint32_t major, uint32_t minor)
{
	struct dm_task *dmt;
	int r = 0;

	log_verbose("Renaming %s (%" PRIu32 ":%" PRIu32 ") to %s", old_name, major, minor, new_name);

	if (!(dmt = dm_task_create(DM_DEVICE_RENAME))) {
		log_error("Rename dm_task creation failed for %s", old_name);
		return 0;
	}

	if (!dm_task_set_name(dmt, old_name)) {
		log_error("Failed to set name for %s rename.", old_name);
		goto out;
	}

	if (!dm_task_set_newname(dmt, new_name))
                goto_out;

	if (!dm_task_no_open_count(dmt))
		log_error("Failed to disable open_count");

	r = dm_task_run(dmt);

out:
	dm_task_destroy(dmt);

	return r;
}

/* FIXME Merge with _suspend_node? */
static int _resume_node(const char *name, uint32_t major, uint32_t minor,
			uint32_t read_ahead, uint32_t read_ahead_flags,
			struct dm_info *newinfo)
{
	struct dm_task *dmt;
	int r;

	log_verbose("Resuming %s (%" PRIu32 ":%" PRIu32 ")", name, major, minor);

	if (!(dmt = dm_task_create(DM_DEVICE_RESUME))) {
		log_error("Suspend dm_task creation failed for %s", name);
		return 0;
	}

	/* FIXME Kernel should fill in name on return instead */
	if (!dm_task_set_name(dmt, name)) {
		log_error("Failed to set readahead device name for %s", name);
		dm_task_destroy(dmt);
		return 0;
	}

	if (!dm_task_set_major(dmt, major) || !dm_task_set_minor(dmt, minor)) {
		log_error("Failed to set device number for %s resumption.", name);
		dm_task_destroy(dmt);
		return 0;
	}

	if (!dm_task_no_open_count(dmt))
		log_error("Failed to disable open_count");

	if (!dm_task_set_read_ahead(dmt, read_ahead, read_ahead_flags))
		log_error("Failed to set read ahead");

	if ((r = dm_task_run(dmt)))
		r = dm_task_get_info(dmt, newinfo);

	dm_task_destroy(dmt);

	return r;
}

static int _suspend_node(const char *name, uint32_t major, uint32_t minor,
			 int skip_lockfs, int no_flush, struct dm_info *newinfo)
{
	struct dm_task *dmt;
	int r;

	log_verbose("Suspending %s (%" PRIu32 ":%" PRIu32 ")%s%s",
		    name, major, minor,
		    skip_lockfs ? "" : " with filesystem sync",
		    no_flush ? "" : " with device flush");

	if (!(dmt = dm_task_create(DM_DEVICE_SUSPEND))) {
		log_error("Suspend dm_task creation failed for %s", name);
		return 0;
	}

	if (!dm_task_set_major(dmt, major) || !dm_task_set_minor(dmt, minor)) {
		log_error("Failed to set device number for %s suspension.", name);
		dm_task_destroy(dmt);
		return 0;
	}

	if (!dm_task_no_open_count(dmt))
		log_error("Failed to disable open_count");

	if (skip_lockfs && !dm_task_skip_lockfs(dmt))
		log_error("Failed to set skip_lockfs flag.");

	if (no_flush && !dm_task_no_flush(dmt))
		log_error("Failed to set no_flush flag.");

	if ((r = dm_task_run(dmt)))
		r = dm_task_get_info(dmt, newinfo);

	dm_task_destroy(dmt);

	return r;
}

int dm_tree_deactivate_children(struct dm_tree_node *dnode,
				   const char *uuid_prefix,
				   size_t uuid_prefix_len)
{
	void *handle = NULL;
	struct dm_tree_node *child = dnode;
	struct dm_info info;
	const struct dm_info *dinfo;
	const char *name;
	const char *uuid;

	while ((child = dm_tree_next_child(&handle, dnode, 0))) {
		if (!(dinfo = dm_tree_node_get_info(child))) {
			stack;
			continue;
		}

		if (!(name = dm_tree_node_get_name(child))) {
			stack;
			continue;
		}

		if (!(uuid = dm_tree_node_get_uuid(child))) {
			stack;
			continue;
		}

		/* Ignore if it doesn't belong to this VG */
		if (!_uuid_prefix_matches(uuid, uuid_prefix, uuid_prefix_len))
			continue;

		/* Refresh open_count */
		if (!_info_by_dev(dinfo->major, dinfo->minor, 1, &info) ||
		    !info.exists || info.open_count)
			continue;

		if (!_deactivate_node(name, info.major, info.minor)) {
			log_error("Unable to deactivate %s (%" PRIu32
				  ":%" PRIu32 ")", name, info.major,
				  info.minor);
			continue;
		}

		if (dm_tree_node_num_children(child, 0))
			dm_tree_deactivate_children(child, uuid_prefix, uuid_prefix_len);
	}

	return 1;
}

void dm_tree_skip_lockfs(struct dm_tree_node *dnode)
{
	dnode->dtree->skip_lockfs = 1;
}

void dm_tree_use_no_flush_suspend(struct dm_tree_node *dnode)
{
	dnode->dtree->no_flush = 1;
}

int dm_tree_suspend_children(struct dm_tree_node *dnode,
				   const char *uuid_prefix,
				   size_t uuid_prefix_len)
{
	void *handle = NULL;
	struct dm_tree_node *child = dnode;
	struct dm_info info, newinfo;
	const struct dm_info *dinfo;
	const char *name;
	const char *uuid;

	/* Suspend nodes at this level of the tree */
	while ((child = dm_tree_next_child(&handle, dnode, 0))) {
		if (!(dinfo = dm_tree_node_get_info(child))) {
			stack;
			continue;
		}

		if (!(name = dm_tree_node_get_name(child))) {
			stack;
			continue;
		}

		if (!(uuid = dm_tree_node_get_uuid(child))) {
			stack;
			continue;
		}

		/* Ignore if it doesn't belong to this VG */
		if (!_uuid_prefix_matches(uuid, uuid_prefix, uuid_prefix_len))
			continue;

		/* Ensure immediate parents are already suspended */
		if (!_children_suspended(child, 1, uuid_prefix, uuid_prefix_len))
			continue;

		if (!_info_by_dev(dinfo->major, dinfo->minor, 0, &info) ||
		    !info.exists || info.suspended)
			continue;

		if (!_suspend_node(name, info.major, info.minor,
				   child->dtree->skip_lockfs,
				   child->dtree->no_flush, &newinfo)) {
			log_error("Unable to suspend %s (%" PRIu32
				  ":%" PRIu32 ")", name, info.major,
				  info.minor);
			continue;
		}

		/* Update cached info */
		child->info = newinfo;
	}

	/* Then suspend any child nodes */
	handle = NULL;

	while ((child = dm_tree_next_child(&handle, dnode, 0))) {
		if (!(uuid = dm_tree_node_get_uuid(child))) {
			stack;
			continue;
		}

		/* Ignore if it doesn't belong to this VG */
		if (!_uuid_prefix_matches(uuid, uuid_prefix, uuid_prefix_len))
			continue;

		if (dm_tree_node_num_children(child, 0))
			dm_tree_suspend_children(child, uuid_prefix, uuid_prefix_len);
	}

	return 1;
}

int dm_tree_activate_children(struct dm_tree_node *dnode,
				 const char *uuid_prefix,
				 size_t uuid_prefix_len)
{
	void *handle = NULL;
	struct dm_tree_node *child = dnode;
	struct dm_info newinfo;
	const char *name;
	const char *uuid;
	int priority;

	/* Activate children first */
	while ((child = dm_tree_next_child(&handle, dnode, 0))) {
		if (!(uuid = dm_tree_node_get_uuid(child))) {
			stack;
			continue;
		}

		if (!_uuid_prefix_matches(uuid, uuid_prefix, uuid_prefix_len))
			continue;

		if (dm_tree_node_num_children(child, 0))
			dm_tree_activate_children(child, uuid_prefix, uuid_prefix_len);
	}

	handle = NULL;

	for (priority = 0; priority < 2; priority++) {
		while ((child = dm_tree_next_child(&handle, dnode, 0))) {
			if (!(uuid = dm_tree_node_get_uuid(child))) {
				stack;
				continue;
			}

			if (!_uuid_prefix_matches(uuid, uuid_prefix, uuid_prefix_len))
				continue;

			if (priority != child->activation_priority)
				continue;

			if (!(name = dm_tree_node_get_name(child))) {
				stack;
				continue;
			}

			/* Rename? */
			if (child->props.new_name) {
				if (!_rename_node(name, child->props.new_name, child->info.major, child->info.minor)) {
					log_error("Failed to rename %s (%" PRIu32
						  ":%" PRIu32 ") to %s", name, child->info.major,
						  child->info.minor, child->props.new_name);
					return 0;
				}
				child->name = child->props.new_name;
				child->props.new_name = NULL;
			}

			if (!child->info.inactive_table && !child->info.suspended)
				continue;

			if (!_resume_node(child->name, child->info.major, child->info.minor,
					  child->props.read_ahead,
					  child->props.read_ahead_flags, &newinfo)) {
				log_error("Unable to resume %s (%" PRIu32
					  ":%" PRIu32 ")", child->name, child->info.major,
					  child->info.minor);
				continue;
			}

			/* Update cached info */
			child->info = newinfo;
		}
	}

	handle = NULL;

	return 1;
}

static int _create_node(struct dm_tree_node *dnode)
{
	int r = 0;
	struct dm_task *dmt;

	log_verbose("Creating %s", dnode->name);

	if (!(dmt = dm_task_create(DM_DEVICE_CREATE))) {
		log_error("Create dm_task creation failed for %s", dnode->name);
		return 0;
	}

	if (!dm_task_set_name(dmt, dnode->name)) {
		log_error("Failed to set device name for %s", dnode->name);
		goto out;
	}

	if (!dm_task_set_uuid(dmt, dnode->uuid)) {
		log_error("Failed to set uuid for %s", dnode->name);
		goto out;
	}

	if (dnode->props.major &&
	    (!dm_task_set_major(dmt, dnode->props.major) ||
	     !dm_task_set_minor(dmt, dnode->props.minor))) {
		log_error("Failed to set device number for %s creation.", dnode->name);
		goto out;
	}

	if (dnode->props.read_only && !dm_task_set_ro(dmt)) {
		log_error("Failed to set read only flag for %s", dnode->name);
		goto out;
	}

	if (!dm_task_no_open_count(dmt))
		log_error("Failed to disable open_count");

	if ((r = dm_task_run(dmt)))
		r = dm_task_get_info(dmt, &dnode->info);

out:
	dm_task_destroy(dmt);

	return r;
}


static int _build_dev_string(char *devbuf, size_t bufsize, struct dm_tree_node *node)
{
	if (!dm_format_dev(devbuf, bufsize, node->info.major, node->info.minor)) {
                log_error("Failed to format %s device number for %s as dm "
                          "target (%u,%u)",
                          node->name, node->uuid, node->info.major, node->info.minor);
                return 0;
	}

	return 1;
}

/* simplify string emiting code */
#define EMIT_PARAMS(p, str...)\
do {\
	int w;\
	if ((w = dm_snprintf(params + p, paramsize - (size_t) p, str)) < 0) {\
		stack; /* Out of space */\
		return -1;\
	}\
	p += w;\
} while (0)

static int _emit_areas_line(struct dm_task *dmt __attribute((unused)),
			    struct load_segment *seg, char *params,
			    size_t paramsize, int *pos)
{
	struct seg_area *area;
	char devbuf[DM_FORMAT_DEV_BUFSIZE];

	dm_list_iterate_items(area, &seg->areas) {
		if (!_build_dev_string(devbuf, sizeof(devbuf), area->dev_node))
			return_0;

		EMIT_PARAMS(*pos, " %s %" PRIu64, devbuf, area->offset);
	}

	return 1;
}

static int _emit_segment_line(struct dm_task *dmt, struct load_segment *seg, uint64_t *seg_start, char *params, size_t paramsize)
{
	unsigned log_parm_count;
	int pos = 0;
	int r;
	char originbuf[DM_FORMAT_DEV_BUFSIZE], cowbuf[DM_FORMAT_DEV_BUFSIZE];
	char logbuf[DM_FORMAT_DEV_BUFSIZE];
	const char *logtype;

	switch(seg->type) {
	case SEG_ERROR:
	case SEG_ZERO:
	case SEG_LINEAR:
		break;
	case SEG_MIRRORED:
		log_parm_count = 1;	/* Region size */
		log_parm_count += hweight32(seg->flags);	/* [no]sync, block_on_error etc. */

		if (seg->flags & DM_CORELOG)
			log_parm_count--;   /* DM_CORELOG does not count in the param list */

		if (seg->clustered) {
			if (seg->uuid)
				log_parm_count++;
			EMIT_PARAMS(pos, "clustered-");
		}

		if (!seg->log)
			logtype = "core";
		else {
			logtype = "disk";
			log_parm_count++;
			if (!_build_dev_string(logbuf, sizeof(logbuf), seg->log))
				return_0;
		}

		EMIT_PARAMS(pos, "%s %u", logtype, log_parm_count);

		if (seg->log)
			EMIT_PARAMS(pos, " %s", logbuf);

		EMIT_PARAMS(pos, " %u", seg->region_size);

		if (seg->clustered && seg->uuid)
			EMIT_PARAMS(pos, " %s", seg->uuid);

		if ((seg->flags & DM_NOSYNC))
			EMIT_PARAMS(pos, " nosync");
		else if ((seg->flags & DM_FORCESYNC))
			EMIT_PARAMS(pos, " sync");

		if ((seg->flags & DM_BLOCK_ON_ERROR))
			EMIT_PARAMS(pos, " block_on_error");

		EMIT_PARAMS(pos, " %u", seg->mirror_area_count);

		break;
	case SEG_SNAPSHOT:
		if (!_build_dev_string(originbuf, sizeof(originbuf), seg->origin))
			return_0;
		if (!_build_dev_string(cowbuf, sizeof(cowbuf), seg->cow))
			return_0;
		EMIT_PARAMS(pos, "%s %s %c %d", originbuf, cowbuf,
			    seg->persistent ? 'P' : 'N', seg->chunk_size);
		break;
	case SEG_SNAPSHOT_ORIGIN:
		if (!_build_dev_string(originbuf, sizeof(originbuf), seg->origin))
			return_0;
		EMIT_PARAMS(pos, "%s", originbuf);
		break;
	case SEG_STRIPED:
		EMIT_PARAMS(pos, "%u %u", seg->area_count, seg->stripe_size);
		break;
	}

	switch(seg->type) {
	case SEG_ERROR:
	case SEG_SNAPSHOT:
	case SEG_SNAPSHOT_ORIGIN:
	case SEG_ZERO:
		break;
	case SEG_LINEAR:
	case SEG_MIRRORED:
	case SEG_STRIPED:
		if ((r = _emit_areas_line(dmt, seg, params, paramsize, &pos)) <= 0) {
			stack;
			return r;
		}
		break;
	}

	log_debug("Adding target: %" PRIu64 " %" PRIu64 " %s %s",
		  *seg_start, seg->size, dm_segtypes[seg->type].target, params);

	if (!dm_task_add_target(dmt, *seg_start, seg->size, dm_segtypes[seg->type].target, params))
		return_0;

	*seg_start += seg->size;

	return 1;
}

#undef EMIT_PARAMS

static int _emit_segment(struct dm_task *dmt, struct load_segment *seg,
			 uint64_t *seg_start)
{
	char *params;
	size_t paramsize = 4096;
	int ret;

	do {
		if (!(params = dm_malloc(paramsize))) {
			log_error("Insufficient space for target parameters.");
			return 0;
		}

		params[0] = '\0';
		ret = _emit_segment_line(dmt, seg, seg_start, params, paramsize);
		dm_free(params);

		if (!ret)
			stack;

		if (ret >= 0)
			return ret;

		log_debug("Insufficient space in params[%" PRIsize_t
			  "] for target parameters.", paramsize);

		paramsize *= 2;
	} while (paramsize < MAX_TARGET_PARAMSIZE);

	log_error("Target parameter size too big. Aborting.");
	return 0;
}

static int _load_node(struct dm_tree_node *dnode)
{
	int r = 0;
	struct dm_task *dmt;
	struct load_segment *seg;
	uint64_t seg_start = 0;

	log_verbose("Loading %s table", dnode->name);

	if (!(dmt = dm_task_create(DM_DEVICE_RELOAD))) {
		log_error("Reload dm_task creation failed for %s", dnode->name);
		return 0;
	}

	if (!dm_task_set_major(dmt, dnode->info.major) ||
	    !dm_task_set_minor(dmt, dnode->info.minor)) {
		log_error("Failed to set device number for %s reload.", dnode->name);
		goto out;
	}

	if (dnode->props.read_only && !dm_task_set_ro(dmt)) {
		log_error("Failed to set read only flag for %s", dnode->name);
		goto out;
	}

	if (!dm_task_no_open_count(dmt))
		log_error("Failed to disable open_count");

	dm_list_iterate_items(seg, &dnode->props.segs)
		if (!_emit_segment(dmt, seg, &seg_start))
			goto_out;

	if (!dm_task_suppress_identical_reload(dmt))
		log_error("Failed to suppress reload of identical tables.");

	if ((r = dm_task_run(dmt))) {
		r = dm_task_get_info(dmt, &dnode->info);
		if (r && !dnode->info.inactive_table)
			log_verbose("Suppressed %s identical table reload.",
				    dnode->name);

		if ((dnode->props.size_changed =
		     (dm_task_get_existing_table_size(dmt) == seg_start) ? 0 : 1))
			log_debug("Table size changed from %" PRIu64 " to %"
				  PRIu64 " for %s",
				  dm_task_get_existing_table_size(dmt),
				  seg_start, dnode->name);
	}

	dnode->props.segment_count = 0;

out:
	dm_task_destroy(dmt);

	return r;
}

int dm_tree_preload_children(struct dm_tree_node *dnode,
			     const char *uuid_prefix,
			     size_t uuid_prefix_len)
{
	void *handle = NULL;
	struct dm_tree_node *child;
	struct dm_info newinfo;

	/* Preload children first */
	while ((child = dm_tree_next_child(&handle, dnode, 0))) {
		/* Skip existing non-device-mapper devices */
		if (!child->info.exists && child->info.major)
			continue;

		/* Ignore if it doesn't belong to this VG */
		if (child->info.exists &&
		    !_uuid_prefix_matches(child->uuid, uuid_prefix, uuid_prefix_len))
			continue;

		if (dm_tree_node_num_children(child, 0))
			dm_tree_preload_children(child, uuid_prefix, uuid_prefix_len);

		/* FIXME Cope if name exists with no uuid? */
		if (!child->info.exists) {
			if (!_create_node(child)) {
				stack;
				return 0;
			}
		}

		if (!child->info.inactive_table && child->props.segment_count) {
			if (!_load_node(child)) {
				stack;
				return 0;
			}
		}

		/* Resume device immediately if it has parents and its size changed */
		if (!dm_tree_node_num_children(child, 1) || !child->props.size_changed)
			continue;

		if (!child->info.inactive_table && !child->info.suspended)
			continue;

		if (!_resume_node(child->name, child->info.major, child->info.minor,
				  child->props.read_ahead,
				  child->props.read_ahead_flags, &newinfo)) {
			log_error("Unable to resume %s (%" PRIu32
				  ":%" PRIu32 ")", child->name, child->info.major,
				  child->info.minor);
			continue;
		}

		/* Update cached info */
		child->info = newinfo;
	}

	handle = NULL;

	return 1;
}

/*
 * Returns 1 if unsure.
 */
int dm_tree_children_use_uuid(struct dm_tree_node *dnode,
				 const char *uuid_prefix,
				 size_t uuid_prefix_len)
{
	void *handle = NULL;
	struct dm_tree_node *child = dnode;
	const char *uuid;

	while ((child = dm_tree_next_child(&handle, dnode, 0))) {
		if (!(uuid = dm_tree_node_get_uuid(child))) {
			log_error("Failed to get uuid for dtree node.");
			return 1;
		}

		if (_uuid_prefix_matches(uuid, uuid_prefix, uuid_prefix_len))
			return 1;

		if (dm_tree_node_num_children(child, 0))
			dm_tree_children_use_uuid(child, uuid_prefix, uuid_prefix_len);
	}

	return 0;
}

/*
 * Target functions
 */
static struct load_segment *_add_segment(struct dm_tree_node *dnode, unsigned type, uint64_t size)
{
	struct load_segment *seg;

	if (!(seg = dm_pool_zalloc(dnode->dtree->mem, sizeof(*seg)))) {
		log_error("dtree node segment allocation failed");
		return NULL;
	}

	seg->type = type;
	seg->size = size;
	seg->area_count = 0;
	dm_list_init(&seg->areas);
	seg->stripe_size = 0;
	seg->persistent = 0;
	seg->chunk_size = 0;
	seg->cow = NULL;
	seg->origin = NULL;

	dm_list_add(&dnode->props.segs, &seg->list);
	dnode->props.segment_count++;

	return seg;
}

int dm_tree_node_add_snapshot_origin_target(struct dm_tree_node *dnode,
                                               uint64_t size,
                                               const char *origin_uuid)
{
	struct load_segment *seg;
	struct dm_tree_node *origin_node;

	if (!(seg = _add_segment(dnode, SEG_SNAPSHOT_ORIGIN, size)))
		return_0;

	if (!(origin_node = dm_tree_find_node_by_uuid(dnode->dtree, origin_uuid))) {
		log_error("Couldn't find snapshot origin uuid %s.", origin_uuid);
		return 0;
	}

	seg->origin = origin_node;
	if (!_link_tree_nodes(dnode, origin_node))
		return_0;

	/* Resume snapshot origins after new snapshots */
	dnode->activation_priority = 1;

	return 1;
}

int dm_tree_node_add_snapshot_target(struct dm_tree_node *node,
                                        uint64_t size,
                                        const char *origin_uuid,
                                        const char *cow_uuid,
                                        int persistent,
                                        uint32_t chunk_size)
{
	struct load_segment *seg;
	struct dm_tree_node *origin_node, *cow_node;

	if (!(seg = _add_segment(node, SEG_SNAPSHOT, size)))
		return_0;

	if (!(origin_node = dm_tree_find_node_by_uuid(node->dtree, origin_uuid))) {
		log_error("Couldn't find snapshot origin uuid %s.", origin_uuid);
		return 0;
	}

	seg->origin = origin_node;
	if (!_link_tree_nodes(node, origin_node))
		return_0;

	if (!(cow_node = dm_tree_find_node_by_uuid(node->dtree, cow_uuid))) {
		log_error("Couldn't find snapshot origin uuid %s.", cow_uuid);
		return 0;
	}

	seg->cow = cow_node;
	if (!_link_tree_nodes(node, cow_node))
		return_0;

	seg->persistent = persistent ? 1 : 0;
	seg->chunk_size = chunk_size;

	return 1;
}

int dm_tree_node_add_error_target(struct dm_tree_node *node,
                                     uint64_t size)
{
	if (!_add_segment(node, SEG_ERROR, size))
		return_0;

	return 1;
}

int dm_tree_node_add_zero_target(struct dm_tree_node *node,
                                    uint64_t size)
{
	if (!_add_segment(node, SEG_ZERO, size))
		return_0;

	return 1;
}

int dm_tree_node_add_linear_target(struct dm_tree_node *node,
                                      uint64_t size)
{
	if (!_add_segment(node, SEG_LINEAR, size))
		return_0;

	return 1;
}

int dm_tree_node_add_striped_target(struct dm_tree_node *node,
                                       uint64_t size,
                                       uint32_t stripe_size)
{
	struct load_segment *seg;

	if (!(seg = _add_segment(node, SEG_STRIPED, size)))
		return_0;

	seg->stripe_size = stripe_size;

	return 1;
}

int dm_tree_node_add_mirror_target_log(struct dm_tree_node *node,
					  uint32_t region_size,
					  unsigned clustered, 
					  const char *log_uuid,
					  unsigned area_count,
					  uint32_t flags)
{
	struct dm_tree_node *log_node = NULL;
	struct load_segment *seg;

	if (!node->props.segment_count) {
		log_error("Internal error: Attempt to add target area to missing segment.");
		return 0;
	}

	seg = dm_list_item(dm_list_last(&node->props.segs), struct load_segment);

	if (log_uuid) {
		if (!(seg->uuid = dm_pool_strdup(node->dtree->mem, log_uuid))) {
			log_error("log uuid pool_strdup failed");
			return 0;
		}
		if (!(flags & DM_CORELOG)) {
			if (!(log_node = dm_tree_find_node_by_uuid(node->dtree, log_uuid))) {
				log_error("Couldn't find mirror log uuid %s.", log_uuid);
				return 0;
			}

			if (!_link_tree_nodes(node, log_node))
				return_0;
		}
	}

	seg->log = log_node;
	seg->region_size = region_size;
	seg->clustered = clustered;
	seg->mirror_area_count = area_count;
	seg->flags = flags;

	return 1;
}

int dm_tree_node_add_mirror_target(struct dm_tree_node *node,
                                      uint64_t size)
{
	struct load_segment *seg;

	if (!(seg = _add_segment(node, SEG_MIRRORED, size)))
		return_0;

	return 1;
}

static int _add_area(struct dm_tree_node *node, struct load_segment *seg, struct dm_tree_node *dev_node, uint64_t offset)
{
	struct seg_area *area;

	if (!(area = dm_pool_zalloc(node->dtree->mem, sizeof (*area)))) {
		log_error("Failed to allocate target segment area.");
		return 0;
	}

	area->dev_node = dev_node;
	area->offset = offset;

	dm_list_add(&seg->areas, &area->list);
	seg->area_count++;

	return 1;
}

int dm_tree_node_add_target_area(struct dm_tree_node *node,
                                    const char *dev_name,
                                    const char *uuid,
                                    uint64_t offset)
{
	struct load_segment *seg;
	struct stat info;
	struct dm_tree_node *dev_node;

	if ((!dev_name || !*dev_name) && (!uuid || !*uuid)) {
		log_error("dm_tree_node_add_target_area called without device");
		return 0;
	}

	if (uuid) {
		if (!(dev_node = dm_tree_find_node_by_uuid(node->dtree, uuid))) {
			log_error("Couldn't find area uuid %s.", uuid);
			return 0;
		}
		if (!_link_tree_nodes(node, dev_node))
			return_0;
	} else {
        	if (stat(dev_name, &info) < 0) {
			log_error("Device %s not found.", dev_name);
			return 0;
		}
#ifndef __NetBSD__
        	if (!S_ISBLK(info.st_mode)) {
			log_error("Device %s is not a block device.", dev_name);
			return 0;
		}
#else
		if (S_ISBLK(info.st_mode)) {
			log_error("Device %s is a block device. Use raw devices on NetBSD.", dev_name);
			return 0;
		}
#endif		
		/* FIXME Check correct macro use */
		if (!(dev_node = _add_dev(node->dtree, node, MAJOR(info.st_rdev), MINOR(info.st_rdev))))
			return_0;
	}

	if (!node->props.segment_count) {
		log_error("Internal error: Attempt to add target area to missing segment.");
		return 0;
	}

	seg = dm_list_item(dm_list_last(&node->props.segs), struct load_segment);

	if (!_add_area(node, seg, dev_node, offset))
		return_0;

	return 1;
}
