/*	$NetBSD: isp1763reg.h,v 1.1 2026/10/11 16:09:06 rkujawa Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

/*
 * ISP1763A USB 2.0 host controller.
 */

#ifndef _DEV_IC_ISP1763REG_H_
#define	_DEV_IC_ISP1763REG_H_

#define	ISP1763_CHIPID		0x70	/* 32 bits */
#define	 ISP1763_CHIPID_VALUE	0x00176320
#define	ISP1763_SCRATCH		0x78

/* HCI regs, 32-bit. */
#define	ISP1763_USBCMD		0x8c
#define	 ISP1763_USBCMD_RS	__BIT(0)
#define	 ISP1763_USBCMD_HCRESET	__BIT(1)
#define	 ISP1763_USBCMD_LHCR	__BIT(7)
#define	ISP1763_USBSTS		0x90
#define	 ISP1763_USBSTS_PCD	__BIT(2)
#define	 ISP1763_USBSTS_FLR	__BIT(3)
#define	ISP1763_USBINTR		0x94
#define	 ISP1763_USBINTR_PCDIE	__BIT(2)
#define	ISP1763_FRINDEX		0x98
#define	ISP1763_CONFIGFLAG	0x9c
#define	 ISP1763_CONFIGFLAG_CF	__BIT(0)
#define	ISP1763_PORTSC1		0xa0
#define	 ISP1763_PORTSC_ECCS	__BIT(0)
#define	 ISP1763_PORTSC_ECSC	__BIT(1)
#define	 ISP1763_PORTSC_PED	__BIT(2)
#define	 ISP1763_PORTSC_FPR	__BIT(6)
#define	 ISP1763_PORTSC_SUSP	__BIT(7)
#define	 ISP1763_PORTSC_PR	__BIT(8)
#define	 ISP1763_PORTSC_LS	__BITS(11, 10)
#define	 ISP1763_PORTSC_PP	__BIT(12)
#define	 ISP1763_PORTSC_PO	__BIT(13)
#define	 ISP1763_PORTSC_PTC	__BITS(19, 16)

/* Transfer descriptor maps, 16-bits. */
#define	ISP1763_ISO_DONE	0xa4
#define	ISP1763_ISO_SKIP	0xa6
#define	ISP1763_ISO_LAST	0xa8
#define	ISP1763_INT_DONE	0xaa
#define	ISP1763_INT_SKIP	0xac
#define	ISP1763_INT_LAST	0xae
#define	ISP1763_ATL_DONE	0xb0
#define	ISP1763_ATL_SKIP	0xb2
#define	ISP1763_ATL_LAST	0xb4

/* Configuration registers, 16 bits. */
#define	ISP1763_HWMODE		0xb6
#define	 ISP1763_HWMODE_GLINTEN	__BIT(0)
#define	 ISP1763_HWMODE_INTLEVEL	__BIT(1)	/* 1 = edge */
#define	 ISP1763_HWMODE_INTPOL	__BIT(2)	/* 1 = active high */
#define	 ISP1763_HWMODE_INTFLOCK	__BIT(3)
#define	 ISP1763_HWMODE_BUS8	__BIT(4)
#define	ISP1763_SWRESET		0xb8
#define	 ISP1763_SWRESET_ALL	__BIT(0)
#define	 ISP1763_SWRESET_HC	__BIT(1)
#define	 ISP1763_SWRESET_ATX	__BIT(3)
#define	ISP1763_BUFSTS		0xba
#define	 ISP1763_BUFSTS_ATL	__BIT(0)
#define	 ISP1763_BUFSTS_INT	__BIT(1)
#define	 ISP1763_BUFSTS_ISO	__BIT(2)
#define	ISP1763_MEMORY		0xc4
#define	ISP1763_DATAPORT	0xc6

/* Interrupt registers, 16 bits. */
#define	ISP1763_HCINT		0xd4
#define	ISP1763_HCINTEN		0xd6
#define	 ISP1763_HCINT_OPR	__BIT(4)
#define	 ISP1763_HCINT_INT	__BIT(7)
#define	 ISP1763_HCINT_ATL	__BIT(8)
#define	 ISP1763_HCINT_ISO	__BIT(9)
#define	 ISP1763_HCINT_ALL	__BITS(10, 0)
#define	ISP1763_ISO_ORMASK	0xd8
#define	ISP1763_INT_ORMASK	0xda
#define	ISP1763_ATL_ORMASK	0xdc
#define	ISP1763_ISO_ANDMASK	0xde
#define	ISP1763_INT_ANDMASK	0xe0
#define	ISP1763_ATL_ANDMASK	0xe2

/* OTG control. */
#define	ISP1763_OTGCTL_SET	0xe4
#define	ISP1763_OTGCTL_CLR	0xe6
#define	 ISP1763_OTGCTL_SWSELHC	__BIT(7)	/* clear = port 1 to host */
#define	 ISP1763_OTGCTL_OTGDIS	__BIT(10)
#define	 ISP1763_OTGCTL_HC2DIS	__BIT(15)

/* Memory map. */
#define	ISP1763_PTD_SIZE	32
#define	ISP1763_NSLOTS		16
#define	ISP1763_ISO_PTD_BASE	0x0400
#define	ISP1763_INT_PTD_BASE	0x0800
#define	ISP1763_ATL_PTD_BASE	0x0c00
#define	ISP1763_PAYLOAD_BASE	0x1000
#define	ISP1763_MEM_END		0x6000

/* Payload */
#define	ISP1763_PTD_ADDR(a)	(((a) - 0x400) >> 3)

/*
 * Transfer descriptor.
 */
#define	ISP1763_PTD_DW0_V	__BIT(0)
#define	ISP1763_PTD_DW0_NBYTES	__BITS(17, 3)
#define	ISP1763_PTD_DW0_MPS	__BITS(28, 18)
#define	ISP1763_PTD_DW0_MULT	__BITS(30, 29)
#define	ISP1763_PTD_DW0_EP0	__BIT(31)

#define	ISP1763_PTD_DW1_EP31	__BITS(2, 0)
#define	ISP1763_PTD_DW1_ADDR	__BITS(9, 3)
#define	ISP1763_PTD_DW1_TOKEN	__BITS(11, 10)
#define	 ISP1763_TOKEN_OUT	0
#define	 ISP1763_TOKEN_IN	1
#define	 ISP1763_TOKEN_SETUP	2
#define	ISP1763_PTD_DW1_EPTYPE	__BITS(13, 12)
#define	 ISP1763_EPTYPE_CTRL	0
#define	 ISP1763_EPTYPE_ISOC	1
#define	 ISP1763_EPTYPE_BULK	2
#define	 ISP1763_EPTYPE_INTR	3
#define	ISP1763_PTD_DW1_SPLIT	__BIT(14)
#define	ISP1763_PTD_DW1_RETIRE	__BIT(15)
#define	ISP1763_PTD_DW1_SE	__BITS(17, 16)
#define	 ISP1763_SE_FULL	0
#define	 ISP1763_SE_LOW		2
#define	ISP1763_PTD_DW1_PORT	__BITS(24, 18)
#define	ISP1763_PTD_DW1_HUB	__BITS(31, 25)

#define	ISP1763_PTD_DW2_UFRAME	__BITS(7, 0)
#define	ISP1763_PTD_DW2_DATA	__BITS(23, 8)
#define	ISP1763_PTD_DW2_RL	__BITS(28, 25)

#define	ISP1763_PTD_DW3_NDONE	__BITS(14, 0)
#define	ISP1763_PTD_DW3_NDONE_SPLIT	__BITS(11, 0)
#define	ISP1763_PTD_DW3_NAKCNT	__BITS(22, 19)
#define	ISP1763_PTD_DW3_CERR	__BITS(24, 23)
#define	ISP1763_PTD_DW3_DT	__BIT(25)
#define	ISP1763_PTD_DW3_PING	__BIT(26)
#define	ISP1763_PTD_DW3_SC	__BIT(27)
#define	ISP1763_PTD_DW3_X	__BIT(28)
#define	ISP1763_PTD_DW3_B	__BIT(29)
#define	ISP1763_PTD_DW3_H	__BIT(30)
#define	ISP1763_PTD_DW3_A	__BIT(31)

#define	ISP1763_PTD_DW4_USA	__BITS(7, 0)
#define	ISP1763_PTD_DW5_USCS	__BITS(7, 0)

#endif /* !_DEV_IC_ISP1763REG_H_ */
