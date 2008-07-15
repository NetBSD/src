/*
 * Copyright (C) 2001 Sistina Software (UK) Limited.
 *
 * This file is released under the GPL.
 */

#include "dm.h"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <linux/slab.h>

MODULE_AUTHOR("Christophe Saout <christophe@saout.de>");
MODULE_DESCRIPTION(DM_NAME " target for reverse block mapping");
MODULE_LICENSE("GPL");

struct reverse_c {
	uint32_t chunk_shift;
	sector_t chunk_mask;
	sector_t chunk_last;

	struct dm_dev *dev;
	sector_t start;
};

/*
 * Construct a reverse mapping.
 * <chunk size (2^^n)> <dev_path> <offset>
 */
static int reverse_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
	struct reverse_c *rc;
	uint32_t chunk_size;
	sector_t chunk_count;
	char *end;

	if (argc != 3) {
		ti->error = "dm-reverse: Not enough arguments";
		return -EINVAL;
	}

	chunk_size = simple_strtoul(argv[0], &end, 10);
	if (*end) {
		ti->error = "dm-reverse: Invalid chunk_size";
		return -EINVAL;
	}

	/*
	 * chunk_size is a power of two
	 */
	if (!chunk_size || (chunk_size & (chunk_size - 1))) {
		ti->error = "dm-reverse: Invalid chunk size";
		return -EINVAL;
	}

	/*
	 * How do we handle a small last <-> first chunk? 
	 * We simply don't... so
	 * length has to be a multiple of the chunk size
	 */
	chunk_count = ti->len;
	if (sector_div(chunk_count, chunk_size) > 0) {
		ti->error = "dm-reverse: Size must be a multiple of the chunk size";
		return -EINVAL;
	}

	rc = kmalloc(sizeof(*rc), GFP_KERNEL);
	if (rc == NULL) {
		ti->error = "dm-reverse: Cannot allocate reverse context ";
		return -ENOMEM;
	}

	if (sscanf(argv[2], SECTOR_FORMAT, &rc->start) != 1) {
		ti->error = "dm-reverse: Invalid device sector";
		goto bad;
	}

	if (dm_get_device(ti, argv[1], rc->start, ti->len,
	                  dm_table_get_mode(ti->table), &rc->dev)) {
		ti->error = "dm-reverse: Device lookup failed";
		goto bad;
	}

	ti->split_io = chunk_size;

	rc->chunk_last = chunk_count - 1;
	rc->chunk_mask = ((sector_t) chunk_size) - 1;
	for (rc->chunk_shift = 0; chunk_size; rc->chunk_shift++)
		chunk_size >>= 1;
	rc->chunk_shift--;

	ti->private = rc;
	return 0;

bad:
	kfree(rc);
	return -EINVAL;
}

static void reverse_dtr(struct dm_target *ti)
{
	struct reverse_c *rc = (struct reverse_c *) ti->private;

	dm_put_device(ti, rc->dev);
	kfree(rc);
}

static int reverse_map(struct dm_target *ti, struct bio *bio)
{
	struct reverse_c *rc = (struct reverse_c *) ti->private;

	sector_t offset = bio->bi_sector - ti->begin;
	uint32_t chunk = (uint32_t) (offset >> rc->chunk_shift);

	chunk = rc->chunk_last - chunk;

	bio->bi_bdev = rc->dev->bdev;
	bio->bi_sector = rc->start + (chunk << rc->chunk_shift)
	                 + (offset & rc->chunk_mask);
	return 1;
}

static int reverse_status(struct dm_target *ti,
			 status_type_t type, char *result, unsigned int maxlen)
{
	struct reverse_c *rc = (struct reverse_c *) ti->private;
	char b[BDEVNAME_SIZE];

	switch (type) {
	case STATUSTYPE_INFO:
		result[0] = '\0';
		break;

	case STATUSTYPE_TABLE:
		snprintf(result, maxlen, SECTOR_FORMAT " %s " SECTOR_FORMAT,
		         rc->chunk_mask + 1, bdevname(rc->dev->bdev, b), rc->start);
		break;
	}
	return 0;
}

static struct target_type reverse_target = {
	.name   = "reverse",
	.module = THIS_MODULE,
	.ctr    = reverse_ctr,
	.dtr    = reverse_dtr,
	.map    = reverse_map,
	.status = reverse_status,
};

int __init dm_reverse_init(void)
{
	int r;

	r = dm_register_target(&reverse_target);
	if (r < 0)
		DMWARN("reverse target registration failed");

	return r;
}

void dm_reverse_exit(void)
{
	if (dm_unregister_target(&reverse_target))
		DMWARN("reverse target unregistration failed");

	return;
}

module_init(dm_reverse_init)
module_exit(dm_reverse_exit)
