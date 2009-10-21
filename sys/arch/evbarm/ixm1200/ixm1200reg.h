/*	$NetBSD: ixm1200reg.h,v 1.2 2009/10/21 14:15:51 rmind Exp $ */
/*
 * Copyright (c) 2002
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
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
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _IXM1200REG_H_
#define _IXM1200REG_H_

/*
 * Memory map and register definitions for the Intel IXM1200
 * Evaluation Board.
 */

/* Virtual address for SlowPort I/O space */
#define IXM1200_SLOWPORT_VBASE	(IXP12X0_PCI_IO_VBASE + IXP12X0_PCI_IO_SIZE)
					/* va=0xf0021000 */

/*
 * Slow Port I/O
 */

/* Status LEDs (4x2 bits) write-only */
#define	IXM1200_LED_VADDR	IXM1200_SLOWPORT_VBASE
#define	IXM1200_LED_ADDR_SIZE	0x1000
#define	IXM1200_LED_ADDR	0x38508000

/* Dip Switches (4 Bits) read-only */
#define	IXM1200_DIP_VADDR	(IXM1200_SLOWPORT_VBASE + IXM1200_LED_VADDR_SIZE)
#define	IXM1200_DIP_ADDR_SIZE	0x1000
#define	IXM1200_DIP_ADDR	0x38509000

/* Board Revision, read-only */
#define	IXM1200_REV_ADDR	0x3850A000

/* SDRAM Address Width, read-only */
#define	IXM1200_SDRAM_WIDTH	0x3850B000

/* MAC0(IXF440 Multiport 10/100Mbps Ethernet Cont. ) */

/* PCI Configuration Cycles */
#define	IXM1200_PCI_CYCLE_SIZE	0x00000100

/* IXP1200, IDSEL=A11 */
#define	IXM1200_CYCLE_ADDR	0x00000800
#define	IXM1200_CYCLE_SIZE	IXM1200_PCI_CYCLE_SIZE

/* PCIBridge, IDSEL=A12 */
#define	IXM1200_PB_CYCLE_ADDR	0x00001000
#define	IXM1200_PB_CYCLE_SIZE	IXM1200_PCI_CYCLE_SIZE

/* Ether MAC/PHY, IDSEL=A13 */
#define	IXM1200_MAC_CYCLE_ADDR	0x00002000
#define	IXM1200_MAC_CYCLE_SIZE	IXM1200_PCI_CYCLE_SIZE

/* PMC Expansion, IDSEL=A14 */
#define	IXM1200_PMC_CYCLE_ADDR	0x00002000
#define	IXM1200_MAC_CYCLE_SIZE	IXM1200_PCI_CYCLE_SIZE

#endif /* _IXM1200REG_H_ */
