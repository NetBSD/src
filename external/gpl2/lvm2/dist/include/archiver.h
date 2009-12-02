/*	$NetBSD: archiver.h,v 1.1.1.3 2009/12/02 00:25:44 haad Exp $	*/

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

#ifndef _LVM_TOOL_ARCHIVE_H
#define _LVM_TOOL_ARCHIVE_H

#include "metadata-exported.h"

/*
 * There are two operations that come under the general area of
 * backups.  'Archiving' occurs just before a volume group
 * configuration is changed.  The user may configure when
 * archived files are expired.  Typically archives will be stored
 * in /etc/lvm/archive.
 *
 * A 'backup' is a redundant copy of the *current* volume group
 * configuration.  As such it should be taken just after the
 * volume group is changed.  Only 1 backup file will exist.
 * Typically backups will be stored in /etc/lvm/backups.
 */

int archive_init(struct cmd_context *cmd, const char *dir,
		 unsigned int keep_days, unsigned int keep_min,
		 int enabled);
void archive_exit(struct cmd_context *cmd);

void archive_enable(struct cmd_context *cmd, int flag);
int archive(struct volume_group *vg);
int archive_display(struct cmd_context *cmd, const char *vg_name);
int archive_display_file(struct cmd_context *cmd, const char *file);

int backup_init(struct cmd_context *cmd, const char *dir, int enabled);
void backup_exit(struct cmd_context *cmd);

void backup_enable(struct cmd_context *cmd, int flag);
int backup(struct volume_group *vg);
int backup_locally(struct volume_group *vg);
int backup_remove(struct cmd_context *cmd, const char *vg_name);

struct volume_group *backup_read_vg(struct cmd_context *cmd,
				    const char *vg_name, const char *file);
int backup_restore_vg(struct cmd_context *cmd, struct volume_group *vg);
int backup_restore_from_file(struct cmd_context *cmd, const char *vg_name,
			     const char *file);
int backup_restore(struct cmd_context *cmd, const char *vg_name);

int backup_to_file(const char *file, const char *desc, struct volume_group *vg);

void check_current_backup(struct volume_group *vg);

#endif
