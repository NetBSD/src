/* $NetBSD: mmeye.h,v 1.2 1999/09/16 13:33:04 msaitoh Exp $ */

/*
 * Brains mmEye specific register definition
 */

#ifndef _MMEYE_MMEYE_H_
#define _MMEYE_H_

/* IRQ mask register */
#ifdef MMEYE_NEW_INT /* for new mmEye */
#define	MMTA_IMASK	(*(volatile unsigned short  *)0xb000000E)
#else /* for old mmEye */
#define	MMTA_IMASK	(*(volatile unsigned short  *)0xb0000010)
#endif

#define MMEYE_LED       (*(volatile unsigned short *)0xb0000008)

/*
 * SCI bitrate
 * 9600bps, 11 = 3750000/(32*9600) -1, Pcyc = 3.75MHz
 */

#define SCI_BITRATE 11

#endif /* !_MMEYE_MMEYE_H_ */
