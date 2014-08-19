/*	$NetBSD: intrdefs.h,v 1.2.6.2 2014/08/20 00:03:05 tls Exp $	*/

#ifndef _HPPA_INTRDEFS_H_
#define _HPPA_INTRDEFS_H_

/* Interrupt priority `levels'. */
#define	IPL_NONE	7	/* nothing */
#define	IPL_SOFTCLOCK	6	/* timeouts */
#define	IPL_SOFTBIO	5	/* block I/O */
#define	IPL_SOFTNET	4	/* protocol stacks */
#define	IPL_SOFTSERIAL	3	/* serial */
#define	IPL_VM		2	/* memory allocation, low I/O */
#define	IPL_SCHED	1	/* clock, medium I/O */
#define	IPL_HIGH	0	/* everything */
#define	NIPL		8

/* Interrupt sharing types. */
#define	IST_NONE	0	/* none */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

#ifdef MULTIPROCESSOR
#define	HPPA_IPI_NOP		0
#define	HPPA_IPI_HALT		1
#define	HPPA_IPI_XCALL		2
#define	HPPA_IPI_GENERIC	3
#define	HPPA_NIPI		4
#endif

#endif
