/*	$NetBSD: rf_diskevent.c,v 1.2 1999/01/26 02:33:56 oster Exp $	*/
/*
 * Copyright (c) 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Rachad Youssef
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
 * rf_diskevent. - support for disk device, by managing a heap of future events
 * adapted from original code by David Kotz, Song Bac Toh (1994)
 */

#include "rf_types.h"
#include "rf_heap.h"
#include "rf_diskevent.h"
#include "rf_general.h"
#include "rf_dag.h"
#include "rf_diskthreads.h"
#include "rf_states.h"
#include "rf_shutdown.h"

/* trace printing can be turned on/off in the Makefile */

RF_TICS_t rf_cur_time;
static RF_Owner_t cur_owner;
static RF_Heap_t heap;

static void rf_DDEventShutdown(ignored)
  void  *ignored;
{
  rf_FreeHeap(heap);
}

/* ======================================================================== */
/*  DDEventInit
 *
 *  Initialize the event heap.
 */
int rf_DDEventInit(listp)
  RF_ShutdownList_t  **listp;
{
  int rc;

  heap = rf_InitHeap(RF_HEAP_MAX); /* initialize the heap */
  if (heap == NULL)
    return(ENOMEM);
  rc = rf_ShutdownCreate(listp, rf_DDEventShutdown, NULL);
  if (rc) {
    RF_ERRORMSG3("RAIDFRAME: failed creating shutdown event file %s line %d rc=%d\n",
      __FILE__, __LINE__, rc);
    rf_FreeHeap(heap);
    return(rc);
  }
  rf_cur_time=(RF_TICS_t)0;
  return(0);
}



/*  DDEventRequest
 *
 *  Put an event request into the event heap.
 */
void rf_DDEventRequest(
  RF_TICS_t      eventTime,
  int          (*CompleteFunc)(),
  void          *argument,
  RF_Owner_t     owner,
  RF_RowCol_t    row,
  RF_RowCol_t    col,
  RF_Raid_t     *raidPtr,
  void          *diskid)
{
  RF_HeapData_t *hpdat;

  RF_Malloc(hpdat,sizeof(RF_HeapData_t),(RF_HeapData_t *) );
  if (hpdat == NULL) {
    fprintf(stderr, "DDEventRequest: malloc failed\n");
    RF_PANIC();
  }

  hpdat->eventTime = eventTime;
  hpdat->CompleteFunc = CompleteFunc;  
  hpdat->argument = argument;
  hpdat->owner = owner;
  hpdat->row = row;
  hpdat->col = col;
  hpdat->raidPtr = raidPtr;
  hpdat->diskid = diskid;
  rf_AddHeap(heap, hpdat, (hpdat->eventTime));
}

void rf_DAGEventRequest(
  RF_TICS_t             eventTime,
  RF_Owner_t            owner,
  RF_RowCol_t           row,
  RF_RowCol_t           col,
  RF_RaidAccessDesc_t  *desc,
  RF_Raid_t            *raidPtr) 
{
  RF_HeapData_t *hpdat;    

  RF_Malloc(hpdat,sizeof(RF_HeapData_t),(RF_HeapData_t *) );
  if (hpdat == NULL) {
    fprintf(stderr, "DDEventRequest: malloc failed\n");
    RF_PANIC();
  }

  hpdat->eventTime = eventTime;
  hpdat->CompleteFunc = NULL;  
  hpdat->argument = NULL;
  hpdat->owner = owner;
  hpdat->row = row;
  hpdat->col = col;
  hpdat->desc=desc;
  hpdat->raidPtr = raidPtr;

  rf_AddHeap(heap, hpdat, (hpdat->eventTime));
}


/* ------------------------------------------------------------------------ */
/* @SUBTITLE "Print out the request queue" */
/* There is only 1 request queue so no argument is needed for this
   function */
void rf_DDPrintRequests()
{
  RF_HeapData_t *Hpdat;
  RF_HeapKey_t Hpkey;
  RF_Heap_t tempHp;

  printf("Events on heap:\n");

  tempHp = rf_InitHeap(RF_HEAP_MAX);
  while (rf_RemHeap(heap, &Hpdat, &Hpkey) != RF_HEAP_NONE)
    {
	 printf ("at %5g HpKey there is: something for owner %d  at disk  %d %d\n",Hpkey,
		 Hpdat->owner,Hpdat->row,Hpdat->col);
	 rf_AddHeap(tempHp, Hpdat, Hpdat->eventTime);
    }

  printf("END heap:\n");
  rf_FreeHeap(heap);		 /* free the empty old heap */

  heap = tempHp;		 /* restore the recycled heap */
}
/* ------------------------------------------------------------------------ */

int rf_ProcessEvent()
{
  RF_HeapData_t *Hpdat;
  RF_HeapKey_t Hpkey;
  int retcode;

  retcode = rf_RemHeap(heap, &Hpdat, &Hpkey);

  if (retcode==RF_HEAP_FOUND) {          
    if (rf_eventDebug) {
      rf_DDPrintRequests();
      printf ("Now processing: at %5g something for owner %d  at disk  %d %d\n",
        Hpkey, Hpdat->owner, Hpdat->row, Hpdat->col);
    }
    rf_cur_time=Hpkey;

    rf_SetCurrentOwner(Hpdat->owner);

    if (Hpdat->row>=0) {/* ongoing dag event */
      rf_SetDiskIdle (Hpdat->raidPtr, Hpdat->row, Hpdat->col);
      if (Hpdat->diskid != NULL) {
        rf_simulator_complete_io(Hpdat->diskid);
      }
      retcode=(Hpdat->CompleteFunc)(Hpdat->argument,0);
      if (retcode==RF_HEAP_FOUND)
        (((RF_DagNode_t *) (Hpdat->argument))->dagHdr->cbFunc)(((RF_DagNode_t *) (Hpdat->argument))->dagHdr->cbArg);
      RF_Free(Hpdat,sizeof(RF_HeapData_t));
      return(retcode);
    }
    else {
      /* this is a dag event or reconstruction event */
      if (Hpdat->row==RF_DD_DAGEVENT_ROW){ /* dag event */
        rf_ContinueRaidAccess(Hpdat->desc);
        retcode = RF_FALSE;
        RF_Free(Hpdat,sizeof(RF_HeapData_t));
        return (RF_FALSE);
      }
      else {
        /* recon event */
        retcode=(Hpdat->CompleteFunc)(Hpdat->argument,0);
        retcode = RF_FALSE;
        RF_Free(Hpdat,sizeof(RF_HeapData_t));
        return (RF_FALSE);
      }
    }
  }
  if (rf_eventDebug)
    printf("HEAP is empty\n");
  return(RF_DD_NOTHING_THERE);
}

RF_Owner_t rf_GetCurrentOwner()
{
  return(cur_owner);
}

void rf_SetCurrentOwner(RF_Owner_t owner)
{
  cur_owner=owner;
}

RF_TICS_t rf_CurTime()
{
  return(rf_cur_time);
}
