/*	$NetBSD: rf_revent.c,v 1.14 2003/12/30 21:59:03 oster Exp $	*/
/*
 * Copyright (c) 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author:
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
/*
 * revent.c -- reconstruction event handling code
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rf_revent.c,v 1.14 2003/12/30 21:59:03 oster Exp $");

#include <sys/errno.h>

#include "rf_raid.h"
#include "rf_revent.h"
#include "rf_etimer.h"
#include "rf_general.h"
#include "rf_desc.h"
#include "rf_shutdown.h"

static struct pool rf_revent_pool;
#define RF_MAX_FREE_REVENT 128
#define RF_REVENT_INC        8
#define RF_REVENT_INITIAL    8



#include <sys/proc.h>
#include <sys/kernel.h>

#define DO_WAIT(_rc)  \
	ltsleep(&(_rc)->eventQueue, PRIBIO,  "raidframe eventq", \
		0, &((_rc)->eq_mutex))

#define DO_SIGNAL(_rc)     wakeup(&(_rc)->eventQueue)


static void rf_ShutdownReconEvent(void *);

static RF_ReconEvent_t *
GetReconEventDesc(RF_RowCol_t col, void *arg, RF_Revent_t type);

static void rf_ShutdownReconEvent(void *ignored)
{
	pool_destroy(&rf_revent_pool);
}

int 
rf_ConfigureReconEvent(RF_ShutdownList_t **listp)
{
	int     rc;

	pool_init(&rf_revent_pool, sizeof(RF_ReconEvent_t),
		  0, 0, 0, "rf_revent_pl", NULL);
	pool_sethiwat(&rf_revent_pool, RF_MAX_FREE_REVENT);
	pool_prime(&rf_revent_pool, RF_REVENT_INITIAL);

	rc = rf_ShutdownCreate(listp, rf_ShutdownReconEvent, NULL);
	if (rc) {
		rf_print_unable_to_add_shutdown(__FILE__, __LINE__, rc);
		rf_ShutdownReconEvent(NULL);
		return (rc);
	}

	return (0);
}

/* returns the next reconstruction event, blocking the calling thread
 * until one becomes available.  will now return null if it is blocked
 * or will return an event if it is not */

RF_ReconEvent_t *
rf_GetNextReconEvent(RF_RaidReconDesc_t *reconDesc,
		     void (*continueFunc)(void *), void *continueArg)
{
	RF_Raid_t *raidPtr = reconDesc->raidPtr;
	RF_ReconCtrl_t *rctrl = raidPtr->reconControl;
	RF_ReconEvent_t *event;

	RF_LOCK_MUTEX(rctrl->eq_mutex);
	/* q null and count==0 must be equivalent conditions */
	RF_ASSERT((rctrl->eventQueue == NULL) == (rctrl->eq_count == 0));

	rctrl->continueFunc = continueFunc;
	rctrl->continueArg = continueArg;


	/* mpsleep timeout value: secs = timo_val/hz.  'ticks' here is
	   defined as cycle-counter ticks, not softclock ticks */

#define MAX_RECON_EXEC_USECS (100 * 1000)  /* 100 ms */
#define RECON_DELAY_MS 25
#define RECON_TIMO     ((RECON_DELAY_MS * hz) / 1000)

	/* we are not pre-emptible in the kernel, but we don't want to run
	 * forever.  If we run w/o blocking for more than MAX_RECON_EXEC_TICKS
	 * ticks of the cycle counter, delay for RECON_DELAY before
	 * continuing. this may murder us with context switches, so we may
	 * need to increase both the MAX...TICKS and the RECON_DELAY_MS. */
	if (reconDesc->reconExecTimerRunning) {
		int     status;

		RF_ETIMER_STOP(reconDesc->recon_exec_timer);
		RF_ETIMER_EVAL(reconDesc->recon_exec_timer);
		reconDesc->reconExecTicks += 
			RF_ETIMER_VAL_US(reconDesc->recon_exec_timer);
		if (reconDesc->reconExecTicks > reconDesc->maxReconExecTicks)
			reconDesc->maxReconExecTicks = 
				reconDesc->reconExecTicks;
		if (reconDesc->reconExecTicks >= MAX_RECON_EXEC_USECS) {
			/* we've been running too long.  delay for
			 * RECON_DELAY_MS */
#if RF_RECON_STATS > 0
			reconDesc->numReconExecDelays++;
#endif				/* RF_RECON_STATS > 0 */

			status = ltsleep(&reconDesc->reconExecTicks, PRIBIO, 
					 "recon delay", RECON_TIMO,
					 &rctrl->eq_mutex);
			RF_ASSERT(status == EWOULDBLOCK);
			reconDesc->reconExecTicks = 0;
		}
	}
	while (!rctrl->eventQueue) {
#if RF_RECON_STATS > 0
		reconDesc->numReconEventWaits++;
#endif				/* RF_RECON_STATS > 0 */
		DO_WAIT(rctrl);
		reconDesc->reconExecTicks = 0;	/* we've just waited */
	}

	reconDesc->reconExecTimerRunning = 1;
	if (RF_ETIMER_VAL_US(reconDesc->recon_exec_timer)!=0) {
		/* it moved!!  reset the timer. */
		RF_ETIMER_START(reconDesc->recon_exec_timer);
	}
	event = rctrl->eventQueue;
	rctrl->eventQueue = event->next;
	event->next = NULL;
	rctrl->eq_count--;

	/* q null and count==0 must be equivalent conditions */
	RF_ASSERT((rctrl->eventQueue == NULL) == (rctrl->eq_count == 0));
	RF_UNLOCK_MUTEX(rctrl->eq_mutex);
	return (event);
}
/* enqueues a reconstruction event on the indicated queue */
void 
rf_CauseReconEvent(RF_Raid_t *raidPtr, RF_RowCol_t col, void *arg, 
		   RF_Revent_t type)
{
	RF_ReconCtrl_t *rctrl = raidPtr->reconControl;
	RF_ReconEvent_t *event = GetReconEventDesc(col, arg, type);

	if (type == RF_REVENT_BUFCLEAR) {
		RF_ASSERT(col != rctrl->fcol);
	}
	RF_ASSERT(col >= 0 && col <= raidPtr->numCol);
	RF_LOCK_MUTEX(rctrl->eq_mutex);
	/* q null and count==0 must be equivalent conditions */
	RF_ASSERT((rctrl->eventQueue == NULL) == (rctrl->eq_count == 0));
	event->next = rctrl->eventQueue;
	rctrl->eventQueue = event;
	rctrl->eq_count++;
	RF_UNLOCK_MUTEX(rctrl->eq_mutex);

	DO_SIGNAL(rctrl);
}
/* allocates and initializes a recon event descriptor */
static RF_ReconEvent_t *
GetReconEventDesc(RF_RowCol_t col, void *arg, RF_Revent_t type)
{
	RF_ReconEvent_t *t;

	t = pool_get(&rf_revent_pool, PR_WAITOK);
	if (t == NULL)
		return (NULL);
	t->col = col;
	t->arg = arg;
	t->type = type;
	t->next = NULL;
	return (t);
}

void 
rf_FreeReconEventDesc(RF_ReconEvent_t *event)
{
	pool_put(&rf_revent_pool, event);
}
