/*	$NetBSD: intcreg.h,v 1.1 2002/07/05 13:31:53 scw Exp $	*/

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

#ifndef _SH5_INTCREG_H
#define _SH5_INTCREG_H

/*
 * Interrupt Control Register: SET
 *
 * 1 <= (n) <= 15
 */
#define	INTC_REG_ICR_SET		0x00
#define	 INTC_ICR_SET_IRL_MODE_INDEP	1
#define  INTC_ICR_SET_ICRn(n)		(2 << (n))

/*
 * Interrupt Control Register: CLEAR
 *
 * 1 <= (n) <= 15
 */
#define	INTC_REG_ICR_CLEAR		0x08
#define	 INTC_ICR_CLEAR_IRL_MODE_LEVEL	1
#define  INTC_ICR_CLEAR_ICRn(n)		(2 << (n))

/*
 * Interrupt Priorities Register
 *
 * 0 <= (n) <= 63
 */
#define INTC_REG_INTPRI(n)		(0x10 + ((n) & 0x38))
#define  INTC_INTPRI_MASK		0x0f
#define  INTC_INTPRI_SHIFT(n)		(((n) & 0x7) * 4)

/*
 * Interrupt Enable Registers
 *
 * 0 <= (n) <= 63
 */
#define INTC_REG_INTENB(n)		(0x70 + (8 * ((n) / 32)))
#define  INTC_INTENB_BIT(n)		(1 << ((n) & 0x1f))
#define  INTC_INTENB_ALL		0xffffffffu

/*
 * Interrupt Disable Registers
 *
 * 0 <= (n) <= 63
 */
#define INTC_REG_INTDISB(n)		(0x80 + (8 * ((n) / 32)))
#define  INTC_INTDISB_BIT(n)		(1 << ((n) & 0x1f))
#define  INTC_INTDISB_ALL		0xffffffffu

/*
 * Interrupt Source Status Registers
 *
 * 0 <= (n) <= 63
 */
#define INTC_REG_INTSRC(n)		(0x50 + (8 * ((n) / 32)))
#define  INTC_INTSRC_BIT(n)		(1 << ((n) & 0x1f))

/*
 * Interrupt Request Status Registers
 *
 * 0 <= (n) <= 63
 */
#define INTC_REG_INTREQ(n)		(0x60 + (8 * ((n) / 32)))
#define  INTC_INTREQ_BIT(n)		(1 << ((n) & 0x1f))

/* Size of INTC register area */
#define	INTC_REG_SIZE			0x90

/* How many interrupts */
#define	INTC_N_IRQS			64

/*
 * Given a priority, evaluate the corresponding INTEVT value for
 * level encoded "IRL" pin interrupts.
 */
#define	INTC_IRL_PRI2INTEVT(p)		(0x200 + ((15 - (p)) * 0x20))

/*
 * The opposite of the above macro...
 */
#define	INTC_IRL_INTEVT2PRI(e)		(15 - (((e) - 0x200) / 0x20))

/*
 * Non-level encoded IRL INTEVT/INUM values
 */
#define	INTC_INTEVT_IRL0		0x240
#define	INTC_INUM_IRL0			0
#define	INTC_INTEVT_IRL1		0x2a0
#define	INTC_INUM_IRL1			1
#define	INTC_INTEVT_IRL2		0x300
#define	INTC_INUM_IRL2			2
#define	INTC_INTEVT_IRL3		0x360
#define	INTC_INUM_IRL3			3

/*
 * INTEVT/INUM values for on-chip resources
 */
#define	INTC_INTEVT_PCI_INTA		0x800
#define	INTC_INUM_PCI_INTA		4
#define	INTC_INTEVT_PCI_INTB		0x820
#define	INTC_INUM_PCI_INTB		5
#define	INTC_INTEVT_PCI_INTC		0x840
#define	INTC_INUM_PCI_INTC		6
#define	INTC_INTEVT_PCI_INTD		0x860
#define	INTC_INUM_PCI_INTD		7
#define	INTC_INTEVT_PCI_SERR		0xa00
#define	INTC_INUM_PCI_SERR		12
#define	INTC_INTEVT_PCI_ERR		0xa20
#define	INTC_INUM_PCI_ERR		13
#define	INTC_INTEVT_PCI_PWR3		0xa40
#define	INTC_INUM_PCI_PWR3		14
#define	INTC_INTEVT_PCI_PWR2		0xa60
#define	INTC_INUM_PCI_PWR2		15
#define	INTC_INTEVT_PCI_PWR1		0xa80
#define	INTC_INUM_PCI_PWR1		16
#define	INTC_INTEVT_PCI_PWR0		0xaa0
#define	INTC_INUM_PCI_PWR0		17

#define	INTC_INTEVT_DMAC_DMTE0		0x640
#define	INTC_INUM_DMAC_DMTE0		18
#define	INTC_INTEVT_DMAC_DMTE1		0x660
#define	INTC_INUM_DMAC_DMTE1		19
#define	INTC_INTEVT_DMAC_DMTE2		0x680
#define	INTC_INUM_DMAC_DMTE2		20
#define	INTC_INTEVT_DMAC_DMTE3		0x6a0
#define	INTC_INUM_DMAC_DMTE3		21
#define	INTC_INTEVT_DMAC_DAERR		0x6c0
#define	INTC_INUM_DMAC_DAERR		22

#define	INTC_INTEVT_TMU_TUNI0		0x400
#define	INTC_INUM_TMU_TUNI0		32
#define	INTC_INTEVT_TMU_TUNI1		0x420
#define	INTC_INUM_TMU_TUNI1		33
#define	INTC_INTEVT_TMU_TUNI2		0x440
#define	INTC_INUM_TMU_TUNI2		34
#define	INTC_INTEVT_TMU_TICPI2		0x460
#define	INTC_INUM_TMU_TICPI2		35

#define	INTC_INTEVT_RTC_ATI		0x480
#define	INTC_INUM_RTC_ATI		36
#define	INTC_INTEVT_RTC_PRI		0x4a0
#define	INTC_INUM_RTC_PRI		37
#define	INTC_INTEVT_RTC_CUI		0x4c0
#define	INTC_INUM_RTC_CUI		38

#define	INTC_INTEVT_SCIF_ERI		0x700
#define	INTC_INUM_SCIF_ERI		39
#define	INTC_INTEVT_SCIF_RXI		0x720
#define	INTC_INUM_SCIF_RXI		40
#define	INTC_INTEVT_SCIF_BRI		0x740
#define	INTC_INUM_SCIF_BRI		41
#define	INTC_INTEVT_SCIF_TXI		0x760
#define	INTC_INUM_SCIF_TXI		42

#define	INTC_INTEVT_WDT_ITI		0x560
#define	INTC_INUM_WDT_ITI		63

#endif /* _SH5_INTCREG_H */
