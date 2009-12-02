/*	$NetBSD: polldaemon.h,v 1.1.1.2 2009/12/02 00:25:53 haad Exp $	*/

/*
 * Copyright (C) 2003-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004 Red Hat, Inc. All rights reserved.
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

#ifndef _LVM_TOOL_POLLDAEMON_H
#define _LVM_TOOL_POLLDAEMON_H

#include "metadata-exported.h"

typedef enum {
	PROGRESS_CHECK_FAILED = 0,
	PROGRESS_UNFINISHED = 1,
	PROGRESS_FINISHED_SEGMENT = 2,
	PROGRESS_FINISHED_ALL = 3
} progress_t;

struct daemon_parms;

struct poll_functions {
	const char *(*get_copy_name_from_lv) (struct logical_volume *lv);
	struct volume_group *(*get_copy_vg) (struct cmd_context *cmd,
					     const char *name,
					     const char *uuid);
	struct logical_volume *(*get_copy_lv) (struct cmd_context *cmd,
					       struct volume_group *vg,
					       const char *name,
					       const char *uuid,
					       uint32_t lv_type);
	progress_t (*poll_progress)(struct cmd_context *cmd,
				    struct logical_volume *lv,
				    const char *name,
				    struct daemon_parms *parms);
	int (*update_metadata) (struct cmd_context *cmd,
				struct volume_group *vg,
				struct logical_volume *lv,
				struct dm_list *lvs_changed, unsigned flags);
	int (*finish_copy) (struct cmd_context *cmd,
			    struct volume_group *vg,
			    struct logical_volume *lv,
			    struct dm_list *lvs_changed);
};

struct daemon_parms {
	unsigned interval;
	unsigned aborting;
	unsigned background;
	unsigned outstanding_count;
	unsigned progress_display;
	const char *progress_title;
	uint32_t lv_type;
	struct poll_functions *poll_fns;
};

int poll_daemon(struct cmd_context *cmd, const char *name, const char *uuid,
		unsigned background,
		uint32_t lv_type, struct poll_functions *poll_fns,
		const char *progress_title);

progress_t poll_mirror_progress(struct cmd_context *cmd,
				struct logical_volume *lv, const char *name,
				struct daemon_parms *parms);

#endif
