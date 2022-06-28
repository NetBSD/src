/*	$NetBSD: rf_compat80.c,v 1.17 2022/06/28 03:13:27 oster Exp $	*/

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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/module.h>

#include <sys/compat_stub.h>

#include <dev/raidframe/raidframeio.h>
#include <dev/raidframe/raidframevar.h>

#include "rf_raid.h"
#include "rf_compat80.h"
#include "rf_kintf.h"

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

typedef struct RF_Config_s80 {
	RF_RowCol_t numRow, numCol, numSpare;	/* number of rows, columns,
						 * and spare disks */
	dev_t   devs[RF_MAXROW][RF_MAXCOL];	/* device numbers for disks
						 * comprising array */
	char    devnames[RF_MAXROW][RF_MAXCOL][50];	/* device names */
	dev_t   spare_devs[RF_MAXSPARE];	/* device numbers for spare
						 * disks */
	char    spare_names[RF_MAXSPARE][50];	/* device names */
	RF_SectorNum_t sectPerSU;	/* sectors per stripe unit */
	RF_StripeNum_t SUsPerPU;/* stripe units per parity unit */
	RF_StripeNum_t SUsPerRU;/* stripe units per reconstruction unit */
	RF_ParityConfig_t parityConfig;	/* identifies the RAID architecture to
					 * be used */
	RF_DiskQueueType_t diskQueueType;	/* 'f' = fifo, 'c' = cvscan,
						 * not used in kernel */
	char    maxOutstandingDiskReqs;	/* # concurrent reqs to be sent to a
					 * disk.  not used in kernel. */
	char    debugVars[RF_MAXDBGV][50];	/* space for specifying debug
						 * variables & their values */
	unsigned int layoutSpecificSize;	/* size in bytes of
						 * layout-specific info */
	void   *layoutSpecific;	/* a pointer to a layout-specific structure to
				 * be copied in */
	int     force;                          /* if !0, ignore many fatal
						   configuration conditions */
	/*
	   "force" is used to override cases where the component labels would
	   indicate that configuration should not proceed without user
	   intervention
	 */
} RF_Config_t80;

static int
rf_check_recon_status_ext80(RF_Raid_t *raidPtr, void *data)
{
	RF_ProgressInfo_t info, **infoPtr = data;

	rf_check_recon_status_ext(raidPtr, &info);
	return copyout(&info, *infoPtr, sizeof(info));
}

static int
rf_check_parityrewrite_status_ext80(RF_Raid_t *raidPtr, void *data)
{
	RF_ProgressInfo_t info, **infoPtr = data;

	rf_check_parityrewrite_status_ext(raidPtr, &info);
	return copyout(&info, *infoPtr, sizeof(info));
}

static int
rf_check_copyback_status_ext80(RF_Raid_t *raidPtr, void *data)
{
	RF_ProgressInfo_t info, **infoPtr = data;

	rf_check_copyback_status_ext(raidPtr, &info);
	return copyout(&info, *infoPtr, sizeof(info));
}

static void
rf_copy_raiddisk80(RF_RaidDisk_t *disk, RF_RaidDisk_t80 *disk80)
{

	/* Be sure the padding areas don't have kernel memory. */
	memset(disk80, 0, sizeof(*disk80));
	memcpy(disk80->devname, disk->devname, sizeof(disk80->devname));
	disk80->status = disk->status;
	disk80->spareRow = 0;
	disk80->spareCol = disk->spareCol;
	disk80->numBlocks = disk->numBlocks;
	disk80->blockSize = disk->blockSize;
	disk80->partitionSize = disk->partitionSize;
	disk80->auto_configured = disk->auto_configured;
	disk80->dev = disk->dev;
}

static int
rf_get_info80(RF_Raid_t *raidPtr, void *data)
{
	RF_DeviceConfig_t *config;
	RF_DeviceConfig_t80 *config80, **configPtr80 = data;
	int rv;

	config = RF_Malloc(sizeof(*config));
	if (config == NULL)
		return ENOMEM;
	config80 = RF_Malloc(sizeof(*config80));
	if (config80 == NULL) {
		RF_Free(config, sizeof(*config));
		return ENOMEM;
	}
	rv = rf_get_info(raidPtr, config);
	if (rv == 0) {
		/* convert new to old */
		config80->rows = 1;
		config80->cols = config->cols;
		config80->maxqdepth = config->maxqdepth;
		config80->ndevs = config->ndevs;
		config80->nspares = config->nspares;
		for (size_t i = 0; i < RF_MAX_DISKS; i++) {
			rf_copy_raiddisk80(&config->devs[i],
					   &config80->devs[i]);
			rf_copy_raiddisk80(&config->spares[i],
					   &config80->spares[i]);
		}
		rv = copyout(config80, *configPtr80, sizeof(*config80));
	}
	RF_Free(config, sizeof(*config));
	RF_Free(config80, sizeof(*config80));

	return rv;
}

static int
rf_get_component_label80(RF_Raid_t *raidPtr, void *data)
{
	RF_ComponentLabel_t **clabel_ptr = (RF_ComponentLabel_t **)data;
	RF_ComponentLabel_t *clabel;
	int retcode;

	/*
	 * Perhaps there should be an option to skip the in-core
	 * copy and hit the disk, as with disklabel(8).
	 */
	clabel = RF_Malloc(sizeof(*clabel));
	if (clabel == NULL)
		return ENOMEM;
	retcode = copyin(*clabel_ptr, clabel, sizeof(*clabel));
	if (retcode) {
		RF_Free(clabel, sizeof(*clabel));
		return retcode;
	}

	rf_get_component_label(raidPtr, clabel);
	/* Fix-up for userland. */
	if (clabel->version == bswap32(RF_COMPONENT_LABEL_VERSION))
		clabel->version = RF_COMPONENT_LABEL_VERSION;

	retcode = copyout(clabel, *clabel_ptr, sizeof(**clabel_ptr));
	RF_Free(clabel, sizeof(*clabel));

	return retcode;
}

static int
rf_config80(struct raid_softc *rs, void *data)
{
	RF_Config_t80 *u80_cfg, *k80_cfg;
	RF_Config_t *k_cfg;
	RF_Raid_t *raidPtr = rf_get_raid(rs);
	size_t i, j;
	int error;

	if (raidPtr->valid) {
		/* There is a valid RAID set running on this unit! */
		printf("raid%d: Device already configured!\n", rf_get_unit(rs));
		return EINVAL;
	}

	/* copy-in the configuration information */
	/* data points to a pointer to the configuration structure */

	u80_cfg = *((RF_Config_t80 **) data);
	k80_cfg = RF_Malloc(sizeof(*k80_cfg));
	if (k80_cfg == NULL)
		return ENOMEM;

	error = copyin(u80_cfg, k80_cfg, sizeof(*k80_cfg));
	if (error) {
		RF_Free(k80_cfg, sizeof(*k80_cfg));
		return error;
	}
	k_cfg = RF_Malloc(sizeof(*k_cfg));
	if (k_cfg == NULL) {
		RF_Free(k80_cfg, sizeof(*k80_cfg));
		return ENOMEM;
	}

	k_cfg->numCol = k80_cfg->numCol;
	k_cfg->numSpare = k80_cfg->numSpare;

	for (i = 0; i < RF_MAXROW; i++)
		for (j = 0; j < RF_MAXCOL; j++)
			k_cfg->devs[i][j] = k80_cfg->devs[i][j];

	memcpy(k_cfg->devnames, k80_cfg->devnames,
	    sizeof(k_cfg->devnames));

	for (i = 0; i < RF_MAXSPARE; i++)
		k_cfg->spare_devs[i] = k80_cfg->spare_devs[i];

	memcpy(k_cfg->spare_names, k80_cfg->spare_names,
	    sizeof(k_cfg->spare_names));

	k_cfg->sectPerSU = k80_cfg->sectPerSU;
	k_cfg->SUsPerPU = k80_cfg->SUsPerPU;
	k_cfg->SUsPerRU = k80_cfg->SUsPerRU;
	k_cfg->parityConfig = k80_cfg->parityConfig;

	memcpy(k_cfg->diskQueueType, k80_cfg->diskQueueType,
	    sizeof(k_cfg->diskQueueType));

	k_cfg->maxOutstandingDiskReqs = k80_cfg->maxOutstandingDiskReqs;

	memcpy(k_cfg->debugVars, k80_cfg->debugVars,
	    sizeof(k_cfg->debugVars));

	k_cfg->layoutSpecificSize = k80_cfg->layoutSpecificSize;
	k_cfg->layoutSpecific = k80_cfg->layoutSpecific;
	k_cfg->force = k80_cfg->force;

	RF_Free(k80_cfg, sizeof(*k80_cfg));
	return rf_construct(rs, k_cfg);
}

static int
rf_fail_disk80(RF_Raid_t *raidPtr, struct rf_recon_req80 *req80)
{
	struct rf_recon_req req = {
		.col = req80->col,
		.flags = req80->flags,
	};
	return rf_fail_disk(raidPtr, &req);
}

static int
raidframe_ioctl_80(struct raid_softc *rs, u_long cmd, void *data)
{
	RF_Raid_t *raidPtr = rf_get_raid(rs);
 
	switch (cmd) {
	case RAIDFRAME_CHECK_RECON_STATUS_EXT80:
	case RAIDFRAME_CHECK_PARITYREWRITE_STATUS_EXT80:
	case RAIDFRAME_CHECK_COPYBACK_STATUS_EXT80:
	case RAIDFRAME_GET_INFO80:
	case RAIDFRAME_GET_COMPONENT_LABEL80:
	case RAIDFRAME_FAIL_DISK80:
		if (!rf_inited(rs))
			return ENXIO;
		break;
	case RAIDFRAME_CONFIGURE80:
		break;
	default:
		return EPASSTHROUGH;
	}

	switch (cmd) {
	case RAIDFRAME_CHECK_RECON_STATUS_EXT80:
		return rf_check_recon_status_ext80(raidPtr, data);
	case RAIDFRAME_CHECK_PARITYREWRITE_STATUS_EXT80:
		return rf_check_parityrewrite_status_ext80(raidPtr, data);
	case RAIDFRAME_CHECK_COPYBACK_STATUS_EXT80:
		return rf_check_copyback_status_ext80(raidPtr, data);
	case RAIDFRAME_GET_INFO80:
		return rf_get_info80(raidPtr, data);
	case RAIDFRAME_GET_COMPONENT_LABEL80:
		return rf_get_component_label80(raidPtr, data);
	case RAIDFRAME_CONFIGURE80:
		return rf_config80(rs, data);
	case RAIDFRAME_FAIL_DISK80:
		return rf_fail_disk80(raidPtr, data);
	default:
		/* abort really */
		return EPASSTHROUGH;
	}
}
 
static void
raidframe_80_init(void)
{
  
	MODULE_HOOK_SET(raidframe_ioctl_80_hook, raidframe_ioctl_80);
}
 
static void
raidframe_80_fini(void)
{
 
	MODULE_HOOK_UNSET(raidframe_ioctl_80_hook);
}

MODULE(MODULE_CLASS_EXEC, compat_raid_80, "raid,compat_80");

static int
compat_raid_80_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		raidframe_80_init();
		return 0;
	case MODULE_CMD_FINI:
		raidframe_80_fini();
		return 0;
	default:
		return ENOTTY;
	}
}
