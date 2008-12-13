/*	$NetBSD: dev-md.c,v 1.1.1.1.2.3 2008/12/13 14:39:33 haad Exp $	*/

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

/*
 * Retrieve chunk size from md device using sysfs.
 */
unsigned long dev_md_chunk_size(const char *sysfs_dir, struct device *dev)
{
	char path[PATH_MAX+1], buffer[64];
	FILE *fp;
	struct stat info;
	unsigned long chunk_size_bytes = 0UL;

	if (MAJOR(dev->dev) != md_major())
		return 0;

	if (!sysfs_dir || !*sysfs_dir)
		return_0;

	if (dm_snprintf(path, PATH_MAX, "%s/dev/block/%d:%d/md/chunk_size",
	    sysfs_dir, MAJOR(dev->dev), MINOR(dev->dev)) < 0) {
		log_error("dm_snprintf md chunk_size failed");
		return 0;
	}

	/* old sysfs structure */
	if (stat(path, &info) &&
	    dm_snprintf(path, PATH_MAX, "%s/block/md%d/md/chunk_size",
			sysfs_dir, MINOR(dev->dev)) < 0) {
		log_error("dm_snprintf old md chunk size failed");
		return 0;
	}

	if (!(fp = fopen(path, "r"))) {
		log_sys_error("fopen", path);
		return 0;
	}

	if (!fgets(buffer, sizeof(buffer), fp)) {
		log_sys_error("fgets", path);
		goto out;
	}

	if (sscanf(buffer, "%lu", &chunk_size_bytes) != 1) {
		log_error("sysfs file %s not in expected format: %s", path,
			  buffer);
		goto out;
	}

	log_very_verbose("Device %s md chunk size is %lu bytes.",
			 dev_name(dev), chunk_size_bytes);

out:
	if (fclose(fp))
		log_sys_error("fclose", path);

	return chunk_size_bytes >> SECTOR_SHIFT;
}

#else

int dev_is_md(struct device *dev __attribute((unused)),
	      uint64_t *sb __attribute((unused)))
{
	return 0;
}

unsigned long dev_md_chunk_size(const char *sysfs_dir __attribute((unused)),
				struct device *dev  __attribute((unused)))
{
	return 0UL;
}

#endif
