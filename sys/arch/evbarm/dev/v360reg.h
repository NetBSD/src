/*	$NetBSD: v360reg.h,v 1.1.2.2 2001/10/27 16:20:30 rearnsha Exp $ */

/*-
 * Copyright (c) 2001 ARM Ltd
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
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * V3 V360EPI Local Bus <-> PCI bridge.
 */


#define V360_PCI_VENDOR		0x00
#define V360_PCI_DEVICE		0x02
#define V360_PCI_CMD		0x04
#define V360_PCI_STAT		0x06
#define V360_PCI_CC_REV		0x08
#define V360_PCI_HDR_CFG		0x0c
#define V360_PCI_IO_BASE		0x10
#define V360_PCI_BASE0		0x14
#define V360_PCI_BASE1		0x18
#define V360_PCI_SUB_VENDOR	0x2c
#define V360_PCI_SUB_ID		0x2e
#define V360_PCI_ROM		0x30
#define V360_PCI_BPARAM		0x3c
#define V360_PCI_MAP0		0x40
#define V360_PCI_MAP1		0x44
#define V360_PCI_INT_STAT	0x48
#define V360_PCI_INT_CFG		0x4c

#define V360_LB_BASE0		0x54
#define V360_LB_BASE1		0x58
#define V360_LB_MAP0		0x5e
#define V360_LB_MAP1		0x62
#define V360_LB_BASE2		0x64
#define V360_LB_MAP2		0x66
#define V360_LB_SIZE		0x68
#define V360_LB_IO_BASE		0x6e

#define V360_FIFO_CFG		0x70
#define V360_FIFO_PRIORITY	0x72
#define V360_FIFO_STAT		0x74

#define V360_LB_ISTAT		0x76
#define V360_LB_IMASK		0x77

#define V360_SYSTEM		0x78

#define V360_LB_CFG		0x7a

#define V360_PCI_CFG		0x7c

#define V360_DMA_PCI_ADDR0	0x80
#define V360_DMA_LOCAL_ADDR0	0x84
#define V360_DMA_LENGTH0		0x88
#define V360_DMA_CSR0		0x8b
#define V360_DMA_CTLB_ADDR0	0x8c

#define V360_DMA_PCI_ADDR1	0x90
#define V360_DMA_LOCAL_ADDR1	0x94
#define V360_DMA_LENGTH1		0x98
#define V360_DMA_CSR1		0x9b
#define V360_DMA_CTLB_ADDR1	0x9c

#define V360_MAIL_DATA0		0xc0
#define V360_MAIL_DATA1		0xc1
#define V360_MAIL_DATA2		0xc2
#define V360_MAIL_DATA3		0xc3
#define V360_MAIL_DATA4		0xc4
#define V360_MAIL_DATA5		0xc5
#define V360_MAIL_DATA6		0xc6
#define V360_MAIL_DATA7		0xc7
#define V360_MAIL_DATA8		0xc8
#define V360_MAIL_DATA9		0xc9
#define V360_MAIL_DATA10		0xca
#define V360_MAIL_DATA11 	0xcb
#define V360_MAIL_DATA12		0xcc
#define V360_MAIL_DATA13		0xcd
#define V360_MAIL_DATA14		0xce
#define V360_MAIL_DATA15		0xcf

#define V360_PCI_MAIL_IEWR	0xd0
#define V360_PCI_MAIL_IERD	0xd2
#define V360_LB_MAIL_IEWR	0xd4
#define V360_LB_MAIL_IERd	0xd6
#define V360_MAIL_WR_STAT	0xd8
#define V360_MAIL_RD_STAT	0xda

#define V360_QBA_MAP		0xdc

#define V360_DMA_DELAY		0xe0

