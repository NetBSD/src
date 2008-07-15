/*
 * Internal header file for device mapper
 *
 * Copyright (C) 2001, 2002 Sistina Software
 *
 * This file is released under the LGPL.
 */

#ifndef DM_INTERNAL_H
#define DM_INTERNAL_H

#include <linux/fs.h>
#include <linux/device-mapper.h>
#include <linux/list.h>
#include <linux/blkdev.h>

#define DM_NAME "device-mapper"
#define DMWARN(f, x...) printk(KERN_WARNING DM_NAME ": " f "\n" , ## x)
#define DMERR(f, x...) printk(KERN_ERR DM_NAME ": " f "\n" , ## x)
#define DMINFO(f, x...) printk(KERN_INFO DM_NAME ": " f "\n" , ## x)

/*
 * FIXME: I think this should be with the definition of sector_t
 * in types.h.
 */
#ifdef CONFIG_LBD
#define SECTOR_FORMAT "%Lu"
#else
#define SECTOR_FORMAT "%lu"
#endif

#define SECTOR_SHIFT 9
#define SECTOR_SIZE (1 << SECTOR_SHIFT)

extern struct block_device_operations dm_blk_dops;

/*
 * List of devices that a metadevice uses and should open/close.
 */
struct dm_dev {
	struct list_head list;

	atomic_t count;
	int mode;
	kdev_t dev;
	struct block_device *bdev;
};

struct dm_table;
struct mapped_device;

/*-----------------------------------------------------------------
 * Functions for manipulating a struct mapped_device.
 * Drop the reference with dm_put when you finish with the object.
 *---------------------------------------------------------------*/
int dm_create(kdev_t dev, struct mapped_device **md);

/*
 * Reference counting for md.
 */
void dm_get(struct mapped_device *md);
void dm_put(struct mapped_device *md);

/*
 * A device can still be used while suspended, but I/O is deferred.
 */
int dm_suspend(struct mapped_device *md);
int dm_resume(struct mapped_device *md);

/*
 * The device must be suspended before calling this method.
 */
int dm_swap_table(struct mapped_device *md, struct dm_table *t);

/*
 * Drop a reference on the table when you've finished with the
 * result.
 */
struct dm_table *dm_get_table(struct mapped_device *md);

/*
 * Event functions.
 */
uint32_t dm_get_event_nr(struct mapped_device *md);
int dm_add_wait_queue(struct mapped_device *md, wait_queue_t *wq,
		      uint32_t event_nr);
void dm_remove_wait_queue(struct mapped_device *md, wait_queue_t *wq);

/*
 * Info functions.
 */
kdev_t dm_kdev(struct mapped_device *md);
int dm_suspended(struct mapped_device *md);

/*-----------------------------------------------------------------
 * Functions for manipulating a table.  Tables are also reference
 * counted.
 *---------------------------------------------------------------*/
int dm_table_create(struct dm_table **result, int mode, unsigned num_targets);

void dm_table_get(struct dm_table *t);
void dm_table_put(struct dm_table *t);

int dm_table_add_target(struct dm_table *t, const char *type,
			sector_t start,	sector_t len, char *params);
int dm_table_complete(struct dm_table *t);
void dm_table_event_callback(struct dm_table *t,
			     void (*fn)(void *), void *context);
void dm_table_event(struct dm_table *t);
sector_t dm_table_get_size(struct dm_table *t);
struct dm_target *dm_table_get_target(struct dm_table *t, unsigned int index);
struct dm_target *dm_table_find_target(struct dm_table *t, sector_t sector);
unsigned int dm_table_get_num_targets(struct dm_table *t);
struct list_head *dm_table_get_devices(struct dm_table *t);
int dm_table_get_mode(struct dm_table *t);
void dm_table_suspend_targets(struct dm_table *t);
void dm_table_resume_targets(struct dm_table *t);

/*-----------------------------------------------------------------
 * A registry of target types.
 *---------------------------------------------------------------*/
int dm_target_init(void);
void dm_target_exit(void);
struct target_type *dm_get_target_type(const char *name);
void dm_put_target_type(struct target_type *t);
int dm_target_iterate(void (*iter_func)(struct target_type *tt,
					void *param), void *param);


/*-----------------------------------------------------------------
 * Useful inlines.
 *---------------------------------------------------------------*/
static inline int array_too_big(unsigned long fixed, unsigned long obj,
				unsigned long num)
{
	return (num > (ULONG_MAX - fixed) / obj);
}

/*
 * ceiling(n / size) * size
 */
static inline unsigned long dm_round_up(unsigned long n, unsigned long size)
{
	unsigned long r = n % size;
	return n + (r ? (size - r) : 0);
}

/*
 * Ceiling(n / size)
 */
static inline unsigned long dm_div_up(unsigned long n, unsigned long size)
{
	return dm_round_up(n, size) / size;
}

const char *dm_kdevname(kdev_t dev);

/*
 * The device-mapper can be driven through one of two interfaces;
 * ioctl or filesystem, depending which patch you have applied.
 */
int dm_interface_init(void);
void dm_interface_exit(void);

/*
 * Targets for linear and striped mappings
 */
int dm_linear_init(void);
void dm_linear_exit(void);

int dm_stripe_init(void);
void dm_stripe_exit(void);

int dm_snapshot_init(void);
void dm_snapshot_exit(void);

#endif
