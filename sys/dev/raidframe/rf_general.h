/*	$NetBSD: rf_general.h,v 1.9 2001/10/04 15:58:53 oster Exp $	*/
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

/*
 * rf_general.h -- some general-use definitions
 */

/* #define NOASSERT */

#ifndef _RF__RF_GENERAL_H_
#define _RF__RF_GENERAL_H_

#ifdef _KERNEL_OPT
#include "opt_raid_diagnostic.h"
#endif /* _KERNEL_OPT */

/* error reporting and handling */

#include<sys/systm.h>		/* printf, sprintf, and friends */

#define RF_ERRORMSG(s)            printf((s))
#define RF_ERRORMSG1(s,a)         printf((s),(a))
#define RF_ERRORMSG2(s,a,b)       printf((s),(a),(b))
#define RF_ERRORMSG3(s,a,b,c)     printf((s),(a),(b),(c))

void rf_print_panic_message(int, char *);
void rf_print_assert_panic_message(int, char *, char *);

extern char rf_panicbuf[];
#define RF_PANIC() {rf_print_panic_message(__LINE__,__FILE__); panic(rf_panicbuf);}

#ifdef RAID_DIAGNOSTIC
#define RF_ASSERT(_x_) { \
  if (!(_x_)) { \
    rf_print_assert_panic_message(__LINE__, __FILE__, #_x_); \
    panic(rf_panicbuf); \
  } \
}
#else /* RAID_DIAGNOSTIC */
#define RF_ASSERT(x) {/*noop*/}
#endif /* RAID_DIAGNOSTIC */

/* random stuff */
#define RF_MAX(a,b) (((a) > (b)) ? (a) : (b))
#define RF_MIN(a,b) (((a) < (b)) ? (a) : (b))

/* divide-by-zero check */
#define RF_DB0_CHECK(a,b) ( ((b)==0) ? 0 : (a)/(b) )

/* get time of day */
#define RF_GETTIME(_t) microtime(&(_t))

/*
 * zero memory- not all memset calls go through here, only
 * those which in the kernel may have a user address
 */

#define RF_BZERO(_bp,_b,_l)  memset(_b,0,_l)	/* XXX This is likely
						 * incorrect. GO */


#define RF_UL(x)           ((unsigned long) (x))
#define RF_PGMASK          RF_UL(NBPG-1)
#define RF_BLIP(x)         (NBPG - (RF_UL(x) & RF_PGMASK))	/* bytes left in page */
#define RF_PAGE_ALIGNED(x) ((RF_UL(x) & RF_PGMASK) == 0)

#ifdef __STDC__
#define RF_STRING(_str_) #_str_
#else				/* __STDC__ */
#define RF_STRING(_str_) "_str_"
#endif				/* __STDC__ */

#endif				/* !_RF__RF_GENERAL_H_ */
