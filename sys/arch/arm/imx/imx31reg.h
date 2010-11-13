/* $NetBSD: imx31reg.h,v 1.4 2010/11/13 06:44:11 bsh Exp $ */
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
/* register definitions in imxgpiore.h */

#define	INTC_BASE		0x68000000
#define	INTC_SIZE		0x0400
#define	IMX31_INTCNTL		0x0000	/* Interrupt Control (RW) */
#define	IMX31_NIMASK		0x0004	/* Normal Interrupt Mask (RW) */
#define	IMX31_INTENNUM		0x0008	/* Interrupt Enable Number (RW) */
#define	IMX31_INTDISNUM		0x000c	/* Interrupt Disable Number (RW) */
#define	IMX31_INTENABLEH	0x0010	/* Interrupt Enable High (RW) */
#define	IMX31_INTENABLEL	0x0014	/* Interrupt Enable Low (RW) */
#define	IMX31_INTTYPEH		0x0018	/* Interrupt Type High (RW) */
#define	IMX31_INTTYPEL		0x001c	/* Interrupt Type Low (RW) */
#define	IMX31_NIPRIORITY7	0x0020	/* Normal Intr Priority Level 7 (RW) */
#define	IMX31_NIPRIORITY6	0x0024	/* Normal Intr Priority Level 6 (RW) */
#define	IMX31_NIPRIORITY5	0x0028	/* Normal Intr Priority Level 5 (RW) */
#define	IMX31_NIPRIORITY4	0x002c	/* Normal Intr Priority Level 4 (RW) */
#define	IMX31_NIPRIORITY3	0x0030	/* Normal Intr Priority Level 3 (RW) */
#define	IMX31_NIPRIORITY2	0x0034	/* Normal Intr Priority Level 2 (RW) */
#define	IMX31_NIPRIORITY1	0x0038	/* Normal Intr Priority Level 1 (RW) */
#define	IMX31_NIPRIORITY0	0x003c	/* Normal Intr Priority Level 0 (RW) */
#define	IMX31_NIVECSR		0x0040	/* Normal Interrupt Vector Status (R) */
#define	IMX31_FIVECSR		0x0044	/* Fast Interrupt Vector Status (R) */
#define	IMX31_INTSRCH		0x0048	/* Interrupt Source High (R) */
#define	IMX31_INTSRCL		0x004c	/* Interrupt Source Low (R) */
#define	IMX31_INTFRCH		0x0050	/* Interrupt Force High (RW) */
#define	IMX31_INTFRCL		0x0054	/* Interrupt Force Low (RW) */
#define	IMX31_NIPNDH		0x0058	/* Normal Intr Pending High (R) */
#define	IMX31_NIPNDL		0x005c	/* Normal Intr Pending Low (R) */
#define	IMX31_FIPNDH		0x0060	/* Fast Intr Pending High (R) */
#define	IMX31_FIPNDL		0x0064	/* Fast Intr Pending Low (R) */

#define	IMX31_VECTOR(n)		(0x0100 + (n) * 4)	/* Vector [N] */

#define	INTCNTL_ABFLAG	(1 << 25)	/* Core Arb. Priorty Risen (W1C) */
#define	INTCNTL_ABFEN	(1 << 24)	/* ABFLAG Sticky Enable */
#define	INTCNTL_NIDIS	(1 << 22)	/* Normal Intr. Disable */
#define	INTCNTL_FIDIS	(1 << 21)	/* Fast Intr. Disable */
#define	INTCNTL_NIAD	(1 << 20)	/* Normal Intr. Arbiter Rise ARM Lvl */
#define	INTCNTL_FIAD	(1 << 19)	/* Fast Intr. Arbiter Rise ARM Level */
#define	INTCNTL_NM	(1 << 18)	/* Normal Intr. Mode Control (1=AVIC) */

#define	NIMASK_DIS_NONE		-1

/*
 * INTTYPE (0 = IRQ, 1 = FIQ)
 */

#endif /* _ARM_IMX_IMX31REG_H_ */
