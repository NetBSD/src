/* $NetBSD: ioebreg.h,v 1.1 2002/03/24 15:47:19 bjh21 Exp $ */

/*
 * This file is in the public domain.
 *
 * Derived from:
 * RISC OS 3 Programmer's Reference Manual
 * A3010/A3020/A4000 Technical Reference Manual
 */

/* Acorn IOEB (Input/Output Extension Block) registers */

#ifndef _IOEBREG_H_
#define _IOEBREG_H_

/*
 * IOEB starts at a rather high offset because it avoids the ranges
 * used on older machines.
 */

#define IOEB_REG_VIDCTL		18 /* Video control latch */
#define IOEB_REG_ID		20 /* Device ID */
#define IOEB_REG_SPEED		21 /* Clock speed (A4?) */
#define IOEB_REG_INTRCLR	22 /* Printer interrupt clear */
#define IOEB_REG_MONID		28 /* Monitor ID */
#define IOEB_REG_VGATEST	29 /* VGA test pin/SCART sound ??? */
#define IOEB_REG_JOY1		30 /* Joystick 1 */
#define IOEB_REG_JOY2		31 /* Joystick 2 */

/* The IOEB is connected to D[3:0], so its internal registers are 4-bit */
#define IOEB_VIDCTL_HSINV	0x1 /* Invert horizontal sync */
#define IOEB_VIDCTL_VSINV	0x2 /* Invert vertical sync */
#define IOEB_VIDCTL_CLK_MASK	0xc /* VIDCLK select */
#define IOEB_VIDCTL_CLK_24MHZ	0x0 /* 24 MHz */
#define IOEB_VIDCTL_CLK_25MHZ	0x4 /* 25.175 MHz */
#define IOEB_VIDCTL_CLK_36MHZ	0x8 /* 36 MHz */

#define IOEB_ID_IOEB		0x5

#endif
