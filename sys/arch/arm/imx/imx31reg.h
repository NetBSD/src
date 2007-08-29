/* $NetBSD: imx31reg.h,v 1.1.2.1 2007/08/29 05:24:24 matt Exp $ */
/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_IMX_IMX31REG_H_
#define _ARM_IMX_IMX31REG_H_

#define	I2C1_BASE	0x43f80000
#define	I2C2_BASE	0x43f90000
#define	I2C3_BASE	0x43f84000

#define	I2C_IADR	0x0000	/* I2C Address */
#define	I2C_IFDR	0x0004	/* I2C Frequency Divider */
#define	I2C_I2CR	0x0008	/* I2C Control */
#define	I2C_I2SR	0x000c	/* I2C Status */
#define	I2C_I2DR	0x0010	/* I2C Data I/O */

#define	ATA_BASE	0x43f8c000
#define	ATA_DMA_BASE	0x50020000

#define	UART1_BASE	0x43f90000
#define	UART2_BASE	0x43f94000
#define	UART3_BASE	0x5000c000
#define	UART4_BASE	0x43fb0000
#define	UART5_BASE	0x43fb4000

#define	CSPI1_BASE	0x43fa4000
#define	CSPI2_BASE	0x50010000

#define	GPIO1_BASE	0x53fcc000
#define	GPIO2_BASE	0x53fd0000
#define	GPIO3_BASE	0x53fa4000

#define	GPIO_DR		0x0000	/* GPIO Data (RW) */
#define	GPIO_GDIR	0x0004	/* GPIO Direction (RW), 1=Output */
#define	GPIO_PSR	0x0008	/* GPIO Pad Status (R) */
#define	GPIO_ICR1	0x000c	/* GPIO Interrupt Configuration 1 (RW) */
#define	GPIO_ICR2	0x0010	/* GPIO Interrupt Configuration 2 (RW) */
#define	GPIO_IMR	0x0014	/* GPIO Interrupt Mask (RW) */
#define	GPIO_ISR	0x0018	/* GPIO Interrupt Status (RW, W1C) */

#define	GPIO_ICR_LEVEL_LOW	0
#define	GPIO_ICR_LEVEL_HIGH	1
#define	GPIO_ICR_EDGE_RISING	2
#define	GPIO_ICR_EDGE_FALLING	3

#endif /* _ARM_IMX_IMX31REG_H_ */
