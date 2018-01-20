/*	$NetBSD: rf_compat80.c,v 1.2 2018/01/20 01:32:45 mrg Exp $	*/

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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>

#include <dev/raidframe/raidframeio.h>
#include <dev/raidframe/raidframevar.h>

#include "rf_raid.h"
#include "rf_compat80.h"
#include "rf_kintf.h"

int
rf_check_recon_status_ext80(RF_Raid_t *raidPtr, void *data)
{
	RF_ProgressInfo_t info, **infoPtr = data;

	rf_check_recon_status_ext(raidPtr, &info);
	return copyout(&info, *infoPtr, sizeof info);
}

int
rf_check_parityrewrite_status_ext80(RF_Raid_t *raidPtr, void *data)
{
	RF_ProgressInfo_t info, **infoPtr = data;

	rf_check_parityrewrite_status_ext(raidPtr, &info);
	return copyout(&info, *infoPtr, sizeof info);
}

int
rf_check_copyback_status_ext80(RF_Raid_t *raidPtr, void *data)
{
	RF_ProgressInfo_t info, **infoPtr = data;

	rf_check_copyback_status_ext(raidPtr, &info);
	return copyout(&info, *infoPtr, sizeof info);
}

static void
rf_copy_raiddisk80(RF_RaidDisk_t *disk, RF_RaidDisk_t80 *disk80)
{

	/* Be sure the padding areas don't have kernel memory. */
	memset(disk80, 0, sizeof *disk80);
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

int
rf_get_info80(RF_Raid_t *raidPtr, void *data)
{
	RF_DeviceConfig_t *config;
	RF_DeviceConfig_t80 *config80, **configPtr80 = data;
	int rv;

	RF_Malloc(config, sizeof *config, (RF_DeviceConfig_t *));
	if (config == NULL)
		return (ENOMEM);
	RF_Malloc(config80, sizeof *config80, (RF_DeviceConfig_t80 *));
	if (config80 == NULL) {
		RF_Free(config, sizeof(RF_DeviceConfig_t))
		return (ENOMEM);
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
		rv = copyout(config80, *configPtr80, sizeof *config80);
	}
	RF_Free(config, sizeof(RF_DeviceConfig_t));
	RF_Free(config80, sizeof(RF_DeviceConfig_t80));

	return rv;
}

int
rf_get_component_label80(RF_Raid_t *raidPtr, void *data)
{
	RF_ComponentLabel_t **clabel_ptr = (RF_ComponentLabel_t **)data;
	RF_ComponentLabel_t *clabel;
	int retcode;

	/*
	 * Perhaps there should be an option to skip the in-core
	 * copy and hit the disk, as with disklabel(8).
	 */
	RF_Malloc(clabel, sizeof(*clabel), (RF_ComponentLabel_t *));
	if (clabel == NULL)
		return ENOMEM;
	retcode = copyin(*clabel_ptr, clabel, sizeof(*clabel));
	if (retcode) {
		RF_Free(clabel, sizeof(*clabel));
		return retcode;
	}

	rf_get_component_label(raidPtr, clabel);
	retcode = copyout(clabel, *clabel_ptr, sizeof(**clabel_ptr));
	RF_Free(clabel, sizeof(*clabel));

	return retcode;
}

int
rf_config80(RF_Raid_t *raidPtr, int unit, void *data, RF_Config_t **k_cfgp)
{
	RF_Config_t80 *u80_cfg, *k80_cfg;
	RF_Config_t *k_cfg;
	size_t i, j;
	int error;

	if (raidPtr->valid) {
		/* There is a valid RAID set running on this unit! */
		printf("raid%d: Device already configured!\n", unit);
		return EINVAL;
	}

	/* copy-in the configuration information */
	/* data points to a pointer to the configuration structure */

	u80_cfg = *((RF_Config_t80 **) data);
	RF_Malloc(k80_cfg, sizeof(RF_Config_t80), (RF_Config_t80 *));
	if (k80_cfg == NULL)
		return ENOMEM;

	error = copyin(u80_cfg, k80_cfg, sizeof(RF_Config_t80));
	if (error) {
		RF_Free(k80_cfg, sizeof(RF_Config_t80));
		return error;
	}
	RF_Malloc(k_cfg, sizeof(RF_Config_t), (RF_Config_t *));
	if (k_cfg == NULL) {
		RF_Free(k80_cfg, sizeof(RF_Config_t80));
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

	RF_Free(k80_cfg, sizeof(RF_Config_t80));
	*k_cfgp = k_cfg;
	return 0;
}
