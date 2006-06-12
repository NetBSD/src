/* $NetBSD: gtreg.h,v 1.1.12.1 2006/06/12 09:44:39 tron Exp $ */
/*
 * Copyright (c) 2003
 *     KIYOHARA Takashi.  All rights reserved.
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

#define GT_TIMER_COUNTER0	0x850
#define GT_TIMER_COUNTER1	0x854
#define GT_TIMER_COUNTER2	0x858
#define GT_TIMER_COUNTER3	0x85c

#define GT_TIMER_CTRL		0x864
#define  ENTC0			0x01
#define  TCSEL0			0x02
#define  ENTC1			0x04
#define  TCSEL1			0x08
#define  ENTC2			0x10
#define  TCSEL2			0x20
#define  ENTC3			0x40
#define  TCSEL3			0x80

#define GT_PCI_COMMAND		0xc00
#define  PCI_BYTESWAP		0x00000001
#define  PCI_SYNCMODE		0x00000006
#define  PCI_PCLK_LOW		0x00000000
#define  PCI_PCLK_HIGH		0x00000002
#define  PCI_PCLK_SYNC		0x00000004

#define GT_INTR_CAUSE		0xc18
#define  INTSUM			0x00000001
#define  MEMOUT			0x00000002
#define  DMAOUT			0x00000004
#define  MASTEROUT		0x00000008
#define  DMA0COMP		0x00000010
#define  DMA1COMP		0x00000020
#define  DMA2COMP		0x00000040
#define  DMA3COMP		0x00000080
#define  T0EXP			0x00000100
#define  T1EXP			0x00000200
#define  T2EXP			0x00000400
#define  T3EXP			0x00000800
#define  MASRDERR		0x00001000
#define  SLVWRERR		0x00002000
#define  MASWRERR		0x00004000
#define  SLVRDERR		0x00008000
#define  ADDRERR		0x00010000
#define  MEMERR			0x00020000
#define  MASABORT		0x00040000
#define  TARABORT		0x00080000
#define  RETRYCTR		0x00100000
#define  MASTER_INT0		0x00200000
#define  MASTER_INT1		0x00400000
#define  MASTER_INT2		0x00800000
#define  MASTER_INT3		0x01000000
#define  MASTER_INT4		0x02000000
#define  PCI_INT0		0x04000000
#define  PCI_INT1		0x08000000
#define  PCI_INT2		0x10000000
#define  PCI_INT3		0x20000000
#define  MASTER_INTSUM		0x40000000
#define  PCI_INTSUM		0x80000000

#define GT_MASTER_MASK		0xc1c

#define GT_PCI_MASK		0xc24

#define GT_PCICFG_ADDR		0xcf8
#define  PCICFG_REG		0x000000ff
#define  PCICFG_FUNC		0x00000700
#define  PCICFG_DEV		0x0000f800
#define  PCICFG_BUS		0x00ff0000
#define  PCICFG_ENABLE		0x80000000

#define GT_PCICFG_DATA		0xcfc
