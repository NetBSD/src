/* $NetBSD: ascreg.h,v 1.6 2006/08/20 19:26:52 bjh21 Exp $ */

/*
 * Copyright (c) 1982, 1990 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from:ahscreg.h,v 1.2 1994/10/26 02:02:46
 */

/*
 * Copyright (c) 1996 Mark Brinicombe
 * Copyright (c) 1994 Christian E. Hopps
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from:ahscreg.h,v 1.2 1994/10/26 02:02:46
 */

#ifndef _ASCREG_H_
#define _ASCREG_H_

#define v_char		volatile char
#define	v_int		volatile int
#define vu_char		volatile u_char
#define vu_short	volatile u_short
#define vu_int		volatile u_int

/* Addresses relative to podule base */

#define ASC_INTSTATUS	0x2000
#define ASC_CLRINT	0x2000
#define ASC_PAGEREG	0x3000

/* Addresses relative to module base */

#define ASC_DMAC		0x3000
#define ASC_SBIC		0x2000
#define ASC_SRAM		0x0000

#define ASC_SBIC_SPACE		8

#define ASC_SRAM_BLKSIZE	0x1000

#define IS_IRQREQ		0x01
#define IS_DMAC_IRQ		0x02
#define IS_SBIC_IRQ		0x08

#if 0

/* SBIC status codes */

#define SBIC_ResetOk	0x00
#define SBIC_ResetAFOk	0x01

/* DMAC constants */

#define DMAC_Bits		0x01
#define DMAC_Ctrl1		0x60
#define DMAC_Ctrl2		0x01
#define DMAC_CLEAR_MASK		0x0E
#define DMAC_SET_MASK		0x0F
#define DMAC_DMA_RD_MODE	0x04
#define DMAC_DMA_WR_MODE	0x08

/* DMAC registers */

#define DMAC_INITIALISE	0x0000	/* WO ---- ---- ---- ---- ---- ----  16B  RES */
#define DMAC_CHANNEL	0x0200	/* R  ---- ---- ---- BASE SEL3 SEL2 SEL1 SEL0 */
				/* W  ---- ---- ---- ---- ---- BASE *SELECT** */
#define DMAC_TXCNTLO	0x0004	/* RW   C7   C6   C5   C4   C3   C2   C1   C0 */
#define DMAC_TXCNTHI	0x0204	/* RW  C15  C14  C13  C12  C11  C10   C9   C8 */
#define DMAC_TXADRLO	0x0008	/* RW   A7   A6   A5   A4   A3   A2   A1   A0 */
#define DMAC_TXADRMD	0x0208	/* RW  A15  A14  A13  A12  A11  A10   A9   A8 */
#define DMAC_TXADRHI	0x000C	/* RW  A23  A22  A21  A20  A19  A18  A17  A16 */
#define DMAC_DEVCON1	0x0010	/* RW  AKL  RQL  EXW  ROT  CMP DDMA AHLD  MTM */
#define DMAC_DEVCON2	0x0210	/* RW ---- ---- ---- ---- ---- ----  WEV BHLD */
#define DMAC_MODECON	0x0014	/* RW **TMODE** ADIR AUTI **TDIR*** ---- WORD */
#define DMAC_STATUS	0x0214	/* RO  RQ3  RQ2  RQ1  RQ0  TC3  TC2  TC1  TC0 */
#if 0
templo  = dmac + 0x0018;/*    RO   T7   T6   T5   T4   T3   T2   T1   T0 */
temphi  = dmac + 0x0218;/*    RO  T15  T14  T13  T12  T11  T10   T9   T8 */
#endif
#define DMAC_REQREG	0x001C	/* RW ---- ---- ---- ---- SRQ3 SRQ2 SRQ1 SRQ0 */
#define DMAC_MASKREG	0x021C	/* RW ---- ---- ---- ----   M3   M2   M1   M0 */

#endif
#endif /* _ASCREG_H_ */
