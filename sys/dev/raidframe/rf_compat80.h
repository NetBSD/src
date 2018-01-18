/*	$NetBSD: rf_compat80.h,v 1.1 2018/01/18 00:32:49 mrg Exp $	*/

/*
 * Copyright (c) 2017 Matthew R. Green
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _RF_COMPAT80_H_
#define _RF_COMPAT80_H_

#include <sys/ioccom.h>

/* NetBSD 8.99.x removed the row, raidPtr and next members */
struct rf_recon_req80 {
	RF_RowCol_t row, col;
	RF_ReconReqFlags_t flags;
	void   *raidPtr;	/* used internally; need not be set at ioctl
				 * time */
	struct rf_recon_req *next;	/* used internally; need not be set at
					 * ioctl time */
};

/* NetBSD 8.99.x made this structure alignment neutral */
typedef struct RF_RaidDisk_s80 {
        char    devname[56];    /* name of device file */
        RF_DiskStatus_t status; /* whether it is up or down */
        RF_RowCol_t spareRow;   /* if in status "spared", this identifies the
                                 * spare disk */
        RF_RowCol_t spareCol;   /* if in status "spared", this identifies the
                                 * spare disk */
        RF_SectorCount_t numBlocks;     /* number of blocks, obtained via READ
                                         * CAPACITY */
        int     blockSize;
        RF_SectorCount_t partitionSize; /* The *actual* and *full* size of
                                           the partition, from the disklabel */
        int     auto_configured;/* 1 if this component was autoconfigured.
                                   0 otherwise. */
        dev_t   dev;
} RF_RaidDisk_t80;

typedef struct RF_DeviceConfig_s80 {
	u_int   rows;
	u_int   cols;
	u_int   maxqdepth;
	int     ndevs;
	RF_RaidDisk_t80 devs[RF_MAX_DISKS];
	int     nspares;
	RF_RaidDisk_t80 spares[RF_MAX_DISKS];
} RF_DeviceConfig_t80;

/*
 * These ioctls were versioned after NetBSD 8.x.
 *
 * RAIDFRAME_FAIL_DISK had unused pointers removed from the structure.
 * RAIDFRAME_CHECK_*EXT and RAIDFRAME_GET_COMPONENT_LABEL were converted
 * from pointers to full structures so that ioctl framework does the copyout.
 * RAIDFRAME_GET_INFO80 had the containing structure reorganised so that
 * the layout became bitsize neutral.
 *
 * These 6 changes enable COMPAT_NETBSD32 support to work sanely with
 * these ioctls, leaving only RAIDFRAME_CONFIGURE needing extra help.
 */

#define RAIDFRAME_FAIL_DISK80				_IOW ('r', 5,  struct rf_recon_req80)
#define RAIDFRAME_GET_COMPONENT_LABEL80			_IOWR('r', 19, RF_ComponentLabel_t *)
#define RAIDFRAME_CHECK_RECON_STATUS_EXT80		_IOWR('r', 32, RF_ProgressInfo_t *)
#define RAIDFRAME_CHECK_PARITYREWRITE_STATUS_EXT80	_IOWR('r', 33, RF_ProgressInfo_t *)
#define RAIDFRAME_CHECK_COPYBACK_STATUS_EXT80		_IOWR('r', 34, RF_ProgressInfo_t *)
#define RAIDFRAME_GET_INFO80				_IOWR('r', 36, RF_DeviceConfig_t80 *)

int rf_check_recon_status_ext80(RF_Raid_t *, void *);
int rf_check_parityrewrite_status_ext80(RF_Raid_t *, void *);
int rf_check_copyback_status_ext80(RF_Raid_t *, void *);
int rf_get_info80(RF_Raid_t *, void *);
int rf_get_component_label80(RF_Raid_t *, void *);

#endif /* _RF_COMPAT80_H_ */
