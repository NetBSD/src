/*	$NetBSD: ixp12x0_pcireg.h,v 1.2 2003/02/17 20:51:52 ichiro Exp $ */

/*
 * Copyright (c) 2002, 2003
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.  All rights reserved.
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
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS 
 * HEAD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _IXP12X0_PCIREG_H_
#define _IXP12X0_PCIREG_H_

#include <arm/ixp12x0/ixp12x0reg.h>

/* PCI Configuration Space Registers */

/* base address */
#define	IXP_PCI_MEM_BAR		0x10
# define IXP1200_PCI_MEM_BAR	0x40000000UL
# define IXP_PCI_MEM_BAR_MASK	0xffffff80

#define	IXP_PCI_IO_BAR		0x14
# define IXP1200_PCI_IO_BAR	0x0000f000UL
# define IXP_PCI_IO_BAR_MASK	0xffffff80

#define	IXP_PCI_DRAM_BAR	0x18
# define IXP1200_PCI_DRAM_BAR	0x00000000UL
# define IXP_PCI_DRAM_BAR_MASK	0xfffc0000

#define PCI_CAP_PTR		0x34
#define PCI_INT_LINE		0x3C
#define MAILBOX_0		0x50
#define MAILBOX_1		0x54
#define MAILBOX_2		0x58
#define MAILBOX_3		0x5C
#define DOORBELL		0x60
#define DOORBELL_SETUP		0x64
#define ROM_BYTE_WRITE		0x68
#define CAP_PTR_EXT		0x70
#define PWR_MGMT		0x74

/* Reset Registers*/
#define	IXPPCI_IXP1200_RESET	0x7C
# define RESET_UE0		(1U << 0)
# define RESET_UE1		(1U << 1)
# define RESET_UE2		(1U << 2)
# define RESET_UE3		(1U << 3)
# define RESET_UE4		(1U << 4)
# define RESET_UE5		(1U << 5)
# define RESET_PCIRST		(1U << 14)
# define RESET_EXRST		(1U << 15)
# define RESET_FBI		(1U << 16)
# define RESET_CMDARB		(1U << 17)
# define RESET_SDRAM		(1U << 18)
# define RESET_SRAM		(1U << 29)
# define RESET_PCI		(1U << 30)
# define RESET_SACORE		(1U << 31)

# define RESET_FULL		(RESET_UE0 | RESET_UE1 | RESET_UE2 | \
				 RESET_UE3 | RESET_UE4 | RESET_UE5 | \
				 RESET_EXRST | RESET_FBI | \
				 RESET_CMDARB | RESET_SDRAM | RESET_SRAM | \
				 RESET_PCI | RESET_SACORE)

#define CHAN_1_BYTE_COUNT	0x80
#define CHAN_1_PCI_ADDR		0x84
#define CHAN_1_DRAM_ADDR	0x88
#define CHAN_1_DESC_PTR		0x8C
#define CHAN_1_CONTROL		0x90
#define DMA_INF_MODE		0x9C
#define CHAN_2_BYTE_COUNT	0xA0
#define CHAN_2_PCI_ADDR		0xA4
#define CHAN_2_DRAM_ADDR	0xA8
#define CHAN_2_DESC_PTR		0xAC
#define CHAN_2_CONTROL		0xB0

#define	CSR_BASE_ADDR_MASK	0x0F8
#define CSR_BASE_ADDR_OFF	0xFC
# define CSR_BASE_ADDR_MASK_1M	0x000c0000UL

#define	DRAM_BASE_ADDR_MASK	0x100
#define DRAM_BASE_ADDR_OFF	0x104
# define DRAM_BASE_ADDR_MASK_256MB	0x0ffc0000UL

#define ROM_BASE_ADDR_MASK	0x108
#define DRAM_TIMING		0x10C
#define DRAM_ADDR_SIZE_0	0x110
#define DRAM_ADDR_SIZE_1	0x114
#define DRAM_ADDR_SIZE_2	0x118
#define DRAM_ADDR_SIZE_3	0x11C
#define I2O_IFH			0x120
#define I2O_IPT			0x124
#define I2O_OPH			0x128
#define I2O_OFT			0x12C
#define I2O_IFC			0x130
#define I2O_OPC			0x134
#define I2O_IPC			0x138
#define SA_CONTROL		0x13C
# define SA_CONTROL_PNR		(1 << 9)
# define SA_CONTROL_COMPLETE	(1 << 0)
#define PCI_ADDR_EXT		0x140
# define PCI_ADDR_EXT_PIOADD(x)	((x) & 0xffff0000)
# define PCI_ADDR_EXT_PMSA(x)	(((x) & 0xe0000000) >> 16)
#define PREFETCH_RANGE		0x144
#define PCI_ABITOR_STATUS	0x148
#define DBELL_PCI_MASK		0x150
#define DBELL_SA_MASK		0x154

/*
 * Interrupt index assignment
 *
 *     FIQ/IRQ bitmap in "PCI Registers Accessible Through StrongARM Core"
 *
 *        3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 * bit    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-------------+-+-+-+-+---+-+-+
 *       |D|R|R|D|D|P|I|S|R|S|D|D| |P|D|D|D|             |T|T|T|T|   |S| |
 *       |P|T|M|P|T|W|I|D|S|B|M|M|R|I|M|M|F|             |4|3|2|1|   |I| |
 *       |E|A|A|E|E|R|P|P|E| |A|A|E|L|A|A|H|     RES     | | | | |RES| | |
 *       | | | |D| |M| |A|R| |2|1|S| |2|1| |             | | | | |   | | |
 *       | | | | | | | |R|R| |N|N| | | | | |             | | | | |   | | |
 *       | | | | | | | | | | |B|B| | | | | |             | | | | |   | | |
 *       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-------------+-+-+-+-+---+-+-+
 *        3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1
 * index  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5               7 6 5 4     1 0
 *
 */

/* PCI_IRQ_STATUS */
#define	IXPPCI_IRQ_STATUS	(IXP12X0_PCI_VBASE + 0x180)
#define	IXPPCI_FIQ_STATUS	(IXP12X0_PCI_VBASE + 0x280)
#define	IXPPCI_IRQ_RAW_STATUS	(IXP12X0_PCI_VBASE + 0x184)
#define	IXPPCI_FIQ_RAW_STATUS	(IXP12X0_PCI_VBASE + 0x284)
#define	IXPPCI_IRQ_ENABLE	(IXP12X0_PCI_VBASE + 0x188)
#define	IXPPCI_FIQ_ENABLE	(IXP12X0_PCI_VBASE + 0x288)
#define	IXPPCI_IRQ_ENABLE_SET	(IXP12X0_PCI_VBASE + 0x188)
#define	IXPPCI_FIQ_ENABLE_SET	(IXP12X0_PCI_VBASE + 0x288)
#define	IXPPCI_IRQ_ENABLE_CLEAR	(IXP12X0_PCI_VBASE + 0x18c)
#define	IXPPCI_FIQ_ENABLE_CLEAR	(IXP12X0_PCI_VBASE + 0x28c)
#define	IXPPCI_IRQ_SOFT		(IXP12X0_PCI_VBASE + 0x190)
#define	IXPPCI_FIQ_SOFT		(IXP12X0_PCI_VBASE + 0x290)
#define	IXPPCI_IRQST_TIMER	(IXP12X0_PCI_VBASE + 0x010)

#define	IXPPCI_INTR_DPE		63
#define	IXPPCI_INTR_RTA		62
#define	IXPPCI_INTR_RMA		61
#define	IXPPCI_INTR_DPED	60
#define	IXPPCI_INTR_DTE		59
#define	IXPPCI_INTR_PWRM	58
#define	IXPPCI_INTR_IIP		57
#define	IXPPCI_INTR_SDPAR	56
#define	IXPPCI_INTR_RSERR	55
#define	IXPPCI_INTR_SB		54
#define	IXPPCI_INTR_DMA2NB	53
#define	IXPPCI_INTR_DMA1NB	52
#define	IXPPCI_INTR_bit19	51
#define	IXPPCI_INTR_PIL		50
#define	IXPPCI_INTR_DMA2	49
#define	IXPPCI_INTR_DMA1	48
#define	IXPPCI_INTR_DFH		47
#define	IXPPCI_INTR_bit14	46
#define	IXPPCI_INTR_bit13	45
#define	IXPPCI_INTR_bit12	44
#define	IXPPCI_INTR_bit11	43
#define	IXPPCI_INTR_bit10	42
#define	IXPPCI_INTR_bit9	41
#define	IXPPCI_INTR_bit8	40
#define	IXPPCI_INTR_T4		39
#define	IXPPCI_INTR_T3		38
#define	IXPPCI_INTR_T2		37
#define	IXPPCI_INTR_T1		36
#define	IXPPCI_INTR_bit3	35
#define	IXPPCI_INTR_bit2	34
#define	IXPPCI_INTR_SI		33
#define	IXPPCI_INTR_bit0	32

#endif /* _IXP12X0_PCIREG_H_ */
