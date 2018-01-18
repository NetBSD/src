/*	$NetBSD: rf_compat80.c,v 1.1 2018/01/18 00:32:49 mrg Exp $	*/

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
		rv = copyout(&config, *configPtr80, sizeof *config80);
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
