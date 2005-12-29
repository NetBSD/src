/*	$NetBSD: sbd_tr2.h,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

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

#ifndef _SBD_TR2_PRIVATE
#error "Don't inlucde this file except for TR2 implemetation"
#endif /* !_SBD_TR2_PRIVATE */

#ifndef _EWS4800MIPS_SBD_TR2_H_
#define	_EWS4800MIPS_SBD_TR2_H_
/*
 * EWS4800/350 (TR2) specific system board definition
 */

/* ROM */
#define	TR2_ROM_FONT_ADDR	0xbfc0ec00
#define	TR2_ROM_FONT_SIZE	((0x7f - 0x20) * 24 * sizeof(int16_t))

#define	TR2_ROM_KEYMAP_NORMAL	((uint8_t *)0xbfc12d6c)
#define	TR2_ROM_KEYMAP_SHIFTED	((uint8_t *)0xbfc12dec)
#define	TR2_ROM_KEYMAP_CONTROL	((uint8_t *)0xbfc12e6c)
#define	TR2_ROM_KEYMAP_CAPSLOCK	((uint8_t *)0xbfc12eec)
#define	TR2_ROM_KBD_TYPE	0xbfc0fe04	/* [d0 00 00 01] used by kbmskbreset. */

#define	TR2_ROM_PUTC		((void (*)(int, int, int))0xbfc04f28)
#define	TR2_ROM_GETC		((int (*)(void))0xbfc11fa0)

/* System board I/O devices */
#define	TR2_PICNIC_ADDR		0xbb000000
#define	TR2_KBMS_ADDR		0xbb010000
#define	TR2_SIO_ADDR		0xbb011000
#define	TR2_NVSRAM_ADDR		0xbb020000
#define	TR2_NVSRAM_SIZE		0x00004000
#define	TR2_FDC_ADDR		0xbb030000
#define	TR2_LPT_ADDR		0xbb040000
#define	TR2_SCSI_ADDR		0xbb050000
#define	TR2_ETHER_ADDR		0xbb060000
#define	TR2_MEMC_ADDR		0xbfa00000
#define	TR2_NABI_ADDR		0xbfb00000
#define	TR2_GAFB_ADDR		0xf0000000
#define	TR2_GAFB_SIZE		0x08000000
#define	TR2_GACTRL_ADDR		0xf5f00000
#define	TR2_GACTRL_SIZE		0x1000

#define	SOFTRESET_REG		((volatile uint32_t *)0xbfb00000)
#define	POWEROFF_REG		((volatile uint8_t *)0xbb004000)
#define	UPS_STATUS_REG		((volatile uint8_t *)0xbb004008) /* mask 0xffffffbb, 0x4 */

#define	LED_TF_REG		((volatile uint8_t *)0xbb006000) /* 0/1 (Red)*/
#define	TF_ERROR_CODE		((volatile uint8_t *)0xbb006004) /* 1-255 */

#define	BUZZER_REG		((volatile uint8_t *)0xbb007000)

/* NABI */
#define	NABI0_CTRL_REG		((volatile uint32_t *)0xbfb00000)
#define	NABI1_CTRL_REG		((volatile uint32_t *)0xbfb00004)
#define	NABI2_CTRL_REG		((volatile uint32_t *)0xbfb00008)
#define	NABI0_INTR_REG		((volatile uint32_t *)0xbfb00010)
#define	NABI1_INTR_REG		((volatile uint32_t *)0xbfb00018) /* VME */
#define	NABI2_INTR_REG		((volatile uint32_t *)0xbfb0001c)

/*
 * PICNIC	(interrupt controller)
 */
#define	PICNIC_INT0_STATUS_REG	((volatile uint8_t *)0xbb000000)
#define	PICNIC_INT2_STATUS_REG	((volatile uint8_t *)0xbb000004)
#define	PICNIC_INT4_STATUS_REG	((volatile uint8_t *)0xbb000008)
#define	PICNIC_INT5_STATUS_REG	((volatile uint8_t *)0xbb000010)
#define	PICNIC_NMI_REG		((volatile uint8_t *)0xbb000014)

#define	PICNIC_INT0_MASK_REG	((volatile uint8_t *)0xbb001000)
#define	PICNIC_INT2_MASK_REG	((volatile uint8_t *)0xbb001004)
#define	PICNIC_INT4_MASK_REG	((volatile uint8_t *)0xbb001008)
#define	PICNIC_INT5_MASK_REG	((volatile uint8_t *)0xbb001010)
/* Interrupt source */
#define	PICNIC_INT_FDDLPT	0x80
#define	PICNIC_INT_ETHER	0x40
#define	PICNIC_INT_SCSI		0x20
#define	PICNIC_INT_SERIAL	0x04
#define	PICNIC_INT_KBMS		0x01
#define	PICNIC_INT_CLOCK	0x01
/*
 *	  76543210
 *        |||  | +-- keyboard, mouse
 *        |||  +-----serial
 *        ||+--------SCSI
 *        |+---------ether
 *	  +----------FDC, printer
 *0xbb00   UX    IPL		      mips int
 *  1000   0x80  0x00	7		INT0
 *  1004   0x60  0x00	 65		INT2
 *  1008   0x05  0x00      2 0		INT4
 *  1010   0x01  0x01        0	Clock	INT5
 */

/* SIO0		Z85C30 */
#define	KBD_STATUS		((volatile uint8_t *)0xbb010000)
#define	KBD_DATA		((volatile uint8_t *)0xbb010004)
#define	MOUSE_STATUS		((volatile uint8_t *)0xbb010008)
#define	MOUSE_DATA		((volatile uint8_t *)0xbb01000c)
/* SIO1		Z85C30 */
#define	SIOA_STATUS		((volatile uint8_t *)0xbb011008)
#define	SIOA_RDATA		((volatile uint8_t *)0xbb01100c)
#define	SIOB_STATUS		((volatile uint8_t *)0xbb011000)
#define	SIOB_RDATA		((volatile uint8_t *)0xbb011004)

/* ETHER	i82589 */
/* read operation invokes channel attention. */
#define	ETHER_SETADDR_REG	((volatile uint32_t *)0xbb060000)

/* DCC (DMA controler. Parallel port and FDD use this.) */
struct DCC {
	uint32_t addr;	/* DMA address */
	uint32_t cnt;	/* transfer count */
	uint32_t ctrl;	/* DMA status/command */
	uint32_t drm;
} __attribute__((__packed__));

/* FDD		uPD72065 (80track ready) */
#define	FDC_DMA			((volatile struct DCC *)0xbb030000)
#define	FDC_STATUS		((volatile uint8_t *)0xbb030010)
#define	FDC_DATA		((volatile uint8_t *)0xbb030014)

/* LPT */
#define	LPT_DMA			(((volatile struct DCC *)0xbb040000)
#define	LPT_COUNT		((volatile uint8_t *)0xbb040010)
#define	LPT_STRR		((volatile uint8_t *)0xbb040011)

/* NVSRAM	MK48T08B-15 (word aligned byte access) */
/* 0, 4, 8, c */
#define	NVSRAM_SIGNATURE	0xbb020000
/* 10, 14 18 1c */
#define	NVSRAM_MACHINEID	0xbb020010
#define	NVSRAM_ETHERADDR	((uint8_t *)0xbb021008)
/* 2000, 2004, 2008, 200c */
#define	NVSRAM_CDUMP_ADDR	((uint8_t *)0xbb022000)
#define	NVSRAM_DUMPDEV_1XXX	0xbb022020
#define	NVSRAM_DUMPDEV_2XXX	0xbb022040
/* 2050, 2054, 2058, 205c */
#define	NVSRAM_TF_SCRATCH_ADDR	0xbb022050
#if 0
/* kbd */
#define	NVSRAM_KBD???		0xbb0220a0 /* 0x90 */
#endif
#define	NVSRAM_TF_TESTDATA1	0xbb023000
#define	NVSRAM_TF_TESTDATA2	0xbb023004
#define	NVSRAM_KEYMAP		((uint8_t *)0xbb023014)	/* scratch */
#define	NVSRAM_TF_PROGRESS	((uint8_t *)0xbb02301c)
#define	 NVSRAM_BEV_ROM		32	/* Exception from ROM routine */

#define	NVSRAM_KBDCONNECT	((uint8_t *)0xbb023010)
#define	HAS_KBD()		(*NVSRAM_KBDCONNECT != 255)
#define	NVSRAM_CONSTYPE		((uint8_t *)0xbb023020)
#define	IS_FBCONS()		(*NVSRAM_CONSTYPE == 0)
#define	NVSRAM_GA		0xbb023008
#define	 HAS_GA			0
#define	NVSRAM_TF_RESULT_HI	0xbb023024
#define	NVSRAM_TF_RESULT_LO	0xbb023028
#define	NVSRAM_IPLMODE		((uint8_t *)0xbb02302c)
/*
 *	0: Normal mode
 *	1: ERROR continue mode
 *	2: Details mode
 *	3: LOOP mode
 */
#define	NVSRAM_BOOTDEV		((uint8_t *)0xbb023030)
#define	NVSRAM_BOOTUNIT		((uint8_t *)0xbb023034)

/* V1 is memory area information */
#define	NVSRAM_1STBOOT_ARG_V1_3	((uint8_t *)0xbb023048)	/* 24-31 */
#define	NVSRAM_1STBOOT_ARG_V1_2	((uint8_t *)0xbb02304c)	/* 16-23 */
#define	NVSRAM_1STBOOT_ARG_V1_1	((uint8_t *)0xbb023050)	/* 8 -15 */
#define	NVSRAM_1STBOOT_ARG_V1_0	((uint8_t *)0xbb023054)	/* 0 - 7 */
#define	NVSRAM_1STBOOT_ARG_V0	((uint8_t *)0xbb023058)

#define	NVSRAM_SIMM_3_2		((uint8_t *)0xbb023050)
#define	NVSRAM_SIMM_1_0		((uint8_t *)0xbb023054)
#define	SIMM_16M		0x1
#define	SIMM_32M		0x2

#define	NVSRAM_RTCADDR		((uint8_t *)0xbb027fe0)

/* Graphic adapter */
#include <machine/gareg.h>

/*
 * VME (350/380)
 */
#define	VME_ADDR		0xf8000000
#define	VME_32_ADDR		0xf8000000
#define	VME_32_SIZE		0x07000000
#define	VME_BUFFER_ADDR		0xff000000
#define	VME_BUFFER_SIZE		0x00800000
#define	VME_24_ADDR		0xff800000
#define	VME_24_SIZE		0x007f0000
#define	VME_SHORTIO_ADDR	0xffff0000
#define	VME_SHORTIO_SIZE	0x00010000

#endif /* !_EWS4800MIPS_SBD_TR2_H_ */
