/*	$NetBSD: superioreg.h,v 1.1 2002/07/05 13:31:38 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _EVBSH5_SUPERIOREG_H
#define _EVBSH5_SUPERIOREG_H

#define	SUPERIO_REG_SZ		0x1000

#define	SUPERIO_REG_INDEX	0xfc0
#define	SUPERIO_REG_DATA	0xfc4

/*
 * Global Configuration Registers
 */
#define	SUPERIO_GLBL_REG_CONFIG		0x02
#define	SUPERIO_GLBL_REG_INDEX		0x03
#define	SUPERIO_GLBL_REG_LDN		0x07
#define	SUPERIO_GLBL_REG_DEVID		0x20
#define	SUPERIO_GLBL_REG_DEVREV		0x21
#define	SUPERIO_GLBL_REG_POWER_CTRL	0x22
#define  SUPERIO_GLBL_POWER_CTRL_FDC	(1<<0)	/* FDC Power Enable */
#define  SUPERIO_GLBL_POWER_CTRL_IDE1	(1<<1)	/* IDE1 Power Enable */
#define  SUPERIO_GLBL_POWER_CTRL_IDE2	(1<<2)	/* IDE2 Power Enable */
#define  SUPERIO_GLBL_POWER_CTRL_LPT	(1<<3)	/* Printer Power Enable */
#define  SUPERIO_GLBL_POWER_CTRL_UART1	(1<<4)	/* UART1 Power Enable */
#define  SUPERIO_GLBL_POWER_CTRL_UART2	(1<<5)	/* UART2 Power Enable */
#define	SUPERIO_GLBL_REG_POWER_MGMT	0x23
#define  SUPERIO_GLBL_POWER_MGMT_FDC	(1<<0)	/* FDC Power Management */
#define  SUPERIO_GLBL_POWER_MGMT_IDE1	(1<<1)	/* IDE1 Power Management */
#define  SUPERIO_GLBL_POWER_MGMT_IDE2	(1<<2)	/* IDE2 Power Management */
#define  SUPERIO_GLBL_POWER_MGMT_LPT	(1<<3)	/* Printer Power Management */
#define  SUPERIO_GLBL_POWER_MGMT_UART1	(1<<4)	/* UART1 Power Management */
#define  SUPERIO_GLBL_POWER_MGMT_UART2	(1<<5)	/* UART2 Power Management */
#define	SUPERIO_GLBL_REG_OSC		0x24
#define	SUPERIO_GLBL_REG_TEST1		0x2d
#define	SUPERIO_GLBL_REG_TEST2		0x2e
#define	SUPERIO_GLBL_REG_TEST3		0x2f

/*
 * Generic Device Configuration Registers
 */
#define	SUPERIO_DEV_REG_ACTIVATE	0x30
#define	SUPERIO_DEV_REG_BAR0_HI		0x60
#define	SUPERIO_DEV_REG_BAR0_LO		0x61
#define	SUPERIO_DEV_REG_BAR1_HI		0x62
#define	SUPERIO_DEV_REG_BAR1_LO		0x63
#define	SUPERIO_DEV_REG_INT0_SEL	0x70
#define	SUPERIO_DEV_REG_INT1_SEL	0x71
#define	SUPERIO_DEV_REG_DMA_SEL		0x74
#define	SUPERIO_DEV_REG_MODE		0xf0
#define	SUPERIO_DEV_REG_OPTION		0xf1

/*
 * FDD-Specific Configuration Registers
 */
#define	SUPERIO_FDD_REG_TYPE		0xf2
#define	SUPERIO_FDD_REG_FDD0		0xf4
#define	SUPERIO_FDD_REG_FDD1		0xf5

/*
 * Logical Device Numbers
 */
#define SUPERIO_LDN_FDD			0
#define SUPERIO_LDN_IDE1		1
#define SUPERIO_LDN_IDE2		2
#define SUPERIO_LDN_LPT			3
#define SUPERIO_LDN_UART1		4
#define SUPERIO_LDN_UART2		5
#define SUPERIO_LDN_RTC			6
#define SUPERIO_LDN_KEYBOARD		7
#define SUPERIO_LDN_AUXIO		8

#endif /* _EVBSH5_SUPERIOREG_H */
