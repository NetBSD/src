/*	$NetBSD: import-export.h,v 1.1.1.2 2008/12/12 11:42:45 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2006 Red Hat, Inc. All rights reserved.
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

#ifndef _LVM_TEXT_IMPORT_EXPORT_H
#define _LVM_TEXT_IMPORT_EXPORT_H

#include "config.h"
#include "lvm-types.h"
#include "metadata.h"

#include <stdio.h>

/*
 * Constants to identify files this code can parse.
 */
#define CONTENTS_FIELD "contents"
#define CONTENTS_VALUE "Text Format Volume Group"

#define FORMAT_VERSION_FIELD "version"
#define FORMAT_VERSION_VALUE 1

/*
 * VGs, PVs and LVs all have status bitsets, we gather together
 * common code for reading and writing them.
 */
enum {
	COMPATIBLE_FLAG = 0x0,
	VG_FLAGS,
	PV_FLAGS,
	LV_FLAGS,
	STATUS_FLAG = 0x8,
};

struct text_vg_version_ops {
	int (*check_version) (struct config_tree * cf);
	struct volume_group *(*read_vg) (struct format_instance * fid,
					 struct config_tree * cf);
	void (*read_desc) (struct dm_pool * mem, struct config_tree * cf,
			   time_t *when, char **desc);
	const char *(*read_vgname) (const struct format_type *fmt,
				    struct config_tree *cft,
				    struct id *vgid, uint32_t *vgstatus,
				    char **creation_host);
};

struct text_vg_version_ops *text_vg_vsn1_init(void);

int print_flags(uint32_t status, int type, char *buffer, size_t size);
int read_flags(uint32_t *status, int type, struct config_value *cv);

int print_tags(struct dm_list *tags, char *buffer, size_t size);
int read_tags(struct dm_pool *mem, struct dm_list *tags, struct config_value *cv);

int text_vg_export_file(struct volume_group *vg, const char *desc, FILE *fp);
int text_vg_export_raw(struct volume_group *vg, const char *desc, char **buf);
struct volume_group *text_vg_import_file(struct format_instance *fid,
					 const char *file,
					 time_t *when, char **desc);
struct volume_group *text_vg_import_fd(struct format_instance *fid,
				       const char *file,
				       struct device *dev,
				       off_t offset, uint32_t size,
				       off_t offset2, uint32_t size2,
				       checksum_fn_t checksum_fn,
				       uint32_t checksum,
				       time_t *when, char **desc);
const char *text_vgname_import(const struct format_type *fmt,
			       struct device *dev,
                               off_t offset, uint32_t size,
                               off_t offset2, uint32_t size2,
                               checksum_fn_t checksum_fn, uint32_t checksum,
                               struct id *vgid, uint32_t *vgstatus,
			       char **creation_host);

#endif
