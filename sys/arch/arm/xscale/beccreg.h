/*	$NetBSD: beccreg.h,v 1.1 2003/01/25 01:57:20 thorpej Exp $	*/

/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
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

#ifndef _BECCREG_H_
#define	_BECCREG_H_

/*
 * Register definitions for the ADI Engineering Big Endian Companion
 * Chip for the Intel i80200.
 */

/* Revision codes */

#define	BECC_REV_V7		0x00		/* rev <= 7 */
#define	BECC_REV_V8		0x01		/* rev 8 */
#define	BECC_REV_V9		0x02		/* rev >= 9 */

/* Memory Map */

#define	BECC_REG_BASE		0x04000000
#define	BECC_REG_SIZE		0x01000000	/* 16M */

#define	BECC_PCI_CONF_BASE	0x08000000
#define	BECC_PCI_CONF_SIZE	0x02000000	/* 32M */

#define	BECC_PCI_IO_BASE	0x0a000000
#define	BECC_PCI_IO_SIZE	0x02000000	/* 32M */

#define	BECC_PCI_MEM1_BASE	0x0c000000
#define	BECC_PCI_MEM1_SIZE	0x02000000	/* 32M */

#define	BECC_PCI_MEM2_BASE	0x0e000000
#define	BECC_PCI_MEM2_SIZE	0x02000000	/* 32M */

#define	BECC_SDRAM_BASE		0xc0000000

/* Peripheral clock is 33.3MHz */
#define	BECC_PERIPH_CLOCK	33300000

/* BECC registers; offsets from BECC_REG_BASE */

#define	BECC_PSISR		0x0000	/* PCI slave interrupt status */
#define	PSISR_SERR		(1U << 4)	/* system error */
#define	PSISR_PERR		(1U << 9)	/* parity error */
#define	PSISR_IFU		(1U << 16)	/* inbound FIFO uflow */
#define	PSISR_IFO		(1U << 17)	/* inbound FIFO oflow */
#define	PSISR_OFU		(1U << 18)	/* outbound FIFO uflow */
#define	PSISR_OFO		(1U << 19)	/* outbound FIFO oflow */

#define	BECC_PSTR0		0x0010	/* PCI slave translation window #0 */
#define	BECC_PSTR1		0x0018	/* PCI slave translation window #1 */
#define	PSTRx_ADDRMASK		(3U << 25)	/* address mask */
#define	PSTRx_BEE		(1U << 0)	/* big-endian enable */
#define	BECC_PSTR2		0x0020	/* PCI slave translation window #2 */
#define	PSTR2_ADDRMASK		(0U)		/* address mask (all SDRAM) */

#define	BECC_PMISR		0x0100	/* PCI master interrupt status */
#define	PMISR_PE		(1U << 0)	/* parity error */
#define	PMISR_TA		(1U << 2)	/* target abort */
#define	PMISR_MA		(1U << 3)	/* master abort */
#define	PMISR_IFU		(1U << 16)	/* inbound FIFO uflow */
#define	PMISR_IFO		(1U << 17)	/* inbound FIFO oflow */
#define	PMISR_OFU		(1U << 18)	/* outbound FIFO uflow */
#define	PMISR_OFO		(1U << 19)	/* outbound FIFO oflow */

#define	BECC_IDSEL_BIT		11	/* first device on PCI bus */

#define	BECC_POMR1		0x0110	/* PCI outbound memory window #1 */
#define	BECC_POMR2		0x0114	/* PCI outbound memory window #2 */
#define	POMRx_ADDRMASK		0xfe000000	/* address mask */
#define	POMRx_BEE		(1U << 2)	/* big-endian enable */
#define	POMRx_F32		(1U << 3)	/* force 32-bit transfer */
#define	POMRx_BO(x)		(x)		/* busrt order (MBZ) */
#define	BECC_POMR3		0x0130	/* PCI outbound memory window #3 */
#define	POMR3_ADDRMASK		0xc0000000	/* address mask */

#define	BECC_POIR		0x0118	/* PCI outbound I/O window */
#define	POIR_ADDRMASK		0xfe000000	/* address mask */
#define	POIR_BEE		(1U << 2)	/* big-endian enable */

#define	BECC_POCR		0x0120	/* PCI outbound config window */
#define	POCR_BEE		(1U << 2)	/* big-endian enable */
#define	POCR_TYPE		(1U << 0)	/* for type 1 cycles */

#define	BECC_DMACR		0x0200	/* DMA control register */
#define	DMACR_CE		(1U << 0)	/* channel enable */

#define	BECC_DMASR		0x0204	/* DMA status register */
#define	DMASR_PEF		(1U << 0)	/* PCI parity error */
#define	DMASR_PTA		(1U << 2)	/* PCI target abort */
#define	DMASR_PMA		(1U << 3)	/* PCI master abort */
#define	DMASR_EOTI		(1U << 8)	/* end of transfer interrupt */
#define	DMASR_CA		(1U << 9)	/* channel active */
#define	DMASR_IFU		(1U << 16)	/* inbound FIFO uflow */
#define	DMASR_IFO		(1U << 17)	/* inbound FIFO oflow */
#define	DMASR_OFU		(1U << 18)	/* outbound FIFO uflow */
#define	DMASR_OFO		(1U << 19)	/* outbound FIFO oflow */

#define	BECC_DMAPCIAR		0x0210	/* DMA PCI address */

#define	BECC_DMALAR		0x021c	/* DMA local address */

#define	BECC_DMABCR		0x0220	/* DMA byte count */

#define	BECC_DMADCR		0x0224	/* DMA descriptor control */
#define	DMADCR_F32		(1U << 6)	/* force 32-bit */
#define	DMADCR_BEE		(1U << 5)	/* big-endian enable */
#define	DMADCR_PCICMD(x)	(x)		/* PCI command */

#define	PCICMD_MR		0x6	/* memory read */
#define	PCICMD_MRL		0xe	/* memory read line */
#define	PCICMD_MRM		0xc	/* memory read multiple */
#define	PCICMD_MW		0x7	/* memory write */

#define	BECC_TSCRA		0x0300	/* Timer status/control A */
#define	BECC_TSCRB		0x0320	/* Timer status/control B */
#define	TSCRx_TE		(1U << 0)	/* timer enable */
#define	TSCRx_CM		(1U << 1)	/* continuous mode */
#define	TSCRx_TIF		(1U << 9)	/* timer interrupt flag */

#define	BECC_TPRA		0x0304	/* Timer preload A */
#define	BECC_TPRB		0x0324	/* Timer preload B */

#define	BECC_TCVRA		0x0308	/* Timer current value A */
#define	BECC_TCVRB		0x0328	/* Timer current value B */

#define	BECC_ICSR		0x0400	/* Interrupt control/status */
#define	BECC_ICMR		0x0404	/* Interrupt mask */
#define	BECC_ICSTR		0x0408	/* Interrupt steer */

#define	ICU_SOFT		0	/* software interrupt */
#define	ICU_TIMERA		1	/* timer A */
#define	ICU_TIMERB		2	/* timer B */
#define	ICU_DIAGERR		7	/* diagnostic error */
#define	ICU_DMA_EOT		8	/* DMA end-of-transfer */
#define	ICU_DMA_PEF		9	/* DMA parity error */
#define	ICU_DMA_TA		10	/* DMA target abort */
#define	ICU_DMA_MA		11	/* DMA master abort */
#define	ICU_PCI_PERR		16	/* PCI parity error */
#define	ICU_PCI_SERR		19	/* PCI system error */
#define	ICU_PCI_POAPEI		20	/* PCI outbound ATU parity error */
#define	ICU_PCI_POATAI		21	/* PCI outbound ATU target abort */
#define	ICU_PCI_POAMAI		22	/* PCI outbound ATU master abort */
#define	ICU_UARTA		24	/* UART A */
#define	ICU_UARTB		25	/* UART B */
#define	ICU_PCI_INTA		26	/* PCI INTA# */
#define	ICU_PCI_INTB		27	/* PCI INTB# */
#define	ICU_PCI_INTC		28	/* PCI INTC# */
#define	ICU_PCI_INTD		29	/* PCI INTD# */
#define	ICU_PUSHBUTTON		30	/* push button pulse */

#define	ICU_RESERVED_MASK	((1U << 3) | (1U << 4) | (1U << 5) |	\
				 (1U << 6) | (1U << 12) | (1U << 13) |	\
				 (1U << 14) | (1U << 15) | (1U << 17) |	\
				 (1U << 18) | (1U << 23) | (1U << 31))
#define	ICU_VALID_MASK		(~ICU_RESERVED_MASK)

#define	BECC_RSSR		0x0500	/* rotary switch status */
#define	RSSR_POS		0x0000000f	/* switch position */
#define	RSSR_BE			(1U << 6)	/* big-endian jumper */

#endif /* _BECCREG_H_ */
