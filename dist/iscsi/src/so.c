
/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By downloading, copying, installing or
 * using the software you agree to this license. If you do not agree to this license, do not download, install,
 * copy or use the software. 
 *
 * Intel License Agreement 
 *
 * Copyright (c) 2002, Intel Corporation
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met: 
 *
 * -Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *  following disclaimer. 
 *
 * -Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 *  following disclaimer in the documentation and/or other materials provided with the distribution. 
 *
 * -The name of Intel Corporation may not be used to endorse or promote products derived from this software
 *  without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE. 
 */

/*
 * Linux SCSI upper layer driver for OSD
 */


#include <linux/config.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/hdreg.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/smp.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/io.h>
#include <linux/blk.h>
#include <linux/blkpg.h>
#include "scsi.h"
#include "hosts.h"
#include "so.h"
#include <scsi/scsi_ioctl.h>
#include "constants.h"
#include <scsi/scsicam.h>	
#include <linux/genhd.h>
#include "util.h"
#include "osd.h"

/*
 * Macros
 */

#define SCSI_OSD_MAJOR          232                 /* major.h */
#define MAJOR_NR                SCSI_OSD_MAJOR      /* hosts.h */
#define DEVICE_NAME             "scsiosd"           /* blk.h   */
#define TIMEOUT_VALUE           (2*HZ)              /* blk.h   */
#define DEVICE_NR(device)       MINOR(device)       /* blk.h   */
#define SCSI_OSDS_PER_MAJOR     256
#define MAX_RETRIES             5
#define SO_TIMEOUT              (30 * HZ)
#define SO_MOD_TIMEOUT          (75 * HZ)

/*
 * Globals
 */

struct hd_struct *so;
static Scsi_Osd *rscsi_osds;
static int *so_sizes;
static int *so_blocksizes;
static int *so_hardsizes;
static int check_scsiosd_media_change(kdev_t);
static int so_init_oneosd(int);

/*
 * Function prototypes
 */

static int so_init(void);
static void so_finish(void);
static int so_attach(Scsi_Device *);
static int so_detect(Scsi_Device *);
static void so_detach(Scsi_Device *);
static int so_init_command(Scsi_Cmnd *);
static void rw_intr(Scsi_Cmnd * SCpnt);
static int fop_revalidate_scsiosd(kdev_t);
static int revalidate_scsiosd(kdev_t dev, int maxusage);


/*
 * Templates
 */

static struct Scsi_Device_Template so_template = {
	name:		"osd",
	tag:		"so",
	scsi_type:	TYPE_OSD,
	major:		SCSI_OSD_MAJOR,
	blk:		1,
	detect:		so_detect,
	init:		so_init,
	finish:		so_finish,
	attach:		so_attach,
	detach:		so_detach,
	init_command:	so_init_command,
};

/*
 * Functions
 */

static int so_ioctl(struct inode * inode, struct file * file, unsigned int cmd, unsigned long arg)
{
	kdev_t dev = inode->i_rdev;
	struct Scsi_Host * host;
	Scsi_Device * SDev;
	int osdinfo[4];

	iscsi_trace(TRACE_OSDSO, "so_ioctl()\n");
    
	SDev = rscsi_osds[DEVICE_NR(dev)].device;
	/*
	 * If we are in the middle of error recovery, don't let anyone
	 * else try and use this device.  Also, if error recovery fails, it
	 * may try and take the device offline, in which case all further
	 * access to the device is prohibited.
	 */

	if( !scsi_block_when_processing_errors(SDev) )
	{
		return -ENODEV;
	}

	switch (cmd) 
	{
		case HDIO_GETGEO:   /* Return BIOS osd parameters */
		{
			struct hd_geometry *loc = (struct hd_geometry *) arg;
			if(!loc)
				return -EINVAL;

			host = rscsi_osds[DEVICE_NR(dev)].device->host;
	
			/* default to most commonly used values */
	
		        osdinfo[0] = 0x40;
	        	osdinfo[1] = 0x20;
	        	osdinfo[2] = rscsi_osds[DEVICE_NR(dev)].capacity >> 11;
	
			/* override with calculated, extended default, or driver values */
	
#if 0
			if(host->hostt->bios_param != NULL)
				host->hostt->bios_param(&rscsi_osds[DEVICE_NR(dev)], dev, &osdinfo[0]);
			else scsicam_bios_param(&rscsi_osds[DEVICE_NR(dev)], dev, &osdinfo[0]);

			if (put_user(osdinfo[0], &loc->heads) ||
				put_user(osdinfo[1], &loc->sectors) ||
				put_user(osdinfo[2], &loc->cylinders) ||
				put_user(so[MINOR(inode->i_rdev)].start_sect, &loc->start))
				return -EFAULT;
			return 0;
#else
			return -1;
#endif
		}
		case HDIO_GETGEO_BIG:
		{
			struct hd_big_geometry *loc = (struct hd_big_geometry *) arg;

			if(!loc)
				return -EINVAL;

			host = rscsi_osds[DEVICE_NR(dev)].device->host;

			/* default to most commonly used values */

			osdinfo[0] = 0x40;
			osdinfo[1] = 0x20;
			osdinfo[2] = rscsi_osds[DEVICE_NR(dev)].capacity >> 11;

			/* override with calculated, extended default, or driver values */

#if 0
			if(host->hostt->bios_param != NULL)
				host->hostt->bios_param(&rscsi_osds[DEVICE_NR(dev)], dev, &osdinfo[0]);
			else scsicam_bios_param(&rscsi_osds[DEVICE_NR(dev)], dev, &osdinfo[0]);

			if (put_user(osdinfo[0], &loc->heads) ||
				put_user(osdinfo[1], &loc->sectors) ||
				put_user(osdinfo[2], (unsigned int *) &loc->cylinders) ||
				put_user(so[MINOR(inode->i_rdev)].start_sect, &loc->start))
				return -EFAULT;
			return 0;
#else
			return -1;
#endif
		}
		case BLKGETSIZE:   /* Return device size */
			if (!arg)
				return -EINVAL;
			return put_user(so[MINOR(inode->i_rdev)].nr_sects, (long *) arg);

		case BLKROSET:
		case BLKROGET:
		case BLKRASET:
		case BLKRAGET:
		case BLKFLSBUF:
		case BLKSSZGET:
		case BLKPG:
                case BLKELVGET:
                case BLKELVSET:
			return blk_ioctl(inode->i_rdev, cmd, arg);

		case BLKRRPART: /* Re-read partition tables */
		        if (!capable(CAP_SYS_ADMIN))
		                return -EACCES;
			return revalidate_scsiosd(dev, 1);

		default:
			return scsi_ioctl(rscsi_osds[DEVICE_NR(dev)].device , cmd, (void *) arg);
	}
}

static void so_devname(unsigned int index, char *buffer) {
  iscsi_trace(TRACE_OSDSO, "so_devname(%i)\n", index);
  sprintf(buffer, "so%i", index);
}

static request_queue_t *so_find_queue(kdev_t dev)
{
	Scsi_Osd *dpnt;
 	int target;

	iscsi_trace(TRACE_OSDSO, "so_find_queue()\n");

 	target = DEVICE_NR(dev);

	dpnt = &rscsi_osds[target];
	if (!dpnt) {
		iscsi_trace_error("no such device\n");
		return NULL;
	}
	return &dpnt->device->request_queue;
}

static int so_init_command(Scsi_Cmnd * SCpnt)
{
	int block, this_count;
	Scsi_Osd *dpnt;
	char nbuff[6];
	osd_args_t args;
	int index;

	iscsi_trace(TRACE_OSDSO, "so_init_command(MAJOR %i, MINOR %i)\n", 
          MAJOR(SCpnt->request.rq_dev), MINOR(SCpnt->request.rq_dev));
        index = MINOR(SCpnt->request.rq_dev);
        so_devname(index, nbuff);
        block = SCpnt->request.sector;
        this_count = SCpnt->request_bufflen >> 9;
        dpnt = &rscsi_osds[index];

        if (index >= so_template.dev_max || !dpnt || !dpnt->device->online ||
            block + SCpnt->request.nr_sectors > so[index].nr_sects) {
          iscsi_trace_error("index %i: request out of range: %i offset + %li count > %li total sectors\n",
                      index, block, SCpnt->request.nr_sectors, so[index].nr_sects);
          return 0;
        }

        block += so[index].start_sect;
        if (dpnt->device->changed) {
          iscsi_trace_error("SCSI osd has been changed. Prohibiting further I/O\n");
          return 0;
        }

	switch (SCpnt->request.cmd) {
	case WRITE:

                iscsi_trace(TRACE_OSDSO, "Translating BLOCK WRITE to OBJECT WRITE\n");
                if (!dpnt->device->writeable) {
                        iscsi_trace_error("device is not writable\n");
                        return 0;
                }
                iscsi_trace(TRACE_OSDSO, "Translating BLOCK WRITE (sector %i, len %i) to OBJECT WRITE\n", block, this_count);
                memset(&args, 0, sizeof(osd_args_t));
                args.opcode = 0x7f;
                args.add_cdb_len = CONFIG_OSD_CDB_LEN-7;
                args.service_action = OSD_WRITE;
                args.GroupID = 0; args.UserID = 0;
                args.length = 512*this_count;
                args.offset = 512*block;
                OSD_ENCAP_CDB(&args, SCpnt->cmnd);
                SCpnt->sc_data_direction = SCSI_DATA_WRITE;
                SCpnt->cmd_len = CONFIG_OSD_CDB_LEN;
		SCpnt->result = 0;
                break;

	case READ:

                iscsi_trace(TRACE_OSDSO, "Translating BLOCK READ (sector %i, len %i) to OBJECT READ\n", block, this_count);
                memset(&args, 0, sizeof(osd_args_t));
                args.opcode = 0x7f;
                args.add_cdb_len = CONFIG_OSD_CDB_LEN-7;
                args.service_action = OSD_READ;
                args.GroupID = 0; args.UserID = 0;
                args.length = 512*this_count;
                args.offset = 512*block;
                OSD_ENCAP_CDB(&args, SCpnt->cmnd);
                SCpnt->sc_data_direction = SCSI_DATA_READ;
                SCpnt->cmd_len = CONFIG_OSD_CDB_LEN;
		SCpnt->result = 0;
                break;

	default:
		panic("Unknown so command %d\n", SCpnt->request.cmd);
	}

        /*
	 * We shouldn't disconnect in the middle of a sector, so with a dumb
	 * host adapter, it's safe to assume that we can at least transfer
	 * this many bytes between each connect / disconnect.
	 */

	SCpnt->transfersize = dpnt->device->sector_size;
	SCpnt->underflow = this_count << 9;
	SCpnt->allowed = MAX_RETRIES;
	SCpnt->timeout_per_command = (SCpnt->device->type == TYPE_OSD ?
				      SO_TIMEOUT : SO_MOD_TIMEOUT);

	/*
	 * This is the completion routine we use.  This is matched in terms
	 * of capability to this function.
	 */

	SCpnt->done = rw_intr;

	/*
	 * This indicates that the command is ready from our end to be
	 * queued.
	 */

	return 1;
}

static int so_open(struct inode *inode, struct file *filp)
{
	int target;
	Scsi_Device * SDev;
	target = DEVICE_NR(inode->i_rdev);

	iscsi_trace(TRACE_OSDSO, "so_open()\n");

	SCSI_LOG_HLQUEUE(1, printk("target=%d, max=%d\n", target, so_template.dev_max));

	if (target >= so_template.dev_max || !rscsi_osds[target].device)
		return -ENXIO;	/* No such device */

	/*
	 * If the device is in error recovery, wait until it is done.
	 * If the device is offline, then disallow any access to it.
	 */
	if (!scsi_block_when_processing_errors(rscsi_osds[target].device)) {
		return -ENXIO;
	}
	/*
	 * Make sure that only one process can do a check_change_osd at one time.
	 * This is also used to lock out further access when the partition table
	 * is being re-read.
	 */

	while (rscsi_osds[target].device->busy)
		barrier();
	if (rscsi_osds[target].device->removable) {
		check_disk_change(inode->i_rdev);

		/*
		 * If the drive is empty, just let the open fail.
		 */
		if (!rscsi_osds[target].ready)
			return -ENXIO;

		/*
		 * Similarly, if the device has the write protect tab set,
		 * have the open fail if the user expects to be able to write
		 * to the thing.
		 */
		if ((rscsi_osds[target].write_prot) && (filp->f_mode & 2))
			return -EROFS;
	}
	SDev = rscsi_osds[target].device;
	/*
	 * It is possible that the osd changing stuff resulted in the device
	 * being taken offline.  If this is the case, report this to the user,
	 * and don't pretend that
	 * the open actually succeeded.
	 */
	if (!SDev->online) {
		return -ENXIO;
	}
	/*
	 * See if we are requesting a non-existent partition.  Do this
	 * after checking for osd change.
	 */
	if (so_sizes[MINOR(inode->i_rdev)] == 0)
		return -ENXIO;

	if (SDev->removable)
		if (!SDev->access_count)
			if (scsi_block_when_processing_errors(SDev))
				scsi_ioctl(SDev, SCSI_IOCTL_DOORLOCK, NULL);

	SDev->access_count++;
	if (SDev->host->hostt->module)
		__MOD_INC_USE_COUNT(SDev->host->hostt->module);
	if (so_template.module)
		__MOD_INC_USE_COUNT(so_template.module);
	return 0;
}

static int so_release(struct inode *inode, struct file *file)
{
	int target;
	Scsi_Device * SDev;

	iscsi_trace(TRACE_OSDSO, "so_release()\n");

	target = DEVICE_NR(inode->i_rdev);
	SDev = rscsi_osds[target].device;

	SDev->access_count--;

	if (SDev->removable) {
		if (!SDev->access_count)
			if (scsi_block_when_processing_errors(SDev))
				scsi_ioctl(SDev, SCSI_IOCTL_DOORUNLOCK, NULL);
	}
	if (SDev->host->hostt->module)
		__MOD_DEC_USE_COUNT(SDev->host->hostt->module);
	if (so_template.module)
		__MOD_DEC_USE_COUNT(so_template.module);
	return 0;
}

static struct block_device_operations so_fops =
{
	open:			so_open,
	release:		so_release,
	ioctl:			so_ioctl,
	check_media_change:	check_scsiosd_media_change,
	revalidate:		fop_revalidate_scsiosd
};

/*
 *  If we need more than one SCSI osd major (i.e. more than
 *  16 SCSI osds), we'll have to kmalloc() more gendisks later.
 */

static struct gendisk so_gendisk =
{
	SCSI_OSD_MAJOR,    	/* Major number */
	"so",			/* Major name */
	0,			/* Bits to shift to get real from partition */
	1,			/* Number of partitions per real */
	NULL,			/* hd struct */
	NULL,			/* block sizes */
	0,			/* number */
	NULL,			/* internal */
	NULL,			/* next */
        &so_fops,		/* file operations */
};

/*
 * rw_intr is the interrupt routine for the device driver.
 * It will be notified on the end of a SCSI read / write, and
 * will take one of several actions based on success or failure.
 */

static void rw_intr(Scsi_Cmnd * SCpnt)
{
	int result = SCpnt->result;
	char nbuff[6];
	int this_count = SCpnt->bufflen >> 9;
	int good_sectors = (result == 0 ? this_count : 0);
	int block_sectors = 1;

	so_devname(DEVICE_NR(SCpnt->request.rq_dev), nbuff);
        iscsi_trace(TRACE_OSDSO, "rw_intr(/dev/%s, host %d, result 0x%x)\n", nbuff, SCpnt->host->host_no, result);

	/*
	   Handle MEDIUM ERRORs that indicate partial success.  Since this is a
	   relatively rare error condition, no care is taken to avoid
	   unnecessary additional work such as memcpy's that could be avoided.
	 */

	/* An error occurred */
	if (driver_byte(result) != 0) {
		/* Sense data is valid */
		if (SCpnt->sense_buffer[0] == 0xF0 && SCpnt->sense_buffer[2] == MEDIUM_ERROR) {
			long error_sector = (SCpnt->sense_buffer[3] << 24) |
			(SCpnt->sense_buffer[4] << 16) |
			(SCpnt->sense_buffer[5] << 8) |
			SCpnt->sense_buffer[6];
			if (SCpnt->request.bh != NULL)
				block_sectors = SCpnt->request.bh->b_size >> 9;
			switch (SCpnt->device->sector_size) {
			case 1024:
				error_sector <<= 1;
				if (block_sectors < 2)
					block_sectors = 2;
				break;
			case 2048:
				error_sector <<= 2;
				if (block_sectors < 4)
					block_sectors = 4;
				break;
			case 4096:
				error_sector <<=3;
				if (block_sectors < 8)
					block_sectors = 8;
				break;
			case 256:
				error_sector >>= 1;
				break;
			default:
				break;
			}
			error_sector -= so[MINOR(SCpnt->request.rq_dev)].start_sect;
			error_sector &= ~(block_sectors - 1);
			good_sectors = error_sector - SCpnt->request.sector;
			if (good_sectors < 0 || good_sectors >= this_count)
				good_sectors = 0;
		}
		if (SCpnt->sense_buffer[2] == ILLEGAL_REQUEST) {
			if (SCpnt->device->ten == 1) {
				if (SCpnt->cmnd[0] == READ_10 ||
				    SCpnt->cmnd[0] == WRITE_10)
					SCpnt->device->ten = 0;
			}
		}
	}
	/*
	 * This calls the generic completion function, now that we know
	 * how many actual sectors finished, and how many sectors we need
	 * to say have failed.
	 */
	scsi_io_completion(SCpnt, good_sectors, block_sectors);
}
/*
 * requeue_so_request() is the request handler function for the so driver.
 * Its function in life is to take block device requests, and translate
 * them to SCSI commands.
 */


static int check_scsiosd_media_change(kdev_t full_dev)
{
	int retval;
	int target;
	int flag = 0;
	Scsi_Device * SDev;

	iscsi_trace(TRACE_OSDSO, "check_scsiosd_media_change()\n");

	target = DEVICE_NR(full_dev);
	SDev = rscsi_osds[target].device;

	if (target >= so_template.dev_max || !SDev) {
		printk("SCSI osd request error: invalid device.\n");
		return 0;
	}
	if (!SDev->removable)
		return 0;

	/*
	 * If the device is offline, don't send any commands - just pretend as
	 * if the command failed.  If the device ever comes back online, we
	 * can deal with it then.  It is only because of unrecoverable errors
	 * that we would ever take a device offline in the first place.
	 */
	if (SDev->online == FALSE) {
		rscsi_osds[target].ready = 0;
		SDev->changed = 1;
		return 1;	/* This will force a flush, if called from
				 * check_disk_change */
	}

	/* Using Start/Stop enables differentiation between drive with
	 * no cartridge loaded - NOT READY, drive with changed cartridge -
	 * UNIT ATTENTION, or with same cartridge - GOOD STATUS.
	 * This also handles drives that auto spin down. eg iomega jaz 1GB
	 * as this will spin up the drive.
	 */
	retval = -ENODEV;
	if (scsi_block_when_processing_errors(SDev))
		retval = scsi_ioctl(SDev, SCSI_IOCTL_START_UNIT, NULL);

	if (retval) {		/* Unable to test, unit probably not ready.
				 * This usually means there is no disc in the
				 * drive.  Mark as changed, and we will figure
				 * it out later once the drive is available
				 * again.  */

		rscsi_osds[target].ready = 0;
		SDev->changed = 1;
		return 1;	/* This will force a flush, if called from
				 * check_disk_change */
	}
	/*
	 * for removable scsi osd ( FLOPTICAL ) we have to recognise the
	 * presence of osd in the drive. This is kept in the Scsi_Osd
	 * struct and tested at open !  Daniel Roche ( dan@lectra.fr )
	 */

	rscsi_osds[target].ready = 1;	/* FLOPTICAL */

	retval = SDev->changed;
	if (!flag)
		SDev->changed = 0;
	return retval;
}

static int so_init_oneosd(int i) {
	unsigned char cmd[10];
	char nbuff[6];
	Scsi_Request *SRpnt;

	iscsi_trace(TRACE_OSDSO, "so_init_oneosd(%i)\n", i);

	so_devname(i, nbuff);
	if (rscsi_osds[i].device->online == FALSE) {
		iscsi_trace_error("device is offline??\n");
		return i;
	}

        /* 
         * TEST_UNIT_READY
         */

	SRpnt = scsi_allocate_request(rscsi_osds[i].device);
	cmd[0] = TEST_UNIT_READY;
	cmd[1] = (rscsi_osds[i].device->lun << 5) & 0xe0;
	memset((void *) &cmd[2], 0, 8);
	SRpnt->sr_cmd_len = 0;
	SRpnt->sr_sense_buffer[0] = 0;
	SRpnt->sr_sense_buffer[2] = 0;
	SRpnt->sr_data_direction = SCSI_DATA_READ;
	scsi_wait_req (SRpnt, (void *) cmd, NULL, 0, SO_TIMEOUT, MAX_RETRIES);
        if (SRpnt->sr_result!=0) {
		iscsi_trace_error("OSD not ready\n");
		return i;
        }

	/* Initialize device */

	rscsi_osds[i].capacity = 1048576*512;
	rscsi_osds[i].device->changed = 0;
	rscsi_osds[i].ready = 0;
	rscsi_osds[i].write_prot = 0;
	rscsi_osds[i].device->removable = 0;
	SRpnt->sr_device->ten = 1;
	SRpnt->sr_device->remap = 1;
	SRpnt->sr_device->sector_size = 512;

	/* printk("%s : block size assumed to be 512 bytes, osd size 1GB.  \n", nbuff); */

	/* Wake up a process waiting for device */

	scsi_release_request(SRpnt);  

	/* Cleanup */

	SRpnt = NULL;
	return i;
}

/*
 * The so_init() function looks at all SCSI drives present, determines
 * their size, and reads partition table entries for them.
 */

static int so_registered;

static int so_init() {
        int i;

	iscsi_trace(TRACE_OSDSO, "so_init()\n");

        if (so_template.dev_noticed == 0) {
          iscsi_trace_error("no OSDs noticed\n");
          return 0;
        }
        if (!rscsi_osds) {
          printf("%i osds detected \n", so_template.dev_noticed);
          so_template.dev_max = so_template.dev_noticed;
        }
        if (so_template.dev_max > SCSI_OSDS_PER_MAJOR) {
          iscsi_trace_error("so_template.dev_max (%i) > SCSI_OSDS_PER_MAJOR\n", so_template.dev_max);
          so_template.dev_max = SCSI_OSDS_PER_MAJOR;
        }
        if (!so_registered) {
          if (devfs_register_blkdev(SCSI_OSD_MAJOR, "so", &so_fops)) {
            printk("Unable to get major %d for SCSI osd\n", SCSI_OSD_MAJOR);
            return 1;
          }
          so_registered++;
        }

	/* No loadable devices yet */

        if (rscsi_osds) return 0;

        /* Real devices */

        if ((rscsi_osds = kmalloc(so_template.dev_max * sizeof(Scsi_Osd), GFP_ATOMIC))==NULL) {
          iscsi_trace_error("kmalloc() failed\n");
          goto cleanup_devfs;
        }
        memset(rscsi_osds, 0, so_template.dev_max * sizeof(Scsi_Osd));
        so_gendisk.real_devices = (void *) rscsi_osds;

        /* Partition sizes */

        if ((so_sizes=kmalloc(so_template.dev_max*sizeof(int), GFP_ATOMIC))==NULL) {
          iscsi_trace_error("kmalloc() failed\n");
          goto cleanup_rscsi_osds;
        }
        memset(so_sizes, 0, so_template.dev_max*sizeof(int));
        so_gendisk.sizes = so_sizes;

        /* Block sizes */

        if ((so_blocksizes=kmalloc(so_template.dev_max*sizeof(int), GFP_ATOMIC))==NULL) {
          iscsi_trace_error("kmalloc() failed\n");
          goto cleanup_so_sizes;
        }
        blksize_size[SCSI_OSD_MAJOR] = so_blocksizes;

        /* Sector sizes */

        if ((so_hardsizes=kmalloc(so_template.dev_max*sizeof(int), GFP_ATOMIC))==NULL) {
          iscsi_trace_error("kmalloc() failed\n");
          goto cleanup_so_blocksizes;
        }
        hardsect_size[SCSI_OSD_MAJOR] = so_hardsizes;

        /* Partitions */

        if ((so=kmalloc(so_template.dev_max*sizeof(struct hd_struct), GFP_ATOMIC))==NULL) {
          iscsi_trace_error("kmalloc() failed\n");
          goto cleanup_so_hardsizes;
        }
        memset(so, 0, so_template.dev_max*sizeof(struct hd_struct));
        so_gendisk.part = so;

        /* Initialize Things */

        for (i=0; i<so_template.dev_max; i++) {
          so_blocksizes[i] = 1024;  /*  minimum request/block size */
          so_sizes[i]      = 1;     /*  number of blocks */
          so_hardsizes[i]  = 512;   /*  sector size */
          so[i].nr_sects   = 2;     /*  number of sectors */
        }

        /* ??? */

        if ((so_gendisk.de_arr=kmalloc(SCSI_OSDS_PER_MAJOR*sizeof(*so_gendisk.de_arr), GFP_ATOMIC))==NULL) {
          iscsi_trace_error("kmalloc() failed\n");
          goto cleanup_so;
        }
        memset(so_gendisk.de_arr, 0, SCSI_OSDS_PER_MAJOR * sizeof *so_gendisk.de_arr);

        /* Flags */

        if ((so_gendisk.flags=kmalloc(SCSI_OSDS_PER_MAJOR*sizeof(*so_gendisk.flags),GFP_ATOMIC))==NULL) {
          iscsi_trace_error("kmalloc() failed\n");
          goto cleanup_so_gendisk_de_arr;
        }
        memset(so_gendisk.flags, 0, SCSI_OSDS_PER_MAJOR * sizeof *so_gendisk.flags);

        if (so_gendisk.next) {
          iscsi_trace_error("Why is this not NULL?\n");
	  so_gendisk.next = NULL;
        }

	return 0;

cleanup_so_gendisk_de_arr:
        kfree(so_gendisk.de_arr);
cleanup_so:
        kfree(so);
cleanup_so_hardsizes:
        kfree(so_hardsizes);
cleanup_so_blocksizes:
        kfree(so_blocksizes);
cleanup_so_sizes:
        kfree(so_sizes);
cleanup_rscsi_osds:
        kfree(rscsi_osds);
cleanup_devfs:
        devfs_unregister_blkdev(SCSI_OSD_MAJOR, "so");
        so_registered--;
        return 1;
}


static void so_finish()
{
	struct gendisk *gendisk;
	int i;

	iscsi_trace(TRACE_OSDSO, "so_finish()\n");

	blk_dev[SCSI_OSD_MAJOR].queue = so_find_queue;
	for (gendisk = gendisk_head; gendisk != NULL; gendisk = gendisk->next)
		if (gendisk == &so_gendisk)
			break;
	if (gendisk == NULL) {
		so_gendisk.next = gendisk_head;
		gendisk_head = &so_gendisk;
	}

	for (i = 0; i < so_template.dev_max; ++i)
		if (!rscsi_osds[i].capacity && rscsi_osds[i].device) {
			so_init_oneosd(i);
			if (!rscsi_osds[i].has_part_table) {
				so_sizes[i] = rscsi_osds[i].capacity;
				register_disk(&so_gendisk, MKDEV(MAJOR(i),MINOR(i)), 1, &so_fops, rscsi_osds[i].capacity);
				rscsi_osds[i].has_part_table = 1;
			}
		}

        /* No read-ahead right not */

        read_ahead[SCSI_OSD_MAJOR] = 0;

	return;
}

static int so_detect(Scsi_Device * SDp)
{
	char nbuff[6];
	iscsi_trace(TRACE_OSDSO, "so_detect()\n");

	if (SDp->type != TYPE_OSD && SDp->type != TYPE_MOD)
		return 0;

	so_devname(so_template.dev_noticed++, nbuff);
	printk("Detected scsi %sosd %s at scsi%d, channel %d, id %d, lun %d\n",
	       SDp->removable ? "removable " : "",
	       nbuff,
	       SDp->host->host_no, SDp->channel, SDp->id, SDp->lun);

	return 1;
}

static int so_attach(Scsi_Device * SDp)
{
        unsigned int devnum;
	Scsi_Osd *dpnt;
	int i;

	iscsi_trace(TRACE_OSDSO, "so_attach(SDpnt 0x%p)\n", SDp);
	if (SDp->type != TYPE_OSD && SDp->type != TYPE_MOD)
		return 0;

	if (so_template.nr_dev >= so_template.dev_max) {
		SDp->attached--;
		return 1;
	}
	for (dpnt = rscsi_osds, i = 0; i < so_template.dev_max; i++, dpnt++)
		if (!dpnt->device)
			break;

	if (i >= so_template.dev_max)
		panic("scsi_devices corrupt (so)");

	rscsi_osds[i].device = SDp;
	rscsi_osds[i].has_part_table = 0;
	so_template.nr_dev++;
	so_gendisk.nr_real++;
        devnum = i % SCSI_OSDS_PER_MAJOR;
        so_gendisk.de_arr[devnum] = SDp->de;
        if (SDp->removable)
		so_gendisk.flags[devnum] |= GENHD_FL_REMOVABLE;
	return 0;
}

#define DEVICE_BUSY rscsi_osds[target].device->busy
#define USAGE rscsi_osds[target].device->access_count
#define CAPACITY rscsi_osds[target].capacity
#define MAYBE_REINIT  so_init_oneosd(target)

/* This routine is called to flush all partitions and partition tables
 * for a changed scsi osd, and then re-read the new partition table.
 * If we are revalidating a osd because of a media change, then we
 * enter with usage == 0.  If we are using an ioctl, we automatically have
 * usage == 1 (we need an open channel to use an ioctl :-), so this
 * is our limit.
 */

int revalidate_scsiosd(kdev_t dev, int maxusage)
{
	int target;
	int max_p;
	int start;
	int i;

	iscsi_trace(TRACE_OSDSO, "revalidate_scsiosd()\n");

	target = DEVICE_NR(dev);

	if (DEVICE_BUSY || USAGE > maxusage) {
		printk("Device busy for revalidation (usage=%d)\n", USAGE);
		return -EBUSY;
	}
	DEVICE_BUSY = 1;

	max_p = so_gendisk.max_p;
	start = target << so_gendisk.minor_shift;

	for (i = max_p - 1; i >= 0; i--) {
		int index = start + i;
		/* invalidate_device(MKDEV(MAJOR(index),MINOR(index)), 1); */
		so_gendisk.part[index].start_sect = 0;
		so_gendisk.part[index].nr_sects = 0;
		/*
		 * Reset the blocksize for everything so that we can read
		 * the partition table.  Technically we will determine the
		 * correct block size when we revalidate, but we do this just
		 * to make sure that everything remains consistent.
		 */
		so_blocksizes[index] = 1024;
		if (rscsi_osds[target].device->sector_size == 2048)
			so_blocksizes[index] = 2048;
		else
			so_blocksizes[index] = 1024;
	}

#ifdef MAYBE_REINIT
	MAYBE_REINIT;
#endif

	grok_partitions(&so_gendisk, target % SCSI_OSDS_PER_MAJOR, 1, CAPACITY);

	DEVICE_BUSY = 0;
	return 0;
}

static int fop_revalidate_scsiosd(kdev_t dev)
{
	iscsi_trace(TRACE_OSDSO, "fop_revalidate_scsiosd()\n");
	return revalidate_scsiosd(dev, 0);
}
static void so_detach(Scsi_Device * SDp)
{
	Scsi_Osd *dpnt;
	int i, j;
	int max_p;
	int start;

	iscsi_trace(TRACE_OSDSO, "so_detach()\n");

	for (dpnt = rscsi_osds, i = 0; i < so_template.dev_max; i++, dpnt++)
		if (dpnt->device == SDp) {

			/* If we are disconnecting a osd driver, sync and invalidate
			 * everything */
			max_p = so_gendisk.max_p;
			start = i << so_gendisk.minor_shift;

			for (j = max_p - 1; j >= 0; j--) {
				int index = start + j;
				/* invalidate_device(MKDEV(MAJOR(index),MINOR(index)), 1); */
				so_gendisk.part[index].start_sect = 0;
				so_gendisk.part[index].nr_sects = 0;
				so_sizes[index] = 0;
			}
                        devfs_register_partitions (&so_gendisk, MINOR(start), 1);
			/* unregister_disk() */
			dpnt->has_part_table = 0;
			dpnt->device = NULL;
			dpnt->capacity = 0;
			SDp->attached--;
			so_template.dev_noticed--;
			so_template.nr_dev--;
			so_gendisk.nr_real--;
			return;
		}
	return;
}

static int __init init_so(void)
{
	iscsi_trace(TRACE_OSDSO, "init_so()\n");
	so_template.module = THIS_MODULE;
	return scsi_register_module(MODULE_SCSI_DEV, &so_template);
}

static void __exit exit_so(void)
{
	struct gendisk **prev_sogd_link;
	struct gendisk *sogd;
	int removed = 0;

	iscsi_trace(TRACE_OSDSO, "exit_so()\n");

	scsi_unregister_module(MODULE_SCSI_DEV, &so_template);

	devfs_unregister_blkdev(SCSI_OSD_MAJOR, "so");

	so_registered--;
	if (rscsi_osds != NULL) {
		kfree(rscsi_osds);
		kfree(so_sizes);
		kfree(so_blocksizes);
		kfree(so_hardsizes);
		kfree((char *) so);

		/*
		 * Now remove &so_gendisk from the linked list
		 */
		prev_sogd_link = &gendisk_head;
		while ((sogd = *prev_sogd_link) != NULL) {
			if (sogd >= &so_gendisk && sogd <= &so_gendisk) {
				removed++;
				*prev_sogd_link = sogd->next;
				continue;
			}
			prev_sogd_link = &sogd->next;
		}

		if (removed != 1)
			printk("%s %d &so_gendisk in osd chain", removed > 1 ? "total" : "just", removed);

	}
	blk_size[SCSI_OSD_MAJOR] = NULL;
	hardsect_size[SCSI_OSD_MAJOR] = NULL;
	read_ahead[SCSI_OSD_MAJOR] = 0;
	so_template.dev_max = 0;
}

module_init(init_so);
module_exit(exit_so);
