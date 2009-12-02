/*	$NetBSD: dev-md.c,v 1.1.1.2 2009/12/02 00:26:33 haad Exp $	*/

/*
 * Copyright (C) 2004 Luca Berra
 * Copyright (C) 2004-2008 Red Hat, Inc. All rights reserved.
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

#include "lib.h"
#include "metadata.h"
#include "xlate.h"
#include "filter.h"

#ifdef linux

/* Lifted from <linux/raid/md_p.h> because of difficulty including it */

#define MD_SB_MAGIC 0xa92b4efc
#define MD_RESERVED_BYTES (64 * 1024ULL)
#define MD_RESERVED_SECTORS (MD_RESERVED_BYTES / 512)
#define MD_NEW_SIZE_SECTORS(x) ((x & ~(MD_RESERVED_SECTORS - 1)) \
				- MD_RESERVED_SECTORS)

static int _dev_has_md_magic(struct device *dev, uint64_t sb_offset)
{
	uint32_t md_magic;

	/* Version 1 is little endian; version 0.90.0 is machine endian */
	if (dev_read(dev, sb_offset, sizeof(uint32_t), &md_magic) &&
	    ((md_magic == xlate32(MD_SB_MAGIC)) ||
	     (md_magic == MD_SB_MAGIC)))
		return 1;

	return 0;
}

/*
 * Calculate the position of the superblock.
 * It is always aligned to a 4K boundary and
 * depending on minor_version, it can be:
 * 0: At least 8K, but less than 12K, from end of device
 * 1: At start of device
 * 2: 4K from start of device.
 */
typedef enum {
	MD_MINOR_VERSION_MIN,
	MD_MINOR_V0 = MD_MINOR_VERSION_MIN,
	MD_MINOR_V1,
	MD_MINOR_V2,
	MD_MINOR_VERSION_MAX = MD_MINOR_V2
} md_minor_version_t;

static uint64_t _v1_sb_offset(uint64_t size, md_minor_version_t minor_version)
{
	uint64_t uninitialized_var(sb_offset);

	switch(minor_version) {
	case MD_MINOR_V0:
		sb_offset = (size - 8 * 2) & ~(4 * 2 - 1ULL);
		break;
	case MD_MINOR_V1:
		sb_offset = 0;
		break;
	case MD_MINOR_V2:
		sb_offset = 4 * 2;
		break;
	}
	sb_offset <<= SECTOR_SHIFT;

	return sb_offset;
}

/*
 * Returns -1 on error
 */
int dev_is_md(struct device *dev, uint64_t *sb)
{
	int ret = 1;
	md_minor_version_t minor;
	uint64_t size, sb_offset;

	if (!dev_get_size(dev, &size)) {
		stack;
		return -1;
	}

	if (size < MD_RESERVED_SECTORS * 2)
		return 0;

	if (!dev_open(dev)) {
		stack;
		return -1;
	}

	/* Check if it is an md component device. */
	/* Version 0.90.0 */
	sb_offset = MD_NEW_SIZE_SECTORS(size) << SECTOR_SHIFT;
	if (_dev_has_md_magic(dev, sb_offset))
		goto out;

	minor = MD_MINOR_VERSION_MIN;
	/* Version 1, try v1.0 -> v1.2 */
	do {
		sb_offset = _v1_sb_offset(size, minor);
		if (_dev_has_md_magic(dev, sb_offset))
			goto out;
	} while (++minor <= MD_MINOR_VERSION_MAX);

	ret = 0;

out:
	if (!dev_close(dev))
		stack;

	if (ret && sb)
		*sb = sb_offset;

	return ret;
}

static int _md_sysfs_attribute_snprintf(char *path, size_t size,
					const char *sysfs_dir,
					struct device *blkdev,
					const char *attribute)
{
	struct stat info;
	dev_t dev = blkdev->dev;
	int ret = -1;

	if (!sysfs_dir || !*sysfs_dir)
		return ret;

	if (MAJOR(dev) == blkext_major()) {
		/* lookup parent MD device from blkext partition */
		if (!get_primary_dev(sysfs_dir, blkdev, &dev))
			return ret;
	}

	if (MAJOR(dev) != md_major())
		return ret;

	ret = dm_snprintf(path, size, "%s/dev/block/%d:%d/md/%s", sysfs_dir,
			  (int)MAJOR(dev), (int)MINOR(dev), attribute);
	if (ret < 0) {
		log_error("dm_snprintf md %s failed", attribute);
		return ret;
	}

	if (stat(path, &info) == -1) {
		if (errno != ENOENT) {
			log_sys_error("stat", path);
			return ret;
		}
		/* old sysfs structure */
		ret = dm_snprintf(path, size, "%s/block/md%d/md/%s",
				  sysfs_dir, (int)MINOR(dev), attribute);
		if (ret < 0) {
			log_error("dm_snprintf old md %s failed", attribute);
			return ret;
		}
	}

	return ret;
}

static int _md_sysfs_attribute_scanf(const char *sysfs_dir,
				     struct device *dev,
				     const char *attribute_name,
				     const char *attribute_fmt,
				     void *attribute_value)
{
	char path[PATH_MAX+1], buffer[64];
	FILE *fp;
	int ret = 0;

	if (_md_sysfs_attribute_snprintf(path, PATH_MAX, sysfs_dir,
					 dev, attribute_name) < 0)
		return ret;

	if (!(fp = fopen(path, "r"))) {
		log_sys_error("fopen", path);
		return ret;
	}

	if (!fgets(buffer, sizeof(buffer), fp)) {
		log_sys_error("fgets", path);
		goto out;
	}

	if ((ret = sscanf(buffer, attribute_fmt, attribute_value)) != 1) {
		log_error("%s sysfs attr %s not in expected format: %s",
			  dev_name(dev), attribute_name, buffer);
		goto out;
	}

out:
	if (fclose(fp))
		log_sys_error("fclose", path);

	return ret;
}

/*
 * Retrieve chunk size from md device using sysfs.
 */
static unsigned long dev_md_chunk_size(const char *sysfs_dir,
				       struct device *dev)
{
	const char *attribute = "chunk_size";
	unsigned long chunk_size_bytes = 0UL;

	if (_md_sysfs_attribute_scanf(sysfs_dir, dev, attribute,
				      "%lu", &chunk_size_bytes) != 1)
		return 0;

	log_very_verbose("Device %s %s is %lu bytes.",
			 dev_name(dev), attribute, chunk_size_bytes);

	return chunk_size_bytes >> SECTOR_SHIFT;
}

/*
 * Retrieve level from md device using sysfs.
 */
static int dev_md_level(const char *sysfs_dir, struct device *dev)
{
	const char *attribute = "level";
	int level = -1;

	if (_md_sysfs_attribute_scanf(sysfs_dir, dev, attribute,
				      "raid%d", &level) != 1)
		return -1;

	log_very_verbose("Device %s %s is raid%d.",
			 dev_name(dev), attribute, level);

	return level;
}

/*
 * Retrieve raid_disks from md device using sysfs.
 */
static int dev_md_raid_disks(const char *sysfs_dir, struct device *dev)
{
	const char *attribute = "raid_disks";
	int raid_disks = 0;

	if (_md_sysfs_attribute_scanf(sysfs_dir, dev, attribute,
				      "%d", &raid_disks) != 1)
		return 0;

	log_very_verbose("Device %s %s is %d.",
			 dev_name(dev), attribute, raid_disks);

	return raid_disks;
}

/*
 * Calculate stripe width of md device using its sysfs files.
 */
unsigned long dev_md_stripe_width(const char *sysfs_dir, struct device *dev)
{
	unsigned long chunk_size_sectors = 0UL;
	unsigned long stripe_width_sectors = 0UL;
	int level, raid_disks, data_disks;

	chunk_size_sectors = dev_md_chunk_size(sysfs_dir, dev);
	if (!chunk_size_sectors)
		return 0;

	level = dev_md_level(sysfs_dir, dev);
	if (level < 0)
		return 0;

	raid_disks = dev_md_raid_disks(sysfs_dir, dev);
	if (!raid_disks)
		return 0;

	/* The raid level governs the number of data disks. */
	switch (level) {
	case 0:
		/* striped md does not have any parity disks */
		data_disks = raid_disks;
		break;
	case 1:
	case 10:
		/* mirrored md effectively has 1 data disk */
		data_disks = 1;
		break;
	case 4:
	case 5:
		/* both raid 4 and 5 have a single parity disk */
		data_disks = raid_disks - 1;
		break;
	case 6:
		/* raid 6 has 2 parity disks */
		data_disks = raid_disks - 2;
		break;
	default:
		log_error("Device %s has an unknown md raid level: %d",
			  dev_name(dev), level);
		return 0;
	}

	stripe_width_sectors = chunk_size_sectors * data_disks;

	log_very_verbose("Device %s stripe-width is %lu bytes.",
			 dev_name(dev),
			 stripe_width_sectors << SECTOR_SHIFT);

	return stripe_width_sectors;
}

#else

int dev_is_md(struct device *dev __attribute((unused)),
	      uint64_t *sb __attribute((unused)))
{
	return 0;
}

unsigned long dev_md_stripe_width(const char *sysfs_dir __attribute((unused)),
				  struct device *dev  __attribute((unused)))
{
	return 0UL;
}

#endif
