/*        $NetBSD: dm.h,v 1.56 2021/08/21 22:23:33 andvar Exp $      */

/*
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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

#ifndef _DM_H_
#define _DM_H_

#ifdef _KERNEL

#include <sys/errno.h>
#include <sys/atomic.h>
#include <sys/fcntl.h>
#include <sys/condvar.h>
#include <sys/kauth.h>
#include <sys/mutex.h>
#include <sys/rwlock.h>
#include <sys/queue.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>

#include <miscfs/specfs/specdev.h> /* for v_rdev */

#include <prop/proplib.h>

#define DM_MAX_TYPE_NAME 16
#define DM_NAME_LEN 128
#define DM_UUID_LEN 129

#define DM_VERSION_MAJOR	4
#define DM_VERSION_MINOR	16

#define DM_VERSION_PATCHLEVEL	0

/*** Internal device-mapper structures ***/

extern const struct dkdriver dmdkdriver;
extern uint32_t dm_dev_counter;

typedef struct dm_mapping {
	union {
		struct dm_pdev *pdev;
	} data;
	TAILQ_ENTRY(dm_mapping) next;
} dm_mapping_t;

/*
 * A device mapper table is a list of physical ranges plus the mapping target
 * applied to them.
 */
typedef struct dm_table_entry {
	struct dm_dev *dm_dev;		/* backlink */
	uint64_t start;
	uint64_t length;

	struct dm_target *target;      /* Link to table target. */
	void *target_config;           /* Target specific data. */
	SLIST_ENTRY(dm_table_entry) next;

	TAILQ_HEAD(, dm_mapping) pdev_maps;
} dm_table_entry_t;

SLIST_HEAD(dm_table, dm_table_entry);

typedef struct dm_table dm_table_t;

typedef struct dm_table_head {
	/* Current active table is selected with this. */
	int cur_active_table;
	dm_table_t tables[2];

	kmutex_t   table_mtx;
	kcondvar_t table_cv; /* I/O waiting cv */

	uint32_t io_cnt;
} dm_table_head_t;

#define MAX_DEV_NAME 32

/*
 * This structure is used to store opened vnodes for disk with name.
 * I need this because devices can be opened only once, but I can
 * have more than one device on one partition.
 */
typedef struct dm_pdev {
	char name[MAX_DEV_NAME];
	char udev_name[MAX_DEV_NAME];

	struct vnode *pdev_vnode;
	uint64_t pdev_numsec;
	unsigned int pdev_secsize;
	int ref_cnt; /* reference counter for users of this pdev */

	SLIST_ENTRY(dm_pdev) next_pdev;
} dm_pdev_t;

/*
 * This structure is called for every device-mapper device.
 * It points to SLIST of device tables and mirrored, snapshoted etc. devices.
 */
TAILQ_HEAD(dm_dev_head, dm_dev);
//extern struct dm_dev_head dm_devs;

typedef struct dm_dev {
	char name[DM_NAME_LEN];
	char uuid[DM_UUID_LEN];

	device_t devt; /* pointer to autoconf device_t structure */
	uint64_t minor; /* Device minor number */
	uint32_t flags; /* store communication protocol flags */

	kmutex_t dev_mtx; /* mutex for general device lock */
	kcondvar_t dev_cv; /* cv for between ioctl synchronisation */

	uint32_t event_nr;
	uint32_t ref_cnt;

	dm_table_head_t table_head;

	//struct dm_dev_head upcalls;

	struct disk *diskp;
	kmutex_t diskp_mtx;

	//TAILQ_ENTRY(dm_dev) next_upcall; /* LIST of mirrored, snapshoted devices. */

	TAILQ_ENTRY(dm_dev) next_devlist; /* Major device list. */
} dm_dev_t;

/* For linear target. */
typedef struct target_linear_config {
	dm_pdev_t *pdev;
	uint64_t offset;
	TAILQ_ENTRY(target_linear_config) entries;
} dm_target_linear_config_t;

/*
 * Striping devices are stored in a linked list, this might be inefficient
 * for more than 8 striping devices and can be changed to something more
 * scalable.
 * TODO: look for other options than linked list.
 */
TAILQ_HEAD(target_linear_devs, target_linear_config);

typedef struct target_linear_devs dm_target_linear_devs_t;

/* constant dm_target structures for error, zero, linear, stripes etc. */
typedef struct dm_target {
	char name[DM_MAX_TYPE_NAME];
	/* Initialize target_config area */
	int (*init)(dm_table_entry_t *, int, char **);

	/* Destroy target_config area */
	int (*destroy)(dm_table_entry_t *);

	int (*strategy)(dm_table_entry_t *, struct buf *);
	//int (*upcall)(dm_table_entry_t *, struct buf *);

	/*
	 * Optional routines.
	 */
	/*
	 * Info/table routine are called to get params string, which is target
	 * specific. When dm_table_status_ioctl is called with flag
	 * DM_STATUS_TABLE_FLAG I have to sent params string back.
	 */
	char *(*info)(void *);
	char *(*table)(void *);
	int (*sync)(dm_table_entry_t *);
	int (*secsize)(dm_table_entry_t *, unsigned int *);

	uint32_t version[3];
	uint32_t ref_cnt;
	int max_argc;

	TAILQ_ENTRY(dm_target) dm_target_next;
} dm_target_t;

/* device-mapper */
void dmgetproperties(struct disk *, dm_table_head_t *);

/* Generic function used to convert char to string */
uint64_t atoi64(const char *);

/* dm_ioctl.c */
int dm_dev_create_ioctl(prop_dictionary_t);
int dm_dev_list_ioctl(prop_dictionary_t);
int dm_dev_remove_ioctl(prop_dictionary_t);
int dm_dev_rename_ioctl(prop_dictionary_t);
int dm_dev_resume_ioctl(prop_dictionary_t);
int dm_dev_status_ioctl(prop_dictionary_t);
int dm_dev_suspend_ioctl(prop_dictionary_t);

int dm_check_version(prop_dictionary_t);
int dm_list_versions_ioctl(prop_dictionary_t);

int dm_table_clear_ioctl(prop_dictionary_t);
int dm_table_deps_ioctl(prop_dictionary_t);
int dm_table_load_ioctl(prop_dictionary_t);
int dm_table_status_ioctl(prop_dictionary_t);

/* dm_target.c */
dm_target_t* dm_target_alloc(const char *);
dm_target_t* dm_target_autoload(const char *);
int dm_target_destroy(void);
int dm_target_insert(dm_target_t *);
prop_array_t dm_target_prop_list(void);
dm_target_t* dm_target_lookup(const char *);
int dm_target_rem(const char *);
void dm_target_unbusy(dm_target_t *);
void dm_target_busy(dm_target_t *);
int dm_target_init(void);

#define DM_MAX_PARAMS_SIZE 1024

/* dm_target_linear.c */
int dm_target_linear_init(dm_table_entry_t *, int, char **);
char *dm_target_linear_table(void *);
int dm_target_linear_strategy(dm_table_entry_t *, struct buf *);
int dm_target_linear_sync(dm_table_entry_t *);
int dm_target_linear_destroy(dm_table_entry_t *);
//int dm_target_linear_upcall(dm_table_entry_t *, struct buf *);
int dm_target_linear_secsize(dm_table_entry_t *, unsigned int *);

/* dm_target_stripe.c */
int dm_target_stripe_init(dm_table_entry_t *, int, char **);
char *dm_target_stripe_info(void *);
char *dm_target_stripe_table(void *);
int dm_target_stripe_strategy(dm_table_entry_t *, struct buf *);
int dm_target_stripe_sync(dm_table_entry_t *);
int dm_target_stripe_destroy(dm_table_entry_t *);
//int dm_target_stripe_upcall(dm_table_entry_t *, struct buf *);
int dm_target_stripe_secsize(dm_table_entry_t *, unsigned int *);

/* dm_target_error.c */
int dm_target_error_init(dm_table_entry_t*, int, char **);
int dm_target_error_strategy(dm_table_entry_t *, struct buf *);
int dm_target_error_destroy(dm_table_entry_t *);
//int dm_target_error_upcall(dm_table_entry_t *, struct buf *);

/* dm_target_zero.c */
int dm_target_zero_init(dm_table_entry_t *, int, char **);
int dm_target_zero_strategy(dm_table_entry_t *, struct buf *);
int dm_target_zero_destroy(dm_table_entry_t *);
//int dm_target_zero_upcall(dm_table_entry_t *, struct buf *);

#if 0
/* dm_target_delay.c */
void dm_target_delay_pool_create(void);
void dm_target_delay_pool_destroy(void);
int dm_target_delay_init(dm_table_entry_t *, int, char **);
char *dm_target_delay_info(void *);
char *dm_target_delay_table(void *);
int dm_target_delay_strategy(dm_table_entry_t *, struct buf *);
int dm_target_delay_sync(dm_table_entry_t *);
int dm_target_delay_destroy(dm_table_entry_t *);
//int dm_target_delay_upcall(dm_table_entry_t *, struct buf *);
int dm_target_delay_secsize(dm_table_entry_t *, unsigned int *);

/* dm_target_flakey.c */
int dm_target_flakey_init(dm_table_entry_t *, int, char **);
char *dm_target_flakey_table(void *);
int dm_target_flakey_strategy(dm_table_entry_t *, struct buf *);
int dm_target_flakey_sync(dm_table_entry_t *);
int dm_target_flakey_destroy(dm_table_entry_t *);
//int dm_target_flakey_upcall(dm_table_entry_t *, struct buf *);
int dm_target_flakey_secsize(dm_table_entry_t *, unsigned int *);
#endif

/* dm_table.c  */
#define DM_TABLE_ACTIVE 0
#define DM_TABLE_INACTIVE 1

int dm_table_destroy(dm_table_head_t *, uint8_t);
uint64_t dm_table_size(dm_table_head_t *);
uint64_t dm_inactive_table_size(dm_table_head_t *);
void dm_table_disksize(dm_table_head_t *, uint64_t *, unsigned int *);
dm_table_t *dm_table_get_entry(dm_table_head_t *, uint8_t);
int dm_table_get_target_count(dm_table_head_t *, uint8_t);
void dm_table_release(dm_table_head_t *, uint8_t s);
void dm_table_switch_tables(dm_table_head_t *);
void dm_table_head_init(dm_table_head_t *);
void dm_table_head_destroy(dm_table_head_t *);
int dm_table_add_deps(dm_table_entry_t *, dm_pdev_t *);

/* dm_dev.c */
dm_dev_t* dm_dev_alloc(void);
void dm_dev_busy(dm_dev_t *);
int dm_dev_destroy(void);
dm_dev_t* dm_dev_detach(device_t);
int dm_dev_free(dm_dev_t *);
int dm_dev_init(void);
int dm_dev_insert(dm_dev_t *);
dm_dev_t* dm_dev_lookup(const char *, const char *, int);
prop_array_t dm_dev_prop_list(void);
dm_dev_t* dm_dev_rem(const char *, const char *, int);
/*int dm_dev_test_minor(int);*/
void dm_dev_unbusy(dm_dev_t *);

/* dm_pdev.c */
int dm_pdev_decr(dm_pdev_t *);
int dm_pdev_destroy(void);
int dm_pdev_init(void);
dm_pdev_t* dm_pdev_insert(const char *);

#endif /*_KERNEL*/

#endif /*_DM_H_*/
