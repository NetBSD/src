/*	$NetBSD: partitions.h,v 1.28 2022/06/09 18:26:06 martin Exp $	*/

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Abstract interface to access arbitrary disk partitioning schemes and
 * keep Sysinst proper independent of the implementation / on-disk
 * details.
 *
 * NOTE:
 *  - all sector numbers, alignment and sizes are in units of the
 *    disks physical sector size (not necessarily 512 bytes)!
 *  - some interfaces pass the disks sector size (when it is easily
 *    available at typical callers), but the backends can always
 *    assume it to be equal to the real physical sector size. If
 *    no value is passed, the backend can query the disk data
 *    via get_disk_geom().
 *  - single exception: disk_partitioning_scheme::size_limit is in 512
 *    byte sectors (as it is not associated with a concrete disk)
 */

#include <sys/types.h>
#include <stdbool.h>
#include "msg_defs.h"

/*
 * Import all the file system types, as enum fs_type.
 */
#define FSTYPE_ENUMNAME	fs_type
#define FSTYPENAMES
#include <sys/disklabel.h>
#undef FSTYPE_ENUMNAME

/*
 * Use random values (outside uint8_t range) to mark special file system
 * types that are not in the FSTYPE enumeration.
 */
#ifndef	FS_TMPFS
#define	FS_TMPFS	256	/* tmpfs (prefered for /tmp if available) */
#endif
#ifndef	FS_MFS
#define	FS_MFS		257	/* mfs, alternative to tmpfs if that is
				   not available */
#endif
#ifndef	FS_EFI_SP
#define	FS_EFI_SP	258	/* EFI system partition, uses FS_MSDOS,
				   but may have a different partition
				   type */
#endif

#define	MAX_LABEL_LEN		128	/* max. length of a partition label */
#define	MAX_SHORTCUT_LEN	8	/* max. length of a shortcut ("a:") */

/*
 * A partition index / handle, identifies a singlepartition within
 * a struct disk_partitions. This is just an iterator/index - whenever
 * changes to the set of partitions are done, partitions may get a new
 * part_id.
 * We assume that partitioning schemes keep partitions sorted (with
 * key = start address, some schemes will have overlapping partitions,
 * like MBR extended partitions).
 */
typedef size_t part_id;

/*
 * An invalid value for a partition index / handle
 */
#define	NO_PART		((part_id)~0U)

/*
 * Intended usage for a partition
 */
enum part_type {
	PT_undef,		/* invalid value */
	PT_unknown,		/* anything we can not map to one of these */
	PT_root,		/* the NetBSD / partition (bootable) */
	PT_swap,		/* the NetBSD swap partition */
	PT_FAT,			/* boot partition (e.g. for u-boot) */
	PT_EXT2,		/* boot partition (for Linux appliances) */
	PT_SYSVBFS,		/* boot partition (for some SYSV machines) */
	PT_EFI_SYSTEM,		/* (U)EFI boot partition */
};

/*
 * A generic structure describing partition types for menu/user interface
 * purposes. The internal details may be richer and the *pointer* value
 * is the unique token - that is: the partitioning scheme will hand out
 * pointers to internal data and recognize the exact partition type details
 * by pointer comparison.
 */
struct part_type_desc {
	enum part_type generic_ptype;	/* what this maps to in generic terms */
	const char *short_desc;		/* short type description */
	const char *description;	/* full description */
};

/* Bits for disk_part_info.flags: */
#define	PTI_SEC_CONTAINER	1		/* this covers our secondary
						   partitions */
#define	PTI_WHOLE_DISK		2		/* all of the NetBSD disk */
#define	PTI_BOOT		4		/* required for booting */
#define	PTI_PSCHEME_INTERNAL	8		/* no user partition, e.g.
						   MBRs extend partition */
#define	PTI_RAW_PART		16		/* total disk */
#define	PTI_INSTALL_TARGET	32		/* marks the target partition
						 * assumed to become / after
						 * reboot; may not be
						 * persistent; may only be
						 * set for a single partition!
						 */

/* A single partition */
struct disk_part_info {
	daddr_t start, size;			/* start and size on disk */
	uint32_t flags;				/* active PTI_ flags */
	const struct part_type_desc *nat_type;	/* native partition type */
	/*
	 * The following will only be available
	 *  a) for a small subset of file system types
	 *  b) if the partition (in this state) has already been
	 *     used before
	 * It is OK to leave all these zeroed / NULL when setting
	 * partition data - or leave them at the last values a get operation
	 * returned. Backends can not rely on them to be valid.
	 */
	const char *last_mounted;		/* last mount point or NULL */
	unsigned int fs_type, fs_sub_type,	/* FS_* type of filesystem
						 * and for some FS a sub
						 * type (e.g. FFSv1 vs. FFSv2)
						 */
		fs_opt1, fs_opt2, fs_opt3;	/* FS specific option, used
						 * for FFS block/fragsize
						 * and inodes
						 */
};

/* An unused area that may be used for new partitions */
struct disk_part_free_space {
	daddr_t start, size;
};

/*
 * Some partition schemes define additional data that needs to be edited.
 * These attributes are described in this structure and referenced by
 * their index into the fixed list of available attributes.
 */
enum custom_attr_type { pet_bool, pet_cardinal, pet_str };
struct disk_part_custom_attribute {
	msg label;			/* Name, like "active partition" */
	enum custom_attr_type type;	/* bool, long, char* */
	size_t strlen;			/* maximum length if pet_str */
};

/*
 * When displaying a partition editor, we have standard columns, but
 * partitioning schemes add custom columns to the table as well.
 * There is a fixed number of columns and they are described by this
 * structure:
 */
struct disk_part_edit_column_desc {
	msg title;
	unsigned int width;
};

struct disk_partitions;	/* in-memory representation of a set of partitions */

/*
 * When querying partition "device" names, we may ask for:
 */
enum dev_name_usage {
	parent_device_only,	/* wd0 instead of wd0i, no path */
	logical_name,		/* NAME=my-root instead of dk7 */
	plain_name,		/* e.g. /dev/wd0i or /dev/dk7 */
	raw_dev_name,		/* e.g. /dev/rwd0i or /dev/rdk7 */
};

/*
 * A scheme how to store partitions on-disk, and methods to read/write
 * them to/from our abstract internal presentation.
 */
struct disk_partitioning_scheme {
	/* name of the on-disk scheme, retrieved via msg_string */
	msg name, short_name;

	/* prompt shown when creating custom partition types */
	msg new_type_prompt;

	/* description of scheme specific partition flags */
	msg part_flag_desc;

	/*
	 * size restrictions for this partitioning scheme (number
	 * of 512 byte sectors max)
	 */
	daddr_t size_limit;	/* 0 if not limited */

	/*
	 * If this scheme allows sub-partitions (i.e. MBR -> disklabel),
	 * this is a pointer to the (potential/optional) secondary
	 * scheme. Depending on partitioning details it may not be
	 * used in the end.
	 * This link is only here for better help messages.
	 * See *secondary_partitions further below for actually accessing
	 * secondary partitions.
	 */
	const struct disk_partitioning_scheme *secondary_scheme;

	/*
	 * Partition editor colum descriptions for whatever the scheme
	 * needs to display (see format_partition_table_str below).
	 */
	size_t edit_columns_count;
	const struct disk_part_edit_column_desc *edit_columns;

	/*
	 * Custom attributes editable by the partitioning scheme (but of
	 * no particular meaning for sysinst)
	 */
	size_t custom_attribute_count;
	const struct disk_part_custom_attribute *custom_attributes;

	/*
	 * Partition types supported by this scheme,
	 * first function gets the number, second queries single elements
	 */
	size_t (*get_part_types_count)(void);
	const struct part_type_desc * (*get_part_type)(size_t ndx);
	/*
	 * Get the preferred native representation for a generic partition type
	 */
	const struct part_type_desc * (*get_generic_part_type)(enum part_type);
	/*
	 * Get the preferred native partition type for a specific file system
	 * type (FS_*) and subtype (fs specific value)
	 */
	const struct part_type_desc * (*get_fs_part_type)(
	    enum part_type, unsigned, unsigned);
	/*
	 * Optional: inverse to above: given a part_type_desc, set default
	 * fstype and subtype.
	 */
	bool (*get_default_fstype)(const struct part_type_desc *,
	    unsigned *fstype, unsigned *fs_sub_type);
	/*
	 * Create a custom partition type. If the type already exists
	 * (or there is a collision), the old existing type will be
	 * returned and no new type created. This is not considered
	 * an error (to keep the user interface simple).
	 * On failure NULL is returned and (if passed != NULL)
	 * *err_msg is set to a message describing the error.
	 */
	const struct part_type_desc * (*create_custom_part_type)
	    (const char *custom, const char **err_msg);
	/*
	 * Return a usable internal partition type representation
	 * for types that are not otherwise mappable.
	 * This could be FS_OTHER for disklabel, or a randomly
	 * created type guid for GPT. This type may or may not be
	 * in the regular type list. If not, it needs to behave like a
	 * custom type.
	 */
	const struct part_type_desc * (*create_unknown_part_type)(void);

	/*
	 * Global attributes
	 */
	/*
	 * Get partition alignment suggestion. The schemen may enforce
	 * additional/different alignment for some partitions.
	 */
	daddr_t (*get_part_alignment)(const struct disk_partitions*);

	/*
	 * Methods to manipulate the in-memory abstract representation
	 */

	/* Retrieve data about a single partition, identified by the part_id.
	 * Fill the disk_part_info structure
	 */
	bool (*get_part_info)(const struct disk_partitions*, part_id,
	    struct disk_part_info*);

	/* Optional: fill an attribute string describing the given partition */
	bool (*get_part_attr_str)(const struct disk_partitions*, part_id,
	    char *str, size_t avail_space);
	/* Format a partition editor element for the "col" column in
	 * edit_columns. Used e.g. with MBR to set "active" flags.
	 */
	bool (*format_partition_table_str)(const struct disk_partitions*,
	    part_id, size_t col, char *outstr, size_t outspace);

	/* is the type of this partition changeable? */
	bool (*part_type_can_change)(const struct disk_partitions*,
	    part_id);

	/* can we add further partitions? */
	bool (*can_add_partition)(const struct disk_partitions*);

	/* is the custom attribute changeable? */
	bool (*custom_attribute_writable)(const struct disk_partitions*,
	    part_id, size_t attr_no);
	/*
	 * Output formatting for custom attributes.
	 * If "info" is != NULL, use (where it makes sense)
	 * values from that structure, as if a call to set_part_info
	 * would have been done before this call.
	 */
	bool (*format_custom_attribute)(const struct disk_partitions*,
	    part_id, size_t attr_no, const struct disk_part_info *info,
	    char *out, size_t out_space);
	/* value setter functions for custom attributes */
	/* pet_bool: */
	bool (*custom_attribute_toggle)(struct disk_partitions*,
	    part_id, size_t attr_no);
	/* pet_cardinal: */
	bool (*custom_attribute_set_card)(struct disk_partitions*,
	    part_id, size_t attr_no, long new_val);
	/* pet_str or pet_cardinal: */
	bool (*custom_attribute_set_str)(struct disk_partitions*,
	    part_id, size_t attr_no, const char *new_val);

	/*
	 * Optional: additional user information when showing the size
	 * editor (especially for existing unknown partitions)
	 */
	const char * (*other_partition_identifier)(const struct
	    disk_partitions*, part_id);


	/* Retrieve device and partition names, e.g. for checking
	 * against kern.root_device or invoking newfs.
	 * For disklabel partitions, "part" will be set to the partition
	 * index (a = 0, b = 1, ...), for others it will get set to -1.
	 * If dev_name_usage is parent_device_only, the device name will
	 * not include a partition letter - obviously this only makes a
	 * difference with disklabel partitions.
	 * If dev_name_usage is logical_name instead of a device name
	 * a given name may be returned in NAME= syntax.
	 * If with_path is true (and the returned value is a device
	 * node), include the /dev/ prefix in the result string
	 * (this is ignored when returning NAME= syntax for /etc/fstab).
	 * If life is true, the device must be made available under
	 * that name (only makes a difference for NAME=syntax if
	 * no wedge has been created yet,) - implied for all variants
	 * where dev_name_usage != logical_name.
	 */
	bool (*get_part_device)(const struct disk_partitions*,
	    part_id, char *devname, size_t max_devname_len, int *part,
	    enum dev_name_usage, bool with_path, bool life);

	/*
	 * How big could we resize the given position (start of existing
	 * partition or free space)
	 */
	daddr_t (*max_free_space_at)(const struct disk_partitions*, daddr_t);

	/*
	 * Provide a list of free spaces usable for further partitioning,
	 * assuming the given partition alignment.
	 * If start is > 0 no space with lower sector numbers will
	 * be found.
	 * If ignore is > 0, any partition starting at that sector will
	 * be considered "free", this is used e.g. when moving an existing
	 * partition around.
	 */
	size_t (*get_free_spaces)(const struct disk_partitions*,
	    struct disk_part_free_space *result, size_t max_num_result,
	    daddr_t min_space_size, daddr_t align, daddr_t start,
	    daddr_t ignore /* -1 */);

	/*
	 * Translate a partition description from a foreign partitioning
	 * scheme as close as possible to what we can handle in add_partition.
	 * This mostly adjusts flags and partition type pointers (using
	 * more lose matching than add_partition would do).
	 */
	bool (*adapt_foreign_part_info)(
	    const struct disk_partitions *myself, struct disk_part_info *dest,
	    const struct disk_partitioning_scheme *src_scheme,
	    const struct disk_part_info *src);

	/*
	 * Update data for an existing partition
	 */
	bool (*set_part_info)(struct disk_partitions*, part_id,
	    const struct disk_part_info*, const char **err_msg);

	/* Add a new partition and return its part_id. */
	part_id (*add_partition)(struct disk_partitions*,
	    const struct disk_part_info*, const char **err_msg);

	/*
	 * Optional: add a partition from an outer scheme, accept all
	 * details w/o verification as best as possible.
	 */
	part_id (*add_outer_partition)(struct disk_partitions*,
	    const struct disk_part_info*, const char **err_msg);

	/* Delete all partitions */
	bool (*delete_all_partitions)(struct disk_partitions*);

	/* Optional: delete any partitions inside the given range */
	bool (*delete_partitions_in_range)(struct disk_partitions*,
	    daddr_t start, daddr_t size);

	/* Delete the specified partition */
	bool (*delete_partition)(struct disk_partitions*, part_id,
	    const char **err_msg);

	/*
	 * Methods for the whole set of partitions
	 */
	/*
	 * If this scheme only creates a singly NetBSD partition, which
	 * then is sub-partitioned (usually by disklabel), this returns a
	 * pointer to the secondary partition set.
	 * Otherwise NULL is returned, e.g. when there is no
	 * NetBSD partition defined (so this might change over time).
	 * Schemes that NEVER use a secondary scheme set this
	 * function pointer to NULL.
	 *
	 * If force_empty = true, ignore all on-disk contents and just
	 * create a new disk_partitions structure for the secondary scheme
	 * (this is used after deleting all partitions and setting up
	 * things for "use whole disk").
	 *
	 * The returned pointer is always owned by the primary partitions,
	 * caller MUST never free it, but otherwise can manipulate it
	 * arbitrarily.
	 */
	struct disk_partitions *
	    (*secondary_partitions)(struct disk_partitions *, daddr_t start,
	        bool force_empty);

	/*
	 * Write the whole set (in new_state) back to disk.
	 */
	bool (*write_to_disk)(struct disk_partitions *new_state);

	/*
	 * Try to read partitions from a disk, return NULL if this is not
	 * the partitioning scheme in use on that device.
	 * Usually start and len are 0 (and ignored).
	 * If this is about a part of a disk (like only the NetBSD
	 * MBR partition, start and len are the valid part of the
	 * disk.
	 */
	struct disk_partitions * (*read_from_disk)(const char *,
	    daddr_t start, daddr_t len, size_t bytes_per_sec,
	    const struct disk_partitioning_scheme *);

	/*
	 * Set up all internal data for a new disk.
	 */
	struct disk_partitions * (*create_new_for_disk)(const char *,
	    daddr_t start, daddr_t len, bool is_boot_drive,
	    struct disk_partitions *parent);

	/*
	 * Optional: this scheme may be used to boot from the given disk
	 */
	bool (*have_boot_support)(const char *disk);

	/*
	 * Optional: try to guess disk geometry from the partition information
	 */
	int (*guess_disk_geom)(struct disk_partitions *,
	    int *cyl, int *head, int *sec);

	/*
	 * Return a "cylinder size" (in number of blocks) - whatever that
	 * means to a particular partitioning scheme.
	 */
	size_t (*get_cylinder_size)(const struct disk_partitions *);

	/*
	 * Optional: change used geometry info and update internal state
	 */
	bool (*change_disk_geom)(struct disk_partitions *,
	    int cyl, int head, int sec);

	/*
	 * Optional:
	 * Get or set a name for the whole disk (most partitioning
	 * schemes do not provide this). Used for disklabel "pack names",
	 * which then may be used for aut-discovery of wedges, so it
	 * makes sense for the user to edit them.
	 */
	bool (*get_disk_pack_name)(const struct disk_partitions *,
	    char *, size_t);
	bool (*set_disk_pack_name)(struct disk_partitions *, const char *);

	/*
	 * Optional:
	 * Find a partition by name (as used in /etc/fstab NAME= entries)
	 */
	part_id (*find_by_name)(struct disk_partitions *, const char *name);

	/*
	 * Optional:
	 * Try to guess install target partition from internal data,
	 * returns true if a safe match was found and sets start/size
	 * to the target partition.
	 */
	bool (*guess_install_target)(const struct disk_partitions *,
		daddr_t *start, daddr_t *size);

	/*
	 * Optional: verify that the whole set of partitions would be bootable,
	 * fix up any issues (with user interaction) where needed.
	 * If "quiet" is true, fix up everything silently if possible
	 * and never return 1.
	 * Returns:
	 *  0: abort install
	 *  1: re-edit partitions
	 *  2: use anyway (continue)
	 */
	int (*post_edit_verify)(struct disk_partitions *, bool quiet);

	/*
	 * Optional: called during updates, before mounting the target disk(s),
	 * before md_pre_update() is called. Can be used to fixup
	 * partition info for historic errors (e.g. i386 changing MBR
	 * partition type from 165 to 169), similar to post_edit_verify.
	 * Returns:
	 *   true if the partition info has changed (write back required)
	 *   false if nothing further needs to be done.
	 */
	bool (*pre_update_verify)(struct disk_partitions *);

	/* Free all the data */
	void (*free)(struct disk_partitions*);

	/* Wipe all on-disk state, leave blank disk - and free data */
	void (*destroy_part_scheme)(struct disk_partitions*);

	/* Scheme global cleanup */
	void (*cleanup)(void);
};

/*
 * The in-memory representation of all partitions on a concrete disk,
 * tied to the partitioning scheme in use.
 *
 * Concrete schemes will derive from the abstract disk_partitions
 * structure (by aggregation), but consumers of the API will only
 * ever see this public part.
 */
struct disk_partitions {
	/* which partitioning scheme is in use */
	const struct disk_partitioning_scheme *pscheme;

	/* the disk device this came from (or should go to) */
	const char *disk;

	/* global/public disk data */

	/*
	 * The basic unit of size used for this disk (all "start",
	 * "size" and "align" values are in this unit).
	 */
	size_t bytes_per_sector;	/* must be 2^n and >= 512 */

	/*
	 * Valid partitions may have IDs in the range 0 .. num_part (excl.)
	 */
	part_id num_part;

	/*
	 * If this is a sub-partitioning, the start of the "disk" is
	 * some arbitrary partition in the parent. Sometimes we need
	 * to be able to calculate absoluted offsets.
	 */
	daddr_t disk_start;
	/*
	 * Total size of the disk (usable for partitioning)
	 */
	daddr_t disk_size;

	/*
	 * Space not yet allocated
	 */
	daddr_t free_space;

	/*
	 * If this is the secondary partitioning scheme, pointer to
	 * the outer one. Otherwise NULL.
	 */
	struct disk_partitions *parent;
};

/*
 * A list of partitioning schemes, so we can iterate over everything
 * supported (e.g. when partitioning a new disk). NULL terminated.
 */
extern const struct disk_partitioning_scheme **available_part_schemes;
extern size_t num_available_part_schemes;

/*
 * Generic reader - query a disk device and read all partitions from it
 */
struct disk_partitions *
partitions_read_disk(const char *, daddr_t disk_size,
    size_t bytes_per_sector, bool no_mbr);

/*
 * Generic part info adaption, may be overridden by individual partitioning
 * schemes
 */
bool generic_adapt_foreign_part_info(
    const struct disk_partitions *myself, struct disk_part_info *dest,
    const struct disk_partitioning_scheme *src_scheme,
    const struct disk_part_info *src);

/*
 * One time initialization and cleanup
 */
void partitions_init(void);
void partitions_cleanup(void);

