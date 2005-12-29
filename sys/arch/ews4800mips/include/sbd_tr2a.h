/*	$NetBSD: sbd_tr2a.h,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _SBD_TR2A_PRIVATE
#error "Don't inlucde this file except for TR2A implemetation"
#endif /* !_SBD_TR2A_PRIVATE */

#ifndef _EWS4800MIPS_SBD_TR2A_H_
#define	_EWS4800MIPS_SBD_TR2A_H_
/*
 * EWS4800/360 (TR2A) specific system board definition
 */
/*
 * [interrupt overview]
 *
 *                   +-----+
 *                   | CPU |
 *                   +--+--+
 *                      |
 *          +----+----+-+--+----+----+
 *        INT5 INT4 INT3 INT2 INT1 INT0
 *          |    |    |    |    |    |
 *        +-+----+----+----+----+----+-+
 *        |            INTC            |
 *        | mask:   0xbe000008         |
 *        | status: 0xbe000004         |
 *        | clear:  0xbe000000         |       +-------------------+
 *        +-+----+----+----+----+----+-+       |      APbus        |
 *          |    |    |    |    +----+---------+Lo                 |
 *  CLOCK---+    |    +----+---------+---------+Hi                 |
 * 0xbe4a0008    |         |         |         |                   |
 * (0x80)      +-+---------+---------+-+       +-------------------+
 *             |         ASObus        |
 *             | mask:   0xbe40a00c    |
 *             | status: 0xbe40a010    |
 *             | DMA int:0xbe408000    |
 *             +-+---------+---------+-+
 *               |         |         |
 * 0xbe440000 ZS-+         |         |
 * 0xbe480000 KBMS         |         |
 *                         |         |
 * 0xbe500000 SCSI-A-------+         |
 * 0xbe510000 SCSI-B-------+         |
 * 0xbe400000 LANCE--------+         |
 *                                   |
 *       NMI-------------------------+
 *
 * [INTC interrupt mask]  0xbe000008
 *  0x80000000    INT5
 *  0x04000000    INT4
 *  0x00200000    INT3
 *  0x00010000    INT2
 *  0x00000800    INT1
 *  0x00000020    INT0
 *
 * [ASObus interrupt mask] 0xbe40a00c
 *                        TR2A
 *  0x00800000    INT4    -
 *  0x00400000    INT4    -
 *  0x00300010    INT4    ZS
 *  0x00000040    INT4    KBMS
 *  0x00000020    INT4    -
 *  0x00000100    INT2    simd2 A
 *  0x00000200    INT2    simd2 B
 *  0x00000001    INT2    limd2
 *  0x00008000    INT0    NMI
 *  0x00000008    INT0    -
 *  0x00000004    INT0    -
 *  0x00f0837d            0x00300351
 */

/* ROM */
#define	TR2A_ROM_FONT_ADDR		0xbfc0ec00
#define	TR2A_SCSIROM_ADDR		0xbfc80000
#define	TR2A_GAROM_ADDR			0xbfc82000

#define	TR2A_ROM_KEYMAP_NORMAL		((uint8_t *)0xbfc39140)
#define	TR2A_ROM_KEYMAP_SHIFTED		((uint8_t *)0xbfc38e40)
#define	TR2A_ROM_KEYMAP_CONTROL		((uint8_t *)0xbfc38ec0)
#define	TR2A_ROM_KEYMAP_CAPSLOCK	((uint8_t *)0xbfc39040)

/* System board I/O devices */
#define	TR2A_IOBASE_ADDR	0xbe000000
#define	TR2A_LANCE_BASE		0xbe400000	/* Ether AM79C90 */
#define	TR2A_SIO_BASE		0xbe440000	/* SIO1	85230 */
#define	TR2A_KBMS_BASE		0xbe480000	/* SIO0 85230 */
#define	TR2A_NVSRAM_BASE	0xbe490000	/* NVSRAM */
#define	TR2A_SCSIA_BASE		0xbe500000	/* SCSI-A NCR53C710 */
#define	TR2A_SCSIB_BASE		0xbe510000	/* SCSI-B NCR53C710 */
#if 0
#define	TR2A_FDC_BASE		0xbe420000
#define	TR2A_LPT_BASE		0xbe430000
#define	TR2A_APBUS_INTC_MASK	0xbe806000
#define	TR2A_VMECHK		0xbe000040
#define	TR2A_CLK		0xbe000024
#endif

/* APbus */
#define	TR2A_APBUS_ADDR		0xe0000000
#define	TR2A_APBUS_SIZE		0x18000000

/* NVSRAM */
#define	TR2A_NVSRAM_ADDR	0xbe490000
#define	NVSRAM_SIGNATURE	0xbe490000
#define	NVSRAM_MACHINEID	0xbe490010
#define	NVSRAM_ETHERADDR	0xbe491008
#define	NVSRAM_TF_PROGRESS	0xbe493010
#define	NVSRAM_TF_ERROR		0xbe493028
#define	NVSRAM_BOOTDEV		((uint8_t *)0xbe493030)
#define	NVSRAM_CONSTYPE		((uint8_t *)0xbe4932a0)
#define	NVSRAM_BOOTUNIT		0xbe493414
#define	NVSRAM_SBDINIT_0	0xbe493450
#define	NVSRAM_SBDINIT_1	0xbe493454
#define	NVSRAM_SBDINIT_2	0xbe493458
#define	NVSRAM_SBDINIT_3	0xbe49345c

/* Frame buffer */
#define	TR2A_GAFB_ADDR		0xf0000000
#define	TR2A_GAFB_SIZE		0x04000000
#define	TR2A_GAREG_ADDR		0xf5f00000
#define	TR2A_GAREG_SIZE		0x00001000

#define	SOFTRESET_REG		((volatile uint8_t *)0xba000004)
#define	SOFTRESET_FLAG		((volatile uint32_t *)0xbe000064)

#define	CLOCK_REG		((volatile uint8_t *)0xbe4a0008)
#define	POWEROFF_REG		((volatile uint8_t *)0xbe4a0030)
#define	LED_TF_REG		((volatile uint8_t *)0xbe4a0040)
#define	  LED_TF_ON		1
#define	  LED_TF_OFF		0
#define	TF_ERROR_CODE		((volatile uint8_t *)0xbe4a0044)
#define	BUZZER_REG		((volatile uint8_t *)0xbe4a0050)

/* Keyboard/Mouse Z85230 */
#define	KBD_STATUS		((volatile uint8_t *)0xbe480000)
#define	KBD_DATA		((volatile uint8_t *)0xbe480004)

/* RTC	*/
#define	RTC_MK48T18_ADDR	((volatile uint8_t *)0xbe493fe0)
#define	RTC_MK48T18_NVSRAM_ADDR	0xbe490000

/* INTC */
#define	INTC_CLEAR_REG		((volatile uint32_t *)0xbe000000)
#define	INTC_STATUS_REG		((volatile uint32_t *)0xbe000004)
#define	INTC_MASK_REG		((volatile uint32_t *)0xbe000008)
#define	INTC_INT5		0x80000000
#define	INTC_INT4		0x04000000
#define	INTC_INT3		0x00200000
#define	INTC_INT2		0x00010000
#define	INTC_INT1		0x00000800
#define	INTC_INT0		0x00000020

/* ASO */
#define	ASO_DMAINT_STATUS_REG	((volatile uint32_t *)0xbe408000)
#define	ASO_INT_MASK_REG	((volatile uint32_t *)0xbe40a00c)
#define	ASO_INT_STATUS_REG	((volatile uint32_t *)0xbe40a010)

#define	TR2A_ASO_INTMASK_ALL	0x00f0837d

/* Graphic adapter */
#include <machine/gareg.h>

#endif /* !_EWS4800MIPS_SBD_TR2_H_ */
