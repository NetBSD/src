/*	$NetBSD: rf_callback.c,v 1.24 2019/10/10 03:43:59 christos Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: rf_callback.c,v 1.24 2019/10/10 03:43:59 christos Exp $");

#include <dev/raidframe/raidframevar.h>
#include <sys/pool.h>

#include "rf_archs.h"
#include "rf_threadstuff.h"
#include "rf_callback.h"
#include "rf_debugMem.h"
#include "rf_general.h"
#include "rf_shutdown.h"
#include "rf_netbsd.h"

#define RF_MAX_FREE_CALLBACK 64
#define RF_MIN_FREE_CALLBACK 32

static void rf_ShutdownCallback(void *);
static void
rf_ShutdownCallback(void *ignored)
{
	pool_destroy(&rf_pools.callbackf);
	pool_destroy(&rf_pools.callbackv);
}

int
rf_ConfigureCallback(RF_ShutdownList_t **listp)
{

	rf_pool_init(&rf_pools.callbackf, sizeof(RF_CallbackFuncDesc_t),
		     "rf_callbackfpl", RF_MIN_FREE_CALLBACK, RF_MAX_FREE_CALLBACK);
	rf_pool_init(&rf_pools.callbackv, sizeof(RF_CallbackValueDesc_t),
		     "rf_callbackvpl", RF_MIN_FREE_CALLBACK, RF_MAX_FREE_CALLBACK);
	rf_ShutdownCreate(listp, rf_ShutdownCallback, NULL);

	return (0);
}

RF_CallbackFuncDesc_t *
rf_AllocCallbackFuncDesc(void)
{
	return pool_get(&rf_pools.callbackf, PR_WAITOK);
}

void
rf_FreeCallbackFuncDesc(RF_CallbackFuncDesc_t *p)
{
	pool_put(&rf_pools.callbackf, p);
}

RF_CallbackValueDesc_t *
rf_AllocCallbackValueDesc(void)
{
	return pool_get(&rf_pools.callbackv, PR_WAITOK);
}

void
rf_FreeCallbackValueDesc(RF_CallbackValueDesc_t *p)
{
	pool_put(&rf_pools.callbackv, p);
}
