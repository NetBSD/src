/*	$NetBSD: rf_reconmap.h,v 1.7 2003/12/29 02:38:18 oster Exp $	*/
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

/******************************************************************************
 * rf_reconMap.h -- Header file describing reconstruction status data structure
 ******************************************************************************/

#ifndef _RF__RF_RECONMAP_H_
#define _RF__RF_RECONMAP_H_

#include <dev/raidframe/raidframevar.h>
#include <sys/pool.h>

#include "rf_threadstuff.h"

/*
 * Main reconstruction status descriptor.
 */
struct RF_ReconMap_s {
	RF_SectorCount_t sectorsPerReconUnit;	/* sectors per reconstruct
						 * unit */
	RF_SectorCount_t sectorsInDisk;	/* total sectors in disk */
	RF_SectorCount_t unitsLeft;	/* recon units left to recon */
	RF_ReconUnitCount_t totalRUs;	/* total recon units on disk */
	RF_ReconUnitCount_t spareRUs;	/* total number of spare RUs on failed
					 * disk */
	RF_StripeCount_t totalParityStripes;	/* total number of parity
						 * stripes in array */
	RF_ReconMapListElem_t **status;	/* array of ptrs to list elements */
	struct pool elem_pool;          /* pool of RF_ReconMapListElem_t's */
	RF_DECLARE_MUTEX(mutex)
};
/* a list element */
struct RF_ReconMapListElem_s {
	RF_SectorNum_t startSector;	/* bounding sect nums on this block */
	RF_SectorNum_t stopSector;
	RF_ReconMapListElem_t *next;	/* next element in list */
};

RF_ReconMap_t *
rf_MakeReconMap(RF_Raid_t * raidPtr, RF_SectorCount_t ru_sectors,
    RF_SectorCount_t disk_sectors, RF_ReconUnitCount_t spareUnitsPerDisk);

void 
rf_ReconMapUpdate(RF_Raid_t * raidPtr, RF_ReconMap_t * mapPtr,
    RF_SectorNum_t startSector, RF_SectorNum_t stopSector);

void    rf_FreeReconMap(RF_ReconMap_t * mapPtr);

int     rf_CheckRUReconstructed(RF_ReconMap_t * mapPtr, RF_SectorNum_t startSector);

RF_ReconUnitCount_t rf_UnitsLeftToReconstruct(RF_ReconMap_t * mapPtr);

void 
rf_PrintReconMap(RF_Raid_t * raidPtr, RF_ReconMap_t * mapPtr,
    RF_RowCol_t fcol);

void    rf_PrintReconSchedule(RF_ReconMap_t * mapPtr, struct timeval * starttime);

#endif				/* !_RF__RF_RECONMAP_H_ */
