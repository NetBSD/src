/*	$NetBSD: rf_callback.c,v 1.26 2026/10/24 00:20:23 tls Exp $	*/
/*
 * Copyright (c) 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Jim Zelenka
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

/*****************************************************************************
 *
 * callback.c -- code to manipulate callback descriptor
 *
 ****************************************************************************/


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rf_callback.c,v 1.26 2026/10/24 00:20:23 tls Exp $");

#include <dev/raidframe/raidframevar.h>
#include <sys/pool.h>

#include "rf_archs.h"
#include "rf_threadstuff.h"
#include "rf_callback.h"
#include "rf_debugMem.h"
#include "rf_general.h"
#include "rf_shutdown.h"
#include "rf_netbsd.h"
#include "rf_raid.h"

static void rf_ShutdownCallback(void *);
static void
rf_ShutdownCallback(void *arg)
{
	RF_Raid_t *raidPtr;

	raidPtr = (RF_Raid_t *) arg;
	
	pool_destroy(&raidPtr->pools.callbackf);
	pool_destroy(&raidPtr->pools.callbackv);
}

int
rf_ConfigureCallback(RF_ShutdownList_t **listp, RF_Raid_t *raidPtr,
		     RF_Config_t *cfgPtr)
{
	size_t stripe_data_bytes, min_cbf, max_cbf, min_cbv, max_cbv;
	u_int max_stripes_per_bp, c;

	c = raidPtr->numCol;
	stripe_data_bytes = (size_t)raidPtr->Layout.dataSectorsPerStripe <<
	    raidPtr->logBytesPerSector;
	max_stripes_per_bp = howmany(MAXPHYS, stripe_data_bytes) + 1;

	/* Up to 2 callbacks (stripe lock + forced recon) per active stripe. */
	min_cbf = 2 * (raidPtr->openings * max_stripes_per_bp + 1);
	max_cbf = 2 * min_cbf;

	/* Up to 3 recon wait lists (head-sep, blocked, buffer) per disk. */
	min_cbv = (raidPtr->Layout.map->SubmitReconBuffer == NULL) ?
	    0 : 3 * (c - 1);
	max_cbv = 2 * min_cbv;

	rf_pool_init(raidPtr, raidPtr->poolNames.callbackf, &raidPtr->pools.callbackf, sizeof(RF_CallbackFuncDesc_t),
		     "callbackf", min_cbf, max_cbf);
	rf_pool_init(raidPtr, raidPtr->poolNames.callbackv, &raidPtr->pools.callbackv, sizeof(RF_CallbackValueDesc_t),
		     "callbackv", min_cbv, max_cbv);
	rf_ShutdownCreate(listp, rf_ShutdownCallback, raidPtr);

	return (0);
}

RF_CallbackFuncDesc_t *
rf_AllocCallbackFuncDesc(RF_Raid_t *raidPtr)
{
	return pool_get(&raidPtr->pools.callbackf, PR_WAITOK);
}

void
rf_FreeCallbackFuncDesc(RF_Raid_t *raidPtr, RF_CallbackFuncDesc_t *p)
{
	pool_put(&raidPtr->pools.callbackf, p);
}

RF_CallbackValueDesc_t *
rf_AllocCallbackValueDesc(RF_Raid_t *raidPtr)
{
	return pool_get(&raidPtr->pools.callbackv, PR_WAITOK);
}

void
rf_FreeCallbackValueDesc(RF_Raid_t *raidPtr, RF_CallbackValueDesc_t *p)
{
	pool_put(&raidPtr->pools.callbackv, p);
}
