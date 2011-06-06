/*	$NetBSD: cpu.h,v 1.14.26.1 2011/06/06 09:05:14 jruoho Exp $	*/

#ifndef _COBALT_CPU_H_
#define _COBALT_CPU_H_

#include <mips/cpu.h>

#if defined(_KERNEL) || defined(_STANDALONE)
#ifndef _LOCORE
extern u_int cobalt_id;

#define COBALT_ID_QUBE2700	3
#define COBALT_ID_RAQ		4
#define COBALT_ID_QUBE2		5
#define COBALT_ID_RAQ2		6

/*
 * Memory map and register definitions.
 * XXX should be elsewhere?
 */
#define PCIB_BASE	0x10000000
#define GT_BASE		0x14000000
#define LED_ADDR	0x1c000000
#define LED_RESET	0x0f		/* Resets machine. */
#define LED_POWEROFF	3
#define COM_BASE	0x1c800000
#define ZS_BASE		0x1c800000
#define PANEL_BASE	0x1d000000
#define LCD_BASE	0x1f000000

#endif /* !_LOCORE */
#endif /* _KERNEL || _STANDALONE */

#endif /* !_COBALT_CPU_H_ */
