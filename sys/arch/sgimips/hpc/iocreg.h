/*	$NetBSD: iocreg.h,v 1.1.8.2 2002/04/01 07:42:23 nathanw Exp $	*/

/*
 * Copyright (c) 2001 Rafal K. Boni
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARCH_SGIMIPS_HPC_IOCREG_H_
#define	_ARCH_SGIMIPS_HPC_IOCREG_H_

/*
 * IOC1/2 memory map.
 *
 * The IOC1/2 is connected to the HPC#0, PBus channel 6, so these registers
 * are based from the external register window for PBus channel 6 on HPC#0.
 *
 * XXX: define register values as well as their offsets.
 *
 */
#define IOC_PLP_REGS		0x00000000	/* Parallel port registers */
#define IOC_PLP_REGS_SIZE	0x0000002c

#define IOC_PLP_DATA		0x00000000	/* Data register */
#define IOC_PLP_CTL		0x00000004	/* Control register */
#define IOC_PLP_STAT		0x00000008	/* Status register */
#define IOC_PLP_DMACTL		0x0000000c	/* DMA control register */
#define IOC_PLP_INTSTAT		0x00000010	/* Interrupt status register */
#define IOC_PLP_INTMASK		0x00000014	/* Interrupt mask register */
#define IOC_PLP_TIMER1		0x00000018	/* Timer 1 register */
#define IOC_PLP_TIMER2		0x0000001c	/* Timer 2 register */
#define IOC_PLP_TIMER3		0x00000020	/* Timer 3 register */
#define IOC_PLP_TIMER4		0x00000024	/* Timer 4 register */

#define IOC_SERIAL_REGS		0x00000030	/* Serial port registers */
#define IOC_SERIAL_REGS_SIZE	0x0000000c

#define IOC_SERIAL_PORT1_CMD	0x00000000	/* Port 1 command transfer */
#define IOC_SERIAL_PORT1_DATA	0x00000004	/* Port 1 data transfer */
#define IOC_SERIAL_PORT2_CMD	0x00000008	/* Port 2 command transfer */
#define IOC_SERIAL_PORT2_DATA	0x0000000c	/* Port 2 data transfer */

#define IOC_KB_REGS		0x00000040	/* Keyboard/mouse registers */
#define IOC_KB_REGS_SIZE	0x00000008

#define IOC_MISC_REGS		0x00000048	/* Misc. IOC regs */
#define IOC_MISC_REGS_SIZE	0x00000034

#define IOC_MISC_GCSEL		0x00000000	/* General control select */
#define IOC_MISC_GCREG		0x00000004	/* General control register */
#define IOC_MISC_PANEL		0x00000008	/* Front Panel register */
/* UNUSED			0x0000000c	*/
#define IOC_MISC_SYSID		0x00000010	/* System ID register */
/* UNUSED			0x00000014	*/
#define IOC_MISC_READ		0x00000018	/* Read register */
/* UNUSED			0x0000001c	*/
#define IOC_MISC_DMASEL		0x00000020	/* DMA select register */
/* UNUSED			0x00000024	*/
#define IOC_MISC_RESET		0x00000028	/* Reset register */
/* UNUSED			0x0000002c	*/
#define IOC_MISC_WRITE		0x00000030	/* Write register */
/* UNUSED			0x00000034	*/

/* The same misc IOC registers as above, but as offsets from IOC base */
#define IOC_GCSEL		0x00000048	/* General control select */
#define IOC_GCREG		0x0000004c	/* General control register */
#define IOC_PANEL		0x00000050	/* Front Panel register */
/* UNUSED			0x00000054	*/
#define IOC_SYSID		0x00000058	/* System ID register */
/* UNUSED			0x0000005c	*/
#define IOC_READ		0x00000060	/* Read register */
/* UNUSED			0x00000064	*/
#define IOC_DMASEL		0x00000068	/* DMA select register */
/* UNUSED			0x0000006c	*/
#define IOC_RESET		0x00000070	/* Reset register */
/* UNUSED			0x00000074	*/
#define IOC_WRITE		0x00000078	/* Write register */
/* UNUSED			0x0000007c	*/

#define IOC_INT3_REGS		0x00000080	/* INT3 interrupt controller */
#define IOC_INT3_REGS_SIZE	0x0000002c

/* See int23regs.h for INT2/3 register layout */

#endif	/* _ARCH_SGIMIPS_HPC_IOCREG_H_ */
