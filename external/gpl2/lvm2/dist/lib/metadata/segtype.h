/*	$NetBSD: segtype.h,v 1.1.1.2 2009/12/02 00:26:34 haad Exp $	*/

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

#ifndef _SEGTYPES_H
#define _SEGTYPES_H

#include "metadata-exported.h"

struct segtype_handler;
struct cmd_context;
struct config_tree;
struct lv_segment;
struct formatter;
struct config_node;
struct dev_manager;

/* Feature flags */
#define SEG_CAN_SPLIT		0x00000001U
#define SEG_AREAS_STRIPED	0x00000002U
#define SEG_AREAS_MIRRORED	0x00000004U
#define SEG_SNAPSHOT		0x00000008U
#define SEG_FORMAT1_SUPPORT	0x00000010U
#define SEG_VIRTUAL		0x00000020U
#define SEG_CANNOT_BE_ZEROED	0x00000040U
#define SEG_MONITORED		0x00000080U
#define SEG_UNKNOWN		0x80000000U

#define seg_is_mirrored(seg)	((seg)->segtype->flags & SEG_AREAS_MIRRORED ? 1 : 0)
#define seg_is_striped(seg)	((seg)->segtype->flags & SEG_AREAS_STRIPED ? 1 : 0)
#define seg_is_snapshot(seg)	((seg)->segtype->flags & SEG_SNAPSHOT ? 1 : 0)
#define seg_is_virtual(seg)	((seg)->segtype->flags & SEG_VIRTUAL ? 1 : 0)
#define seg_can_split(seg)	((seg)->segtype->flags & SEG_CAN_SPLIT ? 1 : 0)
#define seg_cannot_be_zeroed(seg) ((seg)->segtype->flags & SEG_CANNOT_BE_ZEROED ? 1 : 0)
#define seg_monitored(seg)	((seg)->segtype->flags & SEG_MONITORED ? 1 : 0)
#define seg_unknown(seg)	((seg)->segtype->flags & SEG_UNKNOWN ? 1 : 0)

#define segtype_is_striped(segtype)	((segtype)->flags & SEG_AREAS_STRIPED ? 1 : 0)
#define segtype_is_mirrored(segtype)	((segtype)->flags & SEG_AREAS_MIRRORED ? 1 : 0)
#define segtype_is_virtual(segtype)	((segtype)->flags & SEG_VIRTUAL ? 1 : 0)

struct segment_type {
	struct dm_list list;		/* Internal */
	struct cmd_context *cmd;	/* lvm_register_segtype() sets this. */
	uint32_t flags;
	struct segtype_handler *ops;
	const char *name;
	void *library;			/* lvm_register_segtype() sets this. */
	void *private;			/* For the segtype handler to use. */
};

struct segtype_handler {
	const char *(*name) (const struct lv_segment * seg);
	void (*display) (const struct lv_segment * seg);
	int (*text_export) (const struct lv_segment * seg,
			    struct formatter * f);
	int (*text_import_area_count) (struct config_node * sn,
				       uint32_t *area_count);
	int (*text_import) (struct lv_segment * seg,
			    const struct config_node * sn,
			    struct dm_hash_table * pv_hash);
	int (*merge_segments) (struct lv_segment * seg1,
			       struct lv_segment * seg2);
	int (*add_target_line) (struct dev_manager *dm, struct dm_pool *mem,
                                struct cmd_context *cmd, void **target_state,
                                struct lv_segment *seg,
                                struct dm_tree_node *node, uint64_t len,
                                uint32_t *pvmove_mirror_count);
	int (*target_percent) (void **target_state,
			       percent_range_t *percent_range,
			       struct dm_pool * mem,
			       struct cmd_context *cmd,
			       struct lv_segment *seg, char *params,
			       uint64_t *total_numerator,
			       uint64_t *total_denominator);
	int (*target_present) (struct cmd_context *cmd,
			       const struct lv_segment *seg,
			       unsigned *attributes);
	int (*modules_needed) (struct dm_pool *mem,
			       const struct lv_segment *seg,
			       struct dm_list *modules);
	void (*destroy) (const struct segment_type * segtype);
	int (*target_monitored) (struct lv_segment *seg, int *pending);
	int (*target_monitor_events) (struct lv_segment *seg, int events);
	int (*target_unmonitor_events) (struct lv_segment *seg, int events);
};

struct segment_type *get_segtype_from_string(struct cmd_context *cmd,
					     const char *str);

struct segtype_library;
int lvm_register_segtype(struct segtype_library *seglib,
			 struct segment_type *segtype);

struct segment_type *init_striped_segtype(struct cmd_context *cmd);
struct segment_type *init_zero_segtype(struct cmd_context *cmd);
struct segment_type *init_error_segtype(struct cmd_context *cmd);
struct segment_type *init_free_segtype(struct cmd_context *cmd);
struct segment_type *init_unknown_segtype(struct cmd_context *cmd, const char *name);

#ifdef SNAPSHOT_INTERNAL
struct segment_type *init_snapshot_segtype(struct cmd_context *cmd);
#endif

#ifdef MIRRORED_INTERNAL
struct segment_type *init_mirrored_segtype(struct cmd_context *cmd);
#endif

#ifdef CRYPT_INTERNAL
struct segment_type *init_crypt_segtype(struct cmd_context *cmd);
#endif

#endif
