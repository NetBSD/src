/*	$NetBSD: report.h,v 1.1.1.2 2009/12/02 00:25:41 haad Exp $	*/

/*
 * Copyright (C) 2002-2004 Sistina Software, Inc. All rights reserved.
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

#ifndef _LVM_REPORT_H
#define _LVM_REPORT_H

#include "metadata-exported.h"

typedef enum {
	LVS	= 1,
	PVS	= 2,
	VGS	= 4,
	SEGS	= 8,
	PVSEGS	= 16,
	LABEL	= 32
} report_type_t;

struct field;
struct report_handle;

typedef int (*field_report_fn) (struct report_handle * dh, struct field * field,
				const void *data);

void *report_init(struct cmd_context *cmd, const char *format, const char *keys,
		  report_type_t *report_type, const char *separator,
		  int aligned, int buffered, int headings, int field_prefixes,
		  int quoted, int columns_as_rows);
void report_free(void *handle);
int report_object(void *handle, struct volume_group *vg,
		  struct logical_volume *lv, struct physical_volume *pv,
		  struct lv_segment *seg, struct pv_segment *pvseg);
int report_output(void *handle);

#endif
