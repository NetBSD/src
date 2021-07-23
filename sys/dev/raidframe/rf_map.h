/*	$NetBSD: rf_map.h,v 1.14 2021/07/23 00:54:45 oster Exp $	*/
/*
 * Copyright (c) 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Mark Holland
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/* rf_map.h */

#ifndef _RF__RF_MAP_H_
#define _RF__RF_MAP_H_

#include <dev/raidframe/raidframevar.h>

#include "rf_alloclist.h"
#include "rf_raid.h"

/* mapping structure allocation and free routines */
RF_AccessStripeMapHeader_t *rf_MapAccess(RF_Raid_t *, RF_RaidAddr_t,
					 RF_SectorCount_t, void *, int);
void rf_MarkFailuresInASMList(RF_Raid_t *, RF_AccessStripeMapHeader_t *);
int rf_ConfigureMapModule(RF_ShutdownList_t **, RF_Raid_t *, RF_Config_t *);
RF_AccessStripeMapHeader_t *rf_AllocAccessStripeMapHeader(RF_Raid_t *);
void rf_FreeAccessStripeMapHeader(RF_Raid_t *, RF_AccessStripeMapHeader_t *);
RF_PhysDiskAddr_t *rf_AllocPhysDiskAddr(RF_Raid_t *);
RF_PhysDiskAddr_t *rf_AllocPDAList(RF_Raid_t *, int);
void rf_FreePhysDiskAddr(RF_Raid_t *, RF_PhysDiskAddr_t *);
RF_AccessStripeMap_t *rf_AllocASMList(RF_Raid_t *, int);
void rf_FreeAccessStripeMap(RF_Raid_t *, RF_AccessStripeMapHeader_t *);
int rf_CheckStripeForFailures(RF_Raid_t *, RF_AccessStripeMap_t *);
int rf_NumFailedDataUnitsInStripe(RF_Raid_t *, RF_AccessStripeMap_t *);
void rf_PrintAccessStripeMap(RF_AccessStripeMapHeader_t *);
void rf_PrintFullAccessStripeMap(RF_AccessStripeMapHeader_t *, int);
void rf_PrintRaidAddressInfo(RF_Raid_t *, RF_RaidAddr_t, RF_SectorCount_t);
void rf_ASMParityAdjust(RF_Raid_t *, RF_PhysDiskAddr_t *, RF_StripeNum_t, RF_SectorNum_t,
			RF_RaidLayout_t *, RF_AccessStripeMap_t *);
void rf_ASMCheckStatus(RF_Raid_t *, RF_PhysDiskAddr_t *, RF_AccessStripeMap_t *,
		       RF_RaidDisk_t *, int);

RF_VoidFunctionPointerListElem_t *rf_AllocVFPListElem(RF_Raid_t *);
void rf_FreeVFPListElem(RF_Raid_t *, RF_VoidFunctionPointerListElem_t *);
RF_VoidPointerListElem_t *rf_AllocVPListElem(RF_Raid_t *);
void rf_FreeVPListElem(RF_Raid_t *, RF_VoidPointerListElem_t *);
RF_ASMHeaderListElem_t *rf_AllocASMHeaderListElem(RF_Raid_t *);
void rf_FreeASMHeaderListElem(RF_Raid_t *, RF_ASMHeaderListElem_t *);
RF_FailedStripe_t *rf_AllocFailedStripeStruct(RF_Raid_t *);
void rf_FreeFailedStripeStruct(RF_Raid_t *, RF_FailedStripe_t *);

#endif				/* !_RF__RF_MAP_H_ */
