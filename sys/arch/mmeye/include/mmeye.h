/* $NetBSD: mmeye.h,v 1.4.6.2 2002/03/24 18:21:24 uch Exp $ */

/*
 * Brains mmEye specific register definition
 */

#ifndef _MMEYE_MMEYE_H_
#define _MMEYE_MMEYE_H_

/* IRQ mask register */
#ifdef MMEYE_NEW_INT /* for new mmEye */
#define	MMTA_IMASK	(*(volatile unsigned short  *)0xb000000e)
#else /* for old mmEye */
#define	MMTA_IMASK	(*(volatile unsigned short  *)0xb0000010)
#endif

#define MMEYE_LED       (*(volatile unsigned short *)0xb0000008)

#ifndef _LOCORE
void *mmeye_intr_establish(int, int, int, int (*func)(void *), void *);
void mmeye_intr_disestablish(void *);
#endif /* !_LOCORE */
#endif /* !_MMEYE_MMEYE_H_ */
