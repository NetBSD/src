/*	$NetBSD: layout.h,v 1.1.1.1 2008/12/22 00:18:17 haad Exp $	*/

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

#ifndef _LVM_TEXT_LAYOUT_H
#define _LVM_TEXT_LAYOUT_H

#include "config.h"
#include "lvm-types.h"
#include "metadata.h"
#include "uuid.h"

/* On disk */
struct disk_locn {
	uint64_t offset;	/* Offset in bytes to start sector */
	uint64_t size;		/* Bytes */
} __attribute__ ((packed));

/* Data areas (holding PEs) */
struct data_area_list {
	struct dm_list list;
	struct disk_locn disk_locn;
};

/* Fields with the suffix _xl should be xlate'd wherever they appear */
/* On disk */
struct pv_header {
	int8_t pv_uuid[ID_LEN];

	/* This size can be overridden if PV belongs to a VG */
	uint64_t device_size_xl;	/* Bytes */

	/* NULL-terminated list of data areas followed by */
	/* NULL-terminated list of metadata area headers */
	struct disk_locn disk_areas_xl[0];	/* Two lists */
} __attribute__ ((packed));

/* On disk */
struct raw_locn {
	uint64_t offset;	/* Offset in bytes to start sector */
	uint64_t size;		/* Bytes */
	uint32_t checksum;
	uint32_t filler;
} __attribute__ ((packed));

/* On disk */
/* Structure size limited to one sector */
struct mda_header {
	uint32_t checksum_xl;	/* Checksum of rest of mda_header */
	int8_t magic[16];	/* To aid scans for metadata */
	uint32_t version;
	uint64_t start;		/* Absolute start byte of mda_header */
	uint64_t size;		/* Size of metadata area */

	struct raw_locn raw_locns[0];	/* NULL-terminated list */
} __attribute__ ((packed));

struct mda_lists {
	struct dm_list dirs;
	struct dm_list raws;
	struct metadata_area_ops *file_ops;
	struct metadata_area_ops *raw_ops;
};

struct mda_context {
	struct device_area area;
	uint64_t free_sectors;
	struct raw_locn rlocn;	/* Store inbetween write and commit */
};

/* FIXME Convert this at runtime */
#define FMTT_MAGIC "\040\114\126\115\062\040\170\133\065\101\045\162\060\116\052\076"
#define FMTT_VERSION 1
#define MDA_HEADER_SIZE 512
#define LVM2_LABEL "LVM2 001"
#define MDA_SIZE_MIN (8 * (unsigned) lvm_getpagesize())

#endif
