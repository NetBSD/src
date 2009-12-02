/*	$NetBSD: toolcontext.h,v 1.1.1.3 2009/12/02 00:25:44 haad Exp $	*/

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

#ifndef _LVM_TOOLCONTEXT_H
#define _LVM_TOOLCONTEXT_H

#include "dev-cache.h"

#include <stdio.h>
#include <limits.h>

/*
 * Config options that can be changed while commands are processed
 */
struct config_info {
	int debug;
	int verbose;
	int test;
	int syslog;
	int activation;
	int suffix;
	int archive;		/* should we archive ? */
	int backup;		/* should we backup ? */
	int read_ahead;		/* DM_READ_AHEAD_NONE or _AUTO */
	int udev_sync;
	int cache_vgmetadata;
	const char *msg_prefix;
	struct format_type *fmt;
	uint64_t unit_factor;
	int cmd_name;		/* Show command name? */
	mode_t umask;
	char unit_type;
	char _padding[1];
};

struct config_tree;
struct archive_params;
struct backup_params;

/* FIXME Split into tool & library contexts */
/* command-instance-related variables needed by library */
struct cmd_context {
	struct dm_pool *libmem;	/* For permanent config data */
	struct dm_pool *mem;	/* Transient: Cleared between each command */

	const struct format_type *fmt;	/* Current format to use by default */
	struct format_type *fmt_backup;	/* Format to use for backups */

	struct dm_list formats;	/* Available formats */
	struct dm_list segtypes;	/* Available segment types */
	const char *hostname;
	const char *kernel_vsn;

	unsigned rand_seed;
	const char *cmd_line;
	struct command *command;
	char **argv;
	unsigned is_long_lived:1;	/* Optimises persistent_filter handling */
	unsigned handles_missing_pvs:1;
	unsigned handles_unknown_segments:1;
	unsigned partial_activation:1;
	unsigned si_unit_consistency:1;

	struct dev_filter *filter;
	int dump_filter;	/* Dump filter when exiting? */

	struct dm_list config_files;
	int config_valid;
	struct config_tree *cft;
	struct config_tree *cft_override;
	struct config_info default_settings;
	struct config_info current_settings;

	struct archive_params *archive_params;
	struct backup_params *backup_params;
	const char *stripe_filler;

	/* List of defined tags */
	struct dm_list tags;
	int hosttags;

	char system_dir[PATH_MAX];
	char dev_dir[PATH_MAX];
	char proc_dir[PATH_MAX];
	char sysfs_dir[PATH_MAX];
};

/*
 * system_dir may be NULL to use the default value.
 * The environment variable LVM_SYSTEM_DIR always takes precedence.
 */
struct cmd_context *create_toolcontext(unsigned is_long_lived,
				       const char *system_dir);
void destroy_toolcontext(struct cmd_context *cmd);
int refresh_toolcontext(struct cmd_context *cmd);
int refresh_filters(struct cmd_context *cmd);
int config_files_changed(struct cmd_context *cmd);
int init_lvmcache_orphans(struct cmd_context *cmd);

#endif
