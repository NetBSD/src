/*	$NetBSD: rf_reconmap.c,v 1.24 2004/03/18 16:54:54 oster Exp $	*/
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

/*************************************************************************
 * rf_reconmap.c
 *
 * code to maintain a map of what sectors have/have not been reconstructed
 *
 *************************************************************************/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rf_reconmap.c,v 1.24 2004/03/18 16:54:54 oster Exp $");

#include "rf_raid.h"
#include <sys/time.h>
#include "rf_general.h"
#include "rf_utils.h"

/* special pointer values indicating that a reconstruction unit
 * has been either totally reconstructed or not at all.  Both
 * are illegal pointer values, so you have to be careful not to
 * dereference through them.  RU_NOTHING must be zero, since
 * MakeReconMap uses memset to initialize the structure.  These are used
 * only at the head of the list.
 */
#define RU_ALL      ((RF_ReconMapListElem_t *) -1)
#define RU_NOTHING  ((RF_ReconMapListElem_t *) 0)

/* For most reconstructs we need at most 3 RF_ReconMapListElem_t's.
 * Bounding the number we need is quite difficult, as it depends on how
 * badly the sectors to be reconstructed get divided up.  In the current
 * code, the reconstructed sectors appeared aligned on stripe boundaries, 
 * and are always presented in stripe width units, so we're probably 
 * allocating quite a bit more than we'll ever need.
 */
#define RF_NUM_RECON_POOL_ELEM 100

static void 
compact_stat_entry(RF_Raid_t * raidPtr, RF_ReconMap_t * mapPtr,
    int i);
static void crunch_list(RF_ReconMap_t *mapPtr, 
			RF_ReconMapListElem_t * listPtr);
static RF_ReconMapListElem_t *
MakeReconMapListElem(RF_ReconMap_t *mapPtr, RF_SectorNum_t startSector,
		     RF_SectorNum_t stopSector, RF_ReconMapListElem_t * next);
static void 
FreeReconMapListElem(RF_ReconMap_t *mapPtr, RF_ReconMapListElem_t * p);
#if 0
static void PrintList(RF_ReconMapListElem_t * listPtr);
#endif

/*---------------------------------------------------------------------------
 *
 * Creates and initializes new Reconstruction map
 *
 * ru_sectors   - size of reconstruction unit in sectors
 * disk_sectors - size of disk in sectors
 * spareUnitsPerDisk - zero unless distributed sparing
 *-------------------------------------------------------------------------*/

RF_ReconMap_t *
rf_MakeReconMap(RF_Raid_t *raidPtr, RF_SectorCount_t ru_sectors, 
		RF_SectorCount_t disk_sectors,
		RF_ReconUnitCount_t spareUnitsPerDisk)
{
	RF_RaidLayout_t *layoutPtr = &raidPtr->Layout;
	RF_ReconUnitCount_t num_rus = layoutPtr->stripeUnitsPerDisk / layoutPtr->SUsPerRU;
	RF_ReconMap_t *p;

	RF_Malloc(p, sizeof(RF_ReconMap_t), (RF_ReconMap_t *));
	p->sectorsPerReconUnit = ru_sectors;
	p->sectorsInDisk = disk_sectors;

	p->totalRUs = num_rus;
	p->spareRUs = spareUnitsPerDisk;
	p->unitsLeft = num_rus - spareUnitsPerDisk;

	RF_Malloc(p->status, num_rus * sizeof(RF_ReconMapListElem_t *), (RF_ReconMapListElem_t **));
	RF_ASSERT(p->status != (RF_ReconMapListElem_t **) NULL);

	(void) memset((char *) p->status, 0,
	    num_rus * sizeof(RF_ReconMapListElem_t *));

	
	pool_init(&p->elem_pool, sizeof(RF_ReconMapListElem_t), 0,
	    0, 0, "raidreconpl", NULL);
	pool_prime(&p->elem_pool, RF_NUM_RECON_POOL_ELEM);

	rf_mutex_init(&p->mutex);
	return (p);
}


/*---------------------------------------------------------------------------
 *
 * marks a new set of sectors as reconstructed.  All the possible
 * mergings get complicated.  To simplify matters, the approach I take
 * is to just dump something into the list, and then clean it up
 * (i.e. merge elements and eliminate redundant ones) in a second pass
 * over the list (compact_stat_entry()).  Not 100% efficient, since a
 * structure can be allocated and then immediately freed, but it keeps
 * this code from becoming (more of) a nightmare of special cases.
 * The only thing that compact_stat_entry() assumes is that the list
 * is sorted by startSector, and so this is the only condition I
 * maintain here.  (MCH)
 *
 * This code now uses a pool instead of the previous malloc/free
 * stuff.
 *-------------------------------------------------------------------------*/

void 
rf_ReconMapUpdate(RF_Raid_t *raidPtr, RF_ReconMap_t *mapPtr,
		  RF_SectorNum_t startSector, RF_SectorNum_t stopSector)
{
	RF_SectorCount_t sectorsPerReconUnit = mapPtr->sectorsPerReconUnit;
	RF_SectorNum_t i, first_in_RU, last_in_RU;
	RF_ReconMapListElem_t *p, *pt;

	RF_LOCK_MUTEX(mapPtr->mutex);
	while(mapPtr->lock) {
		printf("napping...\n");
		ltsleep(&mapPtr->lock, PRIBIO, "reconupdate", 0, &mapPtr->mutex);
	}
	mapPtr->lock = 1;
	RF_UNLOCK_MUTEX(mapPtr->mutex);
	RF_ASSERT(startSector >= 0 && stopSector < mapPtr->sectorsInDisk && 
		  stopSector >= startSector);

	while (startSector <= stopSector) {
		i = startSector / mapPtr->sectorsPerReconUnit;
		first_in_RU = i * sectorsPerReconUnit;
		last_in_RU = first_in_RU + sectorsPerReconUnit - 1;
		p = mapPtr->status[i];
		if (p != RU_ALL) {
			if (p == RU_NOTHING || p->startSector > startSector) {
				/* insert at front of list */

				mapPtr->status[i] = MakeReconMapListElem(mapPtr,startSector, RF_MIN(stopSector, last_in_RU), (p == RU_NOTHING) ? NULL : p);

			} else {/* general case */
				do {	/* search for place to insert */
					pt = p;
					p = p->next;
				} while (p && (p->startSector < startSector));
				pt->next = MakeReconMapListElem(mapPtr,startSector, RF_MIN(stopSector, last_in_RU), p);

			}
			compact_stat_entry(raidPtr, mapPtr, i);
		}
		startSector = RF_MIN(stopSector, last_in_RU) + 1;
	}
	RF_LOCK_MUTEX(mapPtr->mutex);
	mapPtr->lock = 0;
	wakeup(&mapPtr->lock);
	RF_UNLOCK_MUTEX(mapPtr->mutex);
}



/*---------------------------------------------------------------------------
 *
 * performs whatever list compactions can be done, and frees any space
 * that is no longer necessary.  Assumes only that the list is sorted
 * by startSector.  crunch_list() compacts a single list as much as
 * possible, and the second block of code deletes the entire list if
 * possible.  crunch_list() is also called from
 * MakeReconMapAccessList().
 *
 * When a recon unit is detected to be fully reconstructed, we set the
 * corresponding bit in the parity stripe map so that the head follow
 * code will not select this parity stripe again.  This is redundant
 * (but harmless) when compact_stat_entry is called from the
 * reconstruction code, but necessary when called from the user-write
 * code.
 *
 *-------------------------------------------------------------------------*/

static void 
compact_stat_entry(RF_Raid_t *raidPtr, RF_ReconMap_t *mapPtr, int i)
{
	RF_SectorCount_t sectorsPerReconUnit = mapPtr->sectorsPerReconUnit;
	RF_ReconMapListElem_t *p = mapPtr->status[i];

	crunch_list(mapPtr, p);

	if ((p->startSector == i * sectorsPerReconUnit) &&
	    (p->stopSector == i * sectorsPerReconUnit + 
			      sectorsPerReconUnit - 1)) {
		mapPtr->status[i] = RU_ALL;
		mapPtr->unitsLeft--;
		FreeReconMapListElem(mapPtr, p);
	}
}

static void 
crunch_list(RF_ReconMap_t *mapPtr, RF_ReconMapListElem_t *listPtr)
{
	RF_ReconMapListElem_t *pt, *p = listPtr;

	if (!p)
		return;
	pt = p;
	p = p->next;
	while (p) {
		if (pt->stopSector >= p->startSector - 1) {
			pt->stopSector = RF_MAX(pt->stopSector, p->stopSector);
			pt->next = p->next;
			FreeReconMapListElem(mapPtr, p);
			p = pt->next;
		} else {
			pt = p;
			p = p->next;
		}
	}
}
/*---------------------------------------------------------------------------
 *
 * Allocate and fill a new list element
 *
 *-------------------------------------------------------------------------*/

static RF_ReconMapListElem_t *
MakeReconMapListElem(RF_ReconMap_t *mapPtr, RF_SectorNum_t startSector,
		     RF_SectorNum_t stopSector, RF_ReconMapListElem_t *next)
{
	RF_ReconMapListElem_t *p;

	p = pool_get(&mapPtr->elem_pool, PR_WAITOK);
	p->startSector = startSector;
	p->stopSector = stopSector;
	p->next = next;
	return (p);
}
/*---------------------------------------------------------------------------
 *
 * Free a list element
 *
 *-------------------------------------------------------------------------*/

static void 
FreeReconMapListElem(RF_ReconMap_t *mapPtr, RF_ReconMapListElem_t *p)
{
	pool_put(&mapPtr->elem_pool, p);
}
/*---------------------------------------------------------------------------
 *
 * Free an entire status structure.  Inefficient, but can be called at
 * any time.
 *
 *-------------------------------------------------------------------------*/
void 
rf_FreeReconMap(RF_ReconMap_t *mapPtr)
{
	RF_ReconMapListElem_t *p, *q;
	RF_ReconUnitCount_t numRUs;
	RF_ReconUnitNum_t i;

	numRUs = mapPtr->sectorsInDisk / mapPtr->sectorsPerReconUnit;
	if (mapPtr->sectorsInDisk % mapPtr->sectorsPerReconUnit)
		numRUs++;

	for (i = 0; i < numRUs; i++) {
		p = mapPtr->status[i];
		while (p != RU_NOTHING && p != RU_ALL) {
			q = p;
			p = p->next;
			RF_Free(q, sizeof(*q));
		}
	}
	pool_destroy(&mapPtr->elem_pool);
	RF_Free(mapPtr->status, mapPtr->totalRUs * 
		sizeof(RF_ReconMapListElem_t *));
	RF_Free(mapPtr, sizeof(RF_ReconMap_t));
}
/*---------------------------------------------------------------------------
 *
 * returns nonzero if the indicated RU has been reconstructed already
 *
 *-------------------------------------------------------------------------*/

int 
rf_CheckRUReconstructed(RF_ReconMap_t *mapPtr, RF_SectorNum_t startSector)
{
	RF_ReconMapListElem_t *l;	/* used for searching */
	RF_ReconUnitNum_t i;

	i = startSector / mapPtr->sectorsPerReconUnit;
	l = mapPtr->status[i];
	return ((l == RU_ALL) ? 1 : 0);
}

RF_ReconUnitCount_t 
rf_UnitsLeftToReconstruct(RF_ReconMap_t *mapPtr)
{
	RF_ASSERT(mapPtr != NULL);
	return (mapPtr->unitsLeft);
}

#if 0
static void 
PrintList(RF_ReconMapListElem_t *listPtr)
{
	while (listPtr) {
		printf("%d,%d -> ", (int) listPtr->startSector, 
		       (int) listPtr->stopSector);
		listPtr = listPtr->next;
	}
	printf("\n");
}

void 
rf_PrintReconMap(RF_Raid_t *raidPtr, RF_ReconMap_t *mapPtr, RF_RowCol_t fcol)
{
	RF_ReconUnitCount_t numRUs;
	RF_ReconMapListElem_t *p;
	RF_ReconUnitNum_t i;

	numRUs = mapPtr->totalRUs;
	if (mapPtr->sectorsInDisk % mapPtr->sectorsPerReconUnit)
		numRUs++;

	for (i = 0; i < numRUs; i++) {
		p = mapPtr->status[i];
		if (p == RU_ALL)/* printf("[%d] ALL\n",i) */
			;
		else
			if (p == RU_NOTHING) {
				printf("%d: Unreconstructed\n", i);
			} else {
				printf("%d: ", i);
				PrintList(p);
			}
	}
}
#endif

#if RF_DEBUG_RECON
void 
rf_PrintReconSchedule(RF_ReconMap_t *mapPtr, struct timeval *starttime)
{
	static int old_pctg = -1;
	struct timeval tv, diff;
	int     new_pctg;

	new_pctg = 100 - (rf_UnitsLeftToReconstruct(mapPtr) * 
			  100 / mapPtr->totalRUs);
	if (new_pctg != old_pctg) {
		RF_GETTIME(tv);
		RF_TIMEVAL_DIFF(starttime, &tv, &diff);
		printf("%d %d.%06d\n", (int) new_pctg, (int) diff.tv_sec, 
		       (int) diff.tv_usec);
		old_pctg = new_pctg;
	}
}
#endif

