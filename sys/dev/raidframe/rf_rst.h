/*	$NetBSD: rf_rst.h,v 1.2 1999/01/26 02:34:02 oster Exp $	*/
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

/* rf_rst.h - defines raidSim trace entry */

#ifndef _RF__RF_RST_H_
#define _RF__RF_RST_H_

#include "rf_types.h"

typedef struct RF_ScriptTraceEntry_s {
    RF_int32   blkno;
    RF_int32   size;
    double     delay;
    RF_int16   pid;
    RF_int8    op;
    RF_int8    async_flag;
} RF_ScriptTraceEntry_t;

typedef struct RF_ScriptTraceEntryList_s  RF_ScriptTraceEntryList_t;
struct RF_ScriptTraceEntryList_s {
   RF_ScriptTraceEntry_t       entry;
   RF_ScriptTraceEntryList_t  *next;
};

#endif /* !_RF__RF_RST_H_ */
