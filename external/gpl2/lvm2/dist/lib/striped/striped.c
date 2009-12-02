/*	$NetBSD: striped.c,v 1.1.1.2 2009/12/02 00:26:47 haad Exp $	*/

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
#include "segtype.h"
#include "display.h"
#include "text_export.h"
#include "text_import.h"
#include "config.h"
#include "str_list.h"
#include "targets.h"
#include "lvm-string.h"
#include "activate.h"
#include "pv_alloc.h"
#include "metadata.h"

static const char *_striped_name(const struct lv_segment *seg)
{
	return (seg->area_count == 1) ? "linear" : seg->segtype->name;
}

static void _striped_display(const struct lv_segment *seg)
{
	uint32_t s;

	if (seg->area_count == 1)
		display_stripe(seg, 0, "  ");
	else {
		log_print("  Stripes\t\t%u", seg->area_count);

		if (seg->lv->vg->cmd->si_unit_consistency)
			log_print("  Stripe size\t\t%s",
				  display_size(seg->lv->vg->cmd,
					       (uint64_t) seg->stripe_size));
		else
			log_print("  Stripe size\t\t%u KB",
				  seg->stripe_size / 2);

		for (s = 0; s < seg->area_count; s++) {
			log_print("  Stripe %d:", s);
			display_stripe(seg, s, "    ");
		}
	}
	log_print(" ");
}

static int _striped_text_import_area_count(struct config_node *sn, uint32_t *area_count)
{
	if (!get_config_uint32(sn, "stripe_count", area_count)) {
		log_error("Couldn't read 'stripe_count' for "
			  "segment '%s'.", config_parent_name(sn));
		return 0;
	}

	return 1;
}

static int _striped_text_import(struct lv_segment *seg, const struct config_node *sn,
			struct dm_hash_table *pv_hash)
{
	struct config_node *cn;

	if ((seg->area_count != 1) &&
	    !get_config_uint32(sn, "stripe_size", &seg->stripe_size)) {
		log_error("Couldn't read stripe_size for segment %s "
			  "of logical volume %s.", config_parent_name(sn), seg->lv->name);
		return 0;
	}

	if (!(cn = find_config_node(sn, "stripes"))) {
		log_error("Couldn't find stripes array for segment %s "
			  "of logical volume %s.", config_parent_name(sn), seg->lv->name);
		return 0;
	}

	seg->area_len /= seg->area_count;

	return text_import_areas(seg, sn, cn, pv_hash, 0);
}

static int _striped_text_export(const struct lv_segment *seg, struct formatter *f)
{

	outf(f, "stripe_count = %u%s", seg->area_count,
	     (seg->area_count == 1) ? "\t# linear" : "");

	if (seg->area_count > 1)
		out_size(f, (uint64_t) seg->stripe_size,
			 "stripe_size = %u", seg->stripe_size);

	return out_areas(f, seg, "stripe");
}

/*
 * Test whether two segments could be merged by the current merging code
 */
static int _striped_segments_compatible(struct lv_segment *first,
				struct lv_segment *second)
{
	uint32_t width;
	unsigned s;

	if ((first->area_count != second->area_count) ||
	    (first->stripe_size != second->stripe_size))
		return 0;

	for (s = 0; s < first->area_count; s++) {

		/* FIXME Relax this to first area type != second area type */
		/*       plus the additional AREA_LV checks needed */
		if ((seg_type(first, s) != AREA_PV) ||
		    (seg_type(second, s) != AREA_PV))
			return 0;

		width = first->area_len;

		if ((seg_pv(first, s) !=
		     seg_pv(second, s)) ||
		    (seg_pe(first, s) + width !=
		     seg_pe(second, s)))
			return 0;
	}

	if (!str_list_lists_equal(&first->tags, &second->tags))
		return 0;

	return 1;
}

static int _striped_merge_segments(struct lv_segment *seg1, struct lv_segment *seg2)
{
	uint32_t s;

	if (!_striped_segments_compatible(seg1, seg2))
		return 0;

	seg1->len += seg2->len;
	seg1->area_len += seg2->area_len;

	for (s = 0; s < seg1->area_count; s++)
		if (seg_type(seg1, s) == AREA_PV)
			merge_pv_segments(seg_pvseg(seg1, s),
					  seg_pvseg(seg2, s));

	return 1;
}

#ifdef DEVMAPPER_SUPPORT
static int _striped_add_target_line(struct dev_manager *dm,
				struct dm_pool *mem __attribute((unused)),
				struct cmd_context *cmd __attribute((unused)),
				void **target_state __attribute((unused)),
				struct lv_segment *seg,
				struct dm_tree_node *node, uint64_t len,
				uint32_t *pvmove_mirror_count __attribute((unused)))
{
	if (!seg->area_count) {
		log_error("Internal error: striped add_target_line called "
			  "with no areas for %s.", seg->lv->name);
		return 0;
	}
	if (seg->area_count == 1) {
		if (!dm_tree_node_add_linear_target(node, len))
			return_0;
	} else if (!dm_tree_node_add_striped_target(node, len,
						  seg->stripe_size))
		return_0;

	return add_areas_line(dm, seg, node, 0u, seg->area_count);
}

static int _striped_target_present(struct cmd_context *cmd,
				   const struct lv_segment *seg __attribute((unused)),
				   unsigned *attributes __attribute((unused)))
{
	static int _striped_checked = 0;
	static int _striped_present = 0;

	if (!_striped_checked)
		_striped_present = target_present(cmd, "linear", 0) &&
			  target_present(cmd, "striped", 0);

	_striped_checked = 1;

	return _striped_present;
}
#endif

static void _striped_destroy(const struct segment_type *segtype)
{
	dm_free((void *)segtype);
}

static struct segtype_handler _striped_ops = {
	.name = _striped_name,
	.display = _striped_display,
	.text_import_area_count = _striped_text_import_area_count,
	.text_import = _striped_text_import,
	.text_export = _striped_text_export,
	.merge_segments = _striped_merge_segments,
#ifdef DEVMAPPER_SUPPORT
	.add_target_line = _striped_add_target_line,
	.target_present = _striped_target_present,
#endif
	.destroy = _striped_destroy,
};

struct segment_type *init_striped_segtype(struct cmd_context *cmd)
{
	struct segment_type *segtype = dm_malloc(sizeof(*segtype));

	if (!segtype)
		return_NULL;

	segtype->cmd = cmd;
	segtype->ops = &_striped_ops;
	segtype->name = "striped";
	segtype->private = NULL;
	segtype->flags =
	    SEG_CAN_SPLIT | SEG_AREAS_STRIPED | SEG_FORMAT1_SUPPORT;

	log_very_verbose("Initialised segtype: %s", segtype->name);

	return segtype;
}
