/*	$NetBSD: disk_rep.h,v 1.1.1.2 2008/12/12 11:42:13 haad Exp $	*/

/*
 * Copyright (C) 1997-2004 Sistina Software, Inc. All rights reserved.  
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

#ifndef DISK_REP_FORMAT_POOL_H
#define DISK_REP_FORMAT_POOL_H

#include "label.h"
#include "metadata.h"

#define MINOR_OFFSET 65536

/* From NSP.cf */
#define NSPMajorVersion	4
#define NSPMinorVersion	1
#define NSPUpdateLevel	3

/* From pool_std.h */
#define POOL_NAME_SIZE          (256)
#define POOL_MAGIC 		0x011670
#define POOL_MAJOR              (121)
#define POOL_MAX_DEVICES 	128

/* When checking for version matching, the first two numbers **
** are important for metadata formats, a.k.a pool labels.   **
** All the numbers are important when checking if the user  **
** space tools match up with the kernel module............. */
#define POOL_VERSION		(NSPMajorVersion << 16 | \
				 NSPMinorVersion <<  8 | \
				 NSPUpdateLevel)

/* Pool label is at the head of every pool disk partition */
#define SIZEOF_POOL_LABEL       (8192)

/* in sectors */
#define POOL_PE_SIZE     (SIZEOF_POOL_LABEL >> SECTOR_SHIFT)
#define POOL_PE_START    (SIZEOF_POOL_LABEL >> SECTOR_SHIFT)

/* Helper fxns */
#define get_pool_vg_uuid(id, pd) do { get_pool_uuid((char *)(id), \
                                                    (pd)->pl_pool_id, 0, 0); \
                                    } while(0)
#define get_pool_pv_uuid(id, pd) do { get_pool_uuid((char *)(id), \
                                                    (pd)->pl_pool_id, \
                                                    (pd)->pl_sp_id, \
                                                    (pd)->pl_sp_devid); \
                                    } while(0)
#define get_pool_lv_uuid(id, pd) do { get_pool_uuid((char *)&(id)[0], \
                                                    (pd)->pl_pool_id, 0, 0); \
                                      get_pool_uuid((char*)&(id)[1], \
                                                    (pd)->pl_pool_id, 0, 0); \
                                    } while(0)

struct pool_disk;
struct pool_list;
struct user_subpool;
struct user_device;

struct pool_disk {
	uint64_t pl_magic;	/* Pool magic number */
	uint64_t pl_pool_id;	/* Unique pool identifier */
	char pl_pool_name[POOL_NAME_SIZE];	/* Name of pool */
	uint32_t pl_version;	/* Pool version */
	uint32_t pl_subpools;	/* Number of subpools in this pool */
	uint32_t pl_sp_id;	/* Subpool number within pool */
	uint32_t pl_sp_devs;	/* Number of data partitions in this subpool */
	uint32_t pl_sp_devid;	/* Partition number within subpool */
	uint32_t pl_sp_type;	/* Partition type */
	uint64_t pl_blocks;	/* Number of blocks in this partition */
	uint32_t pl_striping;	/* Striping size within subpool */
	/*
	 * If the number of DMEP devices is zero, then the next field **
	 * ** (pl_sp_dmepid) becomes the subpool ID for redirection.  In **
	 * ** other words, if this subpool does not have the capability  **
	 * ** to do DMEP, then it must specify which subpool will do it  **
	 * ** in it's place
	 */

	/*
	 * While the next 3 field are no longer used, they must stay to keep **
	 * ** backward compatibility...........................................
	 */
	uint32_t pl_sp_dmepdevs;/* Number of dmep devices in this subpool */
	uint32_t pl_sp_dmepid;	/* Dmep device number within subpool */
	uint32_t pl_sp_weight;	/* if dmep dev, pref to using it */

	uint32_t pl_minor;	/* the pool minor number */
	uint32_t pl_padding;	/* reminder - think about alignment */

	/*
	 * Even though we're zeroing out 8k at the front of the disk before
	 * writing the label, putting this in
	 */
	char pl_reserve[184];	/* bump the structure size out to 512 bytes */
};

struct pool_list {
	struct dm_list list;
	struct pool_disk pd;
	struct physical_volume *pv;
	struct id pv_uuid;
	struct device *dev;
};

struct user_subpool {
	uint32_t initialized;
	uint32_t id;
	uint32_t striping;
	uint32_t num_devs;
	uint32_t type;
	uint32_t dummy;
	struct user_device *devs;
};

struct user_device {
	uint32_t initialized;
	uint32_t sp_id;
	uint32_t devid;
	uint32_t dummy;
	uint64_t blocks;
	struct physical_volume *pv;
};

int read_pool_label(struct pool_list *pl, struct labeller *l,
		    struct device *dev, char *buf, struct label **label);
void pool_label_out(struct pool_disk *pl, void *buf);
void pool_label_in(struct pool_disk *pl, void *buf);
void get_pool_uuid(char *uuid, uint64_t poolid, uint32_t spid, uint32_t devid);
int import_pool_vg(struct volume_group *vg, struct dm_pool *mem, struct dm_list *pls);
int import_pool_lvs(struct volume_group *vg, struct dm_pool *mem,
		    struct dm_list *pls);
int import_pool_pvs(const struct format_type *fmt, struct volume_group *vg,
		    struct dm_list *pvs, struct dm_pool *mem, struct dm_list *pls);
int import_pool_pv(const struct format_type *fmt, struct dm_pool *mem,
		   struct volume_group *vg, struct physical_volume *pv,
		   struct pool_list *pl);
int import_pool_segments(struct dm_list *lvs, struct dm_pool *mem,
			 struct user_subpool *usp, int sp_count);
int read_pool_pds(const struct format_type *fmt, const char *vgname,
		  struct dm_pool *mem, struct dm_list *head);
struct pool_list *read_pool_disk(const struct format_type *fmt,
				 struct device *dev, struct dm_pool *mem,
				 const char *vg_name);

#endif				/* DISK_REP_POOL_FORMAT_H */
