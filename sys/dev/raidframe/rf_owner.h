/*	$NetBSD: rf_owner.h,v 1.2 1999/01/26 02:33:59 oster Exp $	*/
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

#ifndef _RF__RF_OWNER_H_
#define _RF__RF_OWNER_H_

#include "rf_types.h"

struct RF_OwnerInfo_s {
  RF_RaidAccessDesc_t *desc;
  int                  owner;
  double               last_start;
  int                  done;
  int                  notFirst;
};

struct RF_EventCreate_s {
  RF_Raid_t       *raidPtr;
  RF_Script_t     *script;
  RF_OwnerInfo_t  *ownerInfo;
  char            *bufPtr;
  int              bufLen;
};

#endif /* !_RF__RF_OWNER_H_ */
