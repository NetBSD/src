/*	$NetBSD: device.h,v 1.1.1.2 2009/12/02 00:25:44 haad Exp $	*/

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

#ifndef _LVM_DEVICE_H
#define _LVM_DEVICE_H

#include "uuid.h"

#include <fcntl.h>

#define DEV_ACCESSED_W		0x00000001	/* Device written to? */
#define DEV_REGULAR		0x00000002	/* Regular file? */
#define DEV_ALLOCED		0x00000004	/* dm_malloc used */
#define DEV_OPENED_RW		0x00000008	/* Opened RW */
#define DEV_OPENED_EXCL		0x00000010	/* Opened EXCL */
#define DEV_O_DIRECT		0x00000020	/* Use O_DIRECT */
#define DEV_O_DIRECT_TESTED	0x00000040	/* DEV_O_DIRECT is reliable */

/*
 * All devices in LVM will be represented by one of these.
 * pointer comparisons are valid.
 */
struct device {
	struct dm_list aliases;	/* struct str_list from lvm-types.h */
	dev_t dev;

	/* private */
	int fd;
	int open_count;
	int block_size;
	int read_ahead;
	uint32_t flags;
	uint64_t end;
	struct dm_list open_list;

	char pvid[ID_LEN + 1];
	char _padding[7];
};

struct device_list {
	struct dm_list list;
	struct device *dev;
};

struct device_area {
	struct device *dev;
	uint64_t start;		/* Bytes */
	uint64_t size;		/* Bytes */
};

/*
 * All io should use these routines.
 */
int dev_get_size(const struct device *dev, uint64_t *size);
int dev_get_sectsize(struct device *dev, uint32_t *size);
int dev_get_read_ahead(struct device *dev, uint32_t *read_ahead);

/* Use quiet version if device number could change e.g. when opening LV */
int dev_open(struct device *dev);
int dev_open_quiet(struct device *dev);
int dev_open_flags(struct device *dev, int flags, int direct, int quiet);
int dev_close(struct device *dev);
int dev_close_immediate(struct device *dev);
void dev_close_all(void);
int dev_test_excl(struct device *dev);

int dev_fd(struct device *dev);
const char *dev_name(const struct device *dev);

int dev_read(struct device *dev, uint64_t offset, size_t len, void *buffer);
int dev_read_circular(struct device *dev, uint64_t offset, size_t len,
		      uint64_t offset2, size_t len2, void *buf);
int dev_write(struct device *dev, uint64_t offset, size_t len, void *buffer);
int dev_append(struct device *dev, size_t len, void *buffer);
int dev_set(struct device *dev, uint64_t offset, size_t len, int value);
void dev_flush(struct device *dev);

struct device *dev_create_file(const char *filename, struct device *dev,
			       struct str_list *alias, int use_malloc);

/* Return a valid device name from the alias list; NULL otherwise */
const char *dev_name_confirmed(struct device *dev, int quiet);

/* Does device contain md superblock?  If so, where? */
int dev_is_md(struct device *dev, uint64_t *sb);
int dev_is_swap(struct device *dev, uint64_t *signature);
unsigned long dev_md_stripe_width(const char *sysfs_dir, struct device *dev);

int is_partitioned_dev(struct device *dev);

int get_primary_dev(const char *sysfs_dir,
		    struct device *dev, dev_t *result);

unsigned long dev_alignment_offset(const char *sysfs_dir,
				   struct device *dev);

unsigned long dev_minimum_io_size(const char *sysfs_dir,
				  struct device *dev);

unsigned long dev_optimal_io_size(const char *sysfs_dir,
				  struct device *dev);

#endif
