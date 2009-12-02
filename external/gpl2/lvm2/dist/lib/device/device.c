/*	$NetBSD: device.c,v 1.1.1.2 2009/12/02 00:26:34 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
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

#include <libgen.h> /* dirname, basename */
#include "lib.h"
#include "lvm-types.h"
#include "device.h"
#include "metadata.h"
#include "filter.h"
#include "xlate.h"

/* See linux/genhd.h and fs/partitions/msdos */

#define PART_MAGIC 0xAA55
#define PART_MAGIC_OFFSET UINT64_C(0x1FE)
#define PART_OFFSET UINT64_C(0x1BE)

struct partition {
	uint8_t boot_ind;
	uint8_t head;
	uint8_t sector;
	uint8_t cyl;
	uint8_t sys_ind;	/* partition type */
	uint8_t end_head;
	uint8_t end_sector;
	uint8_t end_cyl;
	uint32_t start_sect;
	uint32_t nr_sects;
} __attribute__((packed));

static int _is_partitionable(struct device *dev)
{
	int parts = max_partitions(MAJOR(dev->dev));

	/* All MD devices are partitionable via blkext (as of 2.6.28) */
	if (MAJOR(dev->dev) == md_major())
		return 1;

	if ((parts <= 1) || (MINOR(dev->dev) % parts))
		return 0;

	return 1;
}

static int _has_partition_table(struct device *dev)
{
	int ret = 0;
	unsigned p;
	uint16_t buf[SECTOR_SIZE/sizeof(uint16_t)];
	uint16_t *part_magic;
	struct partition *part;

	if (!dev_open(dev)) {
		stack;
		return -1;
	}

	if (!dev_read(dev, UINT64_C(0), sizeof(buf), &buf))
		goto_out;

	/* FIXME Check for other types of partition table too */

	/* Check for msdos partition table */
	part_magic = buf + PART_MAGIC_OFFSET/sizeof(buf[0]);
	if ((*part_magic == xlate16(PART_MAGIC))) {
		part = (struct partition *) (buf + PART_OFFSET/sizeof(buf[0]));
		for (p = 0; p < 4; p++, part++) {
			/* Table is invalid if boot indicator not 0 or 0x80 */
			if ((part->boot_ind & 0x7f)) {
				ret = 0;
				break;
			}
			/* Must have at least one non-empty partition */
			if (part->nr_sects)
				ret = 1;
		}
	}

      out:
	if (!dev_close(dev))
		stack;

	return ret;
}

int is_partitioned_dev(struct device *dev)
{
	if (!_is_partitionable(dev))
		return 0;

	return _has_partition_table(dev);
}

#if 0
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#include <errno.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/genhd.h>

int _get_partition_type(struct dev_filter *filter, struct device *d);

#define MINOR_PART(dev) (MINOR((dev)->dev) % max_partitions(MINOR((dev)->dev)))

int is_extended_partition(struct device *d)
{
	return (MINOR_PART(d) > 4) ? 1 : 0;
}

struct device *dev_primary(struct dev_mgr *dm, struct device *d)
{
	struct device *ret;

	ret = dev_by_dev(dm, d->dev - MINOR_PART(dm, d));
	/* FIXME: Needs replacing with a 'refresh' */
	if (!ret) {
		init_dev_scan(dm);
		ret = dev_by_dev(dm, d->dev - MINOR_PART(dm, d));
	}

	return ret;

}

int partition_type_is_lvm(struct dev_mgr *dm, struct device *d)
{
	int pt;

	pt = _get_partition_type(dm, d);

	if (!pt) {
		if (is_whole_disk(dm, d))
			/* FIXME: Overloaded pt=0 in error cases */
			return 1;
		else {
			log_error
			    ("%s: missing partition table "
			     "on partitioned device", d->name);
			return 0;
		}
	}

	if (is_whole_disk(dm, d)) {
		log_error("%s: looks to possess partition table", d->name);
		return 0;
	}

	/* check part type */
	if (pt != LVM_PARTITION && pt != LVM_NEW_PARTITION) {
		log_error("%s: invalid partition type 0x%x "
			  "(must be 0x%x)", d->name, pt, LVM_NEW_PARTITION);
		return 0;
	}

	if (pt == LVM_PARTITION) {
		log_error
		    ("%s: old LVM partition type found - please change to 0x%x",
		     d->name, LVM_NEW_PARTITION);
		return 0;
	}

	return 1;
}

int _get_partition_type(struct dev_mgr *dm, struct device *d)
{
	int pv_handle = -1;
	struct device *primary;
	ssize_t read_ret;
	ssize_t bytes_read = 0;
	char *buffer;
	unsigned short *s_buffer;
	struct partition *part;
	loff_t offset = 0;
	loff_t extended_offset = 0;
	int part_sought;
	int part_found = 0;
	int first_partition = 1;
	int extended_partition = 0;
	int p;

	if (!(primary = dev_primary(dm, d))) {
		log_error
		    ("Failed to find main device containing partition %s",
		     d->name);
		return 0;
	}

	if (!(buffer = dm_malloc(SECTOR_SIZE))) {
		log_error("Failed to allocate partition table buffer");
		return 0;
	}

	/* Get partition table */
	if ((pv_handle = open(primary->name, O_RDONLY)) < 0) {
		log_error("%s: open failed: %s", primary->name,
			  strerror(errno));
		return 0;
	}

	s_buffer = (unsigned short *) buffer;
	part = (struct partition *) (buffer + 0x1be);
	part_sought = MINOR_PART(dm, d);

	do {
		bytes_read = 0;

		if (llseek(pv_handle, offset * SECTOR_SIZE, SEEK_SET) == -1) {
			log_error("%s: llseek failed: %s",
				  primary->name, strerror(errno));
			return 0;
		}

		while ((bytes_read < SECTOR_SIZE) &&
		       (read_ret =
			read(pv_handle, buffer + bytes_read,
			     SECTOR_SIZE - bytes_read)) != -1)
			bytes_read += read_ret;

		if (read_ret == -1) {
			log_error("%s: read failed: %s", primary->name,
				  strerror(errno));
			return 0;
		}

		if (s_buffer[255] == 0xAA55) {
			if (is_whole_disk(dm, d))
				return -1;
		} else
			return 0;

		extended_partition = 0;

		/* Loop through primary partitions */
		for (p = 0; p < 4; p++) {
			if (part[p].sys_ind == DOS_EXTENDED_PARTITION ||
			    part[p].sys_ind == LINUX_EXTENDED_PARTITION
			    || part[p].sys_ind == WIN98_EXTENDED_PARTITION) {
				extended_partition = 1;
				offset = extended_offset + part[p].start_sect;
				if (extended_offset == 0)
					extended_offset = part[p].start_sect;
				if (first_partition == 1)
					part_found++;
			} else if (first_partition == 1) {
				if (p == part_sought) {
					if (part[p].sys_ind == 0) {
						/* missing primary? */
						return 0;
					}
				} else
					part_found++;
			} else if (!part[p].sys_ind)
				part_found++;

			if (part_sought == part_found)
				return part[p].sys_ind;

		}
		first_partition = 0;
	}
	while (extended_partition == 1);

	return 0;
}
#endif

#ifdef linux

int get_primary_dev(const char *sysfs_dir,
		    struct device *dev, dev_t *result)
{
	char path[PATH_MAX+1];
	char temp_path[PATH_MAX+1];
	char buffer[64];
	struct stat info;
	FILE *fp;
	uint32_t pri_maj, pri_min;
	int ret = 0;

	/* check if dev is a partition */
	if (dm_snprintf(path, PATH_MAX, "%s/dev/block/%d:%d/partition",
			sysfs_dir, (int)MAJOR(dev->dev), (int)MINOR(dev->dev)) < 0) {
		log_error("dm_snprintf partition failed");
		return ret;
	}

	if (stat(path, &info) == -1) {
		if (errno != ENOENT)
			log_sys_error("stat", path);
		return ret;
	}

	/*
	 * extract parent's path from the partition's symlink, e.g.:
	 * - readlink /sys/dev/block/259:0 = ../../block/md0/md0p1
	 * - dirname ../../block/md0/md0p1 = ../../block/md0
	 * - basename ../../block/md0/md0  = md0
	 * Parent's 'dev' sysfs attribute  = /sys/block/md0/dev
	 */
	if (readlink(dirname(path), temp_path, PATH_MAX) < 0) {
		log_sys_error("readlink", path);
		return ret;
	}

	if (dm_snprintf(path, PATH_MAX, "%s/block/%s/dev",
			sysfs_dir, basename(dirname(temp_path))) < 0) {
		log_error("dm_snprintf dev failed");
		return ret;
	}

	/* finally, parse 'dev' attribute and create corresponding dev_t */
	if (stat(path, &info) == -1) {
		if (errno == ENOENT)
			log_error("sysfs file %s does not exist", path);
		else
			log_sys_error("stat", path);
		return ret;
	}

	fp = fopen(path, "r");
	if (!fp) {
		log_sys_error("fopen", path);
		return ret;
	}

	if (!fgets(buffer, sizeof(buffer), fp)) {
		log_sys_error("fgets", path);
		goto out;
	}

	if (sscanf(buffer, "%d:%d", &pri_maj, &pri_min) != 2) {
		log_error("sysfs file %s not in expected MAJ:MIN format: %s",
			  path, buffer);
		goto out;
	}
	*result = MKDEV(pri_maj, pri_min);
	ret = 1;

out:
	if (fclose(fp))
		log_sys_error("fclose", path);

	return ret;
}

static unsigned long _dev_topology_attribute(const char *attribute,
					     const char *sysfs_dir,
					     struct device *dev)
{
	const char *sysfs_fmt_str = "%s/dev/block/%d:%d/%s";
	char path[PATH_MAX+1], buffer[64];
	FILE *fp;
	struct stat info;
	dev_t uninitialized_var(primary);
	unsigned long result = 0UL;

	if (!attribute || !*attribute)
		return_0;

	if (!sysfs_dir || !*sysfs_dir)
		return_0;

	if (dm_snprintf(path, PATH_MAX, sysfs_fmt_str, sysfs_dir,
			(int)MAJOR(dev->dev), (int)MINOR(dev->dev),
			attribute) < 0) {
		log_error("dm_snprintf %s failed", attribute);
		return 0;
	}

	/*
	 * check if the desired sysfs attribute exists
	 * - if not: either the kernel doesn't have topology support
	 *   or the device could be a partition
	 */
	if (stat(path, &info) == -1) {
		if (errno != ENOENT) {
			log_sys_error("stat", path);
			return 0;
		}
		if (!get_primary_dev(sysfs_dir, dev, &primary))
			return 0;

		/* get attribute from partition's primary device */
		if (dm_snprintf(path, PATH_MAX, sysfs_fmt_str, sysfs_dir,
				(int)MAJOR(primary), (int)MINOR(primary),
				attribute) < 0) {
			log_error("primary dm_snprintf %s failed", attribute);
			return 0;
		}
		if (stat(path, &info) == -1) {
			if (errno != ENOENT)
				log_sys_error("stat", path);
			return 0;
		}
	}

	if (!(fp = fopen(path, "r"))) {
		log_sys_error("fopen", path);
		return 0;
	}

	if (!fgets(buffer, sizeof(buffer), fp)) {
		log_sys_error("fgets", path);
		goto out;
	}

	if (sscanf(buffer, "%lu", &result) != 1) {
		log_error("sysfs file %s not in expected format: %s", path,
			  buffer);
		goto out;
	}

	log_very_verbose("Device %s %s is %lu bytes.",
			 dev_name(dev), attribute, result);

out:
	if (fclose(fp))
		log_sys_error("fclose", path);

	return result >> SECTOR_SHIFT;
}

unsigned long dev_alignment_offset(const char *sysfs_dir,
				   struct device *dev)
{
	return _dev_topology_attribute("alignment_offset",
				       sysfs_dir, dev);
}

unsigned long dev_minimum_io_size(const char *sysfs_dir,
				  struct device *dev)
{
	return _dev_topology_attribute("queue/minimum_io_size",
				       sysfs_dir, dev);
}

unsigned long dev_optimal_io_size(const char *sysfs_dir,
				  struct device *dev)
{
	return _dev_topology_attribute("queue/optimal_io_size",
				       sysfs_dir, dev);
}

#else

int get_primary_dev(const char *sysfs_dir,
		    struct device *dev, dev_t *result)
{
	return 0;
}

unsigned long dev_alignment_offset(const char *sysfs_dir,
				   struct device *dev)
{
	return 0UL;
}

unsigned long dev_minimum_io_size(const char *sysfs_dir,
				  struct device *dev)
{
	return 0UL;
}

unsigned long dev_optimal_io_size(const char *sysfs_dir,
				  struct device *dev)
{
	return 0UL;
}

#endif
