/*	$NetBSD: cpu.h,v 1.16 2018/04/09 20:16:16 christos Exp $	*/

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
#define LCDPANEL_BASE	0x1d000000
#define LCD_BASE	0x1f000000

#endif /* !_LOCORE */
#endif /* _KERNEL || _STANDALONE */

#endif /* !_COBALT_CPU_H_ */
