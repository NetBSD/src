/*	$NetBSD: metadata.h,v 1.1.1.3 2009/12/02 00:26:40 haad Exp $	*/

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

/*
 * This is the in core representation of a volume group and its
 * associated physical and logical volumes.
 */

#ifndef _LVM_METADATA_H
#define _LVM_METADATA_H

#include "ctype.h"
#include "dev-cache.h"
#include "lvm-string.h"
#include "metadata-exported.h"

//#define MAX_STRIPES 128U
//#define SECTOR_SHIFT 9L
#define SECTOR_SIZE ( 1L << SECTOR_SHIFT )
//#define STRIPE_SIZE_MIN ( (unsigned) lvm_getpagesize() >> SECTOR_SHIFT)	/* PAGESIZE in sectors */
//#define STRIPE_SIZE_MAX ( 512L * 1024L >> SECTOR_SHIFT)	/* 512 KB in sectors */
//#define STRIPE_SIZE_LIMIT ((UINT_MAX >> 2) + 1)
//#define PV_MIN_SIZE ( 512L * 1024L >> SECTOR_SHIFT)	/* 512 KB in sectors */
//#define MAX_RESTRICTED_LVS 255	/* Used by FMT_RESTRICTED_LVIDS */
#define MIRROR_LOG_OFFSET	2	/* sectors */
#define VG_MEMPOOL_CHUNK	10240	/* in bytes, hint only */

/*
 * Ceiling(n / sz)
 */
#define dm_div_up(n, sz) (((n) + (sz) - 1) / (sz))

/*
 * Ceiling(n / size) * size
 */
#define dm_round_up(n, sz) (dm_div_up((n), (sz)) * (sz))


/* Various flags */
/* Note that the bits no longer necessarily correspond to LVM1 disk format */

//#define PARTIAL_VG		0x00000001U	/* VG */
//#define EXPORTED_VG          	0x00000002U	/* VG PV */
//#define RESIZEABLE_VG        	0x00000004U	/* VG */

/* May any free extents on this PV be used or must they be left free? */
//#define ALLOCATABLE_PV         	0x00000008U	/* PV */

#define SPINDOWN_LV          	0x00000010U	/* LV */
#define BADBLOCK_ON       	0x00000020U	/* LV */
//#define VISIBLE_LV		0x00000040U	/* LV */
//#define FIXED_MINOR		0x00000080U	/* LV */
/* FIXME Remove when metadata restructuring is completed */
//#define SNAPSHOT		0x00001000U	/* LV - internal use only */
//#define PVMOVE			0x00002000U	/* VG LV SEG */
//#define LOCKED			0x00004000U	/* LV */
//#define MIRRORED		0x00008000U	/* LV - internal use only */
#define VIRTUAL			0x00010000U	/* LV - internal use only */
//#define MIRROR_LOG		0x00020000U	/* LV */
//#define MIRROR_IMAGE		0x00040000U	/* LV */
//#define MIRROR_NOTSYNCED	0x00080000U	/* LV */
#define ACTIVATE_EXCL		0x00100000U	/* LV - internal use only */
#define PRECOMMITTED		0x00200000U	/* VG - internal use only */
//#define CONVERTING		0x00400000U	/* LV */

//#define MISSING_PV		0x00800000U	/* PV */
//#define PARTIAL_LV		0x01000000U	/* LV - derived flag, not
//						   written out in metadata*/

#define POSTORDER_FLAG		0x02000000U /* Not real flags, reserved for  */
#define POSTORDER_OPEN_FLAG	0x04000000U /* temporary use inside vg_read_internal. */
#define VIRTUAL_ORIGIN		0x08000000U	/* LV - internal use only */

//#define LVM_READ              	0x00000100U	/* LV VG */
//#define LVM_WRITE             	0x00000200U	/* LV VG */
//#define CLUSTERED         	0x00000400U	/* VG */
#define SHARED            	0x00000800U	/* VG */

/* Format features flags */
//#define FMT_SEGMENTS		0x00000001U	/* Arbitrary segment params? */
//#define FMT_MDAS		0x00000002U	/* Proper metadata areas? */
//#define FMT_TAGS		0x00000004U	/* Tagging? */
//#define FMT_UNLIMITED_VOLS	0x00000008U	/* Unlimited PVs/LVs? */
//#define FMT_RESTRICTED_LVIDS	0x00000010U	/* LVID <= 255 */
//#define FMT_ORPHAN_ALLOCATABLE	0x00000020U	/* Orphan PV allocatable? */
#define FMT_PRECOMMIT		0x00000040U	/* Supports pre-commit? */
//#define FMT_RESIZE_PV		0x00000080U	/* Supports pvresize? */
//#define FMT_UNLIMITED_STRIPESIZE 0x00000100U	/* Unlimited stripe size? */

struct metadata_area;

/* Per-format per-metadata area operations */
struct metadata_area_ops {
	struct volume_group *(*vg_read) (struct format_instance * fi,
					 const char *vg_name,
					 struct metadata_area * mda);
	struct volume_group *(*vg_read_precommit) (struct format_instance * fi,
					 const char *vg_name,
					 struct metadata_area * mda);
	/*
	 * Write out complete VG metadata.  You must ensure internal
	 * consistency before calling. eg. PEs can't refer to PVs not
	 * part of the VG.
	 *
	 * It is also the responsibility of the caller to ensure external
	 * consistency, eg by calling pv_write() if removing PVs from
	 * a VG or calling vg_write() a second time if splitting a VG
	 * into two.
	 *
	 * vg_write() should not read or write from any PVs not included
	 * in the volume_group structure it is handed.
	 * (format1 currently breaks this rule.)
	 */
	int (*vg_write) (struct format_instance * fid, struct volume_group * vg,
			 struct metadata_area * mda);
	int (*vg_precommit) (struct format_instance * fid,
			     struct volume_group * vg,
			     struct metadata_area * mda);
	int (*vg_commit) (struct format_instance * fid,
			  struct volume_group * vg, struct metadata_area * mda);
	int (*vg_revert) (struct format_instance * fid,
			  struct volume_group * vg, struct metadata_area * mda);
	int (*vg_remove) (struct format_instance * fi, struct volume_group * vg,
			  struct metadata_area * mda);

	/*
	 * Returns number of free sectors in given metadata area.
	 */
	uint64_t (*mda_free_sectors) (struct metadata_area *mda);

	/*
	 * Returns number of total sectors in given metadata area.
	 */
	uint64_t (*mda_total_sectors) (struct metadata_area *mda);

	/*
	 * Check if metadata area belongs to vg
	 */
	int (*mda_in_vg) (struct format_instance * fi,
			    struct volume_group * vg, struct metadata_area *mda);
	/*
	 * Analyze a metadata area on a PV.
	 */
	int (*pv_analyze_mda) (const struct format_type * fmt,
			       struct metadata_area *mda);

};

struct metadata_area {
	struct dm_list list;
	struct metadata_area_ops *ops;
	void *metadata_locn;
};

#define seg_pvseg(seg, s)	(seg)->areas[(s)].u.pv.pvseg
#define seg_dev(seg, s)		(seg)->areas[(s)].u.pv.pvseg->pv->dev
#define seg_pe(seg, s)		(seg)->areas[(s)].u.pv.pvseg->pe
#define seg_le(seg, s)		(seg)->areas[(s)].u.lv.le

struct name_list {
	struct dm_list list;
	char *name;
};

struct mda_list {
	struct dm_list list;
	struct device_area mda;
};

struct peg_list {
	struct dm_list list;
	struct pv_segment *peg;
};

struct seg_list {
	struct dm_list list;
	unsigned count;
	struct lv_segment *seg;
};

/*
 * Ownership of objects passes to caller.
 */
struct format_handler {
	/*
	 * Scan any metadata areas that aren't referenced in PV labels
	 */
	int (*scan) (const struct format_type * fmt);

	/*
	 * Return PV with given path.
	 */
	int (*pv_read) (const struct format_type * fmt, const char *pv_name,
			struct physical_volume * pv, struct dm_list *mdas,
			int scan_label_only);

	/*
	 * Tweak an already filled out a pv ready for importing into a
	 * vg.  eg. pe_count is format specific.
	 */
	int (*pv_setup) (const struct format_type * fmt,
			 uint64_t pe_start, uint32_t extent_count,
			 uint32_t extent_size, unsigned long data_alignment,
			 unsigned long data_alignment_offset,
			 int pvmetadatacopies,
			 uint64_t pvmetadatasize, struct dm_list * mdas,
			 struct physical_volume * pv, struct volume_group * vg);

	/*
	 * Write a PV structure to disk. Fails if the PV is in a VG ie
	 * pv->vg_name must be a valid orphan VG name
	 */
	int (*pv_write) (const struct format_type * fmt,
			 struct physical_volume * pv, struct dm_list * mdas,
			 int64_t label_sector);

	/*
	 * Tweak an already filled out a lv eg, check there
	 * aren't too many extents.
	 */
	int (*lv_setup) (struct format_instance * fi,
			 struct logical_volume * lv);

	/*
	 * Tweak an already filled out vg.  eg, max_pv is format
	 * specific.
	 */
	int (*vg_setup) (struct format_instance * fi, struct volume_group * vg);

	/*
	 * Check whether particular segment type is supported.
	 */
	int (*segtype_supported) (struct format_instance *fid,
				  const struct segment_type *segtype);

	/*
	 * Create format instance with a particular metadata area
	 */
	struct format_instance *(*create_instance) (const struct format_type *
						    fmt, const char *vgname,
						    const char *vgid,
						    void *context);

	/*
	 * Destructor for format instance
	 */
	void (*destroy_instance) (struct format_instance * fid);

	/*
	 * Destructor for format type
	 */
	void (*destroy) (const struct format_type * fmt);
};

/*
 * Utility functions
 */
unsigned long set_pe_align(struct physical_volume *pv, unsigned long data_alignment);
unsigned long set_pe_align_offset(struct physical_volume *pv,
				  unsigned long data_alignment_offset);
int vg_validate(struct volume_group *vg);

int pv_write_orphan(struct cmd_context *cmd, struct physical_volume *pv);

/* Manipulate PV structures */
int pv_add(struct volume_group *vg, struct physical_volume *pv);
int pv_remove(struct volume_group *vg, struct physical_volume *pv);
struct physical_volume *pv_find(struct volume_group *vg, const char *pv_name);

/* Find a PV within a given VG */
int get_pv_from_vg_by_id(const struct format_type *fmt, const char *vg_name,
			 const char *vgid, const char *pvid,
			 struct physical_volume *pv);

struct lv_list *find_lv_in_vg_by_lvid(struct volume_group *vg,
				      const union lvid *lvid);

struct lv_list *find_lv_in_lv_list(const struct dm_list *ll,
				   const struct logical_volume *lv);

/* Return the VG that contains a given LV (based on path given in lv_name) */
/* or environment var */
struct volume_group *find_vg_with_lv(const char *lv_name);

/* Find LV with given lvid (used during activation) */
struct logical_volume *lv_from_lvid(struct cmd_context *cmd,
				    const char *lvid_s,
				    unsigned precommitted);

/* FIXME Merge these functions with ones above */
struct physical_volume *find_pv(struct volume_group *vg, struct device *dev);

struct pv_list *find_pv_in_pv_list(const struct dm_list *pl,
				   const struct physical_volume *pv);

/* Find LV segment containing given LE */
struct lv_segment *find_seg_by_le(const struct logical_volume *lv, uint32_t le);

/* Find PV segment containing given LE */
struct pv_segment *find_peg_by_pe(const struct physical_volume *pv, uint32_t pe);

/*
 * Remove a dev_dir if present.
 */
const char *strip_dir(const char *vg_name, const char *dir);

struct logical_volume *alloc_lv(struct dm_pool *mem);

/*
 * Checks that an lv has no gaps or overlapping segments.
 * Set complete_vg to perform additional VG level checks.
 */
int check_lv_segments(struct logical_volume *lv, int complete_vg);

/*
 * Sometimes (eg, after an lvextend), it is possible to merge two
 * adjacent segments into a single segment.  This function trys
 * to merge as many segments as possible.
 */
int lv_merge_segments(struct logical_volume *lv);

/*
 * Ensure there's a segment boundary at a given LE, splitting if necessary
 */
int lv_split_segment(struct logical_volume *lv, uint32_t le);

/*
 * Add/remove upward link from underlying LV to the segment using it
 * FIXME: ridiculously long name
 */
int add_seg_to_segs_using_this_lv(struct logical_volume *lv, struct lv_segment *seg);
int remove_seg_from_segs_using_this_lv(struct logical_volume *lv, struct lv_segment *seg);
struct lv_segment *get_only_segment_using_this_lv(struct logical_volume *lv);

/*
 * Count snapshot LVs.
 */
unsigned snapshot_count(const struct volume_group *vg);

/*
 * Calculate readahead from underlying PV devices
 */
void lv_calculate_readahead(const struct logical_volume *lv, uint32_t *read_ahead);

/*
 * For internal metadata caching.
 */
int export_vg_to_buffer(struct volume_group *vg, char **buf);
struct volume_group *import_vg_from_buffer(char *buf,
					   struct format_instance *fid);

/*
 * Mirroring functions
 */

/*
 * Given mirror image or mirror log segment, find corresponding mirror segment 
 */
int fixup_imported_mirrors(struct volume_group *vg);

/*
 * Begin skeleton for external LVM library
 */
struct id pv_id(const struct physical_volume *pv);
const struct format_type *pv_format_type(const struct physical_volume *pv);
struct id pv_vgid(const struct physical_volume *pv);

struct physical_volume *pv_by_path(struct cmd_context *cmd, const char *pv_name);
int add_pv_to_vg(struct volume_group *vg, const char *pv_name,
		 struct physical_volume *pv);

#endif
