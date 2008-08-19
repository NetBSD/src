/*        $NetBSD: dm.h,v 1.1.2.9 2008/08/19 23:36:18 haad Exp $      */

/*
 * Copyright (c) 1996, 1997, 1998, 1999, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Hamsik.
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

#ifndef _DM_DEV_H_
#define _DM_DEV_H_


#ifdef _KERNEL

#include <sys/fcntl.h>
#include <sys/mutex.h>
#include <sys/vnode.h>
#include <sys/queue.h>

#define DM_MAX_TYPE_NAME 16
#define DM_NAME_LEN 128
#define DM_UUID_LEN 129

#define DM_VERSION_MAJOR	4
#define DM_VERSION_MINOR	13
#define DM_VERSION_PATCHLEVEL	0


/*
 * XXX - Do we need this? Can there be more than one devmapper in the system?
 */
struct dm_softc {
	uint8_t       sc_ref_count;
	uint32_t      sc_minor_num;
};


/*** Internal device-mapper structures ***/

/*
 * A table entry describes a physical range of the logical volume.
 */
#define MAX_TARGET_STRING_LEN 32

/*
 * A device mapper table is a list of physical ranges plus the mapping target
 * applied to them.
 */

struct dm_table_entry {
	struct dm_dev *dm_dev;		/* backlink */
	uint64_t start;
	uint64_t length;

	struct dm_target *target;      /* Link to table target. */
	void *target_config;           /* Target specific data. */
	SLIST_ENTRY(dm_table_entry) next;
};
SLIST_HEAD(dm_table, dm_table_entry);

#define MAX_DEV_NAME 32

/*
 * This structure is used to store opened vnodes for disk with name.
 * I need this because devices can be opened only once, but I can
 * have more then one device on one partition.
 */

struct dm_pdev {
	char name[MAX_DEV_NAME];

	struct vnode *pdev_vnode;
	int ref_cnt;

	SLIST_ENTRY(dm_pdev) next_pdev;
};
SLIST_HEAD(dm_pdevs, dm_pdev) dm_pdev_list;


/*
 * This structure is called for every device-mapper device.
 * It points to SLIST of device tables and mirrored, snapshoted etc. devices.
 */
TAILQ_HEAD(dm_dev_head, dm_dev) dm_devs;
struct dm_dev {
	char name[DM_NAME_LEN];
	char uuid[DM_UUID_LEN];

	int minor;
	uint32_t flags;

	kmutex_t dev_mtx; /* between ioctl routines lock */
	krwlock_t dev_rwlock; /* dmstrategy -> ioctl routines lock */
	
	uint32_t event_nr;
	uint32_t ref_cnt;

	uint32_t dev_type;

#define DM_ZERO_DEV      (1 << 0)
#define DM_ERROR_DEV     (1 << 1)	
#define DM_LINEAR_DEV    (1 << 2)
#define DM_MIRROR_DEV    (1 << 3)
#define DM_STRIPE_DEV    (1 << 4)
#define DM_SNAPSHOT_DEV  (1 << 5)
#define DM_SPARE_DEV     (1 << 6)
	
	struct dm_pdevs pdevs;

	/* Current active table is selected with this. */
	int cur_active_table; 
	struct dm_table tables[2];

	struct dm_dev_head upcalls;

	
	struct disklabel *dm_dklabel;

	TAILQ_ENTRY(dm_dev) next_upcall; /* LIST of mirrored, snapshoted devices. */

        TAILQ_ENTRY(dm_dev) next_devlist; /* Major device list. */
};


/* for zero,error : dm_target->target_config == NULL */
				
/*
 * Target config is initiated with target_init function.
 */
				
/* for linear : */
struct target_linear_config {
	struct dm_pdev *pdev;
	uint64_t offset;
};


/* for mirror : */
struct target_mirror_config {
#define MAX_MIRROR_COPIES 4
	struct dm_pdev *orig;
	struct dm_pdev *copies[MAX_MIRROR_COPIES];

	/* copied blocks bitmaps administration etc*/
	struct dm_pdev *log_pdev;	/* for administration */
	uint64_t log_regionsize;	/* blocksize of mirror */

	/* list of parts that still need copied etc.; run length encoded? */
};


/* for snapshot : */
struct target_snapshot_config {
	struct dm_dev *orig;

	/* modified blocks bitmaps administration etc*/
	struct dm_pdev *log_pdev;
	uint64_t log_regionsize;
	/* list of sector renames to the log device */
};

/* constant dm_target structures for error, zero, linear, stripes etc. */
struct dm_target {
	char name[DM_MAX_TYPE_NAME];
	/* Initialize target_config area */
	int (*init)(struct dm_dev *, void **, char *);

	/* Destroy target_config area */
	int (*destroy)(struct dm_table_entry *);
	
        /*
	 * Status routine is called to get params string, which is target
	 * specific. When dm_table_status_ioctl is called with flag
	 * DM_STATUS_TABLE_FLAG I have to sent params string back.
	 */
	char * (*status)(void *);
	int (*strategy)(struct dm_table_entry *, struct buf *);
	int (*upcall)(struct dm_table_entry *, struct buf *);

	uint32_t version[3];

	TAILQ_ENTRY(dm_target) dm_target_next;
};

/* Interface structures */

/*
 * This structure is used to translate command sent to kernel driver in
 * <key>command</key>
 * <value></value>
 * to function which I can call.
 */
struct cmd_function {
	const char *cmd;
	int  (*fn)(prop_dictionary_t);
};

/* dm_ioctl.c */
int dm_dev_create_ioctl(prop_dictionary_t);
int dm_dev_list_ioctl(prop_dictionary_t);
int dm_dev_remove_ioctl(prop_dictionary_t);
int dm_dev_rename_ioctl(prop_dictionary_t);
int dm_dev_resume_ioctl(prop_dictionary_t);
int dm_dev_status_ioctl(prop_dictionary_t);
int dm_dev_suspend_ioctl(prop_dictionary_t);

int dm_check_version(prop_dictionary_t);

int dm_get_version_ioctl(prop_dictionary_t);
int dm_list_versions_ioctl(prop_dictionary_t);

int dm_table_clear_ioctl(prop_dictionary_t);
int dm_table_deps_ioctl(prop_dictionary_t);
int dm_table_load_ioctl(prop_dictionary_t);
int dm_table_status_ioctl(prop_dictionary_t);


/* dm_target.c */
int dm_target_destroy(void);
int dm_target_insert(struct dm_target *);
prop_array_t dm_target_prop_list(void);
struct dm_target* dm_target_lookup_name(const char *);
int dm_target_rem(char *);

/* XXX temporally add */
int dm_target_init(void);

/* dm_target_zero.c */
int dm_target_zero_init(struct dm_dev *, void**,  char *);
char * dm_target_zero_status(void *);
int dm_target_zero_strategy(struct dm_table_entry *, struct buf *);
int dm_target_zero_destroy(struct dm_table_entry *);
int dm_target_zero_upcall(struct dm_table_entry *, struct buf *);

/* dm_target_error.c */
int dm_target_error_init(struct dm_dev *, void**, char *);
char * dm_target_error_status(void *);
int dm_target_error_strategy(struct dm_table_entry *, struct buf *);
int dm_target_error_destroy(struct dm_table_entry *);
int dm_target_error_upcall(struct dm_table_entry *, struct buf *);

/* dm_target_linear.c */
int dm_target_linear_init(struct dm_dev *, void**, char *);
char * dm_target_linear_status(void *);
int dm_target_linear_strategy(struct dm_table_entry *, struct buf *);
int dm_target_linear_destroy(struct dm_table_entry *);
int dm_target_linear_upcall(struct dm_table_entry *, struct buf *);

/* dm_target_snapshot.c */
int dm_target_snapshot_init(struct dm_dev *, void**, char *);
char * dm_target_snapshot_status(void *);
int dm_target_snapshot_strategy(struct dm_table_entry *, struct buf *);
int dm_target_snapshot_destroy(struct dm_table_entry *);
int dm_target_snapshot_upcall(struct dm_table_entry *, struct buf *);

/* dm_table.c  */
int dm_table_destroy(struct dm_table *);

/* dm_dev.c */
struct dm_dev* dm_dev_alloc(void);
int dm_dev_destroy(void);
prop_array_t dm_dev_prop_list(void);
int dm_dev_free(struct dm_dev *);
int dm_dev_init(void);
int dm_dev_insert(struct dm_dev *);
struct dm_dev* dm_dev_lookup_name(const char *);
struct dm_dev* dm_dev_lookup_uuid(const char *);
struct dm_dev* dm_dev_lookup_minor(int);
int dm_dev_rem(const char *);

/* dm_pdev.c */
int dm_pdev_decr(struct dm_pdev *);
int dm_pdev_destroy(void);
int dm_pdev_init(void);
struct dm_pdev* dm_pdev_insert(const char *);
struct dm_pdev* dm_pdev_lookup_name(const char *);

#endif /*_KERNEL*/

#endif /*_DM_DEV_H_*/
