/*	$NetBSD: gtintrreg.h,v 1.1 2003/03/05 22:08:21 matt Exp $	*/

/*
 * Copyright (c) 2002 Allegro Networks, Inc., Wasabi Systems, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Allegro Networks, Inc., and Wasabi Systems, Inc.
 * 4. The name of Allegro Networks, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 * 5. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ALLEGRO NETWORKS, INC. AND
 * WASABI SYSTEMS, INC. ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL EITHER ALLEGRO NETWORKS, INC. OR WASABI SYSTEMS, INC.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * gt64260intr.h: defines for GT-64260 system controller interrupts
 *
 * creation	Sun Jan  7 18:05:59 PST 2001	cliff
 *
 * NOTE:
 *	Galileo GT-64260 manual bit defines assume Little Endian
 *	ordering of bits within bytes, i.e.
 *		bit #0 --> 0x01
 *	vs. Motorola Big Endian bit numbering where
 *		bit #0 --> 0x80
 *	Consequently we define bits in Little Endian format and plan
 *	to swizzle bytes during programmed I/O by using lwbrx/swbrx
 *	to load/store GT-64260 registers.
 */


#ifndef _DISCOVERY_GT64260INTR_H
#define _DISCOVERY_GT64260INTR_H

#define BIT(n)  (1<<(n))


/*
 * GT-64260 Interrupt Controller Register Map
 */
#define ICR_MIC_LO	0xc18	/* main interrupt cause low */
#define ICR_MIC_HI	0xc68	/* main interrupt cause high */
#define ICR_CIM_LO	0xc1c	/* cpu interrupt mask low */
#define ICR_CIM_HI	0xc6c	/* cpu interrupt mask high */
#define ICR_CSC		0xc70	/* cpu select cause */
#define ICR_P0IM_LO	0xc24	/* PCI_0 interrupt mask low */
#define ICR_P0IM_HI	0xc64	/* PCI_0 interrupt mask high */
#define ICR_P0SC	0xc74	/* PCI_0 select cause */
#define ICR_P1IM_LO	0xca4	/* PCI_1 interrupt mask low */
#define ICR_P1IM_HI	0xce4	/* PCI_1 interrupt mask high */
#define ICR_P1SC	0xcf4	/* PCI_1 select cause */
#define ICR_CI0M	0xe60	/* CPU int[0] mask */
#define ICR_CI1M	0xe64	/* CPU int[1] mask */
#define ICR_CI2M	0xe68	/* CPU int[2] mask */
#define ICR_CI3M	0xe6c	/* CPU int[3] mask */

/*
 * IRQs:
 * we define IRQs based on bit number in the
 * ICU_LEN dimensioned hardware portion of the imask_t bit vector
 * which consists of 64 bits of Main Cause and Mask register pairs
 * (ICR_MIC_LO, ICR_MIC_HI and ICR_CIM_LO, ICR_CIM_HI)
 * as well as 32 bits in GPP registers (see intr.h):
 *
 *      IRQs:                             
 *	31.............................0  63.............................32
 *                                     |   |                             |
 *      imask_t index:                 |   |                             |
 *      |                              |   |                             |
 *      ^--------- IM_PIC_LO ----------^   ^------ IM_PIC_HI ------------^
 *                                     |   |                             |
 *	Bitmasks:                      |   |                             |
 *      |                              |   |                             |
 *      ^--------- IML_* --------------^   ^------ IMH_* ----------------^
 *                                     |   |                             |
 *      Registers:                     |   |                             |
 *      |                              |   |                             |
 *      ^--------- ICR_MIC_LO ---------^   ^------ ICR_MIC_HI -----------^
 *      ^--------- ICR_CIM_LO ---------^   ^------ ICR_CIM_HI -----------^
 *
 *      IRQs:                             
 *	95............................64  127............................96
 *                                     |   |                             |
 *      imask_t index:                 |   |                             |
 *      |                              |   |                             |
 *      ^-------- IMASK_GPP  ----------^   ^-----  IMASK_SOFTINT --------^
 *                                     |   |                             |
 *	Bitmasks:                      |   |                             |
 *      |                              |   |                             |
 *      ^--------- GPP_* --------------^   ^------ SIBIT(irq) -----------^
 *                                     |   |                             |
 *      Registers:                     |   |                             |
 *      |                              |   |                             |
 *      ^--- GT_GPP_Interrupt_Cause ---^   ^-------  (none)   -----------^
 *      ^--- GT_GPP_Interrupt_Mask  ---^  
 *
 *
 * Note that GPP interrupts are summarized in the Main Cause Register.
 *
 * Some IRQs are "resvered" undefined due to gaps in HW register utilization.
 */
#define IRQ_DEV		1	/* device interface interrupt */
#define IRQ_DMA		2	/* DMA addres error interrupt */
#define IRQ_CPU		3	/* CPU interface interrupt */
#define IRQ_IDMA0_1	4	/* IDMA ch. 0..1 complete interrupt */
#define IRQ_IDMA2_3	5	/* IDMA ch. 2..3 complete interrupt */
#define IRQ_IDMA4_5	6	/* IDMA ch. 4..5 complete interrupt */
#define IRQ_IDMA6_7	7	/* IDMA ch. 6..7 complete interrupt */
#define IRQ_TIME0_1	8	/* Timer 0..1 interrupt */
#define IRQ_TIME2_3	9	/* Timer 2..3 interrupt */
#define IRQ_TIME4_5	10	/* Timer 4..5 interrupt */
#define IRQ_TIME6_7	11	/* Timer 6..7 interrupt */
#define IRQ_PCI0_0	12	/* PCI 0 interrupt 0 summary */
#define IRQ_PCI0_1	13	/* PCI 0 interrupt 1 summary */
#define IRQ_PCI0_2	14	/* PCI 0 interrupt 2 summary */
#define IRQ_PCI0_3	15	/* PCI 0 interrupt 3 summary */
#define IRQ_PCI1_0	16	/* PCI 1 interrupt 0 summary */
#define IRQ_ECC		17	/* ECC error interrupt */
#define IRQ_PCI1_1	18	/* PCI 1 interrupt 1 summary */
#define IRQ_PCI1_2	19	/* PCI 1 interrupt 2 summary */
#define IRQ_PCI1_3	20	/* PCI 1 interrupt 3 summary */
#define IRQ_PCI0OUT_LO	21	/* PCI 0 outbound interrupt summary */
#define IRQ_PCI0OUT_HI	22	/* PCI 0 outbound interrupt summary */
#define IRQ_PCI1OUT_LO	23	/* PCI 1 outbound interrupt summary */
#define IRQ_PCI1OUT_HI	24	/* PCI 1 outbound interrupt summary */
#define IRQ_PCI0IN_LO	26	/* PCI 0 inbound interrupt summary */
#define IRQ_PCI0IN_HI	27	/* PCI 0 inbound interrupt summary */
#define IRQ_PCI1IN_LO	28	/* PCI 1 inbound interrupt summary */
#define IRQ_PCI1IN_HI	29	/* PCI 1 inbound interrupt summary */
#define IRQ_ETH0	(32+0)	/* Ethernet controller 0 interrupt */
#define IRQ_ETH1	(32+1)	/* Ethernet controller 1 interrupt */
#define IRQ_ETH2	(32+2)	/* Ethernet controller 2 interrupt */
#define IRQ_SDMA	(32+4)	/* SDMA interrupt */
#define IRQ_I2C		(32+5)	/* I2C interrupt */
#define IRQ_BRG		(32+7)	/* Baud Rate Generator interrupt */
#define IRQ_MPSC0	(32+8)	/* MPSC 0 interrupt */
#define IRQ_MPSC1	(32+10)	/* MPSC 1 interrupt */
#define IRQ_COMM	(32+11)	/* Comm unit interrupt */
#define IRQ_GPP7_0	(32+24)	/* GPP[7..0] interrupt */
#define IRQ_GPP15_8	(32+25)	/* GPP[15..8] interrupt */
#define IRQ_GPP23_16	(32+26)	/* GPP[23..16] interrupt */
#define IRQ_GPP31_24	(32+27)	/* GPP[31..24] interrupt */

/*
 * low word interrupt mask register bits
 */
#define IML_SUM		BIT(0)
#define IML_DEV		BIT(IRQ_DEV)
#define IML_DMA		BIT(IRQ_DMA)
#define IML_CPU		BIT(IRQ_CPU)
#define IML_IDMA0_1	BIT(IRQ_IDMA0_1)
#define IML_IDMA2_3	BIT(IRQ_IDMA2_3)
#define IML_IDMA4_5	BIT(IRQ_IDMA4_5)
#define IML_IDMA6_7	BIT(IRQ_IDMA6_7)
#define IML_TIME0_1	BIT(IRQ_TIME0_1)
#define IML_TIME2_3	BIT(IRQ_TIME2_3)
#define IML_TIME4_5	BIT(IRQ_TIME4_5)
#define IML_TIME6_7	BIT(IRQ_TIME6_7)
#define IML_PCI0_0	BIT(IRQ_PCI0_0)
#define IML_PCI0_1	BIT(IRQ_PCI0_1)
#define IML_PCI0_2	BIT(IRQ_PCI0_2)
#define IML_PCI0_3	BIT(IRQ_PCI0_3)
#define IML_PCI1_0	BIT(IRQ_PCI1_0)
#define IML_ECC		BIT(IRQ_ECC)
#define IML_PCI1_1	BIT(IRQ_PCI1_1)
#define IML_PCI1_2	BIT(IRQ_PCI1_2)
#define IML_PCI1_3	BIT(IRQ_PCI1_3)
#define IML_PCI0OUT_LO	BIT(IRQ_PCI0OUT_LO)
#define IML_PCI0OUT_HI	BIT(IRQ_PCI0OUT_HI)
#define IML_PCI1OUT_LO	BIT(IRQ_PCI1OUT_LO)
#define IML_PCI1OUT_HI	BIT(IRQ_PCI1OUT_HI)
#define IML_PCI0IN_LO	BIT(IRQ_PCI0IN_LO)
#define IML_PCI0IN_HI	BIT(IRQ_PCI0IN_HI)
#define IML_PCI1IN_LO	BIT(IRQ_PCI1IN_LO)
#define IML_PCI1IN_HI	BIT(IRQ_PCI1IN_HI)
#define IML_RES		(BIT(25)|BIT(30)|BIT(31))

/*
 * high word interrupt mask register bits
 */
#define IMH_ETH0	BIT(IRQ_ETH0-32)
#define IMH_ETH1	BIT(IRQ_ETH1-32)
#define IMH_ETH2	BIT(IRQ_ETH2-32)
#define IMH_SDMA	BIT(IRQ_SDMA-32)
#define IMH_I2C		BIT(IRQ_I2C-32)
#define IMH_BRG		BIT(IRQ_BRG-32)
#define IMH_MPSC0	BIT(IRQ_MPSC0-32)
#define IMH_MPSC1	BIT(IRQ_MPSC1-32)
#define IMH_COMM	BIT(IRQ_COMM-32)
#define IMH_GPP7_0	BIT(IRQ_GPP7_0-32)
#define IMH_GPP15_8	BIT(IRQ_GPP15_8-32)
#define IMH_GPP23_16	BIT(IRQ_GPP23_16-32)
#define IMH_GPP31_24	BIT(IRQ_GPP31_24-32)
#define IMH_GPP_SUM	(IMH_GPP7_0|IMH_GPP15_8|IMH_GPP23_16|IMH_GPP31_24)
#define IMH_RES		(BIT(3) |BIT(6) |BIT(9) |BIT(12)|BIT(13)|BIT(14) \
			|BIT(15)|BIT(16)|BIT(17)|BIT(18)|BIT(19)|BIT(20) \
			|BIT(21)|BIT(22)|BIT(23)|BIT(28)|BIT(29)|BIT(30) \
			|BIT(31))

/*
 * ICR_CSC "Select Cause" register bits
 */
#define CSC_SEL		BIT(30)		/* HI/LO select */
#define CSC_STAT	BIT(31)		/* ? "irq active" : "irq none"  */
#define CSC_CAUSE	~(CSC_SEL|CSC_STAT)


/*
 * CPU Int[n] Mask bit(s)
 */
#define CPUINT_SEL	0x80000000	/* HI/LO select */

#endif	/*  _DISCOVERY_GT64260INTR_H */
