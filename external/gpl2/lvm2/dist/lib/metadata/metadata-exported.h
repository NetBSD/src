/*	$NetBSD: metadata-exported.h,v 1.1.1.3 2009/12/02 00:26:42 haad Exp $	*/

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

/*
 * This is the representation of LVM metadata that is being adapted
 * for library export.
 */

#ifndef _LVM_METADATA_EXPORTED_H
#define _LVM_METADATA_EXPORTED_H

#include "uuid.h"

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
//#define POSTORDER_OPEN_FLAG	0x04000000U    temporary use inside vg_read_internal. */
//#define VIRTUAL_ORIGIN	0x08000000U	/* LV - internal use only */

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

/* Mirror conversion type flags */
#define MIRROR_BY_SEG		0x00000001U	/* segment-by-segment mirror */
#define MIRROR_BY_LV		0x00000002U	/* mirror using whole mimage LVs */
#define MIRROR_SKIP_INIT_SYNC	0x00000010U	/* skip initial sync */

/* vg_read and vg_read_for_update flags */
#define READ_ALLOW_INCONSISTENT	0x00010000U
#define READ_ALLOW_EXPORTED	0x00020000U
#define READ_WITHOUT_LOCK       0x00040000U

/* A meta-flag, useful with toollib for_each_* functions. */
#define READ_FOR_UPDATE 	0x00100000U

/* vg's "read_status" field */
#define FAILED_INCONSISTENT	0x00000001U
#define FAILED_LOCKING		0x00000002U
#define FAILED_NOTFOUND		0x00000004U
#define FAILED_READ_ONLY	0x00000008U
#define FAILED_EXPORTED		0x00000010U
#define FAILED_RESIZEABLE	0x00000020U
#define FAILED_CLUSTERED	0x00000040U
#define FAILED_ALLOCATION	0x00000080U
#define FAILED_EXIST		0x00000100U
#define SUCCESS			0x00000000U

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

typedef enum {
	PERCENT_0 = 0,
	PERCENT_0_TO_100 = 1,
	PERCENT_100 = 2,
	PERCENT_INVALID = 3
} percent_range_t;

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
	unsigned long pe_align_offset;

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
	struct dm_pool *vgmem;
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
	 * dm_list_size(lvs) = user visible lv_count + snapshot_count + other invisible LVs
	 *
	 * Snapshots consist of 2 instances of "struct logical_volume":
	 * - cow (lv_name is visible to the user)
	 * - snapshot (lv_name is 'snapshotN')
	 *
	 * Mirrors consist of multiple instances of "struct logical_volume":
	 * - one for the mirror log
	 * - one for each mirror leg
	 * - one for the user-visible mirror LV
	 */
	struct dm_list lvs;

	struct dm_list tags;

	/*
	 * FIXME: Move the next fields into a different struct?
	 */

	/*
	 * List of removed physical volumes by pvreduce.
	 * They have to get cleared on vg_commit.
	 */
	struct dm_list removed_pvs;
	uint32_t open_mode; /* FIXME: read or write - check lock type? */

	/*
	 * Store result of the last vg_read().
	 * 0 for success else appropriate FAILURE_* bits set.
	 */
	uint32_t read_status;
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
	void *segtype_private;

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

struct pvcreate_params {
	int zero;
	uint64_t size;
	uint64_t data_alignment;
	uint64_t data_alignment_offset;
	int pvmetadatacopies;
	uint64_t pvmetadatasize;
	int64_t labelsector;
	struct id id; /* FIXME: redundant */
	struct id *idp; /* 0 if no --uuid option */
	uint64_t pe_start;
	uint32_t extent_count;
	uint32_t extent_size;
	const char *restorefile; /* 0 if no --restorefile option */
	force_t force;
	unsigned yes;
};

struct physical_volume *pvcreate_single(struct cmd_context *cmd,
					const char *pv_name,
					struct pvcreate_params *pp);
void pvcreate_params_set_defaults(struct pvcreate_params *pp);

/*
* Utility functions
*/
int vg_write(struct volume_group *vg);
int vg_commit(struct volume_group *vg);
int vg_revert(struct volume_group *vg);
struct volume_group *vg_read_internal(struct cmd_context *cmd, const char *vg_name,
			     const char *vgid, int *consistent);
struct physical_volume *pv_read(struct cmd_context *cmd, const char *pv_name,
				struct dm_list *mdas, uint64_t *label_sector,
				int warnings, int scan_label_only);
struct dm_list *get_pvs(struct cmd_context *cmd);

/*
 * Add/remove LV to/from volume group
 */
int link_lv_to_vg(struct volume_group *vg, struct logical_volume *lv);
int unlink_lv_from_vg(struct logical_volume *lv);
void lv_set_visible(struct logical_volume *lv);
void lv_set_hidden(struct logical_volume *lv);

/* Set full_scan to 1 to re-read every (filtered) device label */
struct dm_list *get_vgnames(struct cmd_context *cmd, int full_scan);
struct dm_list *get_vgids(struct cmd_context *cmd, int full_scan);
int scan_vgs_for_pvs(struct cmd_context *cmd);

int pv_write(struct cmd_context *cmd, struct physical_volume *pv,
	     struct dm_list *mdas, int64_t label_sector);
int is_pv(struct physical_volume *pv);
int move_pv(struct volume_group *vg_from, struct volume_group *vg_to,
	    const char *pv_name);
int move_pvs_used_by_lv(struct volume_group *vg_from,
			struct volume_group *vg_to,
			const char *lv_name);
int is_orphan_vg(const char *vg_name);
int is_orphan(const struct physical_volume *pv);
int vgs_are_compatible(struct cmd_context *cmd,
		       struct volume_group *vg_from,
		       struct volume_group *vg_to);
uint32_t vg_lock_newname(struct cmd_context *cmd, const char *vgname);

/*
 * Return a handle to VG metadata.
 */
struct volume_group *vg_read(struct cmd_context *cmd, const char *vg_name,
              const char *vgid, uint32_t flags);
struct volume_group *vg_read_for_update(struct cmd_context *cmd, const char *vg_name,
                         const char *vgid, uint32_t flags);

/* 
 * Test validity of a VG handle.
 */
uint32_t vg_read_error(struct volume_group *vg_handle);

/* pe_start and pe_end relate to any existing data so that new metadata
* areas can avoid overlap */
struct physical_volume *pv_create(const struct cmd_context *cmd,
		      struct device *dev,
		      struct id *id,
		      uint64_t size,
		      unsigned long data_alignment,
		      unsigned long data_alignment_offset,
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

struct volume_group *vg_create(struct cmd_context *cmd, const char *vg_name);
int vg_remove_mdas(struct volume_group *vg);
int vg_remove_check(struct volume_group *vg);
int vg_remove(struct volume_group *vg);
int vg_rename(struct cmd_context *cmd, struct volume_group *vg,
	      const char *new_name);
int vg_extend(struct volume_group *vg, int pv_count, char **pv_names,
	      struct pvcreate_params *pp);
int vg_reduce(struct volume_group *vg, char *pv_name);
int vg_set_extent_size(struct volume_group *vg, uint32_t new_extent_size);
int vg_set_max_lv(struct volume_group *vg, uint32_t max_lv);
int vg_set_max_pv(struct volume_group *vg, uint32_t max_pv);
int vg_set_alloc_policy(struct volume_group *vg, alloc_policy_t alloc);
int vg_set_clustered(struct volume_group *vg, int clustered);
int vg_split_mdas(struct cmd_context *cmd, struct volume_group *vg_from,
		  struct volume_group *vg_to);

/* FIXME: refactor / unexport when lvremove liblvm refactoring dones */
int remove_lvs_in_vg(struct cmd_context *cmd,
		     struct volume_group *vg,
		     force_t force);
/*
 * vg_release() must be called on every struct volume_group allocated
 * by vg_create() or vg_read_internal() to free it when no longer required.
 */
void vg_release(struct volume_group *vg);

/* Manipulate LVs */
struct logical_volume *lv_create_empty(const char *name,
				       union lvid *lvid,
				       uint32_t status,
				       alloc_policy_t alloc,
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

uint64_t extents_from_size(struct cmd_context *cmd, uint64_t size,
			   uint32_t extent_size);

/* FIXME: refactor and reduce the size of this struct! */
struct lvcreate_params {
	/* flags */
	int snapshot; /* snap */
	int zero; /* all */
	int major; /* all */
	int minor; /* all */
	int corelog; /* mirror */
	int nosync; /* mirror */

	char *origin; /* snap */
	const char *vg_name; /* all */
	const char *lv_name; /* all */

	uint32_t stripes; /* striped */
	uint32_t stripe_size; /* striped */
	uint32_t chunk_size; /* snapshot */
	uint32_t region_size; /* mirror */

	uint32_t mirrors; /* mirror */

	const struct segment_type *segtype; /* all */

	/* size */
	uint32_t extents; /* all */
	uint32_t voriginextents; /* snapshot */
	uint64_t voriginsize; /* snapshot */
	struct dm_list *pvh; /* all */

	uint32_t permission; /* all */
	uint32_t read_ahead; /* all */
	alloc_policy_t alloc; /* all */

	const char *tag; /* all */
};

int lv_create_single(struct volume_group *vg,
		     struct lvcreate_params *lp);

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
struct physical_volume *find_pv_in_vg_by_uuid(const struct volume_group *vg,
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
int lv_is_virtual_origin(const struct logical_volume *lv);
int lv_is_cow(const struct logical_volume *lv);

/* Test if given LV is visible from user's perspective */
int lv_is_visible(const struct logical_volume *lv);

int pv_is_in_vg(struct volume_group *vg, struct physical_volume *pv);

/* Given a cow LV, return return the snapshot lv_segment that uses it */
struct lv_segment *find_cow(const struct logical_volume *lv);

/* Given a cow LV, return its origin */
struct logical_volume *origin_from_cow(const struct logical_volume *lv);

void init_snapshot_seg(struct lv_segment *seg, struct logical_volume *origin,
		       struct logical_volume *cow, uint32_t chunk_size);

int vg_add_snapshot(struct logical_volume *origin, struct logical_volume *cow,
		    union lvid *lvid, uint32_t extent_count,
		    uint32_t chunk_size);

int vg_remove_snapshot(struct logical_volume *cow);

int vg_check_status(const struct volume_group *vg, uint32_t status);

/*
 * Returns visible LV count - number of LVs from user perspective
 */
unsigned vg_visible_lvs(const struct volume_group *vg);

/*
 * Check if the VG reached maximal LVs count (if set)
 */
int vg_max_lv_reached(struct volume_group *vg);

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
						  const char *uuid,
						  uint32_t lv_type);
const char *get_pvmove_pvname_from_lv(struct logical_volume *lv);
const char *get_pvmove_pvname_from_lv_mirr(struct logical_volume *lv_mirr);
float copy_percent(struct logical_volume *lv_mirr,
		   percent_range_t *percent_range);
struct dm_list *lvs_using_lv(struct cmd_context *cmd, struct volume_group *vg,
			  struct logical_volume *lv);

uint32_t find_free_lvnum(struct logical_volume *lv);
char *generate_lv_name(struct volume_group *vg, const char *format,
		       char *buffer, size_t len);

/*
* Begin skeleton for external LVM library
*/
struct device *pv_dev(const struct physical_volume *pv);
const char *pv_vg_name(const struct physical_volume *pv);
const char *pv_dev_name(const struct physical_volume *pv);
uint64_t pv_size(const struct physical_volume *pv);
uint32_t pv_status(const struct physical_volume *pv);
uint32_t pv_pe_size(const struct physical_volume *pv);
uint64_t pv_pe_start(const struct physical_volume *pv);
uint32_t pv_pe_count(const struct physical_volume *pv);
uint32_t pv_pe_alloc_count(const struct physical_volume *pv);
uint32_t pv_mda_count(const struct physical_volume *pv);

uint64_t lv_size(const struct logical_volume *lv);

int vg_missing_pv_count(const struct volume_group *vg);
uint32_t vg_seqno(const struct volume_group *vg);
uint32_t vg_status(const struct volume_group *vg);
uint64_t vg_size(const struct volume_group *vg);
uint64_t vg_free(const struct volume_group *vg);
uint64_t vg_extent_size(const struct volume_group *vg);
uint64_t vg_extent_count(const struct volume_group *vg);
uint64_t vg_free_count(const struct volume_group *vg);
uint64_t vg_pv_count(const struct volume_group *vg);
uint64_t vg_max_pv(const struct volume_group *vg);
uint64_t vg_max_lv(const struct volume_group *vg);
uint32_t vg_mda_count(const struct volume_group *vg);
int vg_check_write_mode(struct volume_group *vg);
#define vg_is_clustered(vg) (vg_status((vg)) & CLUSTERED)
#define vg_is_exported(vg) (vg_status((vg)) & EXPORTED_VG)
#define vg_is_resizeable(vg) (vg_status((vg)) & RESIZEABLE_VG)

int lv_has_unknown_segments(const struct logical_volume *lv);
int vg_has_unknown_segments(const struct volume_group *vg);

struct vgcreate_params {
	char *vg_name;
	uint32_t extent_size;
	size_t max_pv;
	size_t max_lv;
	alloc_policy_t alloc;
	int clustered; /* FIXME: put this into a 'status' variable instead? */
};

int vgcreate_params_validate(struct cmd_context *cmd,
			     struct vgcreate_params *vp);

int validate_vg_rename_params(struct cmd_context *cmd,
			      const char *vg_name_old,
			      const char *vg_name_new);
#endif
