/*	$NetBSD: rf_mcpair.c,v 1.10 2003/12/29 04:33:31 oster Exp $	*/
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

/* rf_mcpair.c
 * an mcpair is a structure containing a mutex and a condition variable.
 * it's used to block the current thread until some event occurs.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rf_mcpair.c,v 1.10 2003/12/29 04:33:31 oster Exp $");

#include <dev/raidframe/raidframevar.h>

#include "rf_archs.h"
#include "rf_threadstuff.h"
#include "rf_mcpair.h"
#include "rf_debugMem.h"
#include "rf_general.h"
#include "rf_shutdown.h"

#include <sys/pool.h>
#include <sys/proc.h>

static struct pool rf_mcpair_pool;
#define RF_MAX_FREE_MCPAIR 128
#define RF_MCPAIR_INC       16
#define RF_MCPAIR_INITIAL   24

static void rf_ShutdownMCPair(void *);

static void 
rf_ShutdownMCPair(ignored)
	void   *ignored;
{
	pool_destroy(&rf_mcpair_pool);
}

int 
rf_ConfigureMCPair(listp)
	RF_ShutdownList_t **listp;
{
	int     rc;

	pool_init(&rf_mcpair_pool, sizeof(RF_MCPair_t), 0, 0, 0,
		  "rf_mcpair_pl", NULL);
	pool_sethiwat(&rf_mcpair_pool, RF_MAX_FREE_MCPAIR);
	pool_prime(&rf_mcpair_pool, RF_MCPAIR_INITIAL);

	rc = rf_ShutdownCreate(listp, rf_ShutdownMCPair, NULL);
	if (rc) {
		rf_print_unable_to_add_shutdown(__FILE__, __LINE__, rc);
		rf_ShutdownMCPair(NULL);
		return (rc);
	}

	return (0);
}

RF_MCPair_t *
rf_AllocMCPair()
{
	RF_MCPair_t *t;

	t = pool_get(&rf_mcpair_pool, PR_WAITOK);
	if (t) {
		simple_lock_init(&t->mutex);
		t->cond = 0;
		t->flag = 0;
		t->next = NULL;
	}
	return (t);
}

void 
rf_FreeMCPair(t)
	RF_MCPair_t *t;
{
	pool_put(&rf_mcpair_pool, t);
}
/* the callback function used to wake you up when you use an mcpair to wait for something */
void 
rf_MCPairWakeupFunc(mcpair)
	RF_MCPair_t *mcpair;
{
	RF_LOCK_MUTEX(mcpair->mutex);
	mcpair->flag = 1;
	wakeup(&(mcpair->cond));
	RF_UNLOCK_MUTEX(mcpair->mutex);
}
