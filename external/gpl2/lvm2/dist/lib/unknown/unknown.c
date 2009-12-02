/*	$NetBSD: unknown.c,v 1.1.1.1 2009/12/02 00:26:20 haad Exp $	*/

/*
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
#include "segtype.h"
#include "display.h"
#include "text_export.h"
#include "text_import.h"
#include "config.h"
#include "str_list.h"
#include "targets.h"
#include "lvm-string.h"
#include "activate.h"
#include "str_list.h"
#include "metadata.h"

static const char *_unknown_name(const struct lv_segment *seg)
{

	return seg->segtype->name;
}

static int _unknown_text_import(struct lv_segment *seg, const struct config_node *sn,
				struct dm_hash_table *pv_hash)
{
	struct config_node *new, *last = NULL, *head = NULL;
	const struct config_node *current;
	log_verbose("importing unknown segment");
	for (current = sn; current != NULL; current = current->sib) {
		if (!strcmp(current->key, "type") || !strcmp(current->key, "start_extent") ||
		    !strcmp(current->key, "tags") || !strcmp(current->key, "extent_count"))
			continue;
		new = clone_config_node(seg->lv->vg->vgmem, current, 0);
		if (!new)
			return_0;
		if (last)
			last->sib = new;
		if (!head)
			head = new;
		last = new;
	}
	seg->segtype_private = head;
	return 1;
}

static int _unknown_text_export(const struct lv_segment *seg, struct formatter *f)
{
	struct config_node *cn = seg->segtype_private;
	return out_config_node(f, cn);
}

#ifdef DEVMAPPER_SUPPORT
static int _unknown_add_target_line(struct dev_manager *dm __attribute((unused)),
				struct dm_pool *mem __attribute((unused)),
				struct cmd_context *cmd __attribute((unused)),
				void **target_state __attribute((unused)),
				struct lv_segment *seg __attribute((unused)),
				struct dm_tree_node *node, uint64_t len,
				uint32_t *pvmove_mirror_count __attribute((unused)))
{
	return dm_tree_node_add_error_target(node, len);
}
#endif

static void _unknown_destroy(const struct segment_type *segtype)
{
	dm_free((void *)segtype);
}

static struct segtype_handler _unknown_ops = {
	.name = _unknown_name,
	.text_import = _unknown_text_import,
	.text_export = _unknown_text_export,
#ifdef DEVMAPPER_SUPPORT
	.add_target_line = _unknown_add_target_line,
#endif
	.destroy = _unknown_destroy,
};

struct segment_type *init_unknown_segtype(struct cmd_context *cmd, const char *name)
{
	struct segment_type *segtype = dm_malloc(sizeof(*segtype));

	if (!segtype)
		return_NULL;

	segtype->cmd = cmd;
	segtype->ops = &_unknown_ops;
	segtype->name = dm_pool_strdup(cmd->mem, name);
	segtype->private = NULL;
	segtype->flags = SEG_UNKNOWN | SEG_VIRTUAL | SEG_CANNOT_BE_ZEROED;

	log_very_verbose("Initialised segtype: %s", segtype->name);

	return segtype;
}
