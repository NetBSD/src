/*	$NetBSD: rf_compat32.c,v 1.1 2018/01/18 00:32:49 mrg Exp $	*/

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
#include "rf_compat32.h"

#include <compat/netbsd32/netbsd32.h>

typedef netbsd32_int64 RF_SectorNum_t32;
typedef netbsd32_int64 RF_StripeNum_t32;

/* from <dev/raidframe/raidframevar.h> */
typedef struct RF_Config_s32 {
	RF_RowCol_t numCol, numSpare;		/* number of rows, columns,
						 * and spare disks */
	dev_t   devs[RF_MAXROW][RF_MAXCOL];	/* device numbers for disks
						 * comprising array */
	char    devnames[RF_MAXROW][RF_MAXCOL][50];	/* device names */
	dev_t   spare_devs[RF_MAXSPARE];	/* device numbers for spare
						 * disks */
	char    spare_names[RF_MAXSPARE][50];	/* device names */
	RF_SectorNum_t32 sectPerSU;	/* sectors per stripe unit */
	RF_StripeNum_t32 SUsPerPU;/* stripe units per parity unit */
	RF_StripeNum_t32 SUsPerRU;/* stripe units per reconstruction unit */
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
	netbsd32_pointer_t   layoutSpecific;	/* a pointer to a layout-specific structure to
				 * be copied in */
	int     force;                          /* if !0, ignore many fatal
						   configuration conditions */
	/*
	   "force" is used to override cases where the component labels would
	   indicate that configuration should not proceed without user
	   intervention
	 */
} RF_Config_t32 ;

int
rf_config_netbsd32(void *data, RF_Config_t *k_cfg)
{
	RF_Config_t32 *u_cfg = NETBSD32PTR64(*(netbsd32_pointer_t *)data);
	struct RF_Config_s32 *cfg32;
	int rv;

	RF_Malloc(cfg32, sizeof(RF_Config_t), (RF_Config_t32 *));
	if (cfg32 == NULL)
		return (ENOMEM);

	rv = copyin(u_cfg, cfg32, sizeof(RF_Config_t32));
	if (rv)
		goto out;

	k_cfg->numCol = cfg32->numCol;
	k_cfg->numSpare = cfg32->numSpare;
	memcpy(k_cfg->devs, cfg32->devs, sizeof(k_cfg->devs));
	memcpy(k_cfg->devnames, cfg32->devnames, sizeof(k_cfg->devnames));
	memcpy(k_cfg->spare_devs, cfg32->spare_devs, sizeof(k_cfg->spare_devs));
	memcpy(k_cfg->spare_names, cfg32->spare_names, sizeof(k_cfg->spare_names));
	k_cfg->sectPerSU = cfg32->sectPerSU;
	k_cfg->SUsPerPU = cfg32->SUsPerPU;
	k_cfg->SUsPerRU = cfg32->SUsPerRU;
	k_cfg->parityConfig = cfg32->parityConfig;
	memcpy(k_cfg->diskQueueType, cfg32->diskQueueType, sizeof(k_cfg->diskQueueType));
	k_cfg->maxOutstandingDiskReqs = cfg32->maxOutstandingDiskReqs;
	memcpy(k_cfg->debugVars, cfg32->debugVars, sizeof(k_cfg->debugVars));
	k_cfg->layoutSpecificSize = cfg32->layoutSpecificSize;
	k_cfg->layoutSpecific = NETBSD32PTR64(cfg32->layoutSpecific);
	k_cfg->force = cfg32->force;

 out:
	RF_Free(cfg32, sizeof(RF_Config_t32));
	return rv;
}
