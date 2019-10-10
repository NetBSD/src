/*	$NetBSD: rf_dagfuncs.h,v 1.11 2019/10/10 03:43:59 christos Exp $	*/
/*
 * Copyright (c) 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Mark Holland, William V. Courtright II, Jim Zelenka
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

/*****************************************************************************************
 *
 * dagfuncs.h -- header file for DAG node execution routines
 *
 ****************************************************************************************/

#ifndef _RF__RF_DAGFUNCS_H_
#define _RF__RF_DAGFUNCS_H_

int     rf_ConfigureDAGFuncs(RF_ShutdownList_t ** listp);
void    rf_TerminateFunc(RF_DagNode_t * node);
void    rf_TerminateUndoFunc(RF_DagNode_t * node);
void    rf_DiskReadMirrorIdleFunc(RF_DagNode_t * node);
void    rf_DiskReadMirrorPartitionFunc(RF_DagNode_t * node);
void    rf_DiskReadMirrorUndoFunc(RF_DagNode_t * node);
void    rf_ParityLogUpdateFunc(RF_DagNode_t * node);
void    rf_ParityLogOverwriteFunc(RF_DagNode_t * node);
void    rf_ParityLogUpdateUndoFunc(RF_DagNode_t * node);
void    rf_ParityLogOverwriteUndoFunc(RF_DagNode_t * node);
void    rf_NullNodeFunc(RF_DagNode_t * node);
void    rf_NullNodeUndoFunc(RF_DagNode_t * node);
void    rf_DiskReadFuncForThreads(RF_DagNode_t * node);
void    rf_DiskWriteFuncForThreads(RF_DagNode_t * node);
void    rf_DiskUndoFunc(RF_DagNode_t * node);
void    rf_GenericWakeupFunc(void *, int);
void    rf_RegularXorFunc(RF_DagNode_t * node);
void    rf_SimpleXorFunc(RF_DagNode_t * node);
void    rf_RecoveryXorFunc(RF_DagNode_t * node);
int
rf_XorIntoBuffer(RF_Raid_t * raidPtr, RF_PhysDiskAddr_t * pda, char *srcbuf,
		 char *targbuf);
int     rf_bxor(char *src, char *dest, int len);
int
rf_longword_bxor(unsigned long *src, unsigned long *dest, int len);
int
rf_longword_bxor3(unsigned long *dest, unsigned long *a, unsigned long *b,
		  unsigned long *c, int len, void *bp);
int
rf_bxor3(unsigned char *dst, unsigned char *a, unsigned char *b,
    unsigned char *c, unsigned long len, void *bp);

/* function ptrs defined in ConfigureDAGFuncs() */
extern void (*rf_DiskReadFunc) (RF_DagNode_t *);
extern void (*rf_DiskWriteFunc) (RF_DagNode_t *);
extern void (*rf_DiskReadUndoFunc) (RF_DagNode_t *);
extern void (*rf_DiskWriteUndoFunc) (RF_DagNode_t *);
extern void (*rf_SimpleXorUndoFunc) (RF_DagNode_t *);
extern void (*rf_RegularXorUndoFunc) (RF_DagNode_t *);
extern void (*rf_RecoveryXorUndoFunc) (RF_DagNode_t *);

/* macros for manipulating the param[3] in a read or write node */
#define RF_CREATE_PARAM3(pri, wru) (((RF_uint64)(((wru&0xFFFFFF)<<8)|((pri)&0xF)) ))
#define RF_EXTRACT_PRIORITY(_x_)     ((((unsigned) ((unsigned long)(_x_))) >> 0) & 0x0F)
#define RF_EXTRACT_RU(_x_)           ((((unsigned) ((unsigned long)(_x_))) >> 8) & 0xFFFFFF)

#endif				/* !_RF__RF_DAGFUNCS_H_ */
