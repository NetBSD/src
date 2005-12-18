/*	$NetBSD: hd64461reg.h,v 1.3 2005/12/18 21:47:10 uwe Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#ifndef _HPCSH_DEV_HD64461REG_H_
#define _HPCSH_DEV_HD64461REG_H_

#define SH3_AREA4_BASE					0xb0000000
#define SH3_AREA5_BASE					0xb4000000
#define SH3_AREA6_BASE					0xb8000000

/* HD64461 Address mapping (mapped to SH3 memory space) */

/* SH3 Area 4 (Register, Framebuffer) */
#define HD64461_REGBASE				SH3_AREA4_BASE
#define HD64461_REGSIZE				0x02000000
#define HD64461_FBBASE				(SH3_AREA4_BASE + 0x02000000)
#define HD64461_FBSIZE				0x02000000
#define HD64461_FBPAGESIZE			0x1000

/* SH3 Area 5 (PCC1 memory space) */
#define HD64461_PCC1_MEMBASE			SH3_AREA5_BASE
#define HD64461_PCC1_MEMSIZE			0x02000000

/* SH3 Area 6 (PCC0 memory, I/O space*/
#define HD64461_PCC0_MEMBASE			SH3_AREA6_BASE
#define HD64461_PCC0_MEMSIZE			0x02000000
#define HD64461_PCC0_IOBASE			(SH3_AREA6_BASE + 0x02000000)
#define HD64461_PCC0_IOSIZE			0x02000000

/*
 * Register mapping.
 */
#define HD64461_SYSTEM_REGBASE				0xb0000000
#define HD64461_SYSTEM_REGSIZE				0x00001000

#define HD64461_LCDC_REGBASE				0xb0001000
#define HD64461_LCDC_REGSIZE				0x00001000

#define HD64461_PCMCIA_REGBASE				0xb0002000
#define HD64461_PCMCIA_REGSIZE				0x00001000

#define HD64461_AFE_REGBASE				0xb0003000
#define HD64461_AFE_REGSIZE				0x00001000

#define HD64461_GPIO_REGBASE				0xb0004000
#define HD64461_GPIO_REGSIZE				0x00001000

#define HD64461_INTC_REGBASE				0xb0005000
#define HD64461_INTC_REGSIZE				0x00001000

#define HD64461_TIMER_REGBASE				0xb0006000
#define HD64461_TIMER_REGSIZE				0x00001000

#define HD64461_IRDA_REGBASE				0xb0007000
#define HD64461_IRDA_REGSIZE				0x00001000

#define HD64461_UART_REGBASE				0xb0008000
#define HD64461_UART_REGSIZE				0x00001000

/* 
 * System Configuration Register and STANBY Mode
 */
/* Stanby Control Register */
#define HD64461_SYSSTBCR_REG16				0xb0000000
#define HD64461_SYSSTBCR_CKIO_STBY		0x2000
#define HD64461_SYSSTBCR_SAFECKE_IST		0x1000
#define HD64461_SYSSTBCR_SLCKE_IST		0x0800
#define HD64461_SYSSTBCR_SAFECKE_OST		0x0400
#define HD64461_SYSSTBCR_SLCKE_OST		0x0200
#define HD64461_SYSSTBCR_SMIAST			0x0100
#define HD64461_SYSSTBCR_SLCDST			0x0080
#define HD64461_SYSSTBCR_SPC0ST			0x0040
#define HD64461_SYSSTBCR_SPC1ST			0x0020
#define HD64461_SYSSTBCR_SAFEST			0x0010
#define HD64461_SYSSTBCR_STM0ST			0x0008
#define HD64461_SYSSTBCR_STM1ST			0x0004
#define HD64461_SYSSTBCR_SIRST			0x0002
#define HD64461_SYSSTBCR_SURTSD			0x0001

/* System Configuration Register */
#define HD64461_SYSSYSCR_REG16				0xb0000002
#define HD64461_SYSSYSCR_SCPU_BUS_IGAT		0x2000
#define HD64461_SYSSYSCR_SPTA_IR		0x0080
#define HD64461_SYSSYSCR_SPTA_TM		0x0040
#define HD64461_SYSSYSCR_SPTB_UR		0x0020
#define HD64461_SYSSYSCR_WAIT_CTL_SEL		0x0010
#define HD64461_SYSSYSCR_SMODE1			0x0002
#define HD64461_SYSSYSCR_SMODE0			0x0001

/* CPU Data Bus Control Register */
#define HD64461_SYSSCPUCR_REG16				0xb0000004
#define HD64461_SYSSCPUCR_SPDSTOF		0x8000
#define HD64461_SYSSCPUCR_SPDSTIG		0x4000
#define HD64461_SYSSCPUCR_SPCSTOF		0x2000
#define HD64461_SYSSCPUCR_SPCSTIG		0x1000
#define HD64461_SYSSCPUCR_SPBSTOF		0x0800
#define HD64461_SYSSCPUCR_SPBSTIG		0x0400
#define HD64461_SYSSCPUCR_SPASTOF		0x0200
#define HD64461_SYSSCPUCR_SPASTIG		0x0100
#define HD64461_SYSSCPUCR_SLCDSTIG		0x0080
#define HD64461_SYSSCPUCR_SCPU_CS56_EP		0x0040
#define HD64461_SYSSCPUCR_SCPU_CMD_EP		0x0020
#define HD64461_SYSSCPUCR_SCPU_ADDR_EP		0x0010
#define HD64461_SYSSCPUCR_SCPDPU		0x0008
#define HD64461_SYSSCPUCR_SCPU_A2319_EP		0x0001

#endif /* !_HPCSH_DEV_HD64461REG_H_ */
