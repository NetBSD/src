/*	$NetBSD: rf_diskevent.h,v 1.2 1999/01/26 02:33:56 oster Exp $	*/
/* 
 * rf_diskevent.h
 * Adapted from original code by David Kotz (1994)
 *
 * The disk-device module is event driven.  This module keeps the event 
 * request mechanism, which is based on proteus SimRequests, 
 * abstracted away from the bulk of the disk device code. 
 *
 * Functions
 *  	DDEventInit
 *  	DDEventRequest
 *  	DDEventPrint
 *  	DDEventCancel
 */

#ifndef _RF__RF_DISKEVENT_H_
#define _RF__RF_DISKEVENT_H_

#include "rf_types.h"
#include "rf_heap.h"

#define RF_DD_NOTHING_THERE (-1)
#define RF_DD_DAGEVENT_ROW  (-3)
#define RF_DD_DAGEVENT_COL RF_DD_DAGEVENT_ROW

extern RF_TICS_t rf_cur_time;

/*
 * list of disk-device request types, 
 * initialized in diskdevice.c, 
 * used in diskevent.c
 */
typedef void (*RF_DDhandler)(int disk, RF_TICS_t eventTime);
struct RF_dd_handlers_s {
  RF_DDhandler  handler;  /* function implementing this event type */
  char          name[20]; /* name of that event type */
};
extern struct RF_dd_handlers_s rf_DDhandlers[];

int        rf_DDEventInit(RF_ShutdownList_t **listp);
void       rf_DDEventRequest(RF_TICS_t eventTime, int (*CompleteFunc)(),
	void *argument, RF_Owner_t owner, RF_RowCol_t row, RF_RowCol_t col,
	RF_Raid_t *raidPtr, void *diskid);
void       rf_DAGEventRequest(RF_TICS_t eventTime, RF_Owner_t owner,
	RF_RowCol_t row, RF_RowCol_t col, RF_RaidAccessDesc_t *desc,
	RF_Raid_t *raidPtr);
void       rf_DDPrintRequests(void);
int        rf_ProcessEvent(void);
RF_Owner_t rf_GetCurrentOwner(void);
void       rf_SetCurrentOwner(RF_Owner_t owner);
RF_TICS_t  rf_CurTime(void);

#endif /* !_RF__RF_DISKEVENT_H_ */
