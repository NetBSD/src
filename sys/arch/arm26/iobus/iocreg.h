/* $NetBSD: iocreg.h,v 1.2 2002/03/07 23:16:44 bjh21 Exp $ */

/*
 * Copyright (c) 1997 Ben Harris.
 * Copyright (c) 1994 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * iocreg.h - Acorn IOC (Albion/VC2311) registers
 *
 * Based on arch/arm32/include/iomd.h 1.7:
 * Created      : 18/09/94
 * Based on kate/display/iomd.h
 */

#ifndef _ARM26_IOCREG_H
#define _ARM26_IOCREG_H

/*
 * These addresses are intended to be inputs to bus_space functions,
 * and as such bits 0-4 here map to A[2:6] on the IOC.
 */

/* 			        Read		Write		*/
#define IOC_CTL		0x00 /* Control		Control		*/
#define IOC_KBDDAT	0x01 /* Kbd receive	Kbd send	*/

#define IOC_IRQSTA	0x04 /* IRQ status A	-		*/
#define IOC_IRQRQA	0x05 /* IRQ request A	IRQ clear	*/
#define IOC_IRQMSKA	0x06 /* IRQ mask A	IRQ mask A	*/

#define IOC_IRQSTB	0x08 /* IRQ status B	-		*/
#define IOC_IRQRQB	0x09 /* IRQ request B	-		*/
#define IOC_IRQMSKB	0x0a /* IRQ mask B	IRQ mask B	*/

#define IOC_FIQST	0x0c /* FIQ status	-		*/
#define IOC_FIQRQ	0x0d /* FIQ request	-		*/
#define IOC_FIQMSK	0x0e /* FIQ mask	FIQ mask	*/

#define IOC_T0LOW	0x10 /* T0 count low	T0 latch low	*/
#define IOC_T0HIGH	0x11 /* T0 count high	T0 latch high	*/
#define IOC_T0GO	0x12 /* -		T0 go command	*/
#define IOC_T0LATCH	0x13 /* -		T0 latch command */

#define IOC_T1LOW	0x14 /* T1 count low	T1 latch low	*/
#define IOC_T1HIGH	0x15 /* T1 count high	T1 latch high	*/
#define IOC_T1GO	0x16 /* -		T1 go command	*/
#define IOC_T1LATCH	0x17 /* -		T1 latch command */

#define IOC_T2LOW	0x18 /* T2 count low	T2 latch low	*/
#define IOC_T2HIGH	0x19 /* T2 count high	T2 latch high	*/
#define IOC_T2GO	0x1a /* -		T2 go command	*/
#define IOC_T2LATCH	0x1b /* -		T2 latch command */

#define IOC_T3LOW	0x1c /* T3 count low	T3 latch low	*/
#define IOC_T3HIGH	0x1d /* T3 count high	T3 latch high	*/
#define IOC_T3GO	0x1e /* -		T3 go command	*/
#define IOC_T3LATCH	0x1f /* -		T3 latch command */

#define IOC_SIZE	0x20 /* Total amount of bus space used by the IOC */

/* Control register bits (should most of these be defined here?) */
#define IOC_CTL_SDA		0x01	/* IIC serial data	*/
#define IOC_CTL_SCL		0x02	/* IIC serial clock	*/
#define IOC_CTL_FLYBACK		0x80	/* Video flyback	*/

/* Archimdes machines only */
#define IOC_CTL_FDREADY		0x04	/* Floppy disc ready	*/
#define IOC_CTL_SMUTE		0x20	/* Speaker mute		*/
#define IOC_CTL_LPTACK		0x40	/* Printer acknowledge	*/

/* IOEB machines only */
#define IOC_CTL_FDDEN		0x04	/* Floppy disc media density */
#define IOC_CTL_SSN		0x08	/* Electronic machine number */
#define IOC_CTL_NINDEX		0x40	/* Floppy disc INDEX* signal */

/* Internal control register bits */
/* These give the current state of the edge-sensitive IRQ lines */
#define IOC_CTL_NIF	0x40	/* Set if IF* is high (IRQ 2) */
#define IOC_CTL_IR	0x80	/* Set if IR  is high (IRQ 3) */

/* IOC definitions of interrupt sources (independent of machine) */
/* Hmm Interrupt numbers or masks?  Numbers for now. */
#define IOC_IRQ_IL6	0  /* Active low input */
#define IOC_IRQ_IL7	1  /* Active low input */
#define IOC_IRQ_IF	2  /* Falling edge-triggered */
#define IOC_IRQ_IR	3  /* Rising edge-triggered */
#define IOC_IRQ_POR	4  /* Power-on reset */
#define IOC_IRQ_TM0	5  /* Timer 0 reloaded */
#define IOC_IRQ_TM1	6  /* Timer 1 reloaded */
#define IOC_IRQ_1	7  /* Always interrupt */
#define IOC_IRQ_IL0	8  /* Active low input */
#define IOC_IRQ_IL1	9  /* Active low input */
#define IOC_IRQ_IL2	10 /* Active low input */
#define IOC_IRQ_IL3	11 /* Active low input */
#define IOC_IRQ_IL4	12 /* Active low input */
#define IOC_IRQ_IL5	13 /* Active low input */
#define IOC_IRQ_STX	14 /* KART transmit finished */
#define IOC_IRQ_SRX	15 /* KART receive finished */
#define IOC_IRQA_BIT(n) ((n) < 8 ? 1 << (n) : 0)
#define IOC_IRQB_BIT(n) ((n) < 8 ? 0 : 1 << ((n) - 8))

#define IOC_IRQ_CLEARABLE_MASK 0x7c

/* FIQ sources */
#define IOC_FIQ_FH0	0  /* Active high input */
#define IOC_FIQ_FH1	1  /* Active high input */
#define IOC_FIQ_FL	2  /* Active low input */
#define IOC_FIQ_C3	3  /* Active low input (also ctl reg) */
#define IOC_FIQ_C4	4  /* Active low input (also ctl reg) */
#define IOC_FIQ_C5	5  /* Active low input (also ctl reg) */
#define IOC_FIQ_IL0	6  /* Active low input (also IRQ) */
#define IOC_FIQ_1	7  /* Always interrupt */
#define IOC_FIQ_BIT(n)	(1 << (n))

/* Cycle types */
#define IOC_TYPE_SLOW	0
#define IOC_TYPE_MEDIUM	1
#define IOC_TYPE_FAST	2
#define IOC_TYPE_SYNC	3

/* Counter decrement rate */
#define IOC_TIMER_RATE	2000000 /* 2 MHz */

/* Archimedes-specific defines */
#define IOC_TYPE_SHIFT (19 - 2) /* A[20:19] -> T[1:0] */
#define IOC_BANK_SHIFT (16 - 2) /* A[18:16] -> B[2:0] */

#endif
