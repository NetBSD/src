/*	$NetBSD: rf_alloclist.c,v 1.15 2003/12/30 21:59:03 oster Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: rf_alloclist.c,v 1.15 2003/12/30 21:59:03 oster Exp $");

#include <dev/raidframe/raidframevar.h>

#include "rf_options.h"
#include "rf_threadstuff.h"
#include "rf_alloclist.h"
#include "rf_debugMem.h"
#include "rf_etimer.h"
#include "rf_general.h"
#include "rf_shutdown.h"

static unsigned int fl_hit_count, fl_miss_count;

static RF_AllocListElem_t *al_free_list = NULL;
static int al_free_list_count;

#define RF_AL_FREELIST_MAX 256

#define DO_FREE(_p,_sz) RF_Free((_p),(_sz))

static void rf_ShutdownAllocList(void *);

static void rf_ShutdownAllocList(void *ignored)
{
	RF_AllocListElem_t *p, *pt;

	for (p = al_free_list; p;) {
		pt = p;
		p = p->next;
		DO_FREE(pt, sizeof(*pt));
	}
	/*
        printf("Alloclist: Free list hit count %lu (%lu %%) miss count %lu (%lu %%)\n",
	       fl_hit_count, (100*fl_hit_count)/(fl_hit_count+fl_miss_count),
	       fl_miss_count, (100*fl_miss_count)/(fl_hit_count+fl_miss_count));
        */
}

int 
rf_ConfigureAllocList(RF_ShutdownList_t **listp)
{
	int     rc;

	al_free_list = NULL;
	fl_hit_count = fl_miss_count = al_free_list_count = 0;
	rc = rf_ShutdownCreate(listp, rf_ShutdownAllocList, NULL);
	if (rc) {
		rf_print_unable_to_add_shutdown( __FILE__, __LINE__, rc);
		return (rc);
	}
	return (0);
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
		if (al_free_list_count > RF_AL_FREELIST_MAX) {
			DO_FREE(temp, sizeof(*temp));
		} else {
			temp->next = al_free_list;
			al_free_list = temp;
			al_free_list_count++;
		}
	}
}

RF_AllocListElem_t *
rf_real_MakeAllocList()
{
	RF_AllocListElem_t *p;

	if (al_free_list) {
		fl_hit_count++;
		p = al_free_list;
		al_free_list = p->next;
		al_free_list_count--;
	} else {
		fl_miss_count++;
		RF_Malloc(p, sizeof(RF_AllocListElem_t), 
			  (RF_AllocListElem_t *));	
		/* no allocation locking in kernel, so this is fine */
	}
	if (p == NULL) {
		return (NULL);
	}
	memset((char *) p, 0, sizeof(RF_AllocListElem_t));
	return (p);
}
