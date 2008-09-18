/*
 * Copyright (C) 2003 Sistina Software
 *
 * This file is released under the LGPL.
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/vmalloc.h>

#include "dm-log.h"
#include "dm-io.h"

static LIST_HEAD(_log_types);
static spinlock_t _lock = SPIN_LOCK_UNLOCKED;

int dm_register_dirty_log_type(struct dirty_log_type *type)
{
	spin_lock(&_lock);
	type->use_count = 0;
	if (type->module)
		__MOD_INC_USE_COUNT(type->module);

	list_add(&type->list, &_log_types);
	spin_unlock(&_lock);

	return 0;
}

int dm_unregister_dirty_log_type(struct dirty_log_type *type)
{
	spin_lock(&_lock);

	if (type->use_count)
		DMWARN("Attempt to unregister a log type that is still in use");
	else {
		list_del(&type->list);
		if (type->module)
			__MOD_DEC_USE_COUNT(type->module);
	}

	spin_unlock(&_lock);

	return 0;
}

static struct dirty_log_type *get_type(const char *type_name)
{
	struct dirty_log_type *type;
	struct list_head *tmp;

	spin_lock(&_lock);
	list_for_each (tmp, &_log_types) {
		type = list_entry(tmp, struct dirty_log_type, list);
		if (!strcmp(type_name, type->name)) {
			type->use_count++;
			spin_unlock(&_lock);
			return type;
		}
	}

	spin_unlock(&_lock);
	return NULL;
}

static void put_type(struct dirty_log_type *type)
{
	spin_lock(&_lock);
	type->use_count--;
	spin_unlock(&_lock);
}

struct dirty_log *dm_create_dirty_log(const char *type_name, sector_t dev_size,
				      unsigned int argc, char **argv)
{
	struct dirty_log_type *type;
	struct dirty_log *log;

	log = kmalloc(sizeof(*log), GFP_KERNEL);
	if (!log)
		return NULL;

	type = get_type(type_name);
	if (!type) {
		kfree(log);
		return NULL;
	}

	log->type = type;
	if (type->ctr(log, dev_size, argc, argv)) {
		kfree(log);
		put_type(type);
		return NULL;
	}

	return log;
}

void dm_destroy_dirty_log(struct dirty_log *log)
{
	log->type->dtr(log);
	put_type(log->type);
	kfree(log);
}


/*-----------------------------------------------------------------
 * In core log, ie. trivial, non-persistent
 *
 * For now we'll keep this simple and just have 2 bitsets, one
 * for clean/dirty, the other for sync/nosync.  The sync bitset
 * will be freed when everything is in sync.
 *
 * FIXME: problems with a 64bit sector_t
 *---------------------------------------------------------------*/
struct core_log {
	sector_t region_size;
	unsigned int region_count;
	unsigned long *clean_bits;
	unsigned long *sync_bits;
	unsigned long *recovering_bits;	/* FIXME: this seems excessive */

	int sync_search;
};

#define BYTE_SHIFT 3

static int core_ctr(struct dirty_log *log, sector_t dev_size,
		    unsigned int argc, char **argv)
{
	struct core_log *clog;
	sector_t region_size;
	unsigned int region_count;
	size_t bitset_size;

	if (argc != 1) {
		DMWARN("wrong number of arguments to core_log");
		return -EINVAL;
	}

	if (sscanf(argv[0], SECTOR_FORMAT, &region_size) != 1) {
		DMWARN("invalid region size string");
		return -EINVAL;
	}

	region_count = dm_div_up(dev_size, region_size);

	clog = kmalloc(sizeof(*clog), GFP_KERNEL);
	if (!clog) {
		DMWARN("couldn't allocate core log");
		return -ENOMEM;
	}

	clog->region_size = region_size;
	clog->region_count = region_count;

	/*
 	 * Work out how many words we need to hold the bitset.
 	 */
	bitset_size = dm_round_up(region_count,
				  sizeof(*clog->clean_bits) << BYTE_SHIFT);
	bitset_size >>= BYTE_SHIFT;

	clog->clean_bits = vmalloc(bitset_size);
	if (!clog->clean_bits) {
		DMWARN("couldn't allocate clean bitset");
		kfree(clog);
		return -ENOMEM;
	}
	memset(clog->clean_bits, -1, bitset_size);

	clog->sync_bits = vmalloc(bitset_size);
	if (!clog->sync_bits) {
		DMWARN("couldn't allocate sync bitset");
		vfree(clog->clean_bits);
		kfree(clog);
		return -ENOMEM;
	}
	memset(clog->sync_bits, 0, bitset_size);

	clog->recovering_bits = vmalloc(bitset_size);
	if (!clog->recovering_bits) {
		DMWARN("couldn't allocate sync bitset");
		vfree(clog->sync_bits);
		vfree(clog->clean_bits);
		kfree(clog);
		return -ENOMEM;
	}
	memset(clog->recovering_bits, 0, bitset_size);
	clog->sync_search = 0;
	log->context = clog;
	return 0;
}

static void core_dtr(struct dirty_log *log)
{
	struct core_log *clog = (struct core_log *) log->context;
	vfree(clog->clean_bits);
	vfree(clog->sync_bits);
	vfree(clog->recovering_bits);
	kfree(clog);
}

static sector_t core_get_region_size(struct dirty_log *log)
{
	struct core_log *clog = (struct core_log *) log->context;
	return clog->region_size;
}

static int core_is_clean(struct dirty_log *log, region_t region)
{
	struct core_log *clog = (struct core_log *) log->context;
	return test_bit(region, clog->clean_bits);
}

static int core_in_sync(struct dirty_log *log, region_t region, int block)
{
	struct core_log *clog = (struct core_log *) log->context;

	return test_bit(region, clog->sync_bits) ? 1 : 0;
}

static int core_flush(struct dirty_log *log)
{
	/* no op */
	return 0;
}

static void core_mark_region(struct dirty_log *log, region_t region)
{
	struct core_log *clog = (struct core_log *) log->context;
	clear_bit(region, clog->clean_bits);
}

static void core_clear_region(struct dirty_log *log, region_t region)
{
	struct core_log *clog = (struct core_log *) log->context;
	set_bit(region, clog->clean_bits);
}

static int core_get_resync_work(struct dirty_log *log, region_t *region)
{
	struct core_log *clog = (struct core_log *) log->context;

	if (clog->sync_search >= clog->region_count)
		return 0;

	do {
		*region = find_next_zero_bit(clog->sync_bits,
					     clog->region_count,
					     clog->sync_search);
		clog->sync_search = *region + 1;

		if (*region == clog->region_count)
			return 0;

	} while (test_bit(*region, clog->recovering_bits));

	set_bit(*region, clog->recovering_bits);
	return 1;
}

static void core_complete_resync_work(struct dirty_log *log, region_t region,
				      int success)
{
	struct core_log *clog = (struct core_log *) log->context;

	clear_bit(region, clog->recovering_bits);
	if (success)
		set_bit(region, clog->sync_bits);
}

static struct dirty_log_type _core_type = {
	.name = "core",

	.ctr = core_ctr,
	.dtr = core_dtr,
	.get_region_size = core_get_region_size,
	.is_clean = core_is_clean,
	.in_sync = core_in_sync,
	.flush = core_flush,
	.mark_region = core_mark_region,
	.clear_region = core_clear_region,
	.get_resync_work = core_get_resync_work,
	.complete_resync_work = core_complete_resync_work
};

__init int dm_dirty_log_init(void)
{
	int r;

	r = dm_register_dirty_log_type(&_core_type);
	if (r)
		DMWARN("couldn't register core log");

	return r;
}

void dm_dirty_log_exit(void)
{
	dm_unregister_dirty_log_type(&_core_type);
}

EXPORT_SYMBOL(dm_register_dirty_log_type);
EXPORT_SYMBOL(dm_unregister_dirty_log_type);
EXPORT_SYMBOL(dm_dirty_log_init);
EXPORT_SYMBOL(dm_dirty_log_exit);
EXPORT_SYMBOL(dm_create_dirty_log);
EXPORT_SYMBOL(dm_destroy_dirty_log);
