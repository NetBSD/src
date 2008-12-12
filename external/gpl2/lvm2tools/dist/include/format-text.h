/*	$NetBSD: format-text.h,v 1.1.1.1 2008/12/12 11:42:12 haad Exp $	*/

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

#ifndef _LVM_FORMAT_TEXT_H
#define _LVM_FORMAT_TEXT_H

#include "lvm-types.h"
#include "metadata.h"

#define FMT_TEXT_NAME "lvm2"
#define FMT_TEXT_ALIAS "text"
#define FMT_TEXT_ORPHAN_VG_NAME ORPHAN_VG_NAME(FMT_TEXT_NAME)

/*
 * Archives a vg config.  'retain_days' is the minimum number of
 * days that an archive file must be held for.  'min_archives' is
 * the minimum number of archives required to be kept for each
 * volume group.
 */
int archive_vg(struct volume_group *vg,
	       const char *dir,
	       const char *desc, uint32_t retain_days, uint32_t min_archive);

/*
 * Displays a list of vg backups in a particular archive directory.
 */
int archive_list(struct cmd_context *cmd, const char *dir, const char *vgname);
int archive_list_file(struct cmd_context *cmd, const char *file);
int backup_list(struct cmd_context *cmd, const char *dir, const char *vgname);

/*
 * The text format can read and write a volume_group to a file.
 */
struct format_type *create_text_format(struct cmd_context *cmd);
void *create_text_context(struct cmd_context *cmd, const char *path,
			  const char *desc);

struct labeller *text_labeller_create(const struct format_type *fmt);

int pvhdr_read(struct device *dev, char *buf);

int add_da(struct dm_pool *mem, struct dm_list *das,
	   uint64_t start, uint64_t size);
void del_das(struct dm_list *das);

int add_mda(const struct format_type *fmt, struct dm_pool *mem, struct dm_list *mdas,
	    struct device *dev, uint64_t start, uint64_t size);
void del_mdas(struct dm_list *mdas);

const char *vgname_from_mda(const struct format_type *fmt,
			    struct device_area *dev_area, struct id *vgid,
			    uint32_t *vgstatus, char **creation_host,
			    uint64_t *mda_free_sectors);

#endif
