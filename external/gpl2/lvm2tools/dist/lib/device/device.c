/*	$NetBSD: device.c,v 1.1.1.1.2.3 2008/12/13 14:39:33 haad Exp $	*/

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
