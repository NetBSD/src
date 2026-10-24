/*	$NetBSD: rf_driver.c,v 1.146 2026/10/24 00:20:24 tls Exp $	*/
/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Greg Oster
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

/*
 * Copyright (c) 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Mark Holland, Khalil Amiri, Claudson Bornstein, William V. Courtright II,
 *         Robby Findler, Daniel Stodolsky, Rachad Youssef, Jim Zelenka
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
 *
 * rf_driver.c -- main setup, teardown, and access routines for the RAID driver
 *
 * all routines are prefixed with rf_ (raidframe), to avoid conflicts.
 *
 ******************************************************************************/


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rf_driver.c,v 1.146 2026/10/24 00:20:24 tls Exp $");

#ifdef _KERNEL_OPT
#include "opt_raid_diagnostic.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>


#include "rf_archs.h"
#include "rf_threadstuff.h"

#include <sys/errno.h>

#include "rf_raid.h"
#include "rf_dag.h"
#include "rf_aselect.h"
#include "rf_diskqueue.h"
#include "rf_parityscan.h"
#include "rf_alloclist.h"
#include "rf_dagutils.h"
#include "rf_utils.h"
#include "rf_etimer.h"
#include "rf_acctrace.h"
#include "rf_general.h"
#include "rf_desc.h"
#include "rf_states.h"
#include "rf_decluster.h"
#include "rf_map.h"
#include "rf_revent.h"
#include "rf_callback.h"
#include "rf_engine.h"
#include "rf_mcpair.h"
#include "rf_nwayxor.h"
#include "rf_driver.h"
#include "rf_options.h"
#include "rf_shutdown.h"
#include "rf_kintf.h"
#include "rf_paritymap.h"

#include <sys/buf.h>

#ifndef RF_ACCESS_DEBUG
#define RF_ACCESS_DEBUG 0
#endif

/* main configuration routines */
static int raidframe_booted = 0;

static void rf_ConfigureDebug(RF_Config_t * cfgPtr);
static void set_debug_option(char *name, long val);
static void rf_UnconfigureArray(void);
static void rf_ShutdownRDFreeList(void *);
static int rf_ConfigureRDFreeList(RF_ShutdownList_t **, RF_Raid_t *, RF_Config_t *);

rf_declare_mutex2(rf_printf_mutex);	/* debug only:  avoids interleaved
					 * printfs by different stripes */

#define SIGNAL_QUIESCENT_COND(_raid_) \
	rf_broadcast_cond2((_raid_)->access_suspend_cv)
#define WAIT_FOR_QUIESCENCE(_raid_) \
	rf_wait_cond2((_raid_)->access_suspend_cv, \
		      (_raid_)->access_suspend_mutex)

static int configureCount = 0;	/* number of active configurations */
static int isconfigged = 0;	/* is basic raidframe (non per-array)
				 * stuff configured */
static rf_declare_mutex2(configureMutex); /* used to lock the configuration
					   * stuff */
static RF_ShutdownList_t *globalShutdown;	/* non array-specific
						 * stuff */

static int rf_ConfigureRDFreeList(RF_ShutdownList_t ** listp, RF_Raid_t *raidPtr, RF_Config_t *cfgPtr);
static void rf_ComputeOpenings(RF_Raid_t *);
static int rf_AllocReservedBuffers(RF_Raid_t *);
static void rf_FreeReservedBuffers(RF_Raid_t *);
static void rf_destroy_mutex_cond(RF_Raid_t *);
static void rf_alloc_mutex_cond(RF_Raid_t *);

/* called at system boot time */
int
rf_BootRaidframe(bool boot)
{

	if (boot) {
		if (raidframe_booted)
			return (EBUSY);
		raidframe_booted = 1;
		rf_init_mutex2(configureMutex, IPL_NONE);
 		configureCount = 0;
		isconfigged = 0;
		globalShutdown = NULL;
	} else {
		rf_destroy_mutex2(configureMutex);
		raidframe_booted = 0;
	}
	return (0);
}

/*
 * Called whenever an array is shutdown
 */
static void
rf_UnconfigureArray(void)
{

	rf_lock_mutex2(configureMutex);
	if (--configureCount == 0) {	/* if no active configurations, shut
					 * everything down */
		rf_destroy_mutex2(rf_printf_mutex);
		isconfigged = 0;
		rf_ShutdownList(&globalShutdown);

		/*
	         * We must wait until now, because the AllocList module
	         * uses the DebugMem module.
	         */
#if RF_DEBUG_MEM
		if (rf_memDebug)
			rf_print_unfreed();
#endif
	}
	rf_unlock_mutex2(configureMutex);
}

/*
 * Called to shut down an array.
 */
int
rf_Shutdown(RF_Raid_t *raidPtr)
{

	if (!raidPtr->valid) {
		RF_ERRORMSG("Attempt to shut down unconfigured RAIDframe driver.  Aborting shutdown\n");
		return (EINVAL);
	}
	/*
         * wait for outstanding IOs to land
         * As described in rf_raid.h, we use the rad_freelist lock
         * to protect the per-array info about outstanding descs
         * since we need to do freelist locking anyway, and this
         * cuts down on the amount of serialization we've got going
         * on.
         */
	rf_lock_mutex2(raidPtr->rad_lock);
	if (raidPtr->waitShutdown) {
		rf_unlock_mutex2(raidPtr->rad_lock);
		return (EBUSY);
	}
	raidPtr->waitShutdown = 1;
	while (raidPtr->nAccOutstanding) {
		rf_wait_cond2(raidPtr->outstandingCond, raidPtr->rad_lock);
	}

	/* Wait for any parity re-writes to stop... */
	while (raidPtr->parity_rewrite_in_progress) {
		printf("raid%d: Waiting for parity re-write to exit...\n",
		       raidPtr->raidid);
		rf_wait_cond2(raidPtr->parity_rewrite_cv, raidPtr->rad_lock);
	}
	rf_unlock_mutex2(raidPtr->rad_lock);

	/* Wait for any reconstruction to stop... */
	rf_lock_mutex2(raidPtr->mutex);
	while (raidPtr->reconInProgress) {
		printf("raid%d: Waiting for reconstruction to stop...\n",
		       raidPtr->raidid);
		rf_wait_cond2(raidPtr->waitForReconCond, raidPtr->mutex);
	}
	rf_unlock_mutex2(raidPtr->mutex);

	raidPtr->valid = 0;

	if (raidPtr->parity_map != NULL)
		rf_paritymap_detach(raidPtr);

	rf_update_component_labels(raidPtr, RF_FINAL_COMPONENT_UPDATE);

	rf_UnconfigureVnodes(raidPtr);

	rf_FreeReservedBuffers(raidPtr);

	rf_ShutdownList(&raidPtr->shutdownList);

	rf_destroy_mutex_cond(raidPtr);

	rf_UnconfigureArray();

	return (0);
}


#define DO_INIT_CONFIGURE(f) { \
	rc = f (&globalShutdown); \
	if (rc) { \
		RF_ERRORMSG2("RAIDFRAME: failed %s with %d\n", RF_STRING(f), rc); \
		rf_ShutdownList(&globalShutdown); \
		configureCount--; \
		rf_unlock_mutex2(configureMutex); \
		rf_destroy_mutex2(rf_printf_mutex); \
		return(rc); \
	} \
}

#define DO_RAID_FAIL() { \
	rf_UnconfigureVnodes(raidPtr); \
	rf_FreeReservedBuffers(raidPtr); \
	rf_ShutdownList(&raidPtr->shutdownList); \
	rf_UnconfigureArray(); \
	rf_destroy_mutex_cond(raidPtr); \
}

#define DO_RAID_INIT_CONFIGURE(f) { \
	rc = f (&raidPtr->shutdownList, raidPtr, cfgPtr); \
	if (rc) { \
		RF_ERRORMSG2("RAIDFRAME: failed %s with %d\n", RF_STRING(f), rc); \
		DO_RAID_FAIL(); \
		return(rc); \
	} \
}

int
rf_Configure(RF_Raid_t *raidPtr, RF_Config_t *cfgPtr, RF_AutoConfig_t *ac)
{
	RF_RowCol_t col;
	int rc;
	bool swapped = false;
	bool first = true;

	rf_lock_mutex2(configureMutex);
	configureCount++;
	if (isconfigged == 0) {
		rf_init_mutex2(rf_printf_mutex, IPL_VM);

		/* initialize globals */
		DO_INIT_CONFIGURE(rf_ConfigureAllocList);

		/*
	         * Yes, this does make debugging general to the whole
	         * system instead of being array specific. Bummer, drag.
		 */
		rf_ConfigureDebug(cfgPtr);
		DO_INIT_CONFIGURE(rf_ConfigureDebugMem);
#if RF_ACC_TRACE > 0
		DO_INIT_CONFIGURE(rf_ConfigureAccessTrace);
#endif
		DO_INIT_CONFIGURE(rf_ConfigureNWayXor);
		DO_INIT_CONFIGURE(rf_ConfigureDAGFuncs);
		isconfigged = 1;
	}
	rf_unlock_mutex2(configureMutex);

	rf_alloc_mutex_cond(raidPtr);

	/* set up the cleanup list.  Do this after ConfigureDebug so that
	 * value of memDebug will be set */

	rf_MakeAllocList(raidPtr->cleanupList);
	if (raidPtr->cleanupList == NULL) {
		DO_RAID_FAIL();
		return (ENOMEM);
	}
	rf_ShutdownCreate(&raidPtr->shutdownList,
			  (void (*) (void *)) rf_FreeAllocList,
			  raidPtr->cleanupList);

	KASSERT(cfgPtr->numCol < RF_MAXCOL);
	KASSERT(cfgPtr->numCol >= 0);
	KASSERT(cfgPtr->numSpare < RF_MAXSPARE);
	KASSERT(cfgPtr->numSpare >= 0);

	raidPtr->numCol = cfgPtr->numCol;
	raidPtr->numSpare = cfgPtr->numSpare;
	raidPtr->maxQueue = cfgPtr->numSpare;

	raidPtr->status = rf_rs_optimal;
	raidPtr->reconControl = NULL;

	raidPtr->nAccOutstanding = 0;
	raidPtr->waitShutdown = 0;

	raidPtr->iobuf = NULL;
	raidPtr->iobuf_count = 0;
	raidPtr->numReservedBuffers = 0;
	raidPtr->stripebuf = NULL;
	raidPtr->stripebuf_count = 0;
	raidPtr->numReservedStripeBuffers = 0;

	if (ac!=NULL) {
		/* We have an AutoConfig structure..  Don't do the
		   normal disk configuration... call the auto config
		   stuff */
		rf_AutoConfigureDisks(raidPtr, cfgPtr, ac);
	} else {
		DO_RAID_INIT_CONFIGURE(rf_ConfigureDisks);
		DO_RAID_INIT_CONFIGURE(rf_ConfigureSpareDisks);
	}
	/* do this after ConfigureDisks & ConfigureSpareDisks to be sure dev
	 * no. is set */
	DO_RAID_INIT_CONFIGURE(rf_ConfigureDiskQueues);

	DO_RAID_INIT_CONFIGURE(rf_ConfigureLayout);

	/*
	 * Compute openings and buffer counts before sizing per-array pools.
	 * All per-array pools below are primed (xmin) for raidPtr->openings
	 * concurrent MAXPHYS I/Os (plus 1 stripe for parity rewrite) because
	 * rf_RaidIOThread and rf_EngineThread both admit/advance up to
	 * raidPtr->openings I/Os and drain completions on the same thread;
	 * if an admitted I/O blocked in pool_get(PR_WAITOK), in-flight I/Os
	 * could not complete to return items to the pool.
	 */
	rf_ComputeOpenings(raidPtr);
	rf_GrowAllocListPool(raidPtr);

	DO_RAID_INIT_CONFIGURE(rf_ConfigureMapModule);
	DO_RAID_INIT_CONFIGURE(rf_ConfigureReconEvent);
	DO_RAID_INIT_CONFIGURE(rf_ConfigureCallback);
	DO_RAID_INIT_CONFIGURE(rf_ConfigureRDFreeList);
	DO_RAID_INIT_CONFIGURE(rf_ConfigureStripeLockFreeList);
	DO_RAID_INIT_CONFIGURE(rf_ConfigureMCPair);
	DO_RAID_INIT_CONFIGURE(rf_ConfigureDAGs);
	DO_RAID_INIT_CONFIGURE(rf_ConfigureReconstruction);
	DO_RAID_INIT_CONFIGURE(rf_ConfigureDiskQueueSystem);
	DO_RAID_INIT_CONFIGURE(rf_ConfigurePSStatus);
	
	DO_RAID_INIT_CONFIGURE(rf_ConfigureEngine);
	DO_RAID_INIT_CONFIGURE(rf_ConfigureStripeLocks);

	/* Initialize per-RAID PSS bits */
	rf_InitPSStatus(raidPtr);

#if RF_INCLUDE_CHAINDECLUSTER > 0
	for (col = 0; col < raidPtr->numCol; col++) {
		/*
		 * XXX better distribution
		 */
		raidPtr->hist_diskreq[col] = 0;
	}
#endif
	raidPtr->numNewFailures = 0;
	raidPtr->parity_rewrite_in_progress = 0;
	raidPtr->changing_components = 0;
	raidPtr->recon_in_progress = 0;

	raidPtr->maxOutstanding = raidPtr->maxQueueDepth;

	/* autoconfigure and root_partition will actually get filled in
	   after the config is done */
	raidPtr->autoconfigure = 0;
	raidPtr->root_partition = 0;
	raidPtr->last_unit = raidPtr->raidid;
	raidPtr->config_order = 0;

	if (rf_keepAccTotals) {
		raidPtr->keep_acc_totals = 1;
	}

	/*
	 * Pre-allocate reserved I/O and stripe buffers (if needed by this
	 * RAID layout).
	 */
	rc = rf_AllocReservedBuffers(raidPtr);
	if (rc) {
		printf("raid%d: Unable to allocate reserved buffers.\n",
		       raidPtr->raidid);
		DO_RAID_FAIL();
		return(rc);
	}

	/* Set up parity map stuff, if applicable. */
#ifndef RF_NO_PARITY_MAP
	rf_paritymap_attach(raidPtr, cfgPtr->force);
#endif

	raidPtr->valid = 1;

	printf("raid%d: %s\n", raidPtr->raidid,
	       raidPtr->Layout.map->configName);
	printf("raid%d: Components:", raidPtr->raidid);

	for (col = 0; col < raidPtr->numCol; col++) {
		RF_ComponentLabel_t *clabel;
		bool compswapped;

		printf(" %s", raidPtr->Disks[col].devname);
		if (RF_DEAD_DISK(raidPtr->Disks[col].status)) {
			printf("[**FAILED**]");
		}
		clabel = raidget_component_label(raidPtr, col);
		compswapped = clabel->version ==
			      bswap32(RF_COMPONENT_LABEL_VERSION);
		if (first)
			swapped = compswapped;
		else if (swapped != compswapped)
			printf("raid%d: Component %d has different endian "
			       "than first component.", raidPtr->raidid, col);
	}
	printf("\n");
	printf("raid%d: Total Sectors: %" PRIu64 " (%" PRIu64 " MB)\n",
	       raidPtr->raidid,
	       raidPtr->totalSectors,
	       (raidPtr->totalSectors / 1024 *
				(1 << raidPtr->logBytesPerSector) / 1024));
	if (swapped)
		printf("raid%d: Using swapped-endian component labels.\n",
		    raidPtr->raidid);

	return (0);
}


/*

  Routines to compute top-level openings and allocate/free the
  per-set I/O and full-stripe buffers used by parity RAID layouts.

 */

static void
rf_ComputeOpenings(RF_Raid_t *raidPtr)
{
	size_t su_bytes, stripe_data_bytes, pool_bytes_per_stripe;
	uint64_t bytes_per_opening, mem_budget, mem_openings;
	u_int max_stripes_per_bp, max_openings, c, d, p;
	bool need_reserved_bufs;

	char pbuf[9];

	c = raidPtr->numCol;
	d = raidPtr->Layout.numDataCol;
	p = raidPtr->Layout.numParityCol;
	su_bytes = (size_t)raidPtr->Layout.sectorsPerStripeUnit <<
	    raidPtr->logBytesPerSector;
	stripe_data_bytes = (size_t)raidPtr->Layout.dataSectorsPerStripe <<
	    raidPtr->logBytesPerSector;
	max_stripes_per_bp = howmany(MAXPHYS, stripe_data_bytes) + 1;

	/*
	 * Non-parity layouts (RAID 0 has numParityCol == 0; RAID 1 and
	 * mirror declustering have numDataCol == 1) never allocate
	 * intermediate iobuf or stripebuf buffers.
	 */
	if (p == 0) {
		max_openings = uimin(raidPtr->maxQueueDepth * c,
		    RF_MAX_QUEUE_DEPTH);
		need_reserved_bufs = false;
	} else if (d == 1) {
		max_openings = uimin(raidPtr->maxQueueDepth * uimax(1, c / 2),
		    RF_MAX_QUEUE_DEPTH);
		need_reserved_bufs = false;
	} else {
		max_openings = raidPtr->maxQueueDepth;
		need_reserved_bufs = true;
	}

	/*
	 * Approximate per-stripe pool item footprint (DAG nodes, DQDs,
	 * buf_t's, PDAs, ASMs, etc.) plus reserved data buffers (1 iobuf
	 * per column and 1 full-stripe buffer per stripe for parity layouts).
	 * Bound total pre-allocated memory per array to 1% of physmem,
	 * with a floor of RAIDOUTSTANDING openings.
	 */
	pool_bytes_per_stripe =
	    (2 * d + 3 * p + 3) * sizeof(RF_DagNode_t) +
	    2 * c * (sizeof(RF_DiskQueueData_t) + sizeof(buf_t) +
	    sizeof(RF_PhysDiskAddr_t)) +
	    3 * (sizeof(RF_AccessStripeMapHeader_t) +
	    sizeof(RF_AccessStripeMap_t)) +
	    ((2 * d + p > 6) ? 6 * RF_DAGPCACHE_SIZE : 0) +
	    sizeof(RF_DagHeader_t) + sizeof(RF_AllocListElem_t) +
	    sizeof(RF_StripeLockDesc_t) + sizeof(RF_FailedStripe_t);

	bytes_per_opening = (uint64_t)max_stripes_per_bp *
	    pool_bytes_per_stripe + sizeof(RF_RaidAccessDesc_t) +
	    2 * d * (sizeof(RF_DagHeader_t) + sizeof(RF_AllocListElem_t) +
	    sizeof(RF_AccessStripeMapHeader_t) + sizeof(RF_AccessStripeMap_t));
	if (need_reserved_bufs) {
		bytes_per_opening += (uint64_t)2 * max_stripes_per_bp *
		    c * su_bytes;
	}

	mem_budget = ctob((uint64_t)physmem) / 100;
	mem_openings = mem_budget / uimax(1, bytes_per_opening);
	if (mem_openings < RAIDOUTSTANDING)
		mem_openings = RAIDOUTSTANDING;
	if (mem_openings > max_openings)
		raidPtr->openings = max_openings;
	else
		raidPtr->openings = (int)mem_openings;

	if (need_reserved_bufs) {
		raidPtr->numReservedBuffers = raidPtr->openings *
		    max_stripes_per_bp * c;
		raidPtr->numReservedStripeBuffers = raidPtr->openings *
		    max_stripes_per_bp;
	}

	aprint_debug("raid%d: max_stripes_per_bp %u, openings %d, "
	    "maxQueueDepth %d, iobufs %d (%zuB), stripebufs %d (%zuB), "
	    "bytes_per_opening %" PRIu64 "\n",
	    raidPtr->raidid, max_stripes_per_bp, raidPtr->openings,
	    raidPtr->maxQueueDepth, raidPtr->numReservedBuffers, su_bytes,
	    raidPtr->numReservedStripeBuffers, c * su_bytes,
	    bytes_per_opening);
	humanize_number(pbuf, sizeof(pbuf),
	    (uint64_t)raidPtr->openings * bytes_per_opening +
	    pool_bytes_per_stripe, "B", 1024);
	aprint_verbose("raid%d: %d openings, %s reserved\n",
	    raidPtr->raidid, raidPtr->openings, pbuf);
}

static int
rf_AllocReservedBuffers(RF_Raid_t *raidPtr)
{
	void *tmpbuf;
	RF_VoidPointerListElem_t *vple;
	size_t su_bytes;
	int i;

	if (raidPtr->numReservedBuffers == 0 &&
	    raidPtr->numReservedStripeBuffers == 0)
		return (0);

	su_bytes = (size_t)raidPtr->Layout.sectorsPerStripeUnit <<
	    raidPtr->logBytesPerSector;
	for (i = 0; i < raidPtr->numReservedBuffers; i++) {
		tmpbuf = malloc(su_bytes, M_RAIDFRAME, M_WAITOK);
		if (tmpbuf) {
			vple = rf_AllocVPListElem(raidPtr);
			vple->p = tmpbuf;
			vple->next = raidPtr->iobuf;
			raidPtr->iobuf = vple;
			raidPtr->iobuf_count++;
		} else {
			printf("raid%d: failed to allocate reserved buffer!\n",
			       raidPtr->raidid);
			return 1;
		}
	}

	for (i = 0; i < raidPtr->numReservedStripeBuffers; i++) {
		tmpbuf = malloc(raidPtr->numCol * su_bytes,
		    M_RAIDFRAME, M_WAITOK);
		if (tmpbuf) {
			vple = rf_AllocVPListElem(raidPtr);
			vple->p = tmpbuf;
			vple->next = raidPtr->stripebuf;
			raidPtr->stripebuf = vple;
			raidPtr->stripebuf_count++;
		} else {
			printf("raid%d: failed to allocate reserved stripe buffer!\n",
			       raidPtr->raidid);
			return 1;
		}
	}

	return (0);
}

static void
rf_FreeReservedBuffers(RF_Raid_t *raidPtr)
{
	RF_VoidPointerListElem_t *tmp;

	/* Free the reserved I/O buffers */
	while (raidPtr->iobuf != NULL) {
		tmp = raidPtr->iobuf;
		raidPtr->iobuf = raidPtr->iobuf->next;
		free(tmp->p, M_RAIDFRAME);
		rf_FreeVPListElem(raidPtr,tmp);
	}

	/* Free the reserved stripe buffers */
	while (raidPtr->stripebuf != NULL) {
		tmp = raidPtr->stripebuf;
		raidPtr->stripebuf = raidPtr->stripebuf->next;
		free(tmp->p, M_RAIDFRAME);
		rf_FreeVPListElem(raidPtr, tmp);
	}
}


static void
rf_ShutdownRDFreeList(void *arg)
{
	RF_Raid_t *raidPtr;

	raidPtr = (RF_Raid_t *) arg;
	
	pool_destroy(&raidPtr->pools.rad);
}

static int
rf_ConfigureRDFreeList(RF_ShutdownList_t **listp, RF_Raid_t *raidPtr,
		       RF_Config_t *cfgPtr)
{
	size_t min_rad, max_rad;

	/* 1 access desc per admitted opening + 1 for parity rewrite. */
	min_rad = raidPtr->openings + 1;
	max_rad = 2 * min_rad;

	rf_pool_init(raidPtr, raidPtr->poolNames.rad, &raidPtr->pools.rad, sizeof(RF_RaidAccessDesc_t),
		     "rad", min_rad, max_rad);
	rf_ShutdownCreate(listp, rf_ShutdownRDFreeList, raidPtr);
	return (0);
}

RF_RaidAccessDesc_t *
rf_AllocRaidAccDesc(RF_Raid_t *raidPtr, RF_IoType_t type,
		    RF_RaidAddr_t raidAddress, RF_SectorCount_t numBlocks,
		    void *bufPtr, void *bp, RF_RaidAccessFlags_t flags,
		    const RF_AccessState_t *states)
{
	RF_RaidAccessDesc_t *desc;

	desc = pool_get(&raidPtr->pools.rad, PR_WAITOK);

	rf_lock_mutex2(raidPtr->rad_lock);
	if (raidPtr->waitShutdown) {
		/*
	         * Actually, we're shutting the array down. Free the desc
	         * and return NULL.
	         */

		rf_unlock_mutex2(raidPtr->rad_lock);
		pool_put(&raidPtr->pools.rad, desc);
		return (NULL);
	}
	raidPtr->nAccOutstanding++;

	rf_unlock_mutex2(raidPtr->rad_lock);

	desc->raidPtr = (void *) raidPtr;
	desc->type = type;
	desc->raidAddress = raidAddress;
	desc->numBlocks = numBlocks;
	desc->bufPtr = bufPtr;
	desc->bp = bp;
	desc->flags = flags;
	desc->states = states;
	desc->state = 0;
	desc->dagList = NULL;

	desc->status = 0;
	desc->numRetries = 0;
#if RF_ACC_TRACE > 0
	memset(&desc->tracerec, 0, sizeof(desc->tracerec));
#endif
	desc->callbackFunc = NULL;
	desc->callbackArg = NULL;
	desc->next = NULL;
	desc->iobufs = NULL;
	desc->stripebufs = NULL;

	return (desc);
}

void
rf_FreeRaidAccDesc(RF_RaidAccessDesc_t *desc)
{
	RF_Raid_t *raidPtr = desc->raidPtr;
	RF_DagList_t *dagList, *temp;
	RF_VoidPointerListElem_t *tmp;

	RF_ASSERT(desc);

	/* Cleanup the dagList(s) */
	dagList = desc->dagList;
	while(dagList != NULL) {
		temp = dagList;
		dagList = dagList->next;
		rf_FreeDAGList(raidPtr, temp);
	}

	while (desc->iobufs) {
		tmp = desc->iobufs;
		desc->iobufs = desc->iobufs->next;
		rf_FreeIOBuffer(raidPtr, tmp);
	}

	while (desc->stripebufs) {
		tmp = desc->stripebufs;
		desc->stripebufs = desc->stripebufs->next;
		rf_FreeStripeBuffer(raidPtr, tmp);
	}

	pool_put(&raidPtr->pools.rad, desc);
	rf_lock_mutex2(raidPtr->rad_lock);
	raidPtr->nAccOutstanding--;
	if (raidPtr->waitShutdown) {
		rf_signal_cond2(raidPtr->outstandingCond);
	}
	rf_unlock_mutex2(raidPtr->rad_lock);
}
/*********************************************************************
 * Main routine for performing an access.
 * Accesses are retried until a DAG can not be selected.  This occurs
 * when either the DAG library is incomplete or there are too many
 * failures in a parity group.
 *
 * type should be read or write.  bp_in is a buf pointer.  void *to
 * facilitate ignoring it outside the kernel
 ********************************************************************/
int
rf_DoAccess(RF_Raid_t * raidPtr, RF_IoType_t type, RF_RaidAddr_t raidAddress, RF_SectorCount_t numBlocks,
	    void *bufPtr, struct buf *bp, RF_RaidAccessFlags_t flags)
{
	RF_RaidAccessDesc_t *desc;
	void *lbufPtr = bufPtr;

	raidAddress += rf_raidSectorOffset;

#if RF_ACCESS_DEBUG
	if (rf_accessDebug) {

		printf("logBytes is: %d %d %d\n", raidPtr->raidid,
		    raidPtr->logBytesPerSector,
		    (int) rf_RaidAddressToByte(raidPtr, numBlocks));
		printf("raid%d: %s raidAddr %d (stripeid %d-%d) numBlocks %d (%d bytes) buf 0x%lx\n", raidPtr->raidid,
		    (type == RF_IO_TYPE_READ) ? "READ" : "WRITE", (int) raidAddress,
		    (int) rf_RaidAddressToStripeID(&raidPtr->Layout, raidAddress),
		    (int) rf_RaidAddressToStripeID(&raidPtr->Layout, raidAddress + numBlocks - 1),
		    (int) numBlocks,
		    (int) rf_RaidAddressToByte(raidPtr, numBlocks),
		    (long) bufPtr);
	}
#endif

	desc = rf_AllocRaidAccDesc(raidPtr, type, raidAddress,
	    numBlocks, lbufPtr, bp, flags, raidPtr->Layout.map->states);

	if (desc == NULL) {
		return (ENOMEM);
	}
#if RF_ACC_TRACE > 0
	RF_ETIMER_START(desc->tracerec.tot_timer);
#endif

	if (raidPtr->parity_map != NULL && 
	    type == RF_IO_TYPE_WRITE)
		rf_paritymap_begin(raidPtr->parity_map, raidAddress, 
		    numBlocks);

	rf_ContinueRaidAccess(desc);

	return (0);
}
#if 0
/* force the array into reconfigured mode without doing reconstruction */
int
rf_SetReconfiguredMode(RF_Raid_t *raidPtr, int col)
{
	if (!(raidPtr->Layout.map->flags & RF_DISTRIBUTE_SPARE)) {
		printf("Can't set reconfigured mode in dedicated-spare array\n");
		RF_PANIC();
	}
	rf_lock_mutex2(raidPtr->mutex);
	raidPtr->numFailures++;
	raidPtr->Disks[col].status = rf_ds_dist_spared;
	raidPtr->status = rf_rs_reconfigured;
	rf_update_component_labels(raidPtr, RF_NORMAL_COMPONENT_UPDATE);
	/* install spare table only if declustering + distributed sparing
	 * architecture. */
	if (raidPtr->Layout.map->flags & RF_BD_DECLUSTERED)
		rf_InstallSpareTable(raidPtr, col);
	rf_unlock_mutex2(raidPtr->mutex);
	return (0);
}
#endif

int
rf_FailDisk(RF_Raid_t *raidPtr, int fcol, int initRecon)
{

	/* need to suspend IO's here -- if there are DAGs in flight
	   and we pull the rug out from under ci_vp, Bad Things
	   can happen.  */

	rf_SuspendNewRequestsAndWait(raidPtr);

	rf_lock_mutex2(raidPtr->mutex);
	if (raidPtr->Disks[fcol].status != rf_ds_failed) {
		/* must be failing something that is valid, or else it's
		   already marked as failed (in which case we don't
		   want to mark it failed again!) */
		raidPtr->numFailures++;
		raidPtr->Disks[fcol].status = rf_ds_failed;
		raidPtr->status = rf_rs_degraded;
	}
	rf_unlock_mutex2(raidPtr->mutex);

	rf_update_component_labels(raidPtr, RF_NORMAL_COMPONENT_UPDATE);

	/* Close the component, so that it's not "locked" if someone
	   else want's to use it! */

	rf_close_component(raidPtr, raidPtr->raid_cinfo[fcol].ci_vp,
			   raidPtr->Disks[fcol].auto_configured);

	rf_lock_mutex2(raidPtr->mutex);
	raidPtr->raid_cinfo[fcol].ci_vp = NULL;

	/* Need to mark the component as not being auto_configured
	   (in case it was previously). */

	raidPtr->Disks[fcol].auto_configured = 0;
	rf_unlock_mutex2(raidPtr->mutex);
	/* now we can allow IO to continue -- we'll be suspending it
	   again in rf_ReconstructFailedDisk() if we have to.. */

	rf_ResumeNewRequests(raidPtr);

	if (initRecon)
		rf_ReconstructFailedDisk(raidPtr, fcol);
	return (0);
}
/* releases a thread that is waiting for the array to become quiesced.
 * access_suspend_mutex should be locked upon calling this
 */
void
rf_SignalQuiescenceLock(RF_Raid_t *raidPtr)
{
#if RF_DEBUG_QUIESCE
	if (rf_quiesceDebug) {
		printf("raid%d: Signalling quiescence lock\n",
		       raidPtr->raidid);
	}
#endif
	raidPtr->access_suspend_release = 1;

	if (raidPtr->waiting_for_quiescence) {
		SIGNAL_QUIESCENT_COND(raidPtr);
	}
}
/* suspends all new requests to the array.  No effect on accesses that are in flight.  */
int
rf_SuspendNewRequestsAndWait(RF_Raid_t *raidPtr)
{
#if RF_DEBUG_QUIESCE
	if (rf_quiesceDebug)
		printf("raid%d: Suspending new reqs\n", raidPtr->raidid);
#endif
	rf_lock_mutex2(raidPtr->access_suspend_mutex);
	raidPtr->accesses_suspended++;
	raidPtr->waiting_for_quiescence = (raidPtr->accs_in_flight == 0) ? 0 : 1;

	if (raidPtr->waiting_for_quiescence) {
		raidPtr->access_suspend_release = 0;
		while (!raidPtr->access_suspend_release) {
#if RF_DEBUG_QUIESCE
			printf("raid%d: Suspending: Waiting for Quiescence\n",
			       raidPtr->raidid);
#endif
			WAIT_FOR_QUIESCENCE(raidPtr);
			raidPtr->waiting_for_quiescence = 0;
		}
	}
#if RF_DEBUG_QUIESCE
	printf("raid%d: Quiescence reached..\n", raidPtr->raidid);
#endif

	rf_unlock_mutex2(raidPtr->access_suspend_mutex);
	return (raidPtr->waiting_for_quiescence);
}
/* wake up everyone waiting for quiescence to be released */
void
rf_ResumeNewRequests(RF_Raid_t *raidPtr)
{
	RF_CallbackFuncDesc_t *t, *cb;

#if RF_DEBUG_QUIESCE
	if (rf_quiesceDebug)
		printf("raid%d: Resuming new requests\n", raidPtr->raidid);
#endif

	rf_lock_mutex2(raidPtr->access_suspend_mutex);
	raidPtr->accesses_suspended--;
	if (raidPtr->accesses_suspended == 0)
		cb = raidPtr->quiesce_wait_list;
	else
		cb = NULL;
	raidPtr->quiesce_wait_list = NULL;
	rf_unlock_mutex2(raidPtr->access_suspend_mutex);

	while (cb) {
		t = cb;
		cb = cb->next;
		(t->callbackFunc) (t->callbackArg);
		rf_FreeCallbackFuncDesc(raidPtr, t);
	}
}
/*****************************************************************************************
 *
 * debug routines
 *
 ****************************************************************************************/

static void
set_debug_option(char *name, long val)
{
	RF_DebugName_t *p;

	for (p = rf_debugNames; p->name; p++) {
		if (!strcmp(p->name, name)) {
			*(p->ptr) = val;
			printf("[Set debug variable %s to %ld]\n", name, val);
			return;
		}
	}
	RF_ERRORMSG1("Unknown debug string \"%s\"\n", name);
}


/* would like to use sscanf here, but apparently not available in kernel */
/*ARGSUSED*/
static void
rf_ConfigureDebug(RF_Config_t *cfgPtr)
{
	char   *val_p, *name_p, *white_p;
	long    val;
	int     i;

	rf_ResetDebugOptions();
	for (i = 0; i < RF_MAXDBGV && cfgPtr->debugVars[i][0]; i++) {
		name_p = rf_find_non_white(&cfgPtr->debugVars[i][0]);
		white_p = rf_find_white(name_p);	/* skip to start of 2nd
							 * word */
		val_p = rf_find_non_white(white_p);
		if (*val_p == '0' && *(val_p + 1) == 'x')
			val = rf_htoi(val_p + 2);
		else
			val = rf_atoi(val_p);
		*white_p = '\0';
		set_debug_option(name_p, val);
	}
}

void
rf_print_panic_message(int line, const char *file)
{
	kern_assert("raidframe error at line %d file %s", line, file);
}

#ifdef RAID_DIAGNOSTIC
void
rf_print_assert_panic_message(int line,	const char *file, const char *condition)
{
	kern_assert("raidframe error at line %d file %s (failed asserting %s)\n",
	    line, file, condition);
}
#endif

void
rf_print_unable_to_init_mutex(const char *file, int line, int rc)
{
	RF_ERRORMSG3("Unable to init mutex file %s line %d rc=%d\n",
		     file, line, rc);
}

void
rf_print_unable_to_add_shutdown(const char *file, int line, int rc)
{
	RF_ERRORMSG3("Unable to add to shutdown list file %s line %d rc=%d\n",
		     file, line, rc);
}

static void
rf_alloc_mutex_cond(RF_Raid_t *raidPtr)
{

	rf_init_mutex2(raidPtr->mutex, IPL_VM);

	rf_init_cond2(raidPtr->outstandingCond, "rfocond");
	rf_init_cond2(raidPtr->parity_rewrite_cv, "rfprwshutdown");
	rf_init_mutex2(raidPtr->rad_lock, IPL_VM);

	rf_init_mutex2(raidPtr->access_suspend_mutex, IPL_VM);
	rf_init_cond2(raidPtr->access_suspend_cv, "rfquiesce");

	rf_init_cond2(raidPtr->waitForReconCond, "rfrcnw");

	rf_init_cond2(raidPtr->changing_components_cv, "rfcc");
}

static void
rf_destroy_mutex_cond(RF_Raid_t *raidPtr)
{

	rf_destroy_cond2(raidPtr->waitForReconCond);
	rf_destroy_cond2(raidPtr->changing_components_cv);

	rf_destroy_mutex2(raidPtr->access_suspend_mutex);
	rf_destroy_cond2(raidPtr->access_suspend_cv);

	rf_destroy_cond2(raidPtr->parity_rewrite_cv);
	rf_destroy_cond2(raidPtr->outstandingCond);
	rf_destroy_mutex2(raidPtr->rad_lock);

	rf_destroy_mutex2(raidPtr->mutex);
}
