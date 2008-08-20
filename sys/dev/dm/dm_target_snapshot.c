/*        $NetBSD: dm_target_snapshot.c,v 1.1.2.6 2008/08/20 00:45:47 haad Exp $      */

/*
 * Copyright (c) 1996, 1997, 1998, 1999, 2002 The NetBSD Foundation, Inc.
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

/*
 * 1. Suspend my_data to temporarily stop any I/O while the snapshot is being
 * activated.
 * dmsetup suspend my_data
 *
 * 2. Create the snapshot-origin device with no table.
 * dmsetup create my_data_org
 *
 * 3. Read the table from my_data and load it into my_data_org.
 * dmsetup table my_data | dmsetup load my_data_org
 *
 * 4. Resume this new table.
 * dmsetup resume my_data_org
 *
 * 5. Create the snapshot device with no table.
 * dmsetup create my_data_snap
 *
 * 6. Load the table into my_data_snap. This uses /dev/hdd1 as the COW device and
 * uses a 32kB chunk-size.
 * echo "0 `blockdev --getsize /dev/mapper/my_data` snapshot \
 *  /dev/mapper/my_data_org /dev/hdd1 p 64" | dmsetup load my_data_snap
 *
 * 7. Reload my_data as a snapshot-origin device that points to my_data_org.
 * echo "0 `blockdev --getsize /dev/mapper/my_data` snapshot-origin \
 *  /dev/mapper/my_data_org" | dmsetup load my_data
 *
 * 8. Resume the snapshot and origin devices.
 * dmsetup resume my_data_snap
 * dmsetup resume my_data
 */

/*
 * This file implements initial version of device-mapper snapshot target.
 */
#include <sys/types.h>
#include <sys/param.h>

#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/ioccom.h>
#include <sys/kmem.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/queue.h>

#include "dm.h"

/*
 * Init function called from dm_table_load_ioctl.
 * argv:  /dev/mapper/my_data_org /dev/mapper/cow_dev p 64 
 *        snapshot_origin device, cow device, persistent flag, chunk size
 */
int
dm_target_snapshot_init(struct dm_dev *dmv, void **target_config, char *params)
{
	struct target_snapshot_config *tsc;

	char **ap, *argv[5];

	/*
	 * Parse a string, containing tokens delimited by white space,
	 * into an argument vector
	 */
	for (ap = argv; ap < &argv[4] &&
		 (*ap = strsep(&params, " \t")) != NULL;) {
		if (**ap != '\0')
			ap++;
	}
	
	printf("Snapshot target init function called!!\n");
	printf("Snapshotted device: %s, cow device %s, persistent flag: %s,"
	    "chunk size: %s\n", argv[0], argv[1], argv[2], argv[3]);
	
	if ((tsc = kmem_alloc(sizeof(struct target_snapshot_config), KM_NOSLEEP))
	    == NULL)
		return 1;

	tsc->persistent_dev = 0;
	
	tsc->chunk_size = atoi(argv[3]);

	if (strcmp(argv[2], "p") == 0)
		tsc->persistent_dev = 1;
	
	*target_config = tsc;

	
	dmv->dev_type = DM_SNAPSHOT_DEV;
	
	return 0;
}

/*
 * Status routine is called to get params string, which is target
 * specific. When dm_table_status_ioctl is called with flag
 * DM_STATUS_TABLE_FLAG I have to sent params string back.
 */
char *
dm_target_snapshot_status(void *target_config)
{
	struct target_snapshot_config *tlc;
	
	tlc = target_config;
	
	printf("Snapshot target status function called\n");

	return 0;
}

/* Strategy routine called from dm_strategy. */
int
dm_target_snapshot_strategy(struct dm_table_entry *table_en, struct buf *bp)
{

	printf("Snapshot target read function called!!\n");

	bp->b_error = EIO;
	bp->b_resid = 0;

	biodone(bp);
	
	return 0;
}

/* Doesn't do anything here. */
int
dm_target_snapshot_destroy(struct dm_table_entry *table_en)
{
	table_en->target_config = NULL;

	return 0;
}

/* Unsupported for this target. */
int
dm_target_snapshot_upcall(struct dm_table_entry *table_en, struct buf *bp)
{
	return 0;
}
/*
 * dm target snapshot origin routines.
 *
 * Keep for compatibility with linux lvm2tools. They use two targets
 * to implement snapshots. Snapshot target will implement exception
 * store and snapshot origin will implement device which calls every
 * snapshot device when write is done on master device.
 */

/* Init function called from dm_table_load_ioctl. */
int
dm_target_snapshot_orig_init(struct dm_dev *dmv, void **target_config, char *argv)
{

	printf("Snapshot_Orig target init function called!!\n");

	*target_config = NULL;

	
	dmv->dev_type = DM_SNAPSHOT_ORIG_DEV;
	
	return 0;
}

/*
 * Status routine is called to get params string, which is target
 * specific. When dm_table_status_ioctl is called with flag
 * DM_STATUS_TABLE_FLAG I have to sent params string back.
 */
char *
dm_target_snapshot_orig_status(void *target_config)
{
	struct target_snapshot_orig_config *tlc;
	
	tlc = target_config;
	
	printf("Snapshot_Orig target status function called\n");

	return 0;
}

/* Strategy routine called from dm_strategy. */
int
dm_target_snapshot_orig_strategy(struct dm_table_entry *table_en, struct buf *bp)
{

	printf("Snapshot_Orig target read function called!!\n");

	bp->b_error = EIO;
	bp->b_resid = 0;

	biodone(bp);
	
	return 0;
}

/* Doesn't do anything here. */
int
dm_target_snapshot_orig_destroy(struct dm_table_entry *table_en)
{
	table_en->target_config = NULL;

	return 0;
}

/* Unsupported for this target. */
int
dm_target_snapshot_orig_upcall(struct dm_table_entry *table_en, struct buf *bp)
{
	return 0;
}
