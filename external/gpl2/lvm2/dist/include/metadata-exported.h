/*	$NetBSD: metadata-exported.h,v 1.1.1.1.2.1 2009/05/13 18:52:41 jym Exp $	*/

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
 * This is the representation of LVM metadata that is being adapted
 * for library export.
 */

#ifndef _LVM_METADATA_EXPORTED_H
#define _LVM_METADATA_EXPORTED_H

#include "uuid.h"

struct physical_volume;
typedef struct physical_volume pv_t;
struct volume_group;
typedef struct volume_group vg_t;

struct logical_volume;

struct lv_segment;
struct pv_segment;

#define MAX_STRIPES 128U
#define SECTOR_SHIFT 9L
#define STRIPE_SIZE_MIN ( (unsigned) lvm_getpagesize() >> SECTOR_SHIFT)	/* PAGESIZE in sectors */
#define STRIPE_SIZE_MAX ( 512L * 1024L >> SECTOR_SHIFT)	/* 512 KB in sectors */
#define STRIPE_SIZE_LIMIT ((UINT_MAX >> 2) + 1)
#define PV_MIN_SIZE ( 512L * 1024L >> SECTOR_SHIFT)	/* 512 KB in sectors */
#define MAX_RESTRICTED_LVS 255	/* Used by FMT_RESTRICTED_LVIDS */

/* Layer suffix */
#define MIRROR_SYNC_LAYER "_mimagetmp"

/* Various flags */
/* Note that the bits no longer necessarily correspond to LVM1 disk format */

#define PARTIAL_VG		0x00000001U	/* VG */
#define EXPORTED_VG          	0x00000002U	/* VG PV */
#define RESIZEABLE_VG        	0x00000004U	/* VG */

/* May any free extents on this PV be used or must they be left free? */
#define ALLOCATABLE_PV         	0x00000008U	/* PV */

//#define SPINDOWN_LV          	0x00000010U	/* LV */
//#define BADBLOCK_ON       	0x00000020U	/* LV */
#define VISIBLE_LV		0x00000040U	/* LV */
#define FIXED_MINOR		0x00000080U	/* LV */
/* FIXME Remove when metadata restructuring is completed */
#define SNAPSHOT		0x00001000U	/* LV - internal use only */
#define PVMOVE			0x00002000U	/* VG LV SEG */
#define LOCKED			0x00004000U	/* LV */
#define MIRRORED		0x00008000U	/* LV - internal use only */
//#define VIRTUAL			0x00010000U	/* LV - internal use only */
#define MIRROR_LOG		0x00020000U	/* LV */
#define MIRROR_IMAGE		0x00040000U	/* LV */
#define MIRROR_NOTSYNCED	0x00080000U	/* LV */
//#define ACTIVATE_EXCL		0x00100000U	/* LV - internal use only */
//#define PRECOMMITTED		0x00200000U	/* VG - internal use only */
#define CONVERTING		0x00400000U	/* LV */

#define MISSING_PV              0x00800000U	/* PV */
#define PARTIAL_LV              0x01000000U	/* LV - derived flag, not
						   written out in metadata*/

//#define POSTORDER_FLAG	0x02000000U /* Not real flags, reserved for
//#define POSTORDER_OPEN_FLAG	0x04000000U    temporary use inside vg_read. */

#define LVM_READ              	0x00000100U	/* LV VG */
#define LVM_WRITE             	0x00000200U	/* LV VG */
#define CLUSTERED         	0x00000400U	/* VG */
//#define SHARED            	0x00000800U	/* VG */

/* Format features flags */
#define FMT_SEGMENTS		0x00000001U	/* Arbitrary segment params? */
#define FMT_MDAS		0x00000002U	/* Proper metadata areas? */
#define FMT_TAGS		0x00000004U	/* Tagging? */
#define FMT_UNLIMITED_VOLS	0x00000008U	/* Unlimited PVs/LVs? */
#define FMT_RESTRICTED_LVIDS	0x00000010U	/* LVID <= 255 */
#define FMT_ORPHAN_ALLOCATABLE	0x00000020U	/* Orphan PV allocatable? */
//#define FMT_PRECOMMIT		0x00000040U	/* Supports pre-commit? */
#define FMT_RESIZE_PV		0x00000080U	/* Supports pvresize? */
#define FMT_UNLIMITED_STRIPESIZE 0x00000100U	/* Unlimited stripe size? */
#define FMT_RESTRICTED_READAHEAD 0x00000200U	/* Readahead restricted to 2-120? */

/* LVM2 external library flags */
#define CORRECT_INCONSISTENT    0x00000001U /* Correct inconsistent metadata */
#define FAIL_INCONSISTENT       0x00000002U /* Fail if metadata inconsistent */

/* Mirror conversion type flags */
#define MIRROR_BY_SEG		0x00000001U	/* segment-by-segment mirror */
#define MIRROR_BY_LV		0x00000002U	/* mirror using whole mimage LVs */
#define MIRROR_SKIP_INIT_SYNC	0x00000010U	/* skip initial sync */

/* Ordered list - see lv_manip.c */
typedef enum {
	ALLOC_INVALID,
	ALLOC_CONTIGUOUS,
	ALLOC_CLING,
	ALLOC_NORMAL,
	ALLOC_ANYWHERE,
	ALLOC_INHERIT
} alloc_policy_t;

typedef enum {
	AREA_UNASSIGNED,
	AREA_PV,
	AREA_LV
} area_type_t;

/*
 * Whether or not to force an operation.
 */
typedef enum {
	PROMPT = 0, /* Issue yes/no prompt to confirm operation */
	DONT_PROMPT = 1, /* Skip yes/no prompt */
	DONT_PROMPT_OVERRIDE = 2 /* Skip prompt + override a second condition */
} force_t;

struct cmd_context;
struct format_handler;
struct labeller;

struct format_type {
	struct dm_list list;
	struct cmd_context *cmd;
	struct format_handler *ops;
	struct labeller *labeller;
	const char *name;
	const char *alias;
	const char *orphan_vg_name;
	uint32_t features;
	void *library;
	void *private;
};

struct pv_segment {
	struct dm_list list;	/* Member of pv->segments: ordered list
				 * covering entire data area on this PV */

	struct physical_volume *pv;
	uint32_t pe;
	uint32_t len;

	struct lv_segment *lvseg;	/* NULL if free space */
	uint32_t lv_area;	/* Index to area in LV segment */
};

#define pvseg_is_allocated(pvseg) ((pvseg)->lvseg)

struct physical_volume {
	struct id id;
	struct device *dev;
	const struct format_type *fmt;
	const char *vg_name;
	struct id vgid;

	uint32_t status;
	uint64_t size;

	/* physical extents */
	uint32_t pe_size;
	uint64_t pe_start;
	uint32_t pe_count;
	uint32_t pe_alloc_count;
	unsigned long pe_align;

	struct dm_list segments;	/* Ordered pv_segments covering complete PV */
	struct dm_list tags;
};

struct format_instance {
	const struct format_type *fmt;
	struct dm_list metadata_areas;	/* e.g. metadata locations */
	void *private;
};

struct volume_group {
	struct cmd_context *cmd;
	struct format_instance *fid;
	uint32_t seqno;		/* Metadata sequence number */

	struct id id;
	char *name;
	char *system_id;

	uint32_t status;
	alloc_policy_t alloc;

	uint32_t extent_size;
	uint32_t extent_count;
	uint32_t free_count;

	uint32_t max_lv;
	uint32_t max_pv;

	/* physical volumes */
	uint32_t pv_count;
	struct dm_list pvs;

	/*
	 * logical volumes
	 * The following relationship should always hold:
	 * dm_list_size(lvs) = lv_count + 2 * snapshot_count
	 *
	 * Snapshots consist of 2 instances of "struct logical_volume":
	 * - cow (lv_name is visible to the user)
	 * - snapshot (lv_name is 'snapshotN')
	 * Neither of these instances is reflected in lv_count, but we
	 * multiply the snapshot_count by 2.
	 *
	 * Mirrors consist of multiple instances of "struct logical_volume":
	 * - one for the mirror log
	 * - one for each mirror leg
	 * - one for the user-visible mirror LV
	 * all of the instances are reflected in lv_count.
	 */
	uint32_t lv_count;
	uint32_t snapshot_count;
	struct dm_list lvs;

	struct dm_list tags;
};

/* There will be one area for each stripe */
struct lv_segment_area {
	area_type_t type;
	union {
		struct {
			struct pv_segment *pvseg;
		} pv;
		struct {
			struct logical_volume *lv;
			uint32_t le;
		} lv;
	} u;
};

struct segment_type;
struct lv_segment {
	struct dm_list list;
	struct logical_volume *lv;

	const struct segment_type *segtype;
	uint32_t le;
	uint32_t len;

	uint32_t status;

	/* FIXME Fields depend on segment type */
	uint32_t stripe_size;
	uint32_t area_count;
	uint32_t area_len;
	struct logical_volume *origin;
	struct logical_volume *cow;
	struct dm_list origin_list;
	uint32_t chunk_size;	/* For snapshots - in sectors */
	uint32_t region_size;	/* For mirrors - in sectors */
	uint32_t extents_copied;
	struct logical_volume *log_lv;

	struct dm_list tags;

	struct lv_segment_area *areas;
};

#define seg_type(seg, s)	(seg)->areas[(s)].type
#define seg_pv(seg, s)		(seg)->areas[(s)].u.pv.pvseg->pv
#define seg_lv(seg, s)		(seg)->areas[(s)].u.lv.lv

struct logical_volume {
	union lvid lvid;
	char *name;

	struct volume_group *vg;

	uint32_t status;
	alloc_policy_t alloc;
	uint32_t read_ahead;
	int32_t major;
	int32_t minor;

	uint64_t size;		/* Sectors */
	uint32_t le_count;

	uint32_t origin_count;
	struct dm_list snapshot_segs;
	struct lv_segment *snapshot;

	struct dm_list segments;
	struct dm_list tags;
	struct dm_list segs_using_this_lv;
};

struct pe_range {
	struct dm_list list;
	uint32_t start;		/* PEs */
	uint32_t count;		/* PEs */
};

struct pv_list {
	struct dm_list list;
	struct physical_volume *pv;
	struct dm_list *mdas;	/* Metadata areas */
	struct dm_list *pe_ranges;	/* Ranges of PEs e.g. for allocation */
};

struct lv_list {
	struct dm_list list;
	struct logical_volume *lv;
};

/*
* Utility functions
*/
int vg_write(struct volume_group *vg);
int vg_commit(struct volume_group *vg);
int vg_revert(struct volume_group *vg);
struct volume_group *vg_read(struct cmd_context *cmd, const char *vg_name,
			     const char *vgid, int *consistent);
struct physical_volume *pv_read(struct cmd_context *cmd, const char *pv_name,
				struct dm_list *mdas, uint64_t *label_sector,
				int warnings);
struct dm_list *get_pvs(struct cmd_context *cmd);

/* Set full_scan to 1 to re-read every (filtered) device label */
struct dm_list *get_vgs(struct cmd_context *cmd, int full_scan);
struct dm_list *get_vgids(struct cmd_context *cmd, int full_scan);
int scan_vgs_for_pvs(struct cmd_context *cmd);

int pv_write(struct cmd_context *cmd, struct physical_volume *pv,
	     struct dm_list *mdas, int64_t label_sector);
int is_pv(pv_t *pv);
int is_orphan_vg(const char *vg_name);
int is_orphan(const pv_t *pv);
int vgs_are_compatible(struct cmd_context *cmd,
		       struct volume_group *vg_from,
		       struct volume_group *vg_to);
vg_t *vg_lock_and_read(struct cmd_context *cmd, const char *vg_name,
		       const char *vgid,
		       uint32_t lock_flags, uint32_t status_flags,
		       uint32_t misc_flags);

/* pe_start and pe_end relate to any existing data so that new metadata
* areas can avoid overlap */
pv_t *pv_create(const struct cmd_context *cmd,
		      struct device *dev,
		      struct id *id,
		      uint64_t size,
		      uint64_t pe_start,
		      uint32_t existing_extent_count,
		      uint32_t existing_extent_size,
		      int pvmetadatacopies,
		      uint64_t pvmetadatasize, struct dm_list *mdas);
int pv_resize(struct physical_volume *pv, struct volume_group *vg,
             uint32_t new_pe_count);
int pv_analyze(struct cmd_context *cmd, const char *pv_name,
	       uint64_t label_sector);

/* FIXME: move internal to library */
uint32_t pv_list_extents_free(const struct dm_list *pvh);

struct volume_group *vg_create(struct cmd_context *cmd, const char *name,
			       uint32_t extent_size, uint32_t max_pv,
			       uint32_t max_lv, alloc_policy_t alloc,
			       int pv_count, char **pv_names);
int vg_remove(struct volume_group *vg);
int vg_remove_single(struct cmd_context *cmd, const char *vg_name,
		     struct volume_group *vg, int consistent,
		     force_t force);
int vg_rename(struct cmd_context *cmd, struct volume_group *vg,
	      const char *new_name);
int vg_extend(struct volume_group *vg, int pv_count, char **pv_names);
int vg_change_pesize(struct cmd_context *cmd, struct volume_group *vg,
		     uint32_t new_extent_size);
int vg_split_mdas(struct cmd_context *cmd, struct volume_group *vg_from,
		  struct volume_group *vg_to);

/* Manipulate LVs */
struct logical_volume *lv_create_empty(const char *name,
				       union lvid *lvid,
				       uint32_t status,
				       alloc_policy_t alloc,
				       int import,
				       struct volume_group *vg);

/* Write out LV contents */
int set_lv(struct cmd_context *cmd, struct logical_volume *lv,
           uint64_t sectors, int value);

/* Reduce the size of an LV by extents */
int lv_reduce(struct logical_volume *lv, uint32_t extents);

/* Empty an LV prior to deleting it */
int lv_empty(struct logical_volume *lv);

/* Empty an LV and add error segment */
int replace_lv_with_error_segment(struct logical_volume *lv);

/* Entry point for all LV extent allocations */
int lv_extend(struct logical_volume *lv,
	      const struct segment_type *segtype,
	      uint32_t stripes, uint32_t stripe_size,
	      uint32_t mirrors, uint32_t extents,
	      struct physical_volume *mirrored_pv, uint32_t mirrored_pe,
	      uint32_t status, struct dm_list *allocatable_pvs,
	      alloc_policy_t alloc);

/* lv must be part of lv->vg->lvs */
int lv_remove(struct logical_volume *lv);

int lv_remove_single(struct cmd_context *cmd, struct logical_volume *lv,
		     force_t force);

int lv_remove_with_dependencies(struct cmd_context *cmd, struct logical_volume *lv,
				force_t force);

int lv_rename(struct cmd_context *cmd, struct logical_volume *lv,
	      const char *new_name);

/*
 * Functions for layer manipulation
 */
int insert_layer_for_segments_on_pv(struct cmd_context *cmd,
				    struct logical_volume *lv_where,
				    struct logical_volume *layer_lv,
				    uint32_t status,
				    struct pv_list *pv,
				    struct dm_list *lvs_changed);
int remove_layers_for_segments(struct cmd_context *cmd,
			       struct logical_volume *lv,
			       struct logical_volume *layer_lv,
			       uint32_t status_mask, struct dm_list *lvs_changed);
int remove_layers_for_segments_all(struct cmd_context *cmd,
				   struct logical_volume *layer_lv,
				   uint32_t status_mask,
				   struct dm_list *lvs_changed);
int split_parent_segments_for_layer(struct cmd_context *cmd,
				    struct logical_volume *layer_lv);
int remove_layer_from_lv(struct logical_volume *lv,
			 struct logical_volume *layer_lv);
struct logical_volume *insert_layer_for_lv(struct cmd_context *cmd,
					   struct logical_volume *lv_where,
					   uint32_t status,
					   const char *layer_suffix);

/* Find a PV within a given VG */
struct pv_list *find_pv_in_vg(const struct volume_group *vg,
			      const char *pv_name);
pv_t *find_pv_in_vg_by_uuid(const struct volume_group *vg,
			    const struct id *id);

/* Find an LV within a given VG */
struct lv_list *find_lv_in_vg(const struct volume_group *vg,
			      const char *lv_name);

/* FIXME Merge these functions with ones above */
struct logical_volume *find_lv(const struct volume_group *vg,
			       const char *lv_name);
struct physical_volume *find_pv_by_name(struct cmd_context *cmd,
					const char *pv_name);

/* Find LV segment containing given LE */
struct lv_segment *first_seg(const struct logical_volume *lv);


/*
* Useful functions for managing snapshots.
*/
int lv_is_origin(const struct logical_volume *lv);
int lv_is_cow(const struct logical_volume *lv);
int lv_is_visible(const struct logical_volume *lv);

/* Test if given LV is visible from user's perspective */
int lv_is_displayable(const struct logical_volume *lv);

int pv_is_in_vg(struct volume_group *vg, struct physical_volume *pv);

/* Given a cow LV, return return the snapshot lv_segment that uses it */
struct lv_segment *find_cow(const struct logical_volume *lv);

/* Given a cow LV, return its origin */
struct logical_volume *origin_from_cow(const struct logical_volume *lv);

int vg_add_snapshot(const char *name,
		    struct logical_volume *origin, struct logical_volume *cow,
		    union lvid *lvid, uint32_t extent_count,
		    uint32_t chunk_size);

int vg_remove_snapshot(struct logical_volume *cow);

int vg_check_status(const struct volume_group *vg, uint32_t status);

/*
* Mirroring functions
*/
struct lv_segment *find_mirror_seg(struct lv_segment *seg);
int lv_add_mirrors(struct cmd_context *cmd, struct logical_volume *lv,
		   uint32_t mirrors, uint32_t stripes,
		   uint32_t region_size, uint32_t log_count,
		   struct dm_list *pvs, alloc_policy_t alloc, uint32_t flags);
int lv_remove_mirrors(struct cmd_context *cmd, struct logical_volume *lv,
		      uint32_t mirrors, uint32_t log_count,
		      struct dm_list *pvs, uint32_t status_mask);

int is_temporary_mirror_layer(const struct logical_volume *lv);
struct logical_volume * find_temporary_mirror(const struct logical_volume *lv);
uint32_t lv_mirror_count(const struct logical_volume *lv);
uint32_t adjusted_mirror_region_size(uint32_t extent_size, uint32_t extents,
                                    uint32_t region_size);
int remove_mirrors_from_segments(struct logical_volume *lv,
				 uint32_t new_mirrors, uint32_t status_mask);
int add_mirrors_to_segments(struct cmd_context *cmd, struct logical_volume *lv,
			    uint32_t mirrors, uint32_t region_size,
			    struct dm_list *allocatable_pvs, alloc_policy_t alloc);

int remove_mirror_images(struct logical_volume *lv, uint32_t num_mirrors,
			 struct dm_list *removable_pvs, unsigned remove_log);
int add_mirror_images(struct cmd_context *cmd, struct logical_volume *lv,
		      uint32_t mirrors, uint32_t stripes, uint32_t region_size,
		      struct dm_list *allocatable_pvs, alloc_policy_t alloc,
		      uint32_t log_count);
struct logical_volume *detach_mirror_log(struct lv_segment *seg);
int attach_mirror_log(struct lv_segment *seg, struct logical_volume *lv);
int remove_mirror_log(struct cmd_context *cmd, struct logical_volume *lv,
		      struct dm_list *removable_pvs);
int add_mirror_log(struct cmd_context *cmd, struct logical_volume *lv,
		   uint32_t log_count, uint32_t region_size,
		   struct dm_list *allocatable_pvs, alloc_policy_t alloc);

int reconfigure_mirror_images(struct lv_segment *mirrored_seg, uint32_t num_mirrors,
			      struct dm_list *removable_pvs, unsigned remove_log);
int collapse_mirrored_lv(struct logical_volume *lv);
int shift_mirror_images(struct lv_segment *mirrored_seg, unsigned mimage);

struct logical_volume *find_pvmove_lv(struct volume_group *vg,
				      struct device *dev, uint32_t lv_type);
struct logical_volume *find_pvmove_lv_from_pvname(struct cmd_context *cmd,
						  struct volume_group *vg,
						  const char *name,
						  uint32_t lv_type);
const char *get_pvmove_pvname_from_lv(struct logical_volume *lv);
const char *get_pvmove_pvname_from_lv_mirr(struct logical_volume *lv_mirr);
float copy_percent(struct logical_volume *lv_mirr);
struct dm_list *lvs_using_lv(struct cmd_context *cmd, struct volume_group *vg,
			  struct logical_volume *lv);

uint32_t find_free_lvnum(struct logical_volume *lv);
char *generate_lv_name(struct volume_group *vg, const char *format,
		       char *buffer, size_t len);

/*
* Begin skeleton for external LVM library
*/
struct device *pv_dev(const pv_t *pv);
const char *pv_vg_name(const pv_t *pv);
const char *pv_dev_name(const pv_t *pv);
uint64_t pv_size(const pv_t *pv);
uint32_t pv_status(const pv_t *pv);
uint32_t pv_pe_size(const pv_t *pv);
uint64_t pv_pe_start(const pv_t *pv);
uint32_t pv_pe_count(const pv_t *pv);
uint32_t pv_pe_alloc_count(const pv_t *pv);

int vg_missing_pv_count(const vg_t *vg);
uint32_t vg_status(const vg_t *vg);
#define vg_is_clustered(vg) (vg_status((vg)) & CLUSTERED)

struct vgcreate_params {
	char *vg_name;
	uint32_t extent_size;
	size_t max_pv;
	size_t max_lv;
	alloc_policy_t alloc;
	int clustered; /* FIXME: put this into a 'status' variable instead? */
};

int validate_vg_create_params(struct cmd_context *cmd,
			      struct vgcreate_params *vp);

int validate_vg_rename_params(struct cmd_context *cmd,
			      const char *vg_name_old,
			      const char *vg_name_new);
#endif
