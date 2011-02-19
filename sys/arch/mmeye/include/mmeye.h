/* $NetBSD: mmeye.h,v 1.5 2011/02/19 10:46:28 kiyohara Exp $ */

/*
 * Brains mmEye specific register definition
 */

#ifndef _MMEYE_MMEYE_H_
#define _MMEYE_MMEYE_H_

#include "opt_mmeye.h"

/* IRQ mask register */
#ifdef MMEYE_NEW_INT /* for new mmEye */
#define	MMTA_IMASK	(*(volatile unsigned short *)MMEYE_NEW_INT)
#else /* for old mmEye */
#define	MMTA_IMASK	(*(volatile unsigned short *)0xb0000010)
#endif

#define MMEYE_LED       (*(volatile unsigned short *)0xb0000008)

#ifndef _LOCORE
void *mmeye_intr_establish(int, int, int, int (*func)(void *), void *);
void mmeye_intr_disestablish(void *);

#if defined(MMEYE_EPC_WDT)
#define EPC_WDT		(*(volatile short *)0xb1000000)
#define   WDT_RDYCMD	0xaa
#define   WDT_CLRCMD	0x55
#define   WDT_DISCMD	0x0f	/* XXX: Oops, no effect... */
#define   WDT_ENACMD	0xf0

callout_t epc_wdtc;
void epc_watchdog_timer_reset(void *);
#endif

#endif /* !_LOCORE */
#endif /* !_MMEYE_MMEYE_H_ */
