/*	$NetBSD: rf_configure.h,v 1.2 2017/11/20 19:10:45 christos Exp $	*/
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

/********************************
 *
 * rf_configure.h
 *
 * header file for raidframe configuration in the kernel version only.
 * configuration is invoked via ioctl rather than at boot time
 *
 *******************************/


#ifndef _RF__RF_CONFIGURE_H_
#define _RF__RF_CONFIGURE_H_

#include <dev/raidframe/raidframevar.h>

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/ioctl.h>

int     rf_MakeConfig(char *, RF_Config_t *);
int     rf_MakeLayoutSpecificNULL(FILE *, RF_Config_t *, void *);
int     rf_MakeLayoutSpecificDeclustered(FILE *, RF_Config_t *, void *);
void   *rf_ReadSpareTable(RF_SparetWait_t *, char *);

#endif /* !_RF__RF_CONFIGURE_H_ */
