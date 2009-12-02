/*	$NetBSD: disk-rep.h,v 1.1.1.2 2009/12/02 00:26:48 haad Exp $	*/

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

#ifndef DISK_REP_FORMAT1_H
#define DISK_REP_FORMAT1_H

#include "lvm-types.h"
#include "metadata.h"
#include "toolcontext.h"

#define MAX_PV 256
#define MAX_LV 256
#define MAX_VG 99

#define LVM_BLK_MAJOR 58

#define MAX_PV_SIZE	((uint32_t) -1)	/* 2TB in sectors - 1 */
#define MIN_PE_SIZE	(8192L >> SECTOR_SHIFT)	/* 8 KB in sectors */
#define MAX_PE_SIZE	(16L * 1024L * (1024L >> SECTOR_SHIFT) * 1024L)
#define PE_SIZE_PV_SIZE_REL 5	/* PV size must be at least 5 times PE size */
#define	MAX_LE_TOTAL	65534	/* 2^16 - 2 */
#define	MAX_PE_TOTAL	((uint32_t) -2)

#define UNMAPPED_EXTENT 0

/* volume group */
#define	VG_ACTIVE            0x01	/* vg_status */
#define	VG_EXPORTED          0x02	/*     "     */
#define	VG_EXTENDABLE        0x04	/*     "     */

#define	VG_READ              0x01	/* vg_access */
#define	VG_WRITE             0x02	/*     "     */
#define	VG_CLUSTERED         0x04	/*     "     */
#define	VG_SHARED            0x08	/*     "     */

/* logical volume */
#define	LV_ACTIVE            0x01	/* lv_status */
#define	LV_SPINDOWN          0x02	/*     "     */
#define LV_PERSISTENT_MINOR  0x04	/*     "     */

#define	LV_READ              0x01	/* lv_access */
#define	LV_WRITE             0x02	/*     "     */
#define	LV_SNAPSHOT          0x04	/*     "     */
#define	LV_SNAPSHOT_ORG      0x08	/*     "     */

#define	LV_BADBLOCK_ON       0x01	/* lv_badblock */

#define	LV_STRICT            0x01	/* lv_allocation */
#define	LV_CONTIGUOUS        0x02	/*       "       */

/* physical volume */
#define	PV_ACTIVE            0x01	/* pv_status */
#define	PV_ALLOCATABLE       0x02	/* pv_allocatable */

#define EXPORTED_TAG "PV_EXP"	/* Identifier for exported PV */
#define IMPORTED_TAG "PV_IMP"	/* Identifier for imported PV */

struct data_area {
	uint32_t base;
	uint32_t size;
} __attribute__ ((packed));

struct pv_disk {
	int8_t id[2];
	uint16_t version;	/* lvm version */
	struct data_area pv_on_disk;
	struct data_area vg_on_disk;
	struct data_area pv_uuidlist_on_disk;
	struct data_area lv_on_disk;
	struct data_area pe_on_disk;
	int8_t pv_uuid[NAME_LEN];
	int8_t vg_name[NAME_LEN];
	int8_t system_id[NAME_LEN];	/* for vgexport/vgimport */
	uint32_t pv_major;
	uint32_t pv_number;
	uint32_t pv_status;
	uint32_t pv_allocatable;
	uint32_t pv_size;
	uint32_t lv_cur;
	uint32_t pe_size;
	uint32_t pe_total;
	uint32_t pe_allocated;

	/* only present on version == 2 pv's */
	uint32_t pe_start;
} __attribute__ ((packed));

struct lv_disk {
	int8_t lv_name[NAME_LEN];
	int8_t vg_name[NAME_LEN];
	uint32_t lv_access;
	uint32_t lv_status;
	uint32_t lv_open;
	uint32_t lv_dev;
	uint32_t lv_number;
	uint32_t lv_mirror_copies;	/* for future use */
	uint32_t lv_recovery;	/*       "        */
	uint32_t lv_schedule;	/*       "        */
	uint32_t lv_size;
	uint32_t lv_snapshot_minor;	/* minor number of original */
	uint16_t lv_chunk_size;	/* chunk size of snapshot */
	uint16_t dummy;
	uint32_t lv_allocated_le;
	uint32_t lv_stripes;
	uint32_t lv_stripesize;
	uint32_t lv_badblock;	/* for future use */
	uint32_t lv_allocation;
	uint32_t lv_io_timeout;	/* for future use */
	uint32_t lv_read_ahead;
} __attribute__ ((packed));

struct vg_disk {
	int8_t vg_uuid[ID_LEN];	/* volume group UUID */
	int8_t vg_name_dummy[NAME_LEN - ID_LEN];	/* rest of v1 VG name */
	uint32_t vg_number;	/* volume group number */
	uint32_t vg_access;	/* read/write */
	uint32_t vg_status;	/* active or not */
	uint32_t lv_max;	/* maximum logical volumes */
	uint32_t lv_cur;	/* current logical volumes */
	uint32_t lv_open;	/* open logical volumes */
	uint32_t pv_max;	/* maximum physical volumes */
	uint32_t pv_cur;	/* current physical volumes FU */
	uint32_t pv_act;	/* active physical volumes */
	uint32_t dummy;
	uint32_t vgda;		/* volume group descriptor arrays FU */
	uint32_t pe_size;	/* physical extent size in sectors */
	uint32_t pe_total;	/* total of physical extents */
	uint32_t pe_allocated;	/* allocated physical extents */
	uint32_t pvg_total;	/* physical volume groups FU */
} __attribute__ ((packed));

struct pe_disk {
	uint16_t lv_num;
	uint16_t le_num;
} __attribute__ ((packed));

struct uuid_list {
	struct dm_list list;
	char uuid[NAME_LEN] __attribute((aligned(8)));
};

struct lvd_list {
	struct dm_list list;
	struct lv_disk lvd;
};

struct disk_list {
	struct dm_list list;
	struct dm_pool *mem;
	struct device *dev;

	struct pv_disk pvd __attribute((aligned(8)));
	struct vg_disk vgd __attribute((aligned(8)));
	struct dm_list uuids __attribute((aligned(8)));
	struct dm_list lvds __attribute((aligned(8)));
	struct pe_disk *extents __attribute((aligned(8)));
};

/*
 * Layout constants.
 */
#define METADATA_ALIGN 4096UL
#define LVM1_PE_ALIGN (65536UL >> SECTOR_SHIFT)      /* PE alignment */

#define	METADATA_BASE 0UL
#define	PV_SIZE 1024UL
#define	VG_SIZE 4096UL

/*
 * Functions to calculate layout info.
 */
int calculate_layout(struct disk_list *dl);
int calculate_extent_count(struct physical_volume *pv, uint32_t extent_size,
			   uint32_t max_extent_count, uint64_t pe_start);

/*
 * Low level io routines which read/write
 * disk_lists.
 */

struct disk_list *read_disk(const struct format_type *fmt, struct device *dev,
			    struct dm_pool *mem, const char *vg_name);

int read_pvs_in_vg(const struct format_type *fmt, const char *vg_name,
		   struct dev_filter *filter,
		   struct dm_pool *mem, struct dm_list *results);

int write_disks(const struct format_type *fmt, struct dm_list *pvds);

/*
 * Functions to translate to between disk and in
 * core structures.
 */
int import_pv(const struct format_type *fmt, struct dm_pool *mem,
	      struct device *dev, struct volume_group *vg,
	      struct physical_volume *pv, struct pv_disk *pvd,
	      struct vg_disk *vgd);
int export_pv(struct cmd_context *cmd, struct dm_pool *mem,
	      struct volume_group *vg,
	      struct pv_disk *pvd, struct physical_volume *pv);

int import_vg(struct dm_pool *mem,
	      struct volume_group *vg, struct disk_list *dl);
int export_vg(struct vg_disk *vgd, struct volume_group *vg);

int import_lv(struct cmd_context *cmd, struct dm_pool *mem,
	      struct logical_volume *lv, struct lv_disk *lvd);

int import_extents(struct cmd_context *cmd, struct volume_group *vg,
		   struct dm_list *pvds);
int export_extents(struct disk_list *dl, uint32_t lv_num,
		   struct logical_volume *lv, struct physical_volume *pv);

int import_pvs(const struct format_type *fmt, struct dm_pool *mem,
	       struct volume_group *vg,
	       struct dm_list *pvds, struct dm_list *results, uint32_t *count);

int import_lvs(struct dm_pool *mem, struct volume_group *vg, struct dm_list *pvds);
int export_lvs(struct disk_list *dl, struct volume_group *vg,
	       struct physical_volume *pv, const char *dev_dir);

int import_snapshots(struct dm_pool *mem, struct volume_group *vg,
		     struct dm_list *pvds);

int export_uuids(struct disk_list *dl, struct volume_group *vg);

void export_numbers(struct dm_list *pvds, struct volume_group *vg);

void export_pv_act(struct dm_list *pvds);
int munge_pvd(struct device *dev, struct pv_disk *pvd);
int read_vgd(struct device *dev, struct vg_disk *vgd, struct pv_disk *pvd);

/* blech */
int get_free_vg_number(struct format_instance *fid, struct dev_filter *filter,
		       const char *candidate_vg, int *result);
int export_vg_number(struct format_instance *fid, struct dm_list *pvds,
		     const char *vg_name, struct dev_filter *filter);

#endif
