/*	$NetBSD: rf_alloclist.c,v 1.30 2026/10/24 00:20:23 tls Exp $	*/
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

/****************************************************************************
 *
 * Alloclist.c -- code to manipulate allocation lists
 *
 * an allocation list is just a list of AllocListElem structures.  Each
 * such structure contains a fixed-size array of pointers.  Calling
 * FreeAList() causes each pointer to be freed.
 *
 ***************************************************************************/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rf_alloclist.c,v 1.30 2026/10/24 00:20:23 tls Exp $");

#include <dev/raidframe/raidframevar.h>

#include "rf_options.h"
#include "rf_threadstuff.h"
#include "rf_raid.h"
#include "rf_alloclist.h"
#include "rf_debugMem.h"
#include "rf_etimer.h"
#include "rf_general.h"
#include "rf_shutdown.h"
#include "rf_netbsd.h"

#include <sys/pool.h>

#define RF_AL_INIT_MIN 4

static size_t rf_alloclist_primed;

static void rf_ShutdownAllocList(void *);

static void rf_ShutdownAllocList(void *ignored)
{
	pool_destroy(&rf_alloclist_pool);
	rf_alloclist_primed = 0;
}

int
rf_ConfigureAllocList(RF_ShutdownList_t **listp)
{

	pool_init(&rf_alloclist_pool, sizeof(RF_AllocListElem_t), 0, 0, 0, "rf_alloclist_pl", NULL, IPL_BIO);
	pool_sethiwat(&rf_alloclist_pool, 2 * RF_AL_INIT_MIN);
	pool_prime(&rf_alloclist_pool, RF_AL_INIT_MIN);
	rf_alloclist_primed = RF_AL_INIT_MIN;

	rf_ShutdownCreate(listp, rf_ShutdownAllocList, NULL);

	return (0);
}

/*
 * Prime rf_alloclist_pool for an array's worst-case concurrent I/Os.
 * Each admitted I/O uses 1 elem for desc->allocList plus 1 per DAG header,
 * and parity rewrite / array cleanupList use 3 more.
 */
void
rf_GrowAllocListPool(RF_Raid_t *raidPtr)
{
	size_t stripe_data_bytes, needed;
	u_int max_stripes_per_bp, d, o;

	d = raidPtr->Layout.numDataCol;
	o = raidPtr->openings;
	stripe_data_bytes = (size_t)raidPtr->Layout.dataSectorsPerStripe <<
	    raidPtr->logBytesPerSector;
	max_stripes_per_bp = howmany(MAXPHYS, stripe_data_bytes) + 1;

	needed = o * (1 + max_stripes_per_bp + 2 * d) + 3;
	pool_prime(&rf_alloclist_pool, needed);
	rf_alloclist_primed += needed;
	pool_sethiwat(&rf_alloclist_pool, 2 * rf_alloclist_primed);
}


/* we expect the lists to have at most one or two elements, so we're willing
 * to search for the end.  If you ever observe the lists growing longer,
 * increase POINTERS_PER_ALLOC_LIST_ELEMENT.
 */
void
rf_real_AddToAllocList(RF_AllocListElem_t *l, void *p, int size)
{
	RF_AllocListElem_t *newelem;

	for (; l->next; l = l->next)
		RF_ASSERT(l->numPointers == RF_POINTERS_PER_ALLOC_LIST_ELEMENT);	/* find end of list */

	RF_ASSERT(l->numPointers >= 0 && l->numPointers <= RF_POINTERS_PER_ALLOC_LIST_ELEMENT);
	if (l->numPointers == RF_POINTERS_PER_ALLOC_LIST_ELEMENT) {
		newelem = rf_real_MakeAllocList();
		l->next = newelem;
		l = newelem;
	}
	l->pointers[l->numPointers] = p;
	l->sizes[l->numPointers] = size;
	l->numPointers++;

}

void
rf_FreeAllocList(RF_AllocListElem_t *l)
{
	int     i;
	RF_AllocListElem_t *temp, *p;

	for (p = l; p; p = p->next) {
		RF_ASSERT(p->numPointers >= 0 &&
			  p->numPointers <= RF_POINTERS_PER_ALLOC_LIST_ELEMENT);
		for (i = 0; i < p->numPointers; i++) {
			RF_ASSERT(p->pointers[i]);
			RF_Free(p->pointers[i], p->sizes[i]);
		}
	}
	while (l) {
		temp = l;
		l = l->next;
		pool_put(&rf_alloclist_pool, temp);
	}
}

RF_AllocListElem_t *
rf_real_MakeAllocList(void)
{
	return pool_get(&rf_alloclist_pool, PR_WAITOK | PR_ZERO);
}
