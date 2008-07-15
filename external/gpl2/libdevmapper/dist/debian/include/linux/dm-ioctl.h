/*
 * Copyright (C) 2001 Sistina Software (UK) Limited.
 *
 * This file is released under the LGPL.
 */

#ifndef _LINUX_DM_IOCTL_H
#define _LINUX_DM_IOCTL_H

#include "device-mapper.h"
#include <linux/types.h>

/*
 * Implements a traditional ioctl interface to the device mapper.
 */

/*
 * All ioctl arguments consist of a single chunk of memory, with
 * this structure at the start.  If a uuid is specified any
 * lookup (eg. for a DM_INFO) will be done on that, *not* the
 * name.
 */
struct dm_ioctl {
	/*
	 * The version number is made up of three parts:
	 * major - no backward or forward compatibility,
	 * minor - only backwards compatible,
	 * patch - both backwards and forwards compatible.
	 *
	 * All clients of the ioctl interface should fill in the
	 * version number of the interface that they were
	 * compiled with.
	 *
	 * All recognised ioctl commands (ie. those that don't
	 * return -ENOTTY) fill out this field, even if the
	 * command failed.
	 */
	uint32_t version[3];	/* in/out */
	uint32_t data_size;	/* total size of data passed in
				 * including this struct */

	uint32_t data_start;	/* offset to start of data
				 * relative to start of this struct */

	uint32_t target_count;	/* in/out */
	uint32_t open_count;	/* out */
	uint32_t flags;		/* in/out */

	__kernel_dev_t dev;	/* in/out */

	char name[DM_NAME_LEN];	/* device name */
	char uuid[DM_UUID_LEN];	/* unique identifier for
				 * the block device */
};

/*
 * Used to specify tables.  These structures appear after the
 * dm_ioctl.
 */
struct dm_target_spec {
	int32_t status;		/* used when reading from kernel only */
	uint64_t sector_start;
	uint32_t length;

	/*
	 * Offset in bytes (from the start of this struct) to
	 * next target_spec.
	 */
	uint32_t next;

	char target_type[DM_MAX_TYPE_NAME];

	/*
	 * Parameter string starts immediately after this object.
	 * Be careful to add padding after string to ensure correct
	 * alignment of subsequent dm_target_spec.
	 */
};

/*
 * Used to retrieve the target dependencies.
 */
struct dm_target_deps {
	uint32_t count;

	__kernel_dev_t dev[0];	/* out */
};

/*
 * If you change this make sure you make the corresponding change
 * to dm-ioctl.c:lookup_ioctl()
 */
enum {
	/* Top level cmds */
	DM_VERSION_CMD = 0,
	DM_REMOVE_ALL_CMD,

	/* device level cmds */
	DM_DEV_CREATE_CMD,
	DM_DEV_REMOVE_CMD,
	DM_DEV_RELOAD_CMD,
	DM_DEV_RENAME_CMD,
	DM_DEV_SUSPEND_CMD,
	DM_DEV_DEPS_CMD,
	DM_DEV_STATUS_CMD,

	/* target level cmds */
	DM_TARGET_STATUS_CMD,
	DM_TARGET_WAIT_CMD
};

#define DM_IOCTL 0xfd

#define DM_VERSION       _IOWR(DM_IOCTL, DM_VERSION_CMD, struct dm_ioctl)
#define DM_REMOVE_ALL    _IOWR(DM_IOCTL, DM_REMOVE_ALL_CMD, struct dm_ioctl)

#define DM_DEV_CREATE    _IOWR(DM_IOCTL, DM_DEV_CREATE_CMD, struct dm_ioctl)
#define DM_DEV_REMOVE    _IOWR(DM_IOCTL, DM_DEV_REMOVE_CMD, struct dm_ioctl)
#define DM_DEV_RELOAD    _IOWR(DM_IOCTL, DM_DEV_RELOAD_CMD, struct dm_ioctl)
#define DM_DEV_SUSPEND   _IOWR(DM_IOCTL, DM_DEV_SUSPEND_CMD, struct dm_ioctl)
#define DM_DEV_RENAME    _IOWR(DM_IOCTL, DM_DEV_RENAME_CMD, struct dm_ioctl)
#define DM_DEV_DEPS      _IOWR(DM_IOCTL, DM_DEV_DEPS_CMD, struct dm_ioctl)
#define DM_DEV_STATUS    _IOWR(DM_IOCTL, DM_DEV_STATUS_CMD, struct dm_ioctl)

#define DM_TARGET_STATUS _IOWR(DM_IOCTL, DM_TARGET_STATUS_CMD, struct dm_ioctl)
#define DM_TARGET_WAIT   _IOWR(DM_IOCTL, DM_TARGET_WAIT_CMD, struct dm_ioctl)

#define DM_VERSION_MAJOR	1
#define DM_VERSION_MINOR	0
#define DM_VERSION_PATCHLEVEL	3
#define DM_VERSION_EXTRA	"-ioctl (2002-08-14)"

/* Status bits */
#define DM_READONLY_FLAG	0x00000001
#define DM_SUSPEND_FLAG		0x00000002
#define DM_EXISTS_FLAG		0x00000004
#define DM_PERSISTENT_DEV_FLAG	0x00000008

/*
 * Flag passed into ioctl STATUS command to get table information
 * rather than current status.
 */
#define DM_STATUS_TABLE_FLAG	0x00000010

#endif				/* _LINUX_DM_IOCTL_H */
