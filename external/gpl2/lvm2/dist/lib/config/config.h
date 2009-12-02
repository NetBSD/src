/*	$NetBSD: config.h,v 1.1.1.2 2009/12/02 00:26:28 haad Exp $	*/

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

#ifndef _LVM_CONFIG_H
#define _LVM_CONFIG_H

#include "lvm-types.h"

struct device;
struct cmd_context;

enum {
	CFG_STRING,
	CFG_FLOAT,
	CFG_INT,
	CFG_EMPTY_ARRAY
};

struct config_value {
	int type;
	union {
		int64_t i;
		float r;
		char *str;
	} v;
	struct config_value *next;	/* for arrays */
};

struct config_node {
	char *key;
	struct config_node *parent, *sib, *child;
	struct config_value *v;
};

struct config_tree {
	struct config_node *root;
};

struct config_tree_list {
	struct dm_list list;
	struct config_tree *cft;
};

struct config_tree *create_config_tree(const char *filename, int keep_open);
struct config_tree *create_config_tree_from_string(struct cmd_context *cmd,
						   const char *config_settings);
int override_config_tree_from_string(struct cmd_context *cmd,
				     const char *config_settings);
void destroy_config_tree(struct config_tree *cft);

typedef uint32_t (*checksum_fn_t) (uint32_t initial, const void *buf, uint32_t size);

int read_config_fd(struct config_tree *cft, struct device *dev,
		   off_t offset, size_t size, off_t offset2, size_t size2,
		   checksum_fn_t checksum_fn, uint32_t checksum);

int read_config_file(struct config_tree *cft);
int write_config_file(struct config_tree *cft, const char *file,
		      int argc, char **argv);

typedef int (*putline_fn)(const char *line, void *baton);
int write_config_node(const struct config_node *cn, putline_fn putline, void *baton);

time_t config_file_timestamp(struct config_tree *cft);
int config_file_changed(struct config_tree *cft);
int merge_config_tree(struct cmd_context *cmd, struct config_tree *cft,
		      struct config_tree *newdata);

struct config_node *find_config_node(const struct config_node *cn,
				     const char *path);
const char *find_config_str(const struct config_node *cn, const char *path,
			    const char *fail);
int find_config_int(const struct config_node *cn, const char *path, int fail);
float find_config_float(const struct config_node *cn, const char *path,
			float fail);

/*
 * These versions check an override tree, if present, first.
 */
struct config_node *find_config_tree_node(struct cmd_context *cmd,
					  const char *path);
const char *find_config_tree_str(struct cmd_context *cmd,
				 const char *path, const char *fail);
int find_config_tree_int(struct cmd_context *cmd, const char *path,
			 int fail);
float find_config_tree_float(struct cmd_context *cmd, const char *path,
			     float fail);

/*
 * Understands (0, ~0), (y, n), (yes, no), (on,
 * off), (true, false).
 */
int find_config_bool(const struct config_node *cn, const char *path, int fail);
int find_config_tree_bool(struct cmd_context *cmd, const char *path, int fail);

int get_config_uint32(const struct config_node *cn, const char *path,
		      uint32_t *result);

int get_config_uint64(const struct config_node *cn, const char *path,
		      uint64_t *result);

int get_config_str(const struct config_node *cn, const char *path,
		   char **result);

unsigned maybe_config_section(const char *str, unsigned len);

const char *config_parent_name(const struct config_node *n);

struct config_node *clone_config_node(struct dm_pool *mem, const struct config_node *cn,
				      int siblings);
#endif
