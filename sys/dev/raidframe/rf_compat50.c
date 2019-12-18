/*	$NetBSD: rf_compat50.c,v 1.10.4.1 2019/12/18 20:04:32 martin Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/module.h>

#include <sys/compat_stub.h>

#include <dev/raidframe/raidframeio.h>
#include <dev/raidframe/raidframevar.h>

#include "rf_raid.h"
#include "rf_compat50.h"
#include "rf_debugMem.h"

typedef struct RF_Config50_s {
	RF_RowCol_t		numRow, numCol, numSpare;
	int32_t			devs[RF_MAXROW][RF_MAXCOL];
	char			devnames[RF_MAXROW][RF_MAXCOL][50];
	int32_t			spare_devs[RF_MAXSPARE];
	char			spare_names[RF_MAXSPARE][50];
	RF_SectorNum_t		sectPerSU;
	RF_StripeNum_t		SUsPerPU;
	RF_StripeNum_t		SUsPerRU;
	RF_ParityConfig_t	parityConfig;
	RF_DiskQueueType_t	diskQueueType;
	char			maxOutstandingDiskReqs;
	char			debugVars[RF_MAXDBGV][50];
	unsigned int		layoutSpecificSize;
	void		       *layoutSpecific;
	int			force;
} RF_Config50_t;

typedef struct RF_RaidDisk50_s {
        char		 devname[56];
        RF_DiskStatus_t	 status;
        RF_RowCol_t	 spareRow;
        RF_RowCol_t	 spareCol;
        RF_SectorCount_t numBlocks;
        int     	 blockSize;
        RF_SectorCount_t partitionSize;
        int    		 auto_configured;
        int32_t		 dev;
} RF_RaidDisk50_t;

typedef struct RF_DeviceConfig50_s {
	u_int			rows;
	u_int			cols;
	u_int			maxqdepth;
	int			ndevs;
	RF_RaidDisk50_t		devs[RF_MAX_DISKS];
	int			nspares;
	RF_RaidDisk50_t		spares[RF_MAX_DISKS];
} RF_DeviceConfig50_t;

static void
rf_disk_to_disk50(RF_RaidDisk50_t *d50, const RF_RaidDisk_t *d)
{
        memcpy(d50->devname, d->devname, sizeof(d50->devname));
        d50->status = d->status;
        d50->spareRow = 0;
        d50->spareCol = d->spareCol;
        d50->numBlocks = d->numBlocks;
        d50->blockSize = d->blockSize;
        d50->partitionSize = d->partitionSize;
        d50->auto_configured = d->auto_configured;
        d50->dev = d->dev;
}

static int
rf_config50(struct raid_softc *rs, void *data)
{
	RF_Config50_t *u50_cfg, *k50_cfg;
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

	u50_cfg = *((RF_Config50_t **) data);
	k50_cfg = RF_Malloc(sizeof(*k50_cfg));
	if (k50_cfg == NULL)
		return ENOMEM;

	error = copyin(u50_cfg, k50_cfg, sizeof(*k50_cfg));
	if (error) {
		RF_Free(k50_cfg, sizeof(*k50_cfg));
		return error;
	}
	k_cfg = RF_Malloc(sizeof(*k_cfg));
	if (k_cfg == NULL) {
		RF_Free(k50_cfg, sizeof(*k50_cfg));
		return ENOMEM;
	}

	k_cfg->numCol = k50_cfg->numCol;
	k_cfg->numSpare = k50_cfg->numSpare;

	for (i = 0; i < RF_MAXROW; i++)
		for (j = 0; j < RF_MAXCOL; j++)
			k_cfg->devs[i][j] = k50_cfg->devs[i][j];

	memcpy(k_cfg->devnames, k50_cfg->devnames,
	    sizeof(k_cfg->devnames));

	for (i = 0; i < RF_MAXSPARE; i++)
		k_cfg->spare_devs[i] = k50_cfg->spare_devs[i];

	memcpy(k_cfg->spare_names, k50_cfg->spare_names,
	    sizeof(k_cfg->spare_names));

	k_cfg->sectPerSU = k50_cfg->sectPerSU;
	k_cfg->SUsPerPU = k50_cfg->SUsPerPU;
	k_cfg->SUsPerRU = k50_cfg->SUsPerRU;
	k_cfg->parityConfig = k50_cfg->parityConfig;

	memcpy(k_cfg->diskQueueType, k50_cfg->diskQueueType,
	    sizeof(k_cfg->diskQueueType));

	k_cfg->maxOutstandingDiskReqs = k50_cfg->maxOutstandingDiskReqs;

	memcpy(k_cfg->debugVars, k50_cfg->debugVars,
	    sizeof(k_cfg->debugVars));

	k_cfg->layoutSpecificSize = k50_cfg->layoutSpecificSize;
	k_cfg->layoutSpecific = k50_cfg->layoutSpecific;
	k_cfg->force = k50_cfg->force;

	RF_Free(k50_cfg, sizeof(*k50_cfg));
	return rf_construct(rs, k_cfg);
}

static int
rf_get_info50(RF_Raid_t *raidPtr, void *data)
{
	RF_DeviceConfig50_t **ucfgp = data, *d_cfg;
	size_t i, j;
	int error;

	if (!raidPtr->valid)
		return ENODEV;

	d_cfg = RF_Malloc(sizeof(*d_cfg));

	if (d_cfg == NULL)
		return ENOMEM;

	d_cfg->rows = 1; /* there is only 1 row now */
	d_cfg->cols = raidPtr->numCol;
	d_cfg->ndevs = raidPtr->numCol;
	if (d_cfg->ndevs >= RF_MAX_DISKS) {
		error = ENOMEM;
		goto out;
	}

	d_cfg->nspares = raidPtr->numSpare;
	if (d_cfg->nspares >= RF_MAX_DISKS) {
		error = ENOMEM;
		goto out;
	}

	d_cfg->maxqdepth = raidPtr->maxQueueDepth;
	for (j = 0; j < d_cfg->cols; j++)
		rf_disk_to_disk50(&d_cfg->devs[j], &raidPtr->Disks[j]);

	for (j = d_cfg->cols, i = 0; i < d_cfg->nspares; i++, j++)
		rf_disk_to_disk50(&d_cfg->spares[i], &raidPtr->Disks[j]);

	error = copyout(d_cfg, *ucfgp, sizeof(**ucfgp));

out:
	RF_Free(d_cfg, sizeof(*d_cfg));
	return error;
}

static int
raidframe_ioctl_50(struct raid_softc *rs, u_long cmd, void *data)
{
	RF_Raid_t *raidPtr = rf_get_raid(rs);

	switch (cmd) {
	case RAIDFRAME_GET_INFO50:
		if (!rf_inited(rs))
			return ENXIO;
		return rf_get_info50(raidPtr, data);

	case RAIDFRAME_CONFIGURE50:
		return rf_config50(rs, data);
	default:
		return EPASSTHROUGH;
	}
}

static void
raidframe_50_init(void)
{

	MODULE_HOOK_SET(raidframe_ioctl_50_hook, "raid50", raidframe_ioctl_50);
}

static void
raidframe_50_fini(void)
{

	MODULE_HOOK_UNSET(raidframe_ioctl_50_hook);
}

MODULE(MODULE_CLASS_EXEC, compat_raid_50, "raid,compat_50,compat_raid_80");

static int
compat_raid_50_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		raidframe_50_init();
		return 0;
	case MODULE_CMD_FINI:
		raidframe_50_fini();
		return 0;
	default:
		return ENOTTY;
	}
}
