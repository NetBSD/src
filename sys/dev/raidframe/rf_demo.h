/*	$NetBSD: rf_demo.h,v 1.2 1999/01/26 02:33:55 oster Exp $	*/
/*
 * Copyright (c) 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Mark Holland, Khalil Amiri
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

/* rf_demo.h
 * some constants for demo'ing software
 */

#ifndef _RF__RF_DEMO_H_
#define _RF__RF_DEMO_H_

#include "rf_types.h"

#define RF_DEMO_METER_WIDTH    300    /* how wide each meter is */
#define RF_DEMO_METER_HEIGHT   150    /* how tall */
#define RF_DEMO_METER_SPACING   15    /* how much space between horizontally */
#define RF_DEMO_METER_VSPACE    20    /* how much space between vertically */
#define RF_DEMO_FAULT_FREE  0
#define RF_DEMO_DEGRADED    1
#define RF_DEMO_RECON       2

void rf_startup_iops_demo(int meter_vpos, int C, int G);
void rf_update_user_stats(int resptime);
void rf_update_disk_iops(int val);
void rf_meter_update_thread(void);
void rf_finish_iops_demo(void);
void rf_demo_update_mode(int arg_mode);
void rf_startup_recon_demo(int meter_vpos, int C, int G, int init);
void rf_update_recon_meter(int val);
void rf_finish_recon_demo(struct timeval *etime);

extern int rf_demo_op_mode;

#endif /* !_RF__RF_DEMO_H_ */
